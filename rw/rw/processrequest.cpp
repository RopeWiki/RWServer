
#include "processrequest.h"
#include "RWServer.h"


#define DEBUGRWZ(msg) /*Log(LOGINFO, msg": %s", query)*/

void ProcessUrl(const char* url, FCGX_Stream* FCGX_stdout, CString filename, inetdata* data)
{
	// proxy passthrough 
	//http://localhost/rw?gpx=on&filename=on&url=http://ropewiki.com/images/8/8d/La_Jolla_Sea_Caves.kml
	//http://lucac.no-ip.org/rwr?url=http://ropewiki.com/images/d/df/Stevenson_Creek_%28Lower%29.kml

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
}

void ProcessIProgress(const char* url, FCGX_Stream* FCGX_stdout)
{	
	FCGX_PutS("Access-Control-Allow-Origin: *\r\n", FCGX_stdout);
	FCGX_PutS("Content-Type: text/html\r\n", FCGX_stdout);
	FCGX_PutS("\r\n", FCGX_stdout);

	CString html = CFILE::fileread("poi\\iprogress.htm");
	CString base = "/rwr?"; //vara(query, "iprogress=")[0];
	html.Replace("#URL#", base + url);
	FCGX_PutS(html, FCGX_stdout);
}

void ProcessPDFx(const char* url, FCGX_Stream* FCGX_stdout, CString filename, const char* domain)
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
				
	utils::WriteFile("phantomjs.js", &rwdata);

	/*
	DownloadFile f(TRUE, &rwdata);

	vars rwurl = "http://ropewiki.com/index.php/PDFList?limit=100&action=raw&templates=expand&ctype=application/x-zope-edit&query=%5B%5B"+id+"%5D%5D"+opt;

	if (f.Download(rwurl))
		Log(LOGERR, "ERROR: can't download rwurl %.128s", rwurl);
	else
		Log(LOGINFO, "%s rwdownload %s %dKb %.128s", ipaddr, filename, f.size/1024,rwurl);
	*/
}

void ProcessZipx(const char* url, FCGX_Stream* FCGX_stdout, CString filename, const char* domain, char* ipaddr)
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
		utils::WriteFile("phantomjs.js", &rwdata);
	}
}

void ProcessKml(const char* url, FCGX_Stream* FCGX_stdout, CString filename, inetdata* data, const char* domain, char* ipaddr, const char* gpx)
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
}

void ProcessKmlExtract(const char* url, FCGX_Stream* FCGX_stdout, CString filename, inetdata* data, const char* domain, char* ipaddr, const char* gpx, const char* query)
{
	// kml extract http://localhost/rwr?gpx=on&filename=angel&kmlx=http://bluugnome.com/cyn_route/angel-cove_angel-slot/angel-cove_angel-slot.aspx
			
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
}

void ProcessWaterflow(FCGX_Request* request, FCGX_Stream* FCGX_stdout, const char* query)
{
	// Waterflow http://localhost/rwr?gpx=on&filename=on&url=http://ropewiki.com/images/8/8d/La_Jolla_Sea_Caves.kml
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
}

void ProcessCode(const char* url, FCGX_Stream* FCGX_stdout)
{
	// Output response headers
	FCGX_PutS("Access-Control-Allow-Origin: *\r\n", FCGX_stdout);
	//FCGX_PutS("Content-Type: application/octet-stream\r\n", FCGX_stdout);
	FCGX_PutS("Content-Type: text/html\r\n", FCGX_stdout);
	FCGX_PutS("\r\n", FCGX_stdout);

	FCGX_PutS(MkString("<CODE>%s</CODE>",url), FCGX_stdout);
}

void ProcessPictures(const char* url, FCGX_Stream* FCGX_stdout, char* ipaddr)
{
	// Waterflow http://localhost/rwr?gpx=on&filename=on&url=http://ropewiki.com/images/8/8d/La_Jolla_Sea_Caves.kml
  			
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
}

int ProcessRWS(const char* url, FCGX_Request* request, FCGX_Stream* FCGX_stdout)
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
		return TRUE;
	}

	if (url[0]=='*')
	{
		// Click Me
		FCGX_PutS( KMLStart(NULL), FCGX_stdout);
		FCGX_PutS( KMLScanBox(url, cookies), FCGX_stdout);
		FCGX_PutS( KMLEnd(), FCGX_stdout);
		return TRUE;
	}
				
	if (url[0]=='!' || CSVOUT || KMLDATA)
	{		
		vars config = url;
		inetproxy data(FCGX_stdout);
		if (CSVOUT)
			ScoreCanyons(config, cookies, NULL, &data);
		else
			ScoreCanyons(config, cookies, &CKMLOut(&data, NULL), NULL);
		return TRUE;
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

	return false;
}

