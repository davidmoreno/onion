/** Licensed under GPLv3. Contributed by Basile Starynkevitch. */
/* Just a demo to invoke the GNU guile interpreter. */
#define GC_THREADS 1
#define HAVE_PTHREADS 1
#include <time.h>
#include <math.h>
#include <gc.h>
#include <libguile.h>
#include <onion/onion.h>
#include <onion/log.h>
#include <onion/low.h>

#include <errno.h>
#include <string.h>
#include <signal.h>
#include <netdb.h>

// query a clock
static inline double
dbl_clock_time (clockid_t cid)
{
  struct timespec ts = { 0, 0 };
  if (clock_gettime (cid, &ts))
    return NAN;
  else
    return (double) ts.tv_sec + 1.0e-9 * ts.tv_nsec;
}

int
do_onion_guile (void *p, onion_request * req, onion_response * resp)
{
  unsigned flags = onion_request_get_flags (req);
  const char *fullpath = onion_request_get_fullpath (req);
  if ((flags & OR_METHODS) == OR_GET)
    {
      char *obuf = NULL;
      size_t osize = 0;
      FILE *ofil = open_memstream (&obuf, &osize);
      time_t now = 0;
      time (&now);
      struct tm nowtm = { 0 };
      char timbuf[64];
      memset (timbuf, 0, sizeof (timbuf));
      if (!ofil)
	{
	  perror ("open_memstream");
	  exit (EXIT_FAILURE);
	};
      fprintf (ofil,
	       "<!doctype html><html><head><title>Guile thru Onion</title><head>\n"
	       "<body><h1><a href='https://www.gnu.org/software/guile/'>Guile</a> thru Onion.</h1>\n"
	       "<form action=\"eval\" method=\"POST\">\n"
	       "<label for='guile_input'><b>Guile expressions:</b></label>\n"
	       "<textarea id='guile_input' width=70 height=30 style='background-color:lightyellow; color:navy'>\n"
	       ";; edit this at will, it is for Guile\n"
	       "(define (big-useless n)\n"
	       " (if (< n 2) n (list n (big-useless (- n 1)) (big-useless (- n 2)))))\n"
	       ";; test it to make the GC work\n"
	       "(begin (big-useless 20) (gc) 'done)\n" "</textarea><br/>\n"
	       "<input type='clear' name='clear' value='clear'/>\n"
	       "<input type='submit' name='eval' value='evaluate'/>"
	       "</form> <br/>\n");
      strftime (timbuf, sizeof (timbuf), "%c", localtime_r (&now, &nowtm));
      fprintf (ofil, "<small>at %s</small>\n", timbuf);
      fprintf (ofil,
	       "<p>This is just a demo. It can crash or misbehave!\n"
	       " <small>Location <tt>%s</tt></small></p>\n", fullpath);
      fprintf (ofil, "</body></html>\n");
      fclose (ofil);
      onion_response_set_length (resp, osize);
      onion_response_set_header (resp, "Content-Type",
				 " text/html; charset=utf-8");
      onion_response_set_code (resp, HTTP_OK);
      onion_response_write_headers (resp);
      onion_response_write0 (resp, obuf);
      onion_response_flush (resp);
      free (obuf), obuf = NULL;
    }
  else if ((flags & OR_METHODS) == OR_POST)
    {
      char *obuf = NULL;
      size_t osize = 0;
      FILE *ofil = open_memstream (&obuf, &osize);
      time_t now = 0;
      time (&now);
      struct tm nowtm = { 0 };
      char timbuf[64];
      static int nbeval;
      memset (timbuf, 0, sizeof (timbuf));
      if (!ofil)
	{
	  perror ("open_memstream");
	  exit (EXIT_FAILURE);
	};
      const char *evalstr = onion_request_get_post (req, "eval");
      double startrealtime = dbl_clock_time (CLOCK_REALTIME);
      double startcputime = dbl_clock_time (CLOCK_PROCESS_CPUTIME_ID);
      SCM outscm = scm_open_output_string ();
      scm_set_current_output_port (outscm);
      scm_set_current_error_port (outscm);
      SCM rescm = scm_c_eval_string (evalstr);
      scm_write (rescm, outscm);
      scm_newline (outscm);
      scm_force_output (outscm);
      SCM outstrscm = scm_get_output_string (outscm);
      scm_close_output_port (outscm);
      nbeval++;
      double endrealtime = dbl_clock_time (CLOCK_REALTIME);
      double endcputime = dbl_clock_time (CLOCK_PROCESS_CPUTIME_ID);
      onion_response_set_header (resp, "Content-Type",
				 " text/html; charset=utf-8");
      onion_response_set_code (resp, HTTP_OK);
      onion_response_printf
	(resp, "<!doctype html>\n"
	 "<html><head><title>evaluation #%d by Guile</title></head>\n"
	 "<body><h1>Evaluation #%d by Guile</h1>\n", nbeval, nbeval);
      onion_response_printf
	(resp,
	 "<h2>input %d<sup>-th</sup> expression was:</h2>\n"
	 "<pre style='font-size:80%%; background-color:ivory; text-color:brown'>\n",
	 nbeval);
      onion_response_write_html_safe (resp, evalstr);
      onion_response_printf  ///
	(resp, "</pre>\n"
			     "<br/><b>computed</b> in %.3f CPU &amp; %.3f real seconds\n",
			     endcputime - startcputime,
			     endrealtime - startrealtime);
      onion_response_printf (resp,
			     "<h2>input %d<sup>-th</sup> result is:</h2>\n"
			     "<pre style='font: Verdana Italics; font-size:110%%; background-color:pink; text-color:blue'>\n",
			     nbeval);
      onion_response_write_html_safe (resp, scm_to_utf8_string (outstrscm));
      onion_response_printf (resp, "</pre>\n"
			     "<br/><b>computed</b> in %.3f CPU &amp; %.3f real seconds\n",
			     endcputime - startcputime,
			     endrealtime - startrealtime);
      onion_response_printf  ///
	(resp,
			     "<hr/>\n"
			     "<h2>Try again some evaluation #%d:</h2>\n"
			     "<form action=\"eval\" method=\"POST\">\n"
	       "<label for='guile_input'><b>Guile expressions:</b></label>\n"
	       "<textarea id='guile_input' width=70 height=30 style='background-color:lightyellow; color:navy'>\n"
	       ";; edit this at will, it is for Guile\n"
			     "</textarea><br/>\n"
			     "<input type='clear' name='clear' value='clear'/>\n"
			     "<input type='submit' name='eval' value='evaluate'/>"
			     "</form><br/>\n", nbeval+1);
      strftime (timbuf, sizeof (timbuf), "%c", localtime_r (&now, &nowtm));
	       onion_response_printf(resp,
				     "<p>This is just a demo. It can crash or misbehave!<br/>"
				     "<small>Location <tt>%s</tt></small>\n", fullpath);
      onion_response_printf(resp, "at <i>%s</i>.</p></body></html>\n", timbuf);
      onion_response_flush(resp);
      return OCS_PROCESSED;
    }
    return OCS_NOT_PROCESSED;
}

