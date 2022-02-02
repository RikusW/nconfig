const char *CopyRight =
"gconfig is a gtk+2 kernel configuration utility based on the nconfig backend.\n\n"

"Copyright (C) 2004-2006 Rikus Wessels <rikusw at rootshell dot be>\n"
"GNU GPL v2.0";

// line layout +-
// 50
// backend
// 80  - filling
// 150 - interfacing
// frontend
// 220 - help/text browsing
// 150 - menu handlers
// 100 - menu building
// 100 - init

#include <gnome.h>
#include <gtk/gtk.h>

#include "../nodes.h"

NodeRoot *nr=0;
GtkWidget *app=0;
Node *nLastDep=0;
GtkTreeStore *store=0,*depstore=0;
GtkWidget *ArchMenu=0;
GtkWidget *tree,*deptree,*textv;

gboolean bShowFile=0;
gboolean bShowDisabled=1,bShowSkipped=1;

enum
{
    PROMPT_COLUMN,
    NODE_COLUMN,
    VISIBLE_COLUMN,
    COLOR_COLUMN,
    EDITABLE_COLUMN,
    N_COLUMNS
};

struct TreeUserData
{
    GtkTreeStore *ts;
    GtkTreeIter *p,*c; // parent,child
};

//---------------------------------------------------------------------------------------

void SetPromptState(GtkTreeStore *gts, Node *n)
{
    char c,*color="black";
    if(n->GetType() & (NTT_INPUT | NTT_DEF))
    switch(n->Get())
    {
    case 1: c = 'N'; color="red"; break;
    case 2: c = 'M'; color="yellow"; break;
    case 3: c = 'Y'; color="green"; break;
    default:
        if(n->GetType() & NTT_STR)
        { c = 'S'; color = "blue"; } else
        { c = '?'; color = "black"; }
    }else
    switch(n->GetType())
    {
    case NT_ROOT:
    case NT_MENU: c='M'; color="blue"; break;
    case NT_CHOICEP: c='C'; color="blue"; break;
    case NT_COMMENT | NT_PARENT:
    case NT_COMMENT: c='C'; color="white"; break;
    default: c= '?'; color="black"; break;
    }
    
    bool bVis=1;
    if(n->GetState() & NS_SKIPPED) { color="purple"; bVis = bShowSkipped; } else
    if(n->GetState() & NS_DISABLED) { color="slategrey"; bVis = bShowDisabled; }

    GtkTreeIter *iter = (GtkTreeIter*)n->user;
    if(!iter) return;
    char buf[200];
    snprintf(buf,199,"%c %s",c,n->GetPrompt());
    gtk_tree_store_set (gts, iter, PROMPT_COLUMN, buf, COLOR_COLUMN, color, VISIBLE_COLUMN,bVis,-1);
}

// fill the tree store
bool enumFunc(Node *n,int flags,void *pv)
{
    if(!(n->GetType() & NTT_VISIBLE)) return true;
    TreeUserData *tud = (TreeUserData*)pv;
  
    // add to store
    gtk_tree_store_append(tud->ts,tud->c,tud->p);
    gtk_tree_store_set(tud->ts,tud->c,NODE_COLUMN,n,EDITABLE_COLUMN,0,-1);
    n->user = gtk_tree_iter_copy(tud->c); // free in FreeFunc
    SetPromptState(tud->ts,n);
   
    // if parent get the children    
    if(n->GetType() & NTT_PARENT)
    {
	n->Notify(NS_COLLAPSE);//n->state |= NS_COLLAPSE; // set to collapsed WITHOUT notify
	GtkTreeIter iter;
	TreeUserData td;
	td.ts = tud->ts;
	td.p = tud->c;
	td.c = &iter;

	n->Enumerate(enumFunc,NS_T_ALL,&td);
    }
    return true;
}

// free all allocated iterators
bool FreeFunc(Node *n,int flags,void *pv)
{
    if(!(n->GetType() & NTT_VISIBLE)) return true;
    if(n->user) gtk_tree_iter_free((GtkTreeIter*)n->user);
}

bool NotifyFunc(Node *n,int flags,void *pv);
void BuildStore(NodeRoot *r)
{
    // if rebuilding free the old tree first
    if(r)
    {
	r->_Enumerate(FreeFunc,NS_ALL,0);
	delete r;
    }

    // clear store
    gtk_tree_store_clear(store);

    // add nodes to the store
    GtkTreeIter citer;
    TreeUserData tud; tud.p = 0; tud.c = &citer; tud.ts=store;
    enumFunc(nr,0,&tud); // call the callback directly
   
    // expand the root node
    GtkTreePath *tp = gtk_tree_path_new_first();
    gtk_tree_view_expand_row(GTK_TREE_VIEW(tree),tp,FALSE);
    gtk_tree_path_free(tp);
 
    // enable notifications
    nr->SetNotify(NotifyFunc);
}

//---------------------------------------------------------------------------------------
// Tree <--> backend interfacing

struct TreeExpData
{
    GtkTreeView *tv;
    GtkTreeModel *tm;
};

