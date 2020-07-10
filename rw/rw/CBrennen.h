//#pragma once
//
//#include "BETAExtract.h"
//#include "trademaster.h"
//#include "inetdata.h"
//#include "BetaC.h"
//
//class CBRENNEN : BETAC
//{
//public:
//	CBRENNEN(const char *base) : BETAC(base);	
//	
//	int DownloadDetails(DownloadFile &f, CSym &sym, CSymList &symlist);
//	int DownloadBeta(CSymList &symlist);
//	int TEXT_ExtractKML(const char *credit, const char *url, const char *startmatch, inetdata *out);
//
//	static int DownloadKML(const char *ubase, const char *url, inetdata *out, int fx)
//	{
//		/*
//		vars id = GetKMLIDX(f, url, "KmlLayer(", "", ")");
//		id.Trim(" '\"");
//		if (id.IsEmpty())
//			return FALSE; // not available
//		*/
//
//		//vars msid = ExtractString(id, "msid=", "", "&");
//		CString credit = " (Data by " + CString(ubase) + ")";
//
//		TEXT_ExtractKML(credit, url, "<h3><B>Hike", out);
//		return TRUE;
//	}
//};
//
