// TradeMaster.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols

#define DEBUGOUTFILE ">>>>>>>>>>>>>>>>>>>>>>>>>>%s<<<<<<<<<<<<<<<<<<<<<<<<"
#define THREADDAYS	(21+2)	//43	// compute 1M+2 days each night (needed for fP%)
#define MAKEVREF 10			// reuse value for 2 week if gaps are present (reference date)
#define MAKEVDAYS 15		// reuse valid values from last 3 weeks (mindays date, to take into account Thanksgiving)

#define YEARDAYS 253
#define SYMECONOMY "^" // economy ticker
#define EXTRAQUOTEDAYS 10 //@@@ adjust this if working with old data set

//#define DAYS2COUNT(d) ((d)*(252.75/365.25)+1)
#define COUNT2DAYS(d) ((int)((d)*(366.0/252)))
#define PreviousDate(d) (CurrentDate - COUNT2DAYS(d))

#define HDRSYM "SYM"
#define HDRDATE "DATE"
#define HDRVDATE "VDATE"


#define	REFSYM "HPQ"		// reference symbol, in case we need to go look for headers
#define SYMINTRATE "^IRX"	// interest rate 


inline BOOL IsEvtFile(const char *filename) { return strchr(filename, '@')!=NULL; }
inline BOOL IsGroupFile(const char *filename) { return strchr(filename, '#')!=NULL; }
inline BOOL IsGlobalFile(const char *filename) { return strchr(filename, '^')!=NULL; }

// CTradeMasterApp:
// See TradeMaster.cpp for the implementation of this class
//
class PROCESSCGI {

protected:
	int ticks;
	int bcount, lcount;

public:
	PROCESSCGI(); 
	virtual BOOL OUTDATA(const char *line, int len) = 0;   // FALSE to abort output
	virtual BOOL OUTLINE(const char *line) = 0;  // FALSE to abort output
	int PROCESSOUT(const char *querystr, const char *postdata, int postlen);
};


class CTradeMasterApp : public CWinApp
{
public:
	CTradeMasterApp();

// Overrides
	public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

//extern CTradeMasterApp theApp;
extern CString DEBUGSTR, DEBUGCRASH, GLOBAL;
extern cfg WEEKMODE, SUMMARYSTART, SUMMARYMODE, LEARNWIN, LEARNSHIFT, RUNWIN;
extern int MULTIVALUE, NOHIDE, NOCLAMP, NOCACHE, NOCACHEW, NODELAY, NOFUND, NOAPPEND, NORANK, NOECONOMY, NOPROFILE, NOCALC, NOLEARN, NOUPDATE, DEBUGRULES, DEBUGRUN, DEBUGLOAD, TSIGMODE, SYMMODE;
extern int EPSUPDATE, NOWAIT, SPREAD, NODTH;
extern char *RANKGROUP, *TESTGROUP;
extern double TODAYDATE;
extern char *OHLCHEADER;



#define MINYEAR 1950
#define MAXYEAR 2050
#define IsValidYear(y) ((y)>=MINYEAR && (y)<=MAXYEAR)
#define IsValidMonth(m) ((m)>=1 && (m)<=12)
#define IsValidDay(d) ((d)>=1 && (d)<=31)

#define PMAX 100
#define PNMAX 100
#define PEND -6666
#define PENDMAX -6665


typedef double tparam;
typedef tparam* tparams;
typedef CArrayList <tparam> CParamArrayList;

class ParamSearch {
private:
	char COL;
	int icol, ncol;
	CString basesearch, search ; 

public:
	ParamSearch(const char *name);
	BOOL IsMatch(const char *name);
};

// Invalids

#define EPSILON (1e-6)
#undef INFINITY			// FF - warning C4005: 'INFINITY': macro redefinition - in 'math.h'
#define INFINITY ((float)1e30)
#define InvalidNUM ((float)-3.3e33)
#define InvalidDATE 0
#define InvalidSTR ""

#define IsIllegalNUM(x) (x!=InvalidNUM && fabs(x)>1e20)  // numbers should never be this big

class StdDev 
{
	public:
	double n, acc, acc2;

	void Reset() { n = acc = acc2 = 0; }
	StdDev() { Reset(); }
	StdDev(double _n, double _acc, double _acc2);
	StdDev& operator+=(StdDev &std);

	int Cmp(StdDev &std);

	inline void Acc(register double v)
		{
		ASSERT(v!=InvalidNUM);
		if (v==InvalidNUM)
			return;
		++n;
		acc += v; 
		acc2 += v*v;
		}

	inline void Acc(register double nv, register double v)
		{
		ASSERT(nv!=InvalidNUM || v!=InvalidNUM);
		if (nv==InvalidNUM || v==InvalidNUM)
			return;
		n += nv; 
		acc += v*nv; 
		acc2 += v*v*nv;
		}

	double Num() { return n; };
	double Avg(double realn = -1);
	double Std(double realn = -1);
	double Shp(double realn = -1);

};

// General

CString LatestDay(double date, const char *folder = NULL);


class CDATA { // 

public:
	char data[30]; // enough for Date/Num conversion
	static BOOL CompareNUM(double nd, double od, double erreps = 1e-3);

