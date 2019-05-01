#include "stdafx.h"
#include "atlstr.h"
#include "msxml.h"
#include "math.h"
#include "TradeMaster.h"

#define IGNOREMSUPDN	// ignore msupn because already in  BRUpDn

#define IDNULL "0"

// use -debugload [1-3] to debug cache behaviour

//#define USESQL		// SQL Database instead of CSV files
#define USECACHE		// dual thread read/write caching
#define USECACHEW		// dual thread read/write caching ALSO for output
#define USEREADATONCE	// cache files by reading all lines at once
//#define USEDELAYEDWRITE2	// move updating of output file to READ thread
//#define USELOADPATCH	// patch data during load (better not! do it some other time)
#define SKIPNOHIST		// skipping symbols with no history
#define SKIPEMPTYFILE	// skipping empty files for faster caching
#define SKIPISORTED	// skip fully loading and processing isorted files (EPS/DIV/HIST/RANK)
#define AVOIDFUTURE		// avoid presenting future EPS/DIV
#define MAXSORTSIZE		50*(1024*1024)	// 50Mb max sort size in memory

//#define DEBUGMAXCOLUMNS 10	// debug max number of columns
//#define DEBUGEVTFILESPECIAL	// @ files are fully loaded (tried)
//#define DEBUGCACHE2		// debug CACHE and non CACHE match
//#define DEBUGSEEKID		// done during DEBUGLOAD
//#define DEBUGTOKEN			// debug line token indexing (in case of inexpected change of data)
//#define DEBUGLINE			// debug line traversing code

#define MAXCACHESYM 3		// max "read ahead" symbols
#define MAXCACHEFILES (MAXCACHESYM*100)	// max cached file table 
#define INDEXBLOCK	500	// a bunch
#define SEEKBLOCK   500 // 1year
#define CIDXBLOCK	500
#define LINELENBUFF	FILEBUFFLEN
#define LINELENBLOCK (10*LINELENBUFF)
#define TOKENBLOCK	500

#define FLUSHTIME	120		// flush out every 2 minutes
#define RANKQSORT			// use qsort for rank buffer
#define RANKMARK "[##]"  // 0.XX
#define DORANK

#define GROUPSYM ':'

double TODAYDATE = 0;
char *TEMPFOLDER = "$#TMP";

//inline BOOL IsInverseSortedFile(const char *group) { return group[0]=='$'; }
inline BOOL IsEcoFile(const char *group) { return strcmp(group, "$ECO")==0; }

#ifdef DEBUG
#undef FLUSHTIME 
#define FLUSHTIME 10
#endif

void CLoadMap::operator=(CLoadMap& Src) 
{ 
		rule = Src.rule;
		filepos = Src.filepos;

		int n = Src.names.GetSize();
		names.Reset(n);
		for (int i=0; i<n; ++i)
			names[i] = Src.names[i];

		n = Src.inames.GetSize();
		inames.SetSize( n );
		for (int i=0; i<n; ++i)
			inames[i] = Src.inames[i];
} ;       




CString DataDirectory(const char *sym)
{
	char *DATAFOLDER = STATDATAPATH"\\%s";
	return MkString(DATAFOLDER, sym);
}



CString DataFileCore(CString &group, CString &sym, BOOL mkdir)
{
	if (strchr(group,GROUPSYM))
		{
		// resolve BRECONOMY:FOMC_Meetings
		sym = GetToken(group,1,GROUPSYM);
		group = GetToken(group,0,GROUPSYM);
		}

	char *DATAFILE = "%s.csv";
	char *STATFOLDER = STATPATH"%s";
	//if (*sym=='$') // special symbol \STAT\HIST\sym
	//	++sym, group="$HIST";
	if (group[0]=='$') // special folder \STAT\group
		{
		if (IsEcoFile(group)) sym = "^";
		return FilePath( MkString(DATAFILE, sym), MkString(STATFOLDER, (const char *)group+1), mkdir);
		}
	if (group[0]=='#')
		return FilePath( MkString(DATAFILE, group), DataDirectory("#"), mkdir);
	if (group[0]=='^' || strchr(group, '='))
		return FilePath( MkString(DATAFILE, group), DataDirectory("^"), mkdir);
	if (sym[0]=='^' || strchr(sym, '=') )
		{
		const char *hist[] = { "YH$", "FX$", NULL };
		if (IsMatch(group, hist)) // only history data is valid
			return FilePath( MkString(DATAFILE, sym), DataDirectory("^"), mkdir);
		else
			return "";
		}
	return FilePath( MkString(DATAFILE, group), DataDirectory(sym), mkdir);
}

CString DataFile(const char *ogroup, const char *osym, BOOL mkdir) 
{
	CString sym = osym; 
	CString group = ogroup;
	return DataFileCore(group, sym, mkdir);
}

CString OptionFile(const char *date, const char *symbol, const char *ext)
{
	CString path = STATDATAPLPATH;
	return FilePath( MkString("%s%s.csv", symbol, ext), path=MkString("%s\\%s", path, date), TRUE );
}




// =============================================================================
// ============================= PATCH MODE =================================
// =============================================================================
// avoid some weird data in yh server that happened several years ago... maybe not needed now

#define MAXTOKLIST 10

struct {
	char *file;
	char *header;
} PatchMAP[] = {
	// patch mode
	{ "XXXXXXXXXX", "XXXXXXXXXXXXXXXXXXXXX" },
	{ "YHE", "NEXT EPS DATE,EPS EST: CurQ, 1Q Next, CurY, 1Y Next,EPS EST HIGH: CurQ, 1Q Next, CurY, 1Y Next,EPS EST LOW: CurQ, 1Q Next, CurY, 1Y Next,EPS EST NUM: CurQ, 1Q Next, CurY, 1Y Next,EPS HIST: CurQ, 1Q Next, CurY, 1Y Next,EPS UP 1M: CurQ, 1Q Next,EPS DN 1M: CurQ, 1Q Next,EPS EST 1W AGO: CurQ, 1Q Next, CurY, 1Y Next,EPS EST 1M AGO: CurQ, 1Q Next, CurY, 1Y Next,EPS HIST REAL: 4Q Ago, 3Q Ago, 2Q Ago, 1Q Ago,EPS HIST EST: 4Q Ago, 3Q Ago, 2Q Ago, 1Q Ago,EPS HIST SURPRISE%: 4Q Ago, 3Q Ago, 2Q Ago, 1Q Ago" },
	{ "YHK", "BETA,VALUE,PEG 5Y,EPS GROWTH%: CurQ,REV GROWTH%: CurQ,SHORT INTEREST,SHORT RATIO%,SHORT FLOAT%,LASTM SHORT INTEREST,INSIDER %,INSTITUTION %" },
	{ "YHO", "1Y PRICE: Avg,Median,High,Low,Num" },
	{ }
};


class PatchMode {
BOOL patchmode;
int toknum;
int toklist[MAXTOKLIST][2];
CStringList toklines;

public:
	void set(const char *group, const char *header)
	{
		// patch mode
		toknum = 0;
		toklines.RemoveAll();
		patchmode = FALSE;
		for (int i=0; PatchMAP[i].file!=NULL; ++i)
			if (strcmp(group,PatchMAP[i].file)==0)
				patchmode = i;

		if (patchmode)
			patchheader(header);
	}

	BOOL check(const char *line) // if TRUE, ignore line
	{
		if (!patchmode)
			return FALSE;

		CString pline;
		for (int i=0; i<toknum; ++i)
			{
			int start = GetTokenPos(line, toklist[i][0]);
			int end = GetTokenPos(line, toklist[i][1]+1);
			pline += CString(line+start, end-start);
			}
		pline.TrimRight(',');

		if (!toklines.IsEmpty())
			{
			// if same as last valid line, OK!
		    POSITION pos = toklines.GetTailPosition();
			if (toklines.GetPrev(pos).Compare(pline)==0)
				return FALSE;

			// if same as penultim valid line, OK!
			if (toklines.GetSize()>1)
			  if (toklines.GetPrev(pos).Compare(pline)==0)
				return FALSE;

			// if exact line found in past, ERR!
			if (toklines.Find(pline))
				return TRUE; 
			}

		// otherwise, add to valid list 
		toklines.AddTail(pline);
		// keep it under 6m long
		if (toklines.GetSize()>YEARDAYS/2)
			toklines.RemoveHead();
		return FALSE;
	}

	int findnumtok(const char *tok, const char *line, int last)
	{
		int hn = GetTokenCount(line);
		if (last<0) last =0;
		for (int h=last; h<hn; ++h)
			if (GetToken(line, h).Compare(tok)==0)
				return h;
		return -1;
	}

	void patchheader(const char *header)
	{
		// compute map
		int last  = -1;
		int n = GetTokenCount(PatchMAP[patchmode].header);
		for (int i=0; i<n; ++i)
			{
			int h = findnumtok(GetToken(PatchMAP[patchmode].header, i), header, last);
			ASSERT( h>=0 );
			ASSERT( h>last);
			if (h<0) continue;
			if (last+1<h)
				{
				// gap
				if (last>=0) 
					toklist[toknum++][1] = last; // end
				toklist[toknum][0] = h; // start
				}
			last = h;
			}
		toklist[toknum++][1] = last; // end
	}
};


// =============================================================================
// ============================= LOAD/DOWNLOAD =================================
// =============================================================================





int DownloadToFile(const char *file, const char *url)
{
	// use big timeout
	DownloadFile df(TRUE);
	extern int URLTIMEOUT; 
	int old = URLTIMEOUT;
	URLTIMEOUT = 120; //2 minutes
	BOOL err = df.Download(url, file);
	URLTIMEOUT = old;
	if (err)
		{
		Log(LOGWARN, MkString("%s: Could not download URL %s", file, CString(url,100)));
		return TRUE;
		}
	return FALSE;
}


void DownloadUrls(CString filename)
{
	Log(LOGINFO, "Downloading file list %s", filename);

	CFILE file;
	if ((file.fopen(filename, CFILE::MREAD)) == NULL) 
		{
		Log(LOGERR, "Could not read from file "+filename);
		return;
		}

	char *line;
	while (line = file.fgetstr())
		{
		CString surl(GetTokenRest(line,1));
		CString sfile(GetToken(line,0));
		if (sfile.IsEmpty())
			continue;
		// download file		
		if (!DownloadToFile(FilePath(sfile), surl))
			Log(LOGINFO, sfile+" downloaded");
		}
}


static int cmpstr( const void *arg1, const void *arg2)
{
	return ((CString *)arg1)->Compare(*((CString *)arg2));
}

BOOL GetFileList(const char *path, CStringArrayList &list, BOOL sorted)
{
	PROFILE("GetFileList()");
	CFileFind finder;
	BOOL bWorking = finder.FindFile(path);
    while (bWorking)
    {
		bWorking = finder.FindNextFile();
		if (finder.IsDots()) continue;
		if (finder.IsDirectory()) continue;
		list.AddTail(sorted ? finder.GetFileName() : finder.GetFilePath());
	}

	if (sorted)
		{
		CString spath = CString(path)+"\\";
		list.Sort();
		int size = list.GetSize();
		for (int i=0; i<size; ++i)
			list[i] = spath+list[i];
		}

	return list.GetSize()>0;
}




//////////////////////////////////////////////////////////////////
/////////////////////////// INDEX CONTEXT ////////////////////////
//////////////////////////////////////////////////////////////////

static int cmprankdate( const void *arg1, const void *arg2)
{
	return strcmp(((tindexdates *)arg1)->date,((tindexdates *)arg2)->date);
}

static void cpyrankdate(char *rdate, const char *line)
{
	for (int i=0; i<DATELEN && line[i]!=0 && line[i]!=','; ++i)
		rdate[i] = line[i];
	rdate[i] = 0;
}

