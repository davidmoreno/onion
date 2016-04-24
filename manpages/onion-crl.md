# ONION-CRL 1
# NAME
onion-crl -- Onion Compile Run Loop watches for source changes to automatically recompile and rerun.

# SYNOPSYS
onion-crl *./my-onion-server*

# DESCRIPTION
onion-crl inspects the passed executable to find out from which sources it was
generated. It then watches the source files, and if any has changed, reruns the
make file, kills the executable, and re runs it.

On normal servers, as state is stored into a database making the executable
stateless, this effectively creates a live C programming environment where the
programmer changes some C or HTML code and reloads the page to see the changes.

If the server is stateful, onion-crl is not recommended.

onion-crl assumes a cmake environment where its running into a `build` directory,
separated from the source, and with the `make` as compilation program.

# ENVIRONMENT
onion-crl tries to find all the source files, but if unsuccessful these
environment variables can affect the behavior:

`SOURCEDIR`
 Directory where source files are. Normally checks at `..`.

`EXTRASOURCES`
 List of extra files that may trigger recompilation/kill/run.

`REGEX`
 Regex for files to search for. Normally it loks for
 `*.cpp *.hpp *.html *.js *.css *.png *.jpg`.

# BUGS
onion-crl is a simple shell script to help development, and as such has many
limitations. If you find it useful, and want to do something more serious, please
contact the onion development team.

Any bug found should be reported at https://github.com/davidmoreno/onion.

# AUTHOR
David Moreno <dmoreno@coralbits.com>

# SEE ALSO
otemplate(1) opack(1)
[Onion HTTP Library](https://github.com/davidmoreno/onion)
