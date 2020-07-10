
#include "BetaC.h"
#include "inetdata.h"
#include "BETAExtract.h"
#include "TradeMaster.h"

vars BETAC::SELL_Make(double slat, double slng, double elat, double elng)
{
	vara list;
	list.push(CData(slat));
	list.push(CData(slng));
	list.push(CData(elat));
	list.push(CData(elng));
	return list.join(":");
}

int BETAC::SELL_MakeKML(const char *title, const char *credit, const char *url, const char *idx, inetdata *out)
{
	vara idxa(idx, ":");
	if (idxa.length() < 4)
		return FALSE;

	vara styles, points, lines;
	styles.push("start=http://maps.google.com/mapfiles/kml/paddle/grn-blank.png");
	styles.push("end=http://maps.google.com/mapfiles/kml/paddle/red-blank.png");

	double slat = CDATA::GetNum(idxa[0]);
	double slng = CDATA::GetNum(idxa[1]);
	double elat = CDATA::GetNum(idxa[2]);
	double elng = CDATA::GetNum(idxa[3]);

	vars creditstr = CDATAS + vars(credit) + CDATAE;
	points.push(KMLMarker("start", slat, slng, "Start", creditstr));
	points.push(KMLMarker("end", elat, elng, "End", creditstr));

	SaveKML(title, credit, url, styles, points, lines, out);
	return TRUE;
}

int BETAC::SELL_ExtractKML(const char *ubase, const char *url, inetdata *out, int fx)
{
	//https://drive.google.com/uc?id=0B7GRxJzuZ66SS201YjgtRDdLa0k&export=download
	DownloadFile f;
	vars id = GetKMLIDX(f, url, NULL);
	if (id.IsEmpty())
		return FALSE; // not available

	CString credit = " (Data by " + CString(ubase) + ")";
	SELL_MakeKML(ubase, credit, url, id, out);
	return TRUE;
}

vars BETAC::MAP_GetID(const char *memory, const char *id)
{
	vars msid;
	const char *sep[] = { "?", "&", ";", NULL };
	for (int i = 0; msid.IsEmpty() && sep[i] != NULL; ++i)
	{
		vara list(memory, MkString("%s%s", sep[i], id));
		for (int l = 1; msid.IsEmpty() && l < list.length(); ++l)
		{
			msid = GetToken(list[l], 0, "\n& <>\"\'");
			// check for invalid msids
			if (strchr(msid, '=') || strchr(msid, '?'))
				msid = "";
		}
	}
	return msid;
}

vars BETAC::MAP_GetID(const char *memory)
{
	vars mapid;
	mapid = MAP_GetID(memory, "msid=");
	if (!mapid.IsEmpty())
		return "MSID:" + mapid;
	mapid = MAP_GetID(memory, "mid=");
	if (!mapid.IsEmpty())
		return "MID:" + mapid;
	return "";
}

int BETAC::MAP_ExtractKML(const char *ubase, const char *url, inetdata *out, int fx)
{
	DownloadFile f;
	vars id = GetKMLIDX(f, url, NULL);
	if (id.IsEmpty())
		return FALSE; // not available

	vars kmlurl;
	if (IsSimilar(id, "MSID:"))
		kmlurl = "http://maps.google.com/maps/ms?ie=UTF8&hl=en&msa=0&output=kml&msid=" + id.Mid(5);
	if (IsSimilar(id, "MID:"))
		kmlurl = "http://www.google.com/maps/d/u/0/kml?mid=" + id.Mid(4);
	if (kmlurl.IsEmpty())
		return FALSE; // not available

	CString credit = " (Data by " + CString(ubase) + ")";
	KMZ_ExtractKML(credit, kmlurl, out);
	return TRUE;
}


BETAC::BETAC(const char *base, DownloadBetaFunc *_dfunc, DownloadKMLFunc *_kfunc, DownloadConditionsFunc *_cfunc)
{
	if (base)
		umain = burl(ubase = obase = base, "/");
	else
		ubase = NULL;

	pages = 1;
	utf = FALSE;
	dfunc = _dfunc;
	kfunc = _kfunc;
	cfunc = _cfunc;
	tickscounter = 0;
	ticks = 1000;
}