	static double GetNum(char const *buffer, double def = InvalidNUM);
	static double GetDate(const char *value);	
	
	static const char *GetMonth(int m);
	static int GetMonth(const char *m);
	static double GetDate(const char *value, const char *fmt); // fmt = M/D/Y  Y-M-D YYYYMMMDD DDMMMYY


	static double GetNumDbg(char const *buffer, const char *file, int line);
	static double GetDateDbg(const char *value, const char *file, int line);

	static const char *SetString(CString &str, int *count = NULL);
	static const char *SetNum(char *data, double d, int *count = NULL);
	static const char *SetDate(char *data, double d, int *count = NULL);

	operator const TCHAR*()const { return data; }
  
	// Define conversion to type char *.
	//void set(const char *str) { this->Insert(0,str); }

	CDATA(void) { data[0]=0; }
	//CDATA(double d) { SetNum(d); } 
	const char *SetNum(double d, int *count = NULL) 
		{return SetNum(data, d, count); }
	const char *SetDate(double d, int *count = NULL) 
		{return SetDate(data, d, count); }
};

#define CGetDate(str)  CDATA::GetDateDbg(str, __FILE__, __LINE__)
#define CGetNum	CDATA::GetNum

class CDate : public CString {
	public: 
		CDate(double d) { CDATA data; *this += data.SetDate(d);  }
};

class CData : public CString {
	public: 
		CData(double d) { CDATA data; *this += data.SetNum(d);  }
};


BOOL DumpHTML( char *memory, const char *name, BOOL message = TRUE);

// Parse
class CSymList;
class CTickerList;

typedef struct DownloadMapTag DownloadMap;

typedef struct {
	int version;
	int type;
	int num;
	const char *name;
	const char *id;
	void *transfunc;
	int dcol, drow;
	void *valfunc;
	const char *idline;
	// autoload
	BOOL ok, dump;
	CString dumpstr;
	void *data;
	int *count;
	int *countsame;
	double valsame;
	const char *sym;
	CDATA cdata;

	void dumplog(const char *str)
	{
		dump = TRUE;
		dumpstr += "; ";
		dumpstr += str;
		
	}

} TickMap;

struct ErrMap {
	int ok, download, invalid, dump;
	int retry;
	int status;

	void reset(void)
	{
		retry = 0;
		status = 0;
		ok = download = invalid = dump = 0;
	}

	BOOL adddump(int max)
	{
		if (retry>0 || dump>max)
			return FALSE;
		++dump;
		return TRUE;
	}

	BOOL adddownload()
	{
		if (retry>0) 
			return FALSE;
		++download;
		return TRUE;
	}

	BOOL addinvalid()
	{
		if (retry>0) 
			return FALSE;
		++invalid;
		return TRUE;
	}

	BOOL addok()
	{
		++ok;
		return TRUE;
	}

	ErrMap(void)
	{
		reset();
	}
};


typedef struct DownloadMapTag {
	// config
	int dth;
	int days;
	int num, start, end;
	int tlist;
	TickMap *map;
	const char *group;
	const char *name;
	void *symfunc, *symfunc2;
	char const *url;

	// special
	char *urlsymbol, *urlsymbol2;
	int symperpage;
	char *countsymbol;
	//int countdiv, countinc, countoffset;
	char *rules[20];

	// dynamic
	CTickerList *ticklist;
	CSymList *outlist;
	int numticks;
	char const **listticks;
	ErrMap err;
	int progressticks;
	BOOL thdelay;
	int dthcount;
	int hitticks;
	double hittime;
	int maxticks;
	CString lastsym;

	void Reset(void)
	{
		err.reset();
		progressticks = 0;
	}
} DownloadMap;




// Load Save

int ServerToFile(const char *url, const char *file);
int DownloadToFile(const char *file, const char *url);

typedef int CLineFunc(CString &line, CString &header); // FALSE = ignore line

class Idx {
public:
	unsigned date; // int but same as double
	int seekpos, seeklen; // position and length of line

	Idx()
	{
	date = seekpos = seeklen = 0;
	}

	inline int seekend()
	{
	return seekpos + seeklen;
	}
};

typedef CArrayList <Idx> CIdxArrayList;

enum { IDXREAD=CFILE::MREAD, IDXUPDATE=CFILE::MUPDATE, IDXWRITE=CFILE::MWRITE };

class CIdxList : public CIdxArrayList {

	// Mode
	CIdxList *idg;
	BOOL idgmode, grpmode;
	int crccol;

	// Find/Add
	unsigned lastid, seekid;
	int idxskip, idxsize;
	int seekpos, seeksize;

	inline Idx &Head() { return operator[](0); }

	// Open/Close/Put/Seek
	CFILE of, ofi;
	int omode;
	CString ofilename;
	int seekposw, skipposw;

	BOOL Find(CFILE &f, unsigned date, int size = -1);

public:
	int idxpos;

	CIdxList(void);
	~CIdxList(void);

	int InternalCheck(void);
	BOOL IntegrityCheck(const char *filename);

