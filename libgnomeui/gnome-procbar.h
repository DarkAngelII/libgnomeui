/* WARNING ____ IMMATURE API ____ liable to change */

/* gnome-procbar.h - Gnome Process Bar.

   Copyright (C) 1998 Martin Baulig

   Based on the orignal gtop/procbar.c from Radek Doulik.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License
   as published by the Free Software Foundation; either version 2, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.
*/

#ifndef __GNOME_PROCBAR_H__
#define __GNOME_PROCBAR_H__

#include "libgnome/gnome-defs.h"

BEGIN_GNOME_DECLS

#define GNOME_PROC_BAR(obj)		GTK_CHECK_CAST (obj, gnome_proc_bar_get_type (), GnomeProcBar)
#define GNOME_PROC_BAR__CLASS(klass)	GTK_CHECK_CLASS_CAST (klass, gnome_proc_bar_get_type (), GnomeProcBarClass)
#define GNOME_IS_PROC_BAR(obj)		GTK_CHECK_TYPE (obj, gnome_proc_bar_get_type ())

typedef struct _GnomeProcBar GnomeProcBar;
typedef struct _GnomeProcBarClass GnomeProcBarClass;

struct _GnomeProcBar {

	GtkHBox hbox;

	GtkWidget *bar;
	GtkWidget *label;
	GtkWidget *frame;

	GdkPixmap *bs;
	GdkColor *colors;

	gint colors_allocated;
	gint first_request;
	gint n;
	gint tag;

	unsigned *last;

	gint (*cb)();
};

struct _GnomeProcBarClass {
	GtkHBoxClass parent_class;
};

guint       gnome_proc_bar_get_type        (void);
GtkWidget * gnome_proc_bar_new             (GtkWidget *label,
					    gint n, GdkColor *colors,
					    gint (*cb)());
void        gnome_proc_bar_set_values      (GnomeProcBar *pb, unsigned val []);
void        gnome_proc_bar_start           (GnomeProcBar *pb, gint time, gpointer data);
void        gnome_proc_bar_stop            (GnomeProcBar *pb);
void        gnome_proc_bar_update          (GnomeProcBar *pb, GdkColor *colors);

END_GNOME_DECLS

#endif
