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

#include "handler.h"
#include "types_internal.h"

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
		if (handler->handler){
			res=handler->handler(handler->priv_data, request);
			if (res)
				return res;
		}
		handler=handler->next;
	}
	return 0;
}


/** 
 * @short Creates an onion handler with that private datas.
 *
 */
onion_handler *onion_handler_new(onion_handler_handler handler, void *priv_data, onion_handler_private_data_free priv_data_free){
	onion_handler *phandler=malloc(sizeof(onion_handler));
	memset(phandler,0,sizeof(onion_handler));
	phandler->handler=handler;
	phandler->priv_data=priv_data;
	phandler->priv_data_free=priv_data_free;
	return phandler;
}

/**
 * @short Frees the memory used by this handler.
 *
 * It calls the private data handler free if available, and free the 'next' handler too.
 *
 * It should be called when this handler is not going to be used anymore. Most of the cases you
 * call it over the root handler, and from there it removes all the handlers.
 *
 * Returns the number of handlers freed on this level.
 */
int onion_handler_free(onion_handler *handler){
	int n=0;
	onion_handler *next=handler;
	while (next){
		handler=next;
		if (handler->priv_data_free && handler->priv_data){
			handler->priv_data_free(handler->priv_data);
		}
		next=handler->next;
		free(handler);
		n++;
	}
	return n;
}

/**
 * @short Adds a handler to the list of handlers of this level
 *
 * Adds a handler at the end of the list of handlers of this level. Each handler is called in order,
 * until one of them succeds. So each handler is in charge of cheching if its itself who is being called.
 */
void onion_handler_add(onion_handler *base, onion_handler *new_handler){
	while(base->next)
		base=base->next;
	base->next=new_handler;
}

/**
 * @short Returns the private data part of a handler
 * 
 * This is useful to allow external users of a given handler to modify the behaviour. For example
 * on the directory handler this helps to change the default header and footers.
 */
void *onion_handler_get_private_data(onion_handler *handler){
	return handler->priv_data;
}
