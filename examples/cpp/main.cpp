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

#include <onion.hpp>
#include <response.hpp>
#include <url.hpp>

int main(int argc, char **argv){
	if (argc!=3){
		ONION_ERROR("%s <certificate file> <key file>", argv[0]);
		exit(1);
	}
	ONION_INFO("Listening at https://localhost:8080");
	
	Onion::Onion server(O_POOL);
	
	server.setCertificate(O_SSL_CERTIFICATE_KEY, argv[1], argv[2]);
	
	Onion::Url root(&server);
	
	root.add("", "Some static text", HTTP_OK );
	root.add("lambda", [](Onion::Request &req, Onion::Response &res){
		res<<"Lambda handler";
		return OCS_PROCESSED;
	});
	
	server.listen();
}
