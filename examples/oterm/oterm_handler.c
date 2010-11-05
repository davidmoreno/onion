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
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <assert.h>
#include <pty.h>
#include <poll.h>

#include <onion_request.h>
#include <onion_response.h>
#include <onion_handler.h>

void opack_index_html(onion_response *res);
void opack_oterm_js(onion_response *res);
void opack_oterm_input_js(onion_response *res);
void opack_oterm_data_js(onion_response *res);
void opack_jquery_1_4_3_min_js(onion_response *res);

int oterm_index(void *d, onion_request *req){
	onion_response *res=onion_response_new(req);
	onion_response_write_headers(res);
	opack_index_html(res);
	return 1;
}
int oterm_oterm(void *d, onion_request *req){
	onion_response *res=onion_response_new(req);
	onion_response_set_header(res,"Content-Type","text/javascript");
	onion_response_write_headers(res);

	opack_jquery_1_4_3_min_js(res);
	opack_oterm_js(res);
	opack_oterm_input_js(res);
	opack_oterm_data_js(res);
	
	return 1;
}
onion_handler *oterm_handler_index(){
	return onion_handler_new((onion_handler_handler)oterm_index, NULL, NULL);
}
onion_handler *oterm_handler_oterm(){
	return onion_handler_new((onion_handler_handler)oterm_oterm, NULL, NULL);
}


typedef struct{
	int fd;
	unsigned int pid;
}oterm_t;

int oterm_data(oterm_t *o, onion_request *req){

	const char *data=onion_request_get_query(req,"resize");
	if (data){
		int ok=kill(o->pid, SIGWINCH);
		
		onion_response *res=onion_response_new(req);
		onion_response_write_headers(res);
		if (ok==0)
			onion_response_write0(res,"OK");
		else
			onion_response_printf(res, "Error %d",ok);
		onion_response_free(res);
		return 1;
	}

	
	// Write data, if any
	data=onion_request_get_query(req,"type");
	if (data){
		//fprintf(stderr,"%s:%d write %ld bytes\n",__FILE__,__LINE__,strlen(data));
		write(o->fd, data, strlen(data));
	}
	
	// read data, if any. Else return inmediately empty.
	char buffer[4096];
	int n;
	struct pollfd p;
	p.fd=o->fd;
	p.events=POLLIN|POLLERR;
	
	if (poll(&p,1,500)>0){
		if (p.revents==POLLIN){
			//fprintf(stderr,"%s:%d read...\n",__FILE__,__LINE__);
			n=read(o->fd, buffer, sizeof(buffer));
			//fprintf(stderr,"%s:%d read ok, %d bytes\n",__FILE__,__LINE__,n);
		}
	}
	else
		n=0;

	onion_response *res=onion_response_new(req);
	onion_response_write_headers(res);
	if (n)
		onion_response_write(res,buffer,n);
	onion_response_free(res);
	
	return 1;
}



void oterm_oterm_free(oterm_t *o){
	kill(o->pid, SIGTERM);
	
	close(o->fd);
	
	free(o);
}


onion_handler *oterm_handler_data(){
	oterm_t *oterm=malloc(sizeof(oterm_t));

	oterm->pid=forkpty(&oterm->fd, NULL, NULL, NULL);
#ifdef __DEBUG__
	int stderrdup=dup(2);
#endif
	if ( oterm->pid== 0 ){ // on child
		int ok=execl("/bin/bash","bash",NULL);
#ifdef __DEBUG__
		dup2(stderrdup, 2);
#endif
		fprintf(stderr,"%s:%d Could not exec shell: %d\n",__FILE__,__LINE__,ok);
		perror("");
		exit(1);
	}
	/*
	sleep(1);
	if (waitpid(oterm->pid, NULL, WNOHANG) <=0 ){
		fprintf(stderr,"%s:%d child finished too early.\n",__FILE__,__LINE__);
		exit(1);
	}
	*/
	
	return onion_handler_new((onion_handler_handler)oterm_data, oterm, (onion_handler_private_data_free) oterm_oterm_free);
}

