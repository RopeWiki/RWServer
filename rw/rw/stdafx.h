// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

#pragma once

#ifndef _SECURE_ATL
#define _SECURE_ATL 1
#endif

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers
#endif

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER				// Allow use of features specific to Windows XP or later.
#define WINVER 0x0501		// Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows XP or later.                   
#define _WIN32_WINNT 0x0501	// Change this to the appropriate value to target other versions of Windows.
#endif						

#ifndef _WIN32_WINDOWS		// Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0410 // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE			// Allow use of features specific to IE 6.0 or later.
#define _WIN32_IE 0x0600	// Change this to the appropriate value to target other versions of IE.
#endif

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS	// some CString constructors will be explicit

// turns off MFC's hiding of some common and often safely ignored warning messages
#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions


#include <afxdisp.h>        // MFC Automation classes



#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT


#include <afxsock.h>		// MFC socket extensions

void memorycheck(void); // GLOBAL MEMORY CHECK ANYTIME!!!

#define malloc(size) malloclog(size,__FILE__,__LINE__)
#define calloc(num, size) calloclog(num,size,__FILE__,__LINE__)
#define realloc(ptr, size) realloclog(ptr, size,__FILE__,__LINE__)
#define free(ptr) freelog(ptr,__FILE__,__LINE__)
#ifdef DEBUG
#define chkalloc(ptr) chkalloclog(ptr,__FILE__,__LINE__) // only for debug
#else
#define chkalloc(ptr) /**/
#endif
void *malloclog(int size, const char *file, long line);
void *calloclog(int num, int size, const char *file, long line);
void *realloclog(void *ptr, int size, const char *file, long line);
void freelog(void *ptr, const char *file, long line);
void chkalloclog(void *ptr, const char *file, long line);

#include "arraylist.h"



// This macro is the same as IMPLEMENT_OLECREATE, except it passes TRUE
// for the bMultiInstance parameter to the COleObjectFactory constructor.
// We want a separate instance of this application to be launched for
// each automation proxy object requested by automation controllers.
#ifndef IMPLEMENT_OLECREATE2
#define IMPLEMENT_OLECREATE2(class_name, external_name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
	AFX_DATADEF COleObjectFactory class_name::factory(class_name::guid, \
		RUNTIME_CLASS(class_name), TRUE, _T(external_name)); \
	const AFX_DATADEF GUID class_name::guid = \
		{ l, w1, w2, { b1, b2, b3, b4, b5, b6, b7, b8 } };
#endif // IMPLEMENT_OLECREATE2

#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif


CString http_escape(const char *string);
CString base64_decode(CString &encoded_string);

// FILE I/O
#define FILEBUFFLEN	(4*1024)	// I/O Buffering
#define MAXPATHLEN 1024			// paths and mist stuff

#define FILEEOL "\r\n"
#define FILEEOLSIZE 2
inline BOOL IsEOL(char c) { return c=='\n' || c=='\r'; }
int cmpid(const char *line, const char *id, int idlen);

class CSEEKID {
protected:
	int high, low;
public:
	virtual int getidx(int pos) = 0;
	int seekidx(unsigned date, int low, int high);
}; 

//===========================================================================
// vara & vars
//===========================================================================

#define isalphanum(c) (isalpha(c) || isdigit(c))

typedef int var;

class vara;

class vars : public CString {
public:
	vars(const char c) : CString(c) { }
	vars(const CString &str) : CString(str) { }
	vars(const char *str = "") : CString(str) { }
	vars(const char *str, int len) : CString(str, len) { }
	vara split(const char *sep = ","); 
	vars substr(int start, int len = -1);
	int indexOf(const char *str, int start = 0);
	int lastIndexOf(const char *str, int end = -1);
	int length(void) { return GetLength(); }
	vars trim();
	vars upper() { vars tmp(*this); tmp.MakeUpper(); return tmp;}
	vars lower() { vars tmp(*this); tmp.MakeLower(); return tmp;}
	vars replace(const char *A, const char *B) { vars tmp(*this); tmp.Replace(A,B); return tmp;}
};

