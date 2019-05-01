#include "stdafx.h"
#include "atlstr.h"
#include "msxml.h"
#include "math.h"
#include "TradeMaster.h"

#define SECTORSIZE 512
#define SYMBLOCK 1000
#define LINEBLOCK 500

//#define USEFIXHEADER	// fix header on the fly for DATA files (better not)

/*
 * This is the routine used to calculate the 32 bit CRC of a block of data.
 * This is done by processing the input buffer using the coefficient table
 * that was created when the program was initialized.  This routine takes
 * an input value as a seed, so that a running calculation of the CRC can
 * be used as blocks are read and written.
 */

unsigned int CalculateCRC( const char *buffer, unsigned int count, unsigned int crc )
{
#define BASE_HEADER_SIZE   19
#define CRC_MASK           0xFFFFFFFFL
#define CRC32_POLYNOMIAL   0xEDB88320L
#define NUMMAXFILES        1000

	static int init = 0;
  /* table used to calculate the 32 */
  /* bit CRC values.                */
	static unsigned int Ccitt32Table[ 256 ];    /* This array holds the CRC	*/
	if (!init)
		{
			/*
			 * This routine simply builds the coefficient table used to calculate
			 * 32 bit CRC values throughout this program.  The 256 long word table
			 * has to be set up once when the program starts.  Alternatively, the
			 * values could be hard coded in, which would offer a miniscule improvement
			 * in overall performance of the program.
			 */
			int i;
			int j;
			unsigned int value;
			init  = TRUE;
			for ( i = 0; i <= 255 ; i++ ) {
				value = i;
				for ( j = 8 ; j > 0; j-- ) {
					if ( value & 1 )
					value = ( value >> 1 ) ^ CRC32_POLYNOMIAL;
					else
					value >>= 1;
				}
				Ccitt32Table[ i ] = value;
			}
		}
    unsigned char *p = (unsigned char *) buffer;
    unsigned int temp1;
    unsigned int temp2;

    while ( count-- != 0 ) {
	temp1 = ( crc >> 8 ) & 0x00FFFFFFL;
	temp2 = Ccitt32Table[ ( (int) crc ^ *p++ ) & 0xff ];
	crc = temp1 ^ temp2;
    }
    return( crc );
}





inline int ucmp(unsigned a, unsigned b) 
{	//a-b
	if (a<b) return -1;
	else if (a>b) return 1;
	else return 0;
}



// -1 = ERROR   0..high = found  high+1 = not found
inline int CSEEKID::seekidx(unsigned date, int _low, int _high)
{
		//int low, high;
		// first line
		register int lowdate;
		if (!(lowdate=getidx(low=_low)))
			return -1;
		// check first
		register int cmp = ucmp(lowdate,date);
		if (cmp>=0) 
			// out of upper range but good
			return low;

		// read last
		register int highdate;
		if (!(highdate=getidx(high=_high)))
			return -1;
		// check last
		cmp = ucmp(highdate,date);
		if (cmp==0)
			//found it! last one
			return high;
		if (cmp<0) 
			// out of lower range
			return high+1; 

		++low;
		register int pos = -1;
		// quick find with bisectrix 
		while ( high > low)
			{
			// assisted bisectrix // std: pos = (high+low) / 2;
			pos = low + (high-low) * (date-lowdate) / (highdate-lowdate);
			ASSERT( pos>=low && pos<=high );

			register int posdate;
			if (!(posdate = getidx(pos)))
				return -1;
			cmp = ucmp(posdate,date);
			if (cmp==0) 
				return pos; 
			else if (cmp>0) 
				high = pos, highdate = posdate;
			else if (cmp<0)
				low = pos+1, lowdate = posdate+1;
			}
		// FOUND IT!!!!
		if (pos!=low) 
			getidx(pos=low);
		return pos;
}

#define ASSERTIDX(x) ASSERT(x.date>0 && x.seekpos>0 && x.seeklen>0)

// find seekpos
#define IDXSIZE ((int)sizeof(Idx))
class CFSEEKID : public CSEEKID {
	CFILE *f;
	Idx idxbuffer[SECTORSIZE/IDXSIZE+1];  // load last sector in full
	int imemory;
	public:
	Idx idx, idxfirst, idxlast;
	int ilast, ifirst;
	
	inline CFSEEKID(CFILE *_f)
	{
		f = _f;
		ifirst = 0;
		ilast = 0;
		imemory = -1;
	}


	inline BOOL loadlastchunk(int l, int h)
	{
		ASSERT(l<=h);
		if (l>h)
			{
			Log(LOGALERT, "loadlaschunck() malfunction l:%d h:%d", l, h);
			return 0;
			}
		register int sfirst = (l*IDXSIZE)/SECTORSIZE;
		register int slast = ((h+1)*IDXSIZE)/SECTORSIZE;
		if (sfirst!=slast) // different sector
			return FALSE;

		// load in memory
		f->fseek((imemory = l)*IDXSIZE, SEEK_SET);
		return f->fread(&idxbuffer, (h-l+1)*IDXSIZE, 1)==1;
	}

	inline int getidx(int idxpos)
	{
		if (imemory>=0)
			{
			idx  = idxbuffer[idxpos-imemory];
			ASSERTIDX( idx );
			return idx.date;
			}

		if (idxpos<=ifirst)
			{
			if (idxpos<ifirst) // extra checks
				Log(LOGALERT, "IDX: SEEKID malfunctioning!!! %d<%d", idxpos, ifirst);
			idx = idxfirst;
			ASSERTIDX( idx );
			return idx.date;
			}
		else if (idxpos>=ilast)
			{
			if (idxpos>ilast) // extra checks
				Log(LOGALERT, "IDX: SEEKID malfunctioning!!! %d>%d", idxpos, ilast);
			idx = idxlast;
			ASSERTIDX( idx );
			return idx.date;
			}

		if (loadlastchunk(low, high))
			{
			idx = idxbuffer[idxpos-imemory];
			ASSERTIDX( idx );
			return idx.date;
			}

		f->fseek(idxpos*IDXSIZE, SEEK_SET); 
		if (f->fread(&idx,IDXSIZE,1)!=1)
			return 0;
		ASSERTIDX( idx );
		return idx.date;
	}


	inline int seekidx(unsigned date)
	{
		// compute ilast (needed)
		if (ilast<=0)
			{
			int size = f->fsize();
			int isize = size/IDXSIZE;
			// gratuitos check integrity
			if (isize * IDXSIZE != size )
				{
				Log(LOGALERT, "IDX: Corrupted IDX file %s (%d*IDXSIZE!=%d)", f->name(), isize, size);
				return -1;
				}
			ilast = isize-1;
			}
		
		// if small # idx better to load at once
		if (loadlastchunk(ifirst, ilast))
			{
			idxfirst = idxbuffer[ifirst-imemory];
			idxlast = idxbuffer[ilast-imemory];
			}
		else
			{
			// read first and last
			if (ifirst>0)
			   f->fseek(ifirst*IDXSIZE, SEEK_SET); 
			if (f->fread(&idxfirst,IDXSIZE,1)!=1)
				return -1;
			ASSERTIDX( idxfirst );
			f->fseek(ilast*IDXSIZE, SEEK_SET); 
			if (f->fread(&idxlast,IDXSIZE,1)!=1)
				return -1;
			ASSERTIDX( idxlast );
			}

		return CSEEKID::seekidx(date, ifirst, ilast);
	}
};


// ============================= CSymID ===================================


int cmpticker( const void *arg1, const void *arg2 )
{
	return ((CSym**)arg1)[0]->id.Compare(((CSym**)arg2)[0]->id);
//	return CSymID::Compare(((CSym**)arg1)[0]->id, ((CSym**)arg2)[0]->id);
}

CTickerList::CTickerList(int maxcount) : CSymList(maxcount)
{
	cmpfunc = cmpticker;
}



BOOL CTickerList::Add(CSym &newsym)
{
	if (CSymID::Validate(newsym.id))
		return CSymList::Add(newsym);
	return FALSE;
}


/*
CString GetSymbol(const char *osym, BOOL mapsym)
{
	CString symbol = osym ? osym : "";
	if (mapsym) osym = NewSymbol(symbol);
	return CSymID::ID(symbol);
}
*/

//@@@ use symbol translation tables


CString CSymID::Filename(const char *sym)
{
	CString file(sym);
	file.Replace("=X","");
	file.Replace("^","");
	return file;
}


BOOL CSymID::Migrate(CString &str, CString &cmp)
{
	// check for data consistency
	if (Compare(str, cmp)!=0)
		return FALSE;

	// keep longest post-dot
	if (cmp.GetLength()>str.GetLength())
		str = cmp;

	return TRUE;
}

