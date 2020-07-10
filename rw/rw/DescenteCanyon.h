// DescenteCanyon.h : header file
//

#pragma once

#include "inetdata.h"

int DESCENTECANYON_ExtractKML(const char *ubase, const char *url, inetdata *out, int fx);
int DESCENTECANYON_DownloadBeta(const char *ubase, CSymList &symlist);
int DESCENTECANYON_DownloadConditions(const char *ubase, CSymList &symlist);

// defined but not used in BETAExtract.cpp:
int DESCENTECANYON_DownloadPage(DownloadFile &f, const char *urlc, CSym &sym, BOOL condonly = FALSE);
int DESCENTECANYON_DownloadPage2(DownloadFile &f, const char *urlc, CSym &sym);
double DESCENTECANYON_GetDate(const char *date);
int DESCENTECANYON_ExtractPoints(const char *url, vara &styles, vara &points, vara &lines, const char *ubase);
int DESCENTECANYON_DownloadLatLng(CSym &sym);
