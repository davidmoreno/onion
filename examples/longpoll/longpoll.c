/*
	Onion HTTP server library
	Copyright (C) 2015 David Moreno Montero

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

#include <onion/onion.h>
#include <onion/url.h>
#include <onion/log.h>
#include <onion/response.h>
#include <onion/ptr_list.h>
#include <onion/poller.h>
#include <onion/low.h>
#include <onion/types_internal.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>

onion *o;

typedef struct reqres_t{
	onion_request *req;
	onion_response *res;
} reqres_t;

struct request_list_t{
	pthread_mutex_t lock;
	onion_ptr_list *reqres;
	bool running;
};

typedef struct request_list_t request_list_t;

void free_reqres(reqres_t *rr){
	onion_response_free(rr->res);
	onion_request_free(rr->req);
	onion_low_free(rr);
}

bool print_reqn(int *n, reqres_t *reqres){
	ONION_INFO("Writing %d to %p", *n, reqres->res);
	onion_response_printf(reqres->res, "%d\n", *n);
	int err=onion_response_flush(reqres->res);
	if (err<0){ // Manually close connection
		free_reqres(reqres);
		ONION_INFO("Closed connection.");
		return false;
	}
	return true;
}

void long_process(void *request_list_v){
	request_list_t *request_list=request_list_v;
	int n=0;
	while(request_list->running){
		n+=1;
		sleep(1);
		ONION_INFO("%d listeners", onion_ptr_list_count(request_list->reqres));
		pthread_mutex_lock(&request_list->lock);
		request_list->reqres=onion_ptr_list_filter(request_list->reqres, (void*)print_reqn, &n);
		pthread_mutex_unlock(&request_list->lock);
	}
}

onion_connection_status handler(void *request_list_v, onion_request *req, onion_response *res){
	// Some hello message
	onion_response_printf(res, "Starting poll\n");
	onion_response_flush(res);

	// Add it to the request_list lists.
	request_list_t *request_list=request_list_v;
	reqres_t *reqres=onion_low_malloc(sizeof(reqres_t));
	reqres->req=req;
	reqres->res=res;
	pthread_mutex_lock(&request_list->lock);
	request_list->reqres=onion_ptr_list_add(request_list->reqres, reqres);
	pthread_mutex_unlock(&request_list->lock);

	// Yield thread to the poller, ensures request and response are not freed.
	return OCS_YIELD;
}

int main(){
	request_list_t request_list;
	request_list.reqres=onion_ptr_list_new();
	request_list.running=true;
	pthread_mutex_init(&request_list.lock, NULL);

	pthread_t long_process_thread;

	o=onion_new(O_POOL);

	pthread_create(&long_process_thread, NULL, (void*)&long_process, &request_list);

	onion_set_root_handler(o, onion_handler_new( &handler, &request_list, NULL));

	ONION_INFO("Listening at http://localhost:8080");
	onion_listen(o);

	// Close
	request_list.running=false;

	onion_ptr_list_foreach(request_list.reqres, (void*)free_reqres);
	onion_ptr_list_free(request_list.reqres);

	pthread_join(long_process_thread, NULL);

	onion_free(o);
	return 0;
}
