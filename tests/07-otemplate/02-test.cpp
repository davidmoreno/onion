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
#include <string.h>
#include <signal.h>

#include <onion/onion.h>
#include <onion/dict.h>
#include <onion/log.h>

#include <bindings/cpp/dict.hpp>
#include <bindings/cpp/onion.hpp>
#include <bindings/cpp/url.hpp>
#include <bindings/cpp/response.hpp>
#include <bindings/cpp/request.hpp>

#include <boost/bind.hpp>

extern "C"{
	onion_connection_status test_html_template(onion_dict *d, onion_request *req, onion_response *res);
	onion_connection_status extended_html_template(onion_dict *d, onion_request *req, onion_response *res);
}

onion_connection_status test_html_template(Onion::Dict &d, Onion::Request &req, Onion::Response &res){
	test_html_template(onion_dict_dup(d.c_handler()),req.c_handler(), res.c_handler());
}
onion_connection_status extended_html_template(Onion::Dict &d, Onion::Request &req, Onion::Response &res){
	extended_html_template(onion_dict_dup(d.c_handler()),req.c_handler(), res.c_handler());
}
onion_connection_status extended_html_template(Onion::Request &req, Onion::Response &res){
	extended_html_template(NULL,req.c_handler(), res.c_handler());
}

onion_connection_status test_page(Onion::Request &req, Onion::Response &res){
	Onion::Dict dict;
	
	char tmp[16];
	snprintf(tmp, sizeof(tmp), "%d", rand());
	
	dict.add("title", "Test page - works");
	dict.add("random", tmp);
	
	Onion::Dict subd;
	subd.add("0","Hello");
	subd.add("0","World!");
	dict.add("subd", subd);
	
	return test_html_template(dict, req, res);
}

onion_connection_status test2_page(Onion::Request &req, Onion::Response &res){
	Onion::Dict dict;
	
	char tmp[16];
	snprintf(tmp, sizeof(tmp), "%d", rand());
	
	dict.add("title", "Test page - works");
	dict.add("random", tmp);
	
	Onion::Dict subd;
	subd.add("0","Hello");
	subd.add("0","World!");
	dict.add(std::string("subd"), subd);

	
	return extended_html_template(dict, req, res);
}

Onion::Onion o(O_ONE_LOOP);


void stop(int){
	ONION_INFO("Stop");
	o.listenStop();
}

int main(int argc, char **argv){
	
	Onion::Url root(o);
	root.add("", "<a href=\"test1\">Test1</a><br><a href=\"test2\">Test2</a><br><a href=\"test3\">Test3</a>",HTTP_OK);
	root.add("test1", test_page);
	root.add("test2", test2_page);
	root.add("test3", extended_html_template);

	signal(SIGINT,stop);
	
	o.listen();
	
	return 0;
}
