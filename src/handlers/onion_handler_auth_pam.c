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

#include <string.h>
#include <malloc.h>
#include <unistd.h>

#include <security/pam_appl.h>
#include <security/pam_misc.h>

#include <onion_handler.h>
#include <onion_response.h>
//#include <onion_codecs.h>

int authorize(const char *pamname, const char *username, const char *password);


struct onion_handler_auth_pam_data_t{
	char *realm;
	char *pamname;
	onion_handler *inside;
};

typedef struct onion_handler_auth_pam_data_t onion_handler_auth_pam_data;

int onion_handler_auth_pam_handler(onion_handler_auth_pam_data *d, onion_request *request){
	//int l;
	char *auth=NULL;//onion_base64_decode(onion_request_get_header(request, "Authorization"), &l);
	
	if (auth){
		fprintf(stderr, "%s:%d auth %s\n",__FILE__,__LINE__,auth);
		
		
		
		return onion_handler_handle(d->inside, request);
	}
	
	// Not authorized. Ask for it.
	onion_response *res=onion_response_new(request);
	char temp[256];
	sprintf(temp, "Basic realm=\"%230s\"",d->realm);
	onion_response_set_header(res, "WWW-Authenticate",temp);
	onion_response_set_code(res, HTTP_UNAUTHORIZED);

	onion_response_write_headers(res);
	return 1;
}


void onion_handler_auth_pam_delete(onion_handler_auth_pam_data *d){
	free(d->pamname);
	free(d->realm);
	onion_handler_free(d->inside);
	free(d);
}

/**
 * @short Creates an path handler. If the path matches the regex, it reomves that from the regexp and goes to the inside_level.
 *
 * If on the inside level nobody answers, it just returns NULL, so ->next can answer.
 */
onion_handler *onion_handler_auth_pam(const char *realm, const char *pamname, onion_handler *inside_level){
	onion_handler_auth_pam_data *priv_data=malloc(sizeof(onion_handler_auth_pam_data));

	priv_data->inside=inside_level;
	priv_data->pamname=strdup(pamname);
	priv_data->realm=strdup(realm);
	
	onion_handler *ret=onion_handler_new((onion_handler_handler)onion_handler_auth_pam_handler,
																			 priv_data, (onion_handler_private_data_free) onion_handler_auth_pam_delete);
	return ret;
}

/// simple answer to the password question, needed by pam
static int authPAM_passwd(int num_msg, const struct pam_message **msg,
                struct pam_response **resp, void *appdata_ptr){
	//DEBUG("num messages %d",num_msg);
	//DEBUG("Question %s",(*msg)[0].msg);
	struct pam_response *r;
	
	*resp=r=(struct pam_response*)calloc(num_msg,sizeof(struct pam_response));
	if (r==NULL)
		return PAM_BUF_ERR;
	
	int i;
	
	for (i=0;i<num_msg;i++){
		r->resp=strdup((const char*)appdata_ptr);
		r->resp_retcode=0;
		r++;
	}

	//DEBUG("Done");
	return PAM_SUCCESS;
}


/**
 * @short Do the real authorization. Checks if access allowed
 */
int authorize(const char *pamname, const char *username, const char *password){
	int ok;
	pam_handle_t *pamh=NULL;
	char *password_local=strdup(password);
	struct pam_conv conv = {
    authPAM_passwd,
    (void*)password_local
	};
	//DEBUG("Password %s",password_local);
	
	//DEBUG("Auth on service '%s', username '%s'",pamname.toAscii().constData(), username.toUtf8().constData());
	
	ok=pam_start(pamname, username, &conv, &pamh);
	if (ok!=PAM_SUCCESS){
		//WARNING("Error initializing PAM");
		perror("PAM");
		return 0;
	}
	ok = pam_authenticate(pamh, 0);    /* is user really user? */
	if (ok!=PAM_SUCCESS){
		//DEBUG("Not an user. Auth failed.");
	}
	else
		ok = pam_acct_mgmt(pamh, 0);       /* permitted access? */
	
	pam_end(pamh, ok);
	if (ok==PAM_SUCCESS){
		//DEBUG("Authenticated user '%s'", username.toUtf8().constData());
		return 1;
	}
	//DEBUG("NOT authenticated user '%s'", username.toUtf8().constData());
	return 0;
}

