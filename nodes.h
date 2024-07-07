//nodes.h - nconfig kernel configuration backend header.

//Copyright (C) 2004-2006 Rikus Wessels <rikusw at rootshell dot be>
//GNU GPL v2.0

#ifndef _NODES_H_
#define _NODES_H_

#include <stdint.h>

// the number of CONFIG_'s allowed, normally between 2000 and 2500
#define SYMBOL_LIMIT 3000

// Config.in 2.4.x support
// comment to remove 2.4.x support
#define NC24

#ifdef NC24
	// enable for parsing 'nchoice' nodes as well
	// it was never used as far as I know
	//#define USE_NCHOICE
#else
	#undef USE_NCHOICE
	#ifndef LKC26
		#error Either NC24 or LKC26 (or both) must be enabled!!!!
	#endif
#endif

/* Node value -- word
 * Symbols value
 *
 * 0 - undefined
 * 1 - N
 * 2 - M
 * 3 - Y
 * >3- char *
 *
 */

class Node;
class NodeParent;
class NodeSymbols;
class NodeDListP;


// used for loading the config.in, .config and help files.
class CfgFile
{
public:

	CfgFile(Node *n);
	~CfgFile();

	int LoadHelp(NodeSymbols *Symbols);		// help file
	int LoadSymbols(NodeSymbols *Symbols);		// config file
	int Parse(NodeParent *np);			// config.in file
	int ReadLine();
	int Open(const char *strFile, bool bPath=1);
	int TestOpen(const char *strFile, bool bPath=1);
	int Close();
	int StrCmpAdv(const char *, const char *, int);	// internal with advance ptr
	void ParseError(const char *msg);

	char FName[100];
	int LineNum;
	void *pmem;
	char *cmem;
	char *LineStart;
	char *LineCurrent;
	char *LineNext;
	char *EndMem;
	NodeSymbols *Symbols;
};

// callback for Enumerate and Notify
typedef bool (*enumNodes)(Node *n, int flags, void *pv);
typedef bool (*SearchNodes)(Node *n, void *pv);

enum // Node state flags
{
	// used mostly by Notify
	// only ONE of these will be passed to the callback at a time
	NS_SKIP		= 1, 	// skipped by  if...else
	NS_UNSKIP	= 9,
	NS_DISABLE	= 2, 	// disabled by deps
	NS_ENABLE	= 0xA,
//	NS_HIDE		= 4, 	// used to be used...
//	NS_SHOW		= 0xC,
	NS_COLLAPSE	= 4, 	// only for a collapsed parent
	NS_EXPAND	= 0xC,
	NS_INVERT	= 8, 	// when the reverse happens - used by Notify
	NS_STATE	= 0x10, 	// state change by the user
	NS_PROMPT	= 0x20, 	// prompt update - used ONLY in Notify
	NS_SELECT	= 0x30, // ui should select the node (convenience)

	// Flags to be used by Enumerate
	NS_SKIPPED	= 1,
	NS_DISABLED	= 2,
//	NS_HIDDEN	= 4,
	NS_COLLAPSED	= 4,
	NS_EXIT		= 0x20, // used when parent node exit`ing
	NS_DOWN		= 0x40, // _Enumerate down through parents

	// Enumerate flags
	// menu driven app
	NS_M_NORMAL	= NS_EXIT, //| NS_DISABLED,
	NS_M_ALL	= NS_EXIT | NS_DISABLED | NS_SKIPPED,
	
	// listing driven app, with collapsible parents
	NS_P_NORMAL	= NS_EXIT | NS_DOWN, // | NS_DISABLED,
	NS_P_ALL	= NS_EXIT | NS_DOWN | NS_DISABLED | NS_SKIPPED,
	
	// tree view construction
	NS_T_NORMAL	= NS_COLLAPSED, // | NS_DISABLED,
	NS_T_ALL	= NS_COLLAPSED | NS_DISABLED | NS_SKIPPED, // _should use this_
	
