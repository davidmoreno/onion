/**
  Onion HTTP server library
  Copyright (C) 2010-2018 David Moreno Montero and others

  This library is free software; you can redistribute it and/or
  modify it under the terms of, at your choice:

  a. the Apache License Version 2.0.

  b. the GNU General Public License as published by the
  Free Software Foundation; either version 2.0 of the License,
  or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of both licenses, if not see
  <http://www.gnu.org/licenses/> and
  <http://www.apache.org/licenses/LICENSE-2.0>.
*/

#include <stdio.h>
#include <signal.h>

#include <onion/log.h>
#include <onion/onion.h>
#include <onion/websocket.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

#include <unistd.h>

onion *o = NULL;
int onion_run = 1;

#define MAX_ACTIVE_CLIENTS 2

enum onion_websocket_status_e {
    NORMAL_CLOSURE              = 1000,
    GOING_AWAY                  = 1001,
    PROTOCOL_ERROR              = 1002,
    UNSUPPORTED_DATA            = 1003,
    //	RESERVED                = 1004,
    NO_STATUS_RECEIVED          = 1005,
    ABNORMAL_CLOSURE            = 1006,
    INVALID_FRAME_PAYLOAD_DATA  = 1007,
    POLICY_VIOLATION            = 1008,
    MESSAGE_TOO_BIG             = 1009,
    MISSING_EXTENSION           = 1010,
    INTERNAL_ERROR              = 1011,
    SERVICE_RESTART             = 1012,
    TRY_AGAIN_LATER             = 1013,
    BAD_GATEWAY                 = 1014,
    TLS_HANDSHAKE               = 1015,
};
// convert into char[2] e.g.: STATUS(sname, NORMAL_CLOSURE)
#define STATUS_STR(NAME, NUM) char NAME[2] \
    = {((NUM>>8)&0xFF) , ((NUM)&0xFF)};

onion_websocket *ws_active[MAX_ACTIVE_CLIENTS] = { 0 };
pthread_mutex_t ws_lock[MAX_ACTIVE_CLIENTS];

void onion_end_signal(int unused) {
    if( o ){
      onion_run = 0;
      ONION_INFO("Closing connections");

      // Note: Open websocket blocking here.
      // Using list of client to force disconnection
      int i;
      for( i=0; i<MAX_ACTIVE_CLIENTS; ++i){
        if ( ws_active[i] ){
            ONION_INFO("Close websocket");
            pthread_mutex_lock(&ws_lock[i]);
            STATUS_STR(byebye, NORMAL_CLOSURE);
            onion_websocket_close(ws_active[i], byebye);
            ws_active[i] = NULL;
            pthread_mutex_unlock(&ws_lock[i]);
        }
    }


      onion_listen_stop(o);
      //exit(EXIT_SUCCESS); // bad because exit() is not signal save
    }
}

onion_connection_status websocket_example_cont(void *data, onion_websocket * ws,
                                               ssize_t data_ready_len);

onion_connection_status websocket_example(void *data, onion_request * req,
                                          onion_response * res) {

  onion_websocket *ws = onion_websocket_new(req, res);
  if (!ws) {
    onion_response_write0(res,
                          "<html><body><h1>Easy echo</h1><pre id=\"chat\"></pre>"
                          " <script>\ninit = function(){\nmsg=document.getElementById('msg');\nmsg.focus();\n\nws=new WebSocket('ws://'+window.location.host);\nws.onmessage=function(ev){\n var el = document.getElementById('chat'); old = el.textContent.split('\\n').slice(-10).join('\\n'); el.textContent = old + ev.data+'\\n';\n};}\n"
                          "window.addEventListener('load', init, false);\n</script>"
                          "<input type=\"text\" id=\"msg\" oninput=\"javascript:ws.send(msg.value); \"/><br/>\n"
                          "<button onclick='ws.close(1000);'>Close connection</button>"
                          "</body></html>");

    return OCS_PROCESSED;
  }

  // Store in array of active clients
  int n;
  for( n=0;n<MAX_ACTIVE_CLIENTS;++n){
    if (ws_active[n] == NULL){
      ws_active[n] = ws;
      break;
    }
  }

  if (n >= MAX_ACTIVE_CLIENTS){
      ONION_ERROR("All %d websocket(s) already in use. Close connection to client...",
              MAX_ACTIVE_CLIENTS);
      onion_websocket_printf(ws, "Hello. Maximum of clients %d already reached. Close connection...", MAX_ACTIVE_CLIENTS);
      onion_websocket_set_callback(ws, NULL);  //redundart due _close call?!

      STATUS_STR(byebye, NORMAL_CLOSURE);
      onion_websocket_close(ws, byebye);
      return OCS_WEBSOCKET;
  }

  // oninit Event
  pthread_mutex_lock(&ws_lock[n]);
  onion_websocket_printf(ws, "Hello from server. Write something to echo it");
  onion_websocket_set_callback(ws_active[n], websocket_example_cont);
  pthread_mutex_unlock(&ws_lock[n]);

  // ==============================================
  // =========== test onion_websocket_printf ======
  // ==============================================
  // Show bug of onion_websocket_printf
  char *long_string = calloc(1025, sizeof(char));
  memset(long_string, 'x', 1024);
  // Now printf which consumes more than 512 bytes
  onion_websocket_printf(ws, "Long string: %s", long_string);
  free(long_string);

  // ==============================================

  return OCS_WEBSOCKET;
}

