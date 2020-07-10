#include "SwissCanyon.h"
#include "BETAExtract.h"
#include "trademaster.h"


int SWISSCANYON_GetSummary(CSym &sym, const char *summary)
{
	while (*summary != 0 && !isanum(*summary))
		++summary;

	//ASSERT(!strstr(sym.data, "Gribbiasca")); 

	if (tolower(summary[0]) == 'v')
		return GetSummary(sym, summary);

	if (!isdigit(summary[0]))
		return FALSE;

	vara slist2(",,,"), slist(summary, ";");
	int len = strlen(summary);
	for (int s = 0; s < len; ++s)
	{
		if (!isdigit(summary[s]))
		{
			if (summary[s + 1] == 'I' || summary[s + 1] == 'V')
			{
				slist2[2] = GetToken(summary + s, 0, " ;,/.abc");
				s = len;
			}
		}
		else
			switch (summary[s + 1])
			{
			case 'a':
				slist2[1] = vars("a") + summary[s++];
				break;
			case 'b':
				slist2[0] = vars("v") + summary[s++];
				break;
			case 'c':
				++s;
				break;
			}
	}

	return GetSummary(sym, slist2.join(" "));
}

int SWISSCANYON_DownloadPage(DownloadFile &f, CSym &sym)
{
	if (f.Download(sym.id))
	{
		Log(LOGERR, "Can't download page url %s", sym.id);
		return FALSE;
	}

	vars memory(f.memory);
	memory.Replace("<TD", "<td");
	memory.Replace("</TD", "</td");
	memory.Replace("<TR", "<tr");
	memory.Replace("<TABLE", "<table");
	memory.Replace("<BR", "<br");

	// coordinates
	vars startp = stripHTML(ExtractString(strstr(memory, "Partenza<"), "<td", ">", "</td>"));
	startp.Replace(".", "");
	vara div(startp, "/");
	if (div.length() >= 2)
	{
		double east = CDATA::GetNum(div[0].trim().Right(6));
		double north = CDATA::GetNum(div[1].trim().Left(6));
		double lat = CHtoWGSlat(east, north);
		double lng = CHtoWGSlng(east, north);
		if (CheckLL(lat, lng))
		{
			sym.SetNum(ITEM_LAT, lat);
			sym.SetNum(ITEM_LNG, lng);
		}
	}

	// best time
	vars season = UTF8(stripHTML(ExtractString(strstr(memory, "riode id"), "<td", ">", "</td>")));
	season.Replace("&#150;", "-");
	if (!season.IsEmpty())
		sym.SetStr(ITEM_SEASON, season);

	// shuttle
	vars shuttle = UTF8(stripHTML(ExtractString(strstr(memory, ">Navett"), "<td", ">", "</td>")));
	if (!shuttle.IsEmpty())
	{
		int f = shuttle.Find("min");
		if (f > 0) shuttle = shuttle.Mid(0, f + 3);
		shuttle.Replace("Pas", "No");
		shuttle.Replace("(No indispensable)", "Optional");
		if (IsSimilar(shuttle, "min"))
			shuttle = "";
		sym.SetStr(ITEM_SHUTTLE, shuttle);
	}

	sym.SetStr(ITEM_DEPTH, GetMetric(stripHTML(ExtractString(strstr(memory, "Dislivello<"), "<td", ">", "</td>"))));
	sym.SetStr(ITEM_LENGTH, GetMetric(stripHTML(ExtractString(strstr(memory, "Lunghezza<"), "<td", ">", "</td>"))));

	// summary
	vars summary = UTF8(stripHTML(ExtractString(strstr(memory, ">Difficolt"), "<td", ">", "</td>")));
	SWISSCANYON_GetSummary(sym, summary);

	return TRUE;
}

