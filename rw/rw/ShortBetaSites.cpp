#include "BetaC.h"
#include "BetaWPC.h"
#include "BETAExtract.h"
#include "trademaster.h"
#include "SwissCanyon.h"
#include "ShortBetaSitesUnclassed.h"

extern GeoCache _GeoCache;
extern GeoRegion _GeoRegion;


class FLREUNION : public BETAC
{
public:
	FLREUNION(const char *base) : BETAC(base)
	{
		x = BETAX("<p", NULL, "</table", "<table");
		urls.push("http://francois.leroux.free.fr/canyoning/canyions.htm");
		urls.push("http://francois.leroux.free.fr/canyoning/classics.htm");
	}

	int DownloadInfo(const char *data, CSym &sym) override
	{
		sym.SetStr(ITEM_DESC, UTF8(sym.GetStr(ITEM_DESC)));
		sym.SetStr(ITEM_REGION, "Reunion");
		return TRUE;
	}
};


class ESPELEOCANYONS : public BETAWPC
{
public:
	ESPELEOCANYONS(const char *base) : BETAWPC(base)
	{
		urls.push("http://teamespeleocanyons.blogspot.com.es");
	}

	int DownloadInfo(const char *data, CSym &sym) override
	{
		vars id = ExtractString(sym.id, ".blogspot.", "", "/");
		if (id != "com.es")
			sym.id.Replace(id, "com.es");

		// check if empty
		vars text = stripHTML(ExtractString(data, "</h3>", "", "<div class='post-footer"));
		if (text.length() < 20)
			return FALSE;

		vars title = sym.GetStr(ITEM_DESC);
		sym.SetStr(ITEM_DESC, skipnoalpha(title));

		//sym.id = vars(sym.id).replace("www.", "").replace("WWW.", "").replace("//", "//WWW.");
		return TRUE;
	}
};


class BARRANQUISTAS : public BETAWPC
{
public:
	BARRANQUISTAS(const char *base) : BETAWPC(base)
	{
		urls.push(umain = "http://www.barranquistas.es/");
	}

	int DownloadInfo(const char *data, CSym &sym) override
	{
		sym.id = vars(sym.id).replace("www.", "").replace("WWW.", "").replace("//", "//WWW.");
		return TRUE;
	}

};


class ROCJUMPER : public BETAC
{
	int p;
public:
	ROCJUMPER(const char *base) : BETAC(base)
	{
		ticks = 500;
		p = 1;
		urls.push(umain = "http://www.rocjumper.com/barranco/");
		x = BETAX("<h3", "</h3");
		//x = BETAX("itemprop=\"blogPost\"", NULL);
	}

	int DownloadInfo(const char *data, CSym &sym) override
	{
		sym.id = urlstr(GetToken(sym.id, 0, '#'));
		sym.SetStr(ITEM_CLASS, strstr(sym.id, "/barranco/") ? "1:Barranco" : "-1:NoBarranco");
		return strstr(sym.id, "/barranco/") != NULL;
	}

	vars DownloadMore(const char *memory) override
	{
		double maxp = ExtractNum(f.memory, "'pagination-meta'", " de ", "<span");
		if (maxp < 0 || p >= maxp)
			return "";
		return umain + MkString("page/%d/", ++p);
	}
};


class SIXGUN : public BETAC
{
public:
	SIXGUN(const char *base) : BETAC(base)
	{
		urls.push("http://6ixgun.com/journal/trip-log/");
		x = BETAX("<article", "</article");
	}

	int DownloadInfo(const char *data, CSym &sym) override
	{
		if (!strstri(data, "Canyoneering")) // && c<0)
			return FALSE;

		vars name = stripHTML(ExtractString(data, "<h2", ">", "</h2"));
		sym.SetStr(ITEM_DESC, name);

		//name = name.Mid(0,f).Trim();
		//sym.SetStr(ITEM_DESC, name);

		//sym.id = vars(sym.id).replace("www.", "").replace("WWW.", "").replace("//", "//WWW.");
		//sym.SetStr(ITEM_REGION,"United States");
		return TRUE;
	}

	vars DownloadMore(const char *memory) override
	{
		return ::ExtractLink(f.memory, ">Next<");
	}
};


class KARL : public BETAWPC
{
public:
	KARL(const char *base) : BETAWPC(base, "class=\"entry-title\"", "class=\"previous\"", "</li")
	{
		urls.push("http://karl-helser.com/");
	}

	int DownloadInfo(const char *data, CSym &sym) override
	{
		vars name = sym.GetStr(ITEM_DESC);
		int f = name.indexOf("Canyonee");
		//int c = name.indexOf("Creek");
		//if (c>=0) f = name.indexOf(" ", c);
		if (f < 0) // && c<0)
			return FALSE;

		//name = name.Mid(0,f).Trim();
		//sym.SetStr(ITEM_DESC, name);

		//sym.id = vars(sym.id).replace("www.", "").replace("WWW.", "").replace("//", "//WWW.");
		//sym.SetStr(ITEM_REGION,"United States");
		return TRUE;
	}
};


class YADUGAZ : public BETAC
{
public:

	YADUGAZ(const char *base) : BETAC(base)
	{
		urls.push(umain = "http://www.yadugaz07.com/canyon-ardeche-lozere-drome.php");
		x = BETAX("<tr", "</tr", ">Liste des ", "</table");
	}

	int DownloadInfo(const char *data, CSym &sym) override
	{
		double val = CDATA::GetNum(stripHTML(ExtractString(data, "smiley", ">", "</td")));
		if (val > 0)
			sym.SetStr(ITEM_STARS, starstr(val / 2, 1));
		sym.SetStr(ITEM_AKA, sym.GetStr(ITEM_DESC));
		sym.SetStr(ITEM_REGION, "France");
		return TRUE;
	}
};


class CLIMBING7 : public BETAC
{
public:
	int p;

	CLIMBING7(const char *base) : BETAC(base)
	{
		p = 1;
		umain = "http://climbing7.wordpress.com/";
		urls.push("http://climbing7.wordpress.com/category/canyoning/");
		urls.push("http://climbing7.wordpress.com/category/via-ferrata-2/");

		x = BETAX("<!-- .preview -->", "<!-- #post");
	}

	int DownloadInfo(const char *data, CSym &sym) override
	{
		const char *star = "\xE2\x9C\xAF";

		sym.id = GetToken(sym.id, 0, 0xE2);

		vars desc = sym.GetStr(ITEM_DESC);
		int stars = desc.Replace(star, "");
		double val[] = { 0, 1.5, 3, 4.5, 5 };
		if (stars > 3) stars = 4;
		sym.SetStr(ITEM_STARS, starstr(val[stars], 1));

		vara list(desc, ";");
		sym.SetStr(ITEM_DESC, list[0]);
		sym.SetStr(ITEM_AKA, list[0]);
		sym.SetStr(ITEM_REGION, list.last());

		list.RemoveAt(0);
		sym.SetStr(ITEM_LAT, "@" + list.join("."));

		if (strstr(curl, "ferrata"))
		{
			sym.SetStr(ITEM_DESC, "Via Ferrata " + sym.GetStr(ITEM_DESC));
			sym.SetStr(ITEM_CLASS, "3:Ferrata");
		}
		return TRUE;
	}

