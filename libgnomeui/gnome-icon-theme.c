/* GnomeIconThemes - a loader for icon-themes
 * gnome-icon-theme.c Copyright (C) 2002 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <gconf/gconf-client.h>

#include "gnome-icon-theme.h"
#include "gnome-theme-parser.h"

typedef enum
{
  ICON_THEME_DIR_FIXED,  
  ICON_THEME_DIR_SCALABLE,  
  ICON_THEME_DIR_THRESHOLD,  
} IconThemeDirType;

/* In reverse search order: */
typedef enum
{
  ICON_SUFFIX_NONE = 0,
  ICON_SUFFIX_XPM,
  ICON_SUFFIX_SVG,
  ICON_SUFFIX_PNG,  
} IconSuffix;


struct _GnomeIconThemePrivate
{
  gboolean custom_theme;
  char *current_theme;
  char **search_path;
  int search_path_len;

  gboolean allow_svg;
  
  gboolean themes_valid;
  /* A list of all the themes needed to look up icons.
   * In search order, without duplicates
   */
  GList *themes;
  GHashTable *unthemed_icons;

  /* Note: The keys of this hashtable are owned by the
   * themedir and unthemed hashtables.
   */
  GHashTable *all_icons;
  
  /* GConf data: */
  guint theme_changed_id;

  /* time when we last stat:ed for theme changes */
  long last_stat_time;
  GList *dir_mtimes;
};

typedef struct
{
  char *name;
  char *display_name;
  char *comment;
  char *example;

  /* In search order */
  GList *dirs;
} IconTheme;

typedef struct
{
  IconThemeDirType type;
  GQuark context;

  int size;
  int min_size;
  int max_size;
  int threshold;

  char *dir;
  
  GHashTable *icons;
  GHashTable *icon_data;
} IconThemeDir;

typedef struct 
{
  char *dir;
  time_t mtime; /* 0 == not existing or not a dir */
} IconThemeDirMtime;

static void   gnome_icon_theme_class_init (GnomeIconThemeClass *klass);
static void   gnome_icon_theme_init       (GnomeIconTheme      *icon_theme);
static void   gnome_icon_theme_finalize   (GObject              *object);
static void   theme_destroy                (IconTheme            *theme);
static void   theme_dir_destroy            (IconThemeDir         *dir);
static char * theme_lookup_icon            (IconTheme            *theme,
					    const char           *icon_name,
					    int                   size,
					    const GnomeIconData **icon_data,
					    int                  *base_size);
static void   theme_list_icons             (IconTheme            *theme,
					    GHashTable           *icons,
					    GQuark                context);
static void   theme_subdir_load            (GnomeIconTheme      *icon_theme,
					    IconTheme            *theme,
					    GnomeThemeFile       *theme_file,
					    char                 *subdir);
static void   blow_themes                  (GnomeIconThemePrivate *priv);

static IconSuffix suffix_from_name         (const char           *name);


static guint		 signal_changed = 0;


GType
gnome_icon_theme_get_type (void)
{
  static GType type = 0;

  if (type == 0)
    {
      static const GTypeInfo info =
	{
	  sizeof (GnomeIconThemeClass),
	  NULL,           /* base_init */
	  NULL,           /* base_finalize */
	  (GClassInitFunc) gnome_icon_theme_class_init,
	  NULL,           /* class_finalize */
	  NULL,           /* class_data */
	  sizeof (GnomeIconTheme),
	  0,              /* n_preallocs */
	  (GInstanceInitFunc) gnome_icon_theme_init,
	};

      type = g_type_register_static (G_TYPE_OBJECT, "GnomeIconTheme", &info, 0);
    }

  return type;
}

GnomeIconTheme *
gnome_icon_theme_new (void)
{
  return g_object_new (GNOME_TYPE_ICON_THEME, NULL);
}

static void
gnome_icon_theme_class_init (GnomeIconThemeClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = gnome_icon_theme_finalize;

  signal_changed = g_signal_new ("changed",
				 G_TYPE_FROM_CLASS (klass),
				 G_SIGNAL_RUN_LAST,
				 G_STRUCT_OFFSET (GnomeIconThemeClass, changed),
				 NULL, NULL,
				 g_cclosure_marshal_VOID__VOID,
				 G_TYPE_NONE, 0);
}

