/* gnome-href.c
 * Copyright (C) 1998-2001, James Henstridge <james@daa.com.au>
 * All rights reserved.
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
/*
  @NOTATION@
*/

#ifndef GNOME_DISABLE_DEPRECATED_SOURCE

#include "config.h"
#include <libgnome/gnome-macros.h>

/* Must be before all other gnome includes!! */
#include <glib/gi18n-lib.h>

#include <string.h> /* for strlen */

#include <gtk/gtk.h>
#include <libgnome/gnome-url.h>
#include "gnome-href.h"
#include "libgnomeui-access.h"
#include "gnome-url.h"

struct _GnomeHRefPrivate {
	gchar *url;
	GtkWidget *label;
};

static void gnome_href_clicked		(GtkButton *button);
static void gnome_href_destroy		(GtkObject *object);
static void gnome_href_finalize		(GObject *object);
static void gnome_href_get_property	(GObject *object,
					 guint param_id,
					 GValue *value,
					 GParamSpec * pspec);
static void gnome_href_set_property	(GObject *object,
					 guint param_id,
					 const GValue * value,
					 GParamSpec * pspec);
static void gnome_href_realize		(GtkWidget *widget);
static void gnome_href_style_set        (GtkWidget *widget,
					 GtkStyle *previous_style);
static void drag_data_get    		(GnomeHRef          *href,
					 GdkDragContext     *context,
					 GtkSelectionData   *selection_data,
					 guint               info,
					 guint               time,
					 gpointer            data);

enum {
	PROP_0,
	PROP_URL,
	PROP_TEXT
};

/**
 * gnome_href_get_type
 *
 * Returns the type assigned to the GNOME href widget.
 **/
/* The following defines the get_type */
GNOME_CLASS_BOILERPLATE (GnomeHRef, gnome_href,
			 GtkButton, GTK_TYPE_BUTTON)

static void
gnome_href_class_init (GnomeHRefClass *klass)
{
	GtkObjectClass *object_class = (GtkObjectClass *)klass;
	GObjectClass *gobject_class = (GObjectClass *)klass;
	GtkWidgetClass *widget_class = (GtkWidgetClass *)klass;
	GtkButtonClass *button_class = (GtkButtonClass *)klass;

	object_class->destroy = gnome_href_destroy;

	gobject_class->finalize = gnome_href_finalize;
	gobject_class->set_property = gnome_href_set_property;
	gobject_class->get_property = gnome_href_get_property;

	widget_class->realize = gnome_href_realize;
	widget_class->style_set = gnome_href_style_set;
	button_class->clicked = gnome_href_clicked;

	/* By default we link to The World Food Programme */
	g_object_class_install_property (gobject_class,
					 PROP_URL,
					 g_param_spec_string ("url",
							      _("URL"),
							      _("The URL that GnomeHRef activates"),
							      "http://www.wfp.org",
							      (G_PARAM_READABLE |
							       G_PARAM_WRITABLE)));
	g_object_class_install_property (gobject_class,
					 PROP_TEXT,
					 g_param_spec_string ("text",
							      _("Text"),
							      _("The text on the button"),
							      _("End World Hunger"),
							      (G_PARAM_READABLE |
							       G_PARAM_WRITABLE)));

	gtk_widget_class_install_style_property (GTK_WIDGET_CLASS (gobject_class),
						 g_param_spec_boxed ("link_color",
								     _("Link color"),
								     _("Color used to draw the link"),
								     GDK_TYPE_COLOR,
								     G_PARAM_READABLE));
}

static void
gnome_href_instance_init (GnomeHRef *href)
{
	href->_priv = g_new0(GnomeHRefPrivate, 1);

	href->_priv->label = gtk_label_new(NULL);
	gtk_widget_ref(href->_priv->label);


	gtk_button_set_relief(GTK_BUTTON(href), GTK_RELIEF_NONE);

	gtk_container_add(GTK_CONTAINER(href), href->_priv->label);
	gtk_widget_show(href->_priv->label);

	href->_priv->url = NULL;

	/* the source dest is set on set_url */
	g_signal_connect (href, "drag_data_get",
			  G_CALLBACK (drag_data_get), NULL);

	/* Set our accessible description.  We don't set the name as we want it
	 * to be the contents of the label, which is the default anyways.
	 */

	_add_atk_name_desc (GTK_WIDGET (href),
			    NULL,
			    _("This button will take you to the URI that it displays."));
}

static void
drag_data_get(GnomeHRef          *href,
	      GdkDragContext     *context,
	      GtkSelectionData   *selection_data,
	      guint               info,
	      guint               time,
	      gpointer            data)
{
	g_return_if_fail (href != NULL);
	g_return_if_fail (GNOME_IS_HREF (href));

	if( ! href->_priv->url) {
		/*FIXME: cancel the drag*/
		return;
	}

	/* if this doesn't look like an url, it's probably a file */
	if(strchr(href->_priv->url, ':') == NULL) {
		char *s = g_strdup_printf("file:%s\r\n", href->_priv->url);
		gtk_selection_data_set (selection_data,
					selection_data->target,
					8, (unsigned char *)s, strlen(s)+1);
		g_free(s);
	} else {
		gtk_selection_data_set (selection_data,
					selection_data->target,
					8, (unsigned char *)href->_priv->url, strlen(href->_priv->url)+1);
	}
}