bool ExpanderFunc(Node *n,int flags,void *pv)
{
    TreeExpData *ted = (TreeExpData*)pv;
    if(n->GetType() & NTT_PARENT
	&& (n->GetType() != NT_MENU || ted->tv == GTK_TREE_VIEW(deptree))
	&& n->user)
    {
	if(n->GetType() == NT_CHOICEP && ted->tv == GTK_TREE_VIEW(deptree)) return 1;
	    
	GtkTreeIter *ti = (GtkTreeIter*)n->user;
	GtkTreePath *p = gtk_tree_model_get_path(ted->tm,ti);
	gtk_tree_view_expand_row(ted->tv,p,0);
	gtk_tree_path_free(p);

	n->Enumerate(ExpanderFunc,NS_T_ALL,pv);
    }
    return 1;
}

// for lkc26 input parents
void tree_row_expanded(GtkTreeView *tv,GtkTreeIter *ti,GtkTreePath *tp,gpointer ud)
{
    Node *n=0;
    GtkTreeModel *tm;
    if(tv == GTK_TREE_VIEW(tree))
	tm = GTK_TREE_MODEL(store); else
	tm = GTK_TREE_MODEL(depstore);
	
    gtk_tree_model_get (tm, ti, NODE_COLUMN, &n, -1);

    TreeExpData ted; ted.tv=tv; ted.tm=tm;
    if(n) n->Enumerate(ExpanderFunc,NS_T_ALL,&ted);
}
 
void ShowHelp(Node *n);
static void tree_selection_changed_cb (GtkTreeSelection *selection, gpointer data)
{
    static Node *OldN=0;
    static GtkTreeModel *omodel;
    GtkTreeIter iter,oiter;
    GtkTreeModel *model;
    gpointer ptr;

    if (gtk_tree_selection_get_selected (selection, &model, &iter))
    {
        gtk_tree_model_get (model, &iter, NODE_COLUMN, &ptr, -1);
	Node *n = (Node*)ptr;
	if(OldN == n)
	{
	    if(n->GetType() & NTT_STR)
	    {
		printf("CANNOT / should not ever reach here\n");
		//n->Set((int)"300");
	    }else{
	        n->Advance();
	    }
	}else
	{
	    if(OldN && OldN->GetType() & NTT_STR)
	    {
		GtkTreeIter *it = (GtkTreeIter*)(OldN->user);
		NotifyFunc(OldN,NS_STATE,OldN->user); // reset it properly
		//gtk_tree_store_set (GTK_TREE_STORE(omodel), it, PROMPT_COLUMN, OldN->GetPrompt(), -1);
		gtk_tree_store_set (GTK_TREE_STORE(omodel), it, EDITABLE_COLUMN, 0, -1);
	    }
	    if(n && (n->GetType() & NTT_STR))
	    {
		if(n->Get() > 3)
		    gtk_tree_store_set (GTK_TREE_STORE(model), &iter, PROMPT_COLUMN, n->Get(), -1);
		gtk_tree_store_set (GTK_TREE_STORE(model), &iter, EDITABLE_COLUMN, 1, -1);
	    }
	    OldN = n;
	    omodel = model;
	}

	ShowHelp(n);

	// ONLY do the deps for the left tree
	if(deptree == GTK_WIDGET(gtk_tree_selection_get_tree_view(selection))) return;  

	// --- BUILD THE DEPLIST ---
        // if rebuilding free the old iterators first
        if(nLastDep) nLastDep->_Enumerate(FreeFunc,NS_ALL,0);
    
        // clear store
        gtk_tree_store_clear(depstore);

        // add nodes to the store
        GtkTreeIter citer;
        TreeUserData tud; tud.p = 0; tud.c = &citer; tud.ts=depstore;
        nLastDep = n->GetDepTree();
        if(nLastDep) enumFunc(nLastDep,0,&tud); // call the callback directly

	// update it
	nLastDep->Update(1);

        // expand the root node
        GtkTreePath *tp = gtk_tree_path_new_first();
        gtk_tree_view_expand_row(GTK_TREE_VIEW(deptree),tp,FALSE);
        gtk_tree_path_free(tp);
    }
}

void Edited(GtkCellRendererText *gcrt,gchar *path_string,gchar *new_text,gpointer data)
{
    Node *n;
    GtkTreeModel *model = GTK_TREE_MODEL (data);

    GtkTreeIter iter;
    GtkTreePath *path = gtk_tree_path_new_from_string (path_string);
    gtk_tree_model_get_iter (model, &iter, path);
    gtk_tree_model_get (model, &iter, NODE_COLUMN, &n, -1);
    gtk_tree_path_free (path);

    if(!(n->GetType() & NTT_STR)) { printf("Edited: Node is not of type string!\n"); return; }

    // set the new value
    n->Set((int)new_text);
    gtk_tree_store_set (GTK_TREE_STORE(model), &iter, PROMPT_COLUMN, new_text, -1);
    ShowHelp(n);
}


void ExpandPath(GtkTreePath *gtp,GtkTreeView *gtv)
{
    GtkTreePath *p = gtk_tree_path_copy(gtp);
    
    if(gtk_tree_path_up(p))
    {
	ExpandPath(p,gtv);
	gtk_tree_view_expand_row(GTK_TREE_VIEW(gtv),p,0);
    }
    gtk_tree_path_free(p);
}

