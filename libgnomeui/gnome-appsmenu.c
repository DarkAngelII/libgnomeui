/* gnome-appsmenu.c: Copyright (C) 1998 Free Software Foundation
 * Written by: Havoc Pennington
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
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "gnome-appsmenu.h"
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include "libgnome/gnome-util.h"

/* Use a dotfile, so people won't try to edit manually
   unless they really know what they're doing. */
/* Alternatively, use "apps" to be consistent with share/apps? */

#define GNOME_APPS_MENU_DEFAULT_DIR ".Gnome Apps Menu"

typedef struct {
  gchar * extension;
  GnomeAppsMenuLoadFunc load_func;
  GnomeAppsMenuGtkMenuItemFunc menu_item_func;
} GnomeAppsMenuVariety;

/* If there are going to be more than a few varieties, this should be
   a GTree or something. But I don't think there will be, so it's not
   worth it. Easy to change later anyway.*/
static GList * file_varieties = NULL; /* list of above struct */
static GList * dir_varieties = NULL;  /* same */

static GnomeAppsMenuVariety * 
gnome_apps_menu_variety_new( const gchar * extension,
			     GnomeAppsMenuLoadFunc load_func,
			     GnomeAppsMenuGtkMenuItemFunc menu_item_func )
{
  GnomeAppsMenuVariety * v;

  g_return_val_if_fail(extension != NULL, NULL);
  g_return_val_if_fail(load_func != NULL, NULL);
  /* OK to have no menu_item_func */
  
  v = g_new(GnomeAppsMenuVariety, 1);
  
  v->extension = g_strdup(extension);
  v->load_func = load_func;
  v->menu_item_func = menu_item_func;

  return v;
}


void 
gnome_apps_menu_register_variety( gboolean is_directory,
				  const gchar * extension,
				  GnomeAppsMenuLoadFunc load_func,
				  GnomeAppsMenuGtkMenuItemFunc menu_item_func )
{
  GnomeAppsMenuVariety * v;
  GList * which;

  v = gnome_apps_menu_variety_new(extension, load_func, 
				  menu_item_func);

  if ( v == NULL ) {
    g_warning("AppsMenuVariety registration failed");
    return;
  }

  which = is_directory ? dir_varieties : file_varieties;

  g_list_append(which, v);
}


static void 
gnome_apps_menu_variety_destroy( GnomeAppsMenuVariety * v )
{
  g_return_if_fail ( v != NULL );
  
  g_free(v->extension);
  g_free(v);
}

static void clear_varieties(void)
{
  GnomeAppsMenuVariety * v;
  GList * first; 
  GList * varieties = file_varieties;

  while ( TRUE ) {    
    first = varieties;
    while ( varieties ) {
      v = (GnomeAppsMenuVariety *)varieties->data;
      gnome_apps_menu_variety_destroy(v);
      varieties = g_list_next(varieties);
    }
    g_list_free(first);

    if (file_varieties) {          /* Loop second time to clear
				      dir_varieties */
      varieties = dir_varieties;
      file_varieties = NULL;
    }
    else {
      dir_varieties = NULL;        /* done. */
      break;
    }
  }
}


static GtkWidget * gtk_menu_item_new_from_dentry(GnomeAppsMenu * gam)
{
  g_warning("not implemented");
  return NULL;
}

static void make_default_varieties(void)
{
  gnome_apps_menu_register_variety( FALSE, GNOME_APPS_MENU_DENTRY_EXTENSION,
				    gnome_desktop_entry_load,
				    gtk_menu_item_new_from_dentry );
  gnome_apps_menu_register_variety( TRUE, "directory",
				    gnome_desktop_entry_load,
				    gtk_menu_item_new_from_dentry );
}

static GnomeAppsMenuVariety * 
find_variety(GnomeAppsMenu * gam)
{
  GList * list;
  GnomeAppsMenuVariety * v;

  g_return_val_if_fail(gam != NULL, NULL);
  g_return_val_if_fail(gam->extension != NULL, NULL);

  list = GNOME_APPS_MENU_IS_DIR(gam) ? dir_varieties : file_varieties;
  
  while ( list ) {
    v = (GnomeAppsMenuVariety *)list->data;
    g_assert ( v != NULL );

    if ( strcmp (gam->extension, v->extension) == 0 ) {
      return v;
    }
    
    list = g_list_next(list);
  }

  return NULL; /* didn't find an appropriate extension */
}


