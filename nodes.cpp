//nodes.cpp - nconfig kernel configuration backend.

//Copyright (C) 2004-2006 Rikus Wessels <rikusw at rootshell dot be>
//GNU GPL v2.0

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "nodes.h"


// Kconfig 2.6.x support
//#define LKC26 -- done in Makefile
#ifdef LKC26
#define LKC_DIRECT_LINK
#include "kconfig/lkc.h"
#endif



// search ==###$ for internals
// 	  --###$ for user api's
// 	    ###$ for all
//
// /#ifn*def NC24\|#else\|#endif \/\/NC24

int nnew=0; // node alloc
int nfree=0;
int npnew=0; // parent alloc
int npfree=0;

int FileTot=0; // total files
int LineTot=0; // total lines

// show dependencies on symbols not yet defined
//#define DEBUG_DEP_PRE_DEF

//---------------------------------------------------------------------------------------===###
// CfgFile

CfgFile::CfgFile(Node *n)
{
	Symbols = n->Symbols;
	FName[0] = 0;
	LineNum = 0;
	pmem = 0;
}

CfgFile::~CfgFile()
{
	FileTot++;
	LineTot += LineNum;
	Close();
}

int CfgFile::Close()
{
	if (pmem) {
		free(pmem);
	}
	pmem=0;
	return 1;
}

int CfgFile::ReadLine()
{
	// trim left
	while (LineNext < EndMem) {
		if (*LineNext != ' ' && *LineNext != '\x09') {
			break;
		}
		LineNext++;
	}
	LineStart = LineNext;

next:	// get the line
	while (LineNext < EndMem) {
		if (*LineNext == 0xa) {
			goto rOK;
		}
		if (*LineNext == '#') {
			*LineNext = 0;
		}
		LineNext++;
	}
	if (LineNext > EndMem) {
		return 0; // reading after eof...
	}
rOK:
	*LineNext++ = 0;
	LineNum++;

	if (*(LineNext-2) == '\\') {
		*(LineNext-1) = ' ';
		*(LineNext-2) = ' ';
		goto next;
	}

	// trim right
	char *p = LineNext;
	while (--p >= LineStart) {
		if (*p == ' ' || *p == '\x09') {
			*p = 0;
		} else {
			break;
		}
	}

	LineCurrent = LineStart;
	return 1;
}

int CfgFile::StrCmpAdv(const char *s1, const char *s2, int cnt)
{
	for (int i = 0; i < cnt; i++) {
		if (s1[i] != s2[i]) {
			return 1;
		}
	}
	LineCurrent += cnt;
	return 0;
}

int CfgFile::Open(const char *strFile, bool bPath)
{
	int fd;
	struct stat st;
	char fn[1024];

	if (bPath) {
		if (*strFile == '/') {
			fn[0]=0;
		} else {
			strcpy(fn, Symbols->GetPath());
		}
		strcat(fn, strFile);
	} else {
		strcpy(fn, strFile);
	}

	fd = open(fn, O_RDONLY);
	if (fd == -1) {
		printf("Couldn't open <%s>.\n", strFile);
		return 0;
	}

	if (fstat(fd, &st) == -1) {
		printf("Couldn't get file stats <%s>.\n", strFile);
		goto endclose;
	}

	pmem = (void*)malloc(st.st_size+2); // pad end by 2
	if (!pmem) {
		printf("Couldn't malloc memory <%s>.\n", strFile);
		goto endclose;
	}
	cmem = (char*)pmem;
	cmem[st.st_size]=0;
	cmem[st.st_size+1]=0;
	LineNext=cmem;
	EndMem=&cmem[st.st_size];

	if (read(fd, pmem, st.st_size) != st.st_size) {
		printf("File read error <%s>.\n", strFile);
		goto endfree;
	}

	strcpy(FName, strFile);
	close(fd);
	return 1;

endfree:
	free(pmem); pmem=0;
endclose:
	close(fd);
	return 0;
}

int CfgFile::TestOpen(const char *strFile, bool bPath)
{
	int fd;
	char fn[1024];

	if (bPath) {
		if (*strFile == '/') {
			fn[0]=0;
		} else {
			strcpy(fn, Symbols->GetPath());
		}
		strcat(fn, strFile);
	} else {
		strcpy(fn, strFile);
	}

	fd = open(fn, O_RDONLY);
	if (fd == -1) {
		//printf("Couldn't open <%s>.\n", fn);
		return 0;
	}

	close(fd);
	return 1;
}

//-----------------------------------------------------------------------------###
// Parse helpers

#ifndef NC24

int CfgFile::Parse(NodeParent *np)
{
	return 0;
}

void CfgFile::ParseError(const char *msg)
{
}

#else

void CfgFile::ParseError(const char *msg)
{
	LineNext--; LineCurrent=LineStart;
	while (LineCurrent<LineNext) {
		if (!*LineCurrent) {
			*LineCurrent=' ';
		}
		LineCurrent++;
	}
	LineNext++;
	printf("%s\n<%s> %i: %s\n", msg, FName, LineNum, LineStart);
}

#define TRIM while (LineCurrent<LineNext && (*LineCurrent==' ' || *LineCurrent=='\x09'))\
		LineCurrent++;

