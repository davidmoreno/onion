/*
	Onion HTTP server library
	Copyright (C) 2010-2016 David Moreno Montero and others

	This library is free software; you can redistribute it and/or
	modify it under the terms of, at your choice:
	
	a. the Apache License Version 2.0. 
	
	b. the GNU General Public License as published by the 
		Free Software Foundation; either version 2.0 of the License, 
		or (at your option) any later version.
	 
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of both licenses, if not see
	<http://www.gnu.org/licenses/> and 
	<http://www.apache.org/licenses/LICENSE-2.0>.
	*/

#include <assert.h>
#include <errno.h>

#include <libxml/xmlmemory.h>
#include <libxml/xmlwriter.h>

#include <onion/log.h>
#include <onion/response.h>
#include <onion/block.h>
#include <onion/shortcuts.h>
#include <onion/low.h>

#include "webdav.h"
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <libgen.h>

struct onion_webdav_t{
	char *path;
	onion_webdav_permissions_check check_permissions;
};

typedef struct onion_webdav_t onion_webdav;

/**
 * @short Flags of props as queried
 */
enum onion_webdav_props_e{
	WD_RESOURCE_TYPE=(1<<0),
	WD_CONTENT_LENGTH=(1<<1),
	WD_LAST_MODIFIED=(1<<2),
	WD_CREATION_DATE=(1<<3),
	WD_ETAG=(1<<4),
	WD_CONTENT_TYPE=(1<<5),
	WD_DISPLAY_NAME=(1<<6),
	WD_EXECUTABLE=(1<<7),
};

typedef enum onion_webdav_props_e onion_webdav_props;


onion_connection_status onion_webdav_get(const char *filename, onion_webdav *wd, onion_request *req, onion_response *res);
onion_connection_status onion_webdav_put(const char *filename, onion_webdav *wd, onion_request *req, onion_response *res);
onion_connection_status onion_webdav_mkcol(const char *filename, onion_webdav *wd, onion_request *req, onion_response *res);
onion_connection_status onion_webdav_move(const char *filename, onion_webdav *wd, onion_request *req, onion_response *res);
onion_connection_status onion_webdav_delete(const char *filename, onion_webdav *wd, onion_request *req, onion_response *res);
onion_connection_status onion_webdav_options(const char *filename, onion_webdav *wdpath, onion_request *req, onion_response *res);
onion_connection_status onion_webdav_propfind(const char *filename, onion_webdav *wd, onion_request *req, onion_response *res);
onion_connection_status onion_webdav_proppatch(const char *filename, onion_webdav *wd, onion_request *req, onion_response *res);

/**
 * @short Main webdav handler, just redirects to the proper handler depending on headers and method
 */
onion_connection_status onion_webdav_handler(onion_webdav *wd, onion_request *req, onion_response *res){
	onion_response_set_header(res, "Dav", "1,2");
	onion_response_set_header(res, "MS-Author-Via", "DAV");
	
#ifdef __DEBUG__
	const onion_block *data=onion_request_get_data(req);
	if (data){
		ONION_DEBUG0("Have data!\n %s", onion_block_data(data));
	}
#endif

	
	char filename[512];
	snprintf(filename, sizeof(filename), "%s/%s", wd->path, onion_request_get_path(req));
	
	ONION_DEBUG("Check %s and %s", wd->path, filename);
	if (wd->check_permissions(wd->path, filename, req)!=0){
		return onion_shortcut_response("Forbidden", HTTP_FORBIDDEN, req, res);
	}

	
	switch (onion_request_get_flags(req)&OR_METHODS){
		case OR_GET:
		case OR_HEAD:
			return onion_webdav_get(filename, wd, req, res);
		case OR_OPTIONS:
			return onion_webdav_options(filename, wd, req, res);
		case OR_PROPFIND:
			return onion_webdav_propfind(filename, wd, req, res);
		case OR_PUT:
			return onion_webdav_put(filename, wd, req, res);
		case OR_DELETE:
			return onion_webdav_delete(filename, wd, req, res);
		case OR_MOVE:
			return onion_webdav_move(filename, wd, req, res);
		case OR_MKCOL:
			return onion_webdav_mkcol(filename, wd, req, res);
		case OR_PROPPATCH:
			return onion_webdav_proppatch(filename, wd, req, res);
	}
	
	onion_response_set_code(res, HTTP_NOT_IMPLEMENTED);
	onion_response_write0(res, "<h1>Work in progress...</h1>\n");
	
	return OCS_PROCESSED;
}