	static unsigned CRC(CString &id);
	static unsigned DATE(const char *dateid);
	static CString Filename(const char *filename, BOOL idgmode = FALSE);

	// loaded
	void SetID(int col, BOOL grouplines) { crccol = col; grpmode = grouplines; }
	void Sort(void);
	int Add(int pos, const char *line);

	CIdxList &operator=(CIdxList &src);
	BOOL Load(const char *filename, int lastlines = -1);
	BOOL Save(CFILE &f); // force lock with source file
	BOOL Rebuild(const char *filename, BOOL save = TRUE);

	BOOL Find(const char *filename, double date, BOOL loadidx = FALSE); // index search (optional load in memory)
	BOOL FindCRC(const char *filename, unsigned CRC, unsigned date);	// memory+index search
	BOOL Link(CIdxList &idg);

	// file
	void SeekPos();
	BOOL SeekHeader(CString &header);
	BOOL FixHeader(CString &oldheader, CString &newheader);

	BOOL Open(const char *filename, int mode, CIdxList *idg = NULL);  // double index
	BOOL PutHeader(const char *header);
	BOOL PutLine(const char *line, BOOL skip = FALSE);
	int ReadLines(char *data=NULL); 
	void Close(void);
	void Flush(void);
	char *GetHeader(void);
	char *GetLine(void);
	CFILE &File() {return of; }

	void Reset();
};



class CLineList {

private:
	BOOL sorted;
	BOOL idxcol;
	CArrayList <const char *> lines;
	CStringArrayList strings;
	//CString **lines;
	//int numlines;
	//int maxlines;
	CString header;
	CIdxList idx;

public:

	inline int GetSize() { return lines.GetSize(); }
	inline const char *operator[](int i) { return lines[i]; }
	CString &Header() { return header; };
	const char *Head() { return lines[0]; };
	const char *Tail() { return lines[GetSize()-1]; };

	void Sort(int col = 0, int order = 1);

	void Empty();
	BOOL IsEmpty() { return GetSize()==0; }
	void DeleteLine(int n);
	int Purge(int toksync, int start = 0);
	int Find(const char *id);
	CLineList(const char *sheader = NULL);
	~CLineList();

	int Add(CString &str); 
	int Add(const char *line, BOOL copystr = TRUE); 
	const char *Set(int set, const char *line);

	int Merge(int set, const char *&newline, const char *&oldline);
	int Merge(CLineList &newlines, int toksync, BOOL purgeE = FALSE);
	int Add(CLineList &newlines, int toksync = 0, BOOL purgeE = FALSE);
	//BOOL LoadXMAP(const char *memory, TickMap *xM, const char *nextline, const char *sym);
	BOOL IntegrityCheck(const char *file);
	int Check(int i, int toksync = 0);

	BOOL Load(const char *filename, BOOL sort = TRUE);
	BOOL Save(const char *filename, BOOL sort = TRUE);

	BOOL LoadIdx(const char *filename, double date = 0);
	BOOL SaveIdx(const char *filename, int skiplines = 0); 
	BOOL UpdateIdx(const char *filename, int toksync = 0, BOOL purgeE = FALSE);
};

// Index

typedef struct {
	int flushed;
	char date[DATELEN+5];
	CLineList *lines;
	} tindexdates;

class IndexContext {
private:
	CString group;
	int flushticks;
	int nindexdates, maxindexdates;
	tindexdates *indexdates;
	CString indexpath, headers; 
	BOOL Flush(int n);
	int AddDate(const char *date, const char *firstline);
	void Sort();

public:
	IndexContext(const char *path);
	~IndexContext(void);
	BOOL Add(const char *date, const char *line, BOOL hdr = FALSE);
	BOOL AddSwap(const char *date, const char *line, BOOL hdr = FALSE);
	BOOL Flush();
	BOOL AutoFlush();
	void GetList(CLineList &list);
	void Reset(void);
	void ResetFiles();
	CString Group() {return group; };
	CString Headers() { return headers; };
	int Find(const char *filename);
	int GetSize() { return nindexdates; }
	CLineList *Lines(int n);
	CString FilePath(const char *id);
	BOOL Load(const char *date);
};


// Sym

BOOL IsNumeric(const char *str);

class CSymID {
public:
	static BOOL IsIndex(const char *symbol);
	static BOOL IsFX(const char *symbol);
	static BOOL IsStock(const char *symbol);
	static CString Filename(const char *sym);
	static BOOL Validate(CString &sym);
	static BOOL Migrate(CString &str, CString &cmp);
	static int Compare(const char *str, const char *line, int token = 0); 
};

class CSym
{
	public:
	int n;
	double index; // for ranking
	CString id;
	CString data;
	
	public:
	