int CSymID::Compare(const char *sym, const char *line, int token) //@@@ use symbol translation tables
		{
		/*
		for (register int i=0; !CSymIDEnd(carg1[i]); ++i)
			{
			char c1 = carg1[i]; if (c1==' ') c1='_';
			char c2 = carg2[i]; if (c2==' ') c2='_';
			if (c1!=carg2[i])
				break;
			}
		if (CSymIDEnd(carg1[i]) && CSymIDEnd(carg2[i]))
			return 0;
		*/
		return strcmp(sym, GetToken(line, token));
		}

BOOL CSymID::Validate(CString &sym)
{
	if (sym.IsEmpty())
		return FALSE;
	// patch for "FOMC Minutes" vs "FOMC_Minutes"
	//CString str = GetToken(sym, 0, ','); // no .
	sym.Replace(' ', '_');
	
	for (int i=0, n=sym.GetLength(); i<n; ++i)
		{
		char c = sym[i];
		if (c=='-' || c=='/' || c=='/' || c=='&' || c=='.')
			return FALSE;
		}
	/// TODO: VIA-B ==> VIA.B for Sym2 but does not work for all
	return TRUE;
	/*
	CString dot = GetToken(sym, 1, '.');
	if (dot.Compare("N")==0)
		{
		// take out the "N" if any present
		sym = GetToken(sym, 0, '.');
		dot = "";
		}
	return dot.IsEmpty(); // || dot.Compare("PK")==0 || dot.Compare("OB")==0;
	*/
}

BOOL CSymID::IsIndex(const char *symbol)
{
	const char *indexes[] = { 
		"^GSPC","^IXIC","^DJI","^RUT", 
		"^IRX", "^FVX", "^TNX", "^TYX",
		NULL};
	return IsMatch(symbol, indexes);
}

BOOL CSymID::IsFX(const char *symbol)
{
	// error for currencies or indexes
	return strchr(symbol,'=')!=NULL;
}

BOOL CSymID::IsStock(const char *symbol)
{
	// error for currencies or indexes
	if (IsMatch(symbol[0],"^#")) return FALSE;
	if (IsFX(symbol)) return FALSE;
	return TRUE;
}

// ============================= CSym ===================================

BOOL ValidStockDay(const char *file)
{
	CFILE f;
	if (f.fopen(file, CFILE::MREAD)==NULL) 
		return FALSE;

	int i = 0;
	char *line = f.fgetstr();
	BOOL intrate = FALSE, stock = FALSE;
	while (line=f.fgetstr())
		{
		++i;
		if (GetToken(line,0).CompareNoCase(SYMINTRATE)==0)
			intrate = TRUE;

		if (CSymID::IsStock(GetToken(line,0)))
			stock = TRUE;

		if (intrate && stock) // contains stock data
			return TRUE;
		}

	return FALSE;
}

CString LatestDay(double date, const char *folder)
{
	// find  a file to open, max 10 days old
	CString file;
	if (date==InvalidDATE)
		date = CurrentDate;
	if (folder==NULL || *folder==0)
		folder = "DAY";
	for (int i=0; i<=10; ++i)
		{
		file = MkString(STATPATH"%s\\%s.csv", folder, CDate(date-i));
		//if (CFILE::exist(file))
		if (ValidStockDay(file))
			return file;
		}
	return "";
}

// ============================= CSym ===================================



CSym::CSym(CSym &sym)
		{
			Clear();
			Merge(sym);
		}

CSym::CSym(const char *str, const char *datastr)
		{
			Clear(str);
			data = datastr;
			n = max(1, GetTokenCount(datastr));
		}

CSym::CSym(const char *str, int num)
		{
			Clear(str);
			n = num;
		}

CSym::~CSym()
		{
		}

void CSym::Clear(const char *str)
		{
			n = 0;
			index = 0;
			id.Empty();
			data.Empty();
			
			if (str) id = str;
		}

CSym &CSym::operator=(CSym &sym) 
		{ 
			Clear();
			Merge(sym);
			return *this;
		}

void CSym::SetNum(int i, double num)
	{
	PROFILE("CSym::SetNum()");
	SetStr(i, CData(num));
	}


void CSym::SetDate(int i, double num)
	{
	PROFILE("CSym::SetDate()");
	SetStr(i, CDate(num));
	}

void CSym::AddNum(int i, double num)
	{
	double v = GetNum(i);
	if (v==InvalidNUM) v = 0;
	SetNum(i, v+num);
	}

void CSym::MulNum(int i, double num)
	{
	double v = GetNum(i);
	if (v==InvalidNUM || num==InvalidNUM) 
		return;
	SetNum(i, v*num);
	}

void CSym::Insert(int i, CSym &sym)
	{
	if (i>=n)
		{
		while (n<=i)
			data += ",", ++n;

		data += sym.data;
		n += sym.n;
		return;
		}

	// do one by one
	for (int j=0; j<sym.n; ++j)
		SetStr(i+j, sym.GetStr(j));
	}


void CSym::SetStr(int i, const char *str)
	{
	PROFILE("CSym::SetStr()");
	if (i>=n) 
		{
		// extend
		n=GetTokenCount(data);
		if (i>=n) {
			if (n<1) n=1;
			while (n<=i)
				data += ",", ++n;
			data.Append(str);
			return;
			}
		}

	// replace
	int ipos = GetTokenPos(data, i);
	int ilen = GetToken(data, i).GetLength();
	data.Delete(ipos, ilen);
	data.Insert(ipos, str);
	}

CString CSym::GetStr(int i) 
	{ 
		PROFILE("CSym::GetStr()");
		return GetToken(data,i); 
	}

double CSym::GetNum(int i) 
	{ 
		PROFILE("CSym::GetNum()");
		return CGetNum(GetStr(i), InvalidNUM); 
	}

double CSym::GetDate(int i) 
	{ 
		PROFILE("CSym::GetDate()");
		CString date = GetStr(i);
		if (date.IsEmpty())
			return InvalidDATE;
		return CGetDate(date); 
	}

/*
CSym::operator double(void)
	{ 
		return CGetNum(str);
	}
*/	
BOOL CSym::IsEmpty(void)
		{
			return id.IsEmpty();
		}

void CSym::Empty(void)
		{
			Clear();
		}

int CSym::Merge( CSym &sym )
		{
			id = sym.id;
			if (n>0)
				for (int i=GetTokenCount(data); i<=n; ++i)
					data += ","; // padding
			data += sym.data;
			index = sym.index;
			return n = n+sym.n;
		}

int CSym::Load(const char *_id, const char *_data)
		{
			//if (!_id || !*_id) 
			//	return FALSE;

			id = _id; 
			data = _data;
			n = max(n, GetTokenCount(data));
			return n;
		}

int CSym::Load(const char *line)
		{
			if (!line || !*line) 
				return FALSE;

			return Load(GetToken(line,0), line+GetTokenPos(line,1));
		}

BOOL CSym::Valid(void)
		{
			return GetTokenCount(data)==n;
		}
		
CString CSym::Save(void)
		{
			if (data.IsEmpty()) return id;
			return id+","+data; 
		}

BOOL CSym::Save(CString symfile)
		{
			CFILE file;
			if (file.fopen(symfile, CFILE::MAPPEND) == NULL) 
				{
				Log(LOGERR, "Could not append to file %s", symfile);
				return FALSE;
				}
			file.fputstr(Save());
			return TRUE;
		}









// ============================= CSymList ===================================

BOOL CSymList::Add(CSym &newsym)
		{
				if (scount>=scountmax)
					slist = (CSym **)realloc(slist, (scountmax += SYMBLOCK) * sizeof(*slist));
				slist[scount++] = new CSym(newsym);
				sorted = FALSE;
				return TRUE;
		}


BOOL CSymList::AddUnique(CSym &newsym)
		{
				PROFILE("CSymList::AddUnique()");
				// no dup
				if (Find(newsym.id)>=0)
					return FALSE;
				int ret = Add(newsym);
				return ret;
		}

BOOL CSymList::AddUnique(CSymList &list)
		{
		int count = 0;
		for (int n=0; n<list.GetSize(); ++n)
			count += AddUnique(list[n]);
		return count;
		}

BOOL CSymList::Add(CSymList &list)
		{
		int count = 0;
		for (int n=0; n<list.GetSize(); ++n)
			count += Add(list[n]);
		return count;
		}



static int cmpcsym( const CSym **arg1, const CSym **arg2 )
		{
		   return strcmp(arg1[0]->id, arg2[0]->id);
		}