/**
 * @short Simple get on webdav is just a simple get on a default path.
 */
onion_connection_status onion_webdav_get(const char *filename, onion_webdav *wd, onion_request *req, onion_response *res){
	ONION_DEBUG("Webdav gets %s", filename);
	return onion_shortcut_response_file(filename, req, res);
}

/**
 * @short Deletes a resource
 */
onion_connection_status onion_webdav_delete(const char *filename, onion_webdav *wd, onion_request *req, onion_response *res){
	ONION_DEBUG("Webdav delete %s", filename);
	int error=remove(filename);
	if (error==0)
		return onion_shortcut_response("Deleted", HTTP_OK, req, res);
	else{
		ONION_ERROR("Could not remove WebDAV resource");
		return onion_shortcut_response("Could not delete resource", HTTP_INTERNAL_ERROR, req, res);
	}
}

/**
 * @short Moves a resource
 */
onion_connection_status onion_webdav_move(const char *filename, onion_webdav *wd, onion_request *req, onion_response *res){
	const char *dest=onion_request_get_header(req,"Destination");
	if (!dest)
		return OCS_INTERNAL_ERROR;
	const char *dest_orig=dest;
	// Skip the http... part. Just 3 /.
	int i;
	for (i=0;i<3;i+=(*dest++=='/'))
		if (*dest==0)
			return OCS_INTERNAL_ERROR;
	dest--;
	
	const char *fullpath=onion_request_get_fullpath(req);
	const char *partialpath=onion_request_get_path(req);
	// Not the fixed URL part for this handler.
	int fpl=strlen(fullpath); // Full path length
	int ppl=strlen(onion_request_get_path(req)); // Partial, the fullpath[fpl-ppl] is the end point of the handler path
	if (strncmp(fullpath, dest, fpl-ppl)!=0){
		char tmp[512];
		int l=fpl-ppl < sizeof(tmp)-1 ? fpl-ppl : sizeof(tmp)-1;
		strncpy(tmp, fullpath, l);
		tmp[l]=0;
		ONION_WARNING("Move to out of this webdav share! (%s is out of %s)", dest, tmp);
		return onion_shortcut_response("Moving out of shared share", HTTP_FORBIDDEN, req, res);
	}
	dest=&dest[fpl-ppl];


	char orig[512];
	snprintf(orig, sizeof(orig), "%s/%s", wd->path, partialpath);
	
	if (wd->check_permissions(wd->path, orig, req)!=0){
		return onion_shortcut_response("Forbidden", HTTP_FORBIDDEN, req, res);
	}

	const char *fdest=filename;

	ONION_INFO("Move %s to %s (webdav)", fullpath, dest_orig);
	
	int ok=onion_shortcut_rename(orig, fdest);

	if (ok==0){
		ONION_DEBUG("Created %s succesfully", fdest);
		return onion_shortcut_response("201 Created", 201, req, res);
	}
	else{
		ONION_ERROR("Could not rename %s to %s (%s)", orig, fdest, strerror(errno));
		return onion_shortcut_response("Could not create resource", HTTP_FORBIDDEN, req, res);
	}
}

/**
 * @short Creates a collection / directory.
 * 
 * Spec says it must create only if the parent exists.
 */
onion_connection_status onion_webdav_mkcol(const char *filename, onion_webdav *wd, onion_request *req, onion_response *res){
	if (mkdir(filename,0777)!=0){
		return onion_shortcut_response("403 Forbidden", HTTP_FORBIDDEN, req, res);
	}
	return onion_shortcut_response("201 Created", 201, req, res);
}


