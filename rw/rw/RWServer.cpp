#include "stdafx.h"
#include "afxmt.h"
#include "fcgiapp.h"
#include "inetdata.h"
#include "processcommandline.h"
#include "processrequest.h"
#include "RWServer.h"


#define abortfile "abort.rwr"

// Maximum number of bytes allowed to be read from stdin
static const unsigned long STDIN_MAX = 1000000;

#define CRASHPROTECTION

#ifdef DEBUG
#define THREADCOUNT 50	// number of threads
#define FLUSHTIME 1	// flush one file every x minutes
#else
#define THREADCOUNT 20 // number of threads
#define FLUSHTIME 1	// flush one file every x minutes
#endif

//#define WATERFLOW  //load waterflow data at program start
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


#ifndef SIMPLECGI

static int running = TRUE;

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
		
#ifdef WATERFLOW
	// WebServer mode
	WaterflowLoadSites();
#endif

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


CWinThread *scanth = NULL; 
static UINT threadscan(LPVOID pParam)
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


void QueryZip(tzipquery &q)
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


int querycnt = 0;

static int count = 0;

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

	CString user;

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

			if (url=ucheck(query, "user=")) {
				user = ExtractString(url, "", "", "&");
		}

		if (!user || user == "") {
			user = "<guest>";
		}

		Log(LOGINFO, "Incoming request from %s at %s", user, ipaddr);

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
						
		const char *domain = "; domain=ropewiki.com";

		inetproxy urldata(FCGX_stdout);
		inetgpx gpxdata(FCGX_stdout, url);
		inetdata *data = &urldata;
		if (gpx) data = &gpxdata; 
		//data->open(0);

		// ----------------------------------------------------------------------

		if (url=ucheck(query, "url="))
		{
			ProcessUrl(url, FCGX_stdout, filename, data);				
			return;
		}

		if (url=ucheck(query, "iprogress="))
		{
			ProcessIProgress(url, FCGX_stdout);
			return;
		}
			
		if (url=ucheck(query, "pdfx="))
		{
			ProcessPDFx(url, FCGX_stdout, filename, domain);
			return;
		}
			
		if (url=ucheck(query, "zipx="))
		{
			ProcessZipx(url, FCGX_stdout, filename, domain, ipaddr);
			return;
		}

		if (url = ucheck(query, "kml="))
		{
			ProcessKml(url, FCGX_stdout, filename, data, domain, ipaddr, gpx);
			return;
		}

		if (url = ucheck(query, "kmlx="))
		{
			ProcessKmlExtract(url, FCGX_stdout, filename, data, domain, ipaddr, gpx, query);
			return;
		}

  		if (url = ucheck(query, "waterflow="))
		{
			ProcessWaterflow(request, FCGX_stdout, query);			
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
   
  		if (url = ucheck(query, "code="))
		{
			ProcessCode(url, FCGX_stdout);
			return;
		}
			
		if (url = ucheck(query, "pictures="))
		{
			ProcessPictures(url, FCGX_stdout, ipaddr);
			return;
        }

		if (url = ucheck(query, "rws="))
		{
			if (ProcessRWS(url, request, FCGX_stdout))
				return;
			//Luca didn't have a return at the end of this function, but did potentially return from 3 places within the function
		}
			
		if (url = ucheck(query, "rwl="))
		{
			ProcessRWL(url, FCGX_stdout);				
			return;
		}

  		if (url = ucheck(query, "rwz="))
		{
			ProcessRWZ(url, request, FCGX_stdout, query);
			return;
        }

		if (url = ucheck(query, "profile="))
		{
			ProcessProfile(url, FCGX_stdout);
			return;
        }

  		if (url = ucheck(query, "rwzdownload="))
		{
			ProcessRWZDownload(url, FCGX_stdout);
			//Luca didn't have a return here
		}

		if (url = ucheck(query, "rwbeta"))
		{
			ProcessRWBeta(url, FCGX_stdout);
			return;
		}

		if (url = ucheck(query, "test="))
		{
			ProcessTest(url, FCGX_stdout);				
			return;
		}

		for (int i=0; ctable[i].name!=NULL; ++i) 
		{
  			if (url = ucheck(query, ctable[i].name))
			{
				ProcessCTable(url, FCGX_stdout, ctable[i].header);
				return;
			}
		}
	}

	//if (postdata) free(postdata);
	//*((char *)NULL) = 'C';
}

const char *webattr(const char *attr, FCGX_ParamArray envp)
{
	const char *val = FCGX_GetParam(attr, envp);
	if (val != NULL)
		return val;
	return "";
}


#endif //ndef SIMPLECGI



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