bool NotifyFunc(Node *n,int flags,void *pv)
{
    char c,buf[200],*color;
    GtkWidget *tview = tree;
    GtkTreeStore *gts = store;
    GtkTreeIter *iter = (GtkTreeIter*)n->user;
    if(!iter) return true;

    // use the proper store or else....
    if(n->GetParent(NT_DLIST) == nLastDep || n == nLastDep) { gts = depstore; tview = deptree; }

    switch(flags)
    {
    case NS_EXPAND: {
	GtkTreePath *p = gtk_tree_model_get_path(GTK_TREE_MODEL(gts),iter);
	gtk_tree_view_expand_row(GTK_TREE_VIEW(tview),p,0);
	gtk_tree_path_free(p);
	return true;
	}
    case NS_COLLAPSE: {
	GtkTreePath *p = gtk_tree_model_get_path(GTK_TREE_MODEL(gts),iter);
	gtk_tree_view_collapse_row(GTK_TREE_VIEW(tview),p);
	gtk_tree_path_free(p);
	return true;
	}
    case NS_SELECT: {
	GtkTreePath *p = gtk_tree_model_get_path(GTK_TREE_MODEL(gts),iter);
	ExpandPath(p,GTK_TREE_VIEW(tview));
	GtkTreeSelection *gtse = gtk_tree_view_get_selection(GTK_TREE_VIEW(tview));
	gtk_tree_view_set_cursor(GTK_TREE_VIEW(tview),p,0,0);
	gtk_tree_selection_select_path(gtse,p);
        gtk_tree_path_free(p);
	}
    }
    SetPromptState(gts,n);
    return true;
}

// Tree <--> backend interfacing
//---------------------------------------------------------------------------------------
// help & text browsing

/// File
bool bFileNP=0;
int iFileStrs=-1;
char FileStrs[10][255];
int FileLines[10];
Node *HelpNode=0;

bool ShowFile(char *fn,int line,int lfrom=0);
    
void FileInit()
{
    if(bFileNP) { bFileNP=0; return; }
    iFileStrs = -1;
    for(int i=0; i<10; i++)
	FileStrs[i][0] = 0;
}

void FilePush(char *s,int l,int lfrom=0)
{
    if(lfrom) FileLines[iFileStrs] = lfrom; // set the from line
	    
    if(bFileNP) { bFileNP=0; return; }
    if(++iFileStrs > 9) { iFileStrs = 10; return; }
    FileLines[iFileStrs] = l;
    strcpy(FileStrs[iFileStrs],s);
    for(int i=iFileStrs+1; i<10; i++) FileStrs[i][0]=0; // clear
}

void FileNext()
{
    if(++iFileStrs < 9 && FileStrs[iFileStrs][0]) {
	bFileNP=1;
	ShowFile(FileStrs[iFileStrs],FileLines[iFileStrs]);
    }else{
	if(iFileStrs > 9) iFileStrs = 9; else
	    if(!FileStrs[iFileStrs][0]) iFileStrs--;
    }
}

void FilePrev()
{
    if(--iFileStrs > -1) {
	bFileNP=1;
    	ShowFile(FileStrs[iFileStrs],FileLines[iFileStrs]);
    } else {
	if(iFileStrs < -1) iFileStrs = -1;
	if(HelpNode) {
	    bFileNP=1;

	    bool t = bShowFile; bShowFile = 0;
	    ShowHelp(HelpNode);
	    bShowFile = t;
	}
    }
}
/// File

void ColorLinks(GtkTextBuffer *buf)
{
    GtkTextTagTable *tt = gtk_text_buffer_get_tag_table(buf);
    
    GtkTextTag *tag = gtk_text_tag_table_lookup(tt,"blue-foreground");
    if(!tag)
    {
	tag = gtk_text_buffer_create_tag(buf,"blue-foreground","foreground","blue",0);
	gtk_text_buffer_create_tag(buf,"purple-foreground","foreground","purple",0);
    }

    int max = gtk_text_buffer_get_line_count(buf)-1;

    for(int i=0; i<max; i++)
    {
	GtkTextIter si,ei;
	gtk_text_buffer_get_iter_at_line(buf,&si,i);
	gtk_text_buffer_get_iter_at_line(buf,&ei,i+1);

	char *s,*e,*c;
	e = c = gtk_text_buffer_get_text(buf,&si,&ei,false);

        while(nr->GetLink(e,-1,&s,&e))
	{
	    gtk_text_buffer_get_iter_at_line_offset(buf,&si,i,s-c);
	    gtk_text_buffer_get_iter_at_line_offset(buf,&ei,i,e-c);
	    gtk_text_buffer_apply_tag(buf,tag,&si,&ei);
	}
	g_free(c);	
    }
}

bool ShowText(char *pp,int line)
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textv));
    gtk_text_buffer_set_text(buffer,pp,-1);
    ColorLinks(buffer);

    // highligh the line
    GtkTextIter si,ei;
    gtk_text_buffer_get_iter_at_line(buffer,&si,line-1 < 0 ? 0 : line - 1 );
    gtk_text_buffer_get_iter_at_line(buffer,&ei,line);
    gtk_text_buffer_apply_tag_by_name(buffer,"purple-foreground",&si,&ei);

    // get selected line visible on the screen
    static GtkTextMark *gtm=0;
    if(!gtm) gtm = gtk_text_buffer_create_mark(buffer,"cursor-loc-y",&ei,true);
	else gtk_text_buffer_move_mark(buffer,gtm,&ei);

    gtk_text_buffer_place_cursor(buffer,&si);
    gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(textv),gtm,0.4,1,0,0.5);
    return 1;
}

bool ShowFile(char *fn,int line,int lfrom)
{
    char *pp = nr->GetFileH(fn);
    if(!pp) return 0;
    FilePush(fn,line,lfrom);
    ShowText(pp,line);
    return 1;
}