int IndexContext::Find(const char *date)
		{
#ifdef RANKQSORT
			 if (nindexdates<=0) 
				 return -1;
			 tindexdates search;
			 cpyrankdate(search.date, date);

			 // check for out of bounds
			 if (cmprankdate(&search, &indexdates[0])<0 ||
				cmprankdate(&search, &indexdates[nindexdates-1])>0)
				return -1;

		     int high, i, low;
			 for ( low=(-1), high=nindexdates;  high-low > 1;  )
				  {
				  i = (high+low) / 2;
				  int cmp = cmprankdate(&search, &indexdates[i]);
				  if (cmp==0)
						return i;
				  if ( cmp < 0 )  high = i;
					   else             low  = i;
				  }
			 return -1;
#else
			 for (int i=0; i<nindexdates; ++i)
				if (strcmp(date, indexdates[i].date())==0)
					return i;
			return -1;
#endif
		}

void IndexContext::Sort()
		{
			PROFILE("IndexContext::Sort()");
			qsort(indexdates, nindexdates, sizeof(*indexdates), cmprankdate);
		}


int IndexContext::AddDate(const char *date, const char *firstline)
		{
			if (nindexdates>=maxindexdates-1)
				{
				int oldmax = maxindexdates; maxindexdates += INDEXBLOCK;
				indexdates = (tindexdates *)realloc( indexdates, sizeof(*indexdates) * maxindexdates);
				for (int i=oldmax; i<maxindexdates; ++i)
					indexdates[i].lines = NULL;
				}
			int n = nindexdates++;
			cpyrankdate(indexdates[n].date, date);
			indexdates[n].flushed = 0;
			indexdates[n].lines = new CLineList(headers);
			indexdates[n].lines->Add( firstline );
			return n;
		}

CString IndexContext::FilePath(const char *file)
	{
		CString path =  indexpath;
		path += "\\";
		path += file;
		path += ".csv";
		return path;
	}

void IndexContext::ResetFiles()
{
		CStringArrayList list;
		GetFileList(FilePath("*"), list);
		for (int i=0; i<list.GetSize(); ++i)
			DeleteFile(list[i]);
}

IndexContext::~IndexContext(void)
	{
	Reset();
	if (indexdates)
		free(indexdates);
	}

IndexContext::IndexContext(const char *group)
{
		char *sym = "*";
		CString filename = DataFile(group, sym);
		indexpath = filename.Mid(0, filename.Find(sym)-1);
		nindexdates = 0;
		flushticks = 0;
		maxindexdates = 0;
		indexdates = NULL;
		this->group = group;
}

BOOL IndexContext::Load(const char *date)
		{
		Reset();
		CFILE f;;
		if (!f.fopen(date, CFILE::MREAD)) 
			return FALSE;

		// dirty trick
		char *line = f.fgetstr(); headers = line;
		while (line=f.fgetstr())
			AddDate(GetToken(line,0), line);

		Sort();
		return TRUE;
		}


BOOL IndexContext::Add(const char *date, const char *line, BOOL hdr)
	{
		// cache header
		if (hdr)
			{
			headers = line;
			return TRUE;
			}

		// process lines
		int n = Find(date);
		if (n<0) 
			{
			// new headers
			const char *linerest = GetTokenRest(headers, 1);
			n = AddDate(date, CString(date)+","+linerest);
			Sort();
			n = Find(date);
			ASSERT( n>=0 );
			if (n<0) return FALSE;
			}

		// add rank line
		indexdates[n].lines->Add(line);
		if (indexdates[n].lines->GetSize()>INDEXBLOCK)
			Flush();
		return TRUE;
	}

BOOL IndexContext::AddSwap(const char *date, const char *line, BOOL hdr)
{
		const char *linerest = GetTokenRest(line,1);
		return Add(GetToken(line,0), CString(date)+","+linerest, hdr);
}
		
BOOL IndexContext::AutoFlush(void)
{	
	// refresh once in a while 
	if (!pcheck(flushticks, FLUSHTIME)) 
		return FALSE;
	Flush();
	return TRUE;
}

BOOL IndexContext::Flush(void)
	{
		for (int i=0; i<nindexdates; ++i)
			Flush(i);
		return TRUE;
	}

void IndexContext::GetList(CLineList &list)
	{ 
	Flush(); 
	for (int i=0; i<nindexdates; ++i)
		list.Add(indexdates[i].date);
	Reset();
	}

void IndexContext::Reset(void)
	{
	for (int i=0; i<nindexdates; ++i)
		{
		delete indexdates[i].lines;
		indexdates[i].flushed = 0;
		indexdates[i].lines = NULL;
		}
	nindexdates = 0; 
	headers.Empty();
	}


CLineList *IndexContext::Lines(int n)
	{ 
	if (n<0 || n>nindexdates-1)
		return NULL;
	return indexdates[n].lines;
	}

BOOL IndexContext::Flush(int n)
	{
		if (n<0) 
			return FALSE;

		CLineList *list = indexdates[n].lines;
		if (list->IsEmpty())
			return TRUE; // empty list
		
		{
		CFILE file;
		CString filename = FilePath(indexdates[n].date);
		if (file.fopen(filename, indexdates[n].flushed>0 ? CFILE::MAPPEND : CFILE::MWRITE)==NULL)
			{
			Log(LOGALERT, "could not append to rank file %s", filename);
			return FALSE;
			}

		for (int i=0; i<list->GetSize(); ++i)
			file.fputstr((*list)[i]);
		}

		list->Empty();
		++indexdates[n].flushed;
		return TRUE;
	}


//////////////////////////////////////////////////////////////////
/////////////////////////// DAY CONTEXT /////////////////////////
//////////////////////////////////////////////////////////////////
DayContext::DayContext(const char *path, const char *filter) : IndexContext(path), FilterContext(filter)
{
	if (FilterContext::Num()<=0)
		return;
}


DayContext::~DayContext(void)
{
	Flush();
}


BOOL DayContext::AddHist(const char *date, const char *line, BOOL hdr)
{
	IndexContext::AutoFlush();
	if (!FilterContext::Match(line, hdr))
		return FALSE;
#ifdef DEBUGXXX
	CFILE f;
	if (f.fopen("debug.match.csv", CFILE::MAPPEND)) f.fputstr(line);
#endif
	return IndexContext::AddSwap(date, line, hdr);
}

BOOL DayContext::AddDay(const char *date, const char *line, BOOL hdr)
{
	IndexContext::AutoFlush();
	if (!FilterContext::Match(line, hdr))
		return FALSE;
#ifdef DEBUGXXX
	CFILE f;
	if (f.fopen("debug.match.csv", CFILE::MAPPEND)) f.fputstr(line);
#endif
	//ASSERT(hdr==1);
	return IndexContext::Add(date, line, hdr);
}

//////////////////////////////////////////////////////////////////
/////////////////////////// RANK CONTEXT /////////////////////////
//////////////////////////////////////////////////////////////////


char *TrimLine(char *line)
{
	for (int i=strlen(line)-1; i>=0 && (line[i]==' ' || line[i]==','); --i);
	line[i+1] = 0;
	return line;
}

void RankContext::Reset()
	{
		ResetFiles();
	}


RankContext::~RankContext(void)
	{
		// create and/or empty rank folder
		Reset();
	}

RankContext::RankContext(const char *path) : IndexContext( path)
	{
		// reset prior rankings
		Reset();
	}


BOOL RankContext::ComputeRank(const char *id)
{

	CSymList list;
	CString filename = FilePath(id);
	if (!list.Load(filename, TRUE))
		return FALSE;
	DeleteFile(filename);
#ifdef DORANK
	list.Rank();	
#endif
	AddSwap(HDRDATE, list.header, TRUE);
	for (int i=0, n=list.GetSize(); i<n; ++i)
		AddSwap(id, list[i].Save());

	return TRUE;
}

BOOL RankContext::UpdateLine(char *line, const char *rankline)
		{
		ASSERT(strncmp(line, rankline, DATELEN)==0);
		if (strncmp(line, rankline, DATELEN)!=0)
			return FALSE; // WRONG!

		char *l;
		strcat(line, ","); // extra to be trimmed away later
		int rlen = strlen(RANKMARK);
		//if (!Lines(n)) return; 
		const char *r;
		//checkparity(line, rankline);
		for (l = strstr(line, RANKMARK), r = strchr(rankline,','); r!=NULL && l!=NULL; r = strchr(r,','), l =strstr(l, RANKMARK))
			{
			
			int i, gap = 0;
			do 
				{
				// copy 1 term
				for (i=0, ++r; *r!=',' && *r!=0; ++i, ++r)
					if (i<rlen) // avoid overwriting l
						*l++ = *r;
				*l++ = ',';
				// accrue cap
				gap += max(rlen-i,0);
				}
			while (strncmp(l+gap, RANKMARK, rlen)==0 && *r!=0);
			ASSERT(gap>=0);
			memmove(l,l+gap,strlen(l+gap)+2);
			//checkparity(l, r);
			}
		ASSERT( (!l || *l==0) && (!r || *r==0));
		TrimLine(line);
		return TRUE;
		}




BOOL RuleData::UpdateRankFile(const char *filename, BOOL isrankfile)
{
	int err = 0;
	const char *line1 = rankctx->Lines(0)->Head();
	const char *linen = rankctx->Lines(rankctx->GetSize()-1)->Head();
	unsigned date = CIdxList::DATE(line1);
	if (isrankfile) // updated rank file
		{ 
		CIdxList idx;
		idx.Find(filename, date);
		if (!idx.Open(filename, IDXUPDATE))
			return FALSE;
		idx.PutHeader(rankctx->Headers());
		for (int i = 0; i<rankctx->GetSize(); ++i)
			idx.PutLine(rankctx->Lines(i)->Head());
		}
	else // update normal file	  
		{ 

		// load old data (to be updated)
		CLineList data;
		if (!data.LoadIdx(filename, date))
			return FALSE; // WRONG!
		if (data.GetSize()!=rankctx->GetSize())
			return FALSE; // WRONG!

		// update and save new data
		CIdxList idx;
		idx.Find(filename, date);
		if (!idx.Open(filename, IDXUPDATE))
			return FALSE;
		idx.PutHeader(data.Header());
		//if (dayctx) dayctx->AddHist(HDRSYM, line, TRUE);

		int maxwline = 0;
		char *wline = NULL;
		for (int i=0; i<data.GetSize(); ++i)
			{
			const char *line = data[i];
			int linelen = strlen(line)+2;
			if (linelen>maxwline)
				wline = (char*)realloc(wline, maxwline = linelen);
			strcpy(wline, line);

			//int n = rankctx->Find(data[i]);
			//ASSERT( n>=0 );
			//if (n<0 || !rankctx->Lines(n)->IsEmpty())			
			if (!rankctx->UpdateLine(wline, rankctx->Lines(i)->Head()))
				++err; // WRONG

			idx.PutLine(wline);
			//if (dayctx) dayctx->AddHist(sym, wline);
			}
		free(wline);
		}

	return !err;
}






void RuleData::ComputeRank(CSymList &symlist, const char *group)
{
	if (dayctx) dayctx->Flush();

	if (!rankctx || !RANKRULES)
		return;
	
	CProgress mprogress("RANK", 0, 2);

	// compuet rank files
	mprogress.Set(0);
	{
	CLineList list;
	rankctx->GetList(list);
	CProgress progress("CALC", 0, list.GetSize());
	for (int i=0; i<list.GetSize(); ++i)
		{
		progress.Set(i);
		CString id = list[i];
		rankctx->ComputeRank(id);
		}
	rankctx->Flush();
	}

	// reset day files that are to be updated
	if (dayctx)
	{
	dayctx->Flush();
	dayctx->Reset();
	}

	// update files with ranking info
	mprogress.Set(1);
	{
	CLineList list;
	rankctx->GetList(list);

	// patch: keep original sym sort
	list.Empty();
	for (int i=0; i<symlist.GetSize(); ++i)
			list.Add(symlist[i].id);

	CProgress progress("UPDATE", 0, list.GetSize());
	for (int i=0; i<list.GetSize(); ++i)
		{
		progress.Set(i);
		CString sym = list[i];
		// load rank data in memory
		if (!rankctx->Load(rankctx->FilePath(sym)))
			continue; // no data to update for this sucker
		// update rankings
		if (!UpdateRankFile(DataFile(group, sym)))
			Log(LOGALERT, "RANK: Could not update rankings for %s %s", group, sym);
		// update rank group (if not done already) [NOT WORKING? / NOT NEEDD]
		////if (strcmp(group, RANKGROUP)!=0) 
		///	UpdateRankFile(DataFile(MkString("%s%s", RANKGROUP, group), sym), TRUE);
		}	
	}

	// reset day files that are to be updated
	if (dayctx)
	{
	dayctx->Flush();
	dayctx->Reset();
	}
}