static int cmpcsymi( const CSym **arg1, const CSym **arg2 )
		{
		   return stricmp(arg1[0]->id, arg2[0]->id);
		}

void CSymList::Init(int _maxcount)
		{
			// set up usage counters
			sorted = FALSE;
			maxcount = _maxcount;
			scount = scountmax = 0;
			slist = NULL;
			cmpfunc = (qfunc *)cmpcsym;
		}


CSymList::CSymList(int maxcount)
		{
			Init(maxcount);
		}

CSymList::CSymList(CSymList &list) 
		{
		Init(list.maxcount);
		for (int n=0; n<list.GetSize(); ++n)
			Add(list[n]);
		}


CSymList::~CSymList()
		{
		if (slist)
			{
			for (int i=0; i<scount; ++i)
				delete slist[i];
			free(slist);
			}
		}

CSymList &CSymList::operator=(CSymList &symlist)
		{
		Empty();
		for (int i=0; i<symlist.GetSize(); ++i)
			Add(symlist[i]);
		return *this;
		}


void CSymList::Empty()
		{
		if (!slist) return;
		for (int i=0; i<scount; ++i)
			delete slist[i];
		scount = 0;
		}
			
BOOL CSymList::Delete(int pos)
		{
				delete slist[pos];
				for (int i=pos+1; i<scount; ++i)
					slist[i-1]=slist[i];
				--scount;
				return TRUE;
		}

BOOL CSymList::Delete(const CSym &newsym)
		{
				int pos;
				if ((pos = Find(newsym.id))<0) 
					return FALSE;
				return Delete(pos);
		}

CString CSV(const char *line)
{
	int comma = 0;
	CString ret(line);
	for (int i=0; i<ret.GetLength(); ++i)
		{
		if (ret[i]=='\"' && (i==0 || ret[i-1]==','))
			{
			comma = TRUE;
			ret.Delete(i--);
			continue;
			}
		if (comma && ret[i]=='\"' && (ret[i+1]==0 || ret[i+1]==','))
			{
			comma = FALSE;
			ret.Delete(i--);
			continue;
			}
		if (comma && ret[i]==',')
			ret.SetAt(i,';');
		}
	return ret;
}


int CSymList::Load(const char *sfile, BOOL alreadyunique, BOOL commaparsing)
		{
			if (!sfile || *sfile==0)
				return FALSE;

			PROFILE("CSymList::Load()");
			CString symfile(sfile);
			symfile.Trim();
			// symlist=file.sym.csv
			if (symfile.Right(4).CompareNoCase(".csv")!=0)
				{
				// symlist=HPQ+GOOG+HIT.X+...
				const char sep = '+';
				for (int i=0, n=GetTokenCount(symfile, sep); i<n; ++i)
					Add(CSym(GetToken(symfile,i,sep)));
				return GetSize();
				}

			CFILE file;
			if (file.fopen(symfile, CFILE::MREAD) == NULL) 
				{
				Log(LOGERR, "Could not read sym file %s", symfile);
				return FALSE;
				}

			// skip header
			int count = 0;
			char *line = file.fgetstr();
			if (line) header = line;

			while (line = file.fgetstr())
				{
				CSym newsym;
				if (!newsym.Load(commaparsing ? CSV(line) : line))
					continue;

				// add/delete symbol
				BOOL ret = FALSE;
				if (alreadyunique) 
					ret = Add(newsym); 
				else
					ret = AddUnique(newsym);
				if (ret) ++count;
				}

			return count;
		}


int CSymList::MaxSize(void)
		{
		int maxsize = 0;
		for (int i=0; i<scount; ++i)
			{
			CString str = slist[i]->Save();
			maxsize = max(maxsize, str.GetLength());
			}
		return maxsize;
		}

int CSymList::Save(const char *sfile, BOOL withdata, BOOL hash, BOOL append)
		{
			// check append is possible
			if (append && !CFILE::exist(sfile))
				append = FALSE;

			PROFILE("CSymList::Save()");
			// save new unified list
			CFILE file;
			if (file.fopen(sfile, append ? CFILE::MAPPEND : CFILE::MWRITE) == NULL) 
				{
				Log(LOGERR, "Could not write to file %s", sfile);
				return FALSE;
				}

			// hashed output
			int linesize = 0;
			if (hash)
				{	
				Sort();
				linesize = MaxSize();
				ASSERT( linesize > 0 );
				const char *restheader = strchr(header,',');
				header = MkString("#%d#", linesize) + restheader ? restheader : "";
				}

			int n = 0;
			if (!append)
				file.fputstr((withdata && !header.IsEmpty()) ? header : sfile);
			for (int i=0; i<scount; ++i)
				{
				CString str = slist[i]->Save();
				if (str.IsEmpty()) continue;
				if (!withdata) 
					str = GetToken(str,0);
				if (linesize>0) 
					{
					// hashed output
					int extend = linesize - str.GetLength();
					ASSERT( extend >= 0 );
					if (extend>0)
						str += CString(' ', extend);
					}
				
				++n, file.fputstr(str);
				}
			return i;
		}



void CSymList::Sort(qfunc *func)
		{
		qsort(slist, scount, sizeof(*slist), func);
		sorted = FALSE;
		}

void CSymList::iSort(void)
		{
		if (sorted)
			if (cmpfunc == (qfunc *)cmpcsymi)
				return;
		cmpfunc = (qfunc *)cmpcsymi;
		Sort();
		}

void CSymList::Sort(void)
		{
			if (sorted) return;
			PROFILE("CSymList::Sort()");
			qsort(slist, scount, sizeof(*slist), cmpfunc);			
			sorted = TRUE;
		}

void CSymList::SortNum(int i, int order)
		{
			PROFILE("CSymList::Sort(i)");
			for (int s=0; s<scount; ++s)
				{
				double v;
				if (i<0)
					v = CGetNum(slist[s]->id);
				else
					v = slist[s]->GetNum(i);
				if (v!=InvalidNUM)
					slist[s]->index = order * v;
				else
					slist[s]->index = InvalidNUM;
				}
			SortIndex();
		}

void CSymList::SortIndex(void)
		{
			qsort(slist, scount, sizeof(*slist), cmpindex);
			sorted = FALSE;
		}		

int CSymList::Find(const char *str, int sstart)
		{
			if (sstart>=scount)
				return -1;

			PROFILE("CSymList::FindFunc()");
			CSym sym(str);
			CSym *psym = &sym;
			if (sorted)
				return qfind(&psym, &slist[sstart], scount-sstart, sizeof(*slist), cmpfunc); 

			for (int i=sstart; i<scount; ++i)
				if (cmpfunc(&psym, &slist[i]) == 0)
					return i;
			return -1;
		}

int CSymList::FindColumn(int col, const char *str, int start)
	{ 	
		for (int i=start; i<scount; ++i)
			if (strcmp(slist[i]->GetStr(col), str)==0)
				return i;
		return -1;
	}


/*
BOOL CSymList::AddUniqueDot(CSym &newsym)
	{
	int i = FindDot(newsym.id);
	if (i<0) return Add(newsym);

	// check for data consistency
	if (CSymID::Compare(newsym.data, slist[i]->data)!=0)
		return FALSE;

	// keep longest post-dot
	if (newsym.id.GetLength()>slist[i]->id.GetLength())
		slist[i]->id = newsym.id;
	if (newsym.data.GetLength()>slist[i]->data.GetLength())
		slist[i]->data = newsym.data;
	return TRUE;
	}
*/


int CSymList::cmpindex( const void *arg1, const void *arg2 )
		{
			double i1 = ((CSym**)arg1)[0]->index;
			double i2 = ((CSym**)arg2)[0]->index;
			if (i1==i2)
				return 0;
			if (i1==InvalidNUM)
				return -1;
			if (i2==InvalidNUM)
				return 1;
			if (i1>i2)
			   return 1;
			return -1;
		}


int DEBUGRANK = 0;
int CSymList::VerifySort(int i)
		{
		int err = 0;
		CString col = GetToken(header, i+1);
		// verify that rank is proper
		for (int j=0; j<scount-1; ++j)
			{
			CString s1 = slist[j]->GetStr(i);
			CString s2 = slist[j+1]->GetStr(i);
			double v1 = slist[j]->GetNum(i);
			double v2 = slist[j+1]->GetNum(i);
			if (v1==v2) continue;
			if (v2!=InvalidNUM && v1==InvalidNUM)
				continue;
			if (v2==InvalidNUM && v1!=InvalidNUM)
				{
				Log(LOGALERT, "RANK: improper sort of invalids %s => %s[%s]>ERR>%s[%s]",
					col, s1, slist[j]->id, s2, slist[j+1]->id);													
				++err;
				continue;
				}
			if (v1<v2)
				continue;
			Log(LOGALERT, "RANK: improper sorted value %s => %s[%s]>ERR>%s[%s]",
				col, s1, slist[j]->id, s2, slist[j+1]->id);							
			++err;
			}
		return err;
		}