gboolean KeyOverride(GtkWidget *tv,GdkEvent *e,gpointer ud)
{
    
    switch(e->key.keyval)
    {
    case GDK_Left:
	if(e->type == GDK_KEY_PRESS)
	{
	    GtkTreeModel *gtm;
	    GtkTreeIter gti;
	    GtkTreeSelection *gts = gtk_tree_view_get_selection(GTK_TREE_VIEW(tv));
	    if (gtk_tree_selection_get_selected (gts, &gtm, &gti))
	    {
		GtkTreePath *gtp = gtk_tree_model_get_path(GTK_TREE_MODEL(store),&gti);
		if(gtk_tree_view_row_expanded(GTK_TREE_VIEW(tv),gtp))
		{
		    gtk_tree_view_collapse_row(GTK_TREE_VIEW(tv),gtp);
		}else{
	            gtk_tree_path_up(gtp);
				gtk_tree_view_set_cursor(GTK_TREE_VIEW(tv),gtp,0,0);
	            gtk_tree_selection_select_path(gts,gtp);
		}
		gtk_tree_path_free(gtp);
	    }
	}
	return 1;
    case GDK_Right:
	if(e->type == GDK_KEY_PRESS)
	{
	    GtkTreeModel *gtm;
	    GtkTreeIter gti;
	    GtkTreeSelection *gts = gtk_tree_view_get_selection(GTK_TREE_VIEW(tv));
	    if (gtk_tree_selection_get_selected (gts, &gtm, &gti))
	    {
		GtkTreePath *gtp = gtk_tree_model_get_path(GTK_TREE_MODEL(store),&gti);
		if(!gtk_tree_view_row_expanded(GTK_TREE_VIEW(tv),gtp))
		{
        	    gtk_tree_view_expand_row(GTK_TREE_VIEW(tv),gtp,0);
		}else{
		    gtk_tree_path_down(gtp);
		    gtk_tree_view_set_cursor(GTK_TREE_VIEW(tv),gtp,0,0);
		    gtk_tree_selection_select_path(gts,gtp);
		}
		gtk_tree_path_free(gtp);
	    }          
	}
	return 1;
	
    default:
	return 0;
    }
	    
    return 0;
}

gboolean CheckForLink(GtkWidget *tv,GdkEvent *event1,gpointer ud)
{
    if(event1->type == GDK_KEY_RELEASE)
    {
	GdkEventKey *key = (GdkEventKey*)event1;
	switch(key->keyval)
	{
	case GDK_BackSpace:
	case GDK_Escape:
	case GDK_Left:
	case GDK_KP_Left:
	    FilePrev();
	    break;
	case GDK_space:
	case GDK_Return:
	case GDK_Right:
	case GDK_KP_Right:
	    FileNext();
	    break;
	}
	return 0;
    }
    if(event1->type != GDK_BUTTON_RELEASE) return 0;
    GdkEventButton *event = (GdkEventButton*)event1;
    
    // get the position
    int x,y;
    GtkTextIter iter;
    gtk_text_view_window_to_buffer_coords(GTK_TEXT_VIEW(tv),GTK_TEXT_WINDOW_WIDGET,
					    (int)event->x,(int)event->y,&x,&y);
    gtk_text_view_get_iter_at_location(GTK_TEXT_VIEW(tv),&iter,x,y);

    gint line = gtk_text_iter_get_line(&iter);
    gint index = gtk_text_iter_get_visible_line_index(&iter);

    // get the line text
    GtkTextIter si,ei;
    GtkTextBuffer *buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textv));
    gtk_text_buffer_get_iter_at_line(buf,&si,line);
    gtk_text_buffer_get_iter_at_line(buf,&ei,line+1);
    int l;
    char *p,*c,*fn;
    c = gtk_text_buffer_get_text(buf,&si,&ei,false);

    // check for link
    if(!(p = nr->GetLinkFileH(c,index,0,0,&l,&fn))) return 0;
    FilePush(fn,l,line+1);
    ShowText(p,l);
    g_free(c);
    return 0;
}

void ShowHelp(Node *n)
{
    FileInit();
    HelpNode = n;

    // show the file instead ?
    if(bShowFile && ShowFile(n->GetSource(),n->GetLine())) return;
    
    char *p = nr->GetHelpH(n);
    if(!p) return;
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textv));
    gtk_text_buffer_set_text(buffer,p,-1);
    ColorLinks(buffer);
}

// help & text browsing
//---------------------------------------------------------------------------------------
// menu functions
// file menu

void FileDialog(const gchar *title, GCallback Func)
{
   GtkWidget *file_selector = gtk_file_selection_new(title);
   
   g_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(file_selector)->ok_button),
                     "clicked", Func, file_selector);
                           
   g_signal_connect_swapped(GTK_OBJECT(GTK_FILE_SELECTION(file_selector)->ok_button),
                             "clicked",G_CALLBACK (gtk_widget_destroy), (gpointer)file_selector); 

   g_signal_connect_swapped(GTK_OBJECT(GTK_FILE_SELECTION(file_selector)->cancel_button),
                             "clicked", G_CALLBACK(gtk_widget_destroy), (gpointer)file_selector); 
   
   gtk_widget_show(file_selector);
}

