/* sim.c: the supervisor, the model environment and the simulated network
 * nodes.
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
#include <limits.h>
#include <goocanvas.h>
#include <uv.h>

extern struct global_state_struct postel;
G_LOCK_EXTERN(postel);
G_LOCK_DEFINE(node_head);

/* LOCK node_head BEFORE CALLING THIS FUNCTION! */
/* Returns -1 on failure, new node id on success */
int add_node(gdouble x, gdouble y)
{
  int err;
  unsigned int i = 0;
  struct node *nodei = malloc(sizeof(struct node));
  struct node *nodep = malloc(sizeof(struct node));

  if (!nodep || !nodei) {
    err = -1;
    goto peace;
  }
  /* Check for the next greatest unique identifier. the id is always incremental
   * despite the nodes in existence, but realistically we should never hit this
   * limit */
  TAILQ_FOREACH(nodep, &node_head, nodes) {
    i = (nodep->id > i) ? nodep->id : i;
  }
  free(nodep);
  if (i > UINT_MAX) {
    /* No more identifiers! */
    err = -1;
    goto peace;
  }

  /* Initialize the node */
  nodei->id = ++i;
  G_LOCK(postel);
  nodei->point = rndr_new_goo_ellipse(x, y, postel.node_p_size);
  nodei->radius = rndr_new_goo_ellipse(x, y, postel.node_r_size);
  G_UNLOCK(postel);
  if (!nodei->point || !nodei->radius) {
    free(nodei);
    err = -1;
    goto peace;
  }
  TAILQ_INSERT_TAIL(&node_head, nodei, nodes);
  /* Success! */
  err = nodei->id;

peace:
  return err;
}

/* LOCK node_head BEFORE CALLING THIS FUNCTION! */
/* Returns -1 on failure (to find node), 0 on success */
int del_node(unsigned int id)
{
  int err = -1;
  struct node *nodep;

  TAILQ_FOREACH(nodep, &node_head, nodes) {
    if (id == nodep->id) {
      err = 0;
      rndr_destroy_goo_ellipse(nodep->point);
      rndr_destroy_goo_ellipse(nodep->radius);
      TAILQ_REMOVE(&node_head, nodep, nodes);
      free(nodep);
      break;
    }
  }
  return err;
}

/* LOCK node_head BEFORE CALLING THIS FUNCTION, AND MAKING CHANGES TO THE
 * NODE! */
/* Returns a valid pointer to the node, or NULL if not found. */
struct node *get_node(unsigned int id)
{
  struct node *nodep = NULL;

  TAILQ_FOREACH(nodep, &node_head, nodes) {
    if (id == nodep->id)
      break;
  }
  return nodep;
}

void shutdown_simulator(void)
{
  struct node *nodep;

  G_LOCK(node_head);
  while (!TAILQ_EMPTY(&node_head)) {
    nodep = TAILQ_FIRST(&node_head);
    del_node(nodep->id);
  }
  G_UNLOCK(node_head);
}

gpointer init_simulator(gpointer data)
{
  uv_loop_t *loop = uv_loop_new();

  /* Initialize the node list */
  G_LOCK(node_head);
  TAILQ_INIT(&node_head);
  G_UNLOCK(node_head);

  /* Initialize the console */
  init_console(loop);

  /* Start the event loop */
  uv_run(loop, UV_RUN_DEFAULT);
  return NULL;
}