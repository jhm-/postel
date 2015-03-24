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

struct node {
  TAILQ_ENTRY(node) nodes;
  unsigned int id;
  GooCanvasItem *point, *radius;
};
TAILQ_HEAD(node_list, node) node_head;
static unsigned int nodenum = 0; /* a stack counter. nodenum should always be
  less than UINT_MAX if node->id is less than UINT_MAX */

/* returns -1 on failure, new node id on success */
int add_node(gdouble x, gdouble y)
{
  unsigned int i;
  struct node *nodei = malloc(sizeof(struct node));
  struct node *nodep = malloc(sizeof(struct node));

  /* check for the next greatest unique identifier. the id is always incremental
     despite the nodes in existence, but realistically we should never hit this
     limit */
  TAILQ_FOREACH(nodep, &node_head, nodes) {
    i = (nodep->id > i) ? nodep->id : i;
  }
  free(nodep);
  if (i < UINT_MAX) {
  	nodei->id = i++;
    TAILQ_INSERT_TAIL(&node_head, nodei, nodes);
    nodenum++;
    return nodei->id;
  }
  else
  	/* no more identifiers! */
  	return -1;
}

/* returns -1 on failure (to find node), 0 on success */
int del_node(unsigned int id)
{
  int err = -1;
  struct node *nodep;

  TAILQ_FOREACH(nodep, &node_head, nodes) {
    if (id == nodep->id) {
      err = 0;
      TAILQ_REMOVE(&node_head, nodep, nodes);
      free(nodep);
      nodenum--;
      break;
    }
  }
  return err;
}

gpointer supervisor(gpointer data)
{
  uv_loop_t *loop = uv_loop_new();

  /* initialize the node list */
  TAILQ_INIT(&node_head);
  /* initialize the CLI */
  init_cli(loop);
  /* XXX: message the rendering thread */
  /* start the event loop */
  uv_run(loop, UV_RUN_DEFAULT);
  return NULL;
}