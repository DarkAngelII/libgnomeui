/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* gnome-pouch.c - a GnomePouch widget - WiW MDI container

   Copyright (C) 2000 Jaka Mocnik

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Jaka Mocnik <jaka.mocnik@kiss.uni-lj.si>
*/

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <config.h>
#include "libgnome/gnome-defs.h"
#include "libgnome/gnome-config.h"
#include "libgnome/gnome-i18nP.h"
#include "libgnome/gnome-util.h"
#include "libgnomeui/gnome-app.h"
#include "libgnomeui/gnome-app-helper.h"
#include "libgnomeui/gnome-popup-menu.h"
#include "gnome-pouch.h"
#include "gnome-roo.h"

enum {
	CLOSE_CHILD,
	ICONIFY_CHILD,
    UNICONIFY_CHILD,
    MAXIMIZE_CHILD,
	UNMAXIMIZE_CHILD,
	SELECT_CHILD,
	UNSELECT_CHILD,
	LAST_SIGNAL
};

typedef void (*GnomePouchSignal)(GtkObject *, GtkWidget *, gpointer);

guint pouch_signals[LAST_SIGNAL];

static GtkObjectClass *parent_class;

static void gnome_pouch_class_init(GnomePouchClass *klass);
static void gnome_pouch_init(GnomePouch *pouch);

static gboolean gnome_pouch_button_press(GtkWidget *w, GdkEventButton *e);

static void gnome_pouch_add(GtkContainer *container, GtkWidget *child);
static void gnome_pouch_remove(GtkContainer *container, GtkWidget *child);

static void gnome_pouch_maximize_child(GnomePouch *pouch, GnomeRoo *roo);
static void gnome_pouch_unmaximize_child(GnomePouch *pouch, GnomeRoo *roo);
static void gnome_pouch_iconify_child(GnomePouch *pouch, GnomeRoo *roo);
static void gnome_pouch_uniconify_child(GnomePouch *pouch, GnomeRoo *roo);
static void gnome_pouch_select_child(GnomePouch *pouch, GnomeRoo *roo);
static void gnome_pouch_unselect_child(GnomePouch *pouch, GnomeRoo *roo);

static void arrange_roo(GnomePouch *pouch, GnomeRoo *roo);
static void set_active_items(GnomePouch *pouch);

/* popup menu callbacks */
static void tile_callback(GtkWidget *w, gpointer user_data);
static void cascade_callback(GtkWidget *w, gpointer user_data);
static void arrange_callback(GtkWidget *w, gpointer user_data);
static void autoarrange_callback(GtkWidget *w, gpointer user_data);
static void orientation_callback(GtkWidget *w, gpointer user_data);
static void position_callback(GtkWidget *w, gpointer user_data);

static void gnome_pouch_marshal(GtkObject *object, GtkSignalFunc func,
								gpointer func_data, GtkArg *args)
{
	GnomePouchSignal rfunc;

	rfunc = (GnomePouchSignal)func;

	(*rfunc)(object, GTK_VALUE_POINTER(args[0]), func_data);
}

guint gnome_pouch_get_type()
{
	static guint pouch_type = 0;
	
	if (!pouch_type) {
		GtkTypeInfo pouch_info = {
			"GnomePouch",
			sizeof(GnomePouch),
			sizeof(GnomePouchClass),
			(GtkClassInitFunc) gnome_pouch_class_init,
			(GtkObjectInitFunc) gnome_pouch_init,
			(GtkArgSetFunc) NULL,
			(GtkArgGetFunc) NULL,
		};
		pouch_type = gtk_type_unique(gtk_fixed_get_type(), &pouch_info);
	}
	return pouch_type;
}