class vara : public CArray <vars,vars> {
public:
	vara(void) { };
	vara(const char *str, const char *sep = ",");
	vara(const char *str[]);
	vara(const vara &a) { Append(a); };
	vara &operator=(const vara &a) { RemoveAll(); Append(a); return *this; };
    vars join(const char *sep = ",");
	void splice(int i, int n);
	int length(void) { return GetSize(); }
	void push(const char *str) { Add(str); }
	int indexOf(const char *str);
	int indexOfi(const char *str);
	int indexOfSimilar(const char *str);
	static int vara::cmpstr( const void *arg1, const void *arg2);
	void sort(int sortcmpstr( const void *arg1, const void *arg2) = vara::cmpstr);
	vars last(void) { int n = GetSize(); return n>0 ? ElementAt(n-1) : ""; }
	vars first(void) { int n = GetSize(); return n>0 ? ElementAt(0) : ""; }
	vara flip(void);
};

class varas : public CArray <vara,vara> {
public:
	varas(void) { };
	varas(const char *str, const char *sep = ",", char *sep2 = "=");
	const char *match(const char *str);
	int matchnum(const char *str);
	const char *strstr(const char *str);
};

// File Path
char *GetFileExt(const char *str);
char *GetFileNameExt(const char *str);
CString GetFileName(const char *path);
CString GetFileNoExt(const char *path);

char *StrUnquote(char *str, char *sep);
char *StrFixUrl(char *str);
char *StrFixPath(char *str);

// File Time

class CFILE {
	
	char fext;
	CString fname;
	// fgetc
	int fbuffptr, fbuffsize;
	char fbuff[FILEBUFFLEN];
	// fgetstr
	int bufferlen, bufferlenmax;
	char *buffer;
public:
	HANDLE f;
	static BOOL MDEBUG;
	enum { MREAD = 0, MWRITE, MAPPEND, MUPDATE };
	//operator FILE *() { return f; }
	CFILE(void);
	~CFILE(void);
	BOOL isopen() { return f!=NULL; };
	CString name(void) { return fname; }

	BOOL fopen(const char *file, int mode = CFILE::MREAD);
	void fclose(void);
	int fwrite(const void *buff, int size, int count);
	int fread(void *buff, int size, int count);
	int fseek(int offset, int origin = SEEK_SET);
	int ftell(void);
	int fsize(void);
	double ftime(void);
	int ftruncate(int size);
	char fgetc();
	BOOL fgetstrbuff(char *buff, int maxsize); // 0:EOF -1:CONTINUE 1:DONE
	char *fgetstr(void); // automatic buffer allocation
	int fputstr(char const *line);
	
	static inline int fstrlen(const char *str) { return strlen(str)+FILEEOLSIZE; }
	static BOOL getheader(const char *file, CString &header);
	static BOOL exist(const char  *pFilename);
	static BOOL settime(const char *pFilename, FILETIME *writetime);
	static BOOL gettime(const char *pFilename, FILETIME *writetime, FILETIME *accesstime = NULL); 

	static CString fileread(const char *filename, const char *nl = "\n");
	static int filewrite(const char *filename, const char *str, int size = 0);
};


// multiprocess management
int SetForeground();
int SetBackground();
int SetBkPrinting();
int SetPriority(int pr);
void ThreadLock(void);
void ThreadUnlock(void); 
void ThreadSet(const char *string); 
int ThreadSetNum(void);
CWinThread* BeginThread( AFX_THREADPROC pfnThreadProc, LPVOID pParam );
int WaitThread();
void WaitForThreads();
#define REFRESH_TIME 400

class CSection {
volatile void *locked;
public:
CSection(void);
~CSection(void);
void Lock(void);
void Unlock(void);
};


#define STATPATH "STAT\\"
#define STATDATAPATH STATPATH"DATA"
#define STATDATABKPATH STATPATH"DATA.BK"
#define STATDATAPLPATH STATPATH"DATA+"

#define STATQUERYPATH STATPATH"QUERY\\"
#define STATQUERYOPTPATH STATQUERYPATH"OPT"
#define TODAYCSV STATQUERYPATH"TODAY.CSV"


// Log
enum { LOGINFO=0, LOGWARN, LOGERR, LOGALERT, LOGFATAL };
CString MkString(const char *format, ...);

extern int THREADNUM;
void SetLog(const char *app);
void Log(const char *memory); // RAW
void Log(int type, const char *format, ...);

