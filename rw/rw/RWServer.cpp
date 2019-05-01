#include "stdafx.h"
#include "io.h"
#include "fcntl.h"
//#include "TradeMaster.h"

//#include <stdlib.h>
#include <process.h>
#include "rw.h"
#include "fcgiapp.h"
#include "afxmt.h"
#include "xzip.h"
//#include "fcgi_stdio.h"
//#include "fcgi/include/fcgio.h"
//#include "fcgi/include/fcgi_config.h"  // HAVE_IOSTREAM_WITHASSIGN_STREAMBUF

#define abortfile "abort.rwr"
#define legendfile "POI\\legend.png"

// Maximum number of bytes allowed to be read from stdin
static const unsigned long STDIN_MAX = 1000000;

#define CRASHPROTECTION

#define DEBUGRWZ(msg) /*Log(LOGINFO, msg": %s", query)*/

#ifdef DEBUG
#define THREADCOUNT 50	// number of threads
#define FLUSHTIME 1	// flush one file every x minutes
#else
#define THREADCOUNT 20 // number of threads
#define FLUSHTIME 1	// flush one file every x minutes
#endif

//#define SCANTHREAD // automatically scan in the background
//#define ONEZIP	// only one thread zipping
#define TIMEOUT 50		// <1m to return
#define TIMEOUTTEXT "ERROR,QUERY TIMED OUT! RESULTS ARE PARTIAL! TRY AGAIN LATER WHEN SERVER LESS BUSY!"
#define LINEOUT 3000	//
#define LINEOUTTEXT "ERROR,TOO MANY RESULTS! ONLY SHOWING FIRST 3000 LINES!"

//#define SIMPLECGI

CString GetRWText(void)
{
		CString rwtext = "Downloaded from http://ropewiki.com ";
		rwtext += COleDateTime::GetCurrentTime().Format(_T("%A, %B %d, %Y"));
		rwtext += CString(' ', 129-rwtext.GetLength());
		return rwtext;
}


/*%%%


static long gstdin(FCGX_Request * request, char ** content)
{
    char * clenstr = FCGX_GetParam("CONTENT_LENGTH", request->envp);
    unsigned long clen = STDIN_MAX;

    if (clenstr)
    {
        clen = strtol(clenstr, &clenstr, 10);
        if (*clenstr)
        {
            cerr << "can't parse \"CONTENT_LENGTH="
                 << FCGX_GetParam("CONTENT_LENGTH", request->envp)
                 << "\"\n";
            clen = STDIN_MAX;
        }

        // *always* put a cap on the amount of data that will be read
        if (clen > STDIN_MAX) clen = STDIN_MAX;

        *content = new char[clen];

        //%%%cin.read(*content, clen);
        //%%%clen = cin.gcount();
    }
    else
    {
        // *never* read stdin when CONTENT_LENGTH is missing or unparsable
        *content = 0;
        clen = 0;
    }

    // Chew up any remaining stdin - this shouldn't be necessary
    // but is because mod_fastcgi doesn't handle it correctly.

    // ignore() doesn't set the eof bit in some versions of glibc++
    // so use gcount() instead of eof()...
    do cin.ignore(1024); while (cin.gcount() == 1024);

    return clen;
}
*/


#ifndef SIMPLECGI

int WriteFile(const char *ofile, inetdata *data) //, int skip = 0)
{
	CFILE f;
	if (!f.fopen(ofile))
		{
		// check file exist
		Log(LOGERR, "ERROR: can't read file %.128s", ofile);
		return FALSE;
		}

	int r, size = 0;
	char buff[1024*8];
	while ((r = f.fread(buff, 1, sizeof(buff)))>0)
		{
		/*
		if (size<skip)
			{
			skip -= r;
			if (skip<0)
				{
				data->write(buff+r+skip, -skip);
				skip = 0;
				}			
			continue;
			}
		*/
		if (data)
			data->write(buff, r);
		size += r;
		}

	f.fclose();
	return size;
}


int WriteFile(HANDLE hFile, inetdata *data)
{
	char buff[1024*8];
	DWORD read;
	while (ReadFile(hFile, buff, sizeof(buff), &read, NULL)>0 && read>0)
		data->write(buff, read);

	return TRUE;
}



class inetproxy : public inetdata {
	FCGX_Stream *FCGX_stdout;

	int size;

	public:
	inetproxy(FCGX_Stream *FCGX_stdout)
	{
		size  = 0;
		this->FCGX_stdout = FCGX_stdout; 
	}

	void write(const void *buffer, int size)
	{
		FCGX_PutStr((const char *)buffer, size, FCGX_stdout);
		int newsize = this->size + size;
		//if (this->size<32 && newsize>32)
		//	FCGX_FFlush(FCGX_stdout);
		this->size = newsize;
		//Log(LOGINFO, "Write %d", size));
	}
};





class inetgpx : public inetdata {
	FCGX_Stream *FCGX_stdout;
	inetfile *f;
	CString url, infile, outfile;
	int size;

	public:
	inetgpx(FCGX_Stream *FCGX_stdout, CString url)
	{

		this->url = url;
		this->FCGX_stdout = FCGX_stdout; 
		f = NULL;
	}


	void write(const void *buffer, int size)
	{
		if (!f)
			{
			static int count = 0;
			CString id = CTime::GetCurrentTime().Format("%Y-%m-%d_%H-%M-%S");
			CString file = MkString("KMLGPX_%s_%d", id, count++);
			infile = file+KML;
			outfile = file+GPX;
			f = new inetfile(infile);
			}

		CString str;
		const char *blanks = "\n\t\r ";
		const char *cbuffer = (const char *)buffer;
		for (int i=0; i<size; ++i)
			{
			if (cbuffer[i]>127 || cbuffer[i]<0)
				continue;
			if (cbuffer[i]<' ' && !strchr(blanks, cbuffer[i]))
				{
				str += ' ';
				continue;
				}
			str += cbuffer[i];
			}
		f->write((void*)((const char *)str), str.GetLength());
	}

	~inetgpx()
	{
		if (!f)
			return;

		delete f;
		f = NULL;

		// convert to gpx
		int ret = system(MkString("GPSBabel.exe -i kml -f %s -o gpx -F %s 2>> RW.LOG", infile, outfile));
		if (CFILE::exist(outfile)) {
			Log(LOGINFO, "converted gpx ret:%d url:%.128s", ret, url);

			inetproxy data(FCGX_stdout);
			WriteFile(outfile, &data);
		}
		else {
			Log(LOGERR, "ERROR: NOT converted gpx ret:%d url:%.128s", ret, url);
		}

		DeleteFile(infile);
		DeleteFile(outfile);
	}

};


int PhantomJS(const char *file, volatile int &nfiles)
{
	PROCESS_INFORMATION ProcessInfo; //This is what we get as an [out] parameter
	STARTUPINFO StartupInfo; //This is an [in] parameter

	ZeroMemory(&StartupInfo, sizeof(StartupInfo));
	StartupInfo.cb = sizeof StartupInfo ; //Only compulsory field

	vars cmd = "PhantomJS.exe --disk-cache=true --max-disk-cache-size=100000 "; cmd += file;
	if(CreateProcess(NULL,(char *)((const char*)cmd),NULL,NULL,FALSE,CREATE_NEW_CONSOLE,NULL,NULL,&StartupInfo,&ProcessInfo))
	{ 

	int oldfiles = nfiles;
	while (WaitForSingleObject(ProcessInfo.hProcess,1*60*1000) == WAIT_TIMEOUT)
		{
		if (oldfiles == nfiles)
			{
			TerminateProcess(ProcessInfo.hProcess, 1);
			Log(LOGERR, "PhantomJS process terminated for %s", file);
			break;
			}
		oldfiles = nfiles;
		}

	CloseHandle(ProcessInfo.hThread);
	CloseHandle(ProcessInfo.hProcess);
	return TRUE;
	} 
	else
	{
	Log(LOGERR, "PhantomJS could not be started");
	return FALSE;
	}
}


class inetrwpdf : public inetdata {
	FCGX_Stream *FCGX_stdout;
	inetfile *f;
	CString opts, url, infile, outfile, logfile;
	int size;

	public:
	inetrwpdf(FCGX_Stream *FCGX_stdout, CString url, CString opts)
	{

		this->url = url;
		this->opts = opts;
		this->FCGX_stdout = FCGX_stdout; 
		f = NULL;
	}


	void write(const void *buffer, int size)
	{
		if (!f)
			{
			static int count = 0;
			CString id = CTime::GetCurrentTime().Format("%Y-%m-%d_%H-%M-%S");
			CString file = MkString("RWPDF_%s_%d", id, count++);
			infile = file+".rw";
			outfile = file+".pdf";
			logfile = file+".rw.log";
			f = new inetfile(infile);

			// 1 single url
			vara opt;
			opt.push("var gtrans = '"+ExtractString(opts, "gtrans=", "", "&")+"';");
			opt.push("var metric = "+vars(strstr(opts, "metric=on") ? "true" : "false")+";");
			opt.push("var french = "+vars(strstr(opts, "french=on") ? "true" : "false")+";");
			opt.push("var docwidth = '"+ExtractString(opts, "docwidth=", "", "&")+"';");
			opt.push("var singlepage = "+vars(strstr(opts, "singlepage=off") ? "false" : "true")+";");
			opt.push("var tlinks = false;");
			opt.push("var lnkbs = false;");
			opt.push("var gpxbs = false;"); 
			opt.push("var notrans = '';");
			opt.push("var linedata='**"+outfile+" "+url+"\\n*\\n';");

			vars str = opt.join("\n");
			f->write((void *)((const char *)str), strlen(str));
			}

		f->write(buffer, size);
	}

	~inetrwpdf()
	{
		if (!f)
			return;

		delete f;
		f = NULL;

		inetproxy data(FCGX_stdout);

		/*
		int stubsize = WriteFile("stub.pdf", &data);
		FCGX_FFlush(FCGX_stdout);
		*/

		// convert to pdf
		int files = 1;
		PhantomJS(infile, files);
		//int ret = system(MkString("phantomjs.exe --disk-cache=false %s > NUL", infile));
		if (CFILE::exist(outfile)) {
			Log(LOGINFO, "pdf converted .rw url:%.128s", url);
			WriteFile(outfile, &data);
			DeleteFile(infile);
			DeleteFile(outfile);
			DeleteFile(logfile);
		}
		else {
			Log(LOGERR, "ERROR: pdf NOT converted .rw file:%s url:%.128s", infile, url);
		}

	}

};


class inetrwzip : public inetdata {
	FCGX_Stream *FCGX_stdout;
	inetfile *f;
	CString url, opts;
	CString infile, outfile, logfile;
	int size;

	int phantomjs;
	int nfiles;
	vara listfiles;
	HZIP hz;
	HANDLE hwrite, hread; 
	CWinThread *sending,  *checking;

