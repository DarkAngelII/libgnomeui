/* GnomeCanvas widget - Tk-like canvas widget for Gnome
 *
 * GnomeCanvas is basically a port of the Tk toolkit's most excellent canvas
 * widget.  Tk is copyrighted by the Regents of the University of California,
 * Sun Microsystems, and other parties.
 *
 * Copyright (C) 1998 The Free Software Foundation
 *
 * Authors: Federico Mena <federico@nuclecu.unam.mx>
 *          Raph Levien <raph@gimp.org>
 */

#ifndef GNOME_CANVAS_H
#define GNOME_CANVAS_H

#include <libgnome/gnome-defs.h>
#include <gtk/gtklayout.h>
#include <stdarg.h>
#include <libart_lgpl/art_misc.h>
#include <libart_lgpl/art_rect.h>
#include <libart_lgpl/art_svp.h>
#include <libart_lgpl/art_uta.h>

BEGIN_GNOME_DECLS


/* "Small" value used by canvas stuff */
#define GNOME_CANVAS_EPSILON 1e-10


/* Macros for building colors.  The values are in [0, 255] */

#define GNOME_CANVAS_COLOR(r, g, b) ((((int) (r) & 0xff) << 24)	\
				     | (((int) (g) & 0xff) << 16)	\
				     | (((int) (b) & 0xff) << 8)	\
				     | 0xff)

#define GNOME_CANVAS_COLOR_A(r, g, b, a) ((((int) (r) & 0xff) << 24)	\
					  | (((int) (g) & 0xff) << 16)	\
					  | (((int) (b) & 0xff) << 8)	\
					  | ((int) (a) & 0xff))


typedef struct _GnomeCanvas           GnomeCanvas;
typedef struct _GnomeCanvasClass      GnomeCanvasClass;
typedef struct _GnomeCanvasGroup      GnomeCanvasGroup;
typedef struct _GnomeCanvasGroupClass GnomeCanvasGroupClass;
typedef struct _GnomeCanvasBuf        GnomeCanvasBuf;


/* GnomeCanvasItem - base item class for canvas items
 *
 * All canvas items are derived from GnomeCanvasItem.  The only information a GnomeCanvasItem
 * contains is its parent canvas, its parent canvas item group, and its bounding box in canvas pixel
 * coordinates.
 *
 * Items inside a canvas are organized in a tree of GnomeCanvasItemGroup nodes and GnomeCanvasItem
 * leaves.  Each canvas has a single root group, which can be obtained with the
 * gnome_canvas_get_root() function.
 *
 * The abstract GnomeCanvasItem class does not have any configurable or queryable attributes.
 */


/* Object flags for items */
enum {
	GNOME_CANVAS_ITEM_REALIZED      = 1 << 4,
	GNOME_CANVAS_ITEM_MAPPED        = 1 << 5,
	GNOME_CANVAS_ITEM_ALWAYS_REDRAW = 1 << 6,
	GNOME_CANVAS_ITEM_VISIBLE       = 1 << 7,
	GNOME_CANVAS_ITEM_NEED_UPDATE	= 1 << 8
};

/* Update flags for items */
enum {
	GNOME_CANVAS_UPDATE_REQUESTED  = 1 << 0,
	GNOME_CANVAS_UPDATE_AFFINE     = 1 << 1,
	GNOME_CANVAS_UPDATE_CLIP       = 1 << 2,
	GNOME_CANVAS_UPDATE_VISIBILITY = 1 << 3
};

/* Data for rendering in antialiased mode */

struct _GnomeCanvasBuf {
	guchar *buf;			/* 24 bit RGB buffer for rendering */
	int buf_rowstride;		/* The rowstride for buf */
	ArtIRect rect;			/* The rectangle describing the
					 * rendering area.
					 */
	guint32 bg_color;		/* The background color in 0xrrggbb */

	/* Invariant: at least one of the following flags is true. */

	unsigned int is_bg : 1;		/* Set when the render rectangle area is
					 * the solid color bg_color.
					 */
	unsigned int is_buf : 1;	/* Set when the render rectangle area is
					 * represented by the buf
					 */
};


