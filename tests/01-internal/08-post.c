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
#include <fcntl.h>

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
#undef POST_EMPTY

#define POST_EMPTYR "POST / HTTP/1.1\r\nContent-Type: multipart/form-data; boundary=end\r\nContent-Length:84\r\n\r\n--end\r\nContent-Disposition: text/plain; name=\"file\"; filename=\"file.dat\"\r\n\r\n\r\n--end--"
	onion_request_write(req,POST_EMPTYR,sizeof(POST_EMPTYR));
	FAIL_IF_NOT_EQUAL(post.test_ok,1);
#undef POST_EMPTYR

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

/// A BUG were detected: transformed \n to \r\n on files.
void t02_post_new_lines_file(){
	INIT_LOCAL();
	
	buffer *b=buffer_new(1024);
	
	expected_post post;
	post.filename="file.dat";
	post.test_ok=0; // Not ok as not called processor yet
	post.tmpfilename=NULL;
	post.size=1;
	
	onion_server *server=onion_server_new();
	onion_server_set_write(server, (void*)&buffer_append);
	onion_server_set_root_handler(server, onion_handler_new((void*)&post_check,&post,NULL));
	
	onion_request *req=onion_request_new(server,b,"test");

#define POST_EMPTY "POST / HTTP/1.1\nContent-Type: multipart/form-data; boundary=end\nContent-Length:81\n\n--end\nContent-Disposition: text/plain; name=\"file\"; filename=\"file.dat\"\n\n\n\n--end--"
	onion_request_write(req,POST_EMPTY,sizeof(POST_EMPTY));
	FAIL_IF_NOT_EQUAL(post.test_ok,1);
#undef POST_EMPTY

	onion_request_clean(req);
	post.test_ok=0; // Not ok as not called processor yet

#define POST_EMPTYR "POST / HTTP/1.1\r\nContent-Type: multipart/form-data; boundary=end\r\nContent-Length:85\r\n\r\n--end\r\nContent-Disposition: text/plain; name=\"file\"; filename=\"file.dat\"\r\n\r\n\n\r\n--end--"
	onion_request_write(req,POST_EMPTYR,sizeof(POST_EMPTYR));
	FAIL_IF_NOT_EQUAL(post.test_ok,1);
#undef POST_EMPTYR

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


/// A BUG were detected: transformed \n to \r\n on files.
void t03_post_carriage_return_new_lines_file(){
	INIT_LOCAL();
	
	buffer *b=buffer_new(1024);
	
	expected_post post;
	post.filename="file.dat";
	post.test_ok=0; // Not ok as not called processor yet
	post.tmpfilename=NULL;
	post.size=3;
	
	onion_server *server=onion_server_new();
	onion_server_set_write(server, (void*)&buffer_append);
	onion_server_set_root_handler(server, onion_handler_new((void*)&post_check,&post,NULL));
	
	onion_request *req=onion_request_new(server,b,"test");

#define POST_EMPTY "POST / HTTP/1.1\nContent-Type: multipart/form-data; boundary=end\nContent-Length:81\n\n--end\nContent-Disposition: text/plain; name=\"file\"; filename=\"file.dat\"\n\n\n\r\n\n--end--"
	//ONION_DEBUG("%s",POST_EMPTY);
	onion_request_write(req,POST_EMPTY,sizeof(POST_EMPTY));
	FAIL_IF_NOT_EQUAL(post.test_ok,1);
#undef POST_EMPTY

	onion_request_clean(req);
	post.test_ok=0; // Not ok as not called processor yet

#define POST_EMPTYR "POST / HTTP/1.1\r\nContent-Type: multipart/form-data; boundary=end\r\nContent-Length:85\r\n\r\n--end\r\nContent-Disposition: text/plain; name=\"file\"; filename=\"file.dat\"\r\n\r\n\n\r\n\r\n--end--"
	onion_request_write(req,POST_EMPTYR,sizeof(POST_EMPTYR));
	FAIL_IF_NOT_EQUAL(post.test_ok,1);
#undef POST_EMPTYR

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

/// There is a bug when posting large files. Introduced when change write 1 by 1, to write by blocks on the FILE parser
void t04_post_largefile(){
	INIT_LOCAL();
	
	int postfd=open("01-hash", O_RDONLY);
	off_t filesize=lseek(postfd, 0, SEEK_END);
	lseek(postfd, 0, SEEK_SET);
	
	buffer *b=buffer_new(1024);
	expected_post post;
	post.filename="01-hash";
	post.test_ok=0; // Not ok as not called processor yet
	post.tmpfilename=NULL;
	post.size=filesize;
	
	onion_server *server=onion_server_new();
	onion_server_set_write(server, (void*)&buffer_append);
	onion_server_set_root_handler(server, onion_handler_new((void*)&post_check,&post,NULL));
	
	onion_request *req=onion_request_new(server,b,"test");

#define POST_HEADER "POST / HTTP/1.1\nContent-Type: multipart/form-data; boundary=end\nContent-Length: %d\n\n--end\nContent-Disposition: text/plain; name=\"file\"; filename=\"01-hash\"\n\n"
	char tmp[1024];
	ONION_DEBUG("Post size is about %d",filesize+73);
	snprintf(tmp, sizeof(tmp), POST_HEADER, (int)filesize+73);
	ONION_DEBUG("%s",tmp);
	onion_request_write(req,tmp,strlen(tmp));
	
	int r=read(postfd, tmp, sizeof(tmp));
	while ( r>0 ){
		onion_request_write(req, tmp, r);
		r=read(postfd, tmp, sizeof(tmp));
	}
	
	onion_request_write(req,"\n--end--",8);
	FAIL_IF_NOT_EQUAL(post.test_ok,1);
#undef POST_HEADER

	post.test_ok=0; // Not ok as not called processor yet

	int difffd=open(post.tmpfilename, O_RDONLY);
	lseek(postfd, 0, SEEK_SET);
	FAIL_IF_EQUAL_INT(difffd,-1);
	ONION_DEBUG("tmp filename %s",post.tmpfilename);
	int r1=1, r2=1;
	char c1=0, c2=0;
	int p=0;
	while ( r1 && r2 && c1==c2){
		r1=read(difffd, &c1, 1);
		r2=read(postfd, &c2, 1);
		//ONION_DEBUG("%d %d",c1,c2);
		FAIL_IF_NOT_EQUAL_INT(c1,c2);
		p++;
	}
	if ( r1 || r2 ){
		ONION_ERROR("At %d",p);
		FAIL_IF_NOT_EQUAL_INT(r1,0);
		FAIL_IF_NOT_EQUAL_INT(r2,0);
		FAIL_IF_NOT_EQUAL_INT(c1,c2);
		FAIL("Files are different");
	}
	else
		ONION_DEBUG("Files are ok");
	
	close(difffd);
	close(postfd);

	onion_request_clean(req);

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
	t02_post_new_lines_file();
	t03_post_carriage_return_new_lines_file();
	t04_post_largefile();
	
	END();
}

