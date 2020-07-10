
#include "Books.h"
#include "BETAExtract.h"
#include "trademaster.h"


class SAM : public BETAC
{
public:

	SAM(const char *base) : BETAC(base, BOOK_DownloadBeta)
	{
		umain = "http://ropewiki.com/Super_Amazing_Map";
	}

	int DownloadBeta(CSymList &symlist)
	{
		CFILE fm;
		vars kml = "beta\\SuperAmazingMap.kml";
		if (!fm.fopen(kml))
			Log(LOGERR, "could not read %s", kml);

		int mode = 0;
		const char *line;
		vara links;
		vars canyon, region;
		while (line = fm.fgetstr())
		{
			if (strstr(line, "<Folder"))
				mode = 0;
			if (strstr(line, "</Folder"))
				mode = -1;
			if (strstr(line, "<Placemark"))
				mode = 1;
			if (strstr(line, "</Placemark"))
				mode = -1;
			if (strstr(line, "<name")) {
				vars name = stripHTML(ExtractString(line, "<name", ">", "</name"));
				if (mode < 0)
					Log(LOGERR, "Invalid name %.256s", line);
				else if (mode > 0)
					canyon = name;
				else
					region = name;
			}
			if (strstr(line, "<coord")) {
				vars coord = ExtractString(line, "<coord", ">", "</coord");
				if (mode != 1)
					Log(LOGERR, "Invalid coords %s", line);
				else
				{
					vars permit;
					const char *status;
					if (strstr(canyon, status = "(closed)")) {
						canyon.Replace(status, "");
						permit = "Closed";
					}
					if (strstr(canyon, status = "(restricted)")) {
						canyon.Replace(status, "");
						permit = "Restricted";
					}
					//canyon.Replace("'","");
					canyon.Replace(";", "-");
					canyon.Trim();
					CSym sym(urlstr(MkString("http://ropewiki.com/User:Super_Amazing_Map?id=%s_-_%s", canyon.replace(" ", "_").split("aka").first().Trim(" ("), region.replace(" ", "_"))));
					if (links.indexOf(sym.id) >= 0) {
						Log(LOGERR, "Duplicated Super Amazing Map ID  %s", sym.id);
						continue;
					}
					links.push(sym.id);
					//ASSERT(!strstr(canyon, "apos"));
					sym.SetStr(ITEM_DESC, canyon);
					sym.SetStr(ITEM_REGION, region);
					sym.SetStr(ITEM_PERMIT, permit);
					double lat = CDATA::GetNum(GetToken(coord, 1));
					double lng = CDATA::GetNum(GetToken(coord, 0));
					if (!CheckLL(lat, lng)) {
						Log(LOGERR, "Invalid coords %s", coord);
						continue;
					}
					sym.SetNum(ITEM_LAT, lat);
					sym.SetNum(ITEM_LNG, lng);

					// match based on coords
					int match = 0;
					double eps = 1e-5;
					for (int s = 0; s < symlist.GetSize(); ++s)
					{
						if (fabs(lat - symlist[s].GetNum(ITEM_LAT)) < eps)
							if (fabs(lng - symlist[s].GetNum(ITEM_LNG)) < eps)
								if (symlist[s].id != sym.id)
								{
									++match;
									Log(LOGWARN, "SAM ID override %s -> %s", symlist[s].id, sym.id);
									symlist[s].id = sym.id;
									sym.SetStr(ITEM_CONDDATE, symlist[s].GetStr(ITEM_MATCH));
									if (match > 1)
										Log(LOGERR, "SAM ID override multiple times!?!");
								}
					}
					if (match) symlist.ReSort();

					Update(symlist, sym, NULL, FALSE);
				}
			}
		}

		return TRUE;
	}

};