//////////////////////////////////////////////////////////////////
/////////////////////////// LOAD HISTORY /////////////////////////
//////////////////////////////////////////////////////////////////

	
BOOL GetFileHeader(const char *grp, CString &header)
{
	CString group = grp, sym = REFSYM;
	CString file = DataFileCore(group, sym);
	// real files
	/*
	CFileFind finder;
	if (file.Find("*"))
		if (finder.FindFile(file))
			{
			finder.FindNextFile();
			file = finder.GetFilePath();
			}
	*/
	{
	CFILE f;
	if (!f.fopen(file, CFILE::MREAD)) 
		{
		Log(LOGFATAL, "fatal error, could not load headers for file %s", file);
		return FALSE;
		}
	header = f.fgetstr();
	}


	// set config for future use (speed up data load)
	SetConfigHeader(grp, header);
	return TRUE;
}



CSymList cfgheaders;
BOOL SetConfigHeader(const char *grp, const char *header)
{
 	cfgheaders.AddUnique( CSym(grp, header));
	cfgheaders.Sort();
	return TRUE;
}




BOOL GetConfigHeader(const char *group, CString &header)
{
	CString sym = "*", grp = group;
	CString file = DataFileCore(grp, sym);
	if (strcmp(sym,SYMECONOMY)==0) 
		file = DataFileCore(grp, sym = "*");
	// other configs 
	int find = cfgheaders.Find(grp);
	if (find>=0)
		{
		header = cfgheaders[find].data;
		return TRUE;
		}	

	// if not found, try creating it
	if (CSymID::IsIndex(grp) || CSymID::IsFX(grp))
		{
		header = OHLCHEADER;
		SetConfigHeader(grp, header);
		return TRUE;
		}

	// from download groups
	if (GetGroupInfo(grp, header))
		if (!header.IsEmpty())
			{
			SetConfigHeader(grp, header);
			return TRUE;
			}

	// get from file itself
	Log(LOGWARN, "WARNING: Missing header configuration for %s [%s], reading from file", file, grp);
	if (GetFileHeader(grp, header))
		if (!header.IsEmpty())
			return TRUE;
	
	// ERROR
	Log(LOGALERT, "ERROR: Missing header configuration for %s [%s] (not even in file)", file, grp);
	return FALSE;
}


int CreateHeaders(ruledef *def, CStringArrayList &headers)
{
	CString header;
	if (!GetConfigHeader(def->param[0], header ) || header.IsEmpty())
			return 0; // error
/*
	if (IsGroupFile(def->param[0]))
		{
		int pos = GetTokenPos(header,1);
		header.Delete(pos, GetTokenPos(header,2)-pos);
		}
*/
	return CreateSectionList(header, headers);
}






BOOL  SortGroupFile(const char *file, const char *outfile, double date)
{
		if (IsEvtFile(file) && !IsGlobalFile(file) && date>0)
			Log(LOGALERT, "Shorted SortGroupFile for file %s", file);

		//Progress progress(GetFileName(outfile), 0, 4);
#if 1
		// segmented approach: load letter groups A B C for ALL dates (keep array of cur pointers) and sort and save in chunks

		CIdxList ridx, idx, idg;
		if (!ridx.Find(file, date, TRUE))
			return FALSE;
		Idx &rlast = ridx[ridx.GetSize()-1];
		int filesize = rlast.seekend();

		CFILE f;
		CString header;
		if (!f.fopen(file, CFILE::MREAD))
			return FALSE;
		if (f.fsize()!=filesize)
			{
			Log(LOGALERT, "IDX: Invalid IDX for group file %s, rebuilding", file);
			ridx.Rebuild(file, TRUE);
			}
		header=f.fgetstr();
		if (!idx.Open(outfile, IDXWRITE, &idg))
			return FALSE;
		if (!idx.PutHeader(header))
			return FALSE;


		int curseekpos = 0;
		CStringArrayList seeklines(ridx.GetSize());
		Idx &last = ridx[ridx.GetSize()-1];
		int maxreadsize = last.seekend()-ridx[0].seekpos;
		int c = ' ', cend = 128;
		int cstep = ((cend-c)*MAXSORTSIZE)/maxreadsize+1;
		CProgress mprogress(MkString("Indexing %s", file), c, cend);

		do
			{
			c+=cstep;
			// read a chunk starting with a letter/symbol
			CLineList data;
			mprogress.Set(min(cend,c));
			for (int i=0; i<ridx.GetSize(); ++i)
				{
				while (TRUE)
					{
					int &seekpos = ridx[i].seekpos;
					CString &line = seeklines[i];
					if (line.IsEmpty())
						{
						if (ridx[i].seeklen<=0)
							break;
						if (seekpos!=curseekpos)
							f.fseek(curseekpos = seekpos, SEEK_SET);
						line=f.fgetstr();
						if (line.IsEmpty())
							{
							Log(LOGALERT, "IDX: Weird error indexing group file %s", file);
							break;
							}
						int len = f.fstrlen(line);
						curseekpos = seekpos += len;
						ridx[i].seeklen -= len;
						}
					else
						{
						CString sym = GetToken(line,1);
						if (sym[0]>c) 
							break;
						if (CSymID::Validate(sym))
							{
							line.Replace(' ','_');
							data.Add(line);
							}
						line.Empty();
						}
					}
				}
			// sort
			data.Sort(1);
			// write a chunk
			for (int i=0; i<data.GetSize(); ++i)
				idx.PutLine(data[i]);
			}
		while (c<cend);
		//idx.Sort();
		idx.Close();

#else
		// brute force: load all & sort in memory
		CLineList data;
		ShowProgress(MkString("Indexing %s 0%%", file));
		data.LoadIdx(file, date);

		ShowProgress(MkString("Indexing %s 50%%", file));
		data.Sort(1);
 
		// Save with a linked double IDX
		ShowProgress(MkString("Indexing %s 75%%", file));
		CIdxList idx, idg;
		if (!idx.Open(outfile, IDXWRITE, &idg))
			return FALSE;
		if (!idx.PutHeader(data.Header()))
			return FALSE;
		for (int i=0; i<data.GetSize(); ++i)
			idx.PutLine(data[i]);
		idx.Close();
#endif
		return TRUE;
}
	

//static CArray <CIdxList,CIdxList&> idxgrouplist;
void SortGroupFile(const char *file, const char *outfile, double date, CIdxList &idx, int groupnum)
{
	idx.Reset();
/*
	if (groupnum>=0)
		if (groupnum<idxgrouplist.GetSize() && idxgrouplist[groupnum].GetSize()>0)
			{
			// cached group index
			idx=idxgrouplist[groupnum];
			return;
			}
*/
	// check time stamps
	FILETIME ft, outft;
	for (int retry=0; !CFILE::gettime(file, &ft) && retry<5; ++retry)
	    {
		if (!CFILE::exist(file)) break;
		ShowProgress("Sleep(1000) @ SortGroupFile()");
		Sleep(1000);
	    }

	for (int retry=0; !CFILE::gettime(outfile, &outft) && retry<5; ++retry)
	    {
		if (!CFILE::exist(outfile)) break;
		ShowProgress("Sleep(1000) @ SortGroupFile()");
		Sleep(1000);
	    }
	
	BOOL expired = CompareFileTime(&ft,&outft)!=0;
	
	if (!expired)
		if(!idx.Load(outfile))
			expired = TRUE;

	if (expired)
		{
		if (!SortGroupFile(file, outfile, date))
			{
			DeleteFile(outfile);
			if (!SortGroupFile(file, outfile, date))
				{
				Log(LOGALERT, "IDX: unexpected recover indexing group file %s", file);
				return;
				}
			}
		CFILE::settime(outfile, &ft);
		idx.Load(outfile);
		}
	idx.Sort();
/*
	// add to cache group index
	if (groupnum>=0)
		idxgrouplist.SetAtGrow(groupnum, idx);
*/
}










//typedef double tgetnum(const char *s);




class CLine 
{

protected:
	char *line; 
	int nline, nlinemax;
	CString header;

public:

	CLine(void)
	{
		nline = nlinemax = 0;
		line = NULL;
		set("");
	}

	~CLine(void)
	{
		if (line!=NULL)
			free(line);
	}


	char *fget(CFILE &file)
	{
		dirty();
		char *ret = line;
		int linepos = 0;
		while (file.fgetstrbuff((line = alloc(linepos+LINELENBUFF))+linepos, LINELENBUFF)<0)
			linepos += LINELENBUFF; 
		//alloc(-1); // check buffer overflow
#ifdef DEBUGLINE
		if (ret && *line!=0)
			if (line[0]=='\n' || line[strlen(line)-1]=='\n')
			Log(LOGERR, "trim nl not working!");
#endif
		return *line==0 ? NULL : ret;
	}

	char *getheader(CFILE &file)
	{
		// write headers
		char *hdr = fget(file);
		if (hdr) header = hdr;
		return hdr;
	}
	void fputheader(CFILE &file, const char *hdr = NULL)
	{
		// write headers
		file.fputstr(hdr ? hdr : header);
	}

	void fput(CFILE &file, const char *buffer = NULL)
	{
		if (!buffer) buffer = this->buffer();

		// always trim the end if needed
		int nline;
		for (nline = strlen(buffer) ; nline>=0 && (buffer[nline-1]==',' || buffer[nline-1]==' '); --nline);
		((char*)buffer)[nline]=0;

		// write
		file.fputstr(buffer);
	}

	char *set( const char *_line)
	{
		if (!_line)
			_line="";

		dirty();
		nline = strlen(_line);
		line = alloc(nline);
		strcpy(line, _line);
		//alloc(-1);
		return line;
	}

	void add( const char *addstr)
	{
		alloc(nline+LINELENBUFF);
		for (const char *s=addstr; *s!=0; ++s)
			line[nline++] = *s;
		line[nline] = 0;
		dirty();
	}

	void addc( const char *addstr)
	{
		alloc(nline+LINELENBUFF);
		line[nline++] = ',';
		add(addstr);
	}

	/*
	void swap(void)
	{
		dirty();
		swapline(line);
	}
	*/

	virtual void dirty(void)
	{
	}

	inline const char *buffer(void) 
	{ 
		return line; 
	}
	
	inline operator const char *()
	{
		return buffer();
	}


	char *buffer(BOOL write)
	{
		if (write) dirty();
		return line;
	}

	// overriden in Cache
	virtual char *alloc( int nline)
	{
		chkalloc(line);
		if (nline+1>=nlinemax)   // always guarantee can add LINELENBUFF bytes
			line = (char *)realloc( line, (nlinemax= ((nline+1)/LINELENBUFF+1)*LINELENBUFF));
		return line;
	}

};





/////////////////////////// CACHE ////////////////////
#define mstrlen(x) (strlen(x)+1)

// cached file
class CLineCache : public CLine, public CSEEKID {

	char *cline;
	int clinepos, clinemaxsize;
	int *cidxline;
	int cidxlinepos, cidxlineposw, cidxlinesize, cidxlinemaxsize;

	public:

	CLineCache() : CLine()
	{
		cline = line;
		cidxline = NULL;
		clinemaxsize = 0;
		cidxlinemaxsize = 0;
		creset();
	}

	~CLineCache(void) 
	{
		line = cline;
		if (cidxline)
			free(cidxline);
	}

	int csize(void)
	{
		return cidxlinesize;
	}

