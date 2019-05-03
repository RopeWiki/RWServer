#pragma once

#include "stdafx.h"
#include "fcgiapp.h"
#include "rw.h"
#include "utils.h"
#include "xzip.h"

class inetproxy : public inetdata {
	FCGX_Stream *FCGX_stdout;
	int size;

public:
	inetproxy(FCGX_Stream *FCGX_stdout);
	void write(const void *buffer, int size);
};


class inetgpx : public inetdata {
	FCGX_Stream *FCGX_stdout;
	inetfile *f;
	CString url, infile, outfile;
	int size;

public:
	inetgpx(FCGX_Stream *FCGX_stdout, CString url);
	void write(const void *buffer, int size);

	~inetgpx();
};


class inetrwpdf : public inetdata {
	FCGX_Stream *FCGX_stdout;
	inetfile *f;
	CString opts, url, infile, outfile, logfile;
	int size;

public:
	inetrwpdf(FCGX_Stream *FCGX_stdout, CString url, CString opts);
	void write(const void *buffer, int size);

	~inetrwpdf();
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
	inetrwzip(FCGX_Stream *FCGX_stdout, CString id, CString opts);
	void write(const void *buffer, int size);
	
	~inetrwzip();

	static UINT inetrwzip::senddata(LPVOID pParam)
	{
		inetrwzip *rwzip = (inetrwzip *)pParam;
		inetproxy data(rwzip->FCGX_stdout);
		utils::WriteFile(rwzip->hread, &data);
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
};