static GnomeAppsMenuLoadFunc 
get_load_func_and_init_apps_menu( const gchar * filename,
				  GnomeAppsMenu * dest )
{
  GnomeAppsMenuVariety * v;
  gint len;
  gchar * extension = NULL;

  g_return_val_if_fail(filename != NULL, NULL);
  g_return_val_if_fail(dest != NULL, NULL);

  /* Extract extension from filename */

  len = strlen(filename);

  while ( len >= 0 ) {
    --len; /* always skip last character, because if
	      the filename ends in . it doesn't count. */
    if (filename[len] == '.') {
      extension = &filename[len + 1]; /* without period */
      break;
    }
  }
  
  if ( extension == NULL ) {
    g_warning("gnome-appsmenu: file %s has no extension", filename);
    return NULL;
  }

  /* Decide if it's a directory info file or not.  The above loop left
   filename[len] == '.', remember. */
  if ( len == 0 ) {
    dest->is_directory = TRUE; /* filename starts with . */
  }
  else if ( filename[len - 1] == '/' ) {
    dest->is_directory = TRUE;
  }
  else {
    dest->is_directory = FALSE;
  }

  /* set extension */
  dest->extension = g_strdup(extension);

  v = find_variety(dest);

  if ( v ) {
    /* return function. */
    return v->load_func;
  }
  else return NULL;
}

/* Function based on panel code, Miguel de Icaza and Federico Mena 
   (The lame parts are probably mine. :) */

GnomeAppsMenu * gnome_apps_menu_load(const gchar * directory)
{
  GnomeAppsMenu * root_apps_menu = NULL;
  struct dirent * dir_entry;
  DIR * dir;

  dir = opendir(directory);

  if (dir == NULL) {
    g_warning("gnome-appsmenu: error on directory %s: %s",
	      directory, g_unix_error_string(errno));
    return NULL;
  }

  /* FIXME for the moment, this loop assumes that the .directory
     dotfile will be encountered before other kinds of files.  Is this
     guaranteed or even likely? If it isn't I have to fix it. */

  while ( (dir_entry = readdir(dir)) != NULL) {
    GnomeAppsMenuLoadFunc load_func;
    gchar * thisfile;
    GnomeAppsMenu * sub_apps_menu;
    gchar * filename;
    struct stat s;

    thisfile = dir_entry->d_name;

    /* Skip . and .. */

    if ((thisfile[0] == '.' && thisfile[1] == '\0') ||
	(thisfile[0] == '.' && thisfile[1] == '.' && 
	 thisfile[2] == '\0')) {
      continue;
    }

    filename = g_concat_dir_and_file(directory, thisfile);

    if (stat (filename, &s) == -1) {
      g_free(filename);
      g_warning("gnome-appsmenu: error on file %s: %s\n", 
		filename, g_unix_error_string(errno));
      continue;
    }

    /* If it's a directory, recursively descend into it */
    if (S_ISDIR (s.st_mode)) {

      sub_apps_menu = gnome_apps_menu_load(filename);
      
      /* Recursive load failed for some reason. */
      if ( sub_apps_menu == NULL ) {
	g_free(filename);
	continue;
      }      
    }

    /* At this point, filename could be a directory dotfile 
       or a more standard menu item */
    else {
      sub_apps_menu = gnome_apps_menu_new();
      load_func = get_load_func_and_init_apps_menu( filename, 
						    sub_apps_menu ); 

      if ( load_func != NULL ) {
	sub_apps_menu->data = load_func(filename);
      }

      /* if load_func == NULL this will be true too, 
	 because of the default initialization of AppsMenu 
	 in _new() */
      /* This situation means the file type was unrecognized,
	 perhaps belonging to another app, so we want 
	 to ignore this file. */
      if ( sub_apps_menu->data == NULL ) {
	gnome_apps_menu_destroy(sub_apps_menu);
	sub_apps_menu = NULL;
	g_free(filename);
	continue;
      }

      /* Note two identical code blocks above and below. FIXME */

      if ( GNOME_APPS_MENU_IS_DIR(sub_apps_menu) ) {
	if (root_apps_menu) {
	  g_warning("Extra directory information file, %s, in directory %s",
		    filename, directory);
	  gnome_apps_menu_destroy(sub_apps_menu);
	  sub_apps_menu = NULL;
	  g_free(filename);
	  continue;
	}

	/* if there's no root menu yet, make this the root menu we're
           working in */
	root_apps_menu = sub_apps_menu;
	sub_apps_menu = NULL;
	
      } /* if it's a dir */
      else {
	if ( root_apps_menu ) {
	  gnome_apps_menu_append( root_apps_menu,
				  sub_apps_menu );
	}
	else {
	  g_warning("Didn't find a directory information file in directory %s",
		    directory);
	  g_free (filename);
	  break;
	}
      } /* else it's not a dir */
    } /* If it's an info file to load in */

    g_free(filename);
    
  } /* while (there are files in the directory) */

  closedir(dir);

  /* The panel code destroyed the directory menu if it was empty,
     may want to do that.

     if ( root_apps_menu->submenus == NULL ) {
     gnome_apps_menu_destroy(root_apps_menu);
     return NULL;
     }
     */

  /* Could be NULL, note */
  return root_apps_menu;
}