	void creset(void)
	{
		line = cline;
		clinepos = 0;
		cidxlinepos = 0;
		cidxlinesize = 0;
	}

	char *alloc( int nline)
	{
		chkalloc(cline);
		if (clinepos+nline+1>=clinemaxsize)
			cline = (char *)realloc( cline, (clinemaxsize = ((clinepos+nline+1)/LINELENBLOCK+1)*LINELENBLOCK));
		return line = cline + clinepos;
	}

	int cread(int size = 0)
	{
	if (size>0)
		{
		// realloc buffers if necessary
		register int pos = clinepos, posend = pos+size-1;
		register char *data = cline+clinepos;
		register int n = cidxlinesize;
		// fill up line structure
		while (pos<posend)
			{
			if (n+1>=cidxlinemaxsize)
				cidxline = (int*)realloc(cidxline, (cidxlinemaxsize += CIDXBLOCK)*sizeof(*cidxline));
			cidxline[n++] = pos;
			while (data[pos]!=FILEEOL[0] && pos<posend) 
				++pos;
			data[pos] = 0;
			pos += FILEEOLSIZE;
			}
		if (data[pos]!=0)
			return FALSE; // error

		// success!
		cidxlinesize = n;
		clinepos = pos;
		line = cline + clinepos;
		return cidxlinesize;
		}
	else
		{
#ifdef DEBUGLINE
		if (line != cline + clinepos)
			Log(LOGALERT, "illegal clinepos %.15s", line);
		if (line!=NULL && *line!=0)
			if (CGetDate(line)==InvalidDATE)
				Log(LOGALERT, "illegal CLine date %.15s", line);
#endif
		// reallocate buffers if needed
		if (cidxlinesize+5>=cidxlinemaxsize)
			cidxline = (int*)realloc(cidxline, (cidxlinemaxsize += CIDXBLOCK)*sizeof(*cidxline));
		// read one more line
		//char *ret = f.fgetstr(cline+cidxlinepos, MAXLINELEN);
		cidxline[ cidxlinesize++ ] = clinepos;
		clinepos += mstrlen( cline + clinepos );
		if (nlinemax==0) nlinemax = LINELENBUFF; // just in case
		alloc(clinepos+nlinemax);
		return 1;
		}
	chkalloc(cline);
	}

	BOOL copen(void)
	{
#ifdef SKIPEMPTYFILE
		if (!cidxlinesize)
			return NULL;
#endif
		line = cline;
		cidxlinepos = 0;
		return TRUE;
	}

	const char *cget(void)
	{
		if (cidxlinepos>=cidxlinesize)
			return NULL;
		line = cline+cidxline[cidxlinepos++];
#ifdef DEBUGLINE
		if (!line || CGetDate(line)==InvalidDATE)
			Log(LOGALERT, "illegal CLine date %.15s", line);
#endif
		dirty();
		return line;
	}

	BOOL cclose(void)
	{
		chkalloc(cline);
		line = cline;
		cidxlinepos = 0;
		return TRUE;
	}

	inline int getidx(int i)
	{
		return (int)CGetDate(cline+cidxline[i]);
	}

	void cseekid(double mindate)
		{
		cidxlinepos = seekidx((unsigned)mindate, 0, cidxlinesize-1);
		if (DEBUGLOAD) // DEBUGSEEKID
			{
			CString id = CDate(mindate);
			int len = strlen(id);
			for (int pos=0; pos<cidxlinesize && cmpid(id, cline+cidxline[pos], len)>0; ++pos);
			if (cidxlinepos!=pos)
				Log(LOGERR, "mseekid malfunctioning for %d<>%d [0-%d]", pos, cidxlinepos, cidxlinesize);
			}
		}

	BOOL copenw(void)
	{
		cidxlineposw = 0;
		return cidxlinesize>0;
	}

	const char *cgetw(void)
	{
		if (cidxlineposw>=cidxlinesize)
			return NULL;
		return cline+cidxline[cidxlineposw++];
	}

	BOOL cclosew(void)
	{
		chkalloc(cline);
		cidxlineposw = 0;
		return TRUE;
	}
};




/////////////////////////// TOKEN ////////////////////


class CLineToken : public CLineCache {
	int *token;
	int ntoken, ntokenmax;
	CStringArrayList headers;


public:

	CLineToken() : CLineCache() 
	{
 		token = (int *)malloc(sizeof(*token));
		ntoken = ntokenmax = 0;
	}

	~CLineToken()
	{
		if (token)
			free(token);
	}

	void dirty(void)
	{
		ntoken = -1;
	}

	CString getheader(void)
	{
		return header;
	}

	void setheader(const char *hdr)
	{
		header = hdr;
		headers.Reset();
		CreateSectionList(header, headers);
	}

	inline int find(const char *cmp)
	{
		return FindSectionList(cmp, headers);
	}

	CString getdate(void)
	{
		return CString(buffer(), DATELEN);
	}

	int cmpdate(const char *date)
	{
		return strncmp(buffer(), date, DATELEN);
	}

	void calctoken()
	{
		// calc token map
		ntoken = 0;
		const char *ptr = buffer();
		for (int i=0; ptr[i]!=0; ++i)
			if (ptr[i]==',')
				{
				if (ntoken>=ntokenmax)
					token = (int *)realloc( token, ((ntokenmax +=TOKENBLOCK)+5) *sizeof(*token));
				if (!token)
					Log(LOGFATAL, "FATAL: not enough memory for CLine::token");
				token[ntoken++] = i+1;
				}
		// end of line
		token[ntoken] = i;
	}

	int gettokenposn(int n)
	{
		if (ntoken<0) calctoken();
		int pos = (n>0) ? token[n-1] : 0;
#ifdef DEBUGTOKEN
		int oldpos = GetTokenPos(buffer(), n);
		if (pos != oldpos)
			Log(LOGERR, "inconsistency detected with gettokenpos %d<>%d", pos, oldpos);
#endif
		return pos;
	}

	inline const char *gettokenpos(int n)
	{
		return buffer()+gettokenposn(n);
	}

	inline BOOL getvalid(int n, int *_start = NULL, int *_len = NULL)
	{		

		if (ntoken<0) calctoken();
		if (n>ntoken) return FALSE;
		int end = n>=ntoken ? 0 : 1;
		int start = start = n>0 ? token[n-1] : 0;
		int len = token[n]-start-end;
		if (len<=0) return FALSE;
		if (_start!=NULL) *_start=start;
		if (_len!=NULL) *_len=len;
		//ASSERT (FindSectionList("MCAP", headers)!=n);
		return TRUE;
	}
	
};


int LoadProcess::find(const char *cmp)
	{
		return line->find(cmp);
	}

const char *LoadProcess::buffer(void)
	{
		return line->buffer();
	}

#ifdef DEBUGTOKEN
CString LoadProcess::getstring(int n)
	{
		CString oldtok = GetToken(line->buffer(), n);
		CString tok = getstringnew(n);
		if (tok.Compare(oldtok)!=0)
			Log(LOGERR, "inconsistency detected with gettokenpos %s<>%s", tok, oldtok);
		return tok;
	}
CString LoadProcess::getstringnew(int n)
#else
CString LoadProcess::getstring(int n)
#endif
	{
		int start, len;
		if (!getvalid(n, &start, &len)) 
			return "";
		return CString(line->buffer()+start, len);
	}
	
double LoadProcess::getnum(int n)
	{
		int start, len;
		if (!line->getvalid(n, &start, &len))
			return InvalidNUM;
		return CGetNum(line->buffer()+start);
	}

double LoadProcess::getdate(int n)
	{
		if (n==0)
			return CGetDate(line->buffer());
		int start, len;
		if (!getvalid(n, &start, &len))
			return InvalidDATE;
		return CGetDate(line->buffer()+start);
	}

void LoadProcess::setnum(int n, double val)
	{
		char *pos = (char *)line->gettokenpos( n );
		CDATA::SetNum(pos, val); 
		line->dirty();
	}

void LoadProcess::setstring(int n, const char *str)
	{
		char *pos = (char *)line->gettokenpos( n);
		strcpy(pos, str); 
		line->dirty();
		// set proper type? why? is going to be mapped
		//c->map->rule->cfg->RULES[c->map->rule->sr].vtype = rSTR;
	}

BOOL LoadProcess::getvalid(int n, int *start, int *len)
	{
		return line->getvalid(n, start, len);
	}


class CFileNoCache {
	CLineToken rline;
	CIdxList idx;

public:
	CLineToken line;
	CString filename;
	CString sym;
	CString group;
	int groupnum;

	CFileNoCache()
	{
		groupnum = -1;
	}
	~CFileNoCache()
	{	
	}

	void setup(CString &sym, CString &group)
	{
		filename = DataFileCore(group, sym);
		this->sym = sym; // IMPORTANT! Save sym in class AFTER DataFileCore!
		this->group = group;
	}

	virtual BOOL openw(CString &sym, CString &group, double mindate)
	{
		// open new file		
		setup(sym, group);
		idx.Find(filename, mindate);		
		if (!idx.Open(filename, mindate>0 ? IDXUPDATE : IDXWRITE))
			{
			Log(LOGALERT, "Could not write to file %s", filename);
			return FALSE;
			}
		
		return TRUE;
	}

	virtual BOOL putheaderw(const char *header)
	{
		idx.PutHeader(header);
		return TRUE;
	}

	virtual BOOL putlinew(const char *buffer)
	{
		idx.PutLine(buffer);
		return TRUE;
	}

	virtual BOOL closew(void)
	{
		// close files
		idx.Close();
		return TRUE;
	}
};




////////////////////////////////////////////// file mode

class CFileCache : public CFileNoCache {

	// real file
	BOOL cached;
	volatile BOOL cachedw;
	BOOL groupmode;	

#ifdef USELOADPATCH
	PatchMode patch;
#endif

	// indexing
	CIdxList idx;
	CString idxgroup;

