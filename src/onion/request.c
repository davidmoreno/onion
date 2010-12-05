/*
	Onion HTTP server library
	Copyright (C) 2010 David Moreno Montero

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Affero General Public License as
	published by the Free Software Foundation, either version 3 of the
	License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Affero General Public License for more details.

	You should have received a copy of the GNU Affero General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
	*/

#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>

#include "server.h"
#include "dict.h"
#include "request.h"
#include "response.h"
#include "handler.h"
#include "server.h"
#include "types_internal.h"
#include "codecs.h"
#include "log.h"


static int onion_request_parse_query(onion_request *req);
static onion_connection_status onion_request_write_post_urlencoded(onion_request *req, const char *data, size_t length);
static onion_connection_status onion_request_write_post_multipart(onion_request *req, const char *data, size_t length);
static void onion_request_parse_query_to_dict(onion_dict *dict, char *p);
static onion_connection_status onion_request_parse_multipart_part(onion_request *req, char *init_of_part, size_t length);
static onion_connection_status onion_request_write_post(onion_request *req, const char *data, size_t length);
static onion_connection_status onion_request_write_post_multipart(onion_request *req, const char *data, size_t length);
static onion_connection_status onion_request_write_post_urlencoded(onion_request *req, const char *data, size_t length);
static onion_connection_status onion_request_write_post_multipart_file(onion_request *req, const char *data, size_t length);

/// Used by req->parse_state, states are:
typedef enum parse_state_e{
	CLEAN=0,
	HEADERS=1,
	POST_DATA=2,
	POST_DATA_MULTIPART=3,
	POST_DATA_MULTIPART_FILE=4,
	POST_DATA_URLENCODE=5,
	FINISHED=100,
}parse_state;

/**
 *  @short Creates a request object
 * 
 * @param server onion_server that will be used for writing and some other data
 * @param socket Socket as needed by onion_server write method.
 * @param client_info String that describes the client, for example, the IP address.
 */
onion_request *onion_request_new(onion_server *server, void *socket, const char *client_info){
	onion_request *req;
	req=malloc(sizeof(onion_request));
	memset(req,0,sizeof(onion_request));
	
	req->server=server;
	req->headers=onion_dict_new();
	req->socket=socket;
	req->parse_state=0;
	req->buffer_pos=0;
	req->post_data=NULL;
	if (client_info) // This is kept even on clean
		req->client_info=strdup(client_info);
	else
		req->client_info=NULL;

	return req;
}

/**
 * @short Helper to remove temporal files from req->files
 */
static void unlink_files(const char *key, const char *value, void *p){
	ONION_DEBUG0("Unlinking temporal file %s",value);
	unlink(value);
}

static void onion_request_free_post_data(onion_request_post_data *post){
	if (post->post)
		onion_dict_free(post->post);
	if (post->files){
		onion_dict_preorder(post->files, unlink_files, NULL);
		onion_dict_free(post->files);
	}
	free(post->buffer);
	free(post);
}

/// Deletes a request and all its data
void onion_request_free(onion_request *req){
	onion_dict_free(req->headers);
	
	if (req->fullpath)
		free(req->fullpath);
	if (req->query)
		onion_dict_free(req->query);
	if (req->post_data){
		onion_request_free_post_data(req->post_data);
	}

	if (req->client_info)
		free(req->client_info);
	
	free(req);
}


