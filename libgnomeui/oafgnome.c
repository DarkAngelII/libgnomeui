#include "oafgnome.h"
#include <signal.h>
#include <popt.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <gtk/gtk.h>
#include <libgnome/gnome-defs.h>
#include <libgnome/gnome-i18nP.h>
#include <libgnome/gnome-config.h>
#include <liboaf/liboaf.h>
#include <gdk/gdkx.h>
#include <gdk/gdk.h>
#include <libgnomeui/gnome-init.h>
#include <libgnomeui/gnome-dialog-util.h>
#include <libgnomeui/gnome-dialog.h>

static void og_pre_args_parse(GnomeProgram *app,
			      const GnomeModuleInfo *mod_info);
static void og_post_args_parse(GnomeProgram *app,
			       const GnomeModuleInfo *mod_info);
static OAFRegistrationLocation rootwin_regloc;
static CORBA_Object rcmd_activator(const OAFRegistrationCategory *regcat, const char **cmd,
				   int ior_fd, CORBA_Environment *ev);

static GnomeModuleInfo orbit_module_info = {
  "ORBit", orbit_version, "CORBA implementation",
   NULL,
   NULL, NULL,
   NULL,
   NULL
};

static GnomeModuleRequirement liboaf_requirements[] = {
  {"0.5.1", &orbit_module_info},
  {NULL}
};

static GnomeModuleInfo liboaf_module_info = {
  "liboaf", liboaf_version, "Object Activation Framework",
  liboaf_requirements,
  (GnomeModuleHook)oaf_preinit, (GnomeModuleHook)oaf_postinit,
  (struct poptOption *)oaf_popt_options,
  NULL
};

static GnomeModuleRequirement og_requirements[] = {
  {"0.0", &liboaf_module_info},
  {"1.2.0", &gtk_module_info},
  {NULL}
};

GnomeModuleInfo liboafgnome_module_info = {
  "liboafgnome", VERSION, "OAF integration for GNOME programs",
  og_requirements,
  og_pre_args_parse, og_post_args_parse,
  NULL,
  NULL
};

static void
og_pre_args_parse(GnomeProgram *app, const GnomeModuleInfo *mod_info)
{
  int dumb_argc = 1;
  char *dumb_argv[] = {NULL};
  dumb_argv[0] = program_invocation_name;
  (void) oaf_orb_init(&dumb_argc, dumb_argv);
}

static void
og_post_args_parse(GnomeProgram *app, const GnomeModuleInfo *mod_info)
{
  oaf_registration_location_add(&rootwin_regloc, -100, NULL);
  oaf_registration_activator_add(rcmd_activator, 100);
}

/* Registration location stuff */
#include <gdk/gdkx.h>

static void
rootwin_lock(const OAFRegistrationLocation *regloc,
	     gpointer user_data)
{
  XGrabServer(GDK_DISPLAY());
  gdk_flush();
}

static void
rootwin_unlock(const OAFRegistrationLocation *regloc,
	       gpointer user_data)
{
  XUngrabServer(GDK_DISPLAY());
  gdk_flush();
}

static char *
rootwin_check(const OAFRegistrationLocation *regloc,
	      const OAFRegistrationCategory *regcat,
	      int *ret_distance, gpointer user_data)
{
  GdkAtom type;
  int fmt;
  char *ior = NULL;
  gint actual_length;

  if(strcmp(regcat->name, "IDL:OAF/ActivationContext:1.0"))
    return NULL;

  if(!
     gdk_property_get (GDK_ROOT_PARENT(),
		       gdk_atom_intern("OAFGNOME_AC_IOR", FALSE),
		       gdk_atom_intern("STRING", FALSE),
		       0, 99999, FALSE, &type, &fmt, &actual_length,
		       (guchar **)&ior))
    return NULL;

  return ior;
}

static void
rootwin_register(const OAFRegistrationLocation *regloc,
		 const char *ior,
		 const OAFRegistrationCategory *regcat,
		 gpointer user_data)
{
  gdk_property_change(GDK_ROOT_PARENT(), gdk_atom_intern("OAFGNOME_AC_IOR", FALSE),
		      gdk_atom_intern("STRING", FALSE), 8, GDK_PROP_MODE_REPLACE, (guchar *) ior, strlen(ior));
}

static void
rootwin_unregister(const OAFRegistrationLocation *regloc,
		   const char *ior,
		   const OAFRegistrationCategory *regcat,
		   gpointer user_data)
{
  gdk_property_delete(GDK_ROOT_PARENT(), gdk_atom_intern("OAFGNOME_AC_IOR", FALSE));
}