static void gnome_pouch_class_init(GnomePouchClass *klass)
{
	GtkContainerClass *container_class;
	GtkWidgetClass *widget_class;
	GtkObjectClass *object_class;

	parent_class = GTK_OBJECT_CLASS(gtk_type_class(gtk_fixed_get_type()));
	container_class = GTK_CONTAINER_CLASS(klass);
	widget_class = GTK_WIDGET_CLASS(klass);
	object_class = GTK_OBJECT_CLASS(klass);

	pouch_signals[CLOSE_CHILD] = gtk_signal_new ("close-child",
												 GTK_RUN_LAST,
												 object_class->type,
												 GTK_SIGNAL_OFFSET (GnomePouchClass, close_child),
												 gnome_pouch_marshal, GTK_TYPE_NONE, 1, GTK_TYPE_WIDGET);
	pouch_signals[ICONIFY_CHILD] = gtk_signal_new ("iconify-child",
												   GTK_RUN_FIRST,
												   object_class->type,
												   GTK_SIGNAL_OFFSET (GnomePouchClass, iconify_child),
												   gnome_pouch_marshal, GTK_TYPE_NONE, 1, GTK_TYPE_WIDGET);
	pouch_signals[UNICONIFY_CHILD] = gtk_signal_new ("uniconify-child",
													 GTK_RUN_FIRST,
													 object_class->type,
													 GTK_SIGNAL_OFFSET (GnomePouchClass, uniconify_child),
													 gnome_pouch_marshal, GTK_TYPE_NONE, 1, GTK_TYPE_WIDGET);
	pouch_signals[MAXIMIZE_CHILD] = gtk_signal_new ("maximize-child",
													GTK_RUN_FIRST,
													object_class->type,
													GTK_SIGNAL_OFFSET (GnomePouchClass, maximize_child),
													gnome_pouch_marshal, GTK_TYPE_NONE, 1, GTK_TYPE_WIDGET);
	pouch_signals[UNMAXIMIZE_CHILD] = gtk_signal_new ("unmaximize-child",
													  GTK_RUN_FIRST,
													  object_class->type,
													  GTK_SIGNAL_OFFSET (GnomePouchClass, unmaximize_child),
													  gnome_pouch_marshal, GTK_TYPE_NONE, 1, GTK_TYPE_WIDGET);
	pouch_signals[SELECT_CHILD] = gtk_signal_new ("select-child",
												  GTK_RUN_FIRST,
												  object_class->type,
												  GTK_SIGNAL_OFFSET (GnomePouchClass, select_child),
												  gnome_pouch_marshal, GTK_TYPE_NONE, 1, GTK_TYPE_WIDGET);
	pouch_signals[UNSELECT_CHILD] = gtk_signal_new ("unselect-child",
													GTK_RUN_FIRST,
													object_class->type,
													GTK_SIGNAL_OFFSET (GnomePouchClass, unselect_child),
													gnome_pouch_marshal, GTK_TYPE_NONE, 1, GTK_TYPE_WIDGET);

	gtk_object_class_add_signals(object_class, pouch_signals, LAST_SIGNAL);

	widget_class->button_press_event = gnome_pouch_button_press;

	container_class->add = gnome_pouch_add;
	container_class->remove = gnome_pouch_remove;

	klass->iconify_child = gnome_pouch_iconify_child;
	klass->uniconify_child = gnome_pouch_uniconify_child;
	klass->maximize_child = gnome_pouch_maximize_child;
	klass->unmaximize_child = gnome_pouch_unmaximize_child;
	klass->select_child = gnome_pouch_select_child;
	klass->unselect_child = gnome_pouch_unselect_child;
}

static void gnome_pouch_init(GnomePouch *pouch)
{
	pouch->arranged = NULL;
	pouch->selected = NULL;
	pouch->popup_menu = NULL;

	pouch->icon_corner = GTK_CORNER_BOTTOM_LEFT;
	pouch->icon_orientation = GTK_ORIENTATION_HORIZONTAL;
	pouch->auto_arrange = TRUE;
}

static void gnome_pouch_add(GtkContainer *container, GtkWidget *child)
{
	GnomeRoo *roo;

	g_return_if_fail(GNOME_IS_ROO(child));

	roo = GNOME_ROO(child);
	roo->user_allocation.x = roo->user_allocation.y = 0;
	roo->icon_allocation.x = roo->icon_allocation.y = 0;

	gtk_fixed_put(GTK_FIXED(container), child, 0, 0);
}

static void gnome_pouch_remove(GtkContainer *container, GtkWidget *child)
{
	GnomeRoo *roo;
	GnomePouch *pouch;

	g_return_if_fail(child != NULL);
	g_return_if_fail(GNOME_IS_ROO(child));

	pouch = GNOME_POUCH(container);
	roo = GNOME_ROO(child);

	if(pouch->selected == roo)
		gnome_pouch_select_roo(pouch, NULL);

	if(gnome_roo_is_iconified(roo))
		pouch->arranged = g_list_remove(pouch->arranged, roo);

	(*GTK_CONTAINER_CLASS(parent_class)->remove)(container, child);
}

