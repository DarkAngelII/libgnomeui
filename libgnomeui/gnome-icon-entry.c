/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 1998, 1999, 2000 Free Software Foundation
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
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
 */

/* GnomeIconEntry widget - A button with the icon which allows graphical
 *			   picking of new icons.  The browse dialog is the
 *			   gnome-icon-sel with a gnome-file-entry which is
 *			   similiar to gnome-pixmap-entry.
 *
 *
 * Author: George Lebl <jirka@5z.com>
 * icon selection based on original dentry-edit code which was:
 *	Written by: Havoc Pennington, based on code by John Ellis.
 */
#include <config.h>
#include <libgnome/gnome-macros.h>

#include <unistd.h> /*getcwd*/
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkdnd.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkfilesel.h>
#include <gtk/gtkframe.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkpixmap.h>
#include <gtk/gtkscrolledwindow.h>

/* Must be before all other gnome includes!! */
#include "gnome-i18nP.h"

#include <libgnome/gnome-util.h>
#include <libgnome/gnome-config.h>
#include <libgnomevfs/gnome-vfs-ops.h>

#include "gnome-file-entry.h"
#include "gnome-icon-list.h"
#include "gnome-icon-sel.h"
#include "gnome-icon-entry.h"
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

struct _GnomeIconEntryPrivate {
	GtkWidget *fentry;
	GtkWidget *icon_sel;

	GtkWidget *pickbutton;

	GtkWidget *pick_dialog;
	gchar *pick_dialog_dir;

	gchar *history_id;
	gchar *browse_dialog_title;
};

static void drag_data_get		(GtkWidget          *widget,
					 GdkDragContext     *context,
					 GtkSelectionData   *selection_data,
					 guint               info,
					 guint               time,
					 GnomeIconEntry     *ientry);
static void drag_data_received		(GtkWidget        *widget,
					 GdkDragContext   *context,
					 gint              x,
					 gint              y,
					 GtkSelectionData *selection_data,
					 guint             info,
					 guint32           time,
					 GnomeIconEntry   *ientry);
static void ientry_destroy		(GtkObject *object);
static void ientry_finalize		(GObject *object);

static void ientry_set_property		(GObject *object, guint param_id,
					 const GValue *value, GParamSpec *pspec);
static void ientry_get_property		(GObject *object, guint param_id,
					 GValue *value, GParamSpec *pspec);

static void ientry_browse               (GnomeIconEntry *ientry);

static GtkTargetEntry drop_types[] = { { "text/uri-list", 0, 0 } };

/* Property IDs */
enum {
	PROP_0,
	PROP_HISTORY_ID,
	PROP_BROWSE_DIALOG_TITLE,
	PROP_PIXMAP_SUBDIR,
	PROP_FILENAME,
	PROP_PICK_DIALOG
};

enum {
	CHANGED_SIGNAL,
	BROWSE_SIGNAL,
	LAST_SIGNAL
};

static gint gnome_ientry_signals[LAST_SIGNAL] = {0};

GNOME_CLASS_BOILERPLATE (GnomeIconEntry, gnome_icon_entry,
			 GtkVBox, GTK_TYPE_VBOX)
