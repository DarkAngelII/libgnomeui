/*
 * Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation
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

/* GnomeEntry widget - combo box with auto-saved history
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#include <config.h>
#include "gnome-macros.h"

/* Must be before all other gnome includes!! */
#include "gnome-i18nP.h"

#include <stdio.h>
#include <string.h>
#include <gtk/gtkentry.h>
#include <gtk/gtklist.h>
#include <gtk/gtklistitem.h>
#include <gtk/gtksignal.h>

#include <gconf/gconf-client.h>
#include <libgnome/gnome-program.h>
#include <libgnome/gnome-gconf.h>

#include "gnome-entry.h"

enum {
	PROP_0,
	PROP_HISTORY_ID,
	PROP_GTK_ENTRY
};

#define DEFAULT_MAX_HISTORY_SAVED 10  /* This seems to make more sense then 60*/

struct _GnomeEntryPrivate {
	gchar     *history_id;

	GList     *items;

	guint16    max_saved;
	guint32    changed : 1;

	guint      gconf_notify_id;
};


struct item {
	gboolean save;
	gchar *text;
};


static void gnome_entry_class_init (GnomeEntryClass *class);
static void gnome_entry_init       (GnomeEntry      *gentry);
static void gnome_entry_finalize   (GObject         *object);

static void gnome_entry_get_property (GObject        *object,
				      guint           param_id,
				      GValue         *value,
				      GParamSpec     *pspec);
static void gnome_entry_set_property (GObject        *object,
				      guint           param_id,
				      const GValue   *value,
				      GParamSpec     *pspec);
static void gnome_entry_editable_init (GtkEditableClass *iface);
static void gnome_entry_load_history (GnomeEntry *gentry);
static void gnome_entry_save_history (GnomeEntry *gentry);

static char *build_gconf_key (GnomeEntry *gentry);

/* Note, can't use boilerplate with interfaces yet,
 * should get sorted out */
static GtkComboClass *parent_class = NULL;
GType
gnome_entry_get_type (void)
{
	static GType object_type = 0;

	if (object_type == 0) {
		GtkType type_of_parent;
		static const GtkTypeInfo object_info = {
			"GnomeEntry",
			sizeof (GnomeEntry),
			sizeof (GnomeEntryClass),
			(GtkClassInitFunc) gnome_entry_class_init,
			(GtkObjectInitFunc) gnome_entry_init,
			/* reserved_1 */ NULL,
			/* reserved_2 */ NULL,
			(GtkClassInitFunc) NULL
		};
		static const GInterfaceInfo editable_info =
		{
			(GInterfaceInitFunc) gnome_entry_editable_init,	 /* interface_init */
			NULL,			                         /* interface_finalize */
			NULL			                         /* interface_data */
		};
		type_of_parent = GTK_TYPE_COMBO;
		object_type = gtk_type_unique (type_of_parent, &object_info);
		parent_class = gtk_type_class (type_of_parent);

		g_type_add_interface_static (object_type,
					     GTK_TYPE_EDITABLE,
					     &editable_info);
	}
	return object_type;
}

enum {
	CHANGED_SIGNAL,
	ACTIVATE_SIGNAL,
	LAST_SIGNAL
};
static int gnome_entry_signals[LAST_SIGNAL] = {0};