static gboolean gnome_pouch_button_press(GtkWidget *w, GdkEventButton *e)
{
	GnomePouch *pouch = GNOME_POUCH(w);

	if(e->button == 1) {
		/* unselect a possibly selected child */
		if(pouch->selected) {
			gtk_signal_emit_by_name(GTK_OBJECT(pouch->selected), "deselect",
									NULL);
		}
	}
	else if(e->button == 3 && pouch->popup_menu) {
		/* popup a menu */
		gnome_popup_menu_do_popup(pouch->popup_menu, NULL, NULL,
								  e, pouch, GTK_WIDGET(pouch));
		set_active_items(pouch);
		return TRUE;
	}

	return FALSE;
}

static void gnome_pouch_maximize_child(GnomePouch *pouch, GnomeRoo *roo)
{
	GtkWidget *w = GTK_WIDGET(roo);

#ifdef GNOME_ENABLE_DEBUG
		g_message("GnomePouch: maximize");
#endif

	gnome_pouch_move(pouch, roo, 0, 0);
	gtk_widget_set_usize(w,
						 GTK_WIDGET(pouch)->allocation.width,
						 GTK_WIDGET(pouch)->allocation.height);
	gdk_window_raise(w->window);
}

static void gnome_pouch_unmaximize_child(GnomePouch *pouch, GnomeRoo *roo)
{
	GtkWidget *w = GTK_WIDGET(roo);

#ifdef GNOME_ENABLE_DEBUG
		g_message("GnomePouch: unmaximize");
#endif

	if(gnome_roo_is_iconified(roo)) {
		gtk_fixed_move(GTK_FIXED(pouch), GTK_WIDGET(roo),
					   roo->icon_allocation.x, roo->icon_allocation.y);
	}
	else {
		gnome_pouch_move(pouch, roo,
						 roo->user_allocation.x, roo->user_allocation.y);
		gtk_widget_set_usize(w,
							 roo->user_allocation.width,
							 roo->user_allocation.height);
	}
}

static void gnome_pouch_iconify_child(GnomePouch *pouch, GnomeRoo *roo)
{
#ifdef GNOME_ENABLE_DEBUG
		g_message("GnomePouch: iconify");
#endif

	if(!gnome_roo_is_parked(roo)) {
		if(pouch->auto_arrange)
			arrange_roo(pouch, roo);
	}
	else {
		gnome_pouch_move(pouch, roo,
						 roo->icon_allocation.x, roo->icon_allocation.y);
	}
}

static void gnome_pouch_uniconify_child(GnomePouch *pouch, GnomeRoo *roo)
{
#ifdef GNOME_ENABLE_DEBUG
	g_message("GnomePouch: uniconify");
#endif

	gnome_pouch_move(pouch, roo,
					 roo->user_allocation.x, roo->user_allocation.y);
	gtk_widget_set_usize(GTK_WIDGET(roo),
						 roo->user_allocation.width,
						 roo->user_allocation.height);

	pouch->arranged = g_list_remove(pouch->arranged, roo);
}

static void gnome_pouch_select_child(GnomePouch *pouch, GnomeRoo *roo)
{
	if(pouch->selected == roo)
		return;

#ifdef GNOME_ENABLE_DEBUG
	g_message("GnomePouch: select");
#endif
	if(pouch->selected) 
		gtk_signal_emit_by_name(GTK_OBJECT(pouch->selected), "deselect", NULL);
	if(roo) {
		if(pouch->selected && gnome_roo_is_maximized(pouch->selected))
			gnome_roo_set_maximized(roo, TRUE);
		gdk_window_raise(GTK_WIDGET(roo)->window);
	}
	pouch->selected = roo;
}

static void gnome_pouch_unselect_child(GnomePouch *pouch, GnomeRoo *roo)
{
	g_return_if_fail(roo != NULL);
	g_return_if_fail(GNOME_IS_ROO(roo));

#ifdef GNOME_ENABLE_DEBUG
	g_message("GnomePouch: unselect");
#endif
	if(pouch->selected == roo)
		pouch->selected = NULL;

	if(gnome_roo_is_maximized(roo))
		gnome_roo_set_maximized(roo, FALSE);
}

