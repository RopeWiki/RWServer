#pragma once

#include "BetaC.h"
#include "BETAExtract.h"
#include "trademaster.h"

class REUNION : public BETAC
{
public:

	REUNION(const char *base) : BETAC(base)
	{
		ticks = 1000;
		umain = "http://www.lrc-ffs.fr/";
		//urls.push("http://www.gulliver.it/canyoning/");
		//x = BETAX("<tr", "</tr", ">nome itinerario", "</table");

	}

	int DownloadPage(const char *url, CSym &sym);

	int DownloadBeta(CSymList &symlist);

};
