/*
  Onion HTTP server library
  Copyright (C) 2010-2012 David Moreno Montero

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 3.0 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not see <http://www.gnu.org/licenses/>.
  */

#include "handler.hpp"
#include "request.hpp"
#include "response.hpp"

static int onion_handler_call_operator(void *ptr, onion_request *_req, onion_response *_res){
  try{
    Onion::Handler *handler=(Onion::Handler*)ptr;
    Onion::Request req(_req);
    Onion::Response res(_res);
    return (*handler)(req, res);
  }
  catch(const std::exception &e){
    ONION_ERROR("Catched exception: %s", e.what());
    return OCS_INTERNAL_ERROR;
  }
}

static void onion_handler_delete_operator(void *ptr){
  Onion::Handler *handler=(Onion::Handler*)ptr;
  delete handler;
}

Onion::Handler::Handler()
{
  ptr=onion_handler_new(onion_handler_call_operator, this, onion_handler_delete_operator);
}

Onion::Handler::~Handler()
{

}

