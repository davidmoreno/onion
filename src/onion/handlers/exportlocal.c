/**
  Onion HTTP server library
  Copyright (C) 2010-2018 David Moreno Montero and others

  This library is free software; you can redistribute it and/or
  modify it under the terms of, at your choice:

  a. the Apache License Version 2.0.

  b. the GNU General Public License as published by the
  Free Software Foundation; either version 2.0 of the License,
  or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of both licenses, if not see
  <http://www.gnu.org/licenses/> and
  <http://www.apache.org/licenses/LICENSE-2.0>.
*/

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>

#include <onion/shortcuts.h>
#include <onion/handler.h>
#include <onion/response.h>
#include <onion/codecs.h>
#include <onion/log.h>
#include <onion/low.h>

#include "exportlocal.h"

struct onion_handler_export_local_data_t {
  void (*renderer_header) (onion_response * res, const char *dirname);
  void (*renderer_footer) (onion_response * res, const char *dirname);
  char *localpath;
  int is_file:1;
};

typedef struct onion_handler_export_local_data_t
 onion_handler_export_local_data;

int onion_handler_export_local_directory(onion_handler_export_local_data * data,
                                         const char *realp,
                                         const char *showpath,
                                         onion_request * req,
                                         onion_response * res);
int onion_handler_export_local_file(const char *realp, struct stat *reals,
                                    onion_request * request,
                                    onion_response * response);

int onion_handler_export_local_handler(onion_handler_export_local_data * d,
                                       onion_request * request,
                                       onion_response * response) {
  char tmp[PATH_MAX];
  char realp[PATH_MAX];

  if (d->is_file) {
    if (strlen(d->localpath) > (PATH_MAX - 1)) {
      ONION_ERROR("File path too long");
      return OCS_INTERNAL_ERROR;
    }
    strncpy(tmp, d->localpath, PATH_MAX - 1);
  } else
    snprintf(tmp, PATH_MAX, "%s/%s", d->localpath,
             onion_request_get_path(request));

  ONION_DEBUG0("Get %s (base %s)", tmp, d->localpath);

  // First check if it exists and so on. If it does not exist, no trying to escape message
  struct stat reals;
  int ok = stat(tmp, &reals);
  if (ok < 0)                   // Cant open for even stat
  {
    ONION_DEBUG0("Not found %s.", tmp);
    return 0;
  }

  const char *ret = realpath(tmp, realp);
  if (!ret || strncmp(realp, d->localpath, strlen(d->localpath)) != 0) {        // out of secured dir.
    ONION_WARNING
        ("Trying to escape from secured dir (secured dir %s, trying %s).",
         d->localpath, realp);
    return 0;
  }

  if (S_ISDIR(reals.st_mode)) {
    //ONION_DEBUG("DIR");
    return onion_handler_export_local_directory(d, realp,
                                                onion_request_get_path(request),
                                                request, response);
  } else if (S_ISREG(reals.st_mode)) {
    //ONION_DEBUG("FILE");
    return onion_shortcut_response_file(realp, request, response);
  }
  ONION_DEBUG0("Dont know how to handle");
  return OCS_NOT_PROCESSED;
}

/// Default directory handler: The style + dirname on a h1
void onion_handler_export_local_header_default(onion_response * res,
                                               const char *dirname) {
  onion_response_write0(res,
                        "<style>body{ background: #fefefe; font-family: sans-serif; margin-left: 5%; margin-right: 5%; }\n"
                        " table{ background: white; width: 100%; border: 1px solid #aaa; border-radius: 5px; -moz-border-radius: 5px; } \n"
                        " th{	background: #eee; } tbody tr:hover td{ background: yellow; } tr.dir td{ background: #D4F0FF; }\n"
                        " table a{ display: block; } th{ cursor: pointer} h1,h2{ color: black; text-align: center; } \n"
                        " a{ color: red; text-decoration: none; }</style>\n");

  onion_response_printf(res, "<h1>Listing of directory %s</h1>\n", dirname);

  if (dirname[0] != '\0' && dirname[1] != '\0') // It will be 0, when showpath is "/"
    onion_response_write0(res, "<h2><a href=\"..\">Go up..</a></h2>\n");
}

void onion_handler_export_local_footer_default(onion_response * res,
                                               const char *dirname) {
  onion_response_write0(res,
                        "<h2>Onion directory list. (C) 2010 <a href=\"http://www.coralbits.com\">CoralBits</a>. "
                        "Under <a href=\"http://www.gnu.org/licenses/lgpl-3.0.html\">LGPL 3.0.</a> License.</h2>\n");
}

/**
 * @short Returns the directory listing
 */