/// Parses the first query line, GET / HTTP/1.1
static onion_connection_status onion_request_fill_query(onion_request *req, const char *data){
	//ONION_DEBUG("Request: %-64s",data);
	char *method=NULL, *url=NULL, *version=NULL;
	char *ip=strdup(data);
	{ // Parses the query line. Might be a simple sscanf, but too generic and I dont know size beforehand.
		char *p=ip;
		char *lp=p;
		char space=0; // Ignore double spaces
		while(*p){
			if (isspace(*p)){
				if (!space){
					*p='\0';
					ONION_DEBUG("Got token %s",lp);
					if (method==NULL)
						method=lp;
					else if (url==NULL)
						url=lp;
					else if (version==NULL)
						version=lp;
					else{
						free(ip);
						ONION_ERROR("Read unknown token at request. Closing.");
						return OCS_INTERNAL_ERROR;
					}
					lp=p+1;
					space=1;
				}
			}
			else
				space=0;
			p++;
		}
		if (version==NULL && url && method)
						version=lp;
		else if (!version){ // If here url = method = NULL
			free(ip);
			ONION_ERROR("Bad formed request (no %s). Closing.",(!method) ? "method" : (!url) ? "URL" : "version");
			return OCS_INTERNAL_ERROR;
		}
	}

	
	if (strcmp(method,"GET")==0)
		req->flags|=OR_GET;
	else if (strcmp(method,"POST")==0)
		req->flags|=OR_POST;
	else if (strcmp(method,"HEAD")==0)
		req->flags|=OR_HEAD;
	else{
		free(ip);
		return OCS_NOT_IMPLEMENTED; // Not valid method detected.
	}
	if (strcmp(version,"HTTP/1.1")==0)
		req->flags|=OR_HTTP11;

	req->path=strdup(url);
	req->fullpath=req->path;
	onion_request_parse_query(req); // maybe it consumes some CPU and not always needed.. but I need the unquotation.
	
	free(ip);
	return OCS_NEED_MORE_DATA;
}

/// Reads a header and sets it at the request
static onion_connection_status onion_request_fill_header(onion_request *req, const char *data){
	char header[32], value[256];
	sscanf(data, "%31s", header);
	int i=0; 
	const char *p=&data[strlen(header)+1];
	while(*p){
		value[i++]=*p++;
		if (i==sizeof(value)-1){
			break;
		}
	}
	value[i]=0;
	header[strlen(header)-1]='\0'; // removes the :
	onion_dict_add(req->headers, header, value, OD_DUP_ALL);
	return OCS_NEED_MORE_DATA;
}

/**
 * @short Processes one request, calling the handler.
 */
static onion_connection_status onion_request_process(onion_request *req){
	req->parse_state=FINISHED;
	int s=onion_server_handle_request(req);
	return s;
}

/**
 * @short Partially fills a request. One line each time.
 * 
 * @returns onion_request_write_status that sets if there is internal error, keep alive...
 * @see onion_request_write_status
 */
onion_connection_status onion_request_fill(onion_request *req, const char *data){
	ONION_DEBUG0("Request (%d): %s",req->parse_state, data);

	switch(req->parse_state){
	case CLEAN:
		req->parse_state=HEADERS;
		return onion_request_fill_query(req, data);
	case HEADERS:
		if (data[0]=='\0'){
			if (req->flags&OR_POST){
				req->parse_state=POST_DATA;
				return OCS_NEED_MORE_DATA;
			}
			else{
				return onion_request_process(req);
			}
		}
		return onion_request_fill_header(req, data);
	case FINISHED:
		ONION_ERROR("Not accepting more data on this status. Clean the request if you want to start a new one.");
	}
	return OCS_INTERNAL_ERROR;
}


/**
 * @short Write some data into the request, and passes it line by line to onion_request_fill
 *
 * Just reads line by line and passes the data to onion_request_fill. If on POST state, calls onion_request_write_post. 
 * 
 * It generates the query, post and files dictionary as necesary per request. 
 * 
 * The files dict contain the field to filename mapping, where filename is a temporal file where the data is written. At post you
 * have the fieldname -> original filename mapping.
 * 
 * There is no way currently to check the progress of a upload. TODO.
 * 
 * @return Returns the number of bytes writen, or <=0 if connection should close, acording to onion_request_write_status_e.
 * @see onion_request_write_status_e onion_request_write_post
 */
