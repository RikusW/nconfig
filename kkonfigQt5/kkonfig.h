// vim: sts=4 ts=4 sw=4

#ifndef KKVIEW_H
#define KKVIEW_H

#include <qstring.h>
#include <qobject.h>
#include <QMainWindow>
#include <QTextBrowser>
#include <QTreeView>
#include <QTreeWidgetItem>
#include "kktextbrowser.h"
#include "../nodes.h"

//-----------------------------------------------------------------------------

class KKView : public QMainWindow
{
	Q_OBJECT

public:
	KKView( int ac = 0, char **av = 0, QWidget *parent = 0, const char *name = 0 );
	~KKView();

protected:
	void initFolders(const char *arch=0,const char *path=0,int ac=0,char **av=0);

public slots:
//	void Search(int id);
	void ShowDeps(QTreeWidgetItem*);
	void ShowHelp(QTreeWidgetItem*);
//	void closeEvent(QCloseEvent *e);

private slots:
	void fileLoad();
	void fileSave();
	void fileClear();
	void filePath();
	void fileOpen();
	void fileSaveAs();

	void viewDisabled();
	void viewSkipped();
	void viewHorizontal();
	void viewDependencies();
	void viewHelpFile();

	void archMenu(QAction*);
	void helpSearch();
	void helpAbout();
	void helpAboutQt();

	void itemClicked(QTreeWidgetItem *item, int column);
	void itemExpanded(QTreeWidgetItem *item);
	void itemCollapsed(QTreeWidgetItem *item);

protected:
	NodeRoot *nr;
	KKTextBrowser *kkbrowser;
	QTreeWidget *folders,*folders2;
};

//-----------------------------------------------------------------------------

class NodeListItem : public QTreeWidgetItem
{
public:
	NodeListItem( QTreeWidget *p, Node *n);
	NodeListItem( NodeListItem *parent, NodeListItem *after, Node *n );

	// non Qt
	Node *node;
	void SetIcon();

	void activate(int column);
//	void okRename(int col);
//	void paintCell(QPainter *p, const QColorGroup &cg, int c, int w, int a);
};

//-----------------------------------------------------------------------------

#endif