GnomeAppsMenu * gnome_apps_menu_load_default(void)
{
  gchar * s;
  GnomeAppsMenu * gam;

  s = gnome_util_home_file(GNOME_APPS_MENU_DEFAULT_DIR);
 
  gam = gnome_apps_menu_load(s);

  g_free(s);

  return gam;
}



GnomeAppsMenu * gnome_apps_menu_load_system(void)
{
  gchar * s;
  GnomeAppsMenu * gam;

  s = gnome_datadir_file("apps");
 
  gam = gnome_apps_menu_load(s);

  g_free(s);

  return gam;
}

GtkWidget * gtk_menu_item_new_from_directory(GnomeAppsMenu * gam)
{
  GtkWidget * menuitem;
  GtkWidget * submenu = NULL;
  GtkWidget * submenuitem = NULL;
  GList * list = NULL;
  
  menuitem = 
    gtk_menu_item_new_with_label(((GnomeDesktopEntry *)gam->data)->name );
  if (gam->submenus) {
    submenu = gtk_menu_new();
    gtk_menu_item_set_submenu ( GTK_MENU_ITEM(menuitem), submenu );
    list = gam->submenus;
    
    while (list) {
      submenuitem = 
	gtk_menu_item_new_from_apps_menu((GnomeAppsMenu *)list->data);
					
      gtk_menu_append( GTK_MENU(submenu), submenuitem );
      gtk_widget_show(submenuitem);
      list = g_list_next(list);
    }
    gtk_widget_show(submenu);
  }
  return menuitem; 
}

GnomeAppsMenuGtkMenuItemFunc
get_menu_item_func(GnomeAppsMenu * gam)
{
  GnomeAppsMenuVariety * v;

  g_return_val_if_fail(gam != NULL, NULL);
  g_assert(gam->extension != NULL);

  v = find_variety(gam);
  if (v) {
    return v->menu_item_func;
  }
  else return NULL;
}

GtkWidget * gtk_menu_item_new_from_apps_menu(GnomeAppsMenu * gam)
{
  GnomeAppsMenuGtkMenuItemFunc menu_item_func;

  g_return_val_if_fail(gam != NULL, NULL);

  menu_item_func = get_menu_item_func(gam);
  
  if (menu_item_func == NULL) {
    /* This kind of thing shouldn't show up on menus. */
    return NULL;
  }
  
  return menu_item_func(gam);
}

GtkWidget * gtk_menu_new_from_apps_menu(GnomeAppsMenu * gam)
{
  GtkWidget * root_menu;
  GtkWidget * menuitem;

  g_return_val_if_fail(gam != NULL, NULL);    

  root_menu = gtk_menu_new();

  menuitem = gtk_menu_item_new_from_apps_menu(gam);
  
  if (menuitem != NULL) {
    gtk_menu_append( GTK_MENU(root_menu), menuitem );
    gtk_widget_show(menuitem);
  }

  return root_menu;
}

/****************************************************
  Remaining functions are for manipulating the apps_menu
  trees. Don't relate to config files. 
  *******************************************/

GnomeAppsMenu * gnome_apps_menu_new(void)
{
  GnomeAppsMenu * gam = g_new(GnomeAppsMenu, 1);

  gam->extension = NULL;
  gam->data = NULL;
  gam->submenus = NULL;
  gam->in_destroy = FALSE;
  
  return gam;
}

/* GFunc for the g_list_foreach() call  */
static void gnome_apps_menu_destroy_one ( gpointer menu, 
					  gpointer data )
{
  g_return_if_fail ( menu != NULL );
  gnome_apps_menu_destroy((GnomeAppsMenu *)menu);
}