onion_connection_status onion_request_write(onion_request *req, const char *data, size_t length){
	size_t i;
	char msgshown=0;
	
	if (req->parse_state==POST_DATA_MULTIPART ||
			req->parse_state==POST_DATA_MULTIPART_FILE ||
			req->parse_state==POST_DATA_URLENCODE){
			onion_request_post_data *post=req->post_data;
			if (length > (post->size-post->pos)){
				length=(post->size-post->pos);
				ONION_WARNING("Received more data after POST data. Not expected. Closing connection");
				return OCS_CLOSE_CONNECTION;
		}
	}

	switch(req->parse_state){
		case POST_DATA:
			return onion_request_write_post(req, data, length);
		case POST_DATA_MULTIPART:
			return onion_request_write_post_multipart(req, data, length);
		case POST_DATA_MULTIPART_FILE:
			return onion_request_write_post_multipart_file(req, data, length);
		case POST_DATA_URLENCODE:
			return onion_request_write_post_urlencoded(req, data, length);
		default:
			;
	}
	for (i=0;i<length;i++){
		char c=data[i];
		if (c=='\r') // Just skip it
			continue;
		if (c=='\n'){
			req->buffer[req->buffer_pos]='\0';
			int r=onion_request_fill(req,req->buffer);
			req->buffer_pos=0;
			if (r<=0) // Close connection. Might be a rightfull close, or because of an error. Close anyway.
				return r;
			if (req->parse_state==POST_DATA)
				return onion_request_write_post(req, &data[i+1], length-(i+1));
		}
		else{
			req->buffer[req->buffer_pos]=c;
			req->buffer_pos++;
			if (req->buffer_pos>=sizeof(req->buffer)){ // Overflow on line
				req->buffer_pos--;
				if (!msgshown){
					ONION_ERROR("Read data too long for me (max data length %d chars). Ignoring from that byte on to the end of this line. (%16s...)",(int) sizeof(req->buffer),req->buffer);
					if (req->parse_state==CLEAN){
						ONION_ERROR("It happened on the petition line, so i can not deliver it at all.");
						return OCS_INTERNAL_ERROR;
					}
					ONION_ERROR("Increase it at onion_request.h and recompile onion.");
					msgshown=1;
				}
			}
		}
	}
	return OCS_NEED_MORE_DATA;
}

/**
 * @short Parses the query to unquote the path and get the query.
 */
static int onion_request_parse_query(onion_request *req){
	if (!req->path)
		return 0;
	if (req->query) // already done
		return 1;

	char *p=req->path;
	char have_query=0;
	while(*p){
		if (*p=='?'){
			have_query=1;
			break;
		}
		p++;
	}
	*p='\0';
	onion_unquote_inplace(req->path);
	if (have_query){ // There are querys.
		p++;
		req->query=onion_dict_new();
		onion_request_parse_query_to_dict(req->query, p);
	}
	return 1;
}

/**
 * @short Fills the post data.
 * 
 * It calls the proper (multipart/urlencoded) post handler. 
 * 
 * Both fills the post_buffer (created first time). This buffer is not deleted here but at 
 * free of the request as the post dictionary will use it (deconstructed as each part is decoded in place).
 * 
 * Management of such buffer is sligthly diferent on each version.
 * 
 * @see onion_request_write_post_multipart onion_request_write_post_urlencoded
 */
