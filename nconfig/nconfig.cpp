const char *CopyRight = 
"nconfig - ncurses kernel configuration utility based on the nconfig backend.\n\n"

"Copyright (C) 2004-2006 Rikus Wessels <rikusw at rootshell dot be>\n"
"GNU GPL v2.0\n\n";

#include <stdio.h>
#include <stdlib.h>	// malloc/free
#include <unistd.h>
#include <string.h>
#include <signal.h>	// SIGWINCH
#include <sys/ioctl.h>	// TIOCGWINSZ

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#include <termios.h>	// get/set attribs
#include <curses.h>
#include "../nodes.h"

// from nodes.cpp --- don't depend on it...
extern int FileTot;
extern int LineTot;
extern int nnew;
extern int nfree;
extern int npnew;
extern int npfree;

//-------------------------------------------------------------------
// nconfig

WINDOW *wscr=0;		// the main curses screen
int cLines=0;		// the number of lines-2 on the terminal
			// (zero based + 1 at bottom = -2)
int iColumns=0;		// the number of columns on the terminal
int iLines=0;		// the number of lines on the terminal

char msg[80];		// display at the bottom of screen
char spc[30];		// used for indenting, best left alone

int yCursor=0;		// the cursor position
bool bCursorLocked=false;	// locked at top of screen ?

#define NO_START 1000000 // can there be more prompts than this ??
int iStart=1;		// start at line
int iStartMax=0;	// = iCount lines displayed
int iCount;		// the number of lines currently enumerated

NodeRoot *nr=0;		// root node
Node *nCursor;		// node at cursor
Node *nCurrent;		// node at top of screen
Node *nStart;		// if set start displaying here
Node *nEnum;		// used for calling _Enumerate to display
Node **nList=0;		// used by the mouse for line to node conversion.

bool bHelpMode = 0;	// In help mode ?
bool bMenuMode = 1;	// Menu/all mode
int mode=NS_M_NORMAL;	// Menu/all mode

//-------------------------------------------------------------------

bool ShowFunc(Node *n,int flags,void *pv)
{
    int iShow = 1;
    if(n == nStart) { iStart = iCount+1; nStart = 0; }
    
    if((n->GetType() & NTT_VISIBLE))// || n->GetType() == NT_SOURCE)
    {
	if((flags & NS_EXIT) &&
	    ((bMenuMode && n->GetType() == NT_MENU) ||
	     (n->GetState() & NS_COLLAPSED) ||
	     (n->GetType() & NTT_INPUT) ||
	     (n->GetType() == NT_COMMENT | NTT_PARENT)))
	{
	    spc[strlen(spc)-1]=0; // only parents come here
	    return 1;
	}else{
	    iCount++;
	}
	
	if(iCount == iStart) { nStart = 0; nCurrent = n; } // set the current node
	if(iCount < iStart || iCount > (iStart+cLines)) iShow=0; else
	{
	    nList[iCount - iStart] = n;
	    //if(yCursor == iCount - iStart) nCursor = n;
	}
	
	if(flags & NS_EXIT)
	{
	    spc[strlen(spc)-1]=0; // only parents come here
	    if(iShow)
	    {
		attrset(COLOR_PAIR(4)|A_BOLD); 
		printw("[#]%s\x08<%s\n",spc,n->GetPrompt());
	    }
        }else
        {
	    if((n->GetType() & NTT_PARENT) && !(n->GetType() & NTT_INPUT)
		    && (n->GetType() != (NT_COMMENT | NTT_PARENT)))
	    {
		if(flags & NS_SKIPPED) attrset(COLOR_PAIR(5)); else
		if(flags & NS_DISABLED) attrset(COLOR_PAIR(8)); else
		attrset(COLOR_PAIR(4)|A_BOLD);
	
		char c='#';
		switch(n->GetType()) {
		   case NT_MENU:    c='M'; break;
		   case NT_CHOICEP: c='C'; break;
		}
	
		if(iShow) printw("[%c]%s\x08>%s\n",c,spc,n->GetPrompt());
	        strcat(spc,"-");
		if(bMenuMode && n->GetType() == NT_CHOICEP)
		    n->Enumerate(ShowFunc,mode,0);
		return 1;
	    }
	    if(iShow)
	    {
		char st;
		unsigned int val = n->Get();
		switch(val)
		{
		    case 0: st='C'; attrset(COLOR_PAIR(7)|A_BOLD); break;
		    case 1: st='N'; attrset(COLOR_PAIR(1)|A_BOLD); break;
		    case 2: st='M'; attrset(COLOR_PAIR(3)|A_BOLD); break;
		    case 3: st='Y'; attrset(COLOR_PAIR(2)|A_BOLD); break;
		    default: if(n->GetType() & NTT_STR)
			     attrset(COLOR_PAIR(4)|A_BOLD); else
			     { attrset(COLOR_PAIR(2)|A_BOLD); st='?'; }
		}
		if(flags & NS_SKIPPED) attrset(COLOR_PAIR(5)); else
		if(flags & NS_DISABLED) attrset(COLOR_PAIR(8));
		
		if(n->GetType() & NTT_NMY)
		{
		    if(n->GetType() & NTT_TRI)
			printw("<%c>%s%s\n",st,spc,n->GetPrompt()); else
			printw("[%c]%s%s\n",st,spc,n->GetPrompt());
		}else
		if(n->GetType() & NTT_STR)
		{
		    if(!n->GetStr())
		    printw("(S)%s%s == error ptr = %i\n",spc,n->GetPrompt(),n->GetStr()); else
		    printw("(S)%s%s == \"%s\"\n",spc,n->GetPrompt(),(char*)n->GetStr());
		}else
		{
		    printw("(%c)%s%s\n",st,spc,n->GetPrompt());
		}
	    }
	    if(n->GetType() & NTT_PARENT)
	    {
		strcat(spc,"+");
		if(bMenuMode) n->Enumerate(ShowFunc,mode,0);
	    }
        }
    }else
    if(n->GetType() == NT_IF)
    {                        
        if(flags & NS_EXIT) spc[strlen(spc)-1]=0; else strcat(spc,"-");
    }

    return 1;
}

