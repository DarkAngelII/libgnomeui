/* Image item type for GnomeCanvas widget
 *
 * GnomeCanvas is basically a port of the Tk toolkit's most excellent canvas widget.  Tk is
 * copyrighted by the Regents of the University of California, Sun Microsystems, and other parties.
 *
 * Copyright (C) 1998 The Free Software Foundation
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#include <config.h>
#include <math.h>
#include "gnome-canvas-image.h"


enum {
	ARG_0,
	ARG_IMAGE,
	ARG_X,
	ARG_Y,
	ARG_WIDTH,
	ARG_HEIGHT,
	ARG_ANCHOR
};


static void gnome_canvas_image_class_init (GnomeCanvasImageClass *class);
static void gnome_canvas_image_init       (GnomeCanvasImage      *image);
static void gnome_canvas_image_destroy    (GtkObject             *object);
static void gnome_canvas_image_set_arg    (GtkObject             *object,
					   GtkArg                *arg,
					   guint                  arg_id);

static void   gnome_canvas_image_reconfigure (GnomeCanvasItem *item);
static void   gnome_canvas_image_realize     (GnomeCanvasItem *item);
static void   gnome_canvas_image_unrealize   (GnomeCanvasItem *item);
static void   gnome_canvas_image_draw        (GnomeCanvasItem *item, GdkDrawable *drawable,
					      int x, int y, int width, int height);
static double gnome_canvas_image_point       (GnomeCanvasItem *item, double x, double y,
					      int cx, int cy, GnomeCanvasItem **actual_item);
static void   gnome_canvas_image_translate   (GnomeCanvasItem *item, double dx, double dy);


GtkType
gnome_canvas_image_get_type (void)
{
	static GtkType image_type = 0;

	if (!image_type) {
		GtkTypeInfo image_info = {
			"GnomeCanvasImage",
			sizeof (GnomeCanvasImage),
			sizeof (GnomeCanvasImageClass),
			(GtkClassInitFunc) gnome_canvas_image_class_init,
			(GtkObjectInitFunc) gnome_canvas_image_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};

		image_type = gtk_type_unique (gnome_canvas_item_get_type (), &image_info);
	}

	return image_type;
}

static void
gnome_canvas_image_class_init (GnomeCanvasImageClass *class)
{
	GtkObjectClass *object_class;
	GnomeCanvasItemClass *item_class;

	object_class = (GtkObjectClass *) class;
	item_class = (GnomeCanvasItemClass *) class;

	gtk_object_add_arg_type ("GnomeCanvasImage::image", GTK_TYPE_POINTER, GTK_ARG_WRITABLE, ARG_IMAGE);
	gtk_object_add_arg_type ("GnomeCanvasImage::x", GTK_TYPE_DOUBLE, GTK_ARG_WRITABLE, ARG_X);
	gtk_object_add_arg_type ("GnomeCanvasImage::y", GTK_TYPE_DOUBLE, GTK_ARG_WRITABLE, ARG_Y);
	gtk_object_add_arg_type ("GnomeCanvasImage::width", GTK_TYPE_DOUBLE, GTK_ARG_WRITABLE, ARG_WIDTH);
	gtk_object_add_arg_type ("GnomeCanvasImage::height", GTK_TYPE_DOUBLE, GTK_ARG_WRITABLE, ARG_HEIGHT);
	gtk_object_add_arg_type ("GnomeCanvasImage::anchor", GTK_TYPE_ANCHOR_TYPE, GTK_ARG_WRITABLE, ARG_ANCHOR);

	object_class->destroy = gnome_canvas_image_destroy;
	object_class->set_arg = gnome_canvas_image_set_arg;

	item_class->reconfigure = gnome_canvas_image_reconfigure;
	item_class->realize = gnome_canvas_image_realize;
	item_class->unrealize = gnome_canvas_image_unrealize;
	item_class->draw = gnome_canvas_image_draw;
	item_class->point = gnome_canvas_image_point;
	item_class->translate = gnome_canvas_image_translate;
}

static void
gnome_canvas_image_init (GnomeCanvasImage *image)
{
	image->x = 0.0;
	image->y = 0.0;
	image->width = 0.0;
	image->height = 0.0;
	image->anchor = GTK_ANCHOR_CENTER;
}

static void
free_pixmap_and_mask (GnomeCanvasImage *image)
{
	if (image->pixmap)
		gdk_imlib_free_pixmap (image->pixmap);

	if (image->mask)
		gdk_imlib_free_bitmap (image->mask);

	image->pixmap = NULL;
	image->mask = NULL;
	image->cwidth = 0;
	image->cheight = 0;
}

static void
gnome_canvas_image_destroy (GtkObject *object)
{
	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_IMAGE (object));

	free_pixmap_and_mask (GNOME_CANVAS_IMAGE (object));
}

static void
recalc_bounds (GnomeCanvasImage *image)
{
	GnomeCanvasItem *item;
	double wx, wy;

	item = GNOME_CANVAS_ITEM (image);

	/* Get world coordinates */

	wx = image->x;
	wy = image->y;
	gnome_canvas_item_i2w (item, &wx, &wy);

	/* Get canvas pixel coordinates */

	gnome_canvas_w2c (item->canvas, wx, wy, &image->cx, &image->cy);

	/* Anchor image */

	switch (image->anchor) {
	case GTK_ANCHOR_NW:
	case GTK_ANCHOR_W:
	case GTK_ANCHOR_SW:
		break;

	case GTK_ANCHOR_N:
	case GTK_ANCHOR_CENTER:
	case GTK_ANCHOR_S:
		image->cx -= image->cwidth / 2;
		break;

	case GTK_ANCHOR_NE:
	case GTK_ANCHOR_E:
	case GTK_ANCHOR_SE:
		image->cx -= image->cwidth;
		break;
	}

	switch (image->anchor) {
	case GTK_ANCHOR_NW:
	case GTK_ANCHOR_N:
	case GTK_ANCHOR_NE:
		break;

	case GTK_ANCHOR_W:
	case GTK_ANCHOR_CENTER:
	case GTK_ANCHOR_E:
		image->cy -= image->cheight / 2;
		break;

	case GTK_ANCHOR_SW:
	case GTK_ANCHOR_S:
	case GTK_ANCHOR_SE:
		image->cy -= image->cheight;
		break;
	}

	/* Bounds */

	item->x1 = image->cx;
	item->y1 = image->cy;
	item->x2 = image->cx + image->cwidth;
	item->y2 = image->cy + image->cheight;

	gnome_canvas_group_child_bounds (GNOME_CANVAS_GROUP (item->parent), item);
}

