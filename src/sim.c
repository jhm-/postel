/* sim.c: the supervisor, the model environment and the simulated network
 * nodes.
 * Copyright � 2015 Jack Morton <jhm@jemscout.com>
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
#include <math.h>
#include <goocanvas.h>
#include <uv.h>

extern struct global_state_struct postel;
G_LOCK_EXTERN(postel);
G_LOCK_DEFINE(node_head);

#define TREE_INSERT(head, elm) tree_insert(NULL, head, elm, 0)
#define TREE_INIT(head) *head = NULL

/* Insert a node into a 2-dimensional k-d tree */
static void tree_insert(struct node **parent, struct node **nodep, \
  struct node *nodei, int axis)
{
  int flip;

  /* Insert the node... */
  if (!*nodep) {
    nodei->tree.axis = axis;
    if (parent)
      nodei->tree.parent = (*parent);
    nodei->tree.left = nodei->tree.right = NULL;
    *nodep = nodei;
    return;
  }

  /* Or traverse the tree */
  flip = ((*nodep)->tree.axis + 1) & 1;
  if ((*nodep)->tree.axis)
    tree_insert(&(*nodep), (nodei->x < (*nodep)->x) ? &(*nodep)->tree.left : \
      &(*nodep)->tree.right, nodei, flip);
  else
    tree_insert(&(*nodep), (nodei->y < (*nodep)->y) ? &(*nodep)->tree.left : \
      &(*nodep)->tree.right, nodei, flip);
}

/* Remove a node from the k-d tree */
static void tree_remove(struct node **nodep)
{
}

/* Perform a nearest neightor search for the closest node in the tree */
static struct node *find_nearest(struct node *nodep, double x, double y)
{
  struct node *ret;
  double dist_x, dist_y;

  if (!nodep)
    return NULL;

  dist_x = x - nodep->x;
  dist_y = y - nodep->y;

  if (nodep->tree.axis)
    ret = find_nearest((dist_x <= 0.0) ? nodep->tree.left : nodep->tree.right, \
      x, y);
  else
    ret = find_nearest((dist_y <= 0.0) ? nodep->tree.left : nodep->tree.right, \
      x, y);

  return (ret) ? ret : nodep;
}

/* LOCK node_head BEFORE CALLING THIS FUNCTION! */
/* Returns -1 on failure, new node id on success */
int add_node(double x, double y)
{
  int err = 0;
  struct node *nodei = malloc(sizeof(struct node));
  if (!nodei) {
    err = -1;
    goto peace;
  }

  /* Initialize the node */
  nodei->id = (intptr_t)nodei;
  G_LOCK(postel);
  /* X and Y must not exceed the matrix size, and must be greater than zero. */
  if ((x + postel.matrix_zero) > postel.matrix_width || \
    ((y + postel.matrix_zero) > postel.matrix_height || \
     x < 0 || y < 0)) {
    err = -1;
    free(nodei);
    G_UNLOCK(postel);
    goto peace;
  }
  nodei->x = x;
  nodei->y = y;
  nodei->point = rndr_new_goo_ellipse((x + postel.matrix_zero), \
    (y + postel.matrix_zero), postel.node_p_size, \
    "line-width", 1.0, "stroke-color", "Dark Slate Gray",
    "fill-color", "Light Green", NULL);
  nodei->radius = rndr_new_goo_ellipse((x + postel.matrix_zero), \
      (y + postel.matrix_zero), postel.node_r_size, \
    "line-width", 1.0, "stroke-color", "Light Slate Gray", NULL);
  G_UNLOCK(postel);
  if (!nodei->point || !nodei->radius) {
    free(nodei);
    err = -1;
    goto peace;
  }
  TREE_INSERT(&tree_head, nodei);
  LIST_INSERT_HEAD(&node_head, nodei, nodes);

peace:
  return err;
}

/* LOCK node_head BEFORE CALLING THIS FUNCTION! */
/* Returns -1 on failure (to find node), 0 on success */
int del_node(intptr_t id)
{
  int err = -1;
  struct node *nodep;

  /* Search the queue and validate the id (pointer). */
  LIST_FOREACH(nodep, &node_head, nodes) {
    if (id == nodep->id) {
      rndr_destroy_goo_item(nodep->point);
      rndr_destroy_goo_item(nodep->radius);
      LIST_REMOVE(nodep, nodes);
      free(nodep);
      err = 0;
      break;
    }
  }
  return err;
}

void shutdown_simulator(void)
{
  struct node *nodep;

  G_LOCK(node_head);
  while (!LIST_EMPTY(&node_head)) {
    nodep = LIST_FIRST(&node_head);
    del_node(nodep->id);
  }
  G_UNLOCK(node_head);
}

gpointer init_simulator(gpointer data)
{
  uv_loop_t *loop = uv_loop_new();

  /* Initialize the node list */
  G_LOCK(node_head);
  LIST_INIT(&node_head);
  TREE_INIT(&tree_head);
  G_UNLOCK(node_head);

  /* Initialize the console */
  init_console(loop);

  /* Start the event loop */
  uv_run(loop, UV_RUN_DEFAULT);
  return NULL;
}