	// Notify driven app ... used for what...???
	NS_NORMAL	= NS_COLLAPSED | NS_DOWN, // | NS_DISABLED, // 6
	NS_ALL		= NS_COLLAPSED | NS_DOWN | NS_DISABLED | NS_SKIPPED   // 7
};

enum // Node type flags and values
{
	NTT_VISIBLE	= 0x80000000, // user visible ? - /prompt/
	NTT_INPUT	= 0x40000000, // user input ?

	NTT_NMY		= 0x00010000, // the node use <n, m, y> values
	NTT_TRI		= 0x00020000, // 3
	NTT_BOOL	= 0x00040000, // 5
	NTT_MBOOL	= 0x00080000, // D
	
	NTT_STR		= 0x00100000, // the node use a string value
	NTT_HEX		= 0x00200000, // 3
	NTT_INT		= 0x00400000, // 5
	NTT_STRING	= 0x00800000, // 9

	NTT_PARENT	= 0x00008000, // 2.x
	NTT_DEP		= 0x00004000, // 2.4
	NTT_DEF		= 0x00002000, // 2.x
	NTT_CHOICE	= 0x00001000, // 2.x

//-------------------------------------
	
	NT_NODE		= 0x00000000, // this
	NT_PARENT	= 0x00008000, // and this is never used
	
	NT_ROOT		= 0x80008001, // 2.x
	NT_MENU		= 0x80008002, // 2.x
	NT_IF		= 0x00008003, // 2.4
	NT_SOURCE	= 0x00008004, // 2.4
	
//-------------------------------------
// Input Nodes
	
	NT_BOOL		= 0xC0050000, // 2.x
	NT_TRISTATE	= 0xC0030000,
	NT_HEX		= 0xC0300000,
	NT_INT		= 0xC0500000,
	NT_STRING	= 0xC0900000,
	
	NT_DEP		= 0x00004000, // 2.4
	NT_DEPBOOL	= 0xC0054000,
	NT_DEPMBOOL	= 0xC00D4000,
	NT_DEPTRISTATE	= 0xC0034000,

	NT_CHOICEP	= 0x80008005, // parent choice 2.x
	NT_CHOICE	= 0xC0051000, // bool 2.4

//---------------------------
// Define Nodes
	
	NT_UNSET	= 0x00002001, // 2.4
	NT_DEFBOOL	= 0x00052000,
	NT_DEFTRISTATE	= 0x00032000,
	NT_DEFHEX	= 0x00302000,
	NT_DEFINT	= 0x00502000,
	NT_DEFSTRING	= 0x00902000,
	
//---------------------------
// Misc Nodes

	NT_TEXT		= 0x8000000D, // 2.4
	NT_COMMENT	= 0x8000000E, // 2.x
	NT_SYMBOL	= 0x0000000F, // internal
	NT_DLIST	= 0x80008006, // 2.x

};

//---------------------------------------------------------------------------------------
// normal node - can't have children
class Node
{
	// well umh, maybe node don't need that many friends....
	friend class CfgFile; // to enable Parse();
	friend class NodeSymbols;
	friend class NodeParent;
	friend class NodeIf;
	friend class NodeRoot;
	friend class NodeDListP;
	friend class NodeLkc26; // <-- should propably be fixed...
public:
	Node();
	virtual ~Node();

	//--------------------------
	// START node navigation
	
	virtual Node *GetChild() { return 0; };
	virtual bool IsMyChild(Node *n) { return 0; };
	bool IsChildOf(NodeParent *n) { return n == Parent; };
	NodeParent *GetParent(unsigned int tp = 0, unsigned int flags = ~0);

	// get all nodes below this one
	virtual bool Enumerate(enumNodes en, int flags, void *pv);  //excl
	virtual bool _Enumerate(enumNodes en, int flags, void *pv); //incl + next
	
	// get all nodes following this one
	virtual Node *Search(SearchNodes sn, void *pv, int i=3);
	
	// cross referencing
	virtual Node *GetDepTree(); // USE THIS
	virtual void GetDepTree(NodeDListP *d); // !NOT! THIS