static onion_connection_status onion_request_write_post(onion_request *req, const char *data, size_t length){
	// allocate it
	onion_request_post_data *post=malloc(sizeof(onion_request_post_data));
	memset(post, 0, sizeof(onion_request_post_data));
	req->post_data=post;
	post->post=onion_dict_new();

	
	// check content_length
	const char *content_type=onion_request_get_header(req,"Content-Type"); // only old post method, no multipart.
	if (!content_type || strcasecmp(content_type,"application/x-www-form-urlencoded")==0){
		req->flags|=OR_POST_URLENCODED;
		req->parse_state=POST_DATA_URLENCODE;
	}
	else{
		// maybe ugly but on this state req->buffer is unused, i used it here to store the multipart marker
		const char *p=strstr(content_type, "boundary=");
		if (!p){
			ONION_ERROR("Not indicated the boundary for multipart POST data");
			return OCS_INTERNAL_ERROR;
		}
		p+=9; // skip boundary= data
		int i=0;
		for (i=0;p[i] && p[i]!=';' && p[i]!='\r' && p[i]!='\n';i++);
		
		if (i>sizeof(req->buffer)){
			ONION_ERROR("Request multipart boundary string too large (%d bytes). Overflows the internal storage (%d bytes).",i,sizeof(req->buffer));
			return OCS_INTERNAL_ERROR;
		}
		
		post->marker=malloc(i+1);
		memcpy(post->marker,p, i);
		post->marker[i]='\0';
		post->marker_size=i;
		
		ONION_DEBUG("POST multipart boundary is '%s' (%d bytes)",post->marker,post->marker_size);
		
		req->parse_state=POST_DATA_MULTIPART;
		req->flags|=OR_POST_MULTIPART;
	}
	size_t content_length=0;
	const char *cl=onion_dict_get(req->headers,"Content-Length");
	if (!cl){
		ONION_ERROR("Need Content-Length header when in POST method. Aborting petition.");
		return OCS_INTERNAL_ERROR;
	}
	content_length=atol(cl);
	// Some checks of limits
	if (req->server->max_post_size<content_length){
		if (req->parse_state==POST_DATA_MULTIPART){
			if (req->server->max_post_size+req->server->max_file_size<content_length){
				ONION_ERROR("Requested size for POST data (%d bytes) is more than avaliable for both data and files (%d + %d)",
										(unsigned long)content_length, (unsigned long)req->server->max_post_size, (unsigned long)req->server->max_file_size);
				return OCS_INTERNAL_ERROR;
			}
			else{
				ONION_DEBUG("Total POST data (%d bytes) more than limit (%d bytes). I will read it with the hope that some of it are files.",
									(unsigned long)req->server->max_post_size,(unsigned long)content_length);
				content_length=req->server->max_post_size;
			}
		}
		else{
			ONION_ERROR("Onion POST limit is %ld bytes of data (this have %ld). Increase limit with onion_server_set_max_post_size.",
									(unsigned long)req->server->max_post_size,(unsigned long)content_length);
			return OCS_INTERNAL_ERROR;
		}
	}

	post->size=content_length;
	post->buffer=malloc(content_length+1);

	// read data.
	ONION_DEBUG("Reading a post of %d bytes",content_length);

	if (length > (post->size-post->pos)){
		length=(post->size-post->pos);
		ONION_WARNING("Received more data after POST data. Not expected. Closing connection");
		return OCS_CLOSE_CONNECTION;
	}

	if (req->parse_state==POST_DATA_MULTIPART)
		return onion_request_write_post_multipart(req, data, length);
	else
		return onion_request_write_post_urlencoded(req, data, length);
}

/** 
 * @short Fills the request post on urlencoded petitions
 * 
 * All the buffer is requested at a time, and when fully filled it parses it.
 * 
 */
static onion_connection_status onion_request_write_post_urlencoded(onion_request *req, const char *data, size_t length){
	onion_request_post_data *post=req->post_data;
	memcpy(&post->buffer[post->pos], data, length);
	post->pos+=length;
	
	if (post->pos > post->size){
		ONION_ERROR("Readen more POST data than space available");
		return OCS_INTERNAL_ERROR;
	}
	if (post->pos==post->size){
		post->buffer[post->pos]='\0'; // To ease parsing. Already malloced this byte.
		onion_request_parse_query_to_dict(post->post, post->buffer);
		
		return onion_request_process(req);
	}
	return OCS_NEED_MORE_DATA;
}

/**
 * @short Checks if the part marker is on the latest received data part
 */
static off_t get_marker_pos(onion_request *req, size_t length){
	onion_request_post_data *post=req->post_data;
	int pos=post->pos - length - (post->marker_size+1); // not +2 as then it would have been found before.
	if (pos<0)
		pos=0;
	char *p=&post->buffer[pos];
	char *end=&post->buffer[post->pos - (post->marker_size+2)];
	ONION_DEBUG0("Start looking for marker at %d, end at %d", post->pos-length, post->pos - post->marker_size);
	ONION_DEBUG0("%s",p);
	for (;p<end;p++){
		//ONION_DEBUG0("Looking for marker %c [ %d && %d && %d]",*p,
		//	*p=='-',*(p+1)=='-', memcmp(p+2,post->marker, post->marker_size)==0);
		if (*p=='-' && *(p+1)=='-' && memcmp(p+2,post->marker, post->marker_size)==0){
			ONION_DEBUG("Found marker at %d",(int)(p-post->buffer));
			return (off_t)(p-post->buffer);
		}
	}
	return -1;
}

/** 
 * @short Fills the request post on multipart petitions
 * 
 * The buffer is read from part to part, and then copied or to the post_buffer, or straight to a file.
 * 
 * When all fields are readden, it parses the parts.
 */