/**
 * @short Simple put on webdav is just move a file from tmp to the final destination (or copy if could not move).
 * 
 */
onion_connection_status onion_webdav_put(const char *filename, onion_webdav *wd, onion_request *req, onion_response *res){
	ONION_DEBUG("Webdav puts %s", filename);
	
	const char *tmpfile=onion_block_data(onion_request_get_data(req));
	
	int ok=onion_shortcut_rename(tmpfile, filename);
	
	if (ok==0){
		ONION_DEBUG("Created %s succesfully", filename);
		return onion_shortcut_response("201 Created", 201, req, res);
	}
	else{
		ONION_ERROR("Could not rename %s to %s (%s)", tmpfile, filename, strerror(errno));
		return onion_shortcut_response("Could not create resource", HTTP_FORBIDDEN, req, res);
	}
}

/**
 * @short Returns known options.
 * 
 * Just known options, no more. I think many clients ignore this. (without PUT, gnome's file manager was trying).
 */
onion_connection_status onion_webdav_options(const char *filename, onion_webdav *wd, onion_request *req, onion_response *res){
	onion_response_set_header(res, "Allow", "OPTIONS,GET,HEAD,PUT,PROPFIND");
	onion_response_set_header(res, "Content-Type", "httpd/unix-directory");
	
	onion_response_set_length(res, 0);
	
	onion_response_write_headers(res);
	
	return OCS_PROCESSED;
}

/**
 * @short Returns the set of props for this query
 * 
 * The block contains the propfind xml, and it returns the mask of props to show.
 * 
 */
static int onion_webdav_parse_propfind(const onion_block *block){
	// For parsing the data
	xmlDocPtr doc;
	doc = xmlParseMemory((char*)onion_block_data(block), onion_block_size(block));
	
	if (doc == NULL) {
		ONION_ERROR("Error: unable to parse OPTIONS");
		return OCS_INTERNAL_ERROR;
	}
	
	int props=0;
	xmlNode *root = NULL;
	root = xmlDocGetRootElement(doc);
	while (root){
		if (strcmp((const char*)root->name,"propfind")==0){
			xmlNode *propfind = root->children;
			while (propfind){
				if (strcmp((const char*)propfind->name,"prop")==0){
					xmlNode *prop = propfind->children;
					while (prop){
						if (strcmp((const char*)prop->name, "text")==0) // ignore
							;
						else if (strcmp((const char*)prop->name, "resourcetype")==0)
							props|=WD_RESOURCE_TYPE;
						else if (strcmp((const char*)prop->name, "getcontentlength")==0)
							props|=WD_CONTENT_LENGTH;
						else if (strcmp((const char*)prop->name, "getlastmodified")==0)
							props|=WD_LAST_MODIFIED;
						else if (strcmp((const char*)prop->name, "creationdate")==0)
							props|=WD_CREATION_DATE;
						else if (strcmp((const char*)prop->name, "getetag")==0)
							props|=WD_ETAG;
						else if (strcmp((const char*)prop->name, "getcontenttype")==0)
							props|=WD_CONTENT_TYPE;
						else if (strcmp((const char*)prop->name, "displayname")==0)
							props|=WD_DISPLAY_NAME;
						else if (strcmp((const char*)prop->name, "executable")==0)
							props|=WD_EXECUTABLE;
						else{
							char tmp[256];
							snprintf(tmp,sizeof(tmp),"g0:%s", prop->name);
							ONION_DEBUG("Unknown requested property with tag %s", prop->name);
						}
						
						prop=prop->next;
					}
					
				}
				propfind=propfind->next;
			}
		}
		root=root->next;
	}
	xmlFreeDoc(doc); 
	
	return props;
}


