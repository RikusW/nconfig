// vim: sts=4 ts=4 sw=4
const char *CopyRight =
"kkonfig is a Qt5 kernel configuration utility based on the nconfig backend.\n"
"Copyright (C) 2024 Rikus Wessels <rikusw at gmail dot com>\n"
"GNU GPL v2.0";

#include <qapplication.h>
#include <qmessagebox.h>
#include <qfiledialog.h>
#include <qsplitter.h>
#include <qmenubar.h>
#include <QVBoxLayout>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qbuttongroup.h>
#include <qlineedit.h>
//#include <qaccel.h>

#include "kkonfig.h"

#if 0
bool bShowFile = false;
bool bShowSkipped = false;
bool bShowDisabled = false;
#else
bool bShowFile = true;
bool bShowSkipped = true;
bool bShowDisabled = true;
#endif

QBrush brEnabled = QBrush(QColor(200, 200, 200)); //TODO replace with theme color
QBrush brDisabled = QBrush(QColor(100, 100, 100)); //TODO use disabled
QBrush brSkipped = QBrush(QColor(50, 0, 0));

int main( int argc, char **argv )
{
	QApplication a(argc,argv);

	KKView KKView(argc,argv);
	KKView.show();

	return a.exec();
}

QPixmap *IconNo  = 0;
QPixmap *IconMod = 0;
QPixmap *IconYes = 0;
QPixmap *IconStr = 0;
QPixmap *IconMenu = 0;
QPixmap *IconChoice = 0;
QPixmap *IconComment = 0;

static const char *xpm_tree[] = { //Temp fix
"16 16 2 1",
". c None",
"# c #c0c0c0",
".....##.........",
".....##.........",
".....##.........",
".....##.........",
".....##.........",
".....##.........",
".....##.........",
".....###########",
".....###########",
".....##.........",
".....##.........",
".....##.........",
".....##.........",
".....##.........",
".....##.........",
".....##........."};

QPixmap *IconTree = 0;

//-----------------------------------------------------------------------------
// NodeListItem

NodeListItem::NodeListItem(QTreeWidget *p, Node *n)
	: QTreeWidgetItem(p)
{
	node = n;
	n->user = this;
	setText(2, n->GetPrompt());
	SetIcon();
}

NodeListItem::NodeListItem(NodeListItem *parent, NodeListItem *after, Node *n)
	: QTreeWidgetItem(parent, after)
{
	node = n;
	n->user = this;
	setText(2, n->GetPrompt());
	SetIcon();
}

void NodeListItem::activate(int column)
{
	if (column == 1) {
		if (node->GetType() & NTT_STR) {
//			setRenameEnabled(0, true);
//			startRename(0);
		} else {
			node->Advance();
		}
	} else if (column == 0) {
		setExpanded(!isExpanded());
	}
}

void KKView::itemClicked(QTreeWidgetItem *item, int column)
{
	printf("col %i\n", column);
	((NodeListItem*)item)->activate(column);
}

void KKView::itemExpanded(QTreeWidgetItem *item)
{
	((NodeListItem*)item)->node->SetExpanded(true);
}

void KKView::itemCollapsed(QTreeWidgetItem *item)
{
	((NodeListItem*)item)->node->SetExpanded(false);
}

/*
void NodeListItem::okRename(int col)
{
	QTreeWidgetItem::okRename(col);

	// update Node
	char buf[100];
	strcpy(buf, text(0));
	node->Set((uintptr_t)buf);

	// Update the help
	listView()->setSelected(this, false);
	listView()->setSelected(this, true);
}
*/

void NodeListItem::SetIcon()
{
	if (node->GetType() & (NTT_INPUT | NTT_DEF)) {
		switch (node->Get()) {
		case 1:  setIcon(1, *IconNo);  break;
		case 2:  setIcon(1, *IconMod); break;
		case 3:  setIcon(1, *IconYes); break;
		default: setIcon(1, *IconStr); break;
		}
	} else {
		switch (node->GetType()) {
		case NT_ROOT:
		case NT_MENU:    setIcon(1, *IconMenu);    break;
		case NT_CHOICEP: setIcon(1, *IconChoice);  break;
		case NT_COMMENT | NTT_PARENT:
		case NT_COMMENT: setIcon(1, *IconComment); break;
		}
	}
	setIcon(0, *IconTree);

	if (node->GetState() & NS_SKIPPED) {
		setHidden(!(bShowSkipped && bShowDisabled));
		setForeground(2, brSkipped);
	} else {
		if (node->GetState() & NS_DISABLED) {
			setHidden(!bShowDisabled);
			setForeground(2, brDisabled);
		} else {
			setHidden(false);
			setForeground(2, brEnabled);
		}
	}

}

