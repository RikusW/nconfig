// vim: sts=4 ts=4 sw=4

#include <QTextBrowser>
#include "../nodes.h"

class KKTextBrowser : public QTextBrowser
{
	Q_OBJECT

public:
	KKTextBrowser(QWidget *p);
//	void keyPressEvent(QKeyEvent *e);
	void setNodeRoot(NodeRoot **p) { pnr = p; };
	void setNode(Node *n);

protected:
	Node *node;
	NodeRoot **pnr; //really needed ?

public slots:
	void setSource(const QUrl &url);

//	void linkTo(int,int);
//	void ShowText(char *,int);
//	void ShowFile(char *,int);
//	void ShowHelp(QTreeWidgetItem*);
};