static void arrange_roo(GnomePouch *pouch, GnomeRoo *roo)
{
	GList *arranged, *node;
	GnomeRoo *this_roo, *next_roo;
	gint space = 0;
	gint px = 0, py = 0;
	gboolean found = FALSE, new_line = TRUE, was_new_line = FALSE; 

	arranged = pouch->arranged;

	while(arranged && arranged->next && !found) {
		was_new_line = new_line;
		this_roo = GNOME_ROO(arranged->data);
		next_roo = GNOME_ROO(arranged->next->data);

		if(new_line) {
			switch(pouch->icon_orientation) {
			case GTK_ORIENTATION_VERTICAL:
				if(pouch->icon_corner == GTK_CORNER_TOP_LEFT ||
				   pouch->icon_corner == GTK_CORNER_TOP_RIGHT) {
					space = this_roo->icon_allocation.y;
				}
				else {
					space = GTK_WIDGET(pouch)->allocation.height - this_roo->icon_allocation.y - this_roo->icon_allocation.height;
				}
				break;
			case GTK_ORIENTATION_HORIZONTAL:
				if(pouch->icon_corner == GTK_CORNER_TOP_LEFT ||
				   pouch->icon_corner == GTK_CORNER_BOTTOM_LEFT) {
					space = this_roo->icon_allocation.x;
				}
				else {
					space = GTK_WIDGET(pouch)->allocation.width - this_roo->icon_allocation.x - this_roo->icon_allocation.width;
				}
				break;
			}

#ifdef GNOME_ENABLE_DEBUG
			g_message("GnomePouch: found space at new line: %d", space);
#endif

			new_line = FALSE;
		}
		else {
			switch(pouch->icon_orientation) {
			case GTK_ORIENTATION_VERTICAL:
				if(pouch->icon_corner == GTK_CORNER_TOP_LEFT ||
				   pouch->icon_corner == GTK_CORNER_TOP_RIGHT) {
					if(next_roo->icon_allocation.y <= this_roo->icon_allocation.y) {
						space = GTK_WIDGET(pouch)->allocation.height - this_roo->icon_allocation.y - this_roo->icon_allocation.height;
						new_line = TRUE;
					}
					else 
						space = next_roo->icon_allocation.y - this_roo->icon_allocation.y - this_roo->icon_allocation.height;
				}
				else if(pouch->icon_corner == GTK_CORNER_BOTTOM_LEFT ||
						pouch->icon_corner == GTK_CORNER_BOTTOM_RIGHT) {
					if(next_roo->icon_allocation.y >= this_roo->icon_allocation.y) {
						space = this_roo->icon_allocation.y;
						new_line = TRUE;
					}
					else
						space = this_roo->icon_allocation.y - next_roo->icon_allocation.y - this_roo->icon_allocation.height;
				}
				break;
			case GTK_ORIENTATION_HORIZONTAL:
				if(pouch->icon_corner == GTK_CORNER_TOP_LEFT ||
				   pouch->icon_corner == GTK_CORNER_BOTTOM_LEFT) {
					if(next_roo->icon_allocation.x <= this_roo->icon_allocation.x) {
						space = GTK_WIDGET(pouch)->allocation.width - this_roo->icon_allocation.x - this_roo->icon_allocation.width;
						new_line = TRUE;
					}
					else
						space = next_roo->icon_allocation.x - this_roo->icon_allocation.x - this_roo->icon_allocation.width;
				}
				else if(pouch->icon_corner == GTK_CORNER_TOP_RIGHT ||
						pouch->icon_corner == GTK_CORNER_BOTTOM_RIGHT) {					
					if(next_roo->icon_allocation.x >= this_roo->icon_allocation.x) {
						space = this_roo->icon_allocation.x;
						new_line = TRUE;
					}
					else
						space = this_roo->icon_allocation.x - next_roo->icon_allocation.x - this_roo->icon_allocation.width;
				}
				break;
			}
		}

#ifdef GNOME_ENABLE_DEBUG
		g_message("GnomePouch: found %d space", space);
#endif

		switch(pouch->icon_orientation) {
		case GTK_ORIENTATION_VERTICAL:
			px = this_roo->icon_allocation.x;
			if(space >= roo->icon_allocation.height) {
				if(pouch->icon_corner == GTK_CORNER_TOP_LEFT ||
				   pouch->icon_corner == GTK_CORNER_TOP_RIGHT) {
					if(was_new_line)
						py = 0;
					else
						py = this_roo->icon_allocation.y + this_roo->icon_allocation.height;
				}
				else {
					if(was_new_line)
						py = GTK_WIDGET(pouch)->allocation.height - roo->icon_allocation.height;
					else
						py = this_roo->icon_allocation.y - roo->icon_allocation.height;
				}
				found = TRUE;
			}
			break;
		case GTK_ORIENTATION_HORIZONTAL:
			py = this_roo->icon_allocation.y;
			if(space >= roo->icon_allocation.width) {
				if(pouch->icon_corner == GTK_CORNER_TOP_LEFT ||
				   pouch->icon_corner == GTK_CORNER_BOTTOM_LEFT) {
					if(was_new_line)
						px = 0;
					else
						px = this_roo->icon_allocation.x + this_roo->icon_allocation.width;
				}
				else {
					if(was_new_line)
						px = GTK_WIDGET(pouch)->allocation.width - roo->icon_allocation.width;
					else
						px = this_roo->icon_allocation.x - roo->icon_allocation.width;
				}
				found = TRUE;
			}
			break;
		}

		if(!found && !was_new_line)
			arranged = arranged->next;
	}


	if(!found) {
		if(arranged) { /* && !arranged->next */
			this_roo = GNOME_ROO(arranged->data);

			/* have we really found space after the last iconified roo */
#ifdef GNOME_ENABLE_DEBUG
			g_message("GnomePouch: found space after last roo");
#endif
			switch(pouch->icon_orientation) {
			case GTK_ORIENTATION_HORIZONTAL:
				if(pouch->icon_corner == GTK_CORNER_TOP_LEFT ||
				   pouch->icon_corner == GTK_CORNER_BOTTOM_LEFT) { /* left corners */
					if(GTK_WIDGET(pouch)->allocation.width - this_roo->icon_allocation.x - this_roo->icon_allocation.width >= roo->icon_allocation.width) {
						py = this_roo->icon_allocation.y;
						px = this_roo->icon_allocation.x +
							 this_roo->icon_allocation.width;
					}
					else { /* next row */
						px = 0;
						if(pouch->icon_corner == GTK_CORNER_TOP_LEFT)
							py = this_roo->icon_allocation.y +
								 this_roo->icon_allocation.height;
						else
							py = this_roo->icon_allocation.y -
								 roo->icon_allocation.height;
					}
				}
				else { /* right corners */
					if(this_roo->icon_allocation.x >= roo->icon_allocation.width) {
						py = this_roo->icon_allocation.y;
						px = this_roo->icon_allocation.x -
							 roo->icon_allocation.width;
					}
					else { /* next row */
						px = GTK_WIDGET(pouch)->allocation.width - roo->icon_allocation.width;
						if(pouch->icon_corner == GTK_CORNER_TOP_RIGHT)
							py = this_roo->icon_allocation.y +
								 this_roo->icon_allocation.height;
						else
							py = this_roo->icon_allocation.y -
								 roo->icon_allocation.height;
					}
				}
				break;
			case GTK_ORIENTATION_VERTICAL:
				if(pouch->icon_corner == GTK_CORNER_TOP_LEFT ||
				   pouch->icon_corner == GTK_CORNER_TOP_RIGHT) { /* top corners */
					if(GTK_WIDGET(pouch)->allocation.height - this_roo->icon_allocation.y - this_roo->icon_allocation.height >= roo->icon_allocation.height) {
						py = this_roo->icon_allocation.y +
							 this_roo->icon_allocation.height;
						px = this_roo->icon_allocation.x;
					}
					else { /* next row */
						py = 0;
						if(pouch->icon_corner == GTK_CORNER_TOP_LEFT)
							px = this_roo->icon_allocation.x +
								 this_roo->icon_allocation.width;
						else
							px = this_roo->icon_allocation.x -
								 roo->icon_allocation.width;
					}
				}
				else { /* bottom corners */
					if(this_roo->icon_allocation.y >= roo->icon_allocation.height) {
						py = this_roo->icon_allocation.y -
							 roo->icon_allocation.height;
						px = this_roo->icon_allocation.x;
					}
					else { /* next row */
						py = GTK_WIDGET(pouch)->allocation.height - roo->icon_allocation.height;
						if(pouch->icon_corner == GTK_CORNER_BOTTOM_LEFT)
							px = this_roo->icon_allocation.x +
								 this_roo->icon_allocation.width;
						else
							px = this_roo->icon_allocation.x -
								 roo->icon_allocation.width;
					}
				}
				break;
			default:
				break;
			}

			node = g_list_alloc();
			node->data = roo;
			node->prev = arranged;
			node->next = NULL;
			arranged->next = node;
		}
		else { /* the first roo */
			switch(pouch->icon_corner) {
			case GTK_CORNER_TOP_LEFT:
				px = 0;
				py = 0;
				break;
			case GTK_CORNER_BOTTOM_LEFT:
				px = 0;
				py = GTK_WIDGET(pouch)->allocation.height - roo->icon_allocation.height;
				break;
			case GTK_CORNER_BOTTOM_RIGHT:
				px = GTK_WIDGET(pouch)->allocation.width - roo->icon_allocation.width;
				py = GTK_WIDGET(pouch)->allocation.height - roo->icon_allocation.height;
				break;
			case GTK_CORNER_TOP_RIGHT:
				px = GTK_WIDGET(pouch)->allocation.width - roo->icon_allocation.width;
				py = 0;
				break;
			}

			pouch->arranged = g_list_alloc();
			pouch->arranged->data = roo;
		}
	}
	else {
		node = g_list_alloc();
		node->data = roo;
		if(was_new_line) {
			node->next = arranged;
			if(arranged->prev) {
				node->prev = arranged->prev;
				arranged->prev->next = node;
			}
			else
				pouch->arranged = node;
			arranged->prev = node;
		}
		else {
			node->prev = arranged;
			node->next = arranged->next;
			arranged->next = node;
			node->next->prev = node;
		}
	}

	roo->icon_allocation.x = px;
	roo->icon_allocation.y = py;

	gtk_fixed_move(GTK_FIXED(pouch), GTK_WIDGET(roo), px, py);
}