static void
theme_changed (GConfClient* client,
	       guint cnxn_id,
	       GConfEntry *entry,
	       gpointer user_data)
{
  GnomeIconTheme *icon_theme = user_data;
  GnomeIconThemePrivate *priv = icon_theme->priv;
  const char *str;
  

  if (!entry->value || entry->value->type != GCONF_VALUE_STRING)
    return;

  str = gconf_value_get_string (entry->value);

  g_free (priv->current_theme);
  priv->current_theme = g_strdup (str);

  blow_themes (priv);
  g_signal_emit (G_OBJECT (icon_theme), signal_changed, 0);
}

static void
setup_gconf_handler (GnomeIconTheme *icon_theme)
{
  GnomeIconThemePrivate *priv;
  GConfClient *client;
  char *theme;
  
  priv = icon_theme->priv;

  client = gconf_client_get_default ();

  gconf_client_add_dir (client,
			"/desktop/gnome/interface",
			GCONF_CLIENT_PRELOAD_NONE,
			NULL);

  g_assert (priv->theme_changed_id == 0);
  
  priv->theme_changed_id = gconf_client_notify_add (client,
						    "/desktop/gnome/interface/icon_theme",
						    theme_changed,
						    icon_theme, NULL, NULL);

  theme = gconf_client_get_string (client,
				   "/desktop/gnome/interface/icon_theme",
				   NULL);
  
  if (theme)
    {
      g_free (priv->current_theme);
      priv->current_theme = theme;
    }

  g_object_unref (client);
}

static void
remove_gconf_handler (GnomeIconTheme *icon_theme)
{
  GnomeIconThemePrivate *priv;
  GConfClient *client;
  
  priv = icon_theme->priv;

  g_assert (priv->theme_changed_id != 0);

  client = gconf_client_get_default ();
  
  gconf_client_notify_remove (client, priv->theme_changed_id);
  priv->theme_changed_id = 0;

  g_object_unref (client);
}

static void
gnome_icon_theme_init (GnomeIconTheme *icon_theme)
{
  GnomeIconThemePrivate *priv;

  priv = g_new0 (GnomeIconThemePrivate, 1);
  
  icon_theme->priv = priv;

  priv->custom_theme = FALSE;
  priv->current_theme = g_strdup ("default");
  priv->search_path = g_new (char *, 5);
  

  priv->search_path[0] = g_build_filename (g_get_home_dir (),
					     ".icons",
					     NULL);
  priv->search_path[1] = g_strdup (GNOMEUIPIXMAPDIR);
  priv->search_path[2] = g_strdup (GNOMEUIICONDIR);
  priv->search_path[3] = g_strdup ("/usr/share/icons");
  priv->search_path[4] = g_strdup ("/usr/share/pixmaps");
  priv->search_path_len = 5;

  priv->themes_valid = FALSE;
  priv->themes = NULL;
  priv->unthemed_icons = NULL;

  priv->allow_svg = FALSE;
  
  setup_gconf_handler (icon_theme);
}

static void
free_dir_mtime (IconThemeDirMtime *dir_mtime)
{
  g_free (dir_mtime->dir);
  g_free (dir_mtime);
}

static void
blow_themes (GnomeIconThemePrivate *priv)
{
  if (priv->themes_valid)
    {
      g_hash_table_destroy (priv->all_icons);
      g_list_foreach (priv->themes, (GFunc)theme_destroy, NULL);
      g_list_free (priv->themes);
      g_list_foreach (priv->dir_mtimes, (GFunc)free_dir_mtime, NULL);
      g_list_free (priv->dir_mtimes);
      g_hash_table_destroy (priv->unthemed_icons);
    }
  priv->themes = NULL;
  priv->unthemed_icons = NULL;
  priv->dir_mtimes = NULL;
  priv->all_icons = NULL;
  priv->themes_valid = FALSE;
}

static void
gnome_icon_theme_finalize (GObject *object)
{
  GnomeIconTheme *icon_theme;
  GnomeIconThemePrivate *priv;
  int i;

  icon_theme = GNOME_ICON_THEME (object);
  priv = icon_theme->priv;

  g_free (priv->current_theme);
  priv->current_theme = NULL;

  for (i=0; i < priv->search_path_len; i++)
    g_free (priv->search_path[i]);

  g_free (priv->search_path);
  priv->search_path = NULL;

  if (priv->theme_changed_id)
    remove_gconf_handler (icon_theme);

  blow_themes (priv);

  g_free (priv);
}