bool UpdateFunc(Node *n, int flags, void *pv)
{
	if (!(n->GetType() & NTT_VISIBLE)) {
		return 1;
	}
	NodeListItem *li = (NodeListItem*)n->user;
	pv = pv; //compiler shutup

	if (flags & NS_SKIPPED) {
		li->setHidden(!(bShowSkipped && bShowDisabled));
		li->setForeground(2, brSkipped);
	} else {
		if (flags & NS_DISABLED) {
			li->setHidden(!bShowDisabled);
			li->setForeground(2, brDisabled);
		}
	}
	return 1;
}

bool NotifyFunc(Node *n, int flags, void *pv)
{
	if (!flags) {
		printf("Notify: flags = 0 ???%s????\n", n->GetPrompt());
	}
	if (!(n->GetType() & NTT_VISIBLE)) {
		return 0;
	}
	NodeListItem *li = (NodeListItem*)pv;
	if (!li) {
		printf("Notify: usr=0 <%s> t<%0x>\n", n->GetPrompt(), n->GetType());
		return 0;
	}
	switch (flags) {
	//case NS_STATE: break;
	case NS_ENABLE:
	case NS_DISABLE:
	case NS_SKIP:
	case NS_UNSKIP:
		li->SetIcon();
		if (n->GetType() & NTT_PARENT) {
			n->Enumerate(UpdateFunc, NS_ALL, 0);
		}
		// due to a setVisible bug which cause Invisible children of a parent
		// to be shown when the parent is shown(changed), the above must be done... Qt.....
		// to see what I mean: hide skipped, show disabled then toggle
		// MTD support while RAM/ROM/Flash is expanded. -> skipped become visible
		return 1;//break;
	case NS_EXPAND:
		li->setExpanded(true);
		return 1;
	case NS_COLLAPSE:
		li->setExpanded(false);
		return 1;
	case NS_PROMPT:
		li->setText(2, n->GetPrompt());
		return 1;
	case NS_SELECT:	{
		QTreeWidgetItem *qi = li;
		while((qi = qi->parent())) {
			qi->setExpanded(true);
		}
		li->treeWidget()->setCurrentItem(li);
		li->treeWidget()->scrollToItem(li);
		puts("scrolltoitem");
		return 1;
		}
	}
	li->SetIcon();
	return 1;
}

//-----------------------------------------------------------------------------
// init app

enum
{
	mFileLoad = 1,
	mFileSave,
	mFileClear,
	mFileSetPath,
	mFileOpen,
	mFileSaveAs,

	mViewDisabled,
	mViewSkipped,
	mViewHorizontal,
	mViewDependencies,
	mViewHelpFile,

	mHelpSearch,
	mHelpAbout,
	mHelpAboutQt,
};

KKView::~KKView()
{
	delete nr;

	delete IconNo;
	delete IconMod;
	delete IconYes;
	delete IconStr;
	delete IconMenu;
	delete IconChoice;
	delete IconComment;
	delete IconTree;
}

extern const uchar _binary_icons32aa_png_start[];
extern const uchar _binary_icons32aa_png_end[];

