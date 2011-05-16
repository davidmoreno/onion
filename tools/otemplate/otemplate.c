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

#define _GNU_SOURCE
#include <string.h>
#include <ctype.h>
#include <libgen.h>
#include <stdarg.h>
#include <malloc.h>

#include "onion/log.h"
#include "onion/block.h"
#include "list.h"
#include "parser.h"
#include "tags.h"

list *plugin_search_path;

int work(const char *infilename, const char *outfilename);

void help(const char *msg);

int main(int argc, char **argv){
	// Add some plugin searhc paths
	plugin_search_path=list_new(NULL);

	const char *infilename=NULL;
	const char *outfilename=NULL;
	char tmp[256];

	int i;
	for (i=1;i<argc;i++){
		if (strcmp(argv[i], "--help")==0){
			help(NULL);
			return 0;
		}
		else if ((strcmp(argv[i], "--templatetagsdir")==0) || (strcmp(argv[i], "-t")==0)){
			i++;
			if (argc<=i){
				help("Missing templatedir name");
				return 3;
			}
			snprintf(tmp, sizeof(tmp), "%s/lib%%s.so", strdupa(argv[i]));
			ONION_DEBUG("Added templatedir %s", tmp);
			list_add(plugin_search_path, strdupa(tmp)); // dupa is ok, as im at main.
		}
		else{
			if (infilename){
				if (outfilename){
					help("Too many arguments");
					return 1;
				}
				outfilename=argv[i];
				ONION_DEBUG("Set outfilename %s", outfilename);
			}
			else{
				infilename=argv[i];
				ONION_DEBUG("Set infilename %s", infilename);
			}
		}
	}
	
	if (!infilename || !outfilename){
		help("Missing input or output filename");
		return 2;
	}

	if (strcmp(infilename,"-")==0){
		infilename="";
	}
	else{
		snprintf(tmp, sizeof(tmp), "%s/lib%%s.so", dirname(strdupa(argv[1])));
		list_add(plugin_search_path, strdupa(tmp));
		snprintf(tmp, sizeof(tmp), "%s/templatetags/lib%%s.so", dirname(strdupa(argv[1])));
		list_add(plugin_search_path, strdupa(tmp));
	}

	// Default template dirs
	list_add(plugin_search_path, "lib%s.so");
	list_add(plugin_search_path, "templatetags/lib%s.so");
	snprintf(tmp, sizeof(tmp), "%s/templatetags/lib%%s.so", dirname(strdupa(argv[0])));
	list_add(plugin_search_path, strdupa(tmp)); // dupa is ok, as im at main.
	snprintf(tmp, sizeof(tmp), "%s/lib%%s.so", dirname(strdupa(argv[0])));
	list_add(plugin_search_path, strdupa(tmp)); // dupa is ok, as im at main.
	list_add(plugin_search_path, "/usr/local/lib/otemplate/templatetags/lib%s.so");
	list_add(plugin_search_path, "/usr/lib/otemplate/templatetags/lib%s.so");


	int error=work(infilename, outfilename);
	
	list_free(plugin_search_path);
	
	return error;
}

void help(const char *msg){
	if (msg){
		fprintf(stderr, "Error: %s\n", msg);
	}
	fprintf(stderr, "Usage: otemplate --help | otemplate <infilename> <outfilename>\n"
"\n"
"  --help                      Shows this help\n"
"  --templatetagsdir|-t <dirname>  Adds that templatedir to known templatedirs. May be called several times.\n"
"  <infilename>                Input filename or '-' to use stdin.\n"
"  <infilename>                Output filename or '-' to use stdin.\n"
"\n"
"Templatetags plugins are search in this order, always libPLUGIN.so, where PLUGIN is the plugin name:\n"
"   1. Set by command line with --templatetagsdir or -t\n"
"   2. At <infilename directory>, and at <infilename directory>/templatetags\n"
"   3. Current directory (.), and ./templatetags\n"
"   4. Execution directory of otemplate (<dirname(argv[0])>) and <dirname(argv[0])>/templatetags\n"
"   5. System wide at /usr/local/lib/otemplate/templatetags and /usr/lib/otemplate/templatetags\n"
"\n"
"(C) 2011 Coralbits S.L. http://www.coralbits.com/libonion/\n"
	);

}


/**
 * @short Compiles the infilename to outfilename.
 */
int work(const char *infilename, const char *outfilename){
	tag_init();
	parser_status status;
	memset(&status, 0, sizeof(status));
	status.mode=TEXT;
	status.functions=list_new((void*)function_free);
	status.function_stack=list_new(NULL);
	status.status=0;
	status.line=1;
	status.rawblock=onion_block_new();
	status.infilename=infilename;
	const char *tname=basename(strdupa(infilename));
	status.blocks_init=function_new(&status, "%s_blocks_init", tname);
	status.blocks_init->signature="onion_dict *context";
	
	if (strcmp(infilename, "-")==0)
		status.in=stdin;
	else
		status.in=fopen(infilename,"rt");
	
	if (!status.in){
		ONION_ERROR("Could not open in file %s", infilename);
		goto work_end;
	}

	function_new(&status, tname);
	
	function_add_code(&status, "  %s(context);\n",  status.blocks_init->id);
	
	parse_template(&status);
	
	if (status.status){
		ONION_ERROR("Parsing error");
		goto work_end;
	}

	if (strcmp(outfilename, "-")==0)
		status.out=stdout;
	else
		status.out=fopen(outfilename,"wt");
	if (!status.out){
		ONION_ERROR("Could not open out file %s", infilename);
		goto work_end;
	}
	
	fprintf(status.out,
"/** Autogenerated by otemplate v. 0.1.0 */\n"
"\n"
"#include <libintl.h>\n"
"#include <string.h>\n\n"
"#include <onion/onion.h>\n"
"#include <onion/dict.h>\n"
"\n"
"typedef struct dict_res_t{\n"
"	onion_dict *dict;\n"
"	onion_response *res;\n"
"}dict_res;\n"
"\n"
"\n");
	
	functions_write_declarations(&status);

	functions_write_main_code(&status);


	functions_write_code(&status);

work_end:
	if (status.in)
		fclose(status.in);
	if (status.out)
		fclose(status.out);
	list_free(status.functions);
	list_free(status.function_stack);
	//list_free(status.blocks);
	onion_block_free(status.rawblock);
	
	tag_free();
	return status.status;
}

