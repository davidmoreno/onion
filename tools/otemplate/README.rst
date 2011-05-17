OTEMPLATE
=========

Introduction
------------

otemplate is onion solution to templating. 

To fast develop web sites its absolutely mandatory to have some templating language. Otemplate
is onion solution to this problem. It can be externally used to create the C source files 
necessary from the htmls that will be used to generate dynamic websites.

Its syntax is based on django templates (http://docs.djangoproject.com/en/dev/topics/templates/),
and although it has diferent set of tags, the syntax remains the same, and most common tags
work as expected. They are listed bellow.

Usage
-----

Each html template that will be used needs to be compiled by otemplate. It includes all the 
{% include %} templates, so for example if my templates are base.html and index.html, and
index.html has an {% include "base.html" %}, it needs to compile both:

	$ otemplate base.html base_html.c
	$ otemplate index.html index_html.c

It generates several functions, many internal, but external functions will be a response writer
and a request handler. For example from index.html I will get:

	onion_handler *index_html_handler(onion_dict *context);
	int index_html_template(onion_dict *context, onion_request *req);
	void index_html(onion_dict *context, onion_response *res);

The first, `index_html_handler` is a shortcut to use it as onion_handler directly. It will set the
free function to free the dictionary when the handler is destructed.

The second, `index_html_template` is what user will normally call. It frees the dictionary as soon as
used, and do the proper execution.

The last is for power users that want to call it from already generated response objects, for
example because extra headers are needed. This is also the function that {% include ... %} calls.


### cmake rule

If you use cmake, this is the way to compile it, until better integration is implemented:

	add_custom_command(
		OUTPUT test_html.c
		COMMAND otemplate ${CMAKE_CURRENT_SOURCE_DIR}/test.html
									${CMAKE_CURRENT_BINARY_DIR}/test_html.c
		DEPENDS otemplate ${CMAKE_CURRENT_SOURCE_DIR}/test.html
		)

### Extensibility

As the django template system, otemplate allows to be extended. It currently only support tag
extensibility, and the way to add them is create a plugin. This is a share object with a 
`void plugin_init(void)` method, and from there it calls the registers for the tags. For 
example this is the code for the i18n:

	#include <malloc.h>

	#include <onion/codecs.h>

	#include "../tags.h"
	#include "../functions.h"

	/// Following text is for gettext
	void tag_trans(parser_status *st, list *l){
		char *s=onion_c_quote_new(tag_value_arg(l,1));
		function_add_code(st, "  onion_response_write0(res, gettext(%s));\n", s);
		free(s);
	}

	/// Adds the tags.
	void plugin_init(){
		tag_add("trans", tag_trans);
	}

Current status
--------------

Actually its in its infancy and only have limited support:

 * No filters
 * Only if, else, endif, for, endfor, include, extends, block and trans are supported, but can be user extended
 * Only dicts and string types. 
 * For loops are over the values of the dicts.
