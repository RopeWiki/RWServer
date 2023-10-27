#include "BetaC.h"
#include "BETAExtract.h"
#include "Trademaster.h"
#include "ShortBetaSitesUnclassed.h"


// ===============================================================================================

int CCOLLECTIVE_ExtractKML(const char *ubase, const char *url, inetdata *out, int fx)
{
	DownloadFile f;
	vars id = GetKMLIDX(f, url, "KmlLayer(", "", ")");
	id.Trim(" '\"");
	if (id.IsEmpty())
		return FALSE; // not available

	// fix issues with bad link
	id.Replace("&amp;", "&");
	int find = id.Find("http", 5);
	if (find > 0) id = id.Mid(find);
	vars msid = ExtractString(id, "msid=", "", "&");

	CString credit = " (Data by " + CString(ubase) + ")";
	if (msid.IsEmpty())
		KMZ_ExtractKML(credit, id, out);
	else
		KMZ_ExtractKML(credit, "http://maps.google.com/maps/ms?ie=UTF8&hl=en&msa=0&output=kml&msid=" + msid, out);
	
	return TRUE;
}

int CCOLLECTIVE_DownloadPage(DownloadFile &f, const char *url, CSym &sym)
{
	// get full stats
	if (f.Download(url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}
	
	vars stime = stripHTML(ExtractString(f.memory, "Time Commitment:", "<dd>", "</dd"));
	CDoubleArrayList time;
	if (GetValues(stime, utime, time) || GetValues(stime, NULL, time)) {
		sym.SetNum(ITEM_MINTIME, time.Head());
		sym.SetNum(ITEM_MAXTIME, time.Tail());
	}

	//vars area = stripHTML(ExtractString(f.memory, ">Area:", "<dd>", "</dd"));
	//sym.SetStr(ITEM_REGION, reg + ";" + area);
	vara reg;
	int betabase = FALSE;
	vara crumbs(ExtractString(f.memory, "\"breadcrumb\"", "", "</field"), "\"crust");
	for (int i = 1; i < crumbs.length(); ++i)
	{
		vars str = stripHTML(ExtractString(crumbs[i], "href=", ">", "</a"));
		if (betabase)
			reg.push(str);
		if (str == "Betabase")
			betabase = TRUE;
	}
	sym.SetStr(ITEM_REGION, reg.join(";"));
	
	vars season = stripHTML(ExtractString(f.memory, ">Best Seasons:", "<dd>", "</dd"));
	sym.SetStr(ITEM_SEASON, season.replace("Mid-", "").replace("Year", "All year"));

	vars vehicle = stripHTML(ExtractString(f.memory, ">Vehicle:", "<dd>", "</dd"));
	SetVehicle(sym, vehicle);

	//vars shuttle = stripHTML(ExtractString(f.memory, ">Shuttle:", "<dd>", "</dd"));
	//sym.SetStr(ITEM_SHUTTLE, shuttle);

	vars coord = stripHTML(ExtractString(f.memory, ">Coordinates:", "<dd>", "</dd"));
	double lat = CDATA::GetNum(GetToken(coord, 0, ';'));
	double lng = CDATA::GetNum(GetToken(coord, 1, ';'));
	if (CheckLL(lat, lng))
	{
		sym.SetNum(ITEM_LAT, lat);
		sym.SetNum(ITEM_LNG, lng);
	}

	// kml
	vars kmlidx = ExtractString(f.memory, "KmlLayer(", "", ")").Trim("'\" ");
	if (!kmlidx.IsEmpty())
		sym.SetStr(ITEM_KML, "X");

	vara conds(f.memory, "/calendar.png");
	if (conds.length() > 1)
	{
		const char *memory = conds[1];
		vars date = stripHTML(ExtractString(memory, "", ">", "</span"));
		double vdate = CDATA::GetDate(date, "MMM D; YYYY");
		if (vdate > 0)
		{
			//enum { W_DRY=0, W_WADING=1, W_SWIMMING=2, W_VERYLOW=3, W_LOW=4, W_MODLOW=5, W_MODERATE=6, W_MODHIGH=7, W_HIGH=8, W_VERYHIGH=9, W_EXTREME=10 };
			static vara watercc("NA=/,Dry=0,Muddy=0,Shallow=1,Deep=2,Swimming=2,Light=4,Moderate=6,Heavy flow=8,Extreme flow=10");

			//static vara cond_temp("0 - None,1 - Rain jacket,2 - Thin wetsuit,3 - Full wetsuit,4 - Thick wetsuit,5 - Drysuit");
			static vara tempcc("NA=/,None=0,Waterproof=1,Fleece=1,1-3mm=2,4-5mm=3,6mm+=4,Drysuit=5");

			//static vara cond_diff("0 - Nontechnical,1 - Easy,2 - Normal,3 - Special challenges,4 - Advanced,5 - Extreme");
			static vara diffcc("NA=/,Easy=1,Moderate=2,Difficult=3,Strenuous=4,Very strenuous=5");

			vara cond;
			cond.SetSize(COND_MAX);

			cond[COND_DATE] = CDate(vdate);
			cond[COND_WATER] = TableMatch(stripHTML(ExtractString(memory, ">Water:", "", "</br")), watercc, cond_water);
			cond[COND_TEMP] = TableMatch(stripHTML(ExtractString(memory, ">Thermal:", "", "</br")), tempcc, cond_temp);
			cond[COND_DIFF] = TableMatch(stripHTML(ExtractString(memory, ">Difficulty:", "", "</br")), diffcc, cond_diff);

			sym.SetStr(ITEM_CONDDATE, cond.join(";"));
		}
	}

	return TRUE;
}

int CCOLLECTIVE_Empty(CSym &sym)
{
	vars summary = sym.GetStr(ITEM_ACA);
	summary.Replace(";", "");
	return summary.IsEmpty() && sym.GetStr(ITEM_LONGEST).IsEmpty() && sym.GetStr(ITEM_RAPS).IsEmpty() && sym.GetStr(ITEM_STARS).IsEmpty() && sym.GetStr(ITEM_CONDDATE).IsEmpty();
}

int CCOLLECTIVE_DownloadBeta(const char *ubase, CSymList &symlist)
{
	// ALL map 
	ubase = "canyoncollective.com";

	DownloadFile f;
	CString url = burl(ubase, "betabase");
	if (f.Download(url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}

	// tables for ratings
	vars table = ExtractString(f.memory, "<h3>Areas</h3>", "<ol", "</ol");
	vara list(table, "href=");
	if (list.length() <= 1)
		Log(LOGERR, "Could not extract sections from url %.128s", url);
	for (int i = 1; i < list.length(); ++i) {
		vars lnk = ExtractString(list[i], "\"", "", "\"");
		vars reg = ExtractString(list[i], ">", "", "<");
		CString url = burl(ubase, lnk);
		double page = 1;
		while (page != InvalidNUM)
		{
			if (f.Download(url + MkString("?page=%g", page)))
			{
				Log(LOGERR, "ERROR: can't download url %.128s", url);
				continue;
			}

			double last = ExtractNum(f.memory, "data-last=", "\"", "\"");
			if (++page > last)
				page = InvalidNUM;

			// Scan area
			vara jlist(f.memory, "showcaseListItem");
			for (int j = 1; j < jlist.length(); ++j) {
				vars &item = jlist[j];
				vars link = burl(ubase, ExtractLink(item, "betabase/"));
				vars name = stripHTML(ExtractString(item, "<h3", ">", "</h3"));

				vars stars = stripHTML(ExtractString(item, "class=\"RatingValue\"", ">", "/5"));
				vars ratings = stripHTML(ExtractString(item, "class=\"Hint\"", ">", "</"));
				vars summary = stripHTML(ExtractString(item, "Canyon Rating:", "<dd>", "</dd"));
				vars raplong = ExtractString(item, "Rappels, Longest", "<dd>", "</dd");

				CSym sym(urlstr(link), name);
				double vstars = CDATA::GetNum(stars);
				double vratings = CDATA::GetNum(ratings);
				if ((vstars > 0 && vratings < 0) || (vstars < 0 && vratings>0))
					Log(LOGERR, "Invalid star/ratings %s (%s) for %s", stars, ratings, link);
				else
					if (vratings > 0)
						sym.SetStr(ITEM_STARS, starstr(vstars, vratings));
				GetSummary(sym, summary);
				CDoubleArrayList raps, len;
				if (GetValues(GetToken(raplong, 0, '|'), NULL, len))
					sym.SetNum(ITEM_LONGEST, len.Tail());
				if (GetValues(GetToken(raplong, 1, '|'), NULL, raps))
					sym.SetStr(ITEM_RAPS, Pair(raps));

				if (CCOLLECTIVE_Empty(sym) && MODE > -2)
					continue; // skip empty canyons

				printf("Downloading %s/%s %d/%d   \r", CData(page - 1), CData(last), i, list.length());
				//if (!Update(symlist, sym, NULL, FALSE) && MODE>=-1)
				//	continue;

				if (CCOLLECTIVE_DownloadPage(f, sym.id, sym))
					if (!CCOLLECTIVE_Empty(sym))
						Update(symlist, sym, NULL, FALSE);
			}
		}
	}

	return TRUE;
}

int CCOLLECTIVE_DownloadConditions(const char *ubase, CSymList &symlist)
{
	// ALL map 
	ubase = "canyoncollective.com";

	DownloadFile f;
	CString url = burl(ubase, "betabase");
	if (f.Download(url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}

	// tables for ratings
	vars table = ExtractString(f.memory, "<h3>Recent Condition Reports</h3>", "", "<script");
	vara list(table, "<li");
	for (int i = 1; i < list.length(); ++i)
	{
		vars title = ExtractString(list[i], "\"title\"", "", "</li");
		vars link = ExtractString(title, "href=", "\"", "\"");
		vars name = stripHTML(ExtractString(title, "href=", ">", "</a"));
		if (link.IsEmpty() || !strstr(link, "betabase"))
			continue;

		CSym sym(urlstr(burl(ubase, link)), name);
		if (CCOLLECTIVE_DownloadPage(f, sym.id, sym))
		{
			vars redirect = ExtractString(f.memory, "\"redirect\"", "value=\"", "\"");
			if (!redirect.IsEmpty())
				sym.id = urlstr(burl(ubase, redirect));
			UpdateCond(symlist, sym, NULL, FALSE);
		}
	}

	return TRUE;
}


// ===============================================================================================

vara visited;

int CNORTHWEST_DownloadSelect(DownloadFile &f, const char *ubase, const char *url, CSymList &symlist, vara &region)
{
	//ASSERT(!strstr(url,"Cascade"));
	if (f.Download(burl(ubase, url)))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}

	// tables for ratings
	vars name = stripHTML(ExtractString(f.memory, "<strong", ">", "</strong>"));
	vara list(ExtractString(f.memory, "<table", "", "</table>"), "href=");
	if (list.length() <= 1)
		return FALSE;

	for (int i = 1; i < list.length(); ++i) {
		vars lnk = ExtractString(list[i], "\"", "", "\"");
		vars name = stripHTML(ExtractString(list[i], ">", "", "</a"));
		if (IsImage(lnk))
			continue;

		BOOL add = FALSE;
		if (TRUE) //GetToken(lnk, 1, '/').IsEmpty())
		{
			// folders
			if (IsSimilar(lnk, "http") || IsSimilar(lnk, "mailto"))
				continue;
			if (visited.indexOf(lnk) >= 0)
				continue;
			visited.push(lnk);
			vars reg = stripHTML(ExtractString(list[i], ">", "", "</a"));
			vara oldregion = region; region.push(reg);
			if (!CNORTHWEST_DownloadSelect(f, ubase, lnk, symlist, region))
				add = TRUE;

			region = oldregion;
		}
		else
		{
			// canyons
			add = TRUE;
		}
		//add or not to add?
		if (name.IsEmpty() || region.IsEmpty())
			continue;
		/*
			 if (strstr(name, ".jpg") || strstr(name, ".JPG"))
				 continue;
			 if (strstr(name, ".png") || strstr(name, ".PNG"))
				 continue;
			 if (strstr(name, ".pdf") || strstr(name, ".PDF"))
				 continue;
		*/
		
		//patch!
		lnk.Replace("French_Cabin/French_Cabin_Creek.php", "French_Cabin/French_Cabin_Creek.php");

		CSym sym(urlstr(burl(ubase, lnk)));
		sym.SetStr(ITEM_DESC, UTF8(name));
		sym.SetStr(ITEM_ACA, ";;;;;;;;;;" + lnk);
		sym.SetStr(ITEM_REGION, region.join(";"));
		Update(symlist, sym, NULL, FALSE);
	}

	return TRUE;
}

int CNORTHWEST_DownloadBeta(const char *ubase, CSymList &symlist)
{
	// ALL map 

	DownloadFile f;
	vara region;
	CNORTHWEST_DownloadSelect(f, ubase, "beta.php", symlist, region);

	return TRUE;
}


// ===============================================================================================

int CUSA_DownloadBeta(DownloadFile &f, const char *ubase, const char *lnk, const char *region, CSymList &symlist)
{
	vars url = burl(ubase, lnk);
	if (f.Download(url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}

	vara tables(f.memory, "<table");
	for (int t = 1; t < tables.length(); ++t) {
		CString table = ExtractString(tables[t], "", ">", "</table");
		vara lines(table, "href");
		for (int l = 1; l < lines.length(); ++l) {
			vars tr = lines[l];
			vars lnk = ExtractString(tr, "", "\"", "\"");
			vars name = ExtractString(tr, "", ">", "</a>");
			vars summary = ExtractString(tr, "</a>", "", "</tr>");
			if (lnk.IsEmpty() || strstr(name, "Frylette"))
				continue;

			vars lnkurl = burl(ubase, lnk);
			lnkurl.Replace("canyoneeringcentral.com", "canyoneeringusa.com");
			//lnkurl.Replace("//canyoneeringusa.com//", "//");
			CString ext = lnkurl.Right(4);
			if (IsSimilar(ext, ".php") || IsSimilar(ext, ".htm"))
			{
				if (f.Download(lnkurl))
				{
					Log(LOGERR, "ERROR: can't download url %.128s", lnkurl);
					return FALSE;
				}
				vars basehref = ExtractString(f.memory, "<base href=", "\"", "\"");
				if (!basehref.IsEmpty())
					lnkurl = basehref;
			}

			CSym sym(urlstr(lnkurl));
			sym.SetStr(ITEM_DESC, stripHTML(name));
			sym.SetStr(ITEM_REGION, "Utah;" + stripHTML(region));
			int nstars = 0;
			double stars = max(vara(summary, "/yellowStar").length(), vara(summary, "/yellow_star").length()) - 1;
			if (stars == 1) stars = 1.5;
			else if (stars == 2) stars = 3;
			else if (stars == 3) stars = 4.5;
			if (strstr(name, "Corral Hollow"))
				stars = 1;
			//ASSERT(!strstr(name,"Wild"));
			//ASSERT(!strstr(name,"Knotted Rope Ridge"));
			sym.SetStr(ITEM_STARS, starstr(stars, 1));
			GetSummary(sym, stripHTML(summary));
			if (sym.GetNum(ITEM_ACA) == InvalidNUM)
				sym.SetNum(ITEM_CLASS, 0);
			Update(symlist, sym, NULL, FALSE);
		}
	}
	
	return TRUE;
}

/*
int CUSA_DownloadRaves(const char *ubase2, CSymList &symlist)
{
	DownloadFile f;
	vara visitedlist;
	CString ubase = "canyoneeringusa.com";
	CString url = burl(ubase, "rave");
	visitedlist.push(url);
	int page = 1;
	double maxpage = -1;
	do
	{
	if (f.Download(url))
		{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
		}

	if (maxpage<0)
		maxpage = ExtractNum(f.memory, "Page 1 of", "", "<");

	vara list(f.memory, "class=\"nine");
	for (int i=1; i<list.length(); ++i) {

		}
	}
	while (page<=maxpage);
	return TRUE;
}
*/

int CUSA_DownloadBeta(const char *ubase2, CSymList &symlist)
{
	DownloadFile f;
	vara visitedlist;
	CString ubase = "canyoneeringusa.com";
	CString url = burl(ubase, "utah");
	visitedlist.push(url);
	if (f.Download(url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}

	// tables for ratings
	vara list(ExtractString(f.memory, ">Main Menu<", "<ul", "</ul"), "href=");
	for (int i = 1; i < list.length(); ++i) {
		vars lnk = ExtractString(list[i], "", "\"", "\"");
		vars region = ExtractString(list[i], "", ">", "</a>");
		if (strstr(lnk, "escalante"))
			lnk = "utah/escalante/archives";
		if (strstr(lnk, "zion"))
			lnk = "utah/zion/technical";
		vars url = burl(ubase, lnk);
		if (visitedlist.indexOf(url) >= 0)
			continue;
		visitedlist.push(url);

		CUSA_DownloadBeta(f, ubase, lnk, region, symlist);
	}

	CUSA_DownloadBeta(f, ubase, "utah/zion/trails", "Zion National Park", symlist);
	CUSA_DownloadBeta(f, ubase, "utah/zion/off-trail", "Zion National Park", symlist);

	// patch for choprock
	CSym sym("http://www.canyoneeringusa.com/utah/escalante/choprock/", "Choprock Canyon");
	sym.SetStr(ITEM_STARS, starstr(4.5, 1));
	Update(symlist, sym, NULL, FALSE);

	// raves

	return TRUE;
}


// ===============================================================================================

int ASOUTHWEST_DownloadLL(DownloadFile &f, CSym &sym, CSymList &symlist)
{
	if (!UpdateCheck(symlist, sym) && sym.GetNum(ITEM_LAT) != InvalidNUM && MODE > -2)
		return TRUE;
	const char *url = sym.id;
	if (f.Download(url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}
	double lat, lng;
	vara list(f.memory, "lat=");
	for (int i = 1; i < list.length(); ++i)
	{
		lat = CDATA::GetNum(list[i].Trim("\"' "));
		lng = CDATA::GetNum(ExtractString(list[i], "lon=", "", "\"").Trim("\"' "));
		if (CheckLL(lat, lng)) {
			sym.SetNum(ITEM_LAT, lat);
			sym.SetNum(ITEM_LNG, lng);
			return TRUE;
		}
	}
	return FALSE;
}

int ASOUTHWEST_DownloadBeta(DownloadFile &f, const char *ubase, const char *sub, const char *region, CSymList &symlist)
{
	CString url = burl(ubase, sub);
	if (f.Download(url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}

	// tables for ratings
	vara list(f.memory, "class=\"ag\"");
	for (int i = 1; i < list.length(); ++i) {
		vars lnk = ExtractString(list[i], "href=", "\"", "\"").replace("slot_canyons/", "");
		vars name = stripHTML(ExtractString(list[i], "href=", ">", "<"));
		vara stars(ExtractString(list[i], "class=\"star\"", ">", "<"), ";");
		CSym sym(urlstr(burl(ubase, lnk)));
		sym.SetStr(ITEM_DESC, name);
		sym.SetStr(ITEM_REGION, region);
		if (stars.length() > 0)
			sym.SetStr(ITEM_STARS, starstr(stars.length(), 1));
		ASOUTHWEST_DownloadLL(f, sym, symlist);
		Update(symlist, sym, NULL, FALSE);
	}
	
	// tables for ratings
	vara slots(f.memory, "class=\"slot\"");
	for (int s = 1; s < slots.length(); ++s) {
		vara list(slots[s], "<li>");
		for (int i = 1; i < list.length(); ++i) {
			vars lnk = ExtractString(list[i], "href=", "\"", "\"").replace("slot_canyons/", "");
			vars name = stripHTML(ExtractString(list[i], "href=", ">", "<"));
			vara stars(ExtractString(list[i], "class=\"star\"", ">", "<"), ";");
			CSym sym(urlstr(burl(ubase, lnk)));
			sym.SetStr(ITEM_DESC, name);
			sym.SetStr(ITEM_REGION, region);
			if (stars.length() > 0)
				sym.SetStr(ITEM_STARS, starstr(stars.length(), 1));
			ASOUTHWEST_DownloadLL(f, sym, symlist);
			Update(symlist, sym, NULL, FALSE);
		}
	}

	// tables for regions
	vara urls(f.memory, "href=");
	for (int i = 1; i < urls.length(); ++i) {
		vars lnk = ExtractString(urls[i], "", "\"", "\"");
		vars name = stripHTML(ExtractString(urls[i], "", ">", "<"));
		if (strstr(urls[i], "class=\"hotelbold\""))
			ASOUTHWEST_DownloadBeta(f, ubase, lnk, name, symlist);
	}

	return TRUE;
}

int ASOUTHWEST_DownloadBeta(const char *ubase, CSymList &symlist)
{
	DownloadFile f;
	ASOUTHWEST_DownloadBeta(f, ubase, "index.html", "", symlist);
	return TRUE;
}


// ===============================================================================================

int DAVEPIMENTAL_DownloadBeta(const char *ubase, CSymList &symlist)
{
	// ALL map 

	DownloadFile f;
	CString url = burl(ubase, "canyons.php");
	if (f.Download(url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}

	// tables for ratings
	vara list(f.memory, "/canyons/");
	if (list.length() <= 1)
		Log(LOGERR, "Could not extract sections from url %.128s", url);
	for (int i = 1; i < list.length(); ++i) {
		vars lnk = ExtractString(list[i], "?", "", "\"");
		CString url = burl(ubase, "/canyons/?" + lnk);
		if (f.Download(url))
		{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			continue;
		}

		vars link = url;
		vars name = stripHTML(ExtractString(f.memory, "class=\"ttl\"", ">", "<"));
		vars region = stripHTML(ExtractString(f.memory, "class=\"subttl\"", ">", "<"));
		vars summary = stripHTML(ExtractString(f.memory, "ACA Rating:", "", "<"));
		vars rating = ExtractString(f.memory, "Quality Rating:", "img/", "star");

		CSym sym(urlstr(link));
		sym.SetStr(ITEM_DESC, name);
		sym.SetStr(ITEM_REGION, region);
		GetSummary(sym, summary);
		double stars = CDATA::GetNum(rating);
		if (stars > 0)
			sym.SetStr(ITEM_STARS, starstr(stars, 1));

		Update(symlist, sym, NULL, FALSE);
		printf("Downloading %d/%d   \r", i, list.length());
	}

	return TRUE;
}


// ===============================================================================================

int ONROPE_ExtractKML(const char *ubase, const char *url, inetdata *out, int fx)
{
	//if (!fx)
	//	return FALSE;

	DownloadFile f;
	vars id = GetKMLIDX(f, url);
	if (id.IsEmpty())
		return FALSE; // not available

	CString credit = " (Data by " + CString(ubase) + ")";

	//alternate go/gpx/download/578
	CString url2 = burl(ubase, id);
	if (DownloadRetry(f, url2))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url2);
		return FALSE;
	}

	return GPX_ExtractKML(credit, url, f.memory, out);
}

int ONROPE_DownloadBeta(const char *ubase, CSymList &symlist)
{
	// ALL map 

	DownloadFile f;
	CString url = burl(ubase, "about-canyon-beta");
	if (f.Download(url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}

	// tables for ratings
	vara list(f.memory, "class=\"external-link\"");
	if (list.length() <= 1)
		Log(LOGERR, "Could not extract sections from url %.128s", url);
	for (int i = 1; i < list.length(); ++i) {
		vars lnk = ExtractString(list[i], "href=");
		CString url = burl(ubase, lnk);
		if (f.Download(url))
		{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			continue;
		}
		vara jlist(f.memory, "class=\"page-collection\"");
		if (jlist.length() <= 1)
			Log(LOGERR, "Could not extract sections from url %.128s", url);
		for (int j = 1; j < jlist.length(); ++j) {
			vars lnk = ExtractString(jlist[j], "href=");
			CString url = burl(ubase, lnk);
			printf("Downloading %d/%d %d/%d   \r", i, list.length(), j, jlist.length());

			if (f.Download(url))
			{
				Log(LOGERR, "ERROR: can't download url %.128s", url);
				continue;
			}

			vars link = url;
			vars name = GetToken(ExtractString(f.memory, "<h2", ">", "<"), 0, ',').Trim();
			vars region = stripHTML(ExtractString(f.memory, ">Location:", "", "<br"));
			vars longest = ExtractString(f.memory, ">Longest Rappel:", "", "<").Trim();
			vars summary = ExtractString(f.memory, ">Rating:", "", "<").Trim();
			//ASSERT(!strstr(url, "pearl"));
			vars kmlidx = ExtractLink(f.memory, ".gpx");

			CSym sym(urlstr(link));
			sym.SetStr(ITEM_DESC, name);
			sym.SetStr(ITEM_REGION, region);
			sym.SetStr(ITEM_KML, kmlidx);
			CDoubleArrayList len;
			GetSummary(sym, summary);
			if (GetValues(longest, ulen, len))
				sym.SetNum(ITEM_LONGEST, len.Tail());

			Update(symlist, sym, NULL, FALSE);
		}
	}

	return TRUE;
}


// ===============================================================================================

#define CANYONCARTOTICKS 3000
static double canyoncartoticks;

double CANYONCARTO_Stars(double stars)
{
	if (stars == InvalidNUM)
		return InvalidNUM;

	stars = stars / 7 * 5;
	return floor(stars * 10 + 0.5) / 10;
}

int CANYONCARTO_DownloadRatings(CSymList &symlist)
{
	DownloadFile f;
	vars url = "http://canyon.carto.net/cwiki/bin/view/Canyons/BewertungsResultate";
	Throttle(canyoncartoticks, CANYONCARTOTICKS);
	if (f.Download(url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}

	// gather data
	CSymList votes;
	vara list(ExtractString(f.memory, "<table", "", "</table"), "<tr");
	for (int i = 1; i < list.length(); ++i)
	{
		vars id = ExtractString(list[i], "href=", ">", "<");
		vars user = ExtractString(list[i], "/bin/view/Main/", "", "\"");
		double stars = ExtractNum(list[i], "Bewertung=", "", NULL);
		if (stars == InvalidNUM)
			stars = ExtractNum(list[i], "Bewertung</a>", "=", NULL);
		if (id.IsEmpty() || user.IsEmpty() || stars == InvalidNUM)
		{
			Log(LOGERR, "Invalid vote entry #%d %.256s", i, list[i]);
			continue;
		}

		CSym sym(id + "@" + user, urlstr("http://canyon.carto.net/cwiki/bin/view/Canyons/" + id));
		sym.SetNum(ITEM_LAT, stars);

		int f = votes.Find(sym.id);
		if (f < 0)
			votes.Add(sym); // add
		else
			votes[f] = sym; // update
	}

	votes.Sort();

	// compute stars
	for (int i = 0; i < votes.GetSize(); )
	{
		// compute average
		double sum = 0, num = 0;
		vars id = votes[i].GetStr(ITEM_DESC);
		while (i < votes.GetSize() && votes[i].GetStr(ITEM_DESC) == id)
		{
			sum += votes[i].GetNum(ITEM_LAT);
			++num;
			++i;
		}
		// add to list
		int f = symlist.Find(id);
		if (f < 0)
		{
			Log(LOGWARN, "Unmatched CANYONCARTO vote for %s", id);
			continue;
		}

		symlist[f].SetStr(ITEM_STARS, starstr(CANYONCARTO_Stars(sum / num), num));
	}

	return TRUE;
}

int CANYONCARTO_DownloadPage(DownloadFile &f, const char *url, CSym &sym, CSymList &symlist)
{
	// Using CANYONCARTO_Downloadratings instead!
/*
// download list of recent additions
	Throttle(canyoncartoticks, CANYONCARTOTICKS);
	if (f.Download(url))
		{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
		}

	// current stars
	double oldstars = sym.GetNum(ITEM_STARS);
	double newstars = CANYONCARTO_Stars(ExtractNum(f.memory, "Gesamt:", "", "<"));
	double ratings = ExtractNum(f.memory, "Bew total:", "", "<");
	if (ratings>0 && abs(newstars - oldstars)>0.5)
		Log(LOGWARN, "Inconsisten stars %s != %s for %s", CData(oldstars), CData(newstars), sym.id);
	if (ratings!=InvalidNUM) //stars!=InvalidNUM &&
		{
		sym.SetStr(ITEM_STARS, starstr(oldstars, ratings));
		return TRUE;
		}
	return FALSE;
*/
	return TRUE;
}

int CANYONCARTO_DownloadBeta(const char *ubase, CSymList &symlist)
{
	//vara recentlist, urllist;
	//const char *canyonlink = "href=\"/canyoning/canyon/";
	//const char *canyondesclink = "/canyoning/canyon-description/";
	//varas months("janvier=Jan,février=Feb,mars=Mar,avril=Apr,mai=May,juin=Jun,juillet=Jul,août=Aug,septembre=Sep,octobre=Oct,novembre=Nov,décembre=Dec");

	DownloadFile f;
	CString url = "http://canyon.carto.net/cwiki/bin/custom/cmap2kml.pl";

	// download list of recent additions
	if (f.Download(url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}

	vars err;
	vara list(ExtractString(f.memory, "", "<Folder>", "</Folder>"), "<Placemark>");
	for (int i = 1; i < list.length(); ++i)
	{
		const char *memory = list[i];
		vars link = ExtractString(memory, "href=", "'", "'").replace("http:/canyon", "http://canyon");
		vars name = UTF8(stripHTML(ExtractString(memory, "", "<name>", "</name>")));
		CSym sym(urlstr(link), name);

		vara coords(ExtractString(memory, "", "<coordinates>", "</coordinates>"));
		if (coords.length() > 1)
		{
			double lat = CDATA::GetNum(coords[1]);
			double lng = CDATA::GetNum(coords[0]);
			if (CheckLL(lat, lng))
			{
				sym.SetNum(ITEM_LAT, lat);
				sym.SetNum(ITEM_LNG, lng);
			}
		}

		vars summary = stripHTML(ExtractString(memory, ">Bewertung", "<td>", "</td>"));
		GetSummary(sym, summary);

		vars aka = UTF8(stripHTML(ExtractString(memory, ">Aliasname", "<td>", "</td>")));
		sym.SetStr(ITEM_AKA, name + ";" + aka.replace(",", ";"));

		double tmax, tmin;
		vars time = stripHTML(ExtractString(memory, ">Gesamtzeit", "<td>", "</td>").replace("ka", ""));
		GetHourMin(time, tmin, tmax, link);
		if (tmin > 0)
			sym.SetNum(ITEM_MINTIME, tmin);
		if (tmax > 0)
			sym.SetNum(ITEM_MINTIME, tmax);

		sym.SetStr(ITEM_DEPTH, GetMetric(stripHTML(ExtractString(memory, ">Höhendiff.", "<td>", "</td>"))));

		CDoubleArrayList max;
		if (GetValues(memory, ">max.", "<td>", "</td>", ulen, max))
			sym.SetNum(ITEM_LONGEST, max.Tail());

		sym.SetStr(ITEM_KML, "X");

		vars votes = stripHTML(ExtractString(memory, ">subj.", "<td>", "</td>"));
		double stars = CANYONCARTO_Stars(CDATA::GetNum(votes));
		//double ratings = CDATA::GetNum(GetToken(votes, 2, ' '));
		if (stars > 0 && err.IsEmpty())
		{
#if 0
			sym.SetStr(ITEM_STARS, starstr(stars * 5 / 7, 1));
#else
			sym.SetStr(ITEM_STARS, starstr(stars, 0));
			double newstars = sym.GetNum(ITEM_STARS);
			if (newstars > 0)
			{
				int find = symlist.Find(link);
				if (find >= 0)
				{
					CString o = symlist[find].GetStr(ITEM_STARS);
					CString n = sym.GetStr(ITEM_STARS);
					if (GetToken(o, 0, '*') != GetToken(n, 0, '*'))
						find = -1;
				}
				if (find < 0 || MODE <= -2)
				{
					// download new stars
					if (!CANYONCARTO_DownloadPage(f, link, sym, symlist)) {
						err = "ERROR";
						Log(LOGERR, "CANYONCARTO_DownloadBeta blocked for %s ???", sym.id);
					}
				}
				else
				{
					// preserve old stars
					sym.SetStr(ITEM_STARS, symlist[find].GetStr(ITEM_STARS));
				}
			}
#endif
		}
		printf("%d %d/%d %s   \r", symlist.GetSize(), i, list.length(), err);
		Update(symlist, sym, NULL, FALSE);
	}

	CANYONCARTO_DownloadRatings(symlist);
	return TRUE;
}

CString CANYONCARTO_Watermark(const char *scredit, const char *memory, int size)
{
	CString credit(scredit);
	CString kml(memory, size);
	kml.Replace("Einstiege", "Start");
	kml.Replace("Einstieg", "Start");
	kml.Replace("Ausstiege", "End");
	kml.Replace("Ausstieg", "End");
	kml.Replace("ParkPla\xE3\xB3\xBA\x65", "Parking");
	kml.Replace("ParkPlatz", "Parking");

	const char *desc = "</description>";
	const char *name = "</name>";
	vara names(kml, name);
	for (int i = 1; i < names.length(); ++i)
		if (names[i].Replace(desc, credit + desc) <= 0)
			names[i].Insert(0, "<description>" + credit + desc);
	return names.join(name);
}

//http://canyon.carto.net/cwiki/bin/view/Canyons/Bodengo3Canyon

int CANYONCARTO_ExtractKML(const char *ubase, const char *url, inetdata *out, int fx)
{
	CString credit = " (Data by " + CString(ubase) + ")";
	CString kmlurl = "http://canyon.carto.net/cwiki/bin/custom/cmap2kml.pl?canyon=" + GetToken(GetToken(url, GetTokenCount(url, '/') - 1, '/'), 0, '&');
	Throttle(canyoncartoticks, CANYONCARTOTICKS);
	return KMZ_ExtractKML(credit, kmlurl, out, CANYONCARTO_Watermark);
}


// ===============================================================================================

int WROCLAW_DownloadPage(DownloadFile &f, const char *url, CSym &sym)
{
	// download list of recent additions
	Throttle(canyoncartoticks, 1000);
	if (f.Download(url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}

	//double stars = ExtractNum(f.memory, "Gesamt:", "", "<");
	vars desc;
	float lat = InvalidNUM, lng = InvalidNUM;
	vars gps = stripHTML(ExtractString(f.memory, ">GPS", ":", "</table"));
	gps.Replace("&#176;", "o");
	ExtractCoords(gps, lat, lng, desc);
	if (CheckLL(lat, lng))
	{
		sym.SetNum(ITEM_LAT, lat);
		sym.SetNum(ITEM_LNG, lng);
	}

	vara list(ExtractString(f.memory, ">Wasze opinie", "", "</table"), "<td");
	int n = 0;
	double sum = 0;
	for (int i = 1; i < list.length(); ++i)
	{
		vara len(list[i], "star.gif");
		if (len.length() > 1)
		{
			sum += len.length() - 1;
			++n;
		}
	}
	if (n > 0)
		sym.SetStr(ITEM_STARS, starstr(sum / n, n));

	sym.SetStr(ITEM_LONGEST, GetMetric(stripHTML(ExtractString(f.memory, "max kaskada:", "", "<"))));

	vars depthlength = stripHTML(ExtractString(f.memory, "deniwelacja:", "", "</td"));
	sym.SetStr(ITEM_DEPTH, GetMetric(GetToken(depthlength, 0, ':')));
	sym.SetStr(ITEM_LENGTH, GetMetric(GetToken(depthlength, 1, ':')));

	vars rock = stripHTML(ExtractString(strstr(f.memory, "ska\xC5\x82"), ":", "", "</td"));
	sym.SetStr(ITEM_ROCK, rock);

	vara times;
	vars memory = f.memory;
	memory.Replace("\xc5\x9b", "s");
	memory.Replace("\xc3\xb3", "o");
	const char *ids[] = { "dojscie:", "kanion:", "powrot:" };
	//ASSERT( !strstr(url,"/kanion.php?id=41"));
	for (int t = 0; t < sizeof(ids) / sizeof(*ids); ++t)
	{
		vars time = stripHTML(ExtractString(memory, ids[t], "", "<"));
		if (!time.Replace("min", "m"))
			time += "m";
		times.push(time);
	}
	GetTotalTime(sym, times, url);

	return TRUE;
}

int WROCLAW_DownloadBeta(const char *ubase, CSymList &symlist)
{
	//vara recentlist, urllist;
	//const char *canyonlink = "href=\"/canyoning/canyon/";
	//const char *canyondesclink = "/canyoning/canyon-description/";
	//varas months("janvier=Jan,février=Feb,mars=Mar,avril=Apr,mai=May,juin=Jun,juillet=Jul,août=Aug,septembre=Sep,octobre=Oct,novembre=Nov,décembre=Dec");

	DownloadFile f;
	CString url = "http://www.canyoning.wroclaw.pl/bylismy.php?r=0&s=a";

	// download list of recent additions
	if (f.Download(url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}

	vara list(ExtractString(f.memory, "class=\"lista\"", "", "</table>"), "<tr");
	for (int i = 1; i < list.length(); ++i)
	{
		const char *memory = list[i];
		vars link = ExtractString(memory, "href=", "\"", "\"");
		vars name = UTF8(stripHTML(ExtractString(memory, "href=", ">", "</a>")));
		if (link.IsEmpty())
			continue;

		CSym sym(urlstr("http://www.canyoning.wroclaw.pl/" + link), name);
		printf("%d/%d   \r", i, list.length());
		if (!UpdateCheck(symlist, sym) && MODE > -2)
			continue;
		if (WROCLAW_DownloadPage(f, sym.id, sym))
			Update(symlist, sym, NULL, FALSE);
		//ASSERT(!strstr(sym.GetStr(ITEM_DESC),"Corredora"));

	}

	return TRUE;
}


// ===============================================================================================

int COND365_DownloadBeta(const char *ubase, CSymList &symlist)
{
	DownloadFile f;
	CString url = burl(ubase, "");

	// download list of recent additions
	if (f.Download(url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}

	double date = CDATA::GetDate(ExtractString(f.memory, "datetime=", "\"", "\""));
	vars title = stripHTML(ExtractString(f.memory, "datetime=", "title=\"", "\""));
	if (date > 0 && IsSimilar(title, "Info Caudales"))
		for (int i = 0; i < rwlist.GetSize(); ++i)
			if (strstr(rwlist[i].GetStr(ITEM_MATCH), ubase))
			{
				vars title = rwlist[i].GetStr(ITEM_DESC);
				vara cond; cond.SetSize(COND_MAX);
				cond[COND_DATE] = CDate(date);
				cond[COND_LINK] = ubase;
				CSym sym(RWTITLE + title, title);
				sym.SetStr(ITEM_CONDDATE, cond.join(";"));
				Update(symlist, sym, NULL, FALSE);
			}
	
	return TRUE;
}

int CANDITION_DownloadBeta(const char *ubase, CSymList &symlist)
{
	//vara recentlist, urllist;
	//const char *canyonlink = "href=\"/canyoning/canyon/";
	//const char *canyondesclink = "/canyoning/canyon-description/";
	//varas months("janvier=Jan,février=Feb,mars=Mar,avril=Apr,mai=May,juin=Jun,juillet=Jul,août=Aug,septembre=Sep,octobre=Oct,novembre=Nov,décembre=Dec");

	DownloadFile f;
	CString url = "http://candition.com";

	// download list of recent additions
	if (f.Download(url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}

	vara rlist(f.memory, "showonlyone(");
	for (int r = 1; r < rlist.length(); ++r)
	{
		vars region = stripHTML(ExtractString(rlist[r], "'", "", "'"));
		vara list(rlist[r], "href=");
		for (int i = 1; i < list.length(); ++i)
		{
			const char *memory = list[i];
			vars link = ExtractString(memory, "\"", "", "\"");
			vars name = stripHTML(ExtractString(memory, "", ">", "</a>"));
			if (link.IsEmpty() || !strstr(link, "/canyons/"))
				continue;

			CSym sym(urlstr(url + link), name);
			ASSERT(!name.IsEmpty());
			sym.SetStr(ITEM_REGION, region.replace(" - ", ";"));

			// condition
			vars date = stripHTML(ExtractString(memory, "Last Candition:", "", "</div"));
			double vdate = CDATA::GetDate(date, "MMM D; YYYY");
			if (vdate <= 0) // ignore if never posted any condition
				continue;

			vars stars;
			double vstars = CDATA::GetNum(stripHTML(ExtractString(memory, "rating_box", ">", "</div")));
			if (vstars >= 0 && vstars <= cond_stars.length())
				stars = cond_stars[(int)vstars];
			//sym.SetStr(ITEM_CONDDATE, CDate(vdate)+";"+stars);
			// don't use stars
			sym.SetStr(ITEM_CONDDATE, CDate(vdate));
			Update(symlist, sym, NULL, FALSE);
		}
	}
	
	return TRUE;
}


// ===============================================================================================

int CANYONEAST_DownloadPage(DownloadFile &f, const char *url, CSym &sym)
{
	// download list of recent additions
	static double ticks;
	Throttle(ticks, 1000);
	if (f.Download(url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}

	//double stars = ExtractNum(f.memory, "", "", "<");
	vars desc;
	float  lat = InvalidNUM, lng = InvalidNUM;
	vars gps = stripHTML(ExtractString(f.memory, "punto di accesso al greto", ":", "<br"));
	gps.Replace("°", "o");
	gps.Replace(";", ".");
	ExtractCoords(gps, lat, lng, desc);
	if (CheckLL(lat, lng))
	{
		sym.SetNum(ITEM_LAT, lat);
		sym.SetNum(ITEM_LNG, lng);
	}

	sym.SetStr(ITEM_SEASON, stripHTML(ExtractString(f.memory, ">Periodo", ":", "<br")));

	GetSummary(sym, stripHTML(ExtractString(f.memory, "Difficoltà:", "", "<br")));

	sym.SetStr(ITEM_LONGEST, GetMetric(stripHTML(ExtractString(f.memory, " alta:", "", "<br"))));
	sym.SetStr(ITEM_LENGTH, GetMetric(stripHTML(ExtractString(f.memory, "Sviluppo:", "", "<br"))));
	sym.SetStr(ITEM_DEPTH, GetMetric(stripHTML(ExtractString(f.memory, "Dislivello:", "", "<br"))));

	sym.SetStr(ITEM_SHUTTLE, stripHTML(ExtractString(f.memory, "Navetta:", "", "<br").replace(" ", "").replace("Circa", "").replace("No/", "Optional ").replace(",", ".")));

	vars interest = stripHTML(ExtractString(f.memory, "Interesse:", "", "<br"));
	int stars = 0;
	if (strstr(interest, "Nazionale"))
		stars = 5;
	else if (strstr(interest, "Regionale"))
		stars = 4;
	else if (strstr(interest, "Locale"))
		stars = 3;
	else
		Log(LOGERR, "Unknown interest '%s' for %s", interest, url);
	if (stars > 0)
		sym.SetStr(ITEM_STARS, starstr(stars, 1));

	vara times;
	const char *ids[] = { "Avvicinamento:", "Progressione:", "Ritorno:" };
	//ASSERT( !strstr(url,"/kanion.php?id=41"));
	for (int t = 0; t < sizeof(ids) / sizeof(*ids); ++t)
	{
		CString time = stripHTML(ExtractString(f.memory, ids[t], "", "<br"));
		while (!ExtractStringDel(time, "(", "", ")").IsEmpty());
		times.push(vars(time).replace("ore", "h").replace("giorni", "j").replace(";", "."));
	}
	GetTotalTime(sym, times, url);

	return TRUE;
}

int CANYONEAST_DownloadBeta(const char *ubase, CSymList &symlist)
{
	//vara recentlist, urllist;
	//const char *canyonlink = "href=\"/canyoning/canyon/";
	//const char *canyondesclink = "/canyoning/canyon-description/";
	//varas months("janvier=Jan,février=Feb,mars=Mar,avril=Apr,mai=May,juin=Jun,juillet=Jul,août=Aug,septembre=Sep,octobre=Oct,novembre=Nov,décembre=Dec");

	DownloadFile f;
	CString url = "http://www.canyoneast.it/target.php?w=1920&lng=it&target=Elenco";

	// download list of recent additions
	if (f.Download(url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}

	vara list(ExtractString(f.memory, "Elenco Canyons", "", "\"endele\""), "<li>");
	for (int i = 1; i < list.length(); ++i)
	{
		const char *memory = list[i];
		vars link = ExtractString(ExtractString(memory, "href=", "", "</a>"), "\"", "", "\"");
		vars name = UTF8(stripHTML(ExtractString(memory, "href=", ">", "</a>")));
		if (link.IsEmpty())
			continue;

		CSym sym(urlstr(link.replace("www.", "WWW.")), name);
		printf("%d/%d   \r", i, list.length());
		if (!UpdateCheck(symlist, sym) && MODE > -2)
			continue;
		if (CANYONEAST_DownloadPage(f, sym.id, sym))
			Update(symlist, sym, NULL, FALSE);
		//ASSERT(!strstr(sym.GetStr(ITEM_DESC),"Corredora"));

	}

	return TRUE;
}


// ===============================================================================================

int TNTCANYONING_DownloadPage(DownloadFile &f, const char *url, CSym &sym)
{
	// download list of recent additions
	static double ticks;
	Throttle(ticks, 1000);
	if (f.Download(url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}

	GetSummary(sym, stripHTML(ExtractString(f.memory, ">Difficolt", ":", "</p")));

	sym.SetStr(ITEM_SEASON, stripHTML(ExtractString(f.memory, ">Periodo", ":", "</p")));

	sym.SetNum(ITEM_RAPS, CDATA::GetNum(stripHTML(ExtractString(f.memory, "Numero calate", ":", "</p"))));
	sym.SetStr(ITEM_LONGEST, GetMetric(stripHTML(ExtractString(f.memory, "Calata max", ":", "</p"))));
	sym.SetStr(ITEM_LENGTH, GetMetric(stripHTML(ExtractString(f.memory, ">Sviluppo", ":", "</p"))));
	sym.SetStr(ITEM_DEPTH, GetMetric(stripHTML(ExtractString(f.memory, ">Dislivello", ":", "</p"))));

	sym.SetStr(ITEM_SHUTTLE, stripHTML(ExtractString(f.memory, ">Navetta", ":", "</p").replace(" ", "").replace("Circa", "").replace("Si,", "").replace("si,", "").replace(",", ".")));

	/*
	vars interest = stripHTML(ExtractString(f.memory, "Interesse:", "", "<br"));
	int stars = 0;
	if (strstr(interest, "Nazionale"))
		stars = 5;
	else if (strstr(interest, "Regionale"))
		stars = 4;
	else if (strstr(interest, "Locale"))
		stars = 3;
	else
		Log(LOGERR, "Unknown interest '%s' for %s", interest, url);
	if (stars>0)
		sym.SetStr(ITEM_STARS, starstr(stars,1));
	*/

	vara times;
	const char *ids[] = { "Tempo Avvicinamento", "Tempo Discesa", "Tempo Ritorno" };
	//ASSERT( !strstr(url,"/kanion.php?id=41"));
	for (int t = 0; t < sizeof(ids) / sizeof(*ids); ++t)
	{
		CString time = stripHTML(ExtractString(f.memory, ids[t], ":", "</p"));
		while (!ExtractStringDel(time, "(", "", ")").IsEmpty());
		time.Trim("\t -");
		if (!time.IsEmpty()) time = vars(time).split("Tempo").first();
		times.push(vars(time).replace("ore", "h").replace("giorni", "j").replace(";", "."));
	}
	GetTotalTime(sym, times, url);

	return TRUE;
}

int TNTCANYONING_DownloadBeta(const char *ubase, CSymList &symlist)
{
	//vara recentlist, urllist;
	//const char *canyonlink = "href=\"/canyoning/canyon/";
	//const char *canyondesclink = "/canyoning/canyon-description/";
	//varas months("janvier=Jan,février=Feb,mars=Mar,avril=Apr,mai=May,juin=Jun,juillet=Jul,août=Aug,septembre=Sep,octobre=Oct,novembre=Nov,décembre=Dec");

	DownloadFile f;
	CString url = "http://www.tntcanyoning.it/";

	// download list of recent additions
	if (f.Download(url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}

	vara list(ExtractString(f.memory, ">Canyon Percorsi", "", "</ul"), "canyon-percorsi/");
	for (int i = 1; i < list.length(); ++i)
	{
		const char *memory = list[i];
		vars link = ExtractString(memory, "", "", "\"");
		vars name = UTF8(stripHTML(ExtractString(memory, "", ">", "</a>")));
		if (link.IsEmpty())
			continue;

		CSym sym(urlstr("http://www.tntcanyoning.it/canyon-percorsi/" + link), name);
		printf("%d/%d   \r", i, list.length());
		if (!UpdateCheck(symlist, sym) && MODE > -2)
			continue;
		if (TNTCANYONING_DownloadPage(f, sym.id, sym))
			Update(symlist, sym, NULL, FALSE);
		//ASSERT(!strstr(sym.GetStr(ITEM_DESC),"Corredora"));

	}

	return TRUE;
}


// ===============================================================================================

vars VWExtractString(const char *memory, const char *id)
{
	vars cell = ExtractString(strstr(memory, "<table"), id, "", "</tr");
	return ExtractString(cell, "<t", ">", "</t").replace(",", ".");
}

double VWExtractStars(const char *memory, const char *id)
{
	vars cell = VWExtractString(memory, id);

	double num = CDATA::GetNum(stripHTML(cell));
	if (num > 0) return num + 1; // 0-4 scale


	int n = 0, r = 0;
	vars str = ExtractString(cell, "color:", ">", "<");
	for (int i = 0; str[i] != 0; ++i)
		if (str[i] == 'O')
			++n;
		else
			++r;

	return (n + (r > 0 ? 0.5 : 0))*5.0 / 7.0; // 0-7 scale
}

int VWCANYONING_DownloadPage(DownloadFile &f, const char *url, CSym &sym)
{
	// download list of recent additions
	static double ticks;
	Throttle(ticks, 1000);
	if (f.Download(url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}

	if (!strstr(f.memory, "AVVICINAMENTO") && !strstr(f.memory, "Avvicinamento"))
		return FALSE;

	GetSummary(sym, stripHTML(VWExtractString(f.memory, ">DIFFICOLTA")));

	vars season = stripHTML(VWExtractString(f.memory, ">PERIODO"));
	sym.SetStr(ITEM_SEASON, season.replace("(", "; BEST ").replace(")", ""));


	vars raps = stripHTML(VWExtractString(f.memory, ">CALATE"));
	sym.SetNum(ITEM_RAPS, CDATA::GetNum(raps));
	sym.SetStr(ITEM_LONGEST, GetMetric(GetToken(raps, 1, '(')));

	sym.SetStr(ITEM_LENGTH, GetMetric(stripHTML(VWExtractString(f.memory, ">SVILUPPO"))));
	sym.SetStr(ITEM_DEPTH, GetMetric(stripHTML(VWExtractString(f.memory, ">DISLIVELLO"))));

	vars shuttle = stripHTML(VWExtractString(f.memory, ">NAVETTA"));
	sym.SetStr(ITEM_SHUTTLE, shuttle.replace(" ", "").replace("Circa", "").Trim("SsIi,.()"));

	sym.SetStr(ITEM_ROCK, GetMetric(stripHTML(VWExtractString(f.memory, ">GEOLOGIA"))));
	double stars = VWExtractStars(f.memory, ">BELLEZZA");
	double fun = VWExtractStars(f.memory, ">DIVERTIMENTO");
	if (fun > stars)
		stars = fun;
	if (stars > 0)
	{
		if (stars > 5) stars = 5;
		sym.SetStr(ITEM_STARS, starstr(stars, 1));
	}

	vars region;
	vars loc = stripHTML(VWExtractString(f.memory, ">LOCALITA"));
	vara aregion = vara(sym.id, "/");
	if (aregion.length() > 4)
		sym.SetStr(ITEM_REGION, region = aregion[4]);
	if (!loc.IsEmpty())
		sym.SetStr(ITEM_LAT, "@" + loc + "." + region);

	/*
	vars interest = stripHTML(ExtractString(f.memory, "Interesse:", "", "<br"));
	int stars = 0;
	if (strstr(interest, "Nazionale"))
		stars = 5;
	else if (strstr(interest, "Regionale"))
		stars = 4;
	else if (strstr(interest, "Locale"))
		stars = 3;
	else
		Log(LOGERR, "Unknown interest '%s' for %s", interest, url);
	if (stars>0)
		sym.SetStr(ITEM_STARS, starstr(stars,1));
	*/

	vara times;
	const char *ids[] = { ">AVVICINAMENTO", ">TEMPI", ">RIENTRO" };
	//ASSERT( !strstr(url,"/kanion.php?id=41"));
	for (int t = 0; t < sizeof(ids) / sizeof(*ids); ++t)
	{
		CString time = stripHTML(VWExtractString(f.memory, ids[t]));
		while (!ExtractStringDel(time, "(", "", ")").IsEmpty());
		times.push(vars(time).replace("ore", "h").replace("giorni", "j").replace(";", "."));
	}
	GetTotalTime(sym, times, url);

	return TRUE;
}

int VWCANYONING_DownloadBeta(const char *ubase, CSymList &symlist)
{
	//vara recentlist, urllist;
	//const char *canyonlink = "href=\"/canyoning/canyon/";
	//const char *canyondesclink = "/canyoning/canyon-description/";
	//varas months("janvier=Jan,février=Feb,mars=Mar,avril=Apr,mai=May,juin=Jun,juillet=Jul,août=Aug,septembre=Sep,octobre=Oct,novembre=Nov,décembre=Dec");

	DownloadFile f;
	CString url = "http://www.verticalwatercanyoning.com";

	// download list of recent additions
	if (f.Download(url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}

	vara done;
	vara list(f.memory, "href=");
	for (int i = 1; i < list.length(); ++i)
	{
		const char *memory = list[i];
		vars link = ExtractString(memory, "\"", "", "\"");
		vars name = UTF8(stripHTML(ExtractString(memory, "", ">", "</a>")));
		const char *plink = strstr(link, "profili/");
		if (link.IsEmpty() || !plink || strstr(link, "ztemplate"))
			continue;

		if (done.indexOf(link) >= 0)
			continue;
		done.push(link);

		vara plinka(plink, "/");
		if (plinka.length() <= 2 || plinka[2].IsEmpty())
		{
			// region
			if (f.Download(link))
			{
				Log(LOGERR, "ERROR: can't download url %.128s", link);
				continue;
			}

			// add possible links
			vara list2(f.memory, "href=");
			for (int j = 1; j < list2.GetSize(); ++j)
				list.push(list2[j]);
			continue;
		}

		// profili/region/somewhere
		CSym sym(urlstr(link), name);
		printf("%d/%d   \r", i, list.length());
		if (!UpdateCheck(symlist, sym) && MODE > -2)
			continue;
		if (VWCANYONING_DownloadPage(f, sym.id, sym))
			Update(symlist, sym, NULL, FALSE);
		//ASSERT(!strstr(sym.GetStr(ITEM_DESC),"Corredora"));

	}

	return TRUE;
}


// ===============================================================================================

int HIKEAZ_ExtractKML(const char *ubase, const char *url, inetdata *out, int fx)
{
	//if (!fx)
	//	return FALSE;


	DownloadFile f;
	vars id = GetKMLIDX(f, url, "'Download GPX Route'", "data-gps='", "'");
	if (id.IsEmpty())
		return FALSE; // not available

	CString credit = " (Data by " + CString(ubase) + ")";

	//alternate go/gpx/download/578
	CString url2 = "http://hikearizona.com/location_ROUTE.php?ID=" + id;
	if (DownloadRetry(f, url2))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url2);
		return FALSE;
	}

	if (!IsSimilar(f.memory, "<?xml"))
		return FALSE; // no pt data

	vara styles, points, lines;
	styles.push("dot=http://maps.google.com/mapfiles/kml/shapes/open-diamond.png");

	vara linelist;
	vara data(f.memory, "<PT>");
	for (int i = 1; i < data.length(); ++i)
	{
		CString name = ExtractString(data[i], "<TN>", "", "<");
		double lat = ExtractNum(data[i], "<LA>", "", "<");
		double lng = ExtractNum(data[i], "<LO>", "", "<");
		if (!CheckLL(lat, lng))
		{
			Log(LOGERR, "Invalid PT wpt='%s' for %s", data[i], url);
			continue;
		}
		if (!name.IsEmpty())
			points.push(KMLMarker("dot", lat, lng, name, credit));
		linelist.push(CCoord3(lat, lng));
	}
	lines.push(KMLLine("Track", credit, linelist, OTHER, 3));

	return SaveKML("data", credit, url, styles, points, lines, out);
}

int HIKEAZ_GetCondition(const char *memory, CSym &sym)
{
	vara cond; cond.SetSize(COND_MAX);

	vars date = ExtractString(memory, "itemprop='dateCreated'", "content='", "'");
	double vdate = CDATA::GetDate(date, "MMM D YYYY");
	if (vdate <= 0)
		return FALSE;
	cond[COND_DATE] = CDate(vdate);

	double stars = ExtractNum(memory, "itemprop='ratingValue'", "content='", "'");
	if (stars > 0 && stars <= 5)
		cond[COND_STARS] = cond_stars[(int)stars];

	vars trip = ExtractString(memory, "<input", "//hikearizona.com/trip=", ">");
	trip = GetToken(trip.Trim("'\" "), 0, "'\" ");
	if (!trip.IsEmpty())
		cond[COND_LINK] = "http://hikearizona.com/trip=" + trip;
	else
		Log(LOGERR, "Invalid trip id for %s %s", sym.GetStr(ITEM_DESC), sym.id);

	sym.SetStr(ITEM_CONDDATE, cond.join(";"));
	
	return TRUE;
}

int HIKEAZ_DownloadBeta(const char *ubase, CSymList &symlist)
{
	DownloadFile f;
	vars base = "http://hikearizona.com/interest.php?SHOW=YES";
	vars url = base;
	if (f.Download(url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}

	vars optsel = ExtractString(f.memory, "name='STx'", "", "</select>");
	vara opt(optsel, "<option");


	for (int o = 1; o < opt.length(); ++o)
	{
		const char *option = opt[o];
		double optval = ExtractNum(option, "value=", "'", "'");
		if (optval < 0) {
			Log(LOGERR, "ERROR: Invalid STx");
			continue;
		}
		const char *aztypes[] = { "&TYP=12", "&TYP=13", NULL };
		for (int aztype = 0; aztypes[aztype] != NULL; ++aztype)
		{
			vars url = base + aztypes[aztype] + "&STx=" + CData(optval);
			vars state = ExtractString(option, "value=", ">", "<").trim();
			if (state.IsEmpty())
				continue;

			int start = 0;
			while (true)
			{
				if (f.Download(url + MkString("&start=%d", start)))
				{
					Log(LOGERR, "ERROR: can't download url %.128s", url);
					continue;
				}
				int maxstart = 0;
				vara starts(f.memory, "start=");
				for (int i = 1; i < starts.length(); ++i)
				{
					double num = CDATA::GetNum(starts[i]);
					if (num > maxstart) maxstart = (int)num;
				}
				/*
						vars content = ExtractString(f.memory, "<table", "<table", "</table");
						content = content.split(" Adventure Tales ").first();
						content.Replace("href=\"../WM/saratoga.htm\"", "");

						const char *types[] = {
								"Soaking",
								"Spelunking",
								"Slot Canyon", "Canyoneering", "Peekaboo",
								"Hiking", "Backpacking", "Exploring", "Family", "Picknick",
								//"Roadside Attraction",  "Ruins", "Rock Art", "Petroglyphs", "Climb", "Scrambl",
								//"Hunting",  "Rafting", "Dinosaur", "Salt lake",
								//"Swimming", "Mining",
								NULL };
						int typen[] = { 5, 2, 1, 1, 1, 0, 0, 0, 0, 0 };
				*/
				vara list(f.memory, aztype > 0 ? "title='Caving'" : "title='Canyoneering'");
				for (int i = 1; i < list.length(); ++i)
				{
					vars href = ExtractString(list[i], "href=", "", "</a");
					vars link = "http:" + ExtractString(href, "http", ":", "\"");
					vars name = stripHTML(ExtractString(href, "title=", "'", "'"));
					vars region = stripHTML(ExtractString(list[i], "href=", "</div>", "</div>"));
					CSym sym(urlstr(link), name);
					if (!IsSimilar(state, "WW"))
						sym.SetStr(ITEM_REGION, state + ";" + region.replace(">", ";"));
					if (!UpdateCheck(symlist, sym) && MODE > -2)
						continue;

					// get details
					if (f.Download(link))
					{
						Log(LOGERR, "ERROR: can't download url %.128s", link);
						continue;
					}

					// summary
					vara aca;
					aca.push(stripHTML(ExtractString(f.memory, "class='STP'>Grade", "</td>", "</td>")));
					aca.push(stripHTML(ExtractString(f.memory, "class='STP'>Water", "</td>", "</td>")));
					aca.push(stripHTML(ExtractString(f.memory, "class='STP'>Time", "</td>", "</td>")));
					aca.push(stripHTML(ExtractString(f.memory, "class='STP'>Risk", "</td>", "</td>")));

					const char **check[] = { rtech, rwater, rtime, rxtra };
					for (int a = 0; a < 4; ++a)
						if (!aca[a].IsEmpty() && IsMatch(aca[a], check[a]) < 0)
							Log(LOGERR, "Invalid ACA%d %s", a, aca[0]);
					sym.SetStr(ITEM_ACA, aca.join(";"));

					// distance / time
					vars sdist = stripHTML(ExtractString(f.memory, "class='STP'>Distance", "</td>", "</td>"));
					vars stime = stripHTML(ExtractString(f.memory, "class='STP'>Avg Time", "</td>", "</td>"));
					CDoubleArrayList dist, time;
					if (GetValues(stime, utime, time) || GetValues(stime, NULL, time)) {
						sym.SetNum(ITEM_MINTIME, time.Head());
						sym.SetNum(ITEM_MAXTIME, time.Tail());
					}
					if (GetValues(sdist, udist, dist))
						sym.SetNum(ITEM_HIKE, Avg(dist));

					// season
					vars season = stripHTML(ExtractString(f.memory, "<b>Seasons</b>", "", "</td>"));
					vars preferred = stripHTML(ExtractString(f.memory, "<b>Preferred</b>", "", "</td>"));
					if (!preferred.IsEmpty())
						season += "; BEST in " + preferred;
					if (!season.IsEmpty())
						sym.SetStr(ITEM_SEASON, season);

					// lat lng
					vara ll(ExtractString(f.memory, "gmap.setCenter(", "LatLng(", ")"));
					if (ll.length() != 2)
						Log(LOGERR, "Inconsistent LL %s", ll.join());
					else
					{
						double lat = CDATA::GetNum(ll[0]), lng = CDATA::GetNum(ll[1]);
						if (!CheckLL(lat, lng))
							Log(LOGERR, "Invalid LL %s", ll.join());
						else
						{
							sym.SetNum(ITEM_LAT, lat);
							sym.SetNum(ITEM_LNG, lng);
						}
					}

					// ratings
					double stars = ExtractNum(f.memory, "itemprop='ratingValue'", ">", "<");
					double ratings = ExtractNum(f.memory, "itemprop='reviewCount'", ">", "<");
					if (stars >= 0)
						if (ratings <= 0)
							Log(LOGERR, "Invalid reviewCount=%g", ratings);
						else
							sym.SetStr(ITEM_STARS, starstr(stars, ratings));

					// kml
					vars kmlidx = ExtractString(f.memory, "'Download Route'", "data-gps='", "'");
					sym.SetStr(ITEM_KML, kmlidx.IsEmpty() ? "" : "X");

					// class					
					vars desc = ExtractString(f.memory, "itemprop='text'", ">", "<table");
					CString type = "-1";
					//if (stars>=4.5)
					//	type = "1:Stars";
					if (strstr(desc, "hike") || strstr(desc, "backpack"))
						type = "0:Hike";
					if (strstr(desc, "Slot") || strstr(desc, "slot") || strstr(desc, "fissure"))
						type = "0:Slot";
					if (strstr(desc, " rope") || strstr(desc, " webbing"))
						type = "1:Rope";
					if (strstr(desc, "Rappel") || strstr(desc, "rappel") || strstr(desc, "abseil") || strstr(desc, " rap ") || strstr(desc, " raps "))
						type = "1:Rappel";
					if (!aca[0].IsEmpty())
						type = "1:Summary";
					if (aztype > 0)
						type = "2:Cave";
					vars title = sym.GetStr(ITEM_DESC);
					if (strstr(title.lower(), " to ")) // A to B
						type = "-1:ToHike";
					sym.SetStr(ITEM_CLASS, type);

					// Conditions
					vars id = ExtractString(sym.id, "ZTN=", "", 0);
					if (!id.IsEmpty())
					{
						vars curl = MkString("http://hikearizona.com/TLG_BASE.php?ZTN=%s&UID=0&m=0&_=1472613646479", id);
						if (f.Download(curl))
							Log(LOGERR, "Could not download %s", curl);
						else
						{
							vara list(f.memory, "itemtype='http://schema.org/Review'");
							if (list.length() > 1)
								HIKEAZ_GetCondition(list[1], sym);
						}
					}

					Update(symlist, sym, NULL, FALSE);
					//Log(LOGINFO, "%s", sym.Line());
				}
				printf("Downloading %s %d ...    \r", state, symlist.GetSize());
				start += list.length() - 1;
				if (start > maxstart || maxstart == 0)
					break;
			}
		}
	}

	return TRUE;
}

int HIKEAZ_DownloadConditions(const char *ubase, CSymList &symlist)
{
	const char *urls[] = {
		"http://hikearizona.com/TLG_BASE.php?UID=&O=2&ZTN=&G=&C=12&m=0&_=1472613270790",
		"http://hikearizona.com/TLG_BASE.php?UID=&O=2&ZTN=&G=&C=13&m=0&_=1472613270795",
		NULL
	};

	DownloadFile f;
	// if asked to process full, process 500
	int maxstart = (MODE < -2 || MODE>1) ? 3000 : 1;
	for (int u = 0; urls[u] != NULL; ++u)
		for (int start = 0; start < maxstart; start += 20)
		{
			vars url = urls[u];
			if (start > 0)
				url += MkString("&start=%d", start);
			if (f.Download(url))
			{
				Log(LOGERR, "ERROR: can't download url %.128s", url);
				return FALSE;
			}

			vara list(f.memory, "itemtype='http://schema.org/Review'");
			for (int i = 1; i < list.length(); ++i)
			{
				const char *memory = list[i];
				vars link = ExtractString(memory, "itemprop='url'", "content='", "'");
				vars name = ExtractString(memory, "itemprop='itemReviewed'", "content='", "'");
				if (link.IsEmpty())
					continue;

				CSym sym(urlstr(link), name);
				if (!HIKEAZ_GetCondition(memory, sym))
					continue;
				UpdateCond(symlist, sym, NULL, FALSE);
			}
			//ASSERT(!strstr(f.memory, "=15546"));
		}

	return TRUE;
}


// ===============================================================================================

int SUMMIT_DownloadPage(DownloadFile &f, const char *url, CSym &sym)
{
	// download list of recent additions
	static double ticks;
	Throttle(ticks, 1000);
	if (f.Download(url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}

	/*
	vars memory = f.memory;
	memory.Replace("<strong>","<b>");
	memory.Replace("</strong>","</b>");
	memory.Replace("<STRONG>","<b>");
	memory.Replace("</STRONG>","</b>");
	memory.Replace("<B>","<b>");
	memory.Replace("</B>","</b>");
	*/

	float lat, lng;
	vars latlng = stripHTML(ExtractString(f.memory, ">Lat/Lon:", "", "</p"));
	GetCoords(latlng.replace("/", ","), lat, lng);
	if (CheckLL(lat, lng))
	{
		sym.SetNum(ITEM_LAT, lat);
		sym.SetNum(ITEM_LNG, lng);
	}

	vars region = invertregion(stripHTML(ExtractString(f.memory, ">Location:", "", "</p")), "");
	sym.SetStr(ITEM_REGION, region);


	vars type = "-1:Unknown";
	vars rtype = stripHTML(ExtractString(f.memory, ">Activities:", "", "</p"));
	if (strstr(rtype, "Hiking"))
		type = "0:Hiking";
	if (strstr(rtype, "Scrambling"))
		type = "0:Scrambling";
	if (strstr(rtype, "Climbing"))
		type = "0:Climbing";
	if (strstr(rtype, "Canyoneering"))
		type = "1:Canyoneering";
	sym.SetStr(ITEM_CLASS, type);

	//GetSummary(sym, stripHTML(ExtractString(f.memory, "Difficoltà:", "", "<br")));
	sym.SetStr(ITEM_SEASON, stripHTML(ExtractString(f.memory, ">Season:", "", "</p")));

	return TRUE;
}

int SUMMIT_DownloadBeta(const char *ubase, CSymList &symlist)
{
	//vara recentlist, urllist;
	//const char *canyonlink = "href=\"/canyoning/canyon/";
	//const char *canyondesclink = "/canyoning/canyon-description/";
	//varas months("janvier=Jan,février=Feb,mars=Mar,avril=Apr,mai=May,juin=Jun,juillet=Jul,août=Aug,septembre=Sep,octobre=Oct,novembre=Nov,décembre=Dec");

	DownloadFile f;

	for (int page = 1; TRUE; ++page)
	{
		// download list of recent additions
		CString url = "http://www.summitpost.org/object_list.php?search_in=name_only&order_type=DESC&object_type=9&orderby=object_name&page=" + MkString("%d", page);
		if (f.Download(url))
		{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			return FALSE;
		}

		/*
		memory.Replace("<strong>","<b>");
		memory.Replace("</strong>","</b>");
		memory.Replace("<STRONG>","<b>");
		memory.Replace("</STRONG>","</b>");
		memory.Replace("<B>","<b>");
		memory.Replace("</B>","</b>");
		*/

		vara list(f.memory, "srch_results_lft");
		for (int i = 1; i < list.length(); ++i)
		{
			vars line = ExtractString(list[i], "", "", "</td>");

			vars link = ExtractString(line, "href=", "'", "'");
			vars name = stripHTML(ExtractString(line, "href=", "alt=\"", "\""));
			if (link.IsEmpty() || name.IsEmpty())
				continue;

			CSym sym(burl(ubase, link), name);

			printf("%d/%d %d/%d   \r", page, -1, i, list.length());
			if (!UpdateCheck(symlist, sym) && MODE > -2)
				continue;
			if (SUMMIT_DownloadPage(f, sym.id, sym))
				Update(symlist, sym, NULL, FALSE);
		}

		if (list.length() <= 1)
			break;
	}

	return TRUE;
}


// ===============================================================================================

int CLIMBUTAH_DownloadDetails(DownloadFile &f, CSym &sym, CSymList &symlist)
{
	const char *url = sym.id;
	if (f.Download(url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}

	vara akalist;
	vara aka(vars(f.memory).replace("aka:", "AKA:").replace("Aka:", "AKA:"), ">AKA:");
	for (int i = 1; i < aka.length(); ++i)
		akalist.push(vars(stripHTML(GetToken(aka[i].replace("\n", " ").replace("<br>", "\n").replace("</p>", "\n"), 0, '\n'))).replace("&", ";"));
	sym.SetStr(ITEM_AKA, akalist.join(";"));

	CString desc;
	float lat, lng;
	CString fmemory = CoordsMemory(f.memory);
	if (ExtractCoords(fmemory, lat, lng, desc) > 0)
	{
		sym.SetNum(ITEM_LAT, lat);
		sym.SetNum(ITEM_LNG, lng);
	}

	Update(symlist, sym, NULL, FALSE);
	
	return FALSE;
}

int CLIMBUTAH_DownloadBeta(const char *ubase, CSymList &symlist)
{
	DownloadFile f;
	vars base = "http://www.climb-utah.com/";
	vars url = base;
	if (f.Download(url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}

	vars optsel = ExtractString(f.memory, "", "<select", "</select>");
	vara opt(optsel, "<option");
	for (int o = 1; o < opt.length(); ++o)
	{
		vars url = base + ExtractString(opt[o], "value=", "\"", "\"");
		vars region = ExtractString(opt[o], "value=", ">", "<").trim();
		if (region.IsEmpty())
			continue;
		if (f.Download(url))
		{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			continue;
		}

		vars content = ExtractString(f.memory, "<table", "<table", "</table");
		content = content.split(" Adventure Tales ").first();
		content.Replace("href=\"../WM/saratoga.htm\"", "");

		const char *types[] = {
				"Soaking",
				"Spelunking",
				"Slot Canyon", "Canyoneering", "Peekaboo",
				"Hiking", "Backpacking", "Exploring", "Family", "Picknick",
				//"Roadside Attraction",  "Ruins", "Rock Art", "Petroglyphs", "Climb", "Scrambl",
				//"Hunting",  "Rafting", "Dinosaur", "Salt lake", 
				//"Swimming", "Mining", 
				NULL };
		int typen[] = { 5, 2, 1, 1, 1, 0, 0, 0, 0, 0 };

		vara list(content, "href=");
		for (int i = 1; i < list.length(); ++i)
		{
			vara ids(url, "/");
			ids[ids.length() - 1] = ExtractString(list[i], "", "\"", "\"");
			vars link = ids.join("/");
			vars name = stripHTML(ExtractString(list[i], "", ">", "</a>"));
			vars type = stripHTML(ExtractString(list[i], "", "</a>", "<p>"));
			//ASSERT( strstr(id, "shay.htm")==NULL );
			CSym sym(urlstr(link), name);
			sym.SetStr(ITEM_REGION, region);
			int t = GetClass(type, types, typen);
			SetClass(sym, t, type);
			printf("Downloading %d/%d %d/%d...\r", i, list.length(), o, opt.length());
			if (!Update(symlist, sym, NULL, FALSE) && MODE >= -1)
				continue;
			CLIMBUTAH_DownloadDetails(f, sym, symlist);
		}
	}

	return TRUE;
}


// ===============================================================================================

#define CCHRONICLESTICKS 1000
static double cchroniclesticks;

int CCHRONICLES_ExtractKML(const char *ubase, const char *url, inetdata *out, int fx)
{
	DownloadFile f;
	vars id = GetToken(GetKMLIDX(f, url, "google.com", "msid=", "\""), 0, "&");
	if (id.IsEmpty())
		return FALSE; // not available

	CString credit = " (Data by " + CString(ubase) + ")";
	Throttle(cchroniclesticks, CCHRONICLESTICKS);

	KMZ_ExtractKML(credit, "http://maps.google.com/maps/ms?ie=UTF8&hl=en&msa=0&output=kml&msid=" + id, out);
	
	return TRUE;
}

int CCHRONICLES_DownloadBeta(const char *ubase, CSymList &symlist)
{
	DownloadFile f;

	vars url = "http://canyonchronicles.com";
	if (DownloadRetry(f, url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}

	vara donelist;
	vara regions(f.memory, "<li class=");
	for (int r = 1; r < regions.length(); ++r)
	{
		vars li = ExtractString(regions[r], "", "", "</li");
		vars url = urlstr(ExtractString(li, "href=", "\"", "\""));
		vars region = stripHTML(ExtractString(li, "href=", ">", "</a"));
		if (url.IsEmpty() || donelist.indexOf(url) >= 0)
			continue;
		donelist.push(url);

		Throttle(cchroniclesticks, CCHRONICLESTICKS);
		if (DownloadRetry(f, url))
		{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			continue;
		}

		vara list(f.memory, "entry-title");
		for (int i = 1; i < list.length(); ++i) {
			vars url = "http" + ExtractString(list[i], "http", "", "\"");
			vars name = ExtractString(list[i], "http", ">", "<");
			if (name.IsEmpty() || !strstr(url, "canyonchronicles.com") || strstr(url, ".php") || strstr(url, ".jpg"))
				continue;

			CSym sym(urlstr(url), name);
			sym.SetStr(ITEM_REGION, region);

			printf("Downloading %d/%d %d/%d     \r", i, list.length(), r, regions.length());
			if (!UpdateCheck(symlist, sym) && MODE > -2)
				continue;

			Throttle(cchroniclesticks, CCHRONICLESTICKS);
			if (DownloadRetry(f, url))
			{
				Log(LOGERR, "ERROR: can't download url %.128s", url);
				continue;
			}
			vars kmlidx = GetToken(ExtractString(f.memory, "google.com", "msid=", "\""), 0, "&");
			if (kmlidx.IsEmpty() && strstr(f.memory, "google.com")) kmlidx = "MAP?";
			sym.SetStr(ITEM_KML, kmlidx);
			Update(symlist, sym, NULL, FALSE);
		}
	}
	
	return TRUE;
}


// ===============================================================================================

int KARSTPORTAL_DownloadPage(DownloadFile &f, const char *id, CSymList &symlist, const char *type)
{
	// get coordinates
	CString nurl = MkString("http://WWW.karstportal.org/taxonomy/term/%s", id);
	if (!UpdateCheck(symlist, CSym(nurl)) && MODE > -2)
		return TRUE;
	if (f.Download(nurl))
	{
		//Log(LOGERR, "Can't download from %s", nurl);
		return FALSE;
	}

	CString coords;
	double lat = ExtractNum(f.memory, "\"lat\":", "\"", "\"");
	double lng = ExtractNum(f.memory, "\"lng\":", "\"", "\"");
	if (lat == InvalidNUM || lng == InvalidNUM)
	{
		//Log(LOGERR, "Invalid coord for url %s", nurl); 
		return FALSE;
	}

	vars title = ExtractString(f.memory, "id=\"page-title\"", ">", "<");
	title.Replace("\"", "");
	vars name = stripHTML(GetToken(title, 0, "()")).Trim();
	vars region = invertregion(stripHTML(ExtractString(title, "", "(", ")"))).Trim();

	CSym sym(urlstr(nurl), name);
	sym.SetStr(ITEM_LAT, MkString("@%g;%g", lat, lng));
	sym.SetStr(ITEM_LNG, "");
	sym.SetStr(ITEM_CLASS, type);
	sym.SetStr(ITEM_REGION, region);
	Update(symlist, sym, NULL, FALSE);
	
	return TRUE;
}

int KARSTPORTAL_DownloadBeta(const char *ubase, CSymList &symlist)
{
	vara regions;
	DownloadFile f;

	// download base
	CString url = "http://www.karstportal.org/browse?taxonomy=cave_name";
	if (f.Download(url))
	{
		Log(LOGERR, "Can't download base url %s", url);
		return FALSE;
	}
	vara list(f.memory, "/taxonomy/term/");
	double nmin = 1e16, nmax = -1;
	for (int i = 1; i < list.length(); ++i)
	{
		vara vals;
		double n = CDATA::GetNum(list[i]);
		if (n > 0)
		{
			nmin = min(n, nmin);
			nmax = max(n, nmax);
			printf("%d %d/%d   \r", symlist.GetSize(), i, list.length());
			KARSTPORTAL_DownloadPage(f, MkString("%d", (int)n), symlist, "2:Cave");
		}
	}
	/*
		if (MODE==0)
			{
			for (int i=(int)nmin; i<(int)nmax; ++i) {
				printf("%d %d/%d   \r", symlist.GetSize(), i, (int)(nmax-nmin));
				KARSTPORTAL_DownloadPage(f, MkString("%d",i), symlist, "0:Cave Hidden");
				}
			}
	*/
	
	return TRUE;
}


// ===============================================================================================

int CICARUDECLAN_DownloadPage(DownloadFile &f, const char *url, CSym &sym)
{
	if (f.Download(url))
	{
		Log(LOGERR, "Can't download page %s", url);
		return FALSE;
	}

	vars memory(f.memory);
	memory.Replace("<TD", "<td");
	memory.Replace("</TD", "</td");

	vars summary = stripHTML(ExtractString(strstr(memory, ">Diffic"), "<td", ">", "</td"));
	if (summary.IsEmpty())
		summary = stripHTML(ExtractString(strstr(memory, ">Diffic"), "<td", ">", "</td"));
	GetSummary(sym, summary);

	vars season = UTF8(stripHTML(ExtractString(strstr(memory, ">Period"), "<td", ">", "</td")));
	if (season.IsEmpty())
		season = UTF8(stripHTML(ExtractString(strstr(memory, ">Period"), "<td", ">", "</td")));
	sym.SetStr(ITEM_SEASON, season);

	vars rappels = stripHTML(ExtractString(strstr(memory, ">Calate"), "<td", ">", "</td"));
	if (rappels.IsEmpty())
		rappels = stripHTML(ExtractString(strstr(memory, ">Rope"), "<td", ">", "</td"));
	GetRappels(sym, rappels.replace("nessuna", "0"));

	vars time = stripHTML(ExtractString(strstr(memory, ">Tempi"), "<td", ">", "</td"));
	if (time.IsEmpty())
		time = stripHTML(ExtractString(strstr(memory, ">Duration"), "<td", ">", "</td"));
#if 1
	vara otimes(time, "+"), times;
	vara order("avvicinamento,discesa,rientro");
	for (int i = 0; i < order.length(); ++i)
	{
		vars tval;
		for (int t = 0; t < otimes.length(); ++t)
			if (strstr(otimes[t], order[i]))
				tval = otimes[t].replace(order[i], "");
		times.push(tval);
	}
#else
	time.Replace("avvicinamento", "");
	time.Replace("accesso", "");
	time.Replace("discesa", "");
	time.Replace("rientro", "");
	vara times(time, "+");
#endif
	GetTotalTime(sym, times, url);

	vars shuttle = stripHTML(ExtractString(strstr(memory, ">Navetta"), "<td", ">", "</td"));
	if (shuttle.IsEmpty())
		shuttle = stripHTML(ExtractString(strstr(memory, ">Shuttle"), "<td", ">", "</td"));
	shuttle.Replace(";", ".");
	shuttle.Replace("circa", "");
	shuttle = GetToken(shuttle, 0, 'k').Trim() + " km";
	if (shuttle.Left(1) == "0")
		shuttle = "No";
	sym.SetStr(ITEM_SHUTTLE, shuttle);

	sym.SetStr(ITEM_DEPTH, GetMetric(stripHTML(ExtractString(strstr(memory, ">Dislivello"), "<td", ">", "</td>").replace("circa", "").replace("chilo", "k"))));
	sym.SetStr(ITEM_LENGTH, GetMetric(stripHTML(ExtractString(strstr(memory, ">Lunghezza"), "<td", ">", "</td>").replace("circa", "").replace("chilo", "k"))));
	
	//ASSERT(!strstr(sym.data, "Consusa"));
	
	return TRUE;
}

int CICARUDECLAN_DownloadBeta(const char *ubase, CSymList &symlist)
{
	vara regions;
	DownloadFile f;

	// download base
	vars base = "cicarudeclan.com/ita";
	CString url = burl(base, "schede.htm"); //"http://www.cicarudeclan.com/eng/data.htm";
	if (f.Download(url))
	{
		Log(LOGERR, "Can't download base url %s", url);
		return FALSE;
	}
	vara list(f.memory, "<tr");
	for (int i = 1; i < list.length(); ++i)
	{
		vars region = UTF8(ExtractString(list[i], "swapImage(", "'", "'"));
		vars rurl = ExtractString(list[i], "href=", "\"", "\"");
		if (rurl.IsEmpty() || IsSimilar(rurl, "javascript"))
			continue;

		url = burl(base, rurl);
		if (f.Download(url))
		{
			Log(LOGERR, "Can't download region url %s", url);
			return FALSE;
		}
		// page
		vara list(f.memory, "<img src");
		for (int i = 1; i < list.length(); ++i)
		{
			vars line = list[i].replace("<!--<a href", "<!--<trash");
			vars link = ExtractString(line, "href=", "\"", "\"");
			if (link.IsEmpty())
				continue;

			link = burl(base, link);
			vars name = UTF8(stripHTML(ExtractString(line, "href=", ">", "</a")));

			CSym sym(urlstr(link), name);
			sym.SetStr(ITEM_REGION, region);

			double stars = 0;
			vars pallino = ExtractString(line, "=", "\"", "\"");
			if (strstr(pallino, "red"))
				stars = 1.5;
			if (strstr(pallino, "yellow"))
				stars = 3;
			if (strstr(pallino, "green"))
				stars = 4.5;
			ASSERT(stars > 0);
			if (stars > 0)
				sym.SetStr(ITEM_STARS, starstr(stars, 1));

			if (Update(symlist, sym, NULL, FALSE) || MODE <= -2)
			{
				CICARUDECLAN_DownloadPage(f, link, sym);
				Update(symlist, sym, NULL, FALSE);
			}
		}
	}

	return TRUE;
}


// ===============================================================================================

int SCHLUCHT_DownloadBeta(const char *ubase, CSymList &symlist)
{
	DownloadFile f;

	// download base
	vars url = burl(ubase, "index.php?option=com_canyons&view=category&id=39&format=json");
	if (f.Download(url))
	{
		Log(LOGERR, "Can't download base url %s", url);
		return FALSE;
	}
	vara list(f.memory, "\"Point\"");
	for (int i = 1; i < list.length(); ++i)
	{
		vars line = list[i];
		vars name = ExtractString(line, "\"name\"", "\"", "\"");
		vars link = ExtractString(ExtractString(line, "\"descriptionlink\"", "", ","), "\\\"", "", "\\\"");
		if (link.IsEmpty())
			continue;

		CSym sym(urlstr(burl(ubase, link.replace("\\/", "/"))), name);
		sym.SetStr(ITEM_REGION, "Switzerland");
		sym.SetStr(ITEM_AKA, name);

		vars coords = ExtractString(line, "\"coordinates\"", "[", "]");
		double lng = CDATA::GetNum(GetToken(coords, 0, ','));
		double lat = CDATA::GetNum(GetToken(coords, 1, ','));
		if (!CheckLL(lat, lng))
		{
			Log(LOGERR, "Invalid coordinates %s for %s", coords, sym.id);
			continue;
		}
		sym.SetNum(ITEM_LAT, lat);
		sym.SetNum(ITEM_LNG, lng);

		if (Update(symlist, sym, NULL, FALSE) || MODE <= -2)
		{
			//SCHLUCHT_DownloadBeta(f, link, sym);
			Update(symlist, sym, NULL, FALSE);
		}
	}

	return TRUE;
}

int SCHLUCHT_DownloadConditions(const char *ubase, CSymList &symlist)
{
	DownloadFile f;

	vara urls;
	urls.push("https://www.schlucht.ch/index.php?option=com_canyons&task=displayComments&format=json");

	if (MODE > 0 || MODE < -2)
	{
		CSymList list;
		SCHLUCHT_DownloadBeta(ubase, list);
		for (int i = 0; i < list.GetSize(); ++i)
		{
			vars id = GetToken(list[i].id, 1, "=");
			if (id.IsEmpty())
				continue;
			urls.push("https://www.schlucht.ch/index.php?option=com_canyons&task=displayComments&format=json&cid=" + id);
		}
	}

	for (int u = 0; u < urls.length(); ++u)
	{
		// download base
		if (f.Download(urls[u]))
		{
			Log(LOGERR, "Can't download base url %s", urls[u]);
			return FALSE;
		}
		vara list(f.memory, "\"canyon\"");
		for (int i = 1; i < list.length(); ++i)
		{
			vars line = list[i];
			vars id = ExtractString(line, ":", "\"", "\"");
			vars date = ExtractString(line, "\"comment_date\"", "\"", "\"");
			double vdate = CDATA::GetDate(date, "D.MM.YYYY");
			if (id.IsEmpty() || vdate <= 0)
				continue;

			CSym sym(urlstr("http://schlucht.ch/schluchten-der-schweiz-navbar.html?cid=" + id));
			vara cond; cond.SetSize(COND_MAX);
			cond[COND_DATE] = CDate(vdate);
			cond[COND_LINK] = sym.id;
			sym.SetStr(ITEM_CONDDATE, cond.join(";"));
			UpdateCond(symlist, sym, NULL, FALSE);
		}
	}
	
	return TRUE;
}

int INFO_DownloadBeta(const char *ubase, CSymList &symlist)
{
	//vara recentlist, urllist;
	//const char *canyonlink = "href=\"/canyoning/canyon/";
	//const char *canyondesclink = "/canyoning/canyon-description/";
	//varas months("janvier=Jan,février=Feb,mars=Mar,avril=Apr,mai=May,juin=Jun,juillet=Jul,août=Aug,septembre=Sep,octobre=Oct,novembre=Nov,décembre=Dec");

	// initialize data structures 
	if (rwlist.GetSize() == 0)
		LoadRWList();

	DownloadFile f;
	CString url = "http://www.caudal.info/informe_ropewiki.php";

	// download list of recent additions
	if (f.Download(url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}

	vara rlist(ExtractString(f.memory, "<table", "", "</table"), "<tr");
	for (int r = 2; r < rlist.length(); ++r)
	{
		vara td(rlist[r], "<td");
		if (td.length() < 6)
			continue;

		vars num = stripHTML(ExtractString(td[1], ">", "", "</td"));
		vars date = stripHTML(ExtractString(td[3], ">", "", "</td"));
		double vdate = CDATA::GetDate(date, "DD/MM/YYYY");
		if (vdate <= 0)
		{
			Log(LOGERR, "Invalid date '%s'", date);
			continue;
		}

		vars name = stripHTML(ExtractString(td[4], ">", "", "</td"));
		vars id = urlstr(ExtractHREF(td[5]));
		vars rwlink = urlstr(ExtractHREF(td[6]));
		if (!rwlink.Replace("http://ropewiki.com/", ""))
			continue;
		rwlink.Replace("_", " ");
		if (id.IsEmpty())
		{
			Log(LOGERR, "Invalid infid '%s'", id);
			continue;
		}

		CSym sym(urlstr(id), name);

		for (int i = 0; i < rwlist.GetSize(); ++i)
			if (rwlist[i].GetStr(ITEM_DESC) == rwlink)
			{
				sym.id = RWTITLE + rwlink;
				sym.SetStr(ITEM_MATCH, rwlist[i].id);
				sym.SetStr(ITEM_INFO, rwlist[i].id);
				break;
			}

		//enum { W_DRY=0, W_WADING=1, W_SWIMMING=2, W_VERYLOW=3, W_LOW=4, W_MODLOW=5, W_MODERATE=6, W_MODHIGH=7, W_HIGH=8, W_VERYHIGH=9, W_EXTREME=10 };
		static vara watercc("9=/,0=0,1=1,2=2,3=3,4=4,5=6,6=8,7=9,8=10,9=/");

		//static vara cond_temp("0 - None,1 - Rain jacket,2 - Thin wetsuit,3 - Full wetsuit,4 - Thick wetsuit,5 - Drysuit");
		//static vara tempcc("NA=/,None=0,Waterproof=1,Fleece=1,1-3mm=2,4-5mm=3,6mm+=4,Drysuit=5");

		//static vara cond_diff("0 - Nontechnical,1 - Easy,2 - Normal,3 - Special challenges,4 - Advanced,5 - Extreme");
		//static vara diffcc("NA=/,Easy=1,Moderate=2,Difficult=3,Strenuous=4,Very strenuous=5");

		vara cond;
		cond.SetSize(COND_MAX);
		cond[COND_DATE] = CDate(vdate);
		cond[COND_WATER] = TableMatch(num, watercc, cond_water);
		cond[COND_LINK] = id;
		//cond[COND_TEMP] = TableMatch(stripHTML(ExtractString(memory, ">Thermal:", "", "</br")), tempcc, cond_temp);
		//cond[COND_DIFF] = TableMatch(stripHTML(ExtractString(memory, ">Difficulty:", "", "</br")), diffcc, cond_diff);

		sym.SetStr(ITEM_CONDDATE, cond.join(";"));
		UpdateCond(symlist, sym, NULL, FALSE);
	}

	// avoid "malfunction" message
	for (int i = 0; i < symlist.GetSize(); ++i)
		++symlist[i].index;

	return TRUE;
}


// ===============================================================================================

static double altisudticks;

int ALTISUD_DownloadPage(DownloadFile &f, const char *url, CSym &sym)
{
	Throttle(altisudticks, 1000);
	if (f.Download(url))
	{
		Log(LOGERR, "Can't download base url %s", url);
		return FALSE;
	}

	vars memory(f.memory);
	memory.Replace("<DIV", "<div");
	memory.Replace("</DIV", "</div");

	// Region / Name
	vara regionname;
	vara list(ExtractString(memory, "class=\"chemin\"", ">", "</div"), "<li");
	for (int i = 2; i < list.length(); ++i)
		regionname.push(stripHTML(ExtractString(list[i], "<span", ">", "</span")));
	if (regionname.length() < 1)
	{
		Log(LOGERR, "Inconsistend regionname for %s", url);
		return FALSE;
	}
	int l = regionname.length() - 1;
	vars name = regionname[l];
	name = CString(name[0]).MakeUpper() + name.Mid(1);
	name.Replace(" canyoning", "");
	sym.SetStr(ITEM_DESC, name);
	sym.SetStr(ITEM_AKA, name);

	regionname.RemoveAt(l);
	vars region = regionname.join(";");
	sym.SetStr(ITEM_REGION, region);

	// stars
	vara coeur(memory, "/icon_coeur.gif");
	double stars = coeur.length() - 1;
	if (stars > 0)
		sym.SetStr(ITEM_STARS, starstr(stars, 1));

	// season
	vars season = stripHTML(ExtractString(strstr(memory, ">Periode :"), "<div", ">", "</div"));
	if (!season.IsEmpty())
		sym.SetStr(ITEM_SEASON, season);

	// time
	vars time = stripHTML(ExtractString(strstr(memory, ">Temps :"), "<div", ">", "</div"));
	vara times(time, ":");
	double tmax = 0;
	double tdiv[] = { 1, 60, 60 * 60 };
	for (int i = 0; i < 3; ++i)
	{
		double t = CDATA::GetNum(times[i]);
		if (t != InvalidNUM)
			tmax += t / tdiv[i];
	}
	if (tmax > 0)
		sym.SetNum(ITEM_MINTIME, tmax);

	// rappel
	vars longest = stripHTML(ExtractString(strstr(memory, ">Rappel max :"), "<div", ">", "</div"));
	sym.SetStr(ITEM_LONGEST, longest.replace(" ", ""));

	// shuttle
	vars shuttle = stripHTML(ExtractString(strstr(memory, ">Navette :"), "<div", ">", "</div"));
	shuttle.Replace("Oui", "Yes");
	shuttle.Replace("Non", "No");
	sym.SetStr(ITEM_SHUTTLE, shuttle.replace(" ", ""));

	// coords
	/*
	vars ogps;
	double lat = InvalidNUM, lng = InvalidNUM;
	vars coords = stripHTML(ExtractString(memory, ">point gps", "<span", "</span"));
	ExtractCoords(gps, lat, lng, ogps);
	*/
	double lat = ExtractNum(memory, "\"latitude\"", "=\"", "\"");
	double lng = ExtractNum(memory, "\"longitude\"", "=\"", "\"");
	if (CheckLL(lat, lng))
	{
		sym.SetNum(ITEM_LAT, lat);
		sym.SetNum(ITEM_LNG, lng);
	}

	sym.SetStr(ITEM_DEPTH, GetMetric(stripHTML(ExtractString(strstr(memory, UTF8(">Dénivelé")), "<div", ">", "</div").replace(" ", ""))));
	sym.SetStr(ITEM_LENGTH, GetMetric(stripHTML(ExtractString(strstr(memory, UTF8(">Longueur")), "<div", ">", "</div").replace(" ", ""))));

	return TRUE;
}

int ALTISUD_DownloadBeta(const char *ubase, CSymList &symlist)
{
	vara regions;
	DownloadFile f;

	// download base
	int u = 0;
	vars url;
	vara urllist, visited;
	urllist.push("http://www.altisud.com/canyoning.html");

	while (urllist.GetSize() > u)
	{
		Throttle(altisudticks, 1000);
		if (f.Download(url = urllist[u++]))
		{
			Log(LOGERR, "Can't download base url %s", url);
			return FALSE;
		}
		vars sep = "<a href=\"/canyoning/";
		vara list(f.memory, sep);
		for (int i = 1; i < list.length(); ++i)
		{
			vars link = "http://www.altisud.com/canyoning/" + ExtractString(list[i], "", "", "\"");
			if (visited.indexOf(link) >= 0)
				continue;
			visited.push(link);

			if (!strstr(link, "-fiche.html"))
			{
				urllist.push(link);
				continue;
			}

			// download fiche
			CSym sym(urlstr(link));
			printf("%d %d/%d %d/%d  \r", symlist.GetSize(), u, urllist.length(), i, list.length());
			if (!UpdateCheck(symlist, sym) && MODE > -2)
				continue;
			if (ALTISUD_DownloadPage(f, link, sym))
				Update(symlist, sym, NULL, FALSE);
		}

	}

	return TRUE;
}


// ===============================================================================================

int GUARAINFO_DownloadBeta(const char *ubase, CSymList &symlist)
{
	vara regions;
	DownloadFile f;

	// download base
	vars url = "http://www.guara.info/guara-base-de-datos/descenso-de-barrancos/";
	if (f.Download(url))
	{
		Log(LOGERR, "Can't download base url %s", url);
		return FALSE;
	}
	vars region;
	vars memory = ExtractString(f.memory, "\"" + url + "\"", "", "\"http://www.guara.info/guara-base-de-datos/escalada/\"");
	vara list(memory, "<li");
	for (int i = 1; i < list.length(); ++i)
	{
		vars link = ExtractString(list[i], "href=", "\"", "\"");
		vars name = stripHTML(ExtractString(list[i], "<a ", ">", "</a"));
		if (strstr(list[i], "menu-item-has-children") && !IsSimilar(name, "Barranco"))
		{
			region = name;
			continue;
		}
		if (link.IsEmpty())
			continue;
		if (strstr(link, "wp-json"))
			continue;
		if (!strstr(link, "base-de-datos"))
			continue;

		// download fiche
		CSym sym(urlstr(link), name);
		sym.SetStr(ITEM_REGION, region);
		sym.SetStr(ITEM_AKA, name);

		if (IsSimilar(region, "cuenca"))
			sym.SetStr(ITEM_LAT, "@Sierra de Guara.Huesca.Spain");
		Update(symlist, sym, NULL, FALSE);
	}

	return TRUE;
}


// ===============================================================================================

int TRENCANOUS_DownloadBeta(const char *ubase, CSymList &symlist)
{
	vara regions;
	DownloadFile f;

	// download base
	//vars url = "http://www.latrencanous.com/barrancos/barrancos.html";	
	const char *urls[] = { "http://www.latrencanous.com/?page_id=480", "http://www.latrencanous.com/?page_id=490",
				"http://www.latrencanous.com/?page_id=660", "http://www.latrencanous.com/?page_id=644", NULL };
	for (int u = 0; urls[u] != NULL; ++u)
	{
		vars url = urls[u];
		if (f.Download(url))
		{
			Log(LOGERR, "Can't download base url %s", url);
			return FALSE;
		}
		vars region;
		vars memory = ExtractString(f.memory, "<table", "", "</table");
		vara list(memory, "<tr");
		for (int i = 1; i < list.length(); ++i)
		{
			vara td(list[i], "<td");
			if (td.length() < 3)
				continue;
			vars name = stripHTML(ExtractString(td[1], "", ">", "</td"));

			vars link;
			for (int t = 3; t < td.length() && link.IsEmpty(); ++t)
			{
				vars tlink = ExtractString(td[t], "href=", "\"", "\"");
				if (tlink.IsEmpty())
					continue;
				if (!IsSimilar(tlink, "http"))
					tlink = "http://latrencanous.com/barrancos/" + tlink;
				const char *ext = GetFileExt(tlink);
				if (!ext || stricmp(ext, "pdf") != 0)
					continue;
				if (!strstr(tlink, "latrencanous.com"))
					continue;
				link = tlink;
				break;
			}
			/*
			vars link = ExtractString(td[3], "href=", "\"", "\"");
			if (!link.IsEmpty() && !IsSimilar(link,"http"))
				link = "http://latrencanous.com/barrancos/"+link;
			if (link.IsEmpty() || !strstr(link,ubase))
				{
				link = ExtractString(td[8], "href=", "\"", "\"");
				if (!link.IsEmpty() && !IsSimilar(link,"http"))
					link = "http://latrencanous.com/barrancos/"+link;
				}
			*/
			if (link.IsEmpty())
				continue;

			vars subregion, region;
			region = stripHTML(ExtractString(td[2], "", ">", "</td"));
			if (td.length() > 3)
			{
				subregion = stripHTML(ExtractString(td[3], "", ">", "</td"));
				if (!IsSimilar(subregion, "http"))
					subregion = "";
				if (!subregion.IsEmpty())
					if (region.IsEmpty())
						region = subregion;
					else
						region += ";" + subregion;
			}

			// download fiche
			int ferrata = u > 1;
			if (ferrata) name = "Via Ferrata " + name;
			CSym sym(urlstr(link), name);
			sym.SetStr(ITEM_REGION, region);
			sym.SetStr(ITEM_AKA, name);
			sym.SetStr(ITEM_CLASS, ferrata ? "3:Ferrata" : "1:Canyon");

			printf("%d %d/%d   \r", symlist.GetSize(), i, list.length());
			if (!UpdateCheck(symlist, sym) && MODE > -2)
				continue;
			Update(symlist, sym, NULL, FALSE);
		}
	}

	// MAPA
#if 0
	{
		const char *urls[] = { "http://www.latrencanous.com/?page_id=63", "http://www.latrencanous.com/?page_id=69", NULL };
		for (int u = 0; urls[u] != NULL; ++u)
		{
			vars url = urls[u];
			if (f.Download(url))
			{
				Log(LOGERR, "Can't download base url %s", url);
				return FALSE;
			}

			vars mid = ExtractString(f.memory, "google.com", "mid=", "\"");
			url = "http://www.google.com/maps/d/u/0/kml?mid=" + mid + "&forcekml=1";
			if (f.Download(url))
			{
				Log(LOGERR, "Can't download base url %s", url);
				return TRUE;
			}
			if (!strstr(f.memory, "<Placemark>"))
				Log(LOGERR, "ERROR: Could not download kml %s", url);
			vara list(f.memory, "<Placemark");
			for (int i = 1; i < list.length(); ++i)
			{
				vars oname = ExtractString(list[i], "<name", ">", "</name");
				vars name = stripSuffixes(oname);
				if (name.IsEmpty())
				{
					//Log(LOGWARN, "Could not map %s", oname);
					continue;
				}
				vara coords(ExtractString(list[i], "<coordinates", ">", "</coordinates"));
				if (coords.length() < 2)
					continue;
				double lat = CDATA::GetNum(coords[1]);
				double lng = CDATA::GetNum(coords[0]);
				if (!CheckLL(lat, lng))
				{
					Log(LOGERR, "Invalid KML coords %s from %s", coords.join(), url);
					continue;
				}
				int matched = 0;
				vara idlist;
				vara links(list[i], "http");
				for (int i = 1; i < links.length(); ++i)
				{
					//vars pdflink = GetToken(links[i], 0, "[](){}<> ");
					int len = links[i].Find(".pdf");
					if (len < 0)
						len = links[i].Find(".PDF");
					if (len < 0)
						continue;
					vars pdflink = "http" + links[i].Left(len + 4);
					if (pdflink.IsEmpty() || !strstr(pdflink, "latrencanous.com"))
						continue;
					idlist.push(urlstr(pdflink));
				}

				int maxn = symlist.GetSize();
				for (int n = 0; n < maxn; ++n)
				{
					CSym &sym = symlist[n];
					if (idlist.length() > 0)
					{
						// match by id
						if (idlist.indexOf(sym.id) >= 0)
						{
							sym.SetNum(ITEM_LAT, lat);
							sym.SetNum(ITEM_LNG, lng);
							++matched;
						}
					}
					//if (!matched)
					{
						// match by class and name
						int c = sym.GetNum(ITEM_CLASS) == 3;
						if (u != c)
							continue;
						vars symname = stripSuffixes(sym.GetStr(ITEM_DESC));
						//ASSERT( !strstr(name, "Berche") || !strstr(symname, "Berche"));
						if (!symname.IsEmpty())
							if (IsSimilar(symname, name) || strstr(symname, name) || strstr(name, symname))
							{
								sym.SetNum(ITEM_LAT, lat);
								sym.SetNum(ITEM_LNG, lng);
								++matched;
							}
					}
				}

				if (!matched)
				{
					//Log(LOGERR, "Invalid LTN name match '%s'", name);
				}
			}
		}
	}
#endif

	return TRUE;
}


// ===============================================================================================

int ALTOPIRINEO_DownloadPage(DownloadFile &f, const char *url, CSym &sym)
{
	if (f.Download(url))
	{
		Log(LOGERR, "Can't download base url %s", url);
		return FALSE;
	}
	vars memory = stripAccentsL(f.memory);
	vars season = stripHTML(ExtractString(memory, ">epoca", ":", "</li"));
	sym.SetStr(ITEM_SEASON, season);
	vars longest = stripHTML(ExtractString(memory, ">altura", ":", "</li"));
	longest.Replace("metros", "meters");
	sym.SetStr(ITEM_LONGEST, longest.Trim(" ."));
	vars rapnum = stripHTML(ExtractString(memory, ">numero", ":", "</li"));
	sym.SetNum(ITEM_RAPS, CDATA::GetNum(rapnum));

	double paisaje = CDATA::GetNum(ExtractString(memory, ">paisaje", "alt=\"", "\""));
	double ludico = CDATA::GetNum(ExtractString(memory, ">caracter", "alt=\"", "\""));
	double stars = 0;
	if (paisaje > 0) stars = paisaje;
	if (ludico > paisaje) stars = ludico;
	if (stars > 0)
		sym.SetStr(ITEM_STARS, starstr(stars + 1, 1));

	vara times;
	const char *timelab[] = { ">aproximacion", ">descenso", ">retorno", NULL };
	for (int i = 0; timelab[i] != NULL; ++i)
	{
		vars tstr = stripHTML(ExtractString(memory, timelab[i], ":", "</li"));
		tstr.Replace("horas", "h");
		tstr.Replace("hora", "h");
		tstr.Replace("minutos", "m");
		tstr.Replace("minuto", "m");
		tstr.Replace("&#8242", "m");
		tstr.Replace(" y ", "");
		tstr.Replace("unas", "");
		tstr.Replace("inmediato", "5min");
		tstr.Replace("inmediata", "5min");
		tstr.Trim(" .");
		times.push(tstr);
	}
	
	GetTotalTime(sym, times, url);

	return TRUE;
}

int ALTOPIRINEO_DownloadBeta(const char *ubase, CSymList &symlist)
{
	vara regions;
	DownloadFile f;

	vara urllist("http://altopirineo.com/category/barrancos-pirineos/,http://altopirineo.com/category/barrancos-sierra-de-guara/");

	vars url = "http://altopirineo.com/alto-pirineo/la-base/";
	if (f.Download(url))
	{
		Log(LOGERR, "Can't download base url %s", url);
		return FALSE;
	}
	vara ulist(ExtractString(f.memory, ">Archivos", "<ul", "</ul"), "href=");
	for (int u = 1; u < ulist.length(); ++u)
	{
		vars link = ExtractString(ulist[u], "", "", ">").Trim(" '\"");
		if (!link.IsEmpty())
			urllist.push(link);
	}

	for (int u = 0; u < urllist.length(); ++u)
	{
		// download base
		if (f.Download(url = urllist[u]))
		{
			Log(LOGERR, "Can't download base url %s", url);
			return FALSE;
		}
		vara list(f.memory, "<h4");
		for (int i = 1; i < list.length(); ++i)
		{
			vars link = ExtractString(list[i], "href=", "\"", "\"");
			vara regionname(stripHTML(ExtractString(list[i], "", ">", "</h4")), ".");
			vars name = regionname[0];
			vars region = regionname.length() > 1 ? regionname[1] : "";
			if (link.IsEmpty() || region.IsEmpty())
				continue;

			// download fiche
			CSym sym(urlstr(link), name);
			sym.SetStr(ITEM_REGION, "Spain;" + region);
			printf("%d %d/%d %d/%d  \r", symlist.GetSize(), u, urllist.length(), i, list.length());
			if (!UpdateCheck(symlist, sym) && MODE > -2)
				continue;
			if (ALTOPIRINEO_DownloadPage(f, link, sym))
				Update(symlist, sym, NULL, FALSE);
		}
	}

	return TRUE;
}


// ===============================================================================================

int ZIONCANYON_DownloadBeta(const char *ubase, CSymList &symlist)
{
	vara regions;
	DownloadFile f;
	//const char *region = { "Arizona", "Utah", NULL };

	for (int n = 1; n <= 2; ++n)
	{
		// download base
		CString url = MkString("http://zioncanyoneering.com/api/CanyonAPI/GetCanyonsByState?stateId=%d", n);
		if (f.Download(url))
		{
			Log(LOGERR, "Can't download base url %s", url);
			continue;
		}
		vara list(f.memory, "\"Name\":");
		for (int i = 1; i < list.length(); ++i)
		{
			const char *memory = list[i];
			// add to list
			vars name = ExtractString(memory, "", "\"", "\"");
			vars state = ExtractString(memory, "\"State\":", "\"", "\"");
			vars nurl = "http://zioncanyoneering.com/Canyons/State/" + url_encode(state) + "/" + url_encode(ExtractString(memory, "\"URLName\":", "\"", "\""));

			CSym sym(urlstr(nurl), stripHTML(name));
			if (nurl.IsEmpty() || name.IsEmpty())
				continue;

			double ratings = ExtractNum(memory, "\"NumberOfVoters\":", "", ",");
			double stars = ExtractNum(memory, "\"PublicRating\":", "", ",");
			if (stars != InvalidNUM && ratings != InvalidNUM)
				sym.SetStr(ITEM_STARS, starstr(stars / ratings, ratings));
			vars region = ExtractString(memory, "\"Region\"", "\"", "\"");
			sym.SetStr(ITEM_REGION, state);

			double t = ExtractNum(memory, "\"CanyonRating\":", "", ",");
			vars w = ExtractString(memory, "\"WaterVolumeValue\":", "\"", "\"");
			vars m = ExtractString(memory, "\"TimeCommitmentValue\":", "\"", "\"");
			sym.SetStr(ITEM_ACA, CData(t) + ";" + w + ";" + m);

			Update(symlist, sym, NULL, FALSE);
		}
	}
	
	return TRUE;
}


// ===============================================================================================

/*
South
http://www.kiwicanyons.org/?geo_mashup_content=render-map&map_content=global&width=650&height=500&zoom=6&background_color=c0c0c0&show_future=false&marker_select_info_window=true&marker_select_center=true&marker_select_highlight=false&marker_select_attachments=false&center_lat=-43.612217&center_lng=170.727539&auto_info_open=false
http://www.kiwicanyons.org/?geo_mashup_content=render-map&map_content=global&width=650&height=500&zoom=5&background_color=c0c0c0&show_future=false&marker_select_info_window=true&marker_select_center=true&marker_select_highlight=false&marker_select_attachments=false&center_lat=-41.442726&center_lng=173.496094&auto_info_open=false&marker_min_zoom=8

http://www.kiwicanyons.org/?geo_mashup_content=geo-query&object_name=post&object_ids=867&template=info-window-max
http://www.kiwicanyons.org/?geo_mashup_content=geo-query&object_name=post&object_ids=867  ->
http://www.kiwicanyons.org/?geo_mashup_content=geo-query&object_name=post
http://www.kiwicanyons.org/?p=558
*/

int KIWICANYONS_DownloadBeta(const char *ubase, CSymList &symlist)
{
	//vara recentlist, urllist;
	//const char *canyonlink = "href=\"/canyoning/canyon/";
	//const char *canyondesclink = "/canyoning/canyon-description/";

	DownloadFile f;
	CString kc = "http://www.kiwicanyons.org";
	CString infourl = "http://www.kiwicanyons.org/?p=";
	CString nz = "New Zealand";

	// download map
	CString url = kc + "/?geo_mashup_content=geo-query&object_name=post";
	if (f.Download(url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}
	vara list(f.memory, "{\"object_name\"");
	for (int i = 1; i < list.length(); ++i) {
		vars id = ExtractString(list[i], "\"object_id\"");
		if (id.IsEmpty()) {
			Log(LOGERR, "Invalid id from %s", list[i]);
			continue;
		}
		double lat = ExtractNum(list[i], "\"lat\"");
		double lng = ExtractNum(list[i], "\"lng\"");
		if (!CheckLL(lat, lng)) {
			Log(LOGERR, "Invalid lat/lng for %s from %s", list[i], url);
			continue;
		}
		// split title in 4
		vars title = ExtractString(list[i], "\"title\"");
		vars name, rating, stars;
		vars *dst = &name;
		for (int t = 0; title[t] != 0; ++t) {
			if (title[t] == 'v' && isdigit(title[t + 1]))
				dst = &rating;
			if (title[t] == '*')
				stars += title[t];
			*dst += title[t];
		}
		name.Trim(); rating.Trim(); stars.Trim();
		CSym sym(urlstr(infourl + id));
		sym.SetStr(ITEM_DESC, GetToken(name.replace(",", ";"), 0, '(').Trim());
		sym.SetNum(ITEM_LAT, lat);
		sym.SetNum(ITEM_LNG, lng);
		GetSummary(sym, rating);
		//sym.SetNum(ITEM_LNG, lng);
		vara cat(ExtractString(list[i], "\"categories\"", "[", "]"));
		//"16":{"name":"0 Star Canyons","parent_id":"15","color_name":"black"}, -> 2
		//"15":{"name":"1 Star Canyons","parent_id":"14","color_name":"yellow"}, -> 3
		//"14":{"name":"2 Star Canyons","parent_id":"9","color_name":"orange"}, -> 4
		//"9":{"name":"3 Star Canyons","parent_id":"","color_name":"red"}, -> 5
		//"7":{"name":"Beta-less Canyons","parent_id":"16","color_name":"purple"}, -> 0
		//"17":{"name":"Potential Canyons","parent_id":"7","color_name":"blue"},
		//"63":{"name":"Trip Reports","parent_id":"","color_name":"aqua"},
		//"1":{"name":"Uncategorized","parent_id":"","color_name":"blue"}} });	
		int starv = -1;
		if (cat.indexOf("\"7\"") >= 0) starv = 0;
		if (cat.indexOf("\"16\"") >= 0) starv = 2;
		if (cat.indexOf("\"15\"") >= 0) starv = 3;
		if (cat.indexOf("\"14\"") >= 0) starv = 4;
		if (cat.indexOf("\"9\"") >= 0) starv = 5;
		if (strlen(stars) > 0) {
			int nstarv = strlen(stars) + 2;
			if (nstarv != starv)
				Log(LOGWARN, "Inconsistent stars for %s [%s] => Max(%d,%d) ", title, cat.join(), nstarv, starv);
			if (nstarv > starv)
				starv = nstarv;
			if (starv > 5) {
				Log(LOGWARN, "Inconsistent stars>5 for %s [%s]", title, cat.join());
				starv = 5;
			}
		}
		sym.SetStr(ITEM_REGION, nz);
		if (starv < 0)
			sym.SetNum(ITEM_CLASS, -1); // unclassified
		else
			sym.SetStr(ITEM_STARS, MkString("%s*1", CData(starv)));
		Update(symlist, sym, NULL, FALSE);
	}

	// download list
	//CString url = kc + "/?geo_mashup_content=geo-query&object_name=post";
	url = kc;
	if (f.Download(url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}

	const char *region[2] = { "South Island", "North Island" };
	const char *sep[2] = { "http://www.kiwicanyons.org/legend/test/south-island/", "http://www.kiwicanyons.org/legend/test/north-island/" };
	for (int n = 0; n < 2; ++n)
	{
		vara list(f.memory, sep[n]);
		for (int i = 1; i < list.length(); ++i)
		{
			vars subregion = ExtractString(list[i], "", ">", "<");
			vars urlex = ExtractString(list[i], "", "", "\"");
			if (urlex.IsEmpty())
				continue;

			DownloadFile f;
			vars urlr = sep[n] + urlex;
			if (f.Download(urlr))
			{
				Log(LOGERR, "ERROR: can't download url %.128s", urlr);
				return FALSE;
			}
			char *table = strstr(f.memory, "<table");
			if (!table) {
				Log(LOGERR, "ERROR: No table detected from %.128s", urlr);
				continue;
			}
			vara list(table, infourl);
			for (int i = 1; i < list.length(); ++i)
			{
				vars id = ExtractString(list[i], "", "", "\"");
				if (id.IsEmpty())
				{
					Log(LOGERR, "Invalid id from %s", list[i]);
					continue;
				}
				vara td(list[i], "<td");
				vars name = ExtractString(td[0], "", ">", "<");
				vars rating = ExtractString(td[1], "", ">", "<");
				double stars = ExtractNum(td[2], "", ">", "<");

				CSym sym(urlstr(infourl + id));
				if (!name.IsEmpty())
					sym.SetStr(ITEM_DESC, name.replace(",", ";"));
				GetSummary(sym, rating);
				if (stars >= 0) {
					stars += 2;
					if (stars > 5) {
						Log(LOGWARN, "Inconsistent stars>5 for %s [%s]", CData(stars), sym.data);
						stars = 5;
					}

					sym.SetStr(ITEM_CLASS, "1");
					sym.SetStr(ITEM_STARS, MkString("%s*1", CData(stars)));
				}

				//time & longest
				CDoubleArrayList longest, time;
				vars stime = ExtractString(td[3], "", ">", "<").replace("m", " m").replace("h", " h");;
				if (GetValues(stime, utime, time))
				{
					sym.SetNum(ITEM_MINTIME, time.Head());
					sym.SetNum(ITEM_MAXTIME, time.Tail());
				}
				vars slongest = ExtractString(td[4], "", ">", "<").replace("m", " m");;
				if (GetValues(slongest, ulen, longest))
					sym.SetNum(ITEM_LONGEST, longest.Tail());

				sym.SetStr(ITEM_REGION, nz + ";" + region[n] + ";" + subregion);

				Update(symlist, sym, NULL, FALSE);
			}
		}
	}
	
	return TRUE;
}


// ===============================================================================================

int CANONISMO_DownloadPage(DownloadFile &f, const char *url, CSym &sym)
{
	//ASSERT( !strstr(url, "chuveje") );
	if (f.Download(url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}

	vars name = stripHTML(ExtractString(f.memory, ">Nombre", ":", "</div"));
	vara aka(stripHTML(ExtractString(f.memory, ">Conocido", ":", "</div"))); aka.push(name);
	vars country = stripHTML(ExtractString(f.memory, ">Pa\xC3\xADs", ":", "</div"));
	vars region = stripHTML(ExtractString(f.memory, ">Estado", ":", "</div"));
	sym.SetStr(ITEM_DESC, name);
	sym.SetStr(ITEM_AKA, aka.join(";"));
	sym.SetStr(ITEM_REGION, country + "; " + region);

	vars summary = stripHTML(ExtractString(f.memory, ">Clasificac", ":", "</div"));
	GetSummary(sym, summary);

	vars season = stripHTML(ExtractString(f.memory, ">Temporada", ":", "</div"));
	sym.SetStr(ITEM_SEASON, season);

	// coordenates or geocode
	vars geoloc;
	float lat = InvalidNUM, lng = InvalidNUM;
	vars town = stripHTML(ExtractString(f.memory, ">Poblac", ":", "</div"));
	if (town.IsEmpty())
		town = stripHTML(ExtractString(f.memory, ">Municipio", ":", "</div"));
	if (!town.IsEmpty())
		geoloc = MkString("@%s. %s. %s.", town, region, country);
	vars coords = stripHTML(ExtractString(f.memory, ">De inicio", ":", "</div"));
	if (!coords.IsEmpty())
	{
		coords.Replace("O", "W");
		coords.Replace("\xC2\xB0", "o");
		coords.Replace("\xC2\xBA", "o");
		coords.Replace("\xE2\x80\x9D", "\"");
		GetCoords(coords, lat, lng);
		if (!CheckLL(lat, lng))
		{
			if (IsSimilar(region, "Jalisco"))
				UTM2LL("13Q", GetToken(coords, 0, ' '), GetToken(coords, 1, ' '), lat, lng);
			if (!CheckLL(lat, lng))
				Log(LOGERR, "Invalid coords '%s' from %s", coords, url);
		}
	}
	if (CheckLL(lat, lng))
	{
		sym.SetNum(ITEM_LAT, lat);
		sym.SetNum(ITEM_LNG, lng);
	}
	else
	{
		sym.SetStr(ITEM_LAT, geoloc);
		sym.SetStr(ITEM_LNG, "");
	}

	double longest = CDATA::GetNum(stripHTML(ExtractString(f.memory, ">Rappel", ":", "</div")));
	if (longest >= 0)
		sym.SetStr(ITEM_LONGEST, MkString("%.0fm", longest));

	double raps = CDATA::GetNum(stripHTML(ExtractString(f.memory, ">No. de rappeles", ":", "</div")));
	if (raps >= 0)
		sym.SetNum(ITEM_RAPS, raps);

	vara times;
	const char *ids[] = { ">De acceso<", ">De recorrido<", ">De salida<" };
	for (int t = 0; t < sizeof(ids) / sizeof(*ids); ++t)
	{
		vars time = stripHTML(ExtractString(f.memory, ids[t], ":", "</div"));
		time.Replace("minutos", "m");
		time.Replace("horas", "h");
		time.Replace("hora", "h");
		time.Replace("hrs.", "h");
		time.Replace(" o ", " - ");
		time.Replace(" y ", " ");
		times.push(time);
	}
	GetTotalTime(sym, times, url);

	return TRUE;
}

int CANONISMO_DownloadBeta(const char *ubase, CSymList &symlist)
{
	DownloadFile f;
	vars rurl = burl(ubase, "index.php/canones/canones-mexico");
	if (f.Download(rurl))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", rurl);
		return FALSE;
	}

	vara list(ExtractString(f.memory, "data-options=", "'", "'"), "href=");
	for (int i = 1; i < list.length(); ++i)
	{
		vars link = ExtractString(list[i], "", "\\\"", "\\\"");
		link.Replace("\\", "");
		link.Replace("cau00f1onismo.com", ubase);
		if (link.IsEmpty())
			continue;

		CSym sym(urlstr(link));

		printf("%d %d/%d    \r", symlist.GetSize(), i, list.length());
		if (!UpdateCheck(symlist, sym) && MODE > -2)
			continue;

		if (CANONISMO_DownloadPage(f, sym.id, sym))
			Update(symlist, sym, NULL, FALSE);
	}

	return TRUE;
}


// ===============================================================================================

int CNEWMEXICO_DownloadPage(DownloadFile &f, const char *url, CSym &sym)
{
	//ASSERT( !strstr(url, "chuveje") );
	if (f.Download(url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}

	vars coord = stripHTML(ExtractString(f.memory, "<h1>The Canyon", "coord", "<"));
	int find = coord.indexOf(" N");
	if (find > 0)
	{
		float lat = InvalidNUM, lng = InvalidNUM;
		GetCoords(coord.Mid(find), lat, lng);
		if (CheckLL(lat, lng))
		{
			sym.SetNum(ITEM_LAT, lat);
			sym.SetNum(ITEM_LNG, lng);
		}
	}

	return TRUE;
}

int CNEWMEXICO_DownloadBeta(const char *ubase, CSymList &symlist)
{
	DownloadFile f;
	vars rurl = burl(ubase, "http://canyoneeringnm.org/technical-canyoneering");
	if (f.Download(rurl))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", rurl);
		return FALSE;
	}

	vara rlist(f.memory, "/technical-canyoneering/");
	for (int r = 1; r < rlist.length(); ++r)
	{
		vars link = GetToken(rlist[r], 0, "'\" ><");
		if (link.IsEmpty())
			continue;

		rurl = urlstr("http://canyoneeringnm.org/technical-canyoneering/" + link);
		if (f.Download(rurl))
		{
			Log(LOGERR, "ERROR: can't download url %.128s", rurl);
			return FALSE;
		}

		vara list(ExtractString(f.memory, "<div id=\"main\">", "", "<!-- end main"), "href=");
		for (int i = 1; i < list.length(); ++i)
		{
			vars link = ExtractString(list[i], "\"", "", "\"");
			vars namesum = stripHTML(ExtractString(list[i], ">", "", "<"));
			if (link.IsEmpty())
				continue;

			vars name = GetToken(namesum, 0, "()");
			vars sum = GetToken(namesum, 1, "()");

			CSym sym(urlstr(burl(ubase, link)), name);
			GetSummary(sym, sum);

			if (sym.id == rurl)
				continue;

			printf("%d %d/%d    \r", symlist.GetSize(), r, rlist.length());
			if (!UpdateCheck(symlist, sym) && MODE > -2)
				continue;

			if (CNEWMEXICO_DownloadPage(f, sym.id, sym))
				Update(symlist, sym, NULL, FALSE);
		}
	}

	return TRUE;
}


// ===============================================================================================

int SPHERE_DownloadPage(DownloadFile &f, const char *url, CSym &sym)
{
	//ASSERT( !strstr(url, "chuveje") );
	if (f.Download(url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}

	//vars name = stripHTML(ExtractString(f.memory, "<h2", "", "h2"));
	//vars type = stripHTML(ExtractString(f.memory, ">Feature type:", "</b>", "<br"));
	//sym.SetStr(ITEM_DESC, name);

	vars loc = stripHTML(ExtractString(f.memory, ">Location (UTM):", "</b>", "<br"));
	loc.Replace("Datum: WGS84", "");
	vara coord(loc.Trim(), " ");
	if (coord.length() == 3)
	{
		float lat = InvalidNUM, lng = InvalidNUM;
		UTM2LL(coord[0], coord[1], coord[2], lat, lng);
		if (CheckLL(lat, lng))
		{
			sym.SetNum(ITEM_LAT, lat);
			sym.SetNum(ITEM_LNG, lng);
		}
	}

	return TRUE;
}

int SPHERE_DownloadBeta(const char *ubase, CSymList &symlist)
{
	vara regions;
	DownloadFile f;

	// download base
	CString url = "http://speleosphere.org/index.php?content=spdatabase";
	if (f.Download(url))
	{
		Log(LOGERR, "Can't download url %s", url);
		return FALSE;
	}

	vara deps(ExtractString(f.memory, ">Name of Department", "", "</tr>"), "<option");
	for (int d = 1; d < deps.length(); ++d)
	{
		vars val = ExtractString(deps[d], "value=", "\"", "\"");
		vars dep = stripHTML(ExtractString(deps[d], ">", "", "<"));


		CString url = MkString("http://speleosphere.org/index.php?content=guatesearch&checkdept=on&whatdept=%s&search=Search", val);
		if (f.Download(url))
		{
			Log(LOGERR, "Can't download url %s", url);
			continue;
		}

		vara list(ExtractString(f.memory, ">Cave Name", "", "</table"), "<tr");
		for (int i = 1; i < list.length(); ++i)
		{
			vars link = ExtractHREF(list[i]);
			vars name = stripHTML(UTF8(ExtractString(list[i], "href=", ">", "<")));
			if (link.IsEmpty())
				continue;


			CSym sym(urlstr(burl(ubase, link)), name);
			//vars region = stripHTML(ExtractString(f.memory, ">Department:", "</b>", "<br"));
			vara aka(stripHTML(UTF8(ExtractString(list[i], "Also known as ", "", "<"))));
			aka.push(name);
			sym.SetStr(ITEM_AKA, aka.join(";"));
			sym.SetStr(ITEM_REGION, "Guatemala; " + dep);
			sym.SetStr(ITEM_CLASS, "2:Cave");

			// name transform
			const char *trans[] = {
				"Resurgence=Cueva Resumidero del ,Resurgence of ,Rising of ,Resumidero en el , resurgence",
				"Insurgence=Cueva Sumidero del ,Insurgence of , infeeder, insurgence,sumidero en ",
				NULL
			};
			for (int t = 0; trans[t] != NULL; ++t)
			{
				vars out = GetToken(trans[t], 0, '=');
				vara rep(GetToken(trans[t], 1, '='));
				for (int r = 0; r < rep.length(); ++r)
					if (name.Replace(rep[r], ""))
					{
						sym.SetStr(ITEM_DESC, name.Trim() + " (" + out + ")");
						break;
					}
			}

			printf("%d %d/%d %d/%d   \r", symlist.GetSize(), i, list.length(), d, deps.length());
			if (!UpdateCheck(symlist, sym) && MODE > -2)
				continue;

			if (SPHERE_DownloadPage(f, sym.id, sym))
				Update(symlist, sym, NULL, FALSE);
		}

	}

	return TRUE;
}


// ===============================================================================================

/*
int WIKIPEDIA_DownloadPage(DownloadFile &f, const char *url, CSym &sym)
{
	//ASSERT( !strstr(url, "chuveje") );
	if (f.Download(url))
			{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			return FALSE;
			}

	//vars name = stripHTML(ExtractString(f.memory, "<h2", "", "h2"));
	//vars type = stripHTML(ExtractString(f.memory, ">Feature type:", "</b>", "<br"));
	//sym.SetStr(ITEM_DESC, name);

	vars loc = stripHTML(ExtractString(f.memory, ">Location (UTM):", "</b>", "<br"));
	loc.Replace( "Datum: WGS84", "");
	vara coord(loc.Trim(), " ");
	if (coord.length()==3)
		{
		double lat = InvalidNUM, lng =InvalidNUM;
		UTM2LL(coord[0], coord[1], coord[2], lat, lng);
		if (CheckLL(lat,lng))
			{
			sym.SetNum(ITEM_LAT, lat);
			sym.SetNum(ITEM_LNG, lng);
			}
		}

	return TRUE;
}

int WIKIPEDIA_DownloadBeta(const char *ubase, CSymList &symlist)
{
	vara regions;
	DownloadFile f;

	// download base
	CString url = "http://en.wikipedia.org/wiki/List_of_caves";
	if (f.Download(url))
		{
		Log(LOGERR, "Can't download url %s", url);
		return FALSE;
		}

	vara dep( ExtractString(f.memory, "<h2>Contents", "", "id=\"See_also\""), "<h2");
	for (int d=1; d<deps.length(); ++d)
	{
	vars mreg = stripHTML(ExtractString(deps[d], "id=", "\"", "\""));
	vara reg( deps[d], "<h3");
	for (int r=1; r<reg.length(); ++r)
		{

		}

	CString url = MkString("http://speleosphere.org/index.php?content=guatesearch&checkdept=on&whatdept=%s&search=Search", val);
	if (f.Download(url))
		{
		Log(LOGERR, "Can't download url %s", url);
		continue;
		}

	vara list(ExtractString(f.memory, ">Cave Name", "", "</table"), "<tr");
	for (int i=1; i<list.length(); ++i)
		{
		vars link = ExtractHREF(list[i]);
		vars name = stripHTML(UTF8(ExtractString(list[i], "href=", ">", "<")));
		if (link.IsEmpty())
			continue;


		CSym sym(urlstr(burl(ubase, link)), name);
		//vars region = stripHTML(ExtractString(f.memory, ">Department:", "</b>", "<br"));
		vara aka(stripHTML(UTF8(ExtractString(list[i], "Also known as ", "", "<"))));
		aka.push(name);
		sym.SetStr(ITEM_AKA, aka.join(";"));
		sym.SetStr(ITEM_REGION, "Guatemala; " +dep);
		sym.SetStr(ITEM_CLASS, "2:Cave");

		// name transform
		const char *trans[] = {
			"Resurgence=Cueva Resumidero del ,Resurgence of ,Rising of ,Resumidero en el , resurgence",
			"Insurgence=Cueva Sumidero del ,Insurgence of , infeeder, insurgence,sumidero en ",
			NULL
			};
		for (int t=0; trans[t]!=NULL; ++t)
			{
			vars out = GetToken(trans[t],0,'=');
			vara rep( GetToken(trans[t],1,'='));
			for (int r=0; r<rep.length(); ++r)
				if (name.Replace(rep[r],""))
					{
					sym.SetStr(ITEM_DESC, name.Trim() + " ("+out+")");
					break;
					}
			}

		printf("%d %d/%d %d/%d   \r", symlist.GetSize(), i, list.length(), d, deps.length());
		if (!UpdateCheck(symlist, sym) && MODE>-2)
			continue;

		if (SPHERE_DownloadPage(f, sym.id, sym))
			Update(symlist, sym, NULL, FALSE);
		}

	}

	return TRUE;
}
*/


// ===============================================================================================

int LAOS_DownloadBetaList(const char *url, const char *dep, CSymList &symlist)
{
	DownloadFile f;
	if (f.Download(url))
	{
		Log(LOGERR, "Can't download url %s", url);
		return FALSE;
	}
	//ASSERT( !strstr(dep,  "Vieng Phouka"));
	vars memory(f.memory);
	//memory.Replace("</TABLE", "</table");
	//memory.Replace("<TR", "<tr");
	//memory.Replace("<TD", "<td");
	//memory.Replace("</TD", "</td");
	vara list(ExtractString(memory, "No.<", "", "</TBODY"), "<tr");
	int xlen = 7;
	for (int i = 0; i < list.length(); ++i)
	{
		int header = FALSE;
		vara td(list[i], "<td");
		for (int t = 1; t < td.length(); ++t)
			if (strstr(stripHTML(ExtractString(td[t], ">", "", "</td")), "Length (m)"))
			{
				header = TRUE;
				xlen = t + !i;
				break;
			}
		if (header)
			continue;


		if (td.length() <= 2)
			continue;

		int x = 0;
		if (i > 0 && strstr(list[i - 1], "rowspan=\"2\""))
			x = 1;

		vars name = stripHTML(UTF8(ExtractString(td[2 - x], ">", "", "</td")));
		//name.Replace(" cave)", ")");
		if (name.IsEmpty())
			continue;
		vars link = url + vars("?id=") + name.replace(" ", "_");

		CSym sym(urlstr(link), name);
		sym.SetStr(ITEM_REGION, "Laos; " + vars(dep));
		sym.SetStr(ITEM_CLASS, "2:Cave");

		if (td.length() > 4 - x)
		{
			float lat = InvalidNUM, lng = InvalidNUM;
			vars slat = stripHTML(ExtractString(td[3 - x], ">", "", "</td"));
			vars slng = stripHTML(ExtractString(td[4 - x], ">", "", "</td"));
			GetCoords(slat + " " + slng, lat, lng);
			if (CheckLL(lat, lng))
			{
				sym.SetNum(ITEM_LAT, lat);
				sym.SetNum(ITEM_LNG, lng);
			}
		}
		if (td.length() > xlen - x)
		{
			vars slen = stripHTML(ExtractString(td[xlen - x], ">", "", "</td"));
			double len = CDATA::GetNum(slen);
			if (len != InvalidNUM)
				sym.SetStr(ITEM_LENGTH, CData(len) + "m");
		}

		Update(symlist, sym, NULL, FALSE);
	}
	
	return FALSE;
}

int LAOS_DownloadBeta(const char *ubase, CSymList &symlist)
{
	vara regions;
	DownloadFile f;

	// download base
	CString url = "http://www.laoscaveproject.de/cave_areas.htm";
	if (f.Download(url))
	{
		Log(LOGERR, "Can't download url %s", url);
		return FALSE;
	}

	vara deps(ExtractString(f.memory, ">Cave by Area", "", "</TABLE>"), "<TR");
	for (int d = 1; d < deps.length(); ++d)
	{
		vars val = ExtractHREF(deps[d]);
		vars dep = stripHTML(ExtractString(deps[d], "href=", ">", "<"));

		printf("%d %d/%d    \r", symlist.GetSize(), d, deps.length());
		LAOS_DownloadBetaList(burl(ubase, val), dep, symlist);
	}

	CSymList deepest;
	LAOS_DownloadBetaList(burl(ubase, "deep_and_long.htm"), "", symlist);
	/*
	for (int d=0; d<deepest.GetSize(); ++d)
		for (int i=0; i<symlist.GetSize(); ++i)
			{
			CSym &sym = symlist[i];
			CSym &dsym = deepest[d];
			if (sym.GetStr(ITEM_DESC)!=dsym.GetStr(ITEM_DESC))
				continue;
			sym.id = dsym.id;
			sym.SetNum(ITEM_LENGTH, max(dsym.GetNum(ITEM_LENGTH),sym.GetNum(ITEM_LENGTH)) );
			}

	symlist.Sort();
	for (int d=0; d<deepest.GetSize(); ++d)
			Update(symlist, deepest[d], NULL, FALSE);
	*/

	return TRUE;
}


// ===============================================================================================

int JALISCO_DownloadPage(DownloadFile &f, const char *url, CSym &sym)
{
	//ASSERT( !strstr(url, "chuveje") );
	if (f.Download(url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}
	vars memory = f.memory;
	memory.Replace("\n", " ");
	memory.Replace("\r", " ");
	memory.Replace("\t", " ");
	memory.Replace("&ndash;", "-");
	memory.Replace("&nbsp;", " ");
	while (memory.Replace("  ", " "));

	vars name = stripHTML(ExtractString(memory, ">Nombre", "- ", "<br"));

	vars country = "Mexico";
	vara aka(stripHTML(ExtractString(memory, ">Toponimia", "-", "<br").replace("(conocido tambi&eacute;n como)", "")), ";");
	if (!name.IsEmpty()) aka.push(name);
	sym.SetStr(ITEM_AKA, aka.join(";"));

	if (aka.length() == 0)
		return FALSE; // empty page

	vars region = stripHTML(ExtractString(memory, ">Estado", "-", "<br"));
	sym.SetStr(ITEM_REGION, country + "; " + region);

	vars summary = stripHTML(ExtractString(memory, ">Clasificaci&oacute;n", "", "<br"));
	GetSummary(sym, summary);

	vars season = stripHTML(ExtractString(memory, ">&Eacute;poca", "-", "<br"));
	sym.SetStr(ITEM_SEASON, season);

	//ASSERT( !strstr(url, "nogal") );
	vars shuttle = stripHTML(ExtractString(memory, ">Combinaci", "-", "<br"));
	shuttle.Trim(" .");
	if (!shuttle.IsEmpty())
	{
		if (IsSimilar(shuttle, "No") || strstr(shuttle, "egresa"))
			shuttle = "No !" + shuttle;
		else
			shuttle = "Yes !" + shuttle;
		sym.SetStr(ITEM_SHUTTLE, shuttle);
	}

	// coordenates or geocode
	vars geoloc;
	double lat = InvalidNUM, lng = InvalidNUM;
	vars town = stripHTML(ExtractString(memory, ">Municipio", "-", "<br"));
	if (!town.IsEmpty())
		geoloc = MkString("@%s. %s. %s.", town, region, country);
	/*
	vars coords = stripHTML(ExtractString(memory, ">De inicio", ":", "</div"));
	if (!coords.IsEmpty())
		{
		coords.Replace("O", "W");
		coords.Replace("\xC2\xB0", "o");
		coords.Replace("\xC2\xBA", "o");
		coords.Replace("\xE2\x80\x9D", "\"");
		GetCoords(coords, lat, lng);
		if (!CheckLL(lat,lng))
			{
			if (IsSimilar(region, "Jalisco"))
				UTM2LL("13Q", GetToken(coords,0,' '), GetToken(coords,1,' '), lat, lng);
			if (!CheckLL(lat,lng))
				Log(LOGERR, "Invalid coords '%s' from %s", coords, url);
			}
		}
	*/
	if (CheckLL(lat, lng))
	{
		sym.SetNum(ITEM_LAT, lat);
		sym.SetNum(ITEM_LNG, lng);
	}
	else
	{
		sym.SetStr(ITEM_LAT, geoloc);
		sym.SetStr(ITEM_LNG, "");
	}

	double longest = CDATA::GetNum(stripHTML(ExtractString(memory, "Rapel mas alto", "-", "<br")));
	if (longest >= 0)
		sym.SetStr(ITEM_LONGEST, MkString("%.0fm", longest));

	//double raps = CDATA::GetNum(stripHTML(ExtractString(memory, ">No. de rappeles", ":", "</div")));
	//if (raps>=0)
	//	sym.SetNum(ITEM_RAPS, raps);
	//ASSERT( !strstr(url, "matatlan"));
	//if (strstr(url, "matatlan"))
	//		Log(memory);

	vara times;
	memory.Replace("Aproximaci", "aproximaci");
	memory.Replace("Descenso", "descenso");
	memory.Replace("Retorno", "retorno");
	const char *ids[] = { ">Horario de aproximaci", ">Horario de descenso", ">Horario de retorno" };
	for (int t = 0; t < sizeof(ids) / sizeof(*ids); ++t)
	{
		vars time = stripHTML(ExtractString(memory, ids[t], "-", "<br"));
		time.Replace("minutos", "m");
		time.Replace("horas", "h");
		time.Replace("hora", "h");
		time.Replace("hrs.", "h");
		time.Replace(" o ", " - ");
		time.Replace(" y ", " ");
		times.push(time);
	}
	GetTotalTime(sym, times, url);

	sym.SetStr(ITEM_DEPTH, GetMetric(stripHTML(ExtractString(memory, ">Desnivel", "-", "<br"))));
	sym.SetStr(ITEM_LENGTH, GetMetric(stripHTML(ExtractString(memory, ">Longitud", "-", "<br"))));

	sym.SetStr(ITEM_ROCK, stripHTML(ExtractString(memory, ">Tipo de Roca", "-", "<br")));

	return TRUE;
}

int JALISCO_DownloadBeta(const char *ubase, CSymList &symlist)
{
	DownloadFile f;
	vars rurl = burl(ubase, "");
	if (f.Download(rurl))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", rurl);
		return FALSE;
	}

	vara list(ExtractString(f.memory, "/equipo-de-cantildeonismo.html", "", "/escalada.html"), "href=");
	for (int i = 1; i < list.length(); ++i)
	{
		vars link = ExtractString(list[i], "", "\"", "\"");
		vars name = stripHTML(ExtractString(list[i], ">", "", "</a"));
		if (link.IsEmpty())
			continue;

		link = burl(ubase, link);
		CSym sym(urlstr(link), name);

		printf("%d %d/%d    \r", symlist.GetSize(), i, list.length());
		if (!UpdateCheck(symlist, sym) && MODE > -2)
			continue;

		if (JALISCO_DownloadPage(f, sym.id, sym))
			Update(symlist, sym, NULL, FALSE);
	}

	return TRUE;
}


// ===============================================================================================

int DESCENSO_DownloadBeta(const char *ubase, CSymList &symlist)
{
	DownloadFile f;
	vara urls;
	urls.push(burl(ubase, "barrancos.html"));
	urls.push(burl(ubase, "ferratas/ferratas.html"));

	for (int u = 0; u < urls.length(); ++u)
	{
		vars url = urls[u];
		if (f.Download(url))
		{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			continue;
		}

		printf("%d %d/%d    \r", symlist.GetSize(), u, urls.length());

		// extract page
		if (strstr(f.memory, ">Acceso") && strstr(f.memory, ">Retorno"))
		{
			vars name = stripHTML(ExtractString(f.memory, "<title", ">", "</title"));

			CSym sym(urlstr(url), name);

			/*
				if (!UpdateCheck(symlist, sym) && MODE>-2)
					continue;

				if (JALISCO_DownloadPage(f, sym.id, sym))
			*/
			Update(symlist, sym, NULL, FALSE);
			continue;
		}

		// queue links
		vara list(f.memory, "class=\"telist\"");
		for (int i = 1; i < list.length(); ++i)
		{
			vars link = ExtractString(list[i], "href=", "", ">");
			link = GetToken(link, 0, ' ');
			link.Trim(" \"'");
			if (link.IsEmpty())
				continue;
			if (!IsSimilar(link, "http"))
			{
				vara path(url, "/");
				path[path.length() - 1] = link;
				link = path.join("/");
			}
			link = urlstr(link);
			if (strstr(link, "php.cgi") || strstr(link, "mailto:") || strstr(link, "file:") || strstr(link, ".css"))
				continue;
			if (strstr(link, "/fotos/") || strstr(link, "../") || strstr(link, "/otras.html"))
				continue;
			if (!strstr(link, ubase))
				continue;
			if (urls.indexOf(link) >= 0)
				continue;
			//ASSERT( !strstr(link,"centelles"));
			urls.push(link);
		}
	}

	return TRUE;
}


// ===============================================================================================

//https://www.google.com/maps/d/u/0/viewer?mid=1SXfd6dNXXIVNgnna4bKq_4SOK4c
#define AZORESKML "http://www.google.com/maps/d/u/0/kml?mid=1SXfd6dNXXIVNgnna4bKq_4SOK4c&forcekml=1";

int GetKMLCoords(const char *str, double &lat, double &lng)
{
	vara coords(str);
	lat = lng = InvalidNUM;
	if (coords.length() >= 2)
	{
		lat = CDATA::GetNum(coords[1]);
		lng = CDATA::GetNum(coords[0]);
	}
	
	return CheckLL(lat, lng);
}

int AZORES_DownloadPage(const char *memory, CSym &sym)
{
	vars url = sym.id;

	double lat = InvalidNUM, lng = InvalidNUM;
	if (!GetKMLCoords(ExtractString(memory, "<coordinates", ">", "</coordinates"), lat, lng))
	{
		Log(LOGERR, "Invalid KML Start coords from %s", url);
		return FALSE;
	}
	sym.SetNum(ITEM_LAT, lat);
	sym.SetNum(ITEM_LNG, lng);

	//sym.SetStr(ITEM_SEASON, stripHTML(ExtractString(f.memory, ">Periodo", ":", "<br")));

	GetSummary(sym, stripHTML(ExtractString(memory, "Grade:", "", "<br")));

	sym.SetNum(ITEM_RAPS, CDATA::GetNum(stripHTML(ExtractString(memory, " rappels:", "", "<br"))));
	sym.SetStr(ITEM_LONGEST, GetMetric(stripHTML(ExtractString(memory, " rappel:", "", "<br"))));
	//sym.SetStr(ITEM_LENGTH, GetMetric(stripHTML(ExtractString(f.memory, "Sviluppo:", "", "<br"))));
	sym.SetStr(ITEM_DEPTH, GetMetric(stripHTML(ExtractString(memory, "level:", "", "<br"))));

	//sym.SetStr(ITEM_SHUTTLE, stripHTML(ExtractString(memory, "Navetta:", "", "<br").replace(" ","").replace("Circa","").replace("No/","Optional ").replace(",",".")));

	vars interest = stripHTML(ExtractString(memory, "Quality:", "", "<br"));
	double stars = CDATA::GetNum(interest.replace(";", "."));
	if (stars > 0)
		sym.SetStr(ITEM_STARS, starstr(stars, 1));

	vara times;
	const char *ids[] = { "Access:", "Time:", "Return:" };
	//ASSERT( !strstr(url,"/kanion.php?id=41"));
	for (int t = 0; t < sizeof(ids) / sizeof(*ids); ++t)
	{
		CString time = stripHTML(ExtractString(memory, ids[t], "", "<br"));
		while (!ExtractStringDel(time, "(", "", ")").IsEmpty());
		times.push(vars(time).replace("hours", "h").replace("days", "j").replace(";", "."));
	}
	GetTotalTime(sym, times, url);

	return TRUE;
}

int BOOKAZORES_DownloadKML(CSym &sym, vara &descents, const char *file)
{
	vars url = sym.id;
	vars name = sym.GetStr(ITEM_DESC); //ExtractString(start, "<name", ">", "</name");
	//ASSERT(!strstr(name,"Cedro"));

	vara lines, linelist;
	double slat = sym.GetNum(ITEM_LAT), slng = sym.GetNum(ITEM_LNG);
	double elat = InvalidNUM, elng = InvalidNUM;
	int imatch = -1, match = -1, distn = 0;
	double idist = 500;
	for (int i = 1; i < descents.length(); ++i)
		//if (strstr(descents[i], name))
	{
		double eps = 1e-5;
		double lat = InvalidNUM, lng = InvalidNUM;
		vara coords(ExtractString(descents[i], "<coordinates", ">", "</coordinates"), " ");
		if (coords.length() < 3)
		{
			vars invalid = descents[i];
			continue;
		}
		double dist;
		for (int j = 0; j < 1; ++j)
		{
			if (GetKMLCoords(coords[j], lat, lng))
				if ((dist = Distance(lat, lng, slat, slng)) <= idist)
				{
					distn = dist == idist;
					imatch = i, idist = dist, match = 0;
				}
			if (GetKMLCoords(coords[coords.length() - 1 - j], lat, lng))
				if ((dist = Distance(lat, lng, slat, slng)) <= idist)
				{
					distn = dist == idist;
					imatch = i, idist = dist, match = 1;
				}
		}
	}
	if (distn > 0)
		return FALSE;

	if (imatch < 0)
		return FALSE;

	vara coords(ExtractString(descents[imatch], "<coordinates", ">", "</coordinates"), " ");
	descents.RemoveAt(imatch);

	if (!match)
	{
		for (int i = 0; i < coords.length(); ++i)
			if (GetKMLCoords(coords[i], elat, elng))
				linelist.push(CCoord3(elat, elng));
	}
	else
	{
		for (int i = coords.length() - 1; i >= 0; --i)
			if (GetKMLCoords(coords[i], elat, elng))
				linelist.push(CCoord3(elat, elng));
	}

	if (!CheckLL(elat, elng, NULL))
	{
		Log(LOGERR, "AZORES KML no valid descent for %s", name);
		return FALSE;
	}

	vara styles, points;
	styles.push("start=http://maps.google.com/mapfiles/kml/paddle/grn-blank.png");
	styles.push("end=http://maps.google.com/mapfiles/kml/paddle/red-blank.png");

	points.push(KMLMarker("start", slat, slng, "Start"));
	points.push(KMLMarker("end", elat, elng, "End"));

	lines.push(KMLLine("Descent", NULL, linelist, RGB(0xFF, 0, 0), 5));

	inetfile out(file);
	
	return SaveKML("Azores Canyoning Book", "Desnivel", url, styles, points, lines, &out);
}

int BOOKAZORES_DownloadBeta(const char *ubase, CSymList &symlist)
{
	vars url = AZORESKML;
	DownloadFile f;
	if (f.Download(url))
	{
		Log(LOGERR, "Can't download base url %s", url);
		return TRUE;
	}
	if (!strstr(f.memory, "<Placemark>"))
		Log(LOGERR, "ERROR: Could not download kml %s", url);
	vars memory = f.memory;
	vars placemark = "<Placemark>";
	vara starts(ExtractStringDel(memory, "<Folder>", "", "</Folder>", TRUE, TRUE), placemark);
	vara ends(ExtractStringDel(memory, "<Folder>", "", "</Folder>", TRUE, TRUE), placemark);
	vara descents(ExtractStringDel(memory, "<Folder>", "", "</Folder>", TRUE, TRUE), placemark);

	if (rwlist.GetSize() == 0)
		LoadRWList();

	vara duplist;
	for (int i = 1; i < starts.length(); ++i)
	{
		vars oname = ExtractString(starts[i], "<name", ">", "</name");
		vars name = oname;
		if (duplist.indexOf(name) >= 0)
			name += MkString(" #%d", i);
		duplist.push(name);
		CSym sym(urlstr(MkString("http://ropewiki.com/User:Azores_Canyoning_Book?id=%s.", name.replace(" ", "_"))), oname);
		sym.SetStr(ITEM_AKA, oname);
		sym.SetStr(ITEM_REGION, "Azores");

		// update sym
		if (UpdateCheck(symlist, sym) || MODE <= -2)
			if (AZORES_DownloadPage(starts[i], sym))
				Update(symlist, sym, NULL, FALSE);
	}

	for (int i = 0; i < symlist.GetSize(); ++i)
		symlist[i].SetStr(ITEM_KML, "");

	vara order("Upper,Middle,Lower");
	for (int o = 0; o <= 5; ++o)
	{
		for (int i = 0; i < symlist.GetSize(); ++i)
		{
			// save kml
			CSym &sym = symlist[i];
			vars rwid = ExtractString(sym.GetStr(ITEM_MATCH), RWID, "", RWLINK);
			if (!rwid.IsEmpty() && sym.GetStr(ITEM_KML).IsEmpty())
			{
				int found = rwlist.Find(RWID + rwid);
				if (found < 0)
				{
					Log(LOGERR, "Mismatched RWID %s for %s", rwid, sym.id);
					continue;
				}
				CSym &rwsym = rwlist[found];

				if (o < order.length())
					if (!strstr(rwsym.GetStr(ITEM_DESC), order[o]))
						continue;

				vars file = MkString("%s\\%s.kml", KMLFIXEDFOLDER, rwsym.GetStr(ITEM_DESC));
				if (BOOKAZORES_DownloadKML(sym, descents, file))
					sym.SetStr(ITEM_KML, "X");
				else if (o == 5)
					Log(LOGERR, "AZORES KML invalid matched descent for %s %s,%s", sym.GetStr(ITEM_DESC), sym.GetStr(ITEM_LAT), sym.GetStr(ITEM_LNG));

				if (strstr(rwsym.GetStr(ITEM_KML), "ropewiki.com"))
					DeleteFile(file); // do not overwrite if already exists
			}
		}
	}

	for (int i = 1; i < descents.length(); ++i)
		Log(LOGERR, "Unmatched descent for %s", ExtractString(descents[i], "<name", ">", "</name"));

	return TRUE;
}


// ===============================================================================================

void ReplaceIfDigit(CString &str, const char *match, const char *rmatch)
{
	while (TRUE)
	{
		int pos = str.Find(match);
		if (pos <= 0)
			return;
		if (!isdigit(str[pos - 1]))
			return;
		str.Replace(match, rmatch);
	}
}

int BARRANQUISMO_DownloadPage(DownloadFile &fout, const char *url, CSym &sym)
{
	//Throttle(barranquismoticks, 1000);
	CString memory;
	if (!Download_Save(url, BQNFOLDER, memory))
	{
		Log(LOGERR, "Can't download page %s", url);
		return FALSE;
	}

	//ASSERT(!strstr(url, "cuichat"));
	if (IsImage(url))
	{
		sym.SetStr(ITEM_CONDDATE, "!" + vars(GetFileExt(url)));
		return TRUE;
	}


	vars loc3 = UTF8(stripHTML(ExtractString(memory, "País:", "", "<br")).MakeUpper().Trim());
	vars loc2 = UTF8(stripHTML(ExtractString(memory, "Departamento:", "", "<br")).MakeUpper().Trim());
	vars loc1 = UTF8(stripHTML(ExtractString(memory, "Acceso desde:", "", "<br")).MakeUpper().Trim());

	vars loc;
	double glat = InvalidNUM, glng = InvalidNUM;

	loc1.Replace("DESDE ", "");
	loc1 = GetToken(loc1, 0, ".(,;/").Trim();
	loc2 = GetToken(loc2, 0, ".(,;/").Trim();
	if (sym.GetNum(ITEM_LAT) == InvalidNUM && !loc3.IsEmpty())
		//!!!! Commented out the following lines because of build error with finding _GeoCache -- will need to fix this (Michelle)
		//if (!_GeoCache.Get(loc = loc1 != loc2 ? loc1 + "." + loc2 + "." + loc3 : loc1 + "." + loc3, glat, glng))
		//	if (!_GeoCache.Get(loc = loc1 + "." + loc3, glat, glng))
		//		//if (!_GeoCache.Get(loc = loc2+"."+loc3, glat, glng))
		//			//if (!_GeoCache.Get(loc = GetToken(sym.GetStr(ITEM_LAT), 1, '@'), glat, glng))
		//		Log(LOGERR, "Bad Geocode for %s : '%s'", sym.id, loc);
		if (sym.GetNum(ITEM_LAT) == InvalidNUM && !loc.IsEmpty())
			sym.SetStr(ITEM_LAT, "@" + loc);

	double longest = CDATA::GetNum(stripHTML(ExtractString(memory, "mas largo:", "", "<br")));
	if (longest != InvalidNUM)
		sym.SetNum(ITEM_LONGEST, longest*m2ft);
	vars shuttle = UTF8(stripHTML(ExtractString(memory, "vehículos:", "", "<br")));
	//if (shuttle!=InvalidNUM)
	shuttle.Trim();
	if (!shuttle.IsEmpty())
	{
		const char *no[] = { "no necesari", "inneces", "sin combi", "imposible", "no posible", "no hay combi", NULL };
		const char *opt[] = { "posible", "no impresc", "no obligatori", "opcional", "factible", "uno o dos", "no recomend", "no totalmente", "conveniente", "aconsej", NULL };
		const char *yes[] = { "recomend", "obliga", NULL };
		vars oshuttle = shuttle = stripAccentsL(shuttle);
		shuttle.Replace("sin datos", "");
		shuttle.Replace(" es ", " ");
		shuttle.Replace("vehiculo", "coche");
		shuttle.Replace("solo 1", "1 solo");
		shuttle.Replace("solo un", "1 solo");
		shuttle.Replace("solo se necesita", "1 solo");
		shuttle.Replace("un solo", "1 solo");
		shuttle.Replace("un coche solo", "1 solo coche");
		shuttle.Trim();
		BOOL bno = FALSE, bopt = FALSE, byes = FALSE;
		for (int i = 0; !bno && no[i] != NULL; ++i)
			if (strstr(shuttle, no[i]))
				bno = TRUE, shuttle.Replace(no[i], vars(no[i]).upper());
		if (IsSimilar(shuttle, "No") || IsSimilar(shuttle, "1"))
			bno = TRUE;
		for (int i = 0; !bopt && opt[i] != NULL; ++i)
			if (strstr(shuttle, opt[i]))
				bopt = TRUE, shuttle.Replace(opt[i], vars(opt[i]).upper());
		for (int i = 0; !bopt && yes[i] != NULL; ++i)
			if (strstr(shuttle, yes[i]))
				byes = TRUE, shuttle.Replace(yes[i], vars(yes[i]).upper());
		if (!shuttle.IsEmpty())
		{
			if (bopt && !byes)
				shuttle = "Optional ! " + shuttle;
			else if (bno && !byes)
				shuttle = "No ! " + shuttle;
			else
				shuttle = "Yes !" + shuttle;
		}
		sym.SetStr(ITEM_SHUTTLE, shuttle); //*km2mi
	}

	sym.SetStr(ITEM_SEASON, UTF8(stripHTML(ExtractString(memory, "Epoca:", "", "<br"))));

	//ASSERT(!strstr(sym.id, "_brujas"));
	//ASSERT(!strstr(sym.id, "transit"));

	vars gps = stripHTML(ExtractString(memory, "Coord. GPS", ":", "<font"));
	vars gps2 = stripHTML(ExtractString(memory, "Coord. GPS", "final:", "<font"));
	if (gps.IsEmpty())
		gps = gps2;
	gps.Replace("Sin datos", "");
	gps.Trim();
	if (gps.IsEmpty())
		gps = stripHTML(ExtractString(memory, "UTM", " ", "<"));
	if (sym.GetNum(ITEM_LAT) != InvalidNUM)
		gps = "";

	gps.Replace("Sin datos", "");
	if (!gps.IsEmpty())
	{
		// don't worry about location if already known
		static CSymList bslist;
		if (bslist.GetSize() == 0)
			LoadBetaList(bslist);

		int found = bslist.Find(sym.id);
		if (found >= 0)
		{
			found = rwlist.Find(bslist[found].data);
			if (found >= 0 && rwlist[found].GetNum(ITEM_LAT) != InvalidNUM)
				gps = "";
		}
	}

	if (!gps.IsEmpty())
	{
		char gzone[10] = "30T";
		double gx, gy;
		if (glat != InvalidNUM && glng != InvalidNUM)
			LL2UTM(glat, glng, gx, gy, gzone);

		float lat = InvalidNUM, lng = InvalidNUM;
		gps.Replace(" y ", " ");
		gps.Replace(" - ", " ");
		gps.Replace("/", " ");
		gps.Replace("WGS84", "");
		gps.Replace("WGS 84", "");
		gps.Replace("ED50", "");
		gps.Replace("ED 50", "");
		gps.Replace("EU50", "");
		gps.Replace("MAZANDÚ", "");
		gps.Replace("(Datum European 1979)", "");
		gps.Replace("(Datum European 1979)", "");
		gps.Replace("(ETRS89)", "");
		gps.Replace("grados", "o");
		gps.Replace("&deg;", "o");
		gps.Replace("&ordm;", "o");
		gps.Replace("°", "o");
		gps.Replace("\xba", "o");
		gps.Replace("\x92", "'");
		gps.Replace("\x94", "\"");
		gps.Replace("minutos", "'");
		gps.Replace("segundos", "\"");
		gps.Replace("norte", "N");
		gps.Replace("oeste", "O");
		gps.Replace("este", "E");
		gps.Replace("sur", "S");

		// utm
		if (strstr(gps, "Lat") || strstr(gps, "o") || gps[0] == 'N')
		{
			// LAT/LNG 3
			gps.Replace("Lat.", " ");
			gps.Replace("Lng.", " ");
			gps.Replace("Long.", " ");
			gps.Replace("Lat:", " ");
			gps.Replace("Lng:", " ");
			gps.Replace("Long:", " ");
			gps.Replace("Longitude", " ");
			gps.Replace("LAT:", " ");
			gps.Replace("LWNG:", " ");
			gps.Replace("Latitude", " ");
			gps.Replace("||", " ");
			gps.Replace("O", "W");
			gps.Replace("N;", "N");
			gps.Replace("''", "\"");
			gps.Replace("´", "'");
			gps.Replace("MAZAND\xD8\xA0", " ");
			while (gps.Replace("  ", " "));
			vars ogps;
			gps = gps.split("X").first();
			ExtractCoords(gps.replace(";", "."), lat, lng, ogps);
			if (!CheckLL(lat, lng))
				ExtractCoords(gps.replace(";", " "), lat, lng, ogps);
		}
		else
		{
			// UTM
			if (gps.Replace("X-", ""))
			{
				gps.Replace(" ", "");
				gps.Replace("Y-", " ");
			}
			gps.Replace("=", " ");
			gps.Replace("X:", " ");
			gps.Replace("Y:", " ");
			gps.Replace("X", " ");
			gps.Replace("Y", " ");
			gps.Replace("x:", " ");
			gps.Replace("y:", " ");
			gps.Replace("x", " ");
			gps.Replace("y", " ");
			gps.Replace("m;", " ");
			gps.Replace("UTM", " ");
			gps.Replace("Datum:", " ");
			gps.Replace("Datum", " ");
			gps.Replace(" T67 ", " ");
			gps.Replace(";", " ");
			gps.Replace("WAPOINT:", "");
			if (gps.Replace("31T", " "))
				gps = "31T " + gps;
			while (gps.Replace("  ", " "));
			vara coord(gps.Trim(), " ");
			if (coord.length() == 1) // X-Y
				coord = vara(coord[0], "-");
			int c = 0;
			double z = CDATA::GetNum(coord[c++]);
			if (z == (int)z || z > 180)
			{
				if (z >= 1 && z <= 60)
					if (isdigit(coord[0][strlen(coord[0]) - 1]))
						if (c < coord.length() && strlen(coord[c]) == 1)
							coord[0] += coord[c++];
						else
							coord[0] += UTMLetterDesignator(glat);
				if (z > 60)
					coord.InsertAt(0, gzone);
				if (c < coord.length() - 1)
				{
					static int dzone[] = { 0, 1, -1, 0 };
					double z = CDATA::GetNum(coord[0]);
					vars l = coord[0].Right(1);
					if (!l.IsEmpty() && z >= 1 && z <= 60)
						for (int r = 0; r <= 3 && !(fabs(glat - lat) < 1 && fabs(glng - lng)); ++r)
						{
							UTM2LL(CData(z + dzone[r]) + l, coord[c].replace(".", ""), coord[c + 1].replace(".", ""), lat, lng);
							if (!CheckLL(lat, lng))
								UTM2LL(coord[0], coord[c], coord[c + 1], lat, lng);

						}
				}
			}
		}
		const char *special = "Coordenadas lat; lon ():";
		if (gps.Replace(special, ""))
		{
			gps.Replace(";", ".");
			gps.Trim();
			lat = (float)CDATA::GetNum(GetToken(gps, 0, ' '));
			lng = (float)CDATA::GetNum(GetToken(gps, 1, ' '));
		}
		if (!CheckLL(lat, lng))
		{
			Log(LOGERR, "Invalid coordintates '%s' for %s", gps, url);
		}
		else
		{
			//ASSERT(lat>35 && lat<43 && lng>-10 && lng<5);
			if (fabs(glat - lat) < 1 && fabs(glng - lng) < 1)
			{
				sym.SetNum(ITEM_LAT, lat);
				sym.SetNum(ITEM_LNG, lng);
			}
			else
			{
				vars locgps = "UNKNOWN";
				if (glat != InvalidNUM && glng != InvalidNUM)
					locgps = MkString("%s,%s UTM %s %s %s", CData(glat), CData(glng), gzone, CData(gx), CData(gy));
				Log(LOGERR, "Inconsistent coordinates %s '%s' = %s,%s <> %s = '%s'", sym.id, gps, CData(lat), CData(lng), locgps, loc);
			}
		}
	}

	vara times;
	const char *ids[] = { "Horario de aproximaci", "Horario de descenso:", "Horario de retorno:" };
	const char *ids2[] = { "d'approche:", "de parcours:", "de retour:" };
	//ASSERT(sym.id!="http://barranquismo.net/paginas/barrancos/riu_glorieta.htm");
	//ASSERT(sym.id!="http://barranquismo.net/paginas/barrancos/canyon_de_oilloki.htm");
	for (int i = 0; i < 3; ++i)
	{
		vars time = stripHTML(ExtractString(memory, ids[i], "</b>", "<br"));
		if (time.IsEmpty())
			time = stripHTML(ExtractString(memory, ids2[i], "</b>", "<br"));
		if (time.IsEmpty())
		{
			Log(LOGWARN, "Missing A+D+E times '%s' from %s", ids[i], url);
			continue;
		}

		time.MakeLower();
		time.Replace(" a ", "-");
		time.Replace(" o ", "-");
		time.Replace("y media", ".5h");
		time.Replace("media hora", "0.5h");
		time.Replace("una hora", "1h");
		time.Replace(" y ", "-");
		time.Replace("dos ", "2");
		time.Replace("tres ", "3");
		time.Replace("cuatro ", "4");
		time.Replace("cinco ", "5");
		time.Replace("seis ", "6");
		time.Replace("siete ", "7");
		time.Replace("ocho ", "8");
		time.Replace("diez ", "10");
		time.Replace("de", "");
		time.Replace(" ", "");
		time.Replace("\x92", "'");
		time.Replace("\x45", "'");
		time.Replace("\xb4", "'");
		time.Replace("\x60", "'");
		time.Replace("\xba", "x");
		time.Replace("inmediat", "-5m-");
		time.Replace("imediat", "-5m-");
		time.Replace("nada", "-5m-");
		time.Replace("nihil", "-5m-");
		time.Replace("alpi", "-5m-");
		time.Replace("horas", "h");
		time.Replace("hora", "h");
		time.Replace("min", "m-");
		time.Replace("h.", "h");
		time.Replace("m.", "m-");
		time.Replace("veh", "coche");
		time.Replace("2da", "");
		time.Replace("2x", "");
		time.Replace("1x", "");
		time.Replace("con1", "");
		time.Replace("con2", "");
		time.Replace("1coche", "");
		time.Replace("2coche", "");
		time.Replace("4pers", "");
		ReplaceIfDigit(time, "'30", ".5h");
		ReplaceIfDigit(time, ".30", ".5h");
		ReplaceIfDigit(time, "'5", ".5h");
		ReplaceIfDigit(time, ";5", ".5h");
		ReplaceIfDigit(time, ";30", ".5h");
		ReplaceIfDigit(time, ";", ".");
		time.Replace("1/2", ".5h");
		time.Replace("/", "-");
		time.Replace(";", "-");
		time.Replace("sindatos", "");
		time = GetToken(time, 0, '(');
		times.push(time.Trim());
	}
	if (times.length() == 3)
		GetTotalTime(sym, times, url, 20);

	// length & depth
	int idlen[] = { ITEM_LENGTH, ITEM_DEPTH };
	const char *idslen[] = { "<b>Longitud", "<b>Desnivel" };
	for (int i = 0; i < 2; ++i)
	{
		vars len = GetMetric(stripHTML(ExtractString(memory, idslen[i], "</b>", "<br")));
		if (!len.IsEmpty())
			sym.SetStr(idlen[i], len);
	}

	// Rock
	vars rock = stripAccentsL(stripHTML(UTF8(ExtractString(memory, "<b>Tipo de roca", "</b>", "<br")))).Trim(" .");
	if (rock != "sin datos")
		sym.SetStr(ITEM_ROCK, rock.replace(" y ", ";"));

	// user
	vars cdate = stripHTML(ExtractString(memory, "CREADOR DE PAGINA-->", "<b>", "</b"));
	vars cuser = stripHTML(ExtractString(memory, "CREADOR DE PAGINA-->", "<i>", "</i"));
	if (cuser.IsEmpty())
		cuser = stripHTML(ExtractString(memory, "CREADOR DE PAGINA-->", "por:", "</tr"));
	sym.SetStr(ITEM_CONDDATE, UTF8(cuser + " ; " + cdate));
	
	return TRUE;
}

//http://api.bing.com/qsml.aspx?query=b&maxwidth=983&rowheight=20&sectionHeight=160&FORM=IESS02&market=en-US

int IsUpper(const char *word)
{
	while (*word)
	{
		if (*word >= 'a' && *word <= 'z')
			return FALSE;
		++word;
	}
	
	return TRUE;
}

/*
int istrash(char c)
{
	return c==' ' || c=='\'';
}
*/

static double barranquismoticks;

int BARRANQUISMO_GetName(const char *title, vars &name, vars &aka)
{
	//ASSERT( !strstr(title, "GIRONA"));
	//ASSERT( !strstr(title, "ARGAT"));
	vara uwords;
	vara words = getwords(vars(title).replace(" o ", ";").replace(" O ", ";").replace("\xE2\x80\x99", "'"));
	static vara lower, skip;
	if (lower.length() == 0)
	{
		CSymList list;
		list.Load(filename(TRANSBASIC"SYM"));
		list.Load(filename(TRANSBASIC"PRE"));
		list.Load(filename(TRANSBASIC"POST"));
		list.Load(filename(TRANSBASIC"MID"));
		list.Add(CSym("Cañon ")); // no UTF8
		list.Add(CSym("Cañón ")); // no UTF8

		for (int i = 0; i < list.GetSize(); ++i)
		{
			vars word = list[i].id;
			word = word.MakeUpper().Trim();
			if (!isa(word[0]))
				continue;
			if (list[i].id[0] == ' ')
				if (list[i].id[strlen(list[i].id) - 1] == ' ' || list[i].id[strlen(list[i].id) - 1] == '\'')
					lower.push(word);
			skip.push(word);
		}
	}

	// process '
	for (int i = words.length() - 1; i > 0; --i)
		if (words[i] == "\'")
		{
			words[i - 1] += "\'";
			words.RemoveAt(i);
		}

	for (int i = 0; i < words.length(); ++i)
	{
		vars &word = words[i];
		if (IsUpper(word) || word == ";")
		{
			if (lower.indexOf(word) < 0)
				word = word[0] + word.Mid(1).MakeLower();
			else
				word = word.MakeLower();
			if (skip.indexOf(word.upper()) < 0)
				uwords.push(word);
		}
	}
	vara akaa(words.join(""), ";");
	for (int i = akaa.length() - 1; i >= 0; --i)
		if (skip.indexOf(akaa[i].upper().trim()) >= 0)
			akaa.RemoveAt(i);
	aka = akaa.join(";");

	// trim
	while (uwords.length() > 0 && lower.indexOf(uwords[0].upper()) >= 0)
		uwords.RemoveAt(0);
	while (uwords.length() > 0 && uwords[0] < ' ')
		uwords.RemoveAt(0);
	while (uwords.length() > 0 && uwords[uwords.length() - 1][0] < ' ')
		uwords.RemoveAt(uwords.length() - 1);
	name = uwords.join("").Trim(" ;'");
	name.Replace(";'", ";");
	name.Replace(" '", " ");
	while (name.Replace("  ", " "));
	if (name.IsEmpty())
		name = "(empty)";
	
	return TRUE;
}

int BARRANQUISMOKML_DownloadBeta(const char *ubase, CSymList &symlist)
{
	ubase = "barranquismo.net";

	CFILE file;
	CString memory;
	if (file.fopen(vars(filename("bqn")).replace(".csv", ".kml")))
	{
		const char *line;
		while (line = file.fgetstr())
			memory += line;
		file.fclose();
		vara list(memory, "<Placemark");
		for (int i = 1; i < list.length(); ++i)
		{
			vars name, aka;
			vars title = ExtractString(list[i], "<name>", "", "</name>");
			BARRANQUISMO_GetName(title, name, aka);
			vara coords(ExtractString(list[i], "<coordinates>", "", "</coordinates>"));
			vars msid = GetToken(ExtractString(list[i], "&msid=", "", "&"), 0, "]><");
			const char *str = "http://www.barranquismo.net/paginas";
			vars id = ExtractString(list[i], str, "", "]");
			if (!id.IsEmpty())
				id = str + id;
			else
				id = MkString("http://www.barranquismo.net?mapid=%d", i);
			CSym sym(urlstr(id), title);
			//sym.SetStr(ITEM_AKA, aka);
			sym.SetStr(ITEM_LAT, coords[1]);
			sym.SetStr(ITEM_LNG, coords[0]);
			sym.SetStr(ITEM_KML, msid);
			Update(symlist, sym, NULL, FALSE);
		}
	}
	
	return TRUE;
}

varas BARRANQUISMO_trans(
	"http://barranquismo.net/paginas1/barrancos/canon_de_taourarte.htm=http://barranquismo.net/paginas/barrancos1/canon_de_taourarte.htm,"
	"http://barranquismo.net/paginas1/barrancos/trevelez.htm=http://barranquismo.net/paginas/barrancos1/trevelez.htm,"
	"http://barranquismo.net/paginas/barrancos/barranc_de_ensegur.htn=http://barranquismo.net/paginas/barrancos/barranc_de_ensegur.htm,"
	"http://barranquismo.net/paginas/barrancos/ríu_de_cuesta_yanu.htm=http://www.barranquismo.net/paginas/barrancos/riu_de_cuesta_yanu.htm,"
	"http://barranquismo.net/paginas/informacion/barranc_del_mas_del_coix.htm=http://www.barranquismo.net/paginas/barrancos/barranc_del_mas_del_coix.htm,"
	"http://barranquismo.net/paginas/informacion/barranc_de_la_pinella.htm=http://www.barranquismo.net/paginas/barrancos/barranc_de_la_pinella.htm,"
	"http://barranquismo.net/paginas/informacion/barranc_de_les_olles.htm=http://www.barranquismo.net/paginas/barrancos/barranc_de_les_olles.htm,"
	"http://barranquismo.net/paginas/barranco_del_mas_de_fuentes.htm=http://www.barranquismo.net/paginas/barrancos/barranco_del_mas_de_fuentes.htm,"
);

int BARRANQUISMO_DownloadBeta(const char *ubase, CSymList &symlist, int(*DownloadPage)(DownloadFile &f, const char *url, CSym &sym))
{
	DownloadFile f;
	if (MODE >= -2) // work offline
	{
		for (int i = 0; i < symlist.GetSize(); ++i)
		{
			printf("%d/%d  \r", i, symlist.GetSize());
			CSym sym = symlist[i];
			if (!UpdateCheck(symlist, sym) && MODE > -2)
				continue;
			if (DownloadPage(f, sym.id, sym))
				Update(symlist, sym, NULL, FALSE);
		}
		return TRUE;
	}

	// load kml id,name,coords
	CSymList kmllist;
	kmllist.Sort();

	vara regions;
	{
		// download main list
		vars url = "http://www.barranquismo.net/nuevaspaginas.js";
		if (f.Download(url))
		{
			Log(LOGERR, "Can't download base url %s", url);
			return FALSE;
		}

		int findid = 0, findname = 0;
		vara list(f.memory, "<OPTION");
		for (int i = 1; i < list.length(); ++i)
		{
			vars link = ExtractString(list[i], "", "VALUE=", ">").Trim("\"' ");
			vara name(ExtractString(list[i], "", ">", "\""), ".");
			if (!IsSimilar(link, "http"))
				continue;

			CSym sym(urlstr(link), UTF8(name[0]));
			if (BARRANQUISMO_trans[0].indexOf(sym.id) >= 0)
				sym.id = BARRANQUISMO_trans[1][BARRANQUISMO_trans[0].indexOf(sym.id)];

			//sym.SetStr(ITEM_AKA, UTF8(name + ";" + AKA));
			//sym.SetStr(ITEM_REGION, UTF8(region));
			//sym.SetStr(ITEM_LAT, "@"+UTF8(loc));
			int found = kmllist.Find(sym.id);
			if (found >= 0) ++findid;
			vars title = stripAccents(UTF8(name[0])).replace(" o ", ";");
			//ASSERT(!strstr(sym.GetStr(ITEM_DESC),"gedre"));
			//ASSERT(!strstr(sym.id,"gedre"));
			for (int k = 0; found < 0 && k < kmllist.GetSize(); ++k)
			{
				vars desc = stripAccents(kmllist[k].GetStr(ITEM_DESC)).replace(" o ", ";");
				if (IsSimilar(title, desc) || IsSimilar(desc, title))
					found = k, ++findname;
			}
			if (found > 0)
			{
				// found locations
				sym.SetStr(ITEM_LAT, kmllist[found].GetStr(ITEM_LAT));
				sym.SetStr(ITEM_LNG, kmllist[found].GetStr(ITEM_LNG));
			}

			printf("%d %d/%d  \r", symlist.GetSize(), i, list.length());
			if (!UpdateCheck(symlist, sym))
				continue;
			if (DownloadPage(f, sym.id, sym))
				Update(symlist, sym, NULL, FALSE);
			//ASSERT(!strstr(sym.GetStr(ITEM_DESC),"Corredora"));
			Log(LOGINFO, "NEW REF %s,%s", sym.id, sym.GetStr(ITEM_DESC));
		}

		Log(LOGINFO, "#id:%d #name:%d  #tot:%d  #kmllist:%d ", findid, findname, findid + findname, kmllist.GetSize());
	}

	// download base
	vars url = "http://www.barranquismo.net/BD/cuadro_paises.js";
	if (f.Download(url))
	{
		Log(LOGERR, "Can't download base url %s", url);
		return FALSE;
	}
	vara list(f.memory, "href=");
	for (int i = 1; i < list.length(); ++i)
	{
		vars region = stripHTML(ExtractString(list[i], "", ">", "</a"));
		vars rurl = ExtractString(list[i], "", "", " ").Trim("\"' ");
		if (rurl.IsEmpty())
			continue;

		url = rurl;
		Throttle(barranquismoticks, 1000);
		if (f.Download(url))
		{
			Log(LOGERR, "Can't download region url %s", url);
			return FALSE;
		}
		// page
		vars data = ExtractString(f.memory, "", "<hr", "Encontradas");
		vara dlist(data, "<hr");
		for (int d = 0; d < dlist.length(); ++d)
		{
			//ASSERT( !strstr(dlist[d].lower(), "sorrosal"));
			vara lines(dlist[d], "<font");//.replace("<!--<a href", "<!--<trash");
			if (lines.length() < 4)
				continue;
			vars title = stripHTML(ExtractString(lines[1], "", ">", "</font"));
			vars loc = stripHTML(ExtractString(lines[2], "", ">", "</font"));
			vara links(ExtractString(dlist[d], "", "", "</table"), "href=");
			for (int l = 1; l < links.length(); ++l)
			{
				vars link = GetToken(ExtractString(links[l], "", "", ">"), 0, ' ').Trim("\"' ");
				if (!strstr(link, ubase) || strstr(link, "bibliografia.php") || strstr(link, "/reglamentos/"))
					continue;
				//ASSERT(!strstr(title, "BARRANCO DEL AGUA"));
				vars name, AKA;
				BARRANQUISMO_GetName(title, name, AKA);
				CSym sym(urlstr(link), UTF8(name));
				if (BARRANQUISMO_trans[0].indexOf(sym.id) >= 0)
					sym.id = BARRANQUISMO_trans[1][BARRANQUISMO_trans[0].indexOf(sym.id)];

				sym.SetStr(ITEM_AKA, UTF8(name + ";" + AKA));
				sym.SetStr(ITEM_REGION, UTF8(region));
				sym.SetStr(ITEM_LAT, "@" + UTF8(loc));

				printf("%d %d/%d %d/%d  \r", symlist.GetSize(), i, list.length(), d, dlist.length());
				if (!UpdateCheck(symlist, sym) && MODE > -2)
					continue;
				if (DownloadPage(f, sym.id, sym))
					Update(symlist, sym, NULL, FALSE);
				//ASSERT(!strstr(sym.GetStr(ITEM_DESC),"Corredora"));
			}
		}
	}

	return TRUE;
}

int BARRANQUISMO_DownloadBeta(const char *ubase, CSymList &symlist)
{
	ubase = "barranquismo.net";
	return BARRANQUISMO_DownloadBeta(ubase, symlist, BARRANQUISMO_DownloadPage);
}

int BARRANQUISMODB_DownloadBeta(const char *ubase, CSymList &symlist)
{
	DownloadFile f;

	vara sum; sum.SetSize(R_SUMMARY);
	if (MODE <= -2 || symlist.GetSize() == 0)
	{
		int num = 0;
		// download base
		vars url = "http://www.barranquismo.net/BD/cuadro_paises.js";
		if (f.Download(url))
		{
			Log(LOGERR, "Can't download base url %s", url);
			return FALSE;
		}
		vara list(f.memory, "href=");
		for (int i = 1; i < list.length(); ++i)
		{
			vars region = stripHTML(ExtractString(list[i], "", ">", "</a"));
			vars rurl = ExtractString(list[i], "", "", " ").Trim("\"' ");
			if (rurl.IsEmpty())
				continue;

			url = rurl;
			Throttle(barranquismoticks, 1000);
			if (f.Download(url))
			{
				Log(LOGERR, "Can't download region url %s", url);
				return FALSE;
			}
			// page
			vars data = ExtractString(f.memory, "", "<hr", "Encontradas");
			vara dlist(data, "<hr");
			for (int d = 0; d < dlist.length(); ++d)
			{
				//ASSERT( !strstr(dlist[d].lower(), "sorrosal"));
				vara lines(dlist[d], "<font");//.replace("<!--<a href", "<!--<trash");
				if (lines.length() < 4)
					continue;
				vars loc;
				vars title = stripHTML(ExtractString(lines[1], "", ">", "</font"));
				vara locs(ExtractString(lines[2], "", ">", "</font"), "<br>");
				for (int l = 0; l < locs.length(); ++l)
				{
					locs[l] = stripHTML(locs[l]);
					if (!locs[l].IsEmpty())
						loc = locs[l];
				}

				dlist[d].Replace("HREF=", "href=");
				vara links(dlist[d], "href=");

				vara linklist;
				for (int l = 1; l < links.length(); ++l)
				{
					vars link = GetToken(ExtractString(links[l], "", "", ">"), 0, ' ').Trim("\"' ");
					if (strstr(link, "/reglamentos/"))
						continue;
					//ASSERT(!strstr(title, "BARRANCO DEL AGUA"));
					link = urlstr(link);
					if (BARRANQUISMO_trans[0].indexOf(link) >= 0)
						link = BARRANQUISMO_trans[1][BARRANQUISMO_trans[0].indexOf(link)];
					if (linklist.indexOf(link) < 0)
						linklist.push(link);
					//ASSERT(!strstr(sym.GetStr(ITEM_DESC),"Corredora"));
				}

				//vars name, AKA;
				//BARRANQUISMO_GetName(title, name, AKA);
				//vars id = stripAccents(title);
				CSym sym(MkString("BQN:%d", num++), UTF8(title));

				//sym.SetStr(ITEM_KML, UTF8(title));
				//sym.SetStr(ITEM_AKA, UTF8(name + ";" + AKA));
				sym.SetStr(ITEM_REGION, UTF8(region));
				sym.SetStr(ITEM_LAT, "@" + UTF8(loc));
				//sym.SetStr(ITEM_LNG, UTF8(title));
				sym.SetStr(ITEM_EXTRA, linklist.join(" "));
				sym.SetStr(ITEM_CONDDATE, UTF8(locs.join(";")));

				printf("%d %d/%d %d/%d  \r", symlist.GetSize(), i, list.length(), d, dlist.length());
				Update(symlist, sym, NULL, FALSE);
			}
		}
	}

	CSymList bslist;
	LoadBetaList(bslist);

	const char *GetCodeStr(const char *url);
	for (int i = 0; i < symlist.GetSize(); ++i)
	{
		CSym &sym = symlist[i];

		vars name = sym.GetStr(ITEM_DESC);
		name.Replace(" o ", ";");
		name.Replace(" O ", ";");
		vars Capitalize(const char *oldval);
		sym.SetStr(ITEM_DESC, name);
		sym.SetStr(ITEM_AKA, Capitalize(name));

		// try mapping
		//if (!IsSimilar(sym.GetStr(ITEM_INFO),RWID))
		{
			vara linklist(sym.GetStr(ITEM_EXTRA), " ");

			int b;
			vara codes, nocodes;
			vars matched;
			for (int l = 0; l < linklist.length() && matched.IsEmpty(); ++l)
			{
				vars url, ourl;
				ourl = url = linklist[l];
				if (url == "-")
				{
					linklist.RemoveAt(l--);
					continue;
				}
				if (!IsSimilar(url, "http"))
					continue;
				if (strstr(url, "barranquismo.net/BD/bibliografia"))
					continue;
				//http://canyon.carto.net/cwiki/bin/view/Canyons/?topic=AlmbachklammCanyon
				if (strstr(url, "carto.net"))
				{
					url.Replace("/?topic=", "");
					url.Replace("/view/Canyons", "/view/Canyons/");
					url.Replace("Canyons//", "Canyons/");
				}
				//http://descente-canyon.com/canyoning.php/208-21304-canyons.html 
				//http://descente-canyon.com/canyoning/canyon-description/22332
				if (strstr(url, "descente-canyon.com"))
				{
					vars id = ExtractString(url, ".php/", "-", "-");
					if (id.IsEmpty())
						id = ExtractString(url, ".php/", "/", "/");
					if (strstr(url, "/canyoning/canyon/2"))
						id = ExtractString(url, "/canyoning/canyon/", "", "/");
					if (strstr(url, "reglementation/"))
						id = ExtractString(url, "reglementation/", "", "/");
					if (!id.IsEmpty())
						url = "http://descente-canyon.com/canyoning/canyon-description/" + id;
				}// "http://descente-canyon.com/canyoning/canyon-description/2852"

				if (strstr(url, "ankanionla"))
				{
					url.Replace("/Files/Other", "");
					url.Replace(".pdf", ".htm");
				}

				if (strstr(url, "micheleangileri.com"))
				{
					url.Replace("scheda.cgi", "schedap.cgi");
				}
				if (strstr(url, "ankanionla"))
				{
					url.Replace("ankanionla", "ankanionla-madinina.com");
				}
				if ((b = bslist.Find(url)) >= 0)
				{
					matched = bslist[b].data;
				}
				else
				{
					const char *code = GetCodeStr(url);
					if (code)
						codes.push(code);
					else
						nocodes.push(url);
					//Log(LOGWARN, "Could not map url %s", url);
				}
			}

			sym.SetStr(ITEM_LINKS, nocodes.join(" "));

			// update INFO if needed
			if (!matched.IsEmpty())
				if (!IsSimilar(sym.GetStr(ITEM_INFO), RWID))
					sym.SetStr(ITEM_INFO, matched);

			// separate loc and see if 
			vara region;
			int geoloc = FALSE;
			vars loc = sym.GetStr(ITEM_LAT).Trim("@");
			if (!loc.IsEmpty())
			{
				vara locs(loc.replace(".", ";"), ";");
				for (int l = 0; l < locs.length(); ++l)
				{
					int low = FALSE;
					const char *str = locs[l];
					for (const char *s = str; !low && *s != 0; ++s)
						if (islower(*s))
							low = TRUE;
					// is it lowercase?
					if (!low)
						region.push(str);
					else
						geoloc = TRUE;
				}
				sym.SetStr(ITEM_REGION, invertregion(region.join(";")));
				if (!geoloc)
					sym.SetStr(ITEM_LAT, "");
			}

			vars c = "-1:NoLinks";
			if (codes.length() > 0)
				c = "-2:DupLinks" + codes.join("-");
			if (nocodes.length() > 0)
				c = "0:Links";
			if (geoloc)
				c = "1:Geoloc";
			sym.SetStr(ITEM_CLASS, c);
		}

	}

	return TRUE;
}


// ===============================================================================================

int BARRANCOSORG_DownloadBeta(const char *ubase, CSymList &symlist)
{
	//BARRANQUISMO_DownloadBeta(ubase, symlist, BARRANCOSORG_DownloadPage);

	const char *urls[] = {
		"http://barrancos.org/guara.htm",
		"http://barrancos.org/pirineoh.htm",
		"http://barrancos.org/cataluny.htm",
		"http://barrancos.org/andalucia.htm",
		"http://barrancos.org/Euskadi.htm",
		"http://barrancos.org/Asturias.htm",
		"http://barrancos.org/C_Corcega.htm",
		"http://barrancos.org/Ariege/index.htm",
		"http://barrancos.org/alpes/index.htm",
		"http://barrancos.org/lombardia/index.htm",
		"http://barrancos.org/Madeira/index.htm",
		"http://barrancos.org/Eslovenia/index.htm",
		NULL };

	vars url;
	DownloadFile f;
	for (int u = 0; urls[u] != NULL; ++u)
	{
		if (f.Download(url = urls[u]))
		{
			Log(LOGERR, "Can't download base url %s", url);
			continue;
		}

		vara list(f.memory, "<tr");
		int inota = -1, icuerda = -1, icoches = -1, ihoras = -1;
		for (int i = 1; i < list.length(); ++i)
		{
			vara td(ExtractString(list[i], ">", "", "</tr"), "<td");
			if (i == 1)
			{
				for (int i = 1; i < td.length(); ++i)
				{
					if (strstr(td[i], "Cuerda")) icuerda = i;
					if (strstr(td[i], "Nota")) inota = i;
					if (strstr(td[i], "Coches")) icoches = i;
					if (strstr(td[i], "Horas")) ihoras = i;
				}
			}

			vars link = ExtractString(td[1], "href=", "\"", "\"");
			if (link.IsEmpty())
				link = ExtractString(td[2], "href=", "\"", "\"");
			if (link.IsEmpty())
				continue;
			if (!IsSimilar(link, "http") || strstr(link, ".zip"))
			{
				vara path(url, "/");
				path[path.length() - 1] = link;
				link = path.join("/");
			}
			vars name = stripHTML(ExtractString(td[1], ">", "", "</td"));
			CSym sym(urlstr(link), UTF8(skipnoalpha(name)));

			if (inota > 0)
			{
				if (td.length() <= inota) continue;
				vars nota = stripHTML(ExtractString(td[inota], ">", "", "</td"));
				double stars = CDATA::GetNum(nota);
				if (stars > 0)
					sym.SetStr(ITEM_STARS, starstr(stars / 2, 1));
			}
			if (icuerda > 0)
			{
				double longest = -1;
				vars cuerda = stripHTML(ExtractString(td[icuerda], ">", "", "</td"));
				if (IsSimilar(cuerda, "2x"))
					longest = CDATA::GetNum(GetToken(cuerda, 1, 'x'));
				else if (IsSimilar(cuerda, "1x"))
					longest = CDATA::GetNum(GetToken(cuerda, 1, 'x')) / 2;
				else
					longest = CDATA::GetNum(cuerda);
				if (longest >= 0)
					sym.SetStr(ITEM_LONGEST, MkString("%.0fm", longest));
			}
			if (ihoras > 0)
			{
				vars horas = stripHTML(ExtractString(td[7], ">", "", "</td"));
				if (!horas.IsEmpty())
				{
					if (!horas.Replace("minutos", "'"))
						horas += "h";
					double vmin = 0, vmax = 0;
					GetHourMin(horas, vmin, vmax, url);
					if (vmin > 0)
						sym.SetNum(ITEM_MINTIME, vmin);
					if (vmax > 0)
						sym.SetNum(ITEM_MAXTIME, vmax);
				}
			}

			Update(symlist, sym, NULL, FALSE);
		}

	}

	return TRUE;
}


// ===============================================================================================

int ROPEWIKI_DownloadBeta(const char *ubase, CSymList &symlist)
{
	/*
	CString timestampurl= base+"|%3FModification_date|sort=Modification_date|order=desc|limit=1";
	if (f.Download(timestampurl))
		{
		Log(LOGERR, "ERROR: can't download url %.128s", timestampurl);
		return FALSE;
		}
	double timestamp = ExtractNum(f.memory, "<results>", "<value>", "</value>");
	*/

	// sym
	const char *STAMP0 = "2000-01-01T00:00:00Z";
	CString timestamp = STAMP0;
	CString hdr = GetToken(symlist.header, 0);
	if (IsSimilar(hdr, "TS:"))
		timestamp = hdr.Mid(3);

	vars newtimestamp = ROPEWIKI_DownloadList(timestamp, symlist, "[[Category:Canyons]]", "|%3F" + vara(rwprop, "|").join("|%3F"), ROPEWIKI_DownloadItem);
	if (!newtimestamp.IsEmpty())
		symlist.header = "TS:" + newtimestamp + "," + GetTokenRest(symlist.header, 1);

	// regions
	CSymList regionlist;
	regionlist.Load(filename(RWREGIONS));
	if (!CFILE::exist(filename(RWREGIONS)))
		timestamp = STAMP0;;
	ROPEWIKI_DownloadList(timestamp, regionlist, "[[Category:Regions]]", "|%3FLocated in region|%3FIs major region", rwfregion);
	regionlist.Save(filename(RWREGIONS));

	return TRUE;
}

vars ROPEWIKI_DownloadList(const char *timestamp, CSymList &symlist, const char *q, const char *prop, rwfunc func)
{
	CString query = MkString("%s[[Max_Modification_date::>%s]] OR %s[[Modification_date::>%s]]", q, timestamp, q, timestamp);
	CString querysort = "|sort=Modification_date|order=asc";

	return GetASKList(symlist, query + prop + querysort, func);
}

int ROPEWIKI_DownloadItem(const char *line, CSymList &symlist)
{
	vara labels(line, "label=\"");
	vars id = ExtractString(labels[1], "Has pageid", "<value>", "</value>");
	if (id.IsEmpty()) {
		Log(LOGWARN, "Error empty ID from %.50s", line);
		return FALSE;
	}
	CSym sym(RWID + id);
	vars name = getfulltext(line);
	sym.SetStr(ITEM_DESC, name);
	//ASSERT(!IsSimilar(sym.GetStr(ITEM_DESC), "Goblin"));
	sym.SetStr(ITEM_LAT, ExtractString(line, "lat="));
	sym.SetStr(ITEM_LNG, ExtractString(line, "lon="));
	for (int l = 3, m = ITEM_LNG; l < labels.length(); ++l, ++m) {
		vars val = getlabel(labels[l]);
		switch (m)
		{
		case ITEM_LNG:
			// geolocation
			if (!val.IsEmpty())
				sym.SetStr(ITEM_LAT, sym.GetStr(ITEM_LAT) + "@" + val);
			continue;
		case ITEM_NEWMATCH:
			// trip reports
			if (!val.IsEmpty())
				sym.SetStr(ITEM_MATCH, sym.GetStr(ITEM_MATCH) + ";" + val);
			continue;
		case ITEM_RAPS:
			val.Trim("r");
			break;
		case ITEM_REGION:
			val.Replace("[[", "");
			val.Replace("]]", "");
			val.Replace(":", "");
			val = GetToken(val, 0, '|');
			val = GetToken(val, 1, '(') + ";" + GetToken(val, 0, '(');
			val.Trim(" ;");
			break;
		case ITEM_AKA:
			val = name + (val.IsEmpty() ? "" : "; " + val);
			break;
		default:
			break;
		}

		sym.SetStr(m, val);
	}
	
	//ASSERT(strstr(sym.data, "Snowflake")==NULL);
	// processing
	//GetSummary(sym, stripHTML(skipItalics(sym.GetStr(ITEM_ACA))) );

	Update(symlist, sym, FALSE);

	return TRUE;
}

int ROPEWIKI_DownloadKML(const char *line, CSymList &symlist)
{
	vars file = getfulltext(line);
	const char *ext = GetFileExt(file);
	if (!IsSimilar(file.Right(4), ".kml"))
		return TRUE;

	vars mod = ExtractString(line, "<value>", "", "</value>");
	symlist.Add(CSym(file, mod));
	
	return TRUE;
}

int ROPEWIKI_DownloadKML(CSymList &symlist)
{
	CString timestamp = "1";
	CString hdr = GetToken(symlist.header, 0);
	if (IsSimilar(hdr, "TS:"))
		timestamp = hdr.Mid(3);

	CString query = MkString("[[File:%%2B]][[Modification_date::>%s]]|%%3FModification_date", timestamp);
	timestamp = GetASKList(symlist, query + "|sort=Modification_date|order=asc", ROPEWIKI_DownloadKML);

	if (!timestamp.IsEmpty())
		symlist.header = "TS:" + timestamp + "," + GetTokenRest(symlist.header, 1);
	
	return TRUE;
}

int rwxredirect(const char *line, CSymList &redirects)
{
	vars id = htmltrans(ExtractString(line, "", "pageid=\"", "\""));
	vars name = htmltrans(ExtractString(line, "", "title=\"", "\""));
	if (name.IsEmpty() || id.IsEmpty()) {
		Log(LOGWARN, "Error empty ID/name from %.50s", line);
		return FALSE;
	}
	if (strstri(name, DISAMBIGUATION))
		return FALSE;
	vara aka;
	vara list(line, "<lh ");
	for (int i = 1; i < list.length(); ++i)
		aka.push(ExtractString(list[i], "", "title=\"", "\""));

	CSym sym(RWID + id, name);
	sym.SetStr(ITEM_CLASS, "-1:redirect");
	sym.SetStr(ITEM_AKA, aka.join(";"));
	Update(redirects, sym, FALSE);
	
	return TRUE;
}

int ROPEWIKI_DownloadRedirects(const char *ubase, CSymList &symlist)
{
	CSymList redirects;
	GetAPIList(redirects, "generator=allredirects&garunique&prop=linkshere&lhshow=redirect&lhnamespace=0&garnamespace=0&lhlimit=1000&garlimit=1000", rwxredirect);
	//GetAPIList(redirects, "generator=recentchanges&grcshow=redirect&grctoponly&grcnamespace=0&prop=links&grclimit=1000&grcend="+timestamp, rwxredirect);  Max 30 days!?!
	//GetAPIList(redirects, "generator=allredirects&garnamespace=0&garlimit=100&prop=links&pllimit=100&plnamespace=0", rwxredirect);

	redirects.Sort();
	for (int i = 0; i < redirects.GetSize(); ++i)
	{
		CSym &isym = redirects[i];
		vars title = isym.GetStr(ITEM_DESC);
		for (int j = 0; j < redirects.GetSize(); ++j)
			if (i != j)
			{
				CSym &jsym = redirects[j];
				vara aka(jsym.GetStr(ITEM_AKA), ";");
				if (aka.indexOf(title) < 0)
					continue;

				// merge syms
				aka.push(isym.GetStr(ITEM_AKA));
				jsym.SetStr(ITEM_AKA, aka.join(";"));
				redirects.Delete(i--);
			}
	}

	// always download new
	if (redirects.GetSize() < 1)
		return FALSE;

	symlist = redirects;
	
	return TRUE;
}