void CSymList::Rank(void)
		{
			PROFILE("CSymList::Rank()");
			if (!scount) return;

			// rank every data parameter
			int n = slist[0]->n;
			for (int i=0; i<n; ++i)
				{
				// sort by field i
				SortNum(i);
				
				if (DEBUGRANK) VerifySort(i);

				// full rank 0.00-0.99
				int sinv = 0;
				double val = InvalidNUM;
				double rval = InvalidNUM;
				for (int s=0; s<scount; ++s)
					{
					//double v = slist[s]->index;
					double v = slist[s]->GetNum(i);
					if (v==InvalidNUM)
						{
						// do not rank invalids
						++sinv;
						continue;
						}
					if (v==val)
						v = rval;
					else
						val = v, rval = v = ((s-sinv)*10.0)/(scount-sinv);
					slist[s]->SetNum(i, DEBUGRANK ? val : v);
					}
				}			
		}


















// ============================= CLineIdx ===================================



unsigned CIdxList::CRC(CString &sym)
	{
		return CalculateCRC(sym, sym.GetLength(), 0);
	}

unsigned CIdxList::DATE(const char *dateid)
	{
		return (unsigned)CGetDate(dateid);
	}

CIdxList::CIdxList(void)
	{
		Reset();
		idg = NULL;
		idgmode = FALSE;
		grpmode = TRUE;
		crccol = -1;
	}

CIdxList::~CIdxList(void)
	{
		Close();
	}

CString CIdxList::Filename(const char *filename, BOOL idgmode)
	{
		return GetFileNoExt(filename) + (idgmode ? ".idx.idx" : ".idx");
	}


void CIdxList::Reset(void)
	{		
		SetSize(0);
		seekpos = seeksize = -1;
		idxpos = idxsize = -1;
		lastid = 0;	
		idxskip = 0;
		lastid = 0;
	}

CIdxList &CIdxList::operator=(CIdxList &src)
	{
		Reset();
		idxsize = src.GetSize();
		SetSize(idxsize);
		memcpy(&Head(),&src.Head(),idxsize*IDXSIZE);
		return *this;
	}

BOOL CIdxList::Load(const char *filename, int lastlines)
	{
		Reset();
		CString file = Filename(filename);
		CFILE f; ;
		if (!f.fopen(file, CFILE::MREAD)) 
			{
			//Log(LOGALERT, "Could not read IDX file %s", file);
			return FALSE;
			}

		// find out size
		int size = f.fsize();
		idxsize = size / IDXSIZE;
		if (idxsize* IDXSIZE != size || !idxsize)
			{
			Log(LOGALERT, "IDX: Corrupted IDX file %s (%d*IDXSIZE!=%d)", file, idxsize, size);
			return FALSE;
			}

		// load all or just last lines
		if (lastlines>0) 
			f.fseek((idxsize=lastlines)*-IDXSIZE, SEEK_END); 
		else
			f.fseek(0, SEEK_SET); 

		SetSize(idxsize);
		if ((int)f.fread(&Head(),IDXSIZE,idxsize)<idxsize)
			{
			Log(LOGALERT, "IDX: Can't read IDX file %s", file);
			return FALSE;
			}

		return TRUE;
	}

BOOL CIdxList::IntegrityCheck(const char *filename)
	{
		CIdxList idx;
		idx.idgmode = idgmode;
		idx.crccol = crccol;

		if (!idx.Load(filename))
			return TRUE; // no idx
		
		if (idx.InternalCheck()>0)
			{
			if (DEBUGLOAD)
				Log(LOGALERT, "IDX: internal errors detected in IDX for %s", filename);
			return FALSE;
			}
		
		if (idgmode)
			return TRUE;

		// do not rebuild idg indexes
		Rebuild(filename, FALSE);

		if (GetSize()!=idx.GetSize())
			{
			if (DEBUGLOAD)
				Log(LOGALERT, "IDX: IDX not proper size %d<>%d", GetSize(), idx.GetSize());
			return FALSE;
			}

		if (crccol>=0)
			return TRUE;

		for (int i=0; i<GetSize(); ++i)
			if (memcmp(&operator[](i),&idx[i],IDXSIZE)!=0)
				{
				if (DEBUGLOAD)
					Log(LOGALERT, "IDX: IDX different from rebuild for #%d", i);
				return FALSE;
				}

		return TRUE;
	}

int CIdxList::InternalCheck(void)
	{
		if (GetSize()<=0)
			return 0;

		// integrity check
		int err = 0;
		unsigned lastdate=0, date=0;
		int seekpos = Head().seekpos;
		for (int i=0; i<GetSize() && !err; ++i)
			{
			Idx &idx =  operator[](i);
			date = idx.date;
			if (!idgmode && crccol<0 && date<=lastdate)
				{
				++err;
				Log(LOGALERT, "IDX: source was not sorted?!? #%d [%s] %d<=%d [%s]", i, CDate(date), date, lastdate, CDate(lastdate));
				}
			if (idx.seekpos!=seekpos)
				{
				++err;
				Log(LOGALERT, "IDX: wrong seekpos! #%d %d!=%d", i, idx.seekpos, seekpos);
				}

			seekpos = idx.seekend();
			lastdate = date;
			}

		return err;
	}

BOOL CIdxList::Rebuild(const char *filename, BOOL save)
	{
		// rebuild index from scratch, 
		Reset();
		CFILE f, fi;
		if (!f.fopen(filename, CFILE::MREAD))  // we should lock the file, but we assume ronly is ok for this
			return FALSE;  // master file does not exist! return!
		if (save)
			{
			DeleteFile(Filename(filename));
			// lock both files while rebuilding
			Log(LOGWARN, "IDX: Rebuilding IDX for %s", filename);
			CString file = Filename(filename);
			if (!fi.fopen(file, CFILE::MWRITE)) 
				{
				Log(LOGALERT, "IDX: Could not write IDX file %s", file);
				return FALSE;
				}
			}

		// skip header
		f.fseek(0,SEEK_SET);
		char *buffer = f.fgetstr();
		// read lines and add to idx
		seekpos = f.ftell();
		while (buffer=f.fgetstr())
			{
			seekpos += Add(seekpos, buffer);
			ASSERT( seekpos == f.ftell() );
#ifdef DEBUGXXX // debug bad things
			int seek = f.ftell();
			//f.fseek(seek, SEEK_SET);
			if (!f.fgetstr( buffer))
				break;
			int len = strlen(buffer);
			char *check = (char*)malloc(len+10);
			f.fseek(seek, SEEK_SET);
			f.fread(check, len+5, 1);
			if (strncmp(buffer, check, len)!=0)
				{
				for (int i=0; i<len && buffer[i]==check[i]; ++i);
				Log(LOGALERT, "something's wrong with indexing file %s, line %d, pos %d+%d=%d", filename, GetSize(), seek, i, seek+i);
				}
			free(check);
			f.fseek(seek, SEEK_SET);
#endif
			}
		// bonus integrity check
		if (seekpos!=f.ftell())
			Log(LOGALERT, "IDX: integrity check failed rebuilding for %s", filename);

		// quick check
		if (InternalCheck()>0)
			Log(LOGALERT, "IDX: errors detected in IDX for %s", filename);

		if (!save)
			return TRUE;

		// write new records in proper position
		if (GetSize()>0)
			fi.fwrite(&Head(),IDXSIZE,GetSize());

		return TRUE;
	}




BOOL CIdxList::Save(CFILE &f)
	{
		// is there anything to update?
		if (GetSize()<=0)
			return TRUE;

		// if don't know what to do, find if something needs to be updated
		BOOL rewrite = FALSE;
		if (idxpos<0 || seekpos<0)
			{
			rewrite = TRUE;
			idxpos = idxskip = idxsize = 0;			 
			}
		
		// adjust skip and trunc
		if (idxskip<0) 
			idxskip = 0;
		if (idxpos+idxskip>idxsize)
			idxskip = idxsize-idxpos;
		int idxnewsize = idxpos+GetSize();

		// write new records in proper position
		f.fseek((idxpos+idxskip) * IDXSIZE, SEEK_SET); 
		f.fwrite(&Head()+idxskip,IDXSIZE,GetSize()-idxskip);
		if (idxnewsize<idxsize || rewrite)
			f.ftruncate(idxnewsize*IDXSIZE);
		f.fclose();

		// only check (not rebuilding)
		if (DEBUGLOAD)
			if (!ofilename.IsEmpty()) // FIX! avoid checking idg while idx still open
				IntegrityCheck(ofilename);
			
		return TRUE;
	}




