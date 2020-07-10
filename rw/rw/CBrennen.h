#pragma once

#include "inetdata.h"
#include "BetaC.h"

class CBRENNEN : public BETAC
{
public:
	CBRENNEN(const char *base);
	
	int DownloadDetails(DownloadFile &f, CSym &sym, CSymList &symlist);
	int DownloadBeta(CSymList &symlist);
	static int DownloadKML(const char *ubase, const char *url, inetdata *out, int fx);

private:
	static int TEXT_ExtractKML(const char *credit, const char *url, const char *startmatch, inetdata *out);
	CString GetNumbers(const char *str);
};