	// END node navigation
	//--------------------------
	// START ui

	// user input
	virtual uintptr_t Advance(int updt=1);		// cycle through NMY
	virtual uintptr_t Set(uintptr_t w, int updt = 1);	// set word and Symbols->val
	virtual uintptr_t Get();
	
	// user input - str version
	virtual uintptr_t Set(const char *s, int updt = 1);	// set word and Symbols->val
	virtual const char *GetStr();

	void Select() { Notify(NS_SELECT); };	// pass NS_SELECT to the frontend
	void SetExpanded(bool b) { if (b) state &= ~NS_COLLAPSED; else state |= NS_COLLAPSED; };

	// misc getXxx
	virtual char *GetHelp();	// Get the help in Configure.help
	virtual char *GetSymbol();	// Get the CONFIG_.....
	virtual char *GetPrompt();	// Get the prompt
	virtual int  GetDeps();		// Get the number of dependencies on this node -- 2.4
	virtual int GetConfig() { return Config; };		// the index in the symbols node -- 2.4
	virtual int GetState() { return state & NS_ALL; };	// the state of the node
	unsigned int GetType() { return type; };		// get the node type NT_*
	int GetLine() { return line; };				// get the line number
	virtual char *GetSource();				// get the source filename

	////ifdef LKC26 - include even in 2.4.x to keep the class interface the same
	virtual struct menu *GetLkcMenu() { return 0; };
	virtual struct symbol *GetLkcSymbol() { return 0; };
	////endif

	// END ui
	//--------------------------

	
// should be protected:
	virtual void Load();
	virtual void Save(FILE *c, FILE *h);
	virtual void Update(int i = 3);
	virtual void IUpdate(); // start at the root node
	void SetPrompt(const char *s); // INTERNAL USE
	
	void Notify(int flags); // maybe public ???

protected:
	// parsing -- 2.4
	virtual bool Parse(char *s, int l) { return 0; };
	char *ParsePrompt(char *s);
	char *ParseSymbol(char *s);
	char *ParseWord(char *s);
	void DefPrompt(); // set the prompt for def nodes...

// ----variables----
protected:
	// Node Info
	unsigned int state;
	unsigned int type;

	Node *Next;
	NodeParent *Parent;
	NodeSymbols *Symbols;
	
	// Parse Info
	char *prompt;
	int Config, line;
	
	uintptr_t word, wordlookup, prevword;

public:
	// User data, public
	void *user;
};

#ifdef NC24
class NodeDep : public Node // 2.4
{
public:
	NodeDep() { type = NT_DEP; DepCount = 0; };
	NodeDep(unsigned int t) { type = t; DepCount = 0; };

	virtual void GetDepTree(NodeDListP *d);
	virtual bool _Enumerate(enumNodes en, int flags, void *pv);

	virtual uintptr_t Advance(int updt=1); // cycle through nmy
	virtual uintptr_t Set(uintptr_t w, int updt=1); // set word and Symbols
	virtual uintptr_t Get();

protected:
	bool Parse(char *s, int l);
	
	uintptr_t CheckDep(uintptr_t w); // protected ?? -- 2.4
	
	virtual void Update(int i=3);
	char *ParseDeps(char *s);	// -- 2.4
	bool AddDep(int i);		// used when parsing -- 2.4

// ----variables----

	int DepCount;
	int DepList[10];
};
#endif //NC24

// 2.4 + some 2.6
// but so hardcoded that I leave it alone for now, maybe always...
// it not only does the symbols but misc other stuff as well:
// (Modified, Notify, Path)
// (Help -- 2.4)

class NodeRoot;
class NodeSymbols : public Node
{
public:
	NodeSymbols(const char *Arch, NodeRoot *r);
	~NodeSymbols();

	// //* = possibly useful //- = rather use the Node:: version
	int AddSymbol(char *s, uintptr_t v);// used for parsing+loading
	void Clear();			// used for loading a config file
	void Update(int i) {};		//