static void
gnome_canvas_image_set_arg (GtkObject *object, GtkArg *arg, guint arg_id)
{
	GnomeCanvasItem *item;
	GnomeCanvasImage *image;
	int update;
	int calc_bounds;

	item = GNOME_CANVAS_ITEM (object);
	image = GNOME_CANVAS_IMAGE (object);

	update = FALSE;
	calc_bounds = FALSE;

	switch (arg_id) {
	case ARG_IMAGE:
		/* The pixmap and mask will be freed when the item is reconfigured */
		image->im = GTK_VALUE_POINTER (*arg);
		if (image->im) {
			image->width = image->im->rgb_width / item->canvas->pixels_per_unit;
			image->height = image->im->rgb_height / item->canvas->pixels_per_unit;
		} else {
			image->width = 0.0;
			image->height = 0.0;
		}

		update = TRUE;
		break;

	case ARG_X:
		image->x = GTK_VALUE_DOUBLE (*arg);
		calc_bounds = TRUE;
		break;

	case ARG_Y:
		image->y = GTK_VALUE_DOUBLE (*arg);
		calc_bounds = TRUE;
		break;

	case ARG_WIDTH:
		image->width = GTK_VALUE_DOUBLE (*arg);
		update = TRUE;
		break;

	case ARG_HEIGHT:
		image->height = GTK_VALUE_DOUBLE (*arg);
		update = TRUE;
		break;

	case ARG_ANCHOR:
		image->anchor = GTK_VALUE_ENUM (*arg);
		update = TRUE;
		break;

	default:
		break;
	}

	if (update)
		(* GNOME_CANVAS_ITEM_CLASS (item->object.klass)->reconfigure) (item);

	if (calc_bounds)
		recalc_bounds (image);
}

static void
gnome_canvas_image_reconfigure (GnomeCanvasItem *item)
{
	GnomeCanvasImage *image;

	image = GNOME_CANVAS_IMAGE (item);

	free_pixmap_and_mask (image);

	if (image->im) {
		image->cwidth = (int) (image->width * item->canvas->pixels_per_unit + 0.5);
		image->cheight = (int) (image->height * item->canvas->pixels_per_unit + 0.5);

		image->need_recalc = TRUE;
	}

	recalc_bounds (image);
}

static void
gnome_canvas_image_realize (GnomeCanvasItem *item)
{
	GnomeCanvasImage *image;

	image = GNOME_CANVAS_IMAGE (item);

	image->gc = gdk_gc_new (GTK_WIDGET (item->canvas)->window);
	(* GNOME_CANVAS_ITEM_CLASS (item->object.klass)->reconfigure) (item);
}