	public:
	inetrwzip(FCGX_Stream *FCGX_stdout, CString id, CString opts)
	{

		this->url = id;
		this->opts = opts;
		this->FCGX_stdout = FCGX_stdout; 
		f = NULL;
	}


	void write(const void *buffer, int size)
	{
		CString data((const char *)buffer, size);
		if (!f)
			{
			static int count = 0;
			CString id = CTime::GetCurrentTime().Format("%Y-%m-%d_%H-%M-%S");
			CString file = MkString("RWZIP_%s_%d", id, count++);
			outfile = file;
			infile = file+".rw";
			logfile = file+".rw.log";
			CreateDirectory(outfile, NULL);
			f = new inetfile(infile);

			/*
			// 1 single url
			vara opt;
			opt.push("var gtrans = '"+ExtractString(opts, "gtrans=", "", "&")+"';");
			opt.push("var docwidth = '"+ExtractString(opts, "docwidth=", "", "&")+"';");
			opt.push("var metric = "+vars(strstr(opts, "metric=on") ? "true" : "false")+";");
			opt.push("var french = "+vars(strstr(opts, "french=on") ? "true" : "false")+";");
			opt.push("var singlepage = "+vars(strstr(opts, "singlepage=off") ? "false" : "true")+";");
			opt.push("var tlinks = false;");
			opt.push("var lnkbs = false;");
			opt.push("var gpxbs = false;"); 
			opt.push("var notrans = '';");
			opt.push("var linedata='**"+outfile+" "+url+"\\n*\\n';");

			vars str = opt.join("\n");
			f->write((void *)((const char *)str), strlen(str));
			*/
			
			vara lines(data, "\n");
			for (int i=0; i<lines.length(); ++i)
				{
				if (lines[i][0]=='*' && lines[i][1]!='*' && lines[i][1]!='\"')
					lines[i] = "*" + outfile + " " + GetToken(lines[i], 1, ' ');
				if (IsSimilar(lines[i],"var debug"))
					break;
				}
			data = lines.join("\n");
			}

		f->write(data, data.GetLength());
	}


	static UINT senddata(LPVOID pParam)
	{
		inetrwzip *rwzip = (inetrwzip *)pParam;
		inetproxy data(rwzip->FCGX_stdout);
		WriteFile(rwzip->hread, &data);
		CloseHandle(rwzip->hread);
		rwzip->hread = NULL;
		return 0;
	}

	static UINT checkfolder(LPVOID pParam)
	{
		inetrwzip *rwzip = (inetrwzip *)pParam;
		CString path = MkString("%s\\*", rwzip->outfile);
		int retry = 0;
		while (TRUE)
			{
			Sleep(1000);

			// check if file (or files) match expected header
			// if they don't, destroy them to prevent wrong header usage


			int nfiles = 0;
			CFileFind finder;
			BOOL bWorking = finder.FindFile(path);
			while (bWorking)
				{
					bWorking = finder.FindNextFile();
					if (finder.IsDots()) continue;
					if (finder.IsDirectory())
						continue;

					CString file = finder.GetFilePath();
					if (rwzip->listfiles.indexOf(file)>=0)
						continue;
					
					if (ZipAdd(rwzip->hz, finder.GetFileName(), (void *)((const char *)finder.GetFilePath()), 0, ZIP_FILENAME)==ZR_OK)
						{
						rwzip->listfiles.push(file);
						++rwzip->nfiles;
						++nfiles;
						}
				}

			if (!nfiles && !rwzip->phantomjs)
				if (retry++>2)
					break;
			}
		return 0;
	}

	~inetrwzip()
	{
		if (!f)
			return;

		delete f;
		f = NULL;

		if (!CreatePipe(&hread,&hwrite,NULL,0))
			{
			Log(LOGERR, "Pipe creation failed for rwzip %s", outfile);
			return;
			}
		
		hz = CreateZip(hwrite,0,ZIP_HANDLE);
		if (!hz)
			{
			Log(LOGERR, "Zip creation failed for rwzip %s", outfile);
			return;
			}

		//sending = checking = TRUE;
		nfiles = 0;
		listfiles.RemoveAll();
		phantomjs = TRUE;
		sending = AfxBeginThread(senddata, this);
		checking = AfxBeginThread(checkfolder, this);
		if (!sending || !checking)
			{
			Log(LOGERR, "Thread creation failed for rwzip %s", outfile);
			return;
			}
        //              0123456789012345678901234567890123
		//vars rwtext = GetRWText();
		//if (ZipAdd(hz, "ropewiki.txt", (void *)((const char *)rwtext), strlen(rwtext)+1, ZIP_MEMORY)!=ZR_OK)
		if (ZipAdd(hz, "ropewiki.png", "poi\\ropewiki.png", 0, ZIP_FILENAME)!=ZR_OK)
			Log(LOGERR, "Cannot embed ropewiki.txt");

		// convert to pdf
		PhantomJS(infile, nfiles);
		phantomjs = FALSE;

		//int ret = system(MkString("phantomjs.exe --disk-cache=false %s > NUL", infile));
        
		// shut down
        WaitForSingleObject(checking->m_hThread, INFINITE);
		CloseHandle(hwrite);
		CloseZip(hz);
        WaitForSingleObject(sending->m_hThread, INFINITE);

		if (nfiles>0) {
			Log(LOGINFO, "zip converted .rw files:%d url:%.128s", nfiles, url);
			DeleteFile(infile);
			DeleteFile(logfile);
			for (int i=0; i<listfiles.length(); ++i)
				DeleteFile(listfiles[i]);
			RemoveDirectory(outfile);
		}
		else {
			Log(LOGERR, "ERROR: zip NOT converted .rw url:%.128s", url);
		}
	}

};



/*
int KMLMerge(const char *url, inetdata *data)
{
	__try { 

		return _KMLMerge(url, data);

		}
  __except(EXCEPTION_EXECUTE_HANDLER) 
		{ 
		Log(LOGERR, "KMLMERGE CRASHED!!! %.200s", url); 	
		return FALSE; 
		}
}
*/





int querycnt = 0;



// generic passthrough
struct { char *name, *header; } ctable[] = { 
		{ "json=", "application/json"},
		{ "html=", "text/html"},
		{ "xml=", "text/xml"},
		{ "text=", "text/plain"},
		{ "png=", "image/png"},
		{ "bin=", "application/octet-stream"},
		NULL
	 };

static int count = 0;
static BOOL MDEBUG = FALSE;

static int running = TRUE;

typedef struct { 
	int ok;
	const char *query; 
	const char *name; 
	CStringArrayList filelist;
	HANDLE hwrite, hread; 
} tzipquery;

CWinThread *scanth = NULL; 
UINT threadscan(LPVOID pParam)
{
	//queryzip *zs = (queryzip *)pParam;
	++THREADNUM;
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_IDLE);
	while (running)
		{
		ScanCanyons();
		SuspendThread(GetCurrentThread());
		}
	--THREADNUM;
	return 0;
}

#ifdef ONEZIP

CWinThread *zipth; 
tzipquery zipquery[THREADCOUNT+1];
int zipqueue = 0, zipprocess = 0;

static UINT threadzip(LPVOID pParam)
{
	//queryzip *zs = (queryzip *)pParam;
	while (running)
		{
		SuspendThread(GetCurrentThread());
		if (zipqueue>zipprocess)
			{
			tzipquery q = zipquery[zipprocess % THREADCOUNT];
			++zipprocess;

			BOOL clink = strchr(GetTokenRest(q.query, 4), 'M'); 
			CKMLOut out(q.hwrite, "RWZ_"+ElevationFilename(q.query), clink); 
			ElevationQuery(q.query, out);
			}
		}
	return 0;
}

void QueryZip(tzipquery &q)
{
	zipquery[zipqueue % THREADCOUNT] = q;
	++zipqueue;
	if (zipqueue - zipprocess > THREADCOUNT)
		Log(LOGERR, "ERROR: zipqueue - zipprocess > THREADCOUNT!!!");
	zipth->ResumeThread();	
}

#else

static UINT threadzip(LPVOID pParam)
{
	tzipquery *q = (tzipquery *)pParam;
	const char *param = GetTokenRest(q->query, 4);
	int map = strchr(param, 'M')!=NULL;
	int link = map && strchr(param, '$')!=NULL;

	inethandle data(q->hwrite);
	CKMLOut out(&data, map ? q->name : NULL, link); 
	if (link)
		{
		out += MkString("<NetworkLinkControl id=\"%s\"><Update><targetHref>http://%s/rwr/?rwz=RWE.KC</targetHref>\n", q->name, server);
		out += MkString("<Delete><Document targetId=\"%s\"></Document></Delete>\n", q->name);
		out += MkString("<Delete><NetworkLink targetId=\"%s\"></NetworkLink></Delete>\n", q->name);
		out += MkString("<Create><Folder targetId=\"rwres\"><open>0</open><Document id=\"%s\">\n", q->name);
		}

	q->ok = ElevationQuery(q->query, out, q->filelist);

	if (link)
		{
		out += "</Document></Folder></Create></Update>";	
#ifdef DeleteAfterLoad
#if 0
		// output as self deleting network link
		//CString linkd = MkString("http://%s/rwr/?rwl=%s!", server, q->query);
		//out += MkString("<NetworkLink id=\"nlink\"><name>Cleanup</name><Link><href>%s</href></Link></NetworkLink>", linkd);
#else
		// delete
		if (name[0]=='@')
			{
			// delete NetworkLink that loaded the NetworkLinkControl
			CString link = MkString("http://%s/rwr/?rwl=%s", server, q->query);
			out += MkString("<Update><targetHref>%s</targetHref>\n", link);
			out += "<Delete><Document targetId=\"doc\"></Document></Delete>\n";
			out += "</Update>";
			}	
#endif
#endif
		out += "</NetworkLinkControl>";						
		}
	return 0;
}

inline void QueryZip(tzipquery &q)
{
	AfxBeginThread(threadzip,(void*)&q);	
}

#endif

static UINT threadwaterflow(LPVOID pParam)
{
		ThreadLock();
		++THREADNUM;
		ThreadUnlock();

		FCGX_Request *request = (FCGX_Request *)pParam;
		WaterflowQuery(request);
		
		FCGX_Finish_r(request);
		delete request;

		ThreadLock();
		--THREADNUM;
		ThreadUnlock();
		return 0;
}