static void
gnome_entry_class_init (GnomeEntryClass *class)
{
	GtkObjectClass *object_class;
	GObjectClass *gobject_class;

	object_class = (GtkObjectClass *) class;
	gobject_class = (GObjectClass *) class;

	gnome_entry_signals[CHANGED_SIGNAL] =
		gtk_signal_new("changed",
			       GTK_RUN_LAST,
			       GTK_CLASS_TYPE (object_class),
			       GTK_SIGNAL_OFFSET(GnomeEntryClass,
			       			 changed),
			       gtk_signal_default_marshaller,
			       GTK_TYPE_NONE,
			       0);
	gnome_entry_signals[ACTIVATE_SIGNAL] =
		gtk_signal_new("activate",
			       GTK_RUN_LAST,
			       GTK_CLASS_TYPE (object_class),
			       GTK_SIGNAL_OFFSET(GnomeEntryClass,
			       			 activate),
			       gtk_signal_default_marshaller,
			       GTK_TYPE_NONE,
			       0);

	class->changed = NULL;
	class->activate = NULL;

	gobject_class->finalize = gnome_entry_finalize;
	gobject_class->set_property = gnome_entry_set_property;
	gobject_class->get_property = gnome_entry_get_property;

	g_object_class_install_property (gobject_class,
					 PROP_HISTORY_ID,
					 g_param_spec_string ("history_id",
							      _("History id"),
							      _("History id"),
							      NULL,
							      G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class,
					 PROP_GTK_ENTRY,
					 g_param_spec_object ("gtk_entry",
							      _("GTK entry"),
							      _("The GTK entry"),
							      GTK_TYPE_ENTRY,
							      G_PARAM_READABLE));
}

static void
gnome_entry_get_property (GObject        *object,
			  guint           param_id,
			  GValue         *value,
			  GParamSpec     *pspec)
{
	GnomeEntry *entry = GNOME_ENTRY (object);

	switch (param_id) {
	case PROP_HISTORY_ID:
		g_value_set_string (value, entry->_priv->history_id);
		break;
	case PROP_GTK_ENTRY:
		g_value_set_object (value, gnome_entry_gtk_entry (entry));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
	
}

static void
gnome_entry_set_property (GObject        *object,
			  guint           param_id,
			  const GValue   *value,
			  GParamSpec     *pspec)
{
	GnomeEntry *entry = GNOME_ENTRY (object);

	switch (param_id) {
	case PROP_HISTORY_ID:
		gnome_entry_set_history_id (entry, g_value_get_string (value));
		gnome_entry_load_history (entry);
		break;
		
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
	
}

static void
entry_changed (GtkWidget *widget, gpointer data)
{
	GnomeEntry *gentry;

	gentry = data;
	gentry->_priv->changed = TRUE;

	gtk_signal_emit (GTK_OBJECT (gentry), gnome_entry_signals[CHANGED_SIGNAL]);
}

static void
entry_activated (GtkWidget *widget, gpointer data)
{
	GnomeEntry *gentry;
	const gchar *text;

	gentry = data;

	text = gtk_entry_get_text (GTK_ENTRY (widget));

	if (!gentry->_priv->changed || (strcmp (text, "") == 0)) {
		gentry->_priv->changed = FALSE;
	} else {
		gnome_entry_prepend_history (gentry, TRUE, gtk_entry_get_text (GTK_ENTRY (widget)));
	}

	gtk_signal_emit (GTK_OBJECT (gentry), gnome_entry_signals[ACTIVATE_SIGNAL]);
	gnome_entry_save_history (gentry);
}

static void
gnome_entry_init (GnomeEntry *gentry)
{
	gentry->_priv = g_new0(GnomeEntryPrivate, 1);

	gentry->_priv->changed      = FALSE;
	gentry->_priv->history_id   = NULL;
	gentry->_priv->items        = NULL;
	gentry->_priv->max_saved    = DEFAULT_MAX_HISTORY_SAVED;

	gtk_signal_connect (GTK_OBJECT (gnome_entry_gtk_entry (gentry)), "changed",
			    (GtkSignalFunc) entry_changed,
			    gentry);
	gtk_signal_connect (GTK_OBJECT (gnome_entry_gtk_entry (gentry)), "activate",
			    (GtkSignalFunc) entry_activated,
			    gentry);
	gtk_combo_disable_activate (GTK_COMBO (gentry));
        gtk_combo_set_case_sensitive (GTK_COMBO (gentry), TRUE);
}

/**
 * gnome_entry_new
 * @history_id: If not %NULL, the text id under which history data is stored
 *
 * Description: Creates a new GnomeEntry widget.  If  @history_id is
 * not %NULL, then the history list will be saved and restored between
 * uses under the given id.
 *
 * Returns: Newly-created GnomeEntry widget.
 */
GtkWidget *
gnome_entry_new (const gchar *history_id)
{
	GnomeEntry *gentry;

	gentry = g_object_new (GNOME_TYPE_ENTRY,
			       "history_id", history_id,
			       NULL);

	return GTK_WIDGET (gentry);
}

static void
free_item (gpointer data, gpointer user_data)
{
	struct item *item;

	item = data;
	if (item->text)
		g_free (item->text);

	g_free (item);
}

static void
free_items (GnomeEntry *gentry)
{
	g_list_foreach (gentry->_priv->items, free_item, NULL);
	g_list_free (gentry->_priv->items);
	gentry->_priv->items = NULL;
}

static void
gnome_entry_finalize (GObject *object)
{
	GnomeEntry *gentry;
	GConfClient *client;
	gchar *key;
	
	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (object));

	gentry = GNOME_ENTRY (object);

	gnome_gconf_lazy_init ();
	client = gconf_client_get_default ();
	key = build_gconf_key (gentry);
	gconf_client_remove_dir (client, key, NULL);
	g_free (key);
	
	if (gentry->_priv->gconf_notify_id != 0) {
		gconf_client_notify_remove (client, gentry->_priv->gconf_notify_id);
	}

	g_free(gentry->_priv);
	gentry->_priv = NULL;

	g_object_unref (G_OBJECT (client));
	
	if (G_OBJECT_CLASS (parent_class)->finalize)
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

/**
 * gnome_entry_gtk_entry
 * @gentry: Pointer to GnomeEntry object.
 *
 * Description: Obtain pointer to GnomeEntry's internal text entry
 *
 * Returns: Pointer to GtkEntry widget.
 */
GtkWidget *
gnome_entry_gtk_entry (GnomeEntry *gentry)
{
	g_return_val_if_fail (gentry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_ENTRY (gentry), NULL);

	return GTK_COMBO (gentry)->entry;
}

static void
gnome_entry_history_changed (GConfClient* client,
			     guint cnxn_id,
			     GConfEntry *entry,
			     gpointer user_data)
{
	GnomeEntry *gentry;

	gentry = GNOME_ENTRY (user_data);

	gnome_entry_load_history (gentry);
}

/* FIXME: Make this static */
void
gnome_entry_set_history_id (GnomeEntry *gentry, const gchar *history_id)
{
	gchar *key;
	GConfClient *client;
	
	g_return_if_fail (gentry != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (gentry));
	g_return_if_fail (gentry->_priv->history_id == NULL);

	if (history_id == NULL)
		return;

	gentry->_priv->history_id = g_strdup (history_id); /* this handles NULL correctly */

	/* Register with gconf */
	key = build_gconf_key (gentry);
	gnome_gconf_lazy_init ();
	client = gconf_client_get_default ();

	gconf_client_add_dir (client,
			      key,
			      GCONF_CLIENT_PRELOAD_NONE,
			      NULL);
	
	gentry->_priv->gconf_notify_id = gconf_client_notify_add (client,
								  key,
								  gnome_entry_history_changed,
								  gentry,
								  NULL, NULL);

	g_object_unref (G_OBJECT (client));
	g_free (key);
}

/**
 * gnome_entry_get_history_id
 * @gentry: Pointer to GnomeEntry object.
 *
 * Description: Returns the current history id of the GnomeEntry widget.
 *
 * Returns: The current history id.
 */
const gchar *
gnome_entry_get_history_id (GnomeEntry *gentry)
{
	g_return_val_if_fail (gentry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_ENTRY (gentry), NULL);

	return gentry->_priv->history_id;
}


/**
 * gnome_entry_set_max_saved
 * @gentry: Pointer to GnomeEntry object.
 * @max_saved: Maximum number of history items to save
 *
 * Description: Set internal limit on number of history items saved
 * to the config file, when #gnome_entry_save_history() is called.
 * Zero is an acceptable value for @max_saved, but the same thing is
 * accomplished by setting the history id of @gentry to %NULL.
 *
 * Returns:
 */
void
gnome_entry_set_max_saved (GnomeEntry *gentry, guint max_saved)
{
	g_return_if_fail (gentry != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (gentry));

	gentry->_priv->max_saved = max_saved;
}

/**
 * gnome_entry_get_max_saved
 * @gentry: Pointer to GnomeEntry object.
 *
 * Description: Get internal limit on number of history items saved
 * to the config file, when #gnome_entry_save_history() is called.
 * See #gnome_entry_set_max_saved().
 *
 * Returns: An unsigned integer
 */
guint
gnome_entry_get_max_saved (GnomeEntry *gentry)
{
	g_return_val_if_fail (gentry != NULL, 0);
	g_return_val_if_fail (GNOME_IS_ENTRY (gentry), 0);

	return gentry->_priv->max_saved;
}

static char *
build_gconf_key (GnomeEntry *gentry)
{
	return g_strconcat ("/apps/",
			    gnome_program_get_app_id (gnome_program_get()),
			    "/history-",
			    gentry->_priv->history_id,
			    NULL);
}

static void
gnome_entry_add_history (GnomeEntry *gentry, gboolean save,
			 const gchar *text, gboolean append)
{
	struct item *item;
	GList *gitem;
	GtkWidget *li;
	GtkWidget *entry;
	gchar *tmp;

	g_return_if_fail (gentry != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (gentry));
	g_return_if_fail (text != NULL); /* FIXME: should we just return without warning? */

	entry = gnome_entry_gtk_entry (gentry);
	tmp = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
	
	item = g_new (struct item, 1);
	item->save = save;
	item->text = g_strdup (text);

	gentry->_priv->items = g_list_prepend (gentry->_priv->items, item);

	li = gtk_list_item_new_with_label (text);
	gtk_widget_show (li);

	gitem = g_list_append (NULL, li);

	if (append)
	  gtk_list_prepend_items (GTK_LIST (GTK_COMBO (gentry)->list), gitem);
	else
	  gtk_list_append_items (GTK_LIST (GTK_COMBO (gentry)->list), gitem);

	gtk_entry_set_text (GTK_ENTRY (entry), tmp);
	g_free (tmp);

	/* gtk_entry_set_text runs our 'entry_changed' routine, so we have
	   to undo the effect */
	gentry->_priv->changed = FALSE;
}



/**
 * gnome_entry_prepend_history
 * @gentry: Pointer to GnomeEntry object.
 * @save: If %TRUE, history entry will be saved to config file
 * @text: Text to add
 *
 * Description: Adds a history item of the given @text to the head of
 * the history list inside @gentry.  If @save is %TRUE, the history
 * item will be saved in the config file (assuming that @gentry's
 * history id is not %NULL).
 *
 * Returns:
 */
void
gnome_entry_prepend_history (GnomeEntry *gentry, gboolean save,
			     const gchar *text)
{
  gnome_entry_add_history (gentry, save, text, FALSE);
}


/**
 * gnome_entry_append_history
 * @gentry: Pointer to GnomeEntry object.
 * @save: If %TRUE, history entry will be saved to config file
 * @text: Text to add
 *
 * Description: Adds a history item of the given @text to the tail
 * of the history list inside @gentry.  If @save is %TRUE, the
 * history item will be saved in the config file (assuming that
 * @gentry's history id is not %NULL).
 *
 * Returns:
 */
void
gnome_entry_append_history (GnomeEntry *gentry, gboolean save,
			    const gchar *text)
{
	gnome_entry_add_history (gentry, save, text, TRUE);
}



static void
set_combo_items (GnomeEntry *gentry)
{
	GtkList *gtklist;
	GList *items;
	GList *gitems;
	struct item *item;
	GtkWidget *li;
	GtkWidget *entry;
	gchar *tmp;

	gtklist = GTK_LIST (GTK_COMBO (gentry)->list);

	/* We save the contents of the entry because when we insert
	 * items on the combo list, the contents of the entry will get
	 * changed.
	 */

	entry = gnome_entry_gtk_entry (gentry);
	tmp = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
	
	gtk_list_clear_items (gtklist, 0, -1); /* erase everything */

	gitems = NULL;

	for (items = gentry->_priv->items; items; items = items->next) {
		item = items->data;

		li = gtk_list_item_new_with_label (item->text);
		gtk_widget_show (li);
		
		gitems = g_list_append (gitems, li);
	}

	gtk_list_append_items (gtklist, gitems); /* this handles NULL correctly */

	gtk_entry_set_text (GTK_ENTRY (entry), tmp);
	g_free (tmp);

	gentry->_priv->changed = FALSE;
}


static void
gnome_entry_load_history (GnomeEntry *gentry)
{
	struct item *item;
	gchar *key;
	GSList *gconf_items, *items;
	GConfClient *client;
	
	g_return_if_fail (gentry != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (gentry));

	if (gnome_program_get_app_id (gnome_program_get()) == NULL ||gentry->_priv->history_id == NULL)
		return;

	free_items (gentry);

	key = build_gconf_key (gentry);

	gnome_gconf_lazy_init ();
	client = gconf_client_get_default ();
	
	gconf_items = gconf_client_get_list (client, key, GCONF_VALUE_STRING, NULL);

	for (items = gconf_items; items; items = items->next) {

		item = g_new (struct item, 1);
		item->save = TRUE;
		item->text = items->data;

		gentry->_priv->items = g_list_append (gentry->_priv->items, item);
	}

	set_combo_items (gentry);

	g_slist_free (gconf_items);
	g_object_unref (G_OBJECT (client));
}

/**
 * gnome_entry_clear_history
 * @gentry: Pointer to GnomeEntry object.
 *
 * Description:  Clears the history, you should call #gnome_entry_save_history
 * To make the change permanent.
 *
 * Returns:
 */
void
gnome_entry_clear_history (GnomeEntry *gentry)
{
	g_return_if_fail (gentry != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (gentry));

	free_items (gentry);

	set_combo_items (gentry);
}

static gboolean
check_for_duplicates (GSList *gconf_items, 
		      const struct item *item)
{

	for (; gconf_items; gconf_items = gconf_items->next) {
		
		if (strcmp (gconf_items->data, item->text) == 0) {
			return FALSE;
		}
	}

	return TRUE;
}


static void
gnome_entry_save_history (GnomeEntry *gentry)
{
	GList *items;
	GConfClient *client;
	GSList *gconf_items;
	struct item *item;
	gchar *key;
	gint n;
	
	g_return_if_fail (gentry != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (gentry));

	if (gnome_program_get_app_id (gnome_program_get()) == NULL || gentry->_priv->history_id == NULL)
		return;

	key = build_gconf_key (gentry);
	gnome_gconf_lazy_init ();
	client = gconf_client_get_default ();

	gconf_items = NULL;

	for (n = 0, items = gentry->_priv->items; items && n < gentry->_priv->max_saved; items = items->next, n++) {
		item = items->data;

		if (item->save && check_for_duplicates (gconf_items, item)) {
			gconf_items = g_slist_prepend (gconf_items, item->text);
		}
	}

	/* Save the list */
	gconf_client_set_list (client, key, GCONF_VALUE_STRING, gconf_items, NULL);
	
	g_free (key);
	g_object_unref (G_OBJECT (client));
}

static void
insert_text (GtkEditable    *editable,
	     const gchar    *text,
	     gint            length,
	     gint           *position)
{
	GtkWidget *entry = gnome_entry_gtk_entry (GNOME_ENTRY (editable));
	gtk_editable_insert_text (GTK_EDITABLE (entry),
				  text,
				  length,
				  position);
}

static void
delete_text (GtkEditable    *editable,
	     gint            start_pos,
	     gint            end_pos)
{
	GtkWidget *entry = gnome_entry_gtk_entry (GNOME_ENTRY (editable));
	gtk_editable_delete_text (GTK_EDITABLE (entry),
				  start_pos,
				  end_pos);
}

static gchar*
get_chars (GtkEditable    *editable,
	   gint            start_pos,
	   gint            end_pos)
{
	GtkWidget *entry = gnome_entry_gtk_entry (GNOME_ENTRY (editable));
	return gtk_editable_get_chars (GTK_EDITABLE (entry),
				       start_pos,
				       end_pos);
}

static void
set_selection_bounds (GtkEditable    *editable,
		      gint            start_pos,
		      gint            end_pos)
{
	GtkWidget *entry = gnome_entry_gtk_entry (GNOME_ENTRY (editable));
	gtk_editable_select_region (GTK_EDITABLE (entry),
				    start_pos,
				    end_pos);
}

static gboolean
get_selection_bounds (GtkEditable    *editable,
		      gint           *start_pos,
		      gint           *end_pos)
{
	GtkWidget *entry = gnome_entry_gtk_entry (GNOME_ENTRY (editable));
	return gtk_editable_get_selection_bounds (GTK_EDITABLE (entry),
						  start_pos,
						  end_pos);
}

static void
set_position (GtkEditable    *editable,
	      gint            position)
{
	GtkWidget *entry = gnome_entry_gtk_entry (GNOME_ENTRY (editable));
	gtk_editable_set_position (GTK_EDITABLE (entry),
				   position);
}

static gint
get_position (GtkEditable    *editable)
{
	GtkWidget *entry = gnome_entry_gtk_entry (GNOME_ENTRY (editable));
	return gtk_editable_get_position (GTK_EDITABLE (entry));
}

static void
gnome_entry_editable_init (GtkEditableClass *iface)
{
	/* Just proxy to the GtkEntry */
	iface->insert_text = insert_text;
	iface->delete_text = delete_text;
	iface->get_chars = get_chars;
	iface->set_selection_bounds = set_selection_bounds;
	iface->get_selection_bounds = get_selection_bounds;
	iface->set_position = set_position;
	iface->get_position = get_position;
}
