// tconfig implementation
// terminal based kernel configuration utility
// based on the nconfig backend

#include <stdio.h>
#include <unistd.h>

#include <string.h>

#include <signal.h>	// SIGWINCH
#include <sys/ioctl.h>	// TIOCGWINSZ
#include <termios.h>	// get/set attrib

#include "../nodes.h"

// from nodes.cpp
extern int FileTot;
extern int LineTot;
extern int nnew;
extern int nfree;
extern int npnew;
extern int npfree;

//-------------------------------------------------------------------
// tconfig

int cLines=0;		// the number of lines-1 of the terminal
char spc[30];		// used for indenting, best left alone

int iStart=1;		// start at line
int iStartMax=0;	// iStart limit = iCount
int iCount;		// the number of lines currently enumerated

NodeRoot *nr=0;		// root node
Node *nCurrent;		// node at top of screen
Node *nStart;		// if set start displaying here
Node *nEnum=0;		// used for calling _Enumerate to display

bool bMenuMode = 1;	// Menu/all mode
int mode=NS_M_ALL;	// Menu/all mode

//-------------------------------------------------------------------

bool NotifyFunc(Node *n,int flags,void *pv)
{
    switch(flags)
    {
	case NS_SKIP:     printf("Notify: Skip     %s <%0x>\n",n->GetPrompt(),flags); break;
	case NS_UNSKIP:   printf("Notify: Unskip   %s <%0x>\n",n->GetPrompt(),flags); break;
	case NS_DISABLE:  printf("Notify: Disable  %s <%0x>\n",n->GetPrompt(),flags); break;
	case NS_ENABLE:   printf("Notify: Enable   %s <%0x>\n",n->GetPrompt(),flags); break;
	case NS_COLLAPSE: printf("Notify: Collapse %s <%0x>\n",n->GetPrompt(),flags); break;
	case NS_EXPAND:   printf("Notify: Expand   %s <%0x>\n",n->GetPrompt(),flags); break;
	case NS_STATE:    printf("Notify: State    %s <%0x>\n",n->GetPrompt(),flags); break;
	default:	  printf("Notify: ????     %s <%0x>\n",n->GetPrompt(),flags); break;
    }
    return 1;
}

bool NoHelpFunc(Node *n,int flags,void *pv)
{
    if(n->GetType() == NT_MENU || n->GetType() == NT_CHOICEP)
	printf("           %s\n",n->GetPrompt());
    
    if((n->GetType() & NTT_INPUT) && !n->GetHelp())
	printf("No help -> %s - %s\n",n->GetSymbol(),n->GetPrompt());
    
    return 1;
}
    
