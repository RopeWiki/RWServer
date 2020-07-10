// DescenteCanyon.cpp : source file
//

#include "DescenteCanyon.h"
#include "BETAExtract.h"
#include "trademaster.h"

int DESCENTECANYON_DownloadPage2(DownloadFile &f, const char *urlc, CSym &sym)
{
	if (f.Download(urlc))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", urlc);
		return FALSE;
	}

	vars season = ExtractString(f.memory, "aquatique</h2>", "", "</div");
	season = stripHTML(ExtractString(season, "", "", "<h2"));
	if (!season.IsEmpty())
		sym.SetStr(ITEM_SEASON, season);

	vars rock = noHTML(ExtractString(strstr(f.memory, "roche.gif"), "<td", ">", "</td"));
	sym.SetStr(ITEM_ROCK, rock);

	return TRUE;
}

double DESCENTECANYON_GetDate(const char *date)
{
	static CSymList months;
	if (months.GetSize() == 0)
	{
		vara monthsa(UTF8("Jan*=Jan,Fév*=Feb,Mars=Mar,Avr*=Apr,Mai=May,Juin*=Jun,Juil*=Jul,Aoû*=Aug,Sep*=Sep,Oct*=Oct,Nov*=Nov,Déc*=Dec"));
		for (int i = 0; i < monthsa.length(); ++i)
			months.Add(CSym(GetToken(monthsa[i], 0, '='), GetToken(monthsa[i], 1, '=')));
	}
	vars sdate = Translate(date, months);
	if (CGetNum(sdate) <= 0) return 0;
	return CDATA::GetDate(sdate, "DD MMM YY");
}

int DESCENTECANYON_ExtractPoints(const char *url, vara &styles, vara &points, vara &lines, const char *ubase)
{
	static vara labels(
		"parking_aval,"
		"parking_amont,"
		"parking,"
		"depart,"
		"arrivee,"
		"point_interne,"
		"point_externe,", ",");
	static vara enlabels(
		"Parking Exit,"
		"Parking Approach,"
		"Parking,"
		"Start,"
		"End,"
		"Waypoint Descent,"
		"Waypoint Approach/Exit,", ",");
	static vara icons(
		"http://maps.google.com/mapfiles/kml/pal4/icon15.png,"
		"http://maps.google.com/mapfiles/kml/pal4/icon62.png,"
		"http://maps.google.com/mapfiles/kml/pal4/icon62.png,"
		"http://www.descente-canyon.com/design/img/map-markers/marker-canyon-depart.png,"
		"http://www.descente-canyon.com/design/img/map-markers/marker-canyon-arrivee.png,"
		"http://www.descente-canyon.com/design/img/map-markers/marker-canyon-point_interne.png,"
		"http://www.descente-canyon.com/design/img/map-markers/marker-canyon-point_externe.png,", ",");

	if (icons.length() != labels.length()) {
		if (ubase) Log(LOGERR, "ERROR: DESCENTECANYON_Extract #icons!=#labels");
		return FALSE;
	}

	DownloadFile f;

	// get id from url
	double code = InvalidNUM;
	vara urla(vars(url), "/");
	for (int i = urla.length() - 1; i > 0 && code == InvalidNUM; --i)
		code = CDATA::GetNum(urla[i]);

	if (code == InvalidNUM)
	{
		if (ubase) Log(LOGERR, "ERROR: can't get descente-canyon code for url %.128s", url);
		return FALSE;
	}

	vars id = CData(code);

	CString url2 = "http://www.descente-canyon.com/canyoning/canyon-carte/" + id + "/carte.html";
	if (DownloadRetry(f, url2))
	{
		if (ubase) Log(LOGERR, "ERROR: can't download url %.128s", url2);
		return FALSE;
	}

	vara markers(f.memory, "maps.LatLng(");
	for (int i = 1; i < markers.length(); ++i) {
		const char *data = markers[i];
		double lat = CDATA::GetNum(strval(data, "", ","));
		double lng = CDATA::GetNum(strval(data, ",", ")"));
		if (!CheckLL(lat, lng))
		{
			if (ubase) Log(LOGERR, "Invalid descent-canyon marker lat/lng marker='%s'", markers[i]);
			continue;
		}
		CString user = strval(data, "</div><b>", "</b>");
		CString namedesc = strval(data, "remarque: '", "'");
		CString date = strval(data, "date: '", "'");
		CString label = strval(data, "type: '", "'");
		int l = labels.indexOf(label);
		if (l < 0)
		{
			if (ubase) Log(LOGERR, "Invalid descent-canyon marker name/label marker='%s'", markers[i]);
			continue;
		}

		CString credit;
		if (ubase)
			credit = CDATAS + namedesc + " (Data by " + user + " on " + date + ")" + CDATAE;
		points.push(KMLMarker(label, lat, lng, enlabels[l], credit));
	}

	for (int i = 0; i < labels.length(); ++i)
		styles.push(labels[i] + "=" + icons[i]);

	return TRUE;
}