static void
gnome_icon_entry_class_init (GnomeIconEntryClass *class)
{
	GtkObjectClass *object_class = (GtkObjectClass *)class;
	GObjectClass *gobject_class = (GObjectClass *)class;

	gnome_ientry_signals[CHANGED_SIGNAL] =
		g_signal_new("changed",
			     G_TYPE_FROM_CLASS (gobject_class),
			     G_SIGNAL_RUN_LAST,
			     G_STRUCT_OFFSET(GnomeIconEntryClass, changed),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__VOID,
			     G_TYPE_NONE, 0);
	gnome_ientry_signals[BROWSE_SIGNAL] =
		g_signal_new("browse",
			     G_TYPE_FROM_CLASS (gobject_class),
			     G_SIGNAL_RUN_LAST,
			     G_STRUCT_OFFSET(GnomeIconEntryClass, browse),
			     NULL, NULL,
			     g_cclosure_marshal_VOID__VOID,
			     G_TYPE_NONE, 0);

	class->changed = NULL;
	class->browse = ientry_browse;

	object_class->destroy = ientry_destroy;

	gobject_class->finalize     = ientry_finalize;
	gobject_class->set_property = ientry_set_property;
	gobject_class->get_property = ientry_get_property;

	g_object_class_install_property (gobject_class,
					 PROP_HISTORY_ID,
					 g_param_spec_string (
						 "history_id",
						 _("History ID"),
						 _("Unique identifier for the icon entry.  "
						   "This will be used to save the history list."),
						 NULL,
						 G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class,
					 PROP_BROWSE_DIALOG_TITLE,
					 g_param_spec_string (
						 "browse_dialog_title",
						 _("Browse Dialog Title"),
						 _("Title for the Browse icon dialog."),
						 NULL,
						 G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class,
					 PROP_PIXMAP_SUBDIR,
					 g_param_spec_string (
						 "pixmap_subdir",
						 _("Pixmap Directory"),
						 _("Directory that will be searched for icons."),
						 NULL,
						 G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class,
					 PROP_FILENAME,
					 g_param_spec_string (
						 "filename",
						 _("Filename"),
						 _("Filename that should be displayed in the "
						   "icon entry."),
						 NULL,
						 G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class,
					 PROP_PICK_DIALOG,
					 g_param_spec_object (
						 "pick_dialog",
						 _("Picker dialog"),
						 _("Icon picker dialog.  You can use this property "
						   "to get the GtkDialog if you need to modify "
						   "or query any of its properties."),
						 GTK_TYPE_DIALOG,
						 G_PARAM_READABLE));

}

/* set_property handler for the icon entry */
static void
ientry_set_property (GObject *object, guint param_id,
		     const GValue *value, GParamSpec *pspec)
{
	GnomeIconEntry *ientry;

	ientry = GNOME_ICON_ENTRY (object);

	switch (param_id) {
	case PROP_HISTORY_ID:
		gnome_icon_entry_set_history_id (ientry, g_value_get_string (value));
		break;

	case PROP_BROWSE_DIALOG_TITLE:
		gnome_icon_entry_set_browse_dialog_title (ientry, g_value_get_string (value));
		break;

	case PROP_PIXMAP_SUBDIR:
		gnome_icon_entry_set_pixmap_subdir (ientry, g_value_get_string (value));
		break;

	case PROP_FILENAME:
		gnome_icon_entry_set_filename (ientry, g_value_get_string (value));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

/* get_property handler for the icon entry */
static void
ientry_get_property (GObject *object, guint param_id,
		     GValue *value, GParamSpec *pspec)
{
	GnomeIconEntry *ientry;
	GnomeIconEntryPrivate *priv;

	ientry = GNOME_ICON_ENTRY (object);
	priv = ientry->_priv;

	switch (param_id) {
	case PROP_HISTORY_ID:
		g_value_set_string (value, priv->history_id);
		break;

	case PROP_BROWSE_DIALOG_TITLE:
		g_value_set_string (value, priv->browse_dialog_title);
		break;

	case PROP_PIXMAP_SUBDIR: {
		char *path;

		g_object_get (priv->fentry, "default_path", &path, NULL);
		g_value_set_string (value, path);
		g_free (path);
		break; }

	case PROP_FILENAME: {
		char *filename;

		filename = gnome_icon_entry_get_filename (ientry);
		g_value_set_string (value, filename);
		g_free (filename);
		break; }

	case PROP_PICK_DIALOG:
		g_value_set_object (value, gnome_icon_entry_pick_dialog (ientry));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}


static void
entry_changed(GtkWidget *widget, GnomeIconEntry *ientry)
{
	gchar *t;
        GdkPixbuf *pixbuf, *scaled;
	GtkWidget *child;
	int w,h;

	g_return_if_fail (ientry != NULL);
	g_return_if_fail (GNOME_IS_ICON_ENTRY (ientry));

	t = gnome_file_entry_get_full_path(GNOME_FILE_ENTRY(ientry->_priv->fentry),
					   FALSE);

	child = GTK_BIN(ientry->_priv->pickbutton)->child;

	if(!t || !g_file_test (t, G_FILE_TEST_IS_SYMLINK | G_FILE_TEST_IS_REGULAR) ||
	   !(pixbuf = gdk_pixbuf_new_from_file (t, NULL))) {
		if(GTK_IS_IMAGE(child)) {
			gtk_drag_source_unset (ientry->_priv->pickbutton);
			gtk_widget_destroy(child);
			child = gtk_label_new(_("No Icon"));
			gtk_widget_show(child);
			gtk_container_add(GTK_CONTAINER(ientry->_priv->pickbutton),
					  child);
		}
		g_free(t);
		return;
	}
	g_free(t);
	w = gdk_pixbuf_get_width(pixbuf);
	h = gdk_pixbuf_get_height(pixbuf);
	if(w>h) {
		if(w>48) {
			h = h*(48.0/w);
			w = 48;
		}
	} else {
		if(h>48) {
			w = w*(48.0/h);
			h = 48;
		}
	}
	scaled = gdk_pixbuf_scale_simple
		(pixbuf, w, h, GDK_INTERP_BILINEAR);
        g_object_unref (G_OBJECT (pixbuf));

	if (GTK_IS_IMAGE (child)) {
                gtk_image_set_from_pixbuf (GTK_IMAGE (child), scaled);
        } else {
		gtk_widget_destroy (child);
                child = gtk_image_new_from_pixbuf (scaled);
		gtk_widget_show(child);
		gtk_container_add(GTK_CONTAINER(ientry->_priv->pickbutton), child);

		if(!GTK_WIDGET_NO_WINDOW(child)) {
			g_signal_connect (child, "drag_data_get",
					  G_CALLBACK (drag_data_get), ientry);
			gtk_drag_source_set (child,
					     GDK_BUTTON1_MASK|GDK_BUTTON3_MASK,
					     drop_types, 1,
					     GDK_ACTION_COPY);
		}
	}
	g_object_unref (G_OBJECT (scaled));
#if 0
	gtk_drag_source_set (ientry->_priv->pickbutton,
			     GDK_BUTTON1_MASK|GDK_BUTTON3_MASK,
			     drop_types, 1,
			     GDK_ACTION_COPY);
#endif
}

static void
entry_activated(GtkWidget *widget, GnomeIconEntry *ientry)
{
	GnomeIconEntryPrivate *priv;
	struct stat buf;
	const gchar *filename;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GTK_IS_ENTRY (widget));
	g_return_if_fail (ientry != NULL);
	g_return_if_fail (GNOME_IS_ICON_ENTRY (ientry));

	priv = ientry->_priv;

	filename = gtk_entry_get_text (GTK_ENTRY (widget));

	if (!filename)
		return;

	stat (filename, &buf);
	if (S_ISDIR (buf.st_mode)) {
		gnome_icon_selection_clear (GNOME_ICON_SELECTION (priv->icon_sel), TRUE);
		gnome_icon_selection_add_directory (GNOME_ICON_SELECTION (priv->icon_sel), filename);
		gnome_icon_selection_show_icons (GNOME_ICON_SELECTION (priv->icon_sel));
	} else {
		/* We pretend like ok has been called */
		entry_changed (NULL, ientry);
		gtk_widget_hide (ientry->_priv->pick_dialog);
	}
}

static void
setup_preview(GtkWidget *widget)
{
	const gchar *p;
	GList *l;
	GtkWidget *pp = NULL;
        GdkPixbuf *pixbuf, *scaled;
	int w,h;
	GtkWidget *frame;
	GtkFileSelection *fs;

	g_return_if_fail (widget != NULL);
	g_return_if_fail (GTK_IS_WIDGET (widget));

	frame = g_object_get_data(G_OBJECT(widget),"frame");
	fs = g_object_get_data(G_OBJECT(frame),"fs");

	if((l = gtk_container_get_children(GTK_CONTAINER(frame))) != NULL) {
		pp = l->data;
		g_list_free(l);
	}

	if(pp)
		gtk_widget_destroy(pp);

	p = gtk_file_selection_get_filename(fs);
	if(!p || !g_file_test (p,G_FILE_TEST_IS_SYMLINK | G_FILE_TEST_IS_REGULAR) ||
	   !(pixbuf = gdk_pixbuf_new_from_file (p, NULL)))
		return;

	w = gdk_pixbuf_get_width(pixbuf);
	h = gdk_pixbuf_get_height(pixbuf);
	if(w>h) {
		if(w>100) {
			h = h*(100.0/w);
			w = 100;
		}
	} else {
		if(h>100) {
			w = w*(100.0/h);
			h = 100;
		}
	}

	scaled = gdk_pixbuf_scale_simple (pixbuf, w, h, GDK_INTERP_BILINEAR);
        g_object_unref (G_OBJECT (pixbuf));
	pp = gtk_image_new_from_pixbuf (scaled);
        g_object_unref (G_OBJECT (scaled));

	gtk_widget_show(pp);
	gtk_container_add(GTK_CONTAINER(frame),pp);
}

static void
ientry_destroy (GtkObject *object)
{
	GnomeIconEntry *ientry;

	/* remember, destroy can be run multiple times! */

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_ICON_ENTRY (object));

	ientry = GNOME_ICON_ENTRY (object);

	if (ientry->_priv->fentry != NULL)
		gtk_widget_unref (ientry->_priv->fentry);
	ientry->_priv->fentry = NULL;

	if (ientry->_priv->pick_dialog != NULL)
		gtk_widget_destroy (ientry->_priv->pick_dialog);
	ientry->_priv->pick_dialog = NULL;

	GNOME_CALL_PARENT (GTK_OBJECT_CLASS, destroy, (object));
}

static void
ientry_finalize (GObject *object)
{
	GnomeIconEntry *ientry;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_ICON_ENTRY (object));

	ientry = GNOME_ICON_ENTRY(object);

	g_free (ientry->_priv->pick_dialog_dir);
	ientry->_priv->pick_dialog_dir = NULL;

	g_free (ientry->_priv->history_id);
	ientry->_priv->history_id = NULL;

	g_free (ientry->_priv->browse_dialog_title);
	ientry->_priv->browse_dialog_title = NULL;

	g_free (ientry->_priv);
	ientry->_priv = NULL;

	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (object));
}

static void
browse_clicked(GnomeFileEntry *fentry, GnomeIconEntry *ientry)
{
	GtkWidget *w;
	GtkWidget *hbox;
	GtkFileSelection *fs;
	GClosure *closure;

	g_return_if_fail (fentry != NULL);
	g_return_if_fail (GNOME_IS_FILE_ENTRY (fentry));
	g_return_if_fail (ientry != NULL);
	g_return_if_fail (GNOME_IS_ICON_ENTRY (ientry));

	if(!fentry->fsw)
		return;
	fs = GTK_FILE_SELECTION(fentry->fsw);

	hbox = fs->file_list;
	do {
		hbox = hbox->parent;
		if(!hbox) {
			g_warning(_("Can't find an hbox, using a normal file "
				    "selection"));
			return;
		}
	} while(!GTK_IS_HBOX(hbox));

	w = gtk_frame_new(_("Preview"));
	gtk_widget_show(w);
	gtk_box_pack_end(GTK_BOX(hbox),w,FALSE,FALSE,0);
	gtk_widget_set_size_request (w,110,110);
	g_object_set_data(G_OBJECT(w),"fs",fs);

	g_object_set_data(G_OBJECT(fs->file_list),"frame",w);
	g_signal_connect (fs->file_list, "select_row",
			  G_CALLBACK (setup_preview), NULL);
	g_object_set_data(G_OBJECT(fs->selection_entry),"frame",w);

	closure = g_cclosure_new (G_CALLBACK (setup_preview), NULL, NULL);
	g_object_watch_closure (G_OBJECT (fs), closure);
	g_signal_connect_closure (fs->selection_entry, "changed",
				  closure, FALSE);
}

static void
icon_selected_cb (GnomeIconEntry * ientry)
{
	GnomeIconEntryPrivate *priv;
	const gchar * icon;

	g_return_if_fail (ientry != NULL);
	g_return_if_fail (GNOME_IS_ICON_ENTRY (ientry));

	priv = ientry->_priv;

	gnome_icon_selection_stop_loading (GNOME_ICON_SELECTION (priv->icon_sel));
	icon = gnome_icon_selection_get_icon (GNOME_ICON_SELECTION (priv->icon_sel), TRUE);

	if (icon != NULL) {
		GtkWidget *e = gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY (ientry->_priv->fentry));
		gtk_entry_set_text (GTK_ENTRY (e), icon);
		entry_changed (NULL, ientry);
	}

	g_signal_emit (ientry, gnome_ientry_signals[CHANGED_SIGNAL], 0);
}

static void
cancel_pressed (GnomeIconEntry *ientry)
{
	GnomeIconEntryPrivate *priv;

	g_return_if_fail (ientry != NULL);
	g_return_if_fail (GNOME_IS_ICON_ENTRY (ientry));

	priv = ientry->_priv;

	gnome_icon_selection_stop_loading (GNOME_ICON_SELECTION (priv->icon_sel));
}


static void
gil_icon_selected_cb(GnomeIconList *gil, gint num, GdkEvent *event, GnomeIconEntry *ientry)
{
	GnomeIconEntryPrivate *priv;
	const gchar *icon;

	g_return_if_fail (ientry != NULL);
	g_return_if_fail (GNOME_IS_ICON_ENTRY (ientry));

	priv = ientry->_priv;

	icon = gnome_icon_selection_get_icon (GNOME_ICON_SELECTION (priv->icon_sel), TRUE);

	if (icon != NULL) {
		gnome_file_entry_set_filename (GNOME_FILE_ENTRY (ientry->_priv->fentry), icon);
	}

	if(event && event->type == GDK_2BUTTON_PRESS && ((GdkEventButton *)event)->button == 1) {
		gnome_icon_selection_stop_loading (GNOME_ICON_SELECTION (priv->icon_sel));
		entry_changed (NULL, ientry);
		gtk_widget_hide(ientry->_priv->pick_dialog);
	}

	g_signal_emit (ientry, gnome_ientry_signals[CHANGED_SIGNAL], 0);
}

static void
dialog_response (GtkWidget *dialog, gint response_id, gpointer data)
{
	GnomeIconEntry *ientry = data;

	switch (response_id) {
	case GTK_RESPONSE_OK:
		icon_selected_cb (ientry);
		gtk_widget_hide (dialog);
		break;
	default:
		/* This catches cancel and delete event */
		cancel_pressed (ientry);
		gtk_widget_hide (dialog);
		break;
	}
}

static void
ientry_browse(GnomeIconEntry *ientry)
{
	GnomeIconEntryPrivate *priv;
	GnomeFileEntry *fe;
	gchar *p;
	gchar *curfile;
	GtkWidget *tl;

	g_return_if_fail (ientry != NULL);
	g_return_if_fail (GNOME_IS_ICON_ENTRY (ientry));

	priv = ientry->_priv;

	fe = GNOME_FILE_ENTRY(priv->fentry);
	p = gnome_file_entry_get_full_path(fe,FALSE);
	curfile = gnome_icon_entry_get_filename(ientry);

	/* Are we part of a modal window?  If so, we need to be modal too. */
	tl = gtk_widget_get_toplevel (GTK_WIDGET (priv->pickbutton));

	if(!p) {
		if(fe->default_path) {
			p = g_strdup(fe->default_path);
		} else {
			/*get around the g_free/free issue*/
			gchar *cwd = g_get_current_dir ();
			p = g_strdup(cwd);
			g_free(cwd);
		}
		gtk_entry_set_text (GTK_ENTRY (gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY (priv->fentry))),
				    p);
	}

	/*figure out the directory*/
	if(!g_file_test (p,G_FILE_TEST_IS_DIR)) {
		gchar *d;
		d = g_path_get_dirname (p);
		g_free (p);
		p = d;
		if(!g_file_test (p,G_FILE_TEST_IS_DIR)) {
			g_free (p);
			if(fe->default_path) {
				p = g_strdup(fe->default_path);
			} else {
				/*get around the g_free/free issue*/
				gchar *cwd = g_get_current_dir ();
				p = g_strdup(cwd);
				free(cwd);
			}
			gtk_entry_set_text (GTK_ENTRY (gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY (priv->fentry))),
				    p);
			g_return_if_fail(g_file_test (p,G_FILE_TEST_IS_DIR));
		}
	}


	if(priv->pick_dialog==NULL ||
	   priv->pick_dialog_dir==NULL ||
	   strcmp(p,priv->pick_dialog_dir)!=0) {
		GtkWidget *gil;

		if(priv->pick_dialog) {
			gtk_container_remove (GTK_CONTAINER (priv->fentry->parent), priv->fentry);
			gtk_widget_destroy(priv->pick_dialog);
		}

		g_free(priv->pick_dialog_dir);
		priv->pick_dialog_dir = p;
		priv->pick_dialog =
			gtk_dialog_new_with_buttons (priv->browse_dialog_title,
						     GTK_WINDOW (tl),
						     (GTK_WINDOW (tl)->modal ? GTK_DIALOG_MODAL : 0),
						     GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						     GTK_STOCK_OK, GTK_RESPONSE_OK,
						     NULL);
		g_signal_connect (priv->pick_dialog, "destroy",
				  G_CALLBACK (gtk_widget_destroyed),
				  &priv->pick_dialog);

		priv->icon_sel = gnome_icon_selection_new ();

		gnome_icon_selection_add_directory (GNOME_ICON_SELECTION (priv->icon_sel),
						    priv->pick_dialog_dir);

		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(priv->pick_dialog)->vbox),
				   priv->fentry, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(GTK_DIALOG(priv->pick_dialog)->vbox),
				   priv->icon_sel, TRUE, TRUE, 0);

		gtk_widget_show_all(priv->pick_dialog);

		gnome_icon_selection_show_icons(GNOME_ICON_SELECTION(priv->icon_sel));

		if(curfile != NULL) {
			char *base = g_path_get_basename(curfile);
			gnome_icon_selection_select_icon(GNOME_ICON_SELECTION(priv->icon_sel),
							 base);
			g_free(base);
		}

		g_signal_connect (priv->pick_dialog, "response",
				  G_CALLBACK (dialog_response),
				  ientry);

		gil = gnome_icon_selection_get_gil(GNOME_ICON_SELECTION(priv->icon_sel));
		g_signal_connect_after (gil, "select_icon",
					G_CALLBACK (gil_icon_selected_cb),
					ientry);
	} else {
		if (!GTK_WIDGET_VISIBLE(priv->pick_dialog))
			gtk_widget_show(priv->pick_dialog);

		if (priv->icon_sel)
			gnome_icon_selection_show_icons (GNOME_ICON_SELECTION (priv->icon_sel));
	}
}