static OAFRegistrationLocation rootwin_regloc = {
  rootwin_lock,
  rootwin_unlock,
  rootwin_check,
  rootwin_register,
  rootwin_unregister
};

/******** The "execute a remote command" hack :) *******/
typedef struct {
  const OAFRegistrationCategory *regcat;

  char iorbuf[4096];

  int infd, outfd;
  FILE *in_fh, *out_fh;

  guint out_tag;

  enum { CONNECTING, ITEMWAIT, REMOTE_CONV, DONE } state;
  const char **cmd;
} RCMDRunInfo;

static void
item_send_val(gchar *string, gpointer data)
{
  RCMDRunInfo *ri = (RCMDRunInfo *)data;

  if(string) {
    fprintf(ri->in_fh, "%s\n", string);
    g_free(string);
    ri->state = CONNECTING;
  }

  gtk_main_quit();
}

/* What a stupid replacement for expect. It will break if anything isn't working exactly right. oh well :\ */
static void
rcmd_handle_connecting(RCMDRunInfo *ri)
{
  char aline[4096];

  if(!fgets(aline, sizeof(aline), ri->out_fh))
    {
      gtk_main_quit();
      return;
    }

  if(strstr(aline, "login:"))
    {
      GtkWidget *prompt_dialog;

      ri->state = ITEMWAIT;

      prompt_dialog = gnome_request_dialog(FALSE, N_("Username:"), g_get_user_name(), -1, item_send_val, ri, NULL);
      gnome_dialog_run(GNOME_DIALOG(prompt_dialog));

      if(ri->state == ITEMWAIT)
	gtk_main_quit(); /* User cancelled, show is over */
    }
  else if(strstr(aline, "ssword:"))
    {
      GtkWidget *prompt_dialog;

      ri->state = ITEMWAIT;

      prompt_dialog = gnome_request_dialog(TRUE, N_("Password:"), "", -1, item_send_val, ri, NULL);
      gnome_dialog_run(GNOME_DIALOG(prompt_dialog));

      if(ri->state == ITEMWAIT)
	gtk_main_quit(); /* User cancelled, show is over */
    }
  else if(strstr(aline, "continue connecting"))
    {
      fputs("yes\n", ri->in_fh);
    }
  else if(strstr(aline, "GNOME BOOTSTRAP READY"))
    {
      int i;

      ri->state = REMOTE_CONV;

      fprintf(ri->in_fh, "RUN");
      for(i = 0; ri->cmd[i]; i++)
	fprintf(ri->in_fh, " %s", ri->cmd[i]);
      fprintf(ri->in_fh, "\n");
    }
  else if(strstr(aline, "Permission denied.")
	  || strstr(aline, "connection closed."))
    {
      gtk_main_quit(); /* Can't connect */
    }

  fflush(ri->in_fh);
}

static void
rcmd_handle_itemwait(RCMDRunInfo *ri)
{
  g_error("We should never get an FD callback in itemwait state!");
}

static void
rcmd_handle_remote_conv(RCMDRunInfo *ri)
{
  char aline[4096];

  if(!fgets(aline, sizeof(aline), ri->out_fh))
    {
      gtk_main_quit();
      return;
    }

  g_strstrip(aline);

  if(!strncmp(aline, "OUTPUT ", strlen("OUTPUT ")))
    {
      if(!strncmp(aline + strlen("OUTPUT "), "IOR:", strlen("IOR:"))) /* Bingo */
	{
	  ri->state = DONE;

	  fprintf(ri->in_fh, "DONE\n");
	  gtk_main_quit();
	}
    }
  else if(!strncmp(aline, "ERROR ", strlen("ERROR ")))
    {
      ri->state = DONE;
      gtk_main_quit();
    }
}

static gboolean
rcmd_handle_output(GIOChannel *src, GIOCondition condition, gpointer data)
{
  RCMDRunInfo *ri;

  ri = (RCMDRunInfo *)data;

  if(!(condition & G_IO_IN)) /* Some sort of error */
    {
      ri->iorbuf[0] = '\0';
      gtk_main_quit();
      return TRUE;
    }

  switch(ri->state)
    {
    case CONNECTING:
      rcmd_handle_connecting(ri);
      break;
    case ITEMWAIT:
      rcmd_handle_itemwait(ri);
      break;
    case REMOTE_CONV:
      rcmd_handle_remote_conv(ri);
      break;
    case DONE:
      gtk_main_quit();
      break;
    }

  return TRUE;
}