static void set_active_items(GnomePouch *pouch)
{
	gtk_check_menu_item_set_active(pouch->oitem[pouch->icon_orientation], TRUE);
	gtk_check_menu_item_set_active(pouch->citem[pouch->icon_corner], TRUE);
	gtk_check_menu_item_set_active(pouch->aitem, pouch->auto_arrange);
}

/**
 * gnome_pouch_move:
 * @pouch: A pointer to a GnomePouch widget.
 * @roo: A pointer to a child GnomeRoo widget.
 * @x: New horizontal position.
 * @y: New vertical position.
 * 
 * Description:
 * Moves the child @roo to a new position (@x,@y).
 **/
void gnome_pouch_move(GnomePouch *pouch, GnomeRoo *roo, gint x, gint y)
{
	g_return_if_fail(pouch != NULL);
	g_return_if_fail(GNOME_IS_POUCH(pouch));
	g_return_if_fail(roo != NULL);
	g_return_if_fail(GNOME_IS_ROO(roo));

	if(gnome_roo_is_iconified(roo)) {
		pouch->arranged = g_list_remove(pouch->arranged, roo);
		roo->icon_allocation.x = x;
		roo->icon_allocation.y = y;
	}

	gtk_fixed_move(GTK_FIXED(pouch), GTK_WIDGET(roo), x, y);
}