static void
show_icon_selection(GtkButton * b, GnomeIconEntry * ientry)
{
	g_return_if_fail (ientry != NULL);
	g_return_if_fail (GNOME_IS_ICON_ENTRY (ientry));

	g_signal_emit (ientry, gnome_ientry_signals[BROWSE_SIGNAL], 0);
}

static void
drag_data_received (GtkWidget        *widget,
		    GdkDragContext   *context,
		    gint              x,
		    gint              y,
		    GtkSelectionData *selection_data,
		    guint             info,
		    guint32           time,
		    GnomeIconEntry   *ientry)
{
	GList *uris, *li;
	GnomeVFSURI *uri = NULL;
	GnomeVFSFileInfo *file_info;

	/*here we extract the filenames from the URI-list we recieved*/
	uris = gnome_vfs_uri_list_parse (selection_data->data);

	for (li = uris; li != NULL; li = li->next) {
		const char *mimetype;
		const char *path;

		uri = li->data;

		/* FIXME: Support non-local files */
		if ( ! gnome_vfs_uri_is_local (uri)) {
			uri = NULL;
			continue;
		}

		path = gnome_vfs_uri_get_path (uri);
		file_info = gnome_vfs_file_info_new ();
		/* FIXME: OK to do synchronous I/O on a possibly-remote URI here? */
		gnome_vfs_get_file_info_uri (uri, file_info,
					     GNOME_VFS_FILE_INFO_GET_MIME_TYPE |
					     GNOME_VFS_FILE_INFO_FOLLOW_LINKS);
		mimetype = gnome_vfs_file_info_get_mime_type (file_info);

		if (mimetype != NULL &&
		    strcmp (mimetype, "application/x-gnome-app-info") == 0) {
			char *confpath;
			char *icon;

			/* Try to read the .desktop's Icon entry */
			confpath = g_strdup_printf ("=%s=/Desktop Entry/Icon",
						    path);
			icon = gnome_config_get_string (confpath);
			if (icon == NULL || *icon == '\0') {
				gnome_config_drop_file (confpath);
				g_free (confpath);
				g_free (icon);
				confpath = g_strdup_printf ("=%s=/KDE Desktop Entry/Icon",
							    path);
				icon = gnome_config_get_string (confpath);
				gnome_config_drop_file (confpath);
				g_free (confpath);
				if (icon == NULL || *icon == '\0') {
					g_free (icon);
					icon = NULL;
				}
			}

			if (icon == NULL) {
				uri = NULL;
				continue;
			}

			if (icon[0] == G_DIR_SEPARATOR) {
				if(gnome_icon_entry_set_filename(ientry, icon)) {
					g_free (icon);
					break;
				}
				g_free (icon);
			} else {
				char *full;
				full = gnome_program_locate_file (NULL /* program */,
								  GNOME_FILE_DOMAIN_PIXMAP,
								  icon,
								  TRUE /* only_if_exists */,
								  NULL /* ret_locations */);
				if (full == NULL) {
					full = gnome_program_locate_file (NULL /* program */,
									  GNOME_FILE_DOMAIN_APP_PIXMAP,
									  icon,
									  TRUE /* only_if_exists */,
									  NULL /* ret_locations */);
				}
				/* FIXME: try the KDE path if we know it */
				g_free (icon);

				if (full != NULL &&
				    gnome_icon_entry_set_filename (ientry, full)) {
					g_free (full);
					break;
				}
				g_free (full);
			}
		} else if (gnome_icon_entry_set_filename (ientry, path)) {
			gnome_vfs_file_info_unref (file_info);
			break;
		}
		uri = NULL;
		gnome_vfs_file_info_unref (file_info);
	}


	/*if there's isn't a file*/
	if (uri == NULL) {
		gtk_drag_finish (context, FALSE, FALSE, time);
		/*free the list of files we got*/
		gnome_vfs_uri_list_free (uris);
		return;
	}

	/*free the list of files we got*/
	gnome_vfs_uri_list_free (uris);
}

