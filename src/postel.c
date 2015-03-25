/*
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

#include <stdlib.h>
#include <gtk/gtk.h>

static void usage(const char *argv)
{
  fprintf(stderr, "postel - version: %s\n"
                  "usage: %s [-h]\n",
                  VERSION, argv);
}

void shutdown_postel(char *fmt)
{
  shutdown_renderer();
  shutdown_cli();
  shutdown_supervisor();
}

/* welcome to the fantasy zone! get ready! */
int main(int argc, const char *argv[])
{
  int i;
  int err = EXIT_SUCCESS;
  GThread *supervisor_thread;
  GError *error = NULL;

  /* parse the command line */
  for (i = 0; i < argc; i++) {
    if (argv[i][0] == '-') {
      switch(argv[i][1]) {
        case 'h':
        default:
          usage(argv[0]);
          goto peace;
      }
    }
  }

  gtk_init(0, NULL);
  /* launch the supervisor thread */
  supervisor_thread = g_thread_try_new("supervisor", supervisor, NULL, &error);
  if (!supervisor_thread) {
    fprintf(stderr, "Error creating supervisor thread: %s\n", error->message);
    err = error->code;
    goto peace;
  }

  /* launch the renderer */
  err = renderer();

peace:
  return err;
}