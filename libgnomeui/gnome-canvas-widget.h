/* Widget item type for GnomeCanvas widget
 *
 * GnomeCanvas is basically a port of the Tk toolkit's most excellent canvas widget.  Tk is
 * copyrighted by the Regents of the University of California, Sun Microsystems, and other parties.
 *
 * Copyright (C) 1998 The Free Software Foundation
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#ifndef GNOME_CANVAS_WIDGET_H
#define GNOME_CANVAS_WIDGET_H

#include <libgnome/gnome-defs.h>
#include <gtk/gtkpacker.h> /* why the hell is GtkAnchorType here and not in gtkenums.h? */
#include "gnome-canvas.h"


BEGIN_GNOME_DECLS


#define GNOME_TYPE_CANVAS_WIDGET            (gnome_canvas_widget_get_type ())
#define GNOME_CANVAS_WIDGET(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_CANVAS_WIDGET, GnomeCanvasWidget))
#define GNOME_CANVAS_WIDGET_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_CANVAS_WIDGET, GnomeCanvasWidgetClass))
#define GNOME_IS_CANVAS_WIDGET(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_CANVAS_WIDGET))
#define GNOME_IS_CANVAS_WIDGET_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_CANVAS_WIDGET))


typedef struct _GnomeCanvasWidget GnomeCanvasWidget;
typedef struct _GnomeCanvasWidgetClass GnomeCanvasWidgetClass;

struct _GnomeCanvasWidget {
	GnomeCanvasItem item;

	GtkWidget *widget;		/* The child widget */

	double x, y;			/* Position at anchor */
	double width, height;		/* Dimensions of widget */
	GtkAnchorType anchor;		/* Anchor side for widget */

	int cx, cy;			/* Top-left canvas coordinates for widget */
	int cwidth, cheight;		/* Size of widget in pixels */

	guint destroy_id;		/* Signal connection id for destruction of child widget */

	int size_pixels : 1;		/* Is size specified in (unchanging) pixels or units (get scaled)? */
	int in_destroy : 1;		/* Is child widget being destroyed? */
};

struct _GnomeCanvasWidgetClass {
	GnomeCanvasItemClass parent_class;
};


/* Standard Gtk function */
GtkType gnome_canvas_widget_get_type (void);


END_GNOME_DECLS

#endif