void gnome_apps_menu_destroy(GnomeAppsMenu * gam) 
{
  g_return_if_fail ( gam != NULL );

  gam->in_destroy = TRUE;

  if ( gam->parent ) {
    gnome_apps_menu_remove(gam->parent, gam);
  }

  g_free ( gam->extension ); gam->extension = NULL;
  g_free ( gam->data ); gam->extension = NULL;

  if ( gam->submenus ) {
    g_list_foreach ( gam->submenus,
		     gnome_apps_menu_destroy_one, NULL );

    g_list_free ( gam->submenus );
    gam->submenus = NULL;
  }
  
  g_free (gam);
}

void gnome_apps_menu_append(GnomeAppsMenu * dir,
			    GnomeAppsMenu * sub)
{
  g_return_if_fail ( dir != NULL );
  g_return_if_fail ( sub != NULL );
  g_return_if_fail ( GNOME_APPS_MENU_IS_DIR(dir) );
  g_return_if_fail ( sub->parent == NULL );
  g_return_if_fail ( ! dir->in_destroy );
  g_return_if_fail ( ! sub->in_destroy );
  
  sub->parent = dir;

  dir->submenus = 
    g_list_append( dir->submenus,
		   sub );
}


void gnome_apps_menu_prepend(GnomeAppsMenu * dir,
			     GnomeAppsMenu * sub)
{
  g_return_if_fail ( dir != NULL );
  g_return_if_fail ( sub != NULL );
  g_return_if_fail ( GNOME_APPS_MENU_IS_DIR(dir) );
  g_return_if_fail ( sub->parent == NULL );
  g_return_if_fail ( ! dir->in_destroy );
  g_return_if_fail ( ! sub->in_destroy );
  
  sub->parent = dir;

  dir->submenus = 
    g_list_prepend( dir->submenus,
		    sub );
}

void gnome_apps_menu_remove(GnomeAppsMenu * dir,
			    GnomeAppsMenu * sub)
{
  g_return_if_fail ( dir != NULL );
  g_return_if_fail ( sub != NULL );
  g_return_if_fail ( dir->submenus != NULL );

  /* If the parent folder is being destroyed, 
     we don't want to bother removing this item from it.
     In fact, doing so might cause weird stuff to happen.
     */

  if ( ! dir->in_destroy ) {
     dir->submenus = 
       g_list_remove ( dir->submenus,
		       sub );
  }

  /* Set submenu's parent to NULL */
  sub->parent = NULL;
}

void gnome_apps_menu_insert_after(GnomeAppsMenu * dir,
				  GnomeAppsMenu * sub,
				  GnomeAppsMenu * after_me)
{
  GList * where;

  /* OK, a little too defensive. :) 
     These are no-ops if debugging is off, right? */
  g_return_if_fail ( dir != NULL );
  g_return_if_fail ( sub != NULL );
  g_return_if_fail ( after_me != NULL );
  g_return_if_fail ( GNOME_APPS_MENU_IS_DIR(dir) );
  g_return_if_fail ( sub->parent == NULL );
  g_return_if_fail ( after_me->parent == dir->parent );
  g_return_if_fail ( ! dir->in_destroy );
  g_return_if_fail ( ! sub->in_destroy );
  g_return_if_fail ( ! after_me->in_destroy );

  where = g_list_find(dir->submenus, after_me);
  
  if ( where == NULL ) {
    g_warning("Attempt to insert after nonexistent folder!");
    return;
  }

  sub->parent = dir;
 
  /* after_me should be at 0 in `where', so we put our menu at 1 */
  g_list_insert(where, sub, 1);
}

void gnome_apps_menu_foreach(GnomeAppsMenu * dir,
			     GFunc func, gpointer data)
{
  GList * submenus;
  GnomeAppsMenu * gam;
  
  g_return_if_fail ( dir != NULL );
  g_return_if_fail ( func != NULL );

  /* Note that dir doesn't really have to be a directory,
     and on most recursive calls it isn't, but there's no 
     real reason why you'd want to call the function
     on a non-directory, of course. */
  
  (* func)(dir, data);

  submenus = dir->submenus;

  while (submenus) {
    gam = (GnomeAppsMenu *)submenus->data;
    
    /* Recursively descend */
    if ( gam->submenus ) {
      gnome_apps_menu_foreach(gam, func, data);
    }

    submenus = g_list_next(submenus);
  }
}
    
