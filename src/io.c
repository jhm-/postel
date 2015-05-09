/* io.c: input/output and the command-line interface.
 * Copyright © 2015 Jack Morton <jhm@jemscout.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "postel.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <glib.h>
#include <uv.h>

extern struct global_state_struct postel;
G_LOCK_EXTERN(postel);
G_LOCK_DEFINE(node_head);

/* IO callbacks */
static uv_signal_t sigint_watcher;
static uv_poll_t stdin_watcher, stdout_watcher;

/* Set a filestream to non-blocking mode. returns 0 on success */
static int set_non_blocking(int fd)
{
  int val = fcntl(fd, F_GETFL, 0);
  return val == -1 ? -1 : fcntl(fd, F_SETFL, val | O_NONBLOCK);
}

/* Print to screen. Watch those format strings! */
static int print_msg(const char *fmt, ...)
{
  int w;
  char buf[4096];
  size_t bufsz = 0;
  va_list ap;

  va_start(ap, fmt);
    bufsz += vsnprintf(buf + bufsz, (sizeof(buf) - bufsz), fmt, ap);
  va_end(ap);

  w = write(STDOUT_FILENO, buf, bufsz);
  if (w < 0) {
    fprintf(stderr, "Error on write() to stdout: %s\n", strerror(errno));
  }
  while (w >= 0 && w != bufsz) {
    /* Partial buf written, so shift position */
    memmove(buf, buf + w, bufsz - w);
    bufsz -= w;
  }
  return w;
}

static void print_prompt(void)
{
  /* XXX: This is it... for now */
  print_msg(":) > ");
}

static void sigint_cb(uv_signal_t *handle, int signum)
{
  char *msg[1] = {"Shutdown: Caught SIGINT"};
  shutdown_postel(-1, msg);
}

/* Prototypes */
static void help_command(int argc, char **argv);
static void add_command(int argc, char **argv);
static void del_command(int argc, char **argv);
static void list_command(int argc, char **argv);

/* Here are the commands yo! */
#define MAX_ARGV 3
#define CONSOLE_COMMANDS 5
struct commands {
  char *name;
  unsigned int req_arg;
  char *short_desc;
  char *long_desc;
  void (*function)(int argc, char **argv);
} commands[] = {
  {"add", 2, "add <x> <y>: spawn a node at coordinates <x>, <y>.", \
    "spawn a node at coordinates <x>, <y>.", &add_command},
  {"del", 1, "del <id>: remove a node.", \
    "remove the node that identifies by <id>.", &del_command},
  {"list", 0, "list: list information about nodes.", \
    "list id and coordinates for all nodes in the simulation.", &list_command},
  {"help", 0, "help [topic]: display help for a specific [topic].", \
    "display help for a specific [topic].", &help_command},
  {"quit", 0, "quit: safely shutdown the simulation.", \
    "safely shut down the simulation.", &shutdown_postel}};

static void help_command(int argc, char **argv)
{
  int i;

  if (argc < 1) {
    for(i = 0; i < CONSOLE_COMMANDS; i++)
      print_msg("%s\n", commands[i].short_desc);
    return;
  }
  else {
    for(i = 0; i < CONSOLE_COMMANDS; i++) {
      if (!strcasecmp(argv[1], commands[i].name)) {
          print_msg("%s\n", commands[i].long_desc);
          return;
      }
    }
    print_msg("Invalid command: %s\n", argv[1]);
  }
}

static void add_command(int argc, char **argv)
{
  G_LOCK(node_head);
  if (add_node(strtod(argv[1], NULL), strtod(argv[2], NULL)))
    print_msg("Error: unable to add node at %.0f, %.0f\n", \
      strtod(argv[1], NULL), strtod(argv[2], NULL));
  G_UNLOCK(node_head);
}

static void del_command(int argc, char **argv)
{
  G_LOCK(node_head);
  if (sizeof(intptr_t) == sizeof(int)) {
    if (del_node(atoi(argv[1])))
      print_msg("Error: unable to find node %ld\n", atoi(argv[1]));
  }
  else if (sizeof(intptr_t) == sizeof(long)) {
    if (del_node(atol(argv[1])))
      print_msg("Error: unable to find node %ld\n", atol(argv[1]));
  }
  G_UNLOCK(node_head);
}

static void list_command(int argc, char **argv)
{
  struct node *nodep;

  G_LOCK(node_head);
  print_msg("node id\t\t\tx\ty\n");
  print_msg("---------------\t\t----\t----\n");
  LIST_FOREACH(nodep, &node_head, nodes) {
    print_msg("%ld\t\t%.0f\t%.0f\n", nodep->id, nodep->x, nodep->y);
  }
  G_UNLOCK(node_head);
}

/* Callback when data is readable on stdin */
static void stdin_cb(uv_poll_t *handle, int status, int events)
{
  char buf[4096];
  char **arg, *argv[MAX_ARGV];
  int rv, i, argc = 0;

  bzero(&buf, sizeof(buf));
  rv = read(STDIN_FILENO, buf, sizeof(buf));
  buf[4096] = '\0';
  if (rv >= 0) {
    /* Process the input */
    arg = argv;
    if ((*arg++ = strtok(buf, " \n"))) {
      while((*arg++ = strtok(NULL, " \n")) && argc < (MAX_ARGV - 1))
          argc++;
      for(i = 0; i < CONSOLE_COMMANDS; i++) {
        if (!strcasecmp(argv[0], commands[i].name)) {
          if (argc < commands[i].req_arg) {
            print_msg("%s: not enough arguments.\n", argv[0]);
            goto peace;
          }
          else {
            commands[i].function(argc, argv);
            goto peace;
          }
        }
      }
    }
    print_msg("Invalid command: %s\n", argv[0]);
  }
  else if (errno != EAGAIN && errno != EINPROGRESS) {
    fprintf(stderr, "Error on read() from stdin: %s\n", strerror(errno));
    return;
  }

peace:
  print_prompt();
}

void shutdown_console(void)
{
  uv_poll_stop(&stdin_watcher);
  uv_signal_stop(&sigint_watcher);
}

int init_console(uv_loop_t *loop)
{
  int err;

  /* Place stdin and stdout streams in non-blocking mode */
  err = set_non_blocking(STDIN_FILENO);
  if (err) {
    fprintf(stderr, "Unable to set stdin to non-blocking mode.\n");
    goto peace;
  }
  err = set_non_blocking(STDOUT_FILENO);
  if (err) {
    fprintf(stderr, "Unable to set stdout to non-blocking mode.\n");
    goto peace;
  }

  printf(" _  _  __|_ _ |\n" \
         "|_)(_)_> |_(/_|\n" \
         "|\n" \
         "postel: %s\n" \
         "copyright © 2015 Jack Morton <jhm@jemscout.com>\n" \
         "please read LICENSE for copyright details.\n",  VERSION);
  print_prompt();

  /* Spawn input and output callbacks */
  uv_poll_init(loop, &stdin_watcher, STDIN_FILENO);
  uv_poll_init(loop, &stdout_watcher, STDOUT_FILENO);
  uv_poll_start(&stdin_watcher, UV_READABLE, stdin_cb);
  uv_signal_init(loop, &sigint_watcher);
  uv_signal_start(&sigint_watcher, sigint_cb, SIGINT);

peace:
  return err;
}