	vars DownloadMore(const char *memory) override
	{
		vars next = ExtractHREF(ExtractString(memory, "class=\"nav-previous\"", "", "</div>"));
		return next;
	}
};


class NKO : public BETAC
{
public:
	int p;

	NKO(const char *base) : BETAC(base)
	{
		ticks = 500;
		p = 1;
		urls.push(umain = "http://www.nko-extreme.com/category/cronicas/");
		x = BETAX("<h2", "</h2");
	}

	vars DownloadMore(const char *memory) override
	{
		double maxp = ExtractNum(f.memory, ">&rsaquo;<", "page/", ">");
		if (maxp<0 || p>maxp)
			return "";
		return umain + MkString("page/%d/", ++p);
	}
};


class ACTIONMAN4X4 : public BETAC
{
public:
	ACTIONMAN4X4(const char *base) : BETAC(base)
	{
		utf = TRUE;
		urls.push(umain = "http://www.actionman4x4.com/canonesybarrancos/localiza.htm");
		x = BETAX("<td", NULL, ">BARRANCOS DE ANDALUC", "</table");
		region = "Spain";
	}
};


class SIMEBUSCAS : public BETAC
{
public:
	SIMEBUSCAS(const char *base) : BETAC(base)
	{
		utf = TRUE;
		urls.push(umain = "http://simebuscasestoyconlascabras.blogspot.com");
		x = BETAX("<li", NULL, "<h2>Barrancos", "<h2");
		kfunc = MAP_ExtractKML;
		region = "Spain";
	}

	int DownloadPage(const char *url, CSym &sym) override
	{
		Throttle(tickscounter, ticks);
		if (f.Download(url))
		{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			return FALSE;
		}

		vars mapid = MAP_GetID(f.memory);
		if (!mapid.IsEmpty())
			sym.SetStr(ITEM_KML, mapid);

		return TRUE;
	}
};


class BCUENCA : public BETAC
{
public:
	BCUENCA(const char *base) : BETAC(base)
	{
		//utf = TRUE;
		urls.push(umain = "http://barrancosencuenca.es/barrancos.html");
		x = BETAX("<td", NULL, "NDICE DE BARRANCOS<", "</table");
		region = "Spain";
	}
};


class BCANARIOS : public BETAC
{
public:

	vara aregion;
	CSymList tregions;
	BCANARIOS(const char *base) : BETAC(base)
	{
		urls.push(umain = "http://www.barrancoscanarios.com");
		x = BETAX("<li", NULL, "<strong>Barrancos</strong>", "<strong>");
		//region = "Spain";
		//aregion.push(region);
	}

	int DownloadInfo(const char *data, CSym &sym) override
	{

		sym.SetStr(ITEM_REGION, aregion.join(";"));

		if (strstr(data, "</ul") && aregion.length() > 0)
			aregion.RemoveAt(aregion.length() - 1);

		if (strstr(data, "<ul"))
		{
			aregion.push(sym.GetStr(ITEM_DESC));
			return FALSE;
		}

		vara aregion(sym.id, "/");
		aregion.splice(0, 4);
		if (aregion.length() > 0)
			aregion.RemoveAt(aregion.length() - 1);
		sym.SetStr(ITEM_REGION, aregion.join(";"));

		if (tregions.GetSize() == 0)
		{
			tregions.Load(filename("trans-bcanR"));
			tregions.Sort();
		}

		int f = 0;
		for (int i = 0; i < aregion.length(); ++i)
			if ((f = tregions.Find(aregion[i])) >= 0)
				aregion[i] = tregions[f].data;
		vars geoloc = invertregion(aregion.join(";"));
		if (!geoloc.IsEmpty())
			sym.SetStr(ITEM_LAT, "@" + geoloc);

		vars name = sym.GetStr(ITEM_DESC);
		sym.SetStr(ITEM_AKA, name.replace(" o ", ";").replace(" O ", ";"));

		return TRUE;
	}
};


class GULLIVER : public BETAC
{
public:

	GULLIVER(const char *base) : BETAC(base)
	{
		ticks = 500;
		
		urls.push("http://www.gulliver.it/canyoning/");
		//urls.push("http://www.gulliver.it/via-ferrata/");
		x = BETAX("<a  class=\"summary-card route-summary\"", "</a>", "<div class=\"col results w-full py-4\">", "<div class=\'wp-pagenavi\' role=\'navigation\'>");
	}

	vars DownloadMore(const char *memory) override
	{
		const vars pagination = ExtractString(memory, "<link rel=\"next\"", "", "/>");
		vars url = burl("www.gulliver.it", ExtractHREF(pagination));
		if (!strstr(url, "page"))
			return "";

		return url;
	}

	int DownloadLink(const char *data, CSym &sym) override
	{
		vars name = stripHTML(ExtractString(data, "summary-card-title", ">", "</h3"));

		if (utf)
			name = UTF8(name);

		vars link = ExtractString(data, "href=", "", ">");
		link = GetToken(link, 0, ' ');
		link.Trim(" \"'\r\n");

		if (link.IsEmpty())
		{
			Log(LOGERR, "ERROR: can't find link for %s", name);
			return FALSE;
		}

		if (!IsSimilar(link, "http"))
		{
			link = makeurl(ubase, link);
		}

		sym = CSym(urlstr(link), name);

		return TRUE;
	}
	
	int DownloadInfo(const char *data, CSym &sym) override
	{
		//description
		vars title = sym.GetStr(ITEM_DESC);
		sym.SetStr(ITEM_DESC, title.split(" - ").first());

		//lat/lon
		const double lat = ExtractNum(data, "data-lat", "=\"", "\"");
		const double lng = ExtractNum(data, "data-lng", "=\"", "\"");
		
		if (CheckLL(lat, lng))
			sym.SetStr(ITEM_LAT, MkString("@%s;%s", CData(lat), CData(lng)));

		//region(theirs)
		//skip -- use Ropewiki's
		//const vars region = ExtractString(data, "class=\"s-info-2\"", ">", "</");
		
		//length
		const vars length = ExtractString(data, "class=\"s-info-3\"", ">", "</");
		sym.SetStr(ITEM_LENGTH, length);
		
		//commit (III)
		const vars summary = ExtractString(data, "class=\"s-info-4\"", ">", "</");
		GetSummary(sym, summary);

		//photo
		const vars banner = ExtractString(data, "<img class=\"lazyload attachment-thumbnail size-thumbnail\"", "data-src=\"", "\"");
		//sym.SetStr(ITEM_?, banner);
		
		return TRUE;
	}

