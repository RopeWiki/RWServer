
#include "Books.h"
#include "BETAExtract.h"
#include "trademaster.h"


class TODDMARTIN : public BETAC
{
public:

	TODDMARTIN(const char *base) : BETAC(base, BOOK_DownloadBeta)
	{
		umain = "http://toddshikingguide.com/Hikes/Hikes.htm";
	}

	int DownloadBeta(CSymList &symlist)
	{
		// ALL map 
		ubase = "toddshikingguide.com";

		DownloadFile f;
		CString url = umain;
		if (f.Download(url))
		{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			return FALSE;
		}

		// tables for ratings
		vars table = ExtractString(f.memory, "Warning.htm", "<table", "</table>");
		vara list(table, "href=");
		if (list.length() <= 1)
			Log(LOGERR, "Could not extract sections from url %.128s", url);
		for (int i = 1; i < list.length(); ++i) {
			vars lnk = ExtractString(list[i], "\"", "", "\"");
			CString url = burl(ubase, "/Hikes/" + lnk);
			if (f.Download(url))
			{
				Log(LOGERR, "ERROR: can't download url %.128s", url);
				continue;
			}

			if (!strstr(f.memory, ">Required Skills"))
				continue;

			vars link = url;
			vars nameregion = stripHTML(ExtractString(f.memory, "<b>", "", "</b"));
			vars name = GetToken(nameregion, 0, '-').Trim();
			vars region = GetToken(nameregion, 1, '-').Trim();
			// improved region
			vara rlink(link, "/");
			int rlen = rlink.length() - 1;
			if (rlen > 2)
				region = rlink[rlen - 2] + ";" + rlink[rlen - 1];
			vars geoloc = rlink[rlen - 1] + "." + rlink[rlen - 2];
			vars hike = stripHTML(ExtractString(f.memory, ">Length:", "<td", "</td>"));
			vars road = stripHTML(ExtractString(f.memory, ">Road Conditions:", "<td>", "</td>"));

			vars type = "-1", summary = "", swim = "A";
			if (strstr(f.memory, "swimming.gif"))
				swim = "B";
			if (strstr(f.memory, "hiking.gif"))
				type = "0:Hiking", summary = "1" + swim;
			if (strstr(f.memory, "backpacking.gif"))
				type = "0:Backpacking", summary = "1" + swim;
			if (strstr(f.memory, "climbing.gif"))
				type = "0:Climbing", summary = "2" + swim;
			if (strstr(f.memory, "Rappel") || strstr(f.memory, "rappel") || strstr(f.memory, "abseil") || strstr(f.memory, " rap ") || strstr(f.memory, " raps "))
				type = "1:Rappel", summary = "3" + swim;

			// ratings
			double stars = 0;
			vars rating = ExtractString(f.memory, "(1-5 stars):", "<td>", "</td>");
			vara num(rating, "<br>");
			for (int n = 0; n < num.length(); ++n)
			{
				vara nstars(num[n], "/star.gif");
				vara nhstars(num[n], "/halfstar.gif");
				double newstars = nstars.length() - 1 + (nhstars.length() - 1)*0.5;
				if (newstars > stars)
					stars = newstars;
			}
			//vars kmlidx = ExtractLink(f.memory, ".gpx");

			CSym sym(urlstr(link));
			sym.SetStr(ITEM_DESC, name);
			sym.SetStr(ITEM_REGION, region);
			//sym.SetStr(ITEM_LAT, "@"+geoloc);
			GetSummary(sym, summary);
			sym.SetStr(ITEM_CLASS, type);
			sym.SetStr(ITEM_STARS, starstr(stars, 1));
			SetVehicle(sym, road);

			Update(symlist, sym, NULL, FALSE);
			printf("Downloading %d/%d   \r", i, list.length());
		}

		return TRUE;
	}
};