void
gnome_icon_theme_set_allow_svg (GnomeIconTheme      *icon_theme,
				 gboolean              allow_svg)
{
  allow_svg = allow_svg != FALSE;

  if (allow_svg == icon_theme->priv->allow_svg)
    return;
  
  icon_theme->priv->allow_svg = allow_svg;
  
  blow_themes (icon_theme->priv);
  
  g_signal_emit (G_OBJECT (icon_theme), signal_changed, 0);
}

gboolean
gnome_icon_theme_get_allow_svg (GnomeIconTheme *icon_theme)
{
  return icon_theme->priv->allow_svg;
}

void
gnome_icon_theme_set_search_path (GnomeIconTheme *icon_theme,
				   const char *path[],
				   int         n_elements)
{
  GnomeIconThemePrivate *priv;
  int i;

  priv = icon_theme->priv;
  for (i = 0; i < priv->search_path_len; i++)
    g_free (priv->search_path[i]);

  g_free (priv->search_path);

  priv->search_path = g_new (char *, n_elements);
  priv->search_path_len = n_elements;
  for (i = 0; i < priv->search_path_len; i++)
    priv->search_path[i] = g_strdup (path[i]);

  blow_themes (priv);
  g_signal_emit (G_OBJECT (icon_theme), signal_changed, 0);
}


void
gnome_icon_theme_get_search_path (GnomeIconTheme      *icon_theme,
				   char                 **path[],
				   int                   *n_elements)
{
  GnomeIconThemePrivate *priv;
  int i;

  priv = icon_theme->priv;
  
  *path = g_new (char *, priv->search_path_len);
  *n_elements = priv->search_path_len;
  
  for (i = 0; i < priv->search_path_len; i++)
    (*path)[i] = g_strdup (priv->search_path[i]);
}

void
gnome_icon_theme_append_search_path (GnomeIconTheme      *icon_theme,
				      const char           *path)
{
  GnomeIconThemePrivate *priv;

  priv = icon_theme->priv;
  
  priv->search_path_len++;
  priv->search_path = g_realloc (priv->search_path, priv->search_path_len * sizeof(char *));
  priv->search_path[priv->search_path_len-1] = g_strdup (path);

  blow_themes (priv);
  g_signal_emit (G_OBJECT (icon_theme), signal_changed, 0);
}

void
gnome_icon_theme_prepend_search_path (GnomeIconTheme      *icon_theme,
				       const char           *path)
{
  GnomeIconThemePrivate *priv;
  int i;

  priv = icon_theme->priv;
  
  priv->search_path_len++;
  priv->search_path = g_realloc (priv->search_path, priv->search_path_len * sizeof(char *));

  for (i = 0; i < priv->search_path_len - 1; i++)
    {
      priv->search_path[i+1] = priv->search_path[i];
    }
  
  priv->search_path[0] = g_strdup (path);

  blow_themes (priv);
  g_signal_emit (G_OBJECT (icon_theme), signal_changed, 0);
}

void
gnome_icon_theme_set_custom_theme (GnomeIconTheme *icon_theme,
				    const char *theme_name)
{
  GnomeIconThemePrivate *priv;

  priv = icon_theme->priv;
  g_free (priv->current_theme);
  if (theme_name != NULL)
    {
      priv->current_theme = g_strdup (theme_name);
      priv->custom_theme = TRUE;
      remove_gconf_handler (icon_theme);
    }
  else
    {
      priv->custom_theme = FALSE;
      setup_gconf_handler (icon_theme);
    }

  blow_themes (priv);
  g_signal_emit (G_OBJECT (icon_theme), signal_changed, 0);
}

