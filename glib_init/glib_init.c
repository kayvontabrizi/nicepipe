#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <glib.h>
#include <gio/gio.h>

#include <agent.h>

guint forward_port = 1500;
guint stun_port = 3478;
gchar* stun_host = NULL;
gchar* remote_hostname = NULL;
gint* is_caller = NULL;
gboolean not_reliable = FALSE;
gboolean verbose = TRUE;

gint max_size = 8;
gboolean beep = FALSE;
GOptionEntry all_options[] =
{
  { "forwarding_port", 'P', 0, G_OPTION_ARG_INT, &forward_port,
    "Port to listen at (for caller) or to forward to (for callee)", NULL },
  { "hostname", 'H', 0, G_OPTION_ARG_STRING, &remote_hostname,
    "remote hostname (as mentioned in $HOME/.ssh/known_hosts", NULL },
  { "stun_port", 'p', 0, G_OPTION_ARG_INT, &stun_port,
    "STUN server port (default: 3478)", "p" },
  { "stun_host", 's', 0, G_OPTION_ARG_STRING, &stun_host,
    "STUN server host (e.g. stunserver.org)", "s" },
  { "iscaller", 'c', 0, G_OPTION_ARG_INT, &is_caller,
    "c=1: is caller, c=0 if not", "c" },
  { "not-reliable", 'u', 0, G_OPTION_ARG_NONE, &not_reliable,
    "do not use pseudo TCP connection", NULL },
  { "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose,
    "Be verbose", NULL },
  { NULL }
};

#define G_LOG_DOMAIN    ((gchar*) 0)

GMainLoop *gloop;

void parse_argv(int argc, char *argv[]);
void setup_glib();
void log_stderr(const gchar *log_domain,
                      GLogLevelFlags log_level,
                      const gchar *message,
                      gpointer user_data);


int
main(int argc, char *argv[]) {
  parse_argv(argc, argv);
  g_log_set_handler(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, log_stderr, NULL);

  setup_glib();

  g_debug("glib is setup!\n");

  g_debug("Entering main loop...\n");
  g_main_loop_run(gloop);

  g_main_loop_unref(gloop);

  return EXIT_SUCCESS;
}


void
parse_argv(int argc, char *argv[]) {
  GOptionContext *context;
  GError* error = NULL;

  context = g_option_context_new("");
  g_option_context_add_main_entries(context, all_options, NULL);
  if (!g_option_context_parse(context, &argc, &argv, &error)) {
    g_print("option parsing failed: %s\n", error->message);
    g_error_free(error);
    exit(1);
  }

  /*if(remote_hostname == NULL) {*/
    /*g_critical("No remote hostname given! (Please use -H)");*/
    /*exit(1);*/
  /*}*/

  g_option_context_free(context);
}


void
setup_glib() {
  g_type_init();

  gloop = g_main_loop_new(NULL, FALSE);
}


void
log_stderr(const gchar *log_domain,
            GLogLevelFlags log_level,
            const gchar *message,
            gpointer user_data) {
  //if(log_level < G_LOG_LEVEL_DEBUG)
    fprintf(stderr, "%i: %s", is_caller, message);
}

