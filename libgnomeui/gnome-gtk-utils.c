#include <config.h>

#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <argp.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <gtk/gtk.h>
#include <gdk_imlib.h>

#include "libgnome/libgnomeP.h"
#include "gnome-gtk-utils.h"
#include "libgnomeui/gnome-uidefs.h"

/**** gnome_build_labelled_widget
      Inputs: 'labelstr' - A string that the 'child' widget is to be
                           labelled with
              'child' - a widget
      Outputs: 'hbox' - an hbox holding the created label and 'child'

      Description: Creates 'label', a GtkLabel with 'labelstr' as its text.
                   Creates 'hbox', a GtkHBox
		   Inserts 'label' and 'child' into 'hbox'
		   Returns 'hbox'
      Author: Elliot Lee <sopwith@redhat.com>
 */
GtkWidget *gnome_build_labelled_widget(char *labelstr, GtkWidget *child)
{
  GtkWidget *hbox, *label;

  hbox = gtk_hbox_new(FALSE, GNOME_PAD_SMALL);

  label = gtk_label_new(labelstr);
  gtk_widget_show(label);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

  gtk_widget_show(child);
  gtk_container_add(GTK_CONTAINER(hbox), child);

  return hbox;
}

/* same as above, but does widget+label instead of label+widget */
GtkWidget *gnome_build_widget_labelled(GtkWidget *child, const char *labelstr)
{
  GtkWidget *hbox, *label;

  hbox = gtk_hbox_new(FALSE, GNOME_PAD_SMALL);

  gtk_widget_show(child);
  gtk_container_add(GTK_CONTAINER(hbox), child);

  label = gtk_label_new(labelstr);
  gtk_widget_show(label);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

  return hbox;
}
