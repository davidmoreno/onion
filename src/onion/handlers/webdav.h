/*
	Onion HTTP server library
	Copyright (C) 2010 David Moreno Montero

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


#ifndef __WEBDAV_H__
#define __WEBDAV_H__
#include <onion/handler.h>

/// Typedef with the signature of the permission checker.
typedef int (*onion_webdav_permissions_check)(const char *exported_path, const char *file, onion_request *req);

/// Exports the given path through webdav.
onion_handler *onion_handler_webdav(const char *path, onion_webdav_permissions_check);

/// Default permission checker
int onion_webdav_default_check_permissions(const char *exported_path, const char *file, onion_request *req);

#endif