/**
 * gnome_href_new
 * @url: URL assigned to this object.
 * @text: Text associated with the URL.
 *
 * Description:
 * Created a GNOME href object, a label widget with a clickable action
 * and an associated URL.  If @text is set to %NULL, @url is used as
 * the text for the label.
 *
 * Returns:  Pointer to new GNOME href widget.
 **/

GtkWidget *
gnome_href_new(const gchar *url, const gchar *text)
{
  GnomeHRef *href;

  g_return_val_if_fail(url != NULL, NULL);

  href = g_object_new (GNOME_TYPE_HREF,
		       "url", url,
		       "text", text,
		       NULL);

  return GTK_WIDGET (href);
}


/**
 * gnome_href_get_url
 * @href: Pointer to GnomeHRef widget
 *
 * Description:
 * Returns the pointer to the URL associated with the @href href object.  Note
 * that the string should not be freed as it is internal memory.
 *
 * Returns:  Pointer to an internal URL string, or %NULL if failure.
 **/

const gchar *gnome_href_get_url(GnomeHRef *href) {
  g_return_val_if_fail(href != NULL, NULL);
  g_return_val_if_fail(GNOME_IS_HREF(href), NULL);
  return href->_priv->url;
}


/**
 * gnome_href_set_url
 * @href: Pointer to GnomeHRef widget
 * @url: String containing the URL to be stored within @href.
 *
 * Description:
 * Sets the internal URL value within @href to the value of @url.
 **/

void gnome_href_set_url(GnomeHRef *href, const gchar *url) {
  g_return_if_fail(href != NULL);
  g_return_if_fail(GNOME_IS_HREF(href));
  g_return_if_fail(url != NULL);

  if (href->_priv->url) {
	  gtk_drag_source_unset(GTK_WIDGET(href));
	  g_free(href->_priv->url);
  }
  href->_priv->url = g_strdup(url);
  if(strncmp(url, "http://", 7) == 0 ||
     strncmp(url, "https://", 8) == 0) {
	  const GtkTargetEntry http_drop_types[] = {
	    { "text/uri-list",       0, 0 },
	    { "x-url/http",          0, 0 },
	    { "_NETSCAPE_URL",       0, 0 }
	  };

	  gtk_drag_source_set (GTK_WIDGET(href),
			       GDK_BUTTON1_MASK|GDK_BUTTON3_MASK,
			       http_drop_types, G_N_ELEMENTS (http_drop_types),
			       GDK_ACTION_COPY);
  } else if(strncmp(url, "ftp://", 6) == 0) {
	  const GtkTargetEntry ftp_drop_types[] = {
	    { "text/uri-list",       0, 0 },
	    { "x-url/ftp",           0, 0 },
	    { "_NETSCAPE_URL",       0, 0 }
	  };

	  gtk_drag_source_set (GTK_WIDGET(href),
			       GDK_BUTTON1_MASK|GDK_BUTTON3_MASK,
			       ftp_drop_types, G_N_ELEMENTS (ftp_drop_types),
			       GDK_ACTION_COPY);
  } else {
	  const GtkTargetEntry other_drop_types[] = {
	    { "text/uri-list",       0, 0 },
	    { "_NETSCAPE_URL",       0, 0 }
	  };

	  gtk_drag_source_set (GTK_WIDGET(href),
			       GDK_BUTTON1_MASK|GDK_BUTTON3_MASK,
			       other_drop_types, G_N_ELEMENTS (other_drop_types),
			       GDK_ACTION_COPY);
  }
}


/**
 * gnome_href_get_text
 * @href: Pointer to GnomeHRef widget
 *
 * Description:
 * Returns the contents of the label widget used to display the link text.
 * Note that the string should not be freed as it points to internal memory.
 *
 * Returns:  Pointer to text contained in the label widget.
 **/

const gchar *
gnome_href_get_text(GnomeHRef *href)
{

  g_return_val_if_fail(href != NULL, NULL);
  g_return_val_if_fail(GNOME_IS_HREF(href), NULL);

  return gtk_label_get_text (GTK_LABEL(href->_priv->label));
}


/**
 * gnome_href_set_text
 * @href: Pointer to GnomeHRef widget
 * @text: New link text for the href object.
 *
 * Description:
 * Sets the internal label widget text (used to display a URL's link
 * text) to the value given in @label.
 **/
void
gnome_href_set_text (GnomeHRef *href, const gchar *text)
{
  gchar *markup;

  g_return_if_fail(href != NULL);
  g_return_if_fail(GNOME_IS_HREF(href));
  g_return_if_fail(text != NULL);

  /* underline the string */
  markup = g_strdup_printf("<u>%s</u>", text);
  gtk_label_set_markup(GTK_LABEL(href->_priv->label), markup);
  g_free(markup);
}