void _process(FCGX_Request *request)
{
		FCGX_Stream *&FCGX_stdin = request->in; 
		FCGX_Stream *&FCGX_stderr = request->err; 
		FCGX_Stream *&FCGX_stdout = request->out;
		FCGX_ParamArray &envp = request->envp;

		/*
        // Although FastCGI supports writing before reading,
        // many http clients (browsers) don't support it (so
        // the connection deadlocks until a timeout expires!).
        char * postdata = NULL;
		char * clenstr = FCGX_GetParam("CONTENT_LENGTH", envp);
        unsigned long postlen = atoi(clenstr); 
		if (postlen>0) // POST
			{
			//Log( LOGINFO, "CONTENT_LENGTH=%d", nContentLength);
			postdata = (char *)malloc( postlen+1 );
			memset(postdata, 0, postlen+1);

			//%%%gstdin(&request, &content);
			FCGX_GetStr(postdata, postlen, FCGX_stdin);
			}
		*/

#if 1
		if (MDEBUG)
			{
			long pid = getpid();
			Log(LOGINFO, "PID: %u REQ#: %u", pid, ++count);
			//Log(LOGINFO, "REQUEST ENVIRONMENT:", ++count);
			//for (char **p = environ ; *p; ++p)
			//	Log(LOGINFO, "%s", *p);
			Log(LOGINFO, "PROCESS ENVIRONMENT:");
			for (char **p = envp ; *p; ++p)
				Log(LOGINFO, "%s", *p);
			/*
			Log(LOGINFO, "StanDARD INPUT: %db", postlen);
			Log(LOGINFO, "%.100s", postdata ? postdata: "");
			if (postdata)
				{
				CFILE f; // write complete post to file
				if (f.fopen("DEBUGPOST.txt", CFILE::MWRITE))
					f.fwrite(postdata, postlen, 1);
				}
			*/
			}
#endif

		//char * rwmethod = FCGX_GetParam("REQUEST_METHOD", envp);

		/*
		// Output response headers
		FCGX_PutS("Content-Type: application/octet-stream\r\n", FCGX_stdout);
		//FCGX_PutS("Cache-Control: no-cache\r\n", FCGX_stdout);
		//FCGX_PutS("Pragma: no-cache\r\n", FCGX_stdout);
		//P.OUTLINE("Content-Encoding: gzip\r\n");	
		FCGX_PutS("\r\n", FCGX_stdout);
		*/
		char * ipaddr = FCGX_GetParam("REMOTE_ADDR", envp);
		if (!ipaddr) ipaddr = "(null)";

		// ignore bots
		char * from = FCGX_GetParam("HTTP_USER_AGENT", envp); //FCGX_GetParam("HTTP_FROM", envp);
		if (from)
		  if (strstr(from, "Bot"))
			{
			printf("IGNORED BOT %.120s  \r", from, FCGX_GetParam("QUERY_STRING", envp));
			Log(LOGINFO, "IGNORED BOT %s url %.120s", from, FCGX_GetParam("QUERY_STRING", envp));
			return;
			}

		char * method = FCGX_GetParam("REQUEST_METHOD", envp);
		if (stricmp(method, "GET")==0 || stricmp(method, "POST")==0) // just headers
            {
			// Output query results
			CString q = FCGX_GetParam("QUERY_STRING", envp); 
			q.Replace("%25", "%");
			q.Replace("%3D", "=");

			const char *query = q;
			//P.PROCESSOUT(query, postdata, postlen);  
			const char *url = NULL;


			const char *ext;
			ext = strrchr(query,'.');
			if (!ext || strlen(ext)>4) 
				ext = KML;				

			const char *gpx = strstr(query, "gpx=on")!=NULL ? GPX : NULL;
			if (gpx) ext = GPX;

			CString filename = CString("download")+ext; 
			if (url=ucheck(query, "filename=")) {
				filename = ExtractString(url, "", "", "&")+ext;
			}

			inetproxy urldata(FCGX_stdout);
			inetgpx gpxdata(FCGX_stdout, url);
			inetdata *data = &urldata;
			if (gpx) data = &gpxdata; 
			//data->open(0);

			// ----------------------------------------------------------------------

			// proxy passthrough 
			//http://localhost/rw?gpx=on&filename=on&url=http://ropewiki.com/images/8/8d/La_Jolla_Sea_Caves.kml
			//http://lucac.no-ip.org/rwr?url=http://ropewiki.com/images/d/df/Stevenson_Creek_%28Lower%29.kml
			if (url=ucheck(query, "url="))
				{
				// Output response headers
				FCGX_PutS("Access-Control-Allow-Origin: *\r\n", FCGX_stdout);
				FCGX_PutS("Content-Type: application/octet-stream\r\n", FCGX_stdout);
				FCGX_PutS(MkString("Content-Disposition: attachment; filename=%s\r\n", filename), FCGX_stdout);	
				//FCGX_PutS("Cache-Control: no-cache\r\n", FCGX_stdout);
				//FCGX_PutS("Pragma: no-cache\r\n", FCGX_stdout);
				//P.OUTLINE("Content-Encoding: gzip\r\n");	
				FCGX_PutS("\r\n", FCGX_stdout);

				DownloadFile f(TRUE, data);
				if (f.Download(url))
					Log(LOGERR, "ERROR: can't download url %.128s", url);
				//else
				//	Log(LOGINFO, "%s bypassed %s %dKb %.128s", ipaddr, filename, f.size/1024, url);
				return;
				}

			if (url=ucheck(query, "iprogress="))
				{
				FCGX_PutS("Access-Control-Allow-Origin: *\r\n", FCGX_stdout);
				FCGX_PutS("Content-Type: text/html\r\n", FCGX_stdout);
				FCGX_PutS("\r\n", FCGX_stdout);

				CString html = CFILE::fileread("poi\\iprogress.htm");
				CString base = "/rwr?"; //vara(query, "iprogress=")[0];
				html.Replace("#URL#", base + url);
				FCGX_PutS(html, FCGX_stdout);
				return;
				}

			const char *domain = "; domain=ropewiki.com";
			if (url=ucheck(query, "pdfx="))
				{
				// Output response headers
				FCGX_PutS("Access-Control-Allow-Origin: *\r\n", FCGX_stdout);
				FCGX_PutS("Content-Type: application/pdf\r\n", FCGX_stdout);
				FCGX_PutS(MkString("Content-Disposition: attachment; filename=%s\r\n", GetFileName(filename)), FCGX_stdout);	
				FCGX_PutS(MkString("Set-Cookie: rwfilename=%s\r\n", filename+domain), FCGX_stdout);	
				//FCGX_PutS("Cache-Control: no-cache\r\n", FCGX_stdout);
				//FCGX_PutS("Pragma: no-cache\r\n", FCGX_stdout);
				//P.OUTLINE("Content-Encoding: gzip\r\n");	
				FCGX_PutS("\r\n", FCGX_stdout);

				//FCGX_FFlush(FCGX_stdout);
				vars id = GetToken(url, 0, '&');
				vars opt = GetTokenRest(url, 1, '&');
				inetrwpdf rwdata(FCGX_stdout, "http://ropewiki.com/"+id, opt);
				
				WriteFile("phantomjs.js", &rwdata);

				/*
				DownloadFile f(TRUE, &rwdata);

				vars rwurl = "http://ropewiki.com/index.php/PDFList?limit=100&action=raw&templates=expand&ctype=application/x-zope-edit&query=%5B%5B"+id+"%5D%5D"+opt;

				if (f.Download(rwurl))
					Log(LOGERR, "ERROR: can't download rwurl %.128s", rwurl);
				else
					Log(LOGINFO, "%s rwdownload %s %dKb %.128s", ipaddr, filename, f.size/1024,rwurl);
				*/

				return;
				}
			
			if (url=ucheck(query, "zipx="))
				{
				// Output response headers
				FCGX_PutS("Access-Control-Allow-Origin: *\r\n", FCGX_stdout);
				FCGX_PutS("Content-Type: application/zip\r\n", FCGX_stdout);
				FCGX_PutS(MkString("Content-Disposition: attachment; filename=%s\r\n", GetFileName(filename)), FCGX_stdout);	
				FCGX_PutS(MkString("Set-Cookie: rwfilename=%s\r\n", filename+domain), FCGX_stdout);	
				//FCGX_PutS("Cache-Control: no-cache\r\n", FCGX_stdout);
				//FCGX_PutS("Pragma: no-cache\r\n", FCGX_stdout);
				//P.OUTLINE("Content-Encoding: gzip\r\n");	
				FCGX_PutS("\r\n", FCGX_stdout);

				//FCGX_FFlush(FCGX_stdout);
				vars id = GetToken(url, 0, '&');
				vars opt = GetTokenRest(url, 1, '&');
				inetrwzip rwdata(FCGX_stdout, id, opt);

				DownloadFile f(TRUE, &rwdata);

				vars rwurl = "http://ropewiki.com/index.php/PDFList?action=raw&templates=expand&ctype=application/x-zope-edit&noscript=on&pagename="+id+"&"+opt;
				if (f.Download(rwurl))
					Log(LOGERR, "ERROR: can't download rwurl %.128s", rwurl);
				else
					{
					Log(LOGINFO, "%s rwdownload %s %dKb %.128s", ipaddr, filename, f.size/1024,rwurl);
					WriteFile("phantomjs.js", &rwdata);
					}
				return;
				}

			// kml mix
			if (url = ucheck(query, "kml="))
				{
				// Output response headers
				FCGX_PutS("Access-Control-Allow-Origin: *\r\n", FCGX_stdout);
				if (gpx)
					FCGX_PutS("Content-Type: application/octet-stream\r\n", FCGX_stdout);
				else
					FCGX_PutS("Content-Type: application/vnd.google-earth.kml+xml\r\n", FCGX_stdout);				
				FCGX_PutS(MkString("Content-Disposition: attachment; filename=%s\r\n", filename), FCGX_stdout);		
				FCGX_PutS(MkString("Set-Cookie: rwfilename=%s\r\n", filename+domain), FCGX_stdout);	

				//FCGX_PutS("Cache-Control: no-cache\r\n", FCGX_stdout);
				//FCGX_PutS("Pragma: no-cache\r\n", FCGX_stdout);
				//P.OUTLINE("Content-Encoding: gzip\r\n");	
				FCGX_PutS("\r\n", FCGX_stdout);

				// force close and flush of data
				Log(LOGINFO, "%s kmlmixing %s %.128s", ipaddr, filename, url);		
				KMLMerge(url, data);
				return;
				}

			// kml extract http://localhost/rwr?gpx=on&filename=angel&kmlx=http://bluugnome.com/cyn_route/angel-cove_angel-slot/angel-cove_angel-slot.aspx
			if (url = ucheck(query, "kmlx="))
				{
				vars file = GetFileName(filename);
				file += gpx ? ".gpx" : ".kml";
				// Output response headers
				FCGX_PutS("Access-Control-Allow-Origin: *\r\n", FCGX_stdout);
				if (gpx)
					FCGX_PutS("Content-Type: application/octet-stream\r\n", FCGX_stdout);
				else
					FCGX_PutS("Content-Type: application/vnd.google-earth.kml+xml\r\n", FCGX_stdout);				
				FCGX_PutS(MkString("Content-Disposition: attachment; filename=%s\r\n", file), FCGX_stdout);		
				FCGX_PutS(MkString("Set-Cookie: rwfilename=%s\r\n", filename+domain), FCGX_stdout);	
				//FCGX_PutS("Cache-Control: no-cache\r\n", FCGX_stdout);
				//FCGX_PutS("Pragma: no-cache\r\n", FCGX_stdout);
				//P.OUTLINE("Content-Encoding: gzip\r\n");	
				FCGX_PutS("\r\n", FCGX_stdout);

				// convert translated url
				const char *translate = NULL;
				if (translate = ucheck(url, ":translate.google.com"))
					url = ucheck(translate, "&u=");
				if (!url) {
					Log(LOGERR, "ERROR: Illegal translate url %.128s", query);
					return;
				}

				// force close and flush of data
				Log(LOGINFO, "%s kmlextracting %s %.128s", ipaddr, file, url);		
				if (!KMLExtract(url, data, ucheck(query, "kmlfx")!=NULL))
					data->write(KMLStart("No data")+KMLEnd());
				return;
				}


			// ----------------------------------------------------------------------

			// Waterflow http://localhost/rwr?gpx=on&filename=on&url=http://ropewiki.com/images/8/8d/La_Jolla_Sea_Caves.kml
  			if (url = ucheck(query, "waterflow="))
			    {
				// Output response headers
				FCGX_PutS("Access-Control-Allow-Origin: *\r\n", FCGX_stdout);
				//FCGX_PutS("Content-Type: application/octet-stream\r\n", FCGX_stdout);
				if (ucheck(query, "wfkmlrect="))
					{
					FCGX_PutS("Content-Type: application/octet-stream\r\n", FCGX_stdout);
					FCGX_PutS("Content-Disposition: attachment; filename=waterflow\r\n", FCGX_stdout);		
					}
				else
					{
					FCGX_PutS("Content-Type: application/json\r\n", FCGX_stdout);
					}


				//FCGX_PutS("Cache-Control: no-cache\r\n", FCGX_stdout);
				//FCGX_PutS("Pragma: no-cache\r\n", FCGX_stdout);
				//P.OUTLINE("Content-Encoding: gzip\r\n");	
				FCGX_PutS("\r\n", FCGX_stdout);

				//if (++querycnt>1)
				//	Log(LOGINFO, "Multithread %d", querycnt);
#if 0
				AfxBeginThread(threadwaterflow, request);
				request = new FCGX_Request;
				FCGX_InitRequest(request, 0, 0);
#else
				WaterflowQuery(request);
#endif
				//--querycnt;
				return;
                }
/*				
  			if (url = ucheck(query, "BBOX="))
			    {
				static CString str;
				vara bbox = vars(url).split(",");
				double west = atof(bbox[0]);
				double south = atof(bbox[1]);
				double east = atof(bbox[2]);
				double north = atof(bbox[3]);

				double center_lng = ((east - west) / 2) + west;
				double center_lat = ((north - south) / 2) + south;
				query = str = MkString("rwl=box,%.6f,%.6f,0", center_lat, center_lng);
				}
*/
   
			// ----------------------------------------------------------------------
  			if (url = ucheck(query, "code="))
			    {
				// Output response headers
				FCGX_PutS("Access-Control-Allow-Origin: *\r\n", FCGX_stdout);
				//FCGX_PutS("Content-Type: application/octet-stream\r\n", FCGX_stdout);
				FCGX_PutS("Content-Type: text/html\r\n", FCGX_stdout);
				FCGX_PutS("\r\n", FCGX_stdout);

				FCGX_PutS(MkString("<CODE>%s</CODE>",url), FCGX_stdout);
				return;
				}


			// Waterflow http://localhost/rwr?gpx=on&filename=on&url=http://ropewiki.com/images/8/8d/La_Jolla_Sea_Caves.kml
  			if (url = ucheck(query, "pictures="))
			    {
				// Output response headers
				FCGX_PutS("Access-Control-Allow-Origin: *\r\n", FCGX_stdout);
				//FCGX_PutS("Content-Type: application/octet-stream\r\n", FCGX_stdout);
				FCGX_PutS("Content-Type: application/json\r\n", FCGX_stdout);

				//FCGX_PutS("Cache-Control: no-cache\r\n", FCGX_stdout);
				//FCGX_PutS("Pragma: no-cache\r\n", FCGX_stdout);
				//P.OUTLINE("Content-Encoding: gzip\r\n");	
				FCGX_PutS("\r\n", FCGX_stdout);

				//if (++querycnt>1)
				//	Log(LOGINFO, "Multithread %d", querycnt);
				inetproxy data(FCGX_stdout);
				int num = PictureQuery(url, data);

				Log(LOGINFO, "%s picextracting %d pics %.128s", ipaddr, num, url);		
				//--querycnt;
				return;
                }

			if (url = ucheck(query, "rws="))
			    {
				//Log(LOGINFO, "RWS Cookies:%s", FCGX_GetParam("HTTP_COOKIE", request->envp));
				const char *cookies = FCGX_GetParam("HTTP_COOKIE", request->envp);

				BOOL CSVOUT = strstri(url,"OUTPUT=CSV")!=NULL;
				BOOL KMLDATA = strstri(url,"OUTPUT=KMLDATA")!=NULL;
				vars outfile = ExtractString(url, "scanname=","","&");
				if (outfile.IsEmpty() && cookies)
					outfile = ExtractString(cookies, "scanname=","",";");
				outfile = GetToken(outfile, 0, "&;").Trim();
				if (outfile.IsEmpty()) 
					outfile = "download";
				outfile +=  CSVOUT ? CSV : KML;
				char *ext = GetFileExt(outfile);

				// Output response headers
				FCGX_PutS("Access-Control-Allow-Origin: *\r\n", FCGX_stdout);
				//FCGX_PutS("Content-Type: application/octet-stream\r\n", FCGX_stdout);
				if (KMLDATA)
					FCGX_PutS("Content-Type: text/html\r\n", FCGX_stdout); //application/octet-stream\r\n", FCGX_stdout);
				else if (CSVOUT)
					FCGX_PutS("Content-Type: text/csv\r\n", FCGX_stdout);
				else if (ext && stricmp(ext,"kmz")==0)
					FCGX_PutS("Content-Type: application/vnd.google-earth.kmz\r\n", FCGX_stdout);
				else
					FCGX_PutS("Content-Type: application/vnd.google-earth.kml+xml\r\n", FCGX_stdout);
				FCGX_PutS(MkString("Content-Disposition: attachment; filename=%s\r\n", GetFileNameExt(outfile) ), FCGX_stdout);		
				//FCGX_PutS(MkString("Content-Disposition: inline; filename=%s\r\n",outfile), FCGX_stdout);		


				FCGX_PutS(MkString("Status: %d\r\n", 202), FCGX_stdout);
				FCGX_PutS("\r\n", FCGX_stdout);


				if (IsSimilar(url, "scan.kml"))
					{
					// generate file dynamically
					inetproxy data(FCGX_stdout);					
					ElevationSaveScan(CKMLOut(&data, NULL));
					return;
					}

				if (url[0]=='*')
					{
					// Click Me
					FCGX_PutS( KMLStart(NULL), FCGX_stdout);
					FCGX_PutS( KMLScanBox(url, cookies), FCGX_stdout);
					FCGX_PutS( KMLEnd(), FCGX_stdout);
					return;
					}
				
				if (url[0]=='!' || CSVOUT || KMLDATA)
					{		
					vars config = url;
					inetproxy data(FCGX_stdout);
					if (CSVOUT)
						ScoreCanyons(config, cookies, NULL, &data);
					else
						ScoreCanyons(config, cookies, &CKMLOut(&data, NULL), NULL);
					return;
					}

				vars name = ExtractString(url_decode(url), "scanname=", "" ,";");
				FCGX_PutS( KMLStart(NULL, TRUE), FCGX_stdout);
				FCGX_PutS("<NetworkLink><name>"+name+"</name><open>1</open><Link><href>"+MkString("http://%s/rwr/?rws=!;", server)+vars(url)+"</href></Link></NetworkLink>", FCGX_stdout);
				FCGX_PutS( KMLEnd(TRUE), FCGX_stdout);

				/* 
				// GOOGLE EARTH DOES NOT DOWNLOAD AFTER POST!!!
				// posted scanner data
				char * len = FCGX_GetParam("CONTENT_LENGTH", envp);
				unsigned long postlen = atoi(len); 
				if (postlen>0)
					{
					char *postdata = (char *)malloc( postlen+1 );
					memset(postdata, 0, postlen+1);
					FCGX_GetStr(postdata, postlen, FCGX_stdin);
					free(postdata);
					config = postdata;
					}
				*/

				//Log(LOGINFO, "CANYONSCANNER %s", url);
				//Log(config);

				}


			if (url = ucheck(query, "rwl="))
			    {
				int ulen = strlen(url);
				BOOL special = (url[ulen-1]=='@' || url[ulen-1]=='$') || url[ulen-1]=='!';
				CString outfile = "POI\\"+CString(url);
				char *ext = GetFileExt(outfile);
				if (special!=NULL)
					outfile = "output", ext=NULL;

				// Output response headers
				FCGX_PutS("Access-Control-Allow-Origin: *\r\n", FCGX_stdout);
				//FCGX_PutS("Content-Type: application/octet-stream\r\n", FCGX_stdout);
				if (ext && stricmp(ext,"kmz")==0)
					FCGX_PutS("Content-Type: application/vnd.google-earth.kmz\r\n", FCGX_stdout);
				else
					FCGX_PutS("Content-Type: application/vnd.google-earth.kml+xml\r\n", FCGX_stdout);
				FCGX_PutS(MkString("Content-Disposition: attachment; filename=%s\r\n", GetFileNameExt(outfile) ), FCGX_stdout);		
				//FCGX_PutS(MkString("Content-Disposition: inline; filename=%s\r\n",outfile), FCGX_stdout);		


				//FCGX_PutS("Cache-Control: no-cache\r\n", FCGX_stdout);
				//FCGX_PutS("Pragma: no-cache\r\n", FCGX_stdout);
				//P.OUTLINE("Content-Encoding: gzip\r\n");	
				FCGX_PutS("\r\n", FCGX_stdout);
					//CString name = GetToken(url,1)+"_"+GetToken(url,2);
					// http://localhost/rwr/?rwl=X_34.200997_-118.093847.kml

				if (special)
					{
					// Network Link Control
					CString name = ElevationFilename(url);
					FCGX_PutS( KMLStart(name, TRUE), FCGX_stdout);
					CString out = NetworkLink(name, url);
					FCGX_PutS( out, FCGX_stdout);
					FCGX_PutS( KMLEnd(TRUE), FCGX_stdout);
					return;
					}

				if (url[0]=='@')
					{
					// Click Me
					FCGX_PutS( KMLStart(NULL), FCGX_stdout);
					FCGX_PutS( KMLClickMe(url), FCGX_stdout);
					FCGX_PutS( KMLEnd(), FCGX_stdout);
					return;
					}

				if (url[0]=='*')
					{
					// Click Me
					FCGX_PutS( KMLStart(NULL), FCGX_stdout);
					FCGX_PutS( KMLScanBox(url), FCGX_stdout);
					FCGX_PutS( KMLEnd(), FCGX_stdout);
					return;
					}

				// return a kml/kmz file
				if (!ext || strnicmp(GetFileExt(outfile),"km", 2)!=0) 
					return;
				inetproxy data(FCGX_stdout);
				WriteFile(outfile, &data);
				return;
				}

			// Canyon //localhost/rw?rwz=lat,lng[&dist=dist]
			// http://lucac.no-ip.org/rwr/?rwz=Cerberus,36.2138,-116.7293
			// http://localhost/rwr/?rwz=Rubio,34.209,-118.115,1000
  			if (url = ucheck(query, "rwz="))
			    {
				if (IsSimilar(url,"RWE.KC"))
					if (AuthenticateUser(FCGX_GetParam("HTTP_COOKIE", request->envp))!=NULL)
						url = "RWE.+KC";

				const char *nozip = ucheck(query, "nozip=");
				const char *nolines = ucheck(query, "nolines=");
				const char *nolines2 = ucheck(query, "nolines2=");
				int len = strlen(url);
				//int cache = url[len-1]=='C';
				CString name = ElevationFilename(url);
				CString outfile = ElevationPath(name)+ (nozip ? KML : KMZ);
				DEBUGRWZ(TRUE ? "START "+name : "");
				// Output response headers
				FCGX_PutS("Access-Control-Allow-Origin: *\r\n", FCGX_stdout);
				//FCGX_PutS("Content-Type: application/octet-stream\r\n", FCGX_stdout);
				if (nozip)
					FCGX_PutS("Content-Type: application/vnd.google-earth.kml+xml\r\n", FCGX_stdout);
				else
					FCGX_PutS("Content-Type: application/vnd.google-earth.kmz\r\n", FCGX_stdout);
				FCGX_PutS(MkString("Content-Disposition: attachment; filename=%s\r\n", GetFileNameExt(outfile) ), FCGX_stdout);		
				//FCGX_PutS(MkString("Content-Disposition: inline; filename=%s\r\n",outfile), FCGX_stdout);		

				//FCGX_PutS("Cache-Control: no-cache\r\n", FCGX_stdout);
				//FCGX_PutS("Pragma: no-cache\r\n", FCGX_stdout);
				//P.OUTLINE("Content-Encoding: gzip\r\n");	
			
				inetproxy data(FCGX_stdout);

				// if cached, just send back
				if (!nozip && CFILE::exist(outfile)) //cache && 
					{
						CFILE f;
						if (!f.fopen(outfile))
							{
							DEBUGRWZ("FILEBUSY");
							FCGX_PutS(MkString("Status: %d\r\n", 429), FCGX_stdout);
							FCGX_PutS("\r\n", FCGX_stdout);
							return;
							}

						// check expiration date
						double time = f.ftime();
						double expire = GetCacheMaxDays(outfile);
						BOOL expired = time<=0 || expire<=0;
						int minsize = 64;  // minsize valid zip
						if (!expired && time+expire>GetTime())
							{
							DEBUGRWZ("FILECACHED");
							FCGX_PutS(MkString("Status: %d\r\n", 200), FCGX_stdout);
							FCGX_PutS("\r\n", FCGX_stdout);
							int r, size = 0;
							char buff[1024*8];
							while ((r = f.fread(buff, 1, sizeof(buff)))>0)
								data.write(buff, r);
							return;
							}
						else
							{
							DEBUGRWZ("FILENEW");
							}
					}

				// compute 			
				FCGX_PutS(MkString("Status: %d\r\n", 202), FCGX_stdout);
				FCGX_PutS("\r\n", FCGX_stdout);

				tzipquery q; ZeroMemory(&q, sizeof(q)); q.query = url; q.name = name;

				if (nozip)
					{
					// compute 			
					if (CreatePipe(&q.hread,&q.hwrite,NULL,0))
						{
						DEBUGRWZ("NOZIPSTART");
						vars qurl = vars(q.query) + "K" + (nolines ? "N" : "") + (nolines2 ? "X" : "");
						q.query = qurl;
						QueryZip(q);
						WriteFile(q.hread, &data);
						}
					else
						{
						Log(LOGERR, "Pipe creation failed for query=%s", query);
						}
					return;
					}


				//HZIP hz = CreateZip(0,100000,ZIP_MEMORY);
				HZIP hz = CreateZip((void *)((const char *)outfile), 0,ZIP_FILENAME);
				if (hz) 
					{
					//Log(LOGINFO, "ZIP: %s", outfile);
					// adding something from a pipe...
					if (CreatePipe(&q.hread,&q.hwrite,NULL,0))
						{
						DEBUGRWZ("ZIPSTART");
						QueryZip(q);
						ZipAdd(hz,"doc.kml",q.hread,0,ZIP_HANDLE);
						for (int i=0; i<q.filelist.GetSize() && q.ok; ++i)
							ZipAdd(hz,GetFileNameExt(q.filelist[i]), (void *)((const char *)q.filelist[i]), 0, ZIP_FILENAME);
						DEBUGRWZ(q.ok ? "ZIPEND ->"+name : "ZIPEND ERROR!");
						CloseHandle(q.hread);
						}
					else
						{
						Log(LOGERR, "Pipe creation failed for query=%s", query);
						}
					CloseZip(hz);
					if (!q.ok)
						{
						Log(LOGERR, "Zip deleted for query=%s", query);
						DEBUGRWZ("ZIPDELETED");
						DeleteFile(outfile);
						return;
						}
					}
				else
					{
					// file is busy, retry 3 times (takes 10 seconds each retry)
					DEBUGRWZ("ZIPBUSY");
					/*
					CFILE f;
					for (int i=0; i<3; ++i)
						{
						if (f.fopen(outfile))
							break;
						}
					*/
					}

				/*
				// and now that the zip is created, let's do something with it:
				void *zbuf; unsigned long zlen; ZipGetMemory(hz,&zbuf,&zlen);
				FCGX_PutStr((const char *)zbuf, zlen, FCGX_stdout);
				HANDLE hfz = CreateFile("test.zip", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
				DWORD writ; WriteFile(hfz,zbuf,zlen,&writ,NULL);
				CloseHandle(hfz); 
				CloseZip(hz);	
				*/
				WriteFile(outfile, &data);
				DEBUGRWZ("END");
				//if (!cache) DeleteFile(outfile);
				return;
                }

			// Profile http://localhost/rwr/?profile=Rubio_34209_-118115_1000.png
  			if (url = ucheck(query, "profile="))
			    {
				// Output response headers
				FCGX_PutS("Access-Control-Allow-Origin: *\r\n", FCGX_stdout);
				//FCGX_PutS("Content-Type: application/octet-stream\r\n", FCGX_stdout);
				FCGX_PutS("Content-Type: image/png\r\n", FCGX_stdout);
				//FCGX_PutS(MkString("Content-Disposition: attachment; filename=%s\r\n",outfile), FCGX_stdout);		


				//FCGX_PutS("Cache-Control: no-cache\r\n", FCGX_stdout);
				//FCGX_PutS("Pragma: no-cache\r\n", FCGX_stdout);
				//P.OUTLINE("Content-Encoding: gzip\r\n");	
				FCGX_PutS("\r\n", FCGX_stdout);

				CString outfile = ElevationPath(url_decode(url))+PNG;
				if (stricmp(url, "legend")==0)
					outfile = legendfile;

				inetproxy data(FCGX_stdout);
				WriteFile(outfile, &data);
				return;
                }

  			if (url = ucheck(query, "rwzdownload="))
			{
				FCGX_PutS("Access-Control-Allow-Origin: *\r\n", FCGX_stdout);
				FCGX_PutS("Content-Type: text/html\r\n", FCGX_stdout);
				FCGX_PutS("\r\n", FCGX_stdout);

				CString body = "Click the link below to download the data (without links)<br><ul>";
				body += MkString("<li><a href=\"%s\">Download All Data as KMZ</a></li>", MkString("http://%s/rwr/?rwz=%s", server, url));
				body += MkString("<li><a href=\"%s\">Download All Data as KML</a></li>", MkString("http://%s/rwr/?nozip=on&rwz=%s", server, url));
				body += MkString("<li><a href=\"%s\">Download Only Rap Data as KML</a></li>", MkString("http://%s/rwr/?nozip=on&nolines=on&rwz=%s", server, url));
				body += MkString("<li><a href=\"%s\">Download Only Rap Waypoints as KML</a></li>", MkString("http://%s/rwr/?nozip=on&nolines2=on&rwz=%s", server, url));
				FCGX_PutS("<html><body>"+body+"</body></html>", FCGX_stdout);
			}

			if (url = ucheck(query, "rwbeta"))
			    {
				Log(LOGINFO, "rwbeta=%s", url);
				FCGX_PutS("Access-Control-Allow-Origin: *\r\n", FCGX_stdout);
				//FCGX_PutS("Content-Type: application/octet-stream\r\n", FCGX_stdout);
				FCGX_PutS("Content-Type: application/octet-stream\r\n", FCGX_stdout);
				FCGX_PutS(MkString("Content-Disposition: attachment; filename=%s\r\n", "rw.csv"), FCGX_stdout);	
				FCGX_PutS("\r\n", FCGX_stdout);
				inetproxy data(FCGX_stdout);
				WriteFile("beta\\rw.csv", &data);
				return;
				}

			if (url = ucheck(query, "test="))
			    {
				Log(LOGINFO, "Test=%s", url);
				FCGX_PutS("Access-Control-Allow-Origin: *\r\n", FCGX_stdout);
				//FCGX_PutS("Content-Type: application/octet-stream\r\n", FCGX_stdout);
				FCGX_PutS("Content-Type: text/html\r\n", FCGX_stdout);
				FCGX_PutS("\r\n", FCGX_stdout);
				FCGX_PutS("<html><body></body></html>", FCGX_stdout);
				return;
				}

			for (int i=0; ctable[i].name!=NULL; ++i)
  			  if (url = ucheck(query, ctable[i].name))
			    {
				// Output response headers
				FCGX_PutS("Access-Control-Allow-Origin: *\r\n", FCGX_stdout);
				FCGX_PutS(MkString("Content-Type: %s\r\n", ctable[i].header), FCGX_stdout);
				//FCGX_PutS("Content-Type: application/octet-stream\r\n", FCGX_stdout);
				//FCGX_PutS("Cache-Control: no-cache\r\n", FCGX_stdout);
				//FCGX_PutS("Pragma: no-cache\r\n", FCGX_stdout);
				//P.OUTLINE("Content-Encoding: gzip\r\n");	
				

				inetproxy data(FCGX_stdout);
				if (IsSimilar(url, "http"))
					{
					// remote file
					FCGX_PutS("\r\n", FCGX_stdout);
					DownloadFile f(TRUE, &data);
					if (f.Download(url))
						Log(LOGERR, "ERROR: can't download url %.128s", url);
					//else
					//	Log(LOGINFO, "%s bypassed %s %dKb %.128s", ipaddr, ext, f.size/1024, url);
					}
				else
					{
					// local file
					vars file = url_decode(url);
					file.Replace("/", "\\");
					file.Trim(" \\");
					int size = WriteFile(file, NULL);
					vars tmp;
					FCGX_PutS(tmp=MkString("Content-Length: %d\r\n", size), FCGX_stdout);
					FCGX_PutS(MkString("Content-Disposition: attachment; filename=%s\r\n", GetFileNameExt(file)), FCGX_stdout);
					FCGX_PutS("\r\n", FCGX_stdout);
					WriteFile(file, &data);
					}
/*
                JSON LIST
				FCGX_PutS("{\"list\":[\"timeSeriesResponseType\",\"2ndline\"]}\r\n", FCGX_stdout);
			    //\"declaredType\":\"org.cuahsi.waterml.TimeSeriesResponseType\"}\r\n", FCGX_stdout);
				//	"\"list\":[ \"line1\", \"line2\"]\r\n", FCGX_stdout);
*/
				return;
                }

		}

	//if (postdata) free(postdata);
	//*((char *)NULL) = 'C';

}