static void
gnome_canvas_image_unrealize (GnomeCanvasItem *item)
{
	GnomeCanvasImage *image;

	image = GNOME_CANVAS_IMAGE (item);

	gdk_gc_unref (image->gc);
}

static void
recalc_if_needed (GnomeCanvasImage *image)
{
	if (!image->need_recalc)
		return;

	gdk_imlib_render (image->im, image->cwidth, image->cheight);

	image->pixmap = gdk_imlib_move_image (image->im);
	g_assert (image->pixmap != NULL);
	image->mask = gdk_imlib_move_mask (image->im);

	if (image->gc)
		gdk_gc_set_clip_mask (image->gc, image->mask);

	image->need_recalc = FALSE;
}

static void
gnome_canvas_image_draw (GnomeCanvasItem *item, GdkDrawable *drawable,
			 int x, int y, int width, int height)
{
	GnomeCanvasImage *image;

	image = GNOME_CANVAS_IMAGE (item);

	if (!image->im)
		return;

	recalc_if_needed (image);

	if (image->mask)
		gdk_gc_set_clip_origin (image->gc, image->cx - x, image->cy - y);

	gdk_draw_pixmap (drawable,
			 image->gc,
			 image->pixmap,
			 0, 0,
			 image->cx - x,
			 image->cy - y,
			 image->cwidth,
			 image->cheight);
}

static double
dist_to_mask (GnomeCanvasImage *image, int cx, int cy)
{
	GnomeCanvasItem *item;
	GdkImage *gimage;
	GdkRectangle a, b, dest;
	int x, y, tx, ty;
	double dist, best;

	item = GNOME_CANVAS_ITEM (image);

	/* Trivial case:  if there is no mask, we are inside */

	if (!image->mask)
		return 0.0;

	/* Rectangle that we need */

	cx -= image->cx;
	cy -= image->cy;

	a.x = cx - item->canvas->close_enough;
	a.y = cy - item->canvas->close_enough;
	a.width = 2 * item->canvas->close_enough + 1;
	a.height = 2 * item->canvas->close_enough + 1;

	/* Image rectangle */

	b.x = 0;
	b.y = 0;
	b.width = image->cwidth;
	b.height = image->cheight;

	if (!gdk_rectangle_intersect (&a, &b, &dest))
		return a.width * a.height; /* "big" value */

	gimage = gdk_image_get (image->mask, dest.x, dest.y, dest.width, dest.height);

	/* Find the closest pixel */

	best = a.width * a.height; /* start with a "big" value */

	for (y = 0; y < dest.height; y++)
		for (x = 0; x < dest.width; x++)
			if (gdk_image_get_pixel (gimage, x, y)) {
				tx = x + dest.x - cx;
				ty = y + dest.y - cy;

				dist = sqrt (tx * tx + ty * ty);
				if (dist < best)
					best = dist;
			}

	gdk_image_destroy (gimage);
	return best;
}

static double
gnome_canvas_image_point (GnomeCanvasItem *item, double x, double y,
			  int cx, int cy, GnomeCanvasItem **actual_item)
{
	GnomeCanvasImage *image;
	int x1, y1, x2, y2;
	int dx, dy;

	image = GNOME_CANVAS_IMAGE (item);

	*actual_item = item;

	recalc_if_needed (image);

	x1 = image->cx - item->canvas->close_enough;
	y1 = image->cy - item->canvas->close_enough;
	x2 = image->cx + image->cwidth - 1 + item->canvas->close_enough;
	y2 = image->cy + image->cheight - 1 + item->canvas->close_enough;

	/* Hard case: is point inside image's gravity region? */

	if ((cx >= x1) && (cy >= y1) && (cx <= x2) && (cy <= y2))
		return dist_to_mask (image, cx, cy) / item->canvas->pixels_per_unit;

	/* Point is outside image */

	x1 += item->canvas->close_enough;
	y1 += item->canvas->close_enough;
	x2 -= item->canvas->close_enough;
	y2 -= item->canvas->close_enough;

	if (cx < x1)
		dx = x1 - cx;
	else if (cx > x2)
		dx = cx - x2;
	else
		dx = 0;

	if (cy < y1)
		dy = y1 - cy;
	else if (cy > y2)
		dy = cy - y2;
	else
		dy = 0;

	return sqrt (dx * dx + dy * dy) / item->canvas->pixels_per_unit;
}

static void
gnome_canvas_image_translate (GnomeCanvasItem *item, double dx, double dy)
{
	GnomeCanvasImage *image;

	image = GNOME_CANVAS_IMAGE (item);

	image->x += dx;
	image->y += dy;

	recalc_bounds (image);
}