static char *
rcmd_run(const OAFRegistrationCategory *regcat, const char **argv, int argc, const char **remote_cmd)
{
  RCMDRunInfo ri;
  int pipes_in[2], pipes_out[2];
  int childpid;

  if(pipe(pipes_in))
    return NULL;

  if(pipe(pipes_out))
    {
      close(pipes_in[0]);
      close(pipes_in[1]);
      return NULL;
    }

  memset(&ri, 0, sizeof(ri));

  ri.regcat = regcat;
  ri.state = CONNECTING;

  childpid = fork();
  if(!childpid)
    {
      sigset_t mask;

      setsid();

      /* Make sure that the rsh/ssh command doesn't wig out after papa stops listening */
      sigemptyset(&mask);
      sigaddset(&mask, SIGPIPE);
      sigprocmask(SIG_BLOCK, &mask, NULL);

      close(pipes_out[0]);

      if(dup2(pipes_out[1], 1) < 0)
	_exit(1);

      if(dup2(pipes_out[1], 2) < 0)
	_exit(1);

      if(pipes_out[1] != 1 && pipes_out[1] != 2)
	close(pipes_out[1]);

      close(pipes_in[1]);

      if(dup2(pipes_in[0], 0) < 0)
	_exit(1);

      if(pipes_in[0] != 0)
	close(pipes_in[0]);

      /* Now we have all the pipes set up ready to run the process - let's do it */

      execvp(argv[0], (char **)argv);

      _exit(1); /* Doh! */
    }

  ri.infd = pipes_in[1];
  ri.in_fh = fdopen(ri.infd, "w");
  ri.outfd = pipes_out[0];
  ri.out_fh = fdopen(ri.outfd, "r");

  {
    GIOChannel *i_hate_glib;
    i_hate_glib = g_io_channel_unix_new(ri.outfd);
    ri.out_tag = g_io_add_watch(i_hate_glib, G_IO_IN|G_IO_ERR|G_IO_HUP|G_IO_NVAL, rcmd_handle_output, &ri);
    g_io_channel_unref(i_hate_glib);
  }

  gtk_main();

  g_source_remove(ri.out_tag);

  fclose(ri.in_fh);
  fclose(ri.out_fh);

  waitpid(childpid, &ri.out_tag, WNOHANG); /* Try to reap it if possible */

  return (*ri.iorbuf)?g_strdup(ri.iorbuf):NULL;
}

static CORBA_Object
rcmd_activator(const OAFRegistrationCategory *regcat, const char **cmd,
	       int ior_fd, CORBA_Environment *ev)
{
  char *basecmd = gnome_config_get_string("/OAF/RemoteCommand=rsh");
  char *ior_string;
  char *argv[10], iornum_buf[25], display_buf[512], langs_buf[512];
  int argc = 0;
  CORBA_Object retval;

  argv[argc++] = basecmd;
  if(regcat->username)
    {
      argv[argc++] = "-l";
      argv[argc++] = regcat->username;
    }

  argv[argc++] = regcat->hostname?regcat->hostname:"localhost";
  argv[argc++] = "gnome-remote-bootstrap"; /* The command that runs on the remote end to start oafd for us */
  sprintf(iornum_buf, "--ior-fd=%d", ior_fd);
  argv[argc++] = iornum_buf;

  {
    char *mydisplay = NULL;
    gnome_program_attributes_get(gnome_program_get(), LIBGNOMEUI_PARAM_DISPLAY, &mydisplay, NULL);

    g_assert(mydisplay);

    g_snprintf(display_buf, sizeof(display_buf), "--display=%s%s", (mydisplay[0]==':')?oaf_hostname_get():"", mydisplay);
    argv[argc++] = display_buf;
  }

  {
    GList *langs = gnome_i18n_get_language_list(NULL);
    GString *langparam = g_string_new(langs?langs->data:"");

    if(langs)
      for(langs = langs->next; langs; langs = langs->next)
	{
	  g_string_append_c(langparam, ':');
	  g_string_append(langparam, langs->data);
	}

    g_snprintf(langs_buf, sizeof(langs_buf), "--languages=%s", langparam->str);
    g_string_free(langparam, TRUE);
    argv[argc++] = display_buf;
  }

  argv[argc] = NULL; /* The last one */

  ior_string = rcmd_run(regcat, argv, argc, cmd);
  g_free(basecmd);

  retval = CORBA_ORB_string_to_object(oaf_orb_get(), ior_string, ev);
  if(ev->_major != CORBA_NO_EXCEPTION)
    retval = CORBA_OBJECT_NIL;

  g_free(ior_string);

  return retval;
}
