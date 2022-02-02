// Copyright (C) 2004 Rikus Wessels <rikusw at rootshell dot be>
// GNU GPL v2.0

//
// Do whatever you like with and to this file,
// it is only and example.

#include <gnome.h>
#include "../nodes.h"

bool enumFunc(Node *n,int flags,void *pv)
{
    if(!(n->GetType() & NTT_VISIBLE)) return true;
    GtkTree *tree = (GtkTree*)pv;
   
    GtkWidget *w = gtk_tree_item_new_with_label(n->GetPrompt());
    gtk_tree_append((GtkTree*)tree,w);

    // hide skipped nodes
//    if(n->GetState() & NS_SKIPPED) gtk_widget_hide(w); else
    gtk_widget_show(w);

    // use g_object_set_data ???
//    gtk_object_set_data(GTK_OBJECT(w),"Node",(gpointer)n); n->user = w;
    
    if(n->GetType() & NTT_PARENT)
    {
	GtkWidget *tree = gtk_tree_new();
	gtk_tree_item_set_subtree((GtkTreeItem*)w,tree);
	n->Enumerate(enumFunc,NS_T_ALL,tree);
	return true;
    }
    return true;
}

void PrintIt()
{
    printf("Clicked\n");
}

int main(int argc,char *argv[])
{
    gnome_init("test1","0.1",argc,argv);

    NodeRoot *nr = new NodeRoot();
    if(nr->Init_CmdLine(argc,argv) & 7) { printf("init fail\n"); }

    GtkWidget *app = gnome_app_new("test12",nr->GetPrompt());//"window title");
    GtkWidget *roottree = gtk_tree_new();
    gtk_object_set_data(GTK_OBJECT(roottree),"Node",(gpointer)nr); nr->user = roottree;

    // Add it all
    enumFunc(nr,0,roottree);

//    gtk_signal_connect(GTK_OBJECT(roottree),"",GTK_SIGNAL_FUNC(PrintIt),0);
    gtk_signal_connect(GTK_OBJECT(app),"delete_event",
		       GTK_SIGNAL_FUNC(gtk_main_quit),NULL);

    gnome_app_set_contents(GNOME_APP(app),roottree);
    gtk_widget_show_all(app);
    gtk_main();

    delete nr;
	
    return 0;
}
