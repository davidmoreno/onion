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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <onion/onion.h>
#include <onion/request.h>
#include <onion/response.h>
#include <onion/handler.h>
#include <onion/log.h>
#include <onion/block.h>

#include "../ctest.h"
#include "buffer_listen_point.h"
#include <fcntl.h>
#include <onion/dict.h>

// Just a file that is always there, and should be big enought to be several packages
#define BIG_FILE "/etc/services"
#define BIG_FILE_BASE "services"

typedef struct{
	const char *filename;
	char *tmpfilename;
	char *tmplink;
	int size;
	int test_ok;
}expected_post;

onion_connection_status post_check(expected_post *post, onion_request *req){
	const char *filename=onion_request_get_post(req,"file");
	const char *tmpfilename=onion_request_get_file(req,"file");
	if (!filename || !tmpfilename)
		return OCS_INTERNAL_ERROR;

	post->test_ok=1;
	ONION_DEBUG("Got filename %s, expected %s",filename,post->filename);
	if (strcmp(filename, post->filename)!=0){
		ONION_ERROR("File names do not match: %s %s", filename, post->filename);
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

	char tmp[256];
	snprintf(tmp,sizeof(tmp),"%s-",tmpfilename);
	ONION_DEBUG("Linking to %s", tmp);

	FAIL_IF_NOT_EQUAL(link(tmpfilename, tmp),0);

	if (post->tmplink)
		free(post->tmplink);
	post->tmplink=strdup(tmp);

	if (post->tmpfilename)
		free(post->tmpfilename);
	post->tmpfilename=strdup(tmpfilename);

	return OCS_INTERNAL_ERROR;
}

void t01_post_empty_file(){
	INIT_LOCAL();
	expected_post post={};
	post.filename="file.dat";
	post.test_ok=0; // Not ok as not called processor yet
	post.tmpfilename=NULL;
	post.size=0;

	onion *server=onion_new(0);
	onion_listen_point *lp=onion_buffer_listen_point_new();
	onion_add_listen_point(server,NULL,NULL,lp);
	onion_set_root_handler(server, onion_handler_new((void*)&post_check,&post,NULL));

	onion_request *req=onion_request_new(lp);

#define POST_EMPTY "POST / HTTP/1.1\nContent-Type: multipart/form-data; boundary=end\nContent-Length:80\n\n--end\nContent-Disposition: text/plain; name=\"file\"; filename=\"file.dat\"\n\n\n--end--"
	int rs=onion_request_write(req,POST_EMPTY,sizeof(POST_EMPTY));
	FAIL_IF_NOT_EQUAL_INT(rs, OCS_REQUEST_READY);
	onion_request_process(req);
	FAIL_IF_NOT_EQUAL(post.test_ok,1);

	onion_request_clean(req);
	post.test_ok=0; // Not ok as not called processor yet
#undef POST_EMPTY

#define POST_EMPTYR "POST / HTTP/1.1\r\nContent-Type: multipart/form-data; boundary=end\r\nContent-Length:84\r\n\r\n--end\r\nContent-Disposition: text/plain; name=\"file\"; filename=\"file.dat\"\r\n\r\n\r\n--end--"
	rs=onion_request_write(req,POST_EMPTYR,sizeof(POST_EMPTYR));
	FAIL_IF_NOT_EQUAL_INT(rs, OCS_REQUEST_READY);
	onion_request_process(req);
	FAIL_IF_NOT_EQUAL(post.test_ok,1);
#undef POST_EMPTYR

	onion_request_free(req);

	if (post.tmpfilename){
		struct stat st;
		FAIL_IF_EQUAL(stat(post.tmpfilename,&st), 0); // Should not exist
		free(post.tmpfilename);
	}
	if (post.tmplink)
		free(post.tmplink);

	onion_free(server);
	END_LOCAL();
}

/// A BUG were detected: transformed \n to \r\n on files.
void t02_post_new_lines_file(){
	INIT_LOCAL();

	expected_post post={};;
	post.filename="file.dat";
	post.test_ok=0; // Not ok as not called processor yet
	post.tmpfilename=NULL;
	post.size=1;

	onion *server=onion_new(0);
	onion_listen_point *lp=onion_buffer_listen_point_new();
	onion_add_listen_point(server,NULL,NULL,lp);
	onion_set_root_handler(server, onion_handler_new((void*)&post_check,&post,NULL));

	onion_request *req=onion_request_new(lp);

#define POST_EMPTY "POST / HTTP/1.1\nContent-Type: multipart/form-data; boundary=end\nContent-Length:81\n\n--end\nContent-Disposition: text/plain; name=\"file\"; filename=\"file.dat\"\n\n\n\n--end--"
	int rs=onion_request_write(req,POST_EMPTY,sizeof(POST_EMPTY));
	FAIL_IF_NOT_EQUAL_INT(rs, OCS_REQUEST_READY);
	onion_request_process(req);
	FAIL_IF_NOT_EQUAL(post.test_ok,1);
#undef POST_EMPTY

	onion_request_clean(req);
	post.test_ok=0; // Not ok as not called processor yet

#define POST_EMPTYR "POST / HTTP/1.1\r\nContent-Type: multipart/form-data; boundary=end\r\nContent-Length:85\r\n\r\n--end\r\nContent-Disposition: text/plain; name=\"file\"; filename=\"file.dat\"\r\n\r\n\n\r\n--end--"
	rs=onion_request_write(req,POST_EMPTYR,sizeof(POST_EMPTYR));
	FAIL_IF_NOT_EQUAL_INT(rs, OCS_REQUEST_READY);
	onion_request_process(req);
	FAIL_IF_NOT_EQUAL(post.test_ok,1);
#undef POST_EMPTYR

	onion_request_free(req);

	if (post.tmpfilename){
		struct stat st;
		FAIL_IF_EQUAL(stat(post.tmpfilename,&st), 0); // Should not exist
		free(post.tmpfilename);
	}
	if (post.tmplink)
		free(post.tmplink);

	onion_free(server);
	END_LOCAL();
}


/// A BUG were detected: transformed \n to \r\n on files.
void t03_post_carriage_return_new_lines_file(){
	INIT_LOCAL();

	expected_post post={};;
	post.filename="file.dat";
	post.test_ok=0; // Not ok as not called processor yet
	post.tmpfilename=NULL;
	post.size=3;

	onion *server=onion_new(0);
	onion_listen_point *lp=onion_buffer_listen_point_new();
	onion_add_listen_point(server,NULL,NULL,lp);
	onion_set_root_handler(server, onion_handler_new((void*)&post_check,&post,NULL));

	onion_request *req=onion_request_new(lp);

#define POST_EMPTY "POST / HTTP/1.1\nContent-Type: multipart/form-data; boundary=end\nContent-Length:81\n\n--end\nContent-Disposition: text/plain; name=\"file\"; filename=\"file.dat\"\n\n\n\r\n\n--end--"
	//ONION_DEBUG("%s",POST_EMPTY);
	int rs=onion_request_write(req,POST_EMPTY,sizeof(POST_EMPTY));
	FAIL_IF_NOT_EQUAL_INT(rs, OCS_REQUEST_READY);
	onion_request_process(req);

	FAIL_IF_NOT_EQUAL(post.test_ok,1);
#undef POST_EMPTY

	onion_request_clean(req);
	post.test_ok=0; // Not ok as not called processor yet

#define POST_EMPTYR "POST / HTTP/1.1\r\nContent-Type: multipart/form-data; boundary=end\r\nContent-Length:85\r\n\r\n--end\r\nContent-Disposition: text/plain; name=\"file\"; filename=\"file.dat\"\r\n\r\n\n\r\n\r\n--end--"
	rs=onion_request_write(req,POST_EMPTYR,sizeof(POST_EMPTYR));
	FAIL_IF_NOT_EQUAL_INT(rs, OCS_REQUEST_READY);
	onion_request_process(req);

	FAIL_IF_NOT_EQUAL(post.test_ok,1);
#undef POST_EMPTYR

	onion_request_free(req);

	if (post.tmpfilename){
		struct stat st;
		FAIL_IF_EQUAL(stat(post.tmpfilename,&st), 0); // Should not exist
		free(post.tmpfilename);
	}
	if (post.tmplink)
		free(post.tmplink);

	onion_free(server);
	END_LOCAL();
}

/// There is a bug when posting large files. Introduced when change write 1 by 1, to write by blocks on the FILE parser
void t04_post_largefile(){
	INIT_LOCAL();

	int postfd=open(BIG_FILE, O_RDONLY);
	off_t filesize=lseek(postfd, 0, SEEK_END);
	lseek(postfd, 0, SEEK_SET);

	expected_post post={};
	post.filename=BIG_FILE_BASE;
	post.test_ok=0; // Not ok as not called processor yet
	post.tmpfilename=NULL;
	post.size=filesize;

	onion *server=onion_new(0);
	onion_listen_point *lp=onion_buffer_listen_point_new();
	onion_add_listen_point(server,NULL,NULL,lp);
	onion_set_root_handler(server, onion_handler_new((void*)&post_check,&post,NULL));

	onion_request *req=onion_request_new(lp);

#define POST_HEADER "POST / HTTP/1.1\nContent-Type: multipart/form-data; boundary=end\nContent-Length: %d\n\n--end\nContent-Disposition: text/plain; name=\"file\"; filename=\"" BIG_FILE_BASE "\"\n\n"
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

	int rs=onion_request_write(req,"\n--end--",8);
	FAIL_IF_NOT_EQUAL_INT(rs, OCS_REQUEST_READY);
	onion_request_process(req);


	FAIL_IF_NOT_EQUAL_INT(post.test_ok,1);
#undef POST_HEADER
	onion_request_clean(req);


	//post.test_ok=0; // Not ok as not called processor yet

	lseek(postfd, 0, SEEK_SET);

	int difffd=open(post.tmpfilename, O_RDONLY);
	FAIL_IF_NOT_EQUAL_INT(difffd,-1); // Orig file is removed at handler returns. But i have a copy
	if (difffd>=0)
		close(difffd);
	difffd=open(post.tmplink, O_RDONLY);
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

	onion_request_free(req);

	if (post.tmpfilename){
		struct stat st;
		FAIL_IF_EQUAL(stat(post.tmpfilename,&st), 0); // Should not exist
	}

	onion_free(server);
	if (post.tmpfilename)
		free(post.tmpfilename);
	if (post.tmplink)
		free(post.tmplink);

	END_LOCAL();
}

typedef struct{
	int processed;
}json_response;

#define JSON_EXAMPLE "{\n\t\"glossary\": {\n\t\t\"title\": \"example glossary\",\n\t\t\"GlossDiv\": {\n\t\t\t\"title\": \"S\",\n\t\t\t\"GlossList\": {\n\t\t\t\t\"GlossEntry\": {\n\t\t\t\t\t\"ID\": \"SGML\",\n\t\t\t\t\t\"SortAs\": \"SGML\",\n\t\t\t\t\t\"GlossTerm\": \"Standard Generalized Markup Language\",\n\t\t\t\t\t\"Acronym\": \"SGML\",\n\t\t\t\t\t\"Abbrev\": \"ISO 8879:1986\",\n\t\t\t\t\t\"GlossDef\": {\n\t\t\t\t\t\t\"para\": \"A meta-markup language, used to create markup languages such as DocBook.\",\n\t\t\t\t\t\t\"GlossSeeAlso\": [\"GML\", \"XML\"]\n\t\t\t\t\t},\n\t\t\t\t\t\"GlossSee\": \"markup\"\n\t\t\t\t}\n\t\t\t}\n\t\t}\n\t}\n}"

onion_connection_status post_json_check(json_response *post, onion_request *req, onion_response *res){
	post->processed=1;

	FAIL_IF_NOT_EQUAL_INT(onion_request_get_flags(req)&OR_METHODS, OR_POST);
	FAIL_IF_NOT_EQUAL_STR(onion_request_get_header(req, "Content-Type"),"application/json");
	const onion_block *data=onion_request_get_data(req);
	FAIL_IF_EQUAL(data, NULL);
	if (!data)
		return OCS_INTERNAL_ERROR;

	FAIL_IF_NOT_EQUAL_INT(onion_block_size(data), sizeof(JSON_EXAMPLE));
	FAIL_IF_NOT_EQUAL_INT(memcmp(onion_block_data(data), JSON_EXAMPLE, sizeof(JSON_EXAMPLE)), 0);

	post->processed=2;

	return OCS_PROCESSED;
}

void t05_post_content_json(){
	INIT_LOCAL();

	onion *server=onion_new(0);
	onion_listen_point *lp=onion_buffer_listen_point_new();
	json_response post_json = { 0 };

	onion_add_listen_point(server,NULL,NULL,lp);
	onion_set_root_handler(server, onion_handler_new((void*)&post_json_check,&post_json,NULL));

	onion_request *req=onion_request_new(lp);
#define POST_HEADER "POST / HTTP/1.1\nContent-Type: application/json\nContent-Length: %d\n\n"
	char tmp[1024];
	int json_length=sizeof(JSON_EXAMPLE);
	ONION_DEBUG("Post size is about %d",json_length);
	snprintf(tmp, sizeof(tmp), POST_HEADER, json_length);
// 	ONION_DEBUG("%s",tmp);
	onion_request_write(req,tmp,strlen(tmp));
	int rs=onion_request_write(req,JSON_EXAMPLE,json_length);
	FAIL_IF_NOT_EQUAL_INT(rs, OCS_REQUEST_READY);
	onion_request_process(req);
// 	ONION_DEBUG("%s",JSON_EXAMPLE);

	FAIL_IF_NOT_EQUAL_INT(post_json.processed, 2);

	onion_request_free(req);
	onion_free(server);

	END_LOCAL();
}

onion_connection_status post_empty_check(json_response *post, onion_request *req, onion_response *res){
	post->processed=1;

	FAIL_IF_NOT_EQUAL_INT (onion_dict_count( onion_request_get_post_dict(req) ), 0);

	post->processed=2;

	return OCS_PROCESSED;
}

void t06_post_empty(){
	INIT_LOCAL();

	onion *server=onion_new(0);
	onion_listen_point *lp=onion_buffer_listen_point_new();
	json_response post_json = { 0 };

	onion_add_listen_point(server,NULL,NULL,lp);
	onion_set_root_handler(server, onion_handler_new((void*)&post_empty_check,&post_json,NULL));

	onion_request *req=onion_request_new(lp);
#define POST_EMPTY "POST / HTTP/1.1\nContent-Type: application/x-www-form-urlencoded\nContent-Length: 0\n\n"
	int rs=onion_request_write(req,POST_EMPTY,strlen(POST_EMPTY));
	FAIL_IF_NOT_EQUAL_INT(rs, OCS_REQUEST_READY);
	onion_request_process(req);
// 	ONION_DEBUG("%s",JSON_EXAMPLE);

	FAIL_IF_NOT_EQUAL_INT(post_json.processed, 2);

	onion_request_free(req);
	onion_free(server);

	END_LOCAL();
}


int main(int argc, char **argv){
	START();

	t01_post_empty_file();
	t02_post_new_lines_file();
	t03_post_carriage_return_new_lines_file();
	t04_post_largefile();
	t05_post_content_json();
	t06_post_empty();

	END();
}
