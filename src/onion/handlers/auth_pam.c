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

#include <onion/handler.h>
#include <onion/response.h>
#include <onion/codecs.h>
#include <onion/log.h>

int authorize(const char *pamname, const char *username, const char *password);

#define RESPONSE_UNAUTHORIZED "<h1>Unauthorized access</h1>"

struct onion_handler_auth_pam_data_t{
	char *realm;
	char *pamname;
	onion_handler *inside;
};

typedef struct onion_handler_auth_pam_data_t onion_handler_auth_pam_data;

int onion_handler_auth_pam_handler(onion_handler_auth_pam_data *d, onion_request *request, onion_response *res){
	const char *o=onion_request_get_header(request, "Authorization");
	char *auth=NULL;
	char *username=NULL;
	char *passwd=NULL;
	if (o && strncmp(o,"Basic",5)==0){
		//fprintf(stderr,"auth: '%s'\n",&o[6]);
		auth=onion_base64_decode(&o[6], NULL);
		username=auth;
		int i=0;
		while (auth[i]!='\0' && auth[i]!=':') i++;
		if (auth[i]==':'){
			auth[i]='\0'; // so i have user ready
			passwd=&auth[i+1];
		}
		else
			passwd=NULL;
	}
	
	// I have my data, try to authorize
	if (username && passwd){
		int ok=authorize(d->pamname, username, passwd);
		
		if (ok){
			free(auth);
			return onion_handler_handle(d->inside, request, res);
		}
	}
	if (auth)
		free(auth);

	
	// Not authorized. Ask for it.
	char temp[256];
	sprintf(temp, "Basic realm=\"%230s\"",d->realm);
	onion_response_set_header(res, "WWW-Authenticate",temp);
	onion_response_set_code(res, HTTP_UNAUTHORIZED);
	onion_response_set_length(res,sizeof(RESPONSE_UNAUTHORIZED));
	
	onion_response_write(res,RESPONSE_UNAUTHORIZED,sizeof(RESPONSE_UNAUTHORIZED));
	return OCS_PROCESSED;
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
	ONION_DEBUG0("Num messages %d",num_msg);
	struct pam_response *r;
	
	*resp=r=(struct pam_response*)calloc(num_msg,sizeof(struct pam_response));
	if (r==NULL)
		return PAM_BUF_ERR;
	
	int i;
	
	for (i=0;i<num_msg;i++){
		ONION_DEBUG0("Question %d: %s",i, (*msg)[i].msg);
		r->resp=strdup((const char*)appdata_ptr);
		r->resp_retcode=0;
		r++;
	}

	return PAM_SUCCESS;
}


/**
 * @short Do the real authorization. Checks if access allowed
 */
int authorize(const char *pamname, const char *username, const char *password){
	int ok;
	pam_handle_t *pamh=NULL;
	
	const char *password_local=password; //strdup(password);
	struct pam_conv conv = {
    authPAM_passwd,
    (void*)password_local
	};
	
	ok=pam_start(pamname, username, &conv, &pamh);
	
	if (ok==PAM_SUCCESS)
		ok = pam_authenticate(pamh, 0);    /* is user really user? */
	if (ok==PAM_SUCCESS)
		ok = pam_acct_mgmt(pamh, 0);       /* permitted access? */
	
	if (pam_end(pamh, ok)!=PAM_SUCCESS){
		ONION_ERROR("Error releasing PAM structures");
	}
	if (ok==PAM_SUCCESS){
		ONION_DEBUG("Authenticated user %s OK", username);
		return 1;
	}
	ONION_WARNING("NOT authenticated user '%s', code %d", username, ok);
	return 0;
}