bool ShowFunc(Node *n,int flags,void *pv)
{
    int iShow = 1;
    if(n == nStart) { iStart = iCount+1; nStart = 0; }
    
    if((n->GetType() & NTT_VISIBLE))// || n->GetType() == NT_SOURCE)
    {
	if((flags & NS_EXIT) &&
	    ((bMenuMode && n->GetType() == NT_MENU) ||
	     (n->GetState() & NS_COLLAPSED) ||
	     (n->GetType() & NTT_INPUT)))
	{
	    spc[strlen(spc)-1]=0; // only parents come here
	    return 1;
	}else{
	    iCount++;
	}
	
	if(iCount == iStart) { nStart = 0; nCurrent = n; } // set the current node
	if(iCount < iStart || iCount > (iStart+cLines)) iShow=0;
	
	if(flags & NS_EXIT)
	{
	    spc[strlen(spc)-1]=0; // only parents come here
	    if(iShow)
	    {
		printf("\033[0;36m[#]%s\x08<%s\n\033[m",spc,n->GetPrompt());
	    }
        }else
        {
	    if((n->GetType() & NTT_PARENT) && !(n->GetType() & NTT_INPUT))
	    {
		if(iShow) printf("\033[1;34m[#]%s\x08>%s\033[m\n",spc,n->GetPrompt());
	        strcat(spc,"-");
		if(bMenuMode && n->GetType() == NT_CHOICEP)
		    n->Enumerate(ShowFunc,NS_M_NORMAL,0);
		return 1;
	    }
	    if(iShow)
	    {
		char ch,st,br='1';
		unsigned int val = n->Get();
		switch(val)
		{
		    case 0: ch='7'; st='C'; break;
		    case 1: ch='1'; st='N'; break;
		    case 2: ch='3'; st='M'; break;
		    case 3: ch='2'; st='Y'; break;
		    default: ch='1'; st='?';
		}
		if(flags & NS_DISABLED) { ch= '7'; br='0'; }
		if(flags & NS_SKIPPED) ch = '5';
		
		if(n->GetType() & NTT_NMY)
		{
		    if(n->GetType() & NTT_TRI)
			printf("\033[%c;3%cm<%c>%s%s\033[m\n",br,ch,st,spc,n->GetPrompt()); else
			printf("\033[%c;3%cm[%c]%s%s\033[m\n",br,ch,st,spc,n->GetPrompt());
		}else
		if(n->GetType() & NTT_STR)
		{
		    if(!n->GetStr())
		    printf("(S)%s%s == error ptr = %li\n",spc,n->GetPrompt(),(uintptr_t)n->GetStr()); else
		    printf("\033[%c;3%cm(S)%s%s == \"%s\"\033[m\n",br,ch,spc,n->GetPrompt(),n->GetStr());
		}else
		{
		    printf("\033[%c;3%cm(%c)%s%s\033[m\n",br,ch,st,spc,n->GetPrompt());
		}
	    }		
	    if(n->GetType() & NTT_PARENT)
	    {
		strcat(spc,"-");
		if(bMenuMode) n->Enumerate(ShowFunc,NS_M_NORMAL,0);
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
    iCount=0;
    spc[0]='-'; spc[1]=0;
    for(int i=0; i<2; i++) printf("\n");
    if(iStart > iStartMax) iStart = iStartMax; // quick and ...quick
    
    nEnum->_Enumerate(ShowFunc,mode,0);

    iStartMax = iCount;
    
    if(iStart - iCount > cLines) iStart = iCount; // don't want to end up in nowhere...
    while(++iCount <= (iStart+cLines)) printf("^^^^\n");
}

void TermResize(int i)
{
    if(i == SIGWINCH)
    {
    	winsize w;
	ioctl(0,TIOCGWINSZ,&w);
	cLines = w.ws_row - 3;

	if(nEnum) DisplayIt();
    }
}

termios t_old,t_new;

char *ReadString()
{
    int i;
    static char str[256];

    fflush(stdout);
    tcsetattr(0, TCSANOW , &t_old);
    i = read(0,str,250);
    str[i ? i-1:0] = 0;
    tcsetattr(0, TCSANOW , &t_new);
    
    return str;
}

bool CountFunc(Node *n,int flags,void *pv)
{
//    printf("%s\n",n->GetPrompt());
    int *p = (int*)pv;
    (*p)++;
    return 1;
}
bool SearchFunc(Node *n,void *str);
void Show11(NodeRoot *nr)
{
    // catch SIGWINCH
    struct sigaction act;
    act.sa_handler = TermResize;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGWINCH,&act,0);
    TermResize(SIGWINCH);
    
    // setup terminal
    tcgetattr(0,&t_old);
    t_new = t_old;
    t_new.c_lflag &= ~ICANON;
    t_new.c_lflag &= ~ECHO;
    t_new.c_lflag &= ~ISIG;
    t_new.c_cc[VMIN] = 1;
    t_new.c_cc[VTIME]= 0;
    tcsetattr(0, TCSANOW , &t_new);

    int iDep=0;
    Node *ndep=0;
    char ch[7];
    nCurrent = nr;
    nEnum=nr; // enum node

next:
    DisplayIt();
readit:
    ch[0] = 0;
    read(0,ch,6);

    switch(ch[0])
    {
    // extended keys	
    case 0x1B: // esc
    {
	if(ch[1] != '[') goto readit;
	switch(ch[2])
	{
	case 'B': iStart++; break;				// down
	case 'A': iStart--; if(iStart<1) iStart=1; break;	// up
	case '6': iStart+=cLines; break;			// pgdn
	case '5': iStart-=cLines; if(iStart<1) iStart=1; break;	// pgup
	case 'C': goto inm;					// right
	case 'D': goto outm;					// left
	case '2': nCurrent->Set(3); break;				// ins = Y
	case '3': nCurrent->Set(1); break;				// del = N
	case 'H': iStart = 1; break;				// home
	case 'F': iStart = (iCount - cLines) - 1; break;	// end
	default: goto readit;
	}
	goto next;
    }
   
    // movement
    case '\n':
    {
inm:	if(nCurrent->GetType() & NTT_PARENT)
	{
	    NodeParent *p = (NodeParent*)nCurrent;
	    Node *n = p->GetChild();
	    if(!n) goto readit;
	    nEnum = n;
	    iStart=1;
	}
	break;
    }
    case 'p':
    {
outm:	Node *n = nCurrent->GetParent(0,NTT_VISIBLE);
	if(!n) goto readit;
	nStart = nEnum = n;
	n = n->GetParent(0,NTT_VISIBLE);//->GetChild(); // set to start
	if(n) nEnum = n->GetChild();
	iStart=100000;
	break;
    }

    // value changes
    case '1': nCurrent->Set(1); break;
    case '2': nCurrent->Set(2); break;
    case '3': nCurrent->Set(3); break;
    case ' ':
    {
	if(nCurrent->GetType() & NTT_STR)
	{ printf("New value: "); nCurrent->Set(ReadString()); } else
	{ nCurrent->Advance(); nStart = nCurrent; iStart = 100000; }
	break;
    }

    // view settings
    case 'a':
    {
	mode ^= NS_DOWN;
	if(mode & NS_DOWN)
	{ nEnum=nr; bMenuMode = 0; } else
	{ 
	    bMenuMode = 1;
	    if(nCurrent->GetType() == NT_ROOT) nEnum = nCurrent; else
		nEnum=nCurrent->GetParent(0,NTT_VISIBLE)->GetChild();
	}
	nStart = nCurrent; iStart = 100000;
	break;
    }
    case 'd':
    {
	mode ^= NS_DISABLED;
	goto rrrr;
    }
    case 's':
    {
	mode ^= NS_SKIPPED;
rrrr:
	nStart = nCurrent; iStart = 100000;
	
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
	printf("File to load: ");
	char *str = ReadString();
	printf("Loading: <%s>....\n",str);
	if(nr->Load(*str=='#' ? 0 : str))
	    printf("....OK\n"); else printf("....Failed\n");
	goto rrrr;
    }
    case 'S':
    {
	printf("File to save: ");
	char *str = ReadString();
	printf("Saving: <%s>....\n",str);
	if(nr->Save(*str=='#' ? 0 : str))
	    printf("....OK\n"); else printf("....Failed\n");
	break;
    }

    // help
    case 'h':
    {
	printf("\n\n----HELP----\n\n\n%s\n%s\n%s:%i\n\n%s\n",
		nCurrent->GetPrompt(),nCurrent->GetSymbol(),
		nCurrent->GetSource(),nCurrent->GetLine(),nCurrent->GetHelp());
	read(0,ch,1);
	break;
    }
    case 'x': // show dep tree
    {
	if(ndep) { iStart = iDep; nEnum = ndep; ndep=0; break; }

	printf("--------------------\n");
	Node *n = nCurrent->GetDepTree();
	Node *c = n->GetChild(); if(c) n = c;
	iDep = iStart; iStart = 1;
	ndep = nEnum; nEnum = n;
	printf("--------------------\n");
	break;
    }

    // diagnostics
    case 'H': // show options's that don't have help.
    {
	nr->Enumerate(NoHelpFunc,NS_ALL,0);
	read(0,ch,1);
	break;
    }
    case 'Q':
    {
	printf("Query promts: ");
	char *str = ReadString();
	printf("Searching for: <%s>....\n\n",str);

	Node *n = nr;

	// Enumerate would be better for this,
	// but I need to test Search
	int i=0,j=0;
//	nr->Enumerate(CountFunc,NS_ALL,&j);
	while(1)
	{
	    n = n->Search(SearchFunc,str);
	    if(!n) break;
            printf("%s\n",n->GetPrompt());
	    i++;
	}//*/
	printf("----\nDone: %i, enum: %i\n",i,j);
	read(0,ch,1);
	break;
    }

   
    case 'q': // quit
    {
	tcsetattr(0, TCSANOW , &t_old);
	return;
    }   
    default:
	goto readit;
    }
    goto next;
}

bool SearchFunc(Node *n,void *str)
{
    char *c = n->GetPrompt();
    int i = strlen((char*)str);
    while(*c)
    {
	if(!strncmp((char*)str,c++,i)) return 1;
//	if(*c++ == *((char*)str)) return 1; // only one char
    }
    return 0;
}

//-------------------------------------------------------------------

int cnt[SYMBOL_LIMIT];


bool DiagFunc(Node *n,int flags,void *pv)
{
    if(!n->GetConfig()) return true;

    if(n->GetDeps() > 1) printf("dep:%3i %s:%s\n",n->GetDeps(),n->GetSymbol(),n->GetPrompt());
    cnt[n->GetConfig()]++;
    return true;
}

bool Diag2Func(Node *n,int flags,void *pv)
{
    if(!n->GetConfig()) return true;
    
    if(cnt[n->GetConfig()] > 1)
    {
	printf("def:%3i %s:%s\n",cnt[n->GetConfig()],n->GetSymbol(),n->GetPrompt());
    }
    return true;
}

/*bool Diag3Func(Node *n,int flags,void *pv)
{
    if(n->wordlookup)
	printf("lup:    %s:%s\n",n->GetSymbol(),n->GetPrompt());
    return true;
}*/

void Diag(NodeRoot *nr)
{
    for(int i=0; i<SYMBOL_LIMIT; i++) cnt[i]=0;
    printf("--------------------\n");
    nr->Enumerate(DiagFunc,NS_ALL,0);
    printf("--------------------\n");
    nr->Enumerate(Diag2Func,NS_ALL,0);
    printf("--------------------\n");
//    nr->Enumerate(Diag3Func,NS_ALL,0);
//    printf("--------------------\n");
}

//-------------------------------------------------------------------

int main(int argc,char *argv[])
{
//-------------------------------------
    nr = new NodeRoot();

    int ret = nr->Init_CmdLine(argc,argv);
    if(ret & 7) { if(ret & 3) printf("Init failure.\n"); delete nr; return 1; }
    nr->SetNotify(NotifyFunc);

    // use tconfig -d | sort | uniq
    // to see all redefinitions and dependencies
    if(argc > 1 && *argv[1] == '-')
    {
	switch(argv[1][1])
	{
	case 'd':
	    Diag(nr);
	    break;
	default:
	    printf("Unknown option %c\n",argv[2][1]);
	    Show11(nr);
	}
    }else
    {
	Show11(nr);
    }
    int scount = nr->GetSymbols()->GetCount();

    delete nr;
//-------------------------------------
// Stats

    printf("\n");
    printf("Nodes allocated: %i, freed: %i\n",nnew,nfree);
    printf("ParentNodes allocated: %i, freed: %i\n",npnew,npfree);

    printf("Total files: %i.\n",FileTot);
    printf("Total Symbols: %i\n",scount);
    printf("Total lines parsed: %i\n",LineTot);
    
    return 0;
 
}


