#include <glib.h>
#include <gio/gio.h>
#include <agent.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "util.h"
#include "callbacks.h"
#include "global.h"

static const gchar *state_name[] = {"disconnected", "gathering", "connecting",
                                    "connected", "ready", "failed"};

gboolean
exchange_credentials(NiceAgent *agent, guint stream_id, gpointer data) {
  g_debug("exchange_credentials(): candidate gathering done\n");

  publish_local_credentials(agent, stream_id);
  lookup_remote_credentials(agent, stream_id);

  pipe_stdio_to_hook("NICE_PIPE_BEFORE", exit_if_child_exited);

  g_debug("candidate exchange complete\n");
}

gboolean
new_selected_pair(
  NiceAgent *agent, guint stream_id, guint component_id,
  gchar *lfoundation, gchar *rfoundation, gpointer data
) {
  g_debug("SIGNAL: selected pair %s %s", lfoundation, rfoundation);
}

void
start_server(NiceAgent *agent, guint stream_id, guint component_id, guint state, gpointer server_ptr) {
  GSocketService* server = (GSocketService*) server_ptr;
  g_debug("`start_server`\n");
g_debug("SIGNAL: state changed %d %d %s[%d]\n", stream_id, component_id, state_name[state], state);

  if (state == NICE_COMPONENT_STATE_CONNECTED) {
    // NiceCandidate *local, *remote;

    // // Get current selected candidate pair and print IP address used
    // if (nice_agent_get_selected_pair (agent, _stream_id, component_id,
    //             &local, &remote)) {
    //   gchar ipaddr[INET6_ADDRSTRLEN];

    //   nice_address_to_string(&local->addr, ipaddr);
    //   printf("\nNegotiation complete: ([%s]:%d,",
    //       ipaddr, nice_address_get_port(&local->addr));
    //   nice_address_to_string(&remote->addr, ipaddr);
    //   printf(" [%s]:%d)\n", ipaddr, nice_address_get_port(&remote->addr));
    // }

    // // Listen to stdin and send data written to it
    // printf("\nSend lines to remote (Ctrl-D to quit):\n");
    // g_io_add_watch(io_stdin, G_IO_IN, stdin_send_data_cb, agent);
    // printf("> ");
    // fflush (stdout);
    if(is_caller)
      g_socket_service_start(server);
    else
      setup_client(agent);

    pipe_stdio_to_hook("NICE_PIPE_AFTER", exit_if_child_exited);

    g_message("Connection to %s established.\n", remote_hostname);
  // } else if (state == NICE_COMPONENT_STATE_GATHERING) {
  // } else if (state == NICE_COMPONENT_STATE_CONNECTING) {
  } else if (state == NICE_COMPONENT_STATE_FAILED) {
    g_main_loop_quit (gloop);
  }
}

void
start_server_reliable(NiceAgent *agent, guint stream_id, guint component_id, gpointer server_ptr) {
  GSocketService* server = (GSocketService*) server_ptr;
  g_debug("`start_server`\n");

  if(is_caller)
    g_socket_service_start(server);
  else
    setup_client(agent);

  pipe_stdio_to_hook("NICE_PIPE_AFTER", exit_if_child_exited);

  g_message("Connection to %s established.\n", remote_hostname);
}

void
attach_stdin2send_callback(NiceAgent *agent, guint stream_id, guint component_id, guint state) {
  if (state == NICE_COMPONENT_STATE_READY) {
    unpublish_local_credentials(agent, stream_id);
    GIOChannel* io_stdin;
    io_stdin = g_io_channel_unix_new(fileno(stdin));

    g_io_add_watch(io_stdin, G_IO_IN, send_data, agent);
  }

  if (state == NICE_COMPONENT_STATE_FAILED) {
    g_main_loop_quit (gloop);
  }
}

void
attach_stdin2send_callback_reliable(NiceAgent *agent, guint stream_id, guint component_id, gpointer data) {
  unpublish_local_credentials(agent, stream_id);

  GIOChannel* io_stdin;
  io_stdin = g_io_channel_unix_new(fileno(stdin));

  g_io_add_watch(io_stdin, G_IO_IN, send_data, agent);
}

gboolean
send_data(GIOChannel *source, GIOCondition cond, gpointer agent_ptr) {
  static char buffer[20480];
  struct iovec io;
  char crap[20480];
  const gsize max_size = 20480;
  gsize len;

  NiceAgent *agent = agent_ptr;
  struct msghdr msgh;
  struct cmsghdr *cmsg;

  gint res;
  do {
    // THIS STUFF IS PROBABLY NOW WRONG
    io.iov_base = buffer;
    io.iov_len = max_size;
    memset(&msgh, 0, sizeof(msgh));
    msgh.msg_iov = &io;
    msgh.msg_iovlen = 5;
    msgh.msg_control = &crap;
    msgh.msg_controllen = sizeof(crap);

    int sock = g_io_channel_unix_get_fd(source);
    
    res = recvmsg(sock, &msgh, MSG_DONTWAIT); // this is upset!
    if(res > -1) {
      if(res == 0) {
        // probably FLUSHED
        g_main_loop_quit(gloop);
        return FALSE;
      }

      g_debug("recvmsg: %i\n", res);
      res = nice_agent_send(agent, nice_stream_id, 1, res, buffer);
      g_debug("nice_agent_send: %i\n", res);
    }
    else {
      if(errno != EAGAIN && errno != EWOULDBLOCK) {
        g_critical("Error sending: recvmsg() = %i, errno=%i\n", res, errno);
        g_main_loop_quit(gloop);
        break;
      }
      if(errno == EAGAIN || errno == EWOULDBLOCK)
        break;
    }
  }
  while(len < max_size);

  return TRUE;
}

void
recv_data2fd(NiceAgent *agent, guint stream_id, guint component_id, guint len,
    gchar *buf, gpointer data) {
  unpublish_local_credentials(agent, stream_id);
  g_debug("recv_data2fd(fd=%u, len=%u)\n", output_fd, len);
  write(output_fd, buf, len);
//  syncfs(output_fd);
}
