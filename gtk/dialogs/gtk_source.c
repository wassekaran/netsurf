/*
 * Copyright 2009 Mark Benjamin <MarkBenjamin@dfgh.net>
 *
 * This file is part of NetSurf, http://www.netsurf-browser.org/
 *
 * NetSurf is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * NetSurf is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <glade/glade.h>
#include "gtk/dialogs/gtk_source.h"
#include "gtk/dialogs/gtk_about.h"
#include "gtk/gtk_window.h"
#include "gtk/gtk_gui.h"
#include "gtk/gtk_print.h"
#include "gtk/gtk_selection.h"
#include "desktop/netsurf.h"
#include "desktop/print.h"
#include "utils/messages.h"
#include "utils/utils.h"
#include "utils/utf8.h"
#include "render/font.h"
#include "content/content.h"
#include "content/content_type.h"
#include "render/textplain.h"

#include "utils/log.h"

struct menu_events {
	const char *widget;
	GCallback handler;
};

static GladeXML *glade_File;
static gchar *glade_Location;
static struct nsgtk_source_window *nsgtk_source_list = 0;
static char source_zoomlevel = 10;

void nsgtk_attach_source_menu_handlers(GladeXML *xml, gpointer g);
gboolean nsgtk_source_delete_event(GtkWindow * window, gpointer g);
gboolean nsgtk_source_destroy_event(GtkWindow * window, gpointer g);
void nsgtk_source_update_zoomlevel(gpointer g);

#define MENUEVENT(x) { #x, G_CALLBACK(nsgtk_on_##x##_activate) }
#define MENUPROTO(x) static gboolean nsgtk_on_##x##_activate( \
					GtkMenuItem *widget, gpointer g)

MENUPROTO(source_save_as);
MENUPROTO(source_print);
MENUPROTO(source_close);
MENUPROTO(source_select_all);
MENUPROTO(source_cut);
MENUPROTO(source_copy);
MENUPROTO(source_paste);
MENUPROTO(source_delete);
MENUPROTO(source_zoom_in);
MENUPROTO(source_zoom_out);
MENUPROTO(source_zoom_normal);
MENUPROTO(source_about);

struct menu_events source_menu_events[] = {
MENUEVENT(source_save_as),
MENUEVENT(source_print),
MENUEVENT(source_close),
MENUEVENT(source_select_all),
MENUEVENT(source_cut),
MENUEVENT(source_copy),
MENUEVENT(source_paste),
MENUEVENT(source_delete),
MENUEVENT(source_zoom_in),
MENUEVENT(source_zoom_out),
MENUEVENT(source_zoom_normal),
MENUEVENT(source_about),
{NULL, NULL}
};

void nsgtk_source_dialog_init(GtkWindow * parent, struct browser_window * bw)
{		
	if (bw->current_content->type == CONTENT_HTML) {
		glade_Location = g_strconcat(res_dir_location, "source.glade", 
				NULL);
		glade_File = glade_xml_new(glade_Location, NULL, NULL);
		if (glade_File != NULL) {
			char *ndata = 0;
			char **data = &ndata;
			utf8_convert_ret r = utf8_from_enc(
					bw->current_content->source_data,
					bw->current_content->data.html.encoding,
					bw->current_content->source_size,
					data);
			if (r == UTF8_CONVERT_OK) {
				GtkWindow * wndSource;
				wndSource = malloc(sizeof(GtkWindow));
				wndSource = GTK_WINDOW(glade_xml_get_widget(
						glade_File, "wndSource"));
				
				GtkWidget *cutbutton = glade_xml_get_widget(
						glade_File, "source_cut");
				GtkWidget *pastebutton = glade_xml_get_widget(
						glade_File, "source_paste");
				GtkWidget *deletebutton = glade_xml_get_widget(
						glade_File, "source_delete");
				GtkWidget *printbutton = glade_xml_get_widget(
						glade_File, "source_print");
				gtk_widget_set_sensitive(cutbutton, FALSE);
				gtk_widget_set_sensitive(pastebutton, FALSE);
				gtk_widget_set_sensitive(deletebutton, FALSE);
				/* for now */
				gtk_widget_set_sensitive(printbutton, FALSE);
							
				struct nsgtk_source_window *thiswindow = (struct
						nsgtk_source_window *)malloc(
						sizeof(
						struct  nsgtk_source_window));
				int len = strlen(bw->current_content->url);
				thiswindow->url = (char *)malloc(len + 1);
				strcpy(thiswindow->url, 
						bw->current_content->url);
				*(thiswindow->url + len) = '\0';
			

				thiswindow->data = *data;
			
				thiswindow->sourcewindow = wndSource;
				thiswindow->bw = bw;
				
				if (nsgtk_source_list) {
					thiswindow->next = nsgtk_source_list;
					thiswindow->prev = NULL;
					nsgtk_source_list->prev = thiswindow;
				} else {
					thiswindow->next = NULL;
					thiswindow->prev = NULL;
				}
				nsgtk_source_list = thiswindow;
	
				char *title;
				title = malloc(strlen(bw->current_content->url) 
						+ SLEN("Source of ") + 1);
				sprintf(title, "Source of %s", 
						bw->current_content->url);
				
				nsgtk_attach_source_menu_handlers(glade_File, 
						thiswindow);
				
				gtk_window_set_title(wndSource, title);
				free(title);
				
				g_signal_connect(G_OBJECT(wndSource), "destroy",
						G_CALLBACK(
						nsgtk_source_destroy_event),
						thiswindow);
				g_signal_connect(G_OBJECT(wndSource), 
						"delete-event", G_CALLBACK(
						nsgtk_source_delete_event),
						thiswindow);
			
				GtkTextView * sourceview = 
						GTK_TEXT_VIEW(
						glade_xml_get_widget(
						glade_File, "source_view"));
				thiswindow->gv = sourceview;
				GtkTextBuffer * tb = gtk_text_view_get_buffer(
						sourceview);
				gtk_text_buffer_set_text(tb, thiswindow->data, 
						-1);
				
				gtk_widget_show(GTK_WIDGET(wndSource));
			} else if (r == UTF8_CONVERT_NOMEM) {
				warn_user("NoMemory",0);
			} else if (r == UTF8_CONVERT_BADENC) {
				warn_user("EncNotRec",0);
			}
		} else {
			LOG(("error loading glade tree"));
		}
	}
}