static void
drag_data_get  (GtkWidget          *widget,
		GdkDragContext     *context,
		GtkSelectionData   *selection_data,
		guint               info,
		guint               time,
		GnomeIconEntry     *ientry)
{
	gchar *string;
	gchar *file;

	g_return_if_fail (ientry != NULL);
	g_return_if_fail (GNOME_IS_ICON_ENTRY (ientry));

	file = gnome_file_entry_get_full_path(GNOME_FILE_ENTRY(ientry->_priv->fentry),
					      TRUE);

	if(!file) {
		/*FIXME: cancel the drag*/
		return;
	}

	string = g_strdup_printf("file:%s\r\n",file);
	g_free(file);
	gtk_selection_data_set (selection_data,
				selection_data->target,
				8, string, strlen(string)+1);
	g_free(string);
}


static void
gnome_icon_entry_instance_init (GnomeIconEntry *ientry)
{
	GtkWidget *w;
	gchar *p;
	GClosure *closure;

	ientry->_priv = g_new0(GnomeIconEntryPrivate, 1);

	gtk_box_set_spacing (GTK_BOX (ientry), 4);

	ientry->_priv->icon_sel = NULL;
	ientry->_priv->pick_dialog = NULL;
	ientry->_priv->pick_dialog_dir = NULL;

	w = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
	gtk_widget_show(w);
	gtk_box_pack_start (GTK_BOX (ientry), w, TRUE, TRUE, 0);
	ientry->_priv->pickbutton = gtk_button_new_with_label(_("No Icon"));
	gtk_drag_dest_set (GTK_WIDGET (ientry->_priv->pickbutton),
			   GTK_DEST_DEFAULT_MOTION |
			   GTK_DEST_DEFAULT_HIGHLIGHT |
			   GTK_DEST_DEFAULT_DROP,
			   drop_types, 1, GDK_ACTION_COPY);
	g_signal_connect (ientry->_priv->pickbutton, "drag_data_received",
			  G_CALLBACK (drag_data_received),
			  ientry);
	g_signal_connect (ientry->_priv->pickbutton, "drag_data_get",
			  G_CALLBACK (drag_data_get),
			  ientry);

	g_signal_connect (ientry->_priv->pickbutton, "clicked",
			  G_CALLBACK (show_icon_selection),
			  ientry);
	/*FIXME: 60x60 is just larger then default 48x48, though icon sizes
	  are supposed to be selectable I guess*/
	gtk_widget_set_size_request (ientry->_priv->pickbutton, 60, 60);
	gtk_container_add (GTK_CONTAINER (w), ientry->_priv->pickbutton);
	gtk_widget_show (ientry->_priv->pickbutton);

	ientry->_priv->fentry = gnome_file_entry_new (NULL, _("Browse"));
	/*BORPORP */
	gnome_file_entry_set_modal (GNOME_FILE_ENTRY (ientry->_priv->fentry),
				    TRUE);
	gtk_widget_ref (ientry->_priv->fentry);
	g_signal_connect_after (ientry->_priv->fentry, "browse_clicked",
				G_CALLBACK (browse_clicked),
				ientry);

	gtk_widget_show (ientry->_priv->fentry);

	p = gnome_program_locate_file (NULL /* program */,
				       GNOME_FILE_DOMAIN_PIXMAP,
				       ".",
				       FALSE /* only_if_exists */,
				       NULL /* ret_locations */);
	gnome_file_entry_set_default_path(GNOME_FILE_ENTRY(ientry->_priv->fentry),p);
	g_free(p);

	w = gnome_file_entry_gtk_entry(GNOME_FILE_ENTRY(ientry->_priv->fentry));

	closure = g_cclosure_new (G_CALLBACK (entry_activated), ientry, NULL);
	g_object_watch_closure (G_OBJECT (ientry), closure);
	g_signal_connect_closure (w, "activate",
				  closure, FALSE);

	/*just in case there is a default that is an image*/
	entry_changed(w,ientry);
}

