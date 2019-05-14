// stdafx.cpp : source file that includes just the standard includes
// TradeMaster.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"
#include <afxinet.h>

#define AGENT "Mozilla/5.0 (Windows NT 6.3; WOW64; Trident/7.0; Touch; rv:11.0) like Gecko"

//=======================================================================
#include <windows.h>
#include <psapi.h>
#pragma comment(lib, "psapi")
#include <io.h>
#include "afxmt.h"

//#define BUFSIZE 512
#define SECURITYCHECK
#define CONSOLETIME 1  // update console display every 1s
#define FINISHTIME 30   // update finish time every 30s
//#define USESIMPLETHREADLOCK // use simple critical section semaphore
//#define USEWRITEBUFFER	// avoid calling twice to fwrite for EOL
//#define USESIMPLECSECTION

//=======================================================================

#ifdef DEBUG
//#define DEBUGMEMORY // very deep memory checks
#endif

BOOL DEBUGLOAD;

#ifdef STATMASTER
#define APPNAME "RW"
void ShowProgress(LPCTSTR lpszText) { }
#else
#define APPNAME "TradeMaster"
#endif

#define ALERTFILE "ALERT.LOG"
#define LOGDATEFORMAT "%y/%m/%d %H:%M:%S"
#define SESSIONIDDEF "SessionID=" 
#define SESSIONMARK "#SESSIONID#"
#define FORMMARK "?POSTFORM?"
#define POSTFILEMARK "?POSTFILE?"  // http://url?POSTFILE?test.kml
#define POSTMARK "?POST?"  // http://url?POST?parameter=x&parameter=y&parameter=z
#define SOAPMARK "?SOAP?"  // http://url?SOAP?<xml >
#define URLMARK "?URL?" // http://loginurl?URL?http://nexturl?SessionID=#SESSIONID#&token=#SESSIONID#
#define REDIRECTMARK "?REDIRECT?"
#define MINMEMORY (100*1024) // 100K min buffer for downloading (will self grow if needed)
#define MINBUFFERSIZE (50*1024) // 50K min for incremental downloading


CString http_escape(const char *string)
{
	CString ret;
	for (int i=0; string[i]!=0; ++i)
		if (isalnum(string[i]))
			ret += string[i];
		else if (string[i]==' ')
			ret += '+';
		else
			ret += MkString("%%%X", (int)((unsigned char)string[i]));
	return ret;
}

static CString base64_chars = 
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";


static inline bool is_base64(unsigned char c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}

CString base64_decode(CString &encoded_string) 
{
  int in_len = encoded_string.GetLength();
  int i = 0;
  int j = 0;
  int in_ = 0;
  unsigned char char_array_4[4], char_array_3[3];
  CString ret;

  while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
    char_array_4[i++] = encoded_string[in_]; in_++;
    if (i ==4) {
      for (i = 0; i <4; i++)
        char_array_4[i] = base64_chars.Find(char_array_4[i]);

      char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (i = 0; (i < 3); i++)
        ret += char_array_3[i];
      i = 0;
    }
  }

  if (i) {
    for (j = i; j <4; j++)
      char_array_4[j] = 0;

    for (j = 0; j <4; j++)
      char_array_4[j] = base64_chars.Find(char_array_4[j]);

    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
    char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

    for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
  }

  return ret;
}


int qfind(const void *search, const void *table, int num, int size, int cmpid( const void *arg1, const void *arg2), int *aprox)
	{
	if (num<=0)
			return -1;
	 // check for out of bounds
	 if (cmpid(search, table)<0 ||
		cmpid(search, (char *)table + (num-1)*size)>0)
		return -1;

	 register int high, i, low;
	 for ( low=(-1), high=num;  high-low > 1;  )
		  {
		  i = (high+low) / 2;

		  int cmp = cmpid(search, (char *)table + i*size);
		  if (cmp==0)
				return i;
		  if ( cmp < 0 )  high = i;
			   else             low  = i;
		  }
	 if (aprox) *aprox = low;
	 return -1;
	 }


//#define DISABLE_THREADS	// disable creation of thread

#if 0

BOOL pcheck(int &ticks, double sec)
{
	int nticks = GetTickCount();
	if ((DWORD)(ticks+sec*1000)>(DWORD)nticks)
		return FALSE;
	ticks = nticks;
	return TRUE;
}

#else // ignite first time

BOOL pcheck(int &ticks, double sec)
{
	int nticks = GetTickCount();
	if (nticks<ticks)
		return FALSE;
	ticks = nticks+(int)(sec*1000);
	return TRUE;
}

#endif

int CProgress::index = 0;
CProgress *CProgress::list[20];

CProgress::CProgress(const char *_name, int _minn, int _maxn)
{
	maxn = _maxn;
	n = minn = _minn;
	name = _name;
	
	id = index++;
	list[id] = this;

	Set(minn);
	ticks = 0;
}

CProgress::~CProgress(void)
{
	//ticks = 0;
	Set(maxn);
	// truncate indexes to avoid multithread nesting
	list[id] = NULL;
	--index;
	if (index!=id)
		Log(LOGALERT, "Nested multithread conditions detected in CProgress %s", name);

	if (index<=0) 
		{
		index = 0;
		ShowProgress(""); // grand finale
		}
}

int CProgress::finishticks = 0;
int CProgress::ticks = 0;
int CProgress::pct = -1;
int CProgress::npct = 0;
int CProgress::accpct = 0;

void CProgress::Set(int _n, const char *text)
{
	n = _n;
	//static int cticks;
	if (!pcheck(ticks,CONSOLETIME)) // update max every second
		return;



	CString line;
	if (pct>=0)
		line += MkString("[BUFF:%d%%] ", pct);

	if (!text) text = "";
	double g = 0, maxg = 1;
	for (int i=0; i<index; ++i)
		{
		CProgress *p = list[i];
		if (!p) continue;
		int pn = p->n-p->minn;
		int pmaxn = p->maxn-p->minn;
		g = g*pmaxn + pn;
		maxg = maxg*pmaxn;
		if (pmaxn == 0) pmaxn = 1;
		line += MkString( "%s:%d%% ", p->name, PERCENT(pn,pmaxn));
		}

	// finish
	static int ogticks;
	static double finish, ogpct;
	if (pcheck(finishticks,FINISHTIME))
			{
			double gpct = g/maxg;
			int gticks = GetTickCount();
			if (gpct>ogpct)
				finish = (gticks-ogticks)/1000.0/60.0/60.0/(gpct-ogpct);
			ogpct = gpct, ogticks = gticks;
			}

	CString finishstr;
	if (finish>1)
		finishstr += MkString("%dh", (int)finish);
	if (finish>0)
		finishstr += MkString("%dm", (int)(finish*60) % 60);
	if (finish<=0)
		finishstr = "?";

	int tot = PERCENT(g, maxg);
	ShowProgress( MkString("%d%% [%s left] = %s%s  ", tot, finishstr, line, text));
}

void CProgress::SetBuffering(int _pct)
{
	pct = _pct;
	accpct += _pct;
	npct++;
}

int CProgress::GetAvgBuffering()
{
	return npct>0 ? accpct/npct : 0;
}



// Token functions

const char *strstri(const char *str, const char *str2)
{
	int len = strlen(str2);
	for (const char *s=str; *s!=0; ++s)
		if (strnicmp(s, str2, len)==0)
			return s;
	return NULL;
}

BOOL IsSimilar(const char *str, const char *cmp)
{
	// compare beginning case insensitive
	if (str==NULL) return FALSE;
	return strnicmp(str,cmp,strlen(cmp))==0;
}


int Map(const char *buffer, tmap *m, BOOL substr)
{
	for (int i=0; m[i].str!=NULL; ++i)
		{
		//if (strnicmp(buffer, m[i].str, strlen(m[i].str))==0)
		if (substr && strnicmp(buffer, m[i].str, strlen(m[i].str))==0)
			return i;
		if (stricmp(buffer, m[i].str)==0)
			return i;
		}
	return -1;
}


BOOL MatchStr(CString &str, const char *match[])
{
	for (int i=0; match[i]!=NULL; ++i)
		if (IsSimilar(str, match[i]))
			{
			str.Delete(0, strlen(match[i]));
			return TRUE;
			}
	return FALSE;
}


int IsMatch(const char buffer, const char *str)
{
	for (const char *s =str; *s!=0; ++s)
		if (*s==buffer)
			return TRUE;
	return FALSE;
}


int IsMatch(const char *buffer, const char *strs[])
{
	return IsMatchN(buffer, strs)>=0;
}

int IsMatchN(const char *buffer, const char *strs[])
{
	if (!buffer) return FALSE;
	for (int i=0; strs[i]!=NULL; ++i)
		{
		int len = strlen(strs[i]);
		if (strncmp((char *)buffer, strs[i], len) ==0)
			return i;
		}
	return -1;
}

int IsContained(const char *buffer, const char *strs[])
{
	return IsContainedN(buffer, strs)>=0;
}

int IsContainedN(const char *buffer, const char *strs[])
{
	if (!buffer) return FALSE;
	for (int i=0; strs[i]!=NULL; ++i)
		{
		if (strstr((char *)buffer, strs[i])!=0)
			return i;
		}
	return -1;
}



int IsMatchNC(const char *buffer, const char *strs[])
{
	if (!buffer) return FALSE;
	for (int i=0; strs[i]!=NULL; ++i)
		{
		int len = strlen(strs[i]);
		if (strnicmp((char *)buffer, strs[i], len) ==0)
			return i;
		}
	return -1;
}

int IsMatchNC(const char *buffer, vara &strs)
{
	if (!buffer) return FALSE;
	for (int i=0; i<strs.length(); ++i)
		{
		int len = strlen(strs[i]);
		if (strnicmp((char *)buffer, strs[i], len) ==0)
			return i;
		}
	return -1;
}

BOOL IsMatchSimilar(const char *sstr, const char *match[], int startonly)
{
	char *sep = " _:";
	while (*sstr!=0)
		{
		for (int i=0; match[i]!=NULL; ++i)
			if (IsSimilar(sstr, match[i]))
				return TRUE;
		if (startonly)
			return FALSE;
		while (!strchr(sep,*sstr) && *sstr!=0) ++sstr;
		while (strchr(sep,*sstr) && *sstr!=0) ++sstr;
		}
	return FALSE;
}

CString GetToken(const char *str, int n, const char *separator) 
{
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

	return CString(tok, len);
}

#define IsMatchC(x,y) (x == y)

int GetTokenPre(const char *line, int count, char sep)
{
	register int i, c;
	for (i=0, c=0; line[i]!=0; ++i)
		if (line[i]==sep)
			if (++c>=count)
				return i;
	return i;
}

int GetTokenPos(register const char *line, register int count, register char sep)
{
	register int i, c;
	for (i=0, c=0; line[i]!=0 && c<count; ++i)
		if (line[i]==sep)
			++c;
	return i;
}

const char *GetTokenRest(const char *line, int count, char sep)
{
	return line + GetTokenPos(line, count, sep);
}

int GetTokenCount(const char *str, char sep)
{
	int i = 0;
	while (str!=NULL && *str!=0)
		if (IsMatchC(*str++, sep))
			++i;
	return i+1;
}

CString GetNextToken(const char *&str, char sep) 
{
	// find end
	char const *tok = str;
	int len = 0; 
	while (*str!=0 && !IsMatchC(*str, sep))
		++str, ++len;
	
	if (*str!=0)++str;

	return CString(tok, len);
}


CString GetToken(const char *str, int n, char sep) 
{
	// find start
	int i = 0;
	while (*str!=0 && i<n)
		if (IsMatchC(*str++, sep))
			++i;

	const char *cstr = str;
	return GetNextToken(cstr, sep);
}


// String functions


#define IDS_DOWNLOADP -5
#define IDS_ERR_DOWNLOAD -3
#define IDS_ERR_WRITE -4







CString MkString(const char *format, ...)
{
		TCHAR text[5*1024];
		int checkptr = sizeof(text)-3;
		char checkval = (char)253;
		text[checkptr] = checkval;

		va_list		argptr;
		va_start (argptr, format);
		vsprintf (text, format, argptr);
		va_end (argptr);

		ASSERT(text[checkptr]==checkval);

		CString str(text);
		return str;
}


int SetProgress(int m, int n, int d = 0)
{
	return  0;
}




// JLIB functions
#include "jlib.h"

#define DATKEY 0xCACA
#define MAXDATSIZE (100*1024) //100Kb
#define MKDAT(file) GetToken(file,0,'.')+".dat"

CString DATFILE = "sym.dat";

BOOL SetDAT(const char *file)
{	
	DATFILE = MKDAT(file);
	return TRUE;
}