	int DownloadPage(const char *url, CSym &sym) override
	{
		Throttle(tickscounter, ticks);
		
		if (f.Download(url))
		{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			return FALSE;
		}

		//altitude diff
		const double alt_high = ExtractNum(f.memory, "Quota partenza", "\">", "</dd>");
		const double alt_low = ExtractNum(f.memory, "Quota arrivo", "\">", "</dd>");
		if (alt_high != InvalidNUM && alt_low != InvalidNUM)
		{
			const double diff = alt_high - alt_low;
			sym.SetNum(ITEM_DEPTH, diff);
		}
		
		//conditions
		/*
		const char *str = strstr(f.memory, "href=");
		if (str) str = strstr(str, "<td");
		if (str) str += 2;

		CSym condsym;
		
		if (ExtractLink(str, curl, condsym))
		{
			double vdate = CDATA::GetDate(condsym.data, "DD/MM/YY");
			if (vdate <= 0)
				return FALSE;

			vara cond;
			cond.SetSize(COND_MAX);

			cond[COND_DATE] = CDate(vdate);
			cond[COND_LINK] = condsym.id;
			sym.SetStr(ITEM_CONDDATE, cond.join(";"));
		}
		*/
		
		//gpx tracks(if available)
		const vars gpx_file = ExtractString(f.memory, "download=", "\"", "\"");
		sym.SetStr(ITEM_KML, gpx_file);
		
		return TRUE;
	}
	
	int ExtractKML(const char *ubase, const char *url, inetdata *out, int fx) const
	{
		DownloadFile f;
		vars id = GetKMLIDX(f, url, NULL);
		if (id.IsEmpty())
			return FALSE; // not available

		const CString credit = " (Data by " + CString(ubase) + ")";

		//alternate go/gpx/download/578
		const CString url2 = burl(ubase, id);
		if (DownloadRetry(f, url2))
		{
			Log(LOGERR, "ERROR: can't download url %.128s", url2);
			return FALSE;
		}

		return GPX_ExtractKML(credit, url, f.memory, out);
	}
};


class CMADEIRA : public BETAC
{
public:

	CMADEIRA(const char *base) : BETAC(base)
	{
		ticks = 1000;
		urls.push("http://canyoning.camadeira.com/canyons");
		x = BETAX("<tr", "</tr", ">Localiza", "</table");

	}

	int DownloadInfo(const char *data, CSym &sym) override
	{
		vara td(data, "<td");

		sym.SetStr(ITEM_REGION, "Madeira");
		vars summary = stripHTML(ExtractString(td[2], ">", "", "</td"));
		GetSummary(sym, summary);

		//sym.SetStr(ITEM_LONGEST, stripHTML(ExtractString(td[3], ">", "", "</td")));
		return TRUE;
	}

	int DownloadPage(const char *url, CSym &sym) override
	{
		Throttle(tickscounter, ticks);
		if (f.Download(url))
		{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			return FALSE;
		}

		vars coords = stripHTML(ExtractString(f.memory, "<h3>Coordenadas", ">", "<h3")) + ":";
		double lat = ExtractNum(coords, ":", "", ";");
		double lng = ExtractNum(coords, ":", ";", ":");
		if (CheckLL(lat, lng))
		{
			sym.SetNum(ITEM_LAT, lat);
			sym.SetNum(ITEM_LNG, lng);
		}
		else
			return FALSE;


		sym.SetStr(ITEM_LENGTH, GetMetric(stripHTML(ExtractString(f.memory, ">Extens", ">", "</tr"))));
		sym.SetStr(ITEM_LONGEST, GetMetric(stripHTML(ExtractString(f.memory, ">Cascata mais", ">", "</tr"))));
		sym.SetNum(ITEM_RAPS, CDATA::GetNum(stripHTML(ExtractString(f.memory, ">Rap", ">", "</tr"))));

		vara atopo;
		CSymList topolinks;
		DownloadMemory(f.memory, topolinks, BETAX("<a", NULL, "<h3>Croqui", "class=\"comments-message\""), D_LINK);
		for (int i = 0; i < topolinks.GetSize(); ++i)
			atopo.push(topolinks[i].id);
		sym.SetStr(ITEM_LINKS, atopo.join(" "));

		// time
		vara times;
		const char *ids[] = { "Aproxima", "Percurso", "Regresso" };
		for (int t = 0; t < sizeof(ids) / sizeof(*ids); ++t)
			times.push(noHTML(ExtractString(f.memory, MkString("<td>%s", ids[t]), "</td>", "</td")));
		GetTotalTime(sym, times, url, -1);

		return TRUE;
	}
};


class MURDEAU : public BETAC
{
public:

	static vars ExtractKMLIDX(const char *memory)
	{
		//return GetToken(::ExtractLink(memory, ".kmz?"), 0, '?');
		vars link = ::ExtractString(memory, "//docs.google.com/file", "", "/preview");
		return vara(link, "/").last();
	}

	static int ExtractKML(const char *ubase, const char *url, inetdata *out, int fx)
	{
		//https://drive.google.com/uc?id=0B7GRxJzuZ66SS201YjgtRDdLa0k&export=download
		DownloadFile f;
		vars id = GetKMLIDX(f, url, ExtractKMLIDX);
		if (id.IsEmpty())
			return FALSE; // not available

		CString credit = " (Data by " + CString(ubase) + ")";
		vars kmzurl = MkString("https://drive.google.com/uc?id=%s&export=download", id);
		KMZ_ExtractKML(credit, kmzurl, out);
		return TRUE;
	}

	MURDEAU(const char *base) : BETAC(base)
	{
		kfunc = ExtractKML;
		umain = "http://www.murdeau-caraibe.com";
		urls.push("http://www.murdeau-caraibe.com/system/app/pages/subPages?path=/les-canyons/dominique");
		urls.push("http://www.murdeau-caraibe.com/system/app/pages/subPages?path=/les-canyons/martinique");
		x = BETAX("<tr", "</tr", "</th", "</table");
	}

	int DownloadInfo(const char *data, CSym &sym) override
	{
		sym.id.Replace("//murdeau", "//WWW.murdeau");
		if (strstr(sym.id, "/dominique/"))
			sym.SetStr(ITEM_REGION, "Dominica");
		if (strstr(sym.id, "/martinique/"))
			sym.SetStr(ITEM_REGION, "Martinique");
		return TRUE;
	}

	int DownloadPage(const char *url, CSym &sym) override
	{
		Throttle(tickscounter, ticks);
		if (f.Download(url))
		{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			return TRUE;
		}

		vars cot = stripHTML(ExtractString(f.memory, ">Cotation", ":", "</td"));
		vara sum(GetToken(cot, 0, '-'), ".");
		if (sum.length() >= 3)
			GetSummary(sym, "v" + sum[0] + "a" + sum[1] + sum[2]);

		int stars = cot.Replace("*", "x");
		sym.SetStr(ITEM_STARS, starstr(stars + 1, 1));

		sym.SetStr(ITEM_KML, "X");
		return TRUE;
	}
};


