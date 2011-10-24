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

#include <stdio.h>
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
#include <onion/handlers/exportlocal.h>
#endif

#include <onion/handlers/opack.h>
#include <onion/shortcuts.h>
#include <pwd.h>
#include <onion/dict.h>

#include "oterm_handler.h"

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

#define BUFFER_SIZE 4096*2

/**
 * @short Information about a process
 * 
 * The process has a circular buffer. The pos is mod with BUFFER_SIZE. 
 * 
 * First time clients just ask data, and as a custom control command we send the latest position.
 * 
 * Then on next request the client sets that want from that position onwards. If data avaliable,
 * it is sent, if not, then the first blocks on the fd, and the followings on a pthread_condition.
 */
typedef struct process_t{
	int fd;
	pid_t pid;
	char *title;
	char *buffer;
	int32_t buffer_pos;
	struct process_t *next;
}process;

/**
 * @short Information about all the processes.
 */
typedef struct{
	pthread_mutex_t head_mutex;
	process *head;
}oterm_session;

/**
 * @short Data for the onion terminal handler itself
 */
struct oterm_data_t{
	char *exec_command;
	onion_handler *next;
	onion_dict *sessions; // Each user (session['username']) has a session.
};

typedef struct oterm_data_t oterm_data;

process *oterm_new(oterm_data *d, oterm_session *s, const char *username, char impersonate);
oterm_session *oterm_session_new();
static int oterm_status(oterm_session *o, onion_request *req, onion_response *res);

static int oterm_resize(process *o, onion_request *req, onion_response *res);
static int oterm_title(process *o, onion_request *req, onion_response *res);
static int oterm_in(process *o, onion_request *req, onion_response *res);
static int oterm_out(process *o, onion_request *req, onion_response *res);


/// Returns the term from the list of known terms. FIXME, make this structure a tree or something faster than linear search.
process *oterm_get_process(oterm_session *o, const char *id){
	pthread_mutex_lock( &o->head_mutex );
	process *p=o->head;
	int pid=atoi(id);
	int i=1;
	while(p){
		if (i==pid){
			pthread_mutex_unlock( &o->head_mutex );
			return p;
		}
		p=p->next;
		i++;
	}
	pthread_mutex_unlock( &o->head_mutex );
	return NULL;
}

/// Plexes the request depending on arguments.
int oterm_get_data(oterm_data *data, onion_request *req, onion_response *res){
	oterm_session *o=(oterm_session*)onion_dict_get(data->sessions, onion_request_get_session(req,"username"));
	if (!o){
		o=oterm_session_new();
		onion_dict_lock_write(data->sessions);
		onion_dict_add(data->sessions,onion_request_get_session(req,"username"),o, 0);
		onion_dict_unlock(data->sessions);
	}
	
	
	const char *path=onion_request_get_path(req);
	
	if (strcmp(path,"new")==0){
		if (onion_request_get_post(req, "command")){
			free(data->exec_command);
			data->exec_command=strdup(onion_request_get_post(req, "command"));
		}
		oterm_new(data, o, onion_request_get_session(req, "username"), onion_request_get_session(req, "nopam") ? 0 : 1 );
		return onion_shortcut_response("ok", 200, req, res);
	}
	if (strcmp(path,"status")==0)
		return oterm_status(o,req, res);

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
				return onion_shortcut_response("Bad formed petition. (1)", 500, req, res);
			function=id+i+1;
			func_pos=i;
			break;
		}
	}
	
	if (!function)
		return onion_shortcut_response("Bad formed petition. (2)", 500, req, res);
	// do it
	process *term=oterm_get_process(o, id);
	if (!term)
		return onion_shortcut_response("Terminal Id unknown", 404, req, res);
	if (strcmp(function,"out")==0)
		return oterm_out(term,req,res);
	if (strcmp(function,"in")==0)
		return oterm_in(term,req,res);
	if (strcmp(function,"resize")==0)
		return oterm_resize(term,req,res);
	if (strcmp(function,"title")==0)
		return oterm_title(term,req,res);
	onion_request_advance_path(req, func_pos);
	
	return onion_handler_handle(data->next, req, res);
}

/// Variables that will be passed to the new environment.
const char *onion_clearenvs[]={ "HOME", "TMP" };
const char *onion_extraenvs[]={ "TERM=xterm" };

