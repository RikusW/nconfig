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
#include "../nodes.h"

#if 0
bool bShowFile = false;
bool bShowSkipped = false;
bool bShowDisabled = false;
#else
bool bShowFile = true;
bool bShowSkipped = true;
bool bShowDisabled = true;
#endif

//QColorGroup cgDisabled,cgSkipped;
QBrush brDisabled = QBrush(QColor(100, 100, 100)); //TODO use disabled
QBrush brSkipped = QBrush(QColor(200, 100, 200));

int main( int argc, char **argv )
{
	QApplication a(argc,argv);

	KKView KKView(argc,argv);
	KKView.resize( 800, 600 );
//	KKView.setCaption( "Kernel Konfig" );
//	a.setMainWidget( &KKView );
	KKView.show();

	return a.exec();
}

static const char *xpm_no[]={
"16 16 2 1",
". c None",
"# c #ff0000",
"................",
"................",
"..##........##..",
"..###.......##..",
"..####......##..",
"..#####.....##..",
"..##.###....##..",
"..##..###...##..",
"..##...###..##..",
"..##....###.##..",
"..##.....#####..",
"..##......####..",
"..##.......###..",
"..##........##..",
"................",
"................"};

static const char *xpm_mod[]={
"16 16 2 1",
". c None",
"# c #d0f000",
"................",
"................",
"..###......###..",
"..####....####..",
"..#####..#####..",
"..##.######.##..",
"..##..####..##..",
"..##...##...##..",
"..##........##..",
"..##........##..",
"..##........##..",
"..##........##..",
"..##........##..",
"..##........##..",
"................",
"................"};

static const char *xpm_yes[]={
"16 16 2 1",
". c None",
"# c #00ff00",
"................",
"................",
"..##........##..",
"..###......###..",
"...###....###...",
"....###..###....",
".....######.....",
"......####......",
".......##.......",
".......##.......",
".......##.......",
".......##.......",
".......##.......",
".......##.......",
"................",
"................"};

static const char *xpm_str[]={
"16 16 2 1",
". c None",
"1 c #6060ff",
"................",
"................",
".....111111.....",
"....11111111....",
"...111....111...",
"..111......111..",
"...1111.........",
".....1111.......",
".......1111.....",
".........1111...",
"..111......111..",
"...111....111...",
"....11111111....",
".....111111.....",
"................",
"................"};

static const char *xpm_menu[]={
"16 16 2 1",
". c None",
"# c #6060ff",
"................",
"................",
"..###......###..",
"..####....####..",
"..#####..#####..",
"..##.######.##..",
"..##..####..##..",
"..##...##...##..",
"..##........##..",
"..##........##..",
"..##........##..",
"..##........##..",
"..##........##..",
"..##........##..",
"................",
"................"};

static const char *xpm_choice[]={
"16 16 2 1",
". c None",
"1 c #6060ff",
"................",
"................",
".....111111.....",
"....11111111....",
"...111....111...",
"..111......111..",
"..111...........",
"..111...........",
"..111...........",
"..111...........",
"..111......111..",
"...111....111...",
"....11111111....",
".....111111.....",
"................",
"................"};

static const char *xpm_comment[]={
"16 16 3 1",
". c None",
"0 c #000000",
"1 c #d0f000",
"................",
"................",
"......0000......",
"....00111100....",
"...0111111110...",
"...0111111110...",
"..011111111110..",
"..011111111110..",
"..011111111110..",
"..011111111110..",
"...0111111110...",
"...0111111110...",
"....00111100....",
"......0000......",
"................",
"................"};

QPixmap *IconNo  = 0;
QPixmap *IconMod = 0;
QPixmap *IconYes = 0;
QPixmap *IconStr = 0;
QPixmap *IconMenu = 0;
QPixmap *IconChoice = 0;
QPixmap *IconComment = 0;

//-----------------------------------------------------------------------------
// NodeListItem

NodeListItem::NodeListItem(QTreeWidget *p, Node *n)
	: QTreeWidgetItem(p)
{
	node = n;
	n->user = this;
	setText(0, n->GetPrompt());
	SetIcon();
}