/**
 * gnome_icon_entry_construct:
 * @ientry: the GnomeIconEntry to work with
 * @history_id: the id given to #gnome_entry_new in the browse dialog
 * @browse_dialog_title: title of the icon selection dialog
 *
 * Description: For language bindings and subclassing, from C use
 * #gnome_icon_entry_new
 *
 * Returns:
 **/
void
gnome_icon_entry_construct (GnomeIconEntry *ientry,
			    const gchar *history_id,
			    const gchar *browse_dialog_title)
{
	g_return_if_fail (ientry != NULL);
	g_return_if_fail (GNOME_IS_ICON_ENTRY (ientry));

	gnome_icon_entry_set_history_id(ientry, history_id);
	gnome_icon_entry_set_browse_dialog_title(ientry, browse_dialog_title);
}

/**
 * gnome_icon_entry_new:
 * @history_id: the id given to #gnome_entry_new
 * @browse_dialog_title: title of the browse dialog and icon selection dialog
 *
 * Description: Creates a new icon entry widget
 *
 * Returns: Returns the new object
 **/
GtkWidget *
gnome_icon_entry_new (const gchar *history_id, const gchar *browse_dialog_title)
{
	GnomeIconEntry *ientry;

	ientry = g_object_new (GNOME_TYPE_ICON_ENTRY, NULL);

	gnome_icon_entry_construct (ientry, history_id, browse_dialog_title);

	return GTK_WIDGET (ientry);
}