	CSym(CSym &sym);
	CSym(const char *id, const char *data);
	CSym(const char *id = NULL, int num = 0);
	~CSym();
	void Clear(const char *str = NULL);
	CSym &operator=(CSym &sym);
	double GetNum(int i);
	double GetDate(int i);
	CString GetStr(int i);
	void MulNum(int i, double num);
	void AddNum(int i, double num);
	void SetNum(int i, double num);
	void SetNum2(int i, double num, int decimals = 2);
	void SetMultiStr(int i, const char *str, int num = -1);
	void SetStr(int i, const char *str);
	void SetDate(int i, double num);
	BOOL IsEmpty(void);
	void Empty(void);
	int Merge( CSym &sym);
	void Insert(int i, CSym &sym);
	CString Line() { return id+","+data; }
	int Load(const char *line);
	int Load(const char *_id, const char *_data);
	BOOL Valid(void);
	CString Save(void);
	BOOL Save(CString symfile);

	int Compare(CSym &sym);
};


class CSymList 
{
	private:
		BOOL isorted;
		BOOL sorted;
		int scount, scountmax;
		CSym **slist;
		int maxcount;
		void Init(int maxcount);

	protected:
		qfunc *cmpfunc;

	public:
		CString header;
		//int check; // for external checks
		//int *count;

		CSymList(int _maxcount = 0);
		CSymList(CSymList &list);
		~CSymList();
		void Empty();
		int GetSize(void) { return scount; }
		CSymList &operator=(CSymList &symlist);

		CSym &operator[](int n) { return *slist[n]; }
		virtual BOOL Add(CSym &newsym);
		BOOL Add(CSymList &list);
		BOOL AddUnique(CSym &newsym);
		BOOL AddUnique(CSymList &list);
		BOOL Delete(const CSym &newsym);
		BOOL Delete(int pos);
		// my.csv or AAPL+GOOG+IBM+...  autodetect
		int Load(const char *sfile, BOOL alreadyunique = TRUE, BOOL commaparsing = TRUE);
		int MaxSize(void);
		int Save(const char *sfile, BOOL withdata = TRUE, BOOL hash = FALSE, BOOL append = FALSE);
		void SortIndex(void);
		void SortNum(int field=-1, int order = 1);		
		void iSort(); // case insensitive Sort and Find afterwords
		void Sort();		
		void ReSort(qfunc *func = NULL) { sorted = FALSE; func ? Sort(func) : Sort(); };		
		void Sort(qfunc *func);	
		int Find(const char *str, int start = 0);
		int FindColumn(int col, const char *str, int start = 0);

		static int cmpindex( const void *arg1, const void *arg2 );
		void Rank(void);
		int VerifySort(int i);
};



class CTickerList : public CSymList {
	public:
	CTickerList(int _maxcount = 100);
	BOOL Add(CSym &newsym);
	BOOL Add(CTickerList &newlist) { return CSymList::Add(newlist); }
};


int FindTokenNum(const char *id, const char *hdr);
int CreateSectionList(const char *line, CStringArrayList &headers);
int FindSectionList(const char *cmp, CStringArrayList &headers);


class CIdfList;

class tfilter { 
public:
	int n; 
	int o;
	int valid;
	CString id; 
	CString strval, strop;
	int val;
	int type;
	bool error;
	CDoubleArrayList vallist;
	CIdfList *idf;
	tfilter(void)
	{
		n = -1;
		type = -1;
		idf = NULL;
		error = false;
	}
};


class FilterContext {
	int num;
	tfilter *filter;
	CIntArrayList orlist;

	// idf
	CFILE f, fi;
	CIdxList idx;
	BOOL idxmode;
	int idxpos, idxseekpos;
	char *buffer;
	int buffermaxsize;

public:
	FilterContext(const char *filterstr = NULL);
	~FilterContext(void);
	void Set(const char *filterstr);
	int Num() { return num; }
	CString Filter(); // with error correction

	// mem based filtering
	BOOL Match(const char *line, BOOL headers = FALSE, BOOL idfbuilding = FALSE);

	// file based filtering (autoindexing)
	BOOL Open(const char *filename, int mode);
	const char *GetHeader(void);
	const char *GetLine(void);
	BOOL PutHeader(const char *line);
	BOOL PutLine(const char *line);
	void Close(void);

	int GetValues(int n, double *values); // extract filter parameters for coded filter
	CString GetRules(const char *var); // foe easy rule macro writing
};

class QueryContext : public FilterContext {
	int maxcol;
	CIntArrayList transout;
	CIntArrayList transidx;
	CIntArrayList translen;
	const char *transmap;
	
	int bufferlen;
	int bufferlenmax;
	char *buffer;

	void add(const char *str, int len);

public:
	QueryContext(const char *transformcfg = NULL, const char *filtercfg = NULL, int timeout = 0);	
	~QueryContext(void);
	void Set(const char *transformcfg, const char *filtercfg, int timeout = 0);
	char *Transform(const char *line, BOOL headers = FALSE, const char *A0 = NULL);
	const char *GetHeader(const char *A0 = NULL) { return Transform(FilterContext::GetHeader(), TRUE, A0); }
	const char *GetLine() { return Transform(FilterContext::GetLine()); }
};




class DayContext : public IndexContext , FilterContext {
public:
	DayContext(const char *path, const char *filter);
	~DayContext(void);
	BOOL AddDay(const char *date, const char *line, BOOL hdr = FALSE);
	BOOL AddHist(const char *date, const char *line, BOOL hdr = FALSE);
};


class RankContext : public IndexContext {

