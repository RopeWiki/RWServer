// RoadTripRyan.cpp : source file
//

#include "RoadTripRyan.h"
#include "BETAExtract.h"
#include "trademaster.h"

int ROADTRIPRYAN_ExtractKML(const char *ubase, const char *url, inetdata *out, int fx)
{
	//if (!fx)
	return FALSE;


	DownloadFile f;
	CString credit = " (Data by " + CString(ubase) + ")";
	// requires login, app may fix but rw does not
	/*
	CString login = burl(ubase, "go/login/authenticate?POST?spring-security-redirect=%2F&username=ROADTRIPRYAN_USERNAME&password=ROADTRIPRYAN_PASSWORD");
	if (f.Download(login, "login.htm"))
		{
		Log(LOGERR, "ERROR: can't login roadtripryan.com");
		//return FALSE;
		}
	*/

	vars id = GetKMLIDX(f, url);
	if (id.IsEmpty())
		return FALSE; // not available

	//alternate go/gpx/download/578
	CString url2 = burl(ubase, id);
	if (f.Download(url2))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url2);
		return FALSE;
	}

	return GPX_ExtractKML(credit, url, f.memory, out);
}


int ROADTRIPRYAN_DownloadBeta(const char *ubase, CSymList &symlist2)
{
	DownloadFile f;
	/*
	CString login = burl(ubase, "go/login/authenticate?POST?spring-security-redirect=%2F&username=ROADTRIPRYAN_USERNAME&password=ROADTRIPRYAN_PASSWORD&remember-me=on");
	if (f.Download(login))
		{
		Log(LOGERR, "ERROR: can't login roadtripryan.com");
		//return FALSE;
		}
	*/
	// ALL map 
	CSymList symlist(symlist2);
	ROADTRIPRYAN_DownloadXML(f, "http://www.roadtripryan.com/maps/mapdata.xml", symlist);

	CString url = burl(ubase, "/go/trips/");
	if (f.Download(url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}

	// tables for ratings
	vara list(f.memory, "border:0px solid red");
	for (int i = 1; i < list.length(); ++i) {
		CString data = list[i].split("</td").first();
		vara jlist(data, "<a href=\"");
		for (int j = 1; j < jlist.length(); ++j) {
			CString url = burl(ubase, GetToken(jlist[j], 0, '\"'));
			if (f.Download(url))
			{
				Log(LOGERR, "ERROR: can't download url %.128s", url);
				continue;
			}

			// get star rates
			vara tables(f.memory, "<table cellpadding=\"0\"");
			for (int t = 1; t < tables.length(); ++t) {
				vara rows(tables[t].split("</table").first(), "<tr");
				for (int r = 2; r < rows.length(); ++r) {
					const char *row = rows[r];
					CString link;
					GetSearchString(row, "<a href=", link, 0, '>');
					link = link.Trim("\"' ");
					//GetSearchString(row, "</a", summary, "<td>", "</td");
					double stars = ExtractNum(row, "class=\"stars\"", ">", "<");
					double ratings = ExtractNum(row, "class=\"stars\"", "(", " Rating");
					if (link.IsEmpty())
					{
						Log(LOGERR, "ERROR: invalid name/link %s from %.128s", row, url);
						continue;
					}
					link = burl(ubase, link);
					CSym sym(urlstr(link));
					//sym.SetStr(ITEM_DESC, name);
					if (stars == InvalidNUM || ratings == InvalidNUM)
						Log(LOGWARN, "WARNING: No star rating for %s", link);
					else
						sym.SetStr(ITEM_STARS, starstr(stars, ratings));
					//sym.SetStr(ITEM_ACA, summary);
					Update(symlist, sym, NULL, FALSE);
				}
			}

			/*
			// map
			CString xml;
			GetSearchString(f.memory, "/go/map/iframe?data", xml, "=", "\"");
			if (xml.IsEmpty())
				{
				Log(LOGERR, "ERROR: can't finx xml link in url %.128s", url);
				continue;
				}
			ROADTRIPRYAN_DownloadXML(rr + xml, symlist);
			*/
		}

	}

	for (int i = 0; i < symlist.GetSize(); ++i)
		Update(symlist2, symlist[i]);
	return TRUE;
}


int ROADTRIPRYAN_DownloadXML(DownloadFile &f, const char *url, CSymList &symlist) {
	if (f.Download(url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}
	vara markers(f.memory, "<marker ");
	for (int m = 1; m < markers.length(); ++m) {
		vars &marker = markers[m];
		CString link, name, action, summary, len, kmlidx;
		GetSearchString(marker, "url=", link, '\"', '\"');
		GetSearchString(marker, "name=", name, '\"', '\"');
		GetSearchString(marker, "type=", action, '\"', '\"');
		GetSearchString(marker, "rating=", summary, '\"', '\"');
		//GetSearchString(marker, "length=", len, '\"', '\"');
		if (name.IsEmpty() || link.IsEmpty() || summary.IsEmpty() || action.IsEmpty())
		{
			Log(LOGERR, "ERROR: invalid name/link %s from %.128s", marker, url);
			continue;
		}
		double vlat = ExtractNum(marker, "lat=", "\"", "\"");
		double vlng = ExtractNum(marker, "lng=", "\"", "\"");
		if (!CheckLL(vlat, vlng, NULL))
		{
			Log(LOGERR, "ERROR: invalid ll %s from %.128s", marker, url);
			continue;
		}

		CSym sym(urlstr(link));
		CString nameregion = stripHTML(name), n = GetToken(nameregion, 0, '-').Trim(), r = GetToken(nameregion, 1, '-').Trim();
		sym.SetStr(ITEM_DESC, n);
		sym.SetStr(ITEM_REGION, r);
		GetSummary(sym, stripHTML(summary));
		sym.SetNum(ITEM_LAT, vlat);
		sym.SetNum(ITEM_LNG, vlng);

		// set class
		int typen[] = { 2, 3, 1, 0, -1 };
		const char *types[] = { "Cave", "Ferrata", "Canyon", "Hike", NULL };
		int a = GetClass(action, types, typen);
		if (strstr(n, "Ferrata")) a = 3;
		SetClass(sym, a, action);
		//GetTimeDistance(sym, len);
		CSym osym(sym.id);
		if (!UpdateCheck(symlist, osym) && osym.GetNum(ITEM_CLASS) >= sym.GetNum(ITEM_CLASS) && MODE >= -1)
			continue;
		if (!Update(symlist, sym, NULL, FALSE) && MODE >= -1)
			continue;
		ROADTRIPRYAN_DownloadDetails(f, link, symlist);
	}
	return TRUE;
}


int ROADTRIPRYAN_DownloadDetails(DownloadFile &f, const char *url, CSymList &symlist) {
	if (f.Download(url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}
	const char *data = f.memory;



	CSym sym(urlstr(url));
	CString summary = stripHTML(ExtractString(data, "id=\"beta_rating\"", ":", "</div>"));
	CString time = stripHTML(ExtractString(data, "id=\"beta_length\"", ":", "</div>"));
	time.Replace("Approach - 1 hour. Headless Hen - 2 hours; Raven - 2-3 hours; Exit - 1 hour.", "4-5 hours");
	CString raps = stripHTML(ExtractString(data, "id=\"beta_rappels\"", ":", "</div>"));
	raps.Replace(" - ", " to ");
	raps.Replace(" to ", " upto ");
	raps.Replace("None", "0");
	raps.Replace("1-60 m", "1 upto 60 m");
	raps.Replace("1-20 m", "1 upto 20 m");
	CString season = stripHTML(ExtractString(data, "id=\"beta_season\"", ":", "</div>"));
	season.Replace("to pass", "");

	CString kmlidx = "X"; //ExtractLink(data, "gpx");
	//ASSERT(strstr(url,"/five-mile")==NULL);
	//ASSERT(!strstr(sym.id,"wild-horse"));
	GetSummary(sym, summary);
	// fix ratings for hikes / scrambles
	vars summtext = sym.GetStr(ITEM_ACA);
	vara summ(summtext, ";");
	if (sym.GetNum(ITEM_CLASS) == 0)
		if (summ.length() > 1 && summ[0].IsEmpty())
		{
			if (strstr(summtext, "Class") || strstr(summtext, "crambl"))
				summ[0] = '2';
			else
				summ[0] = '1';
			if (summ[1].IsEmpty())
				summ[1] = "A";
			sym.SetStr(ITEM_ACA, summ.join(";"));
		}


	GetTimeDistance(sym, time);
	GetRappels(sym, raps);
	sym.SetStr(ITEM_KML, kmlidx);
	sym.SetStr(ITEM_SEASON, season);
	return Update(symlist, sym, NULL, FALSE);
}
