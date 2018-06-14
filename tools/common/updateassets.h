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

#ifndef __ONION_UPDATE_ASSETS_H__
#define __ONION_UPDATE_ASSETS_H__

struct onion_assets_file_t;
typedef struct onion_assets_file_t onion_assets_file;

/**
 * @short Prepares the structure to load the assets file, to be able to upload it later.
 */
onion_assets_file *onion_assets_file_new(const char *filename);
/**
 * @short Updates an assets file: when given line is not found it adds it to the file.
 * 
 * It checks for a last-line #endif (multiple inclusion guard).
 * 
 * Does not work on multilines.
 */
int onion_assets_file_update(onion_assets_file * file, const char *line);
/**
 * @short Closes and saves all data on the onion assets file.
 */
int onion_assets_file_free(onion_assets_file * file);

#endif