class ALPESGUIDE : public BETAC
{
public:

	int p;
	ALPESGUIDE(const char *base) : BETAC(base)
	{
		p = 1;
		urls.push(umain = "http://alpes-guide.com/sources/topo/index.asp?pmotcle%22=&psecteur=&pacti=1%2C14");
		x = BETAX("class=\'liste\'", "En savoir plus...");
	}

	int DownloadInfo(const char *data, CSym &sym) override
	{
		vars title = sym.GetStr(ITEM_DESC);
		sym.SetStr(ITEM_DESC, UTF8(skipnoalpha(title)));
		return TRUE;
	}

	/*
		virtual int DownloadPage(const char *url, CSym &sym)
		{
			Throttle(tickscounter, ticks);
			if (f.Download(url))
				{
				Log(LOGERR, "ERROR: can't download url %.128s", url);
				return FALSE;
				}

			vars state = stripHTML(ExtractString(f.memory, ">Stato<", ">", "</span>"));
			vars loc1 = stripHTML(ExtractString(f.memory, ">Comune<", ">", "</span>"));
			vars loc2 = stripHTML(ExtractString(f.memory, ">Localit&agrave;<", ">", "</span>"));

			if (!loc2.IsEmpty() || !loc1.IsEmpty())
				sym.SetStr(ITEM_LAT, "@"+loc2+"."+loc1+"."+state);
			sym.SetStr(ITEM_REGION, state + ";" +loc2 );

			sym.SetStr(ITEM_LENGTH, GetMetric(stripHTML(ExtractString(f.memory, ">Sviluppo totale<", ">", "</span>"))));
			sym.SetStr(ITEM_DEPTH, GetMetric(stripHTML(ExtractString(f.memory, ">Dislivello<", ">", "</span>"))));
			sym.SetStr(ITEM_LONGEST, GetMetric(stripHTML(ExtractString(f.memory, ">Salto pi&ugrave; alto<", ">", "</span>"))));
			sym.SetNum(ITEM_RAPS, CDATA::GetNum(stripHTML(ExtractString(f.memory, ">Calate<", ">", "</span>"))));

			GetSummary(sym, stripHTML(ExtractString(f.memory, ">Difficolt&agrave;<", ">", "</span>")));

			sym.SetStr(ITEM_SEASON, stripHTML(ExtractString(f.memory, ">Periodo dell'anno<", ">", "</span>")));
			sym.SetStr(ITEM_SHUTTLE, vars(stripHTML(ExtractString(f.memory, ">Navetta<", ">", "</span>"))).replace("si","Yes").replace("Si","Yes").replace("Nessun","No").replace("circa",""));

			sym.SetStr(ITEM_AKA, stripHTML(ExtractString(f.memory, ">Sinonimi<", ">", "</span>")));

			return TRUE;
		}
	*/

	int DownloadLink(const char *data, CSym &sym) override
	{
		vars name, link;
		name = stripHTML(ExtractString(data, "<b", ">", "</b"));
		name.Replace("Canyoning", "");
		link = ExtractString(data, "href=", "", ">");
		link = GetToken(link, 0, ' ');
		link.Trim(" \"'");
		if (!IsSimilar(link, "http"))
		{
			link = makeurl(ubase, link);
			/*
			vara aurl(url, "/");
			aurl[aurl.length()-1] = link;
			link = aurl.join("/");
			*/
		}
		sym = CSym(urlstr(link), name);
		return TRUE;
	}

	vars DownloadMore(const char *memory) override
	{
		vars page = MkString("&ppage=%d", ++p);
		if (!strstr(f.memory, page))
			return "";
		return "http://alpes-guide.com/sources/topo/index.asp?pacti=1%2C14&psecteur=&pmotcle=" + page;
	}
};


class OPENSPELEO : public BETAC
{
public:

	int p;
	OPENSPELEO(const char *base) : BETAC(base)
	{
		p = 1;
		urls.push(umain = "http://www.openspeleo.org/openspeleo/canyons.html");
		x = BETAX("class=\"searchresult2\"");
	}

	int DownloadPage(const char *url, CSym &sym) override
	{
		Throttle(tickscounter, ticks);
		if (f.Download(url))
		{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			return FALSE;
		}

		vars state = stripHTML(ExtractString(f.memory, ">Stato<", ">", "</span>"));
		vars loc1 = stripHTML(ExtractString(f.memory, ">Comune<", ">", "</span>"));
		vars loc2 = stripHTML(ExtractString(f.memory, ">Localit&agrave;<", ">", "</span>"));

		if (!loc2.IsEmpty() || !loc1.IsEmpty())
			sym.SetStr(ITEM_LAT, "@" + loc2 + "." + loc1 + "." + state);
		sym.SetStr(ITEM_REGION, state + ";" + loc2);

		sym.SetStr(ITEM_LENGTH, GetMetric(stripHTML(ExtractString(f.memory, ">Sviluppo totale<", ">", "</span>"))));
		sym.SetStr(ITEM_DEPTH, GetMetric(stripHTML(ExtractString(f.memory, ">Dislivello<", ">", "</span>"))));
		sym.SetStr(ITEM_LONGEST, GetMetric(stripHTML(ExtractString(f.memory, ">Salto pi&ugrave; alto<", ">", "</span>"))));
		sym.SetNum(ITEM_RAPS, CDATA::GetNum(stripHTML(ExtractString(f.memory, ">Calate<", ">", "</span>"))));

		SWISSCANYON_GetSummary(sym, stripHTML(ExtractString(f.memory, ">Difficolt&agrave;<", ">", "</span>")));

		sym.SetStr(ITEM_SEASON, stripHTML(ExtractString(f.memory, ">Periodo dell'anno<", ">", "</span>")));
		sym.SetStr(ITEM_SHUTTLE, vars(stripHTML(ExtractString(f.memory, ">Navetta<", ">", "</span>"))).replace("si", "Yes").replace("Si", "Yes").replace("Nessun", "No").replace("circa", ""));

		sym.SetStr(ITEM_AKA, stripHTML(ExtractString(f.memory, ">Sinonimi<", ">", "</span>")));

		/*
		double lat = ExtractNum(f.memory, "place:location:latitude", "=\"", "\"");
		double lng = ExtractNum(f.memory, "place:location:longitude", "=\"", "\"");
		if (CheckLL(lat,lng))
			sym.SetStr(ITEM_LAT, MkString("@%s;%s", CData(lat), CData(lng)));
		*/

		return TRUE;
	}

	vars DownloadMore(const char *memory) override
	{
		if (!strstr(f.memory, "class=\"searchresult2\""))
			return "";
		return MkString("http://www.openspeleo.org/openspeleo/canyons.html?page=%d", ++p);
	}
};


class CENS : public BETAC
{
public:

