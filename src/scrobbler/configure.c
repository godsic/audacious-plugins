#include "settings.h"

#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <glib.h>
#include <audacious/plugin.h>
#include <audacious/audutil.h>
#include <audacious/i18n.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "configure.h"
#include "plugin.h"

GtkWidget *entry1, *entry2, *entry3, *ge_entry1, *ge_entry2, *cfgdlg;
static GdkColor disabled_color;
guint apply_timeout = 0; /* ID of timeout to save new config */
gboolean running = TRUE; /* if plugin threads are running */

static char *hexify(char *pass, int len)
{
        static char buf[33];
        char *bp = buf;
        char hexchars[] = "0123456789abcdef";
        int i;

        memset(buf, 0, sizeof(buf));

        for(i = 0; i < len; i++) {
                *(bp++) = hexchars[(pass[i] >> 4) & 0x0f];
                *(bp++) = hexchars[pass[i] & 0x0f];
        }
        *bp = 0;
        return buf;
}

static char *pwd = NULL;
static char *ge_pwd = NULL;

static void saveconfig(void)
{
    mcs_handle_t *cfgfile;

    const char *uid = gtk_entry_get_text(GTK_ENTRY(entry1));
    const char *url = gtk_entry_get_text(GTK_ENTRY(entry3));
    const char *ge_uid = gtk_entry_get_text(GTK_ENTRY(ge_entry1));

    if ((cfgfile = aud_cfg_db_open())) {
        aud_md5state_t md5state;
        unsigned char md5pword[16], ge_md5pword[16];

        if (uid != NULL && uid[0] != '\0' &&
            pwd != NULL && pwd[0] != '\0')
        {
            aud_cfg_db_set_string(cfgfile, "audioscrobbler", "username", (char *)uid);
            aud_md5_init(&md5state);
            aud_md5_append(&md5state, (unsigned const char *)pwd, strlen(pwd));
            aud_md5_finish(&md5state, md5pword);
            aud_cfg_db_set_string(cfgfile, "audioscrobbler", "password",
                                 hexify((char*)md5pword, sizeof(md5pword)));
        } else if (!uid || uid[0] == '\0') {
            aud_cfg_db_set_string(cfgfile, "audioscrobbler", "username", "");
            aud_cfg_db_set_string(cfgfile, "audioscrobbler", "password", "");
        }
        
        if (url != NULL && url[0] != '\0')
        	aud_cfg_db_set_string(cfgfile, "audioscrobbler", "sc_url", (char *)url);
       	else if (!url || url[0] == '\0')
       		aud_cfg_db_set_string(cfgfile, "audioscrobbler", "sc_url", LASTFM_HS_URL);

        if (ge_uid != NULL && ge_uid[0] != '\0' &&
            ge_pwd != NULL && ge_pwd[0] != '\0')
        {
            aud_cfg_db_set_string(cfgfile, "audioscrobbler", "ge_username", (char *)ge_uid);
            aud_md5_init(&md5state);
            aud_md5_append(&md5state, (unsigned const char *)ge_pwd, strlen(ge_pwd));
            aud_md5_finish(&md5state, ge_md5pword);
            aud_cfg_db_set_string(cfgfile, "audioscrobbler", "ge_password",
                                  hexify((char*)ge_md5pword, sizeof(ge_md5pword)));
        } else if (!ge_uid || ge_uid[0] == '\0') {
            aud_cfg_db_set_string(cfgfile, "audioscrobbler", "ge_username", "");
            aud_cfg_db_set_string(cfgfile, "audioscrobbler", "ge_password", "");
        }

        aud_cfg_db_close(cfgfile);
    }
}

static gboolean apply_config_changes(gpointer data) {
    apply_timeout = 0;
    saveconfig();
    start();
    running = TRUE;
    return FALSE;
}

void configure_cleanup(void) {
    if (apply_timeout) { /* config has been changed, but wasn't saved yet */
        g_source_remove(apply_timeout);
        apply_timeout = 0;
        saveconfig();
    }
    g_free(pwd);
    g_free(ge_pwd);
}

static void
entry_changed(GtkWidget *widget, gpointer data)
{
    if (running) {
        stop();
        running = FALSE;
    }

    if (apply_timeout)
        g_source_remove(apply_timeout);

    apply_timeout = g_timeout_add_seconds(10, (GSourceFunc) apply_config_changes, NULL);
}

static void entry_focus_in(GtkWidget *widget, gpointer data)
{
  gtk_entry_set_text(GTK_ENTRY(widget), "");
  gtk_entry_set_visibility(GTK_ENTRY(widget), FALSE);
  gtk_widget_modify_text(widget, GTK_STATE_NORMAL, NULL);
}