BOOL WriteDAT(const char *file)
{
	char src[MAXDATSIZE];
	char dst[MAXDATSIZE];
	CString outfile = MKDAT(file);
	
	// read
	CFILE f;
	if (!f.fopen(file, CFILE::MREAD)) 
		return FALSE;
	int srclen = f.fread(src, 1, MAXDATSIZE);
	f.fclose();

	// pack
	int dstlen = jpack(dst, src, srclen);
	jcrypt(DATKEY, dst, dstlen);

	// write
	if (!f.fopen(outfile, CFILE::MWRITE)) 
		return FALSE;
	f.fwrite(dst, 1, dstlen);
	f.fclose();
	return TRUE;
}

BOOL ReadDAT(const char *outfile, CString &dststr)
{
	char src[MAXDATSIZE];
	char dst[MAXDATSIZE];

	// read
	CFILE f;
	if (!f.fopen(outfile, CFILE::MREAD)) 
		return FALSE;
	int srclen = f.fread(src, 1, MAXDATSIZE);
	f.fclose();

	// pack
	jcrypt(DATKEY, src, srclen);
	int dstlen = junpack(dst, MAXDATSIZE, src, srclen);
	dst[dstlen] = 0;
	dststr=dst;
	return dstlen;
}

#define USER "#USER#"
#define PASS "#PASS#"
#define ACCT "#ACCT#"

CString URLDAT(CString str)
{
	int found = str.Find(USER);
	if (found<0) found = str.Find(ACCT);
	if (found<0) return str;

	CString data;
	CString outfile = MKDAT(DATFILE);
	if (!ReadDAT(outfile, data))
		{
		Log(LOGFATAL, "ERROR: UPSTRING invalid DAT file for %s", outfile);
		return str;
		}

	// get id
	found += strlen(USER);
	CString id = USER;
	id += str.Mid(found, 2);
	str.Delete(found, 2);

	const char *buffer = data;
	while (strncmp(buffer, id, id.GetLength())!=0)
		{
		while (*buffer!=0 && !isspace(*buffer)) ++buffer;
		while (*buffer!=0 && isspace(*buffer)) ++buffer;
		if (*buffer==0) 
			{
			Log(LOGFATAL,"ERROR: UPSTRING invalid #USER#ID for %s", outfile);
			return str;
			}
		}
	// found!!!
	char *sep = ",\n\r";
	str.Replace(USER, GetToken(buffer, 1, sep));
	str.Replace(PASS, GetToken(buffer, 2, sep));
	str.Replace(ACCT, GetToken(buffer, 3, sep));
	return str;	
}

// Load functions
char *DownloadMalloc(int size);
int DownloadSize(char *buffer);
char *DownloadRealloc(char *buffer, int size);
void DownloadFree(char *buffer);



int URLRETRY = 3;
int URLTIMEOUT = 10;
HINTERNET SessionMalloc(void)
{
	HINTERNET m_hSession = NULL;
	SHIELDURL( m_hSession = InternetOpen(AGENT,
									INTERNET_OPEN_TYPE_PRECONFIG,
									NULL,
									NULL,
									0); );
	if (!m_hSession) return NULL;

	unsigned long timeout = URLTIMEOUT*1000;
	unsigned long retries = 5;
	unsigned long cookies = TRUE;

	SHIELDURL(
	InternetSetOption(m_hSession, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout) );
	InternetSetOption(m_hSession, INTERNET_OPTION_SEND_TIMEOUT, &timeout, sizeof(timeout) );
	InternetSetOption(m_hSession, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout) );
	InternetSetOption(m_hSession, INTERNET_OPTION_DATA_SEND_TIMEOUT, &timeout, sizeof(timeout) );
	InternetSetOption(m_hSession, INTERNET_OPTION_DATA_RECEIVE_TIMEOUT, &timeout, sizeof(timeout) );
	InternetSetOption(m_hSession, INTERNET_OPTION_CONNECT_RETRIES, &retries, sizeof(retries) );
	InternetSetOption(m_hSession, INTERNET_OPTION_CONNECT_BACKOFF, &timeout, sizeof(timeout) );
	InternetSetOption(m_hSession, INTERNET_SUPPRESS_COOKIE_POLICY, &cookies, sizeof(cookies) );
	);
	//
	return m_hSession;
}

void SessionFree(HINTERNET &m_hSession)
{
	if (m_hSession)
		SHIELDURL( InternetCloseHandle(m_hSession); );
	m_hSession = NULL;
}

char *GetSearchString(const char *mbuffer, const char *searchstr, CString &symbol, char startchar, char endchar)
{
	if (!mbuffer) 
		return NULL;
	char *cbuffer = (char *)mbuffer;
	cbuffer = strstr(cbuffer, searchstr);
	if (!cbuffer || *cbuffer==0) 
		return NULL;

	char *sym = cbuffer + strlen(searchstr);
	// automatic start/end
	if (startchar==0 && endchar==0)
		{
		if (*sym=='\'' || *sym=='\"')
			startchar = endchar = *sym;
		else
			endchar = ' ';
		}
	// find beginning 
	if (startchar) 
		while (sym[0]!=0 && sym[-1]!=startchar) ++sym;
	// find end
	int i;
	if (endchar)
		for (i=0; sym[i]!=0 && sym[i]!=endchar; ++i);
	else
		for (i=0; sym[i]!=0 && sym[i]!='&' && sym[i]!='\"' && sym[i]!='<'; ++i);
	
	symbol=CString(sym, i);
	return (char *)(sym+i);
}



char *GetSearchString(const char *mbuffer, const char *searchstr, CString &symbol, const char *startchar, const char *endchar)
{
	if (!mbuffer) 
		return NULL;
	char *cbuffer = (char *)mbuffer;
	cbuffer = strstr(cbuffer, searchstr);
	if (!cbuffer || *cbuffer==0) 
		return NULL;

	char *sym = cbuffer + strlen(searchstr);
	// find beginning 
	if (startchar) 
		{
		int len = strlen(startchar);
		while (*sym!=0 && strnicmp(sym,startchar,len)!=0) ++sym;
		if (*sym!=0) sym+=len;
	    }
	// find end
	int i;
	if (endchar)
		{
		int len = strlen(endchar);
		for (i=0; sym[i]!=0 && strnicmp(sym+i, endchar, len)!=0; ++i);
		}
	else
		for (i=0; sym[i]!=0 && sym[i]!='&' && sym[i]!='\"' && sym[i]!='<'; ++i);
	
	symbol=CString(sym, i);
	return (char *)(sym+i);
}



CString GetSessionID(const char *memory, CString id)
{
	// parse memory to produce POST string
	CString str;
	if (id.IsEmpty()) id=SESSIONIDDEF;
	GetSearchString(memory, id, str);
	if (str.IsEmpty())
		GetSearchString(memory, MkString("name=\"%s\"", id.Mid(0,id.GetLength()-1)), str,  "value=\"", "\"");
	str.Replace("&quot;", "\"");
	str.Trim("\"\' ");
	return str;
}


CString GetRedirectUrl(const char *memory, const char *str)
{
	CString val;
	GetSearchString(memory, str, val, '\"', '\"');
	return val;
}



CString ToHex(void *data, int bytes)
{
	CString pbuf;
	unsigned char *pstr = (unsigned char*)data;	
	for (int i=0; i<bytes; ++i)
		{
		pbuf+= to_hex(pstr[i] >> 4);
	    pbuf+= to_hex(pstr[i] & 0xF);
		}
	return pbuf;
}

void FromHex(const char *str, void *data)
{
	unsigned char *pstr2 = (unsigned char*)data;
	for (int i=0; str[i]!=0; i+=2)
		pstr2[i>>1] = (from_hex(str[i])<<4) | from_hex(str[i+1]);
}