/**
 * gnome_pouch_select_roo:
 * @pouch: A pointer to a GnomePouch widget.
 * @roo: A pointer to a GnomeRoo widget that should be selected or NULL
 * for no selected GnomeRoo.
 *
 * Description:
 * Selects @roo.
 **/
void gnome_pouch_select_roo(GnomePouch *pouch, GnomeRoo *roo)
{
	g_return_if_fail(pouch != NULL);
	g_return_if_fail(GNOME_IS_POUCH(pouch));

	if(roo)
		gtk_signal_emit_by_name(GTK_OBJECT(roo), "select", NULL);
	else if(pouch->selected)
		gtk_signal_emit_by_name(GTK_OBJECT(pouch->selected), "deselect", NULL);
}

/**
 * gnome_pouch_arrange_icons:
 * @pouch: A pointer to a GnomePouch widget.
 * 
 * Description:
 * Arranges all iconified and not parked children according to the user's
 * preferred arrangement set with a call to gnome_pouch_set_icon_arrangement().
 **/
void gnome_pouch_arrange_icons(GnomePouch *pouch)
{
	GList *child_info;
	GnomeRoo *roo;

	g_return_if_fail(pouch != NULL);
	g_return_if_fail(GNOME_IS_POUCH(pouch));

	if(pouch->arranged) {
		g_list_free(pouch->arranged);
		pouch->arranged = NULL;
	}

	child_info = GTK_FIXED(pouch)->children;
	while(child_info) {
		roo = GNOME_ROO(((GtkFixedChild *)child_info->data)->widget);
		if(gnome_roo_is_iconified(roo)) {
			if(gnome_roo_is_parked(roo))
				gnome_roo_unpark(roo);
			arrange_roo(pouch, roo);
		}
		child_info = child_info->next;
	}
}