	BOOL _open(CString &sym, CString &group, double mindate, BOOL cache = FALSE)
	{		
		setup(sym, group);
		if (filename.IsEmpty())
			return FALSE;

		//ASSERT( sym!="AA" || group!="@BREps");
		//ASSERT(group!="$RANK");
		//ASSERT( sym.Find("Consumer_Confidence")!=0 );

		//isortedmode = IsInverseSortedFile(group);
		groupmode = IsGroupFile(filename);
		omindate = mindate;

		// override headers asap!
		CString header;
		GetConfigHeader(group, header);

		//ASSERT( filename.Find("@TMDiv")<=0);
		if (!groupmode)
			{
			// NORMALMODE
			// open file
			if (!idx.Open(filename, IDXREAD))
				return FALSE;
			// seek to pos and get header
			idx.Find(filename, mindate);
			if (!idx.SeekHeader(header))
				{
				Log(LOGALERT, "IDX: Find failed for indexed file %s (%s %s)", filename, sym, group);
				idx.Close();
				return FALSE;
				}
			}
		else
			{
			//ASSERT( sym!="FOMC_Minutes");
			// GROUPMODE
			CString outfile = GetToken(DataFile(TEMPFOLDER, "*"),0,'.');
			outfile.Replace("*", GetFileNameExt(filename));
			if (!idx.GetSize() || idxgroup!=group)
				{
				//ASSERT(idxgroup==group);
				// compute and cache IDX table
				SortGroupFile(filename, outfile, mindate, idx, groupnum);
				idxgroup = group;
				}
			filename = outfile;

			// openfile
			if (!idx.Open(filename, IDXREAD))
				return FALSE;
			if (!idx.FindCRC(filename, idx.CRC(sym), (int)mindate))
				{
				// equivalent to file not exist!
				if (DEBUGLOAD) 
					Log(LOGWARN, "IDX NOT PRESENT: sym %s in %s", sym, filename);
				if (idx.GetSize()<=0)
					Log(LOGALERT, "IDX: Empty group index for %s", group);
				idx.Close();
				return FALSE;
				}
			}

		// check all is fine
		if (DEBUGLOAD)  // DEBUGSEEKID
			{
				CFILE &file = idx.File();
				long seek = file.ftell();

				// check next line is consistent
				char *line;
				if (line=file.fgetstr())
					{
					double fdate = CGetDate(line);
					if (fdate==InvalidDATE || fdate<mindate)
						Log(LOGALERT, "inconsitency data for %s %s mindate=%s fdate=%s ", sym, group, CDate(mindate), CDate(fdate));					
					if (groupmode)
						if (CSymID::Compare(sym, line, 1)!=0)
							Log(LOGALERT, "inconsitency group data for %s %s in line \"%100s\"", sym, group, line);
					}

				if (DEBUGLOAD>1)
				{
				CString linechk = line;
				// find seek symbol
				if (groupmode)
					{
					file.fseek(0, SEEK_SET);
					line = file.fgetstr(); // header
					while ((line=file.fgetstr())!=NULL && (CGetDate(line)<mindate || CSymID::Compare(sym, line, 1)!=0));
					}
				else
					{
					file.fseek(0, SEEK_SET);
					line = file.fgetstr(); // header
					while ((line=file.fgetstr())!=NULL && CGetDate(line)<mindate);				
					}
				if (line && linechk!=line)
					Log(LOGALERT, "IDX: IDXed seek and normal seek do not match: %.20s vs %.20s", line, linechk);
				}

				// recover seek position
				file.fseek(seek, SEEK_SET); 
			}

		// success!!!
#ifdef USELOADPATCH
		patch.set(group, header);
#endif
		line.setheader(header);

		// cache in memory!!!
		if (cache)
			{
			//ASSERT( filename.Find("@TMDiv")<=0);
#ifdef USEREADATONCE
			int num = 0, size = idx.ReadLines();
			if (size<=0) // empty
				return TRUE;
			line.alloc(size);
			if (!idx.ReadLines(line.buffer(TRUE)) || !(num = line.cread(size)))
				{
				Log(LOGALERT, "IDX: Inconsistency reading lines for %s, deleting IDX to force rebuild", filename);
				idx.Close();
				DeleteFile(CIdxList::Filename(filename));
				return FALSE;
				}
			if (DEBUGLOAD)
				{
				idx.SeekPos();
				idx.ReadLines();
				// read to end of file
				int num2 = 0;
				while (_getline()!=NULL)
					++num2;
				if (num!=num2)
					Log(LOGALERT, "IDX: Cache/noncache num lines not matching for %s", filename);
				}
#else				
			// read to end of file
			while (_getline()!=NULL)
				line.cread();
#endif
			}


		return TRUE;
	}




	const char *_getline()
	{
		if (!line.fget(idx.File()))
			return NULL;
		if (groupmode)
			{
			BOOL match = CSymID::Compare(sym, line, 1)==0;
#ifdef DEBUGLINE
			// add ',' and compare?
			CString tok = GetToken(line,0);
			BOOL tokb = tok.CompareNoCase(sym)==0;			
			if (match!=tokb)
				Log( LOGALERT, "strcmpdot inconsistency" );
			//if (CSymID::Compare(sym,line)!=0)
			// return NULL; // end
#endif
			if (!match)
				{
				// check consitency before disregaring possible data
				if (CGetDate(line) == InvalidDATE )
					{
					const char *str = line;
					Log(LOGALERT, "inconsitency group data for %s %s in line \"%100s\"", sym, group, str);
					}
				return NULL;
				}
			}
		return line;
	}
	
	BOOL _close(void)
	{
		// normal file
		idx.Close();
		return TRUE;
	}


	public:
	//CString id;
	double omindate; // mindate when cached

	CString name(void)
	{
		return filename;
	}

	BOOL open(CString &sym, CString &group, double mindate)
	{
		//ASSERT(group!="$@DIV");
		// file does not exist
		if (cached<0)
			return FALSE;

#ifdef DEBUGEVTFILESPECIAL
		if (IsEvtFile(group+sym) && mindate>0) // Evt files read more
			Log(LOGWARN, "mindate>0 for loading event file %s %s", sym, group);
#endif
		//id = "0"; //reset 

		if (!cached)
			{
			line.creset();
			return _open(sym, group, mindate);
			}
		// cached file
		if (omindate>mindate || (DEBUGLOAD>2 && omindate!=mindate))
			Log(LOGALERT, "CACHE: MINDATE mismatch in %s %s Cached:%g %s Open:%g", 
			sym, group, omindate, omindate>mindate ? ">ERR>" : "<OK!<", mindate);

		if (!line.copen())
			return FALSE;

		//ASSERT( group!="$@DIV");
		// seek if needed
		if (mindate>0)
			line.cseekid(mindate);
		return TRUE;
	}


	const char *getline()
	{
		const char *str;
#ifdef USELOADPATCH
		repeat:
#endif
		str = (!cached) ? _getline() : line.cget();
#ifdef USELOADPATCH
		if (patch.check(str))
			goto repeat;
#endif
		return str;
	}

	BOOL close(void)
	{
		return (!cached) ? _close() : line.cclose();
	}

	inline BOOL iscached(void)
	{
		return cached!=0;
	}

	inline BOOL iscachedw(void)
	{
		return cachedw!=0;
	}

	BOOL cache(CString &s, CString &group, double mindate)
	{
#ifdef DEBUGCACHE
		//CString id = "0";
		//if (mindate>0) id = CDate(mindate);
		//Log(LOGINFO, "CACHEID:,%s,%s,%s [%g]", s, group, id, mindate);
#endif
		//ASSERT(group!="$@DIV");
		cached = TRUE;
		line.creset();
		if (!_open(s, group, mindate, TRUE))
			{
			cached = -1; // file does not exist
			return TRUE;
			}

		_close();

		// if empty, we treat like file not exist
#ifdef SKIPEMPTYFILE
		if (line.csize()==0) 
			cached = -1;
#endif
		return TRUE;
	}

	enum { CWDISABLED=0, CWAPPEND, CWOVERWRITE};
	BOOL openw(CString &sym, CString &group, double mindate)
	{
		// if not already flushed
		if (cachedw) 
			{
			Log(LOGALERT, "unexpected racing condition for flushing %s", filename);
			while (cachedw) Sleep(100);
			}

		setup(sym, group);

		// setup
		omindate = 0;
		cached = TRUE;
		cachedw = TRUE;
		omindate = mindate;
		line.creset();
		return TRUE;
	}

	BOOL putheaderw(const char *header)
	{
		line.setheader(header);
		return TRUE;
	}

	BOOL putlinew(const char *buffer)
	{
		if (buffer != line)
			line.set(buffer);
		line.cread();
		return TRUE;
	}
		

	BOOL closew(void)
	{
		// nothing
		return TRUE;
	}

	BOOL flush(void)
	{
		if (!cachedw) return TRUE;

		// WRITE NEW DATA TO DISK (write even empty files for EPS / DIV)
		if (!line.copenw() && omindate>0)
			return FALSE;

		//CFileNoCache f;
		if (!CFileNoCache::openw(sym, group, omindate))
			return FALSE;

		CFileNoCache::putheaderw(line.getheader());
		const char *curline;
		while((curline=line.cgetw())!=NULL)
			CFileNoCache::putlinew(curline);
		CFileNoCache::closew();

		cachedw = FALSE; // flushed!
		return TRUE;
	}

		
	void reset()
	{
		cached = FALSE;
		cachedw = FALSE;
		omindate = -1; // invalid on purpose
		line.creset();
	}

	CFileCache()
	{

		reset();
	}

	~CFileCache()
	{		
		idx.Close();
		flush();
	}

};


int MAXREADSYM = MAXCACHESYM;

class CFileCacheTable {
public:
	BOOL output;
	double mindate;
	CFileCache file;
	CFileCacheTable(int gnum = -1, const char *g = NULL, double mdate =0, BOOL o = FALSE) 
		{ 
		file.groupnum=gnum;
		file.group=g; 
		mindate=mdate; 
		output = o; 
		}
};

int cmpft( const void *arg1, const void *arg2 )
{
	return  strcmp( (*(CFileCacheTable**)arg1)->file.group, ((*(CFileCacheTable**)arg2)->file.group) ) ;
}

#ifdef DEBUGCACHE2
	CFileCache debugfile;
#endif


class CDisk {
	int nfiles, ngroup;
	CString syms[MAXCACHESYM];
	CFileCacheTable *files[MAXCACHEFILES];

	CFileCacheTable deffile;

	CFileCacheTable deffilew;
	CFileNoCache nocachefile;

	public:
	// public for ASSERTs
	CFileCache *file;
	CFileNoCache *filew; 
	
	CString status;
	int cmiss, chit, cmissw, chitw, cflushw;	

	CDisk()
	{
		chit = 0;
		cmiss = 0;
		chitw = 0;
		cmissw = 0;
		cflushw = 0;
		nfiles = 0;
	}

	~CDisk()
	{
		finalize();
	}

	void finalize(void)
	{
		flushall();
		int i;
		for (i=0; i<nfiles; ++i)
			delete files[i];
		ngroup = 0;
		nfiles = 0;
		for (i=0; i<MAXCACHESYM; ++i)
			syms[i].Empty();
	}

	void Reset(CLoadFileList &loadmap, CLoadFileList &savemap)
	{
		chit = 0;
		cmiss = 0;
		chitw = 0;
		cmissw = 0;
		cflushw = 0;
#ifndef USECACHE
		status = "off<";
#else
#ifndef USECACHEW
		status = "Rw<";
#else
		status = "RW";
#endif
#endif
		if (NOCACHEW) status = "Rw";
		if (NOCACHE) status = "off";

		finalize();

#ifdef USECACHEW
		if (!NOCACHEW)
			{
			// add output to cache, so it is cached too
			POSITION pos = savemap.GetHeadPosition();
			while (pos!=NULL)
				{			
				CLoadFile &f = savemap.GetNext(pos);
				loadmap.Add(f.filename, f.rulename, f.maxdays);
				}
			}
#endif
		// report what we'll load
		if (DEBUGLOAD>1) loadmap.Summary("CACHING");

		// create new table of files (base but extensible)
		for (int i=0; i<MAXCACHESYM; ++i)
			{
			POSITION pos = loadmap.GetHeadPosition();
			for (ngroup=0; pos!=NULL; ++ngroup)
				{			
				CLoadFile &f = loadmap.GetNext(pos);
				files[ nfiles++] = new CFileCacheTable(ngroup, f.filename, f.OldestDate(), savemap.Find(f.filename)!=NULL);
				if (nfiles>=MAXCACHEFILES)
					{
					Log(LOGERR, "cache is full! please extend MAXCACHEFILES>%d!", nfiles );
					return;
					}
				}		
			// presort
			qsort(files+i*ngroup, ngroup, sizeof(*files), cmpft);
			}
	}

	BOOL flushw(void)
	{
		BOOL ret = FALSE;
#ifdef USEDELAYEDWRITE2
		static volatile int flushing = 0, flushing2 = 0;
		if (flushing>0) 
			return FALSE;
		++flushing2;
		if (flushing>0 || flushing2>1)
			{
			--flushing2;
			return FALSE;
			}
		++flushing;

		// critical section flush
		if (!NOCACHEW)
			ret = deffilew.file.flush();

		--flushing;
		--flushing2;
#endif
		return ret;
	}

	BOOL flushall(BOOL deffile = TRUE)
	{
		if (DEBUGLOAD>1) Log(LOGINFO, "CACHE: Flushing...");
		// flush any pending INPUT/OUTPUT action
		if (deffile) flushw();
		for (int i=0; i<nfiles; ++i)
			files[i]->file.flush();
		return TRUE;
	}	

