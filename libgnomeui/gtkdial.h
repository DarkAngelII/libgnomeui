/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Cambridge, MA 02139, USA.
 */
#ifndef __GTK_DIAL_H__
#define __GTK_DIAL_H__


#include <gdk/gdk.h>
#include <gtk/gtkadjustment.h>
#include <gtk/gtkwidget.h>
#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS

#define GTK_DIAL(obj)          GTK_CHECK_CAST (obj, gtk_dial_get_type (), GtkDial)
#define GTK_DIAL_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, gtk_dial_get_type (), GtkDialClass)
#define GTK_IS_DIAL(obj)       GTK_CHECK_TYPE (obj, gtk_dial_get_type ())


typedef struct _GtkDial        GtkDial;
typedef struct _GtkDialClass   GtkDialClass;

struct _GtkDial
{
  GtkWidget     widget;
  GdkPixmap     *offscreen_pixmap;
  GtkAdjustment *adjustment;

  gfloat angle;
  gfloat percentage;
  gfloat old_value;
  gfloat old_lower;
  gfloat old_upper;

  /* Dimensions of dial components */
  gint radius;
  gint pointer_width;

  guint32 timer;

  guint8 button;

  guint policy : 2;
  guint view_only : 1;
};

struct _GtkDialClass
{
  GtkWidgetClass parent_class;
};


GtkWidget*     gtk_dial_new                    (GtkAdjustment  *adjustment);
guint          gtk_dial_get_type               (void);
GtkAdjustment* gtk_dial_get_adjustment         (GtkDial        *dial);
void           gtk_dial_set_update_policy      (GtkDial        *dial,
						GtkUpdateType  policy);

void           gtk_dial_set_adjustment         (GtkDial        *dial,
						GtkAdjustment  *adjustment);
gfloat         gtk_dial_set_percentage         (GtkDial        *dial,
						gfloat         percent);
gfloat         gtk_dial_get_percentage         (GtkDial        *dial);
gfloat         gtk_dial_set_value              (GtkDial        *dial,
						gfloat         value);
gfloat         gtk_dial_get_value              (GtkDial        *dial);
void           gtk_dial_set_view_only          (GtkDial        *dial,
						gboolean       view_only);

END_GNOME_DECLS

#endif /* __GTK_DIAL_H__ */