KKView::KKView(int ac, char **av, QWidget *parent, const char *name)
{
	QPixmap icons;
	icons.loadFromData(_binary_icons32aa_png_start,_binary_icons32aa_png_end - _binary_icons32aa_png_start);
	IconNo  =     new QPixmap(icons.copy(0,   0, 32, 32));
	IconMod =     new QPixmap(icons.copy(0,  32, 32, 32));
	IconYes =     new QPixmap(icons.copy(0,  64, 32, 32));
	IconStr =     new QPixmap(icons.copy(0,  96, 32, 32));
	IconMenu =    new QPixmap(icons.copy(0, 128, 32, 32));
	IconChoice =  new QPixmap(icons.copy(0, 160, 32, 32));
	IconComment = new QPixmap(icons.copy(0, 192, 32, 32));

	IconTree = new QPixmap(xpm_tree); //temp

	setWindowTitle("Kernel Konfig");
	setMinimumSize(160, 160);
	resize( 1200, 800 );

// MENUS
	QMenu *mfile = menuBar()->addMenu("&File");
	mfile->addAction("&Load",	 this, SLOT(fileLoad())); //, CTRL + Key_L)
	mfile->addAction("&Save",	 this, SLOT(fileSave())); //, CTRL + Key_S)
	mfile->addAction("&Clear",	this, SLOT(fileClear())); //
	mfile->addAction("Set &Path", this, SLOT(filePath())); //
	mfile->addSeparator();
	mfile->addAction("&Open",	 this, SLOT(fileOpen())); //, CTRL + Key_O)
	mfile->addAction("Save &as",  this, SLOT(fileSaveAs())); //, CTRL + Key_A)
	mfile->addSeparator();
	mfile->addAction("E&xit",	 this, SLOT(close()));

	QMenu *mview = menuBar()->addMenu("&View");
//	mview->setCheckable(true);
	mview->addAction("&Disabled",	this, SLOT(viewDisabled())); //, CTRL + Key_D)bShowDisabled -> checked ?
	mview->addAction("&Skipped",	 this, SLOT(viewSkipped()) ); //, CTRL + Key_K)bShowSkipped
	mview->addAction("&Horizontal",  this, SLOT(viewHorizontal()));
	mview->addSeparator();
	mview->addAction("De&pendencies",this, SLOT(viewDependencies())); //, CTRL + Key_P)true
	mview->addAction("&Help/File",   this, SLOT(viewHelpFile())); //, CTRL + Key_H)bShowFile

	QMenu *march = menuBar()->addMenu("&Arch");
	march->addAction("i386");
	connect(march, SIGNAL(triggered(QAction*)), this, SLOT(archMenu(QAction*)));

	QMenu *mhelp = menuBar()->addMenu("&Help");
	mhelp->addAction("&Search",   this, SLOT(helpSearch())); //, CTRL + Key_F
	mhelp->addAction("&About",	this, SLOT(helpAbout()));
	mhelp->addAction("About &Qt", this, SLOT(helpAboutQt()));

// TREES & HELP
	QSplitter *qs = new QSplitter(Qt::Horizontal, this);
	folders = new QTreeWidget(qs);
	QSplitter *qs2 = new QSplitter(Qt::Vertical, qs);
	folders2 = new QTreeWidget(qs2);
	kkbrowser = new KKTextBrowser(qs2);

	setCentralWidget(qs);
	qs->setStretchFactor(0, 40);
	qs->setStretchFactor(1, 60);
	qs2->setStretchFactor(0, 30);
	qs2->setStretchFactor(1, 70);
	folders->setColumnCount(3);
	folders2->setColumnCount(3);
	folders ->headerItem()->setSizeHint(0, QSize(200, 20)); //TODO improve this
	folders2->headerItem()->setSizeHint(0, QSize(20, 20));
	folders ->headerItem()->setSizeHint(1, QSize(20, 20));
	folders2->headerItem()->setSizeHint(1, QSize(20, 20));
	folders ->headerItem()->setText(0, "");
	folders2->headerItem()->setText(0, "");
	folders ->headerItem()->setText(1, "Icon");
	folders2->headerItem()->setText(1, "Icon");
	folders ->headerItem()->setText(2, "Description");
	folders2->headerItem()->setText(2, "Description");

	// ---- fill tree here ----
	nr = 0;
	initFolders(0, 0, ac, av);
	kkbrowser->setNodeRoot(&nr);

	connect(folders,  SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
				this, SLOT(ShowDeps(QTreeWidgetItem*)));
	connect(folders2, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
				this, SLOT(ShowHelp(QTreeWidgetItem*)));

	connect(folders,  SIGNAL(itemClicked(QTreeWidgetItem*, int)),
				this,   SLOT(itemClicked(QTreeWidgetItem*, int)));
	connect(folders2, SIGNAL(itemClicked(QTreeWidgetItem*, int)),
				this,   SLOT(itemClicked(QTreeWidgetItem*, int)));

	connect(folders,  SIGNAL(itemExpanded(QTreeWidgetItem*)),
				this,   SLOT(itemExpanded(QTreeWidgetItem*)));
	connect(folders2, SIGNAL(itemExpanded(QTreeWidgetItem*)),
				this,   SLOT(itemExpanded(QTreeWidgetItem*)));

	connect(folders,  SIGNAL(itemCollapsed(QTreeWidgetItem*)),
				this,   SLOT(itemCollapsed(QTreeWidgetItem*)));
	connect(folders2, SIGNAL(itemCollapsed(QTreeWidgetItem*)),
				this,   SLOT(itemCollapsed(QTreeWidgetItem*)));
}
/*
void KKView::closeEvent(QCloseEvent *e)
{
	if (!nr->Modified()) {
		e->accept();
		return;
	}
	switch (QMessageBox::warning(this, "kkonfig", "Save before closing ?",
		QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel))
	{
	case QMessageBox::Yes:
		nr->Save(0);
	case QMessageBox::No:
		e->accept();
		break;
	case QMessageBox::Cancel:
		e->ignore();
		break;
	}
}
*/
//-----------------------------------------------------------------------------
// init tree