	CENS(const char *base) : BETAC(base)
	{
		urls.push("http://www.cens.it/");
		x = BETAX("<li", "</li", ">Forre", "</ul");
		region = " Italy";
	}

	int DownloadPage(const char *url, CSym &sym) override
	{
		CSymList links;
		DownloadUrl(url, links, BETAX("<h2"), D_LINK);

		vara sketch;
		for (int i = 0; i < links.GetSize(); ++i)
			if (IsSimilar(links[i].data, "Sezione"))
				sketch.push(links[i].id);
		sym.SetStr(ITEM_LINKS, sketch.join(" "));

		return TRUE;
	}
};


class ECDC : public BETAC
{
public:

	ECDC(const char *base) : BETAC(base)
	{
		// urls.push("http://www.ecdcportugal.com/portugal-cds5l");
		//urls.push("file:beta\\ecdc.html");
		x = BETAX("<p");
		region = " Portugal";
		umain = "http://www.ecdcportugal.com/portugal-cds5l";
	}

	int DownloadBeta(CSymList &symlist) override
	{
		return DownloadHTML("beta\\ecdc.html", symlist, x);
	}
};


class MICHELEA : public BETAC
{
public:
	MICHELEA(const char *base) : BETAC(base)
	{
		ticks = 500;
		//urls.push();
		x = BETAX("<li", "</li", ">Gole e forre", ">Libro: ");

	}


	int DownloadPage(const char *url, CSym &sym) override
	{
		Throttle(tickscounter, ticks);
		if (f.Download(url))
		{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			return FALSE;
		}

		vars city = UTF8(stripHTML(ExtractString(f.memory, "<b>Centro", "<TD>", "</TD")));
		vars region = UTF8(stripHTML(ExtractString(f.memory, "<b>Regione", "<TD>", "</TD")));
		vars depth = stripHTML(ExtractString(f.memory, "<b>Dislivello", "<TD>", "</TD"));
		vars length = stripHTML(ExtractString(f.memory, "<b>Sviluppo", "<TD>", "</TD"));
		vars longest = stripHTML(ExtractString(f.memory, "<b>Verticale massima", "<TD>", "</TD"));
		vars shuttle = stripHTML(ExtractString(f.memory, "<b>Navetta", "<TD>", "</TD"));
		vars rock = stripHTML(ExtractString(f.memory, "<b>Roccia", "<TD>", "</TD"));


		vars address, oaddress;
		double lat, lng;
		if (!_GeoCache.Get(oaddress = address = city + ";" + GetToken(region, 0, ",;-") + ";Italy", lat, lng))
		{
			while (!ExtractStringDel(address, "(", "", ")").IsEmpty());
			if (!_GeoCache.Get(address, lat, lng))
				if (!_GeoCache.Get(address = city + ";" + region + ";Italy", lat, lng))
					if (!_GeoCache.Get(address = city + ";Italy", lat, lng))
						if (!_GeoCache.Get(address = city + ";" + region + ";Italy", lat, lng))
							address = address;
		}

		//if (CheckLL(lat,lng))
		//	if (address!=oaddress)
		//		address=MkString("%s;%s", CData(lat), CData(lng));
		sym.SetStr(ITEM_LAT, "@" + address);

		sym.SetStr(ITEM_AKA, sym.GetStr(ITEM_DESC));

		sym.SetStr(ITEM_REGION, "Italy;" + region);
		sym.SetStr(ITEM_DEPTH, GetMetric(depth));
		sym.SetStr(ITEM_LENGTH, GetMetric(length));
		sym.SetStr(ITEM_LONGEST, GetMetric(longest));
		sym.SetStr(ITEM_ROCK, rock);
		sym.SetStr(ITEM_SHUTTLE, shuttle.replace(" ", "").replace("Circa", "").replace("Possibile", "Optional ").replace("Si", "Yes").replace("---", "").replace(",", ".").replace("Indispensabile", "Yes").replace("Consigliabile", "Optional").replace("Consigl", "Optional").replace("Necessaria", "Yes")
			.replace("Necessaria", "Yes").replace("Decisa", "Yes").replace("Forte", "Yes").replace("Poco", "No").replace("Raccom", "Yes").replace("Sconsig", "No").replace("Senepu", "No"));
		return TRUE;
	}

	vara done;
	int DownloadRegion(const char *url, CSymList &symlist)
	{
		CSymList urllist;
		DownloadUrl(url, urllist, x, D_LINK);

		for (int i = 0; i < urllist.GetSize(); ++i)
		{
			vars url = urllist[i].id;
			if (done.indexOf(url) >= 0)
				continue;

			done.push(url);
			printf("%d %dD %s    \r", symlist.GetSize(), done.length(), url);


			if (strstr(url, "/scheda"))
			{
				urllist[i].SetStr(ITEM_DESC, UTF8(urllist[i].GetStr(ITEM_DESC)));
				DownloadSym(urllist[i], symlist);
				continue;
			}

			DownloadRegion(url, symlist);
		}
		return urllist.GetSize();
	}

	int DownloadBeta(CSymList &symlist) override
	{
		return DownloadRegion("http://www.micheleangileri.com/cgi-bin/index.cgi?it", symlist);
	}
};


class ICOPRO : public BETAC
{
public:
	int page;
	ICOPRO(const char *base) : BETAC(base)
	{
		page = 1;
		urls.push("http://www.icopro.org/canyon/map-data/p/1");
		x = BETAX("content");
		umain = "http://www.icopro.org/canyon";
	}


	void DownloadPatch(vars &memory) override
	{
		memory.Replace("\\/", "/");
		memory.Replace("\\\"", "\"");
		memory.Replace("\\\'", "\'");

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

	int DownloadInfo(const char *data, CSym &sym) override
	{
		sym.data = sym.data;
		vars summary = ExtractString(strstr(data, "<dt>Rate"), "<dd", ">", "</dd");
		vars region = stripHTML(ExtractString(strstr(data, "<dt>Country"), "<dd", ">", "</dd"));
		vars geoloc = stripHTML(ExtractString(strstr(data, "<dt>Location"), "<dd", ">", "</dd")) + "." + region;
		double lat, lng;
		lat = ExtractNum(data, "\"lat\"", "\"", "\"");
		lng = ExtractNum(data, "\"lng\"", "\"", "\"");
		if (CheckLL(lat, lng) && Distance(lat, lng, -8.3183, 114.8607) > 100)
		{
			//sym.SetNum(ITEM_LAT, lat);
			//sym.SetNum(ITEM_LNG, lng);
			sym.SetStr(ITEM_LAT, MkString("@%s;%s", CData(lat), CData(lng)));
		}
		else
			if (!geoloc.IsEmpty())
			{
				sym.SetStr(ITEM_LAT, "@" + geoloc);
			}

		sym.SetStr(ITEM_REGION, region);
		GetSummary(sym, summary);
		return TRUE;
	}

	vars DownloadMore(const char *memory) override
	{
		vars last = ExtractString(memory, "\"isLast\"", ":", ",");
		if (IsSimilar(last, "true"))
			return "";
		return MkString("http://www.icopro.org/canyon/map-data/p/%d", ++page);
	}
};


class AICFORUM : public BETAC
{
public:
	AICFORUM(const char *base) : BETAC(base)
	{
	}

