// Wikiloc.cpp : source file
//

#include "Wikiloc.h"
#include "BETAExtract.h"
#include "trademaster.h"
#include "passwords.h"


// ===============================================================================================

#define WIKILOCTICKS 3000
static double wikilocticks;

int WIKILOC_ExtractKML(const char *ubase, const char *url, inetdata *out, int fx)
{
	DownloadFile f;
	Throttle(wikilocticks, WIKILOCTICKS);
	if (DownloadRetry(f, url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}

	const vars user = stripHTML(ExtractString(f.memory, "\"author\":", R"("name": ")", "\","));
	CString credit = " (Data by " + user + " at " + CString(ubase) + ")";
	const vars desc = stripHTML(ExtractString(f.memory, "\"mainEntity\":", R"("name": ")", "\",")) + " " + credit;

	vara styles, waypoints, tracks;
	styles.push(  "dot=http://maps.google.com/mapfiles/kml/shapes/open-diamond.png");
	styles.push("begin=http://maps.google.com/mapfiles/kml/paddle/go.png");
	styles.push(  "end=http://maps.google.com/mapfiles/kml/paddle/red-square.png");
	
	const vars mapData = stripHTML(ExtractString(f.memory, "\"mapData\":", "[", "]"));
		
	vara wpt(mapData, "{");
	for (int i = 1; i < wpt.length(); ++i)
	{
		vars isWaypoint = ExtractString(wpt[i], "\"waypoint\"", ":", ";");

		if (isWaypoint == "true")
		{
			WIKILOC_ProcessWaypoint(&wpt[i], &waypoints, &credit, url);
		}
		else
		{
			WIKILOC_ProcessTrack(&wpt[i], &tracks, &waypoints, &credit, url);
		}		
	}

	if (waypoints.length() == 0 && tracks.length() == 0)
		return FALSE;

	// generate kml
	SaveKML("WikiLoc", desc, url, styles, waypoints, tracks, out);
	return TRUE;
}

void WIKILOC_ProcessWaypoint(vars* entry, vara* waypoints, CString* credit, const char* url)
{
	vars name = ExtractString(*entry, "\"nom\":", "\"", "\"");
	if (name.IsEmpty()) return;

	CString type = "b";	
	WIKILOC_ExtractWaypointCoords(entry, &type, "dot", &name, waypoints, credit, url);
}

void WIKILOC_ProcessTrack(vars* entry, vara* tracks, vara* waypoints, CString* credit, const char *url)
{
	CString type;  vars name;

	//begin waypoint
	type = "b"; name = "Start";
	WIKILOC_ExtractWaypointCoords(entry, &type, "begin", &name, waypoints, credit, url);

	//end waypoint
	type = "e"; name = "End";
	WIKILOC_ExtractWaypointCoords(entry, &type, "end",&name, waypoints, credit, url);

	name = ExtractString(*entry, "\"nom\":", "\"", "\"");

	vars encodedPolyline = "geom:" + ExtractString(*entry, "\"geom\":", "\"", "\"");

	//the following flow to decode the polyline string were converted from the embedded javascript on wikiloc:
	//TODO: convert the javascript to decode the line to c++. For now, just pass it off to our javascript
	vara linelist;
	linelist.push(encodedPolyline);
	
	tracks->push(KMLLine(name, *credit, linelist, OTHER, 3));

	//luca's original code to add coordinates:
		
	/*vara linelist;
	vara lats(ExtractString(f.memory, "lat:", "[", "]"));
	vara lngs(ExtractString(f.memory, "lng:", "[", "]"));
	for (int i = 0; i < lats.length() && i < lngs.length(); ++i)
	{
		double lat = CDATA::GetNum(lats[i]);
		double lng = CDATA::GetNum(lngs[i]);
		linelist.push(CCoord3(lat, lng));
	}	
	tracks->push(KMLLine("Track", name, linelist, OTHER, 3));*/
}

void WIKILOC_ExtractWaypointCoords(vars* entry, CString* type, const char* style, vars* name, vara* waypoints, CString* credit, const char *url)
{
	const vars latStr = ExtractString(*entry, "\"" + *type + "lat\"", ":", ";");
	const vars lngStr = ExtractString(*entry, "\"" + *type + "lng\"", ":", ";");

	if (latStr.IsEmpty() || lngStr.IsEmpty()) return;

	const double lat = CDATA::GetNum(latStr);
	const double lng = CDATA::GetNum(lngStr);

	if (!CheckLL(lat, lng, url)) return;

	waypoints->push(KMLMarker(style, lat, lng, *name, *credit));
}