void nsgtk_attach_source_menu_handlers(GladeXML *xml, gpointer g)
{
	struct menu_events *event = source_menu_events;

	while (event->widget != NULL)
	{
		GtkWidget *w = glade_xml_get_widget(xml, event->widget);
		g_signal_connect(G_OBJECT(w), "activate", event->handler, g);
		event++;
	}
}

gboolean nsgtk_on_source_save_as_activate(GtkMenuItem *widget, gpointer g)
{
	struct nsgtk_source_window * nsg = (struct nsgtk_source_window *)g;
	GtkWidget * fc = gtk_file_chooser_dialog_new(messages_get(
			"gtkSourceSave"), nsg->sourcewindow,
					GTK_FILE_CHOOSER_ACTION_SAVE, 
					GTK_STOCK_CANCEL,
			GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, 
					GTK_RESPONSE_ACCEPT, NULL);
	char *filename, *temp;
	temp = strrchr(nsg->url, '/');
	int len = strlen(nsg->url);
	filename = malloc(len + 1);
	strcpy(filename, nsg->url);
	filename[len] = '\0';
	if (temp == nsg->url + len - 1) {
		filename[len-1] = '\0';
	}
	temp = strrchr(filename, '/') + 1;
	free(filename);
	gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(fc), temp);
	if (gtk_dialog_run(GTK_DIALOG(fc)) == GTK_RESPONSE_ACCEPT) {
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fc));
		nsgtk_source_file_save(nsg->sourcewindow, filename, nsg->data);
		g_free(filename);
	}
	gtk_widget_destroy(fc);
	return TRUE;
}

void nsgtk_source_file_save(GtkWindow * parent, const char * filename, const char * data)
{
	FILE *f;
	bool auth = true;
	char temp[15];
	GtkWidget * notif, *label;
	if (!(access(filename, F_OK))) {
		auth = false;
		GtkWidget * confd = gtk_dialog_new_with_buttons(messages_get(
				"gtkOverwriteTitle"), parent,
				GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, 
				GTK_RESPONSE_ACCEPT, GTK_STOCK_CANCEL,
				GTK_RESPONSE_REJECT, NULL);
		char * warn;
		int len = strlen(filename) + 100;
		warn = malloc (len);
		snprintf(warn, len, messages_get("gtkOverwrite"), 
				filename);
		label = gtk_label_new(g_strconcat("\n",warn,"\n", NULL));
		gtk_container_add(GTK_CONTAINER(GTK_DIALOG(confd)->vbox), 
				label);
		gtk_widget_show(label);
		if (gtk_dialog_run(GTK_DIALOG(confd)) == GTK_RESPONSE_ACCEPT) {
			auth = true;
		}
		gtk_widget_destroy(confd);
		free(warn);
	}
	if (auth) {
		f = fopen(filename, "w+");
		fprintf(f, "%s", data);
		fclose(f);
		strcpy(temp, messages_get("gtkSaveConfirm"));
	} else {
		strcpy(temp, messages_get("gtkSaveCancelled"));
	}
	
	notif = gtk_dialog_new_with_buttons(temp, 
			parent, GTK_DIALOG_MODAL, GTK_STOCK_OK,
			GTK_RESPONSE_NONE, NULL);
	g_signal_connect_swapped(notif, "response",
			G_CALLBACK(gtk_widget_destroy), notif);
	label = gtk_label_new(g_strconcat("\n                    ", temp, 
			"                    \n", NULL));
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(notif)->vbox), label);
	gtk_widget_show_all(notif);
}

