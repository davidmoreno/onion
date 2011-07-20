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

#include "webdav.h"
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

enum onion_webdav_props_e{
	WD_RESOURCE_TYPE=1,
	WD_CONTENT_LENGTH=2,
	WD_LAST_MODIFIED=4,
	WD_CREATION_DATE=8,
};

typedef enum onion_webdav_props_e onion_webdav_props;


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
		case OR_OPTIONS:
			return onion_webdav_options(path, req, res);
		case OR_PROPFIND:
			return onion_webdav_propfind(path, req, res);
	}
	
	
	onion_response_write0(res, "<h1>Work in progress...</h1>\n");
	
	return HTTP_OK;
}

onion_connection_status onion_webdav_options(const char *path, onion_request *req, onion_response *res){
	onion_response_set_header(res, "Allow", "OPTIONS,GET,HEAD,POST,DELETE,TRACE,PROPFIND,PROPPATCH,COPY,MOVE,LOCK,UNLOCK");
	onion_response_set_header(res, "Content-Type", "httpd/unix-directory");
	
	onion_response_set_length(res, 0);
	
	onion_response_write_headers(res);
	
	return OCS_PROCESSED;
}

static int onion_webdav_parse_propfind(const onion_block *block){
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
						if (strcmp((const char*)prop->name, "resourcetype")==0)
							props|=WD_RESOURCE_TYPE;
						else if (strcmp((const char*)prop->name, "getcontentlength")==0)
							props|=WD_CONTENT_LENGTH;
						else if (strcmp((const char*)prop->name, "getlastmodified")==0)
							props|=WD_LAST_MODIFIED;
						else if (strcmp((const char*)prop->name, "creationdate")==0)
							props|=WD_CREATION_DATE;
						else{
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


int onion_webdav_write_props(xmlTextWriterPtr writer, 
														 const char *realpath, const char *urlpath, const char *filename, 
														 int props){
	ONION_DEBUG("Info for path '%s', urlpath '%s', file '%s'", realpath, urlpath, filename);
	char tmp[512];
	snprintf(tmp, sizeof(tmp), "%s/%s", realpath, filename);
	struct stat st;
	if (stat(tmp, &st)<0){
		ONION_ERROR("Error making stat for %s", tmp);
		return 1;
	}
	snprintf(tmp, sizeof(tmp), "%s/%s", urlpath, filename);
	
	xmlTextWriterStartElement(writer, BAD_CAST "D:response");
	xmlTextWriterWriteAttribute(writer, BAD_CAST "xmlns:lp1" ,BAD_CAST "DAV:");
		xmlTextWriterWriteElement(writer, BAD_CAST "D:href", BAD_CAST  tmp); // FIXME, url path.
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
					// FIXME, real dates
					if (props&WD_LAST_MODIFIED)
						xmlTextWriterWriteElement(writer, BAD_CAST "lp1:getlastmodified", BAD_CAST "Mon, 18 Jul 2011 09:36:18 GMT"); 
					// FIXME, real dates
					if (props&WD_CREATION_DATE)
						xmlTextWriterWriteElement(writer, BAD_CAST "lp1:creationdate", BAD_CAST "2011-07-18T09:36:18Z");
					if (props&WD_CONTENT_LENGTH){
						if (!S_ISDIR(st.st_mode)){ // no marker for other resources
							snprintf(tmp, sizeof(tmp), "%d", (int)st.st_size);
							xmlTextWriterWriteElement(writer, BAD_CAST "lp1:getcontentlength", BAD_CAST tmp);
						}
					}
				xmlTextWriterEndElement(writer);
				xmlTextWriterWriteElement(writer, BAD_CAST "D:status", BAD_CAST  "HTTP/1.1 200 OK"); 
			xmlTextWriterEndElement(writer);
	xmlTextWriterEndElement(writer);

	return 0;
}

onion_block *onion_webdav_write_propfind(const char *realpath, const char *urlpath, int depth, int props){
	onion_block *data=onion_block_new();
	xmlTextWriterPtr writer;
	xmlBufferPtr buf;
	buf = xmlBufferCreate();
	if (buf == NULL) {
		printf("testXmlwriterMemory: Error creating the xml buffer\n");
		return data;
	}
	writer = xmlNewTextWriterMemory(buf, 0);
	if (writer == NULL) {
		printf("testXmlwriterMemory: Error creating the xml writer\n");
		return data;
	}
	xmlTextWriterStartDocument(writer, NULL, "utf-8", NULL);
	xmlTextWriterStartElement(writer, BAD_CAST "D:multistatus");
		xmlTextWriterWriteAttribute(writer, BAD_CAST "xmlns:D" ,BAD_CAST "DAV:");
			onion_webdav_write_props(writer, realpath, urlpath, "", props);
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
	
	onion_block *block=onion_webdav_write_propfind(path, onion_request_get_path(req), depth, props);
	
	ONION_DEBUG0("Printing block %s", onion_block_data(block));
	
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