#define GNOME_TYPE_CANVAS_ITEM            (gnome_canvas_item_get_type ())
#define GNOME_CANVAS_ITEM(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_CANVAS_ITEM, GnomeCanvasItem))
#define GNOME_CANVAS_ITEM_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_CANVAS_ITEM, GnomeCanvasItemClass))
#define GNOME_IS_CANVAS_ITEM(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_CANVAS_ITEM))
#define GNOME_IS_CANVAS_ITEM_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_CANVAS_ITEM))


typedef struct _GnomeCanvasItem GnomeCanvasItem;
typedef struct _GnomeCanvasItemClass GnomeCanvasItemClass;

struct _GnomeCanvasItem {
	GtkObject object;

	GnomeCanvas *canvas;		/* The parent canvas for this item */
	GnomeCanvasItem *parent;	/* The parent canvas group (of type GnomeCanvasGroup) */

	double x1, y1, x2, y2;		/* Bounding box for this item, in canvas pixel coordinates.
					 * The bounding box contains (x1, y1) but not (x2, y2).
					 */
};

struct _GnomeCanvasItemClass {
	GtkObjectClass parent_class;

	/* Tell the item to update itself.  The flags are from the update flags
	 * defined above.  The item should update its internal state from its
	 * queued state, recompute and request its repaint area, etc.  The
	 * affine, if used, is a pointer to a 6-element array of doubles.
	 */
	void (* update) (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags);

	/* Realize an item -- create GCs, etc. */
	void (* realize) (GnomeCanvasItem *item);

	/* Unrealize an item */
	void (* unrealize) (GnomeCanvasItem *item);

	/* Map an item - normally only need by items with their own GdkWindows */
	void (* map) (GnomeCanvasItem *item);

	/* Unmap an item */
	void (* unmap) (GnomeCanvasItem *item);

	/* Return the microtile coverage of the item */
	ArtUta *(* coverage) (GnomeCanvasItem *item);

	/* Draw an item of this type.  (x, y) are the upper-left canvas pixel coordinates of the *
	 * drawable, a temporary pixmap, where things get drawn.  (width, height) are the dimensions
	 * of the drawable.
	 */
	void (* draw) (GnomeCanvasItem *item, GdkDrawable *drawable,
		       int x, int y, int width, int height);

	/* Render the item over the buffer given.  The buf data structure
	 * contains both a pointer to a packed 24-bit RGB array, and the
	 * coordinates.  This method is only used for libart-based canvases.
	 *
	 * TODO: figure out where affine transforms and clip paths fit into the
	 * rendering framework.
	 */
	void (* render) (GnomeCanvasItem *item, GnomeCanvasBuf *buf);

	/* Calculate the distance from an item to the specified point.  It also returns a canvas
         * item which is the item itself in the case of the object being an actual leaf item, or a
         * child in case of the object being a canvas group.  (cx, cy) are the canvas pixel
         * coordinates that correspond to the item-relative coordinates (x, y).
	 */
	double (* point) (GnomeCanvasItem *item, double x, double y, int cx, int cy, GnomeCanvasItem **actual_item);

/* FIXME: remove ::translate and ::bounds */

	/* Move an item by the specified amount */
	void (* translate) (GnomeCanvasItem *item, double dx, double dy);

	/* Fetch the item's bounding box (need not be exactly tight) */
	void (* bounds) (GnomeCanvasItem *item, double *x1, double *y1, double *x2, double *y2);

	/* Signal: an event ocurred for an item of this type.  The (x, y) coordinates are in the
	 * canvas world coordinate system.
	 */
	gint (* event) (GnomeCanvasItem *item, GdkEvent *event);
};


/* Standard Gtk function */
GtkType gnome_canvas_item_get_type (void);

/* Create a canvas item using the standard Gtk argument mechanism.  The item is automatically
 * inserted at the top of the specified canvas group.  The last argument must be a NULL pointer.
 */
GnomeCanvasItem *gnome_canvas_item_new (GnomeCanvasGroup *parent, GtkType type, const gchar *first_arg_name, ...);

/* Same as above, with parsed args */
GnomeCanvasItem *gnome_canvas_item_newv (GnomeCanvasGroup *parent, GtkType type, guint nargs, GtkArg *args);

/* Constructors for use in derived classes and language wrappers */
void gnome_canvas_item_construct (GnomeCanvasItem *item, GnomeCanvasGroup *parent, const gchar *first_arg_name, va_list args);

void gnome_canvas_item_constructv (GnomeCanvasItem *item, GnomeCanvasGroup *parent,
				   guint nargs, GtkArg *args);