void BuildArchMenu(GtkWidget *menu);
void PathFileName(GtkButton *button, gpointer user_data)
{
    // get selected path here
    const gchar *selected_filename;
    selected_filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (user_data));
    g_print("Path filename: %s\n", selected_filename);

    // load new path
    NodeRoot *r = nr; // save nr to delete it properly
    nr = new NodeRoot;
    if(nr->Init(r->GetArch(),selected_filename) & 3)
    {
	delete nr; nr = r;

	GtkWidget *d = gtk_message_dialog_new(GTK_WINDOW(app),
	GTK_DIALOG_MODAL,GTK_MESSAGE_ERROR,GTK_BUTTONS_OK,"Invalid path");
        gtk_dialog_run(GTK_DIALOG(d));
	gtk_widget_destroy(d);

	printf("reload after path fail\n");
	return;
    }else
    {
        // rebuild treestore
	BuildStore(r); // r is freed in here
   
	// rebuild the arch menu
        BuildArchMenu(ArchMenu);
    }
}

void OpenFileName(GtkButton *button, gpointer user_data)
{
    const gchar *selected_filename;
    selected_filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (user_data));
    g_print ("Open filename: %s\n", selected_filename);
    nr->Load(selected_filename);
}

void SaveAsFileName(GtkButton *button, gpointer user_data)
{
    const gchar *selected_filename;
    selected_filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (user_data));
    g_print ("Save filename: %s\n", selected_filename);
    nr->Save(selected_filename);
}

void LoadFunc(GtkMenuItem *mi,gpointer ud) { nr->Load(0); }
void SaveFunc(GtkMenuItem *mi,gpointer ud) { nr->Save(0); }
void ClearFunc(GtkMenuItem *mi,gpointer ud){ nr->Load("");}
void PathFunc(GtkMenuItem *mi,gpointer ud)
    { FileDialog("Set Path:", G_CALLBACK(PathFileName)); }
void OpenFunc(GtkMenuItem *mi,gpointer ud)
    { FileDialog("Open: Config File", G_CALLBACK(OpenFileName)); }
void SaveAsFunc(GtkMenuItem *mi,gpointer ud)
    { FileDialog("Save as:", G_CALLBACK(SaveAsFileName)); }

//-------------------------------------------------------------------
// view menu

bool UpdateFunc(Node *n,int flags,void *pv)
{
    if(!(n->GetType() & NTT_VISIBLE)) return true;

    GtkTreeIter *iter = (GtkTreeIter*)n->user; if(!iter) return 1;
    if(flags & NS_SKIPPED)  gtk_tree_store_set(store, iter, VISIBLE_COLUMN, bShowSkipped, -1); else
    if(flags & NS_DISABLED) gtk_tree_store_set(store, iter, VISIBLE_COLUMN, bShowDisabled, -1);

    return 1;
}

void DisabledFunc(GtkCheckMenuItem *mi,gpointer ud)
{
    bShowDisabled = gtk_check_menu_item_get_active(mi);
    nr->Enumerate(UpdateFunc,NS_ALL,0);
}

void SkippedFunc(GtkCheckMenuItem *mi,gpointer ud)
{
    bShowSkipped = gtk_check_menu_item_get_active(mi);
    nr->Enumerate(UpdateFunc,NS_ALL,0);
}

gint vpos=0;

void DependenciesFunc(GtkCheckMenuItem *mi,gpointer ud)
{
    GtkPaned *pd = GTK_PANED(gtk_widget_get_parent(
			     gtk_widget_get_parent(deptree)));
    if(!pd) return;
    
    if(gtk_check_menu_item_get_active(mi))
    {
	//gtk_widget_show(deptree);
	gtk_paned_set_position(pd,vpos);
    }else
    {
	//gtk_widget_hide(deptree);
	vpos = gtk_paned_get_position(pd);
	gtk_paned_set_position(pd,0);
    }
}


void HelpFileFunc(GtkCheckMenuItem *mi,gpointer ud)
{
    bShowFile = !gtk_check_menu_item_get_active(mi);
}

//-------------------------------------------------------------------
// arch menu

void ArchFunc(GtkCheckMenuItem *mi,gpointer ud)
{
    const gchar *c = gtk_label_get_text(GTK_LABEL(gtk_bin_get_child(GTK_BIN(mi)))) ;
   
    if(mi->active && strcmp(nr->GetArch(),c)) // did it change ?
    {
	//printf("archfunc  = %s,%s\n",c,nr->GetArch() );
	
	// load new arch 
	NodeRoot *r = nr; // save nr to delete it properly
	nr = new NodeRoot;
        if(nr->Init(c,r->GetPath()) & 3)
        {
	    delete nr; nr = r;
	    printf("reload after arch fail\n");
	    return;
	}else
        {
	    // rebuild treestore
	    BuildStore(r);
        }
    }
}

//---------------------------------------------------------------------------------------
// help menu

bool MatchStr(const char *a,const char *b)
{
	if(!a || !b) return 0;

	int la = strlen(a); // search for -- b in a --
	int lb = strlen(b);
	int lc = la-lb;

	if(lb > la) return 0;

	for(int i=0; i<=lc; i++)
		if(!strncmp(b,a+i,lb)) {
			//printf("match %s to %s\n",b,a);
			return 1;
		}
	
	return 0;
}

Node *sn=0;

bool PromptSf(Node *n,void *pv)
{
	//printf("Prompt search %s\n",pv);
	
	if(MatchStr(n->GetPrompt(),(char*)pv)) {
		n->Select(); sn=n;
		return 1;
	}
	return 0;
}