	inline bool HasModules();	//*is CONFIG_MODULES = y ?
	int GetDeps(int s);		//-get dependency count
	char *GetSymbol(int s);		//-get "CONFIG_*"
	int GetCount() { return Count; }//*Symbol Count

	uintptr_t Get(int s);	//-get the value
	void Reset(int s, Node *n);	// used when NodeIf skip nodes
	bool Set(int s, uintptr_t v, Node *n); // set the value
	bool IsDefined(int s);		//*was the symbol defined ?
	bool IsRedefined(int s);	//*was the symbol redefined ?

	void UnloadHelp();		//*use to reload Configure.help
	char *GetHelp(int s);		//-use Node::GetHelp instead
	void AddHelp(char *cfg, char *helpstr); // used by CfgFile::LoadHelp();

	inline char *GetArch() { return arch; };
	inline char *GetPath() { return path; };
	bool SetPath(const char *s);	//*don't use unless you know what you do...
					// parsing, help, loading and saving depend on this.

	bool bModified;			// did the user change a setting ?
	enumNodes Ntfy;			// notify function
	bool bPromptSpaces;		// keep the original prompt spacing ? - ___deprecated___

//	void GetRedefs(NodeRoot *r);
//	int iRedef;
//	Node **Redefs;
//	Node **pRedefs;

	NodeRoot *Root;			// the root node

protected:

	CfgFile *HelpFile;		// !0 if Configure.help is loaded
	int Count;			// number of symbols
	char *path;			// kernel path
	char *arch;			// kernel architecture
	char *help[SYMBOL_LIMIT];	// Pointers to help texts
	int dep[SYMBOL_LIMIT];		// number of deps on config_ + defined flag 0x80000000
	Node *who[SYMBOL_LIMIT];	// which node set the config_? used by Reset
	char *syms[SYMBOL_LIMIT];	// config_ name
	uintptr_t val[SYMBOL_LIMIT]; // value NMY/str
};

//-------------------------------------------------------------------

// parent node - can have children
class NodeParent : public Node
{
//    friend class Node;
public:
	NodeParent();
	virtual ~NodeParent();
	
	virtual bool Enumerate(enumNodes en, int flags, void *pv);  //excl
	virtual bool _Enumerate(enumNodes en, int flags, void *pv); //incl + next
	virtual Node *Search(SearchNodes sn, void *pv, int i = 3);
	virtual uintptr_t Advance(int updt = 1);
	virtual Node *GetChild() { return Child; };
	virtual bool IsMyChild(Node *n);

protected:

	virtual void Load();
	virtual void Save(FILE *c, FILE *h);
public:
	virtual void Update(int i = 3);
protected:
	virtual bool Parse(char *s, int l) { return 0; };
public:
	virtual void AddChild(Node *n); // used when parsing

// ----variables----
public:
	Node *Child, *Last;
};

//-------------------------------------------------------------------

class NodeRoot : public NodeParent
{
public:
	NodeRoot(); //{ type=NT_ROOT; };
	~NodeRoot() { FreeH(); };

	Node *GetRealRoot() { return Parent; };			// get the root source node
	char *GetArch() { return (char*)Symbols->Get(4); };	// = i386 ...
	char *GetPath() { return Symbols->GetPath(); };		// = /usr/src/linux ...
	NodeSymbols *GetSymbols() { return Symbols; };		// get extra info from symboltable
	virtual char *GetHelp() { return (char*)word; };
	Node *GetDepTree();

	const char *GetFirstArch();		// to be used as:
	const char *GetNextArch(char *path = 0);// for(p=GetFirstArch(); p; p=GetNextArch()) {}

	char *GetHelpH(Node *n);		// get formatted help
	char *GetFileH(const char *fn);		// load a file
	char *GetDirH(const char *dn);		// load a directory - use by GetFileH
	void FreeH(char *f);			// use ONLY when autofree is off
	void SetAutoFreeH(bool b);		// on by default
	char *GetLinkFileH(const char *line, int x, char **start, char **end, int *l = 0, char **fn = 0); // convenience
	char *GetLink(const char *line, int x, char **start, char **end);// used for browsing docs
	
