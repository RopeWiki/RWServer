// BluuGnome.cpp : source file
//

#include "BluuGnome.h"
#include "BETAExtract.h"
#include "trademaster.h"

int BLUUGNOME_ExtractKML(const char *ubase, const char *url, inetdata *out, int fx)
{
	CString credit = " (Data by " + CString(ubase) + ")";
	DownloadFile f;
	if (DownloadRetry(f, url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}

	vars data = xmlval(f.memory, "<table", "</table");

	vara lines;
	CSymList list;
	vara sec = data.split("font-size: 170%");
	if (sec.length() < 2)
	{
		Log(LOGERR, "Invalid BLUUGNOME sec page %.128s", url);
		return FALSE;
	}

	for (int s = 1; s < sec.length(); ++s)
	{
		vara linelist;
		CString title = stripHTML(xmlval(sec[s], "<strong", "</strong"));
		int color = DESCENT;
		if (title.Find("Exit") > 0)
			color = EXIT;
		if (title.Find("Approach") > 0)
			color = APPROACH;
		if (title.Find("Canyon") > 0)
			color = RGB(0xFF, 0x00, 0x00);
		if (title.Find("Drive") > 0)
			color = ROAD;
		if (title.Find("Descent") > 0)
			color = RGB(0xFF, 0x00, 0x00);


		vara li = sec[s].split("<li");
		for (int i = 1; i < li.length(); ++i) {
			vara li3 = li[i].split("<strong>");
			if (li3.length() < 4)
			{
				Log(LOGERR, "Invalid li3 (%d!=3) %s", li3.length(), li[i]);
				continue;
			}

			// get data
			CString id = stripHTML(li3[1]);
			if (id.IsEmpty())
			{
				Log(LOGERR, "Invalid li3 id='%s'", id);
				continue;
			}

			vara desca = li3[3].split("<br"); desca[0] = "";
			CString desc = stripHTML(desca.join("<br"));

			double lat, lng;
			if (!BLUUGNOME_ll(li3[2], lat, lng))
				continue;

			// add line 

			// add only DESCENT waypoints & start and end of EXIT & APPROACH
			BOOL add = color == DESCENT;
			add += (color != ROAD) && (i == 1 || i == li.length() - 1);
			if (add && list.Find(id) < 0)
			{
				// add marker
				CSym item(id);
				item.SetNum(ITEM_LAT, lat);
				item.SetNum(ITEM_LNG, lng);
				item.SetStr(ITEM_DESC, desc + credit);
				list.Add(item);
			}

			// add line coords
			linelist.push(CCoord3(lat, lng));
		}

		lines.push(KMLLine(title, credit, linelist, color, 3));
	}

	if (list.GetSize() == 0 && lines.length() == 0)
		return FALSE;

	// generate kml
	vara styles, points;
	styles.push("dot=http://maps.google.com/mapfiles/kml/shapes/open-diamond.png");
	for (int i = 0; i < list.GetSize(); ++i)
		points.push(KMLMarker("dot", list[i].GetNum(ITEM_LAT), list[i].GetNum(ITEM_LNG), list[i].id, list[i].GetStr(ITEM_DESC)));
	SaveKML(ubase, credit, url, styles, points, lines, out);

	return TRUE;
}

int BLUUGNOME_DownloadBeta(const char *ubase, CSymList &symlist)
{
	DownloadFile f;
	CString url = burl(ubase, "canyoneer_tripreport_list.aspx");
	if (f.Download(url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}

	vars base = "canyoneer_tripreport_list_";
	vara list(f.memory, base);
	for (int i = 1; i < list.length(); ++i) {
		CString region = ExtractString(list[i], "", ">", "<");
		CString url = burl(ubase, base + GetToken(list[i], 0, '\"'));
		if (f.Download(url))
		{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			continue;
		}
		vars cbase = "cyn_route/";
		vara clist(f.memory, cbase);
		for (int c = 1; c < clist.length(); ++c) {
			const char *cdata = clist[c];
			CString name;
			GetSearchString(cdata, "", name, ">", "<");
			CString summary;
			GetSearchString(cdata, "</a>", summary, "-", "<");
			CString link = burl(ubase, cbase + GetToken(cdata, 0, '\"'));
			link.Trim();
			name = stripHTML(name);
			summary = stripHTML(summary);
			while (!summary.IsEmpty() && !isdigit(summary[0]))
				summary.Delete(0);
			if (name.IsEmpty() || link.IsEmpty())
			{
				Log(LOGERR, "ERROR: invalid name/link %s from %.128s", cdata, url);
				continue;
			}
			CSym sym(urlstr(link));
			if (!UpdateCheck(symlist, sym) && MODE > -2)
				continue;
			// download detail only if new
			printf("Downloading %d/%d %d/%d\r   ", c, clist.length(), i, list.length());
			CString url = link;
			if (f.Download(url))
			{
				Log(LOGERR, "ERROR: can't download url %.128s", url);
				continue;
			}

			CString RP1;
			const char *RP1s[] = { "RP-01", "RP01", "RP-1", "RP1", "R1", "Tp", "", NULL };
			for (int r = 0; RP1.IsEmpty() && RP1s[r] != NULL; ++r)
				GetSearchString(f.memory, "<table", RP1, RP1s[r], "</li>");
			if (RP1.IsEmpty())
			{
				Log(LOGERR, "ERROR: No valid RP1 from url %.128s", url);
				continue;
			}


			double lat, lng;
			if (!BLUUGNOME_ll(RP1, lat, lng))
			{
				Log(LOGERR, "ERROR: bad RP1 %s from url %.128s", RP1, url);
				continue;
			}

			CDoubleArrayList longest, raps;
			if (GetValues(f.memory, ">Rappels", ">", "<br", ulen, longest))
				sym.SetNum(ITEM_LONGEST, longest.Tail());
			if (GetValues(f.memory, ">Rappels", ">", "<br", NULL, raps)) {
				if (raps.Tail() >= 50)
					Log(LOGERR, "Too many rappels %d from %.128s", raps.Tail(), url);
				else
					sym.SetStr(ITEM_RAPS, Pair(raps));
			}

			CDoubleArrayList time, dist;
			if (GetValues(f.memory, ">Time Required", ">", "<br", utime, time))
			{
				sym.SetNum(ITEM_MINTIME, time.Head());
				sym.SetNum(ITEM_MAXTIME, time.Tail());
			}
			if (GetValues(f.memory, ">Distance", ">", "<br", udist, dist))
				sym.SetNum(ITEM_HIKE, dist.Tail());

			sym.SetStr(ITEM_PERMIT, stripHTML(ExtractString(f.memory, ">Permit Required", "</strong>", "<")));
			sym.SetStr(ITEM_AKA, stripHTML(ExtractString(f.memory, "Aka -", "", "<br").replace(",", ";")));
			//sym.SetStr(ITEM_VEHICLE, stripHTML(ExtractString(f.memory, ">Vehicle", "</strong>", "<")));
			//sym.SetStr(ITEM_SHUTTLE, stripHTML(ExtractString(f.memory, ">Shuttle", "</strong>", "<")));
			sym.SetStr(ITEM_DESC, stripHTML(name));
			GetSummary(sym, stripHTML(summary));
			sym.SetNum(ITEM_LAT, lat);
			sym.SetNum(ITEM_LNG, lng);
			sym.SetStr(ITEM_REGION, stripHTML(region));
			sym.SetStr(ITEM_KML, "X");
			Update(symlist, sym);
		}

	}

	return TRUE;
}

int BLUUGNOME_ll(const char *item, double &lat, double &lng)
{
	CString ll = stripHTML(item);
	lat = CDATA::GetNum(strval(ll, " N "));
	lng = CDATA::GetNum(strval(ll, " W "));
	if (ll.Find("WGS84") < 0)
	{
		Log(LOGERR, "BG No WGS84 in ll='%s'", ll);
		return FALSE;
	}
	if (!CheckLL(lat, lng, MkString("BG Invalid ll='%s'", ll)))
		return FALSE;
	return TRUE;
}