bool ConfigSf(Node *n,void *pv)
{
	//printf("Config search %s\n",pv);

	if(MatchStr(n->GetSymbol(),(char*)pv)) {
		n->Select(); sn=n;
		return 1;
	}
	return 0;
}

void SearchDlgRes(GtkButton *btn,gpointer a1) //GtkDialog *d,gint a1,gpointer ud)
{
	if(!sn) sn=nr;

	void *pv = g_object_get_data(G_OBJECT(btn),"txtctl"); 
	GtkEntry *ge = pv ? GTK_ENTRY(pv) : 0;
	const gchar* txt = ge ? gtk_entry_get_text(ge) : "";
	
	switch((gint)a1)
	{
	case 1: if(!sn->Search(PromptSf,(char*)txt)) sn=nr; break; //prompt
	case 2: if(!sn->Search(ConfigSf,(char*)txt)) sn=nr; break; //CONFIG_
	case 3:										 sn=nr;	break; //reset
	default:
	case GTK_RESPONSE_ACCEPT:	return;
	}
}

void SearchFunc(GtkMenuItem *mi,gpointer ud)
{
	static char bb[105]="";
	
	GtkWidget *d = gtk_dialog_new_with_buttons("Search",GTK_WINDOW(app),
						GTK_DIALOG_DESTROY_WITH_PARENT,0);

	GtkWidget *te = gtk_entry_new_with_max_length(100);
	gtk_widget_show(te);	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(d)->vbox),te,0,0,0);

	GtkBox *box = GTK_BOX(GTK_DIALOG(d)->action_area);
	GtkWidget *b1 = gtk_button_new_with_mnemonic("_Prompt");
		gtk_widget_show(b1);	gtk_box_pack_end(box,b1,0,0,0);
		g_signal_connect (G_OBJECT(b1),"clicked",G_CALLBACK (SearchDlgRes),(gpointer)1);
		g_object_set_data(G_OBJECT(b1),"txtctl",te);
	GtkWidget *b2 = gtk_button_new_with_mnemonic("_CONFIG_");
		gtk_widget_show(b2);	gtk_box_pack_end(box,b2,0,0,0);
		g_signal_connect (G_OBJECT(b2),"clicked",G_CALLBACK (SearchDlgRes),(gpointer)2);
		g_object_set_data(G_OBJECT(b2),"txtctl",te);
	GtkWidget *b3 = gtk_button_new_with_mnemonic("_Reset");
		gtk_widget_show(b3);	gtk_box_pack_end(box,b3,0,0,0);
		g_signal_connect (G_OBJECT(b3),"clicked",G_CALLBACK (SearchDlgRes),(gpointer)3);
	gtk_dialog_add_button(GTK_DIALOG(d),GTK_STOCK_CLOSE,GTK_RESPONSE_ACCEPT);

	gtk_entry_set_text(GTK_ENTRY(te),bb);
	gtk_dialog_run(GTK_DIALOG(d));
	strcpy(bb,gtk_entry_get_text(GTK_ENTRY(te)));
	
    gtk_widget_destroy(d);

}

void AboutFunc(GtkMenuItem *mi,gpointer ud)
{
    GtkWidget *d = gtk_message_dialog_new(GTK_WINDOW(app),
	    GTK_DIALOG_MODAL,GTK_MESSAGE_INFO,GTK_BUTTONS_CLOSE,CopyRight);
    gtk_dialog_run(GTK_DIALOG(d));
    gtk_widget_destroy(d);
}
    
//---------------------------------------------------------------------------------------
// menu construction

gboolean TestExit(GtkWidget *w,GdkEvent *e,gpointer ud)
{
    if(!nr->Modified()) { gtk_main_quit(); return 0; }

    GtkWidget *d = gtk_message_dialog_new(GTK_WINDOW(app),
    GTK_DIALOG_MODAL,GTK_MESSAGE_WARNING,GTK_BUTTONS_YES_NO,"Save before exit ?");
    gtk_dialog_add_button(GTK_DIALOG(d),"_Cancel",GTK_RESPONSE_CANCEL);
    gint i = gtk_dialog_run(GTK_DIALOG(d));
    gtk_widget_destroy(d);

    switch(i) {
    case GTK_RESPONSE_YES: nr->Save(0);
    case GTK_RESPONSE_NO:
	gtk_widget_destroy(GTK_WIDGET(app));
	gtk_main_quit();
	return 0;
    }
    return 1;
}
void ExitFunc(GtkMenuItem *mi,gpointer ud) { TestExit(0,0,0); }

void DeleteWidgets(GtkWidget *w,gpointer p)
{
    GtkContainer *c = GTK_CONTAINER(p);
    gtk_container_remove(c,w);
}

void BuildArchMenu(GtkWidget *menu)
{
    // delete arch menu items
    GtkContainer *c = GTK_CONTAINER(menu);
    gtk_container_foreach(c,DeleteWidgets,c);

    char *Arch = nr->GetArch();
    GSList *rg = 0;

    for(const char *p = nr->GetFirstArch(); p; p = nr->GetNextArch())
    {
	GtkWidget *mi = gtk_radio_menu_item_new_with_label(rg,p);
	gtk_widget_show(GTK_WIDGET(mi));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu),mi);
	gtk_signal_connect(GTK_OBJECT(mi),"toggled",GTK_SIGNAL_FUNC(ArchFunc),0);
	rg = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(mi));
	if(!strcmp(Arch,p)) gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(mi),TRUE);
    }
}