	bool bLkc26;
	NodeDListP *DepList;

	int  Init_CmdLine(int argc, char *argv[], bool bSpc = false);
	int  Init(const char *Arch = 0, const char *Path = 0, const char *ConfigFile = 0, bool bSpc = false);
	bool Modified() { return Symbols->bModified; };
	bool Save(const char *ConfigFile);
	bool Load(const char *ConfigFile);
	void SetNotify(enumNodes en) { Symbols->Ntfy = en; };	// want node state callbacks ?
	virtual bool _Enumerate(enumNodes en, int flags, void *pv);
	virtual Node *Search(SearchNodes sn, void *pv, int i = 3);

//xxxxprotected: xxxx
	virtual void Update(int i = 3);
protected:
	bool Parse(char *s, int l);
private:
	void FreeH();
	bool bAutoFreeH;
	char *pHelpMem;
};

//-------------------------------------------------------------------
//START 2.4 specific
#ifdef NC24

struct Condition
{
	int op1, op2, cond;
};

class NodeIf : public NodeParent
{
	friend class CfgFile; // for bElse
	friend class Node; // for Else
public:
	NodeIf(); //{ type=NT_IF; };
	~NodeIf();
	
	bool If();
	virtual char *GetPrompt();
	virtual void GetDepTree(NodeDListP *d);
	virtual bool Enumerate(enumNodes en, int flags, void *pv);
	virtual bool _Enumerate(enumNodes en, int flags, void *pv);
	virtual Node *Search(SearchNodes sn, void *pv, int i = 3);
	virtual Node *GetChild() { return Child ? Child : Else; };

	void AddChild(Node *n); // protected ????
	
protected:
	virtual void Load();
	virtual void Save(FILE *c, FILE *h);
	void Update(int i = 3);

protected:
	bool Parse(char *s, int l);
	bool AddCondition(int op1, int op2, int cond); // used by parse

// ----variables----

	int Count; // condition count
	Condition xxcond[13];
	Condition *Cond;
	int CountLimit;

	int prev_f; // used ONLY by Update
	int bElse;
	Node *Else;
};
#endif //NC24

class NodeSource : public NodeParent {
public:	NodeSource() { type = NT_SOURCE; };
protected: bool Parse(char *s, int l);
};
class NodeMenu : public NodeParent {
public:	NodeMenu() { type = NT_MENU; };
protected: bool Parse(char *s, int l);
};

#ifdef NC24
class NodeComment : public Node {
public:	NodeComment() { type = NT_COMMENT; };
protected: bool Parse(char *s, int l);
};

//-------------------------------------------------------------------
// 2.4

class NodeTri : public Node {
public:	NodeTri(unsigned int t) { type = t; };
protected: bool Parse(char *s, int l);
};
class NodeStr : public Node {
public:	NodeStr(unsigned int t) { type = t; };
protected: bool Parse(char *s, int l);
};

//-------------------------------------------------------------------
// 2.4

class NodeDef : public Node {
public: NodeDef(unsigned int t) { type = t; };
protected: bool Parse(char *s, int l);
};
class NodeUnset : public Node {
public: NodeUnset() { type = NT_UNSET; };
protected: bool Parse(char *s, int l);
};

//-------------------------------------------------------------------
// 2.4

class NodeChoiceP : public NodeParent
{
public:
	NodeChoiceP() { type = NT_CHOICEP; };
	char *GetHelp();
	char *GetSymbol();
	int GetConfig();
protected:
	bool Parse(char *s, int l);
};
#ifdef USE_NCHOICE
class NodeChoiceNP : public NodeChoiceP {
public:	NodeChoiceNP() { type = NT_CHOICEP; };
protected: bool Parse(char *s, int l);
};
#endif