void DisplayIt()
{
redo:
    clear(); move(0,0);
    spc[0]='-'; spc[1]=0;
    iCount=0; nCursor=0;
    for(int i=0; i<=cLines; i++) nList[i]=0;
    
    nEnum->_Enumerate(ShowFunc,mode,0);

    if(iStart > iCount) // for skip/disable
    {
	iStart = iCount;
	goto redo;
    }

    iStartMax = iCount;
    attrset(COLOR_PAIR(0));
    mvprintw(cLines+1,0,"%s",msg); msg[0]=0;
    
    for(;yCursor >= 0; yCursor--) {
	nCursor = nList[yCursor];
	if(nCursor) break;
    }
    
    // still not found ??!!!
    if(!nCursor) nCursor = nEnum->GetChild();
    if(!nCursor) nCursor = nEnum;
		    
    move(yCursor,0);
    
    refresh();
}

void TermResize(int i)
{
    if(i == SIGWINCH)
    {
    	winsize w;
	ioctl(0,TIOCGWINSZ,&w);
	cLines = w.ws_row - 2; // 1 open line + zero based = 2
	iColumns = w.ws_col;
	iLines = w.ws_row;

	if(nList) free(nList);
	nList = (Node**)malloc((cLines+2)*sizeof(Node*));
	if(!nList)
	{
	    endwin();
	    delete nr;
	    printf("malloc fail.\n");
	    exit(1);
	}

	if(wscr) // init yet ?
	{
	    resizeterm(w.ws_row,w.ws_col);
	    if(!bHelpMode) DisplayIt();
	}
    }
}

char *ReadString()
{
    static char str[256];
    
    echo();
    getnstr(str,250);
    noecho();
    
    return str;
}

// used by in/out menu
int ipp=0;
int ypp[40];
Node *npp[40];
Node *epp[40]; // maybe I should use a struct here...

void PushPosition()
{
    if(ipp >= 40) return;
    npp[ipp] = nCurrent;
    ypp[ipp] = yCursor;
    epp[ipp] = nEnum;
    ipp++;
}

bool PopPosition()
{
    if(!ipp) return 0;
    ipp--;
    if(bCursorLocked) yCursor = 0; else yCursor = ypp[ipp];
    nStart = npp[ipp];
    nEnum = epp[ipp];
    return 1;
}


bool ShowFile(char *fn,int line);
void ShowText(char *,int);

// searching
Node *sn=0;
char txt[100]="";
unsigned int fSearchType=' ';
bool MatchStr(const char *a,const char *b);
bool ConfigSf(Node *n,void *pv);
bool PromptSf(Node *n,void *pv);