void AddMenuItem(GtkWidget *menu,const gchar *name,const gchar *sig,GtkSignalFunc func)
{
    GtkWidget *mi = gtk_menu_item_new_with_mnemonic(name);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu),mi);
    gtk_signal_connect(GTK_OBJECT(mi),sig,func,0);
}

void AddCheckMenuItem(GtkWidget *menu,const gchar *name,const gchar *sig,GtkSignalFunc func,gboolean act)
{
    GtkWidget *mi = gtk_check_menu_item_new_with_mnemonic(name);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu),mi);
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(mi),act);
    gtk_signal_connect(GTK_OBJECT(mi),sig,func,0);
}

GtkWidget *BuildMenus()
{
    // menu bar
    GtkWidget *menu,*mi,*mbar = gtk_menu_bar_new();

    // file menu
    mi = gtk_menu_item_new_with_mnemonic("_File");
    gtk_menu_shell_append(GTK_MENU_SHELL(mbar),mi);
    menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi),menu);
    AddMenuItem(menu,"_Load",	"activate",GTK_SIGNAL_FUNC(LoadFunc));
    AddMenuItem(menu,"Sa_ve",	"activate",GTK_SIGNAL_FUNC(SaveFunc));
    AddMenuItem(menu,"_Clear",	"activate",GTK_SIGNAL_FUNC(ClearFunc));
    AddMenuItem(menu,"Set _Path","activate",GTK_SIGNAL_FUNC(PathFunc));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu),gtk_separator_menu_item_new()); //----
    AddMenuItem(menu,"_Open",	"activate",GTK_SIGNAL_FUNC(OpenFunc));
    AddMenuItem(menu,"_Save as","activate",GTK_SIGNAL_FUNC(SaveAsFunc));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu),gtk_separator_menu_item_new()); //----
    AddMenuItem(menu,"E_xit",	"activate",GTK_SIGNAL_FUNC(ExitFunc));
    
    // view menu
    mi = gtk_menu_item_new_with_mnemonic("_View");
    gtk_menu_shell_append(GTK_MENU_SHELL(mbar),mi);
    menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi),menu);
    AddCheckMenuItem(menu,"_Disabled",	"toggled",GTK_SIGNAL_FUNC(DisabledFunc),bShowDisabled);
    AddCheckMenuItem(menu,"_Skipped",	"toggled",GTK_SIGNAL_FUNC(SkippedFunc),bShowSkipped);
    AddCheckMenuItem(menu,"De_pendencies","toggled",GTK_SIGNAL_FUNC(DependenciesFunc),1);
    AddCheckMenuItem(menu,"_Help/File","toggled",GTK_SIGNAL_FUNC(HelpFileFunc),!bShowFile);
    
    // arch menu
    mi = gtk_menu_item_new_with_mnemonic("_Arch");
    gtk_menu_shell_append(GTK_MENU_SHELL(mbar),mi);
    ArchMenu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi),ArchMenu);
    BuildArchMenu(ArchMenu);

    // help menu
    mi = gtk_menu_item_new_with_mnemonic("_Help");
    gtk_menu_shell_append(GTK_MENU_SHELL(mbar),mi);
    menu = gtk_menu_new();
    gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi),menu);
    AddMenuItem(menu,"_Search",	"activate",GTK_SIGNAL_FUNC(SearchFunc));
    AddMenuItem(menu,"_About",	"activate",GTK_SIGNAL_FUNC(AboutFunc));
    
    return mbar;   
}

// end menu functions
//---------------------------------------------------------------------------------------
/*
void TreeDataFunc(  GtkTreeViewColumn *tv,GtkCellRenderer *cr,
		    GtkTreeModel *tm,GtkTreeIter *ti,gpointer ud)
{
    Node *n;
    gtk_tree_model_get (tm, ti, NODE_COLUMN, &n, -1);
    if(n->GetState() & NS_SKIPPED) gtk_tree_model_iter_next(tm,ti);
}//*/