int WIKILOC_DownloadPage(DownloadFile &f, const char *url, CSym &sym)
{
	if (strstr(url, "imgServer"))
	{
		Log(LOGERR, "ERROR wikiloc.com ImgServer reference: %s", url);
		return FALSE;
	}
	if (f.Download(url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}

	vara coords(ExtractString(f.memory, "maps.LatLng(", "", ")"));
	if (coords.length() > 1)
	{
		sym.SetStr(ITEM_LAT, coords[0]);
		sym.SetStr(ITEM_LNG, coords[1]);
	}
	/*
		vara rating(ExtractString(f.memory, "title=\"Rated:","","\""), "(");
		double stars = 0, num = 0;
		if (rating.length()>1)
			{
			stars = CDATA::GetNum(rating[0]);
			num = CDATA::GetNum(rating[1]);
			sym.SetStr(ITEM_STARS, starstr(stars, num));
			}
	*/
	sym.SetStr(ITEM_KML, "X");

	vara wpt(f.memory, "maps.LatLng(");
	vars description = stripHTML(ExtractString(f.memory, "class=\"description\"", ">", "</div"));
	description.Replace(sym.GetStr(ITEM_DESC), "");
	int n = wpt.length() - 2;
	int len = description.length();
	int score = n * 100 + len;
	vars desc = MkString("%05d:%db+%dwpt", score, len, n);
	if (score < 300) sym.SetStr(ITEM_CLASS, "0:" + desc);
	else sym.SetStr(ITEM_CLASS, "1:" + desc);

	return TRUE;
}

/*
int WIKI_DownloadBeta(const char *ubase, CSymList &symlist, int (*DownloadPage)(DownloadFile &f, const char *url, CSym &sym))
{
	int n = 0, step = 20;
	DownloadFile f;
	int from=0;
	vars url = "http://www.wikiloc.com/trails/canyoneering";
	while (!url.IsEmpty())
	{
		if (f.Download(url))
			{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			return FALSE;
			}
		url = ExtractLink(f.memory, ">Next&nbsp;&rarr;");
		double count = ExtractNum(f.memory, "&nbsp;of&nbsp;", "", "&nbsp;");
		if (count<=0 || count>10000)
			{
			Log(LOGERR, "Bad count %s!", CData(count));
			return FALSE;
			}

		vara list(f.memory, "wikiloc/view.do?id=");
		for (int i=1; i<list.length(); ++i)
			{
			vars id = ExtractString(list[i], "", "", "\"").Trim(" \"'");
			if (id.IsEmpty())
				continue;

			vars name = stripHTML(ExtractString(list[i], ">", "", "<"));
			vars loc = stripHTML(ExtractString(list[i], "\"near\"", "\"", "\""));
			double lat = ExtractNum(list[i], "\"lat\"", ":", ",");
			double lng = ExtractNum(list[i], "\"lon\"", ":", ",");
			if (!CheckLL(lat,lng))
				{
				Log(LOGERR, "Invalid WIKILOC coords for %s", list[i]);
				continue;
				}

			vars link = "http://wikiloc.com/wikiloc/view.do?id="+id;

			++n;
			CSym sym(urlstr(link), name);
			vars country = stripHTML(ExtractString(loc, "(", "", ")"));
			vars region = invertregion( GetToken(loc, 0, '('), country + " @ ");
			sym.SetNum(ITEM_LAT, lat);
			sym.SetNum(ITEM_LNG, lng);
			sym.SetStr(ITEM_REGION, region);
			sym.SetStr(ITEM_KML, "X");
			vara rated(name,"*");
			double stars = rated[0][strlen(rated[0])-1]-(double)'0';
			if (stars>=1 && stars<=5)
				sym.SetStr(ITEM_STARS, starstr(stars, 1));

			printf("%d %d/%d %d/%d/%d    \r", symlist.GetSize(), i, list.length(), from, n, (int)count);
			if (!UpdateCheck(symlist, sym) && MODE>-2)
				continue;

			if (DownloadPage(f, sym.id, sym))
				Update(symlist, sym, NULL, FALSE);
			}

		// next
		from += step;
		if (from>count)
			break;
	}

	return TRUE;
}

*/

int WIKI_DownloadBeta(const char *ubase, CSymList &symlist, int(*DownloadPage)(DownloadFile &f, const char *url, CSym &sym))
{
	int n = 0, step = 20;
	DownloadFile f;
	if (f.Download("http://www.wikiloc.com/wikiloc/start.do"))
	{
		Log(LOGERR, "ERROR: can't login in wikiloc");
		return FALSE;
	}
	f.GetForm();
	f.SetFormValue("email", WIKILOC_EMAIL);
	f.SetFormValue("password", WIKILOC_PASSWORD);
	if (f.SubmitForm())
	{
		Log(LOGERR, "ERROR: can't login in wikiloc");
		return FALSE;
	}

	int from = 0;
	while (true)
	{
		//vars url = "https://www.wikiloc.com/wikiloc/find.do?act=46%2C&uom=mi&s=id"+MkString("&from=%d&to=%d", from, from+step); 
		vars url = "https://www.wikiloc.com/wikiloc/find.do?a=46&z=2&sw=-90%2C-180&ne=90%2C180" + MkString("&from=%d&to=%d", from, from + step);
		//vars url = https://www.wikiloc.com/wikiloc/find.do?a=46&z=2&from=0&to=15&sw=-90%2C-180&ne=90%2C180
		if (f.Download(url)) // sw=-90%%2C-180&ne=90%%2C180&tr=0 https://www.wikiloc.com/wikiloc/find.do?act=46%2C&uom=mi&from=10&to=20
		{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			return FALSE;
		}

		//double count = ExtractNum(f.memory, "&nbsp;of&nbsp;", "", "</a");
		double count = ExtractNum(f.memory, "count", ":", ",");
		if (count <= 0 || count > 10000)
		{
			Log(LOGERR, "Count too big %s>10K", CData(count));
			return FALSE;
		}

		vara mem(f.memory, "\"tiles\"");
		vara list(mem.length() > 0 ? mem[0] : f.memory, "{\"id\"");
		for (int i = 1; i < list.length(); ++i)
		{
			vars id = ExtractString(list[i], "", ":", ",").Trim(" \"'");
			if (id.IsEmpty())
				continue;

			vars name = stripHTML(ExtractString(list[i], "\"name\"", "\"", "\""));
			vars loc = stripHTML(ExtractString(list[i], "\"near\"", "\"", "\""));
			double lat = ExtractNum(list[i], "\"lat\"", ":", ",");
			double lng = ExtractNum(list[i], "\"lon\"", ":", ",");
			if (!CheckLL(lat, lng))
			{
				Log(LOGERR, "Invalid WIKILOC coords for %s", list[i]);
				continue;
			}

			vars link = "http://wikiloc.com/wikiloc/view.do?id=" + id;

			++n;
			CSym sym(urlstr(link), name);
			vars country = stripHTML(ExtractString(loc, "(", "", ")"));
			vars region = invertregion(GetToken(loc, 0, '('), country + " @ ");
			sym.SetNum(ITEM_LAT, lat);
			sym.SetNum(ITEM_LNG, lng);
			sym.SetStr(ITEM_REGION, region);
			sym.SetStr(ITEM_KML, "X");
			vara rated(name, "*");
			double stars = rated[0][strlen(rated[0]) - 1] - (double)'0';
			if (stars >= 1 && stars <= 5)
				sym.SetStr(ITEM_STARS, starstr(stars, 1));

			printf("%d %d/%d %d/%d/%d    \r", symlist.GetSize(), i, list.length(), from, n, (int)count);
			if (!UpdateCheck(symlist, sym) && MODE > -2)
				continue;

			if (DownloadPage(f, sym.id, sym))
				Update(symlist, sym, NULL, FALSE);
		}

		// next
		from += step;
		if (from > count)
			break;
	}

	return TRUE;
}

int WIKILOC_DownloadBeta(const char *ubase, CSymList &symlist)
{
	/*
	if (MODE==-2)
		{
		//TRENCANOUS_DownloadBeta(ubase, symlist, WIKILOC_DownloadPage);
		BARRANQUISMO_DownloadBeta(ubase, symlist, WIKILOC_DownloadPage);
		}
	*/
	WIKI_DownloadBeta(ubase, symlist, WIKILOC_DownloadPage);
	return TRUE;
}

