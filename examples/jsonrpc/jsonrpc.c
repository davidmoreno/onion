/** Licensed under Apache 2.0. (C) 2014 David Moreno Montero. http://coralbits.com */

#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#include <onion/onion.h>
#include <onion/dict.h>
#include <onion/block.h>
#include <onion/request.h>
#include <onion/response.h>
#include <onion/url.h>
#include <onion/low.h>

/// Same as strcmp, but also checks for NULL values.
bool safe_strcmp(const char *a, const char *b){
	if (!a && !b)
		return true;
	if (!a || !b)
		return false;
	return strcmp(a,b)==0;
}

onion_connection_status strip_rpc(void *_, onion_request *req, onion_response *res){
	const onion_block *dreq=onion_request_get_data(req);
	onion_dict *jreq=NULL;
	if (dreq)
		jreq=onion_dict_from_json( onion_block_data( dreq ) );
	if (jreq){ // safe_strcmp(onion_request_get_header(req, "content-type"), "application/json") // Not used in tinyrpc, not really mandatory
		//onion_dict_print_dot(jreq);
		
		/// Check is the proper call.
		if (!safe_strcmp(onion_dict_get(jreq,"jsonrpc"), "2.0")){
			onion_response_write0(res, "{\"jsonrpc\": \"2.0\", \"error\": {\"code\": -32700, \"message\": \"Parse error\"}, \"id\": null}");
			return OCS_PROCESSED;
		}
		if (!safe_strcmp(onion_dict_get(jreq,"method"), "strip")){
			onion_response_printf(res, "{\"jsonrpc\": \"2.0\", \"error\": {\"code\": -32601, \"message\": \"Method not found\"}, \"id\": \"%s\"}", 
					onion_dict_get(jreq,"id")
				);
			return OCS_PROCESSED;
		}
		char *str=onion_low_strdup( onion_dict_rget(jreq,"params", "str", NULL) );

		/// check real start and end. To prepare to write back the stripped string.
		int start=0, end=strlen(str);
		while(start<end && isspace(str[start])) start++;
		end--;
		while(start<end && isspace(str[end])) end--;
		end++;
		str[end]=0;
		
		/// Prepare message
		onion_dict *jres=onion_dict_new();
		onion_dict_add(jres, "jsonrpc", "2.0", 0);
		onion_dict_add(jres, "result", str+start, OD_DUP_VALUE);
		onion_dict_add(jres, "id", onion_dict_get(jreq, "id"), 0);
		
		/// Write
		onion_block *jresb=onion_dict_to_json(jres);
		onion_response_write( res,  onion_block_data( jresb ), onion_block_size( jresb ) );
		
		
		/// Clean up.
		onion_block_free(jresb);
		onion_dict_free(jres);
		onion_low_free(str);
		onion_dict_free(jreq);
		
		return OCS_PROCESSED;
	}
	else{
		onion_response_write0(res, "This is a JSON rpc service. Please send jsonrpc requests.");
		
		return OCS_PROCESSED;
	}
}

int main(void){
	onion *o=onion_new(0);
	
	onion_url *url=onion_root_url(o);
	onion_url_add(url, "", strip_rpc);
	
	onion_listen(o);
	return 0;
}