/**
 * gnome_pouch_set_icon_arrangement:
 * @pouch: A pointer to a GnomePouch widget.
 * @corner: GtkCornerType specifying corner for the iconified children.
 * @orientation: GtkOrientationType specifiying whether the iconified children
 * should be laid out horizontally or vertically.
 *
 * Description:
 * Specifies the corner and the direction in which the iconified roos arranged
 * by the pouch should be put. The children are stacked according to
 * @orientation, starting in corner @corner.
 **/
void gnome_pouch_set_icon_arrangement(GnomePouch *pouch,
									  GtkCornerType corner,
									  GtkOrientation orientation)
{
	g_return_if_fail(pouch != NULL);
	g_return_if_fail(GNOME_IS_POUCH(pouch));

	if(corner != pouch->icon_corner || orientation != pouch->icon_orientation) {
		pouch->icon_corner = corner;
		pouch->icon_orientation = orientation;
	}
}

#if 0
static GtkWidget *gnome_app_find_menu_item(GtkWidget *parent, gchar *path)
{
	GtkBin *item;
	gchar *label = NULL;
	GList *children;
	gchar *name_end;
	gchar *part, *transl;
	gint p;
	int  path_len;
	int  stripped_path_len;
	
	g_return_val_if_fail (parent != NULL, NULL);
	g_return_val_if_fail (path != NULL, NULL);

	children = GTK_MENU_SHELL (parent)->children;
	
	name_end = strchr(path, '/');
	if(name_end == NULL)
		path_len = strlen(path);
	else
		path_len = name_end - path;

	if (path_len == 0) {
		return NULL;
	}

	/* this ugly thing should fix the localization problems */
	part = g_malloc(path_len + 1);
	if(!part)
	        return NULL;
	strncpy(part, path, path_len);
	part[path_len] = '\0';
	transl = L_(part);
	path_len = strlen(transl);

	stripped_path_len = path_len;
	for ( p = 0; p < path_len; p++ )
	        if( transl[p] == '_' )
		        stripped_path_len--;
		
	while (children){
		item = GTK_BIN (children->data);
		children = children->next;
		label = NULL;
		
		if (GTK_IS_TEAROFF_MENU_ITEM(item))
			label = NULL;
		else if (!item->child)          /* this is a separator, right? */
			label = "<Separator>";
		else if (GTK_IS_LABEL (item->child))  /* a simple item with a label */
			label = GTK_LABEL (item->child)->label;
		else
			label = NULL; /* something that we just can't handle */
		if (label && (stripped_path_len == strlen (label)) &&
		    (g_strncmp_ignore_char (transl, label, path_len, '_') == 0)){
			if (name_end == NULL) {
				g_free(part);
				return GTK_WIDGET(item);
			}
			else if (GTK_MENU_ITEM(item)->submenu) {
			        g_free(part);
					return gnome_app_find_menu_item(GTK_MENU_ITEM(item)->submenu, 
													(gchar *)(name_end + 1));
			}
			else {
			        g_free(part);
					return NULL;
			}
		}
	}
	
	g_free(part);
	return NULL;
}
#endif

/**
 * gnome_pouch_enable_auto_arrange:
 * @auto_arr: A gboolean specifying if the children should be automatically
 * arranged when iconified.
 *
 * Description:
 * If set to %TRUE, the children which have not been gnome_roo_park()ed,
 * will be automatically arranged by the pouch when iconified.
 **/
void gnome_pouch_enable_auto_arrange(GnomePouch *pouch, gboolean auto_arr)
{
	g_return_if_fail(pouch != NULL);
	g_return_if_fail(GNOME_IS_POUCH(pouch));

	pouch->auto_arrange = auto_arr;
}

/** 
 * popup menu
 **/