// hack to get the ordering RIGHT, because
// qt -insert- children thus reversing EVERYTHING.........
struct RW_UserData
{
	NodeListItem *parent, *last;
};

bool enumFunc(Node *n, int flags, void *pv)
{
	// debug
	if (flags & NS_EXIT) {
		printf("Unexpected NS_EXIT...\n");
		return 1;
	}
	if (n->user) {
		printf("enum !!! uservalue non zero !!! %s\n", n->GetPrompt());
		n->user = 0;
	}
	if (!(n->GetType() & NTT_VISIBLE)) {
		return 1;
	}
	RW_UserData *prwd = (RW_UserData*)pv;

	NodeListItem *li = new NodeListItem(prwd->parent, prwd->last, n);
	prwd->last = li;

	if (n->GetType() & NTT_PARENT) {
		switch (n->GetType())
		{
		case NT_MENU:
			if (n->GetParent(NT_ROOT)) {
				break;
			} else {
				goto def; // open in deptree
			}
		case NT_CHOICEP:
		//	if (!n->GetParent(NT_ROOT)) {
		//		break; // don't open in deptree
		//	}
def:	default: {}
			li->setExpanded(true);
		}
		RW_UserData rwd; rwd.parent=li;
		rwd.last=0;
		n->Enumerate(enumFunc, NS_T_ALL, &rwd);
	}
	return 1;
}

void KKView::initFolders(const char *arch, const char *path, int ac, char **av)
{
	NodeRoot *nnr = new NodeRoot();

	if (!nnr->Init("i386", "../../kernels/linux-2.6.0")) { //debug testing
		goto done;
	}
	delete nnr;
/*	int ret;
	if (ac & !nr) { // initial init ?
		if (!(ret = nnr->Init_CmdLine(ac, av) & 7)) {
			goto done;
		}
		nr = nnr;
	} else {
		if (!(ret = nnr->Init(arch, path) & 7)) {
			goto done;
		}
		delete nnr;
	}*/
	/*
	switch (ret) {
		case 1:
			fileMenu(mFileSetPath);
			break;
		case 2: {
			QMessageBox mb(this);
			mb.information(this, "Warning !", "Invalid Architecture selected.");
			goto fillarch;
		}
		case 3: {
			QMessageBox mb(this);
			mb.critical(this, "Warning !", "Parsing failed.");
			break;
		}
		case 4:
			break; // silent fail
	}
	*/
	printf("init fail...\n");
	return;
done:

//	if (nr) {
//		delete nr;
//	}
	nr = nnr;
	folders->clear();
/*
fillarch:
	QPopupMenu *pm = (QPopupMenu*)child("ArchMenu");	// fill the arch menu
	if (!pm) {
		printf("no archmenu\n");
		return;
	}
	pm->clear();

	QString Arch = (char *)nr->GetArch();
	int i = 1;
	for (const char *p = nr->GetFirstArch(); p; p = nr->GetNextArch(), i++) {
		pm->insertItem(p, i);
		pm->setCheckable(true);
		if (Arch == p) {
			pm->setItemChecked(i, true);
		}
	}
	if (ret) {
		return; // fill only the arch menu
	}
*/
	nr->SetNotify(NotifyFunc); 		// set notifications

	NodeListItem *li = new NodeListItem(folders, nr);	// add the root
	RW_UserData rwd;
	rwd.parent = li;
	rwd.last = 0;
	nr->Enumerate(enumFunc, NS_T_ALL, &rwd);		// and the rest
	li->setExpanded(true);
	folders->resizeColumnToContents(0);
	folders->resizeColumnToContents(1);
}