static void
insert_theme (GnomeIconTheme *icon_theme, const char *theme_name)
{
  int i;
  GList *l;
  char **dirs;
  char **themes;
  GnomeIconThemePrivate *priv;
  IconTheme *theme;
  char *path;
  char *contents;
  char *directories;
  char *inherits;
  GnomeThemeFile *theme_file;
  IconThemeDirMtime *dir_mtime;
  struct stat stat_buf;
  
  priv = icon_theme->priv;
  
  for (l = priv->themes; l != NULL; l = l->next)
    {
      theme = l->data;
      if (strcmp (theme->name, theme_name) == 0)
	return;
    }
  
  for (i = 0; i < priv->search_path_len; i++)
    {
      path = g_build_filename (priv->search_path[i],
			       theme_name,
			       NULL);
      dir_mtime = g_new (IconThemeDirMtime, 1);
      dir_mtime->dir = path;
      if (stat (path, &stat_buf) == 0 && S_ISDIR (stat_buf.st_mode))
	dir_mtime->mtime = stat_buf.st_mtime;
      else
	dir_mtime->mtime = 0;

      priv->dir_mtimes = g_list_prepend (priv->dir_mtimes, dir_mtime);
    }

  theme_file = NULL;
  for (i = 0; i < priv->search_path_len; i++)
    {
      path = g_build_filename (priv->search_path[i],
			       theme_name,
			       "index.theme",
			       NULL);
      if (g_file_test (path, G_FILE_TEST_IS_REGULAR)) {
	if (g_file_get_contents (path, &contents, NULL, NULL)) {
	  theme_file = gnome_theme_file_new_from_string (contents, NULL);
	  g_free (contents);
	  g_free (path);
	  break;
	}
      }
      g_free (path);
    }

  if (theme_file == NULL)
    return;
  
  theme = g_new (IconTheme, 1);
  if (!gnome_theme_file_get_locale_string (theme_file,
					   "Icon Theme",
					   "Name",
					   &theme->display_name))
    {
      g_warning ("Theme file for %s has no name\n", theme_name);
      g_free (theme);
      gnome_theme_file_free (theme_file);
      return;
    }

  if (!gnome_theme_file_get_string (theme_file,
				    "Icon Theme",
				    "Directories",
				    &directories))
    {
      g_warning ("Theme file for %s has no directories\n", theme_name);
      g_free (theme->display_name);
      g_free (theme);
      gnome_theme_file_free (theme_file);
      return;
    }
  
  theme->name = g_strdup (theme_name);
  gnome_theme_file_get_locale_string (theme_file,
				      "Icon Theme",
				      "Comment",
				      &theme->comment);
  gnome_theme_file_get_string (theme_file,
			       "Icon Theme",
			       "Example",
			       &theme->example);
  
  dirs = g_strsplit (directories, ",", 0);

  theme->dirs = NULL;
  for (i = 0; dirs[i] != NULL; i++)
      theme_subdir_load (icon_theme, theme, theme_file, dirs[i]);
  
  g_strfreev (dirs);
  
  theme->dirs = g_list_reverse (theme->dirs);

  g_free (directories);

  /* Prepend the finished theme */
  priv->themes = g_list_prepend (priv->themes, theme);

  if (gnome_theme_file_get_string (theme_file,
				   "Icon Theme",
				   "Inherits",
				   &inherits))
    {
      themes = g_strsplit (inherits, ",", 0);

      for (i = 0; themes[i] != NULL; i++)
	insert_theme (icon_theme, themes[i]);
      
      g_strfreev (themes);

      g_free (inherits);
    }

  gnome_theme_file_free (theme_file);
}

static gboolean
my_g_str_has_suffix (const gchar  *str,
		     const gchar  *suffix)
{
  int str_len;
  int suffix_len;
  
  g_return_val_if_fail (str != NULL, FALSE);
  g_return_val_if_fail (suffix != NULL, FALSE);

  str_len = strlen (str);
  suffix_len = strlen (suffix);

  if (str_len < suffix_len)
    return FALSE;

  return strcmp (str + str_len - suffix_len, suffix) == 0;
}

static void
load_themes (GnomeIconTheme *icon_theme)
{
  GnomeIconThemePrivate *priv;
  GDir *gdir;
  int base;
  char *dir, *base_name, *dot;
  const char *file;
  char *abs_file;
  char *old_file;
  IconSuffix old_suffix, new_suffix;
  struct timeval tv;
  
  priv = icon_theme->priv;

  priv->all_icons = g_hash_table_new (g_str_hash, g_str_equal);
  
  insert_theme (icon_theme, priv->current_theme);
  /* Always look in the "default" icon theme */
  insert_theme (icon_theme, "default");
  priv->themes = g_list_reverse (priv->themes);
  
  priv->unthemed_icons = g_hash_table_new_full (g_str_hash, g_str_equal,
						g_free, g_free);

  for (base = 0; base < icon_theme->priv->search_path_len; base++)
    {
      dir = icon_theme->priv->search_path[base];
      gdir = g_dir_open (dir, 0, NULL);

      if (gdir == NULL)
	continue;
      
      while ((file = g_dir_read_name (gdir)))
	{
	  if (my_g_str_has_suffix (file, ".png") ||
	      (priv->allow_svg && my_g_str_has_suffix (file, ".svg")) ||
	      my_g_str_has_suffix (file, ".xpm"))
	    {
	      abs_file = g_build_filename (dir, file, NULL);

	      base_name = g_strdup (file);
		  
	      dot = strrchr (base_name, '.');
	      if (dot)
		*dot = 0;

	      if ((old_file = g_hash_table_lookup (priv->unthemed_icons,
						   base_name)))
		{
		  old_suffix = suffix_from_name (old_file);
		  new_suffix = suffix_from_name (file);

		  if (new_suffix > old_suffix)
		    g_hash_table_replace (priv->unthemed_icons,
					  base_name,
					  abs_file);
		  else
		    {
		      g_free (base_name);
		      g_free (abs_file);
		    }
		}
	      else
		{
		  g_hash_table_insert (priv->unthemed_icons,
				       base_name,
				       abs_file);
		  g_hash_table_insert (priv->all_icons,
				       base_name, NULL);
		}
	    }
	}
      g_dir_close (gdir);
    }

  priv->themes_valid = TRUE;
  
  gettimeofday(&tv, NULL);
  priv->last_stat_time = tv.tv_sec;
}