static GnomeUIInfo position_list[] = {
	/* to be kept in sync with GtkCornerType enumeration */
	GNOMEUIINFO_RADIOITEM_DATA(N_("Top left"), NULL, position_callback, (gpointer)GTK_CORNER_TOP_LEFT, NULL),
	GNOMEUIINFO_RADIOITEM_DATA(N_("Bottom left"), NULL, position_callback, (gpointer)GTK_CORNER_BOTTOM_LEFT, NULL),
	GNOMEUIINFO_RADIOITEM_DATA(N_("Top right"), NULL, position_callback, (gpointer)GTK_CORNER_TOP_RIGHT, NULL),
	GNOMEUIINFO_RADIOITEM_DATA(N_("Bottom right"), NULL, position_callback, (gpointer)GTK_CORNER_BOTTOM_RIGHT, NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo position_menu[] = {
	GNOMEUIINFO_RADIOLIST(position_list),
	GNOMEUIINFO_END
};

static GnomeUIInfo orientation_list[] = {
	/* to be kept in sync with GtkOrientation enumeration */
	GNOMEUIINFO_RADIOITEM_DATA(N_("Horizontal"), NULL, orientation_callback, (gpointer)GTK_ORIENTATION_HORIZONTAL, NULL),
	GNOMEUIINFO_RADIOITEM_DATA(N_("Vertical"), NULL, orientation_callback, (gpointer)GTK_ORIENTATION_VERTICAL, NULL),
	GNOMEUIINFO_END
};

static GnomeUIInfo orientation_menu[] = {
	GNOMEUIINFO_RADIOLIST(orientation_list),
	GNOMEUIINFO_END
};

static GnomeUIInfo popup_menu[] = {
	GNOMEUIINFO_SUBTREE(N_("Icon position"), position_menu),
	GNOMEUIINFO_SUBTREE(N_("Icon orientation"), orientation_menu),
	GNOMEUIINFO_SEPARATOR,
	GNOMEUIINFO_ITEM(N_("Tile children"), NULL, tile_callback, NULL),
	GNOMEUIINFO_ITEM(N_("Cascade children"), NULL, cascade_callback, NULL),
	GNOMEUIINFO_SEPARATOR,
	GNOMEUIINFO_ITEM(N_("Arrange icons"), NULL, arrange_callback, NULL),
	GNOMEUIINFO_TOGGLEITEM(N_("Autoarrange icons"), NULL, autoarrange_callback, NULL),
	GNOMEUIINFO_END
};

static void tile_callback(GtkWidget *w, gpointer user_data)
{
}

static void cascade_callback(GtkWidget *w, gpointer user_data)
{
}

static void arrange_callback(GtkWidget *w, gpointer user_data)
{
	gnome_pouch_arrange_icons(GNOME_POUCH(user_data));
}

static void autoarrange_callback(GtkWidget *w, gpointer user_data)
{
	GNOME_POUCH(user_data)->auto_arrange = GTK_CHECK_MENU_ITEM(w)->active;
}

static void orientation_callback(GtkWidget *w, gpointer user_data)
{
	GtkOrientation orient;
	GnomePouch *pouch = GNOME_POUCH(user_data);

	if(!GTK_CHECK_MENU_ITEM(w)->active)
		return;

	orient = (GtkOrientation)gtk_object_get_data(GTK_OBJECT(w),
												 GNOMEUIINFO_KEY_UIDATA);

	gnome_pouch_set_icon_arrangement(pouch, pouch->icon_corner, orient);
}

static void position_callback(GtkWidget *w, gpointer user_data)
{
	GtkCornerType corner;
	GnomePouch *pouch = GNOME_POUCH(user_data);

	if(!GTK_CHECK_MENU_ITEM(w)->active)
		return;

	corner = (GtkCornerType)gtk_object_get_data(GTK_OBJECT(w),
												GNOMEUIINFO_KEY_UIDATA);

	gnome_pouch_set_icon_arrangement(pouch, corner, pouch->icon_orientation);
}

/**
 * gnome_pouch_enable_popup_menu:
 * @enable: A gboolean specifying if the pouch pop-up menu should be enabled.
 *
 * Description:
 * When set to %TRUE, a right click on the pouch will pop up a menu with
 * some common options.
 **/
void gnome_pouch_enable_popup_menu(GnomePouch *pouch, gboolean enable)
{
	gint i;

	g_return_if_fail(pouch != NULL);
	g_return_if_fail(GNOME_IS_POUCH(pouch));

	if(enable && pouch->popup_menu == NULL) {
		pouch->popup_menu = gnome_popup_menu_new(popup_menu);
		for(i = 0; i < 2; i++)
			pouch->oitem[i] = GTK_CHECK_MENU_ITEM(orientation_list[i].widget);
		for(i = 0; i < 4; i++)
			pouch->citem[i] = GTK_CHECK_MENU_ITEM(position_list[i].widget);
		pouch->aitem = GTK_CHECK_MENU_ITEM(popup_menu[7].widget);
	}
	else if(!enable && pouch->popup_menu) {
		gtk_widget_destroy(pouch->popup_menu);
		pouch->popup_menu = NULL;
	}
}

/**
 * gnome_pouch_new:
 * 
 * Description:
 * Creates a new GnomePouch container.
 * 
 * Return value:
 * A pointer to a new GnomePouch widget.
 **/
GtkWidget *gnome_pouch_new()
{
	return gtk_type_new(gnome_pouch_get_type());
}