//-----------------------------------------------------------------------------
// help handling

NodeListItem *OldN = 0;

void KKView::ShowDeps( QTreeWidgetItem *li )
{
	if (!li) {
		return;
	}
	if (OldN && !OldN->node->GetParent(NT_ROOT)) {
		OldN = 0;
	}
	folders2->clear();
	NodeListItem *n = ( NodeListItem* )li;
	if (!n) {
		printf("showdeps n=0\n");
		return;
	}
	NodeDListP *nd = (NodeDListP*)n->node->GetDepTree();
	if (!nd) {
		printf("showdeps nd=0\n");
		return;
	}
	NodeListItem *l = new NodeListItem(folders2, nd);	// add the root
//	l->setText(2, "Dependencies");
	RW_UserData rwd;
	rwd.parent = l;
	rwd.last = 0;
	nd->Enumerate(enumFunc, NS_T_ALL, &rwd);		// and the rest
	l->setExpanded(true);
	folders2->resizeColumnToContents(0);
	folders2->resizeColumnToContents(1);
	nd->Update(1);

	ShowHelp(li);
}

void KKView::ShowHelp(QTreeWidgetItem *li)
{
	if (!li) {
		return;
	}
	NodeListItem *n = ( NodeListItem* )li;
	if (!n->node) {
		printf("sc: n->node==0\n");
		return;
	}
	// rename helper
	if (OldN != n && OldN) {
		if (OldN && OldN->node->GetType() & NTT_STR) {
			OldN->setText(2, OldN->node->GetPrompt());
//x			OldN->setRenameEnabled(0, false);
		}
		if (n && n->node->GetType() & NTT_STR && n->node->Get() > 3) {
			n->setText(2, (char*)n->node->Get());
//x			n->setRenameEnabled(0, true);
		}
	}
	OldN = n;

	kkbrowser->setNode(n->node);
}

//------------------------------------------------------------------------------
//file menu handlers

void KKView::fileLoad()
{
	//	nr->Load(0);
	puts("fileload");
}

void KKView::fileSave()
{
	//	nr->Save(0);
	puts("filesave");
}

void KKView::fileClear()
{
	//	nr->Load("");
	puts("fileclear");
}

void KKView::filePath()
{
	puts("filepath");
	//QFileDialog fd;
	//	fd.setMode(QFileDialog::DirectoryOnly);
	//	QString qs = fd.getExistingDirectory(0,this,"dirdialog","Set Kernel Path");
	//	initFolders(0,qs);
}

void KKView::fileOpen()
{
	puts("fileopen");
		QFileDialog fd;
//	  nr->Load(
		fd.getOpenFileName(this, "Open config file");//);
}

void KKView::fileSaveAs()
{
	puts("filesaveas");
		QFileDialog fd;
		//nr->Save(
		fd.getSaveFileName(this, "Save config file");//);
}

//------------------------------------------------------------------------------
//view menu handlers

void KKView::viewDisabled()
{
//	bShowDisabled = !bShowDisabled;
//	pm->setItemChecked(mViewDisabled,bShowDisabled);
//	pm->setItemChecked(mViewSkipped,bShowSkipped && bShowDisabled);
//	nr->Enumerate(UpdateFunc, NS_ALL, 0);
}

void KKView::viewSkipped()
{
//	bShowSkipped=!bShowSkipped;
//	pm->setItemChecked(mViewSkipped,bShowSkipped && bShowDisabled);
//	nr->Enumerate(UpdateFunc, NS_ALL, 0);
}