int main(int argc,char *argv[])
{
    int ret;
    FileInit();
    nr = new NodeRoot();
    if(ret = nr->Init_CmdLine(argc,argv) & 7)
    switch(ret)
    {
	case 1: printf("Invalid path.\n"); delete nr; return 1;
	case 2: printf("Invalid arch.\n"); delete nr; return 1;
	case 3: printf("Parsing failed.\n"); delete nr; return 1;
	default: delete nr; return 1;
    }    
    GdkColor dgray; dgray.pixel=0; dgray.red=45000; dgray.green=45000; dgray.blue=45000;
    GdkColor lgray; lgray.pixel=0; lgray.red=49000; lgray.green=49000; lgray.blue=49000;

// init gnome
    //gnome_init("test1","0.1",argc,argv);
    
    GnomeProgram *app1;
    app1 = gnome_program_init("gconfig","0.1",LIBGNOMEUI_MODULE,argc,argv,GNOME_PARAM_NONE);
    
    app = gnome_app_new("gconfig-0.1",nr->GetPrompt() );//"window title");
    gtk_signal_connect(GTK_OBJECT(app),"delete-event",GTK_SIGNAL_FUNC(TestExit),NULL);

// create the views
// create the tree and store 
    store = gtk_tree_store_new(N_COLUMNS,
	    G_TYPE_STRING,
	    G_TYPE_POINTER,
	    G_TYPE_BOOLEAN,
	    G_TYPE_STRING,
	    G_TYPE_BOOLEAN);
    tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree),FALSE);
    gtk_widget_modify_base(tree,GTK_STATE_NORMAL,&dgray);
    gtk_widget_modify_base(tree,GTK_STATE_SELECTED,&lgray);

    BuildStore(0);

   // create the column
    GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
    GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes
	("Nodes",renderer,
	 "text",PROMPT_COLUMN,
	 "visible",VISIBLE_COLUMN,
	 "foreground",COLOR_COLUMN,
	 "editable",EDITABLE_COLUMN,NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
//    gtk_tree_view_column_set_cell_data_func(column,renderer,TreeDataFunc,0,0);
    

    /* Setup the selection handler */
    g_signal_connect (G_OBJECT (tree),"key-press-event",G_CALLBACK (KeyOverride),0);
    g_signal_connect (G_OBJECT (tree),"key-release-event",G_CALLBACK (KeyOverride),0);
    g_signal_connect (G_OBJECT (renderer), "edited",G_CALLBACK (Edited), store);
    GtkTreeSelection *select;
    select = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
    gtk_tree_selection_set_mode (select, GTK_SELECTION_BROWSE);
    g_signal_connect (G_OBJECT (select), "changed",G_CALLBACK (tree_selection_changed_cb),NULL);
    g_signal_connect (G_OBJECT (tree),"row-expanded",G_CALLBACK(tree_row_expanded),0);

    // scrolled tree window
    GtkWidget *sw = gtk_scrolled_window_new(0,0);
    gtk_container_add(GTK_CONTAINER(sw),tree);

// create the dep tree and store 
    depstore = gtk_tree_store_new(N_COLUMNS,
	    G_TYPE_STRING,
	    G_TYPE_POINTER,
	    G_TYPE_BOOLEAN,
	    G_TYPE_STRING,
	    G_TYPE_BOOLEAN);
    deptree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(depstore));
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(deptree),FALSE);
    gtk_widget_modify_base(deptree,GTK_STATE_NORMAL,&dgray);
    gtk_widget_modify_base(deptree,GTK_STATE_SELECTED,&lgray);

    // create the dep column
    GtkCellRenderer *deprenderer = gtk_cell_renderer_text_new ();
    GtkTreeViewColumn *depcolumn = gtk_tree_view_column_new_with_attributes
	("Nodes",deprenderer,
	 "text",PROMPT_COLUMN,
	 "visible",VISIBLE_COLUMN,
	 "foreground",COLOR_COLUMN,
	 "editable",EDITABLE_COLUMN,NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (deptree), depcolumn);
    
    /* Setup dep the selection handler */
    g_signal_connect (G_OBJECT (deprenderer), "edited",G_CALLBACK (Edited), depstore);
    GtkTreeSelection *depselect;
    depselect = gtk_tree_view_get_selection(GTK_TREE_VIEW(deptree));
    gtk_tree_selection_set_mode (depselect, GTK_SELECTION_BROWSE);
    g_signal_connect (G_OBJECT (depselect), "changed",G_CALLBACK (tree_selection_changed_cb),NULL);
    g_signal_connect (G_OBJECT (deptree),"row-expanded",G_CALLBACK(tree_row_expanded),0);

    // scrolled deptree window
    GtkWidget *depsw = gtk_scrolled_window_new(0,0);
    gtk_container_add(GTK_CONTAINER(depsw),deptree);

// scrolled text window
    GtkWidget *textsw = gtk_scrolled_window_new(0,0);
    textv = gtk_text_view_new();
    gtk_container_add(GTK_CONTAINER(textsw),textv);
    gtk_widget_modify_base(textv,GTK_STATE_NORMAL,&dgray);
    gtk_widget_modify_base(textv,GTK_STATE_SELECTED,&lgray);
    g_signal_connect (G_OBJECT (textv),"event-after",G_CALLBACK (CheckForLink),0);

// insert the menu, tree and text views
    GtkWidget *mbar = BuildMenus();
    GtkWidget *mbox = gtk_vbox_new(FALSE,0);
    gtk_box_pack_start(GTK_BOX(mbox),mbar,0,0,0);

    GtkWidget *hpd = gtk_hpaned_new();
    gtk_box_pack_end(GTK_BOX(mbox),hpd,1,1,0);
    gtk_paned_pack1(GTK_PANED(hpd),sw,1,1);

    GtkWidget *vpd = gtk_vpaned_new();
    gtk_paned_pack1(GTK_PANED(vpd),depsw,1,1);
    gtk_paned_pack2(GTK_PANED(vpd),textsw,1,1);
    gtk_paned_set_position(GTK_PANED(vpd),150);
		
    gtk_paned_pack2(GTK_PANED(hpd),vpd,1,1);
    gtk_paned_set_position(GTK_PANED(hpd),300);
    
// start the app
    gnome_app_set_contents(GNOME_APP(app),mbox);//tree);
    gtk_window_set_default_size(GTK_WINDOW(app),800,600);
    gtk_widget_show_all(GTK_WIDGET(app));
    gtk_main();

    // free all allocated iterators
    nr->_Enumerate(FreeFunc,NS_ALL,0);
    // and the root node
    delete nr;
    
    return 0;
}
//---------------------------------------------------------------------------------------