/**
 * gnome_icon_entry_set_pixmap_subdir:
 * @ientry: the GnomeIconEntry to work with
 * @subdir: subdirectory
 *
 * Description: Sets the subdirectory below gnome's default
 * pixmap directory to use as the default path for the file
 * entry.  The path can also be an absolute one.  If %NULL is passed
 * then the pixmap directory itself is used.
 *
 * Returns:
 **/
void
gnome_icon_entry_set_pixmap_subdir(GnomeIconEntry *ientry,
				   const gchar *subdir)
{
	g_return_if_fail (ientry != NULL);
	g_return_if_fail (GNOME_IS_ICON_ENTRY (ientry));

	if(!subdir)
		subdir = ".";

	if(g_path_is_absolute(subdir)) {
		gnome_file_entry_set_default_path(GNOME_FILE_ENTRY(ientry->_priv->fentry), subdir);
	} else {
		char *p = gnome_program_locate_file (NULL /* program */,
						     GNOME_FILE_DOMAIN_PIXMAP,
						     subdir,
						     FALSE /* only_if_exists */,
						     NULL /* ret_locations */);
		gnome_file_entry_set_default_path(GNOME_FILE_ENTRY(ientry->_priv->fentry), p);
		g_free(p);
	}
}

/**
 * gnome_icon_entry_set_filename:
 * @ientry: the GnomeIconEntry to work with
 * @filename: a filename
 *
 * Description: Sets the icon of GnomeIconEntry to be the one pointed to by
 * @filename (in the current subdirectory).
 *
 * Returns: %TRUE if icon was loaded ok, %FALSE otherwise
 **/
