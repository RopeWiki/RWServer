// RWServer.h : main header file
//

#pragma once

#include "stdafx.h"

#define legendfile "POI\\legend.png"

int Command(TCHAR *argv[]);

static UINT threadmain(LPVOID pParam);
static UINT threadscan(LPVOID pParam);
static UINT threadwaterflow(LPVOID pParam);
static UINT threadzip(LPVOID pParam);

const char *webattr(const char *attr, FCGX_ParamArray envp);

int process(FCGX_Request *request);

int PhantomJS(const char *file, volatile int &nfiles);

void _process(FCGX_Request *request);


typedef struct { 
	int ok;
	const char *query; 
	const char *name; 
	CStringArrayList filelist;
	HANDLE hwrite, hread; 
} tzipquery;

void QueryZip(tzipquery &q);


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