void Show11(NodeRoot *nr)
{
    // catch SIGWINCH
    struct sigaction act;
    act.sa_handler = TermResize;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGWINCH,&act,0);
    TermResize(SIGWINCH);

    // start ncurses
    wscr = initscr();
    noecho();
    keypad(wscr,TRUE);
    mousemask(ALL_MOUSE_EVENTS,0);

    // if color is not available fail --- for now at least
    if(!(has_colors() && start_color() == OK))
    {
	endwin();
	printf("Color not available.");
	return;
    }

    // setup colors
    use_default_colors();
    init_pair(1,COLOR_RED,-1);
    init_pair(2,COLOR_GREEN,-1);
    init_pair(3,COLOR_YELLOW,-1);
    init_pair(4,COLOR_BLUE,-1);
    init_pair(5,COLOR_MAGENTA,-1);
    init_pair(6,COLOR_CYAN,-1);
    init_pair(7,COLOR_WHITE,-1);
    init_pair(8,COLOR_WHITE,-1);

    // misc variables
    strcpy(msg,"Press h now to see some help.");
    nCurrent = nr; // current node
    nEnum = nr; // enumerate node
    sn = nr; // search node

next:
    DisplayIt();
readit:
    int wch = getch();
    switch(wch)
    {
    // mouse
    case KEY_MOUSE:
    {
	MEVENT m;
	if(getmouse(&m) != OK) break;

	if(m.bstate & (BUTTON1_CLICKED|BUTTON1_RELEASED))
	{
	    if(m.y > cLines || !nList[m.y]) goto readit; // invalid location

	    if(!bCursorLocked) yCursor = m.y;
	    nCursor = nList[m.y];
	    
	    if(nList[m.y]->GetType() & NTT_PARENT && !(nList[m.y]->GetType() & NTT_INPUT))
		goto inm; // case KEY_RIGHT:
	    else
		goto adv; // case ' ':
	}else
	if(m.bstate & (BUTTON3_CLICKED|BUTTON3_RELEASED))
	{
	    if((m.y > cLines || !nList[m.y]) && fSearchType!='X') goto readit; // invalid location
	    nCursor = nList[m.y];
	    goto xxxx;
	}
	break;
    }
    // movement
    case KEY_PPAGE:	iStart-=cLines; if(iStart<1) iStart=1; break;
    case KEY_NPAGE:	iStart+=cLines; if(iStart>iStartMax) iStart = iStartMax; break;
    case KEY_HOME:	iStart = 1; break;
    case KEY_END:	iStart = (iStartMax - cLines); break;
    case KEY_RIGHT: // into menu
    {
inm:    if(fSearchType!=' ') break;

	if(nCursor->GetType() & NTT_PARENT)
	{
	    Node *n = nCursor->GetChild();
	    if(!n) goto readit;

	    PushPosition();
	    yCursor = 0; iStart = 1; nEnum = n;
	}
	break;
    }

    case KEY_LEFT: // out of menu
    {
outm:	if(fSearchType!=' ') { // cancel searching
	    mode &= ~NS_DOWN; bMenuMode = 1;
	    strcpy(msg,"Out of Search Mode"); // xref is also a search mode
	    fSearchType = ' ';
	}

	Node *n = nCursor->GetParent(0,NTT_VISIBLE); // defaults if restore fail
	if(nCursor == nr) n = nr;
	yCursor = 0; iStart = NO_START; if(n) nStart = nEnum = n; else n = nEnum;
 
	if(PopPosition()) break;
	
	n = n->GetParent(0,NTT_VISIBLE); // set enum to first child
	if(n) nEnum = n->GetChild();
	break;
    }

    case 'c': // "lock/unlock" the cursor
	bCursorLocked = !bCursorLocked;
	if(bCursorLocked) { yCursor = 0; nCursor = nCurrent; move(0,0); }
	goto readit;
	
    case KEY_UP:
	if(bCursorLocked)
	{
	    if(--iStart<1) iStart=1;
	}else
	{
	    if(--yCursor<0)
	    {
		yCursor=0;
		if(--iStart<1) iStart=1;
	    }else
	    {
		nCursor = nList[yCursor];
		if(!nCursor) { nCursor = nCurrent; yCursor = 0; }
		move(yCursor,0);
		goto readit;
	    }
	}
	break;
    case KEY_DOWN:
	if(bCursorLocked)
	{
	    if(++iStart > iStartMax) iStart = iStartMax;
	}else
	{
	    if(++yCursor>cLines)
	    {
		yCursor = cLines;
		if(++iStart > iStartMax) iStart = iStartMax;
	    }else
	    {
retrydn:	nCursor = nList[yCursor];
		if(!nCursor)
		{   // will this _ever_ run ????
		    if(!yCursor) { nCursor = nCurrent; yCursor = 0; } else
				 { yCursor--; goto retrydn; }
		}
		move(yCursor,0);
		goto readit;
	    }
	}
	break;

    // value changes
    case KEY_IC: nCurrent->Set(3); break; // ins = Y
    case KEY_DC: nCurrent->Set(1); break; // del = N
    case '1': nCurrent->Set(1); break;
    case '2': nCurrent->Set(2); break;
    case '3': nCurrent->Set(3); break;
    case ' ':
    {
adv:	if(nCursor->GetType() & NTT_STR)
	{ mvprintw(cLines+1,0,"New value: "); refresh(); nCursor->Set(ReadString()); } else
	{ nCursor->Advance(); nStart = nCurrent; iStart = NO_START; }
	break;
    }

    // view settings
    case 'a': // menu/all
    {
	mode ^= NS_DOWN;
	if(mode & NS_DOWN)
	{
	    bMenuMode = 0;
	    strcpy(msg,"Full mode");
a_getenum:   nEnum=nCurrent->GetParent(0,NTT_VISIBLE);
	    if(nEnum)
		nEnum = nEnum->GetChild();
	    else
		nEnum=nCurrent; // root node
	}else
	{ 
	    bMenuMode = 1; strcpy(msg,"Menu mode");
	    if(nCurrent->GetType() == NT_ROOT) nEnum = nCurrent; else goto a_getenum;
		//nEnum=nCurrent->GetParent(0,NTT_VISIBLE)->GetChild();
	}
	nStart = nCurrent; iStart = NO_START;
	break;
    }
    case 'd':
    {
	mode ^= NS_DISABLED;
	strcpy(msg,mode & NS_DISABLED ? "Show Disabled":"Hide Disabled");
	goto rrrr;
    }
    case 's':
    {
	mode ^= NS_SKIPPED;
	strcpy(msg,mode & NS_SKIPPED ? "Show Skipped":"Hide Skipped");
rrrr:
	// make sure the position is visible
	nStart = nCurrent; iStart = NO_START; //yCursor = 0;
	
	while(nStart->GetState() & (NS_SKIPPED | NS_DISABLED))
	{
	    nStart = nStart->GetParent(0,NTT_VISIBLE);
	    if(!nStart) { iStart=1; break; } // shouldn't happen
	}
	break;
    }

    // load&save
    case 'O':
    case 'L':
    {
        mvprintw(cLines+1,0,"File to load, # = .config: "); refresh();
	char *str = ReadString();
	snprintf(msg,79,"Loading: <%s>....",str);
	if(nr->Load(*str=='#' ? 0 : str))
	    strcat(msg,"....OK"); else strcat(msg,"....Failed");
	goto rrrr;
    }
    case 'S':
    {
	mvprintw(cLines+1,0,"File to save, # = .config: "); refresh();
	char *str = ReadString();
	snprintf(msg,79,"Saving: <%s>....",str);
	if(nr->Save(*str=='#' ? 0 : str))
	    strcat(msg,"....OK"); else strcat(msg,"....Failed");
	break;
    }

    // help
    case 'f': // source
    {
	char buf[255];
	strcpy(buf,nr->GetPath());
	strcat(buf,nCursor->GetSource());
	ShowFile(buf,nCursor->GetLine());
	break;
    }
    case 'h': // help
    {
	char *p = nr->GetHelpH(nCursor);
	if(!p) break;
	ShowText(p,0);
	nr->FreeH(p);
	break;
    }
    case 'x': // cross reference
    {
xxxx:	if(fSearchType=='X') goto outm; // back to normal
	if(fSearchType==' ') PushPosition();
	fSearchType = 'X';
    } 
    case 'X': // xref in xref (possibly useful)
    {
	if(fSearchType!='X') break;
	strcpy(msg,"Cross Reference mode.");
	Node *n = nCursor->GetDepTree();
	Node *c = n->GetChild(); if(c) n = c;
	if(n->GetType() == NT_MENU) c = n->GetChild(); if(c) n = c;
	iStart = 1; nEnum = n; yCursor = 0;
	break;
    }

    // searching
    case 'N': strcpy(txt,nCursor->GetPrompt()); goto jPSf;
    case 'n':		    // next
	switch(fSearchType)
	{
	case 'F': if(!sn->Search(PromptSf,txt)) sn=nr; goto jC;
	case 'C': if(!sn->Search(ConfigSf,txt)) sn=nr; goto jC;
	}
	break;
    case 'r': sn=nr; break; // Reset
	      
    case '/':
    case 'F':		    // find prompt
    {
	mvprintw(cLines+1,0,"Find in prompt: "); refresh();
	strcpy(txt,ReadString());
jPSf:	if(!sn->Search(PromptSf,txt)) {
	    sn=nr; sn->Search(PromptSf,txt);
	}
	if(fSearchType==' ') PushPosition();
	fSearchType = 'F';
	goto jC;
    }
    case 'C':		    // find CONFIG_
    {
	mvprintw(cLines+1,0,"Find in CONFIG_: "); refresh();
	strcpy(txt,ReadString());
	if(!sn->Search(ConfigSf,txt)) {
	    sn=nr; sn->Search(ConfigSf,txt);
	}
        if(fSearchType==' ') PushPosition();
	fSearchType = 'C';
jC:	yCursor = 0; iStart = NO_START; nEnum = sn;
	mode |= NS_DOWN; bMenuMode = 0;
	nStart = nEnum; nEnum = nr;
	strcpy(msg,"Search Mode");
	break;
    }

    case 'q': // quit
    {
	if(nr->Modified())
	{
	    mvprintw(cLines+1,0,"Exit without saving ? (y/n):");
	    refresh();
	    if(getch() != 'y') break;
	}
	endwin();
	return;
    }   
    default:
	goto readit; // unknown key -> do nothing
    }
    goto next;
}


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

