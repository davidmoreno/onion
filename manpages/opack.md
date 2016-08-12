# OPACK 1
# NAME
opack - Embeds static assets into a C file.

# SYNOPSIS
`opack` *files...* *directories...* [`-o` *outputfile.c*] [`-a` *assets.h*]`

# DESCRIPTION
Opacks embeds some static files into a C file so that other C code can use it
as a big memory object. It provides as well functions to be used with the
onion http library.

It also can update an assets.h file with the functions that can be called to get
the resources.

This is helpful when the programmer of an HTTP server wants all the assets it
provides into a single executable, not an executable + static files.

Onion will access the embeded files via a
`opack_[filename_extension](void *_ignore, onion_request *, onion_response *)`,
which can be used directly by an URL handler.

# OPTIONS
`--help`
 Shows usage help information

`-o` *outputfile.c*
 Output C file that will contains the assets

`-a` *assetfile.h*
 Assets file that will be **updated** with the functions and code necesary to
 access the embedded resources.

# BUGS
Any bug found should be reported at https://github.com/davidmoreno/onion.

# AUTHOR
David Moreno <dmoreno@coralbits.com>

# SEE ALSO
otemplate(1)
[Onion HTTP Library](https://github.com/davidmoreno/onion)
