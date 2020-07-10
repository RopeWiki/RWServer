
#include "BetaC.h"
#include "BETAExtract.h"
#include "trademaster.h"

class MADININA : public BETAC
{
protected:
	CSymList trans;

public:

	MADININA(const char *base) : BETAC(base)
	{
		ticks = 500;
		kfunc = SELL_ExtractKML;
	}

	void GetXY(const char *zone, const char *x, const char *y, float &lat, float &lng)
	{
		if (strstr(x, "°"))
			GetCoords(vars(vars(x).replace(";", ".") + " " + vars(y).replace(";", ".")).replace(". ", " "), lat, lng);
		else
			UTM2LL(zone, vars(x).replace(";", "").replace(".", ""), vars(y).replace(";", "").replace(".", ""), lat, lng);
	}


	virtual vars simplify(const char *sname)
	{
		vars name = stripAccents(sname);
		name.Replace("'", "");
		name.Replace(".", "");
		name.Replace("-", "");
		name.Replace("La ", "");
		name.Replace(" la ", " ");
		name.Replace(" de ", " ");
		return name;
	}

	vars mkdesc2(const char *name, const char *id)
	{
		return MkString("%s [%s]", name, id);
	}

	virtual int DownloadIndex(CSymList &symlist, const char *pdfurl, const char *zone, int regcol, int datacol, int lastcol)
	{

		vars pdf;
		if (!DownloadPDFHTML(pdfurl, pdf))
			return FALSE;

		// process table
		double tcheck = 1;
		vara list(pdf, "<tr");
		CSymList tlist;
		for (int i = 1; i < list.length(); ++i)
		{
			vara line(list[i], "<td");
			vars tid = stripHTML(ExtractString(line[1], ">", "", "</td"));
			vara tida(tid, "-");
			if (tida.length() != 2 || strlen(tida[0]) == 0 || CDATA::GetNum(tida[1]) <= 0)
				continue;

			int col = 2;
			vars name = stripHTML(ExtractString(line[col++], ">", "", "</td")).Trim("; .");
			if (name.IsEmpty())
				continue;

			if (line.length() != lastcol)
			{
				Log(LOGERR, "ERROR: INDEX PDF Table is wrong size (%d!=%d), skipping %s for %s", line.length(), lastcol, tid, pdfurl);
				continue;
			}

			double tn = CDATA::GetNum(tida[1]);
			if (tn != 1 && (tn != tcheck && tn != tcheck - 1))
				Log(LOGERR, "INDEX PDF Table Out of Sync %s != %s", tid, CData(tcheck));
			tcheck = tn + 1;

			CSym sym(tid, name);

			if (regcol >= 0)
			{
				vars region = stripHTML(ExtractString(line[col++], ">", "", "</td"));
				sym.SetStr(ITEM_REGION, region);
			}
			else
			{
				sym.SetStr(ITEM_REGION, tida[0]);
			}

			col = datacol;
			vars x = stripHTML(ExtractString(line[col++], ">", "", "</td"));
			vars y = stripHTML(ExtractString(line[col++], ">", "", "</td"));
			col++;
			vars x2 = stripHTML(ExtractString(line[col++], ">", "", "</td"));
			vars y2 = stripHTML(ExtractString(line[col++], ">", "", "</td"));
			col++;
			float lat = 0, lng = 0, lat2 = 0, lng2 = 0;
			GetXY(zone, x, y, lat, lng);
			GetXY(zone, x2, y2, lat2, lng2);
			if (!CheckLL(lat, lng))
			{
				Log(LOGWARN, "WARNING: Bad startcoord %s zn:%s x:%s y:%s in %s", tid, zone, x, y, pdfurl);
				lat = lat2, lng = lng2;
			}
			if (!CheckLL(lat2, lng2))
			{
				Log(LOGWARN, "WARNING: Bad endcoord %s zn:%s x:%s y:%s in %s", tid, zone, x2, y2, pdfurl);
				lat2 = lat, lng2 = lng;
			}
			if (CheckLL(lat, lng))
			{
				sym.SetNum(ITEM_LAT, lat);
				sym.SetNum(ITEM_LNG, lng);
				sym.SetStr(ITEM_KML, SELL_Make(lat, lng, lat2, lng2));
			}
			else
			{
				Log(LOGERR, "ERROR: Bad Coords %s zn:%s x:%s y:%s in %s", tid, zone, x, y, pdfurl);
			}

			vars depth = stripHTML(ExtractString(line[col++], ">", "", "</td"));
			sym.SetStr(ITEM_DEPTH, GetMetric(depth.replace(" ", "")));
			vars length = stripHTML(ExtractString(line[col++], ">", "", "</td"));
			sym.SetStr(ITEM_LENGTH, GetMetric(length.replace(" ", "")));

			ASSERT(sym.GetStr(ITEM_LENGTH) != "1m");

			vara times;
			times.push(stripHTML(ExtractString(line[col + 1], ">", "", "</td")));
			times.push(stripHTML(ExtractString(line[col], ">", "", "</td")));
			times.push(stripHTML(ExtractString(line[col + 2], ">", "", "</td")));
			col += 3;
			for (int t = 0; t < times.length(); ++t)
			{
				times[t] = times[t].replace(" h \xC2½", ".5h").replace(" h \xC2¾", ".75h").replace(" h \xC2¼", ".25h");
				times[t] = times[t].replace(" h\xC2½", ".5h").replace(" h\xC2¾", ".75h").replace(" h\xC2¼", ".25h");
				times[t] = times[t].replace("\xC2½ h", "0.5h").replace("\xC2¾ h", "0.75h").replace("\xC2¼ h", "0.25h");
				if (strstr(times[t], "½") || strstr(times[t], "¾") || strstr(times[t], "¼"))
					Log(LOGERR, "ERROR: Invalid time for %s %s : %s", tid, sym.id, times[t]);
			}
			GetTotalTime(sym, times, tid, -1);

			vars shuttle = stripHTML(ExtractString(line[col++], ">", "", "</td"));
			if (CDATA::GetNum(shuttle) == 0)
				sym.SetStr(ITEM_SHUTTLE, "No");
			else
				sym.SetStr(ITEM_SHUTTLE, shuttle);

			double num = CDATA::GetNum(stripHTML(ExtractString(line[col++], ">", "", "</td")));
			if (num >= 0) sym.SetNum(ITEM_RAPS, num);

			vars longest = stripHTML(ExtractString(line[col++], ">", "", "</td"));
			int fx = longest.indexOf("x");
			if (fx >= 0) longest = longest.Mid(fx + 1);
			sym.SetStr(ITEM_LONGEST, GetMetric(longest));

			sym.SetStr(ITEM_AKA, sym.GetStr(ITEM_DESC));

			Update(tlist, sym, NULL, 0);
		}
#if 1
		// do not match
		for (int i = 0; i < tlist.GetSize(); ++i)
		{
			CSym sym(tlist[i].id, tlist[i].data);
			sym.SetStr(ITEM_DESC, mkdesc2(sym.GetStr(ITEM_DESC), sym.id));
			sym.id = vars(pdfurl) + "?id=" + sym.id;
			Update(symlist, sym, NULL, 0);
		}

#else
		// match
		// patch for duplicates
		for (int i = 0; i < tlist.GetSize(); ++i)
		{
			vars newname = tlist[i].GetStr(ITEM_DESC);
			for (int j = i + 1; j < tlist.GetSize(); ++j)
				if (tlist[i].GetStr(ITEM_DESC) == tlist[j].GetStr(ITEM_DESC))
				{
					newname = mkdesc2(tlist[i].GetStr(ITEM_DESC), tlist[i].id);
					tlist[j].SetStr(ITEM_DESC, mkdesc2(tlist[j].GetStr(ITEM_DESC), tlist[j].id));
				}
			tlist[i].SetStr(ITEM_DESC, newname);
		}

		vara matched;
		CSymList unmatched;
		for (int i = 0; i < tlist.GetSize(); ++i)
		{
			vars tid = tlist[i].id;
			vars name = tlist[i].GetStr(ITEM_DESC);

			// trans
			vars tname = name;
			vars tname2 = mkdesc2(name, tid);

			int ti = trans.Find(name);
			if (ti >= 0)
				tname = trans[ti].GetStr(ITEM_DESC);

			int match = -1;
			vars cname = simplify(tname);
			vars cname2 = simplify(tname2);
			if (IsSimilar(tname, "http"))
			{
				for (int s = 0; s < symlist.GetSize() && match < 0; ++s)
					if (symlist[s].id == tname)
						match = s;
			}
			else
			{
				for (int s = 0; s < symlist.GetSize() && match < 0; ++s)
				{
					vars sname = simplify(symlist[s].GetStr(ITEM_DESC));
					//sname.Replace("riv ", "riviere ");
					if (strcmp(cname, sname) == 0 || strcmp(cname2, sname) == 0)
						match = s;
				}
			}

			if (match < 0)
			{
				unmatched.Add(tlist[i]);
				continue;
			}


			CSym &sym = symlist[match];
			//ASSERT(!strstr(sym.data, "Teruvea"));
			if (matched.indexOf(sym.id) >= 0)
			{
				Log(LOGERR, "INDEX PDF Double match for %s in %s", sym.id, pdfurl);
				continue;
			}
			matched.push(sym.id);

			// merge
			tlist[i].id = sym.id;
			tlist[i].SetStr(ITEM_DESC, sym.GetStr(ITEM_DESC));
			Update(symlist, tlist[i], NULL, 0);
		}

		vara unmatchedsymlist;
		for (int i = 0; i < symlist.GetSize(); ++i)
			if (matched.indexOf(symlist[i].id) < 0)
				unmatchedsymlist.push(symlist[i].GetStr(ITEM_DESC));

		// match summary
		if (unmatchedsymlist.length() > 0)
		{
			Log(LOGERR, "UNMATCHED SYMLIST: %d/%d (see log, ed beta\trans-*code*.csv) %s", unmatchedsymlist.length(), symlist.GetSize(), pdfurl);
			Log("," + unmatchedsymlist.join("\n,"));
			Log(LOGERR, "UNMATCHED INDEX PDF: %d/%d (see log, ed beta\trans-*code*.csv)", unmatched.GetSize(), tlist.GetSize());
			if (unmatchedsymlist.length() > 0)
			{
				vara unmatchedstr;
				for (int i = 0; i < unmatched.GetSize(); ++i)
					unmatchedstr.push(unmatched[i].GetStr(ITEM_DESC) + "," + unmatched[i].id);
				Log(unmatchedstr.join("\n"));
			}
		}
		else
		{
			// Add unmatched
			for (int i = 0; i < unmatched.GetSize(); ++i)
			{
				vars id = urlstr(pdfurl) + "?id=" + unmatched[i].id;
				vars desc = mkdesc2(unmatched[i].GetStr(ITEM_DESC), unmatched[i].id);
				unmatched[i].id = id;
				unmatched[i].SetStr(ITEM_DESC, desc);
				Update(symlist, unmatched[i], NULL, 0);
			}
			Log(LOGINFO, "Useful excel formulas to sort unmatched:\n+SUBSTITUTE(SUBSTITUTE(SUBSTITUTE(SUBSTITUTE(MID(B52,1,FIND(\"[\",B52)-2),\" du \",\" \"),\" de \",\" \"),\"Riviere \",\"\"),\"Ravine \",\"\")\n+COUNTIF(E$1:E$5000,\"*\"&E52&\"*\")");
		}
#endif
		return TRUE;
	}