bool PromptSf(Node *n,void *pv)
{
	if(MatchStr(n->GetPrompt(),(char*)pv)) {
		n->Select(); sn=n;
		return 1;
	}
	return 0;
}

bool ConfigSf(Node *n,void *pv)
{
	if(MatchStr(n->GetSymbol(),(char*)pv)) {
		n->Select(); sn=n;
		return 1;
	}
	return 0;
}


//-----------------------------------------------------------------------------
// Help browsing functions

bool ShowFile(char *fn,int line)
{
    char *p = nr->GetFileH(fn);
    if(!p) return 0;
    ShowText(p,line);
    nr->FreeH(p);
    return 1;
} 

bool ClickedLink(char *t,int x,int y)
{
    // find line
    for(int l=0; l<y; l++)
    {
	if(!*t) return 0; // not found
	for(int i=0; i<iColumns; i++,t++)
	{
	    if(!*t) break;
	    if(*t=='\n') { t++; break; }
	    if(*t=='\t') i+=7;
	}
    }
    // tab--xpos fixup
    for(char *c=t; *c==' ' || *c=='\t'; c++) if(*c=='\t') x-=7;
    
    
    int l=0;
    char *p = nr->GetLinkFileH(t,x,0,0,&l);
    if(!p) return 0;
    ShowText(p,l);
    nr->FreeH(p);
    return 1;
}  
    