/**
 * gnome_href_get_label
 * @href: Pointer to GnomeHRef widget
 *
 * Deprecated, use #gnome_href_get_text.
 *
 * Returns:
 **/

const gchar *
gnome_href_get_label(GnomeHRef *href)
{
	g_warning("gnome_href_get_label is deprecated, use gnome_href_get_text");
	return gnome_href_get_text(href);
}


/**
 * gnome_href_set_label
 * @href: Pointer to GnomeHRef widget
 * @label: New link text for the href object.
 *
 * Description:
 * deprecated, use #gnome_href_set_text
 **/

void
gnome_href_set_label (GnomeHRef *href, const gchar *label)
{
	g_return_if_fail(href != NULL);
	g_return_if_fail(GNOME_IS_HREF(href));

	g_warning("gnome_href_set_label is deprecated, use gnome_href_set_text");
	gnome_href_set_text(href, label);
}

static void
gnome_href_clicked (GtkButton *button)
{
  GnomeHRef *href;

  g_return_if_fail(button != NULL);
  g_return_if_fail(GNOME_IS_HREF(button));

  GNOME_CALL_PARENT (GTK_BUTTON_CLASS, clicked, (button));

  href = GNOME_HREF(button);

  g_return_if_fail(href->_priv->url != NULL);

  /* FIXME: Use the error variable from gnome_url_show_on_screen */
  if(!gnome_url_show_on_screen (href->_priv->url, gtk_widget_get_screen (GTK_WIDGET (href)), NULL)) {
    GtkWidget *dialog = gtk_message_dialog_new(NULL, 0, GTK_MESSAGE_ERROR,
					       GTK_BUTTONS_OK,
					       _("An error has occurred while trying to launch the "
						 "default web browser.\n"
						 "Please check your settings in the "
						 "'Preferred Applications' preference tool."));
    
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
  }
}

static void
gnome_href_destroy (GtkObject *object)
{
	GnomeHRef *href;

	/* remember, destroy can be run multiple times! */

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_HREF(object));

	href = GNOME_HREF (object);

	if (href->_priv->label != NULL) {
		gtk_widget_unref (href->_priv->label);
		href->_priv->label = NULL;
	}

	GNOME_CALL_PARENT (GTK_OBJECT_CLASS, destroy, (object));
}

static void
gnome_href_finalize (GObject *object)
{
	GnomeHRef *href;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_HREF(object));

	href = GNOME_HREF (object);

	g_free (href->_priv->url);
	href->_priv->url = NULL;

	g_free (href->_priv);
	href->_priv = NULL;

	GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (object));
}

static void
gnome_href_realize(GtkWidget *widget)
{
	GdkCursor *cursor;

	GNOME_CALL_PARENT (GTK_WIDGET_CLASS, realize, (widget));

	cursor = gdk_cursor_new (GDK_HAND2);
	gdk_window_set_cursor (GTK_BUTTON (widget)->event_window, cursor);
	gdk_cursor_unref (cursor);
}

static void
gnome_href_style_set (GtkWidget *widget,
		      GtkStyle *previous_style)
{
	GdkColor *link_color;
	GdkColor blue = { 0, 0x0000, 0x0000, 0xffff };
	GnomeHRef *href;

	href = GNOME_HREF (widget);
	
	gtk_widget_style_get (widget,
			      "link_color", &link_color,
			      NULL);

	if (!link_color)
		link_color = &blue;

	gtk_widget_modify_fg (GTK_WIDGET(href->_priv->label),
			      GTK_STATE_NORMAL, link_color);
	gtk_widget_modify_fg (GTK_WIDGET(href->_priv->label),
			      GTK_STATE_ACTIVE, link_color);
	gtk_widget_modify_fg (GTK_WIDGET(href->_priv->label),
			      GTK_STATE_PRELIGHT, link_color);
	gtk_widget_modify_fg (GTK_WIDGET(href->_priv->label),
			      GTK_STATE_SELECTED, link_color);

	if (link_color != &blue)
		gdk_color_free (link_color);
}

static void
gnome_href_set_property (GObject *object,
			 guint param_id,
			 const GValue * value,
			 GParamSpec * pspec)
{
	GnomeHRef *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_HREF (object));

	self = GNOME_HREF (object);

	switch (param_id) {
	case PROP_URL:
		gnome_href_set_url(self, g_value_get_string (value));
		break;
	case PROP_TEXT:
		gnome_href_set_text(self, g_value_get_string (value));
		break;
	default:
		break;
	}
}

static void
gnome_href_get_property (GObject *object,
		      guint param_id,
		      GValue *value,
		      GParamSpec * pspec)
{
	GnomeHRef *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_HREF (object));

	self = GNOME_HREF (object);

	switch (param_id) {
	case PROP_URL:
		g_value_set_string (value,
				    gnome_href_get_url(self));
		break;
	case PROP_TEXT:
		g_value_set_string (value,
				    gnome_href_get_text(self));
		break;
	default:
		break;
	}
}

#endif /* not GNOME_DISABLE_DEPRECATED_SOURCE */
