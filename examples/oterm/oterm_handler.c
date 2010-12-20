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
#include <pthread.h>

#include <onion/request.h>
#include <onion/response.h>
#include <onion/handler.h>
#include <onion/log.h>

#ifdef __DEBUG__
#include <onion/handlers/directory.h>
#endif

#include <onion/handlers/opack.h>

/// Time to wait for output, or just return.
#define TIMEOUT 60000

void opack_oterm_html(onion_response *res);
void opack_oterm_js(onion_response *res);
void opack_oterm_input_js(onion_response *res);
void opack_oterm_data_js(onion_response *res);
void opack_oterm_parser_js(onion_response *res);

extern unsigned int opack_oterm_html_length;
extern unsigned int opack_oterm_js_length;
extern unsigned int opack_oterm_input_js_length;
extern unsigned int opack_oterm_data_js_length;
extern unsigned int opack_oterm_parser_js_length;

/**
 * @short Information about a process
 */
typedef struct process_t{
	int fd;
	unsigned int pid;
	struct process_t *next;
}process;

/**
 * @short Information about all the processes.
 */
typedef struct{
	pthread_mutex_t head_mutex;
	process *head;
	onion_handler *data;
}oterm_t;


process *oterm_new(oterm_t *o);
static int oterm_status(oterm_t *o, onion_request *req);

static int oterm_resize(process *o, onion_request *req);
static int oterm_in(process *o, onion_request *req);
static int oterm_out(process *o, onion_request *req);

/// Returns the term from the list of known terms. FIXME, make this structure a tree or something faster than linear search.
process *oterm_get_process(oterm_t *o, const char *id){
	pthread_mutex_lock( &o->head_mutex );
	process *p=o->head;
	int pid=atoi(id);
	while(p){
		if (p->pid==pid){
			pthread_mutex_unlock( &o->head_mutex );
			return p;
		}
		p=p->next;
	}
	pthread_mutex_unlock( &o->head_mutex );
	return NULL;
}

/// Plexes the request depending on arguments.
int oterm_data(oterm_t *o, onion_request *req){
	const char *path=onion_request_get_path(req);
	
	if (strcmp(path,"new")==0){
		oterm_new(o);
		return onion_response_shortcut(req, "ok", 200);
	}
	if (strcmp(path,"status")==0)
		return oterm_status(o,req);

	// split id / function
	int l=strlen(path)+1;
	char *id=alloca(l);
	char *function=NULL;
	
	int i;
	memcpy(id,path,l);
	int func_pos=0;
	for (i=0;i<l;i++){
		if (id[i]=='/'){
			id[i]=0;
			if (function)
				return onion_response_shortcut(req, "Bad formed petition. (1)", 500);
			function=id+i+1;
			func_pos=i;
			break;
		}
	}
	
	if (!function)
		return onion_response_shortcut(req, "Bad formed petition. (2)", 500);
	// do it
	process *term=oterm_get_process(o, id);
	if (!term)
		return onion_response_shortcut(req, "Terminal Id unknown", 404);
	if (strcmp(function,"out")==0)
		return oterm_out(term,req);
	if (strcmp(function,"in")==0)
		return oterm_in(term,req);
	if (strcmp(function,"resize")==0)
		return oterm_resize(term,req);
	onion_request_advance_path(req, func_pos);
	
	return onion_handler_handle(o->data, req);
}

/// Variables that will be passed to the new environment.
const char *onion_clearenvs[]={ "HOME", "TMP" };
const char *onion_extraenvs[]={ "TERM=xterm" };

#define ONION_CLEARENV_COUNT (sizeof(onion_clearenvs)/sizeof(onion_clearenvs[0]))
#define ONION_EXTRAENV_COUNT (sizeof(onion_extraenvs)/sizeof(onion_extraenvs[0]))

/// Creates a new oterm
process *oterm_new(oterm_t *o){
	process *oterm=malloc(sizeof(process));

	oterm->pid=forkpty(&oterm->fd, NULL, NULL, NULL);
	if ( oterm->pid== 0 ){ // on child
		// Copy env vars.
		char **envs=malloc(sizeof(char*)*(1+ONION_CLEARENV_COUNT+ONION_EXTRAENV_COUNT));
		int i,j=0;
		for (i=0;i<ONION_CLEARENV_COUNT;i++){
			const char *env=onion_clearenvs[i];
			const char *val=getenv(env);
			if (val){
				int l=strlen(env)+1+strlen(val)+1;
				envs[j]=malloc(l);
				sprintf(envs[j],"%s=%s",env,val);
				j++;
			}
		}
		for (i=0;i<ONION_EXTRAENV_COUNT;i++){
			envs[j]=strdup(onion_extraenvs[i]);
			j++;
		}
		envs[j]=NULL;

		int ok=execle("/bin/bash","bash",NULL,envs);
		fprintf(stderr,"%s:%d Could not exec shell: %d\n",__FILE__,__LINE__,ok);
		perror("");
		exit(1);
	}
	// I set myself at head
	pthread_mutex_lock( &o->head_mutex );
	oterm->next=o->head;
	o->head=oterm;
	pthread_mutex_unlock( &o->head_mutex );
	
	return oterm;
}