	void Reset();

	public:
	RankContext(const char *path);
	~RankContext(void);
	// second pass
	BOOL UpdateLine(char *line, const char *rankline);
	BOOL ComputeRank(const char *id);
};



/*
class HISTORYCOLUMN {
public:
	//int maxdata;
	CDoubleArrayList data; //Y
};
*/

typedef CDoubleArrayList HISTORYCOLUMN;

class ruleitem;
class HISTORY;
class HISTORYDATA;
class LoadProcess;

class HISTORYDATA {
friend class HISTORY;
private:
	HISTORYCOLUMN rf; //returns
	HISTORYCOLUMN date;
	HISTORYCOLUMN **column;
	HISTORYCOLUMN **mapcolumn; //X
	CIntArrayList maprows, maxrows;
	int maxcolumn, maxmapcolumn, maxrow;
	int nline;
	//int link;
	//CIntArrayList strlast;
	CStringArrayList strlist;
	LoadProcess *lp;
public:
	HISTORYDATA();
	~HISTORYDATA();
	void SetSize(int rows, int columns, CIntArrayList &maplist, CIntArrayList &rowslist);
	void Reset(void);
	//void Reset(int col, int value, int size);
};


class HISTORY {
	int Ystart;
	//int rowend;
	HISTORYDATA *hdata;
public:

	HISTORY(HISTORYDATA *_hdata = NULL, int _Ystart = 0)
	{
		Ystart = _Ystart;
		hdata = _hdata;
	}

	~HISTORY(void)
	{
	}

	inline double &date(void)
	{
		return hdata->date[Ystart];
	}

	inline double &rf(void)
	{
		return hdata->rf[Ystart];
	}

	inline double &data(int i)
	{
		ASSERT(i>=0 && i<hdata->maxcolumn);
		ASSERT(Ystart<hdata->column[i]->GetSize());
		return (*hdata->column[i])[Ystart];
	}

	inline double *cdata(int i, int len)
	{
		ASSERT(i>=0 && i<hdata->maxcolumn);
		ASSERT(Ystart+len-1<hdata->column[i]->GetSize());
		return &((*hdata->column[i])[Ystart]);
	}
	
	inline double *cdate()
	{
		return &hdata->date[Ystart];
	}

	inline HISTORY operator[](int Y) 
	{ 
		return HISTORY(hdata, Ystart+Y);
	}

	
	inline HISTORY& operator++(void) 
	{ 
		++Ystart;
		return *this;
	}
	

	inline HISTORY operator+(int Y) 
	{ 
		return HISTORY(hdata, Ystart+Y);
	}

	inline BOOL InRange(int Y, int col)
	{
		return Ystart+Y>=0 && Ystart+Y<hdata->maxrows[col];
	}

	inline int GetDateSize()
	{
		return hdata->nline;
	}

	inline int SetDateSize(int ncount)
	{
		hdata->nline = ncount;
		return ncount;
	}

	void StartupLoadProcess(void);
	int ShutdownLoadProcess(void);
	LoadProcess *LoadProc(void) { return !hdata ? NULL : hdata->lp; }

	void Malloc(int maxcount, int maxdata, CIntArrayList &maplist, CIntArrayList &rowlist, const char *group);
	void Free(void);
	void Reset(void);
	BOOL IsNull();

	void ResetDateStart(void);
	int	GetDateSizeMax(void);
	int SortDate(int ncount, ruleitem *rule);
	int	CalcDateSize(void);
	BOOL IntegrityCheck(const char *str);

	CString GetString(int i);
	void SetString(int i, const char *str);

	int FindAprox(double date);
	//int FindN(double date);
	int FindData(double data, int i);

