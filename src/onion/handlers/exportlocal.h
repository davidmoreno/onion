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

#ifndef __ONION_HANDLER_EXPORT_LOCAL__
#define __ONION_HANDLER_EXPORT_LOCAL__

#include <onion/types.h>

#ifdef __cplusplus
extern "C"{
#endif

/// Creates an export local handler. When path matches, it returns a file from localpath (final localpath + path). No dir listing.
onion_handler *onion_handler_export_local_new(const char *localpath);

/// Calls to render a header after the "Listing of directory..."
void onion_handler_export_local_set_header(onion_handler *dir, void (*renderer)(onion_response *res, const char *dirname));
/// Calls to render a footers before end.
void onion_handler_export_local_set_footer(onion_handler *dir, void (*renderer)(onion_response *res, const char *dirname));


#ifdef __cplusplus
}
#endif

#endif
