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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <onion/onion.h>
#include <onion/request.h>
#include <onion/response.h>
#include <onion/handler.h>
#include <onion/log.h>
#include <onion/dict.h>
#include <onion/block.h>

#include "../ctest.h"
#include "../01-internal/buffer_listen_point.h"
#include <regex.h>

onion *server;

typedef struct{
	onion_response *res;
	const char *part;
}allinfo_dict_print_t;

void allinfo_query(allinfo_dict_print_t *aid, const char *key, const char *value, int flags){
	onion_response *res=aid->res;
	
	onion_response_write0(res,aid->part);
	onion_response_write0(res,": ");
	onion_response_write0(res,key);
	onion_response_write0(res," = ");
	onion_response_write0(res,value);
	onion_response_write0(res,"\n");
}


/**
 * @short Handler that just echoes all data, writing what was a header, what the method...
 */
onion_connection_status allinfo_handler(void *data, onion_request *req, onion_response *res){
	onion_response_write_headers(res);
	
	int f=onion_request_get_flags(req);
	onion_response_printf(res, "Method: %s\n",(f&OR_GET) ? "GET" : (f&OR_HEAD) ? "HEAD" : "POST");
	onion_response_printf(res, "Path: %s\n",onion_request_get_path(req));
	onion_response_printf(res, "Version: %s\n",onion_request_get_flags(req)&OR_HTTP11 ? "HTTP/1.1" : "HTTP/1.0");

	allinfo_dict_print_t aid;
	aid.res=res;
	aid.part="Query";
	onion_dict_preorder(onion_request_get_query_dict(req),allinfo_query, &aid);
	aid.part="Header";
	onion_dict_preorder(onion_request_get_header_dict(req),allinfo_query, &aid);
	aid.part="POST";
	onion_dict_preorder(onion_request_get_post_dict(req),allinfo_query, &aid);
	aid.part="FILE";
	onion_dict_preorder(onion_request_get_file_dict(req),allinfo_query, &aid);
	
	return OCS_PROCESSED;
}

/**
 * @short Performs regexec on each line of string.
 * 
 * As regexec cant use ^$ to find the line limits, but the string limits, we split the string to make
 * the find of ^ and $ on each line.
 */
int regexec_multiline(const regex_t *preg, const char *string, size_t nmatch,
                   regmatch_t pmatch[], int eflags){
	char tmp[1024];
	int i=0;
	const char *p=string;
	int r=1;
	while (*p){
		if (*p=='\n'){
			tmp[i]='\0';
			//ONION_DEBUG("Check against %s",tmp);
			if ( (r=regexec(preg,tmp,nmatch, pmatch, eflags))==0 ){
				return r;
			}
			i=0;
		}
		else{
			tmp[i++]=*p;
		}
		p++;

	}
	return r;
}

/**
 * @short Opens a script file, and executes it.
 */
void prerecorded(const char *oscript, int do_r){
	INIT_LOCAL();
	FILE *fd=fopen(oscript, "r");
	if (!fd){
		FAIL("Could not open script file");
		END_LOCAL();
		return;
	}
	const char *script=basename((char*)oscript);
	
	onion_listen_point *listen_point=onion_get_listen_point(server,0);
	onion_request *req=onion_request_new(listen_point);
	onion_block *buffer=onion_buffer_listen_point_get_buffer(req);
	
	ssize_t r;
	const size_t LINE_SIZE=1024;
	char *line=malloc(LINE_SIZE);
	size_t len=LINE_SIZE;
	onion_connection_status ret;
	int ntest=0;
	int linen=0;
	while (!feof(fd)){
		ntest++;
		ret=OCS_NEED_MORE_DATA;
		ONION_DEBUG("Test %d",ntest);
		// Read request
		while ( (r=getline(&line, &len, fd)) != -1 ){
			linen++;
			if (strcmp(line,"-- --\n")==0){
				break;
			}
			if (do_r){
				line[r-1]='\r';
				line[r]='\n';
				r++;
			}
			if (ret==OCS_NEED_MORE_DATA)
				ret=onion_request_write(req, line, r);
			//line[r]='\0';
			//ONION_DEBUG0("Write: %s\\n (%d). Ret %d",line,r,ret);
			len=LINE_SIZE;
		}
		if (r<0){
			fclose(fd);
			onion_request_free(req);
			free(line);
			END_LOCAL();
			return;
		}
		
		if (r==0){
			FAIL_IF("Found end of file before end of request");
			fclose(fd);
			onion_request_free(req);
			free(line);
			END_LOCAL();
			return;
		}
		
		// Check response
		if (onion_block_size(buffer)==0){
			ONION_DEBUG("Empty response");
		}
		else{
			ONION_DEBUG0("Response: %s",onion_block_data(buffer));
		}

		while ( (r=getline(&line, &len, fd)) != -1 ){
			linen++;
			if (strcmp(line,"++ ++\n")==0){
				break;
			}
			line[strlen(line)-1]='\0';
			if (strcmp(line,"INTERNAL_ERROR")==0){ // Checks its an internal error
				ONION_DEBUG("%s:%d Check INTERNAL_ERROR",script,linen);
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
				
				int l=strlen(line)-1;

				int _not=line[0]=='!';
				if (_not){
					ONION_DEBUG("Oposite regexp");
					memmove(line, line+1, l);
					l--;
				}

				memmove(line+1,line,l+1);
				line[0]='^';
				line[l+2]='$';
				line[l+3]='\0';
				ONION_DEBUG("%s:%d Check regexp: '%s'",script, linen, line);
				int r;
				r=regcomp(&re, line, REG_EXTENDED);
				if ( r !=0 ){
					char error[1024];
					regerror(r, &re, error, sizeof(error));
					ONION_ERROR("%s:%d Error compiling regular expression %s: %s",script, linen, line, error);
					FAIL("%s",line);
				}
				else{
					int _match=regexec_multiline(&re, onion_block_data(buffer), 1, match, 0);
					if ( (_not && _match==0) || (!_not && _match!=0) ){
						ONION_ERROR("%s:%d cant find %s",script, linen, line);
						FAIL("%s",line);
					}
					else{
						ONION_DEBUG0("Found at %d-%d",match[0].rm_so, match[0].rm_eo);
						SUCCESS(); // To mark a passed test
					}
				}
				regfree(&re);
			}
			len=1024;
		}

		
		onion_block_clear(buffer);
		onion_request_clean(req);
	}
	
	free(line);
	
	onion_request_free(req);
	
	onion_block_free(buffer);
	
	fclose(fd);
	END_LOCAL();
}

/**
 * @short Executes each script file passed as argument.
 * 
 * Optionally a -r sets the new lines to \r\n. It takes care of not changing content types.
 */
int main(int argc, char **argv){
	START();
	server=onion_new(O_ONE);
	onion_listen_point *listen_point=onion_buffer_listen_point_new();
	onion_set_root_handler(server,onion_handler_new(allinfo_handler,NULL,NULL));
	onion_add_listen_point(server,NULL,NULL, listen_point);
	
	int i;
	int do_r=0;
	for (i=1;i<argc;i++){
		if (strcmp(argv[i],"-r")==0){
			ONION_WARNING("Setting the end of lines to \\r\\n");
			do_r=1;
		}
		else{
			ONION_INFO("Launching test %s",argv[i]);
			prerecorded(argv[i], do_r);
		}
	}
	
	onion_free(server);
	END();
}
