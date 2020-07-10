
#include "UKCanyonGuides.h"
#include "BETAExtract.h"
#include "rw.h"


int UKCG_DownloadPage(const char *memory, CSym &sym)
{
	vars url = sym.id;

	//http://www.canyonguides.org/guide-book/canyon-locations-map/

	double lat = InvalidNUM, lng = InvalidNUM;
	if (!GetKMLCoords(ExtractString(memory, "<coordinates", ">", "</coordinates"), lat, lng))
	{
		Log(LOGERR, "Invalid KML Start coords from %s", url);
		return FALSE;
	}
	sym.SetNum(ITEM_LAT, lat);
	sym.SetNum(ITEM_LNG, lng);

	//sym.SetStr(ITEM_SEASON, stripHTML(ExtractString(f.memory, ">Periodo", ":", "<br")));

	vars grade = stripHTML(ExtractString(memory, "Grade:", "", "<br"));
	GetSummary(sym, grade.upper().replace("-", ""));

	/*
	sym.SetNum(ITEM_RAPS, CDATA::GetNum(stripHTML(ExtractString(memory, " rappels:", "", "<br"))));
	sym.SetStr(ITEM_LONGEST, GetMetric(stripHTML(ExtractString(memory, " rappel:", "", "<br"))));
	//sym.SetStr(ITEM_LENGTH, GetMetric(stripHTML(ExtractString(f.memory, "Sviluppo:", "", "<br"))));
	sym.SetStr(ITEM_DEPTH, GetMetric(stripHTML(ExtractString(memory, "level:", "", "<br"))));
	*/

	//sym.SetStr(ITEM_SHUTTLE, stripHTML(ExtractString(memory, "Navetta:", "", "<br").replace(" ","").replace("Circa","").replace("No/","Optional ").replace(",",".")));

	vars interest = stripHTML(ExtractString(memory, "Quality:", "", "<br"));
	double stars = CDATA::GetNum(interest.replace(";", "."));
	if (stars > 0)
		sym.SetStr(ITEM_STARS, starstr(stars, 1));

	/*
	vara times;
	const char *ids[] = { "Access:", "Time:", "Return:" };
	//ASSERT( !strstr(url,"/kanion.php?id=41"));
	for (int t=0; t<sizeof(ids)/sizeof(*ids); ++t)
		{
		CString time = stripHTML(ExtractString(memory, ids[t], "", "<br"));
		while (!ExtractStringDel(time, "(", "", ")").IsEmpty());
		times.push(vars(time).replace("hours", "h").replace("days", "j").replace(";","."));
		}
	GetTotalTime(sym, times, url);
	*/

	return TRUE;
}


int UKCG_DownloadBeta(const char *ubase, CSymList &symlist)
{
	vars url = UKCGKML;
	DownloadFile f;
	if (f.Download(url))
	{
		Log(LOGERR, "Can't download base url %s", url);
		return TRUE;
	}
	if (!strstr(f.memory, "<Placemark>"))
		Log(LOGERR, "ERROR: Could not download kml %s", url);

	vara lines(f.memory, "<Placemark>");
	for (int i = 1; i < lines.length(); ++i)
	{
		vars name = stripHTML(ExtractString(lines[i], "<name", ">", "</name"));
		name.Trim(" .");

		if (name.IsEmpty())
			continue;

		CSym sym(urlstr(MkString("http://www.canyonguides.org/guide-book/canyon-locations-map?id=%s", name.replace(" ", "_"))), name);
		//sym.SetStr(ITEM_REGION, "United Kingdom");

		int stars = 0;
		vars style = ExtractString(lines[i], "<styleUrl>", "#", "</styleUrl>");
		vars styledef = ExtractString(f.memory, "<Style id='" + style + "'", ">", "</Style>");
		vars icon = ExtractString(styledef, "<Icon>", "<href>", "</href>");
		if (strstr(icon, "-yellow"))
			stars = 2;
		else if (strstr(icon, "-green"))
			stars = 3;
		else if (strstr(icon, "-blue"))
			stars = 4;
		else if (strstr(icon, "-red"))
			stars = 5;
		if (stars > 0)
			sym.SetStr(ITEM_ACA, MkString("%d;;;;;;;", stars));

		// update sym
		if (UKCG_DownloadPage(lines[i], sym))
			Update(symlist, sym, NULL, FALSE);
	}
	
	return TRUE;
}