int cmpidx( const void *arg1, const void *arg2 )
{
		return ucmp(((Idx*)arg1)->date, ((Idx*)arg2)->date);
}



void CIdxList::Sort(void)
{
		qsort(&Head(), GetSize(), IDXSIZE, cmpidx);
}



BOOL CIdxList::Find(CFILE &f, unsigned date, int size)
{	
		CFSEEKID obj(&f);
		idxpos = obj.seekidx(date);
		idxsize = obj.ilast+1;
		seeksize = obj.idxlast.seekend();
		if (idxpos<0) 
			{
			Log(LOGALERT, "IDX: Corrupted IDX %s [Find()], deleting to force rebuild", f.name());
			f.fclose();
			DeleteFile(f.name());
			return FALSE;
			}
		if (idxpos>obj.ilast)
			seekpos = obj.idx.seekend();
		else
			seekpos = obj.idx.seekpos;
		return TRUE;
}



BOOL CIdxList::Find(const char *filename, double ddate, BOOL loadidx)
{
		Reset();
		//BOOL retry = TRUE;
		seekid = (unsigned)ddate;
		CString file = Filename(filename);

		//retry: { Reset()
		CFILE f;
		if (!f.fopen(file, CFILE::MREAD)) 
			{
			// AUTOREBUILD!
			// if it does not exist, create it!
			CIdxList newlist;
			if (!newlist.Rebuild(filename))
				{
				// master file probably does not exist, no need to alert
				//Log(LOGALERT, "IDX: Can't rebuild IDX for %s (and file exist)", filename);
				return FALSE;
				}
			// try again now
			if (!f.fopen(file, CFILE::MREAD)) 
				{
				Log(LOGALERT, "IDX: Can't rebuild IDX (2) for %s", filename);
				return FALSE;
				}
			}

		if (Find(f, seekid))
			{
			if (loadidx)
				{
				f.fseek(idxpos*IDXSIZE, SEEK_SET);
				SetSize(idxsize-idxpos);
				if ((int)f.fread(&operator[](0), IDXSIZE, GetSize())<GetSize())
					{
					Log(LOGALERT, "IDX: Could not load all IDX records for %s", filename);
					return FALSE;
					}
				}
			return TRUE;
			}
/*
		Why??? Did not find it first time

		// try rebuilding index		
		if (retry)
			{
			retry = FALSE;
			DeleteFile(file);
			goto retry;
			}
*/
		return FALSE;
}


BOOL CIdxList::FindCRC(const char *filename, unsigned CRC, unsigned date)
{
		Idx find;
		find.date = CRC;
		if((idxpos = qfind(&find, &Head(), GetSize(), IDXSIZE, cmpidx))<0) 
			return FALSE;
		// Found it!
		find = operator[](idxpos);
		if (find.date!=CRC)
			{
			Log(LOGALERT, "IDX: FindCRC messed up records in %s", filename);
			return FALSE;
			}

		
		//ASSERT( date==0 );
		CFILE f;
		if (!f.fopen(Filename(filename, TRUE), CFILE::MREAD))
			return FALSE;

		CFSEEKID obj(&f);
		obj.ifirst = find.seekpos/IDXSIZE;
		obj.ilast = find.seekend()/IDXSIZE-1;
		idxpos = obj.seekidx(date);
		idxsize = find.seeklen/IDXSIZE;
		if (idxpos>obj.ilast)
			{
			idxpos = -1;
			return FALSE; // out of range
			}
		if (idxpos<0)
			{
			Log(LOGALERT, "IDX: Corrupted IDX %s [FindCRC()], deleting to force rebuild", f.name());
			f.fclose();
			DeleteFile(f.name());
			return FALSE;
			}
		// seek to start, set max size to read
		of.fseek(seekpos = obj.idx.seekpos, SEEK_SET);
		seeksize = obj.idxlast.seekend();
		return TRUE;
}

BOOL CIdxList::Link(CIdxList &idg)
{
	for (int i=0, g=0; i<GetSize(); ++i)
		{
		Idx &idx = operator[](i);
		int seekpos = idx.seekpos;
		int seekend = idx.seekend();

		// adjust seekpos
		while (g<idg.GetSize() && idg[g].seekpos<seekpos) ++g;
		if (idg[g].seekpos!=seekpos)
			return FALSE; // corrupted
		idx.seekpos = g*IDXSIZE;

		// adjust seeklen
		while (g<idg.GetSize() && idg[g].seekend()<seekend) ++g;
		if (idg[g].seekend()!=seekend)
			return FALSE; // corrupted
		idx.seeklen = (++g)*IDXSIZE - idx.seekpos;
		}
	return TRUE;
}


char *CIdxList::GetLine(void)
{
	return of.fgetstr();
}

char *CIdxList::GetHeader(void)
{
	return of.fgetstr();
}

#ifdef USEFIXHEADER
BOOL CIdxList::FixHeader(CString &oldheader, CString &newheader)
{
	// @@@ automatic recolumn?

	// only do for DATA* files
	if (strncmp(ofilename, STATDATAPATH, strlen(STATDATAPATH))!=0)
		return TRUE;

	// rewrite file
	Log(LOGWARN, "REWRITING HEADER: file %s", ofilename);
	of.fclose();
	CLineList data;
	data.Load(ofilename, FALSE);
	if (data.Header() != oldheader )
		{
		Log(LOGALERT, "FixHeader mismatched header %s (line too long?)", ofilename);
		return FALSE;
		}
	else
		{
		data.Header() = newheader;
		data.Save(ofilename, FALSE);
		}
	if (!Open(ofilename, omode))
		{
		Log(LOGALERT, "FixHeader failed reopening file %s", ofilename);
		return FALSE;
		}
	oldheader = GetHeader();
	if (oldheader!=newheader)
		{
		Log(LOGALERT, "FixHeader failed to fix header mismatch (line too long?)", ofilename);
		return FALSE;
		}
	seeksize = -1; // rebuild index
	return TRUE;
}
#endif

void CIdxList::SeekPos(void)
{
	of.fseek(seekpos, SEEK_SET);
}

BOOL CIdxList::SeekHeader(CString &header)
{
	// check header match expectations, rewrite/update file otherwise
	if (header.IsEmpty())
		{
		// fill up header 
		char *fheader = GetHeader();
		if (!fheader) return FALSE;
		header = fheader;
		}
#ifdef USEFIXHEADER
	else
		{
		CString fheader;
		if (!GetHeader(fheader))
			return FALSE;
		if (strcmp(GetTokenRest(fheader,1), GetTokenRest(header,1))!=0)
			FixHeader(fheader, header);
		}
#endif

	if (seekpos<0) 
		{
		Log(LOGALERT, "IDX: unexpected seekpos<0 %s", ofilename);
		return FALSE;  // nothing was found
		}

	// self-check and recovery
	if (seeksize!=of.fsize())
		{
		// AUTOREBUILD!
		// corrupted index! rebuild!
		Log(LOGWARN, "IDX: rebuilding index for not matching source for %s", ofilename);
		if (!Rebuild(ofilename))
			{
			Log(LOGALERT, "IDX: could not rebuild index for %s", ofilename);
			return FALSE;
			}
		//ASSERT(seekid>0);
		if (!Find(ofilename, seekid))
			return FALSE;
		if (seeksize!=of.fsize())
			{
			of.fclose();
			Log(LOGALERT, "IDX: could not recover from index not matching source for %s", ofilename);
			return FALSE;
			}
		}

	// seek to found
	of.fseek(seekpos, SEEK_SET);
	return TRUE;
}


int CIdxList::Add(int pos, const char *line)
{
		int len = CFILE::fstrlen(line);
		unsigned id = crccol>=0 ? CRC( GetToken(line, crccol) ) : DATE(line);
		if (!id) // reject lines with invalid IDs
			return len;
		if (grpmode && id==lastid)
			{
			// add to existing record
			int ilast = GetSize()-1;
			Idx &last = operator[](ilast);
			ASSERT(last.date==lastid);
			if (ilast<idxskip) 
				--idxskip;
			last.seeklen += len;
			}
		else
			{
			Idx idx;
			/* direct access creates too much data on disk
			if (!crcmode && !idgmode)
				{
				// add inbetweens
				//if (lastid<=0)
				//	lastid=idxpos;
				if (lastid<=0)
					lastid=id;
				for (int i=(int)lastid+1, n=(int)id; i<n; ++i)
					{
					idx.date = i;
					idx.seekpos = pos;
					idx.seeklen = 0;
					AddTail(idx);
					}
				}
			*/

			idx.date = id;
			idx.seekpos = pos;
			idx.seeklen = len;
			// add new record
			AddTail( idx );
			if (crccol<0 && lastid>id)
				Log(LOGALERT, "IDX: Broken sequence adding to IDX files %d>%d %.20s", lastid, id, line);
			lastid = id;
			}
		return len;
}