gboolean nsgtk_on_source_print_activate( GtkMenuItem *widget, gpointer g)
{
	/* correct printing */

	return TRUE;
}

gboolean nsgtk_on_source_close_activate( GtkMenuItem *widget, gpointer g)
{
	struct nsgtk_source_window * nsg = (struct nsgtk_source_window *)g;
	gtk_widget_destroy(GTK_WIDGET(nsg->sourcewindow));
	
	return TRUE;
}

gboolean nsgtk_source_delete_event(GtkWindow * window, gpointer g)
{
	return FALSE;
}

gboolean nsgtk_source_destroy_event(GtkWindow * window, gpointer g)
{
	struct nsgtk_source_window * nsg = (struct nsgtk_source_window *)g;	
	if (nsg->next) {
		if (nsg != nsgtk_source_list) { /* in the middle */
			nsg->prev->next = nsg->next;
			nsg->next->prev = nsg->prev;
		} else { /* last */
			nsg->next->prev = NULL;
			nsgtk_source_list = nsg->next;
		}
	} else if (nsg->prev) { /* first */
		nsg->prev->next = NULL;
	} else { /* unique */
		nsgtk_source_list = NULL;
	}
	free(nsg->data);
	free(nsg->url);
	free(g);
	return FALSE;
}

gboolean nsgtk_on_source_select_all_activate (GtkMenuItem *widget, gpointer g) {
	struct nsgtk_source_window * nsg = (struct nsgtk_source_window *)g;
	GtkTextBuffer *buf = gtk_text_view_get_buffer(nsg->gv);
	GtkTextIter start, end;
	gtk_text_buffer_get_bounds(buf, &start, &end);
	gtk_text_buffer_select_range(buf, &start, &end);
	return TRUE;
}

gboolean nsgtk_on_source_cut_activate( GtkMenuItem *widget, gpointer g)
{
	return TRUE;
}

gboolean nsgtk_on_source_copy_activate( GtkMenuItem *widget, gpointer g)
{
	struct nsgtk_source_window * nsg = (struct nsgtk_source_window *)g;
	GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(nsg->gv));
	gtk_text_buffer_copy_clipboard(buf, gtk_clipboard_get
			(GDK_SELECTION_CLIPBOARD));
	return TRUE;
}

gboolean nsgtk_on_source_paste_activate( GtkMenuItem *widget, gpointer g)
{
	return TRUE;
}

gboolean nsgtk_on_source_delete_activate( GtkMenuItem *widget, gpointer g)
{
	return TRUE;
}

gboolean nsgtk_on_source_zoom_in_activate( GtkMenuItem *widget, gpointer g)
{
	source_zoomlevel++;
	nsgtk_source_update_zoomlevel(g);
	return TRUE;
}

gboolean nsgtk_on_source_zoom_out_activate( GtkMenuItem *widget, gpointer g)
{
	if (source_zoomlevel > 1) {
		source_zoomlevel--;
		nsgtk_source_update_zoomlevel(g);
	}
	return TRUE;
}

gboolean nsgtk_on_source_zoom_normal_activate( GtkMenuItem *widget, gpointer
 		g)
{
	source_zoomlevel = 10;
	nsgtk_source_update_zoomlevel(g);
	return TRUE;
}

gboolean nsgtk_on_source_about_activate( GtkMenuItem *widget, gpointer g)
{
	struct nsgtk_source_window *nsg = (struct nsgtk_source_window *)g;
	nsgtk_about_dialog_init(nsg->sourcewindow, nsg->bw, netsurf_version);
	return TRUE;
}

void nsgtk_source_update_zoomlevel(gpointer g)
{
	struct nsgtk_source_window *nsg;
	GtkTextBuffer *buf;
	GtkTextTagTable * tab;
	GtkTextTag *tag;
	nsg = nsgtk_source_list;
	while (nsg) {
		if (nsg->gv) {
			buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(nsg->gv));
			tab = gtk_text_buffer_get_tag_table(
					GTK_TEXT_BUFFER(buf));
			tag = gtk_text_tag_table_lookup(tab, "zoomlevel");
			if (!tag) {
				tag = gtk_text_tag_new("zoomlevel");
				gtk_text_tag_table_add(tab, GTK_TEXT_TAG(tag));
			}
			gdouble fscale = ((gdouble)source_zoomlevel) / 10;
			g_object_set(GTK_TEXT_TAG(tag), "scale", fscale, NULL);
			GtkTextIter start, end;
			gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(buf), 
					&start, &end);
			gtk_text_buffer_remove_all_tags(GTK_TEXT_BUFFER(buf), 
					&start,	&end);
			gtk_text_buffer_apply_tag(GTK_TEXT_BUFFER(buf), 
					GTK_TEXT_TAG(tag), &start, &end);
		}
		nsg = nsg->next;
	}
}