static void
ensure_valid_themes (GnomeIconTheme *icon_theme)
{
  GnomeIconThemePrivate *priv = icon_theme->priv;
  struct timeval tv;
  
  if (priv->themes_valid)
    {
      gettimeofday(&tv, NULL);

      if (ABS (tv.tv_sec - priv->last_stat_time) > 5)
	gnome_icon_theme_rescan_if_needed (icon_theme);
    }
  
  if (!priv->themes_valid)
    load_themes (icon_theme);
}

char *
gnome_icon_theme_lookup_icon (GnomeIconTheme      *icon_theme,
			      const char           *icon_name,
			      int                   size,
			      const GnomeIconData **icon_data,
			      int                  *base_size)
{
  GnomeIconThemePrivate *priv;
  GList *l;
  char *icon;
  
  priv = icon_theme->priv;

  ensure_valid_themes (icon_theme);

  if (icon_data)
    *icon_data = NULL;
  
  l = priv->themes;
  while (l != NULL)
    {
      icon = theme_lookup_icon (l->data, icon_name, size, icon_data, base_size);
      if (icon)
	return icon;
      
      l = l->next;
    }

  icon = g_hash_table_lookup (priv->unthemed_icons, icon_name);
  if (icon)
    {
      if (base_size)
	*base_size = 0;

      return g_strdup (icon);
    }

  return NULL;
}

gboolean 
gnome_icon_theme_has_icon (GnomeIconTheme      *icon_theme,
			    const char           *icon_name)
{
  GnomeIconThemePrivate *priv;
  
  priv = icon_theme->priv;
  
  ensure_valid_themes (icon_theme);

  return g_hash_table_lookup_extended (priv->all_icons,
				       icon_name, NULL, NULL);
}


static void
add_key_to_hash (gpointer  key,
		 gpointer  value,
		 gpointer  user_data)
{
  GHashTable *hash = user_data;

  g_hash_table_insert (hash, key, NULL);
}

static void
add_key_to_list (gpointer  key,
		 gpointer  value,
		 gpointer  user_data)
{
  GList **list = user_data;

  *list = g_list_prepend (*list, g_strdup (key));
}

GList *
gnome_icon_theme_list_icons (GnomeIconTheme *icon_theme,
			      const char *context)
{
  GnomeIconThemePrivate *priv;
  GHashTable *icons;
  GList *list, *l;
  GQuark context_quark;
  
  priv = icon_theme->priv;
  
  ensure_valid_themes (icon_theme);

  if (context)
    {
      context_quark = g_quark_try_string (context);

      if (!context_quark)
	return NULL;
    }
  else
    context_quark = 0;

  icons = g_hash_table_new (g_str_hash, g_str_equal);
  
  l = priv->themes;
  while (l != NULL)
    {
      theme_list_icons (l->data, icons, context_quark);
      l = l->next;
    }

  if (context_quark == 0)
    g_hash_table_foreach (priv->unthemed_icons,
			  add_key_to_hash,
			  icons);

  list = 0;
  
  g_hash_table_foreach (icons,
			add_key_to_list,
			&list);

  g_hash_table_destroy (icons);
  
  return list;
}

char *
gnome_icon_theme_get_example_icon_name (GnomeIconTheme *icon_theme)
{
  GnomeIconThemePrivate *priv;
  GList *l;
  IconTheme *theme;
  
  priv = icon_theme->priv;
  
  ensure_valid_themes (icon_theme);

  l = priv->themes;
  while (l != NULL)
    {
      theme = l->data;
      if (theme->example)
	return g_strdup (theme->example);
      
      l = l->next;
    }
  
  return NULL;
}