BOOL CIdxList::Open(const char *filename, int mode, CIdxList *_idg)
{
	if (seekpos<=0 && mode == IDXUPDATE)
		mode = IDXWRITE;
	if (!of.fopen( ofilename = filename, omode = mode ))
		{
		if (mode!=IDXREAD)
			Log(LOGALERT, "ERROR: unexpected error rewriting/updating %s", ofilename);
		return FALSE;
		}

	if (mode!=IDXREAD)
		{
		// if updating a file, make sure to lock both file and index
		CString file = Filename(ofilename, idgmode);
		if (!ofi.fopen(file, CFILE::MUPDATE))
			{
			Log(LOGALERT, "ERROR: unexpected error locking index for %s", ofilename);
			return FALSE;
			}
		}

	if (_idg)
		{
		idg = _idg;
		idg->idgmode = TRUE;
		crccol = 1;
		}

	return TRUE;
}

BOOL CIdxList::PutHeader(const char *header)
{
	skipposw = 0;
	if (omode==IDXWRITE)
		{
		// REWRITE
		of.fputstr(header); 
		seekposw = of.ftell();
		return TRUE;
		}
	else
		{
		// UPDATE
		// seek to proper position
		if (!SeekHeader(CString(header)))
			return NULL;
		seekposw = seekpos;
		}
	return FALSE;
}


int CIdxList::ReadLines(char *data) 
{
	//ASSERT( seekpos == of.ftell() );
	//num = idxsize-idxpos;
	int size = seeksize-seekpos;
	if (size<=0)
		return -1;
	if (!data) 
		return size+1;
	if (of.fread(data, size, 1)<1)
		return 0;
	data[size] = 0;
	return size+1;
}

BOOL CIdxList::PutLine(const char *line, BOOL skip)
{
	int size = GetSize();
	int linelen = Add(seekposw, line);
	if (idg) 
		{
		// full mode idg
		if (GetSize()>size) 
			idg->lastid = 0;
		idg->Add(seekposw, line);
		}
	seekposw += linelen;

	// process the line
	if (skip) 
		{
		idxskip = GetSize();
		skipposw = seekposw;
		}
	else
		{
		if (skipposw>0)
			{
			of.fseek(skipposw, SEEK_SET);
			skipposw = 0;
			}
		return of.fputstr(line);
#ifdef DEBUG
		if (seekposw!=of.ftell())
			Log(LOGALERT, "IDX: Wrong seekposw detected %d!=%d", seekposw, of.ftell());
#endif
		}
	return linelen;
}


void CIdxList::Close()
{
	if (!of.isopen()) return;
	

	if (omode==IDXREAD)
		{
		// simple for reading
		of.fclose(); 
		return;
		}

	// bonus integrity check
	if (seekposw!=of.ftell())
		Log(LOGALERT, "IDX: integrity check failed for %s", ofilename);
	// truncate if necessary
	if (omode==IDXUPDATE && seekposw<seeksize)
		of.ftruncate(seekposw);
	of.fclose();


	if (idg) // link	double IDX
		{
		CFILE f;		
		Link(*idg);
		CString file = Filename(/*idg->ofilename = */ofilename, TRUE);
		if (!f.fopen(file, CFILE::MUPDATE))
			{
			Log(LOGALERT, "IDX: Problem updating group index for %s", ofilename);
			}
		else
			{
			idg->Save(f);
			f.fclose();
			}
		}

	// Save index
	Save(ofi);
	ofi.fclose();
}

void CIdxList::Flush(void)
{
	Save(ofi);
	seekpos = seekposw;
	idxpos = GetSize();
	SetSize(0);
	if (idg)
		{
		idg->idxpos = idg->GetSize();
		idg->SetSize(0);
		}
}

// ============================= CLineList ===================================
int cmpid(const char *line, const char *id, int idlen)
{
	for (register int i=0; line[i]!=0 && id[i]!=0 && i<idlen; ++i)
		{
		if (i==DATELEN)
			{
			// skip estimate 'E'
			if (line[i]==ESTIMATE) 
				++line;
			if (id[i]==ESTIMATE) 
				++id;
			}
		register int diff = line[i]-id[i];
		if (diff!=0) return diff;
		}
	// check if really end of 
	if (!IsEmptyToken(line[i]) || !IsEmptyToken(id[i]))
		return line[i]-id[i];
	return 0;  // match
}

static int cmpline( const void *arg1, const void *arg2)
{
	//return strcmp(**(CString **)arg1, **(CString **)arg2);
	return cmpid(*(const char **)arg1, *(const char **)arg2, FILEBUFFLEN);
}

static int cmplineinv( const void *arg1, const void *arg2)
{
	return -cmpline(arg1, arg2);
}

static int cmpfind(  const void *arg1, const void *arg2)
{
	return cmpid(*(const char **)arg1, *(const char **)arg2, DATELEN);
}

static int cmptok( const char *arg1, const char *arg2, int toksyn)
{
	int maxcnt = (toksyn>0) ? GetTokenPre(arg2, toksyn+1) : DATELEN;
	return cmpid(arg1, arg2, maxcnt);
}

static int cmptok1( const char *arg1, const char *arg2, int toksyn)
{
	int maxcnt = (toksyn>0) ? GetTokenPre(arg2, toksyn+1) : DATELEN+1; //+1 for 'E'
	return cmpid(arg1, arg2, maxcnt);
}


int CLineList::Check(int i, int toksync)
{
	return cmptok1(lines[i-1], lines[i], toksync);
}

typedef struct {const char *line; CString id; } colsort;
static int cmplinecol( const void *arg1, const void *arg2)
{
	int cmp = ((colsort *)arg1)->id.Compare( ((colsort *)arg2)->id);
	if (cmp==0)
		cmp = cmpid(((colsort *)arg1)->line, ((colsort *)arg2)->line, DATELEN+1);
	return cmp;
}

static int cmplinecolinv( const void *arg1, const void *arg2)
{
	return -cmplinecol(arg1, arg2);
}

void CLineList::Sort(int idxcol, int order)
{
	if (GetSize()<=0)
		return;

	int inv = order <1;
	int sortedcode = 0x1 + (inv<<1) + (idxcol<<2);
	if (sorted==sortedcode) return;
	sorted = sortedcode;

	if (!idxcol)
	{
	PROFILE("CLineList::Sort()LINE");
	qsort(&lines[0], GetSize(), sizeof(lines[0]), inv ? cmplineinv : cmpline);
	sorted = TRUE;
	}
	else
	{
	PROFILE("CLineList::Sort()SYM");
	CArray <colsort,colsort> sortlist;
	sortlist.SetSize(GetSize());
	for (int i=0; i<GetSize(); ++i)
		{
		sortlist[i].line = lines[i];
		sortlist[i].id = GetToken(lines[i], idxcol);
		}
	qsort(&sortlist[0], GetSize(), sizeof(sortlist[0]), inv ? cmplinecolinv : cmplinecol);
	for (int i=0; i<GetSize(); ++i)
		lines[i] = sortlist[i].line;
	}
}

void CLineList::DeleteLine(int n)
{
	PROFILE("CLineList::Delete()");
	for (int j=n; j<GetSize()-1; ++j)
		lines[j] = lines[j+1];
	lines[j] = NULL;
	lines.SetSize(GetSize()-1);
}

int CLineList::Purge(int toksync, int start)
{
	PROFILE("CLineList::Purge()");
	Sort();
	if (toksync<0) 
		return 0;

	// merge with itself to avoid duplicate
	int count  = 0;
	for (int i=start; i<GetSize()-1; ++i)
		{
		const char *oldline = lines[i];
		const char *newline = lines[i+1];
		if (*oldline==0 || cmptok(oldline, newline, toksync)==0) // old and new
			//if (strcmp(newline,oldline)!=0)
				{
				// merge lines & purge
				if (strcmp(newline,oldline)!=0)
					Merge(i, newline, oldline);
				DeleteLine(i+1);
				++count;
				--i;
				}
		}
	return count;
}


int CLineList::Find(const char *id)
{
	if (GetSize()==0)
		return -1;
	PROFILE("CLineList::Find()");
	Sort();
	return qfind(&id, &lines[0], GetSize(), sizeof(lines[0]), cmpfind);
}