/**
 * @short Write the properties of a path.
 * 
 * Given a path, and optionally a file at that path, writes the XML of its properties.
 * 
 * If no file given, data is for that path, else it is for the path+filename.
 * 
 * @param writer XML writer to write the data to
 * @param realpath The real path on the file server, used to check permissions and read data TODO
 * @param urlpath The base URL of the element, needed in the response. Composed with filename if filename!=NULL.
 * @param filename NULL if that element itself, else the subelement (file in path).
 * @param props Bitmask of the properties the user asked for.
 * 
 * @return 0 is ok, 1 if could not stat file.
 */
int onion_webdav_write_props(xmlTextWriterPtr writer, const char *basepath, 
														 const char *realpath, const char *urlpath, const char *filename, 
														 int props){
	ONION_DEBUG("Info for path '%s', urlpath '%s', file '%s', basepath '%s'", realpath, urlpath, filename, basepath);
	// Stat the thing itself
	char tmp[PATH_MAX];
	if (filename)
		snprintf(tmp, sizeof(tmp), "%s/%s", realpath, filename);
	else{
		if (strlen(realpath)>=sizeof(tmp)-1){
			ONION_ERROR("File path too long");
			return 1;
		}
		strncpy(tmp, realpath, sizeof(tmp)-1);
	}
	struct stat st;
	if (stat(tmp, &st)<0){
		ONION_ERROR("Error on %s: %s", tmp, strerror(errno));
		return 1;
	}
	while (*urlpath=='/') // No / at the begining.
		urlpath++;

	if (filename){
		if (urlpath[0]==0)
			snprintf(tmp, sizeof(tmp), "%s/%s", basepath, filename);
		else
			snprintf(tmp, sizeof(tmp), "%s/%s/%s", basepath, urlpath, filename);
	}
	else{
		if (urlpath[0]==0)
			snprintf(tmp, sizeof(tmp), "%s/", basepath);
		else{
			snprintf(tmp, sizeof(tmp), "%s/%s", basepath, urlpath);
		}
	}
	if (S_ISDIR(st.st_mode) && urlpath[0]!=0)
		strncat(tmp,"/", sizeof(tmp) - 1);
	ONION_DEBUG0("Props for %s", tmp);
	
	xmlTextWriterStartElement(writer, BAD_CAST "D:response");
	xmlTextWriterWriteAttribute(writer, BAD_CAST "xmlns:lp1" ,BAD_CAST "DAV:");
	xmlTextWriterWriteAttribute(writer, BAD_CAST "xmlns:g0" ,BAD_CAST "DAV:");
	xmlTextWriterWriteAttribute(writer, BAD_CAST "xmlns:a" ,BAD_CAST "http://apache.org/dav/props/");
	
		xmlTextWriterWriteElement(writer, BAD_CAST "D:href", BAD_CAST  tmp); 
		
		/// OK
		xmlTextWriterStartElement(writer, BAD_CAST "D:propstat");
			xmlTextWriterStartElement(writer, BAD_CAST "D:prop");
				if (props&WD_RESOURCE_TYPE){
					xmlTextWriterStartElement(writer, BAD_CAST "lp1:resourcetype");
						if (S_ISDIR(st.st_mode)){ // no marker for other resources
							xmlTextWriterStartElement(writer, BAD_CAST "D:collection");
							xmlTextWriterEndElement(writer);
						}
					xmlTextWriterEndElement(writer);
				}
				if (props&WD_LAST_MODIFIED){
					char time[32];
					onion_shortcut_date_string(st.st_mtime, time);
					xmlTextWriterWriteElement(writer, BAD_CAST "lp1:getlastmodified", BAD_CAST time); 
				}
				if (props&WD_CREATION_DATE){
					char time[32];
					onion_shortcut_date_string_iso(st.st_mtime, time);
					xmlTextWriterWriteElement(writer, BAD_CAST "lp1:creationdate", BAD_CAST time );
				}
				if (props&WD_CONTENT_LENGTH && !S_ISDIR(st.st_mode)){
					snprintf(tmp, sizeof(tmp), "%d", (int)st.st_size);
					xmlTextWriterWriteElement(writer, BAD_CAST "lp1:getcontentlength", BAD_CAST tmp);
				}
				if (props&WD_CONTENT_TYPE && S_ISDIR(st.st_mode)){
					xmlTextWriterWriteElement(writer, BAD_CAST "lp1:getcontenttype", BAD_CAST "httpd/unix-directory");
				}
				if (props&WD_ETAG){
					char etag[32];
					onion_shortcut_etag(&st, etag);
					xmlTextWriterWriteElement(writer, BAD_CAST "lp1:getetag", BAD_CAST etag);
				}
				if (props&WD_EXECUTABLE && !S_ISDIR(st.st_mode)){
					if (st.st_mode&0111)
						xmlTextWriterWriteElement(writer, BAD_CAST "a:executable", BAD_CAST "true");
					else
						xmlTextWriterWriteElement(writer, BAD_CAST "a:executable", BAD_CAST "false");
				}

			xmlTextWriterEndElement(writer);
			xmlTextWriterWriteElement(writer, BAD_CAST "D:status", BAD_CAST  "HTTP/1.1 200 OK"); 
		xmlTextWriterEndElement(writer); // /propstat

		/// NOT FOUND
		xmlTextWriterStartElement(writer, BAD_CAST "D:propstat");
			xmlTextWriterStartElement(writer, BAD_CAST "D:prop");
				if (props&WD_CONTENT_LENGTH && S_ISDIR(st.st_mode)){
					snprintf(tmp, sizeof(tmp), "%d", (int)st.st_size);
					xmlTextWriterWriteElement(writer, BAD_CAST "g0:getcontentlength", BAD_CAST "");
				}
				if (props&WD_CONTENT_TYPE && !S_ISDIR(st.st_mode)){
					xmlTextWriterWriteElement(writer, BAD_CAST "g0:getcontenttype", BAD_CAST "");
				}
				if (props&WD_DISPLAY_NAME){
					xmlTextWriterWriteElement(writer, BAD_CAST "g0:displayname", BAD_CAST "");
				}
			xmlTextWriterEndElement(writer);
			xmlTextWriterWriteElement(writer, BAD_CAST "D:status", BAD_CAST  "HTTP/1.1 404 Not Found"); 
		xmlTextWriterEndElement(writer); // /propstat
		
	xmlTextWriterEndElement(writer);
	
	return 0;
}

