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
#ifndef ONION_LISTEN_POINT_HPP
#define ONION_LISTEN_POINT_HPP

#include <onion/listen_point.h>
#include <memory>

namespace Onion {
	/**
	 * @short Creates a listen point for an Onion::Onion object.
	 * Not meant to be used directly, as a default listen point doesn't
	 * do very much.
	 */
	class ListenPoint {
	protected:
		using internal_pointer = std::unique_ptr<onion_listen_point, decltype(onion_listen_point_free)*>;
		internal_pointer ptr;
	public:
		ListenPoint() 
			: ptr(onion_listen_point_new(), onion_listen_point_free)
		{}

		ListenPoint(onion_listen_point* lp)
			: ptr(lp, [](onion_listen_point*) { return; })
		{}

		ListenPoint(ListenPoint&& other) : ptr { std::move(other.ptr) }
		{}

		/**
		 * @short Release ownership of the underlying pointer.
		 */
		void release() {
			ptr.reset(nullptr);
		}

		/**
		 * @short Get the underlying C object.
		 */
		onion_listen_point* c_handler() const { return ptr.get(); }
	};
}

#endif