int process(FCGX_Request *request)
{
#ifdef CRASHPROTECTION
#ifndef DEBUG
	__try 
#endif
#endif
	    { 
		_process(request);
		}

#ifdef CRASHPROTECTION
#ifndef DEBUG
  __except(EXCEPTION_EXECUTE_HANDLER) 
		{ 
		Log(LOGALERT, "PROCESS CRASHED!!! %.250s (see log for full url)", FCGX_GetParam("QUERY_STRING", request->envp)); 	
		Log(FCGX_GetParam("QUERY_STRING", request->envp));
		return 0; 
		}
#endif
#endif
		return 0;
}

const char *webattr(const char *attr, FCGX_ParamArray envp)
{
	const char *val = FCGX_GetParam(attr, envp);
	if (val != NULL)
		return val;
	return "";
}

static UINT threadmain(LPVOID pParam)
{
	static CCriticalSection Accept;
	// go back one directory
	//SetCurrentDirectory("..");

	ThreadLock();
	++THREADNUM;
	ThreadSet(NULL);
	ThreadUnlock();

	FCGX_OpenSocket(":9001", 500);

    FCGX_Request *request = new FCGX_Request;
    FCGX_InitRequest(request, 0, 0);

    //while (FCGX_Accept(&FCGX_stdin, &FCGX_stdout, &FCGX_stderr, &envp) == 0)
    while (running)
    {
		Accept.Lock();
		int rc = FCGX_Accept_r(request);
		Accept.Unlock();

		if (rc < 0)
			continue;

/*
DOCUMENT_ROOT	The root directory of your server
HTTP_COOKIE	The visitor's cookie, if one is set
HTTP_HOST	The hostname of the page being attempted
HTTP_REFERER	The URL of the page that called your program
HTTP_USER_AGENT	The browser type of the visitor
HTTPS	"on" if the program is being called through a secure server
PATH	The system path your server is running under
QUERY_STRING	The query string (see GET, below)
REMOTE_ADDR	The IP address of the visitor
REMOTE_HOST	The hostname of the visitor (if your server has reverse-name-lookups on; otherwise this is the IP address again)
REMOTE_PORT	The port the visitor is connected to on the web server
REMOTE_USER	The visitor's username (for .htaccess-protected pages)
REQUEST_METHOD	GET or POST
REQUEST_URI	The interpreted pathname of the requested document or CGI (relative to the document root)
SCRIPT_FILENAME	The full pathname of the current CGI
SCRIPT_NAME	The interpreted pathname of the current CGI (relative to the document root)
SERVER_ADMIN	The email address for your server's webmaster
SERVER_NAME	Your server's fully qualified domain name (e.g. www.cgi101.com)
SERVER_PORT	The port number your server is listening on
SERVER_SOFTWARE	The server software you're using (e.g. Apache 1.3)
*/
		int n = 0;
		CString strings; 
		strings += " IP:";
		strings += webattr("REMOTE_ADDR", request->envp);
		strings += " URL:";
		strings += webattr("QUERY_STRING", request->envp);
		strings += " From:";
		strings += webattr("HTTP_FROM", request->envp);
		strings += " Agt:";
		strings += webattr("HTTP_USER_AGENT", request->envp);
		strings += " Ref:";
		strings += webattr("HTTP_REFERER", request->envp);
		ThreadSet(strings); 
		process(request);
		ThreadSet(NULL);
		FCGX_Finish_r(request);
    }
   
	delete request;

	ThreadLock();
	--THREADNUM;
	ThreadUnlock();
	return 0;
}