#define ADDNODE(str, l, nt, type) \
if (!StrCmpAdv(LineCurrent, str, l))\
{\
	Node##nt *n = new Node##nt(type);\
	np->AddChild(n);\
	if (!((Node*)n)->Parse(LineCurrent, LineNum)) {\
		ParseError("Node"#nt"->Parse failed."); return 0; }\
	continue;\
}

// Parse helpers
//-----------------------------------------------------------------------------

int CfgFile::Parse(NodeParent *np)
{
	while (ReadLine()) {
		if (*LineStart ==  0 ) {
			continue; // empty lines + comments
		}
//-----------------------------------------------------------------------------
// Mainmenu

		if (!StrCmpAdv(LineStart, "mainmenu_option ", 16)) {
			if (StrCmpAdv(LineCurrent, "next_comment", 12)) {
				ParseError("<next_comment> expected after mainmenu_option");
				return 0;
			}

			while (ReadLine() && *LineStart == 0); // catch blank lines
			if (StrCmpAdv(LineStart, "comment ", 8)) {
				ParseError("<comment > expected after mainmenu_option");
				return 0;
			}

			NodeMenu *p = new NodeMenu();
			np->AddChild(p);
			if (!((Node*)p)->Parse(LineCurrent, LineNum)) {
				ParseError("menu comment parse error");
				return 0;
			}

			if (!Parse(p)) {
				return 0; // recurse
			}
			continue;
		}

		if (!StrCmpAdv(LineStart, "endmenu", 7)) {
			if (np->type != NT_MENU)	{
				ParseError("Unexpected <endmenu>, expecting <fi>.");
				return 0;
			}
			return 1;
		}

//-----------------------------------------------------------------------------
// if

		if (!StrCmpAdv(LineStart, "if", 2)) {
			TRIM // check for [
			if (*LineCurrent !='[') {
				ParseError("[ expected after if."); return 0;
			}
			LineCurrent++;

			// check for ] and terminate string
			char *s= LineCurrent ; // set start for parse func
			while (LineCurrent < LineNext && *LineCurrent != ']') {
				LineCurrent++;
			}
			if (*LineCurrent!=']') {
				ParseError("] expected after if [."); return 0;
			}
			*LineCurrent++=0;

			TRIM // check for ;
			if (*LineCurrent++!=';') {
				ReadLine(); // is 'then' on the next line ?
			}

			TRIM // check for then
			if (StrCmpAdv(LineCurrent, "then", 4)) {
				ParseError("then expected after if[]"); return 0;
			}

			NodeIf *p = new NodeIf();
			np->AddChild(p);
			if (!((Node*)p)->Parse(s, LineNum)) {
				ParseError("If parsing."); return 0;
			}

			if (!Parse(p)) {
				return 0; // recurse
			}
			continue;
		}

		if (!StrCmpAdv(LineStart, "else", 4)) {
			NodeIf *ni=(NodeIf*)np;
			if (np->type!=NT_IF || ni->bElse) {
				ParseError("Unexpected <else>.");
				return 0;
			}
			ni->bElse=1;
			continue;
		}


		if (!StrCmpAdv(LineStart, "fi", 2)) {
			if (np->type != NT_IF) {
				ParseError("Unexpected <fi>, expecting <endmenu>.");
				return 0;
			}
			return 1;
		}

//-----------------------------------------------------------------------------$$$

		ADDNODE("bool", 4, Tri, NT_BOOL)
		ADDNODE("hex", 3, Str, NT_HEX)
		ADDNODE("int", 3, Str, NT_INT)
		ADDNODE("string", 6, Str, NT_STRING)
		ADDNODE("tristate", 8, Tri, NT_TRISTATE);

//-----------------------------------------------------------------------------$$$

		if (!StrCmpAdv(LineStart, "define_", 7)) {
			ADDNODE("bool", 4, Def, NT_DEFBOOL)
			ADDNODE("hex", 3, Def, NT_DEFHEX)
			ADDNODE("int", 3, Def, NT_DEFINT)
			ADDNODE("string", 6, Def, NT_DEFSTRING)
			ADDNODE("tristate", 8, Def, NT_DEFTRISTATE)
			goto unrec;
		}

//-----------------------------------------------------------------------------

		if (!StrCmpAdv(LineStart, "dep_", 4)) {
			ADDNODE("bool", 4, Dep, NT_DEPBOOL)
			ADDNODE("mbool", 5, Dep, NT_DEPMBOOL)
			ADDNODE("tristate", 8, Dep, NT_DEPTRISTATE)
			goto unrec;
		}

//-----------------------------------------------------------------------------

		ADDNODE("source", 6, Source, ) // file including
		ADDNODE("comment", 7, Comment, )
		ADDNODE("choice", 6, ChoiceP, )
		ADDNODE("text", 4, Comment, )
		ADDNODE("unset", 5, Unset, )
#ifdef USE_NCHOICE
		ADDNODE("nchoice", 7, ChoiceNP, )
#endif

//-----------------------------------------------------------------------------

		if (!StrCmpAdv(LineStart, "mainmenu_name", 13)) { // should occur only once...
			NodeParent *p;
			if (np->GetType() == NT_SOURCE) {
				p = np;
			} else {
				p = np->GetParent(NT_SOURCE);
			}
			if (p->GetParent()->GetType() == NT_ROOT) {
setmm:				Node *n = p->GetParent(); // will be root->source
				if (!n->Parse(LineCurrent, LineNum))
					ParseError("mainmenu_name parse error.");
				continue;
			} else {
				// mips fixup where this is in a second file
				if (p->GetType() == NT_SOURCE &&
				    !::strncmp(np->prompt, "arch/", 5)) {
					p = p->GetParent(NT_SOURCE);
					if (p->GetParent()->GetType() == NT_ROOT) {
						goto setmm;
					}
				}
				ParseError("Unexpected <mainmenu_name>");
				return 0;
			}
		}

//-----------------------------------------------------------------------------
// unrecognized commands
unrec:
		ParseError("Unrecognized command.");
		return 0;

//-----------------------------------------------------------------------------
// end of parse loop
	}//while

	// Missing fi or endmenu at eof....
	switch(np->type) {
	case NT_MENU:
		ParseError("Expecting <endmenu>.");
		return 0;
	case NT_IF:
		ParseError("Expecting <fi>.");
		return 0;
	default:
		break;
	}
	return 1;
}

#undef TRIM
#undef ADDNODE

#endif //NC24

// CfgFile
//---------------------------------------------------------------------------------------===###
// Symbols Node

NodeSymbols::NodeSymbols(const char *Arch, NodeRoot *r)
{
	type = NT_SYMBOL;
	Root = r;
	path = 0;
	Ntfy = 0;
	Count = 5;
	HelpFile = 0;

	val[0] = 0; syms[0] = "\0	  ";//<Undefined>
	val[1] = 1; syms[1] = "n\0	 ";
	val[2] = 2; syms[2] = "m\0	 ";
	val[3] = 3; syms[3] = "y\0	 ";

	arch = (char*)malloc(strlen(Arch)+1);
	strcpy(arch, Arch);

	val[4] = (uintptr_t)arch;
	syms[4] = "ARCH"; // a hack for $ARCH usage (3 times = mips sparc sparc64)

	int i;
	for (i = 5; i < SYMBOL_LIMIT; i++) {
		val[i] = 0;
		syms[i] = 0;
	}
	for (i = 0; i < SYMBOL_LIMIT; i++) {
		dep[i] = 0;
		who[i] = 0;
		help[i] = 0;
	}
	AddSymbol("CONFIG_MODULES", 1); // added into 5;
}

inline bool NodeSymbols::HasModules()
{
	return val[5] == 3; // CONFIG_MODULES == y ?
}

NodeSymbols::~NodeSymbols()
{
	free(arch);
	if (HelpFile) {
		delete HelpFile;
	}
	if (path) {
		free(path);
	}
	for (int i = 5; i < Count; i++) {
		free(syms[i]);
	}
}

bool NodeSymbols::SetPath(const char *s)
{
	DIR *ArchDir;
	char fn[500];

	// default path
	if (s == 0 || *s == 0) {
		s = "/usr/src/linux/";
	}
	// set it
	if (path) {
		free(path);
	}
	path = (char*)malloc(strlen(s)+2);
	if (!path) {
		return 0;
	}
	strcpy(path, s);
	if (*s && s[strlen(s)-1] != '/') {
		strcat(path, "/");
	}

	// valid ?
	strcpy(fn, s);
	if (*fn && fn[strlen(fn)-1] != '/') {
		strcat(fn, "/");
	}
	strcat(fn, "arch");
	ArchDir = opendir(fn);
	if (!ArchDir) {
		return 0;
	}
	closedir(ArchDir);
	return 1;
}

int NodeSymbols::AddSymbol(char *s, uintptr_t v)
{
	if (Count >= SYMBOL_LIMIT) {
		printf("Symbol limit exceeded.\n");
		return 0;
	}
	if (*s == '$') {
		s++;
	}

	char *s1, *s2;
	// is it already added ?
	if (!strncmp(s, "CONFIG_", 7)) { // make the parse +-3 times faster
		for (int i = Count-1; i > 4; i--) {
			s1 = syms[i] + 7;
			s2 = s + 7;
			while (*s1++ == *s2++) // much faster than strcmp
			if (!*s1 && !*s2) {
				if (v) {
					if (dep[i] & 0x80000000) {
						dep[i] |= 0x40000000;
					}
					val[i] = v;
					dep[i] |= 0x80000000;
				} else {
					val[i] = 1;
					dep[i]++;
				}
				return i;
			}
		}
	} else {
		if (!s[1]) switch(*s) {
		case 'n': return 1;
		case 'm': return 2;
		case 'y': return 3;
		}
		for (int i = Count-1; i > 3; i--) {
			s1 = syms[i]; s2 = s;
			while (*s1++ == *s2++) // much faster than strcmp
			if (!*s1 && !*s2) {
				if (v) {
					val[i] = v;
					dep[i] |= 0x80000000;
				} else {
					val[i]=1;
					dep[i]++;
				}
				return i;
			}
		}
	}
	char *p;
	int l = strlen(s)+1;
	if (l < 10) {
		p = (char*)malloc(10);
		memset(p, 0, 10);
	} else {
		p = (char*)malloc(l);
	}
	if (!p) {
		printf("AddSymbol malloc fail.\n");
		return 0;
	}
	strcpy(p, s);
	syms[Count] = p;
	if (v) {
		val[Count] = v;
		dep[Count] |= 0x80000000;
	} else {
		val[Count] = 1;
		dep[Count]++;
	}
	return Count++;
}

void NodeSymbols::Reset(int s, Node *n)
{
	if (s < 5 || s >= Count) {
		return;
	}
	// who dunnit ?   \/ nobody so guilty anyway... used for loading
	if (who[s] == n || !who[s]) {
		if (n->GetType() & NTT_STR) {
			val[s] = (uintptr_t)"";
		} else {
			val[s] = 1; // guilty...
		}
	}
}

bool NodeSymbols::Set(int s, uintptr_t v, Node *n)
{
	if (s < 5 || s >= Count) {
		return 0;
	}
	if (v == 2 && !HasModules()) {
		v = 3;
	}

/*XXX	if (who[s] != n && who[s])
		printf("override from: %s:%i to: %s:%i\n",
				who[s]->GetPrompt(), who[s]->Get(), n->GetPrompt(), n->Get());//*/
	who[s] = n;
	val[s] = v;
	return 1;
}

int NodeSymbols::GetDeps(int s)
{
	if (s>=Count) {
		return 0;
	}
	return dep[s] & 0xFFFF;
}

uintptr_t NodeSymbols::Get(int s)
{
	if (s>=Count) {
		return 0;
	}
	return val[s];
}

char *NodeSymbols::GetSymbol(int s)
{
	if (s>=Count) {
		return 0;
	}
	return syms[s];
}

void NodeSymbols::Clear()
{
	for (int i = 5; i < Count; i++) {
		who[i] = 0;
		// skip constants allocated by NodeIf::Parse()
		if (val[i] != (uintptr_t)syms[i]) {
			val[i] = 1;
		}
	}
}

bool NodeSymbols::IsDefined(int s)
{
	if (s >= Count) {
		return 0;
	}
	if (dep[s] & 0x80000000) {
		return 1;
	}
	return 0;
}

bool NodeSymbols::IsRedefined(int s)
{
	if (s >= Count) {
		return 0;
	}
	if (dep[s] & 0x40000000) {
		return 1;
	}
	return 0;
}

void NodeSymbols::UnloadHelp()
{
	delete HelpFile;
	HelpFile = 0;
	for (int i = 0; i < SYMBOL_LIMIT; i++) {
		help[i]=0;
	}
}

char *NodeSymbols::GetHelp(int s)
{
	if (!s) {
		return 0; //"No help available."; // prevent unneeded load
	}
	if (!HelpFile) {
		HelpFile = new CfgFile(this);
		if (!HelpFile->LoadHelp(Symbols)) {
			delete HelpFile;
			HelpFile = 0;
			printf("Load help fail.\n");
			return "Documentation/Configure.help is missing.";
		}
		//for (int i=0; i<Count; i++) if (!help[i]) help[i]="No help available.";
	}
	if (s>=Count) {
		return "Invalid query."; // shouldn't ever happen...
	}
	return help[s];
}

void NodeSymbols::AddHelp(char *cfg, char *helpstr)
{
	char *s1, *s2;
	// is it already added ?
	if (!::strncmp(cfg, "CONFIG_", 7)) { // make the search +-3 times faster
		 for (int i = Count-1; i > 4; i--) {
			s1 = syms[i] + 7;
			s2 = cfg + 7;
			while (*s1++ == *s2++) // much faster than strcmp
			if (*s1==0 && (*s2==0x0A || *s2==' ' || *s2=='\t' || *s2==0)) {
				help[i] = helpstr;
				return;
			}
		}
	} else {
		cfg[40] = 0;
		for (int i = 0; i < 40; i++) {
			if (cfg[i] == '\n') {
				cfg[i] = 0;
				break;
			}
		}
		printf("Invalid AddHelp <%s>\n", cfg);
	}
	return;
}

int CfgFile::LoadHelp(NodeSymbols *Symbols)
{
	if (!Open("Documentation/Configure.help")) {
		return 0;
	}
	char *LineEnd;
	char *term = LineNext;

	while (LineNext < EndMem) {
		LineStart = LineNext;
		while (1) { // get a line
			if (LineNext == EndMem) {
				LineEnd = LineNext;
				break;
			}
			if (*LineNext++ == '\n') {
				LineEnd = LineNext - 1;
				break;
			}
		}

		switch(*LineStart) {
			case '#':
				*term = 0;
			case '\n':
				continue;
			case ' ':
			case '\t':
				term = LineEnd;
				continue;
			default:
			{
				*term = 0;
				if (*LineStart == 'C' && // CONFIG_ ?
				   (*LineNext == ' ' || *LineNext == '\t' || *LineNext == '\n')) {
					Symbols->AddHelp(LineStart, LineNext);
					term = LineNext;
				}
			}
		}
	} // while

	*term = 0;
	return 1;
}

// Symbols
//---------------------------------------------------------------------------------------===###
// Constructors & Destructors

Node::Node()
{
	nnew++;
	type = NT_NODE;

	Next = Parent = 0;

	prompt = 0;
	prevword = ~1;
	word = 0;
	wordlookup = 0;

	Config = 0;
	state = 0;
	line = 0;
	user = 0;
}

Node::~Node()
{
	nfree++;
	if (Next) {
		delete Next;
	}
	Next = 0;

	if (prompt) {
		free(prompt);
	}
	if (word > 3) {
		free((void*)word);
	}
}

NodeParent::NodeParent()
{
	npnew++;
	type = NT_PARENT;

	Child = Last = 0;
}

NodeParent::~NodeParent()
{
	npfree++;
	if (Child) {
		delete Child;
	}
	Child = 0;
}

NodeRoot::NodeRoot()
{
	pHelpMem = 0;
	bAutoFreeH = 1; // on by default
	bLkc26 = false;
	type = NT_ROOT;
	prompt = (char*)malloc(10);
	strcpy(prompt, "NodeRoot"); // DON'T use Parse here...
}


void Node::SetPrompt(const char *s)
{
	if (prompt) {
		free(prompt);
	}
	prompt = (char *)malloc(strlen(s)+1);
	if (!prompt) {
		return;
	}
	strcpy(prompt, s);
}


#ifdef NC24
NodeIf::NodeIf()
{
	Else = 0;
	bElse = 0;
	type = NT_IF;
	Cond = xxcond;
	Count = 0;
	CountLimit = 13;
}

NodeIf::~NodeIf()
{
	if (Else) {
		delete Else;
	}
	Else = 0;
	if (Cond != xxcond) {
		free(Cond);
	}
	Cond = xxcond;
}
#endif //NC24

// Constructors & Destructors
//---------------------------------------------------------------------------------------===###
// Parsing Functions is only supported on NC24
#ifndef NC24

bool NodeMenu::Parse(char *s, int l) // mainmenu_option - comment
{
	return 0;
}

bool NodeRoot::Parse(char *s, int l) // mainmenu_name
{
	return 0;
}

bool NodeSource::Parse(char *s, int l)
{
	return 0;
}

char *Node::ParsePrompt(char *s)
{
	return 0;
}

char *Node::ParseSymbol(char *s)
{
	return 0;
}

char *Node::ParseWord(char *s)
{
	return 0;
}

void  Node::DefPrompt()
{
}

#else

#define TRIMs while ((*s == ' ' || *s == '\x09') && *s != 0) s++;
#define NEXTs while (*s) { if (*s == ' ' || *s == '\x09') { *s++ = 0; break; } s++; };
#define GETWORDs(ptr)\
	if (*s == '\'' || *s == '\"') stop = *s++; else stop = 0; ptr = s;\
	while (1)\
	{\
		if (*s == stop) { if (stop) { *s++ = 0; } break; }\
		if (*s == 0) { printf("%c expected after /word/.\n", stop); return 0; } s++;\
	}


char *Node::ParsePrompt(char *s)
{
	int i = 0;
	char stop;
	char *b;

	TRIMs
	if (*s != '\'' && *s != '\"') {
		printf("\' or \" expected.\n");
		return 0;
	}
	stop = *s++;
	if (!Symbols->bPromptSpaces) {
		TRIMs
		b=s;
	}
	while (1) {
		if (*s == stop) {
			*s++ = 0;
			i++;
			break;
		}
		if (*s==0) {
			printf("%c expected after /prompt/.\n", stop);
			return 0;
		}
		s++;
		i++;
	}
	if (prompt) {
		free(prompt);
	}
	prompt = (char*)malloc(i);
	if (!prompt) {
		return 0;
	}
	strcpy(prompt, b);

	return s;
}

char *Node::ParseSymbol(char *s)
{
	TRIMs char *b = s;
	if (strncmp(s, "CONFIG_", 7)) {
		printf("<CONFIG_> expected.");
		return 0;
	}
	NEXTs

	Config = Symbols->AddSymbol(b, 1);
	if (!Config) {
		return 0;
	}
	return s;
}

char *Node::ParseWord(char *s)
{
//	return s;
	char stop;
	char *b;
	
	TRIMs
	GETWORDs(b)

reget:	// for 2.2 buggy Config.in
	// where 'int' and 'hex' is sometimes defined as:
	// <prompt symbol $word word> Where $word is used for what ????
	// <prompt symbol word> is what it should be.

	word = 0;
	if (*(b+1) == 0) {
		wordlookup = 0;
		if (*b == 'n') word = 1; else
		if (*b == 'm') word = 2; else
		if (*b == 'y') word = 3; else goto next;
		Symbols->Set(Config, word, this);
		return s;
	}

	if (*b == '$') { // lookup
		if (type & NTT_INPUT) {
			//printf("wordlookup not allowed in input node.\n"); return 0; 
			printf("$lookup ignored in %s:%i\n", GetSource(), line);
			while (*b && *b++!=' '); // skip past first space
			goto reget;
		}			
		wordlookup = Symbols->AddSymbol(++b, 0);
		word = Symbols->Get(wordlookup);

#ifdef DEBUG_DEP_PRE_DEF
		if (!Symbols->IsDefined(wordlookup)) { // debug
			printf("(word) %s dep on: %s which isn't defined.\n",
					Symbols->GetSymbol(wordlookup), b);
		}
#endif
		if (word>3) {
			char *c = (char*)malloc(strlen((char*)word)+1);
			strcpy(c, (char*)word); word = (uintptr_t)c;
		}
		Symbols->Set(Config, word, this);
	} else {
next:		wordlookup = 0;
		char *c = (char*)malloc(strlen(b)+1);
		strcpy(c, b); word = (uintptr_t)c;
		Symbols->Set(Config, word, this);
	}
//	printf("<WORD>: %s\n", b);

	return s;
}

//---------------------------------------------------------

char *NodeDep::ParseDeps(char *s)
{
	char *b;

	TRIMs
next:
	if (*s != 'm' && strncmp(s, "$CONFIG_", 8)) {
		printf("<$CONFIG_> or 'm' expected.\n");
		return 0;
	}
	b=s;
	NEXTs

	int xx;
	if (!AddDep(xx = Symbols->AddSymbol(b, 0))) {
		return 0;
	}
#ifdef DEBUG_DEP_PRE_DEF
	if (!Symbols->IsDefined(xx)) {
		printf("(dep) %s dep on: %s which isn't defined.\n", Symbols->GetSymbol(xx), b);
	}
#endif

	TRIMs // needed ???
	if (*s) {
		goto next;
	}
	return s;
}

bool NodeDep::AddDep(int i)
{
	if (!i) {
		return 0;
	}
	DepList[DepCount++]=i;
	if (DepCount > 9) {
		printf("Error depcount exceeded.");
		return 0;
	}
	return 1;
}

//---------------------------------------------------------

// only to be used from NodeDef*****::Parse();
void Node::DefPrompt()
{
	char buf[200];
	if (!(type & NTT_DEF)) {
		return;
	}
	if (wordlookup) {
		snprintf(buf, 190, "Def: %s = %s", Symbols->GetSymbol(Config), Symbols->GetSymbol(wordlookup));
	} else {
		if (word > 3) {
			snprintf(buf, 190, "Def: %s = %s", Symbols->GetSymbol(Config), (char*)word);
		} else {
			char *conv = "0nmy";
			snprintf(buf, 190, "Def: %s = <%c>", Symbols->GetSymbol(Config), conv[word]);
		}
	}
	SetPrompt(buf);
}

// Parsing Functions
//---------------------------------------------------------------------------------------===###
// Node Parsing

#define PARSEPROMPT s = ParsePrompt(s); if (!s) return 0;
#define PARSESYMBOL s = ParseSymbol(s); if (!s) return 0;
#define PARSEWORD   s = ParseWord(s);   if (!s) return 0;
#define PARSEDEPS   s = ParseDeps(s);   if (!s) return 0;

bool NodeSource::Parse(char *s, int l)
{
	line = l;
	TRIMs
	prompt = (char*)malloc(strlen(s)+1);
	if (!prompt) {
		return 0;
	}
	strcpy(prompt, s);

	CfgFile cfile(this);
	if (cfile.Open(s)) {
		int ret;
		ret = cfile.Parse(this);
		cfile.Close();
		if (!ret) return 0;
	} else {
		printf("NodeSource file open fail: %s\n", s);
		return 0;
	}
	return 1;
}

//---------------------------------------------------------

bool NodeRoot::Parse(char *s, int l) // mainmenu_name
{
	line = l;
	PARSEPROMPT
	return 1;
}

bool NodeMenu::Parse(char *s, int l) // mainmenu_option - comment
{
	line = l;
	PARSEPROMPT
	return 1;
}

bool NodeComment::Parse(char *s, int l)
{
	line = l;
	PARSEPROMPT
	return 1;
}

//---------------------------------------------------------

bool NodeTri::Parse(char *s, int l)
{
	line = l;
	PARSEPROMPT
	PARSESYMBOL
	word=1;
	return 1;
}

bool NodeStr::Parse(char *s, int l)
{
	line = l;
	PARSEPROMPT
	PARSESYMBOL
	PARSEWORD
	return 1;
}

//---------------------------------------------------------

bool NodeDep::Parse(char *s, int l)
{
	line = l;
	PARSEPROMPT
	PARSESYMBOL
	PARSEDEPS
	word=1;
	return 1;
}

bool NodeDef::Parse(char *s, int l)
{
	line = l;
	PARSESYMBOL
	PARSEWORD
	DefPrompt();
	return 1;
}

bool NodeUnset::Parse(char *s, int l)
{
	line = l;
	PARSESYMBOL
	word = 1;
	DefPrompt();
	return 1;
}

//---------------------------------------------------------

bool NodeChoice::Parse(char *s, int l) {
	line = l;
	PARSEPROMPT
	PARSESYMBOL
	word=1;
	return 1;
}

bool NodeChoiceP::Parse(char *s, int l)
{
	line = l;
	PARSEPROMPT

	char stop = 0;
	char *w1, *w2; // word 1+2

	// WORD1
	TRIMs
	GETWORDs(w1)

	// WORD2
	TRIMs
	stop=0;
	if (*s=='\'' || *s=='\"') {
		stop=*s++;
	}
	w2=s;
	while (1) {
		if (*s == stop || *s==' ' || *s=='\x09') {
			if (stop) {
				*s++=0;
			}
			break;
		}
		if (*s == 0) {
			printf("%c expected after /word/.\n", stop);
			return 0;
		}
		s++;
	}

	// Choice List - WORD1
	bool bFoundY = 0;
	s = w1;
	while (1) {
		TRIMs
		if (!*s) {
			break;
		}
		w1 = s-1;
		*w1 = '\''; // prefix with \'
		NEXTs
		*(s-1) = '\'';
		TRIMs
		NEXTs // <'prompt'   symbol>

		NodeChoice *n = new NodeChoice;
		AddChild(n);
		if (!n->Parse(w1, l)) {
			return 0;
		}
		if (!strncmp(n->prompt, w2, strlen(w2)) && !bFoundY)
		{
			bFoundY = 1;
			n->word = 3;
			n->DefaultChoice = 1;
			Symbols->Set(n->Config, 3, n); // set default
		}
	}
	if (!bFoundY && Child)
	{
		((NodeChoice*)Child)->word = 3; // one must be == y
		((NodeChoice*)Child)->DefaultChoice = 1;
		Symbols->Set(Child->GetConfig(), 3, Child);
	}
	return 1;
}

#ifdef USE_NCHOICE
bool NodeChoiceNP::Parse(char *s, int l)
{
	char *tmp;
	line = l;
	PARSEPROMPT
	PARSESYMBOL // default option

	bool bFoundY=0;
	while (1) {
		TRIMs
		if (!*s) {
			break;
		}
		tmp=s;
		NEXTs
		*(s-1)=' '; // gotcha! remove NEXTs null terminator
		TRIMs
		NEXTs // <'prompt'   symbol>

		NodeChoice *n = new NodeChoice;
		AddChild(n);
		if (!n->Parse(tmp, l)) return 0;

		if (n->Config == Config && !bFoundY)
		{
			bFoundY = 1;
			n->word = 3;
			n->DefaultChoice = 1;
			Symbols->Set(n->Config, 3, n); // set default
		}
	}
	if (!bFoundY && Child) {
		((NodeChoice*)Child)->word = 3; // one must be == y
		((NodeChoice*)Child)->DefaultChoice = 1;
		Symbols->Set(Child->GetConfig(), 3, Child);
	}
	Config = 0;
	word = 0;
	return 1;
}
#endif //USE_NCHOICE

//---------------------------------------------------------

bool NodeIf::Parse(char *s, int l)
{
	line = l;

/*	prompt = (char*)malloc(strlen(s)+1);
	if (!prompt) return 0;
	strcpy(prompt, s);//*/

	int s1, s2, op, inv; // <op=1 - = 0 - !=>  <op=2 - -a 0 - -o>
	char *b, stop;
	while (1) {
		inv=0;
		op=0;
nextx:
		TRIMs
		if (!*s) {
			break;
		}
		if (*s=='!') {
			s++;
			inv = 1;
			goto nextx;
		}
		stop = 0;
		GETWORDs(b)

		if (strncmp(b, "$CONFIG_", 8)) {
			if (strcmp(b, "$ARCH")) {
				printf("$CONFIG_ or $ARCH expected.<%s>\n", b);
				return 0;
			}
			s1 = 4; // $ARCH
		} else {
			s1 = Symbols->AddSymbol(++b, 0);
#ifdef DEBUG_DEP_PRE_DEF
			if (!Symbols->IsDefined(s1)) {
				printf("(-if-) %s isn't defined.\n", b);
			}
#endif
		}

		TRIMs
		if (!(*s == '=' || (*s == '!' && *(s+1) == '='))) {
			printf("= or != expected.\n");
			return 0;
		}
		if (*s == '=') {
			op |= 1;
			s++;
		} else {
			s += 2;
		}
		op ^= inv;

		TRIMs
		stop = 0;
		GETWORDs(b)

		if (*b == '$') {
			b++;
			s2 = Symbols->AddSymbol(b, 0);
#ifdef DEBUG_DEP_PRE_DEF
			if (!Symbols->IsDefined(s2)) {
				printf("(-if-s2) %s isn't defined.\n", b);
			}
#endif
		} else {
			if (!*(b+1)) {
				if (*b=='n') s2=1; else
				if (*b=='m') s2=2; else
				if (*b=='y') s2=3; else goto AddSym;
			} else {
				// if op2 == string - add it to the symbol table
				// and set both the name and value to string
AddSym:				s2 = Symbols->AddSymbol(b, 1);
				Symbols->Set(s2, (uintptr_t)Symbols->GetSymbol(s2), this);
			}
		}

		TRIMs

		if (*s && (*s++ != '-' && (*s != 'a' && *s != 'o'))) {
			printf("-a or -o expected<%s>\n", s);
			return 0;
		}
		if (*s == 'a') {
			op|=2;
		}
		if (*s) {
			s++;
		}
		if (!AddCondition(s1, s2, op)) {
			return 0;
		}
//		 printf("%i %c %i %c ", s1, (op&1) ? '=' : '!', s2, (op&2) ? '&' : '|');

	}
//	printf("%i\n", Count);//*/
	return 1;
}

bool NodeIf::AddCondition(int op1, int op2, int cond)
{
	if (Count >= CountLimit) { // reallocate ?
		CountLimit += 30; //printf("Reallocating if conds to: %i\n", CountLimit);
		Condition *c = (Condition*)malloc(sizeof(Condition)*CountLimit);
		if (!c) {
			return 0;
		}
		for (int j=0; j<Count; j++) {
			c[j]=Cond[j];
		}
		if (Cond != xxcond) {
			free(Cond);
		}
		Cond = c;
	}
	Cond[Count].op1=op1;
	Cond[Count].op2=op2;
	Cond[Count].cond=cond;
	Count++;
	return 1;
}

#undef PARSEPROMPT
#undef PARSESYMBOL
#undef PARSEWORD
#undef PARSEDEPS
#undef GETWORDs
#undef NEXTs
#undef TRIMs

#endif //NC24
// Node Parsing
//---------------------------------------------------------------------------------------===###
// Loading & Saving

bool NodeRoot::Load(const char *ConfigFile)
{
#ifdef LKC26
	if (bLkc26) {	// no segv's here!
		if (!ConfigFile || !*ConfigFile) {
			ConfigFile = 0;
		}
		bool b = !conf_read(ConfigFile);
		Update(1);
		Symbols->bModified = false;
		return b;
	}
#endif
	bool ret = 0;
#ifdef NC24
	CfgFile cfile(this);
	if (!ConfigFile) {
		char buf[255];
		snprintf(buf, 254, "arch/%s/defconfig", GetArch());

		if (cfile.Open(".config", 1)) {
			goto _if_;
		}
		if (cfile.Open(buf, 1)) {
			goto _if_;
		}
		printf("NodeRoot::Load: Couldn't Load .config or defconfig\n");
		goto end;
	}
	if (!*ConfigFile) { // == "" so clear all symbols
		Symbols->Clear();
		if (Child) {
			Child->Load();
		}
		ret = 1;
		goto end;
	}
	if (cfile.Open(ConfigFile, 0)) {
_if_:		if (cfile.LoadSymbols(Symbols)) {
			if (Child) {
				Child->Load();
			}
			ret = 1;
		} else {
			printf("NodeRoot::Load: Config file parse error.\n");
		}
		cfile.Close();
	} else {
		printf("NodeRoot::Load: Couldn't Load config file. <%s>\n", ConfigFile);
	}
end:
	if (ret) {
		if (Child) {
			Child->Update(0x19);
			Child->Update(1);
		} // init + normal
		Symbols->bModified = false;
	}
#endif //NC24
	return ret;
}

bool NodeRoot::Save(const char *ConfigFile)
{
	Symbols->bModified = false;
#ifdef LKC26
	if (bLkc26) { // no segv's here!
		if (!ConfigFile || !*ConfigFile) {
			ConfigFile = 0;
		}
		return !conf_write(ConfigFile);
	}
#endif
#ifndef NC24
	return 0;
#else
	FILE *cfg = 0, *cfgh = 0;
	if (ConfigFile) { // save user settings, else save to kernel
		if (!*ConfigFile || !::strcmp(ConfigFile, ".config") ||
			(strlen(ConfigFile) > 7 &&
			 !::strcmp(&ConfigFile[strlen(ConfigFile) - 8], "/.config"))) { //XXX XXX
			printf("NodeRoot::Save: Invalid config filename: <%s>\n", ConfigFile);
			return 0;
		}

		cfg = fopen(ConfigFile, "w");
		if (!cfg) {
			printf("NodeRoot::Save: Couldn't open <%s>\n", ConfigFile);
			return 0;
		}
		cfgh = 0;
	} else {
		char fn[1024];

		// setup the correct include link
		strcpy(fn, "cd ");
		strcat(fn, Symbols->GetPath());
		strcat(fn, "include;rm -f asm;ln -sf asm-");
		strcat(fn, Symbols->GetArch());
		strcat(fn, " asm;mkdir -p linux/modules");
		system(fn); // do it.

		strcpy(fn, Symbols->GetPath());
		strcat(fn, ".config");
		cfg = fopen(fn, "w");
		if (!cfg) {
			printf("NodeRoot::Save: Couldn't open \".config\"\n");
			return 0;
		}
		strcpy(fn, Symbols->GetPath());
		strcat(fn, "include/linux/autoconf.h");
		cfgh = fopen(fn, "w");
		if (!cfg) {
			printf("NodeRoot::Save: Couldn't open <include/linux/autoconf.h>\n");
			fclose(cfg);
			return 0;
		}
		fprintf(cfgh,
		"/*\n * Automatically generated by the nconfig backend, don't edit\n */\n"
		"#define AUTOCONF_INCLUDED\n");
	}

	fprintf(cfg, "#\n# Automatically generated by the nconfig backend, don't edit\n#\n");

	if (Child) {
		Child->Save(cfg, cfgh);
	}
	fclose(cfg);
	if (cfgh) {
		fclose(cfgh);
	}
	return 1;
#endif //NC24
}

#ifndef NC24

void Node::Load()
{
}

void Node::Save(FILE *c, FILE *h)
{
}

void NodeParent::Load()
{
}

void NodeParent::Save(FILE *c, FILE *h)
{
}

int CfgFile::LoadSymbols(NodeSymbols *Symbols)
{
	return 0;
}

#else
	
int CfgFile::LoadSymbols(NodeSymbols *Symbols)
{
	Symbols->Clear();
	while (ReadLine()) {
		if (*LineStart == 0) {
			continue;
		}
		if (strncmp("CONFIG_", LineStart, 7)) {
			printf("Load: CONFIG_ expected, not <%s>\n", LineStart);
			return 0;
		}
		while (LineCurrent<LineNext) {
			if (*LineCurrent == '=') {
				*LineCurrent++=0;
				break;
			}
			if (*LineCurrent == 0) {
				printf("Load: = expected.\n");
				return 0;
			}
			LineCurrent++;
		}

		uintptr_t val;
		if (*LineCurrent == '"') { // string
			val = (uintptr_t)++LineCurrent;
			while (LineCurrent<LineNext) {
				if (*LineCurrent == '"') {
					*LineCurrent++=0;
					break;
				}
				if (*LineCurrent == 0) {
					printf("Load: \" expected.\n");
					return 0;
				}
				LineCurrent++;
			}
		} else if (*LineCurrent == 'y') {
			val = 3;
		} else if (*LineCurrent == 'm') {
			val = 2;
		} else {
			val = (uintptr_t)LineCurrent; // a hex/dec number
		}

		int s = Symbols->AddSymbol(LineStart, 0);
		if (Symbols->IsDefined(s) && s) {
			Symbols->Set(s, val, 0);
		}
	}
	return 1;
}

//--------------------------------------------------------------------

void Node::Load()
{
	if (type & NTT_INPUT) {
		int p = state;
		state &= ~(NS_SKIPPED | NS_DISABLED);
		Set(Symbols->Get(Config), 0);
		prevword = word;
		state = p;
	}
	if (Next) {
		Next->Load();
	}
}

void Node::Save(FILE *c, FILE *h)
{
	char *sym;
	if (state & (NS_SKIPPED | NS_DISABLED)) {
		goto skipped;
	}
	sym = Symbols->GetSymbol(Config);
	if (type & NTT_STR) {
		uintptr_t ustr = Symbols->Get(Config);
		if (ustr < 4) {
			char xx[4][2] = {"", "n", "m", "y"};
			fprintf(c, "%s=%s\n", sym, xx[ustr]);
			if (h) {
				fprintf(h, "%s=%s\n", sym, xx[ustr]);
			}
		} else {
			char *str = (char*)ustr;
			if (type & NTT_STRING) {
				fprintf(c, "%s=\"%s\"\n", sym, str);
				if (h) {
					fprintf(h, "#define %s \"%s\"\n", sym, str);
				}
			} else {
				fprintf(c, "%s=%s\n", sym, str);
				if (type & NTT_HEX) {
					if (h) {
						fprintf(h, "#define %s 0x%s\n", sym, str);
					}
				} else {
					if (h) {
						fprintf(h, "#define %s (%s)\n", sym, str);
					}
				}
			}
		}
	}else if (type & NTT_NMY) {
		int i = Get();
		if (!i || i > 3) {
			i = 1;
		}
		if ((type & NTT_BOOL) && i == 2) {
			i = 3;
		}
		switch(i) {
		case 1:
			fprintf(c, "# %s is not set\n", sym);
			if (h) {
				fprintf(h, "#undef  %s\n", sym);
			}
			break;
		case 2:
			fprintf(c, "%s=m\n", sym);
			if (h) {
				fprintf(h, "#undef  %s\n", sym);
				fprintf(h, "#define %s_MODULE 1\n", sym);
			}
			break;
		case 3:
			fprintf(c, "%s=y\n", sym);
			if (h) {
				fprintf(h, "#define %s 1\n", sym);
			}
			break;
		}
	} else if (type == NT_COMMENT) {
		fprintf(c, "\n#\n# %s\n#\n", GetPrompt());
		if (h) { 
			fprintf(h, "/*\n * %s\n */\n", GetPrompt());
		}
	}
skipped:
	if (Next) {
		Next->Save(c, h);
	}
}

void NodeParent::Load()
{
	if (Child) {
		Child->Load();
	}
	if (Next) {
		Next->Load();
	}
}

void NodeParent::Save(FILE *c, FILE *h)
{
	if (type == NT_MENU && !(state & NS_SKIPPED)) {
		fprintf(c, "\n#\n# %s\n#\n", GetPrompt());
		if (h) {
			fprintf(h, "/*\n * %s\n */\n", GetPrompt());
		}
	}
	if (Child) {
		Child->Save(c, h);
	}
	if (Next) {
		Next->Save(c, h);
	}
}

void NodeIf::Load()
{
	if (Child) {
		Child->Load();
	}
	if (Else) {
		Else->Load();
	}
	if (Next) {
		Next->Load();
	}
}

void NodeIf::Save(FILE *c, FILE *h)
{
	if (Child) {
		Child->Save(c, h);
	}
	if (Else) {
		Else->Save(c, h);
	}
	if (Next) {
		Next->Save(c, h);
	}
}

#endif //NC24
// Loading & Saving
//---------------------------------------------------------------------------------------===###
// Update

#ifdef NC24
// hack for those redefinitions....
bool NtfyFunc(Node *n, int flags, void *pv)
{
	// Node::Notify()
	((enumNodes)pv)(n, NS_STATE, n->user);
	return 1;
}
#endif //NC24

void NodeRoot::Update(int i)
{
	if (Child && i&1) {
		Child->Update(1);
		if (bLkc26) {
			Child->Update(1);
		}
	}
	if (DepList) {
		DepList->Update(1);
	}
#ifdef NC24
	if (Symbols->Ntfy && !bLkc26) {
		Enumerate(NtfyFunc, NS_DOWN | NS_COLLAPSED, (void*)Symbols->Ntfy);
	}
#endif //NC24
}

void Node::IUpdate()
{
	Symbols->Root->Update(1);
}

// use only 1 in the API.
// 1 = down bit, used by NodeParent
// DON'T use the following in the API.
// 2 = up bit - deprecated - use IUpdate
// 3 = update all nodes following the current node, -- OBSOLETED by IUpdate
//	 use only on the node whose value changed
// 4 = skip bit, used by NodeIf-else //2.4
// 8 = skip/unskip?
//16 = initializing

#ifndef NC24
void Node::Update(int i)
{
}

void NodeParent::Update(int i)
{
}

#else

void Node::Update(int i)
{
	if ((i & 0xC) == 0xC) { // skipped ?
		Symbols->Reset(Config, this); // set to n
		Notify(NS_SKIP);
		if (Next) {
			Next->Update(i & ~2);
		}
		return;
	}
	if (Config) {
		if (wordlookup) { //mainly define nodes
			if (word > 3) {
				free((void*)word);
			}
			word = Symbols->Get(wordlookup);
			if (word > 3) {
				char *c = (char*)malloc(strlen((char*)word)+1);
				strcpy(c, (char*)word);
				word = (uintptr_t)c;
			}
		}
		uintptr_t w = word;
		if (w == 2 && !Symbols->HasModules()) {
			w = 3;
		}
		Symbols->Set(Config, w, this);
	}
	if (i & 8) {
		Notify(NS_UNSKIP);
	}
	Notify(NS_STATE);

	if (Next) {
		Next->Update(i & ~2);
	}
	if ((i & 2) && Parent) {
		Parent->Update(2);
	}
}

void NodeDep::Update(int i)
{
	if ((i & 0xC) == 0xC) {// skipped ?
		Symbols->Reset(Config, this); // set to n
		Notify(NS_SKIP);
		if (Next) {
			Next->Update(i & ~2);
		}
		return;
	}
	if (i & 8) {
		Notify(NS_UNSKIP);
	}
	if (CheckDep(3) == 1) {
		Symbols->Reset(Config, this);
		Notify(NS_DISABLE);
	} else {
		uintptr_t w = CheckDep(word);
		if (w == 2 && !Symbols->HasModules()) {
			w = 3;
		}
		Symbols->Set(Config, w, this);
		Notify(NS_ENABLE);
		Notify(NS_STATE);
	}

	if (Next) {
		Next->Update(i & ~2);
	}
	if ((i & 2) && Parent) {
		Parent->Update(2);
	}
}

void NodeParent::Update(int i)
{
	if ((i & 0xC) == 0xC) { // skipped ?
		//Symbols->Reset(Config, this); ??? // set to n
		Notify(NS_SKIP);
		if (Child) {
			Child->Update(i & ~2);
		}
		if (Next) {
			Next->Update(i & ~2);
		}
		return;
	}
	if (i & 8) {
		Notify(NS_UNSKIP);
	}
	if ((i & 1) && Child) {
		Child->Update(i & ~2);
	}
	if (Next) {
		Next->Update(i & ~2);
	}
	if ((i & 2) && Parent) {
		Parent->Update(2);
	}
}

void NodeIf::Update(int i)
{
	int f = If();

	if (i & 1) {// update down ?
		if (i & 8) {// skip/unskip
			if (i & 0x10) {// initializing ?
				if (i & 4) { // skipping ?
					Notify(NS_SKIP); //else Notify(unskipped);
					if (Child) {
						Child->Update(0x1D);
					}
					if (Else) {
						Else->Update(0x1D);
					}
				} else if (f) {
					if (Else) {
						Else->Update(0x1D);
					}
					if (Child) {
						Child->Update(0x19);
					}
				} else {
					if (Child) {
						Child->Update(0x1D);
					}
					if (Else) {
						Else->Update(0x19);
					}
				}
				prev_f = f;
			}else
			if (i & 4) { // skipping ?
				Notify(NS_SKIP);
				if (prev_f) {
					if (Child) {
						Child->Update(0xD);
					}
				} else {
					if (Else) {
						Else->Update(0xD);
					}
				}
			} else {
				Notify(NS_UNSKIP);
				if (f) {
					if (Child) {
						Child->Update(0x9);
					}
				} else {
					if (Else) {
						Else->Update(0x9);
					}
				}
				prev_f = f;
			}
			if (Next) {
				Next->Update(i & ~2);
			}
			return;
		}//*/

		// normal update
		if (f == prev_f) {
			if (f) {
				if (Child) {
					Child->Update(1);
				}
			} else {
				if (Else) {
					Else->Update(1);
				}
			}
		} else {
			if (f) {
				if (Else) {
					Else->Update(0xD);
				}
				if (Child) {
					Child->Update(0x9);
				}
			} else {
				if (Child) {
					Child->Update(0xD);
				}
				if (Else) {
					Else->Update(0x9);
				}
			}
		}
		prev_f = f;
	}

	if (Next) {
		Next->Update(i & ~2);
	}
	if ((i & 2) && Parent) {
		Parent->Update(2);
	}
}

void NodeChoice::Update(int i)
{
	int bFoundY=0;
	NodeChoice *n = (NodeChoice*)Parent->Child;
	NodeChoice *Def=n;

	if ((i & 0xC) == 0xC) { // skipped ?
		while (n) {
			Symbols->Reset(n->Config, n);
			n->Notify(NS_SKIP);
			n=(NodeChoice*)n->Next;
		}
		return;
	}

	if ((i & 3) != 3) { // called from the parent ? > i == 1 ?
		while (n) {
			if (n->DefaultChoice) {
				Def=n;
			}
			if (n->word == 3) { // y
				if (bFoundY) {
					n->word = 1; // n
					Symbols->Set(n->Config, 1, n);
					n->Notify(NS_STATE);
				}
				bFoundY = 1;
			}
			Symbols->Set(n->Config, n->word, n);
			if (i & 8) {
				n->Notify(NS_UNSKIP);
			}
			n=(NodeChoice*)n->Next;
		}
		if (!bFoundY) {
			Def->word = 3; // one must be -> y
			Symbols->Set(Def->Config, 3, Def);
			Def->Notify(NS_STATE);
		}
		return;
	}

	if ((i & 2) && Parent) {
		Parent->Update(2);
	}
}

#endif //NC24
// Update
//---------------------------------------------------------------------------------------===###
// Notify

void Node::Notify(int flags)
{
	switch(flags)
	{
	case NS_STATE:
	{
		if (type & NTT_STR) {
			break;
		}
		uintptr_t w = Get();
		if (prevword == w) {
			return;
		}
		if (prevword != (uintptr_t)~1) {
			Symbols->bModified = true; // ignore first update
		}
		prevword = w;
		if (state & (NS_SKIPPED | NS_DISABLED)) {
			return;
		} else {
			break;
		}
	}

	case NS_SKIP:		if (state & NS_SKIP) return;		state |= NS_SKIP; break;
	case NS_UNSKIP:		if (!(state & NS_SKIP)) return;		state &= ~NS_SKIP; break;

	case NS_DISABLE:	if (state & NS_DISABLE) return;		state |= NS_DISABLE;
				if (state & NS_SKIP) return;
				break;

	case NS_ENABLE:		if (!(state & NS_DISABLE)) return;	state &= ~NS_DISABLE;
				if (state & NS_SKIP) return;
				break;

	case NS_COLLAPSE:	if (state & NS_COLLAPSE) return;	state |= NS_COLLAPSE; break;
	case NS_EXPAND:		if (!(state & NS_COLLAPSE)) return;	state &= ~NS_COLLAPSE;break;

	case NS_PROMPT: break;
			
	case NS_SELECT: break;

	default: return; // disallow unknown states.
	}

	// send the notification to the user
	if (Symbols->Ntfy) {
		Symbols->Ntfy(this, flags, user);
	}
}

/* old version
void Node::Notify(int flags)
{
	if (flags == NS_PROMPT) goto end;
	if (flags == NS_STATE)
	{
		uintptr_t w = Get();
		if (prevword == w) return; else  prevword = w;
		if (state & (NS_SKIPPED | NS_DISABLED)) return; else goto end;
	}

	if ((flags & NS_ALL) & state) {// already in this state ?
		if (!(flags & NS_INVERT)) return; // set
	}else{
		if (flags & NS_INVERT) return; // not set
	}

	// update the internal state
	if (flags & NS_INVERT) state &= ~flags; else		state |= flags;
end:
	// send the notification to the user
	if (Symbols->Ntfy) Symbols->Ntfy(this, flags, user);
}//*/

// Notify
//---------------------------------------------------------------------------------------===###
// Dependencies
#ifdef NC24

uintptr_t NodeDep::CheckDep(uintptr_t w)
{
	uintptr_t s;

	if (!w || w > 3) {
		return w;
	}
	if (w == 2 && (type & NTT_BOOL)) {
		return 1; // can't set a bool to m
	}
	for (int i=0; i<DepCount; i++) {
		s = Symbols->Get(DepList[i]);
		if (!s) {
			return 1; // n
		}
		if (s > 3) {
			return 1; // n if string
		}
		if (s == 2) {
			if (type & NTT_MBOOL) {
				s = 3; // mbool treats m as y
			} else if (type & NTT_BOOL) {
				return 1;
			}
		}

		if (s < w) {
			w = s;
		}
	}
	return w;
}

bool NodeIf::If()
{
	uintptr_t op1, op2;
	bool b = 0, p = 0; // bool prev
	int pcond = 0; // prevcond
	for (int i = 0; i < Count; i++) {
		op1 = Symbols->Get(Cond[i].op1);
		op2 = Symbols->Get(Cond[i].op2);

		if (op1 > 3 && op2 > 3) {
			b = !strcmp((char*)op1, (char*)op2);
		} else {
			if (op1 > 3 || op2 > 3) {
				b=0;
			} else {
				b = (op1 == op2);
			}
		}
		if (!(Cond[i].cond & 1)) {
			b=!b; // cond == '!='
		}
		if (pcond & 2) {
			p = (b && p);
		} else {
			p = b; // <and> else <or>
		}
		if (!(Cond[i].cond & 2) && p) {
			return 1; // (1 <or> next) = true
		}
		pcond = Cond[i].cond;
	}
	return 0;
}
#endif //NC24
// Dependencies
//---------------------------------------------------------------------------------------===###
// AddChild

void NodeParent::AddChild(Node *n)
{
	if (Child) {
		Last->Next = n;
		Last = n;
	} else {
		Child = Last = n;
	}
	n->Parent = this;
	n->Symbols = Symbols;
}

#ifdef NC24
void NodeIf::AddChild(Node *n)
{
	if (!bElse) {
		if (Child) {
			Last->Next=n;
			Last = n;
		} else {
			Child = Last = n;
		}
	} else {
		if (Else) {
			Last->Next = n;
			Last = n;
		} else {
			Else = Last = n;
		}
	}
	n->Parent = this;
	n->Symbols = Symbols;
}
#endif //NC24

// AddChild
//---------------------------------------------------------------------------------------===###
// -----END OF Internal Functions-----
//------------------------------------------------------------------------------------------
// -----User Functions-----
//
// This is the API of this parser.
//
//------------------------------------------------------------------------------------------###
// Initialization

int NodeRoot::Init_CmdLine(int argc, char *argv[], bool bSpc)
{
	char *Arch = "i386";
	char *Path = "/usr/src/linux/";
	char *Config = 0;
	char cfg[255];

	if (argc < 2) {
		printf("Defaulting to: \"-P %s -A %s -C %s%s\"\n", Path, Arch, Path, ".config");
	}else{
		for (int i = 1; i < argc; i++) {
			if (*argv[i] != '-') {
				continue;
			}
			switch(*(argv[i]+1)) {
			case 'A': if (i+1 < argc) Arch = argv[++i]; break;
			case 'P': if (i+1 < argc) Path = argv[++i]; break;
			case 'C': if (i+1 < argc) Config = argv[++i]; break;
			case 'S':
			{
				for (const char *p = GetNextArch(Path); p; p = GetNextArch()) {
					printf("%s\n", p);
				}
				return 4;
			}
			case 'h':
				printf("Usage: -A arch -P path -C config -h help\n");
				printf("-S to show all available architectures for path.\n");
				printf("-C =config = path/config\n");
				return 4;
			}
		}

		char *s="/", *p=Path, *c=".config";
		if (Path[strlen(Path)-1] == '/') s="";
		if (Config) {
			if (*Config == '=') {
				snprintf(cfg, 254, "%s%s%s", p, s, Config+1);
				Config = cfg;
			}
			s = "";
			p = "";
			c = Config;
		}
		printf("Using: \"-P %s -A %s -C %s%s%s\"\n", Path, Arch, p, s, c);
	}
	return Init(Arch, Path, Config, bSpc);
}

#ifdef LKC26
void AddMenus(NodeParent *np, struct menu *mp);
#endif

static char InitPath[255];
int NodeRoot::Init(const char *Arch, const char *Path, const char *ConfigFile, bool bSpc)
{
	char fn[100];

	if (Child) {
		printf("Already initialized.\n");
		return 1;
	}

	// default architecture
	if (Arch == 0 || *Arch == 0) {
		Arch = "i386";
	}

// add the first few nodes manually
	// add the Symbols node 				-- XXX make 2.6 compatible
	NodeSymbols *ns = new NodeSymbols(Arch, this);
	Next = ns; Symbols = ns; // Important Symbols is always Next to Root
	ns->Symbols = ns; // !!!!
	ns->bPromptSpaces = bSpc; // XXX REMOVE THIS line
	if (!ns->SetPath(Path)) {
		printf("The specified directory doesn't contain the kernel sources.\n");
		return 1;
	}

	// add the dependency list node XXX
	DepList = new NodeDListP(this);
	Symbols->Next = DepList;
	DepList->Symbols = ns; // <--- n->AddChild(DepList);

	// add the first source node manually
	NodeSource *n = new NodeSource();
	AddChild(n);

	// hack - use 'word' for NodeRoot help
	char *rh = (char*)malloc(256);
	snprintf(rh, 254, "<file:%s>", Path);
	word = (uintptr_t)rh;
	// ~Node will free it

// the tree now look like this:
// * NodeRoot
// ** source --> continue adding nodes below this one
// * Symbols - special purpose
// * deplist - special purpose
// ---------------------------

	CfgFile tmpf(this);
	
	InitPath[0]=0;
	readlink("/proc/self/exe", InitPath, 254);

#ifdef LKC26
// use this until I find a way to cleanup LKC
	static bool bInitLkc26 = false;
	if (bInitLkc26) {
		 char buf[255];
		 snprintf(buf, 254, "%s -P %s -A %s", InitPath, Path, Arch);
		 system(buf);

		 printf("Cannot reinit lkc26 at this time.");
		 return 4;
	} else {
		 //bInitLkc26 = true;
	}

// open the architecture specific Kconfig and parse it				--- 2.6 ---
	strcpy(fn, "arch/");
	strcat(fn, Arch);
	strcat(fn, "/Kconfig");
	if (tmpf.TestOpen(fn)) { //--> do the 2.6 kernel and RETURN
		bInitLkc26 = true;

		// set the root source file
		n->SetPrompt(fn);

		// change PWD to PATH here! or patch LKC to ----conf_set_path(const char *)----
		chdir(Path);

		// setup env
		setenv("ARCH", Arch, 1);
		//setenv("KERNELRELEASE", 2.6.7xxx); ???

		// init it all
		conf_parse(fn); // will exit() on error ---- should have a bool version.
		conf_read(ConfigFile);
		bLkc26 = true;

		// init the node tree here
		if (rootmenu.prompt) {
			SetPrompt(rootmenu.prompt->text);
			//pmenu = &rootmenu;
			line = rootmenu.lineno;
		}
		AddMenus(this, rootmenu.list);
		Update(1);
		Symbols->bModified = false;

		return 0;
	} else {
#ifndef NC24
		strcpy(fn, "arch/");
		strcat(fn, Arch);
		strcat(fn, "/config.in");
		if (tmpf.TestOpen(fn)) {
			printf("Found only %s and 2.4.x support disabled, failing.\n", fn);
			return 1;
		}
		printf("Can't find %sarch/%s/Kconfig, failing.\n", GetPath(), Arch);
		return 2;
#endif
	}
#endif //LKC26

#ifdef NC24
// open the architecture specific config.in and parse it		--- 2.4 ---
	strcpy(fn, "arch/");
	strcat(fn, Arch);
	strcat(fn, "/config.in");
	if (!tmpf.TestOpen(fn)) {
#ifndef LKC26
		strcpy(fn, "arch/");
		strcat(fn, Arch);
		strcat(fn, "/Kconfig");
		if (tmpf.TestOpen(fn)) {
			printf("Found only %s and 2.6.x support disabled, failing\n", fn);
			return 1;
		}
#endif
		printf("Can't find %sarch/%s/Config.in, failing.\n", GetPath(), Arch);
		return 2;
	}

	if (!((Node*)n)->Parse(fn, 0)) {
		printf("Parsing failed.\n");
		return 3;
	}
	bLkc26 = false;

	int ret = 0;
	// load the configuration settings
	if (!Load(ConfigFile)) {
		ret = 8;
	}
	Update(1);
	Symbols->bModified = false;

	return ret;
#endif //NC24

//  1 - invalid directory
//  2 - invalid architecture
//  3 - parsing failed
//  4 - commandline: -h -S or reinit failed
//  8 - loading ConfigFile failed - may be ignored
//  eg: if (root->Init() & 7) return FAIL;
//  eg: if (root->Init_CmdLine() & 7) return FAIL;
}

// Initialization
//------------------------------------------------------------------------------------------###
// Loading & Saving -> Line:1220
//------------------------------------------------------------------------------------------
// Enumerate

bool Node::Enumerate(enumNodes en, int flags, void *pv)
{
	return 0;
//	if (Next) if (!Next->_Enumerate(en, flags, pv)) return 0;
//	return 1;
}

bool Node::_Enumerate(enumNodes en, int flags, void *pv)
{
	if (!en(this, state, pv)) {
		return 0;
	}
	if (Next && !Next->_Enumerate(en, flags, pv)) {
		return 0;
	}
	return 1;
}

#ifdef NC24
bool NodeDep::_Enumerate(enumNodes en, int flags, void *pv)
{
	if (!(state & ~flags & NS_DISABLED)) {
		if (!en(this, state, pv)) {
			return 0;
		}
	}
	if (Next && !Next->_Enumerate(en, flags, pv)) {
		return 0;
	}
	return 1;
}
#endif

// only used when a Parent is called directly
bool NodeParent::Enumerate(enumNodes en, int flags, void *pv)
{
	if (!(state & ~flags & NS_COLLAPSED)) {
		if (Child && !Child->_Enumerate(en, flags, pv)) {
			return 0;
		}
	}
	return 1;
}

bool NodeParent::_Enumerate(enumNodes en, int flags, void *pv)
{
	if (!en(this, state, pv)) {
		return 0;
	}
	if (!(state & ~flags & NS_COLLAPSED) && (type == NT_SOURCE || (flags & NS_DOWN))) {
		if (Child && !Child->_Enumerate(en, flags, pv)) {
			return 0;
		}
	}
	if (flags & NS_EXIT) {
		if (!en(this, state | NS_EXIT, pv)) {
			return 0;
		}
	}
	if (Next && !Next->_Enumerate(en, flags, pv)) {
		return 0;
	}
	return 1;
}

bool NodeRoot::_Enumerate(enumNodes en, int flags, void *pv)
{
	if (!en(this, state, pv)) {
		return 0;
	}
	if (Child && !Child->_Enumerate(en, flags, pv)) {
		return 0;
	}
	if (flags & NS_EXIT) {
		if (!en(this, state | NS_EXIT, pv)) {
			return 0;
		}
	}
	return 1;
}

#ifdef NC24
bool NodeIf::Enumerate(enumNodes en, int flags, void *pv)
{
	if (flags & NS_SKIPPED) {
		if (Child && !Child->_Enumerate(en, flags, pv)) {
			return 0;
		}
		if (Else && !Else->_Enumerate(en, flags, pv)) {
			return 0;
		}
	} else {
		if (If()) {
			if (Child && !Child->_Enumerate(en, flags, pv)) {
				return 0;
			}
		} else {
			if (Else && !Else->_Enumerate(en, flags, pv)) {
				return 0;
			}
		}
	}
	return 1;
}

bool NodeIf::_Enumerate(enumNodes en, int flags, void *pv)
{
	if (!en(this, state, pv)) {
		return 0;
	}
	if (flags & NS_SKIPPED) {
		if (Child && !Child->_Enumerate(en, flags, pv)) {
			return 0;
		}
		if (Else  && !Else->_Enumerate(en, flags, pv)) {
			return 0;
		}
	} else {
		if (If()) {
			if (Child && !Child->_Enumerate(en, flags, pv)) {
				return 0;
			}
		} else {
			if (Else  && !Else->_Enumerate(en, flags, pv)) {
				return 0;
			}
		}
	}
	if (flags & NS_EXIT) {
		if (!en(this, state | NS_EXIT, pv)) {
			return 0;
		}
	}
	if (Next && !Next->_Enumerate(en, flags, pv)) {
		return 0;
	}
	return 1;
}
#endif //NC24

// Enumerate
//------------------------------------------------------------------------------------------###
// Search

// i = 1 = down
// i = 2 = up
// i = 4 = else
// if type == IF and not a -child- then is MUST be -else-

Node *Node::Search(SearchNodes sn, void *pv, int i)
{
	Node *n;
	if (Next) {
		if (sn(Next, pv)) {
			return Next;
		}
		n = Next->Search(sn, pv, 1);
		if (n) {
			return n;
		}
	}
	if ((i & 2) && Parent) {
		return Parent->Search(sn, pv, (Parent->GetType() ==
				NT_IF && Parent->IsMyChild(this)) ? 6:2);
	}
	return 0;
}

Node *NodeParent::Search(SearchNodes sn, void *pv, int i)
{
	Node *n;
	if ((i & 1) && Child) {
		if (sn(Child, pv)) {
			return Child;
		}
		n = Child->Search(sn, pv, 1);
		if (n) {
			return n;
		}
	}
	if (Next) {
		if (sn(Next, pv)) {
			return Next;
		}
		n = Next->Search(sn, pv, 1);
		if (n) {
			return n;
		}
	}
	if ((i & 2) && Parent) {
		return Parent->Search(sn, pv, (Parent->GetType() ==
				NT_IF && Parent->IsMyChild(this)) ? 6:2);
	}
	return 0;
}

Node *NodeRoot::Search(SearchNodes sn, void *pv, int i)
{
	Node *n;
	if ((i & 1) && Child) {
		if (sn(Child, pv)) {
			return Child;
		}
		n = Child->Search(sn, pv, 1);
		if (n) {
			return n;
		}
	}
	return 0;
}

#ifdef NC24
Node *NodeIf::Search(SearchNodes sn, void *pv, int i)
{
	Node *n;
	if ((i & 1) && Child) {
		if (sn(Child, pv)) {
			return Child;
		}
		n = Child->Search(sn, pv, 1);
		if (n) {
			return n;
		}
	}
	if ((i & 5) && Else) {
		if (sn(Else, pv)) {
			return Else;
		}
		n = Else->Search(sn, pv, 1);
		if (n) {
			return n;
		}
	}
	if (Next) {
		if (sn(Next, pv)) {
			return Next;
		}
		n = Next->Search(sn, pv, 1);
		if (n) {
			return n;
		}
	}
	if ((i & 2) && Parent) {
		return Parent->Search(sn, pv, (Parent->GetType() == 
				NT_IF && Parent->IsMyChild(this)) ? 6:2);
	}
	return 0;
}
#endif //NC24

// Search
//------------------------------------------------------------------------------------------###
// Advance, Get & Set

const char *Node::GetStr()
{
	if (state & (NS_SKIPPED | NS_DISABLED)) {
		if (type & NTT_STR) {
			return "";
		} else {
			return "n";
		}
	}
	if (type & NTT_STR) {
		if (word > 3) {
			return (char*)word;
		} else {
			return "";
		}
	} else if (type & NTT_NMY) {
		switch(word) {
		case 1: return "n";
		case 2: return "m";
		case 3: return "y";
		default: return "";
		}
	} else {
		return "";
	}
}

uintptr_t Node::Set(const char *s, int updt)
{
	if (type & NTT_STR) {
		 return Set((uintptr_t)s, updt);
	} else if (type & NTT_NMY) {
		if (!*(s+1)) switch(*s)	{
		case 'n': case 'N': Set(1, updt);
		case 'm': case 'M': Set(2, updt);
		case 'y': case 'Y': Set(3, updt);
		default: return 0;
		}
	}
	return 0;
}

uintptr_t Node::Advance(int updt)
{
	if (!(type & NTT_INPUT)) {
		return 0;
	}
	if (!(type & NTT_NMY)) {
		return 0; // only nmy can be advanced.
	}
	if (state & NS_SKIPPED) {
		return 0;
	}
	if (word == 2 && !Symbols->HasModules()) {
		word = 3;
	}
	if (++word > 3) {
		word = 1;
	}
	if (word == 2) {
		if (!Symbols->HasModules()) {
			word = 3;
		}
		if (type & NTT_BOOL) {
			word = 3;
		}
	}
	Symbols->Set(Config, word, this);
	if (updt) IUpdate(); else Notify(NS_STATE);
	return word;
}

uintptr_t Node::Get()
{
	if (state & (NS_SKIPPED | NS_DISABLED)) {
		if (type & NTT_STR) {
			return (uintptr_t)"";
		} else {
			return 1;
		}
	} else {
		if (word == 2 && !Symbols->HasModules()) {
			return 3;
		} else {
			return word;
		}
	}

//	uintptr_t w = Symbols->Get(Config);
//	if ((type & NTT_STR) && w<4 && w) { printf("invalid string value %i from: %s\n", w, prompt); w=0; }
//	return w;
}

uintptr_t Node::Set(uintptr_t w, int updt)
{
	// only input nodes is allowed to use Set
	if (!(type & NTT_INPUT)) {
		return 0;
	}
	if (state & NS_SKIPPED) {
		return 0;
	}
//	if (w == word) return w; // already set to w
	if (!w) {
		printf("Node::Set Error: Cannot unset a node.\n");
		return 0;
	}
	if (type & NTT_NMY) { // tristate / bool
		if (w > 3) {
			w = 1; // string ? set to n
		}
		if (w == 2) { // m
			if (type & NTT_BOOL) {
				w = 3;
			}
			if (!Symbols->HasModules()) {
				w = 3;
			}
		}
		word = w;
		Symbols->Set(Config, w, this);
	} else if (type & NTT_STR) { // hex int str
		if (w < 4) {
			return word;
		}
		char *c = (char*)malloc(strlen((char*)w)+2);
		strcpy(c, (char*)w);

		// validate numbers
		if (type & NTT_HEX) {
			char *s = c;
			// remove the "0x"
			if (s[0] == '0' && s[1] == 'x') {
				while (s[2]) {
					*s = s[2];
					s++;
				};
				*s = 0;
				s = c;
			}
			if (!*c) {
				*s++ = '0';
				*s = 0;
			}
			while (*s) {
				if (!((*s >= '0' && *s <= '9') || ((*s >= 'a' && *s <= 'f') && (*s &= 0xDF)) || (*s >= 'A' && *s <= 'F'))) {
					*s = 0;
					break;
				}
				s++;
			}
			if (!*c) {
				*c = '0';
				c[1] = 0;
			} // if empty
		}else
		if (type & NTT_INT) {
			char *s = c;
			if (!*c) {
				*s++ = '0';
				*s = 0;
			}
			while (*s) {
				if (!(*s >= '0' && *s <= '9')) {
					*s = 0;
					break;
				}
				s++;
			}
			if (!*c) {
				*c = '0';
				c[1] = 0;
			} // if empty
		}

		// same as previous ?
		if (word < 4 || !strcmp(c, (char*)word)) {
			prevword = (uintptr_t)c;
		}
		if (word > 4) {
			free((void*)word);
		}
		word = (uintptr_t)c;
		Symbols->Set(Config, word, this);
	}
	if (updt) {
		IUpdate();
	} else {
		Notify(NS_STATE);
	}
	return w;
}

#ifdef NC24
uintptr_t NodeDep::Advance(int updt)
{
	if (!(type & NTT_NMY)) {
		return 0; // only nmy can be advanced.
	}
	if (state & (NS_SKIPPED | NS_DISABLED)) {
		return 0;
	}
	if (word == 2 && !Symbols->HasModules()) {
		word = 3;
	}
	if (++word > 3) {
		word = 1;
	}
	if (word == 2) {
		if (!Symbols->HasModules()) {
			word = 3;
		}
		if (type & NTT_BOOL) {
			word = 3;
		}
	}
	if (word != CheckDep(word)) {
		word = 1;
	}
	Symbols->Set(Config, word, this);
	if (updt) {
		IUpdate();
	} else {
		Notify(NS_STATE);
	}
	return word;
}

uintptr_t NodeDep::Get()
{
	if (state & (NS_SKIPPED | NS_DISABLED)) {
		return 1;
	} else {
		uintptr_t w = CheckDep(word);
		if (w == 2 && !Symbols->HasModules()) {
			return 3;
		} else {
			return w;
		}
	}

/*	// CheckDep should not be needed here... just making sure...
	uintptr_t p, w; p = w = Symbols->Get(Config);
	w = CheckDep(w);
	if (w!=p) printf("NodeDep::Get: Inconsistency value changed from: %i to %i\n", p, w);
	if (word!=p) printf("NodeDep::Get: Inconsistency2 value get: %i word %i\n", p, word);
	return w;//*/
}

uintptr_t NodeDep::Set(uintptr_t w, int updt)
{
	// only input nodes is allowed to use Set
//	if (!(type & NTT_INPUT)) return 0; // will this ever happen ???
	if (state & (NS_SKIPPED | NS_DISABLED)) {
		return 0;
	}
	if (w == word) {
		return w; // already set to w
	}
	if (!w) {
		printf("NodeDep::Set Error: Cannot unset a node.\n");
		return 0;
	}
	if (type & NTT_NMY) { // tristate / bool
		if (w > 3) {
			w = 1;
		}
		if (w == 2) { // m
			if (type & NTT_BOOL) {
				w = 3;
			}
			if (!Symbols->HasModules()) {
				w = 3;
			}
		}
		w = CheckDep(w); // check deps
		word = w;
		Symbols->Set(Config, w, this);
	}else if (type & NTT_STR) { // hex int str
		printf("NodeDep::Set Error: Dep nodes use only NMY.\n");
		return 0;
	}
	if (updt) {
		IUpdate();
	} else {
		Notify(NS_STATE);
	}
	return w;
}

uintptr_t NodeChoice::Advance(int updt)
{
	if (state & NS_SKIPPED) {
		return 0;
	}
	if (word == 3) {
		return 3; // is it already y ?
	}
	return Set(3, updt);
}

uintptr_t NodeChoice::Set(uintptr_t w, int updt)
{
	// only input nodes is allowed to use Set
//	if (!(type & NTT_INPUT)) return 0; // will this ever happen ???
	if (state & NS_SKIPPED) {
		return 0;
	}
	if (!w) {
		printf("NodeChoice::Set Error: Cannot unset a node.\n");
		return 0;
	}
	if (w != 3) {
		return 0;
	}
	// w == 3 here always...

	NodeChoice *n = (NodeChoice*)Parent->Child;
	while (n) // set all other choices to n
	{
		if (n == this) {
			n = (NodeChoice*)n->Next;
			continue;
		} // skip ourself
		if (n->word > 1) { // m y
			n->word = 1; // n
			Symbols->Set(n->GetConfig(), 1, n);
			n->Notify(NS_STATE);
		}
		n = (NodeChoice*)n->Next;
	}
	if (word == 3) {
		return 3; // already set to 3
	}
	word = 3;
	Symbols->Set(Config, 3, this);

	Notify(NS_STATE);
	if (updt) {
		IUpdate();
	}
	return w;
}
#endif //NC24

uintptr_t NodeParent::Advance(int updt)
{
	Notify(state & NS_COLLAPSED ? NS_EXPAND : NS_COLLAPSE);
	return 1;
}

// Advance, Get & Set
//------------------------------------------------------------------------------------------###
// Misc Get/Set Functions

bool NodeParent::IsMyChild(Node *c)
{
	Node *n = Child;
	while (n) {
		if (c == n) {
			return 1;
		}
		n = n->Next;
	}
	return 0;
}

NodeParent *Node::GetParent(unsigned int tp, unsigned int flags)
{
	NodeParent *pp, *p = Parent;
	pp = (NodeParent*)this; // if !p pp == NodeRoot ALWAYS
	if (tp == 0 && flags == (unsigned int)~0) {
		flags = 0; // find the first parent
	}
	while (p) {
		if (tp == p->type) {
			break;
		}
		if ((p->type & flags) == flags) {
			break;
		}
		pp = p;
		p = p->Parent;
	}
	if (!p && tp==NT_SOURCE) {
		return (NodeParent*)pp->GetChild(); // NodeRoot==pp
	}
	return p;
}

char *Node::GetHelp()
{
	return Symbols->GetHelp(Config);
}

char *Node::GetSymbol()
{
	return Symbols->GetSymbol(Config);
}

char *Node::GetPrompt()
{
	if (prompt) {
		return prompt;
	} else {
		return "---";
	}
}

int Node::GetDeps()
{
	return Symbols->GetDeps(Config);
}

char *Node::GetSource()
{
	Node *n = GetParent(NT_SOURCE);
	if (n) {
		return n->GetPrompt();
	} else {
		return "No Source"; // shouldn't ever happen
	}
	return 0;
}

#ifdef NC24
char *NodeIf::GetPrompt()
{
	return "if";
}

char *NodeChoiceP::GetHelp()
{
	if (Child) {
		return Symbols->GetHelp(Child->GetConfig());
	} else {
		return Symbols->GetHelp(Config); // shouldn't happen
	}
}

char *NodeChoiceP::GetSymbol()
{
	if (Child) {
		return Symbols->GetSymbol(Child->GetConfig());
	} else {
		return Symbols->GetSymbol(Config); // shouldn't happen either.
	}
}

int NodeChoiceP::GetConfig()
{
	if (Child) {
		return Child->GetConfig();
	} else {
		return Config;
	}
}
#endif //NC24

//-----------------

const char *NodeRoot::GetFirstArch()
{
	return GetNextArch(GetPath());
}

const char *NodeRoot::GetNextArch(char *path)
{
	static int max, n;
	static struct dirent **nl = 0;

	// new search ? - free old first
	if (path && nl) {
		while (n < max) {
			free(nl[n++]);
		}
		free(nl);
		nl = 0;
		max = 0;
	}

	if (nl) {
next:		free(nl[n]);
		if (++n >= max) {
			free(nl);
			nl = 0;
			max = 0;
			return 0;
		}
		if (nl[n]->d_name[0] == '.') {
			goto next;
		}
		return nl[n]->d_name;
	}

	if (!path) {
		printf("null path!!!!\n");
		return 0;
	}

	// start a new search
	char buf[255];
	strcpy(buf, path);
	if (buf[strlen(buf)-1] !='/') strcat(buf, "/");
	strcat(buf, "arch");

	n = 0;
	max = scandir(buf, &nl, 0, alphasort);
	if (max < 0) {
		 return 0;
	} else if (!max) {
		free(nl); // can this happen ????
		return 0;
	} else {
		if (nl[n]->d_name[0] == '.') {
			goto next;
		}
		return nl[n]->d_name;
	}
	return 0;
}

// Misc Get/Set Functions
//------------------------------------------------------------------------------------------###
// Help & text functions

void strcutr(char *s, char c)
{
	for (char *e = s + strlen(s); e >= s; e--) {
		if (*e == c) {
			*e = '\0';
			break;
		}
	}
}

char *NodeRoot::GetHelpH(Node *n)
{
	FreeH();
	int max = 255; // should be plenty for the prompt, symbol and filename
	char *help = n->GetHelp();
	if (!help) {
		help = "No help available.";
	}
	max += strlen(help);
	max += (strlen(InitPath) + 30) * 2;

	char *m = (char*)malloc(max+1);
	if (!m) {
		return 0;
	}
	snprintf(m, max, "%s\n%s = %s\n<file:%s%s>:%i\n\n%s\n",
			n->GetPrompt(), n->GetSymbol(), n->GetStr(), GetPath(), n->GetSource(), n->GetLine(), help);

	// check for a README file
	CfgFile f(this);
	char buf[256];

	strcpy(buf, InitPath);
	strcutr(buf, '/');
	strcutr(buf, '/');
	strcat(buf, "/README");
	
	if (f.TestOpen(buf)) {
		sprintf(m+strlen(m), "\n<file:%s>\n", buf);
	} else {
		strcat(m+strlen(m), "\nCouldn't find the README.\n");
	}
	pHelpMem = m;
	return m;
}

char *NodeRoot::GetFileH(const char *fn)
{
	FreeH();
	int f = open(fn, O_RDONLY);
	if (f == -1) {
		return GetDirH(fn); // maybe it is a dir ?
	}

	struct stat st;
	if (fstat(f, &st) == -1) {
		return 0;
	}
	if (S_ISDIR(st.st_mode)) {
		close(f);
		return GetDirH(fn); // maybe it is a dir ?
	}

	char *p = (char*)malloc(st.st_size+20);
	if (!p) {
		close(f);
		return 0;
	}
	p[st.st_size]=0;
	p[st.st_size+1]=0;

	if (read(f, p, st.st_size) != st.st_size) {
		free(p);
		close(f);
		return 0;
	}
	close(f);

	pHelpMem = p;
	return p;
}

char *NodeRoot::GetDirH(const char *dn)
{
	FreeH();
	char *p=0, *t=0;
	char buf[250];
	strcpy(buf, dn);
	int l = strlen(buf) - 1;
	if (buf[l] == '/') buf[l] = 0;

	t = p = (char*)malloc(8192);
	if (!p) {
		return 0;
	}
dirit:
	DIR *d = opendir(buf);
	if (!d) {
		l = strlen(buf) - 1;
		while (l) {
			if (buf[l] == '/') {
				buf[l] = 0;
				goto dirit;
			}
			l--;
		}
		free(p);
		return 0;
	}

	t = p;

	struct dirent *de;
	while ((de = readdir(d))) {
		if (de->d_name[0] == '.') {
			continue;
		}
		snprintf(t, 580, "<file:%s/%s> %i\n", buf, de->d_name, de->d_type);
		t += strlen(t);
		if (t-p > 7600) { //XXX TODO check this again 2024
			snprintf(t, 100, "buffer overflow. (almost)\n");
			break;
		}
	}
	closedir(d);
	pHelpMem = p;
	return p;
}

char *NodeRoot::GetLinkFileH(const char *line, int x, char **start, char **end, int *ll, char **fn)
{
	char *d1, *d2; // temp stores if start/end == 0
	if (!end) {
		end = &d2;
	}
	if (!start) {
		start = &d1;
	}
	char *p = GetLink(line, x, start, end);
	if (!p) {
		return 0;
	}
	if (fn) {
		*fn = p;
	}

/*	//Debug code
	const char *px;
	char buf[1000], *d=buf;
	for (px=*start; px <= *end && *px != '\n' && *px; ) *d++ = *px++; *d=0; 
	fprintf(stderr, "-->%s\n", buf);
	if (p)
		fprintf(stderr, "==>%s\n", p);*/

	// get line, if there is one
	int l = 0;
	char *nn = *end;
	if (nn && *nn == ':') {
		for (int j = 1; j < 7; j++) { // 6 digits
			if (nn[j] >= '0' && nn[j] <= '9') {
				l *= 10;
				l += nn[j] - '0';
			} else {
				break;
			}
		}
	}
	if (ll) {
		*ll = l;
	}
	return GetFileH(p);
}

void NodeRoot::FreeH()
{
	if (!bAutoFreeH) {
		return;
	}
	if (pHelpMem) {
		free(pHelpMem);
	}
	pHelpMem = 0;
}

void NodeRoot::FreeH(char *f)
{
	if ((!bAutoFreeH) && f) {
		free(f);
	}
}

void NodeRoot::SetAutoFreeH(bool b)
{
	if (!b) {
		FreeH();
	} else {
		pHelpMem = 0;
	}
	bAutoFreeH = b;
}

char *NodeRoot::GetLink(const char *line, int x, char **start, char **end)
{
	const int Links = 4;
	char *LinkStr[Links][2] =
		{{"<file:", ">"},
		{"Documentation/", ".txt"},
		{"source", "Kconfig"},
		{"source", "Config.in"}};
	int LinkLen[Links][4] = {{6, 6, 1, 0}, {14, 0, 4, 4}, {6, 6, 7, 7}, {6, 6, 9, 9}}; //start, end, include_end

	char *d1, *d2; // temp stores if start/end == 0
	if (!end) {
		end = &d2;
	}
	if (!start) {
		start = &d1;
	}
	for (char *d = (char*)line; ; d++) {
		if (*d == '\n' || !*d) {
			return 0;
		}
		for (int i=0; i<Links; i++) {
			if (!strncmp(d, LinkStr[i][0], LinkLen[i][0])) {
				*start = d;
				for (char *c = d; ;c++)
//=============================================================================
{
	if (*c == '\n' || !*c) {
		break;
	}
	if (!strncmp(c, LinkStr[i][1], LinkLen[i][2])) { // found
		*end = c + LinkLen[i][2];
		char *stop = c + LinkLen[i][3];

		// check for quotes
		char *dt = *start + LinkLen[i][0];
		for (; *dt == ' ' || *dt == '\t'; dt++); // trim start
		if (*dt == '\"' || *dt == '\'') {
			for (dt++; *dt != '\n' && *dt; dt++) // seek "
			if (*dt == '\"' || *dt == '\'') {
				*end = dt+1;
				stop = dt;
				break;
			}
		}

		// check for Kconfig.hz, without quotes the .hz isn't seen
		if (i == 2) {
			char *dt = stop;
			if (*dt == '.') {
				for (dt++; *dt != '\n' && *dt != ' ' && *dt != '\t' && *dt; dt++); // seek end
				*end = stop = dt;
			}
		}

		if (x == -1) { // return link
			return "OK";
		} else if ((line+x) >= *start && (line + x) < *end) { //return filename
			char buf[255];
			char *dt = buf;
			char *st = *start + LinkLen[i][1];

			for (;*st != '\n' && *st && st < stop; *dt++ = *st++);
			*dt = 0; // copy it
			for (dt--; *dt == ' ' || *dt == '\t' || *dt == '\"'; *dt-- = 0); // trim end
			for (dt = buf; *dt == ' ' || *dt == '\t' || *dt == '\"'; dt++); // trim start

			// is path specified ?
			static char buf2[255]; buf2[0]=0;
			if (*dt != '/') strcpy(buf2, GetPath());
			strcat(buf2, dt);
			return buf2;
		} else break;
	} // if (strncmp)
} // for (char *c;;)
//=============================================================================
			} // if (strncmp)
		} // for (int i;;) links
	} // for (char *d;;)
	return 0;
}

// Help & text functions
//------------------------------------------------------------------------------------------###
// -----END OF User Functions-----
//------------------------------------------------------------------------------------------
// Dependency listing

NodeDListP::NodeDListP(NodeRoot *n)
{
#ifdef NC24
	Root = n;
#endif
	type = NT_DLIST;
	SetPrompt(n->GetPrompt());
}

Node *NodeRoot::GetDepTree()
{
	DepList->Clean();
	DepList->SetPrompt(prompt);
	return DepList;
}

#ifndef NC24
Node *Node::GetDepTree()
{
	return 0;
}

void Node::GetDepTree(NodeDListP *d)
{
}

#else
NodeDList::NodeDList(char *s, int c)
{
	type = NT_COMMENT;
	Config = c;
	NLink = 0;

	prompt = (char*)malloc(strlen(s)+1);
	if (!prompt) {
		return;
	}
	strcpy(prompt, s);
}

NodeDList::NodeDList(Node *n)
{
	type = (n->GetType() | NTT_VISIBLE) & ~NTT_PARENT;
	prompt = n->GetPrompt();
	Config = n->GetConfig();
	line = n->GetLine();
	NLink = n;

	char *p = (char*)malloc(strlen(prompt)+1);
	if (!p) {
		prompt = 0;
		return;
	}
	strcpy(p, prompt);
	prompt = p;
}

void NodeDList::Update(int i)
{
	int s = GetState();

	if (!(s & NS_SKIP)) {
		Notify(NS_UNSKIP);
	}
	if (!(s & NS_DISABLE)) {
		Notify(NS_ENABLE);
	}
	Notify(NS_STATE);
	if ((s & NS_DISABLE)) {
		Notify(NS_DISABLE);
	}
	if ((s & NS_SKIP)) {
		Notify(NS_SKIP);
	}
	state = s;
	if (Next) {
		Next->Update();
	}
}

bool NodeDList::_Enumerate(enumNodes en, int flags, void *pv)
{
	if (!en(this, GetState(), pv)) {
		return 0;
	}
	if (Next && !Next->_Enumerate(en, flags, pv)) {
		return 0;
	}
	return 1;
}

void NodeDListP::AddChild(NodeDList *n)
{
	if (Child)
	{
		NodeDList *p = (NodeDList*)Child;
		Node *l = ((NodeDList*)n)->NLink;
		if (l)
		{
			while (p)
			{
				if (p->NLink == l) { delete n; return; }
				p = (NodeDList*)p->Next;
			}
		}else
		{
			int c = n->GetConfig();
			while (p)
			{
				if (p->GetConfig() == c) { delete n; return; }
				p = (NodeDList*)p->Next;
			}
		}

		Last->Next=n;
		Last = n;
	}else{
		Child = Last = n;
	}
	n->Parent = this;
	n->Symbols = Symbols;
}

//-----------------------------------------------------------------------------

struct BDep
{
	int Config;
	NodeDListP *d;
};

bool BuildDeps(Node *n, int flags, void *pv)
{
	BDep *d = (BDep*)pv;
	if (n->GetConfig() == d->Config) {
		n->GetDepTree(d->d);
	}
	return 1;
}

void NodeDListP::CheckOutConfig(int c)
{
	if (c < 5) {
		return; // skip nmy+ARCH
	}
	if (!Symbols->IsDefined(c)) {
		char buf[80];
		snprintf(buf, 80, "Not Def: %s", Symbols->GetSymbol(c));
		NodeDList *l = new NodeDList(buf, c); AddChild(l);
		return;
	}

	// already done ?
	NodeDList *n = (NodeDList*)Child;
	while (n) {
		if (n->GetConfig() == c) {
			return;
		}
		n = (NodeDList*)n->Next;
	}
	BDep bd;
	bd.d = this;
	bd.Config = c;
	Root->Enumerate(BuildDeps, NS_ALL, &bd);
}

Node *Node::GetDepTree()
{
	NodeDListP *p;
	if (type == NT_ROOT) {
		p = ((NodeRoot*)this)->DepList;
	} else {
		p = ((NodeRoot*)GetParent(NT_ROOT))->DepList;
	}
	p->Clean();
	GetDepTree(p); // add self
	if (Config > 4 && type != NT_ROOT) {
		BDep bd;
		bd.d=p;
		bd.Config = Config;
		GetParent(NT_ROOT)->Enumerate(BuildDeps, NS_ALL, &bd);
	}
	return p;
}

void Node::GetDepTree(NodeDListP *d)
{
	if (!(type & NTT_DEF) || !Symbols->IsRedefined(Config)) {
		NodeDList *l = new NodeDList(this);
		d->AddChild(l);
	}
	Node *n = GetParent(NT_IF);
	if (n) {
		n->GetDepTree(d); //crash here
	}
}

void NodeDep::GetDepTree(NodeDListP *d)
{
	NodeDList *l = new NodeDList(this);
	d->AddChild(l);

	for (int i=0; i<DepCount; i++) {
		d->CheckOutConfig(DepList[i]);
	}
	Node *n = GetParent(NT_IF);
	if (n) {
		n->GetDepTree(d);
	}
}

void NodeIf::GetDepTree(NodeDListP *d)
{
	for (int i=0; i<Count; i++) {
		d->CheckOutConfig(Cond[i].op1);
		d->CheckOutConfig(Cond[i].op2);
	}
	Node *n = GetParent(NT_IF);
	if (n) {
		n->GetDepTree(d);
	}
}
#endif //NC24

// Dependency listing
//------------------------------------------------------------------------------------------###
// -----Lkc 2.6.x support wrapper-----
//------------------------------------------------------------------------------------------
// 2.6.x Dependency listing
#ifdef LKC26

struct LkcDep
{
	struct symbol *sym;
	NodeParent *p;
	Node *ne;//temp
	bool bFound;
};

int symindex = 0;
struct symbol *symlist[250];

bool AddSym(struct symbol *s)
{
	for (int i=0; i<symindex; i++) {
		if (symlist[i] == s) {
			return false;
		}
	}
	if (symindex > 248) {
		printf("symlist overflow\n");
		return false;
	}
	symlist[symindex++] = s;
	return true;
}

bool CheckSym(struct symbol *s)
{
	for (int i=0; i<symindex; i++) {
		if (symlist[i] == s) {
			return false;
		}
	}
	return true;
}

void ClearSym()
{
	symindex = 0;
}

int GetSyms(struct expr *dep, LkcDep *ld);
void NodeLkc26::SearchExpr(LkcDep *ld)
{
	type |= NTT_PARENT;
	if (psym) {
		LkcDep ldd;
		if (psym->prop) {
			ldd = *ld;
			ldd.sym = psym;
			ldd.p = this;
			GetSyms(psym->prop->visible.expr, &ldd);
		}

		if (psym->rev_dep.expr) {
			ldd = *ld;
			ldd.sym = psym;
			ldd.p = this;
			GetSyms(psym->rev_dep.expr, &ldd);
		}

		// depcheck defaults for defines
		if (!pmenu || !pmenu->prompt) {
			struct property *p;
			ldd = *ld;
			ldd.sym = psym;
			ldd.p = this;
			for_all_defaults(psym, p) GetSyms(p->expr, &ldd);
		}
	}
}

bool BuildLkcDeps(Node *nn, int flags, void *pv)
{
	NodeLkc26 *n = (NodeLkc26*)nn;
	LkcDep *ld = (LkcDep*)pv;
	if (n->pmenu && n->pmenu->sym == ld->sym) {
		ld->bFound = 1;
		if (!AddSym(n->pmenu->sym)) {
			return 0;
		}
		NodeLkc26 *nn = new NodeLkc26(ld->p, n->pmenu);
		nn->SearchExpr(ld);
	}
	return 1;
}

void NodeLkc26::SearchSyms(struct symbol *s, LkcDep *ld)
{
	if (!s) {
		return;
	}
	if (!CheckSym(s)) {
		return;
	}
	ld->sym = s; ld->bFound = 0;
//	printf("searching %s - %s\n", s->name, ld->sym ? ld->sym->name:0);
	ld->ne->_Enumerate(BuildLkcDeps, NS_ALL, ld);

	if (!ld->bFound) {
		//printf("Didn't find a node.\n");
		int i;
		struct symbol *ss=0;
		for_all_symbols(i, ss)
		if (ss == ld->sym) {
			if (!AddSym(ss)) {
				return; // already done
			}
			goto found;
		}
		return; // sym not found eg: n m y
found:
		NodeLkc26 *n = new NodeLkc26(ld->p, ss);
		if (ss) {
			n->SearchExpr(ld);
		}
	}
}

int NodeLkc26::GetSyms(struct expr *dep, LkcDep *ld)
{

		if (!dep) {
			return 0;
		}
		switch (dep->type) {
		case E_AND:
		case E_OR:
				return GetSyms(dep->left.expr, ld) ||
						GetSyms(dep->right.expr, ld);
		case E_SYMBOL:
				SearchSyms(dep->left.sym, ld);
				return 0;
		case E_EQUAL:
		case E_UNEQUAL:
				SearchSyms(dep->left.sym, ld);
				SearchSyms(dep->right.sym, ld);
				return 0;
		case E_NOT:
				return GetSyms(dep->left.expr, ld);
		default:
				;
		}
		return 0;
}

Node *NodeLkc26::GetDepTree()
{
	Node *ne; // enum
	NodeRoot *nr; // root
	NodeDListP *p;

	nr = Symbols->Root;
	ne = nr->Child->Next; // skip the first source node
	p = nr->DepList;
	p->Clean();

	LkcDep ld;
	ld.sym = pmenu ? pmenu->sym : psym; ld.p=p;
	ld.ne = ne; //temp

	ClearSym();

	if (type == NT_MENU) {
		if (pmenu->prompt) {
			ld.p = new NodeLkc26(p, pmenu);
			GetSyms(pmenu->prompt->visible.expr, &ld);
		}
	} else {
		// do this instead SearchExpr in case of multiple defines ???
		// or is all the symbol deps combined ???
		SearchSyms(ld.sym, &ld);
		//NodeLkc26 *nn = new NodeLkc26(p, pmenu);
		//nn->SearchExpr(&ld);
	}
	return p;
}
// 2.6.x Dependency listing
//------------------------------------------------------------------------------------------###
// NodeLkc26

// include all choice items in deplist ?
#define XREFCHOICE

void AddMenus(NodeParent *np, struct menu *mp)
{
	struct menu *m;
	for (m = mp; m; m = m->next) {
		if (m->prompt || m->sym) {
			NodeLkc26 *n = new NodeLkc26(np, m);
#ifdef XREFCHOICE
			if (n->GetType() != NT_CHOICEP)
#endif
				if (m->list) {
					AddMenus(n, m->list);
				}
		}
	}
}

NodeLkc26::NodeLkc26(NodeParent *np, struct symbol *s)
{
	np->AddChild(this);

	type = NT_COMMENT;
	pmenu = 0;
	psym = s;
	word = 0;

	if (s) {
		char buf[200];
		const char *p = sym_get_string_value(s);
		snprintf(buf, 198, "Def: %s = %s", s->name, p);
		if (s->prop) {
			line = s->prop->lineno;
		} else {
			line = 0;
		}
		SetPrompt(buf);
		//if (!*(p+1)) switch(*p) { case 'n': case 'm': case 'y': type |= NTT_INPUT; }
	} else {
		SetPrompt("????");
		line=0;
	}
}

NodeLkc26::NodeLkc26(NodeParent *np, struct menu *m)
{
	np->AddChild(this);

	if (np->GetType() == NT_CHOICEP) {
		type = NTT_CHOICE;
	} else {
		type = 0;
	}
	pmenu = m;
	psym = m->sym;

	line = m->lineno;
	word = 1;

	// debug
	if (!psym && (!m || !m->prompt)) {
		printf("sym:0 list:%lx - %s\n", (uintptr_t)m->list, menu_get_prompt(m));
		return;
	}
	if (m && !m->prompt) {
		type |= NTT_DEF;
		SetPrompt(menu_get_prompt(m));
		goto getvtype;
	} else if (psym && !sym_has_value(psym)) {
		char buf[200];
		snprintf(buf, 198, "%s %s", menu_get_prompt(m), "(NEW)");
		SetPrompt(buf);
		bNew = 1;
	} else {
		SetPrompt(menu_get_prompt(m));
	}

	if (pmenu->parent && !menu_is_visible(pmenu->parent)) {
		state |= NS_SKIP;
	}
	if (!menu_is_visible(pmenu)) {
		state |= NS_DISABLE;
	}

	// is it a comment ?
	for (struct property *p = m->prompt; p; p = p->next) {
		if ((!p->sym || !p->sym->name) && (p->type == P_COMMENT)) {
			type = NT_COMMENT;
			if (m->list) {
				type |= NT_PARENT; // will this ever ? happen ???
			}
			return;
		}
	}

	// is this a menu ?
	if (!m->sym) {
		type = NT_MENU;
		return;
	}

	// is it a choice menu ?
	for (struct property *p = m->sym->prop; p; p = p->next) {
		if ((!p->sym || !p->sym->name) && (p->type == P_CHOICE)) {
			type = NT_CHOICEP;
#ifdef XREFCHOICE
			if (m->list) {
				AddMenus(this, m->list);
			}
#endif
			return;
		}
	}

getvtype:
	// what is it ?
	switch(m->sym->type) {
	case S_BOOLEAN:		type |= NT_BOOL; goto tri;
	case S_TRISTATE:	type |= NT_TRISTATE;
tri:
		word = sym_get_tristate_value(m->sym) + 1; // conv to 123 NOT 012
		break;
	case S_INT:		type |= NT_INT; goto str;
	case S_HEX:		type |= NT_HEX; goto str;
	case S_STRING:		type |= NT_STRING;
str:
		Set(sym_get_string_value(m->sym));
		break;
	break;
	default:
		type = NT_COMMENT;
		break;
	}
	if (m->list) type |= NTT_PARENT;

	return;
}

void NodeLkc26::Update(int i)
{
	if (!pmenu) {
		goto next;
	}
	if (!psym) {
		goto nextN;
	}
	if (type & NTT_NMY) {
		sym_calc_value(pmenu->sym);
		word = sym_get_tristate_value(pmenu->sym) + 1; // 012 to 123
		Notify(NS_STATE);
	} else if (type & NTT_STR) {
		word = 0;
	}
	if (bNew && sym_has_value(psym)) {
		bNew=0;
		SetPrompt(menu_get_prompt(pmenu));
		Notify(NS_PROMPT);
	}
nextN:
	if (pmenu->parent) {
		if (menu_is_visible(pmenu->parent)) {
			Notify(NS_UNSKIP);
		} else {
			Notify(NS_SKIP);
		}
	}
	if (menu_is_visible(pmenu)) {
		Notify(NS_ENABLE);
	} else if (type & NTT_DEF) {
		Notify(NS_SKIP);
	} else {
		Notify(NS_DISABLE);
	}
next:
	if (i && Child) {
		Child->Update(1);
	}
	if (i && Next) {
		Next->Update(1);
	}
}

bool NodeLkc26::Enumerate(enumNodes en, int flags, void *pv) // SAME for as np for now
{
	if (!(state & ~flags & (NS_SKIPPED | NS_DISABLED | NS_COLLAPSED))) {
		if (Child && !Child->_Enumerate(en, flags, pv)) {
			return 0;
		}
	}
	return 1;
}

bool NodeLkc26::_Enumerate(enumNodes en, int flags, void *pv)
{
	if (!(state & ~flags & (NS_SKIPPED | NS_DISABLED))) {
		if (!en(this, state, pv)) {
			return 0;
		}
	}
	if (type & NTT_PARENT) {
		if (!(state & ~flags & NS_COLLAPSED) && (flags & NS_DOWN) && Child) {
			if (!Child->_Enumerate(en, flags, pv)) {
				return 0;
			}
		}
		if (!(state & ~flags & (NS_SKIPPED | NS_DISABLED))) {
			if ((flags & NS_EXIT) && (!en(this, state | NS_EXIT, pv))) {
				return 0;
			}
		}
	}
	if (Next && !Next->_Enumerate(en, flags, pv)) {
		return 0;
	}
	return 1;
}

uintptr_t NodeLkc26::Set(const char *s, int updt)
{
	if (pmenu && pmenu->sym) {
		sym_set_string_value(pmenu->sym, s);
		if (updt) {
			IUpdate();
		} else {
			Update(0); // only this node
		}
		return word;
	}
	return (uintptr_t)"";
}

const char *NodeLkc26::GetStr()
{
	if (pmenu && pmenu->sym) {
		 return sym_get_string_value(pmenu->sym);
	} else {
		return "";
	}
}

uintptr_t NodeLkc26::Advance(int updt)
{
	if (type & NTT_PARENT && !(type & NTT_INPUT)) {
		Notify(state & NS_COLLAPSED ? NS_EXPAND : NS_COLLAPSE);
		return 1;
	}
	if (type & NTT_NMY && pmenu && pmenu->sym) {
		word = sym_toggle_tristate_value(pmenu->sym) + 1;
		IUpdate();
		return word;
	}
	return 1;
}

uintptr_t NodeLkc26::Set(uintptr_t w, int updt)
{
	if (pmenu && pmenu->sym) {
		if (type & NTT_NMY && w < 4) {
			tristate
			val=no;
			switch(w) {
			case 1: val = no; break;
			case 2: val = mod; break;
			case 3: val = yes; break;
			}
			sym_set_tristate_value(pmenu->sym, val); // conv 123 to 012
			if (updt) {
				IUpdate();
			} else {
				Update(0); // only this node
			}
			return word;
		}else if (type & NTT_STR && w > 3) {
			sym_set_string_value(pmenu->sym, (char*)w);
			if (updt) {
				IUpdate();
			} else {
				Update(0); // only this node
			}
			return word;
		}
	}
	return 1;
}

uintptr_t NodeLkc26::Get()
{
	if (pmenu && pmenu->sym) {
		if (type & NTT_NMY) {
			return sym_get_tristate_value(pmenu->sym) + 1; // conv 012 to 123
		} else if (type & NTT_STR) {
			sym_calc_value(pmenu->sym);
			const char *c = sym_get_string_value(pmenu->sym);
			return (uintptr_t)c;
		} else {
			return 0;
		}
	}
	return 0;
}

char *NodeLkc26::GetHelp()
{
	if (psym) {
		return psym->help;
	} else {
		return "No help available.";
	}
}

char *NodeLkc26::GetSymbol()
{
	if (psym) {
		return psym->name;
	} else {
		return "---";
	}
}

char *NodeLkc26::GetSource()
{
	if (pmenu && pmenu->file) {
		return pmenu->file->name;
	} else if (psym && psym->prop && psym->prop->file) {
		return psym->prop->file->name;
	} else {
		return "---";
	}
}

#endif

// NodeLkc26
//------------------------------------------------------------------------------------------###
// -----END OF Lkc 2.6.x support wrapper-----
//------------------------------------------------------------------------------------------
// END OF FILE


