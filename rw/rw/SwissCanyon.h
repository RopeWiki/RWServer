#pragma once

#include "BETAExtract.h"
#include "trademaster.h"


int SWISSCANYON_GetSummary(CSym &sym, const char *summary);
int SWISSCANYON_DownloadPage(DownloadFile &f, CSym &sym);
int SWISSCANYON_DownloadTable(const char *url, const char *base, CSymList &symlist);
int SWISSCANYON_DownloadBeta(const char *ubase, CSymList &symlist);
int SWESTCANYON_DownloadBeta(const char *ubase, CSymList &symlist);

double CHtoWGSlat(double y, double x);
double CHtoWGSlng(double y, double x);