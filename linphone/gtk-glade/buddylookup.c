/*
linphone, gtk-glade interface.
Copyright (C) 2008  Simon MORLAT (simon.morlat@linphone.org)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "linphone.h"
#include "sipsetup.h"

enum {
	LOOKUP_RESULT_NAME,
	LOOKUP_RESULT_SIP_URI,
	LOOKUP_RESULT_ADDRESS,
	LOOKUP_RESULT_NCOL
};

void linphone_gtk_buddy_lookup_window_destroyed(GtkWidget *w){
	guint tid=GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w),"typing_timeout"));
	if (tid!=0){
		g_source_remove(tid);
	}
	tid=GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w),"buddylookup_processing"));
	if (tid!=0){
		g_source_remove(tid);
	}
	
}

void linphone_gtk_show_buddy_lookup_window(SipSetupContext *ctx){
	GtkListStore *store;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeSelection *select;
	GtkWidget *w=linphone_gtk_create_window("buddylookup");
	GtkWidget *results=linphone_gtk_get_widget(w,"search_results");
	
	
	store = gtk_list_store_new(LOOKUP_RESULT_NCOL, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	
	gtk_tree_view_set_model(GTK_TREE_VIEW(results),GTK_TREE_MODEL(store));
	g_object_unref(G_OBJECT(store));

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Firstname, Lastname"),
                                                   renderer,
                                                   "text", LOOKUP_RESULT_NAME,
                                                   NULL);
	g_object_set (G_OBJECT(column), "resizable", TRUE, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (results), column);

	column = gtk_tree_view_column_new_with_attributes (_("SIP address"),
                                                   renderer,
                                                   "text", LOOKUP_RESULT_SIP_URI,
                                                   NULL);
	g_object_set (G_OBJECT(column), "resizable", TRUE, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (results), column);
	
	select = gtk_tree_view_get_selection (GTK_TREE_VIEW (results));
	gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);

	gtk_tree_view_set_tooltip_column(GTK_TREE_VIEW(results),LOOKUP_RESULT_ADDRESS);
	g_object_set_data(G_OBJECT(w),"SipSetupContext",ctx);
	g_object_weak_ref(G_OBJECT(w),(GWeakNotify)linphone_gtk_buddy_lookup_window_destroyed,w);
	gtk_widget_show(w);
}

static gboolean linphone_gtk_process_buddy_lookup(GtkWidget *w){
	BuddyLookupStatus bls;
	SipSetupContext *ctx;
	GtkProgressBar *pb=GTK_PROGRESS_BAR(linphone_gtk_get_widget(w,"progressbar"));
	ctx=(SipSetupContext*)g_object_get_data(G_OBJECT(w),"SipSetupContext");
	bls=sip_setup_context_get_buddy_lookup_status(ctx);

	switch(bls){
		case BuddyLookupNone:
		case BuddyLookupFailure:
			gtk_progress_bar_set_fraction(pb,0);
			gtk_progress_bar_set_text(pb,NULL);
			break;
		case BuddyLookupConnecting:
			gtk_progress_bar_set_fraction(pb,20);
			gtk_progress_bar_set_text(pb,_("Connecting..."));
			break;
		case BuddyLookupConnected:
			gtk_progress_bar_set_fraction(pb,40);
			gtk_progress_bar_set_text(pb,_("Connected"));
			break;
		case BuddyLookupReceivingResponse:
			gtk_progress_bar_set_fraction(pb,80);
			gtk_progress_bar_set_text(pb,_("Receiving data..."));
			break;
		case BuddyLookupDone:
			gtk_progress_bar_set_fraction(pb,100);
			gtk_progress_bar_set_text(pb,_("Done !"));
			break;
	}
	return TRUE;
}

static gboolean keyword_typing_finished(GtkWidget *w){
	guint tid=GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w),"typing_timeout"));
	const char *keyword;
	SipSetupContext *ctx;
	if (tid!=0){
		g_source_remove(tid);
	}
	keyword=gtk_entry_get_text(GTK_ENTRY(w));
	if (strlen(keyword)>=4){
		guint tid2;
		ctx=(SipSetupContext*)g_object_get_data(G_OBJECT(w),"SipSetupContext");
		sip_setup_context_lookup_buddy(ctx,keyword);
		tid2=g_timeout_add(250,(GSourceFunc)linphone_gtk_process_buddy_lookup,w);
		g_object_set_data(G_OBJECT(w),"buddylookup_processing",GINT_TO_POINTER(tid2));
	}
	return FALSE;
}

void linphone_gtk_keyword_changed(GtkEditable *e){
	GtkWidget *w=gtk_widget_get_toplevel(GTK_WIDGET(e));
	guint tid=GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w),"typing_timeout"));
	if (tid!=0){
		g_source_remove(tid);
	}
	tid=g_timeout_add(2000,(GSourceFunc)keyword_typing_finished,w);
	g_object_set_data(G_OBJECT(w),"typing_timeout",GINT_TO_POINTER(tid));
}