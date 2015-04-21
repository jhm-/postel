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

/* Initialize the global configuration state and protect with a lock */
struct global_state_struct postel = {
  DEFAULT_MATRIX_WIDTH,
  DEFAULT_MATRIX_HEIGHT,
  DEFAULT_NODE_POINT_SIZE,
  DEFAULT_NODE_RADIUS_SIZE
};
G_LOCK_DEFINE(postel);

static void usage(const char *argv)
{
  fprintf(stderr, "postel - version: %s\n"
                  "usage: %s [-h]\n",
                  VERSION, argv);
}

void shutdown_postel(char *fmt)
{
  shutdown_console();
  shutdown_simulator();
  shutdown_renderer();
}

/* Welcome to the fantasy zone! Get ready! */
int main(int argc, const char *argv[])
{
  int i;
  int err = EXIT_SUCCESS;
  GThread *sim_thread;
  GError *error = NULL;

  /* Parse the command line */
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

  /* The simulation is run in a seperate thread, which includes a console
   * interface to supervise, modify network topography and control the flow of
   * execution. The gtk renderer is initiated from the main postel thread, and
   * provides a visual representation of the simulation, with a more narrow
   * interface to the model parameters than the console. */

  gtk_init(0, NULL);
  /* Launch the supervisor thread */
  sim_thread = g_thread_try_new("simulator", init_simulator, NULL, &error);
  if (!sim_thread) {
    fprintf(stderr, "Error creating simulator thread: %s\n", error->message);
    err = error->code;
    goto peace;
  }

  /* Launch the renderer */
  err = init_renderer();

peace:
  return err;
}