NodeListItem::NodeListItem(NodeListItem *parent, NodeListItem *after, Node *n)
	: QTreeWidgetItem(parent, after)
{
	node = n;
	n->user = this;
	setText(0, n->GetPrompt());
	SetIcon();
}
/*
void NodeListItem::activate()
{
	QPoint pt;
	if (!activatedPos(pt) || QRect(0, 0, height(), height()).contains(pt)) {
		if (node->GetType() & NTT_STR) {
			setRenameEnabled(0, true);
			startRename(0);
		} else {
			node->Advance();
		}
	}
}

void NodeListItem::okRename(int col)
{
	QListViewItem::okRename(col);

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
		case 1:  setIcon(0, *IconNo);  break;
		case 2:  setIcon(0, *IconMod); break;
		case 3:  setIcon(0, *IconYes); break;
		default: setIcon(0, *IconStr); break;
		}
	} else {
		switch (node->GetType()) {
		case NT_ROOT:
		case NT_MENU:    setIcon(0, *IconMenu);    break;
		case NT_CHOICEP: setIcon(0, *IconChoice);  break;
		case NT_COMMENT | NTT_PARENT:
		case NT_COMMENT: setIcon(0, *IconComment); break;
		}
	}

	if (node->GetState() & NS_SKIPPED) {
		setHidden(!(bShowSkipped && bShowDisabled));
		setForeground(0, brSkipped);
	} else {
		if (node->GetState() & NS_DISABLED) {
			setHidden(!bShowDisabled);
			setForeground(0, brDisabled);
		} else {
			setHidden(false);
		}
	}

}

/*
void NodeListItem::paintCell(QPainter *p, const QColorGroup &cg, int c, int w, int a)
{
	NodeView *nv = (NodeView*)listView();
	const QColorGroup *pcg = &cg;
	if (node->GetState() & NS_SKIPPED) {
		pcg = &nv->cgSkipped; // override disabled
	} else {
		if (node->GetState() & NS_DISABLED) {
			pcg = &nv->cgDisabled;
		}
	}
	QListViewItem::paintCell(p, *pcg, c, w, a);
}

bool UpdateFunc(Node *n, int flags, void *pv)
{
	if (!(n->GetType() & NTT_VISIBLE)) {
		return 1;
	}
	NodeListItem *li = (NodeListItem*)n->user;
	pv=pv; //compiler shutup

	if (flags & NS_SKIPPED) {
		li->setHidden(!(bShowSkipped && bShowDisabled));
	} else {
		if (flags & NS_DISABLED) {
			li->setHidden(!bShowDisabled);
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
	//case NS_SKIP: break;
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
	//case NS_ENABLE: break;
	//case NS_DISABLE: break;
	case NS_EXPAND:
		li->setExpanded(true);
		return 1;
	case NS_COLLAPSE:
		li->setExpanded(false);
		return 1;
	case NS_PROMPT:
		li->setText(0, n->GetPrompt());
		return 1;
	case NS_SELECT:	{
		QListViewItem *qi = li;
		while((qi = qi->parent())) {
			qi->setExpanded(true);
		}
		li->listView()->setSelected(li, 1);
		li->listView()->ensureItemVisible(li);
		return 1;
		}
	}
	li->SetIcon();
	return 1;
}
*/
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
//	delete nr;

	delete IconNo;
	delete IconMod;
	delete IconYes;
	delete IconStr;
	delete IconMenu;
	delete IconChoice;
	delete IconComment;
}