typedef struct {const char *id, *desc; } tcmd;
class ccmdlist {
	public:
	CArrayList<tcmd> list;
	int chk(const char *arg, const char *id, const char *desc)
	{
		tcmd c; c.id = id; c.desc = desc;
		list.push(c);
		return stricmp(arg, id)==0;
	}
};


int MODE = -1, SIMPLIFY = 0;
extern LLRect *Savebox;

extern double FLOATVAL;
extern int INVESTIGATE;
int Command(TCHAR *argv[])
{
	if (!argv || !*argv)
		return FALSE;
	// command line parameters
	ccmdlist cmd;
	if (cmd.chk(*argv,"-bbox", "bounding box of region, def in regions.csv, use -bbox TEST to get region test.kml"))
	   {
	   const char *region = argv[1];
	   if ( region == NULL || region[0] =='-')
		{
		Log(LOGERR, "Invalid region %s", region);
		exit(0);
		}

	   CArrayList<LLRect> boxes; 
	   vara regions = vars(region).split();
	   {
	   CKMLOut out("BBox"); 
	   for (int i=0; i<regions.length(); ++i)
		   boxes.push(GetBox(out, regions[i]));
	   }
	   Log(LOGINFO, "BBOX: %s", regions.join());
	   for (int i=0; i<regions.length(); ++i)
		   if (!boxes[i].IsNull())
				{
				Log(LOGINFO, "PROCESSING BBOX: %s", regions[i]);
				Savebox = &boxes[i];
				Command(argv+2);
				}
	   return TRUE;
	   }


	if (cmd.chk(*argv,"-investigate", "-1 disabled (def), 0 only if not known, 1 for every link"))		
		{
		INVESTIGATE = atoi(argv[1]);
		return Command(argv+2);
		}

	if (cmd.chk(*argv,"-val", "floating point value"))		
		{
		FLOATVAL = atof(argv[1]);
		return Command(argv+2);
		}

	if (cmd.chk(*argv,"-mode", "-1 0 1"))		
		{
		MODE = atoi(argv[1]);
		return Command(argv+2);
		}

	if (cmd.chk(*argv,"-tmode", "-1:smartmerge 0:forcemerge -2:overwrite"))		
		{
		extern int TMODE;
		TMODE = atoi(argv[1]);
		return Command(argv+2);
		}

	if (cmd.chk(*argv,"-simplify", "to simplify"))		
		{
		SIMPLIFY = 1;
		return Command(argv+1);
		}

	if (cmd.chk(*argv,"-getcraiglist", "keyfile.csv [htmfile.htm] (MODE:1 ignore done list) (1 in keyword list too)") && argv[1])
	   {
	   GetCraigList(argv[1], argv[2]);
	   return TRUE;
	   }

	if (cmd.chk(*argv,"-gettop40", "[year] [D:\\Music\\00+Top\\] (MODE: -1:latest 0:download csv 1:download mp3)"))
	   {
	   GetTop40(argv[1], argv[1] ? argv[2] : "40");
	   return TRUE;
	   }

	if (cmd.chk(*argv,"-getsites", "[name] update waterflow sites"))
	   {
	   //CStringArrayList list;
	   //WaterflowRect("rect=10,10,10,20", list);
	   WaterflowSaveSites(argv[1]);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-getwatershed", "[name] update waterflow sites with watershed information"))
	   {
	   //CStringArrayList list;
	   //WaterflowRect("rect=10,10,10,20", list);
	   WatershedSaveSites(argv[1]);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-testsites", "[name] update waterflow sites"))
	   {
	   //CStringArrayList list;
	   //WaterflowRect("rect=10,10,10,20", list);
	   WaterflowTestSites(argv[1]);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-getfsites", "update waterflow forecast maps (NOOA)"))
	   {
	   //CStringArrayList list;
	   //WaterflowRect("rect=10,10,10,20", list);
	   WaterflowSaveForecastSites();
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-getusgssites", "update usgs site, old and current"))
	   {
	   //CStringArrayList list;
	   //WaterflowRect("rect=10,10,10,20", list);
	   WaterflowSaveUSGSSites();
	   return TRUE;
	   }

	if (cmd.chk(*argv,"-getkml", "get KML for all the CSV data files, use -bbox to specify region"))
	   {
	   //CStringArrayList list;
	   //WaterflowRect("rect=10,10,10,20", list);
	    CString name = "RW";
        if (argv[1]!=NULL)
		   name = argv[1];
	   SaveKML(name);
	   return TRUE;
	   }

	if (cmd.chk(*argv,"-getcsvkml", "<file.csv> get KML of CSV data") && argv[1])
	   {
	   //CStringArrayList list;
	   //WaterflowRect("rect=10,10,10,20", list);
	   SaveCSVKML(argv[1]);
	   return TRUE;
	   }

	if (cmd.chk(*argv,"-getdem", "[demfile.csv] update DEM.kml and DEM.csv (MODE=0 move irrelevant files out MODE=1 to create thermal png/kml)"))
	   {
	   ElevationSaveDEM(argv[1]);
	   return TRUE;
	   }

	if (cmd.chk(*argv,"-getscan", "[quad] [scanID] generates quad CSV file, info on ScanID OR generates SCAN.kml (no parameters)"))
	   {  
	   // output SCAN.kml
	   ElevationSaveScan(argv[1]);
	   return TRUE;
	   }

	if (cmd.chk(*argv,"-testcanyons", "run canyon mapping from canyons.txt or specific lat lng dist"))
	   {
	   ElevationProfile(argv+1);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-getlegend", "produce legend.png"))
	   {
	   ElevationLegend(legendfile);
	   return TRUE;
	   }
	
	if (cmd.chk(*argv,"-getpanoramio", "update panoramio with BBOX (-mode -1:all 0:tags 1:title, -investigate >=0 to force refresh)"))
	   {
	   DownloadPanoramioUrl(MODE);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-getflickr", "update flickr with BBOX (-mode -1:all 0:tags 1:title, -investigate >=0 to force refresh)"))
	   {
	   DownloadFlickrUrl(MODE);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-getpoi", "update POI database from web, static and dynamic kmls"))
	   {
	   DownloadPOI();
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-getrwloc", "<num days> update RWLOC database from web (default 7, 1e6 to reset database)"))
	   {
	   DownloadRWLOC(argv[1]);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-getbeta1", "[code] BETA -> CHG.CSV (CHANGES)"))
	   {
	   DownloadBeta(MODE=1, (argv[1] && strcmp(argv[1], "rw")==0) ? NULL : argv[1]);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-getbetakmlx", "<file.kml> <url> run kml extraction on url") && argv[1] && argv[2] )
		{
		inetfile file(argv[1]);
		KMLExtract(argv[2], &file, TRUE);
		return TRUE;
		}
	if (cmd.chk(*argv,"-getbetakml", "-mode -2:from web FULL -1:from web update [default], 0:from web verbose, 1:from current files)"))
	   {
	   DownloadBetaKML(MODE, argv[1]);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-fixbetakml","[color_section | file.csv] (-mode -1:transform 1:select-only) "))
	   {
	   FixBetaKML(MODE, argv[1]);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-uploadbetakml", ""))
	   {
	   UploadBetaKML(MODE, argv[1]);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-getbeta", "[code] BETA -> CHG.CSV (DOWNLOAD) -mode -2:from web FULL -1:from web update [default], 0:from web verbose, 1:from current files)"))
	   {
	   DownloadBeta(MODE, argv[1]);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-getbetalinks", "[code] BETA -> CHG.CSV (investigate: -1 auto filter bad links, 0 no filter, 1 recheck all) use -setbeta to upload"))
	   {
	   extern int ITEMLINKS;
	   ITEMLINKS = TRUE;
	   DownloadBeta(MODE=1, argv[1]);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-checkbeta", "[code] check for duplicate links and other incongruencies [MODE 1 for dead links]"))
	   {
	   CheckBeta(MODE, argv[1]);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-setbeta", "[file.csv] (beta\\chg.csv by default) -> ropewiki (-mode -2:force set -1:set, 0:set verbose, 1:simulate )"))
	   {
	   UploadBeta(MODE, argv[1]);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-setbetaregion", "file.csv create regions if needed"))
	   {
	   UploadRegions(MODE, argv[1]);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-setstars", "[code] BETA -> ropewiki (-mode -1:set, 0:set verbose, 1:simulate)"))
	   {
	   UploadStars(MODE, argv[1]);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-setconditions", "[code] CONDITIONS -> ropewiki (-mode -3:force all -2:force latest -1:update latest 0:update all 1:test/simulate )"))
	   {
	   UploadConditions(MODE, argv[1]);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-getfbconditions", "fbc.csv [TEST/fcb2.csv/fbname/file.log/http:] -> FB Conditions (-mode -1:groups+users 0:groups 1:users 2:posting users 3:new friends, -investigate: numpages / -days)") && argv[1])
	   {
	   DownloadFacebookConditions(MODE, argv[1], argv[2]);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-setfbconditions", "fbc.csv CONDITIONS -> ropewiki (-mode -2:force -1:update 0:no likes 1:filter new)") && argv[1])
	   {
	   UploadFacebookConditions(MODE, argv[1]);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-getfbpics", "[name/url filter] -> fbpics.csv (-mode -1:latest 1:last10 5:last50)") && argv[1])
	   {
	   DownloadFacebookPics(MODE, argv[1]);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-setpics", "pic.csv -> ropewiki (-mode -2:overwrite_pic 1:preview_new 2:preview_all)") && argv[1])
	   {
	   UploadPics(MODE, argv[1]);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-resetbetacol", "ITEM_X will reset column") && argv[1])
	   {
	   ResetBetaColumn(MODE, argv[1]);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-setbetaprop", "\"string=newstring\" <file.csv/file.log/Region>\"") && argv[1] && argv[2])
	   {
	   ReplaceBetaProp(MODE, argv[1], argv[2]);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-getquery", "\"[[Category:Page ratings]][[Has page rating user::QualityBot]]\" file.csv") && argv[1] && argv[2])
	   {
	   QueryBeta(MODE, argv[1], argv[2]);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-getbetakml", "\"url\" [out.kml] (Mode:1 to force unprotected extraction)") && argv[1])
	   {
	   ExtractBetaKML(MODE, argv[1], argv[2]);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-getbetalist", "gets list of extracted beta sites (Mode:1 test rwlang=?)"))
	   {
	   ListBeta();
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-uploadbetabqn", "ynfile.csv -> ropewiki (-mode -4:rebuildStarsYes -3:overwrite -2:merge -1:set, 0:set verbose, 1:simulate )") && argv[1])
	   {
	   UpdateBetaBQN(MODE, argv[1]);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-uploadbetaccs", "chgfile.csv -> ropewiki (-mode -4:rebuildStarsYes -3:overwrite -2:merge -1:set, 0:set verbose, 1:simulate )") && argv[1])
	   {
	   UpdateBetaCCS(MODE, argv[1]);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-undobeta", "<file.csv/file.log/Region/Canyon/Query> undo last posted updates ") && argv[1])
	   {
	   UndoBeta(MODE, argv[1]);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-purgebeta", "<file.csv/file.log/Region/Canyon/Query>, ALL if empty (-mode 0:disambiguation 1:regions -2:Autorefresh)"))
	   {
	   PurgeBeta(MODE, argv[1]);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-refreshbeta", "<file.csv/file.log/Region/Canyon/Query> ( force Beta Site link refresh )") && argv[1] )
	   {
	   RefreshBeta(MODE, argv[1]);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-fixbeta", "<file.csv/file.log/Region/Canyon/Query>, ALL if empty (including disambiguation) if empty (MODE 1 to fake update)"))
	   {
	   FixBeta(MODE, argv[1]);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-fixconditions", "[[Query]] or ALL "))
	   {
	   FixConditions(MODE, argv[1]);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-fixbetaregion", "Region (mode:0,1,2,3 (minlevel) -> georegionfix.csv, nomode:->chg.csv)"))
	   {
	   FixBetaRegion(MODE, argv[1]);
	   //DisambiguateBeta();
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-disbeta", "[rename.csv] (-mode 1 to add new disambiguations only)"))
	   {
	   DisambiguateBeta(MODE, argv[1]);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-movebeta", "rename.csv (pairs of old name, new name) [MODE 1 to force move of subpages]") && argv[1])
	   {
	   MoveBeta(MODE, argv[1]);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-deletebeta", "file.csv (name or id) comment") && argv[1] && argv[2])
	   {
	   DeleteBeta(MODE, argv[1], argv[2]);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-simplynamebeta", "rwfile.csv movefile.csv (mode:0 NO REGIONS, mode:1 capitalize, 2 extra simple)") && argv[1] && argv[2])
	   {
	   SimplynameBeta(MODE, argv[1], argv[2]);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-testbeta", "column [code] (MODE 1 to force extra checks)") && argv[1])
	   {
	   TestBeta(MODE, argv[1], argv[2]);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-getriversuskml", "update <outfolder> with RIVERS + WATERSHED data with BBOX (-mode -1:both 0:rivers 1:watershed)")&& argv[1]!=NULL)
	   {
	   DownloadRiversUS(MODE, argv[1]);
	   if (MODE<0) CalcRivers(argv[1], "POI\\RIVERSUS", 0, 0, 0);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-getriversusurl", "update <outfolder> with WATERSHED (-investigate 0 to check, 1 to process oldest to newest)") && argv[1]!=NULL)
	   {
	   DownloadRiversUSUrl(MODE, argv[1]);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-getriversus", "update RIVERSUS with <infolder> data (-mode 1 to join)") && argv[1]!=NULL)
	   {
	   CalcRivers(argv[1], "POI\\RIVERSUS", MODE>0, 0, 0);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-getriversca", "update RIVERSCA with <infolder> data (-mode 0 to NOT reorder)") && argv[1]!=NULL)
	   {
	   extern gmlread gmlcanada;
	   extern gmlfix gmlcanadafix;
	   DownloadRiversGML(argv[1], argv[1], gmlcanada, gmlcanadafix);
	   CalcRivers(argv[1], "POI\\RIVERSCA", 1, 1, MODE!=0);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-getriversmx", "update RIVERSMX with <infolder> data (-mode 1 to reorder)") && argv[1]!=NULL)
	   {
	   extern gmlread gmlmexico;
	   extern gmlfix gmlmexicofix;
	   DownloadRiversGML(argv[1], argv[1], gmlmexico, gmlmexicofix);
	   CalcRivers(argv[1], "POI\\RIVERSMX", 1, 1, MODE>0);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-calcrivers", "<infolder> to <outfolder>  (-simplify to simplify, -mode 1 to reorder, -mode 0 not join)") && argv[1]!=NULL && argv[2]!=NULL)
	   {
	   CalcRivers(argv[1], argv[2], MODE!=0, SIMPLIFY, MODE==1, argv[3]);
	   return TRUE;
		}
	if (cmd.chk(*argv,"-transrivers", "<infolder> to <outfolder> (-mode 0 old format, -mode 1 reset summary)") && argv[1]!=NULL && argv[2]!=NULL)
	   {
	   TransRivers(argv[1], argv[2], MODE);
	   return TRUE;
		}
	if (cmd.chk(*argv,"-checkriversid", "check <folder> RIVERS") && argv[1]!=NULL)
	   {
	   CheckRiversID(argv[1]);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-fixrivers", "check & fix with timestamp <folder> RIVERS bounding box match and duplicates") && argv[1]!=NULL)
	   {
	   FixRivers(argv[1]);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-getriverselev", "update with timestamp <folder> [def ALL] with elevation data (-investigate 0 to force recalc, 1 to check values/changes)"))
	   {
	   GetRiversElev(argv[1]);
	   return TRUE;
	   }
	/*
	if (cmd.chk(*argv,"-getriverswg", "update with timestamp <folder> with waterfall/gage data (-investigate 0 to force recalc, 1 to check values/changes)"))
	   {
	   GetRiversWG(argv[1]);
	   return TRUE;
	   }
   */
	if (cmd.chk(*argv,"-fixjlink", "check & fix <folder> <*name*> CSV files and change naked urls to jlink") && argv[1]!=NULL && argv[2]!=NULL)
	   {
	   FixJLink(argv[1], argv[2]);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-fixpanoramio", "check & fix from <folderin> to <folderout> CSV files and change naked urls to jlink") && argv[1]!=NULL && argv[2]!=NULL)
	   {
	   FixPanoramio(argv[1], argv[2]);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-scan", "[file] for direct, [file.csv] for list, -bbox for region, blank for all files in SCAN (-MODE -3:check -2:check&fix -1:add missing 0:redoDCONN 1:redoWF 2:full redo -TMODE GoogleElev -1:auto 0:disabled 1:redo errora -investigate 0 for extra messages)"))
	   {
 	   ScanCanyons(argv[1], Savebox);
	   return TRUE;
	   }

	if (cmd.chk(*argv,"-scanbk", "scans in background, [file] for direct, [file.csv] for list, -bbox for region"))
	   {
	    if (argv[1]!=NULL || Savebox)
			{
			MODE=-20;
			ScanCanyons(argv[1], Savebox);
			return TRUE;
			}
		// infinite loop
		while (TRUE)
			{
			// wake up every minute and scan remaining
			ScanCanyons();
			Sleep(60*1000);
			}
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-score", "report use -bbox BBOX to specify area otherwise from default config"))
	   {
	   inetfile data("score.csv");
	   CKMLOut out("score");
	   ScoreCanyons(NULL, NULL, &out, &data);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-id", "id SEGMENT from <id> & BBOX")  && argv[1]!=NULL && Savebox)
	   {
	   IdCanyons(Savebox, argv[1]);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-cmpcsv", "compare <folderin> vs <folderout> CSV files (-investigate 1 to get details)") && argv[1]!=NULL && argv[2]!=NULL)
	   {
	   CompareCSV(argv[1], argv[2]);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-copycsv", "copy from <folderin> to <folderout> CSV files to match a BBOX (-mode 1 to move files)") && argv[1]!=NULL && argv[2]!=NULL && Savebox)
	   {
	   CopyCSV(argv[1], argv[2]);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-copydem", "copy from <demfile> to <folderout> DEM files to match a BBOX (-mode 1 to move files)") && argv[1]!=NULL && argv[2]!=NULL && Savebox)
	   {
	   CopyDEM(argv[1], argv[2]);
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-checkcsv", "check all CVS (POI & RIVERS) for duplicates or bad data"))
	   {
	   CheckCSV();
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-checkkmz", "check expired files in KMZ (-mode 1 to delete)"))
	   {
	   CheckKMZ();
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-getdem", "download ASTER GDEM tiles (BBOX required)"))
	   {
	   DownloadDEM();
	   return TRUE;
	   }
	if (cmd.chk(*argv,"-test", "<file>/folder"))
	    {
		void Test(const char *);
		Test(argv[1]);
		return TRUE;
		}
	if (cmd.chk(*argv,"-testpics", "<file.csv>"))
	    {
		void TestPics(void);
		TestPics();
		return TRUE;
		}

	if (cmd.chk(*argv,"-debug", "enable debug mode (inet)"))
	    {
		MDEBUG = TRUE;
		return FALSE;
		}
	if (cmd.chk(*argv,"-download", "download <file> <url>"))
	   {
	   if (!argv[1] || !argv[2])
		{
		Log(LOGERR, "-download <file> <url>");
		return TRUE;
		}
	   CString url = argv[2], file = argv[1];
	   DownloadFile f;
	   if (f.Download(url, file))
		Log(LOGERR, "Could not download %s", url);
	   return TRUE;
	   }

	printf("Commands:\n");
	for (int i=0; i<cmd.list.length(); ++i)
		printf("  %s : %s\n", cmd.list[i].id, cmd.list[i].desc);
	exit(0);
}


int main(int argc, TCHAR* argv[])
{
	extern int URLTIMEOUT; URLTIMEOUT= 60;
	extern int URLRETRY; URLRETRY= 2;

	extern BOOL NOWEEKEND;
	NOWEEKEND = FALSE;

	vara cmd;
	for (int i=1; argv[i]!=NULL; ++i)
		cmd.push(argv[i]);

	SetLog(argv[0]);
	Log(LOGINFO, "=================== %s %s =====================", argv[0], cmd.join(" "));

	// run command mode
	for (int i=1; argv[i]!=NULL; ++i)
		if (Command(argv+i))
			return 0;
		
	// WebServer mode
	WaterflowLoadSites();

	//CFILE::MDEBUG = MDEBUG;
	//Log(LOGINFO, "STATMASTER: SM v:%d %s", version, MDEBUG ? "DEBUGMODE" : "");

	// test
	//WaterflowQuery("&wfid=CDEC:HIB&wfdates=2012-08-29N,2012-06-29,2014-06-29", NULL);
	//WaterflowQuery("&wfid=CNWO:08GA047&wfdates=2014-10-28N,2014-06-29,2012-07-26",NULL);


	//SetOutput(NULL);
	//fclose(stderr);

    FCGX_Init();

	//CWinThread* BeginThread( AFX_THREADPROC pfnThreadProc, LPVOID pParam );

    CWinThread* threads[THREADCOUNT];
    for (int i = 0; i < THREADCOUNT; i++)
        threads[i] = AfxBeginThread((AFX_THREADPROC)threadmain, (void*)i);

#ifdef ONEZIIP
	zipth = AfxBeginThread(threadzip,(void*)NULL);
#endif
#ifdef SCANTHREAD
	scanth = AfxBeginThread(threadscan,(void*)NULL);
#endif

	DeleteFile(abortfile);

	Log(LOGINFO, "Version %s %s Actively Listening", __DATE__, __TIME__);  
    while (running)
		{
		Sleep(FLUSHTIME*60*1000);
		WaterflowFlush();
		ElevationFlush();
		running = !CFILE::exist(abortfile);
		}

	Log(LOGINFO, "waiting for %d threads (%d ip)", THREADNUM, ThreadSetNum());
	while (THREADNUM>0)
		WaitThread();

	//WaterflowUnloadSites();
	return 0;
}


#endif


/*



void OUTBINARY(void)
{
	//setmode(fileno(stdout), O_BINARY);
}

	
		if (IsSimilar(tstr, "zss=") && !zss)
			{
			zssfile = MkString( "%s%d.tmp", GetTokenRest(tstr, 1, '='), GetTickCount());
			zss = fopen(zssfile, "wb");
			continue;
			}


CString zssfile;
FILE *zss = NULL;
*/
/*
if (zss)
		{
		fclose(zss);
		if (zsscrc)
			if (CRC(zssfile)==zsscrc)
				{
				OUTDATA(" \n");
				//Log(LOGERR, "deleting zssfile1 %s", zssfile);
				if (!DeleteFile(zssfile))
					Log(LOGERR, "cound not delete zssfile1");
				return ret;
				}
		// reimplement, keep in memory
		//%%%zss = fopenretry(zssfile, "rb");
		//%%%zssencode(zss, stdout, zssflags);
		//%%%fclose(zss);
		//Log(LOGERR, "deleting zssfile2 %s", zssfile);
		//%%%if (!DeleteFile(zssfile))
			//%%%Log(LOGERR, "cound not delete zssfile2");
		}
	return ret;
*/



/*
// get file name and save it @@@ reimplement
CString file = GetToken(GetToken((char *)postdata, 0), 0, '='); file.Trim();
file = GetFileName(file) + ".csv";
if (file.Find('/')>=0 || file.Find('\\')>=0 || file.Right(3).CompareNoCase("csv")!=0)
	{
	Log( LOGERR, "ERROR: STAT-POST of illegal file %s", file);
	return 0;
	}

CFILE f; f.fopen(file, "wb");
if (!f)
	{
	Log( LOGERR, "ERROR: STAT-POST Can't write to %s", file);
	return 0;
	}

fwrite(postdata, 1, nContentLength, f);
*/

































#ifdef SIMPLECGI

using namespace std;

int main(int argc, TCHAR* argv[], TCHAR* envp[])
{
	DWORD started = GetTickCount();
/*
	// initialize MFC and print and error on failure
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		// TODO: change error code to suit your needs
		_tprintf(_T("Fatal Error: MFC initialization failed\n"));
		return 1;

	}
*/
	//SetOutput(NULL);
	fclose(stderr);

	// go back one directory
	SetCurrentDirectory("..");

	// Output response headers
	fprintf(stdout, "Content-Type: application/octet-stream\n");
	fprintf(stdout, "Cache-Control: no-cache\n");	
	fprintf(stdout, "Pragma: no-cache\n");	
	//fprintf(stdout, "Content-Encoding: gzip\n");	
	//Content-encoding: x-gzip
	fprintf(stdout, "\n");	

	//CGIParser *parser = CGIParser::instance();
#ifdef DEBUGPARAM
	for (int i=0; argv[i]!=NULL; ++i)
		printf("ARG%d: %s\n", i, argv[i]);
	for (int i=0; envp[i]!=NULL; ++i)
		printf("ENV%d: %s\n", i, envp[i]);
#endif
	/*
	FILE *f = fopen("stat\\yday.csv.gz", "rb");
	if (f)
		{
			int cnt;
		Log(LOGINFO, "Transferring file: %s", "yday");
		char line[FILEBUFFLEN];
		while ((cnt=fread(line, 1, FILEBUFFLEN, f))>0)
			fwrite(line, 1, cnt, stdout);
		fclose(f);
		}
	return 1;
	*/
	//char* acc = getenv("HTTP_ACCEPT_ENCODING");
	//Log(LOGINFO, "Accept encoding: %s", acc);

	//char postdata[MAXLINELEN];
	//GetCurrentDirectory(sizeof(buffer), buffer);
	//Log( LOGINFO, "cwd=%s", buffer);
	char *postdata = NULL;
	//Log( LOGINFO, "StatMaster.exe called");
	char* lpszContentLength = getenv("CONTENT_LENGTH");
	int nContentLength = !lpszContentLength ? 0 : atoi(lpszContentLength);
	if (nContentLength>0) // POST
		{
		//Log( LOGINFO, "CONTENT_LENGTH=%d", nContentLength);
		postdata = (char *)malloc( nContentLength+1 );
		memset(postdata, 0, nContentLength+1);
		fread(postdata, 1, nContentLength, stdin);
		//Log( LOGINFO, "postdata=%s", postdata);

		/*

		// get file name and save
		CString file = GetToken(GetToken((char *)postdata, 0), 0, '='); file.Trim();
		file = GetFileName(file) + ".csv";
		if (file.Find('/')>=0 || file.Find('\\')>=0 || file.Right(3).CompareNoCase("csv")!=0)
			{
			Log( LOGERR, "ERROR: STAT-POST of illegal file %s", file);
			return 0;
			}

		{
		CFILE f; f.fopen(file, "wb");
		if (!f)
			{
			Log( LOGERR, "ERROR: STAT-POST Can't write to %s", file);
			return 0;
			}

		fwrite(postdata, 1, nContentLength, f);
		}
		*/

		}

	char* querystring= getenv("QUERY_STRING");


#ifdef DEBUG
	querystring = argv[1];
	postdata = argv[2];
#endif
	if (!querystring)
		return FALSE;

	int ret = 0, crashed = TRUE;
	Log( LOGINFO, "START-%s: %.100s %.100s (%db)", postdata ? "POST" : "GET", querystring ? querystring : "NULL", postdata ? postdata : "NULL", nContentLength);		
	SHIELD( /*ret = PROCESSOUT(querystring, postdata);*/  crashed = FALSE; );
	if (ret<0) crashed = TRUE;
	Log( crashed ? LOGALERT : LOGINFO , "END-%s: %s %dsec", postdata ? "POST" : "GET", crashed ? "CRASHED" : "OK", (GetTickCount()-started)/1000);

	if (postdata) free(postdata);
	return ret;
}



#endif