/* Configure an item using the standard Gtk argument mechanism.  The last argument must be a NULL pointer. */
void gnome_canvas_item_set (GnomeCanvasItem *item, const gchar *first_arg_name, ...);

/* Same as above, with parsed args */
void gnome_canvas_item_setv (GnomeCanvasItem *item, guint nargs, GtkArg *args);

/* Used only for language wrappers and the like; ignore. */
void gnome_canvas_item_set_valist (GnomeCanvasItem *item, const gchar *first_arg_name,
				   va_list var_args);

/* Move an item by the specified amount */
void gnome_canvas_item_move (GnomeCanvasItem *item, double dx, double dy);

/* Scale an item about a point by the specified factors */
void gnome_canvas_item_scale (GnomeCanvasItem *item, double x, double y, double scale_x, double scale_y);

/* Rotate an item about a point by the specified number of degrees */
void gnome_canvas_item_rotate (GnomeCanvasItem *item, double x, double y, double angle);

/* Apply the specified transformation matrix to an item */
void gnome_canvas_item_transform (GnomeCanvasItem *item, double *affine);

/* Raise an item in the z-order of its parent group by the specified
 * number of positions.  The specified number must be larger than or
 * equal to 1.
 */
void gnome_canvas_item_raise (GnomeCanvasItem *item, int positions);

/* Lower an item in the z-order of its parent group by the specified
 * number of positions.  The specified number must be larger than or
 * equal to 1.
 */
void gnome_canvas_item_lower (GnomeCanvasItem *item, int positions);

/* Raise an item to the top of its parent group's z-order. */
void gnome_canvas_item_raise_to_top (GnomeCanvasItem *item);

/* Lower an item to the bottom of its parent group's z-order */
void gnome_canvas_item_lower_to_bottom (GnomeCanvasItem *item);

/* Show an item (make it visible).  If the item is already shown, it has no effect. */
void gnome_canvas_item_show (GnomeCanvasItem *item);

/* Hide an item (make it invisible).  If the item is already invisible, it has no effect. */
void gnome_canvas_item_hide (GnomeCanvasItem *item);

/* Grab the mouse for the specified item.  Only the events in event_mask will be reported.  If
 * cursor is non-NULL, it will be used during the duration of the grab.  Time is a proper X event
 * time parameter.  Returns the same values as XGrabPointer().
 */
int gnome_canvas_item_grab (GnomeCanvasItem *item, unsigned int event_mask, GdkCursor *cursor, guint32 etime);

/* Ungrabs the mouse -- the specified item must be the same that was passed to
 * gnome_canvas_item_grab().  Time is a proper X event time parameter.
 */
void gnome_canvas_item_ungrab (GnomeCanvasItem *item, guint32 etime);

/* These functions convert from a coordinate system to another.  "w" is world coordinates and "i" is
 * item coordinates.
 */
void gnome_canvas_item_w2i (GnomeCanvasItem *item, double *x, double *y);
void gnome_canvas_item_i2w (GnomeCanvasItem *item, double *x, double *y);

/* Remove the item from its parent group and make the new group its parent.  The item will be put on
 * top of all the items in the new group.  The item's coordinates relative to its new parent to
 * *not* change -- this means that the item could potentially move on the screen.
 * 
 * The item and the group must be in the same canvas.  An item cannot be reparented to a group that
 * is the item itself or that is an inferior of the item.
 */
void gnome_canvas_item_reparent (GnomeCanvasItem *item, GnomeCanvasGroup *new_group);

/* Used to send all of the keystroke events to a specific item as well as GDK_FOCUS_CHANGE events. */
void gnome_canvas_item_grab_focus (GnomeCanvasItem *item);

/* Fetch the bounding box of the item.  The bounding box may not be exactly tight, but the canvas
 * items will do the best they can.
 */
void gnome_canvas_item_get_bounds (GnomeCanvasItem *item, double *x1, double *y1, double *x2, double *y2);

/* Request that the update method eventually gets called. */
void gnome_canvas_item_request_update (GnomeCanvasItem *item);


