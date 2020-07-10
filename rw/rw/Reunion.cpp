
#include "Reunion.h"
#include "BETAExtract.h"
#include "trademaster.h"

	int REUNION::DownloadPage(const char *url, CSym &sym)
	{
		CSymList lines;
		DownloadUrl(url, lines, BETAX("class=", NULL, "<h1", ">Statistiques"), D_DATA);

		vara sketch;
		for (int i = 0; i < lines.GetSize(); ++i)
		{
			CSym link;
			if (!strstr(lines[i].id, "<img") || strstr(lines[i].id, "<IMG"))
				continue;
			if (!DownloadLink(lines[i].id, link))
				continue;
			vars url = link.id;
			const char *nameext = GetFileNameExt(url);
			const char *ext = GetFileExt(url);
			if (!ext || !nameext)
				continue;
			if (!IsSimilar(ext, "jpg"))
				continue;
			if (IsSimilar(nameext, "legende.jpg"))
				continue;
			if (IsSimilar(nameext, "photo"))
				continue;
			sketch.push(ACP8(url));
		}
		sym.SetStr(ITEM_LINKS, sketch.join(" "));

		/*
			vars memory(f.memory);
			memory.Replace("<DIV","<div");
			memory.Replace("</DIV","</div");

			// Region / Name
			vara regionname;
			vara list(ExtractString(memory, "class=\"chemin\"", ">", "</div"), "<li");
			for (int i=2; i<list.length(); ++i)
				regionname.push(stripHTML(ExtractString(list[i], "<span", ">", "</span")));
			if (regionname.length()<1)
				{
				Log(LOGERR, "Inconsistend regionname for %s", url);
				return FALSE;
				}
			int l = regionname.length()-1;
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
			double stars = coeur.length()-1;
			if (stars>0)
				sym.SetStr(ITEM_STARS, starstr(stars,1));

			// season
			vars season = stripHTML(ExtractString(strstr(memory, ">Periode :"), "<div", ">", "</div"));
			if (!season.IsEmpty())
				sym.SetStr(ITEM_SEASON, season);

			// time
			vars time = stripHTML(ExtractString(strstr(memory, ">Temps :"), "<div", ">", "</div"));
			vara times(time, ":");
			double tmax = 0;
			double tdiv[] = { 1, 60, 60*60 };
			for (int i =0; i<3; ++i)
				{
				double t = CDATA::GetNum(times[i]);
				if (t!=InvalidNUM)
					tmax += t/tdiv[i];
				}
			if (tmax>0)
				sym.SetNum(ITEM_MINTIME, tmax);

			// rappel
			vars longest = stripHTML(ExtractString(strstr(memory, ">Rappel max :"), "<div", ">", "</div"));
			sym.SetStr(ITEM_LONGEST, longest.replace(" ", ""));

			// shuttle
			vars shuttle = stripHTML(ExtractString(strstr(memory, ">Navette :"), "<div", ">", "</div"));
			shuttle.Replace("Oui", "Yes");
			shuttle.Replace("Non", "No");
			sym.SetStr(ITEM_SHUTTLE, shuttle.replace(" ", ""));


			sym.SetStr(ITEM_DEPTH, GetMetric(stripHTML(ExtractString(strstr(memory, UTF8(">Dénivelé")), "<div", ">", "</div").replace(" ",""))));
			sym.SetStr(ITEM_LENGTH, GetMetric(stripHTML(ExtractString(strstr(memory, UTF8(">Longueur")), "<div", ">", "</div").replace(" ",""))));
		*/
		return TRUE;
	}

	int REUNION::DownloadBeta(CSymList &symlist)
	{
		vara done;
		DownloadFile f;

		// download base
		vars url = "http://www.lrc-ffs.fr/?page_id=167";
		Throttle(tickscounter, 1000);
		if (f.Download(url))
		{
			Log(LOGERR, "Can't download base url %s", url);
			return FALSE;
		}

		vara rlist(ExtractString(f.memory, "<map", "", "</ul"), "href=");
		for (int r = 1; r < rlist.length(); ++r)
		{
			vars link = GetToken(rlist[r], 0, " <>").Trim("'\"");
			if (link.IsEmpty())
				continue;
			link = burl(ubase, link);
			if (done.indexOf(link) >= 0)
				continue;

			url = link;
			done.push(link);
			Throttle(tickscounter, 1000);
			if (f.Download(url))
			{
				Log(LOGERR, "Can't download base url %s", url);
				return FALSE;
			}

			vara geotags(f.memory, "lmm-geo-tags");
			CSymList clist;
			for (int i = 1; i < geotags.length(); ++i)
			{
				vars name = stripHTML(ExtractString(geotags[i], ">", "", ":"));
				vars coords = stripHTML(ExtractString(geotags[i], ">", ":", "</div"));
				if (name.IsEmpty())
					continue;

				CSym c(name);
				c.SetStr(ITEM_LAT, GetToken(coords, 0, ';'));
				c.SetStr(ITEM_LNG, GetToken(coords, 1, ';'));
				clist.Add(c);
			}
			clist.Sort();

			vara markers(ExtractString(f.memory, "lmm-listmarkers", "", "</table"), "<tr");
			for (int i = 1; i < markers.length(); ++i)
			{
				vars topolink;
				vars name = stripHTML(ExtractString(markers[i], "lmm-listmarkers-markername", ">", "<"));
				vara links(markers[i], "href=");
				for (int l = 1; l < links.length() && topolink.IsEmpty(); ++l)
				{
					if (IsSimilar(links[l], "\"\""))
						continue;
					vars link = ExtractString(links[l], "\"", "", "\"").Trim();
					if (link.IsEmpty())
						link = ExtractString(links[l], "'", "", "'");
					if (link.IsEmpty() || !IsSimilar(link, "http"))
						Log(LOGWARN, "Empty link for %s in %s", name, url);
					if (strstr(link, "page_id"))
						topolink = burl(ubase, link.replace("\xC2\xA0", ""));
				}
				if (topolink.IsEmpty())
					continue;

				const char *sname = name;
				while (isdigit(*sname) || isspace(*sname))
					++sname;

				CSym sym(urlstr(topolink), sname);
				sym.SetStr(ITEM_REGION, "Reunion");

				int find = clist.Find(name);
				if (find >= 0)
				{
					double lat = clist[find].GetNum(ITEM_LAT);
					double lng = clist[find].GetNum(ITEM_LNG);
					if (CheckLL(lat, lng))
						if (lat > -21.429 && lat<-20.801 && lng>55.127 && lng < 55.918) //-21.429,55.127,-20.801,55.918
						{
							sym.SetNum(ITEM_LAT, lat);
							sym.SetNum(ITEM_LNG, lng);
						}
				}

				printf("%d %d/%d %d/%d  \r", symlist.GetSize(), i, markers.length(), r, rlist.length());
				DownloadSym(sym, symlist);
			}
		}

		return TRUE;
	}