/**
 * @short Prepares the response for propfinds
 * 
 * @param realpath Shared folder
 * @param urlpath URL of the requested propfind
 * @param depth Depth of query, 0 or 1.
 * @param props Properties of the query
 * 
 * @returns An onion_block with the XML data.
 */
onion_block *onion_webdav_write_propfind(const char *basepath, const char *realpath, const char *urlpath, int depth, 
																				 int props){
	onion_block *data=onion_block_new();
	xmlTextWriterPtr writer;
	xmlBufferPtr buf;
	buf = xmlBufferCreate();
	if (buf == NULL) {
		ONION_ERROR("testXmlwriterMemory: Error creating the xml buffer");
		return data;
	}
	writer = xmlNewTextWriterMemory(buf, 0);
	if (writer == NULL) {
		ONION_ERROR("testXmlwriterMemory: Error creating the xml writer");
		return data;
	}
	int error;
	xmlTextWriterStartDocument(writer, NULL, "utf-8", NULL);
	xmlTextWriterStartElement(writer, BAD_CAST "D:multistatus");
		xmlTextWriterWriteAttribute(writer, BAD_CAST "xmlns:D" ,BAD_CAST "DAV:");
			error=onion_webdav_write_props(writer, basepath, realpath, urlpath, NULL, props);
			if (depth>0){
				ONION_DEBUG("Get also all files");
				DIR *dir=opendir(realpath);
				if (!dir){
					ONION_ERROR("Error opening dir %s to check files on it", realpath);
				}
				else{
					struct dirent *de;
					while ( (de=readdir(dir)) ){
						if (de->d_name[0]!='.')
							onion_webdav_write_props(writer, basepath, realpath, urlpath, de->d_name, props);
					}
					closedir(dir);
				}
			}
		xmlTextWriterEndElement(writer);
	xmlTextWriterEndElement(writer);
	
	
	xmlTextWriterEndDocument(writer);
	xmlFreeTextWriter(writer);
	
	onion_block_add_str(data, (const char*)buf->content);
	xmlBufferFree(buf);

	if (error){
		onion_block_free(data);
		return NULL;
	}
	return data;
}