int DESCENTECANYON_DownloadLatLng(CSym &sym)
{
	// lat lng (in case not reported properly)
	if (sym.GetNum(ITEM_LAT) == InvalidNUM || sym.GetNum(ITEM_LNG) == InvalidNUM)
	{
		vara styles, points, lines;
		if (DESCENTECANYON_ExtractPoints(sym.id, styles, points, lines, NULL))
		{
			double xlat = InvalidNUM, xlng = InvalidNUM;
			for (int i = 0; i < points.length(); ++i)
			{
				vars name = ExtractString(points[i], "<name>", "", "</name>");
				vars coords = ExtractString(points[i], "<coordinates>", "", "</coordinates>");
				double lat = CGetNum(GetToken(coords, 1));
				double lng = CGetNum(GetToken(coords, 0));
				if (!CheckLL(lat, lng))
					continue;
				xlat = lat; xlng = lng;
				if (name == "Start")
					break;
			}
			if (xlat != InvalidNUM && xlng != InvalidNUM)
			{
				Log(LOGWARN, "Salvaged lat,lng for %s %s", sym.GetStr(ITEM_DESC), sym.id);
				sym.SetNum(ITEM_LAT, xlat);
				sym.SetNum(ITEM_LNG, xlng);
				sym.SetStr(ITEM_KML, "X");
				return TRUE;
			}
		}
	}
	return FALSE;
}

int DESCENTECANYON_DownloadPage(DownloadFile &f, const char *urlc, CSym &sym, BOOL condonly)
{
	if (f.Download(urlc))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", urlc);
		return FALSE;
	}

	//ASSERT(sym.id!="http://descente-canyon.com/canyoning/canyon-description/21850");

	// cond
	vars condstr = noHTML(ExtractString(f.memory, "Dernier d", "", "<br"));
	vars conddate = stripHTML(ExtractString(condstr, "Le ", "", ":"));
	vars condwater = stripHTML(ExtractString(condstr, "Le ", ":", "("));
	if (!conddate.IsEmpty())
	{
		double date = DESCENTECANYON_GetDate(conddate);
		if (date <= 0)
			Log(LOGERR, "Intelligeble date %s : %s", sym.id, condstr);
		else
		{
			vara cond; cond.SetSize(COND_MAX);
			cond[COND_DATE] = CDate(date);
			//enum { W_DRY=0, W_WADING=1, W_SWIMMING=2, W_VERYLOW=3, W_LOW=4, W_MODLOW=5, W_MODERATE=6, W_MODHIGH=7, W_HIGH=8, W_VERYHIGH=9, W_EXTREME=10 };
			static vara waterdc(UTF8("Sec=0,Petit filet d'eau=3,Débit correct=6,Gros débit=8,Très gros débit=9,Trop d'eau=10"));
			cond[COND_WATER] = TableMatch(condwater, waterdc, cond_water);
			cond[COND_LINK] = MkString("http://descente-canyon.com/canyoning/canyon-debit/%s/observations.html", vara(sym.id, "/").last());

			sym.SetStr(ITEM_CONDDATE, cond.join(";"));
		}
		if (condonly)
			return TRUE;
	}

	// properties 
	sym.SetStr(ITEM_LONGEST, noHTML(ExtractString(f.memory, "alt=\"Corde\"", "</td>", "</td")));
	GetSummary(sym, noHTML(ExtractString(f.memory, "alt=\"Cot\"", "</td>", "</td")));
	sym.SetStr(ITEM_LENGTH, noHTML(ExtractString(f.memory, "alt=\"Long\"", "</td>", "</td")));
	sym.SetStr(ITEM_DEPTH, noHTML(ExtractString(f.memory, "alt=\"Deniv\"", "</td>", "</td")));

	vars region = noHTML(ExtractString(f.memory, "Situation", ":", "<br"));
	vars city = noHTML(ExtractString(f.memory, "Commune", ":", "<br"));
	sym.SetStr(ITEM_REGION, region);
	if (sym.GetNum(ITEM_LAT) == InvalidNUM)
	{
		sym.SetStr(ITEM_LAT, "@" + invertregion(sym.GetStr(ITEM_REGION), city));
		//sym.SetStr(ITEM_LNG, "X" );
	}

	vars shuttle = noHTML(ExtractString(f.memory, "alt=\"Nav\"", "</td>", "</td"));
	if (shuttle[0] == 'n') shuttle = "No";
	sym.SetStr(ITEM_SHUTTLE, shuttle);

	// set stars //érêt\"
	vars stars = noHTML(ExtractString(f.memory, "alt=\"int", ">", "votes"));
	vara starsa(stars, "(");
	if (starsa.length() >= 2) {
		double s = CDATA::GetNum(starsa[0]);
		double c = CDATA::GetNum(starsa[1]);
		if (s == InvalidNUM || c == InvalidNUM) {
			if (!IsSimilar(stars, "Pas d'infos")) // no info
				Log(LOGERR, "Bad star vote %.50s for %.128s", stars, urlc);
		}
		else
			sym.SetStr(ITEM_STARS, CData(s + 1) + "*" + CData(c));
	}

	sym.SetStr(ITEM_AKA, sym.GetStr(ITEM_DESC) + ";" + noHTML(ExtractString(f.memory, "<h1", "<h1>", "</h1>")));

	// time
	vara times;
	const char *ids[] = { "Appr", "Desc", "Ret" };
	for (int t = 0; t < sizeof(ids) / sizeof(*ids); ++t)
		times.push(noHTML(ExtractString(f.memory, MkString("alt=\"%s\"", ids[t]), "</td>", "</td")));
	GetTotalTime(sym, times, urlc, -1);


	return TRUE;
}

