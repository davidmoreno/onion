/*
	Onion HTTP server library
	Copyright (C) 2010 David Moreno Montero

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 3.0 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not see <http://www.gnu.org/licenses/>.
	*/

#include <assert.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>

#include <onion/log.h>
#include <onion/response.h>
#include <onion/block.h>
#include <onion/shortcuts.h>

#include "webdav.h"
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

enum onion_webdav_props_e{
	WD_RESOURCE_TYPE=(1<<0),
	WD_CONTENT_LENGTH=(1<<1),
	WD_LAST_MODIFIED=(1<<2),
	WD_CREATION_DATE=(1<<3),
	WD_ETAG=(1<<4),
	WD_CONTENT_TYPE=(1<<5),
	WD_DISPLAY_NAME=(1<<5),
};

typedef enum onion_webdav_props_e onion_webdav_props;


onion_connection_status onion_webdav_get(const char *path, onion_request *req, onion_response *res);
onion_connection_status onion_webdav_options(const char *path, onion_request *req, onion_response *res);
onion_connection_status onion_webdav_propfind(const char *path, onion_request *req, onion_response *res);

onion_connection_status onion_webdav_handler(const char *path, onion_request *req, onion_response *res){
	onion_response_set_header(res, "Dav", "1,2");
	//onion_response_set_header(res, "Dav", "<http://apache.org/dav/propset/fs/1>");
	onion_response_set_header(res, "MS-Author-Via", "DAV");
	
#ifdef __DEBUG__
	const onion_block *data=onion_request_get_data(req);
	if (data){
		ONION_DEBUG0("Have data! %s", onion_block_data(data));
	}
#endif
	

	switch (onion_request_get_flags(req)&OR_METHODS){
		case OR_GET:
			return onion_webdav_get(path, req, res);
		case OR_OPTIONS:
			return onion_webdav_options(path, req, res);
		case OR_PROPFIND:
			return onion_webdav_propfind(path, req, res);
	}
	
	
	onion_response_write0(res, "<h1>Work in progress...</h1>\n");
	
	return HTTP_OK;
}

onion_connection_status onion_webdav_get(const char *path, onion_request *req, onion_response *res){
	char tmp[512];
	snprintf(tmp, sizeof(tmp), "%s/%s", path, onion_request_get_path(req));
	return onion_shortcut_response_file(tmp, req, res);
}


onion_connection_status onion_webdav_options(const char *path, onion_request *req, onion_response *res){
	onion_response_set_header(res, "Allow", "OPTIONS,GET,HEAD,POST,DELETE,TRACE,PROPFIND,PROPPATCH,COPY,MOVE,LOCK,UNLOCK");
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
int onion_webdav_write_props(xmlTextWriterPtr writer, 
														 const char *realpath, const char *urlpath, const char *filename, 
														 int props){
	ONION_DEBUG("Info for path '%s', urlpath '%s', file '%s'", realpath, urlpath, filename);
	// Stat the thing itself
	char tmp[512];
	if (filename)
		snprintf(tmp, sizeof(tmp), "%s/%s", realpath, filename);
	else
		strncpy(tmp, realpath, sizeof(tmp));
	struct stat st;
	if (stat(tmp, &st)<0){
		ONION_ERROR("Error making stat for %s", tmp);
		return 1;
	}

	if (filename){
		if (urlpath[0]==0)
			snprintf(tmp, sizeof(tmp), "/%s", filename);
		else
			snprintf(tmp, sizeof(tmp), "/%s/%s", urlpath, filename);
	}
	else{
		if (urlpath[0]==0)
			snprintf(tmp, sizeof(tmp), "/");
		else
			snprintf(tmp, sizeof(tmp), "/%s", urlpath);
	}
	
	xmlTextWriterStartElement(writer, BAD_CAST "D:response");
	xmlTextWriterWriteAttribute(writer, BAD_CAST "xmlns:lp1" ,BAD_CAST "DAV:");
	xmlTextWriterWriteAttribute(writer, BAD_CAST "xmlns:g0" ,BAD_CAST "DAV:");
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

onion_block *onion_webdav_write_propfind(const char *realpath, const char *urlpath, int depth, 
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
	xmlTextWriterStartDocument(writer, NULL, "utf-8", NULL);
	xmlTextWriterStartElement(writer, BAD_CAST "D:multistatus");
		xmlTextWriterWriteAttribute(writer, BAD_CAST "xmlns:D" ,BAD_CAST "DAV:");
			onion_webdav_write_props(writer, realpath, urlpath, NULL, props);
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
							onion_webdav_write_props(writer, realpath, urlpath, de->d_name, props);
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

	
	return data;
}

onion_connection_status onion_webdav_propfind(const char *path, onion_request* req, onion_response* res){
	ONION_DEBUG0("PROPFIND");
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
	
	char tmp[512];
	snprintf(tmp, sizeof(tmp), "%s/%s", path, onion_request_get_path(req));
	
	onion_block *block=onion_webdav_write_propfind(tmp, onion_request_get_path(req), depth, props);
	
	ONION_DEBUG0("Printing block %s", onion_block_data(block));
	
	onion_response_set_header(res, "Content-Type", "text/xml; charset=\"utf-8\"");
	onion_response_set_length(res, onion_block_size(block));
	onion_response_set_code(res, 207);
	
	onion_response_write(res, onion_block_data(block), onion_block_size(block));
	
	onion_block_free(block);
	
	return OCS_PROCESSED;
}


void onion_webdav_free(char *d){
	free(d);
	
	xmlCleanupParser();
	xmlMemoryDump();
}

onion_handler *onion_webdav(const char *path){
	xmlInitParser();
	LIBXML_TEST_VERSION

	onion_handler *ret=onion_handler_new((void*)onion_webdav_handler, strdup(path), (void*)onion_webdav_free);
	return ret;
}