	BOOL prefetch(const char *sym, int nsym)
	{
		if (NOCACHE) return FALSE;
#ifndef USECACHE
		return FALSE;
#endif
		PROFILE("prefetch()");
		// flush any pending INPUT/OUTPUT action
		//flushall();

		// find proper slot / create one if needed
		int n = nsym % MAXCACHESYM;
		
		if (DEBUGLOAD>1) Log(LOGINFO, "CACHE: PREFETCH %s #%d [%d]", sym, nsym, n );

		// refresh all contents of slot n
		syms[n] = sym;
		for (int i=0; i<ngroup; ++i)
			{
			CFileCacheTable *ft = files[n*ngroup+i];
			CString s(sym), g(ft->file.group);
			//ASSERT( strstr(g, "$ECO")==0 && strstr(sym, "$ECO")==0);
			// special case for global economy data, load only once
			if (IsEcoFile(g))
				if (n>0 || ft->file.iscached())
					continue;
			// flush any pending action
			ft->file.flush();
			// read ahead all files EXCEPT outputs
			if (ft->output) 
				ft->file.reset();
			else
				ft->file.cache(s, g, ft->mindate);
			}
		return TRUE;
	}

	CFileCache *find(const char *sym, const char *group, BOOL write = FALSE)
	{
		if (NOCACHE) return FALSE;
#ifndef USECACHE
		return NULL;
#endif
		if (!nfiles)
			return NULL;

		if (IsEcoFile(group)) // special case for global economy, once loaded once reuse for everyone
			sym = syms[0]; 

		// we neeed to compare same items, so use default
		//ASSERT( strstr(group, "$ECO")==0 && strstr(sym, "$ECO")==0);
		CFileCacheTable findfile;
		CFileCacheTable *pfindfile = &findfile;
		findfile.file.group = group;
		for (int i=0; i<MAXCACHESYM; ++i)
			{
			if (strcmp(sym, syms[i])==0)
				{
				int f = qfind(&pfindfile, files, ngroup, sizeof(*files), cmpft); 
				if (f>=0) 
					return &files[f+i*ngroup]->file;
				}
			}
		if (!write)
			Log(LOGWARN, "unexpected miss in file cache for %s %s", sym, group );
		return NULL;			
	}


	CLineToken *open(CString &sym, CString &group, double mindate)
	{
		CString s = sym, g = group;
		CString filename = DataFileCore(g, s);
		if (filename.IsEmpty())
			return FALSE;
		// do not cache global data, because used only to calc $ECO
		if (IsGlobalFile(filename) && !IsEcoFile(g)) 
			file = NULL;
		else
			file = find(s, g);
		if (DEBUGLOAD>1 && !file && !NOCACHE)
			Log(LOGINFO, "CACHE: MISSED   %s %s [%g] %s", sym, group, mindate, filename);
		if (!file) 
			{
			++cmiss;
			file = &deffile.file; // only ONE non cached file
			}
		else
			{
			++chit;
			}
		if (!file->open(sym, group, mindate))
			return NULL;
#ifdef DEBUGCACHE2
		// compare cache and no cache
		if (file!=&deffile.file)
			{
			file->flush();
			debugfile.open(sym, group, mindate); // use mindate to be consistent with cache
			if (debugfile.omindate != file->omindate)
				Log(LOGALERT, "mindate inconsistency in cache %s %s C%g>O%g (DEBUGCAHE2)", sym, group, debugfile.omindate, file->omindate);
			//return &debugfile.line; //@@@
			}
#endif
		return &file->line;
	}

	const char *getline(void)
	{
		const char *ret = file->getline();
#ifdef DEBUGCACHE2
		// compare cache and no cache
		if (file!=&deffile.file)
			{
			const char *oret = debugfile.getline();
			if (oret!=NULL && ret!=NULL)
				if (strcmp(oret, ret)!=0)
					{
					BOOL mindatebug = debugfile.omindate != file->omindate;
					CString extra;
					if (mindatebug) 
						extra = MkString("<MINDATE!>%s", CDate(debugfile.omindate));
					Log(LOGALERT, "cache synch inconsistency for %s [%s%s] %.10s<>%.10s", file->name(), CDate(file->omindate), extra, ret, oret);
					/*
					CString date = CDate(file->omindate);
					file->line.cseekid(date);
					const char *ret2 = file->getline();
					debugfile.fseekid(date);
					const char *oret2 = debugfile.getline();
					*/
					
					while (ret!=NULL && oret!=NULL && strcmp(ret, oret)!=0)
						{
						int cmp = strcmp(ret,oret);
						if (cmp>0) oret=debugfile.getline();
						if (cmp<0) ret=file->getline();
						}

					}
			if (oret==NULL || ret==NULL)
				if (oret!=ret)
					Log(LOGALERT, "cache synch inconsistency for end of file");
			}
#endif
		return ret;
	}

	BOOL close(void)
	{
#ifdef DEBUGCACHE2
		// compare cache and no cache
		if (file!=&deffile.file)
			debugfile.close();
#endif
		return file->close();
	}



	CLineToken *openw(CString sym, CString group, double mindate)
	{
		CString s = sym, g = group;
		CString filename = DataFileCore(g, s);
		filew = find(s, g, TRUE);
		if (DEBUGLOAD>1) Log(LOGINFO, "CACHE: OPENW %s %s %s", sym, group, filename);
		if (!filew) 
			{
			++cmissw;
			filew = &nocachefile;
#ifdef USEDELAYEDWRITE2
			if (!NOCACHEW)
				{
				filew = &deffilew.file;
				if (!flushw())
					{
					--cmissw;
					++cflushw; 
					}
				}
#endif
			}
		else
			{
			++chitw;
			}

		filew->openw(sym, group, mindate);
		return &filew->line;
	}

	BOOL putheaderw(const char *header)
	{
		return filew->putheaderw(header);
	}

	BOOL putlinew(const char *line)
	{
		return filew->putlinew(line);
	}

	BOOL closew(void)
	{
		return filew->closew();
	}

} *disk = NULL;



CDiskCache::CDiskCache(void)
{
	disk = new CDisk();
}

CDiskCache::~CDiskCache(void)
{
	delete disk;
	disk = NULL;
}

void CDiskCache::Prefetch(const char *sym, int symnum)
{
	disk->prefetch(sym, symnum);
}

void CDiskCache::Reset(CLoadFileList &loadmap, CLoadFileList &savemap)
{
	disk->Reset(loadmap, savemap);
}

void CDiskCache::Flush(void)
{
	disk->flushall();
}

void CDiskCache::Usage(void)
{
		extern int csectionviolation;
		Log(LOGINFO, "THREADV:%d BUFF:%d%%[%d/%d] CACHE:%s READ:%dh/%dm WRITE:%d/%df/%dm", 
			csectionviolation, 
			CProgress::GetAvgBuffering(), (CProgress::GetAvgBuffering()*MAXREADSYM)/100, MAXREADSYM,
			disk->status, disk->chit, disk->cmiss, disk->chitw, disk->cflushw, disk->cmissw );
}






////////////////////////////
// LoadProcess
////////////////////////////

POSITION CLoadFileList::Find(const char *filename)
{
	for (POSITION pos = GetHeadPosition(); pos!=NULL; GetNext(pos))
		if (GetAt(pos).filename==filename)
				return pos;
	return NULL;
}

POSITION CLoadFileList::Add(const char *filename, const char *rulename, int maxdays)
{
	POSITION fpos = Find(filename); 
	// avoid loading files twice, except for special
	if (!fpos)
		{
		// new record
		CLoadFile f(filename, rulename, maxdays);
		fpos = AddTail(f);
		}
	else
		{
		// existing record
		CLoadFile &f = GetAt(fpos);
		if (f.maxdays>=0)
			if (maxdays>f.maxdays || maxdays<0)
				{
				f.rulename = rulename;
				f.maxdays = maxdays;
				}
		}
	return fpos;
}


int CLoadFileList::Summary(const char *title)
{
	// aggregate loadmaps and report
	CString map;
	int maxdays = 0;
	CFILE flog;
	if (DEBUGRULES>0 || DEBUGLOAD>0)
		{
		static int cnt;
		CString file = MkString("DEBUG.%s.LOADMAP#%d.csv", title, ++cnt);
		Log(LOGINFO, DEBUGOUTFILE, file);
		flog.fopen(file,CFILE::MWRITE);
		if (flog.isopen()) flog.fputstr("maxdays,group,rule#,rule");
		}

	POSITION pos = GetHeadPosition();
	while (pos!=NULL)
		{
		CLoadFile &f = GetNext(pos);
		if (f.maxdays>maxdays) maxdays = f.maxdays;
		if (flog.isopen()) flog.fputstr(MkString("%d,%s,%s,%s", f.maxdays, f.filename, GetToken(f.rulename,0,':'), GetToken(f.rulename,1,':')));
		map += MkString("[%s:%dd]", f.filename, f.maxdays);
		}
	if (!title) title = "LOADMAP";
	Log(LOGINFO, "%s: #%d:%dd {%s}", title, GetSize(), maxdays, map);
	return maxdays;
	//return (int)files.GetSize();
}


/////////////////////////////////////// Load Process (Reading Files and Mapping Values)
#define NLOADPASS 2
class CLoadMapList : public CList <CLoadMap,CLoadMap&> {

	POSITION looppos, filepos;
	
public:
	void FindFileMaps(POSITION pos)
	{
		filepos = pos;
		looppos = GetHeadPosition();
	}

	CLoadMap *GetNextMap(void)
	{
		while (looppos!=NULL)
			{
			CLoadMap *map = &GetNext(looppos);
			if (map->filepos == filepos) // only matching file 
				return map;
			}
		return NULL;
	}

};

LoadProcess::LoadProcess(void)
	{
		lowdate = 0;
		highdate = 0;
		lowdatecnt = 0;
		mapref = 0;
		ncount = 0;
		// compute mindate
		//clampdate = 0;
	}

LoadProcess::~LoadProcess(void)
	{
	}


BOOL LoadProcess::open(const char *symbol, const char *fname, double mindate)
	{
		// allow group:sym for referencing files
		lowdatecnt = 0;
		// make sure to not cut off event files (no minimum OR maximum date)
		if (TODAYDATE>0 && !IsEvtFile(fname)) 
			highdate = TODAYDATE;
		line = disk->open(sym = symbol, group = fname, lowdate = mindate);
		if (!line)
			return FALSE;
		return TRUE;
	}
		
BOOL LoadProcess::getline(double &date)
	{
		// normal file
		while (disk->getline()!=NULL) 
			{
			//CString tok = GetToken(line->buffer(),100); 
			//ASSERT(strncmp("2007-05-10", line->buffer(), DATELEN)>0);
			//ASSERT( tok.IsEmpty() );
			date = CGetDate(line->buffer());
			if (date == InvalidDATE)
				{
				Log( LOGALERT, "unexpected invalid date in file %s.%s for %.20s", group, sym, line->buffer());
				return TRUE;
				}
			if (highdate>0 && date>highdate) // simulate other TODAY
				continue;
			if (date<lowdate)
				{
				// check no superflous loading (it should never happen)
				if (!lowdatecnt)
					Log(LOGALERT, "Unexpected load of extra lines for file %s %s for %s<%.20s", group, sym, CDate(lowdate), line->buffer());
				++lowdatecnt; // warn only once
				continue;
				}
			return TRUE; // found a valid line
			}
		return FALSE; //end
	}


void LoadProcess::close(void)
	{
		disk->close();
	}

