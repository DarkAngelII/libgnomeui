/* Color picker button for GNOME
 *
 * Copyright (C) 1998 Red Hat Software, Inc.
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#ifndef GNOME_COLOR_PICKER_H
#define GNOME_COLOR_PICKER_H

#include <libgnome/gnome-defs.h>
#include <gtk/gtkbutton.h>
#include <gdk_imlib.h>


BEGIN_GNOME_DECLS


/* The GnomeColorPicker widget is a simple color picker in a button.  The button displays a sample
 * of the currently selected color.  When the user clicks on the button, a color selection dialog
 * pops up.  The color picker emits the "color_changed" signal when the color is set
 *
 * By default, the color picker does dithering when drawing the color sample box.  This can be
 * disabled for cases where it is useful to see the allocated color without dithering.
 */

#define GNOME_TYPE_COLOR_PICKER            (gnome_color_picker_get_type ())
#define GNOME_COLOR_PICKER(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_COLOR_PICKER, GnomeColorPicker))
#define GNOME_COLOR_PICKER_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_COLOR_PICKER, GnomeColorPickerClass))
#define GNOME_IS_COLOR_PICKER(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_COLOR_PICKER))
#define GNOME_IS_COLOR_PICKER_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_COLOR_PICKER))


typedef struct _GnomeColorPicker GnomeColorPicker;
typedef struct _GnomeColorPickerClass GnomeColorPickerClass;

struct _GnomeColorPicker {
	GtkButton button;

	double r, g, b, a;	/* Red, green, blue, and alpha values */

	GdkImlibImage *im;	/* Imlib image for rendering dithered sample */
	GdkPixmap *pixmap;	/* Pixmap with the sample contents */
	GdkGC *gc;		/* GC for drawing */

	GtkWidget *da;		/* Drawing area for color sample */
	GtkWidget *cs_dialog;	/* Color selection dialog */

	char *title;		/* Title for the color selection window */

	int dither : 1;		/* Dither or just paint a solid color? */
	int use_alpha : 1;	/* Use alpha or not */
};

struct _GnomeColorPickerClass {
	GtkButtonClass parent_class;

	/* Signal that is emitted when the color is set.  The rgba values are in the [0, 65535]
	 * range.  If you need a different color format, use the provided functions to get the
	 * values from the color picker.
	 */
	void (* color_set) (GnomeColorPicker *cp, gint r, gint g, gint b, gint a);
};


/* Standard Gtk function */
GtkType gnome_color_picker_get_type (void);

/* Creates a new color picker widget */
GtkWidget *gnome_color_picker_new (void);

/* Set/get the color in the picker.  Values are in [0.0, 1.0] */
void gnome_color_picker_set_d (GnomeColorPicker *cp, double r, double g, double b, double a);
void gnome_color_picker_get_d (GnomeColorPicker *cp, double *r, double *g, double *b, double *a);

/* Set/get the color in the picker.  Values are in [0, 255] */
void gnome_color_picker_set_i8 (GnomeColorPicker *cp, int r, int g, int b, int a);
void gnome_color_picker_get_i8 (GnomeColorPicker *cp, int *r, int *g, int *b, int *a);

/* Set/get the color in the picker.  Values are in [0, 65535] */
void gnome_color_picker_set_i16 (GnomeColorPicker *cp, int r, int g, int b, int a);
void gnome_color_picker_get_i16 (GnomeColorPicker *cp, int *r, int *g, int *b, int *a);

/* Sets whether the picker should dither the color sample or just paint a solid rectangle */
void gnome_color_picker_set_dither (GnomeColorPicker *cp, int dither);

/* Sets whether the picker should use the alpha channel or not */
void gnome_color_picker_set_use_alpha (GnomeColorPicker *cp, int use_alpha);

/* Sets the title for the color selection dialog */
void gnome_color_picker_set_title (GnomeColorPicker *cp, char *title);


END_GNOME_DECLS

#endif