/* GnomeCanvasGroup - a group of canvas items
 *
 * A group is a node in the hierarchical tree of groups/items inside a canvas.  Groups serve to
 * give a logical structure to the items.
 *
 * Consider a circuit editor application that uses the canvas for its schematic display.
 * Hierarchically, there would be canvas groups that contain all the components needed for an
 * "adder", for example -- this includes some logic gates as well as wires.  You can move stuff
 * around in a convenient way by doing a gnome_canvas_item_move() of the hierarchical groups -- to
 * move an adder, simply move the group that represents the adder.
 *
 * The (xpos, ypos) fields of a canvas group are the relative origin for the group's children.
 *
 * The following arguments are available:
 *
 * name		type		read/write	description
 * --------------------------------------------------------------------------------
 * x		double		RW		X coordinate of group's origin
 * y		double		RW		Y coordinate of group's origin
 */


#define GNOME_TYPE_CANVAS_GROUP            (gnome_canvas_group_get_type ())
#define GNOME_CANVAS_GROUP(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_CANVAS_GROUP, GnomeCanvasGroup))
#define GNOME_CANVAS_GROUP_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_CANVAS_GROUP, GnomeCanvasGroupClass))
#define GNOME_IS_CANVAS_GROUP(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_CANVAS_GROUP))
#define GNOME_IS_CANVAS_GROUP_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_CANVAS_GROUP))


struct _GnomeCanvasGroup {
	GnomeCanvasItem item;

	GList *item_list;
	GList *item_list_end;

	double xpos, ypos;	/* Point that defines the group's origin */
};

struct _GnomeCanvasGroupClass {
	GnomeCanvasItemClass parent_class;
};


/* Standard Gtk function */
GtkType gnome_canvas_group_get_type (void);

/* For use only by the core and item type implementations.  If item is non-null, then the group adds
 * the item's bounds to the current group's bounds, and propagates it upwards in the item hierarchy.
 * If item is NULL, then the group asks every sub-group to recalculate its bounds recursively, and
 * then propagates the bounds change upwards in the hierarchy.
 */
void gnome_canvas_group_child_bounds (GnomeCanvasGroup *group, GnomeCanvasItem *item);


/*** GnomeCanvas ***/


#define GNOME_TYPE_CANVAS            (gnome_canvas_get_type ())
#define GNOME_CANVAS(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_CANVAS, GnomeCanvas))
#define GNOME_CANVAS_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_CANVAS, GnomeCanvasClass))
#define GNOME_IS_CANVAS(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_CANVAS))
#define GNOME_IS_CANVAS_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_CANVAS))


struct _GnomeCanvas {
	GtkLayout layout;

	guint idle_id;				/* Idle handler ID */

	GnomeCanvasItem *root;			/* root canvas group */
	guint root_destroy_id;			/* Signal handler ID for destruction of root item */

	double scroll_x1, scroll_y1;		/* scrolling limit */
	double scroll_x2, scroll_y2;

	double pixels_per_unit;			/* scaling factor to be used for display */

	int redraw_x1, redraw_y1;
	int redraw_x2, redraw_y2;		/* Area that needs redrawing.  Contains (x1, y1)
						 * but not (x2, y2) -- specified in canvas pixel units.
						 */

	ArtUta *redraw_area;			/* Area that needs redrawing, stored as microtiles */

	int draw_xofs, draw_yofs;		/* Offsets of the temporary drawing pixmap */

	int zoom_xofs, zoom_yofs; 		/* Internal pixel offsets for when zoomed out */

	int state;				/* Last known modifier state, for deferred repick
						 * when a button is down
						 */

	GnomeCanvasItem *current_item;		/* The item containing the mouse pointer, or NULL if none */
	GnomeCanvasItem *new_current_item;	/* Item that is about to become current
						 * (used to track deletions and such)
						 */
	GnomeCanvasItem *grabbed_item;		/* Item that holds a pointer grab, or NULL if none */
	guint grabbed_event_mask;		/* Event mask specified when grabbing an item */

	GnomeCanvasItem *focused_item;		/* If non-NULL the currently focused item */

	GdkEvent pick_event;			/* Event on which selection of current item is based */

	int close_enough;			/* Tolerance distance for picking items */

	GdkColorContext *cc;			/* Color context used for color allocation */
	GdkGC *pixmap_gc;			/* GC for temporary pixmap */

	unsigned int need_update : 1;		/* Will update at next idle loop iteration */
	unsigned int need_redraw : 1;		/* Will redraw at next idle loop iteration */
	unsigned int need_repick : 1;		/* Will repick current item at next idle loop iteration */
	unsigned int left_grabbed_item : 1;	/* For use by the internal pick_event function */
	unsigned int in_repick : 1;		/* For use by the internal pick_event function */

