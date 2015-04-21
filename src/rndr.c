/* rndr.c: graphical rendering of the model.
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
#include <goocanvas.h>

extern struct global_state_struct postel;
G_LOCK_EXTERN(postel);

static GtkWidget *canvas;

void shutdown_renderer(void)
{
  gtk_main_quit();
}

void rndr_destroy_goo_ellipse(GooCanvasItem *ellipse)
{
  goo_canvas_item_remove(ellipse);
}

/* XXX: needs a va_list of properties */
GooCanvasItem *rndr_new_goo_ellipse(gdouble x, gdouble y, unsigned int size)
{
  GooCanvasItem *root = goo_canvas_get_root_item(GOO_CANVAS(canvas));
  GooCanvasItem *ellipse = goo_canvas_ellipse_new(root, x, y, size, size,
    "stroke-color", "red",
    "line-width", 1.0, NULL);
  return ellipse;
}

int init_renderer(void)
{
  int err = 0; /* XXX: initialize */
  GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
  canvas = goo_canvas_new();

  /* XXX: error checking? */
  gtk_container_add(GTK_CONTAINER(window), scrolled_window);
  gtk_container_add(GTK_CONTAINER(scrolled_window), canvas);
  G_LOCK(postel);
  goo_canvas_set_bounds(GOO_CANVAS(canvas), 0, 0, postel.matrix_width,
  	postel.matrix_height);
  G_UNLOCK(postel);
  gtk_widget_show_all(window);

  gtk_main();
  return err;
}
