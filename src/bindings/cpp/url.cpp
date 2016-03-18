/*
	Onion HTTP server library
	Copyright (C) 2010-2014 David Moreno Montero and othes

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

	You should have received a copy of both libraries, if not see 
	<http://www.gnu.org/licenses/> and 
	<http://www.apache.org/licenses/LICENSE-2.0>.
	*/

#include "url.hpp"
#include "request.hpp"
#include "response.hpp"

#ifndef ONION_HAS_LAMBDAS
void onion_url_no_free(onion_url*) {
	return;
}
#endif

Onion::Url::Url()
	: ptr { onion_url_new(), &onion_url_free }
{
}

Onion::Url::Url(onion_url* _ptr)
	: ptr { _ptr, &onion_url_free }
{
}

Onion::Url::Url(Onion* o)
#ifdef ONION_HAS_LAMBDAS
	: ptr { onion_root_url(o->c_handler()), [](onion_url*) -> void {} }
#else
	: ptr { onion_root_url(o->c_handler()), &onion_url_no_free }
#endif
{
}

Onion::Url::Url(Onion& o)
#ifdef ONION_HAS_LAMBDAS
	: ptr { onion_root_url(o.c_handler()), [](onion_url*) -> void {} }
#else
	: ptr { onion_root_url(o.c_handler()), &onion_url_no_free }
#endif
{
}

Onion::Url::Url(Url &&o)
	: ptr { o.ptr.get(), &onion_url_free }
{
#ifdef ONION_HAS_LAMBDAS
  o.ptr.get_deleter() = [](onion_url*) -> void {};
#else
  o.ptr.get_deleter() = &onion_url_no_free;
#endif
}

Onion::Url::~Url()
{
}

onion_url* Onion::Url::c_handler()
{
	return ptr.get();
}

Onion::Url& Onion::Url::add(const std::string& url, Handler &&h)
{
	onion_url_add_handler(ptr.get(), url.c_str(), onion_handler_cpp_to_c(std::move(h)));
	return *this;
}

Onion::Url& Onion::Url::add(const std::string& url, onion_handler *h)
{
	onion_url_add_handler(ptr.get(), url.c_str(), h);
	return *this;
}

Onion::Url& Onion::Url::add(const std::string& url, HandlerFunction::fn_t fn)
{
	add(url, static_cast<Handler&&>(Handler::make<HandlerFunction>(fn)));
	return *this;
}

Onion::Url& Onion::Url::add(const std::string &url, const std::string& s, int http_code)
{
	onion_url_add_static(ptr.get(), url.c_str(), s.c_str(), http_code);
	return *this;
}

Onion::Url& Onion::Url::add(const std::string& url, onion_handler_handler handler)
{
	return add(url, static_cast<Handler&&>(Handler::make<HandlerCFunction>(handler)));
}

//FIXME: Make sure this is correct
Onion::Url& Onion::Url::add(const std::string& url, Url url_handler)
{
	add(url, onion_url_to_handler(url_handler.c_handler()));
#ifdef ONION_HAS_LAMBDAS
	url_handler.ptr.get_deleter() = [](onion_url*) -> void {};
#else
	url_handler.ptr.get_deleter() = &onion_url_no_free;
#endif
	return *this;
}

onion_connection_status Onion::Url::operator()(::Onion::Request& req, ::Onion::Response& res)
{
	return onion_handler_handle(onion_url_to_handler(c_handler()), req.c_handler(), res.c_handler());
}

