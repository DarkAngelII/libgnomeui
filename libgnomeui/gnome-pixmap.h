#ifndef GNOME_PIXMAP_H
#define GNOME_PIXMAP_H


#include <gdk/gdk.h>
#include <gtk/gtkwidget.h>
#include <gdk_imlib.h>


BEGIN_GNOME_DECLS


#define GNOME_PIXMAP(obj)         GTK_CHECK_CAST (obj, gnome_pixmap_get_type (), GnomePixmap)
#define GNOME_PIXMAP_CLASS(klass) GTK_CHECK_CLASS_CAST (klass, gnome_pixmap_get_type (), GnomePixmapClass)
#define GNOME_IS_PIXMAP(obj)      GTK_CHECK_TYPE (obj, gnome_pixmap_get_type ())


typedef struct _GnomePixmap GnomePixmap;
typedef struct _GnomePixmapClass GnomePixmapClass;

struct _GnomePixmap {
	GtkWidget widget;

	GdkPixmap *pixmap;
	GdkBitmap *mask;
};

struct _GnomePixmapClass {
	GtkWidgetClass parent_class;
};


guint      gnome_pixmap_get_type               (void);

GtkWidget *gnome_pixmap_new_from_file          (const char *filename);
GtkWidget *gnome_pixmap_new_from_file_at_size  (const char *filename, int width, int height);
GtkWidget *gnome_pixmap_new_from_xpm_d         (char **xpm_data);
GtkWidget *gnome_pixmap_new_from_xpm_d_at_size (char **xpm_data, int width, int height);
GtkWidget *gnome_pixmap_new_from_rgb_d         (unsigned char *data, unsigned char *alpha,
						int rgb_width, int rgb_height);
GtkWidget *gnome_pixmap_new_from_rgb_d_shaped  (unsigned char *data, unsigned char *alpha,
						int rgb_width, int rgb_height,
						GdkImlibColor *shape_color);
GtkWidget *gnome_pixmap_new_from_rgb_d_at_size (unsigned char *data, unsigned char *alpha,
						int rgb_width, int rgb_height,
						int width, int height);
GtkWidget *gnome_pixmap_new_from_gnome_pixmap  (GnomePixmap *gpixmap);

void       gnome_pixmap_load_file              (GnomePixmap *gpixmap, const char *filename);
void       gnome_pixmap_load_file_at_size      (GnomePixmap *gpixmap, const char *filename, int width, int height);
void       gnome_pixmap_load_xpm_d             (GnomePixmap *gpixmap, char **xpm_data);
void       gnome_pixmap_load_xpm_d_at_size     (GnomePixmap *gpixmap, char **xpm_data, int width, int height);
void       gnome_pixmap_load_rgb_d             (GnomePixmap *gpixmap, unsigned char *data, unsigned char *alpha,
						int rgb_width, int rgb_height);
void       gnome_pixmap_load_rgb_d_shaped      (GnomePixmap *gpixmap, unsigned char *data, unsigned char *alpha,
						int rgb_width, int rgb_height,
						GdkImlibColor *shape_color);
void       gnome_pixmap_load_rgb_d_at_size     (GnomePixmap *gpixmap, unsigned char *data, unsigned char *alpha,
						int rgb_width, int rgb_height,
						int width, int height);


END_GNOME_DECLS


#endif /* __GNOME_PIXMAP_H__ */
