/*
    Onion HTTP server library
    Copyright (C) 2010-2011 David Moreno Montero

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

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <errno.h>
#include <stdbool.h>

#include <onion/onion.h>
#include <onion/log.h>
#include "../ctest.h"

static onion * t01_server;
static size_t t01_errors_count;
static bool t01_failed;

void t01_listen_port_error_handler(onion_log_level level, const char *filename, int lineno, const char *fmt, ...) {
	if (level == O_ERROR && (errno == EBADF || errno == EDOTDOT)){
		if (!t01_failed) {
			t01_failed = true;
			onion_listen_stop(t01_server);
		}
	}
}

void t01_listen_port() {
	INIT_LOCAL();
	
	if (!geteuid()) {
		// current user is root
		// set user to nobody
		
		struct passwd * pwd = calloc(1, sizeof(struct passwd));
		FAIL_IF_NOT(pwd);
		size_t buffer_length = sysconf(_SC_GETPW_R_SIZE_MAX);
		FAIL_IF_NOT(buffer_length > 0);
		char * buffer = malloc(buffer_length * sizeof(char));
		FAIL_IF_NOT(buffer);
		int lookup_result = getpwnam_r("nobody", pwd, buffer, buffer_length, &pwd);
		FAIL_IF(lookup_result);
		FAIL_IF_NOT(pwd);
		int setuid_result = setuid(pwd->pw_uid);
		FAIL_IF(setuid_result);
		free(pwd);
		free(buffer);
	}
	// current user is not root
	// it has no permissions to bind to port 88
	
	t01_server=onion_new(O_THREADED);
	onion_set_max_threads(t01_server, 2);
	t01_errors_count=0;
	t01_failed=false;
	onion_log=t01_listen_port_error_handler;
	onion_set_port(t01_server, "88");
	onion_listen(t01_server);
	onion_free(t01_server);
	FAIL_IF(t01_failed);
	
	END_LOCAL();
}

int main(int argc, char **argv){
	START();
    
	t01_listen_port();
    
	END();
}