CLineList::CLineList(const char *sheader) // BOOL order = 1)
{
	if (sheader) 
		header = sheader;
	sorted = FALSE;
	//numlines = maxlines = 0;
	//lines = NULL;
}

void CLineList::Empty(void)
{
	PROFILE("CLineList::Empty()");
	lines.SetSize(0);
	strings.SetSize(0);
	/*
	for (int i=0; i<GetSize(); ++i)
		delete lines[i];
	free(lines);	
	lines = NULL;
	numlines = maxlines = 0;
	*/
}

CLineList::~CLineList()
{
	Empty();
}

const char *CLineList::Set(int set, const char *newline)
{
	if (strcmp(lines[set], newline)!=0)
		{
		lines[set] = strings[strings.AddTail(newline)];
		}
	return lines[set];
}

int CLineList::Add(CString &line)
{
	if (line.IsEmpty()) //skip empty
		return GetSize();
	sorted = FALSE;	
	lines.AddTail(strings[strings.AddTail(line)]);
	return GetSize();
}

int CLineList::Add(const char *line, BOOL copystr)
{
	if (*line==0) //skip empty
		return GetSize();
	sorted = FALSE;
	if (!copystr)
		lines.AddTail(line);
	else
		lines.AddTail(strings[strings.AddTail(line)]);
	return GetSize();
}

int CLineList::Merge(int set, const char *&newl, const char *&oldl)
{
	CString newline(newl), oldline(oldl);
	int count = 0;

	// different old and new data
	int oldc = GetTokenCount(oldline);
	int newc = GetTokenCount(newline);
	for (int n=0; n<oldc-newc; ++n)
		newline += ',';
	for (int c=0; c<oldc; ++c)
		{
		// find empty new data
		int pos = GetTokenPos(newline, c);
		if (IsEmptyToken(newline[pos]))
			{
			// try to get old value
			CString ins = GetToken(oldline,c);
			if (!ins.IsEmpty())
				{
				// actually there was an old value (but new is empty)
				newline.Insert(pos, ins);
				++count;
				}
			}
		}
	newline.TrimRight(',');
	oldl = newl = Set(set, newline);
	return count; // inserted values
}	

int CLineList::Merge(CLineList &newlines, int toksync, BOOL purgeE)
{
	PROFILE("CLineList::Merge()");
	//Purge(toksync); // assume newlines is presorted & purged (otherwise size computation can mess up)
	newlines.Purge(toksync);
	header = newlines.header;

	// check for empty lists
	if (newlines.GetSize()<=0)
		return GetSize();
	if (GetSize()<=0)
		{
		// empty list
		for (int i=0; i<newlines.GetSize(); ++i)
			Add(newlines[i]);
		return 0;
		}

	int firstdiff = GetSize();
	int ncurlines = GetSize(), nnewlines = newlines.GetSize();
	int inewlines = 0,  icurlines = ncurlines-1;
	// find first match
	while (icurlines>=0 && cmptok(lines[icurlines], newlines[0], toksync)>=0)
		--icurlines;
	++icurlines;
		
	// merge (if needed)
	while (icurlines<ncurlines || inewlines<nnewlines)
		{
		int cmp = 0;
		if (icurlines>=ncurlines)
			cmp = 1;
		else 
		if (inewlines>=nnewlines)
			cmp = -1;
		else 
		{
		const char *newline = newlines[inewlines];
		const char *oldline = lines[icurlines];
		cmp = cmptok(oldline, newline, toksync);
		if (cmp==0) // old and new
			{
			if (strcmp(newline,oldline)!=0)
				{
				// overwrite old with (merged) new
				CString orig = oldline;
				newlines.Merge(inewlines, newline, oldline);
				this->Set(icurlines, oldline);
				// only flag lines that are really different 
				if (strcmp(orig,oldline)!=0)
					if (icurlines<firstdiff) // flag difference
						firstdiff=icurlines;
				}
			inewlines++, icurlines++;
			}
		}
		if (cmp<0) // only in old
			{
			if (purgeE && IsEstimate(lines[icurlines])) 
				{
				// delete estimate 
				DeleteLine(icurlines);
				--ncurlines;
				if (icurlines<firstdiff) // flag difference
					firstdiff=icurlines;
				}
			else
				icurlines++;
			}
		if (cmp>0) // only in new
			{
			Add(newlines[inewlines++]); 
			if (icurlines<firstdiff) // flag difference
				firstdiff=icurlines;
			}
		}


	return firstdiff;
}

int CLineList::Add(CLineList &newlines, int toksync, BOOL purgeE)
{
	int firstdiff = Merge(newlines, toksync, purgeE);
	Purge(toksync, firstdiff);
	return firstdiff;
}

BOOL CLineList::Load(const char *filename, BOOL sort)
{
	PROFILE("CLineList::Load()");
	{
	CFILE f;
	if (!f.fopen(filename, CFILE::MREAD)) 
		return FALSE;

	// header
	char *line = f.fgetstr();
	if (!line) return FALSE;
	header = line; 

	// body
	while (line = f.fgetstr())
		Add(line);
	}

	if (sort) 
		Sort();
	return TRUE;
}

BOOL CLineList::Save(const char *filename, BOOL sort)
{
	PROFILE("CLineList::Save()");

	if (sort) 
		{
		Sort();
		if (DEBUGLOAD)
			if (!IntegrityCheck(filename))
				return FALSE;
		}

	BOOL rewrite = TRUE;

	// append
	CFILE f;
	if (!f.fopen(filename, rewrite ? CFILE::MWRITE : CFILE::MAPPEND)) 
		return FALSE;

	// header
	if (rewrite)
		f.fputstr( header); 

	// write new data
	for (int i=0; i<GetSize(); ++i)
		f.fputstr( lines[i]);

	return TRUE;
}


BOOL CLineList::IntegrityCheck(const char *file)
{
		double olddate = 0;
		for (int i=0; i<GetSize(); ++i)
			{
				double date = CGetDate(lines[i]);
				if (date==InvalidDATE)
					{
					Log(LOGALERT, "unexpected integrity error (invalid date) in file %s line #%d", file, i);
					Log(LOGALERT, "LINE: %.50s", lines[i]);
					}
				else
				if (olddate>date)
					{
					Log(LOGALERT, "unexpected integrity error (not sorted) in file %s line #%d", file, i);
					Log(LOGALERT, "LINE: %.50s", lines[i]);
					}
				olddate = date;
			}
		return TRUE;
}





BOOL CLineList::LoadIdx(const char *filename, double date)
{
	PROFILE("CLineList::LoadIdx()");

	// search the specified date (if any)
	if (!idx.Find(filename, date))
		return FALSE;

	// found! now load the proper data
	if (!idx.Open(filename, IDXREAD))
		{
		// can't read file, so any idx is invalid
		idx.Reset();
		return FALSE;
		}

	if (!idx.SeekHeader(header))
		{
		idx.Close();
		return FALSE;
		}

	// @@@ load at once?
	char *str;
	CFILE &f = idx.File();
	int seekpos = f.ftell();
	while (str=idx.GetLine())	
		{
		Add(str);
		seekpos += CFILE::fstrlen(str);
		ASSERT(seekpos==f.ftell());
		}

	if (seekpos!=f.ftell())
		Log(LOGALERT, "IDX: wrong file size estimation for %s", filename);

	idx.Close();

	if (DEBUGLOAD)
		if (!IntegrityCheck(filename))
			return FALSE;

	return TRUE;
}




BOOL CLineList::SaveIdx(const char *filename, int skiplines)
{
	PROFILE("CLineList::SaveIdx()");
	if (skiplines>=GetSize())
		{
		// no update necessary
		return TRUE;
		}

	if (!idx.Open(filename, IDXUPDATE)) 
		return FALSE;

	// header
	if (idx.PutHeader(header))
		{
		// full rewrite
		skiplines = 0;
		}
	else
		{
		// idx need to be skipped
		for (int i=0; i<skiplines; ++i)
			idx.PutLine(lines[i], TRUE);
		}

	// write new data
	for (int i=skiplines; i<GetSize(); ++i)
		idx.PutLine(lines[i]);

	idx.Close();
	return TRUE;
}



BOOL CLineList::UpdateIdx(const char *filename, int toksync, BOOL purgeE)
{
	PROFILE("CLineList::Update()");
	Sort();
	if (GetSize()==0) // empty
		return TRUE;

	/*
	// just need to search for date
	CString id = GetToken(Head(), 0);
	if (toksync>0)
		id = CString( Head(), GetTokenPre(Head(), toksync+1));
	*/
	// load old data from new data forward
	CLineList data;
	data.LoadIdx(filename, CGetDate(Head()));
	
	// merge new and old data, find first difference
	int firstdiff = data.Add(*this, toksync, purgeE);
	
	// save new data
	if (!data.SaveIdx(filename, firstdiff))
		return FALSE;

	return TRUE;
}











