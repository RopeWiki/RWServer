// RoadTripRyan.h : header file
//

#pragma once

#include "inetdata.h"

int ROADTRIPRYAN_ExtractKML(const char *ubase, const char *url, inetdata *out, int fx);
int ROADTRIPRYAN_DownloadBeta(const char *ubase, CSymList &symlist2);

// defined but not used in BETAExtract.cpp:
int ROADTRIPRYAN_DownloadXML(DownloadFile &f, const char *url, CSymList &symlist);
int ROADTRIPRYAN_DownloadDetails(DownloadFile &f, const char *url, CSymList &symlist);
