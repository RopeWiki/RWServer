// Wikiloc.h : header file
//

#pragma once

#include "inetdata.h"

#define WIKILOC "wikiloc.com"

#define MAXWIKILOCLINKS 5

int WIKILOC_ExtractKML(const char *ubase, const char *url, inetdata *out, int fx);
int WIKILOC_DownloadPage(DownloadFile &f, const char *url, CSym &sym);
int WIKI_DownloadBeta(const char *ubase, CSymList &symlist, int(*DownloadPage)(DownloadFile &f, const char *url, CSym &sym));
int WIKILOC_DownloadBeta(const char *ubase, CSymList &symlist);