	int DownloadBeta(CSymList &gsymlist)
	{
		const char *urls[] = {
			"http://ankanionla-madinina.com/Dominique/Dominique.htm",
			"http://ankanionla-madinina.com/Martinique/Martinique.htm",
			"http://ankanionla-madinina.com/Guadeloupe/Guadeloupe.htm",
			"http://ankanionla-madinina.com/St%20Vincent/Saint%20Vincent.htm",
			NULL
		};

		trans.Load(filename("trans-mad"));
		trans.Sort();

		for (int u = 0; urls[u] != NULL; ++u)
		{
			CSymList symlist;

			// get page list
			x = BETAX("<a", NULL, ">Inventaire<", "<style");
			vara urla(urls[u], "/");
			urla.SetSize(urla.length() - 1);
			ubase = umain = urla.join("/");
			DownloadUrl(urls[u], symlist, x);
			for (int i = 0; i < symlist.GetSize(); ++i)
				symlist[i].id = GetToken(symlist[i].id, 0, '#');


			// get table coords
			vars invurl = ::ExtractLink(f.memory, ">Inventaire<");
			vars pdfpageurl = makeurl(ubase, invurl);
			if (f.Download(pdfpageurl))
			{
				Log(LOGERR, "ERROR: Could not download Inventaire page %s", invurl);
				continue;
			}

			vars pdffile = ::ExtractLink(f.memory, ".pdf");
			if (!DownloadIndex(symlist, makeurl(ubase, pdffile), "20P", -1, 3, 18))
				Log(LOGERR, "ERROR: Could not find/download INDEX PDF for MADININA");

			for (int i = 0; i < symlist.GetSize(); ++i)
				Update(gsymlist, symlist[i], 0, 0);
		}

		return gsymlist.GetSize() > 0;
	}