/**
 * @short Handles a propfind
 * 
 * @param path the shared path.
 */
onion_connection_status onion_webdav_propfind(const char *filename, onion_webdav *wd, onion_request* req, onion_response* res){
	// Prepare the basepath, necesary for props.
	char *basepath=NULL;
	int pathlen=0;
	const char *current_path=onion_request_get_path(req);
	const char *fullpath=onion_request_get_fullpath(req);
	pathlen=(current_path-fullpath);
	basepath=alloca(pathlen+1);
	memcpy(basepath, fullpath, pathlen+1);
	ONION_DEBUG0("Pathbase initial <%s> %d", basepath, pathlen); 
	while(basepath[pathlen]=='/' && pathlen>0)
		pathlen--;
	basepath[pathlen+1]=0;
				 
	ONION_DEBUG0("PROPFIND; pathbase %s", basepath);
	int depth;
	{
		const char *depths=onion_request_get_header(req, "Depth");
		if (!depths){
			ONION_ERROR("Missing Depth header on webdav request");
			return OCS_INTERNAL_ERROR;
		}
		if (strcmp(depths,"infinity")==0){
			ONION_ERROR("Infinity depth not supported yet.");
			return OCS_INTERNAL_ERROR;
		}
		depth=atoi(depths);
	}

	int props=onion_webdav_parse_propfind(onion_request_get_data(req));
	ONION_DEBUG("Asking for props %08X, depth %d", props, depth);
	
	onion_block *block=onion_webdav_write_propfind(basepath, filename, onion_request_get_path(req), depth, props);
	
	if (!block) // No block, resource does not exist
		return onion_shortcut_response("Not found", HTTP_NOT_FOUND, req, res);
		
	ONION_DEBUG0("Printing block %s", onion_block_data(block));
	
	onion_response_set_header(res, "Content-Type", "text/xml; charset=\"utf-8\"");
	onion_response_set_length(res, onion_block_size(block));
	onion_response_set_code(res, HTTP_MULTI_STATUS);
	onion_response_write_headers(res);
	onion_response_flush(res);
	
	onion_response_write(res, onion_block_data(block), onion_block_size(block));
	
	onion_block_free(block);
	
	return OCS_PROCESSED;
}

/**
 * @short Allows to change some properties of the file
 */
onion_connection_status onion_webdav_proppatch(const char *filename, onion_webdav *wd, onion_request *req, onion_response *res){
	xmlDocPtr doc;
	const onion_block *block=onion_request_get_data(req);
// 	ONION_DEBUG("%s",onion_block_data(block));
	if (!block)
		return OCS_INTERNAL_ERROR;
	doc = xmlParseMemory((char*)onion_block_data(block), onion_block_size(block));
	
	xmlNode *root = NULL;
	root = xmlDocGetRootElement(doc);
	int ok=0;
	
	while (root){
		ONION_DEBUG("%s", root->name);
		if (strcmp((const char*)root->name,"propertyupdate")==0){
			xmlNode *propertyupdate = root->children;
			while (propertyupdate){
				ONION_DEBUG("%s", propertyupdate->name);
				if (strcmp((const char*)propertyupdate->name,"set")==0){
					xmlNode *set = propertyupdate->children;
					while (set){
						ONION_DEBUG("%s", set->name);
						if (strcmp((const char*)set->name,"prop")==0){
							ONION_DEBUG("in prop");
							xmlNode *prop = set->children;
							while (prop){
								ONION_DEBUG("prop %s", prop->name);
								if (strcmp((const char*)prop->name,"executable")==0){
									ONION_DEBUG("Setting executable %s", prop->children->content);
									struct stat st;
									stat(filename, &st);
									if (prop->children->content[0] =='T' || prop->children->content[0] == 't'){
										chmod(filename, st.st_mode | S_IXUSR | S_IXGRP | S_IXOTH);
									}
									else{
										chmod(filename, st.st_mode & ~( S_IXUSR | S_IXGRP | S_IXOTH) );
									}
									ok=1;
								}
								prop=prop->next;
							}
						}
						set=set->next;
					}
				}
				propertyupdate=propertyupdate->next;
			}
		}
		root=root->next;
	}
	
	xmlFreeDoc(doc); 
	if (ok){
		onion_response_write_headers(res);
		return OCS_PROCESSED;
	}
	else{
		return OCS_INTERNAL_ERROR;
	}
}


