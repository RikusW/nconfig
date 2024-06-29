// vim: sts=4 ts=4 sw=4 cindent

#ifndef KKVIEW_H
#define KKVIEW_H

#include <qtextedit.h>
#include <qstring.h>
#include <qobject.h>
#include <qlistview.h>
#include <qvbox.h>

class Node;
class NodeRoot;
class HelpText;

//-----------------------------------------------------------------------------

class KKView : public QVBox
{
    Q_OBJECT

public:
    KKView( int ac = 0, char **av = 0, QWidget *parent = 0, const char *name = 0 );
    ~KKView();

protected:
    void initFolders(const char *arch=0,const char *path=0,int ac=0,char **av=0);

public slots:
    void Search(int id);
    void fileMenu(int id);
    void viewMenu(int id);
    void archMenu(int id);
    void helpMenu(int id);
    void ShowDeps( QListViewItem* );
    void closeEvent(QCloseEvent *e);

protected:
    NodeRoot *nr;
    HelpText *helptext;
    QListView *folders,*folders2;
};

//-----------------------------------------------------------------------------

class HelpText : public QTextEdit
{
    Q_OBJECT

public:
    HelpText(QWidget *p,const char *n) : QTextEdit(p,n)
    { bFileNP=0; iFileStrs=-1; HelpNode=0; setPaletteBackgroundColor(QColor(150, 150, 150)); };
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
    void ShowHelp( QListViewItem* );
};

//-----------------------------------------------------------------------------

class NodeListItem : public QListViewItem
{
public:
    NodeListItem( QListView *p, Node *n);
    NodeListItem( NodeListItem *parent,NodeListItem *after, Node *n );

    // non Qt
    Node *node;
    void SetIcon();

    void activate();
    void okRename(int col);
    void paintCell(QPainter *p,const QColorGroup &cg,int c,int w,int a);
};

class NodeView : public QListView
{
public:
	NodeView(QWidget *p=0,const char *n=0,WFlags f=0) : QListView(p,n,f)
	{
		QColor bcolor(150, 150, 150);
		setPaletteBackgroundColor(bcolor);

		cgDisabled = palette().active();
		cgSkipped  = palette().active();
		cgDisabled.setColor(QColorGroup::Text, palette().disabled().text());
		cgSkipped .setColor(QColorGroup::Text, QColor(200,0,200));
		cgDisabled.setColor(QColorGroup::Background, bcolor);
		cgSkipped .setColor(QColorGroup::Background, bcolor);

	};
	QColorGroup cgDisabled,cgSkipped;
};
//-----------------------------------------------------------------------------

#endif



