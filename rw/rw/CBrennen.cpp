#include "CBrennen.h"
#include "BETAExtract.h"
#include "trademaster.h"

CBRENNEN::CBRENNEN(const char *base) : BETAC(base)
{
	kfunc = DownloadKML;
	umain = burl(ubase, "/content.htm");
}

int CBRENNEN::DownloadDetails(DownloadFile &f, CSym &sym, CSymList &symlist)
{
	if (!UpdateCheck(symlist, sym) && sym.GetNum(ITEM_LAT) != InvalidNUM && MODE > -2)
		return TRUE;

	const char *url = sym.id;
	if (f.Download(url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}

	vars summary = ExtractString(ExtractString(f.memory, "ACA Rating", ":", "</UL>"), "", "", "<IMG");
	GetSummary(sym, stripHTML(summary));
	GetRappels(sym, GetNumbers(stripHTML(ExtractString(f.memory, "Difficulties", ":", "<LI>"))));
	GetTimeDistance(sym, stripHTML(ExtractString(f.memory, "iking distance", ":", "<LI>")));
	GetTimeDistance(sym, stripHTML(ExtractString(f.memory, "iking time", ":", "<LI>").replace("minute", "xinute")));

	double stars = 0;
	if (strstr(summary, "1star"))
		stars = 1.5;
	if (strstr(summary, "2star"))
		stars = 3;
	if (strstr(summary, "3star"))
		stars = 4.5;
	sym.SetStr(ITEM_STARS, starstr(stars, 1));

	//ASSERT( !strstr(sym.data, "Rathlin"));

	CString desc;
	float lat = InvalidNUM, lng = InvalidNUM;
	CString fmemory = CoordsMemory(f.memory);
	const char *hike = strstr(fmemory, "<h3><B>Hike");
	if (hike)
		ExtractCoords(hike, lat, lng, desc);
	if (!CheckLL(lat, lng))
		ExtractCoords(fmemory, lat, lng, desc);


	if (CheckLL(lat, lng))
	{
		sym.SetNum(ITEM_LAT, lat);
		sym.SetNum(ITEM_LNG, lng);
		sym.SetStr(ITEM_KML, "X");
	}
	return FALSE;
}

int CBRENNEN::DownloadBeta(CSymList &symlist)
{
	DownloadFile f;
	CString url = burl(ubase, "content.htm");
	if (f.Download(url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}

	vars region;
	// tables for ratings
	vars content = ExtractString(f.memory, "BY REGION", "", "</TABLE");

	vara list(content, "<TD>");
	for (int i = 1; i < list.length(); ++i) {
		vars lnk = ExtractString(list[i], "A HREF=", "\"", "\"");
		vars name = stripHTML(list[i]);
		if (name.Right(1) == ':') {
			region = name.Trim(" : .");
			continue;
		}
		if (name.IsEmpty())
			continue;
		CSym sym(urlstr(burl(ubase, lnk)));
		sym.SetStr(ITEM_DESC, name);
		sym.SetStr(ITEM_REGION, region);
		DownloadDetails(f, sym, symlist);
		Update(symlist, sym, NULL, FALSE);
	}

	return TRUE;
}


int CBRENNEN::DownloadKML(const char *ubase, const char *url, inetdata *out, int fx)
{
	/*
	vars id = GetKMLIDX(f, url, "KmlLayer(", "", ")");
	id.Trim(" '\"");
	if (id.IsEmpty())
		return FALSE; // not available
	*/

	//vars msid = ExtractString(id, "msid=", "", "&");
	CString credit = " (Data by " + CString(ubase) + ")";

	TEXT_ExtractKML(credit, url, "<h3><B>Hike", out);
	return TRUE;
}

int CBRENNEN::TEXT_ExtractKML(const char *credit, const char *url, const char *startmatch, inetdata *out)
{
	DownloadFile f;
	if (DownloadRetry(f, url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}

	vara styles, points, lines, linelist;
	styles.push("dot=http://maps.google.com/mapfiles/kml/shapes/open-diamond.png");

	// process points

	float lat, lng;
	vars desc;
	const char *start = strstr(f.memory, startmatch);
	if (!start) start = f.memory;
	CString fmemory = CoordsMemory(start);
	const char *memory = fmemory;
	int cont = 0;
	for (int i = 1; ((cont = ExtractCoords(memory += cont, lat, lng, desc)) > 0); ++i)
	{
		linelist.push(CCoord3(lat, lng));
		points.push(KMLMarker("dot", lat, lng, MkString("Wpt%d", i), desc + credit));
		// add markers
	}
	if (points.length() == 0)
		return FALSE;
	//lines.push( KMLLine("Aprox. Route", credit, linelist, OTHER, 3) );

	// generate kml
	SaveKML("xtract", credit, url, styles, points, lines, out);
	return TRUE;
}

CString CBRENNEN::GetNumbers(const char *str)
{
	CString s(str);
	s.MakeLower();
	s.Replace("one", "1");
	s.Replace("two", "2");
	s.Replace("three", "3");
	s.Replace("four", "4");
	s.Replace("five", "5");
	s.Replace("six", "6");
	s.Replace("seven", "7");
	s.Replace("eight", "8");
	s.Replace("nine", "9");
	s.Replace("ten", "10");
	s.Replace("eleven", "11");
	s.Replace("twelve", "12");
	s.Replace("thirteen", "13");
	return s;
}