/**
 * @short Frees the webdav data
 */
void onion_webdav_free(onion_webdav *wd){
	onion_low_free(wd->path);
	onion_low_free(wd);
	
	xmlCleanupParser();
	xmlMemoryDump();
}

/**
 * @short Default permission checker
 * 
 * It checks the given path complies with the exported area.
 */ 
int onion_webdav_default_check_permissions(const char *exported_path, const char *filename, onion_request *req){
	if (strstr(filename,"/../")){
		ONION_ERROR("Trying to escape! %s is trying to escape from %s", filename, exported_path);
		return 1;
	}
	
	ONION_DEBUG0("Checking permissions for path %s, file %s", exported_path, filename);
	char *base, *file;
	int ret=0;
	
	base=realpath(exported_path, NULL);
	if (!base){
		ONION_ERROR("Base directory does not exist.");
		return 1;
	}
	
	file=realpath(filename, NULL);
	if (!file){ // Maybe it reffers to a non existent file, so we need the parent permissions
		char *fname=alloca(strlen(filename)+1);
		strcpy(fname, filename);
		file=realpath(dirname(fname), NULL);
	}
	if (!base || !file){
		ret=1;
		ONION_ERROR("Could not resolve real path for %s or %s", exported_path, filename);
	}
	if ((ret==0) && strncmp(base, file, strlen(base))!=0){
		ret=1;
		ONION_ERROR("Base %s is not for file %s (%p)", base, file, strncmp(base, file, strlen(base)));
	}
	
	if (base)
		onion_low_free(base); 
	if (file)
		onion_low_free(file);
	ONION_DEBUG0("Permission %s", ret==0 ? "granted" : "denied");
	return ret;
}

/**
 * @short Creates a webdav handler
 * 
 * The check_permissions parameter, if set, sets a custom security permission checker.
 * 
 * If not set, the default permissions apply, that will try to do not access files out of the restricted area.
 * 
 * This permission checker gets the exported path, the path to the file that wants to be exported/checked/moved.., 
 * and the request as it arrived to the handler.
 * 
 * The exported path and file path might be relative, if onion_handler_webdav was initialized so.
 * 
 * On move it will check for both the original and final files, on other methods, it will check just the file, and
 * the semantics is that the user is allowed to do that method.
 * 
 * It should return 0 on success, any other if error/not allowed.
 * 
 * @param path Path to share
 * @returns The onion handler.
 */
onion_handler *onion_handler_webdav(const char *path, onion_webdav_permissions_check perm){
	onion_webdav *wd=onion_low_malloc(sizeof(onion_webdav));
	if (!wd)
		return NULL;
	
	xmlInitParser();
	LIBXML_TEST_VERSION

	wd->path=onion_low_strdup(path);
	
	if (perm)
		wd->check_permissions=perm;
	else
		wd->check_permissions=onion_webdav_default_check_permissions;

	onion_handler *ret=onion_handler_new((void*)onion_webdav_handler, wd, (void*)onion_webdav_free);
	return ret;
}
