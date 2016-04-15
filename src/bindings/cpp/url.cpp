/*
	Onion HTTP server library
	Copyright (C) 2010-2016 David Moreno Montero and others

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

#include "url.hpp"
#include "request.hpp"
#include "response.hpp"

Onion::Url::Url()
	: Handler { new HandlerMethod<Url>(this, &::Onion::Url::operator()) }, ptr { onion_url_new(), &onion_url_free }
{
}

Onion::Url::Url(onion_url* _ptr)
	: Handler { new HandlerMethod<Url>(this, &::Onion::Url::operator()) }, ptr { _ptr, &onion_url_free }
{
}

Onion::Url::Url(Onion* o)
	: Handler { new HandlerMethod<Url>(this, &::Onion::Url::operator()) }, ptr { onion_root_url(o->c_handler()), [](onion_url*) -> void {} }
{
}

Onion::Url::Url(Onion& o)
	: Handler{ new HandlerMethod<Url>(this, &::Onion::Url::operator()) }, ptr { onion_root_url(o.c_handler()), [](onion_url*) -> void {} }
{
}

Onion::Url::Url(Url &&o)
	: Handler { new HandlerMethod<Url>(this, &::Onion::Url::operator()) }, ptr { o.ptr.get(), &onion_url_free }
{
	o.ptr.get_deleter() = [](onion_url*) -> void {};
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

onion_connection_status Onion::Url::operator()(::Onion::Request& req, ::Onion::Response& res)
{
	return onion_handler_handle(onion_url_to_handler(c_handler()), req.c_handler(), res.c_handler());
}