/// Returns the status of all known terminals.
int oterm_status(oterm_t *o, onion_request *req){
	onion_response *res=onion_response_new(req);
	onion_response_write_headers(res);
	
	onion_response_write0(res,"{");

	
	process *n=o->head;
	while(n->next){
		onion_response_printf(res, " \"%d\":{ \"pid\":\"%d\" }, ", n->pid, n->pid);
		n=n->next;
	}
	onion_response_printf(res, " \"%d\":{ \"pid\":\"%d\" } ", n->pid, n->pid);
	
	onion_response_write0(res,"}\n");
	
	return onion_response_free(res);
}

/// Input data to the process
int oterm_in(process *o, onion_request *req){
	const char *data;
	data=onion_request_get_post(req,"type");
	ssize_t w;
	if (data){
		//fprintf(stderr,"%s:%d write %ld bytes\n",__FILE__,__LINE__,strlen(data));
		size_t r=strlen(data);
		w=write(o->fd, data, r);
		if (w!=r){
			ONION_WARNING("Error writing data to process. Not all data written. (%d).",w);
			return onion_response_shortcut(req, "Error", HTTP_INTERNAL_ERROR);
		}
	}
	return onion_response_shortcut(req, "OK", HTTP_OK);
}

/// Resize the window. Do not work yet, and I dont know whats left. FIXME.
int oterm_resize(process *o, onion_request* req){
	//const char *data=onion_request_get_query(req,"resize");
	int ok=kill(o->pid, SIGWINCH);
	
	if (ok==0)
		return onion_response_shortcut(req,"OK",HTTP_OK);
	else
		return onion_response_shortcut(req,"Error",HTTP_INTERNAL_ERROR);
}

/// Gets the output data
int oterm_out(process *o, onion_request *req){
	// read data, if any. Else return inmediately empty.
	char buffer[4096];
	int n=0; // -O2 complains of maybe used uninitialized
	struct pollfd p;
	p.fd=o->fd;
	p.events=POLLIN|POLLERR;
	
	if (poll(&p,1,TIMEOUT)>0){
		if (p.revents==POLLIN){
			//fprintf(stderr,"%s:%d read...\n",__FILE__,__LINE__);
			n=read(o->fd, buffer, sizeof(buffer));
			//fprintf(stderr,"%s:%d read ok, %d bytes\n",__FILE__,__LINE__,n);
		}
	}
	else
		n=0;
	onion_response *res=onion_response_new(req);
	onion_response_set_length(res, n);
	onion_response_write_headers(res);
	if (n)
		onion_response_write(res,buffer,n);
	return onion_response_free(res);
}

/// Terminates all processes, and frees the memory.
void oterm_oterm_free(oterm_t *o){
	process *p=o->head;
	process *t;
	while (p){
		kill(p->pid, SIGTERM);
		close(p->fd);
		t=p;
		p=p->next;
		free(t);
	}
	onion_handler_free(o->data);
	pthread_mutex_destroy(&o->head_mutex);
	free(o);
}

/// Prepares the oterm handler
onion_handler *oterm_handler_data(){
	oterm_t *oterm=malloc(sizeof(oterm_t));
	
	pthread_mutex_init(&oterm->head_mutex, NULL);
	oterm->head=NULL;
	oterm->head=oterm_new(oterm);
	onion_handler *data;
#ifdef __DEBUG__
	if (getenv("OTERM_DEBUG"))
		data=onion_handler_directory(".");
	else{
#endif
	data=onion_handler_opack("/",opack_oterm_html, opack_oterm_html_length);
	onion_handler_add(data, onion_handler_opack("/oterm.js", opack_oterm_js, opack_oterm_js_length));
	onion_handler_add(data, onion_handler_opack("/oterm_input.js", opack_oterm_input_js, opack_oterm_input_js_length));
	onion_handler_add(data, onion_handler_opack("/oterm_parser.js", opack_oterm_parser_js, opack_oterm_parser_js_length));
	onion_handler_add(data, onion_handler_opack("/oterm_data.js", opack_oterm_data_js, opack_oterm_data_js_length));
#ifdef __DEBUG__
	}
#endif
	
	oterm->data=data;
	
	return onion_handler_new((onion_handler_handler)oterm_data, oterm, (onion_handler_private_data_free) oterm_oterm_free);
}

