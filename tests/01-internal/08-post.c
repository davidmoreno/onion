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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <onion/request.h>
#include <onion/response.h>
#include <onion/handler.h>
#include <onion/server.h>
#include <onion/log.h>

#include "../test.h"

typedef struct{
	char *data;
	size_t size;
	off_t pos;
}buffer;

/// Just appends to the handler. Must be big enought or segfault.. Just for tests.
int buffer_append(buffer *handler, const char *data, unsigned int length){
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

typedef struct{
	const char *filename;
	char *tmpfilename;
	int size;
	int test_ok;
}expected_post;

onion_connection_status post_check(expected_post *post, onion_request *req){
	const char *filename=onion_request_get_post(req,"file");
	const char *tmpfilename=onion_request_get_file(req,"file");
	post->test_ok=1;
	ONION_DEBUG("Got filename %s, expected %s",filename,post->filename);
	if (strcmp(filename, post->filename)!=0){
		ONION_ERROR("File names do not match");
		post->test_ok=0;
	}
	
	ONION_DEBUG("Temporal file %s",tmpfilename);
	
	struct stat st;
	if (stat(tmpfilename, &st)!=0){
		ONION_ERROR("Could not stat temp file");
		post->test_ok=0;
	}
	if (st.st_size!=post->size){
		ONION_ERROR("Size do not match, expected %d, got %d",post->size,st.st_size);
		post->test_ok=0;
	}

	post->tmpfilename=strdup(tmpfilename);

	return OCS_INTERNAL_ERROR;
}

void t01_post_empty_file(){
	INIT_LOCAL();
	buffer *b=buffer_new(1024);
	
	expected_post post;
	post.filename="file.dat";
	post.test_ok=0; // Not ok as not called processor yet
	post.tmpfilename=NULL;
	post.size=0;
	
	onion_server *server=onion_server_new();
	onion_server_set_write(server, (void*)&buffer_append);
	onion_server_set_root_handler(server, onion_handler_new((void*)&post_check,&post,NULL));
	
	onion_request *req=onion_request_new(server,b,"test");

#define POST_EMPTY "POST / HTTP/1.1\nContent-Type: multipart/form-data; boundary=end\nContent-Length:80\n\n--end\nContent-Disposition: text/plain; name=\"file\"; filename=\"file.dat\"\n\n\n--end--"
	onion_request_write(req,POST_EMPTY,sizeof(POST_EMPTY));
	FAIL_IF_NOT_EQUAL(post.test_ok,1);
	
	onion_request_clean(req);
	post.test_ok=0; // Not ok as not called processor yet

#define POST_EMPTYR "POST / HTTP/1.1\r\nContent-Type: multipart/form-data; boundary=end\r\nContent-Length:84\r\n\r\n--end\r\nContent-Disposition: text/plain; name=\"file\"; filename=\"file.dat\"\r\n\r\n\r\n--end--"
	onion_request_write(req,POST_EMPTY,sizeof(POST_EMPTY));
	FAIL_IF_NOT_EQUAL(post.test_ok,1);

	onion_request_free(req);

	if (post.tmpfilename){
		struct stat st;
		FAIL_IF_EQUAL(stat(post.tmpfilename,&st), 0); // Should not exist
		free(post.tmpfilename);
	}
	
	onion_server_free(server);
	buffer_free(b);
	END_LOCAL();
}



int main(int argc, char **argv){
	t01_post_empty_file();
	
	END();
}