onion_connection_status websocket_example_cont(void *data, onion_websocket * ws,
                                               ssize_t data_ready_len) {
  char tmp[256];
  if (data_ready_len > sizeof(tmp))
    data_ready_len = sizeof(tmp) - 1;

  // Detect closing
  int opcode = onion_websocket_get_opcode(ws);
  //ONION_INFO("Close? %d", (opcode == OWS_CONNECTION_CLOSE) );
  if( opcode == OWS_CONNECTION_CLOSE ){
      int i;
      for( i=0; i<MAX_ACTIVE_CLIENTS; ++i){
          if( ws == ws_active[i] ){
              ws_active[i] = NULL;
          }
      }
    return OCS_PROCESSED;
  }

  int len = onion_websocket_read(ws, tmp, data_ready_len);
  if (len <= 0) {
    ONION_ERROR("Error reading data: %d: %s (%d)", errno, strerror(errno),
                data_ready_len);
    return OCS_NEED_MORE_DATA;
  }
  tmp[len] = 0;

  int n;
  // reply to this client
  for (n=0;n<MAX_ACTIVE_CLIENTS; ++n){
    if (ws_active[n] && ws_active[n] == ws ){
      ONION_INFO("Send echo to source client...");

      pthread_mutex_lock(&ws_lock[n]);
      onion_websocket_printf(ws, "Echo: %s", tmp);
      pthread_mutex_unlock(&ws_lock[n]);
    }
  }
#if 0
  // send data to other clients
  for (n=0;n<MAX_ACTIVE_CLIENTS; ++n){
    if (ws_active[n] && ws_active[n] != ws ){
      ONION_INFO("Send echo to other clients...");

      pthread_mutex_lock(&ws_lock[n]);
      onion_websocket_printf(ws_active[n], "Other: %s", tmp);
      pthread_mutex_unlock(&ws_lock[n]);
    }
  }
#endif

  ONION_INFO("Read from websocket: %d: %s", len, tmp);

  return OCS_NEED_MORE_DATA;
}

int main() {

  int n;
  for( n=0;n<MAX_ACTIVE_CLIENTS;++n){
    ws_active[n] = NULL;
    pthread_mutex_init(&ws_lock[n], NULL);
  }

  o = onion_new(O_THREADED|O_NO_SIGTERM|O_DETACH_LISTEN);
  int port = 9000;
  onion_set_port(o, "9000");
  ONION_INFO("Using port %d", port);

  onion_url *urls = onion_root_url(o);

  onion_url_add(urls, "", websocket_example);

  signal(SIGINT, onion_end_signal);
  signal(SIGTERM, onion_end_signal);

  onion_listen(o);

  while( onion_run ){
    // Push periodical some info to connected clients
    // Use mutex to prevent mixup with other threads.

    int n;
    for( n=0;n<MAX_ACTIVE_CLIENTS;++n){
      if (ws_active[n] != NULL){
        pthread_mutex_lock(&ws_lock[n]);
        onion_websocket_printf(ws_active[n], "%s", ".");
        pthread_mutex_unlock(&ws_lock[n]);
      }
    }

    sleep(5);
  }

  onion_free(o);
  return 0;
}