KKView::KKView(int ac, char **av, QWidget *parent, const char *name)
{
//	nr=0;

	IconNo  = new QPixmap(xpm_no);
	IconMod = new QPixmap(xpm_mod);
	IconYes = new QPixmap(xpm_yes);
	IconStr = new QPixmap(xpm_str);
	IconMenu = new QPixmap(xpm_menu);
	IconChoice = new QPixmap(xpm_choice);
	IconComment = new QPixmap(xpm_comment);

	setWindowTitle("Kernel Konfig");
	setMinimumSize(160, 160);

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
	setCentralWidget(qs);

	// folders
	folders = new QTreeWidget(qs);
//	folders->setColumnCount(1);
//	folders->header()->setClickEnabled(FALSE);
//	folders->addColumn("Folder");
//	folders->setSorting(-1,0);
	// ---- fill tree here ----
	initFolders(0, 0, ac, av);
//	folders->setRootIsDecorated( TRUE );
//	qs->setResizeMode( folders, QSplitter::KeepSize );

	// folders2
	QSplitter *qs2 = new QSplitter(Qt::Vertical, qs);
	folders2 = new QTreeWidget(qs2);
//	folders2->setColumnCount(1);
//	folders2->header()->setClickEnabled(FALSE);
//	folders2->addColumn("Dependencies");
//	folders2->setSorting(-1,0);
	// helptext
	helptext = new QTextEdit(qs2);
/*
	helptext = new HelpText(qs2,"Text1");
	helptext->setNodeRoot(&nr);
	helptext->setTextFormat(Qt::PlainText);
	helptext->setText("Click on any item to display help.\n");
	helptext->setWordWrap(QTextEdit::NoWrap);
	//helptext->setReadOnly(true);

	connect(folders,SIGNAL(selectionChanged(QListViewItem*)),
		this,SLOT(ShowDeps(QListViewItem*)));

	connect(folders2,SIGNAL(selectionChanged(QListViewItem*)),
		helptext,SLOT(ShowHelp(QListViewItem*)));

	connect(helptext,SIGNAL(cursorPositionChanged(int,int)),
		helptext,SLOT(linkTo(int,int)));

	QValueList<int> lst2; lst2.append(130); lst2.append(470);
	qs2->setSizes(lst2);

	QValueList<int> lst; lst.append(350); lst.append(440);
	qs->setSizes( lst );*/
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
			if (!n->GetParent(NT_ROOT)) {
				break; // don't open in deptree
			}
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

	if (!nnr->Init("i386", "../../kernels/linux-2.2.0")) { //debug testing
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
	nr->GetSymbols()->Ntfy = NotifyFunc; 		// set notifications
*/
	NodeListItem *li = new NodeListItem(folders, nr);	// add the root
	RW_UserData rwd;
	rwd.parent = li;
	rwd.last = 0;
	nr->Enumerate(enumFunc, NS_T_ALL, &rwd);		// and the rest
	li->setExpanded(true);
}

//-----------------------------------------------------------------------------
// class HelpText
// forward & back
/*
void HelpText::FileInit()
{
	if (bFileNP) {
		bFileNP=0;
		return;
	}
	iFileStrs = -1;
	for (int i = 0; i < 10; i++) {
		FileStrs[i][0] = 0;
	}
}

void HelpText::FilePush(char *s, int l, int lfrom=0)
{
	if (lfrom) {
		FileLines[iFileStrs] = lfrom;
	}
	if (bFileNP) {
		bFileNP=0;
		return;
	}
	if (++iFileStrs > 9) {
		iFileStrs = 10;
		return;
	}
	FileLines[iFileStrs] = l;
	strcpy(FileStrs[iFileStrs], s);
	for (int i = iFileStrs + 1; i < 10; i++) {
		FileStrs[i][0] = 0; // clear
	}
}

void HelpText::FileNext(int i)
{
	if (++iFileStrs < 9 && FileStrs[iFileStrs][0]) {
		bFileNP = 1;
		if (i>-1 && iFileStrs > -1) {
			FileLines[iFileStrs-1] = i;
		}
		ShowFile(FileStrs[iFileStrs], FileLines[iFileStrs]);
	} else {
		if (iFileStrs > 9) {
			iFileStrs = 9;
		} else {
			if (!FileStrs[iFileStrs][0]) {
				iFileStrs--;
			}
		}
	}
}

void HelpText::FilePrev(int i)
{
	if (--iFileStrs > -1) {
		bFileNP = 1;
		if (i > -1 && iFileStrs < 10) {
			FileLines[iFileStrs + 1] = i;
		}
		ShowFile(FileStrs[iFileStrs], FileLines[iFileStrs]);
	} else {
		if (iFileStrs < -1) {
			iFileStrs = -1;
		}
		if (HelpNode) {
			bFileNP = 1;
			bool t = bShowFile;
			bShowFile = 0;
			ShowHelp((QListViewItem*)HelpNode->user);
			bShowFile = t;
		}
	}
}

void HelpText::keyPressEvent(QKeyEvent *e)
{
	int l, i;
	getCursorPosition(&l, &i);
	l++;
	switch (e->key()) {
	case Qt::Key_Escape:
	case Qt::Key_Backspace:
	case Qt::Key_Left:
		FilePrev(l);
		break;
	case Qt::Key_Space:
	case Qt::Key_Return:
	case Qt::Key_Enter:
	case Qt::Key_Right:
		FileNext(l);
		break;
	default:
		QTextEdit::keyPressEvent(e);
		return;
	}
	e->ignore();
}
*/
//-----------------------------------------------------------------------------
// help handling
/*
NodeListItem *OldN = 0;

void KKView::ShowDeps( QListViewItem *li )
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
	RW_UserData rwd;
	rwd.parent = l;
	rwd.last = 0;
	nd->Enumerate(enumFunc, NS_T_ALL, &rwd);		// and the rest
	l->setExpanded(true);
	nd->Update(1);

	helptext->ShowHelp(li);
}

void HelpText::ShowHelp( QListViewItem *li )
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
			OldN->setText(0, OldN->node->GetPrompt());
			OldN->setRenameEnabled(0, false);
		}
		if (n && n->node->GetType() & NTT_STR && n->node->Get() > 3) {
			n->setText(0, (char*)n->node->Get());
			n->setRenameEnabled(0, true);
		}
	}
	OldN = n;

	FileInit();
	HelpNode = n->node;

	// show file instead ?
	int ll = n->node->GetLine();
	char *pt, *nn = n->node->GetSource();
	if (bShowFile && (pt = (*pnr)->GetFileH(nn))) {
		FilePush(nn, ll);
		ShowText(pt, ll);
		return;
	}

	// start help
	char *p = (*pnr)->GetHelpH(n->node);
	if (!p) {
		return;
	}
	ShowText(p, -1);
	return;
}

void HelpText::linkTo(int para, int pos)
{
	// if in a link get the filename
	QString s = text(para);

	int l;
	char *p, *fn;
	if (!(p = (*pnr)->GetLinkFileH(s, pos, 0, 0, &l, &fn))) {
		return;
	}
	FilePush(fn, l ? l : -1, para+1);
	ShowText(p, l);
}

void HelpText::ShowFile(char *cc, int ll)
{
	char *p = (*pnr)->GetFileH(cc);
	if (!p) {
		return;
	}
	FilePush(cc, ll);
	ShowText(p, ll);
}

void HelpText::ShowText(char *cc, int ll)
{
	QString qs(cc);
	setText(qs);

	// hilight the links
	for (int i = 0, j = paragraphs(); i < j; i++) {
		QString s = text(i); //l=0;
		char *st, *e;
		const char *b = s;
		for (e = (char*)b; (*pnr)->GetLink(e, -1, &st, &e);) {
			setSelection(i, st - b, i, e - b);// l=r;
			setColor(QColor(0, 0, 255));
		}
	}
	setSelection(0, 0, 0, 0);

	// hilight the linked line and go to it if specified
	setSelection(ll-1, 0, ll, 0);
	setColor(QColor(255, 0, 255));
	setSelection(ll-1, 0, ll-1, 0);
	insertAt(">>>", ll-1, 0);

	//ensureCursorVisible(); // don't seem to work....?????
	int hh=0; // so lets do this UGLY hack.........
	for (int xx = 0; xx <= ll; xx++) {
		hh+=paragraphRect(xx).height();
	}
	ensureVisible(10, hh, 0, 99999);
}
*/
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
//	nr->Enumerate(UpdateFunc,NS_ALL,0);
}

void KKView::viewSkipped()
{
//	bShowSkipped=!bShowSkipped;
//	pm->setItemChecked(mViewSkipped,bShowSkipped && bShowDisabled);
//	nr->Enumerate(UpdateFunc,NS_ALL,0);
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

