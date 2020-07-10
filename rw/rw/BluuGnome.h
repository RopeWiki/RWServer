// BluuGnome.h : header file
//

#pragma once

#include "inetdata.h"

int BLUUGNOME_ExtractKML(const char *ubase, const char *url, inetdata *out, int fx);
int BLUUGNOME_DownloadBeta(const char *ubase, CSymList &symlist);

// defined but not used in BETAExtract.cpp:
int BLUUGNOME_ll(const char *item, double &lat, double &lng);
