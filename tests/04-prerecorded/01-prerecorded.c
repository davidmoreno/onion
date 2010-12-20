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

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <malloc.h>

#include <onion/server.h>
#include <onion/request.h>
#include <onion/response.h>
#include <onion/handler.h>
#include <onion/log.h>

#include "../test.h"
#include <regex.h>

onion_server *server;

/**
 * @{ @name Buffer type. 
 * 
 * Its just a in memory buffer properly handled
 */
typedef struct{
	char *data;
	size_t size;
	off_t pos;
}buffer;

/// Just appends to the handler. Must be big enought or segfault.. Just for tests.
int buffer_append(buffer *handler, const char *data, unsigned int length){
	ONION_DEBUG0("Appending %d bytes",length);
	int l=length;
	if (handler->pos+length>handler->size){
		l=handler->size-handler->pos;
	}
	memcpy(handler->data+handler->pos,data,l);
	handler->pos+=l;
	return l;
}

buffer *buffer_new(size_t size){
	buffer *b=malloc(sizeof(buffer));
	b->data=malloc(size);
	b->pos=0;
	b->size=size;
	return b;
}

void buffer_free(buffer *b){
	free(b->data);
	free(b);
}
/// @}

/**
 * @short Handler that just echoes all data, writing what was a header, what the method...
 */
onion_connection_status allinfo_handler(void *data, onion_request *req){
	onion_response *res=onion_response_new(req);
	onion_response_write_headers(res);
	
	int f=onion_request_get_flags(req);
	onion_response_printf(res, "Method: %s\n",(f&OR_GET) ? "GET" : (f%OR_HEAD) ? "HEAD" : "POST");
	onion_response_printf(res, "Path: %s\n",onion_request_get_path(req));
	
	return onion_response_free(res);;
}

/**
 * @short Opens a script file, and executes it.
 */
void prerecorded(const char *script){
	INIT_LOCAL();
	FILE *fd=fopen(script, "r");
	if (!fd){
		FAIL("Could not open script file");
		END_LOCAL();
		return;
	}
	
	buffer *buffer=buffer_new(1024*1024);
	onion_request *req=onion_request_new(server, buffer, "test");
	
	ssize_t r;
	char *line=malloc(1024);
	size_t len=sizeof(line);
	onion_connection_status ret=0;
	while (!feof(fd)){
		// Read request
		while ( (r=getline(&line, &len, fd)) != -1 ){
			if (strcmp(line,"-- --\n")==0){
				break;
			}
			ONION_DEBUG0("Write: %s",line);
			ret=onion_request_write(req, line, len-1);
			len=1024;
		}
		if (r==0){
			FAIL_IF("Found end of file before end of request");
			fclose(fd);
			END_LOCAL();
			return;
		}
		
		// Check response
		buffer->data[buffer->pos]='\0';
		ONION_DEBUG0("Response: %s",buffer->data);

		while ( (r=getline(&line, &len, fd)) != -1 ){
			if (strcmp(line,"++ ++\n")==0){
				break;
			}
			line[strlen(line)-1]='\0';
			if (strcmp(line,"INTERNAL_ERROR")==0){ // Checks its an internal error
				ONION_DEBUG("Check INTERNAL_ERROR");
				ONION_DEBUG0("Returned %d",ret);
				FAIL_IF_NOT_EQUAL(ret, OCS_INTERNAL_ERROR);
			}
			else if (strcmp(line,"NOT_IMPLEMENTED")==0){ // Checks its an internal error
				ONION_DEBUG("Check NOT_IMPLEMENTED");
				ONION_DEBUG0("Returned %d",ret);
				FAIL_IF_NOT_EQUAL(ret, OCS_NOT_IMPLEMENTED);
			}
			else{
				regex_t re;
				regmatch_t match[1];
				
				//int l=strlen(line)-1;
				//memmove(line+1,line,l);
				//line[0]='^';
				//line[l+1]='$';
				//line[l+2]='\0';
				ONION_DEBUG("Check regexp: '%s'",line);
				int er;
				if ( (er=regcomp(&re, line, REG_EXTENDED)) !=0){
					char error[1024];
					regerror(er, &re, error, sizeof(error));
					ONION_ERROR("Error compiling regular expression %s: %s",line, error);
					FAIL(line);
				}
				else{
					if (regexec(&re, buffer->data, 1, match, 0)!=0){
						FAIL(line);
					}
					else{
						ONION_DEBUG0("Found at %d-%d",match[0].rm_so, match[0].rm_eo);
						FAIL_IF(0); // To mark a passed test
					}
				}
				regfree(&re);
			}
			len=1024;
		}

		
		buffer->pos=0;
		onion_request_clean(req);
	}
	
	free(line);
	
	buffer_free(buffer);
	
	fclose(fd);
	END_LOCAL();
}

int main(int argc, char **argv){
	server=onion_server_new();
	onion_server_set_root_handler(server,onion_handler_new((void*)allinfo_handler,NULL,NULL));
	onion_server_set_write(server,(void*)buffer_append);
	
	int i;
	for (i=1;i<argc;i++){
		ONION_INFO("Launching test %s",argv[i]);
		prerecorded(argv[1]);
	}
	END();
}