void ShowProgress(LPCTSTR lpszText);
#define SHIELDC( x ) 	try { x; } catch (CException *e ) { e->Delete(); } catch (...) { }
//#define SHIELDSEH(x) { __try { x; } __except(EXCEPTION_EXECUTE_HANDLER) { Log(LOGERR, "CRASH"); } }
#define SHIELDURL SHIELDC
#define SHIELD SHIELDC
#define SHIELDPROC(x) { \
	BOOL crashed = TRUE; \
	SHIELD(x; crashed=FALSE;); \
	if (crashed) { \
		Log(LOGALERT, "PROCESSCRASH: Crashed in %s", DEBUGCRASH); \
		return TRUE; \
		} \
}

// functions
enum { M_NONE = 0, M_RULE, M_ALL };
enum { T_LONG = 0, T_SHORT, T_BOTH, T_BOTHINV };

void BuyStock(const char *quant, const char *price, const char *stock);
void BuyOptions(const char *quant, const char *price, const char *opt1, const char *opt2);
void BuyOptionsFile(const char *file);
void RollOptionsFile(const char *file);

void MergeSymbols(const char *symfile, const char *symfiles);
void SortSymbols(const char *symfile, const char *symfiles, const char *params = NULL);
void CompareSymbols(const char *symfileA, const char *symfileB, const char *logdiff);
void CompareHistory(const char *symfileA, const char *symfileB, const char *logdiff);
void GetOptionsData(void);
void GetOptions(const char *symfile, const char *dayfile);
void GetFundamentals(const char *symfiles, const char *group = NULL);
void GetHistoricals(int cfg, const char *symfile, const char *filter);
void GetOptionsHistoricals(const char *sym, const char *optlist, const char *optprice);
void GetQuotes(const char *symfiles, const char *outfile);
void GetAccount(const char *outfile);
void GetPosition(const char *outfile, const char *accountfile);

int GetServer(const char *file);
void GetToday(const char *symfile, const char *file);
void GetServerSym(const char *symfile);
void GetServerData(const char *symfile);
void GetServerDataOpt(const char *symfile);

void GetRankings(void);
void PostSymbols( const char *symfile);
void UpdateSymbols(const char *symfile, const char *symmap, const char *symrule = NULL);
void GetSymbols(const char *symfile, const char *symmap, const char *symrule = NULL);
void ReSort(const char *group);
void RePurge(const char *group);
void RePurgeSym(const char *group);
void ReMerge(const char *group);
void ReMergeBk(const char *group);
void ReCheck(const char *group);
void ReCheckSym(const char *group);
void ReCheckRank(const char *folder);
void ReCheckBk(const char *group);
void ReCheckPl(const char *group);

void ReOptions(void);
void ReFormat(CString input);
void ReScanStats(CString symfile, CString fromdate);
void ReStatFile(CString statfile, CString *params);
void TestHistory(CString sym, int runw, int rulew, int simw = 0);
void RunHistory(CString sym, int runw, int rulew, int simw);

// file functions


#define DATELEN 10			// length of date is 10 characters
#define ESTIMATE 'E'

inline int GetLength(const char *strval)
{
register int len;
for (len=0; strval[len]!=0 && strval[len]!=','; ++len);
return len;
}

inline BOOL IsDate(register const char *strval, register int len = -1)
{
if (len<0) len = GetLength(strval);
return (len>=DATELEN && len<=DATELEN+1 && strval[4]=='-' && strval[7]=='-');
}


inline BOOL IsEstimate(const char *date) 
{ 
	return date[DATELEN]==ESTIMATE; 
}

class inetdata {
public:
	//virtual void open(int expectedsize) {};
	//virtual void close(int receviedsize) {};
	virtual void write(const void *buffer, int size) = 0;
	void write(const char *buffer) { write((void *)buffer, strlen(buffer)); };	
	int PhantomJS(const char *file, volatile int &nfiles);
};

class inetfile : public inetdata {
	FILE *f;
	//int size;

	public:
	inetfile(const char *file) 
	{
		f=fopen(file,"wb");
	}
		~inetfile()
	{
		if (f) fclose(f);
	}

	void write(const void *buffer, int size)
	{
		if (f) fwrite(buffer,size,1,f);
	}

};



class inetmemory : public inetdata {
	public:
	CString memory;

	inetmemory(void)
	{
	}

	~inetmemory()
	{
	}


	void write(const void *buffer, int size)
	{
		memory.Append( CString((const char *)buffer, size) );
	}

};


class inethandle: public inetdata {
	HANDLE h;

	public:

	inethandle(HANDLE h) : inetdata()
	{
		this->h = h;
	}
	virtual ~inethandle()
	{
		CloseHandle(h);
	}