int LoadProcess::LoadHistoryMap(HISTORY &hist, const char *sym, POSITION filepos, CLoadMapList &maplist, CLoadFile &f)
	{
		PROFILE("LoadHistoryMap()");
		// check if need to do some initialization
		int count = 0;
		maplist.FindFileMaps(filepos);
		while((map = maplist.GetNextMap())!=NULL)
			{
			++count;
			// do load time computations (if any needed)
			if (!map->rule->mapping)
				{
				// lazy computing
				map->rule->Compute(hist, hist.GetDateSize());
				return FALSE;
				}
			
			// do load time initializations (NO! Reset better)
			//map->rule->Invalidate(hist, f.maxdays<0 ? hist.GetDateSize() : f.maxdays, map->rule->sr);
			}
		if (!count) // nothing to do with this file
			return 0;

		// build header map
		BOOL mismatch = FALSE;
		double oldestdate = f.OldestDate();
		//ASSERT( CString(file)!="$@EPS" );
		if (!open(sym, f.filename, oldestdate)) 
			{
			if (DEBUGLOAD>0) Log(LOGINFO, "LOAD: FAILED   %s %s, mindate=%s [%g]", sym, f.filename, CDate(oldestdate), oldestdate);
			return FALSE; // tolerate some files will not exist
			}

		{
		// map names to columns, only done once per Config & file group
		PROFILE("LoadHistoryMap()::MAP");
		maplist.FindFileMaps(filepos);
		while((map = maplist.GetNextMap())!=NULL)
			if (!map->mapped) // only need to map headers once per map
				{
				map->mapped = TRUE;
				int n = map->names.GetSize();
				map->inames.SetSize(n);
				ASSERT( map->inames.GetSize()>0 );
				for (int i=0; i<n; ++i)
					{
					// reset hist to invalid so we know when it's not available
					if ((map->inames[i] = find(map->names[i]))<0)
						if (!CSymID::IsIndex(sym)) // dont warn on index missing data
							Log(LOGALERT, "unexpected map mismatch for %s for rule %s, can't find %s in file %s", sym, map->rule->GetName(), map->names[i], f.filename);
					}
				}
		}

		if (DEBUGLOAD)
			hist.IntegrityCheck("LoadHistoryMap::STARTUP");

		{
		PROFILE("LoadHistoryMap()::STARTUP");
		// startup (if any rule has to initialize, like MapDATE)
		maplist.FindFileMaps(filepos);
		while((map = maplist.GetNextMap())!=NULL)
			map->rule->Map(0, 0);
		}

		{
		PROFILE("LoadHistoryMap()::LOOP");
		// map only between start date & end date? not really
		double date = 0;
		int nline = 0;
		while (getline(date))
			{
			// avoid duplicate dates (sometimes caused by NoWeekend)
			++nline;
			int idate = hist.FindAprox(date);
			maplist.FindFileMaps(filepos);
			while((map = maplist.GetNextMap())!=NULL)
				map->rule->Map((int)date, idate);
			}
		if (DEBUGLOAD>2) Log(LOGINFO, "LOAD: COMPLETED %s %s, mindate=%s [%g], %d lines", sym, f.filename, CDate(oldestdate), oldestdate, nline);
		}

		{
		PROFILE("LoadHistoryMap()::SHUTDOWN");
		// shutdown (if any rule has to shutdown, like MapDATE and MapRANK)
		int maxdays = f.maxdays;
		if (maxdays<=0) maxdays = max (hist.GetDateSize(), 1);
		maplist.FindFileMaps(filepos);
		while((map = maplist.GetNextMap())!=NULL)
			map->rule->Map(0, -maxdays);
		}

		close();
		if (DEBUGLOAD)
			hist.IntegrityCheck("LoadHistoryMap::SHUTDOWN");
		return TRUE;
	}


BOOL SomeLowercase(const char *str)
{
	for (int i=0, n=strlen(str); i<n; ++i)
		if (islower(str[i]))
			return TRUE;
	return FALSE;
}




class RuleLoadSave : public RuleData {

public:


int IntegrityCheck(const char *str)
{
		CSymList list;
		CString prefix;
		list.Add(CSym(HDRDATE,0));
		int maxrules = MAXHRULES;

		for (int j=0; j<maxrules; ++j)
			{
			if (RULES[j].IsInvisible()) continue;
			CString name = RuleName(j,TRUE);

			// check type
			switch(RULES[j].vtype)
				{
				case rSTR:
						if (name[0]!='%' && strstr(name,HDRSYM)==NULL)
							Log(LOGINFO, "WARNING: STRING vs NUM inconsistency header for %s", name);
						if (strstr(name,HDRDATE)!=NULL)
							Log(LOGINFO, "WARNING: STRING vs DATE inconsistency header for %s", name);
						break;
				case rDATE:
						if (strstr(name,HDRDATE)==NULL)
							Log(LOGINFO, "WARNING: DATE postfix missing for %s", name);
						if (name[0]=='%')
							Log(LOGINFO, "WARNING: DATE vs STRING inconsistency header for %s", name);
						break;
				default:
						if (strstr(name,HDRDATE)!=NULL)
							Log(LOGINFO, "WARNING: NUM vs DATE inconsistency header for %s", name);
						if (name[0]=='%')
							Log(LOGINFO, "WARNING: NUM vs STRING inconsistency header for %s", name);
				}

			// avoid duplicated columns
			if (DEBUGRULES>0)
				{
				int found = list.Find(prefix+name);
				if (found>=0)
					{
					int foundj = (int)list[found].n;
					Log(LOGINFO, "WARNING: duplicated column [%s]==[%s] %s (%s)", colstr(j+1), colstr(foundj+1), name, prefix);
					CString rname = name;
					for (int n=1; list.Find(name)>=0; ++n)
						name = MkString("%s_#%d", rname, n);
					}
				if (name.Find(':')>0)
					prefix = GetToken(name,0,':')+":";
				list.Add(CSym(prefix+name,j));
				}
			}
		
	return TRUE;
}




int BuildLoadMap(int numdays, int extradays, CLoadFileList &filelist, CLoadMapList &maplist, BOOL loadval = TRUE)
{
	int m = 0;
	int gmaxdays = numdays + extradays;
	// process all loading maps
	for (int j=0; j<NDEFRULES; ++j)
		if (DEFRULES[j].load==loadval)
			{
			CLoadMap map;
			map.rule = &DEFRULES[j];
			
			// compute maximum days for all columns of this map
			CString rulename;
			int extradays = -1;
			for (int i=map.rule->sr; i<map.rule->er; ++i)
				if (RULES[i].extradays>extradays)
					{
					rulename = MkString("#%d:%s", i, RULES[i].Name(TRUE));
					extradays = RULES[i].extradays;
					}
			int maxdays = numdays + extradays;
			if (loadval<0 && maxdays<gmaxdays) // first load is special
				maxdays = gmaxdays; // has to load enough dates for everyone
			if (numdays<0) 
				maxdays = numdays;

			// accumulate to a unique files list
			CString filename = map.rule->param[0]; 
			if (!map.rule->mapping) // calc rules during load 
				filename = MkString("-R#%d",map.rule->sr);
			map.filepos = filelist.Add(filename, rulename, maxdays);

			// keep a list of mapped contents
			for (int i=1; !map.rule->param[i].IsEmpty(); ++i)
				map.names.AddTail(map.rule->param[i]);
			maplist.AddTail(map);
			++m;
			}

	return m;
}


int BuildHistoryColumnMap(CIntArrayList &rowmap, CIntArrayList &lastref, CIntArrayList &colmap)
{
	int MAX = lastref.GetSize();
	// build last_reference map, MAX for static entries
	// initialize all unreferenced to self referenced
	for (int i=HMAX; i<MAX; ++i)
		lastref[i] = RULES[i].Base()->def->er; 
	// note max last reference for all group
	for (int i=HMAX; i<MINRULES; ++i)
		{
		for (int j=0; j<RULES[i].def->paramlink.GetSize(); ++j)
			{
			int lastr = RULES[i].Base()->def->er;
			int linkr = (int)RULES[i].param[ RULES[i].def->paramlink[j] ];
			ruledef *def = RULES[linkr].Base()->def;
			for (int k=def->sr; k<def->er; ++k)
				if (lastref[k]<lastr)
					lastref[k]=lastr;
			}
		}
	// initialize all static
	for (int i=0; i<MAX; ++i)
		if (i<HMAX || i>=MINRULES || RULES[i].def->weekly || !RULES[i].IsInvisible() || NOHIDE) 
			lastref[i] = MAX;

	// build hist map
	CIntArrayList colref(MAX);
	int nmap = 0;
	// pre-allocate HMAX and load rules
	for (int r=0; r<MAX; ++r) 
		{
		if (r<HMAX || RULES[r].def->load)
			{
			colref[nmap] = lastref[r];
			colmap[r] = nmap++;
			}
		else
			colmap[r] = -1;
		}

	// build column map
	CIntArrayList rowref(MAX);
	for (int r=HMAX; r<MAX; ++r)
	  if (colmap[r]<0)
		{
		// find available slot with closest row requirements
		int minn = -1, minrowdiff = rowmap[0];
		for (int n=HMAX; n<nmap; ++n)
			if (colref[n]<=r)
				{
				int rowdiff = abs( rowref[n]-rowmap[r] );
				if (rowdiff<minrowdiff)
					{
					minn = n;
					minrowdiff = rowdiff;
					}
				}
		// found empty slot or new slot
		if (minn<0)
			{
			// create new slot
			colref[nmap] = lastref[r];
			rowref[nmap] = rowmap[r];
			colmap[r] = nmap++;
			}
		else
			{
			// reuse empty slot
			colref[minn] = lastref[r];
			if (rowref[minn]<rowmap[r])
				rowref[minn] = rowmap[r];
			colmap[r] = minn;
			}
		}

#ifdef DEBUGHISTMAP
	 nmap = MAX;
  	 for (int i=0; i<MAX; ++i)
		colmap[i] = MAX-i-1;
#endif

	// dump debug info
	if (DEBUGRULES || DEBUGLOAD)
		{
		// write maps
		static int cnt = 0;
		CString file = MkString("DEBUG.%s.HISTMAP#%d.csv", group, ++cnt);
		Log(LOGINFO, DEBUGOUTFILE, file);
		CFILE flog; flog.fopen(file,CFILE::MWRITE);
		if (flog.isopen()) flog.fputstr(MkString("R#,lastref,map,rows,load,weekly"));
		for (int i=HMAX; i<MINRULES; ++i)
			if (flog.isopen()) 
				flog.fputstr(MkString("#%d,%d,%d,%d,%d,%d", i, lastref[i], colmap[i], rowmap[i],
						RULES[i].def->load, RULES[i].def->weekly));
		}
	 
	 return nmap;
}

void BuildHistoryRowMap(int numdays, int maxrows, CIntArrayList &rowmap)
{
	int nmap = rowmap.GetSize();

	// init row size
	for (int i=0; i<nmap; ++i)
		rowmap[i] = maxrows;

	// accumulate maximum row size
	int maxnrows = 0;
	for (int i=HMAX; i<MINRULES; i+=RULES[i].def->nr)
		for (int j=RULES[i].def->sr; j<RULES[i].def->er; ++j)
			{
			//ASSERT(j!=722);
			int extra = RULES[i].def->weekly ? 10 : 0; //extra space for unweek
			int nrows = rowmap[j] = numdays+RULES[i].extradays+RULES[i].adddays+extra;
			if (maxnrows<nrows) maxnrows=nrows;
			}
	if (maxnrows<maxrows)
		Log(LOGALERT, "ERROR: Miscalculation in number of MAX rows %d <> %d", maxnrows, maxrows);

	// adjust for load functions based on filelist and mapping loops
	for (int l=0; l<NLOADPASS; ++l)
		{
		CLoadMapList &rlist = maplist[l];
		POSITION pos = filelist.GetHeadPosition();
		while (pos!=NULL)
			{
			// extract proper symbol and id to get filename
			POSITION filepos = pos;
			CLoadFile &f = filelist.GetNext(pos);
			// find all mapping related to this file
			POSITION looppos = rlist.GetHeadPosition();
			while (looppos!=NULL)
				{
				CLoadMap *map = &rlist.GetNext(looppos);
				if (map->filepos == filepos) // only matching file 
					for (int i=map->rule->sr; i<map->rule->er; ++i)
						{
						//ASSERT(i!=722);
						int &nrowsmap = rowmap[i];
						extern rulefunc MapEVT, RuleCMP;
						if (nrowsmap<f.maxdays) 
							nrowsmap=f.maxdays;
						if (f.maxdays<0 || map->rule->func==MapEVT)
							nrowsmap=maxrows;
						}
				}
			}
		}
}



void ShutdownRules(void)
{
	//if (hist) hist.Free(hist);
	hist.Free();
	if (maplist) delete [] maplist;
	maplist = NULL;
}


void StartupRules(int numdays, int extradays, CLoadFileList *listp, BOOL epsdiv)
{
	// not all config use rules and history
	if (numdays==0)
		return;

	IntegrityCheck("StartupRules");

	// build load maps (2 pass)
	maplist = new CLoadMapList[NLOADPASS];
	int loaddays = epsdiv ? -1 : numdays;
	BuildLoadMap(loaddays, extradays, filelist, maplist[0], -1);
	BuildLoadMap(loaddays, extradays, filelist, maplist[1]);

	// some CFG do not use history
	if (filelist.IsEmpty())
		return;

	// build summary loadmap
	CLoadFileList curfilelist;
	POSITION pos = filelist.GetHeadPosition();
	while (pos!=NULL)
		{
		CLoadFile &f = filelist.GetNext(pos);
		CString filename = GetToken(f.filename,0,GROUPSYM);
		curfilelist.Add(filename, f.rulename, f.maxdays);
		if (listp && filename[0]!='-') 
			listp->Add(filename, f.rulename, f.maxdays);
		}
	

	int maxdays = curfilelist.Summary(group);
	if (!maxdays) maxdays = numdays+extradays;
	if (maxdays != numdays+extradays)
		Log(LOGALERT, "ERROR: Miscalculation in MAX number of days %d <> %d", maxdays, numdays+extradays);

	// build history row and column maps
	int MAX = MINRULES + 8;
	CIntArrayList lastref(MAX), colmap(MAX), rowmap(MAX);
	BuildHistoryRowMap(numdays, maxdays, rowmap);
	int nmap = BuildHistoryColumnMap(rowmap, lastref, colmap);

	hist.Malloc(maxdays, nmap, colmap, rowmap, group);
}






int LoadHistory(const char *symbol)
{
	PROFILE("LoadHistory()");

	// reset rule computation
	for (int i=0; i<MINRULES; ++i) 
		RULES[i].computed=FALSE;
	
	hist.StartupLoadProcess();
	// loop for all files 
	for (int i=0; i<NLOADPASS; ++i)
		{
		POSITION pos = filelist.GetHeadPosition();
		while (pos!=NULL)
			{
			// extract proper symbol and id to get filename
			POSITION filepos = pos;
			CLoadFile &f = filelist.GetNext(pos);
			hist.LoadProc()->LoadHistoryMap(hist, symbol, filepos, maplist[i], f);
			}
#ifdef SKIPNOHIST
		// after first pass, if it does not have good history we know its NOT good 
		if (!hist.GetDateSize()) // || hist[0].date()<oldestdate)
			{
			//if (DEBUGLOAD>2) Log(LOGINFO, "WARNING: No history found for symbol %s", symbol);
			break;
			}
#endif
		}

	
	return hist.ShutdownLoadProcess();
}


class DataLine {
	public:
	CLineToken *hist, *rank;