static void entry_focus_out(GtkWidget *widget, gpointer data)
{
  if (widget == entry2)
  {
    g_free(pwd);
    pwd = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry2)));
  }
  if (widget == ge_entry2)
  {
    g_free(ge_pwd);
    ge_pwd = g_strdup(gtk_entry_get_text(GTK_ENTRY(ge_entry2)));
  }
  entry_changed(widget, data);
  gtk_entry_set_text(GTK_ENTRY(widget), _("Change password"));
  gtk_widget_modify_text(widget, GTK_STATE_NORMAL, &disabled_color);
  gtk_entry_set_visibility(GTK_ENTRY(widget), TRUE);
}

/* Generated by glade, sorta. */
GtkWidget *
create_cfgdlg(void)
{
  mcs_handle_t *db;
  GtkWidget *vbox2;
  GtkWidget *table1;
  GtkWidget *label3;
  GtkWidget *label1;
  GtkWidget *label2;
  GtkWidget *label4;
  GtkWidget *himage1;
  GtkWidget *align1;
  GtkWidget *notebook1;
  GtkStyle *style;

  vbox2 = gtk_vbox_new (FALSE, 0);

  label1 = gtk_label_new (_("<b>Services</b>"));
  gtk_widget_show (label1);
  gtk_label_set_use_markup (GTK_LABEL (label1), TRUE);
  gtk_misc_set_alignment (GTK_MISC (label1), 0, 0.5);

  gtk_box_pack_start (GTK_BOX (vbox2), label1, FALSE, FALSE, 0);

  notebook1 = gtk_notebook_new();
  gtk_widget_show (notebook1);


  // last fm
  align1 = gtk_alignment_new(0, 0, 0, 0);
  gtk_widget_show (align1);
  gtk_alignment_set_padding(GTK_ALIGNMENT(align1), 0, 0, 12, 0);

  table1 = gtk_table_new (2, 2, FALSE);
  gtk_widget_show (table1);
  gtk_container_add(GTK_CONTAINER(align1), table1);
  gtk_table_set_row_spacings (GTK_TABLE(table1), 6);
  gtk_table_set_col_spacings (GTK_TABLE(table1), 6);

  label2 = gtk_label_new (_("Username:"));
  gtk_widget_show (label2);
  gtk_table_attach_defaults (GTK_TABLE (table1), label2, 0, 1, 2, 3);
  gtk_label_set_justify (GTK_LABEL (label2), GTK_JUSTIFY_RIGHT);
  gtk_misc_set_alignment (GTK_MISC (label2), 1, 0.5);

  label3 = gtk_label_new (_("Password:"));
  gtk_widget_show (label3);
  gtk_table_attach (GTK_TABLE (table1), label3, 0, 1, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_label_set_justify (GTK_LABEL (label3), GTK_JUSTIFY_RIGHT);
  gtk_misc_set_alignment (GTK_MISC (label3), 1, 0.5);

  label4 = gtk_label_new (_("Scrobbler URL:"));
  gtk_widget_show (label4);
  gtk_table_attach (GTK_TABLE (table1), label4, 0, 1, 4, 5,
  					(GtkAttachOptions) (GTK_FILL),
  					(GtkAttachOptions) (0), 0, 0);
  gtk_label_set_justify (GTK_LABEL (label4), GTK_JUSTIFY_RIGHT);
  gtk_misc_set_alignment (GTK_MISC (label4), 1, 0.5);

  entry1 = gtk_entry_new ();
  gtk_widget_show (entry1);
  gtk_table_attach_defaults (GTK_TABLE (table1), entry1, 1, 2, 2, 3);

  entry2 = gtk_entry_new ();

  style = gtk_widget_get_style(entry2);
  memcpy(&disabled_color, &(style->text[GTK_STATE_INSENSITIVE]), sizeof(GdkColor));
  gtk_widget_modify_text(entry2, GTK_STATE_NORMAL, &disabled_color);

  gtk_entry_set_text(GTK_ENTRY(entry2), _("Change password"));
  g_signal_connect(G_OBJECT(entry2), "focus-in-event",
                   G_CALLBACK(entry_focus_in),
                   NULL);
  g_signal_connect(G_OBJECT(entry2), "focus-out-event",
                   G_CALLBACK(entry_focus_out),
                   NULL);
  gtk_widget_show (entry2);
  gtk_table_attach_defaults (GTK_TABLE (table1), entry2, 1, 2, 3, 4);
  
  entry3 = gtk_entry_new ();
  gtk_widget_show (entry3);
  gtk_table_attach_defaults (GTK_TABLE (table1), entry3, 1, 2, 4, 5);

  label1 = gtk_label_new (_("<b>Last.FM</b>"));
  gtk_label_set_use_markup (GTK_LABEL (label1), TRUE);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook1), GTK_WIDGET(align1), label1);


  // gerpok
  align1 = gtk_alignment_new(0, 0, 0, 0);
  gtk_widget_show (align1);
  gtk_alignment_set_padding(GTK_ALIGNMENT(align1), 0, 0, 12, 0);

  table1 = gtk_table_new (2, 2, FALSE);
  gtk_widget_show (table1);
  gtk_container_add(GTK_CONTAINER(align1), table1);
  gtk_table_set_row_spacings (GTK_TABLE(table1), 6);
  gtk_table_set_col_spacings (GTK_TABLE(table1), 6);

  label2 = gtk_label_new (_("Username:"));
  gtk_widget_show (label2);
  gtk_table_attach_defaults (GTK_TABLE (table1), label2, 0, 1, 2, 3);
  gtk_label_set_justify (GTK_LABEL (label2), GTK_JUSTIFY_RIGHT);
  gtk_misc_set_alignment (GTK_MISC (label2), 1, 0.5);

  label3 = gtk_label_new (_("Password:"));
  gtk_widget_show (label3);
  gtk_table_attach (GTK_TABLE (table1), label3, 0, 1, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_label_set_justify (GTK_LABEL (label3), GTK_JUSTIFY_RIGHT);
  gtk_misc_set_alignment (GTK_MISC (label3), 1, 0.5);

  ge_entry1 = gtk_entry_new ();
  gtk_widget_show (ge_entry1);
  gtk_table_attach_defaults (GTK_TABLE (table1), ge_entry1, 1, 2, 2, 3);

  ge_entry2 = gtk_entry_new ();
  gtk_widget_modify_text(ge_entry2, GTK_STATE_NORMAL, &disabled_color);

  gtk_entry_set_text(GTK_ENTRY(ge_entry2), "Change password");
  g_signal_connect(G_OBJECT(ge_entry2), "focus-in-event",
                   G_CALLBACK(entry_focus_in),
                   NULL);
  g_signal_connect(G_OBJECT(ge_entry2), "focus-out-event",
                   G_CALLBACK(entry_focus_out),
                   NULL);

  gtk_widget_show (ge_entry2);
  gtk_table_attach_defaults (GTK_TABLE (table1), ge_entry2, 1, 2, 3, 4);

  label1 = gtk_label_new (_("<b>Gerpok</b>"));
  gtk_label_set_use_markup (GTK_LABEL (label1), TRUE);
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook1), GTK_WIDGET(align1), label1);

  // common
  gtk_box_pack_start (GTK_BOX (vbox2), notebook1, TRUE, TRUE, 6);

  himage1 = gtk_image_new_from_file (DATA_DIR "/images/audioscrobbler_badge.png");
  gtk_widget_show (himage1);
  gtk_box_pack_start (GTK_BOX (vbox2), himage1, FALSE, FALSE, 0);
  gtk_misc_set_alignment (GTK_MISC (himage1), 1, 0.5);

	gtk_entry_set_text(GTK_ENTRY(entry1), "");
	gtk_entry_set_text(GTK_ENTRY(ge_entry1), "");

        if ((db = aud_cfg_db_open())) {
                gchar *username = NULL;
                gchar *sc_url = NULL;
		// last fm
                aud_cfg_db_get_string(db, "audioscrobbler", "username",
                        &username);
                if (username) {
                        gtk_entry_set_text(GTK_ENTRY(entry1), username);
                        g_free(username);
			username = NULL;
                }
                
				aud_cfg_db_get_string(db, "audioscrobbler", "sc_url", &sc_url);
				if (sc_url) {
                		gtk_entry_set_text(GTK_ENTRY(entry3), sc_url);
                		g_free(sc_url);
		             	sc_url = NULL;
				}
				
		// gerpok
                aud_cfg_db_get_string(db, "audioscrobbler", "ge_username",
                        &username);
                if (username) {
                        gtk_entry_set_text(GTK_ENTRY(ge_entry1), username);
                        g_free(username);
			username = NULL;
                }

                aud_cfg_db_close(db);
        }

  g_signal_connect(entry1, "changed", G_CALLBACK(entry_changed), NULL);
  g_signal_connect(entry3, "changed", G_CALLBACK(entry_changed), NULL);
  g_signal_connect(ge_entry1, "changed", G_CALLBACK(entry_changed), NULL);

  return vbox2;
}

