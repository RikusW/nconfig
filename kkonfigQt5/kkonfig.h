// vim: sts=4 ts=4 sw=4

#ifndef KKVIEW_H
#define KKVIEW_H

#include <qstring.h>
#include <qobject.h>
#include <QMainWindow>
#include <QTextBrowser>
#include <QTreeView>
#include <QTreeWidgetItem>

class Node;
class NodeRoot;
class HelpText;

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
	HelpText *helptext;
	QTreeWidget *folders,*folders2;
};

//-----------------------------------------------------------------------------

class HelpText : public QTextBrowser
{
	Q_OBJECT

public:
	HelpText(QWidget *p) : QTextBrowser(p)
	{
		bFileNP = 0;
		iFileStrs =- 1;
		HelpNode = 0;
	};
	void keyPressEvent(QKeyEvent *e);
	void setNodeRoot(NodeRoot **p) { pnr = p; };

protected:
	bool bFileNP;
	int iFileStrs;
	char FileStrs[10][255];
	int FileLines[10];
	Node *HelpNode;
	NodeRoot **pnr;

	void FileInit();
	void FilePush(char*,int,int);
	void FileNext(int=-1);
	void FilePrev(int=-1);

public slots:
	void linkTo(int,int);
	void ShowText(char *,int);
	void ShowFile(char *,int);
	void ShowHelp(QTreeWidgetItem*);
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



