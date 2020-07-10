
#include "BetaC.h"
#include "BETAExtract.h"
#include "trademaster.h"

class CMAGAZINE : public BETAC
{
public:
	int p;

	CMAGAZINE(const char *base) : BETAC(base)
	{
		p = 1;
		urls.push(umain = "https://canyonmag.net/wordpress/wp-admin/admin-ajax.php?POST?action=wpas_ajax_load&page=1&form_data=paged%3D1%26wpas_id%3Ddatabase_location%26wpas_submit%3D1%26paged%3D1%26wpas_id%3Ddatabase_vertical%26wpas_submit%3D1%26paged%3D1%26wpas_id%3Ddatabase_aquatic%26wpas_submit%3D1%26paged%3D1%26wpas_id%3Ddatabase_rating%26wpas_submit%3D1");

		//"action=wpas_ajax_load&page=1&form_data=paged%3D1%26wpas_id%3Ddatabase_location%26wpas_submit%3D1%26paged%3D1%26wpas_id%3Ddatabase_vertical%26wpas_submit%3D1%26paged%3D1%26wpas_id%3Ddatabase_aquatic%26wpas_submit%3D1%26paged%3D1%26wpas_id%3Ddatabase_rating%26wpas_submit%3D1
		//action=wpas_ajax_load&page=1&form_data=paged=1&wpas_id=database_location&wpas_submit=1&paged=1&wpas_id=database_vertical&wpas_submit=1&paged=1&wpas_id=database_aquatic&wpas_submit=1&paged=1&wpas_id=database_rating&wpas_submit=1
		x = BETAX("<tr", NULL, "\"Name\"", "</table");
	}

	void DownloadPatch(vars &memory)
	{
		memory.Replace("\\/", "/");
		memory.Replace("\\\"", "\"");
		memory.Replace("\\\'", "\'");
		memory.Replace("\\n", " ");
		memory.Replace("\\t", " ");
		memory.Replace("\\r", " ");

		int find;
		vars unicode = "\\u00";
		while ((find = memory.Find(unicode)) >= 0)
		{
			vars code = memory.Mid(find + unicode.GetLength(), 2);
			vars ostr = unicode + code;
			unsigned char nstr[8] = "X";
			FromHex(code, nstr);
			memory.Replace(ostr, CString(nstr));
		}

		memory = UTF8(memory);
	}

	int DownloadPage(const char *url, CSym &sym)
	{
		// download list of recent additions
		static double ticks;
		Throttle(ticks, 1000);
		if (f.Download(url))
		{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			return FALSE;
		}


		vars country = stripHTML(ExtractString(f.memory, ">Country", ">", "</tr"));
		vars prefecture = stripHTML(ExtractString(f.memory, ">Prefecture", ">", "</tr"));
		vars city = stripHTML(ExtractString(f.memory, ">City", ">", "</tr"));

		sym.SetStr(ITEM_REGION, country + ";" + prefecture);


		sym.SetStr(ITEM_DEPTH, GetMetric(stripHTML(ExtractString(f.memory, ">Difference", ">", "</tr"))));
		sym.SetStr(ITEM_LENGTH, GetMetric(stripHTML(ExtractString(f.memory, ">Length", ">", "</tr"))));

		sym.SetNum(ITEM_RAPS, CDATA::GetNum(stripHTML(ExtractString(f.memory, ">Rappels", ">", "</tr"))));
		sym.SetStr(ITEM_LONGEST, GetMetric(stripHTML(ExtractString(f.memory, ">Max", ">", "</tr"))));

		vars sum = ExtractString(f.memory, ">Rating", ">", "</tr");
		GetSummary(sym, stripHTML(sum));

		int stars = sum.Replace("class=\"fa fa-star\"", "");
		if (stars > 0)
			sym.SetStr(ITEM_STARS, starstr(stars + 1, 1));

		vars coords = stripHTML(ExtractString(f.memory, ">Entry", ">", "</tr"));
		double lat = CDATA::GetNum(GetToken(coords, 0, ';'));
		double lng = CDATA::GetNum(GetToken(coords, 1, ';'));
		if (CheckLL(lat, lng))
		{
			sym.SetNum(ITEM_LAT, lat);
			sym.SetNum(ITEM_LNG, lng);
		}
		else
		{
			sym.SetStr(ITEM_LAT, "@" + city + "." + prefecture + "." + country);
		}

		return TRUE;
	}

};