	void write(const void *buffer, int size)
	{
		DWORD dwWritten;
		WriteFile(h, buffer, size, &dwWritten, NULL);
	}

};

class inetmemorybin : public inetdata {

	public:
	char *memory;
	int size;

	inetmemorybin(void)
	{
		size = 0;
		memory = NULL;
	}
	~inetmemorybin()
	{
		if (memory)
			free(memory);
	}


	void write(const void *buffer, int bsize)
	{
		memory = (char *)realloc(memory, size+bsize);
		memcpy(memory+size, buffer, bsize);
		size += bsize;
	}

	void write(const char *buffer) { write((void *)buffer, strlen(buffer)); };
};

char *GetSearchString(const char *mbuffer, const char *searchstr, CString &symbol, char startchar = 0, char endchar = 0);

char *GetSearchString(const char *mbuffer, const char *searchstr, CString &symbol, const char *startchar, const char *endchar);


CString colstr(int n);

typedef int qfunc(const void *arg1, const void *arg2);
int qfind(const void *search, const void *table, int num, int size, int cmpid( const void *arg1, const void *arg2), int *aprox = NULL);

class CStringArrayList {
	int size;
	int maxsize;
	CString **data;
	BOOL sorted;
	void SetMaxSize(int newsize);
public:
	void SetSize(int newsize) { SetMaxSize( size = max(size, newsize) ); size = newsize; }
	CStringArrayList(int presetsize = 0);
	CStringArrayList(CStringArrayList &other);
	int AddTail(const char *str); // { CString s(str); return AddTail(s); }
	//int AddTail(CString &str) { AddTail} ;
	void AddTail(CStringArrayList &list);
	inline int GetSize(void) { return size; }
	inline CString& operator[](int i) { return *data[i]; }
	~CStringArrayList(void);
	void Reset(int presetsize = 0);
	void Remove(int n);
	static int cmpcomma( const void *arg1, const void *arg2);
	static int cmpstr( const void *arg1, const void *arg2);
	void Sort(qfunc *cmpfnc = cmpstr);
	int Find(const char *str, qfunc *cmpfnc = cmpstr);
};

class DownloadFile;
typedef int fpostfile(DownloadFile *df, const char *file, inetdata &data);

class DownloadFile {
inetdata *idata;
int DownloadUMON( const char *urlstr, const char *file);
int DownloadINET( const char *urlstr, const char *file);

CString ScanFormValue(const char *id, const char *pre = "name");
CString FormValue(const char *name, const char *val);
CString FormFile(const char *name, const char *val, const char *mime);
CString FormEnd(void);

public:
CStringArrayList sessionids;
CString tmpfile;
BOOL userpass;
BOOL binary;
BOOL usereferer;
int size;
void *hSession;
char *memory;
const char *headers, *hostname;

varas lists;
vars lasturl, action, enctype, method;
struct {vars name, local, remote, mime; } file;

// form
CString FormValues(void);
const char *GetFormValue2(const char *name);
int SetFormValue(const char *name, const char *val);
int SetFormFile(const char *name, const char *remotefile, const char *mime, const char *localfile);
int GetForm(const char *key = NULL); // form key for multiforms
int SubmitForm(void);
int PostFile(inetdata &data);


// custom post using ?POSTFILE?
//void SetPostFile(fpostfile func) { postfile = func; }


DownloadFile(BOOL binary = FALSE, inetdata *idataobj = NULL);
~DownloadFile();
int Login(const char *url);
int Download(CString url, const char *file = NULL, BOOL dump = FALSE);
int DownloadNoRetry( const char *urlstr, const char *file = NULL);
int Load(const char *filename);
static void AddUrl( CString &url, const char *line);
};

// registry access
BOOL RegReset();
const TCHAR *RegKey();
const TCHAR *RegApp();
BOOL RegSetInt(TCHAR *section, TCHAR *name, int val);
int RegGetInt(TCHAR *section, TCHAR *name, int def);
BOOL RegSetString(TCHAR *section, TCHAR *name, const TCHAR *val);
CString RegGetString(TCHAR *section, TCHAR *name, const TCHAR *def = NULL);
BOOL RegSetData(TCHAR *section, TCHAR *name, void *ptr, int size);
void *RegGetData(TCHAR *section, TCHAR *name, void *data = NULL, int rsize = 0);
BOOL RegSetStringList(TCHAR *section, TCHAR *name, CStringList *list, int n = -1);
int RegGetStringList(TCHAR *section, TCHAR *name, CStringList *list);
BOOL RegSetComboBox(TCHAR *section, TCHAR *name, CComboBox *list, int n = -1);
int RegGetComboBox(TCHAR *section, TCHAR *name, CComboBox *list);