#define ONION_CLEARENV_COUNT (sizeof(onion_clearenvs)/sizeof(onion_clearenvs[0]))
#define ONION_EXTRAENV_COUNT (sizeof(onion_extraenvs)/sizeof(onion_extraenvs[0]))

/// Creates a new oterm
process *oterm_new(oterm_data *data, oterm_session *session, const char *username, char impersonate){
	process *oterm=malloc(sizeof(process));

	const char *command_name;
	int i;
	for (i=strlen(data->exec_command);i>=0;i--)
		if (data->exec_command[i]=='/')
			break;
	command_name=&data->exec_command[i+1];

	oterm->buffer=malloc(BUFFER_SIZE);
	memset(oterm->buffer, 0, BUFFER_SIZE);
	oterm->buffer_pos=0;
	
	ONION_DEBUG("Creating new terminal, exec %s (%s)", data->exec_command, command_name);
	
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

		// Change personality to that user
		if (impersonate){
			struct passwd *pw;
			pw=getpwnam(username);
			int error;
			if (!pw){
				ONION_ERROR("Cant find user to drop priviledges: %s", username);
				exit(1);
			}
			else{
				error=setgid(pw->pw_gid);
				error|=setuid(pw->pw_uid);
			}
			if (error){
				ONION_ERROR("Cant set the uid/gid for user %s", username);
				exit(1);
			}
		}
		for (i=3;i<256;i++) // Force close file descriptors. Dirty but it works.
			close(i);

		int ok=execle(data->exec_command, command_name, NULL, envs);
		fprintf(stderr,"%s:%d Could not exec shell: %d\n",__FILE__,__LINE__,ok);
		perror("");
		exit(1);
	}
	oterm->title=strdup(data->exec_command);
	ONION_DEBUG("Default title is %s", oterm->title);
	oterm->next=NULL;
	// I set myself at end
	pthread_mutex_lock( &session->head_mutex );
	if (!session->head)
		session->head=oterm;
	else{
		process *next=session->head;
		while (next->next) next=next->next;
		next->next=oterm;
	}
	pthread_mutex_unlock( &session->head_mutex );
	
	return oterm;
}

/// Checks if the process is running and if it stopped, set name
static void oterm_check_running(process *n){
	if (n->pid==-1)
		return; // Already dead
	int changed=waitpid(n->pid, NULL, WNOHANG);
	if (changed){
		free(n->title);
		n->title=strdup("- Finished -");
		n->pid=-1;
	}
}

/// Returns the status of all known terminals.
int oterm_status(oterm_session *session, onion_request *req, onion_response *res){
	onion_dict *status=onion_dict_new();
	
	pthread_mutex_lock( &session->head_mutex );
	if (session){
		process *n=session->head;
		int i=1;
		while(n){
			oterm_check_running(n);
			
			char *id=malloc(6);
			sprintf(id, "%d", i);
			onion_dict *term=onion_dict_new();
			onion_dict_add(term, "title", n->title, OD_DUP_VALUE);
			onion_dict_add(status, id, term, OD_DICT|OD_FREE_ALL);
			// Just a check, here is ok, no hanging childs
			n=n->next;
			i++;
		}
	}
	pthread_mutex_unlock( &session->head_mutex );
	
	return onion_shortcut_response_json(status, req, res);
}

/// Input data to the process
int oterm_in(process *p, onion_request *req, onion_response *res){
	oterm_check_running(p);

	const char *data;
	data=onion_request_get_post(req,"type");
	ssize_t w;
	if (data){
		//fprintf(stderr,"%s:%d write %ld bytes\n",__FILE__,__LINE__,strlen(data));
		size_t r=strlen(data);
		w=write(p->fd, data, r);
		if (w!=r){
			ONION_WARNING("Error writing data to process. Not all data written. (%d).",w);
			return onion_shortcut_response("Error", HTTP_INTERNAL_ERROR, req, res);
		}
	}
	return onion_shortcut_response("OK", HTTP_OK, req, res);
}

/// Resize the window. 
int oterm_resize(process *p, onion_request* req, onion_response *res){
	//const char *data=onion_request_get_query(req,"resize");
	//int ok=kill(o->pid, SIGWINCH);
	
	struct winsize winSize;
	memset(&winSize, 0, sizeof(winSize));
	const char *t=onion_request_get_post(req,"width");
	winSize.ws_row = (unsigned short)atoi(t ? t : "80");
	t=onion_request_get_post(req,"height");
	winSize.ws_col = (unsigned short)atoi(t ? t : "25");
	
	int ok=ioctl(p->fd, TIOCSWINSZ, (char *)&winSize) == 0;

	if (ok>=0)
		return onion_shortcut_response("OK",HTTP_OK, req, res);
	else
		return onion_shortcut_response("Error",HTTP_INTERNAL_ERROR, req, res);
}

