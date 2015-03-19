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
#include <functional>
#include <iostream>

#include "../ctest.h"

#include <bindings/cpp/onion.hpp>
#include <bindings/cpp/response.hpp>
#include <bindings/cpp/url.hpp>
#include <bindings/cpp/request.hpp>

class DefaultHandler : public Onion::Handler{
  onion_connection_status operator()(Onion::Request &req, Onion::Response &res){
    res<<std::endl<<"World "<<std::hex<<255<<"\n";
    return OCS_PROCESSED;
  }
};

onion_connection_status handler(Onion::Request &req, Onion::Response &res){
  res<<"<html><h1>Hello world from handler.</h1><ul><li><a href='/static'>Static</a><li><a href='/m'>Handler class</a><li><a href='/m-error'>Error</a>"<<std::endl;
  return OCS_PROCESSED;
}

class MyHandler{
  int n;
public:
  MyHandler(){ n=rand(); }
  onion_connection_status index(Onion::Request &req, Onion::Response &res){
    res<<"index "<<n<<std::endl;
		return OCS_PROCESSED;
  }
  onion_connection_status error(Onion::Request &req, Onion::Response &res){
    res<<"error "<<n<<std::endl;
		return OCS_PROCESSED;
  }
};

void t01_basic(){
  INIT_LOCAL()
  
  Onion::Onion o;
  MyHandler m;

  Onion::Url url(o);
  
  url.add("", handler);
  url.add("static", std::string("Static data"));
  url.add<MyHandler>("m", &m, &MyHandler::index);
  
  o.setInternalErrorHandler( Onion::Handler::make<Onion::HandlerMethod<MyHandler>>(&m, &MyHandler::error) );
  
  o.listen();
  
  END_LOCAL()
}

int main(int argc, char **argv){
  START();
  t01_basic();

  
  END();
}
