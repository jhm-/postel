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
#include "queue.h"

#include <stdint.h>
#include <goocanvas.h>
#include <uv.h>

#define VERSION "0.1.0-alpha"

/* Rendering defaults */
#define DEFAULT_MATRIX_WIDTH 2048
#define DEFAULT_MATRIX_HEIGHT 2048
#define DEFAULT_NODE_POINT_SIZE 16
#define DEFAULT_NODE_RADIUS_SIZE 128

/* Define TRUE/FALSE */
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

/* A structure for the global state of postel */
struct global_state_struct {
  unsigned int matrix_width;
  unsigned int matrix_height;
  unsigned int matrix_zero;
  unsigned int node_p_size;
  unsigned int node_r_size;
};

/* The structure for each network node. _Any_ operation on a node, is protected
 * by a lock on node_head defined in sim.c */
struct node {
  LIST_ENTRY(node) nodes;
  intptr_t id;
  double x, y;
  GooCanvasItem *point, *radius;
  /* The structure for the k-d tree topology */
  struct {
    int depth;
    int axis;
    struct node *parent;
    struct node *left, *right;
  } tree;
};
LIST_HEAD(node_list, node) node_head;

/* Prototypes */
/* Initialize */
gpointer init_simulator(gpointer data);
int init_console(uv_loop_t *loop);
int init_renderer(void);

/* Render-thread GooCanvas functions */
GooCanvasItem *rndr_new_goo_line(gdouble x1, gdouble y1, gdouble x2, \
  gdouble y2, const char *properties, ...);
GooCanvasItem *rndr_new_goo_ellipse(gdouble x, gdouble y, unsigned int size, \
  const char *properties, ...);
void rndr_destroy_goo_item(GooCanvasItem *item);

/* Simulation control */
int add_node(double x, double y);
int del_node(intptr_t id);

/* Shutdown */
void shutdown_postel(int err, char **msg);
void shutdown_console(void);
void shutdown_simulator(void);
void shutdown_renderer(void);