gboolean
gnome_icon_entry_set_filename(GnomeIconEntry *ientry,
			      const gchar *filename)
{
	GtkWidget *child;
	GtkWidget *e;

	g_return_val_if_fail (ientry != NULL, FALSE);
	g_return_val_if_fail (GNOME_IS_ICON_ENTRY (ientry), FALSE);

	if(!filename)
		filename = "";

	e = gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY (ientry->_priv->fentry));
	gtk_entry_set_text (GTK_ENTRY (e), filename);
	entry_changed (NULL, ientry);
	g_signal_emit (ientry, gnome_ientry_signals[CHANGED_SIGNAL], 0);

	child = GTK_BIN(ientry->_priv->pickbutton)->child;
	/* this happens if it doesn't exist or isn't an image */
	if ( ! GTK_IS_IMAGE (child))
		return FALSE;

	return TRUE;
}

/**
 * gnome_icon_entry_get_filename:
 * @ientry: the GnomeIconEntry to work with
 *
 * Description: Gets the file name of the image if it was possible
 * to load it into the preview. That is, it will only return a filename
 * if the image exists and it was possible to load it as an image.
 *
 * Returns: a newly allocated string with the path or %NULL if it
 * couldn't load the file
 **/
gchar *
gnome_icon_entry_get_filename(GnomeIconEntry *ientry)
{
	GtkWidget *child;

	g_return_val_if_fail (ientry != NULL,NULL);
	g_return_val_if_fail (GNOME_IS_ICON_ENTRY (ientry),NULL);

	child = GTK_BIN(ientry->_priv->pickbutton)->child;

	/* this happens if it doesn't exist or isn't an image */
	if ( ! GTK_IS_IMAGE (child))
		return NULL;

	return gnome_file_entry_get_full_path(GNOME_FILE_ENTRY(ientry->_priv->fentry),
					      TRUE);
}