	unsigned int aa : 1;			/* antialiased rendering */
};

struct _GnomeCanvasClass {
	GtkLayoutClass parent_class;
};


/* Standard Gtk function */
GtkType gnome_canvas_get_type (void);

/* Creates a new canvas.  You should check that the canvas is created with the proper visual and
 * colormap if you want to insert imlib images into it.  You can do this by doing
 * gtk_widget_push_visual(gdk_imlib_get_visual()) and
 * gtk_widget_push_colormap(gdk_imlib_get_colormap()) before calling gnome_canvas_new().  After
 * calling it, you should do gtk_widget_pop_visual() and gtk_widget_pop_colormap().
 *
 * You should call gnome_canvas_set_scroll_region() soon after calling this function to set the
 * desired scrolling limits for the canvas.
 */
GtkWidget *gnome_canvas_new (void);

/* Returns the root canvas item group of the canvas */
GnomeCanvasGroup *gnome_canvas_root (GnomeCanvas *canvas);

/* Sets the limits of the scrolling region */
void gnome_canvas_set_scroll_region (GnomeCanvas *canvas, double x1, double y1, double x2, double y2);

/* Gets the limits of the scrolling region */
void gnome_canvas_get_scroll_region (GnomeCanvas *canvas, double *x1, double *y1, double *x2, double *y2);

/* Sets the number of pixels that correspond to one unit in world coordinates */
void gnome_canvas_set_pixels_per_unit (GnomeCanvas *canvas, double n);

/* Scrolls the canvas to the specified offsets, given in canvas pixel coordinates */
void gnome_canvas_scroll_to (GnomeCanvas *canvas, int cx, int cy);

/* Returns the scroll offsets of the canvas in canvas pixel coordinates.  You can specify NULL for
 * any of the values, in which case that value will not be queried.
 */
void gnome_canvas_get_scroll_offsets (GnomeCanvas *canvas, int *cx, int *cy);

/* Requests that the canvas be repainted immediately instead of in the idle loop. */
void gnome_canvas_update_now (GnomeCanvas *canvas);

/* For use only by item type implementations. Request that the canvas eventually redraw the
 * specified region. The region is specified as a microtile array. This function takes over
 * responsibility for freeing the uta argument.
 */
void gnome_canvas_request_redraw_uta (GnomeCanvas *canvas, ArtUta *uta);

/* For use only by item type implementations.  Request that the canvas eventually redraw the
 * specified region.  The region contains (x1, y1) but not (x2, y2).
 */
void gnome_canvas_request_redraw (GnomeCanvas *canvas, int x1, int y1, int x2, int y2);

/* These functions convert from a coordinate system to another.  "w" is world coordinates (the ones
 * in which objects are specified), "c" is canvas coordinates (pixel coordinates that are (0,0) for
 * the upper-left scrolling limit and something else for the lower-left scrolling limit).
 */
void gnome_canvas_w2c (GnomeCanvas *canvas, double wx, double wy, int *cx, int *cy);
void gnome_canvas_w2c_d (GnomeCanvas *canvas, double wx, double wy, double *cx, double *cy);
void gnome_canvas_c2w (GnomeCanvas *canvas, int cx, int cy, double *wx, double *wy);

/* This function takes in coordinates relative to the GTK_LAYOUT (canvas)->bin_window and converts
 * them to world coordinates.
 */
void gnome_canvas_window_to_world (GnomeCanvas *canvas, double winx, double winy, double *worldx, double *worldy);

/* This is the inverse of gnome_canvas_window_to_world */
void gnome_canvas_world_to_window (GnomeCanvas *canvas, double worldx, double worldy, double *winx, double *winy);

/* Takes a string specification for a color and allocates it into the specified GdkColor.  If the
 * string is null, then it returns FALSE. Otherwise, it returns TRUE.
 */
int gnome_canvas_get_color (GnomeCanvas *canvas, char *spec, GdkColor *color);

/* Sets the stipple origin of the specified gc so that it will be aligned with all the stipples used
 * in the specified canvas.  This is intended for use only by canvas item implementations.
 */
void gnome_canvas_set_stipple_origin (GnomeCanvas *canvas, GdkGC *gc);


END_GNOME_DECLS

#endif
