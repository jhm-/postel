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

#define CONSOLE_MAX_HISTORY_SIZE 255

/* IO callbacks */
static uv_signal_t sigint_watcher;
static uv_poll_t stdin_watcher, stdout_watcher;

/* We keep track of a history of commands, and manipulate a LIFO tail queue so
 * that we can recall previous commands */
struct history {
  TAILQ_ENTRY(history) histories;
  char *cmd;
};
TAILQ_HEAD(history_list, history) history_head;
static unsigned int history_stack = 0; /* a stack counter */

/* Set a filestream to non-blocking mode. returns 0 on success */
static int set_non_blocking(int fd)
{
  int val = fcntl(fd, F_GETFL, 0);
  return val == -1 ? -1 : fcntl(fd, F_SETFL, val | O_NONBLOCK);
}

/* Print to screen. watch those format strings! */
static int print_msg(const char *fmt, ...)
{
  int w;
  char log_buf[4096];
  size_t log_buf_pos = 0;
  va_list ap;

  va_start(ap, fmt);
    log_buf_pos += vsnprintf(log_buf + log_buf_pos,
      sizeof(log_buf) - log_buf_pos, fmt, ap);
  va_end(ap);

  w = write(STDOUT_FILENO, log_buf, log_buf_pos);
  if (w < 0) {
    fprintf(stderr, "Error on write() to stdout: %s\n", strerror(errno));
  }
  while (w >= 0 && w != log_buf_pos) {
    /* partial buf written, so shift position */
    memmove(log_buf, log_buf + w, log_buf_pos - w);
    log_buf_pos -= w;
  }
  return w;
}

static void print_prompt(void)
{
  /* XXX: this is it... for now */
  print_msg(":) > ");
}

/* XXX: console history is not fully implemented */
/* Pop the first piece of history off the stack */
static void pop_history(void)
{
  struct history *historyp = malloc(sizeof(struct history));
  if (!historyp)
    return;
  historyp = TAILQ_FIRST(&history_head);
  TAILQ_REMOVE(&history_head, historyp, histories);
  free(historyp);
  history_stack--;
}

/* Push a piece of history onto the end of the stack */
static void push_history(char *buf)
{
  struct history *historyp = malloc(sizeof(struct history));
  if (!historyp)
    return;

  historyp->cmd = malloc(strlen(buf));
  if (!historyp->cmd) {
    free(historyp);
    return;
  }
  strncpy(historyp->cmd, buf, strlen(buf));
  TAILQ_INSERT_TAIL(&history_head, historyp, histories);
  history_stack++;
}

static void help_console(char *fmt)
{
  /* XXX: not yet implemented */
}

static void sigint_cb(uv_signal_t *handle, int signum)
{
  shutdown_postel(NULL);
}

/* Here are the commands yo! */
#define CONSOLE_COMMANDS 2
struct commands {
  char *name;
  int args;
  char *summary;
  void (*function)(char *);
} commands[] = {
  { "quit", 0, "safely shutdown the simulation.", &shutdown_postel},
  { "help", 1, "display help for a specific topic.", &help_console}};

/* Callback when data is readable on stdin */
static void stdin_cb(uv_poll_t *handle, int status, int events)
{
  char buf[4096];
  int rv, i, n, l;

  bzero(&buf, sizeof(buf));
  rv = read(STDIN_FILENO, buf, sizeof(buf));
  if (rv >= 0) {
    /* Push the command to the history stack */
    push_history(buf);
    if (history_stack > CONSOLE_MAX_HISTORY_SIZE)
      pop_history();

    /* Process the input */
    l = FALSE;
    for (n = 1; n < strlen(buf); n++) {
      if (buf[n] == ' ' || buf[n] == '\n')
        break;
    }
    for (i = 0; i < CONSOLE_COMMANDS; i++) {
      if (strncmp(commands[i].name, buf, n) == 0) {
        l = TRUE;
        commands[i].function(buf);
      }
    }
    if (l == FALSE)
      print_msg("Invalid command: %s", buf);
  }
  else if (errno != EAGAIN && errno != EINPROGRESS) {
    fprintf(stderr, "Error on read() from stdin: %s\n", strerror(errno));
    return;
  }
  print_prompt();
}

void shutdown_console(void)
{
  /* Free the history */
  struct history *historyp;
  while (!TAILQ_EMPTY(&history_head)) {
    historyp = TAILQ_FIRST(&history_head);
    TAILQ_REMOVE(&history_head, historyp, histories);
    free(historyp);
  }
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

  /* Initialize the history list */
  TAILQ_INIT(&history_head);

  printf(" _  _  __|_ _ |\n" \
         "|_)(_)_> |_(/_|\n" \
         "|\n" \
         "postel: %s\n" \
         "copyright © 2014 Jack Morton <jhm@jemscout.com>\n" \
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