	int DownloadBeta(CSymList &symlist) override
	{
		ubase = "win.aic-canyoning.it/forum";

		DownloadFile f;

		const char *rurls[] = {
			"http://win.aic-canyoning.it/forum/default.asp?CAT_ID=3",
			"http://win.aic-canyoning.it/forum/default.asp?CAT_ID=8",
			NULL
		};

		for (int ru = 0; rurls[ru] != NULL; ++ru)
		{
			vars rurl = rurls[ru];
			if (f.Download(rurl))
			{
				Log(LOGERR, "ERROR: can't download url %.128s", rurl);
				return FALSE;
			}
			vara region(ExtractString(f.memory, ">Discussioni<", "", "<script"), "href=");
			for (int r = 1; r < region.length(); ++r)
			{
				vars url = ExtractString(region[r], "\"", "", "\"");
				if (url.IsEmpty() || !strstr(url, "forum.asp"))
					continue;
				vars regname = stripHTML(ExtractString(region[r], ">", "", "</a"));
				if (regname.IsEmpty())
					continue;

				regname = !ru ? "Italy; " + regname : regname;

				double maxpg = 1;
				for (int pg = 1; pg <= maxpg; ++pg)
				{
					vars urlpg = burl(ubase, url);
					if (pg > 1) urlpg += MkString("&whichpage=%d", pg);
					if (f.Download(urlpg))
					{
						Log(LOGERR, "ERROR: can't download url %.128s", urlpg);
						continue;
					}

					maxpg = ExtractNum(f.memory, ">Pagina:", " di ", "</");

					vara list(ExtractString(f.memory, ">Discussione<", "", ">Mostra Discussioni"), "<tr ");
					for (int i = 1; i < list.length(); ++i)
					{
						vara links, names, llist(list[i], "href=");
						for (int l = 1; l < llist.length(); ++l)
						{
							vars link = ExtractString(llist[l], "\"", "", "\"");
							vars name = stripHTML(ExtractString(llist[l], ">", "", "</a"));
							if (name.IsEmpty() || link.IsEmpty())
								continue;
							links.push(burl(ubase, link));
							names.push(name);
						}

						if (links.length() <= 0)
							continue;


						CSym sym(urlstr(links[0]), names[0].replace("\"", "").replace("0xC2", "").replace("0", "Z").replace("1", "A").replace("2", "B").replace("3", "C").replace("4", "D"));
						sym.SetStr(ITEM_REGION, ";" + regname);

						printf("%d %d/%d %d/%d    \r", symlist.GetSize(), i, list.length(), r, region.length());
						if (!UpdateCheck(symlist, sym) && MODE > -2)
							continue;

						if (DownloadPage(sym.id, sym))
							Update(symlist, sym, NULL, FALSE);
					}
				}
			}
		}

		return TRUE;
	}
};


class CANYONINGCULT : public AICFORUM
{
public:

	CANYONINGCULT(const char *base) : AICFORUM(base)
	{
	}
	
	int Download_File(const char *url, const char *folderfile, int pdf2html = FALSE)
	{
		if (CFILE::exist(folderfile))
			return TRUE;

		DownloadFile f(TRUE);
		if (f.Download(url, folderfile))
		{
			Log(LOGERR, "Could not download pdf %s from %s", folderfile, url);
			return FALSE;
		}
		/*
		vars folderhtmlfile = vars(folderfile)+".html";
		// convert and load
		if (pdf2html && !CFILE::exist(folderhtmlfile))
			{
			vars pdffile = "pdf2html.pdf";
			vars htmlfile = "pdf2htmls.html";
			CopyFile(folderfile, pdffile, FALSE);
			system(MkString("pdftohtml.exe %s", pdffile));
			CopyFile(htmlfile, folderhtmlfile, FALSE);
			// cleanup
			DeleteFile("pdf2html_ind.html");
			DeleteFile("pdf2html.html");
			DeleteFile("pdf2htmls.html");
			DeleteFile(pdffile);
			DeleteFile(htmlfile);
			}
		*/
		return TRUE;
	}

	int DownloadPage(const char *url, CSym &sym) override
	{
		//ASSERT(!strstr(sym.data, "Brezzi"));
		double maxpg = 1;
		int img = 0, pdf = 0;
		//vars folderfile = CCS"\\" + sym.GetStr(ITEM_DESC);
		vars folderfile = url2file(sym.GetStr(ITEM_DESC), CCS);
		for (int pg = 1; pg <= maxpg; ++pg)
		{
			vars urlpg = url;
			if (pg > 1) urlpg += MkString("&whichpage=%d", pg);
			if (f.Download(urlpg))
			{
				Log(LOGERR, "ERROR: can't download url %.128s", urlpg);
				return FALSE;
			}

			maxpg = ExtractNum(f.memory, ">Pagina:", " di ", "</");

			{
				vara links(f.memory, "href=");
				for (int i = 1; i < links.length(); ++i)
				{
					vars link = ExtractString(links[i], "\"", "", "\"");
					if (strstr(link, "/davidov/") || strstr(link, "/gebu/"))
						Download_File(burl("win.aic-canyoning.it", link), MkString("%s.pdf", folderfile), TRUE), ++pdf;
				}
			}

			{
				vara links(f.memory, "src=");
				for (int i = 1; i < links.length(); ++i)
				{
					vars link = ExtractString(links[i], "\"", "", "\"");
					if (strstr(link, "/davidov/") || strstr(link, "/gebu/"))
						Download_File(burl("win.aic-canyoning.it", link), MkString("%s%d.jpg", folderfile, ++img), FALSE);
				}
			}

		}

		//davidlink.Trim().Replace(" ", "%20");
		if (!pdf && !img)
			return FALSE;

		if (!pdf)
			return FALSE;

		sym.SetStr(ITEM_KML, MkString("#%d", pdf));
		return TRUE;
	}
};


class AICCATASTO : public BETAC
{
public:
	AICCATASTO(const char *base) : BETAC(base)
	{
		cfunc = DownloadConditions;
	}