	// download and convert PDF
	int DownloadPDFHTML(const char *url, CString &str)
	{
		char buffer[MAX_PATH];
		GetCurrentDirectory(sizeof(buffer), buffer);

		vars path = vars(buffer) + "\\pdf2html\\";
		vars pdffile = path + "pdfhtml.pdf";
		vars htmlfile = path + "pdfhtml.html";
		vars pdf2htmlexe = path + "pdfhtml.vbs"; //"pdf2htmlex\\pdf2htmlex.exe --optimize-text 0 --embed-font 0 --embed-javascript 0 --embed-outline 0 --embed-image 0 --embed-css 0 --process-nontext 0 ";

		DeleteFile(pdffile);
		DeleteFile(htmlfile);

		DownloadFile fb(TRUE);
		if (fb.Download(url, pdffile))
		{
			Log(LOGERR, "Could not download PDF from %s", url);
			return FALSE;
		}

		vars cmd = pdf2htmlexe + " " + pdffile + " " + htmlfile;
		system(cmd);

		CFILE f;
		if (f.fopen(htmlfile))
		{
			str = "";
			int bread = 0;
			char buff[8 * 1024];
			while (bread = f.fread(buff, 1, sizeof(buff)))
				str += CString(buff, bread);
			return TRUE;
		}

		return FALSE;
	}
};