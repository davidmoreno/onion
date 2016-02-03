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

#ifndef ONION_MIME_HPP
#define ONION_MIME_HPP

#include <onion/mime.h>
#include "dict.hpp"

namespace Onion {
	class Mime {
	public:
		/**
		 * @short Sets the mime dictionary (extension -> mime_type)
		 */
		static void set(Onion::Dict& dict) {
			onion_mime_set(dict.c_handler());
		}

		/**
		 * @short Get the mime type based on the file name.
		 */
		static std::string get(const std::string& filename) {
			return onion_mime_get(filename.c_str());
		}

		/**
		 * @short Update a mime record.
		 */
		static void update(const std::string& extension, const std::string& mimetype) {
			onion_mime_update(extension.c_str(), mimetype.c_str());
		}

		/**
		 * @short Removes a mime record for the given extension.
		 */
		static void remove(const std::string& extension) {
			onion_mime_update(extension.c_str(), nullptr);
		}
	};
}

#endif

