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

#include <onion/log.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <curl/curl.h>

#include <onion/onion.h>

#include "../ctest.h"
#include <pthread.h>

const int MAX_TIME=120;

int processed;
pthread_mutex_t processed_mutex = PTHREAD_MUTEX_INITIALIZER;

volatile char okexit=0;
onion *o;
FILE *null_file=NULL;

typedef struct{
  float wait_s;
  float wait_t;
  int n_requests;
  char close_at_n;
}params_t;

onion_response_codes process_request(void *_, onion_request *req, onion_response *res){
  pthread_mutex_lock(&processed_mutex);
  processed++;
  pthread_mutex_unlock(&processed_mutex);
  onion_response_write0(res, "Done");
  
  return OCS_PROCESSED;
}

int curl_get(const char *url){
  if (!null_file)
    null_file=fopen("/dev/null","w");
  FAIL_IF(null_file == NULL);
  CURL *curl;
  curl = curl_easy_init();
  FAIL_IF_NOT(curl_easy_setopt(curl, CURLOPT_URL, url)==CURLE_OK);
  FAIL_IF_NOT(curl_easy_setopt(curl, CURLOPT_WRITEDATA, null_file)==CURLE_OK);
  CURLcode res=curl_easy_perform(curl);
  FAIL_IF_NOT_EQUAL((int)res,0);
  long int http_code;
  res=curl_easy_getinfo(curl, CURLINFO_HTTP_CODE, &http_code);
  FAIL_IF_NOT_EQUAL((int)res,0);
  char buffer[1024]; size_t l;
  curl_easy_recv(curl,buffer,sizeof(buffer),&l);
  curl_easy_cleanup(curl);
  FAIL_IF_NOT_EQUAL_INT((int)http_code, HTTP_OK);
  
  return http_code;
}

int curl_get_to_fail(const char *url){
  if (!null_file)
    null_file=fopen("/dev/null","w");
  FAIL_IF(null_file == NULL);
  CURL *curl;
  curl = curl_easy_init();
  FAIL_IF_NOT(curl_easy_setopt(curl, CURLOPT_URL, url)==CURLE_OK);
  FAIL_IF_NOT(curl_easy_setopt(curl, CURLOPT_WRITEDATA, null_file)==CURLE_OK);
  CURLcode res=curl_easy_perform(curl);
  FAIL_IF_EQUAL((int)res,0);
  long int http_code;
  res=curl_easy_getinfo(curl, CURLINFO_HTTP_CODE, &http_code);
  FAIL_IF_NOT_EQUAL((int)res,0);
  char buffer[1024]; size_t l;
  curl_easy_recv(curl,buffer,sizeof(buffer),&l);
  curl_easy_cleanup(curl);
  FAIL_IF_EQUAL_INT((int)http_code, HTTP_OK);
  
  return http_code;
}

void *do_requests(params_t *t){
  ONION_DEBUG("Do %d petitions",t->n_requests);
  int i;
  usleep(t->wait_s*1000000);
  for(i=0;i<t->n_requests;i++){
    curl_get("http://localhost:8080/");
    usleep(t->wait_t*1000000);
  }
  if (t->close_at_n)
    onion_listen_stop(o);
  
  return NULL;
}

void *watchdog(void *_){
  sleep(MAX_TIME);
  if (!okexit){
    ONION_ERROR("Error! Waiting too long, server must have deadlocked!");
    exit(1);
  }
  return NULL;
}

void do_petition_set_threaded(float wait_s, float wait_c, int nrequests, char close, int nthreads){
  ONION_DEBUG("Using %d threads, %d petitions per thread",nthreads,nrequests);
  processed=0;
  
  params_t params;
  params.wait_s=wait_s;
  params.wait_t=wait_c;
  params.n_requests=nrequests;
  params.close_at_n=close;
  
  pthread_t *thread=malloc(sizeof(pthread_t*)*nthreads);
  int i;
  for (i=0;i<nthreads;i++){
    pthread_create(&thread[i], NULL, (void*)do_requests, &params);
  }
  onion_listen(o);
  for (i=0;i<nthreads;i++){
    pthread_join(thread[i], NULL);
  }
  free(thread);
  
  FAIL_IF_NOT_EQUAL_INT(params.n_requests, processed);
}