static onion_connection_status onion_request_write_post_multipart(onion_request *req, const char *data, size_t length){
	onion_request_post_data *post=req->post_data;
	
	if (post->pos+length>post->size){
		ONION_ERROR("Got more data than expected on POST multipart. Expeted %d bytes, got at least %d",post->size,post->pos+length);
		return OCS_INTERNAL_ERROR;
	}
	
	memcpy(&post->buffer[post->pos], data, length);
	post->pos+=length;
	post->buffer[post->pos]='\0';
	
	off_t marker_pos;
	while ( (marker_pos=get_marker_pos(req, length)) >= 0){
		if (post->last_part_start){
			post->buffer[marker_pos]='\0';
			int r=onion_request_parse_multipart_part(req, &post->buffer[post->last_part_start], marker_pos - post->last_part_start);
			if (r!=OCS_NEED_MORE_DATA)
				return r;
		}
		post->last_part_start=marker_pos+post->marker_size+2;
		length=post->pos - post->last_part_start;
		ONION_DEBUG("Setting length to %d - %d = %d",post->pos, post->last_part_start, length);
	}
	if ((post->pos==post->size) && (length<=4)){
		if (post->buffer[post->pos-1]=='\n')
			post->pos--;
		if (post->buffer[post->pos-1]=='\r')
			post->pos--;
		if (post->buffer[post->pos-1]=='-' &&post->buffer[post->pos-2]=='-'){
			return onion_request_process(req);
		}
		else{
			ONION_ERROR("Error parsing the final POST multipart marker");
			return OCS_INTERNAL_ERROR;
		}
	}
	
	return OCS_NEED_MORE_DATA;
}

static onion_connection_status onion_request_write_post_multipart_file(onion_request *req, const char *data, size_t length){
	return OCS_INTERNAL_ERROR;
}

/**
 * @short Parses a delimited multipart part.
 * 
 * It overwrites as needed the init_of_part buffer, to set the right data at req->post. This dict uses the
 * data at init_of_part so dont use it after deleting init_of_part.
 */
static onion_connection_status onion_request_parse_multipart_part(onion_request *req, char *init_of_part, size_t length){
	ONION_DEBUG("Post parse (%d+%d) %s",(int)(init_of_part-req->post_data->buffer), length, init_of_part);
	// First parse the headers
	onion_dict *headers=onion_dict_new();
	int in_headers=1;
	char *p=init_of_part;
	char *lp=p;
	const char *key=NULL;
	const char *value=NULL;
	int header_length=0;
	//onion_request_post_data *post=req->post_data;
	
	// Skip initial \r\n
	while (*p=='\n' || *p=='\r') p++;
	
	while(in_headers){
		if (*p=='\0'){
			ONION_ERROR("Unexpected end of multipart part headers");
			onion_dict_free(headers);
			return OCS_INTERNAL_ERROR;
		}
		if (!key){
			if (lp==p && ( *p=='\r' || *p=='\n' ) )
				in_headers=0;
			else if (*p==':'){
				value=p+1;
				*p='\0';
				key=lp;
			}
		}
		else{ // value
			if (*p=='\n' || *p=='\r'){
				*p='\0';
				while (*value==' ') value++;
				while (*key==' ' || *key=='\r' || *key=='\n') key++;
				ONION_DEBUG0("Found header <%s> = <%s>",key,value);
				onion_dict_add(headers, key, value, 0);
				key=NULL;
				value=NULL;
				lp=p+1;
			}
		}
		p++;
		header_length++;
	}
	
	// Remove heading and trailing [\r][\n].
	if (*p=='\r') p++;
	if (*p=='\n') p++;
	int len=strlen(p);
	if (*(p+len-1)=='\n'){ *(p+len-1)='\0'; len--; }
	if (*(p+len-1)=='\r') *(p+len-1)='\0';
	ONION_DEBUG0("Data is <%s> (%d bytes)",p, length-header_length);
	
	// Get field name
	char *name=(char*)onion_dict_get(headers, "Content-Disposition");
	if (!name){
		ONION_ERROR("Unnamed POST field");
		return OCS_INTERNAL_ERROR;
	}
	name=strstr(name,"name=")+5;
	
	char *filename=NULL;
	
	if (name[0]=='"'){
		name++;
		char *q=name;
		while (*q!='\0'){
			if (*q=='"'){
				*q='\0';
				break;
			}
			q++;
		}
		q++;
		filename=strstr(q,"filename=");
		if (filename){ // Im in a FILE post.
			filename+=9;
			if (*filename=='"'){
				filename++;
				q=filename;
				while (*q!='\0'){
					if (*q=='"'){
						*q='\0';
						break;
					}
					q++;
				}
			}
		}
	}
	if (filename){
		ONION_DEBUG0("Set FILE data %s=%s, %d bytes",name,filename, length-header_length);
		char tfilename[256];
		const char *tmp=getenv("TEMP");
		if (!tmp)
			tmp="/tmp/";
		
		snprintf(tfilename,sizeof(tfilename),"%s/%s-XXXXXX",tmp,filename);
		int fd=mkstemp(tfilename);
		if (fd<0){
			ONION_ERROR("Could not open temporary file %s",tfilename);
			onion_dict_free(headers);
			return OCS_INTERNAL_ERROR; 
		}
		size_t w=0;
		w+=write(fd,p, length-header_length);
		close(fd);
		
		ONION_DEBUG("Saved temporal FILE data at %s",tfilename);
		if (w!=length-header_length){
			ONION_ERROR("Could not save completly the POST file. Saved %d of %d bytes.",w,length-header_length);
			onion_dict_free(headers);
			return OCS_INTERNAL_ERROR;
		}
		if (!req->post_data->files)
			req->post_data->files=onion_dict_new();
		onion_dict_add(req->post_data->files, filename, tfilename, OD_DUP_VALUE); // Temporal file at files
		onion_dict_add(req->post_data->post, name, filename, 0); // filename at post
	}
	else{
		ONION_DEBUG0("Set POST data %s=%s",name,p);
		onion_dict_add(req->post_data->post, name, p, 0);
	}
	
	onion_dict_free(headers);
	return OCS_NEED_MORE_DATA;
}

