// vim: sts=4 ts=4 sw=4

#include "kktextbrowser.h"
#include <QTextStream>
#include <QFileInfo>
#include <QFile>

//-----------------------------------------------------------------------------

KKTextBrowser::KKTextBrowser(QWidget *p) : QTextBrowser(p)
{
	node = 0;
	pnr = 0;

	setText("Click on any item to display help.\n");
	setWordWrapMode(QTextOption::NoWrap);
	setOpenLinks(false);

	connect(this, SIGNAL(anchorClicked(const QUrl&)),
			this, SLOT(setSource(const QUrl&)));
}

void KKTextBrowser::setNode(Node *n)
{
	char *help = n->GetHelp();
	if (!help) {
		help = "No help available.";
	}

	QString link;
	QTextStream(&link) << (*pnr)->GetPath() << n->GetSource();// << ":" << n->GetLine();

	QString html;
	QTextStream h(&html);
	h << "<head></head><body><big>";
	h << n->GetPrompt() << "<br><br>";
	h << n->GetSymbol() << " = " << n->GetStr() << "<br><br>";
	h << "<a href=\"" << link << "\">" << link << "</a><br><br>";
	h << "<pre>" << help << "</pre></big><body>";

	node = n;
	clearHistory();
	setHtml(html);
}

void KKTextBrowser::setSource(const QUrl &url)
{
	printf("link clicked -> %s <\n", qPrintable(url.toString()));

	QFile f(url.toString());
	f.open(QIODevice::ReadOnly);
	QString s(f.readAll());
	f.close();

	QString html;
	QTextStream h(&html);
	h << "<head></head><body><big>";
	h << "<pre>" << s << "</pre></big><body>";

	setHtml(html);
}

//-----------------------------------------------------------------------------
// class HelpText
// forward & back
#if 0

    connect(helptext,SIGNAL(cursorPositionChanged(int,int)),
        helptext,SLOT(linkTo(int,int)));


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
		if (node) {
			bFileNP = 1;
			bool t = bShowFile;
			bShowFile = 0;
			ShowHelp((QTreeWidgetItem*)node->user);
			bShowFile = t;
		}
	}
}

void HelpText::keyPressEvent(QKeyEvent *e)
{
/*
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
	*/
}

//-----------------------------------------------------------------------------


void HelpText::ShowHelp(QTreeWidgetItem *li)
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

	FileInit();
	node = n->node;
/*
	// show file instead ?
	int ll = n->node->GetLine();
	char *pt, *nn = n->node->GetSource();
	if (bShowFile && (pt = (*pnr)->GetFileH(nn))) {
		FilePush(nn, ll);
		ShowText(pt, ll);
		return;
	}
*/
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
	return;
	/*
	// if in a link get the filename
	QString s = text(para);

	int l;
	char *p, *fn;
	if (!(p = (*pnr)->GetLinkFileH(s, pos, 0, 0, &l, &fn))) {
		return;
	}
	FilePush(fn, l ? l : -1, para+1);
	ShowText(p, l);
	*/
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
/*
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
*/
}
#endif

//-----------------------------------------------------------------------------