#ifdef SYMBOLMIGRATION // Discontinued
BOOL CLineList::UpdateTrans(const char *file, CLineFunc *trans)
{
	PROFILE("CLineList::Update2()");
	CString oldfile = MkString("%s.old", file);

	// move file out (protect against changes)
	if (!MoveFileEx(file, oldfile, MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))
		return FALSE;

	{
	CFILE newf.fopen(file, CFILE::MWRITE);
	if (!newf) 
		{
		MoveFileEx(oldfile, file, MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);
		return FALSE;
		}

	CFILE f.fopen(oldfile, CFILE::MREAD);
	if (f!=NULL)
		{
		// headers
		CString line;
		f.fgetstr( line);
		fputstr(newf, line);
		CString sheader = line;

		while (f.fgetstr( line)!=NULL)
			{
			CString sline = line;
			if (trans)
				if (!trans(sline, sheader))
					continue;
			fputstr(newf, sline);
			}
		}
	}

	DeleteFile( oldfile );
	return f!=NULL;
}
#endif








ParamSearch::ParamSearch(const char *name)
	{
	COL = ':';
	ncol = GetTokenCount(basesearch = name, COL);
	search = GetToken(basesearch, icol = 0,COL);
	}

BOOL match(const char *str1, const char *str2, int tok, char col)
{
	//CString t = GetToken(str2, tok, col);
	int s = GetTokenPos(str2, tok, col);
	str2 += s;
	for (int len=0; str2[len]!=0 && str2[len]!=col; ++len);

	if (*str1=='-') 
		++str1;
	if (*str2=='-') 
		++str2;
	if (strlen(str1)!=len)
		return FALSE;
	return strnicmp(str1, str2, len)==0;
}

/*
	char *sep = separator ? separator : ",";

	// find start
	int i = 0;
	while (*str!=0 && i<n)
		if (IsMatch(*str++, sep))
			++i;

	// find end
	char const *tok = str;
	int len = 0; 
	while (*str!=0 && !IsMatch(*str, sep))
		++str, ++len;

BOOL match(const char *str1, const char *str2, int tok, char col)
{
	int len = 0;
	for (int i=0; i<=tok; ++i)
		for (str2+=len; str2[len]!=0 && str2[len]!=col; ++len);

	if (*str1=='-') 
		++str1;
	if (*str2=='-' || *str2==col) 
		++str2;
	if (strlen(str1)!=len)
		return FALSE;
	return strnicmp(str1, str2, len)==0;
}
*/
BOOL ParamSearch::IsMatch(const char *name)
	{

		if (ncol==1)
			{
			// simple case
			if (match(search,name,0,COL))
				return TRUE;
			if (match(search,name,1,COL))
				return TRUE;
			return FALSE;
			}

		// NASDAQ:Foreign:Tokyo  case
		if (search.IsEmpty())
			return FALSE;

		CString str = name;
		while (!search.IsEmpty() && match(search,str,0,COL))
			{
			if (ncol-icol>1 && str.Find(COL)<0) 
				return FALSE; // false match (missing : for NASDAQ)
			search = GetToken(basesearch, ++icol, COL);
			str = (const char*)str+GetTokenPos(str,1,COL);
			}

		// for X: stuff break out
		return search.IsEmpty();
	}

/*
	CString str(cmp), str2;
	BOOL strsec = strchr(cmp,':')!=NULL;
	if (strsec)
		{
		str2 = GetToken(cmp,1,":").Trim();
		str = GetToken(cmp,0,":").Trim();
		}
	// map headers to queried values
	int n=0;
	POSITION pos = headers.GetHeadPosition();
	for (int i=0; pos!=NULL; ++i)
		{
		BOOL cmp = FALSE;
		const char *hdr = headers.GetNext( pos );
		// compare independent of section
		if (strchr(hdr,':')==NULL) 
			cmp = str.CompareNoCase(hdr)==0;
		else
			{
			cmp = str.CompareNoCase(GetToken(hdr,0,":"))==0;
			if (!cmp && !strsec)
				cmp = str.CompareNoCase(GetToken(hdr,1,":"))==0;
			}					
		if (cmp)
			{
			if (++n>=2 || !strsec) 
				return i;
			str = str2;
			if (str.CompareNoCase(GetToken(hdr,1,":"))==0)
				return i;
			} 
		}
	return -1;
*/



CString &ParseSection(CString &hdr, CString &section)
{
		hdr = GetToken(hdr,0,'=').Trim(); // skip post = 
		if (hdr.Find(':')>0) 
			{
			// new section
			section = GetToken(hdr,0,':').Trim();
			hdr = GetToken(hdr,1,':').Trim();
			section.Replace(' ','_'); // replace space with _
			hdr.Replace(' ','_');
			hdr = section+":"+hdr;
			}	
		hdr.Replace(' ','_'); // replace space with _
		return hdr;
}

int CreateSectionList(const char *line, CStringArrayList &headers)
{
	CString section;
	int n = GetTokenCount(line);
	for (int i=0; i<n; ++i)
		headers.AddTail(ParseSection(GetToken(line,i), section));
	return n;
}

int FindSectionList(const char *cmp, CStringArrayList &headers)
{
	ParamSearch ctx(cmp);
	int n = headers.GetSize();
	for (int i=0; i<n; ++i)
		if (ctx.IsMatch(headers[i]))
			return i;
	return -1;
}








CString GetForm(const char *memory, const char *param)
{
	// parse memory to produce POST string
	CSymList list; 
	const char *buffer = memory;
	while ((buffer = strstr(buffer, "<input"))!=NULL)
		{
		CString input, name, value, type;
		++buffer;
		GetSearchString(buffer, "", input, ' ', '>');
		GetSearchString(input, "name=", name);
		GetSearchString(input, "value=", value);
		GetSearchString(input, "type=", type); // skip radio buttons
		if (type.CompareNoCase("hidden")!=0)
			continue;
		if (name.IsEmpty())
			continue;
		list.AddUnique( CSym(name, http_escape(value) ));
		//Log(LOGINFO, "FORM: %s = %.50s [%s]", name, value, type);
		}

	// add form data to original form
	int num = GetTokenCount( param, '&');
	for (int i=0; i<num; ++i)
		{
		CString p = GetToken(param, i, '&');
		CString name = GetToken(p, 0, '=');
		CString value = GetTokenRest(p, 1, '=');
		if (name.IsEmpty())
			continue;
		int found = list.Find(name);
		if (found>=0) 
			list[found].data = value;
		else
			list.Add( CSym(name, value) );
		//Log(LOGINFO, "PARAM: %s = %.50s [%d]", name, value, found>=0);
		}

	// prepare url for posting
	CString str;
	for (int i=0; i<list.GetSize(); ++i)
		{
		//Log(LOGINFO, "POST: %s = %.50s", list[i].id, list[i].data);
		str += list[i].id;
		str += "=";
		str += list[i].data;
		str += "&";
		}
	str.Trim("&");
	return str;
}





const char *StatPath(const char *outfile)
{
	const char *stat = STATPATH;
	const char *file = strstr(outfile, stat);
	ASSERT( file!=NULL );
	if (!file) return outfile;
	return file + strlen(stat);
}

int CRCDays(const char *path)
{
	if (IsSimilar(path, "HIST") || IsSimilar(path, "ECO"))
		return THREADDAYS+2;
	if (IsSimilar(path, "EPS") || IsSimilar(path, "DIV"))
		return 8;
	// anyone else
	return 5;
}

unsigned int CRC(const char *filename, CString *last)
{

	CFILE file;
	if (file.fopen(filename, CFILE::MREAD) == NULL) 
		return 0;

	// lenght
    int size = file.fsize();
    file.fseek(0L, SEEK_SET );

	char *line;
	unsigned int crc = size, lastcrc = 0;
	int CRCDAYS = CRCDays(StatPath(filename));
	for (int i=0; i<CRCDAYS && (line=file.fgetstr())!=NULL; ++i)
		{
		int len = strlen(line);
		crc = CalculateCRC( line , len, crc);
		lastcrc = CalculateCRC( line , len);
		}

	if (last && lastcrc!=0)
		{
		CString date = GetToken(line, 0);
		if (strlen(date)==DATELEN && CGetDate(date)!=InvalidDATE)
			*last = MkString("%s:%u", date, lastcrc);
		}
	return crc;
}