int SWISSCANYON_DownloadTable(const char *url, const char *base, CSymList &symlist)
{
	DownloadFile f;
	if (f.Download(url))
	{
		Log(LOGERR, "Can't download base url %s", url);
		return FALSE;
	}
	vars memory(f.memory);
	memory.Replace("<TD", "<td");
	memory.Replace("</TD", "</td");
	memory.Replace("<TR", "<tr");
	memory.Replace("<TABLE", "<table");
	memory.Replace("<BR", "<br");
	vara tables(memory, "teschio");
	for (int t = 1; t < tables.length(); ++t)
	{
		vars table = ExtractString(tables[t], "", "", "</table");
		vara list(table, "<tr");
		for (int i = 1; i < list.length(); ++i)
		{
			vars &line = list[i];
			vara td(line, "<td");
			vars &td1 = td[1];
			vars link = ExtractString(td[1], "href", "\"", "\"");
			vars name = UTF8(stripHTML(ExtractString(td[1], ">", "", "<td").split("<br").first())).trim();
			// fake url
			BOOL fake = link.IsEmpty();
			if (fake)
				link = url + vars("?") + url_encode(name);
			else
				link = burl(base, link);

			CSym sym(urlstr(link), name);

			vars region = UTF8(stripHTML(ExtractString(td[2], ">", "", "</td")));
			sym.SetStr(ITEM_REGION, "Switzerland;" + region);
			sym.SetStr(ITEM_LAT, "@" + region + ";Switzerland");

			double stars = ExtractNum(td[4], "stella-", "", ".");
			double smiles = ExtractNum(td[5], "smile-mini-", "", ".");
			if (smiles > stars)
				stars = smiles;
			if (stars > 0)
				sym.SetStr(ITEM_STARS, starstr(stars + 1, 1));

			vars summary = stripHTML(ExtractString(td[6], ">", "", "</td"));
			SWISSCANYON_GetSummary(sym, summary);

			if (Update(symlist, sym, NULL, FALSE) || MODE <= -2)
				if (!fake)
				{
					SWISSCANYON_DownloadPage(f, sym);
					Update(symlist, sym, NULL, FALSE);
				}
		}
	}
	return TRUE;
}

int SWISSCANYON_DownloadBeta(const char *ubase, CSymList &symlist)
{
	vara regions;
	DownloadFile f;
	const char *urls[] = { "http://www.swisscanyon.ch/canyons/lista-ti.htm", "http://www.swisscanyon.ch/canyons/lista-ch-west.htm", NULL };
	for (int u = 0; urls[u] != NULL; ++u)
		SWISSCANYON_DownloadTable(urls[u], "swisscanyon.ch/canyons", symlist);
	return TRUE;
}

int SWESTCANYON_DownloadBeta(const char *ubase, CSymList &symlist)
{
	vara regions;
	DownloadFile f;
	const char *urls[] = { "http://www.swestcanyon.ch/www_swisscanyon_ch_fichiers/lista-ch-west.htm",  NULL };
	for (int u = 0; urls[u] != NULL; ++u)
	{
		// download base
		SWISSCANYON_DownloadTable(urls[u], ubase, symlist);
	}
	return TRUE;
}

// Convert CH y/x to WGS lat
double CHtoWGSlat(double y, double x)
{
	// Converts military to civil and  to unit = 1000km
	// Auxiliary values (% Bern)
	double y_aux = (y - 600000.0) / 1000000.0;
	double x_aux = (x - 200000.0) / 1000000.0;

	// Process lat
	double lat = 16.9023892
		+ 3.238272 * x_aux
		- 0.270978 * pow(y_aux, 2)
		- 0.002528 * pow(x_aux, 2)
		- 0.0447 * pow(y_aux, 2) * x_aux
		- 0.0140 * pow(x_aux, 3);

	// Unit 10000" to 1 " and converts seconds to degrees (dec)
	lat = lat * 100 / 36;

	return lat;
}

// Convert CH y/x to WGS long
double CHtoWGSlng(double y, double x)
{
	// Converts military to civil and  to unit = 1000km
	// Auxiliary values (% Bern)
	double y_aux = (y - 600000.0) / 1000000.0;
	double x_aux = (x - 200000.0) / 1000000.0;

	// Process long
	double lng = 2.6779094
		+ 4.728982 * y_aux
		+ 0.791484 * y_aux * x_aux
		+ 0.1306 * y_aux * pow(x_aux, 2)
		- 0.0436 * pow(y_aux, 3);

	// Unit 10000" to 1 " and converts seconds to degrees (dec)
	lng = lng * 100.0 / 36.0;

	return lng;
}