int DESCENTECANYON_DownloadBeta(const char *ubase, CSymList &symlist)
{

	vara recentlist, urllist;
	const char *canyonlink = "href=\"/canyoning/canyon/";
	const char *canyondesclink = "/canyoning/canyon-description/";

	DownloadFile f;
	CString dc = "http://www.descente-canyon.com";

	// download list of recent additions
	{
		CString url = dc + "/canyoning/suivi-fichescanyons";
		if (f.Download(url))
		{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			return FALSE;
		}
		vara list(f.memory, canyonlink);
		for (int i = 0; i < list.length(); ++i)
		{
			CString id = ExtractString(f.memory, "href=", "/2", "/");
			if (!id.IsEmpty())
				recentlist.push(id);
		}
		recentlist.sort();
	}


	CString url = dc + "/canyoning/";
	if (f.Download(url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}

	// tables for ratings
	const char *sep = "/canyoning/lieu/13";
	vara list(f.memory, sep);
	for (int i = 1; i < list.length(); ++i) {
		CString url = dc + sep + GetToken(list[i], 0, '\"');
		if (urllist.indexOf(url) >= 0)
			continue;
		urllist.push(url);
		urllist.sort();
		if (f.Download(url))
		{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			continue;
		}

		// process markers (located canyons)
		vara markersidx; markersidx.push("");
		vara markers(ExtractString(f.memory, "", "canyonMarkers=[", "];"), "{\"i\":");
		for (int m = 1; m < markers.length(); ++m)
			markersidx.push(ExtractString(markers[m], "", "\"", "\""));

		const char *sep2 = canyonlink;
		vara canyons(f.memory, sep2);
		for (int c = 1; c < canyons.length(); ++c) {
			vars canyon = canyons[c];
			if (canyon[0] != '2')
				Log(LOGERR, "2 missing from %s%s", sep2, canyon);
			vars endlink = GetToken(GetToken(canyon, 0, '\"'), 0, "/");
			vars link = dc + canyondesclink + endlink;
			//vars linkinfo = dc+"/canyoning/canyon/"+endlink;
			vars name = ExtractString(canyon, "", "<b>", "</b>").Trim();

			// set marker
			vars marker = "";
			vars id = ExtractString(link, "", "/2", "/");
			if (id.IsEmpty())
			{
				Log(LOGERR, "Invalid id %s", id);
				continue;
			}
			int m = markersidx.indexOf(id);
			if (m >= 0) {
				marker = markers[m];
				markersidx[m] = ""; //done!
			}

			// process marker (only located canyons)			
			if (name.IsEmpty())
				name = ExtractString(marker, "\"n\":", "\"", "\"").Trim();
			//vars uname = ExtractString(marker, "\"u\":", "\"", "\"");
			double lat = ExtractNum(marker, "\"la\":", "", ",");
			double lng = ExtractNum(marker, "\"ln\":", "", ",");
			if (!CheckLL(lat, lng))
			{
				if (MODE >= 0)
					Log(LOGWARN, "Invalid ll %s from %s", name, url);
				lat = lng = InvalidNUM;
				//continue
				// allow invalid coordinates!
			}

			vars symbol = ExtractString(marker, "\"s\":", "\"", "\"").Trim();
			//double stars = ExtractNum(marker, "\"it\":", "\"", "\"").Trim();
			vars summary = ExtractString(marker, "\"cot\":", "\"", "\"").Trim();
			vars er = ExtractString(marker, "\"er\":", "\"", "\"").Trim();
			//"ii":"","ed":"1","ep":"1","eb":"1",
			vars cond = ExtractString(marker, "\"dm\":", "\"", "\"").Trim();

			// set sym
			CSym sym(urlstr(link));
			name.Replace(",", ";");
			sym.SetStr(ITEM_DESC, name);
			GetSummary(sym, summary);
			sym.SetNum(ITEM_LAT, lat);
			sym.SetNum(ITEM_LNG, lng);
			if (lat != InvalidNUM && lng != InvalidNUM)
				sym.SetStr(ITEM_KML, "X");
			if (symbol == "exclam")
				sym.SetStr(ITEM_PERMIT, "Closed");
			if (er == "1")
				sym.SetStr(ITEM_PERMIT, "Yes");

			// lat,lng
			DESCENTECANYON_DownloadLatLng(sym);

			// latest conditions
			if (!cond.IsEmpty() && cond != "??")
			{
				double date = DESCENTECANYON_GetDate(cond);
				vars ncond = CDate(date);
				if (ncond.IsEmpty())
					Log(LOGERR, "Intelligeble date %s : %s", name, cond);
				else
					sym.SetStr(ITEM_CONDDATE, ncond);
			}

			// update changes
			if (!Update(symlist, sym) && MODE >= -1)
				if (recentlist.indexOf(id) < 0)
					continue;

			// download more information (for located and unlocated canyons)
			DESCENTECANYON_DownloadPage(f, "http://www.descente-canyon.com/canyoning/canyon/2" + id, sym);
			DESCENTECANYON_DownloadPage2(f, sym.id, sym); //"http://www.descente-canyon.com/canyoning/canyon-description/2"+id+"/topo.html", sym);
			Update(symlist, sym, NULL, FALSE);
		}

		// check consistency
		for (int m = 0; m < markersidx.length(); ++m)
			if (!markersidx[m].IsEmpty())
				Log(LOGERR, "Unused marker id %s from %s", markersidx[m], url);

		// update
		printf("Downloading %d/%d = %d ...\r", i, list.length(), symlist.GetSize());
		symlist.Save(filename("dcnew"));
	}

	return TRUE;
}

int DESCENTECANYON_DownloadConditions(const char *ubase, CSymList &symlist)
{
	DownloadFile f;
	CString url = "http://www.descente-canyon.com/derniers_debits_canyon";
	if (f.Download(url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}
	vara list(f.memory, "/canyon-debit/");
	for (int i = 1; i < list.length(); ++i)
	{
		vars id = ExtractString(list[i], "", "", "/");
		CSym sym(urlstr("http://descente-canyon.com/canyoning/canyon-description/" + id));
		if (DESCENTECANYON_DownloadPage(f, "http://www.descente-canyon.com/canyoning/canyon/" + id, sym, TRUE))
			UpdateCond(symlist, sym, NULL, FALSE);
	}
	return TRUE;
}

int DESCENTECANYON_ExtractKML(const char *ubase, const char *url, inetdata *out, int fx)
{
	CString credit = " (Data by " + CString(ubase) + ")";

	DownloadFile f;

	vara styles, points, lines;
	if (!DESCENTECANYON_ExtractPoints(url, styles, points, lines, ubase))
		return FALSE;

	// generate kml
	SaveKML(ubase, credit, url, styles, points, lines, out);
	return TRUE;
}