gboolean
gnome_icon_theme_rescan_if_needed (GnomeIconTheme *icon_theme)
{
  GnomeIconThemePrivate *priv;
  IconThemeDirMtime *dir_mtime;
  char *path;
  GList *d;
  int stat_res;
  struct stat stat_buf;
  struct timeval tv;

  priv = icon_theme->priv;
  
  for (d = priv->dir_mtimes; d != NULL; d = d->next)
    {
      dir_mtime = d->data;

      stat_res = stat (dir_mtime->dir, &stat_buf);

      /* dir mtime didn't change */
      if (stat_res == 0 && 
	  S_ISDIR (stat_buf.st_mode) &&
	  dir_mtime->mtime == stat_buf.st_mtime)
	continue;
      /* didn't exist before, and still doesn't */
      if (dir_mtime->mtime == 0 &&
	  (stat_res != 0 || !S_ISDIR (stat_buf.st_mode)))
	continue;
	  
      blow_themes(icon_theme->priv);
      g_signal_emit (G_OBJECT (icon_theme), signal_changed, 0);
      return TRUE;
    }
  
  gettimeofday (&tv, NULL);
  priv->last_stat_time = tv.tv_sec;

  return FALSE;
}

static void
theme_destroy (IconTheme *theme)
{
  g_free (theme->display_name);
  g_free (theme->comment);
  g_free (theme->name);
  g_free (theme->example);

  g_list_foreach (theme->dirs, (GFunc)theme_dir_destroy, NULL);
  g_list_free (theme->dirs);
  g_free (theme);
}

static void
theme_dir_destroy (IconThemeDir *dir)
{
  g_hash_table_destroy (dir->icons);
  if (dir->icon_data)
    g_hash_table_destroy (dir->icon_data);
  g_free (dir->dir);
  g_free (dir);
}

static int
theme_dir_size_difference (IconThemeDir *dir, int size)
{
  int min, max;
  switch (dir->type)
    {
    case ICON_THEME_DIR_FIXED:
      return abs (size - dir->size);
      break;
    case ICON_THEME_DIR_SCALABLE:
      if (size < dir->min_size)
	return dir->min_size - size;
      if (size > dir->max_size)
	return size - dir->max_size;
      return 0;
      break;
    case ICON_THEME_DIR_THRESHOLD:
      min = dir->size - dir->threshold;
      max = dir->size + dir->threshold;
      if (size < min)
	return min - size;
      if (size > max)
	return size - max;
      return 0;
      break;
    }
  g_assert_not_reached ();
  return 1000;
}

static const char *
string_from_suffix (IconSuffix suffix)
{
  switch (suffix)
    {
    case ICON_SUFFIX_XPM:
      return ".xpm";
    case ICON_SUFFIX_SVG:
      return ".svg";
    case ICON_SUFFIX_PNG:
      return ".png";
    default:
      g_assert_not_reached();
    }
  return NULL;
}

static IconSuffix
suffix_from_name (const char *name)
{
  IconSuffix retval;

  if (my_g_str_has_suffix (name, ".png"))
    retval = ICON_SUFFIX_PNG;
  else if (my_g_str_has_suffix (name, ".svg"))
    retval = ICON_SUFFIX_SVG;
  else if (my_g_str_has_suffix (name, ".xpm"))
    retval = ICON_SUFFIX_XPM;
  else
    retval = ICON_SUFFIX_NONE;

  return retval;
}