int BETAC::ExtractLink(const char *data, const char *ubase, CSym &sym, int utf)
{
	vars name, link;
	name = stripHTML(ExtractString(data, "href=", ">", "</a"));
	link = ExtractString(data, "href=", "", ">");
	link = GetToken(link, 0, ' ');
	link.Trim(" \"'");
	if (link.IsEmpty())
		return FALSE;
	if (!IsSimilar(link, "http"))
	{
		link = makeurl(ubase, link);
		/*
		vara aurl(url, "/");
		aurl[aurl.length()-1] = link;
		link = aurl.join("/");
		*/
	}
	if (utf)
		name = UTF8(name);
	sym = CSym(urlstr(link), name);
	return TRUE;
}


// Customizable functions
int BETAC::DownloadLink(const char *data, CSym &sym)
{
	return ExtractLink(data, ubase, sym, utf);
}
	
int BETAC::DownloadInfo(const char *data, CSym &sym)
{
	if (!region.IsEmpty())
		sym.SetStr(ITEM_REGION, region);
	return TRUE;
}

int BETAC::DownloadBeta(CSymList &symlist)
{
	if (dfunc)
		return dfunc(ubase, symlist);

	if (!x.start)
		return FALSE;

	for (int u = 0; u < urls.length(); ++u)
	{
		vars url = curl = urls[u];
		while (!url.IsEmpty())
			url = DownloadUrl(url, symlist, x), ++pages;
	}

	return symlist.GetSize() > 0;
}

int BETAC::DownloadSym(CSym &sym, CSymList &symlist)
{
	if (!UpdateCheck(symlist, sym) && MODE > -2)
		return FALSE;
	if (!DownloadPage(sym.id, sym))
		return FALSE;
	return Update(symlist, sym, NULL, FALSE);
}

vars BETAC::DownloadUrl(const char *url, CSymList &symlist, BETAX &x, int mode) // 1:recurse // 0:list -1:raw
{
	Throttle(tickscounter, ticks);
	if (f.Download(curl = url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return "";
	}

	vars nexturl = DownloadMore(f.memory);

	// links		
	vars memory = f.memory;
	DownloadPatch(memory);

	DownloadMemory(memory, symlist, x, mode);
	return nexturl;
}

int BETAC::DownloadHTML(const char *path, CSymList &symlist, BETAX &x, int mode)
{
	CFILE file;
	if (!file.fopen(path))
	{
		Log(LOGERR, "ERROR: Could not load file %s", path);
		return FALSE;
	}

	vars memory;
	const char *line;
	while (line = file.fgetstr())
	{
		memory += line;
		memory += " ";
	}
	file.fclose();

	return DownloadMemory(memory, symlist, x);
}

int BETAC::DownloadMemory(const char *memory, CSymList &symlist, BETAX &x, int mode)
{
	vara list((x.bstart && x.bend) ? ExtractString(memory, x.bstart, "", x.bend) : memory, x.start);
	for (int i = 1; i < list.length(); ++i)
	{
		vars line = list[i];
		if (x.end)
		{
			int iend = line.Find(x.end);
			if (iend > 0)
				line.Delete(iend);
		}

		CSym sym;
		if (mode == D_DATA)
		{
			// RAW mode
			sym.id = line;
			Update(symlist, sym, NULL, FALSE);
			continue;
		}

		if (!DownloadLink(line, sym))
			continue;

		if (mode == D_LINK)
		{
			Update(symlist, sym, NULL, FALSE);
			continue;
		}

		printf("%d %dP %d/%d    \r", symlist.GetSize(), pages, i, list.GetSize());

		if (!DownloadInfo(list[i], sym))
			continue;

		DownloadSym(sym, symlist);
	}

	return list.GetSize();
}

vars BETAC::GetKMLIDX(DownloadFile &f, const char *url, ExtractKMLIDXF fid)
{
	vara ids(url, "kmlidx=");
	if (ids.length() > 1 && !ids[1].IsEmpty() && ids[1] != "X")
		return ids[1];

	if (DownloadRetry(f, url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return "";
	}

	return fid(f.memory);
}