void ProcessRWL(const char* url, FCGX_Stream* FCGX_stdout)
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
	utils::WriteFile(outfile, &data);
}

void ProcessRWZ(const char* url, FCGX_Request* request, FCGX_Stream* FCGX_stdout, const char* query)
{
	// Canyon //localhost/rw?rwz=lat,lng[&dist=dist]
	// http://lucac.no-ip.org/rwr/?rwz=Cerberus,36.2138,-116.7293
	// http://localhost/rwr/?rwz=Rubio,34.209,-118.115,1000

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
			utils::WriteFile(q.hread, &data);
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
	utils::WriteFile(outfile, &data);
	DEBUGRWZ("END");
	//if (!cache) DeleteFile(outfile);
}

void ProcessProfile(const char* url, FCGX_Stream* FCGX_stdout)
{
	// Profile http://localhost/rwr/?profile=Rubio_34209_-118115_1000.png
  			
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
	utils::WriteFile(outfile, &data);
}

void ProcessRWZDownload(const char* url, FCGX_Stream* FCGX_stdout)
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

void ProcessRWBeta(const char* url, FCGX_Stream* FCGX_stdout)
{
	Log(LOGINFO, "rwbeta=%s", url);
	FCGX_PutS("Access-Control-Allow-Origin: *\r\n", FCGX_stdout);
	//FCGX_PutS("Content-Type: application/octet-stream\r\n", FCGX_stdout);
	FCGX_PutS("Content-Type: application/octet-stream\r\n", FCGX_stdout);
	FCGX_PutS(MkString("Content-Disposition: attachment; filename=%s\r\n", "rw.csv"), FCGX_stdout);	
	FCGX_PutS("\r\n", FCGX_stdout);
	inetproxy data(FCGX_stdout);
	utils::WriteFile("beta\\rw.csv", &data);
}

void ProcessTest(const char* url, FCGX_Stream* FCGX_stdout)
{
	Log(LOGINFO, "Test=%s", url);
	FCGX_PutS("Access-Control-Allow-Origin: *\r\n", FCGX_stdout);
	//FCGX_PutS("Content-Type: application/octet-stream\r\n", FCGX_stdout);
	FCGX_PutS("Content-Type: text/html\r\n", FCGX_stdout);
	FCGX_PutS("\r\n", FCGX_stdout);
	FCGX_PutS("<html><body></body></html>", FCGX_stdout);
}

void ProcessHeartbeat(const char* url, FCGX_Stream* FCGX_stdout)
{
	FCGX_PutS("Access-Control-Allow-Origin: *\r\n", FCGX_stdout);
	FCGX_PutS("Content-Type: text/html\r\n", FCGX_stdout);
	FCGX_PutS("\r\n", FCGX_stdout);
	FCGX_PutS("<html><body></body></html>", FCGX_stdout);
}
void ProcessCTable(const char* url, FCGX_Stream* FCGX_stdout, char* header)
{
	// Output response headers
	FCGX_PutS("Access-Control-Allow-Origin: *\r\n", FCGX_stdout);
	FCGX_PutS(MkString("Content-Type: %s\r\n", header), FCGX_stdout);
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
		int size = utils::WriteFile(file, NULL);
		vars tmp;
		FCGX_PutS(tmp=MkString("Content-Length: %d\r\n", size), FCGX_stdout);
		FCGX_PutS(MkString("Content-Disposition: attachment; filename=%s\r\n", GetFileNameExt(file)), FCGX_stdout);
		FCGX_PutS("\r\n", FCGX_stdout);
		utils::WriteFile(file, &data);
	}
/*
			JSON LIST
			FCGX_PutS("{\"list\":[\"timeSeriesResponseType\",\"2ndline\"]}\r\n", FCGX_stdout);
			//\"declaredType\":\"org.cuahsi.waterml.TimeSeriesResponseType\"}\r\n", FCGX_stdout);
			//	"\"list\":[ \"line1\", \"line2\"]\r\n", FCGX_stdout);
*/
}