	void Expand(int &days);
	void Fill(int count, double date, int step, int maxm = 5);
	void Extend(int count, int start, int end);
	//void Event(ruleitem *rule, double date, LoadProcess *c = NULL);
};


class LoadProcess;
int EventSMAX(ruleitem *rule, int smax);
void Event(HISTORY hist, ruleitem *rule, double date, LoadProcess *c = NULL);
void MAKEV(int signal, HISTORY hist, int days, int result, BOOL zeroinv = FALSE);

CString FilePath(CString file, CString path = "", BOOL mkdir = TRUE);
BOOL GetFileList(const char *path, CStringArrayList &rankfiles, BOOL sorted = FALSE);

BOOL SetDAT(const char *file);
BOOL WriteDAT(const char *file);

extern "C" {
int zssmain(int argc, char *argv[]); 
int zssencode(FILE *_infile, FILE *_outfile, int flags = 0);
int zssdecode(FILE *_infile, FILE *_outfile, int *flags = NULL);
int zssdecodemem(char *_inbuffer, int _lenbuffer, FILE *_outfile, int *flags = NULL);
}

int DownloadData( DownloadFile* dfile, const char *filename, const char *url);
DownloadMap *GetDownloadMap(DownloadMap dmap[], TickMap *map);
DownloadMap *GetGroupInfo(const char *group, CString &header, int *toksync = NULL, int *disc = NULL);

CString GetMode(void);

void DownloadUrls(CString filename);

const char *StatPath(const char *outfile);
unsigned int CRC(const char *file, CString *last = NULL);
unsigned int CalculateCRC( const char *buffer, unsigned int count, unsigned int crc = 0 );

// Rules

enum { R_HOLD = 0, R_COMBO, R_VAR, R_LBUY, R_LSELL, R_SBUY, R_SSELL, RMAX };

#define RHOLD ((1<<R_HOLD))
//#define RCOMBO ((1<<R_COMBO))
#define RSIGVAR ((1<<R_VAR))
#define RSIGLBUY ((1<<R_LBUY))
#define RSIGSBUY ((1<<R_SBUY))
#define RSIGLSELL ((1<<R_LSELL))
#define RSIGSSELL ((1<<R_SSELL))
#define RSIG (RSIGLBUY|RSIGLSELL|RSIGSBUY|RSIGSSELL)
#define RCHECK(x,flags) (((x)&(flags))!=0)
BOOL RTRUE(double v);

enum {	HOUT = 0, HSHORT, HBUY, HSELL, HAUX, HAUX2, HAUX3, HAUX4, HAUX5, HAUX6, HAUX7, HAUX8, HAUX9, HAUX10, HMAX };
static char *HHEADERS[] = { "OUT", "L/S", "BUY", "SELL", "'-AUX1", "'-AUX2", "'-AUX3", "'-AUX4", "'-AUX5", "'-AUX6", "'-AUX7", "'-AUX8", "'-AUX9", "-AUX10", NULL };

typedef struct {
	char *str;
	void *data;
	int weekly;
} ruletrans;


// evaluate stats
class ruleres {
public:
	ruleres(void);
	ruleres(int b, int s, int h, int ls);
	void set(int b, int s, int h, int ls);
	BOOL equal(ruleres &res);
	void accumulater(double r, double rf);
	void accumulate(double percent, double duration);
	void combine(ruleres &res, BOOL acc = TRUE);
	void evaluate(void);
	void reset(void);
	double weight(int w);

	CString id();

public:
	// rule definition
	int bsig, ssig, hsig, mshort; 
	//tparams params;
	// transactions
	double ntrans, nwin, nloose, ndays; 
	// accumulators
	StdDev r, rf, perc, hold;

	// maximums
	double minperc, maxperc, minhold, maxhold; 
	// computed
	double avgperc, stddevperc, sharpeperc, avghold, stddevhold;
	double avgr, avgrf, yret, cyret, sharper, sharperf,  utility;
	// status
	int lastbuy;
	// name
	char name[64];
};

typedef CList <ruleres> rulereslist;

class rulename { 
public:
	rulename()
	{
		show = FALSE;
	}

	CString name;
	BOOL show;
};

class RuleData;

enum { rNUM = 0, rDATE, rSTR };

class ruledef {
public:
	ruledef::ruledef()
	{
		needed = FALSE;
		weekly = FALSE;
		multidate = FALSE;
		rank = FALSE;
		load = FALSE;
		mapping = FALSE;
		sr = nr = er = 0;
		func = NULL;
		vfunc = NULL;
		cfg = NULL;
	}

	ruledef::~ruledef()
	{
	}
	
	int Map(int date, int idate);
	int Compute(HISTORY data, int days);
	int Invalidate(HISTORY hist, int days, int signal);
	CString GetName(int subsig = 0);
	ruledef *GetRuleDef(char const *name, int &nr);

	BOOL weekly, rank, mapping;
	int load, multidate, needed;
	int sr, nr, er;
	CArray <rulename,rulename&> names;
	void *func, *vfunc;
	CStringArrayList param;
	CIntArrayList paramlink; // more than 1 linked variable per param
	CString string;
	RuleData *cfg;
} ;



class ruleitem {
public:
	ruleitem(void);
	ruleitem &operator=(ruleitem &sym);
	CString Name(BOOL showdetails = FALSE);
	void Reset(int sub = 0, ruledef *def = NULL, tparam *params = NULL);
	BOOL IsInvisible();
	BOOL IsRank();
	int Type();
	static tparam *SetParam(CParamArrayList &params);
	tparam *ExtendParam(int max);
	int MinDays(int mdays);
	int MinDays(int MINRULES, int MAXRULES, int mdays);
	void Link(ruleitem *linkr, int adddays);
	int Compute(HISTORY hist, int days, int signal);
	void Check(HISTORY hist, int days, int signal);
	void Debug(HISTORY hist, int days, int signal);
	ruleitem *Base(int *signal = NULL);
public:

	BYTE type;
	short int mindays;
	short int extradays;
	short int adddays;
	signed char vtype;
	unsigned short int subsig;
	unsigned char computed;
	ruledef *def;
	ruleres *res;
	tparam *param;
	CIntArrayList *testlist;
};

// learning rules
class CLearn {
	public:
	int LEARNPARAM; 

