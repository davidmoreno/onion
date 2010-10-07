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

#include <malloc.h>
#include <string.h>

#include "onion_handler.h"

/**
 * @short Tryes to handle the petition with that handler.
 *
 * If can not, returns NULL.
 *
 * It checks this parser, and siblings.
 */
int onion_handler_handle(onion_handler *handler, onion_request *request){
	int res;
	while (handler){
	
		if (handler->parser){
			onion_parser=handler->parser;
			res=onion_parser(handler, request);
			if (res)
				return res;
		}
		
		handler=handler->next;
	}
	return 0;
}



