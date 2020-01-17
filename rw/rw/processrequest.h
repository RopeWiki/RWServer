// processrequest.h : main header file
//

#pragma once

#include "stdafx.h"
#include "afxmt.h"
#include "fcgiapp.h"
#include "inetdata.h"


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
void ProcessHeartbeat(const char* url, FCGX_Stream* FCGX_stdout);
void ProcessCTable(const char* url, FCGX_Stream* FCGX_stdout, char* header);