	virtual void Reset() = NULL;
	virtual int MinDays(ruleitem *rule, int run, int learn) = NULL;
	virtual BOOL Learn(HISTORY hist, int days) = NULL;
	virtual void Evaluate(HISTORY hist, int days, int signal) = NULL;
	virtual BOOL Load(const char *file) = NULL;
	virtual BOOL Save(const char *file) = NULL;
};

int RuleLEARN(ruleitem *rule, HISTORY data, int days, int signal, CLearn &learn);


class DayContext;
class RankContext;

typedef int cmpres(ruleres *r1, ruleres *r2);

typedef double pricefunc(int i, HISTORY data);
typedef int rulefunc(ruleitem *rule, HISTORY hist, int days, int result);
typedef int vrulefunc(HISTORY hist, int &days, int result);

class BANDDELAY {	
	public:
	BANDDELAY(double band = 0, double delay = 0);
	BOOL Invalid(BOOL excludebd = TRUE);
	BOOL Invalid(BANDDELAY &bd, BOOL excludebd = TRUE);

	double Delay(BOOL sig);
	double Greater(double v1, double v2, double scale = 0);
	double Lower(double v1, double v2, double scale = 0);

	protected:
	double b;
	double d;
	int ds;
};




typedef CIntArrayList iparams;

class CLoadMap : public CObject { 
public:
	ruledef *rule; // rule
	//const char *file; // only load from same file!!!
	CStringArrayList names; // columns to load
	iparams inames; // column mapped to RULES
	int maxdays; // maximum extra days for all columns
	POSITION filepos;	// link to CLoadFile table
	BOOL mapped;

	CLoadMap(void)
	{
	rule = NULL;
	mapped = FALSE;
	filepos = NULL;
	maxdays = 0;
	}

	void operator=(CLoadMap& Src);
};

class CLoadFile : public CObject {
public:
	CString filename;	// filename
	CString rulename;	// rulename
	int maxdays;		// maximum extra days for all maps 

	double OldestDate()
	{
		return maxdays<0 ? 0 : (PreviousDate(maxdays) - EXTRAQUOTEDAYS); // 10d extra for safety
	}

	CLoadFile(const char *file = "", const char *rule = "", int n = 0)
	{
	filename = file;
	rulename = rule;
	maxdays = n;
	}

	void operator=(CLoadFile& Src)
	{
		filename = Src.filename;
		rulename = Src.rulename;
		maxdays = Src.maxdays;
	}
};

typedef CList <CLoadFile,CLoadFile&> _CLoadFileList;

class CLoadFileList : public _CLoadFileList {
public:
	POSITION Find(const char *filename);
	POSITION Add(const char *filename, const char *rulename, int maxdays);
	int Summary(const char *title = NULL);
};


class CLineToken;
class CLoadMapList;
class LoadProcess {
	public:

	// loadmap
	//HISTORY hist;
	CLoadMap *map;
	CLineToken *line;
	CString sym, group;
	double lowdate, highdate;
	int lowdatecnt;

	// locals
	int ncount;
	int mapref;
	//CString val, lastval;
	//tgetnum *getnum;

	LoadProcess(void);
	~LoadProcess(void);

	// map
	void MapValue(ruledef *def, HISTORY hist, int jii, int xii);
	int Map(ruleitem *rule, HISTORY hist, BOOL evtmap = FALSE);

	// file
	BOOL getline(double &date);
	BOOL open(const char *symbol, const char *fname, double mindate);
	void close(void);

	// line
	int find(const char *cmp);
	BOOL getvalid(int n, int *_start = NULL, int *_len = NULL);
	CString getstring(int n);
	double getdate(int n = 0);
	double getnum(int n);
	void setnum(int n, double val);
	void setstring(int n, const char *str);
	const char *buffer();

	int LoadHistoryMap(HISTORY &hist, const char *sym, POSITION filepos, CLoadMapList &maplist, CLoadFile &f);
};


typedef double tparamarray[PMAX+1][PNMAX+1];
int ReportLoadFiles(CLoadFileList &filelist, const char *title = "");

#define SIGENUM static enum { SLB = 0, SLS, SSB, SSS, SMAX };

BOOL SetConfigHeader(const char *grp, const char *header);

class RuleData	{
public:
	int MAXRULES;
	int MINHRULES;
	int INVRULES;
	int RANKRULES;
	int MAXHRULES;
	int MINRULES;
	ruleitem *RULES;
	int NDEFRULES;
	ruledef *DEFRULES;
	int MINSIGRULES, MAXSIGRULES;
	int MINSIMRULES, MAXSIMRULES;
	int DOMULTIVALUE;
	const char *sym;

	int symcount;
	int process;
	int rankmode;
	rulereslist nlist;

	// loading caches
	CLoadFileList filelist;
	CLoadMapList *maplist;

	HISTORY hist;
	DayContext *dayctx;
	RankContext *rankctx;
	CString group;


	RuleData(BOOL rank = FALSE, const char *dayfilter = NULL, int process = -1);
	virtual ~RuleData();

	int LoadRules(const char *name, const char *RULECFG, BOOL log = FALSE);
	void UnloadRules(void);

	void StartupRules(int numdays, int extradays, CLoadFileList *list, BOOL epsdiv);
	void ShutdownRules(void);

