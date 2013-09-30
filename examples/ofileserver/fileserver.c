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

#include <signal.h>
#include <string.h>
#include <stdio.h>

#include <onion/onion.h>
#include <onion/log.h>
#include <onion/dict.h>
#include <dirent.h>
#include <sys/stat.h>
#include <onion/shortcuts.h>
#include <locale.h>
#include <libintl.h>
#include <onion/handlers/webdav.h>
#include <onion/handlers/exportlocal.h>

onion *o=NULL;

void free_onion(int unused){
	ONION_INFO("Closing connections");
	onion_free(o);
	exit(0);
}

int show_help(){
	printf("Onion basic fileserver (otemplate edition). (C) 2011 Coralbits S.L.\n\n"
				"Usage: fileserver [options] [directory to serve]\n\n"
				"Options:\n"
				"  --port N\n" 
				"   -p N           Listens at given port. Default 8080\n"
				"  --listen ADDRESS\n"
				"   -l ADDRESS     Listen to that address. It can be a IPv4 or IPv6 IP, or a local host name. Default: 0.0.0.0\n"
				"  --help\n"
				"   -h             Shows this help\n"
#ifdef HAVE_WEBDAV
				"  --no-webdav     Disables webdav support\n"
#endif
				"\n"
				"\n");
	return 0;
}

int fileserver_page(const char *basepath, onion_request *req, onion_response *res);
int fileserver_html_template(onion_dict *context, onion_request *req, onion_response *res);

int main(int argc, char **argv){
	//onion_log=onion_log_syslog;
	char *port="8080";
	char *hostname="::";
	const char *dirname=".";
#ifdef HAVE_WEBDAV
	int withwebdav=1;
#endif
	int i;
	for (i=1;i<argc;i++){
		if ((strcmp(argv[i],"--port")==0) || (strcmp(argv[i],"-p")==0)){
			port=argv[++i];
			ONION_INFO("Listening at port %s",port);
		}
		if ((strcmp(argv[i],"--listen")==0) || (strcmp(argv[i],"-l")==0)){
			hostname=argv[++i];
			ONION_INFO("Listening at hostname %s",hostname);
		}
		else if (strcmp(argv[i],"--help")==0 || strcmp(argv[i],"-h")==0){
			return show_help();
		}
#ifdef HAVE_WEBDAV
		else if (strcmp(argv[i],"--no-webdav")==0){
			ONION_INFO("WebDAV support disabled");
			withwebdav=0;
		}
#endif
		else{
			dirname=argv[i];
			ONION_INFO("Exporting directory %s", dirname);
		}
	}
	
	onion_handler *root=onion_handler_new((onion_handler_handler)fileserver_page, (void *)dirname, NULL);
#ifdef HAVE_WEBDAV
	if (withwebdav)
		onion_handler_add(root, onion_handler_webdav(dirname, NULL)); // fallback.
	else
#endif
		onion_handler_add(root, onion_handler_export_local_new(dirname));
		
// This is the root directory where the translations are.
#define W "."
	setenv("LANGUAGE","locale",1); // Remove LANGUAGE env var, set it to the locale name,
	setlocale(LC_ALL,""); 
	bindtextdomain("locale", W); // This is necesary because of the fake name
	bindtextdomain("es", W); // One per language supported.
	bindtextdomain("zh", W);
	bindtextdomain("fr", W);
	bindtextdomain("pl", W);
	textdomain("C"); // Default language
  // All is configured now, now in hands of dgettext(LANG, txt);
	
	o=onion_new(O_POOL);

	onion_set_root_handler(o, root);
	onion_set_port(o, port);
	onion_set_hostname(o, hostname);
	
	signal(SIGINT, free_onion);
	int error=onion_listen(o);
	if (error){
		perror("Cant create the server");
	}
	
	onion_free(o);
	 
	return 0;
}

/**
 * @short Serves a directory listing.
 * 
 * It checks if the given request is a directory listing and processes it, or fallbacks to the
 * next handler.
 */
int fileserver_page(const char *basepath, onion_request *req, onion_response *res){
	if ((onion_request_get_flags(req)&OR_METHODS) == OR_GET){ // only get.
		const char *path=onion_request_get_path(req); // Try to get the real path, and check if its a dir
		
		char dirname[256];
		snprintf(dirname, sizeof(dirname), "%s/%s", basepath, onion_request_get_path(req));
		char *realp=realpath(dirname, NULL);
		if (!realp)
			return OCS_INTERNAL_ERROR;
		
		DIR *dir=opendir(realp);
		if (dir){ // its a dir, fill the dictionary.
			onion_dict *d=onion_dict_new();
			onion_dict_add(d, "dirname", path, 0);
			if (path[0]!='\0' && path[1]!='\0')
				onion_dict_add(d, "go_up", "true", 0);
			onion_dict *files=onion_dict_new();
			onion_dict_add(d, "files", files, OD_DICT|OD_FREE_VALUE);
			
			struct dirent *de;
			while ( (de=readdir(dir)) ){ // Fill one files.[filename] per file.
				onion_dict *file=onion_dict_new();
				onion_dict_add(files, de->d_name, file, OD_DUP_KEY|OD_DICT|OD_FREE_VALUE);
				
				onion_dict_add(file, "name", de->d_name, OD_DUP_VALUE);

				char tmp[256];
				snprintf(tmp, sizeof(tmp), "%s/%s", realp, de->d_name);
				struct stat st;
				stat(tmp, &st);
			
				snprintf(tmp, sizeof(tmp), "%d", (int)st.st_size);
				onion_dict_add(file, "size", tmp, OD_DUP_VALUE);
				
				snprintf(tmp, sizeof(tmp), "%d", st.st_uid);
				onion_dict_add(file, "owner", tmp, OD_DUP_VALUE);
				
				if (S_ISDIR(st.st_mode))
					onion_dict_add(file, "type", "dir", 0);
				else
					onion_dict_add(file, "type", "file", 0);
			}
			closedir(dir);
			free(realp);
			
			return fileserver_html_template(d, req, res);
		}
		free(realp);
	}
	return OCS_NOT_PROCESSED;
}