class NodeChoice : public Node
{
	friend class NodeChoiceP;
#ifdef USE_NCHOICE
	friend class NodeChoiceNP;
#endif
public:
	NodeChoice() { type = NT_CHOICE; DefaultChoice = 0; };
	virtual uintptr_t Advance(int updt = 1);		// Set to y
	virtual uintptr_t Set(uintptr_t w = 3, int updt = 1);	// default to y
protected:
	virtual void Update(int i = 3);
	bool Parse(char *s, int l);
	bool DefaultChoice;
};

#endif //NC24
//END 2.4 specific
//-------------------------------------------------------------------

class NodeDList;

class NodeDListP : public NodeParent // 2.4 + some 2.6
{
public:
	NodeDListP(NodeRoot *n);
	~NodeDListP() {};

	void AddChild(Node *n) { NodeParent::AddChild(n); };
	void Clean() { if(Child) delete Child; Child=0; };
	void Update(int i = 3) { if(Child) Child->Update(i); };
#ifdef NC24
	void AddChild(NodeDList *n);
	void CheckOutConfig(int c);
protected:
	Node *Root;
#endif
};

#ifdef NC24
class NodeDList : public Node // 2.4
{
	friend class NodeDListP;
public:
	NodeDList(Node *n);
	NodeDList(char *s, int c);
	virtual ~NodeDList() {};

	// internal
	virtual void Update(int i = 3);
	
	// get all nodes below this one
	virtual bool _Enumerate(enumNodes en, int flags, void *pv); //incl
	
	// user interfacing
	Node *GetDepTree() { if(NLink) return NLink->GetDepTree(); else return 0; };
	char *GetSource() { if(NLink) return NLink->GetSource(); else return 0; };
	int GetState() { if(NLink) return NLink->GetState(); else return state & NS_ALL; };

	// user input
	uintptr_t Get()
	{ if(NLink) return NLink->Get(); else return 0; };
	uintptr_t Advance(int updt = 1)
	{ if(NLink) return NLink->Advance(updt); else return 0; };
	uintptr_t Set(uintptr_t w, int updt = 1)
	{ if(NLink) return NLink->Set(w, updt); else return 0; };

protected:
	Node *NLink;
};
#endif //NC24

//-------------------------------------------------------------------

#ifdef LKC26
struct LkcDep; // internal use

class NodeLkc26 : public NodeParent // 2.6 ... of course :)
{
	friend bool BuildLkcDeps(Node *n, int flags, void *pv);
public:
	NodeLkc26(NodeParent *np, struct menu *m);
	NodeLkc26(NodeParent *np, struct symbol *s);

	virtual bool Enumerate(enumNodes en, int flags, void *pv);
	virtual bool _Enumerate(enumNodes en, int flags, void *pv);
	
	// user input
	virtual uintptr_t Advance(int updt = 1);		// cycle through NMY
	virtual uintptr_t Set(uintptr_t w, int updt = 1);	// set word and Symbols->val
	virtual uintptr_t Get();
	virtual uintptr_t Set(const char *s, int updt = 1);	// set word and Symbols->val
	virtual const char *GetStr();
	virtual void Update(int i = 3);

public:
	// user interfacing
	virtual char *GetHelp();// { return pmenu->sym->help; };// Get the help in Configure.help
	virtual char *GetSymbol();// { return pmenu->sym->name; };	// Get the CONFIG_.....
	virtual int  GetDeps() { return 0; };		// Get the number of dependencies on this node
	virtual int GetConfig() { return 0; };		// the index in the symbols node
	virtual char *GetSource();// { return pmenu->file->name; };	// get the source filename

	virtual struct menu *GetLkcMenu() { return pmenu; };
	virtual struct symbol *GetLkcSymbol() { return psym; };
	
	Node *GetDepTree();

protected:
	void SearchExpr(LkcDep*);
	int GetSyms(struct expr*, LkcDep*);
	void SearchSyms(struct symbol*, LkcDep*);


protected:
	struct menu *pmenu;
	struct symbol *psym;
	bool bNew;

private:
	bool Parse(char *s, int l) { return 0; }; // not used
};
#endif

#endif //_NODES_H_