int onion_handler_export_local_directory(onion_handler_export_local_data * data,
                                         const char *realp,
                                         const char *showpath,
                                         onion_request * req,
                                         onion_response * res) {
  DIR *dir = opendir(realp);
  if (!dir)                     // Continue on next. Quite probably a custom error.
    return 0;
  onion_response_set_header(res, "Content-Type", "text/html; charset=utf-8");

  onion_response_write0(res,
                        "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n"
                        "<html>\n"
                        " <head><meta content=\"text/html; charset=UTF-8\" http-equiv=\"content-type\"/>\n");
  onion_response_printf(res, "<title>Directory listing of %s</title>\n",
                        showpath);
  onion_response_write0(res,
                        "</head>\n" " <body>\n" "<script>\n"
                        "showListing = function(){\n"
                        "	var q = function(t){\n"
                        "		return t.replace('\"','%22')\n"
                        "	}	\n"
                        "	var t=document.getElementById('filebody')\n"
                        "  while ( t.childNodes.length >= 1 )\n"
                        "		t.removeChild( t.firstChild );       \n"
                        "	for (var i=0;i<files.length;i++){\n"
                        "		var f=files[i]\n"
                        "		var h='<tr class=\"'+f[3]+'\"><td><a href=\"'+q(f[0])+'\">'+f[0]+'</a></td><td>'+f[1]+'</td><td>'+f[2]+'</td></tr>'\n"
                        "		t.innerHTML+=h\n"
                        "	}\n"
                        "}\n"
                        "\n"
                        "update = function(n){\n"
                        "	var _cmp = function(a,b){\n"
                        "		if (a[n]<b[n])\n"
                        "			return -1\n"
                        "		if (a[n]>b[n])\n"
                        "			return 1\n"
                        "		return 0\n"
                        "	}\n"
                        "	files=files.sort(_cmp)\n"
                        "	showListing()\n"
                        "}\n"
                        "window.onload=function(){\n"
                        "	files=files.splice(0,files.length-1)\n"
                        "	update(0)\n" "}\n" "\n" "files=[\n");

  struct dirent *fi;
  struct stat st;
  char temp[1024];
  struct passwd *pwd;
  while ((fi = readdir(dir)) != NULL) {
    if (fi->d_name[0] == '.')
      continue;
    snprintf(temp, sizeof(temp), "%s/%s", realp, fi->d_name);
    stat(temp, &st);
    pwd = getpwuid(st.st_uid);

    if (S_ISDIR(st.st_mode))
      onion_response_printf(res, "  ['%s/',%ld,'%s','dir'],\n", fi->d_name,
                            st.st_size, pwd ? pwd->pw_name : "???");
    else
      onion_response_printf(res, "  ['%s',%ld,'%s','file'],\n", fi->d_name,
                            st.st_size, pwd ? pwd->pw_name : "???");
  }

  onion_response_write0(res, "  [] ]\n</script>\n");

  if (data->renderer_header)
    data->renderer_header(res, showpath);

  onion_response_write0(res, "<table id=\"filelist\">\n"
                        "<thead><tr>\n"
                        " <th onclick=\"update(0)\" id=\"filename\">Filename</th>\n"
                        " <th onclick=\"update(1)\" id=\"size\">Size</th>"
                        " <th onclick=\"update(2)\" id=\"owner\">Owner</th></tr>\n"
                        "</thead>\n"
                        "<tbody id=\"filebody\">\n</tbody>\n</table>\n</body>\n");

  if (data->renderer_footer)
    data->renderer_footer(res, showpath);

  onion_response_write0(res, "</body></html>");

  closedir(dir);

  return OCS_PROCESSED;
}

/// Frees local data from the directory handler
void onion_handler_export_local_delete(void *data) {
  onion_handler_export_local_data *d = data;
  onion_low_free(d->localpath);
  onion_low_free(d);
}

/// Sets the header renderer
void onion_handler_export_local_set_header(onion_handler * handler,
                                           void (*renderer) (onion_response *
                                                             res,
                                                             const char
                                                             *dirname)) {
  onion_handler_export_local_data *d = onion_handler_get_private_data(handler);
  d->renderer_header = renderer;
}

/// Sets the footer renderer
void onion_handler_export_local_set_footer(onion_handler * handler,
                                           void (*renderer) (onion_response *
                                                             res,
                                                             const char
                                                             *dirname)) {
  onion_handler_export_local_data *d = onion_handler_get_private_data(handler);
  d->renderer_footer = renderer;
}

/**
 * @short Creates an local filesystem handler.
 * 
 * Exports the given localpath and any file inside this localpath is returned.
 * 
 * It performs security checks, so that the returned data is saftly known to be inside 
 * that localpath. Normal permissions apply.
 */
onion_handler *onion_handler_export_local_new(const char *localpath) {
  char *rp = realpath(localpath, NULL);
  if (!rp) {
    ONION_ERROR("Cant calculate the realpath of the given directory (%s).",
                localpath);
    return NULL;
  }
  struct stat st;
  if (stat(rp, &st) != 0) {
    ONION_ERROR("Cant access to the exported directory/file (%s).", rp);
    onion_low_free(rp);
    return NULL;
  }
  onion_handler_export_local_data *priv_data =
      onion_low_malloc(sizeof(onion_handler_export_local_data));

  priv_data->localpath = rp;
  priv_data->renderer_header = onion_handler_export_local_header_default;
  priv_data->renderer_footer = onion_handler_export_local_footer_default;

  priv_data->is_file = S_ISREG(st.st_mode);

  onion_handler *ret = onion_handler_new((onion_handler_handler)
                                         onion_handler_export_local_handler,
                                         priv_data,
                                         (onion_handler_private_data_free)
                                         onion_handler_export_local_delete);
  return ret;
}