/// Sets internally the window title, for reference.
int oterm_title(process *p, onion_request* req, onion_response *res){
	const char *t=onion_request_get_post(req, "title");
	
	if (!t)
		return onion_shortcut_response("Error, must set title", HTTP_INTERNAL_ERROR, req, res);
	
	if (p->title)
		free(p->title);
	p->title=strdup(t);
	
	ONION_DEBUG("Set term %d title %s", p->pid, p->title);
	
	return onion_shortcut_response("OK", HTTP_OK, req, res);
}

/// Gets the output data
int oterm_out(process *o, onion_request *req, onion_response *res){
	if (onion_request_get_query(req, "initial")){
		if (o->buffer[BUFFER_SIZE-1]!=0){ // If 0 then never wrote on it. So if not, write from pos to end too, first.
			onion_response_write(res, &o->buffer[o->buffer_pos], BUFFER_SIZE-o->buffer_pos);
		}
		onion_response_write(res, o->buffer, o->buffer_pos);
		return OCS_PROCESSED;
	}
	
	
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
	
	// Store on buffer
	{
		const char *data=buffer;
		int sd=n;
		if (sd>BUFFER_SIZE){
			data=&data[sd-BUFFER_SIZE];
			sd=BUFFER_SIZE;
		}

		if (o->buffer_pos+sd > BUFFER_SIZE){
			memcpy(&o->buffer[o->buffer_pos], data, BUFFER_SIZE-o->buffer_pos);
			data=&data[BUFFER_SIZE-o->buffer_pos];
			sd=sd-(BUFFER_SIZE-o->buffer_pos);
			o->buffer_pos=0;
		}
		memcpy(&o->buffer[o->buffer_pos], data, sd);
		o->buffer_pos+=sd;
	}
		
	// Return data
	
	onion_response_set_length(res, n);
	onion_response_write_headers(res);
	if (n)
		onion_response_write(res,buffer,n);

	oterm_check_running(o);

	return OCS_PROCESSED;
}

/// Terminates all processes, and frees the memory.
void oterm_session_free(oterm_session *o){
	process *p=o->head;
	process *t;
	while (p){
		kill(p->pid, SIGTERM);
		close(p->fd);
		free(p->buffer);
		t=p;
		p=p->next;
		free(t);
	}
	pthread_mutex_destroy(&o->head_mutex);
	free(o);
}

/// Creates a new session, one per user at oterm_data->sessions.
oterm_session *oterm_session_new(){
	oterm_session *oterm=malloc(sizeof(oterm_session));
	
	pthread_mutex_init(&oterm->head_mutex, NULL);
	oterm->head=NULL;
	
	return oterm;
}

/// Frees memory used by the handler
void oterm_free(oterm_data *data){
	// TODO Clean properly all processes
	onion_dict_free(data->sessions);
	free(data->exec_command);
	onion_handler_free(data->next);
	free(data);
}

/// Prepares the oterm handler
onion_handler *oterm_handler(const char *exec_command){
	onion_handler *next_handler;
#ifdef __DEBUG__
	if (getenv("OTERM_DEBUG"))
		next_handler=onion_handler_export_local_new(".");
	else{
#endif
	next_handler=onion_handler_opack("/",opack_oterm_html, opack_oterm_html_length);
	onion_handler_add(next_handler, onion_handler_opack("/oterm.js", opack_oterm_js, opack_oterm_js_length));
	onion_handler_add(next_handler, onion_handler_opack("/oterm_input.js", opack_oterm_input_js, opack_oterm_input_js_length));
	onion_handler_add(next_handler, onion_handler_opack("/oterm_parser.js", opack_oterm_parser_js, opack_oterm_parser_js_length));
	onion_handler_add(next_handler, onion_handler_opack("/oterm_data.js", opack_oterm_data_js, opack_oterm_data_js_length));
#ifdef __DEBUG__
	}
#endif
	
	oterm_data *data=malloc(sizeof(oterm_data));
	data->sessions=onion_dict_new();
	data->next=next_handler;
	data->exec_command=strdup(exec_command);
	
	return onion_handler_new((void*)oterm_get_data, data, (void*)oterm_free);
}