onion *o = NULL;

static void
shutdown_server (int _)
{
  if (o)
    onion_listen_stop (o);
}

void
memory_allocation_error (const char *msg)
{
  ONION_ERROR ("memory failure: %s (%s)", msg, strerror (errno));
  exit (EXIT_FAILURE);
}

void *
GC_calloc (size_t n, size_t size)
{
  void *ret = onion_low_malloc (n * size);
  memset (ret, 0, n * size);
  return ret;
}


void* do_inner_main (void *data)
{
  o = onion_new (O_ONE_LOOP);
  onion_set_timeout (o, 5000);
  onion_set_hostname (o, "0.0.0.0");
  onion_url *urls = onion_root_url (o);
  ONION_DEBUG("in do_inner_main o@%p", o);
  onion_url_add_static (urls, "static", "Hello static world", HTTP_OK);
  onion_url_add (urls, "", do_onion_guile);
  onion_url_add (urls, "^(.*)$", do_onion_guile);

  ONION_INFO("in do_inner_main before listen");
  onion_listen (o);
  ONION_INFO("after listen");
  onion_free (o);
  return data;
}

#warning this does not work because of Guile or GC issues
int
main (int argc, char **argv)
{
  // perhaps I should not call GC_INIT here...
  GC_INIT();
  onion_low_initialize_memory_allocation (GC_malloc, GC_malloc_atomic,
					  GC_calloc, GC_realloc, GC_strdup,
					  GC_free, memory_allocation_error);
  onion_low_initialize_threads (GC_pthread_create, GC_pthread_join,
				GC_pthread_cancel, GC_pthread_detach,
				GC_pthread_exit, GC_pthread_sigmask);

  signal (SIGINT, shutdown_server);
  signal (SIGTERM, shutdown_server);

  
  scm_with_guile (do_inner_main, NULL);
  return 0;
}

/**** for emacs
  ++ Local Variables: ++
  ++ compile-command: "gcc -Wall -g3 guile-boehm.c -L /usr/local/lib -I/usr/local/include -lonion -lonion_handlers $(pkg-config --cflags --libs guile-2.0) -o $HOME/bin/guile-boehm-onion" ++
  ++ End: ++
***/