int GetLines(char *t,int &line)
{
    int i,l=1,reall=1,tmpl=line;

    while(1)
    {
	for(i=0; i<iColumns; i++,t++)
	{
	    if(!*t)
	    {
		line = tmpl;
		return l;
	    }
	    if(*t=='\n')
	    {
		reall++; t++;
		if(reall == line) tmpl = l;
		break;
	    }
	    if(*t=='\t')
	    {
		i+=7;
	    }
	}
	l++;
    }
    return 0;
}

void ShowText(char *t,int iline)
{
    int cx=0,cy=0,line;
    int col,l,offs=0,maxoffs;
    WINDOW *p = 0;

    clear();
rerender:
    offs = 0;
    refresh(); // do this or else.... you have a strange resize bug....
    if(p) delwin(p);
    line = iline;
    l = GetLines(t,line);
    col = iColumns;
    p = newpad(l < iLines ? iLines : l ,iColumns);

    maxoffs = l - iLines;
    if(maxoffs < 0) maxoffs = 0;

    // line + cursor positioning
    int iLines2 = iLines/2;
    if(line < iLines2)
    {
	cy = line;
    }else
    if(line <= maxoffs)
    {
	offs = line - iLines2;
	cy  = iLines2;
    }else
    {
	offs = maxoffs;
	if(line - maxoffs < iLines) cy = line - maxoffs; else cy = iLines-1;
    }

    // render it
    wmove(p,0,0);
    wattrset(p,COLOR_PAIR(0));

    char *c = t,*d = t,tmp;
    while(1)
    {
	if((*d=='\n' && d++) || !*d)
	{
	    for(char *s,*e; nr->GetLink(c,-1,&s,&e); c = e)
	    {
		tmp = *s; *s = 0; wprintw(p,"%s",c); *s = tmp; // pre link
		wattrset(p,COLOR_PAIR(4)|A_BOLD);
		tmp = *e; *e = 0; wprintw(p,"%s",s); *e = tmp; // link
		wattrset(p,COLOR_PAIR(0));
	    }
	    if(!*d) { wprintw(p,"%s",c); break; }
	    tmp = *d; *d = 0; wprintw(p,"%s",c); *d = tmp; // post link(s)//*/
	    c = d;
	}else
	    d++;
    }
    
redo:
    prefresh(p,offs,0,0,0,iLines-1,iColumns-1);
    move(cy,cx); refresh();
reget:    
    int wch = getch();
    if(col != iColumns) goto rerender;
    switch(wch)
    {
    // mouse
    case KEY_MOUSE:
    {
	MEVENT m;
	static int bstate=0;
	if(getmouse(&m) != OK) break;
//	printf("%x\r\n",m.bstate);
	if(m.bstate & (BUTTON1_CLICKED|BUTTON1_RELEASED))
	{
	    if(ClickedLink(t,m.x,m.y+offs) && col != iColumns) goto rerender;
	}

/*	// UGLY YUCK mousewheel hack
	if(m.bstate == 0x8000000) {
		if(bstate == 0x02) goto ppage;
		if(bstate == 0x80) goto npage;
	}
	
	if(bstate == 0x8000000 && m.bstate == 0x02) {
ppage:
		offs-=3; if(offs<0) { offs=0; if(--cy<0) cy=0; }
	}
	if(bstate == 0x8000000 && m.bstate == 0x80) {
npage:
		offs+=3; if(offs>maxoffs) { offs=maxoffs; if(++cy>iLines-1) cy=iLines-1; }
	}
	if(m.bstate == 0x8000000)
		bstate = m.bstate;
	// END HACK*/
	break;
    }
    case KEY_PPAGE: offs -= iLines/2; if(offs<0) offs=0; break;
    case KEY_NPAGE: offs += iLines/2; if(offs>maxoffs) offs=maxoffs; break;
    case KEY_HOME: offs=0; break;
    case KEY_END: offs=maxoffs; break;

    case KEY_UP: offs--; if(offs<0) { offs=0; if(--cy<0) cy=0; } break;
    case KEY_DOWN: offs++; if(offs>maxoffs) { offs=maxoffs; if(++cy>iLines-1) cy=iLines-1; } break;

    case KEY_LEFT: if(--cx<0) cx=0; break;
    case KEY_RIGHT: if(++cx>iColumns-1) cx = iColumns-1; break;
    case ' ':
    case '\n': if(ClickedLink(t,cx,cy+offs) && col!=iColumns) goto rerender; break;

    case '\x1B':
    case 'f':		   
    case 'h': 
    case 'q':
	delwin(p);
	return;
    default:
	goto reget;
    }
    goto redo;
}

//-------------------------------------------------------------------

int main(int argc,char *argv[])
{
    printf(CopyRight);

    mode |= NS_DISABLED;

//-------------------------------------
// Load backend
    
    nr = new NodeRoot();
    
    nr->SetAutoFreeH(false);
    int ret = nr->Init_CmdLine(argc,argv);
    if(ret & 7) { if(ret & 3) printf("Init failure.\n"); delete nr; return 1; }
    Show11(nr); // main loop
    int scount = nr->GetSymbols()->GetCount(); // stats
    
    delete nr;
    
//-------------------------------------
// Stats

    printf("Stats:\n");
    printf("Nodes allocated: %i, freed: %i\n",nnew,nfree);
    if(scount > 20) // (kernel 2.4.x) > 2000 
    {
	printf("ParentNodes allocated: %i, freed: %i\n",npnew,npfree);
        printf("Total files: %i.\n",FileTot);
        printf("Total Symbols: %i\n",scount);
        printf("Total lines parsed: %i\n",LineTot);
    }
    printf("\n");
    return 0;
}

//-------------------------------------------------------------------

