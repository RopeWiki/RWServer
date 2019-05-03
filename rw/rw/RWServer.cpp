#include "stdafx.h"
#include "afxmt.h"
#include "fcgiapp.h"
#include "inetdata.h"
#include "RWServer.h"
#include "processrequest.h"


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
static BOOL MDEBUG = FALSE;

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

int MODE = -1, SIMPLIFY = 0;
extern LLRect *Savebox;

extern double FLOATVAL;
extern int INVESTIGATE;
int Command(TCHAR *argv[]) // command line parameters
{
	if (!argv || !*argv)
		return FALSE;

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

