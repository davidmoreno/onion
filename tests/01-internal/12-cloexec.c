/*
	Onion HTTP server library
	Copyright (C) 2011 David Moreno Montero

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

#include <onion/log.h>
#include <onion/onion.h>
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <curl/curl.h>
#include "../ctest.h"
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>

int checkfds();
int handler(const char *me, onion_request *req, onion_response *res);
void init_onion();

void t01_get(){
	INIT_LOCAL();

	ONION_DEBUG("Now CURL get it");
	CURL *curl;
	curl = curl_easy_init();
	if (!curl){
		FAIL("Curl was not initialized");
	}
	else{
		curl_easy_setopt(curl, CURLOPT_URL, "http://localhost:8080");

		CURLcode res=curl_easy_perform(curl);
		FAIL_IF_NOT_EQUAL((int)res,0);
		long int http_code;
		res=curl_easy_getinfo(curl, CURLINFO_HTTP_CODE, &http_code);
		FAIL_IF_NOT_EQUAL((int)res,0);
		curl_easy_cleanup(curl);

		FAIL_IF_NOT_EQUAL_INT((int)http_code, HTTP_OK);
	}
	END_LOCAL();
}

/**
 * @short Simple server that always launch testfds, and returns the result.
 */
int main(int argc, char **argv){
	START();

	if (argc==2 && strcmp(argv[1],"--checkfds")==0)
		return checkfds(argc, argv);

	pid_t pid;
	if ( (pid=fork())==0){
		init_onion(argv[0]);
	}
	sleep(1);
	ONION_DEBUG("Ok, now ask");

	t01_get();

	kill(pid, SIGKILL);
	waitpid(pid, NULL, 0);

	END();
}

void init_onion(const char *me){
	// Lets close all I dont need.
	// Sounds weird but some shells and terminals (yakuake) has laking file descriptors.
	// Fast stupid way.
	int i;
	for (i=3;i<256;i++)
		close(i);
	onion *o;

	o=onion_new(O_ONE);

	onion_url *urls=onion_root_url(o);
	onion_url_add_with_data(urls, "^.*", handler, (void*)me, NULL);

	onion_listen(o);
	exit(0);
}

/**
 * @short Forks a process and calls the fd checker
 */
int handler(const char *me, onion_request *req, onion_response *res){
	ONION_DEBUG("Im %s", me);
	pid_t pid;
	if ( (pid=fork()) == 0 ){
		ONION_DEBUG("On the other thread");
		execl(me,me,"--checkfds",NULL);
		ONION_ERROR("Could not execute %s", me);
		exit(1); // should not get here
	}

	int error;
	waitpid(pid, &error, 0);
	ONION_DEBUG("Error code is %d", error);

	if (error==0){
		onion_response_write0(res, "OK");
	}
	else{
		onion_response_set_code(res, HTTP_FORBIDDEN);
		onion_response_write0(res, "ERROR. Did not close everything!");
	}
	return OCS_PROCESSED;
}

/**
 * @short Tests how many file descriptor it has open.
 *
 * Should be the ones that argv says, no more no less. Prints result, and returns 0 if ok, other if different.
 */
int checkfds(){
	ONION_INFO("Hello, Im going to check it all");

	int nfd=0;

	DIR *dir=opendir("/proc/self/fd");
	struct dirent *de;

	while ( (de=readdir(dir))!=NULL ){
		if (de->d_name[0]=='.')
			continue;
		ONION_INFO("Checking fd %s", de->d_name);

		int cfd=atoi(de->d_name);
		bool ok=false;

		if (cfd<3){
			ONION_INFO("fd <3 is stdin, stdou, stderr");
			ok=true;
		}

		if (dirfd(dir)==cfd){
			ONION_INFO("Ok, its the dir descriptor");
			ok=true;
		}
		// GNUTls opens urandom at init, before main, checks fd=3 may be that one.
		if (cfd==3){
			char filename[256];
			size_t fns=readlink("/proc/self/fd/3", // I know exactly which file.
			         	filename, sizeof(filename));
			if (fns>0){
				filename[fns+1]=0; // stupid readlinkat... no \0 at end.
				if (strcmp(filename, "/dev/urandom")==0){
					ok=true;
					ONION_INFO("GNU TLS opens /dev/urandom at init.");
				}
				else{
					ONION_DEBUG("fd3 is %s", filename);
				}
			}
		}
		// Unknown not valid fd.
		if (!ok){
			ONION_ERROR("Hey, one fd I dont know about!");
			char tmp[256];
			snprintf(tmp, sizeof(tmp), "ls --color -l /proc/%d/fd/%s", getpid(), de->d_name);
			int err=system(tmp);
			FAIL_IF_NOT_EQUAL_INT(err,0);
			nfd++;
		}
	}

	closedir(dir);

	ONION_INFO("Exit code %d", nfd);
	exit(nfd);
}