	int LoadHistory(const char *sym);
	int SaveHistory(const char *sym, const char *group, int maxcount, BOOL overwrite); 
	int ReSaveHistory(const char *sym, const char *group, int maxcount); 
	//int ReSaveHistory(const char *date, const char *group, CSymList &ticks);
	CString SaveHistoryHeader(void);
	int ExtendHistory(int maxcount);

	CString ComputeInverseLinks(int n);
	int ComputeExtraDays(int numdays);
	int ComputeVariables(HISTORY data, int days);
	void DebugReset(HISTORY data, int days, int n, double val);

	virtual BOOL RunRule(const char *sym, int maxhist);
	virtual BOOL RunSimulation( const char *sym, int maxhist);
	virtual BOOL RunProcess( const char *sym, int maxhist);
	virtual void RunFinish( void );

	void RunReport( rulereslist &slist );
	BOOL SaveResList(rulereslist *rlist, const char *rulefile, const char *append = NULL, const char *extra = NULL);


	void Flush() { if (dayctx) dayctx->Flush(); }
	void ComputeRank(CSymList &symlist, const char *group);
	BOOL UpdateRankFile(const char *filename, BOOL isrankfile = FALSE);

	void ProcessGroupFiles(CSymList &symlist, CStringArrayList &loadmap, const char *group);
};

// Config
enum { CFGRANK = 0, CFGRANK2, CFGNULL, CFGECONOMY, CFGDIV, CFGEPS, CFGHIST, CFGSIG, CFGTEST, CFGDAY, CFGSIM, CFGTEST2, CFGSIM2  };

extern ruletrans CALCRULES[], MAPRULES[];

class RuleConfig {
	int cfg;
	BOOL overwrite;
	BOOL epsdiv;
	CString rules;
	CString group;
	int numdays, extradays, moredays; 
	RuleData *ctx;
	CTickerList ticks;

public:
	RuleConfig(int vcfg, int maxdays, CTickerList &ticks, int &extradays, CLoadFileList *loadmap = NULL, CLoadFileList *savemap = NULL, const char *filter = NULL);
	~RuleConfig(void);
	BOOL Skip(const char *sym);
	BOOL Process(const char *symbol);
	int Load(const char *sym, BOOL extend = FALSE);
	void Flush(void);

};




CString DataFile(const char *group, const char *sym, BOOL mkdir = TRUE);
CString DataFileCore(CString &group, CString &sym, BOOL mkdir = TRUE);

CString OptionFile(const char *date, const char *symbol, const char *ext);


double GetHistDays(const char *value);
int mindays(int mindays, ruleitem *r);


int CMPTEST(int signal, HISTORY data);



// Ticks


class TickInfo {
	public:
	DownloadMap *dmap;

	int xnumptr;
	int xstrptr;
	int xdateptr;
	int xcountptr;

	// DATA
	double *XNUM;
	CString *XSTR;
	double *XDATE;
	int *XCOUNT;
	int *XCOUNTSAME;

	TickInfo();
	~TickInfo();
	void reset(void);
	void startup(void);
	void shutdown(void);
};

int ProcessXMAP(TickMap &xM, char *buffer, char *label, const char *symbol);
BOOL ProcessItem(TickMap &xMapItem, const char *value, int j = 0, void *transfunc = NULL);


// Summary Window


class SummaryWindow {
public:
	int ys10;
	HISTORY hist;
	int hmax;
	int unit;
	double tmax;
	CString period;
	CProgress progress;

public:
	SummaryWindow(HISTORY histdata, int maxhist);
	CString GetPeriod(HISTORY h, int n);
	static int CalcWindow(int simw);
	BOOL FindYear(HISTORY hist, int y, int &i);
	BOOL History(HISTORY &outhist, int &outnhist);

};


// Portfolio

class sigstat {
public:
	int sig;
	double n, avg, std, val; // computed
	double ntot;
	StdDev all, plus, minus;

public:
	sigstat(void);
	void accumulate(double val);
	void compute();
	CString string(void);
	static CString header(void);
};

class portfolio {
public:
	double num;
	double avg, std, lambda;
	CArray <double,double> percent;
	sigstat stat;

	portfolio(void);
	portfolio(portfolio &a, double perc, portfolio &b);
	void operator=(const portfolio& p);
	CString string(void);
	CString header();
};

typedef CList <portfolio,portfolio&> portfoliolist;


#define SYMQUOTEMAXURL 500 // max number of symultaneus syms in yahoo quotes
#define QHEADER "SYM,pCLOSE,OPEN,HIGH,LOW,CLOSE,LAST,BID,ASK"
enum { QPCLOSE = 0, QOPEN, QHIGH, QLOW, QCLOSE, QLAST, QBID, QASK, QMAX };
int GetQuotes(CSymList &list);

class CDisk;
class CDiskCache {
public:
	CDiskCache(void);
	~CDiskCache(void);
	void Reset(CLoadFileList &loadmap, CLoadFileList &savemap);
	void Prefetch(const char *sym, int symnum);
	void Flush(void);
	void Usage(void);
};