/* Returns a url-encoded version of str */
/* IMPORTANT: be sure to free() the returned string after use */
CString url_encode(const char *str) {
  CString pbuf;
  const char *pstr = str;
  
  while (*pstr) {
    if (isalnum(*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' || *pstr == '~') 
      pbuf+= *pstr;
    else if (*pstr == ' ') 
      pbuf+= '+';
    else 
      pbuf+= '%', pbuf+= to_hex(*pstr >> 4), pbuf+= to_hex(*pstr & 15);
    pstr++;
  }
  return pbuf;
}

/* Returns a url-decoded version of str */
/* IMPORTANT: be sure to free() the returned string after use */
CString url_decode(const char *str) {
  CString pbuf;
  const char *pstr = str;
  while (*pstr) {
    if (*pstr == '%') {
      if (pstr[1] && pstr[2]) {
        pbuf+= (char)(from_hex(pstr[1]) << 4 | from_hex(pstr[2]));
        pstr += 2;
      }
    } else if (*pstr == '+') { 
      pbuf+= ' ';
    } else {
      pbuf+= *pstr;
    }
    pstr++;
  }
  return pbuf;
}

DownloadFile::DownloadFile(BOOL _binary, inetdata *idataobj) 
{ 
	size = 0;
	idata = idataobj;
	binary = _binary;
	headers = NULL;
	usereferer = FALSE;
	hostname = NULL;
	userpass = FALSE;
	hSession = NULL;
	memory = DownloadMalloc(MINMEMORY);
}

DownloadFile::~DownloadFile() 
{
	if (!tmpfile.IsEmpty())
		DeleteFile(tmpfile);
	DownloadFree(memory);
	SessionFree(hSession);
}


int DownloadFile::Load( const char *filename)
{
	CFILE file;
	if (file.fopen(filename, CFILE::MREAD) == NULL) 
		return FALSE;

	// check size
	size = file.fsize();
	file.fseek(0, SEEK_SET);

	memory = DownloadRealloc( memory, size+1);
	int err = (file.fread(memory,1,size)!=size);
	memory[size] = 0; // string termination
	return err ? FALSE : size;
}

void DownloadFile::AddUrl( CString &url, const char *line)
{
	//url.Trim("& \n\t");
	if (!line || !*line)
		return;

	if (IsSimilar(line, "//"))
		return;

	// first url
	if (url.IsEmpty())
		{
		url = line;
		return;
		}

	// line w URLMARK
	if (IsSimilar(line, URLMARK))
		{
		url += line;
		return;
		}

	// line w/o URLMARK
	if (IsSimilar(line, "https:") || IsSimilar(line, "http:"))
		{
		url += URLMARK;
		url += line;
		return;
		}

	// line w parameter
	//if (strchr(url, '&')!=NULL)
	url += line;
	url += "&";	
}


int DownloadFile::Download( CString url, const char *file, BOOL dump)
{
	if (url.IsEmpty())
		return FALSE;
	dump = dump || DEBUGLOAD;
#ifdef DEBUG
	//dump = TRUE;
#endif

	// process Login
	int LOGIN = url.Find(URLMARK);
	if (LOGIN>=0)
		{
		// login only once per session
		if (!hSession)
			{
			hSession = SessionMalloc();
			// login sequence
			if (Download(url.Left(LOGIN)))
				{
				SessionFree(hSession);
				Log(LOGERR, "login failed [%s]", url);
				return TRUE;
				}
			for (int mark = url.Find(SESSIONMARK); mark>=0; mark = url.Find(SESSIONMARK, mark+1) )
				{
				int i;
				for (i=mark-1; i>0 && url[i]!='&' && url[i]!='?'; --i); i++;
				CString id = url.Mid(i, mark-i);
				for (i=0; i<sessionids.GetSize(); ++i)
					if (strncmp(sessionids[i], id, id.GetLength())==0)
						continue;
				// new sessionid
				sessionids.AddTail(id+GetSessionID(memory, id));
				}
			}
		// load all ?URL? urls
		do {
			url = url.Mid(LOGIN+strlen(URLMARK));
			if ((LOGIN = url.Find(URLMARK))>=0)
				if (Download(url.Left(LOGIN)))
					return TRUE;
		} while (LOGIN>=0);
		return Download(url);
		}
#ifndef STATMASTER
	// process self filling forms
	int FORM = url.Find(FORMMARK);
	if (FORM>=0)
		{
		//ASSERT(FORM==0);
		CString GetForm(const char *memory, const char *param);
		url = url.Left(FORM)+"?POST?"+GetForm(memory, (const char *)url + FORM + strlen(FORMMARK));
		}
#endif	
	int REDIRECT = url.Find(REDIRECTMARK);
	if (REDIRECT>=0)
		url = GetRedirectUrl(memory, url.Mid(REDIRECT+strlen(REDIRECTMARK)));

	// process session propagation
	if (url.Find(SESSIONMARK)>=0)
		{
		for (int i=0; i<sessionids.GetSize(); ++i)
			{
			CString id = sessionids[i].Mid(0, sessionids[i].Find('='))+'=';
			if (id.IsEmpty()) id=SESSIONIDDEF;
			url.Replace(id+SESSIONMARK, sessionids[i]);
			if (dump)
				{
				CFILE f; f.fopen(MkString("session#%d.url", i), CFILE::MWRITE);
				f.fwrite(url, url.GetLength(), 1);
				}
			}
		}


	// retry up to 3 times
	BOOL err = TRUE;
	for (int retry = 0; retry<URLRETRY && err; ++retry)
		{
		err = DownloadNoRetry( url, file);
		//Log(LOGERR, "%d", err)); ///////
		if (err) 
			{
			//ShowProgress("Sleep(5000) @ DownloadFile::Download()");
			Sleep(5000); // 5s between retries to avoid getting banned
			continue;
			}
		// set zero termination
		memory[size] = 0;
		if (!binary)
			{
			// avoif zero termination in the middle! bastards!
			for (int i=0; i<size; ++i)
				if (!memory[i]) 
					memory[i]=0x20;
			}
		}
	if (dump)
		{
		static int NUM = 0;

		// guarantee no file conflict
		ThreadLock();
		int num = NUM++;
		ThreadUnlock();
		CFILE f; 
		char name[MAXPATHLEN];
		sprintf(name, "url%04d.url", num);
		f.fopen(name, CFILE::MWRITE);
		f.fwrite(url, 1, url.GetLength());
		f.fclose();


		sprintf(name, "url%04d.htm", num);
		Log(LOGINFO, "%s = %s [%.250s]", err ? "ERR" : "OK", name, url);
		f.fopen(name, CFILE::MWRITE);
		f.fwrite(memory, 1, size);
		f.fclose();
		}
	return err;
}




int DownloadCoreNoRetry( HINTERNET &hRequest, const char *file, int &size, char *&m, inetdata *idata)
{
		// setup buffer
		int BUFFERSIZE = MINBUFFERSIZE;
		BOOL err = IDS_ERR_DOWNLOAD;


		DWORD cReadCount = 0; 
		DWORD dwBytesRead = 0;
		DWORD dwReadSize = DownloadSize(m)-2;

	//if (idata) idata->open(cReadCount); 
	SHIELDURL( 

		// read file
		do
			{
			dwBytesRead = 0;
			InternetReadFile(hRequest, m + cReadCount, dwReadSize, &dwBytesRead); 
			if (idata) idata->write(m + cReadCount, dwBytesRead); 
			if (dwBytesRead>0)
				{
				dwReadSize = BUFFERSIZE;
				cReadCount += dwBytesRead;
				m = DownloadRealloc(m, cReadCount + dwReadSize +2);
				}
			}
		while (dwBytesRead>0);
	)
	//if (idata) idata->close(cReadCount); 

	// check for correctness
	//if (dwContentLen<=0) dwContentLen = cReadCount;
	//if (dwContentLen==cReadCount && cReadCount>0)
	if (cReadCount>0)
		{
		err = FALSE;
		size = cReadCount;
		// write file, if needed
		if (file!=NULL && *file!=0)
			{
			err = IDS_ERR_WRITE;
			CFile f;
			if (f.Open(file, CFile::modeCreate|CFile::modeWrite|CFile::shareExclusive|CFile::typeBinary))
				{
				err = FALSE;
				f.Write(m, cReadCount);
				}
			}
		}

	m[size] = 0;

	return err;
}


#define SEP "--"
#define SEPFORM "---------------------------7de1723175012";

CString DownloadFile::FormEnd(void)
{
	CString ret(SEP);
	ret += SEPFORM;
	ret += SEP;
	return ret;
}

CString DownloadFile::FormValue(const char *name, const char *val)
{
	CString ret(SEP);
	ret += SEPFORM;
	ret += "\nContent-Disposition: form-data; name=\""+CString(name)+"\"\n\n";
	ret += val;
	ret += "\n";
	return ret;
}

CString DownloadFile::FormFile(const char *name, const char *f, const char *mime)
{
	CString ret(SEP);
	ret += SEPFORM;
	ret += "\nContent-Disposition: form-data; name=\""+CString(name)+"\"; filename=\""+CString(GetFileNameExt(f))+"\"\nContent-Type: "+CString(mime)+"\n\n";
	return ret;
}



int DownloadFile::PostFile(inetdata &data)
{
	if (file.name.IsEmpty())
		return TRUE;

	data.write( FormFile(file.name, file.remote, file.mime) );
	CFILE f;
	if (!f.fopen(file.local, CFILE::MREAD))
		{
		Log(LOGERR, "ERROR: %s file not exist\n", file.local);
		return FALSE;
	    }
	int bread = 0;
	char buff[8*1024];
	while ( bread = f.fread(buff, 1, sizeof(buff)))
		data.write(buff, bread);
	data.write("\n");
	return TRUE;
}



CString DownloadFile::ScanFormValue(const char *id, const char *pre)
{
		CString value;
		GetSearchString(memory+1, CString(pre)+"=\""+CString(id)+"\"", value, "value=\"", "\"");
		return value;
}


int DownloadFile::SubmitForm(void)
{
	vars url = action;
	url += IsSimilar(method,"POST") ? (strstr(enctype, "multipart") ? POSTFILEMARK : POSTMARK) : "?";
	url += FormValues();
#ifdef DEBUG
	return Download(url, "testform.html");
#else
	return Download(url);
#endif
}

int DownloadFile::GetForm(const char *key)
{
	lists.SetSize(2);
	lists[0].RemoveAll();
	lists[1].RemoveAll();

	vars mem(memory);
	mem.Replace("<FORM", "<form");
	mem.Replace("</FORM", "</form");
	mem.Replace("<INPUT", "<input");
	mem.Replace("<SELECT", "<select");

	vars form = ExtractString(mem, "<form", "", "</form");

	// find specific form
	if (key)
		{
		int n = -1;
		vara forms(mem, "<form");	
		for (int i=1; i<forms.length() && n<0; ++i)
			{
			const char *formi = forms[i];
			if (strstr(formi, key))
				n = i;
			}
		if (n<0) 
			{
			//Log(LOGERR, "ERROR could not find action=submit");
			return FALSE;
			}
		form = ExtractString(forms[n], " ", "", "</form");
		}

	action = ExtractString(form, "action=", "\"", "\""); action.Replace("&amp;", "&");
	enctype = ExtractString(form, "enctype=", "\"", "\"");
	method = ExtractString(form, "method=", "\"", "\"");

	if (!IsSimilar(action,"http"))
		{
		vara urla(hostname ? hostname : lasturl, "/"); urla.SetSize(3);
		action = urla.join("/")+action;
		}

	vara input(form, "<input");
	vara select(form, "<select");


	for (int i=1; i<select.length(); ++i) {
		CString line = GetToken(select[i], 0, '>');
		CString value, name;
		if (name.IsEmpty())
			GetSearchString(line, " name=", name, "\"", "\"");
		if (name.IsEmpty())
			GetSearchString(line, " id=", name, "\"", "\"");
		if (name.IsEmpty())
			continue;
	#ifdef DEBUGXXX
		Log(LOGINFO, "'%s' = '%s'", name, value);
	#endif
		vara options( ExtractString(select[i], "", "", "</select"), "<option");
		for (int o=1; o<options.length(); ++o)
			if (strstr(options[o], "selected="))
				GetSearchString(options[o], "value=", value, "\"", "\"");
		lists[0].Add(vars(name));
		lists[1].Add(vars(value));
		}

	for (int i=1; i<input.length(); ++i) {
		CString line = GetToken(input[i], 0, '>');
		CString value, name;
		if (name.IsEmpty())
			GetSearchString(line, " name=", name, "\"", "\"");
		if (name.IsEmpty())
			GetSearchString(line, " id=", name, "\"", "\"");
		if (name.IsEmpty())
			continue;
	#ifdef DEBUGXXX
		Log(LOGINFO, "'%s' = '%s'", name, value);
	#endif
		GetSearchString(line, "value=", value, "\"", "\"");

		int found = lists[0].indexOf(name);
		if (found<0)
			{
			lists[0].Add(vars(name));
			lists[1].Add(vars(value));
			}
		else
			{
			lists[0][found] = name;
			lists[1][found] = value;
			}
		}

	return lists[0].GetSize();
}

const char *DownloadFile::GetFormValue2(const char *name)
{
	int size = lists[0].GetSize();
	for (int i=0; i<size; ++i)
		if (stricmp(name, lists[0][i])==0)
			return lists[1][i];
	return NULL;
}

int DownloadFile::SetFormFile(const char *name, const char *remotefile, const char *mime, const char *localfile)
{
	file.name = name;
	file.remote = remotefile;
	file.local = localfile;
	file.mime = mime;
	return TRUE;
}

int DownloadFile::SetFormValue(const char *name, const char *val)
{
	int size = lists[0].GetSize();
	for (int i=0; i<size; ++i)
		if (stricmp(name, lists[0][i])==0) {
			if (val)
				lists[1][i] = val;
			else
				{
				// remove
				for (int j=i+1; j<size; ++j) {
					lists[0][j-1] = lists[0][j];
					lists[1][j-1] = lists[1][j];
					}
				lists[0].SetSize(size-1);
				lists[1].SetSize(size-1);
				}
			return 1;
			}
	if (val) {
		lists[0].Add(name);
		lists[1].Add(val);
		}
	return -1;
}

CString DownloadFile::FormValues(void)
{
	BOOL multipart = strstr(enctype, "multipart")!=NULL;

	CString out;
	for (int i=0; i<lists[0].GetSize(); ++i)
		if (multipart)
			out += FormValue(lists[0][i], lists[1][i]);
		else
			out += (i>0?"&":"")+lists[0][i]+"="+url_encode(lists[1][i]);
	return out;
}



int DownloadFile::DownloadINET( const char *urlstr, const char *file)
{
	vars url = urlstr;

	size = 0;
	DWORD dwStatus = 0;
	DWORD dwServiceType;
	CString strServer;
	CString strObject;
	INTERNET_PORT nPort;
	CString strUsername = "";
	CString strPassword = "";
	if (!file || *file==0)
		file = NULL;
	memory[0] = 0;

	// process POST
	BOOL urlpost = FALSE;
	CString urlhdr;
	inetmemorybin urlvar;
	int POST = url.Find(POSTMARK);
	if (POST>0)
		{
		urlhdr = "Content-Type: application/x-www-form-urlencoded" ;
		vars data = url.Right(url.GetLength()-POST-strlen(POSTMARK));
		urlvar.write( data, strlen(data) );
		url = url.Left(POST);
		urlpost = TRUE;
		}
	POST = url.Find(SOAPMARK);
	if (POST>0)
		{
		urlhdr = "Content-Type: application/soap+xml; charset=utf-8";
		urlvar.write( url.Right(url.GetLength()-POST-strlen(SOAPMARK)) );
		url = url.Left(POST);
		urlpost = TRUE;
		}
	POST = url.Find(POSTFILEMARK);
	if (POST>0)
		{
		urlhdr = "Content-Type: multipart/form-data; boundary=";
		urlhdr += SEPFORM;
		urlpost = PostFile(urlvar);
		urlvar.write( url.Right(url.GetLength()-POST-strlen(POSTMARK)) );
		urlvar.write( FormEnd() ); 
		url = url.Left(POST);
#ifdef DEBUG
		CFILE f; 
		f.fopen("post.url", CFILE::MWRITE);
		f.fwrite(urlvar.memory, 1, urlvar.size);
		f.fclose();
#endif
		}


	if (!AfxParseURLEx(url, dwServiceType, strServer, strObject, nPort,
		strUsername, strPassword))
		return -1;

	const char *referer = NULL;
	if (usereferer)
		referer = lasturl;

	HINTERNET m_hSession = hSession ? hSession : SessionMalloc();
	if (!m_hSession) 
		return -1;

	int err = IDS_ERR_DOWNLOAD;
	HINTERNET hConnection = NULL;
	if (m_hSession)
		SHIELDURL ( hConnection = InternetConnect(m_hSession, 
											strServer,  // Server
											nPort,
											strUsername,     // Username
											strPassword,     // Password
											INTERNET_SERVICE_HTTP, //dwServiceType,
											0,        // Synchronous
											0););    // No Context    
	HINTERNET hRequest = NULL;
	LPCSTR wstr[] = { "*/*", 0 };
	DWORD wflags = 0;
	if (nPort==INTERNET_DEFAULT_HTTPS_PORT)
		wflags = INTERNET_FLAG_SECURE|INTERNET_FLAG_IGNORE_CERT_CN_INVALID;
	if (hConnection)
		SHIELDURL( hRequest = HttpOpenRequest(hConnection, 
											urlpost ? "POST" : "GET",
											strObject,
                                            NULL, //"HTTP/1.1",
											referer,    // No Referer
											wstr, // Accept
																	// anything
											/*INTERNET_FLAG_RELOAD|*/INTERNET_FLAG_NO_CACHE_WRITE|INTERNET_FLAG_DONT_CACHE|INTERNET_FLAG_PRAGMA_NOCACHE|wflags, // Flags
											0););   // No Context
	if (hRequest)
		{
		dwStatus = 401;
		DWORD dwBufLen = sizeof(dwStatus);
		const char *HEADERS = !urlpost ? headers : urlhdr;
		SHIELDURL( 
		//DWORD dwContentLen = 0;
		HttpSendRequest(hRequest,
							HEADERS,              // extra headers
							!HEADERS ? 0 : strlen(HEADERS),  // Header length
							!urlpost ? NULL : urlvar.memory,              // Body
							!urlpost ? 0 : urlvar.size ); // Body length    
	/*
		if (!HttpQueryInfo(hRequest, 
						HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, 
						(LPVOID)&dwContentLen,
						&dwBufLen, 0))
			dwContentLen = 0;
		*/
		)

		SHIELDURL( 
		if (!HttpQueryInfo(hRequest, 
				HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, 
				(LPVOID)&dwStatus,
				&dwBufLen, 0))
			dwStatus = 401;
		)

		SHIELDURL( 
		if (dwStatus<400)
			err = DownloadCoreNoRetry(hRequest, file, size, memory, idata);
		)
		}


	// clean up
	if (hRequest) 
		SHIELDURL( InternetCloseHandle(hRequest); );
	if (hConnection) 
		SHIELDURL( InternetCloseHandle(hConnection); );
	if (!hSession) 
		SessionFree(m_hSession);
	
	//Log(LOGERR, "%d", err)); ///////
	lasturl = url;
	return err;
}


int DownloadFile::DownloadNoRetry( const char *urlstr, const char *file)
{
	// user + passwd (last minute and only in memory
	CString url;
	if (strstr(urlstr,USER) || strstr(urlstr,ACCT))
		urlstr = url = URLDAT(urlstr);

	const char *uhttp = "umhttp:";
	if (strnicmp(uhttp,urlstr,strlen(uhttp))!=0)
		return DownloadINET(urlstr, file);

	// umhttp:
	urlstr += 2;

	// alloc tmp
	static int TMPCNT = 0;
	// guarantee no file conflict

	if (tmpfile.IsEmpty()) 
		{
		ThreadLock();
		int tmpcnt = TMPCNT++;
		ThreadUnlock();
		tmpfile = MkString("TMP%d.htm", tmpcnt);
		}

	// download
	if (!file || *file==0) file = tmpfile;
	if (URLDownloadToFile(NULL, urlstr, file, NULL, NULL)!=S_OK)
		return TRUE;

	// read in memory
	CFILE f;
	if (!f.fopen(file, CFILE::MREAD)) return TRUE;
	size = f.fsize();

	memory = DownloadRealloc(memory, size +2);

	f.fseek(0, SEEK_SET);
	f.fread(memory, 1, size);

	if (size<=0) return TRUE;

	return FALSE;
}

/*
void TestDownload(const char *url, const char *file)
{
	HINTERNET Initialize,Connection,File;
	DWORD dwBytes;
	char ch;
	FILE *f = fopen(file, CFILE::MWRITE);
	Initialize = SessionMalloc();
	Connection = InternetConnect(Initialize,"www.marketwatch.com",INTERNET_DEFAULT_HTTP_PORT,
	NULL,NULL,INTERNET_SERVICE_HTTP,0,0);

	File = HttpOpenRequest(Connection,NULL,"/investing/stock/HPQ/analystestimates?subview=estimates",NULL,NULL,NULL,0,0);

	if(HttpSendRequest(File,NULL,0,NULL,0))
	{
	while(InternetReadFile(File,&ch,1,&dwBytes))
	{
	if(dwBytes != 1)break;
	putc(ch, f);
	}
	}
	InternetCloseHandle(File);
	InternetCloseHandle(Connection);
	InternetCloseHandle(Initialize);
	fclose(f);
}

void TestDownload2(const char *url, const char *file)
{
	URLDownloadToFile(NULL, url, file, NULL, NULL);
}

void TestDownload3(const char *url, const char *file)
{
	HINTERNET hINet, hConnection, hData;
	CHAR buffer[1024*100] ; 
	CString m_strContents ;
	DWORD dwRead;

	CFILE f; 
	f.fopen(file, CFILE::MWRITE);

	hINet = InternetOpen("InetURL/1.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0 );
	if ( !hINet )
	{
	AfxMessageBox("InternetOpen Failed");
	return;
	}
	try 
	{
	hConnection = InternetConnect( hINet, "www.marketwatch.com", 80, " "," ", INTERNET_SERVICE_HTTP, 0, 0 );
	if ( !hConnection )
	{
	InternetCloseHandle(hINet);
	return;
	}
	// Get data
	hData = HttpOpenRequest( hConnection, "GET", "/investing/stock/HPQ/analystestimates?subview=estimates", NULL, NULL, NULL, INTERNET_FLAG_KEEP_CONNECTION, 0 );
	if ( !hData )
	{
	InternetCloseHandle(hConnection);
	InternetCloseHandle(hINet);
	return;
	}
	HttpSendRequest( hData, NULL, 0, NULL, 0);
	while( InternetReadFile( hData, buffer, 255, &dwRead ) )
	{
	if ( dwRead == 0 )
	return;
	buffer[dwRead] = 0;
	f.fwrite(buffer, dwRead, 1);
	m_strContents += buffer;
	}
	}
	catch( CInternetException* e)
	{
	e->ReportError();
	e->Delete();
	}
	InternetCloseHandle(hConnection);InternetCloseHandle(hINet);
	InternetCloseHandle(hData);
}
*/




//
// Multiprocessing management
// 

#define ForegroundPriority THREAD_PRIORITY_ABOVE_NORMAL
#define BackgroundPriority THREAD_PRIORITY_BELOW_NORMAL
#define BkPrintingPriority THREAD_PRIORITY_LOWEST

static int m_numthreads = 0;


void WaitForThreads()
{
	while (m_numthreads>0)
		WaitThread();
}

int WaitThread()
{
	int n = 100;
	Sleep(n);
	return n;
} 

int SetForeground(void)
{
	return SetThreadPriority( GetCurrentThread(), ForegroundPriority);
}

int SetBackground(void)
{
	return SetThreadPriority( GetCurrentThread(), BackgroundPriority);
}

int SetBkPrinting(void)
{
	return SetThreadPriority( GetCurrentThread(), BkPrintingPriority);
}

int SetPriority(int pr)
{
	return SetThreadPriority( GetCurrentThread(), pr);
}


static AFX_THREADPROC m_threadproc;
static UINT RunThread( LPVOID pParam )
{
	++m_numthreads;
	m_threadproc(pParam);
	--m_numthreads;
	return FALSE;
}

CWinThread* BeginThread( AFX_THREADPROC pfnThreadProc, LPVOID pParam )
{
#ifdef DISABLE_THREADS
	return NULL;
#else
	m_threadproc = pfnThreadProc;	
	CWinThread* th = AfxBeginThread( RunThread, pParam, BackgroundPriority);
	if (!th) ::MessageBox( NULL, "No thread?!?", "ERROR", MB_OK );
	return th;
#endif
}






//
// Registry access
// 

const TCHAR *RegKey()
{
	return RegApp();
}

const TCHAR *RegApp()
{
	return APPNAME; //AFX_IDS_APP_TITLE
}

BOOL RegDeleteTree(HKEY phKey, const TCHAR *strKey)
{
    TCHAR    achKey[MAX_PATH]; 
    TCHAR    achClass[MAX_PATH] = "";  /* buffer for class name   */ 
    DWORD    cchClassName = MAX_PATH;  /* length of class string  */ 
    DWORD    cSubKeys;                 /* number of subkeys       */ 
    DWORD    cbMaxSubKey;              /* longest subkey size     */ 
    DWORD    cchMaxClass;              /* longest class string    */ 
    DWORD    cValues;              /* number of values for key    */ 
    DWORD    cchMaxValue;          /* longest value name          */ 
    DWORD    cbMaxValueData;       /* longest value data          */ 
    DWORD    cbSecurityDescriptor; /* size of security descriptor */ 
    FILETIME ftLastWriteTime;      /* last write time             */ 
    HKEY hKey;
 
    DWORD i; 
    DWORD retCode;

    if ( RegOpenKey(phKey, strKey, &hKey) != ERROR_SUCCESS)
        {
        //_ftprintf(log, "ERROR: Could not open-delete key \"%s\"\n", strKey);
        return 0; 
        }

    // Get the class name and the value count. 
    if (RegQueryInfoKey(hKey,        /* key handle                    */ 
        achClass,                /* buffer for class name         */ 
        &cchClassName,           /* length of class string        */ 
        NULL,                    /* reserved                      */ 
        &cSubKeys,               /* number of subkeys             */ 
        &cbMaxSubKey,            /* longest subkey size           */ 
        &cchMaxClass,            /* longest class string          */ 
        &cValues,                /* number of values for this key */ 
        &cchMaxValue,            /* longest value name            */ 
        &cbMaxValueData,         /* longest value data            */ 
        &cbSecurityDescriptor,   /* security descriptor           */ 
        &ftLastWriteTime)        /* last write time               */ 
        != ERROR_SUCCESS)       
        {
        //_ftprintf(log, "ERROR: Could not query-delete key \"%s\"\n", strKey);
        RegCloseKey(hKey);
        return 0; 
        }
    
    // Enumerate the child keys, looping until RegEnumKey fails. Then 
    // get the name of each child key and copy it into the list box. 
    for (i = 0, retCode = ERROR_SUCCESS; retCode == ERROR_SUCCESS; i++) 
        { 
        *achKey = 0;
        retCode = RegEnumKey(hKey, i, achKey, MAX_PATH); 
        if (retCode == (DWORD)ERROR_SUCCESS) 
            RegDeleteTree(hKey, achKey);
        } 
 
    if (RegDeleteKey(phKey, strKey) != ERROR_SUCCESS)
        {
        //_ftprintf(log, "ERROR: Could not delete key \"%s\"\n", strKey);
        }
    return 1;
}
	

BOOL RegReset(void)
{
    CString regdata;
	regdata.Format( "Software\\%s\\%s", RegKey(), RegApp());
    return RegDeleteTree(HKEY_CURRENT_USER, regdata);
}


BOOL RegKey(TCHAR *section, HKEY *hKey)
{
    CString regdata;
	regdata.Format( "Software\\%s\\%s\\%s", RegKey(), RegApp(), section);
    return RegCreateKey(HKEY_CURRENT_USER, regdata, hKey) == ERROR_SUCCESS;
}


int RegGetInt(TCHAR *section, TCHAR *name, int def)
{
	//return !AfxGetApp()->GetProfileInt(section, name, def);
    HKEY hKey;
    if (!RegKey(section, &hKey))
        return def;

	DWORD dval = 0;
	DWORD type = 0;
	DWORD size = sizeof(dval);
    if (RegQueryValueEx(hKey, name, 0, &type, (BYTE *)&dval, 
		&size) != ERROR_SUCCESS || type!=REG_DWORD)
		dval  = def;

	RegCloseKey(hKey);
	return (int)dval;
}

BOOL RegSetInt(TCHAR *section, TCHAR *name, int val)
{
	//return !AfxGetApp()->WriteProfileInt(section, name, val);
    HKEY hKey;
    if (!RegKey(section, &hKey))
        return TRUE;

	DWORD dval = val;
    BOOL ret = RegSetValueEx(hKey, name, 0, REG_DWORD, 
        (BYTE *)&dval, sizeof(dval)) != ERROR_SUCCESS;

	RegCloseKey(hKey);
	return ret;
}

CString RegGetString(TCHAR *section, TCHAR *name, const TCHAR *def)
{
	//return !AfxGetApp()->GetProfileString(section, name, def);
    HKEY hKey;
    if (!RegKey(section, &hKey))
        return CString(def);

	TCHAR str[1024];
	DWORD type = 0;
	DWORD size = sizeof(str);
    if (RegQueryValueEx(hKey, name, 0, &type, (BYTE *)str, 
		&size) != ERROR_SUCCESS || type!=REG_SZ)
		{
		if (def)
			_tcscpy(str, def);
		else
			*str = 0;
		}

	RegCloseKey(hKey);
	return CString(str);
}

BOOL RegSetString(TCHAR *section, TCHAR *name, const TCHAR *val)
{
	//return !AfxGetApp()->WriteProfileString(section, name, val);
    HKEY hKey;
    if (!RegKey(section, &hKey))
        return TRUE;

	BOOL ret = FALSE;
	if (!val)
		ret = RegDeleteValue(hKey, name) != ERROR_SUCCESS;
	else
		ret = RegSetValueEx(hKey, name, 0, REG_SZ, 
			(BYTE *)val, _tcslen(val)*sizeof(*val)+1) != ERROR_SUCCESS;

	RegCloseKey(hKey);
	return ret;
}

void *RegGetData(TCHAR *section, TCHAR *name, void *data, int rsize)
{
    HKEY hKey;
    if (!RegKey(section, &hKey))
        return NULL;

    DWORD size = 0;
    if (RegQueryValueEx(hKey, name, 0, NULL, NULL, &size)!=ERROR_SUCCESS ||
       (size<=0 || (rsize>0 && size>(DWORD)rsize)))
	    {
		RegCloseKey(hKey);
		return NULL;
		}

    void *ptr = !data ? malloc( size ) : data;

    if (!ptr || RegQueryValueEx(hKey, name, 0, 
        NULL, (BYTE *)ptr, &size) != ERROR_SUCCESS)
        {
        if (!data) free( ptr );
		ptr = NULL;
        }

	RegCloseKey(hKey);
    return ptr;
}


BOOL RegSetData(TCHAR *section, TCHAR *name, void *ptr, int size)
{
    if (size<=0 || !ptr)
        return TRUE;

    HKEY hKey;
    if (!RegKey(section, &hKey) || !ptr)
        return TRUE;

    BOOL ret = RegSetValueEx(hKey, name, 0, REG_BINARY, 
        (BYTE *)ptr, size) != ERROR_SUCCESS;

	RegCloseKey(hKey);
	return ret;
}


int RegGetStringList(TCHAR *section, TCHAR *name, CStringList *list)
{
	int i;
    list->GetHeadPosition();
    for (i=0; TRUE; ++i)
        {
        TCHAR id[128];
        _stprintf(id, "%s%d", name, i);
        CString str = RegGetString(section, id);
        if (str.IsEmpty())
            return i;
        list->AddTail(str);
        }
    return i;
}

BOOL RegSetStringList(TCHAR *section, TCHAR *name, CStringList *list, int n)
{
	if (n<0) n = (int)list->GetCount();
    n = min(n, (int)list->GetCount());
    POSITION p = list->GetHeadPosition();
    for (int i=0; i<n; ++i)
        {
        TCHAR id[128];
        _stprintf(id, "%s%d", name, i);
        CString &str = list->GetNext(p);
        if (RegSetString(section, id, str))
            return TRUE;
        }
    return FALSE;
}


int RegGetComboBox(TCHAR *section, TCHAR *name, CComboBox *list)
{
	int i;
    list->ResetContent();
    for (i=0; TRUE; ++i)
        {
        TCHAR id[128];
        _stprintf(id, "%s%d", name, i);
        CString str = RegGetString(section, id);
        if (str.IsEmpty())
            return i;
        list->AddString(str);
        }
    return i;
}


BOOL RegSetComboBox(TCHAR *section, TCHAR *name, CComboBox *list, int n)
{
	CString str;
	if (n<0) n = list->GetCount();
    n = min(n, list->GetCount());
    for (int i=0; i<n; ++i)
        {
        TCHAR id[128];
        _stprintf(id, "%s%d", name, i);
		list->GetLBText(i, str);
        if (RegSetString(section, id, str))
            return TRUE;
        }
    return FALSE;
}





//
// Error box
// 
static BOOL use_console = FALSE;


BOOL SetOutput(const char *file)
{

	if (!file || *file==0)
		{
		use_console = TRUE;
		//MessageBox("TradeMaster started...");
		return use_console;
		}

	HANDLE h = CreateFile( CString(file), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, 
		OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (!h || h==INVALID_HANDLE_VALUE)
		return FALSE;

	SetFilePointer(h, 0, 0, FILE_END);
	use_console = SetStdHandle(STD_ERROR_HANDLE, h);
	return use_console;
}


//
// Log management
// 
#include <conio.h>
CString DEBUGSTR, DEBUGCRASH;	// debug string in case of problems

int THREADNUM = 0;

typedef struct { DWORD id; const char *string; } threaddef;

static CArrayList <threaddef> ThreadList;

threaddef *ThreadGet(int create = FALSE)
{
	DWORD id = GetCurrentThreadId();
	for (int i=ThreadList.length()-1; i>=0; --i)
		if (ThreadList[i].id==id)
			return &ThreadList[i];

	if (!create)
		return NULL;

	threaddef t;
	t.id = id;
	t.string = NULL;
	return ThreadList.push(t);
}

int ThreadSetNum(void)
{
	int num = 0;
	for (int i=ThreadList.length()-1; i>=0; --i)
		if (ThreadList[i].string!=NULL)
			++num;
	return num;
}

void ThreadSet(const char *string)
{
	threaddef *t = ThreadGet(TRUE);
	t->string = string;
}

static FILE *fopenretry(const char *file, const char *mode)
{
	PROFILE("fopenretry()");
	for (int retry=0; retry<5; ++retry)
		{
		FILE *f = fopen(file, mode);
		// if ok or readonly, return with no wait
		if (f!=NULL || toupper(*mode)=='R') 
			return f;
		if (!CFILE::exist(file))
			return NULL;
		// wait
		ShowProgress("Sleep(1000) FILE@fopenretry()");
		Sleep(1000);
		}
	return NULL;
}

static CCriticalSection LOG;

CString LOGFILE = APPNAME".LOG";

void SetLog(const char *app)
{
	LOGFILE = GetFileName(app)+".LOG";
	LOGFILE.MakeUpper();
}

void Log(const char *memory)
{
		FILE *f;
		if ((f  = fopenretry(LOGFILE, "at"))!=NULL) 
			{
			fputs(memory, f);
			fputs("\n", f);
			fclose(f);
			}
}

void _vsprintf(TCHAR str[], int size, const char *format, va_list &argptr)
{
	__try 
		{
		vsprintf_s (str, size, format, argptr);		
		}
  __except(EXCEPTION_EXECUTE_HANDLER) 
		{ 
		strcpy(str, "Log() CRASHED!!!! ");
		strcpy(str+strlen(str), format);
		}
}

void Log(int index, const char *format, ...)
{

		LOG.Lock();
	//if (index>=LOGERR)
	//	ASSERT( !"Error Logged" );
//	static int logging = -1;
	/*
	static char logfile[MAX_PATH];
	if (logging<0)
		{
		// logging is activated from .INI file
		logging = GetPrivateProfileString("CONFIG", "logfile", "", 
			logfile, sizeof(logfile), UNDOCUMENTED);
		}
	*/
//	if (logging)

		// take out \n if it was there
		//if (text[strlen(text)-1]=='\n')
		//	text[strlen(text)-1]=0;

		TCHAR str[5*1024];
		va_list	argptr;
		va_start (argptr, format);
		_vsprintf(str, sizeof(str)-1, format, argptr);
		va_end (argptr);
		str[sizeof(str)-1] = 0;

		// output to list
		COleDateTime t = COleDateTime::GetCurrentTime();
		CString date = t.Format(LOGDATEFORMAT);

#ifdef _DEBUG
		OutputDebugString(str);
		OutputDebugString("\n");
#endif
		// = !ALERT
		// = !ERROR
		// = .WARN
		// =  info
		const char clog[] = " .!!!";
		CString text = MkString("[%dL] %s = %c", index, (LPCTSTR)date, clog[index]);
		if (index==LOGALERT)
			text += "!!!ALERT!!! ";

		text += (LPCTSTR)str;
		if (index>=LOGALERT)
			{
			text += " = DEBUGSTR:"+DEBUGSTR;
			//ASSERT( !"WTF?");
			}
		if (index>=LOGERR)
			{
			threaddef *t = ThreadGet();
			if (t!=NULL && t->string!=NULL)
				text += " = " +MkString("#%d ", ThreadSetNum())+t->string;
			//ASSERT( !"WTF?");
			}
		text += "\n";
		if (stderr)
			fputs(text, stderr);
		// append to file, if possible
		FILE *f;;
		if ((f  = fopenretry(LOGFILE, "at"))!=NULL) 
			{
			fputs(text, f);
			fclose(f);
			}
		if (index>=LOGALERT)
		  if ((f  = fopenretry(ALERTFILE, "at"))!=NULL) 
			{
			fputs(text, f);
			fclose(f);
			}
		// output to console
		//_cprintf("[%d] %s = %s\n", index, (LPCTSTR)date, (LPCTSTR)str);
		LOG.Unlock();
		
		ASSERT( index!=LOGFATAL );
		// suicide
		if (index==LOGFATAL)
			{
			// throw exception
			((char*)NULL)[0]=0;
			exit(1);
			}
		/*
		// Console Log
		DWORD written;
		HANDLE h = GetStdHandle(STD_ERROR_HANDLE);
		if (h!=NULL && h!=INVALID_HANDLE_VALUE)
			{
			char nl = '\n';
			WriteFile( h, lpszText, strlen(lpszText), &written, 0);
			if (lpszText[strlen(lpszText)-1]!=nl)
				WriteFile( h, &nl, 1, &written, 0);
			}
		*/
}






int MessageBox(LPCTSTR lpszText, BOOL newline)
{
	if (use_console)
		{
		char space[80];
		int len = 75-strlen(lpszText);
		int i;
		for (i=0; i<len; ++i) space[i] = ' ';
		space[i]=0;
		ThreadLock();
		_cprintf("%s%s\r", lpszText, space);
		ThreadUnlock();
		}
	return TRUE;
}

int ErrorBox( LPCTSTR lpszText, UINT nType)
{
	if (!lpszText || *lpszText==0) 
		return IDOK;
	if (use_console)
		{
		Log(LOGERR, "ERRORBOX: %s", lpszText);
		Sleep(5000);
		return IDOK;
		}
	// Message Box
	return ::MessageBox(NULL, lpszText, RegApp(), nType | 
			MB_SETFOREGROUND | MB_TASKMODAL); //MB_TOPMOST
}

int ErrorBox( UINT idText, UINT nType)
{
	return ErrorBox( CString((LPCSTR)idText), nType );
}

int ErrorBox( UINT idText, LPCTSTR lpszText, UINT nType)
{
	CString str;
	str.Format( idText, lpszText);
	return ErrorBox( str, nType );
}


//
// String processing
// 

char *StrUnquote(char *str, char *sep)
{
	// NULL detection
	if (!str) return str;
	char *s = str; 
	char *e = s+strlen(s)-1;

	BOOL changed = TRUE;
	while (changed)
		{
		changed = FALSE;

		// manage ""
		if (s<e && *s=='\"' && *e=='\"')
			{
			++s; --e;
			changed = TRUE;
			}

		// trim left
		while (*s && strchr(sep, *s))
			{
			++s;
			changed = TRUE;
			}

		// trim right
		while (e>s && strchr(sep, *e))
			{
			--e;
			changed = TRUE;
			}
		}

	// return value
	*(e+1) = 0;
	return s;
}


char *GetFileExt(const char *str)
{
    int i;
    for (i=strlen(str)-1; i>0; --i)
		{
        if (str[i]=='\\' || str[i]=='/' || str[i]==':')
			return NULL;
        if (str[i]=='.')
            return (char *)(str+i+1);
		}
    return NULL;
}

char *GetFileNameExt(const char *str)
{
    int i;
    for (i=strlen(str)-1; i>0; --i)
        if (str[i]=='\\' || str[i]=='/' || str[i]==':')
            return (char *)(str+i+1);
    return (char *)str;
}



CString GetFileName(const char *path)
{
	char name[MAX_PATH];
	strcpy(name, GetFileNameExt(path));
	char *ext = strrchr(name,'.');
	if (ext) *ext = 0;
	return CString(name);
}

CString GetFileNoExt(const char *path)		
{
	char name[MAX_PATH];
	strcpy(name, path);
	char *ext = strrchr(name,'.');
	if (ext) *ext = 0;
	return CString(name);
}

char *StrFixChar(char *str, char from, char to)
{
    char *s;
    for (s = str; *s!=0; ++s)
        if (*s==from)
            *s = to;

	// trim string
	for ( ; *str!=0 && isspace(*str); ++str);
	int n;
	for (n=strlen(str)-1; n>=0 && isspace(str[n]); --n);
	str[n+1] = 0;
    return str;
}


char *StrFixUrl(char *str)
{
	return StrFixChar(str, '\\', '/');
}

char *StrFixPath(char *str)
{ 
	return StrFixChar(str, '/', '\\');
}


int csectionviolation = 0;
#ifdef USESIMPLETHREADLOCK 
volatile int csection = 0, csection2;

void ThreadLock(void) 
{
	// simulate csection
	while (csection>0 || csection2>0) 
		Sleep(100);
	++csection2;
	while (csection>0 || csection2>1) 
		Sleep(100);
	++csection;

	// keep track of violations
	if (csection>1)
		++csectionviolation;
} 

void ThreadUnlock(void) 
{
	--csection;
	--csection2;
}

#else

static CCriticalSection CC;

void ThreadLock(void) 
{
	CC.Lock();
}

void ThreadUnlock(void) 
{
	CC.Unlock();
}


#endif

/*

BOOL GetFileNameFromHandle(HANDLE hFile, char *pszFilename) 
{
  BOOL bSuccess = FALSE;
  HANDLE hFileMap;

  // Get the file size.
  DWORD dwFileSizeHi = 0;
  DWORD dwFileSizeLo = GetFileSize(hFile, &dwFileSizeHi); 

  if( dwFileSizeLo == 0 && dwFileSizeHi == 0 )
  {
     printf("Cannot map a file with a length of zero.\n");
     return FALSE;
  }

  // Create a file mapping object.
  hFileMap = CreateFileMapping(hFile, 
                    NULL, 
                    PAGE_READONLY,
                    0, 
                    1,
                    NULL);

  if (hFileMap) 
  {
    // Create a file mapping to get the file name.
    void* pMem = MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 1);

    if (pMem) 
    {
      if (GetMappedFileName (GetCurrentProcess(), 
                             pMem, 
                             pszFilename,
                             MAX_PATH)) 
      {

        // Translate path with device name to drive letters.
        TCHAR szTemp[BUFSIZE];
        szTemp[0] = '\0';

        if (GetLogicalDriveStrings(BUFSIZE-1, szTemp)) 
        {
          TCHAR szName[MAX_PATH];
          TCHAR szDrive[3] = TEXT(" :");
          BOOL bFound = FALSE;
          TCHAR* p = szTemp;

          do 
          {
            // Copy the drive letter to the template string
            *szDrive = *p;

            // Look up each device name
            if (QueryDosDevice(szDrive, szName, BUFSIZE))
            {
              UINT uNameLen = _tcslen(szName);

              if (uNameLen < MAX_PATH) 
              {
                bFound = _tcsnicmp(pszFilename, szName, 
                    uNameLen) == 0;

                if (bFound) 
                {
                  // Reconstruct pszFilename using szTemp
                  // Replace device path with DOS path
                  TCHAR szTempFile[MAX_PATH];
                  _stprintf(szTempFile,
                            TEXT("%s%s"),
                            szDrive,
                            pszFilename+uNameLen);
                  _tcsncpy(pszFilename, szTempFile, MAX_PATH);
                }
              }
            }

            // Go to the next NULL character.
            while (*p++);
          } while (!bFound && *p); // end of string
        }
      }
      bSuccess = TRUE;
      UnmapViewOfFile(pMem);
    } 

    CloseHandle(hFileMap);
  }
  //printf("File name is %s\n", pszFilename);
  return(bSuccess);
}
*/

/*
BOOL TruncateFile(const char *file, int filesize)
{
		// truncate (if needed)
		long size1 = 0;
		long size2 = 0;
		if (DEBUGLOAD) 
			Log(LOGWARN, "TRUNCATING: file %s to %d", file, filesize);
		HANDLE hFile = CreateFile(file, GENERIC_READ | GENERIC_WRITE , 0, NULL,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if ( h == INVALID_HANDLE_VALUE ) 
		  return FALSE;

		BOOL error = FALSE;
		size1 = GetFileSize(hFile, NULL); // No HiOrder DWORD required

		if (filesize<size1)
				{
				// check for EOL to avoid messing up data
				SetFilePointer(hFile, filesize, 0, FILE_BEGIN); // == 0xFFFFFFFF;
				SetEndOfFile(hFile); // == FALSE;
				size2 = GetFileSize(hFile, NULL); // No HiOrder DWORD required
				if (size2!=filesize)
					Log(LOGALERT, "Could not trim file %s, trying old fashion way...", file);
				}
			else
				{
				Log(LOGALERT, "BAD TRUNCATION: cursize<trucsize for file %s", file);
				}
		CloseHandle(hFile);
		return TRUE;
}
*/

BOOL CFILE::gettime(const char* pFilename, FILETIME *writetime, FILETIME *accesstime) 
{
	ZeroMemory(writetime, sizeof(*writetime));
	if (accesstime)
		ZeroMemory(accesstime, sizeof(*accesstime));
	HANDLE h = CreateFile(pFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                 FILE_ATTRIBUTE_NORMAL, NULL);
    if ( h == INVALID_HANDLE_VALUE ) 
	  return FALSE;
  
    BOOL ok = GetFileTime(h, NULL, accesstime, writetime);
    CloseHandle(h);
	return ok;
}

BOOL CFILE::settime(const char* pFilename, FILETIME *writetime) 
{
	HANDLE h = CreateFile(pFilename, GENERIC_WRITE, 0, NULL, OPEN_EXISTING,
             FILE_ATTRIBUTE_NORMAL, NULL);

	if ( h == INVALID_HANDLE_VALUE ) 
	  return FALSE;

	BOOL ok = SetFileTime(h, NULL, NULL, writetime);
	CloseHandle(h);
    return ok;
}


BOOL CFILE::exist(const char  *pFilename)
{
	PROFILE("CFILE::exist()");
#if 0
	WIN32_FIND_DATA lpFind;
	HANDLE h = FindFirstFile(pFilename, &lpFind);
	if (h==INVALID_HANDLE_VALUE)
		return FALSE;
	FindClose(h);
	return TRUE;
#else
  DWORD dwAttrib = GetFileAttributes(pFilename);

  return (dwAttrib != INVALID_FILE_ATTRIBUTES && 
         !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
#endif
}



BOOL CFILE::getheader(const char *file, CString &header)
{
	CFILE f;
	if (!f.fopen(file, CFILE::MREAD))
		return FALSE;
	header = f.fgetstr();
	return TRUE;
}

BOOL CFILE::MDEBUG = 0;


#if 1 // Windows 

BOOL CFILE::fopen(const char *file, int mode)
	{	// enum {		MREAD = 0,		MWRITE,			MAPPEND,						MUPDATE };
		char *stext[] = { "READ",		"WRITE",		"APPEND",						"UPDATE" };
		DWORD smode[] = { GENERIC_READ,	GENERIC_WRITE,	GENERIC_READ | GENERIC_WRITE,	GENERIC_READ | GENERIC_WRITE };
		int smode2[] = { OPEN_EXISTING,	CREATE_ALWAYS,	OPEN_ALWAYS,					OPEN_ALWAYS };

		ASSERT( f == NULL );
		if (f)
			{
			Log(LOGALERT, "CFILE: NOT properly closed file %s", name());
			fclose();
			}

		DWORD attr = FILE_ATTRIBUTE_NORMAL;
		// no big difference, actually worse with flags
		fext = tolower(file[strlen(file)-1]); //last char of .ext
		if (fext=='x') attr |= FILE_FLAG_RANDOM_ACCESS;
		//if (last=='v') attr |= FILE_FLAG_RANDOM_ACCESS; //FILE_FLAG_SEQUENTIAL_SCAN;

		int retry = 0;
		PROFILE("fopen()");
		// for MREAD share file, all others NOT
doretry:
		f = CreateFile(fname = file, smode[mode], mode==MREAD ? FILE_SHARE_READ : 0, NULL, smode2[mode], attr, NULL);
		if ( f == INVALID_HANDLE_VALUE ) 
			{
			// skip rety if file does not exist
			int error = GetLastError();
			if (mode==MREAD)
			  if (error==ERROR_FILE_NOT_FOUND || error==ERROR_PATH_NOT_FOUND)
				{
				// file does not exist
				f = NULL;
				return FALSE;
				}
			// retry 5 times
			if (retry<10)
				{
				ShowProgress("Sleep(1000) CFILE@fopen()");
				Sleep(1000);
				++retry;
				goto doretry;
				}
			// log error
			Log(LOGALERT, "Could not %s file %s (retried for 10 sec)", stext[mode], file);
			f = NULL;
			return FALSE;
			}

		// for MAPPEND and UPDATE move to end of file
		if (mode==MAPPEND)
			fseek(0, SEEK_END);

		fbuffptr = -1;
		return TRUE;
	}


void CFILE::fclose()
	{
		if (f) CloseHandle(f);
		f = NULL;
	}

int CFILE::fwrite(const void *buff, int size, int count)
{ 
	PROFILE("fwrite()");
	DWORD bytes = size*count;
	if (MDEBUG) 
		if (fext=='x' || fext=='v')
		{
		CString extra;
		unsigned *idx = (unsigned *)buff;
		const char *sextra = (const char *)buff;
		if (fext=='x') 
			sextra = extra = MkString("[%u, %u, %u]", idx[0], idx[1], idx[2]);
		Log(LOGINFO, "fwrite %db %s %.25s", bytes, fname, extra);
		}
	if (bytes<=0) return 0;
	if (!WriteFile(f,  buff, bytes, &bytes,  NULL))
		{
		Log(LOGALERT, "CFILE: WriteFile failed?!? Disk Full?");
		return 0;
		}
	return bytes/size; 
}

int CFILE::fread(void *buff, int size, int count)
{
	PROFILE("fread()");
	DWORD oldbytes, bytes;
	bytes = oldbytes = size*count;
	if (bytes<=0) return 0;
	if (!ReadFile(f,  buff,  oldbytes, &bytes,  NULL))
		{
		Log(LOGALERT, "CFILE: ReadFile failed?!? Disk Corrupt? Err:%d file=%s bytes=%d -> bytes=%d", GetLastError(), name(), oldbytes, bytes);
		return 0;
		}
	if (MDEBUG) 
		if (fext=='x' || fext=='v')
		{
		CString extra;
		unsigned *idx = (unsigned *)buff;
		const char *sextra = (const char *)buff;
		if (fext=='x') 
			sextra = extra = MkString("[%u, %u, %u]", idx[0], idx[1], idx[2]);
		Log(LOGINFO, "fread %db %s %.25s", bytes, fname, sextra);
		// ASSERTS
		/*
		if (strstr(fname, "\\RANK\\")!=NULL && strstr(sextra,"#")!=NULL)
			fext=fext;
		if (strstr(fname, "#MS.idx")!=NULL && strstr(fname, "#MS.idx.idx")==NULL)
			fext=fext;
		*/
		}
	return bytes/size;
}
	
int CFILE::fseek(int offset, int origin)
{ 
	fbuffptr = -1;
	if (MDEBUG) 
		if (fext=='x' || fext=='v')
		{
		Log(LOGINFO, "fseek %db [%d] %s", offset, origin, fname);
		}
	return SetFilePointer(f, offset, NULL, origin);
}

int CFILE::ftell(void)
{
	int pos = SetFilePointer(f, 0, NULL, FILE_CURRENT);
	return pos - ((fbuffptr<0) ? 0 : (fbuffsize-fbuffptr));
}

int CFILE::fsize(void)
{
	return GetFileSize(f, NULL);
}

double CFILE::ftime(void) 
{
	FILETIME writetime;
	ZeroMemory(&writetime, sizeof(writetime));
    if (GetFileTime(f, NULL, NULL, &writetime))
		return COleDateTime(writetime);
	return 0;	
}


int CFILE::ftruncate(int filesize)
{
	int pos = ftell();
	if (pos>filesize)
		Log(LOGALERT, "BAD TRUNCATION: cursize<trucsize for file %s", fname);
	fseek(filesize);
	SetEndOfFile(f); // == FALSE;
	int newfilesize = GetFileSize(f, NULL); // No HiOrder DWORD required
	if (newfilesize!=filesize)
		Log(LOGALERT, "BAD TRUNCATION: Could not truncate file %s...", fname);
	return TRUE;
}

inline char CFILE::fgetc()
{
	if (fbuffptr==FILEBUFFLEN || fbuffptr<0)
		{
		// read new data
		if (fbuffptr<0)
			{
			// align with buffer
			int pos = ftell();
			int alignedpos = (pos/FILEBUFFLEN)*FILEBUFFLEN;
			fbuffptr = pos - alignedpos;
			}
		else
			{
			fbuffptr = 0;
			}
		// replenish buffer
		fbuffsize=fbuffptr+fread(fbuff+fbuffptr, 1, FILEBUFFLEN-fbuffptr);
		//if (fbuffsize<FILEBUFFLEN)
		//	fbuff[fbuffptr+bytes] = (unsigned char)EOF;
		}
	// EOF special condition
	if (fbuffptr>=fbuffsize)
		return (unsigned char)EOF;
	// normal char 
	return fbuff[fbuffptr++];
}


#else // fopen/fclose  implementation

BOOL CFILE::fopen(const char *file, int mode)
	{
		const char *smode[] = { "rb", "wb", "ab", "r+b" };

		ASSERT( f == NULL );
		if (f)
			{
			Log(LOGALERT, "CFILE: NOT properly closed file %s", name());
			::fclose(f);
			}
		f = ::fopenretry(fname = file, smode[mode]);
		return isopen();
	}

void CFILE::fclose()
	{
		if (f) ::fclose(f);
		f = NULL;
	}

int CFILE::fwrite(const void *buff, int size, int count)
{ 
	return ::fwrite(buff, size, count, f); 
}

int CFILE::fread(void *buff, int size, int count)
{
	return ::fread(buff, size, count, f);
}
	
int CFILE::fseek(int offset, int origin)
{ 
	return ::fseek(f, offset, origin);
}

int CFILE::ftell(void)
{
	return ::ftell(f);
}

inline char CFILE::fgetc()
{
	return fgetc(f);
}

#endif



CFILE::CFILE(void)
	{
		f = NULL;
		buffer = NULL;
		bufferlen = bufferlenmax = 0;
	}

CFILE::~CFILE(void)
	{
		if (f) fclose();
		if (buffer) free(buffer);
	}

int CFILE::fputstr(char const *line)
{
	PROFILE("fputstr()");
	char *nl = FILEEOL;
#ifdef USEWRITEBUFFER
	int slen = strlen(line);
	int len = slen + FILEEOLSIZE;
	while (len>=bufferlenmax)
		buffer = (char *)realloc(buffer, bufferlenmax += FILEBUFFLEN);
	memcpy(buffer, line, slen);
	memcpy(buffer+slen, FILEEOL, FILEEOLSIZE);
	return fwrite(buffer, 1, len);
#else
	int len = fwrite(line, 1, strlen(line));
	len += fwrite(nl, 1, FILEEOLSIZE);		
#endif
	return len;
}




// -1:EOF 0:readmore 1:done
int CFILE::fgetstrbuff(char *buff, int maxbuff)
{
	PROFILE("fgetstrbuff()");
	register int ibuff = 0;
	while (TRUE)
		{
		register unsigned char c = fgetc();
		// skip some chars
		if (c==0 || c=='\t') 
			{
			c=' ';
			//Log(LOGALERT, "recovered from bad chars in %s", name());
			}
		else if (c=='\r')
			{
			if ((c=fgetc())=='\n')
				{
				buff[ibuff] = 0;
				return 1; // EOL
				}
			c=' ';
			Log(LOGALERT, "recovered from single '\\r' in %s", name());
			}
		else if (c=='\n')
			{
			buff[ibuff] = 0;
			return 1; // EOL
			}
		else if (c==(unsigned char)EOF)
			{
			buff[ibuff] = 0;
			return 0; // EOF
			}

		// add char
		buff[ibuff++] = c;
		// add buffer
		if (ibuff==maxbuff)
			{
			//buff[ibuff] = 0;
			return -1; // CONT
			}
		}
	return -1; // impossible
}

char *CFILE::fgetstr(void)
{
	PROFILE("fgetstr()");

	int ret;
	bufferlen = 0;
	do
	{		
		bufferlen += FILEBUFFLEN;
		if (bufferlen>=bufferlenmax)
			buffer = (char *)realloc(buffer, bufferlenmax += FILEBUFFLEN);
	} while ((ret=fgetstrbuff(buffer+bufferlen-FILEBUFFLEN, FILEBUFFLEN))<0);

	return ret ? buffer : NULL;  // 0 = EOF  1 = GOTLINE
}




CString CFILE::fileread(const char *path, const char *nl)
{
		CFILE file;
		if (!file.fopen(path))
			{
			Log(LOGERR, "ERROR: Could not read file %s", path);
			return "";
			}

		CString memory;
		const char *line;
		while (line=file.fgetstr())
			{
			memory += line;
			memory += nl;
			}
		file.fclose();
		return memory;
}


static int filewrite(const char *path, const char *str, int size)
{

	if (size==0)
		size = strlen(str);

		CFILE file;
		if (!file.fopen(path,  CFILE::MWRITE))
			{
			Log(LOGERR, "ERROR: Could not write file %s", path);
			return FALSE;
			}

		int ret = file.fwrite(str, size, 1);
		file.fclose();

		return ret;
}







CString colstr(int n)
{
	char str[10];
	char *s = str;
	while (n>0)
		{
		*s++ = 'A'+ ((n+1) % 26);
	    n = n/26;
		}
	*s++ = 0;
	char istr[10];
	s = istr;
	n = strlen(str);
	for (int i=n-1; i>=0; --i)
		*s++ = str[i];
	*s++ = 0;
	return CString(istr);
}


// Profiling functions

#define PRMAX 100
__int64 PRstart, PRend;
static int PRNUM;
typedef struct { 
		int id;
		int count;
		double ticks;
		__int64 start, end;
		int n;
		char name[80];
} tprof;

static tprof PROFILEDATA[PRMAX];


#if 1
#define ProfileTicks(ticks) QueryPerformanceCounter((LARGE_INTEGER*)&ticks);
#else
#define ProfileTicks(ticks) ticks=GetTickCount();
#endif


CProfileID::CProfileID(const char *name)
{
	if (PRNUM>=PRMAX)
		{
		Log(LOGALERT, "Not enough PRMAX");
		return;
		}
	id = PRNUM++;
	tprof &P = PROFILEDATA[id];
	P.id = id;
	strcpy(P.name, name);
	P.ticks = 0;
	P.count = 0;
}

BOOL NOPROFILE;
volatile BOOL stopprofile;

CProfile::CProfile(int i)
	{
		ProfileTicks(PROFILEDATA[id = i].start);
		ASSERT( PROFILEDATA[id].id == id );
	}

CProfile::~CProfile(void)
	{
		if (NOPROFILE || stopprofile)
			return;
		tprof &P = PROFILEDATA[id];
		ProfileTicks(P.end);
		P.ticks += (P.end-P.start);
		++P.count;
	}



int prcmp( const void *arg1, const void *arg2 )
{
	if (((tprof*)arg2)->ticks > ((tprof*)arg1)->ticks)
		return 1;
	if (((tprof*)arg2)->ticks < ((tprof*)arg1)->ticks)
		return -1;
	return 0;
}

void PROFILEReset(void)
{
	stopprofile = TRUE;
	Sleep(1000); // wait for everyone to stop
	for (int i=0; i<PRNUM; ++i)
		{
		PROFILEDATA[i].ticks = 0;
		PROFILEDATA[i].count = 0;	
		}
	ProfileTicks(PRstart);
	stopprofile = FALSE;
	Sleep(1000);
}

void PROFILESummary(const char *name)
{
	if (NOPROFILE)
		{
		Log(LOGINFO, "PROFILER: Disabled");
		return;
		}

	stopprofile = TRUE;
	Sleep(1000); // wait for everyone to stop
	//Log(LOGINFO, "%%%%PROF: STARTED");
	ProfileTicks(PRend);

	tprof *TP = (tprof *)malloc(PRNUM*sizeof(tprof));
	memcpy(TP, PROFILEDATA, PRNUM*sizeof(tprof));
	qsort(TP, PRNUM, sizeof(tprof), prcmp);

	CString str;
	double TICKS = (double)(PRend-PRstart);
	Log(LOGINFO, "PROF: %s %.0ft %s", name, TICKS, str);
	for (int i=0; i<PRNUM; ++i)
		{
		tprof &P = TP[i];
		CString name = P.name;
		if (P.n>0) 
			name = MkString("%s#%d", P.name, P.n);
		Log(LOGINFO, "PROF: %15dx %15.0ft %d%% : %s", 
			P.count, (double)P.ticks, (int)(P.ticks/TICKS*100), name);
		}
	free(TP);
	stopprofile = FALSE;
	Sleep(1000);
	//Log(LOGINFO, "%%%%PROF: FINISHED");
}







////////////////////////////////////////////
// ArrayLists
////////////////////////////////////////////
//const TYPE& operator[](INT_PTR nIndex) const;
//TYPE& operator[](INT_PTR nIndex);




// Strings
CStringArrayList::~CStringArrayList(void)
{
	for (int i=0; i<maxsize; ++i)
		delete data[i];
	if (data) free(data);
}

CStringArrayList::CStringArrayList(int presetsize)
{
	data = NULL;
	size = maxsize = 0;
	SetSize(presetsize);
}

CStringArrayList::CStringArrayList(CStringArrayList &other)
{
	data = NULL;
	size = maxsize = 0;
	AddTail(other);
}


void CStringArrayList::SetMaxSize(int newsize)
{
	if (newsize>maxsize)
		data = (CString **)realloc(data, newsize *sizeof(*data));
	for (maxsize; maxsize<newsize; ++maxsize)
		data[maxsize] = new CString;
	sorted = FALSE;
}

void CStringArrayList::Reset(int presetsize)
{
	for (int i=0; i<size; ++i)
		data[i]->Empty();
	SetMaxSize(presetsize);
	sorted = FALSE;
	size = presetsize;
}

void CStringArrayList::Remove(int n)
{
	CString *del = data[n];
	int i;
	for (i=n; i<size-1; ++i)
		data[i] = data[i+1];
	data[i] = del;
	del->Empty();
	--size;
}

int CStringArrayList::AddTail(const char *str) //CString &str)
{
	if (size>=maxsize)
		SetMaxSize(max(10, maxsize + maxsize));
	sorted = FALSE;
	*data[size] = str;
	return size++;
}

void CStringArrayList::AddTail(CStringArrayList &list)
{
	int size = list.GetSize();
	for (int i=0; i<size; ++i)
		AddTail(list[i]);
}

int CStringArrayList::cmpcomma( const void *arg1, const void *arg2)
{
	register char c1, c2;
	register const char *str1 = **((const char ***)arg1);
	register const char *str2 = **((const char ***)arg2);
	do
	{
		c1 = *str1++; 
		if (c1==',') c1=0;
		c2 = *str2++; 
		if (c2==',') c2=0;
	}
	while (c1==c2 && c1!=0);
	return c1 - c2;
}

int CStringArrayList::cmpstr( const void *arg1, const void *arg2)
{
	return strcmp(**((const char ***)arg1), **((const char ***)arg2));
}

int CStringArrayList::Find(const char *str, qfunc *cmpfnc)
{
	const char **id = &str;
	if (sorted) 
		return qfind(&id, &data[0], size, sizeof(*data), cmpfnc);
	for (int i=0; i<size; ++i)
		if (cmpfnc(&id, &data[i])==0)
			return i;
	return -1;
}

void CStringArrayList::Sort(qfunc *cmpfnc)
{
	qsort(data, size, sizeof(*data), cmpstr);
	sorted = TRUE;
}














#undef malloc
#undef free
#undef realloc

#if 1
#define MEMMARK 0xCAC1  // 51905
#define checkptr(ptr, size) int *sizeptr = (int *)(ptr-4); WORD *check1 = (WORD*)(ptr-6), *check2 = (WORD*)(ptr+size); 

#ifdef DEBUGMEMORY

typedef struct { void *ptr; unsigned char del; } voidptr;

class ptrlist
{
int ptrmax;

public:
int delcnt, size, sortsize, maxsize;
voidptr *data;

static int disabled;
static void onexit(void)
{
	disabled = TRUE;
}

ptrlist()
{
	delcnt, size = sortsize = maxsize = 0;
	data = NULL;
	atexit( onexit );
}

~ptrlist()
{
	if (data) free(data);
}

void set(void *p)
{
	if (p==NULL || disabled)
		return;

	int i = get(p);
	if (i<0)
		{
		// reuse ptr
		data[-i].ptr = p;
		data[-i].del = 0;
		return;
		}
	if (i>0)
		{
		// duplicated???
		ASSERT(!"WTF???");
		}

	// add
	if (size>=maxsize)
		{
		maxsize += 10000;
		data = (voidptr *)realloc(data, maxsize*sizeof(data[0]));
		if (size==0) data[size++].ptr = NULL; // never 0!
		}
	data[size].ptr = p;
	data[size].del = 0;
	++size;
}


static int cmpvoiddata( const void *arg1, const void *arg2)
{	
	if (((voidptr *)arg1)->ptr>((voidptr *)arg2)->ptr) return 1;
	if (((voidptr *)arg1)->ptr<((voidptr *)arg2)->ptr) return -1;
	return 0;
}

void *reset(void *ptr)
{
	if (!ptr || disabled) 
		return ptr;

	int i = get(ptr);
	if (i==0)
		{
		ASSERT(!"UNKNOWN POINTER???");		
		// i==-2 deleted 
		// i==-1 unknown
		return ptr;
		}
	if (i<0)
		{
		ASSERT(!"DELETED TWICE???");		
		return ptr;
		}
	if (data[i].ptr!=ptr)
		{
		ASSERT(!"WTF???");		
		return ptr;
		}
	// ok
	data[i].del = TRUE;
	if (++delcnt % 10000 == 0)
		{
		printf("MemChk %dptr\r", maxsize);
		memorycheck();
		}
	return ptr;
}

/*
int get(void *p)
{
	int i = getA(p);
	int j = getB(p);
	ASSERT(i==j);
	return i;
}
*/


#if 0
int get(void *p)
{
	if (size<=1)
		return 0;
	if (sortsize!=size)
		{
		sortsize = size;
		qsort(data+1, sortsize-1, sizeof(data[0]), cmpvoiddata);
		}
	voidptr vp = { p, 0 };
	int i = qfind(&vp, data+1, sortsize-1, sizeof(data[0]), cmpvoiddata);
	if (i<0) return 0;
	// valid i
	++i;
	if (!data[i].del)
		return i;
	return -i;
}
#else
int get(void *p)
{
	register int wasdeleted = -1;
	register int size = this->size;
	register voidptr *data = this->data;
	register void *ptr = p;
	for (register int i=size-1; i>=0; --i)
		if (data[i].ptr==ptr)
			{
			if (data[i].del)
				return -i;
			return i;
			}
	return 0; // unknown
}
#endif

} ptrlist;

int ptrlist::disabled = 0;

#define setptr(ptr) ptrlist.set(ptr)
#define resetptr(ptr) ptrlist.reset(ptr)
#define getptr(ptr) ptrlist.get(ptr)

void *vchkalloc(void *vptr, const char *file, long line);

void memorycheck(void)
	{
		for (int i=0; i<ptrlist.size; ++i)
			if (!ptrlist.data[i].del)
				{
				char *ptr = ((char *)ptrlist.data[i].ptr)+6;
				vchkalloc(ptr, "GLOBAL", 0);
				}
	}

#else
#define setptr(ptr) /**/
#define resetptr(ptr) ptr
#define getptr(ptr) -2
#define getdelptr(ptr) -2

void memorycheck(void)
{
}

#endif


static void *setalloc(void *vptr, int size)
{
	setptr(vptr);
	char *ptr = (char*)vptr+6;
	checkptr(ptr, size);
	*check1 = *check2 = MEMMARK;
	*sizeptr = size;
	return ptr;
}

static void *vchkalloc(void *vptr, const char *file, long line)
{
	char *ptr = ((char *)vptr)-6;
	checkptr(ptr+6, *sizeptr);
	// 0Mb to 500Mb max
	if (*sizeptr<=0 || *sizeptr>=500*1024*1024 || *check1!=MEMMARK || *check2!=MEMMARK)
		{
		ASSERT(!"Memory Corruption!");
		Log(LOGALERT, "MEMORY CORRUPTION: free() at %s #%d [ptr:%d (-1:unk -2:del)]", file, getptr(ptr));
		Log(LOGALERT, "MEMORY CORRUPTION: sizeptr=%d check1=%d check2=%d", *sizeptr, *check1, *check2);
		}
	int msize = _msize(ptr);
	if (msize!=*sizeptr+8)
		{
		ASSERT(!"Memory Corruption!");
		Log(LOGALERT, "HEAPS MEMORY CORRUPTION: free() at %s #%d", file, line);
		}
	return ptr;
}

void *malloclog(int size, const char *file, long line)
{
	void *vptr = ::malloc(size+8);
	if (!vptr) 
		Log(LOGFATAL, "MEMORY ERROR: malloc(%d) at %s #%d", size, file, line);
	return setalloc(vptr, size);
}


void *realloclog(void *vptr, int size, const char *file, long line)
{
	if (vptr!=NULL)
		vptr = vchkalloc(vptr, file, line);
	vptr = ::realloc(resetptr(vptr), size+8);
	if (!vptr) 
		Log(LOGFATAL, "MEMORY ERROR: realloc(%d) at %s #%d", size, file, line);
	return setalloc(vptr, size);
}

void *calloclog(int num, int size, const char *file, long line)
{
	return malloclog(num*size, file, line);
}

void freelog(void *vptr, const char *file, long line)
{
	if (vptr!=NULL)
		::free(resetptr(vchkalloc(vptr, file, line)));
}

void chkalloclog(void *vptr, const char *file, long line)
{
	if (vptr!=NULL)
		vchkalloc(vptr, file, line);
}



#else

void *malloclog(int size, const char *file, long line)
{
	return malloc(size);
}

void *calloclog(int num, int size, const char *file, long line)
{
	return calloc(num, size);
}

void freelog(void *vptr, const char *file, long line)
{
	free(vptr);
}

void *realloclog(void *vptr, int size, const char *file, long line)
{
	return realloc(vptr, size);
}

#endif



HANDLE DownloadH(char *buffer)
{
	HANDLE h = LocalHandle(buffer);
	if (h) return h;

	Log(LOGERR, "invalid pointer!!!");
	return NULL;
}

char *DownloadMalloc(int size)
{
	size += 10; // buffer
	HANDLE h = LocalAlloc(LMEM_MOVEABLE, size);
	if (h) return (char*)LocalLock(h);

	Log(LOGERR, "out of memory error!!!");
	return NULL;
}

int DownloadSize(char *buffer)
{
	return (int)LocalSize(DownloadH(buffer));
}

char *DownloadRealloc(char *buffer, int size)
{
	size += 10; // buffer

	if (!buffer) 
		return DownloadMalloc(size);

	HANDLE h = DownloadH(buffer);
	DWORD csize = LocalSize(h);
	if (csize >= (DWORD)size)
		return buffer;

	LocalUnlock(h);
	h = LocalReAlloc(h, max(size,(int)csize*2) , LMEM_MOVEABLE);
	if (h) return (char*)LocalLock(h);
	Log(LOGFATAL, "out of memory error!!!");
	return NULL;
}




void DownloadFree(char *buffer)
{
	HANDLE h = LocalHandle(buffer);
	LocalUnlock(h);
	LocalFree(h);
}













vars vara::join(const char *sep)
{
	vars v;
	int num = length();
	for (int i=0; i<num; ++i)
	    {
		v += ElementAt(i);
		if (i<num-1)
			v += sep;
	    }

	return v;
}


int vara::cmpstr( const void *arg1, const void *arg2)
{
	return strcmp(*((const vars *)arg1), *((const vars *)arg2));
}

void vara::sort(int sortcmpstr( const void *arg1, const void *arg2))
{
	qsort(this->GetData(), this->GetSize(), sizeof(*this->GetData()),  sortcmpstr);
}

void vara::splice(int i, int n)
{
	for (int c=0; c<n; ++c)
		RemoveAt(i);
}

vara::vara(const char *str[])
{
	for (int i=0; str[i]!=NULL; ++i)
		Add(vars(str[i]));
}

vara vara::flip(void)
{
	vara ret;
	for (int i=length()-1; i>=0; --i)
		ret.push(ElementAt(i));
	return ret;
}

vara::vara(const char *ptr, const char *sep)
{
	if (!ptr)
		return;
	int seplen = strlen(sep), len = strlen(ptr);
	int i;
	int lasti;
    for (i=0, lasti=0; i<len; ++i)
	  if (strnicmp(ptr+i, sep, seplen)==0)
	     {
         Add(vars(ptr+lasti, i-lasti));
		 lasti = i+= seplen;
		 --i;
	     }
    if (lasti<i)
      Add(vars(ptr+lasti, i-lasti));
}

vara vars::split(const char *sep)
{ 
	return vara(this->GetBuffer(), sep); 
}


int vars::indexOf(const char *s, int start)
{
	int slen = strlen(s);
	int end = length();
	const char *ptr = GetBuffer();
	for (int i=start; i<end; ++i)
		if (strncmp(ptr+i, s, slen)==0)
			return i;
	return -1;
}


int vars::lastIndexOf(const char *s, int end)
{
	int slen = strlen(s);
	if (end<0) end = length();
	const char *ptr = GetBuffer();
	for (int i=end-slen; i>=0; --i)
		if (strncmp(ptr+i, s, slen)==0)
			return i;
	return -1;
}

vars vars::trim()
{
	const char *str = GetBuffer();
	while (isspace(*str) && *str!=0)
		++str;
	var len;
	for (len=strlen(str)-1; len>=0 && isspace(str[len]); --len);
	return vars(str, len+1);
}


int vara::indexOf(const char *s)
{
	for (int i=0; i<length(); ++i)
		if (strcmp(ElementAt(i), s)==0)
			return i;
	return -1;
}

int vara::indexOfi(const char *s)
{
	for (int i=0; i<length(); ++i)
		if (stricmp(ElementAt(i), s)==0)
			return i;
	return -1;
}

int vara::indexOfSimilar(const char *str)
{
	for (int i=0; i<length(); ++i)
		if (IsSimilar(ElementAt(i), str))
			return i;
	return -1;
}

vars vars::substr(int start, int len) 
{ 
	if (len<0)
		return vars(Mid(start));
	else
		return vars(Mid(start, len));
}


varas::varas(const char *str, const char *sep, char *sep2)
{
	vara lines(str, sep);
	int len = vara(lines[0], sep2).length();
	SetSize(len);
	for (int i=0; i<lines.length(); ++i)
		{
		vara line(lines[i], sep2);
		if (line.length()!=len) 
			{
			Log(LOGERR, "Inconsistent varas %d != %d for line %s", line.length(), len, lines[i]);
			continue;
			}
		for (int j=0; j<len; j++)
			ElementAt(j).Add(line[j]);
		}
}


const char *varas::match(const char *str)
{
	if (!str || *str==0)
		return NULL;

	int len = ElementAt(0).GetSize();
	for (int i=0; i<len; ++i)
		if (IsSimilar(str, ElementAt(0)[i]))
			return ElementAt(1)[i];
	return NULL;
}


int varas::matchnum(const char *str)
{
	const char *m = match(str);
	if (!m) return -1;
	return atoi(m);
}

const char *varas::strstr(const char *str)
{
	if (!str || *str==0)
		return NULL;

	int len = ElementAt(0).GetSize();
	for (int i=0; i<len; ++i)
		if (::strstr(str, ElementAt(0)[i]))
			return ElementAt(1)[i];
	return NULL;
}

#ifdef USESIMPLECSECTION

CSection::CSection(void)
{
	locked = 0;
}

CSection::~CSection(void)
{
}


int inline getLock(void *locked)
{
	register int ret = FALSE;
	ThreadLock();
	if (locked==0)
		++locked, ret = TRUE;
	ThreadUnlock();
	return ret;
}

void CSection::Lock(void) 
{
	// simulate csection
	while (!getLock(locked))
		Sleep(100);
} 

void CSection::Unlock(void) 
{
	ThreadLock();
	--locked;
	ThreadUnlock();
}


#else

void CSection::Lock(void) 
{
	((CCriticalSection *)locked)->Lock();
}

void CSection::Unlock(void) 
{
	((CCriticalSection *)locked)->Unlock();
}

CSection::CSection(void)
{
	locked = (CCriticalSection *)new CCriticalSection;
}

CSection::~CSection(void)
{
	delete (CCriticalSection *)locked;
}

#endif





vars ExtractString(const char *mbuffer, const char *searchstr, const char *startchar, const char *endchar)
{
	CString symbol;
	if (mbuffer) GetSearchString(mbuffer, searchstr, symbol, startchar, endchar);
	return vars(symbol);
}

vars ExtractLink(const char *memory, const char *str)
{
	const char *href = "href=";
	int len = strlen(str), hlen = strlen(href);
	for (int i=0; memory[i]!=0; ++i)
		if (strnicmp(memory+i, str, len)==0) {
			// found it!
			int s = i;
			while (s>=0 && strnicmp(memory+s, href, hlen)!=0)
				--s;
			if (s<0)
				{
				i+=hlen;
				continue;
				}

			s += hlen;
			int e = s;
			while (!isspace(memory[e]) && memory[e]!='>' && memory[e]!=0)
				++e;
			//if (s>hlen && strnicmp(memory+s-hlen, href, hlen)==0) {
			//	++s;
			return vars(memory+s, e-s).Trim(" \r\n\t\"'");
		}
	return "";
}