/**
 * @short Parses the multipart post
 * 
 * Gets the data from req->post_buffer, and at req->buffer is the marker. Reserves memory for req->post always.
 * 
 * It overwrites the data at req->post_buffer and uses it to fill (and not copy) data as needed, so dont free(req->post) 
 * if you plan to use the POST data.
 * 
 * @return a close error or 0 if everything ok.
 */
/*
static onion_connection_status onion_request_parse_multipart(onion_request *req){
	if (!req->post)
		req->post=onion_dict_new();
	
	ONION_DEBUG("multipart:\n%s",req->post_buffer);
	
	const char *token=req->buffer;
	unsigned int token_length=strlen(token); // token have -- at head, and maybe -- at end.
	char *p=req->post_buffer;
	char *end_p=p + (req->post_buffer_pos);
	char *init_of_part=NULL;
	ONION_DEBUG0("Final POST size without the final token: %ld",(long int)(end_p-p));
	while (p<end_p){ // equal important, as the last token is really the last if everything ok.
		if (*p=='-' && *(p+1)=='-' && memcmp(p+2,token,token_length)==0){ // found token.
			*p='\0';
			ONION_DEBUG0("Found part");
			if (init_of_part){ // Ok, i have it delimited, parse a part
				int r=onion_request_parse_multipart_part(req, init_of_part, (size_t)((p-4)-init_of_part)); // It assumes \r\n, but maybe broken client only \n. FIXME.
				if (r){
					ONION_DEBUG("Return from parse part. Should not be error, but it is (%d)",r);
					return r;
				}
			}
			p+=2+token_length; // skip the token.
			if (*p=='-' && *(p+1)=='-'){
				p+=2;
				ONION_DEBUG("All parsed");
				break;
			}
			while (*p=='\n' || *p=='\r'){ p++; }
			init_of_part=p;
		}
		else
			p++;
	}
	while (*p=='\n' || *p=='\r'){ p++; }
	if (p!=end_p){
		ONION_ERROR("At end of multipart message, found more data (read %d, have %d): %-16s...",(int)(p-req->post_buffer),req->post_buffer_pos,p);
		return OCS_INTERNAL_ERROR;
	}
	
	ONION_DEBUG("Multiparts parsed ok");
	return 0;
}

*/
/**
 * @short Parses the query part to a given dictionary.
 * 
 * The data is overwriten as necessary. It is NOT dupped, so if you free this char *p, please free the tree too.
 */
