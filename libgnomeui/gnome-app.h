/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 1997, 1998, 1999, 2000 Red Hat Software, The Free
 * Software Foundation, Miguel de Icaza, Federico Menu, Chris Toshok.
 * All rights reserved.
 *
 * This file is part of the Gnome Library.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.  */
/*
  @NOTATION@
 */

/*
 *
 * Originally by Elliot Lee,
 *
 * improvements and rearrangement by Miguel,
 * and I don't know what you other people did :)
 *
 * Even more changes by Federico Mena.
 */

#ifndef GNOME_APP_H
#define GNOME_APP_H

#include <gtk/gtkmenubar.h>
#include <gtk/gtktoolbar.h>
#include <gtk/gtkwindow.h>

#include <bonobo/bonobo-dock.h>

G_BEGIN_DECLS

#define GNOME_APP_MENUBAR_NAME "Menubar"
#define GNOME_APP_TOOLBAR_NAME "Toolbar"


#define GNOME_TYPE_APP            (gnome_app_get_type ())
#define GNOME_APP(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_APP, GnomeApp))
#define GNOME_APP_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_APP, GnomeAppClass))
#define GNOME_IS_APP(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_APP))
#define GNOME_IS_APP_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_APP))
#define GNOME_APP_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), GNOME_TYPE_APP, GnomeAppClass))


typedef struct _GnomeApp        GnomeApp;
typedef struct _GnomeAppPrivate GnomeAppPrivate;
typedef struct _GnomeAppClass   GnomeAppClass;

struct _GnomeApp {
	GtkWindow parent_object;

	/* Application name. */
	gchar *name;

	/* Prefix for gnome-config (used to save the layout).  */
	gchar *prefix;

        /* The dock.  */
        GtkWidget *dock;

	/* The status bar.  */
        GtkWidget *statusbar;

	/* The vbox widget that ties them.  */
	GtkWidget *vbox;

	/* The menubar.  This is a pointer to a widget contained into
           the dock.  */
	GtkWidget *menubar;

	/* The contents.  This is a pointer to dock->client_area.  */
	GtkWidget *contents;

	/* Dock layout.  */
	BonoboDockLayout *layout;

	/* Main accelerator group for this window (hotkeys live here).  */
	GtkAccelGroup *accel_group;

	/* If TRUE, the application uses gnome-config to retrieve and
           save the docking configuration automagically.  */
	gboolean enable_layout_config : 1;

	/*< private >*/
	GnomeAppPrivate *_priv;
};

struct _GnomeAppClass {
	GtkWindowClass parent_class;

	/* Padding for possible expansion */
	gpointer padding1;
	gpointer padding2;
};


/* Standard Gtk function */
GType gnome_app_get_type (void) G_GNUC_CONST;

/* Create a new (empty) application window.  You must specify the application's name (used
 * internally as an identifier).  The window title can be left as NULL, in which case the window's
 * title will not be set.
 */
GtkWidget *gnome_app_new (const gchar *appname, const gchar *title);

/* Constructor for language bindings; you don't normally need this. */
void gnome_app_construct (GnomeApp *app, const gchar *appname, const gchar *title);

/* Sets the menu bar of the application window */
void gnome_app_set_menus (GnomeApp *app, GtkMenuBar *menubar);

/* Sets the main toolbar of the application window */
void gnome_app_set_toolbar (GnomeApp *app, GtkToolbar *toolbar);

/* Sets the status bar of the application window */
void gnome_app_set_statusbar (GnomeApp *app, GtkWidget *statusbar);

/* Sets the status bar of the application window, but uses the given
 * container widget rather than creating a new one. */
void gnome_app_set_statusbar_custom (GnomeApp *app,
				     GtkWidget *container,
				     GtkWidget *statusbar);

/* Sets the content area of the application window */
void gnome_app_set_contents (GnomeApp *app, GtkWidget *contents);

void gnome_app_add_toolbar (GnomeApp *app,
			    GtkToolbar *toolbar,
			    const gchar *name,
			    BonoboDockItemBehavior behavior,
			    BonoboDockPlacement placement,
			    gint band_num,
			    gint band_position,
			    gint offset);

GtkWidget *gnome_app_add_docked (GnomeApp *app,
				 GtkWidget *widget,
				 const gchar *name,
				 BonoboDockItemBehavior behavior,
				 BonoboDockPlacement placement,
				 gint band_num,
				 gint band_position,
				 gint offset);

void gnome_app_add_dock_item (GnomeApp *app,
			      BonoboDockItem *item,
			      BonoboDockPlacement placement,
			      gint band_num,
			      gint band_position,
			      gint offset);

void gnome_app_enable_layout_config (GnomeApp *app, gboolean enable);

BonoboDock *gnome_app_get_dock (GnomeApp *app);

BonoboDockItem *gnome_app_get_dock_item_by_name (GnomeApp *app,
						const gchar *name);

G_END_DECLS

#endif /* GNOME_APP_H */
