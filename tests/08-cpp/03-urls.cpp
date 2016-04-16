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

#include "../ctest.h"
#include "onion.hpp"
#include "url.hpp"
#include "request.hpp"
#include "response.hpp"
#include "shortcuts.hpp"

#ifndef ONION_HAS_LAMBDAS
onion_connection_status root_handler(Onion::Url& url, Onion::Request &req, Onion::Response &res) {
	if(req.query().has("go"))
		return url(req, res);
	res << "<html><body>Go to <a href=\"?go\">?go</a>";
	return OCS_PROCESSED;
}

onion_connection_status go(Onion::Request &req, Onion::Response &res) {
	res << "Index, now try <a href=\"/more/?go\">/more</a>";
	return OCS_PROCESSED;
}

onion_connection_status go_more(Onion::Url &more_url, Onion::Request& req, Onion::Response& res) {
	return more_url(req, res);
}

onion_connection_status more(Onion::Request &req, Onion::Response &res) {
	res << "more index, try <a href=\"/more/less?go\">Less</a>";
	return OCS_PROCESSED;
}

onion_connection_status less(Onion::Request &req, Onion::Response &res) {
	res << "More is less! (or was is <a href=\"/less/more?go\">less is more</a>?";
	return OCS_PROCESSED;
}
#endif

void t01_url(){
	INIT_LOCAL();
	
	Onion::Onion o;

	// Create an empty url handler. These are not directly used by onion, and must me freed.
	Onion::Url url;
	Onion::Url more_url;
	Onion::Url more_url2;
	
	// Set the main handler, does nothing if not called as ?go, if so, uses the url handler.
#ifdef ONION_HAS_LAMBDAS
	o.setRootHandler(Onion::Handler::make<Onion::HandlerFunction>([&url](Onion::Request &req, Onion::Response &res) -> onion_connection_status{
		if (req.query().has("go"))
			return url(req, res);
		res<<"<html><body>Go to <a href=\"?go\">?go</a>";
		return OCS_PROCESSED;
	}));
#else
	o.setRootHandler(Onion::Handler::make<Onion::HandlerFunction>(std::bind(&root_handler, std::ref(url), std::placeholders::_1, std::placeholders::_2)));
#endif


	// Populate the url handler a little bit.
#ifdef ONION_HAS_LAMBDAS
	url.add("", [](Onion::Request &req, Onion::Response &res){
		res<<"Index, now try <a href=\"/more/?go\">/more</a>";
		return OCS_PROCESSED;
	});
#else
	url.add("", std::bind(&go, std::placeholders::_1, std::placeholders::_2));
#endif
	
#ifdef ONION_HAS_LAMBDAS
	url.add("^more/", [&more_url](Onion::Request &req, Onion::Response &res){
		return more_url(req, res);
	});
#else
	url.add("^more/", std::bind(&go_more, std::ref(more_url), std::placeholders::_1, std::placeholders::_2));
#endif
	
#ifdef ONION_HAS_LAMBDAS
	more_url.add("", [](Onion::Request &req, Onion::Response &res){
		res<<"more index, try <a href=\"/more/less?go\">Less</a>";
		return OCS_PROCESSED;
	});
#else
	more_url.add("", std::bind(&more, std::placeholders::_1, std::placeholders::_2));
#endif

#ifdef ONION_HAS_LAMBDAS
	more_url.add("less", [](Onion::Request &req, Onion::Response &res){
		res<<"More is less! (or was is <a href=\"/less/more?go\">less is more</a>?)";
		return OCS_PROCESSED;
	});
#else
	more_url.add("less", std::bind(&less, std::placeholders::_1, std::placeholders::_2));
#endif
	
	url.add("^less/", std::move(more_url2));
	
	more_url2.add("more", std::string("Yes, the saying is that <a href=\"http://bit.ly/1fdCbP0\">Less is more</a>"));
	
	// Finally we get all guarded by at least the ?go and the following structure:
	// / (at index)
	// /more (at url -> more_url)
	// /more/less (at url -> more_url)
	// /less/more (at url -> more_url2)
	
	// from BUG #105. 
	auto onionlocalwebdir=Onion::ExportLocal(".");
  url.add("^webdir/", std::move(onionlocalwebdir));
	
	o.listen();
	
	END_LOCAL();
};

int main(int argc, char **argv){
  START();
	t01_url();
	
	END();
}
