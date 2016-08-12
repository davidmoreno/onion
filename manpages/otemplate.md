# OTEMPLATE 1
# NAME
otemplate - Compiles django/moustache like templates to use with Onion HTTP Library.

# SYNOPSIS
`otemplate` *file.html* *outputfile.c* [`-a` *asset.h*] [`--templatetagsdir` *directory*]

# DESCRIPTION
otemplate takes a templated html file as an input and generates a C file to be
compiled with an Onion HTTP Library powered web server. The html file can contain
special tags (see *LANGUAGE*) to create complex behavior, as including external
files looping over arrays and dictionaries, add translation...

# OPTIONS
`--help`
 Shows usage help

`-a`|`--asset-file`
 Updates an asset header file with the generated functions to be called from
 an Onion HTTP library powered server.

`--no-orig-lines`|`-n`
 Turn off debug help. When on it generated debug information that can be used by
 the C compiler to provide accurate debugging with gdb, including breakpoints,
 inspection and such into the source HTML file.

Both the input file or the output can be `-` which will then use standard input
or output for such file.

# LANGUAGE

The templating language used is based on Django Templates and Moustache. It can
be expanded via plugins, and basically has these two constructs:

`{{var}}`
 Just print that variable value in that html file position. Var can be an `onion_dict`
 and nesting is possible using `.`, for example `{{context.user.first_name}}`.

`{% tagname arg1 arg2 %}`
 Use that template tag with these arguments.

Some builtin template tags:

`{% for x in list %}...{{x}}...{% endfor %}`
 Loops over the values of the list creating the variable x.

`{% if expression %}...{% else %}...{% endif %}`
 Outputs the first or second part of the conditional based on the expression
 `expression`. Available expresisons are `==`, `!=`, `<=`, `>=`. There is no
 support for boolean operations.

`{% include htmlfile %}`
 Imports the html file and the output will be put at calling point. It can also
 contain template tags, and will use current variables. Normally used as
 `{% include "file.html" %}`, but could used variables to set what file to include.

`{% extend htmlfile %}`
 Imports the given htmlfile using it as outer level html file. It will contain
 `blocks` that may be overwritten later.

`{% block blockname %}...{% endblock %}`
 Defines a block that may be overwritten later. The first place where a block is
 defined will be using place, but it may be redefined later. This is very helpful
 to create a main html file that is used via `{% extend "main.html" %}` or similar
 which defines some blocks that later pages just replace. Normally `head` and
 `content` block names are used as extension of the html `<head>` and as body
 of the final html file, where in the `main.html` file common header, footer
 html head with CSS and JS is defined.

Refer to Django documentation on use cases and possibilities. Please take into
account that otemplate just reimplements the most useful functionalities of
Django tags. If more is needed, please create an issue at Onion HTTP Library
issue tracker at Github.

# Usage from code using Onion HTTP library

The generated functions should be called with a onion_dict as the first argument.
Normal usage should look like this:

```
onion_connection_status myhandler(void *_, onion_request *req, onion_response *res){
  onion_dict *d=onion_dict_new();
  onion_dict_add(d, "section", "index");
  onion_dict *user=onion_dict_new();
  onion_dict_add(user, "username", "dmoreno");
  onion_dict_add(user, "email", "dmoreno@coralbits.com");
  onion_dict_add_dict(d, "user", user);

	return myfile_html_template(d, req, res);
}
```

The `onion_dict *` passed to the `*_template` function will be freed properly. In
C++ code that can be a problem, so `Onion::render_to_response` from `shortcuts.hpp`
should be used, to ensure it handles pointer ownership properly.

# BUGS
There is no variable scoping. If a variable is defined anywhere in the template
it could be accessed later in other scopes. Do not count on this behavior as
it should change.

# AUTHOR
David Moreno <dmoreno@coralbits.com>

# SEE ALSO
opack(1)

[Onion HTTP Library](https://github.com/davidmoreno/onion)

[Django Templates](https://docs.djangoproject.com/en/stable/topics/templates/)
