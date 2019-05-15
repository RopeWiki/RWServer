
#pragma once

#include "inetdata.h"

// inetdata classes:


int inetdata::PhantomJS(const char *file, volatile int &nfiles)
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


//inetproxy
inetproxy::inetproxy(FCGX_Stream *FCGX_stdout)
{
	size  = 0;
	this->FCGX_stdout = FCGX_stdout; 
}

void inetproxy::write(const void *buffer, int size)
{
	FCGX_PutStr((const char *)buffer, size, FCGX_stdout);
	int newsize = this->size + size;
	//if (this->size<32 && newsize>32)
	//	FCGX_FFlush(FCGX_stdout);
	this->size = newsize;
	//Log(LOGINFO, "Write %d", size));
}


//inetgpx
inetgpx::inetgpx(FCGX_Stream *FCGX_stdout, CString url)
{

	this->url = url;
	this->FCGX_stdout = FCGX_stdout; 
	f = NULL;
}

void inetgpx::write(const void *buffer, int size)
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

inetgpx::~inetgpx()
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
		utils::WriteFile(outfile, &data);
	}
	else {
		Log(LOGERR, "ERROR: NOT converted gpx ret:%d url:%.128s", ret, url);
	}

	DeleteFile(infile);
	DeleteFile(outfile);
}


//inetrwpdf
inetrwpdf::inetrwpdf(FCGX_Stream *FCGX_stdout, CString url, CString opts)
{

	this->url = url;
	this->opts = opts;
	this->FCGX_stdout = FCGX_stdout; 
	f = NULL;
}

void inetrwpdf::write(const void *buffer, int size)
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

inetrwpdf::~inetrwpdf()
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
		utils::WriteFile(outfile, &data);
		DeleteFile(infile);
		DeleteFile(outfile);
		DeleteFile(logfile);
	}
	else {
		Log(LOGERR, "ERROR: pdf NOT converted .rw file:%s url:%.128s", infile, url);
	}
}


//inetrwzip
inetrwzip::inetrwzip(FCGX_Stream *FCGX_stdout, CString id, CString opts)
{

	this->url = id;
	this->opts = opts;
	this->FCGX_stdout = FCGX_stdout; 
	f = NULL;
}

void inetrwzip::write(const void *buffer, int size)
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

		// This is a workaround for a problem where the PDFList script gives us some newlines that makes phantomjs hang
		// and eventually time out without any useful error message. Example, the results of this query
		//	http://ropewiki.com/index.php/PDFList?action=raw&templates=expand&ctype=application/x-zope-edit&noscript=on&pagename=Behunin_Canyon&bslinks=on&summary=off&docwidth=1903&ext=.rw
		// contains this:
		//	var linedata="\
		//	*Behunin_Canyon http://ropewiki.com/index.php/PDFList?pagename=Behunin+Canyon\n\
		//	**(INFO).pdf http://ropewiki.com/Behunin_Canyon\n\
		//	**(MAP).pdf http://ropewiki.com/index.php/Map?pagename=Behunin+Canyon\n\
		//	**(MAP).kml http://ropewiki.com/images/c/cb/Behunin_Canyon.kml\n\
		//	**BS1.kml 
		//	http://luca.ropewiki.com/rwr?gpx=off&filename=X&kmlx=http://bluugnome.com/cyn_route/zion_behunin/zion_behunin-canyon.aspx&ext=.kml\n\
		//	...
		//
		// The problem there is that the **BS1.kml line ends with a newline without a backslash indicating
		// that the string contains on the next line. The code below tries to fix up this situation by concatenating
		// these lines. A more proper fix would be to figure out why PDFList gives us these lines, and fix it there.
		// - Fredrik
		//
		// TODO: Remove the code block below when we have fixed the PDFList issue.
		do
		{
			// Create an array of lines from the input string.
			vara lines(data, "\n");

			// Loop over the lines.
			bool parsingstring = false;		// Not parsing a string initially.
			for(int i = 0; i < lines.length(); i++)
			{
				// Remember if we were parsing a string when we started this line.
				bool wasparsingstring = parsingstring;

				// Get the current line and look for " characters.
				vars currentline = lines[i];
				int len = currentline.length();
				for(int j = 0; j < len; j++)
				{
					if(currentline[j] == '\"')
					{
						// This " indicates that we started parsing a string, or stopped.
						parsingstring = !parsingstring;
					}
				}

				// Having reached the end of the line, check if we are currently parsing a string.
				if(parsingstring)
				{
					// Does it end with a backslash?
					if(len >= 1 && currentline[len - 1] == '\\')
					{
						// This line has the proper ending for the string to continue on the next line.
					}
					else
					{
						// printf("Found possible problem: '%s'\n", currentline);

						// In this case, we seem to have a line in a string that ends without a backslash.
						// Check if there is a line after this, which we can add in (eventually essentially removing the newline character).
						if(i + 1 < lines.length())
						{
							// Add the next line to this line.
							lines[i] = currentline + lines[i + 1];

							// Remove that next line.
							lines.RemoveAt(i + 1);

							// Go back one step - we need to look at this line again, since the concatenated line might also
							// have an improper ending.
							i--;

							// Also, since we are looking at this whole line again, any " we found on this line before don't
							// count, we will find them again.
							parsingstring = wasparsingstring;
						}
					}
				}
			}

			// Put the lines back together into a string.
			data = lines.join("\n");
		}
		while(0);
			
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

inetrwzip::~inetrwzip()
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