// Token
int IsMatch(const char buffer, const char *str);
int IsMatch(const char *buffer, const char *strs[]);
int IsMatchN(const char *buffer, const char *strs[]); // num match
int IsContained(const char *buffer, const char *strs[]);
int IsContainedN(const char *buffer, const char *strs[]); // num match
int IsMatchNC(const char *buffer, const char *strs[]); // case insens
int IsMatchNC(const char *buffer, vara &strs); // case insens

const char *strstri(const char *str, const char *str2);
int IsSimilar(const char *str, const char *cmp);
#define IsEqual(str, cmp) (stricmp(str,cmp)==0)
BOOL MatchStr(CString &str, const char *match[]);
BOOL IsMatchSimilar(const char *sstr, const char *match[], int startonly = FALSE);

typedef struct {
		char *str;
		double num;
	} tmap;

int Map(const char *buffer, tmap *m, BOOL substr = FALSE);

inline int IsEmptyToken(const char c) { return c==',' || c==0; }
int GetTokenCount(const char *str, char sep =',');
int GetTokenPre(const char *line, int count, char sep = ',');
inline int GetTokenPos(register const char *line, register int count, register char sep =',');
const char *GetTokenRest(const char *str, int count, char sep = ',');
CString GetNextToken(const char *&ptr, char sep =',');
CString GetToken(const char *str, int n, char sep =',');
CString GetToken(const char *str, int n, const char *separator);

// Date
double GetTime(void);
extern double CurrentDate, RealCurrentDate;
double SetCurrentDate( double date = 0);
double NoWeekend( double date, BOOL advance = FALSE );
double Date( COleDateTime &date, BOOL real = FALSE);
double DateFloat( COleDateTime &date, BOOL real = FALSE);
COleDateTime OleDateTime( double date);
double Date( int D, int M, int Y);

// error box
BOOL SetOutput(const char *file);
BOOL ResetOutput();
int MessageBox(LPCTSTR lpszText, BOOL newline = TRUE);
int ErrorBox( LPCTSTR lpszText, UINT nType = MB_OK | MB_ICONERROR);
int ErrorBox( UINT idText, UINT nType = MB_OK | MB_ICONERROR);
int ErrorBox( UINT idText, LPCTSTR lpszText, UINT nType = MB_OK | MB_ICONERROR);


// math
#define DATEFORMAT "%Y-%m-%d"  
#define OLDDATEFORMAT "%m-%d-%y"



BOOL pcheck(int &counter, double seconds);

#define PERCENT(n, maxn) ((maxn)==0 ? 0 :(int)((n)*100.0/(maxn)))

class CProgress;
class CProgress {
	int id, n, maxn, minn;
	static int pct, accpct, npct;
	static int ticks, finishticks;
	CString name;
	static int index;
	static CProgress *list[20];
public:
	CProgress(const char *name, int minn, int maxn);
	~CProgress(void);
	void Set(int n, const char *text = NULL);
	void SetBuffering(int pct);
	static int GetAvgBuffering();
};

class cfg {
private:
	int val;
	int min;
	int max;
public:
	cfg(void);
	cfg(int v);
	cfg(const char *v);
	operator int();
};

// profiling

extern BOOL NOPROFILE;

class CProfileID {
public:
	int id;
	CProfileID(const char *str);
};

class CProfile {
private:
	int id;
public:
	CProfile(int id);
	~CProfile(void);
};

#define PROFILE(str) static CProfileID prfid(str); CProfile prf(prfid.id);
void PROFILEReset(void);
void PROFILESummary(const char *name);

/* Converts a hex character to its integer value */
inline char from_hex(char ch) {
  return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}

/* Converts an integer value to its hex character*/
inline char to_hex(char code) {
  const char *hex = "0123456789abcdef";
  return hex[code & 15];
}
void FromHex(const char *str, void *data);
CString ToHex(void *data, int bytes);
CString url_encode(const char *str);
CString url_decode(const char *str);


vars ExtractString(const char *mbuffer, const char *searchstr, const char *startchar = "\"", const char *endchar = "\"");
vars ExtractLink(const char *mbuffer, const char *searchstr);