/**
 * gnome_icon_entry_pick_dialog:
 * @ientry: the GnomeIconEntry to work with
 *
 * Description: If a pick dialog exists, return a pointer to it or
 * return NULL.  This is if you need to do something with all dialogs.
 * You would use the browse signal with connect_after to get the
 * pick dialog when it is displayed.
 *
 * Returns: The pick dialog or %NULL if none exists
 **/
GtkWidget *
gnome_icon_entry_pick_dialog(GnomeIconEntry *ientry)
{
	g_return_val_if_fail (ientry != NULL,NULL);
	g_return_val_if_fail (GNOME_IS_ICON_ENTRY (ientry),NULL);

	return ientry->_priv->pick_dialog;
}

/**
 * gnome_icon_entry_set_browse_dialog_title:
 * @ientry: the GnomeIconEntry to work with
 * @browse_dialog_title: title of the icon selection dialog
 *
 * Description:  Set the title of the browse dialog.  It will not effect
 * an existing dialog.
 *
 * Returns:
 **/
void
gnome_icon_entry_set_browse_dialog_title(GnomeIconEntry *ientry,
					 const gchar *browse_dialog_title)
{
	g_return_if_fail (ientry != NULL);
	g_return_if_fail (GNOME_IS_ICON_ENTRY (ientry));

	g_free(ientry->_priv->browse_dialog_title);
	ientry->_priv->browse_dialog_title = g_strdup(browse_dialog_title);
}

/**
 * gnome_icon_entry_set_history_id:
 * @ientry: the GnomeIconEntry to work with
 * @history_id: the id given to #gnome_entry_new in the browse dialog
 *
 * Description:  Set the history_id of the entry in the browse dialog
 * and reload the history
 *
 * Returns:
 **/
void
gnome_icon_entry_set_history_id(GnomeIconEntry *ientry,
				const gchar *history_id)
{
	GtkWidget *gentry;

	g_free(ientry->_priv->history_id);
	ientry->_priv->history_id = g_strdup(history_id);

	gentry = gnome_file_entry_gnome_entry(GNOME_FILE_ENTRY(ientry->_priv->fentry));
	g_object_set (G_OBJECT (gentry),
		      "history_id", history_id,
		      NULL);
}

/* DEPRECATED routines left for compatibility only, will disapear in
 * some very distant future */

/**
 * gnome_icon_entry_set_icon:
 * @ientry: the GnomeIconEntry to work with
 * @filename: a filename
 *
 * Description: Deprecated in favour of #gnome_icon_entry_set_filename
 *
 * Returns:
 **/
void
gnome_icon_entry_set_icon(GnomeIconEntry *ientry,
			  const gchar *filename)
{
	g_warning("gnome_icon_entry_set_icon deprecated, "
		  "use gnome_icon_entry_set_filename!");
	gnome_icon_entry_set_filename(ientry, filename);
}

/**
 * gnome_icon_entry_gnome_file_entry:
 * @ientry: the GnomeIconEntry to work with
 *
 * Description: Get the GnomeFileEntry widget that's part of the entry
 * DEPRECATED! Use the "changed" signal for getting changes
 *
 * Returns: Returns GnomeFileEntry widget
 **/
GtkWidget *
gnome_icon_entry_gnome_file_entry (GnomeIconEntry *ientry)
{
	g_return_val_if_fail (ientry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_ICON_ENTRY (ientry), NULL);

	g_warning("gnome_icon_entry_gnome_file_entry deprecated, "
		  "use changed signal!");
	return ientry->_priv->fentry;
}

/**
 * gnome_icon_entry_gnome_entry:
 * @ientry: the GnomeIconEntry to work with
 *
 * Description: Get the GnomeEntry widget that's part of the entry
 * DEPRECATED! Use the "changed" signal for getting changes
 *
 * Returns: Returns GnomeEntry widget
 **/
GtkWidget *
gnome_icon_entry_gnome_entry (GnomeIconEntry *ientry)
{
	g_return_val_if_fail (ientry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_ICON_ENTRY (ientry), NULL);

	g_warning("gnome_icon_entry_gnome_entry deprecated, "
		  "use changed signal!");

	return gnome_file_entry_gnome_entry(GNOME_FILE_ENTRY(ientry->_priv->fentry));
}

/**
 * gnome_icon_entry_gtk_entry:
 * @ientry: the GnomeIconEntry to work with
 *
 * Description: Get the GtkEntry widget that's part of the entry.
 * DEPRECATED! Use the "changed" signal for getting changes
 *
 * Returns: Returns GtkEntry widget
 **/
GtkWidget *
gnome_icon_entry_gtk_entry (GnomeIconEntry *ientry)
{
	g_return_val_if_fail (ientry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_ICON_ENTRY (ientry), NULL);

	g_warning("gnome_icon_entry_gtk_entry deprecated, "
		  "use changed signal!");

	return gnome_file_entry_gtk_entry (GNOME_FILE_ENTRY (ientry->_priv->fentry));
}
