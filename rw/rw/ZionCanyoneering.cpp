#include "BetaC.h"
#include "BETAExtract.h"
#include "trademaster.h"

class ZIONCANYONEERING : public BETAC
{
	vars ctitle, cmore, cmore2;

public:
	ZIONCANYONEERING(const char *base) : BETAC(base)
	{
		umain = "http://zioncanyoneering.com";
	}

	vars ExtractVal(const char *xml, const char *id)
	{
		vars item = *id != 0 ? MkString("\"%s\"", id) : "";
		vars str = ExtractString(xml, item, ":", ",");
		str.Trim("\" ");
		if (str == "null")
			str = "";
		return str;
	}

	int DownloadBeta(CSymList &symlist)
	{
		if (f.Download("http://zioncanyoneering.com/api/CanyonAPI/GetAllCanyons"))
			return FALSE;

		vara list(f.memory, "\"Name\"");
		for (int i = 1; i < list.GetSize(); ++i)
		{
			vars &xml = list[i];
			vars name = ExtractVal(xml, "");
			vars desc = ExtractVal(xml, "Description");
			vars url = ExtractVal(xml, "URLName");
			vars region = ExtractVal(xml, "State");

			double n = CDATA::GetNum(ExtractVal(xml, "NumberOfVoters"));
			double score = CDATA::GetNum(ExtractVal(xml, "PublicRating"));

			if (n > 0 || !desc.IsEmpty())
			{
				// add only if voted of desc not empty
				CSym sym(urlstr("http://zioncanyoneering.com/Canyon/" + url), name);
				sym.SetStr(ITEM_REGION, region);
				if (n > 0 && score > 0)
					sym.SetStr(ITEM_STARS, starstr(score / n, n));

				double lat = CDATA::GetNum(ExtractVal(xml, "Latitude"));
				double lng = CDATA::GetNum(ExtractVal(xml, "Longitude"));
				if (CheckLL(lat, lng))
					sym.SetStr(ITEM_LAT, MkString("@%s;%s", CData(lat), CData(lng)));

				Update(symlist, sym, NULL, 0);
			}

		}

		return TRUE;
	}

};