static char *
theme_lookup_icon (IconTheme *theme,
		   const char *icon_name,
		   int size,
		   const GnomeIconData **icon_data,
		   int *base_size)
{
  GList *l;
  IconThemeDir *dir, *min_dir;
  char *file, *absolute_file;
  int min_difference, difference;
  IconSuffix suffix;

  l = theme->dirs;
  while (l != NULL)
    {
      dir = l->data;

      if (theme_dir_size_difference (dir, size) == 0)
	{
	  suffix = GPOINTER_TO_INT (g_hash_table_lookup (dir->icons, icon_name));
	  if (suffix != ICON_SUFFIX_NONE) {
	    file = g_strconcat (icon_name, string_from_suffix (suffix), NULL);
	    absolute_file = g_build_filename (dir->dir, file, NULL);
	    g_free (file);

	    if (icon_data && dir->icon_data != NULL)
	      *icon_data = g_hash_table_lookup (dir->icon_data, icon_name);

	    if (base_size)
	      *base_size = dir->size;
	    
	    return absolute_file;
	  }
	}
      
      l = l->next;
    }

  min_difference = G_MAXINT;
  min_dir = NULL;
  l = theme->dirs;
  while (l != NULL)
    {
      dir = l->data;

      difference = theme_dir_size_difference (dir, size);
      if (difference < min_difference &&
	  g_hash_table_lookup (dir->icons, icon_name) != ICON_SUFFIX_NONE)
	{
	  min_difference = difference;
	  min_dir = dir;
	}
      
      l = l->next;
    }

  if (min_dir)
    {
      suffix = GPOINTER_TO_INT (g_hash_table_lookup (min_dir->icons, icon_name));
      file = g_strconcat (icon_name, string_from_suffix (suffix), NULL);
      absolute_file = g_build_filename (min_dir->dir, file, NULL);
      g_free (file);

      if (icon_data && min_dir->icon_data != NULL)
	*icon_data = g_hash_table_lookup (min_dir->icon_data, icon_name);
	    
      if (base_size)
	*base_size = min_dir->size;
      
      return absolute_file;
    }
 
  return NULL;
}

static void
theme_list_icons (IconTheme *theme, GHashTable *icons,
		  GQuark context)
{
  GList *l = theme->dirs;
  IconThemeDir *dir;
  
  while (l != NULL)
    {
      dir = l->data;

      if (context == dir->context ||
	  context == 0)
	g_hash_table_foreach (dir->icons,
			      add_key_to_hash,
			      icons);

      l = l->next;
    }
}

static void
load_icon_data (IconThemeDir *dir, const char *path, const char *name)
{
  GnomeThemeFile *icon_file;
  char *base_name;
  char **split;
  char *contents;
  char *dot;
  char *str;
  char *split_point;
  int i;
  
  GnomeIconData *data;

  if (g_file_get_contents (path, &contents, NULL, NULL))
    {
      icon_file = gnome_theme_file_new_from_string (contents, NULL);
      
      if (icon_file)
	{
	  base_name = g_strdup (name);
	  dot = strrchr (base_name, '.');
	  *dot = 0;
	  
	  data = g_new0 (GnomeIconData, 1);
	  g_hash_table_replace (dir->icon_data, base_name, data);
	  
	  if (gnome_theme_file_get_string (icon_file, "Icon Data",
					   "EmbeddedTextRectangle",
					   &str))
	    {
	      split = g_strsplit (str, ",", 4);
	      
	      i = 0;
	      while (split[i] != NULL)
		i++;

	      if (i==4)
		{
		  data->has_embedded_rect = TRUE;
		  data->x0 = atoi (split[0]);
		  data->y0 = atoi (split[1]);
		  data->x1 = atoi (split[2]);
		  data->y1 = atoi (split[3]);
		}

	      g_strfreev (split);
	      g_free (str);
	    }


	  if (gnome_theme_file_get_string (icon_file, "Icon Data",
					   "AttachPoints",
					   &str))
	    {
	      split = g_strsplit (str, "|", -1);
	      
	      i = 0;
	      while (split[i] != NULL)
		i++;

	      data->n_attach_points = i;
	      data->attach_points = g_malloc (sizeof (GnomeIconDataPoint) * i);

	      i = 0;
	      while (split[i] != NULL && i < data->n_attach_points)
		{
		  split_point = strchr (split[i], ',');
		  if (split_point)
		    {
		      *split_point = 0;
		      split_point++;
		      data->attach_points[i].x = atoi (split[i]);
		      data->attach_points[i].y = atoi (split_point);
		    }
		  i++;
		}
	      
	      g_strfreev (split);
	      g_free (str);
	    }
	  
	  gnome_theme_file_get_locale_string (icon_file, "Icon Data",
					      "DisplayName",
					      &data->display_name);
	  
	  gnome_theme_file_free (icon_file);
	}
      g_free (contents);
    }
  
}