void KKView::viewHorizontal()
{
//	QSplitter *qs = (QSplitter*)child("Split1");
//	if(!qs) { printf("no split1\n"); break; }

//	if(qs->orientation() == Qt::Horizontal)
//	{
//	pm->changeItem(id,"Vertical");
//	qs->setOrientation(Qt::Vertical);
//	}else
//	{
//	pm->changeItem(id,"Horizontal");
//	qs->setOrientation(Qt::Horizontal);
//	}

}

void KKView::viewDependencies()
{
//	if(pm->isItemChecked(id))
//	{
//	folders2->hide();
//	pm->setItemChecked(id,false);
//	}else
//	{
//	folders2->show();
//	pm->setItemChecked(id,true);
//	}

}

void KKView::viewHelpFile()
{
//	bShowFile = !bShowFile;
//	pm->setItemChecked(id,bShowFile);
}

//------------------------------------------------------------------------------
//arch menu handlers

void KKView::archMenu(QAction *a)
{
	printf("arch action = %lx\n", (long unsigned int) a);
}

/*
void KKView::archMenu(int id)
{
	QPopupMenu *pm = (QPopupMenu*)child("ArchMenu");
	if (!pm) {
		printf("no archmenu\n");
		return;
	}
	QString str = nr->GetPath();
	initFolders(pm->text(id), str);
}

//------------------------------------------------------------------------------
//help menu handlers

bool MatchStr(const char *a, const char *b)
{
	if (!a || !b) {
		return 0;
	}
	int la = strlen(a); // search for -- b in a --
	int lb = strlen(b);
	int lc = la-lb;

	if (lb > la) {
		return 0;
	}
	for (int i = 0; i <= lc; i++) {
		if (!strncmp(b, a + i, lb)) {
			return 1;
		}
	}
	return 0;
}

Node *sn=0;

bool PromptSf(Node *n, void *pv)
{
	if (MatchStr(n->GetPrompt(), (char*)pv)) {
		n->Select();
		sn = n;
		return 1;
	}
	return 0;
}

bool ConfigSf(Node *n, void *pv)
{
	if (MatchStr(n->GetSymbol(), (char*)pv)) {
		n->Select();
		sn = n;
		return 1;
	}
	return 0;
}

*/

// to search from elsewhere in this app use:
// sn->Search(PromptSf/ConfigSf,<string>);
/*
void KKView::Search(int id)
{
	if (!sn) {
		sn = nr;
	}
	QLineEdit *le = (QLineEdit*)child("dLgLiNe", 0, TRUE);
	const char *cc = le ? le->text() : "-+-";
	switch (id) {
	case 0: //prompt
		if (!sn->Search(PromptSf, (char*)cc)) {
			sn=nr;
		}
		break;
	case 1: //CONFIG_
		if (!sn->Search(ConfigSf, (char*)cc)) {
			sn = nr;
		}
		break;
	case 2: //reset
		sn = nr;
		break;
	}
	if (sn == nr) {
		nr->Select();
	}
}
*/
void KKView::helpSearch()
{
	puts("search");
/*		static QString str="";
		QDialog d(this,"SearchDlg");
		QVBoxLayout *vb = new QVBoxLayout(&d,11,6);
			QLineEdit *le = new QLineEdit(&d,"dLgLiNe");	 vb->addWidget(le);
		QButtonGroup *hb = new QButtonGroup(1,QGroupBox::Vertical,"",&d); vb->addWidget(hb);
		connect(hb,SIGNAL(clicked(int)),this,SLOT(Search(int)));
		QPushButton *b1 = new QPushButton("&Prompt" ,hb); b1=b1;
		QPushButton *b2 = new QPushButton("&CONFIG_",hb); b2=b2;
		QPushButton *b3 = new QPushButton("&Reset"  ,hb); b3=b3;
		QPushButton *b4 = new QPushButton("&Close"  ,hb);
		connect(b4,SIGNAL(clicked()),&d,SLOT(reject()));
		le->setText(str); d.exec(); str=le->text();*/
}

void KKView::helpAbout()
{
	puts("about");
//	mb.about(this,"About kkonfig",CopyRight);
}

void KKView::helpAboutQt()
{
	puts("aboutQt");
//	mb.aboutQt(this);
}

//------------------------------------------------------------------------------

