// RWServer.h : main header file
//

#pragma once

#include "stdafx.h"


int Command(TCHAR *argv[]);

static UINT threadmain(LPVOID pParam);

const char *webattr(const char *attr, FCGX_ParamArray envp);

int process(FCGX_Request *request);

int PhantomJS(const char *file, volatile int &nfiles);

void _process(FCGX_Request *request);

void ProcessUrl(const char* url, FCGX_Stream* FCGX_stdout, CString filename, inetdata* data);
void ProcessIProgress(const char* url, FCGX_Stream* FCGX_stdout);
void ProcessPDFx(const char* url, FCGX_Stream* FCGX_stdout, CString filename, const char* domain);
void ProcessZipx(const char* url, FCGX_Stream* FCGX_stdout, CString filename, const char* domain, char* ipaddr);
void ProcessKml(const char* url, FCGX_Stream* FCGX_stdout, CString filename, inetdata* data, const char* domain, char* ipaddr, const char* gpx);
void ProcessKmlExtract(const char* url, FCGX_Stream* FCGX_stdout, CString filename, inetdata* data, const char* domain, char* ipaddr, const char* gpx, const char* query);
void ProcessWaterflow(FCGX_Request* request, FCGX_Stream* FCGX_stdout, const char* query);
void ProcessCode(const char* url, FCGX_Stream* FCGX_stdout);
void ProcessPictures(const char* url, FCGX_Stream* FCGX_stdout, char* ipaddr);
int  ProcessRWS(const char* url, FCGX_Request* request, FCGX_Stream* FCGX_stdout);
void ProcessRWL(const char* url, FCGX_Stream* FCGX_stdout);
void ProcessRWZ(const char* url, FCGX_Request* request, FCGX_Stream* FCGX_stdout, const char* query);
void ProcessProfile(const char* url, FCGX_Stream* FCGX_stdout);
void ProcessRWZDownload(const char* url, FCGX_Stream* FCGX_stdout);
void ProcessRWBeta(const char* url, FCGX_Stream* FCGX_stdout);
void ProcessTest(const char* url, FCGX_Stream* FCGX_stdout);
void ProcessCTable(const char* url, FCGX_Stream* FCGX_stdout, int i);


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


typedef struct { 
	int ok;
	const char *query; 
	const char *name; 
	CStringArrayList filelist;
	HANDLE hwrite, hread; 
} tzipquery;


const char *webattr(const char *attr, FCGX_ParamArray envp)
{
	const char *val = FCGX_GetParam(attr, envp);
	if (val != NULL)
		return val;
	return "";
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