	static int DownloadPage(DownloadFile &f, const char *url, CSym &sym)
	{
		if (f.Download(url))
		{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			return FALSE;
		}

		sym.SetStr(ITEM_CLASS, "0:base");

		double lat = ExtractNum(f.memory, "var lat", "=", ";");
		double lng = ExtractNum(f.memory, "var lon", "=", ";");
		if (CheckLL(lat, lng))
		{
			vars marker = ExtractString(f.memory, "google.maps.Marker", "(", ")");
			if (strstr(marker, "icon:"))
			{
				sym.SetStr(ITEM_LAT, MkString("@%s;%s", CData(lat), CData(lng)));
				sym.SetStr(ITEM_LNG, "");
				sym.SetStr(ITEM_CLASS, "0:NoLatLng");
			}
			else
			{
				sym.SetStr(ITEM_CLASS, "1:LatLng!");
				sym.SetNum(ITEM_LAT, lat);
				sym.SetNum(ITEM_LNG, lng);
			}
		}

		vars season = stripHTML(ExtractString(f.memory, ">Periodo", ":", "</li"));
		if (!season.IsEmpty())
			sym.SetStr(ITEM_SEASON, season);

		sym.SetStr(ITEM_DEPTH, GetMetric(stripHTML(ExtractString(f.memory, ">Dislivello", ":", "</li"))));
		sym.SetStr(ITEM_LENGTH, GetMetric(stripHTML(ExtractString(f.memory, ">Sviluppo", ":", "</li"))));
		sym.SetStr(ITEM_LONGEST, GetMetric(stripHTML(ExtractString(f.memory, ">Verticale Max", ":", "</li"))));
		sym.SetNum(ITEM_RAPS, CDATA::GetNum(stripHTML(ExtractString(f.memory, ">Numero Calate", ":", "</li"))));

		sym.SetNum(ITEM_SEASON, CDATA::GetNum(stripHTML(ExtractString(f.memory, ">Periodo", ":", "</li"))));

		vars interest = stripHTML(ExtractString(f.memory, ">Interesse", ":", "</li"));
		if (!strstr(interest, "SI"))
			sym.SetStr(ITEM_CLASS, "-1:" + interest);

		vars table = ExtractString(f.memory, ">Data discesa", "<tr", "</table");
		vars date = stripHTML(ExtractString(table, "<td", ">", "</td"));
		if (!date.IsEmpty())
		{
			double vdate = CDATA::GetDate(date);
			if (vdate > 0)
				sym.SetStr(ITEM_CONDDATE, CDate(vdate));
		}

		sym.SetStr(ITEM_AKA, PatchName(stripHTML(vars(ExtractString(strstr(f.memory, "class=\"page\""), "<h1", ">", "</h1")).replace("\n", "; "))));

		return TRUE;
	}

	static vars PatchName(const char *_name)
	{
		vars name(_name);
		//ASSERT( !strstr(name, "Lago") );
		name.Replace(" - ", ";");
		name.Replace(" o ", ";");
		name.Replace(" O ", ";");
		name.Replace("(", ";");
		name.Replace(")", ";");
		while (name.Replace(" ;", ";"));
		while (name.Replace(";;", ";"));
		return name.Trim(" ;");
	}

	int DownloadBeta(CSymList &symlist) override
	{
		vars rurl = "http://catastoforre.aic-canyoning.it/index/regioni";
		if (f.Download(rurl))
		{
			Log(LOGERR, "ERROR: can't download url %.128s", rurl);
			return FALSE;
		}

		vara region(ExtractString(f.memory, "id=\"mappa\"", "", "</div"), "href=");
		for (int r = 1; r < region.length(); ++r)
		{
			vars url = burl(ubase, ExtractString(region[r], "", "\"", "\""));
			vars regname = ExtractString(region[r], "title", "\"", "\"");
			regname = "Italy; " + regname;
			if (f.Download(url))
			{
				Log(LOGERR, "ERROR: can't download url %.128s", url);
				continue;
			}

			vara list(f.memory, "google.maps.Marker(");
			for (int i = 1; i < list.length(); ++i)
			{
				vars link = ExtractString(list[i], ".location", "\"", "\"");
				vars name = stripHTML(ExtractString(list[i], "title:", "\"", "\""));

				if (link.IsEmpty())
					continue;

				link = burl(ubase, link);
				ExtractStringDel(link, "reg/", "", "/");

				//ASSERT(!strstr(sym.data, "Brezzi"));

				CSym sym(urlstr(link), PatchName(name));
				//sym.SetStr(ITEM_AKA, name);
				sym.SetStr(ITEM_REGION, regname);

				printf("%d %d/%d %d/%d    \r", symlist.GetSize(), i, list.length(), r, region.length());
				if (!UpdateCheck(symlist, sym) && MODE > -2)
					continue;

				if (DownloadPage(f, sym.id, sym))
					Update(symlist, sym, NULL, FALSE);
			}
		}


		return TRUE;
	}
	
	static int DownloadConditions(const char *ubase, CSymList &symlist)
	{
		DownloadFile f;

		// download base
		vars url = "http://catastoforre.aic-canyoning.it/";
		if (f.Download(url))
		{
			Log(LOGERR, "Can't download base url %s", url);
			return FALSE;
		}
		vars memory = ExtractString(f.memory, ">Ultime forre scese", "<ul", "</ul");
		vara list(memory, "<li>");
		for (int i = 1; i < list.length(); ++i)
		{
			vars line = list[i];
			vars link = ExtractString(line, "href=", "\"", "\"");
			if (link.IsEmpty())
				continue;
			CSym sym(urlstr(burl(ubase, link)));
#if 0
			vars date = stripHTML(GetToken(line, 0, ":"));
			double vdate = CDATA::GetDate(date, "MMM D; YYYY");
			if (vdate <= 0)
				continue;

			vara cond; cond.SetSize(COND_MAX);
			cond[COND_DATE] = CDate(vdate);
			cond[COND_LINK] = sym.id;
			sym.SetStr(ITEM_CONDDATE, cond.join(";"));
			UpdateCond(symlist, sym, NULL, FALSE);
#else
			if (DownloadPage(f, sym.id, sym))
				UpdateCond(symlist, sym, NULL, FALSE);
#endif
		}

		return TRUE;
	}
};


class NEWMEXICOS : public BETAC
{
public:

	NEWMEXICOS(const char *base) : BETAC(base, BOOK_DownloadBeta)
	{
		umain = "http://dougscottart.com/hobbies/SlotCanyons.htm";
	}

#define AKAMARK "*AKA*"
	int DownloadPage(const char *url, CSym &sym) override
	{
		//ASSERT( !strstr(url, "chuveje") );
		if (f.Download(url))
		{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			return FALSE;
		}
		
		vara aka;
		vara akalist(ExtractString(f.memory, "class=WordSection", ">", ">ACA ").replace(">Spanish<", ">" AKAMARK "<").replace(">English<", ">" AKAMARK "<"), "<p");
		for (int i = 0; i < akalist.length(); ++i)
		{
			vars name = stripHTML(UTF8(ExtractString(akalist[i], ">", "", "</p")));
			name.Replace("English = ", AKAMARK);
			name.Replace("Spanish = ", AKAMARK);
			name.Replace(" or ", ";");
			//name.Replace(" & ", ";");
			name = GetToken(name, 0, "()");
			if (i <= 1 || name.Replace(AKAMARK, ""))
				if (!name.IsEmpty() && !strstr(name, "http"))
				{
					while (name.Replace(AKAMARK, ""));
					aka.push(name);
				}
		}
		if (aka.length() > 0)
			sym.SetStr(ITEM_AKA, aka.join(";"));
		else
			Log(LOGWARN, "Inalid AKA for %s", sym.id);

		vars sum = stripHTML(ExtractString(f.memory, ">ACA ", "", "</p"));
		GetSummary(sym, sum);

		vars tovisit = "to visit";
		vars season = stripHTML(ExtractString(f.memory, ">season", "", "</p"));
		if (IsSimilar(season, tovisit))
			season = season.Mid(tovisit.GetLength());
		const char *sseason = season;
		while (isspace(*sseason) || *sseason == '-')
			++sseason;
		sym.SetStr(ITEM_SEASON, sseason);

		vars coord = stripHTML(ExtractString(f.memory, "GPS ", "", "</p"));
		const char *start = coord;
		while (*start != 0 && !isdigit(*start))
			++start;
		if (*start != 0)
		{
			float lat = InvalidNUM, lng = InvalidNUM;
			GetCoords(start, lat, lng);
			if (CheckLL(lat, lng))
			{
				sym.SetNum(ITEM_LAT, lat);
				sym.SetNum(ITEM_LNG, lng);
			}
			else
				Log(LOGWARN, "Invalid GPS coords for %s", sym.id);
		}

		return TRUE;
	}

	int DownloadBeta(CSymList &symlist) override
	{
		DownloadFile f;
		vars rurl = burl(ubase, umain);
		if (f.Download(rurl))
		{
			Log(LOGERR, "ERROR: can't download url %.128s", rurl);
			return FALSE;
		}

		vara list(f.memory, "hobbies/SlotCanyons/");
		for (int i = 1; i < list.length(); ++i)
		{
			vars link = GetToken(list[i], 0, "\"");
			vars name = stripHTML(UTF8(ExtractString(list[i], ">", "", "</a")));
			if (link.IsEmpty())
				continue;

			CSym sym(urlstr("http://dougscottart.com/hobbies/SlotCanyons/" + link), name);
			sym.SetStr(ITEM_REGION, "New Mexico");
			printf("%d %d/%d    \r", symlist.GetSize(), i, list.length());
			if (!UpdateCheck(symlist, sym) && MODE > -2)
				continue;

			if (DownloadPage(sym.id, sym))
				Update(symlist, sym, NULL, FALSE);
		}

		return TRUE;
	}
};


class DOBLEVUIT : public BETAC
{
public:
	DOBLEVUIT(const char *base) : BETAC(base)
	{
		urls.push("http://doblevuit.com/blog/barrancs-i-torrents/");
		x = BETAX("<h4", "</tr");
	}

	int DownloadPage(const char *url, CSym &sym) override
	{
		if (strstr(url, "/wp.me") || strstr(url, "?p="))
		{
			if (f.Download(url))
			{
				Log(LOGERR, "ERROR: can't translate wp.me link %.128s", url);
				return FALSE;
			}
			else
			{
				vars newlink = ExtractString(f.memory, "<link rel=\"canonical\"", "href=\"", "\"");
				if (newlink.IsEmpty())
				{
					Log(LOGERR, "ERROR: can't find canonical wp.me link %.128s", url);
					return FALSE;
				}
				sym.id = urlstr(newlink);
			}
		}

		sym.SetStr(ITEM_REGION, "Spain");
		return TRUE;
	}

};


class NEOTOPO : public BETAC
{
public:

	NEOTOPO(const char *base) : BETAC(base)
	{
		urls.push("http://neotopossport.blogspot.com.es/p/barranquismo.html");
		x = BETAX("<br", NULL, ">REALIZADOS", ">PENDIENTES");
	}

	int DownloadPage(const char *url, CSym &sym) override
	{
		if (strstr(url, "picasaweb.google.com"))
			return FALSE;
		sym.SetStr(ITEM_REGION, "Spain");
		return TRUE;
	}
};


class MALLORCAV : public BETAC
{
public:

	MALLORCAV(const char *base) : BETAC(base)
	{
		urls.push("http://www.mallorcaverde.es/menu-barrancos.htm");
		x = BETAX("<td", "</td", "banner-barrancos.jpg", "</table");
		region = "Spain";
	}

};


class ESCONAT : public BETAC
{
public:

	ESCONAT(const char *base) : BETAC(base)
	{
		urls.push("http://www.esconatura.com/barranquismo-listado.htm");
		x = BETAX("<tr", "</tr", ">Nombre", "</table");
		region = "Spain";
	}
};


class UNODEAVENTURAS : public BETAC
{
public:

	UNODEAVENTURAS(const char *base) : BETAC(base)
	{
		ticks = 500;
		urls.push("http://unodeaventuras.com/tag/canyoning/");
		x = BETAX("<tr", "</tr", ">nome itinerario", "</table");
	}

	int DownloadInfo(const char *data, CSym &sym) override
	{
		const char *str = strstr(data, "href=");
		if (str) str = strstr(str, "<td");
		if (str) str += 2;

		CSym condsym;
		if (ExtractLink(str, curl, condsym))
		{
			double vdate = CDATA::GetDate(condsym.data, "DD/MM/YY");
			if (vdate <= 0)
				return FALSE;

			vara cond;
			cond.SetSize(COND_MAX);

			cond[COND_DATE] = CDate(vdate);
			cond[COND_LINK] = condsym.id;
			sym.SetStr(ITEM_CONDDATE, cond.join(";"));
		}

		vars title = sym.GetStr(ITEM_DESC);
		sym.SetStr(ITEM_DESC, title.split(" - ").first());

		vars summary = ExtractString(str, "<td", ">", "</td");
		GetSummary(sym, summary);

		/*
		vars pre, aka = GetToken( sym.GetStr(ITEM_DESC), 0, '-').Trim();
		while (!(pre = ExtractStringDel(aka, "(", "", ")")).IsEmpty())
			aka = pre.Trim() + " " + aka.Trim();
		sym.SetStr(ITEM_AKA, aka.replace("  "," "));
		*/

		return TRUE;
	}

	int DownloadPage(const char *url, CSym &sym) override
	{
		Throttle(tickscounter, ticks);
		if (f.Download(url))
		{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			return FALSE;
		}
		double lat = ExtractNum(f.memory, "place:location:latitude", "=\"", "\"");
		double lng = ExtractNum(f.memory, "place:location:longitude", "=\"", "\"");
		if (CheckLL(lat, lng))
			sym.SetStr(ITEM_LAT, MkString("@%s;%s", CData(lat), CData(lng)));

		return TRUE;
	}

	vars DownloadMore(const char *memory) override
	{
		vars pagination = ExtractString(memory, "class=\"pagination\"", "", "</ul");
		vars url = burl("www.gulliver.it", ExtractHREF(strstr(pagination, "ref=\"#\"")));
		if (!strstr(url, "?"))
			return "";
		return url;
	}
};