	DataLine(CLineToken *hist, CLineToken *rank)
	{
		this->hist = hist;
		this->rank = rank;
	}


	void Reset(const char *_hist, const char *_rank)
	{
		hist->set(_hist);
		rank->set(_rank);
	}

	void Add(const char *add, int &write, BOOL hdr = FALSE)
	{
		if (!write) return;
		if (write<0) 
			{
			rank->addc(add);
			if (!hdr)
				add = RANKMARK;
			}
		hist->addc(add);
	}

};



CString RuleName(int j, int show = FALSE) 
{
	return ((j<HMAX) ? HHEADERS[j] : RULES[j].Name(show));
}

CString SaveHistoryHeader(void)
{
	int maxrules;
	CIntArrayList write;
	CLineToken hist, rank;
	DataLine hdr(&hist,&rank);
	SaveHistorySetup(maxrules, write, hdr);
	CString str(hdr.hist->buffer());
	return str;
}

void SaveHistorySetup(int &maxrules, CIntArrayList &write, DataLine &hdr)
{
	int j;
	BOOL showmode = rankmode<0 ? -1 : NOHIDE;
	maxrules = MAXHRULES;

	// set up write conditionals: -1 rank, 0 hide, 1 write
	hdr.Reset( sym, HDRSYM );
	write.SetSize(maxrules);
	for (j=0; j<maxrules; ++j)
		{
		int w = RULES[j].IsInvisible() ? 0 : 1; // default
		if (j<HAUX) w = 1; // special cases
		if (j>=MINRULES) w = 0; // special cases
		if (showmode<0) w = 0; // hide all
		if (showmode>0) w = 1; // show all
		if (RULES[j].IsRank()) w = -1; // rank are special
		write[j]= w;

		CString name = RuleName(j, showmode>0);

		hdr.Add(name, write[j], TRUE);
		}
	//for (j=MINRULES; j<maxrules; ++j)
	//	hdr.Add("TMP", write[j]);
}

BOOL ReSaveHistory(const char *sym, const char *group, int maxcount)
{
	ASSERT( dayctx!=NULL );

	double mindate = PreviousDate(maxcount);
	CString curdate = CDate(CurrentDate); // break point in case of TODAYDATE

	CString header;
	CString filename = DataFile(group, sym);

	// open existing file
	CIdxList idx;
	if (!idx.Open(filename, IDXREAD))
		return FALSE;

	// seek to pos and get header
	idx.Find(filename, (int)mindate);
	if (!idx.SeekHeader(header))
		return FALSE;
		
	// header
	if (!SYMMODE)
		dayctx->AddHist(HDRSYM, header, TRUE);
	else
		dayctx->AddDay(HDRDATE, header, TRUE);

	// process everything between lastdate and newdate
	char *histline;
	while (histline=idx.GetLine())
		{
		if (strncmp(histline, curdate, DATELEN)>0)
			break; // out of scope
		if (!SYMMODE)	
			dayctx->AddHist(sym, histline);
		else
			dayctx->AddDay(sym, histline);
		}

	return TRUE;
}

/*
BOOL ReSaveHistory(const char *date, const char *group, CSymList &ticks)
{
	ASSERT( dayctx!=NULL );

	// open existing file
	CString histline;
	CString histfile = DataFile(group, sym);
	CFILE rfile;
	if (!rfile.fopen(histfile, CFILE::MREAD)) 
		return FALSE;
		
	// header
	int num = 0;
	int tick = GetTickCount();
	rfile.fgetstr(histline);
	if (SYMMODE)
		dayctx->AddHist(HDRSYM, histline, TRUE);
	else
		dayctx->AddDay(HDRDATE, histline, TRUE);

	// process everything between lastdate and newdate
	while (rfile.fgetstr(histline)!=NULL)
		{
		// ticks controlled by filter @
		//if (ticks.Find(GetToken(histline,0))<0)
		//	continue;
		if (SYMMODE)
			dayctx->AddHist(date, histline);
		else
			dayctx->AddDay(date, histline);
		++num;
		}

#ifdef DEBUGXXX
	Log( LOGINFO, "file: %s lines: %d: ticks: %d", histfile, num, GetTickCount()-tick);
#endif
	return TRUE;
}
*/

class CSQLDATA : CDATA {
	public:
		const char *SetString(CString &str)
		{
			str.Remove('\"');
			str.Remove('\'');
			if (str.IsEmpty())
				return "NULL";
			return str;
		}

		const char *SetNum(double d)
		{
			if (d==InvalidNUM)
				return "NULL";
			return CDATA::SetNum(data, d);
		}

		const char *SetDate(double d)
		{
			if (d==InvalidDATE)
				return "NULL";
			data[0] ='\'';
			CDATA::SetDate(data+1, d);
			strcat(data, "\'");
			return data;
		}

};




BOOL SaveHistory(const char *sym, const char *group, int maxcount, BOOL overwrite)
{
	if (maxcount<0)
		return FALSE;

	// force to write partial history
	PROFILE("SaveHistory");

	// expand hist (for EPS/DIV)
	double MostCurrentDate = hist.date();
	hist.Expand(maxcount);
#ifdef DEBUG
	CString mostd = CDate(MostCurrentDate);
	CString newd = CDate(hist.date());
#endif

	// 
	CLineToken *line;
	for (int end=0; end<maxcount && hist[end].date()!=InvalidDATE; ++end);
	if (!(line=disk->openw(sym, group, overwrite ? 0 : hist[end-1].date())))
		{	
		Log(LOGALERT, "critical error in openw() %s %s not updated", sym, group);
		return FALSE;
		}

	// headers
	int maxrules;
	CIntArrayList write;
	CLineToken rank;
	DataLine dataline(line, &rank);
	SaveHistorySetup(maxrules, write, dataline);
	//int count = GetTokenCount(hdr.hist);

	// WRITE HEADER ROW
	if (dayctx) dayctx->AddHist(HDRSYM, dataline.hist->buffer(), TRUE);
	if (rankctx && RANKRULES) rankctx->Add(HDRDATE, dataline.rank->buffer(), TRUE);

	// compute end
	disk->putheaderw(dataline.hist->buffer());
	
	// data
	CDATA cdata;
	double lastdate = 0;
	//Progress progress(MkString(">> %s %s",group, sym), 0, maxcount);
	for (int i=end-1; i>=0; --i)
		{
		//progress.Set(i);
		// check date consistency
		double date = hist[i].date();
		if (lastdate>=date)
			Log(LOGALERT, "HIST: Inconsistent dates (not properly sorted) %g>%g", lastdate, date);
		lastdate = date;
#ifdef AVOIDFUTURE
		if (MostCurrentDate>0 && date>MostCurrentDate) 
			continue; // skip invalid dates (future)
#endif
				
		// PROCESS LINE
		dataline.Reset(CDate(date), sym);
		for (int j=0; j<maxrules; ++j)
			{
			if (!write[j]) continue;
#ifdef DEBUGMAXCOLUMNS
			if (GetTokenCount(line.hist)>DEBUGMAXCOLUMNS)
				break;
#endif
			//ASSERT(strstr(RULEHeader(j,TRUE),"CPE")==0);
			switch(RULES[j].vtype)
				{
				case rSTR:
					{
					CString str = hist[i].GetString(j);
					dataline.Add(cdata.SetString(str), write[j]);
					}
					break;
				case rDATE:
					{
					double date = hist[i].data(j);
					if (date==InvalidNUM) date = InvalidDATE;
					dataline.Add(cdata.SetDate(date), write[j]);
					}
					break;
				case rNUM:
				default:
					dataline.Add(cdata.SetNum(hist[i].data(j)), write[j]);
					break;
				}
			}
	
		// WRITE DATA ROW
		// add data to other files
		//ASSERT( dayctx==NULL && hist[i].date()<=validdate);
		if (dayctx) dayctx->AddHist(sym, dataline.hist->buffer());
		if (rankctx && RANKRULES) rankctx->Add(CDate(hist[i].date()), dataline.rank->buffer());
		disk->putlinew(dataline.hist->buffer());
		}

	disk->closew();
	return TRUE;
}


};




int RuleData::LoadHistory(const char *sym)
{
	return ((RuleLoadSave *)this)->LoadHistory(sym);
}

BOOL RuleData::SaveHistory(const char *sym, const char *group, int maxcount, BOOL overwrite)
{
	return ((RuleLoadSave *)this)->SaveHistory(sym, group, maxcount, overwrite);
}

BOOL RuleData::ReSaveHistory(const char *sym, const char *group, int maxcount)
{
	return ((RuleLoadSave *)this)->ReSaveHistory(sym, group, maxcount);
}

/*
BOOL RuleData::ReSaveHistory(const char *date, const char *group, CSymList &ticks)
{
	return ((RuleLoadSave *)this)->ReSaveHistory(date, group, ticks);
}
*/

CString RuleData::SaveHistoryHeader(void)
{
	return ((RuleLoadSave *)this)->SaveHistoryHeader();
}


void RuleData::ShutdownRules(void)
{
	((RuleLoadSave *)this)->ShutdownRules();
}

void RuleData::StartupRules(int maxcount, int extradays, CLoadFileList *loadmap, BOOL epsdiv)
{
	((RuleLoadSave *)this)->StartupRules(maxcount, extradays, loadmap, epsdiv);
}