static void onion_request_parse_query_to_dict(onion_dict *dict, char *p){
	ONION_DEBUG0("Query to dict %s",p);
	char *key, *value;
	int state=0;  // 0 key, 1 value
	key=p;
	while(*p){
		if (state==0){
			if (*p=='='){
				*p='\0';
				value=p+1;
				state=1;
			}
		}
		else{
			if (*p=='&'){
				*p='\0';
				onion_unquote_inplace(key);
				onion_unquote_inplace(value);
				ONION_DEBUG0("Adding key %s=%-16s",key,value);
				onion_dict_add(dict, key, value, 0);
				key=p+1;
				state=0;
			}
		}
		p++;
	}
	if (state!=0){
		onion_unquote_inplace(key);
		onion_unquote_inplace(value);
		ONION_DEBUG0("Adding key %s=%-16s",key,value);
		onion_dict_add(dict, key, value, 0);
	}
}

/// Returns a pointer to the string with the current path. Its a const and should not be trusted for long time.
const char *onion_request_get_path(onion_request *req){
	return req->path;
}

/// Gets the current flags, as in onion_request_flags_e
onion_request_flags onion_request_get_flags(onion_request *req){
	return req->flags;
}

/// Moves the pointer inside fullpath to this new position, relative to current path.
void onion_request_advance_path(onion_request *req, int addtopos){
	req->path=&req->path[addtopos];
}

/// Gets a header data
const char *onion_request_get_header(onion_request *req, const char *header){
	return onion_dict_get(req->headers, header);
}

/// Gets a query data
const char *onion_request_get_query(onion_request *req, const char *query){
	if (req->query)
		return onion_dict_get(req->query, query);
	return NULL;
}

/// Gets a post data
const char *onion_request_get_post(onion_request *req, const char *query){
	if (req->post_data && req->post_data->post)
		return onion_dict_get(req->post_data->post, query);
	return NULL;
}

/// Gets file data
const char *onion_request_get_file(onion_request *req, const char *query){
	if (req->post_data && req->post_data->files)
		return onion_dict_get(req->post_data->files, query);
	return NULL;
}

/// Gets the header header data dict
const onion_dict *onion_request_get_header_dict(onion_request *req){
	return req->headers;
}

/// Gets request query dict
const onion_dict *onion_request_get_query_dict(onion_request *req){
	return req->query;
}

/// Gets post data dict
const onion_dict *onion_request_get_post_dict(onion_request *req){
	if (req->post_data)
		return req->post_data->post;
	return NULL;
}

/// Gets files data dict
const onion_dict *onion_request_get_file_dict(onion_request *req){
	if (req->post_data)
		return req->post_data->files;
	return NULL;
}


/**
 * @short Cleans a request object to reuse it.
 */
void onion_request_clean(onion_request* req){
	onion_dict_free(req->headers);
	req->headers=onion_dict_new();
	req->parse_state=0;
	req->flags&=0xFF00;
	if (req->fullpath){
		free(req->fullpath);
		req->path=req->fullpath=NULL;
	}
	if (req->query){
		onion_dict_free(req->query);
		req->query=NULL;
	}
	if (req->post_data){
		onion_request_free_post_data(req->post_data);
		req->post_data=NULL;
	}
}

/**
 * @short Forces the request to process only one request, not doing the keep alive.
 * 
 * This is usefull on non threaded modes, as the keep alive blocks the loop.
 */
void onion_request_set_no_keep_alive(onion_request *req){
	req->flags|=OR_NO_KEEP_ALIVE;
	ONION_DEBUG("Disabling keep alive %X",req->flags);
}

/**
 * @short Returns if current request wants to keep alive.
 * 
 * It is a complex set of circumstances: HTTP/1.1 and no connection: close, or HTTP/1.0 and connection: keep-alive
 * and no explicit set that no keep alive.
 */
int onion_request_keep_alive(onion_request *req){
	if (req->flags&OR_NO_KEEP_ALIVE)
		return 0;
	if (req->flags&OR_HTTP11){
		const char *connection=onion_request_get_header(req,"Connection");
		if (!connection || strcasecmp(connection,"Close")!=0) // Other side wants keep alive
			return 1;
	}
	else{ // HTTP/1.0
		const char *connection=onion_request_get_header(req,"Connection");
		if (connection && strcasecmp(connection,"Keep-Alive")==0) // Other side wants keep alive
			return 1;
	}
	return 0;
}