static void
scan_directory (GnomeIconThemePrivate *icon_theme,
		IconThemeDir *dir, char *full_dir, gboolean allow_svg)
{
  GDir *gdir;
  const char *name;
  char *base_name, *dot;
  char *path;
  IconSuffix suffix, hash_suffix;
  
  dir->icons = g_hash_table_new_full (g_str_hash, g_str_equal,
				      g_free, NULL);
  
  gdir = g_dir_open (full_dir, 0, NULL);

  if (gdir == NULL)
    return;

  while ((name = g_dir_read_name (gdir)))
    {
      if (my_g_str_has_suffix (name, ".icon"))
	{
	  if (dir->icon_data == NULL)
	    dir->icon_data = g_hash_table_new_full (g_str_hash, g_str_equal,
						    g_free, (GDestroyNotify)gnome_icon_data_free);
	  
	  path = g_build_filename (full_dir, name, NULL);
	  if (g_file_test (path, G_FILE_TEST_IS_REGULAR))
	    load_icon_data (dir, path, name);
	  
	  g_free (path);
	  
	  continue;
	}

      suffix = suffix_from_name (name);
      if (suffix == ICON_SUFFIX_NONE ||
	  (suffix == ICON_SUFFIX_SVG && !allow_svg))
	continue;
      
      base_name = g_strdup (name);
      dot = strrchr (base_name, '.');
      *dot = 0;
      
      hash_suffix = GPOINTER_TO_INT (g_hash_table_lookup (dir->icons, base_name));

      if (suffix > hash_suffix)
	g_hash_table_replace (dir->icons, base_name, GINT_TO_POINTER (suffix));
      else
	g_free (base_name);
      g_hash_table_insert (icon_theme->all_icons, base_name, NULL);
    }
  
  g_dir_close (gdir);
}

static void
theme_subdir_load (GnomeIconTheme *icon_theme,
		   IconTheme *theme,
		   GnomeThemeFile *theme_file,
		   char *subdir)
{
  int base;
  char *type_string;
  IconThemeDir *dir;
  IconThemeDirType type;
  char *context_string;
  GQuark context;
  int size;
  int min_size;
  int max_size;
  int threshold;
  char *full_dir;

  if (!gnome_theme_file_get_integer (theme_file,
				     subdir,
				     "Size",
				     &size))
    {
      g_warning ("Theme directory %s of theme %s has no size field\n", subdir, theme->name);
      return;
    }
  
  type = ICON_THEME_DIR_THRESHOLD;
  if (gnome_theme_file_get_string (theme_file, subdir, "Type", &type_string))
    {
      if (strcmp (type_string, "Fixed") == 0)
	type = ICON_THEME_DIR_FIXED;
      else if (strcmp (type_string, "Scalable") == 0)
	type = ICON_THEME_DIR_SCALABLE;
      else if (strcmp (type_string, "Threshold") == 0)
	type = ICON_THEME_DIR_THRESHOLD;

      g_free (type_string);
    }
  
  context = 0;
  if (gnome_theme_file_get_string (theme_file, subdir, "Context", &context_string))
    {
      context = g_quark_from_string (context_string);
      g_free (context_string);
    }

  if (!gnome_theme_file_get_integer (theme_file,
				     subdir,
				     "MaxSize",
				     &max_size))
    max_size = size;
  
  if (!gnome_theme_file_get_integer (theme_file,
				     subdir,
				     "MinSize",
				     &min_size))
    min_size = size;
  
  if (!gnome_theme_file_get_integer (theme_file,
				     subdir,
				     "Threshold",
				     &threshold))
    threshold = 2;

  for (base = 0; base < icon_theme->priv->search_path_len; base++)
    {
      full_dir = g_build_filename (icon_theme->priv->search_path[base],
				   theme->name,
				   subdir,
				   NULL);
      if (g_file_test (full_dir, G_FILE_TEST_IS_DIR))
	{
	  dir = g_new (IconThemeDir, 1);
	  dir->type = type;
	  dir->context = context;
	  dir->size = size;
	  dir->min_size = min_size;
	  dir->max_size = max_size;
	  dir->threshold = threshold;
	  dir->dir = full_dir;
	  dir->icon_data = NULL;
	  
	  scan_directory (icon_theme->priv, dir, full_dir, icon_theme->priv->allow_svg);

	  theme->dirs = g_list_append (theme->dirs, dir);
	}
      else
	g_free (full_dir);
    }
}

void
gnome_icon_data_free (GnomeIconData *icon_data)
{
  g_free (icon_data->attach_points);
  g_free (icon_data->display_name);
  g_free (icon_data);
}

GnomeIconData *
gnome_icon_data_dup (const GnomeIconData *icon_data)
{
  GnomeIconData *copy;

  copy = g_memdup (icon_data, sizeof (GnomeIconData));
  
  copy->display_name = g_strdup (copy->display_name);
  
  if (copy->attach_points)
    copy->attach_points = g_memdup (copy->attach_points,
				    copy->n_attach_points * sizeof (GnomeIconDataPoint));
  return copy;
}