void do_petition_set(float wait_s, float wait_c, int n_requests, char close){
  do_petition_set_threaded(wait_s, wait_c, n_requests, close, 1);
}

void t01_server_one(){
  INIT_LOCAL();
  
  o=onion_new(O_ONE);
  onion_set_root_handler(o,onion_handler_new((void*)process_request,NULL,NULL));
  do_petition_set(1,0.1,1,0);
  onion_free(o);
  
  o=onion_new(O_ONE_LOOP);
  onion_set_root_handler(o,onion_handler_new((void*)process_request,NULL,NULL));
  do_petition_set(1,0.1,1,1);
  onion_free(o);
  
  o=onion_new(O_ONE_LOOP);
  onion_set_root_handler(o,onion_handler_new((void*)process_request,NULL,NULL));
  do_petition_set(1,0.001,100,1);
  onion_free(o);
  
  END_LOCAL();
}

void t02_server_epoll(){
  INIT_LOCAL();
  
  o=onion_new(O_POLL);
  onion_set_root_handler(o,onion_handler_new((void*)process_request,NULL,NULL));
  do_petition_set(1,0.1,1,1);
  onion_free(o);

  o=onion_new(O_POLL);
  onion_set_root_handler(o,onion_handler_new((void*)process_request,NULL,NULL));
  do_petition_set(1,0.001,100,1);
  onion_free(o);

  o=onion_new(O_POOL);
  onion_set_root_handler(o,onion_handler_new((void*)process_request,NULL,NULL));
  do_petition_set(1,0.001,100,1);
  onion_free(o);

  o=onion_new(O_POOL);
  onion_set_root_handler(o,onion_handler_new((void*)process_request,NULL,NULL));
  do_petition_set_threaded(1,0.001,100,1,10);
  onion_free(o);

  o=onion_new(O_POOL);
  onion_set_root_handler(o,onion_handler_new((void*)process_request,NULL,NULL));
  do_petition_set_threaded(1,0.001,100,1,15);
  onion_free(o);

  o=onion_new(O_POOL);
  onion_set_root_handler(o,onion_handler_new((void*)process_request,NULL,NULL));
  do_petition_set_threaded(1,0.001,100,1,100);
  onion_free(o);

  END_LOCAL();
}

void t03_server_https(){
  INIT_LOCAL();
  
  o=onion_new(O_THREADED | O_DETACH_LISTEN);
  onion_set_root_handler(o,onion_handler_new((void*)process_request,NULL,NULL));
  FAIL_IF_NOT_EQUAL_INT(onion_set_certificate(o, O_SSL_CERTIFICATE_KEY, "mycert.pem", "mycert.pem"),0);
  onion_listen(o);
  //do_petition_set(1,1,1,1);
  sleep(1);
  FAIL_IF_EQUAL_INT(  curl_get_to_fail("http://localhost:8080"), HTTP_OK);
  sleep(1);
  FAIL_IF_NOT_EQUAL_INT(  curl_get("https://localhost:8080"), HTTP_OK);
  sleep(1);
  onion_free(o);
  
  END_LOCAL();
}

int main(int argc, char **argv){
  START();
  pthread_t watchdog_thread;
  pthread_create(&watchdog_thread, NULL, (void*)watchdog, NULL);
  
//  t01_server_one();
//  t02_server_epoll();
  t03_server_https();
  
  okexit=1;
  pthread_cancel(watchdog_thread);
  pthread_join(watchdog_thread,NULL);
  
  if (null_file)
    fclose(null_file);
  
	END();
}

