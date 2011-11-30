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


#ifndef __OTERM_HANDLER__
#define __OTERM_HANDLER__

#include <onion/types.h>

#ifdef __cplusplus
extern "C"{
#endif

onion_handler *oterm_handler(onion *, const char *exec_command);
onion_connection_status oterm_uuid(void *_, onion_request *req, onion_response *res);

#ifdef __cplusplus
}
#endif


#endif
