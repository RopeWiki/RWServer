
#include <afxinet.h>
#include <atlimage.h>
#include "stdafx.h"
#include "Trademaster.h"
#include "rw.h"
#include "Math.h"
#include <Xunzip.h>
#include "BetaC.h"
#include "BETAExtract.h"
#include "Code.h"
#include "CPage.h"
#include "GeoRegion.h"
#include "AllBetaSiteHeaders.h"

#include "passwords.h"


GeoCache _GeoCache;
GeoRegion _GeoRegion;


// ===============================================================================================

Code *codelist[] = {
	new Code(0,0,"rw", NULL, "Ropewiki", NULL,new BETAC(RWID, ROPEWIKI_DownloadBeta), NULL),
	new Code(0,0,RWREDIR, NULL, "Redirects", NULL,new BETAC(RWTITLE, ROPEWIKI_DownloadRedirects), NULL),

	// local databases
	new Code(0,1,"kmlmap", NULL, NULL, NULL, new BETAC("KML:", KMLMAP_DownloadBeta), "World"),
	new Code(0,1,"book_GrandCanyon", NULL, "Grand Canyoneering Book by Todd Martin", "Grand Canyoneering Book", new BOOK("ropewiki.com/User:Grand_Canyoneering_Book"), "USA (Arizona)"),
	new Code(0,1,"book_Arizona", NULL, "Arizona Technical Canyoneering Book by Todd Martin", "Arizona Technical Canyoneering Book", new BOOK("ropewiki.com/User:Arizona_Technical_Canyoneering_Book"), "USA (Grand Canyon)"),
	new Code(0,1,"book_Zion", NULL, "Zion Canyoneering Book by Tom Jones", "Zion Canyoneering Book", new BOOK("ropewiki.com/User:Zion_Canyoneering_Book"), "USA (Zion)"),
	new Code(0,1,"book_Vegas", NULL, "Las Vegas Slots Book by Rick Ianiello", "Las Vegas Slots Book", new BOOK("ropewiki.com/User:Las_Vegas_Slots_Book"), "USA (Las Vegas)"),
	new Code(0,1,"book_Moab", NULL, "Moab Canyoneering Book by Derek A. Wolfe", "Moab Canyoneering Book", new BOOK("ropewiki.com/User:Moab_Canyoneering_Book"), "USA (Moab)"),
	new Code(0,1,"book_Ossola", NULL, "Canyons du Haut-Piemont Italien Book by Speleo Club de la Valle de la Vis", "Canyons du Haut-Piemont Italien Book", new BOOK("ropewiki.com/User:Canyons_du_Haut-Piemont_Italien_Book"), "Italy (Piemonte)"),
	new Code(0,1,"book_Alps", NULL, "Canyoning in the Alps Book by Simon Flower", "Canyoning in the Alps Book ", new BOOK("ropewiki.com/User:Canyoning_in_the_Alps_Book"), "Europe (Alps)"),
	new Code(0,1,"book_SwissAlps", NULL, "Canyoning in the Swiss Alps Book by Association Openbach", "Canyoning in the Swiss Alps Book ", new BOOK("ropewiki.com/User:Canyoning_in_the_Swiss_Alps_Book"), "Switzerland"),
	new Code(0,1,"book_NordItalia", NULL, "Canyoning Nord Italia Book by Pascal van Duin", "Canyoning Nord Italia Book ", new BOOK("ropewiki.com/User:Canyoning_Nord_Italia_Book"), "Italy (North)"),
	//new Code(0,"book_Azores", NULL, "Azores Canyoning Book by Desnivel", "Azores Canyoning Book ", new BETAC( "ropewiki.com/User:Azores_Canyoning_Book"), BOOKAZORES_DownloadBeta, "Azores" ),

	// USA kml extract
	new Code(1,1,"rtr", NULL, "RoadTripRyan.com", "RoadTripRyan.com", new BETAC("roadtripryan.com",ROADTRIPRYAN_DownloadBeta, ROADTRIPRYAN_ExtractKML), "USA", "1-5", "Disabled"),
	new Code(1,1,"cco", NULL, "CanyonCollective.com", "CanyonCollective.com", new BETAC("canyoncollective.com/betabase",CCOLLECTIVE_DownloadBeta, CCOLLECTIVE_ExtractKML, CCOLLECTIVE_DownloadConditions), "World", "1-5", "From KML data", "From Conditions"),
	new Code(1,1,"cch", NULL, "CanyonChronicles.com",  NULL, new BETAC("canyonchronicles.com",CCHRONICLES_DownloadBeta, CCHRONICLES_ExtractKML), "World", "", "From KML data"),
	new Code(1,1,"haz", NULL, "HikeArizona.com", "HikeArizona.com",new BETAC("hikearizona.com",HIKEAZ_DownloadBeta, HIKEAZ_ExtractKML, HIKEAZ_DownloadConditions), "USA", "1-5", "From Custom Data", "From Trip Reports"),
	new Code(1,1,"orc", NULL, "OnRopeCanyoneering.com",  NULL,new BETAC("onropecanyoneering.com",ONROPE_DownloadBeta, ONROPE_ExtractKML), "USA", "", "From GPX data"),
	new Code(1,1,"blu", NULL, "BluuGnome.com",  NULL,new BETAC("bluugnome.com",BLUUGNOME_DownloadBeta, BLUUGNOME_ExtractKML), "USA", "", "From HTML data"),
	new Code(1,1,"cba", NULL, "Chris Brennen's Adventure Hikes (St Gabriels)", "Chris Brennen's Adventure Hikes (St Gabriels)", new CBRENNEN("brennen.caltech.edu/advents"), "USA", "1-3 -> 1.5-4.5", "From HTML data"),
	new Code(1,1,"cbs", NULL, "Chris Brennen's Adventure Hikes (Southwest)", "Chris Brennen's Adventure Hikes (Southwest)", new CBRENNEN("brennen.caltech.edu/swhikes"), "USA", "1-3 -> 1.5-4.5", "From HTML data"),
	new Code(1,1,"cbw", NULL, "Chris Brennen's Adventure Hikes (World)", "Chris Brennen's Adventure Hikes (World)", new CBRENNEN("brennen.caltech.edu/world"), "World", "1-3 -> 1.5-4.5", "From HTML data"),

	// USA star extract
	new Code(1,1,"cus", NULL, "CanyoneeringUSA.com", "CanyoneeringUSA.com",new BETAC("canyoneeringusa.com",CUSA_DownloadBeta), "USA", "1-3 -> 1.5-4.5"),
	new Code(1,1,"zcc", NULL, "ZionCanyoneering.com", "ZionCanyoneering.com",new ZIONCANYONEERING("zioncanyoneering.com"), "USA", "1-5"),
	new Code(1,1,"asw", NULL, "AmericanSouthwest.net", "AmericanSouthwest.net",new BETAC("americansouthwest.net/slot_canyons",ASOUTHWEST_DownloadBeta),"USA", "1-5"),
	new Code(1,1,"thg", NULL, "ToddsHikingGuide.com", "ToddsHikingGuide.com",new TODDMARTIN("toddshikingguide.com/Hikes"), "USA", "1-5"),
	new Code(1,1,"dpm", NULL, "Dave Pimental's Minislot Guide", "Dave Pimental's Minislot Guide",new BETAC("math.utah.edu/~sfolias/minislot",DAVEPIMENTAL_DownloadBeta), "USA", "1-5"),

	// USA no extract
	new Code(1,1,"cnw", NULL, "CanyoneeringNorthwest.com",  NULL,new BETAC("canyoneeringnorthwest.com",CNORTHWEST_DownloadBeta), "USA / Canada"),
	new Code(1,1,"cut", NULL, "Climb-Utah.com",  NULL,new BETAC("climb-utah.com",CLIMBUTAH_DownloadBeta), "USA (Utah)"),
	new Code(1,1,"spo", NULL, "SummitPost.org",  NULL,new BETAC("summitpost.org",SUMMIT_DownloadBeta), "USA / World"),
	new Code(1,1,"cnm", NULL, "CanyoneeringNM.org",  NULL,new BETAC("canyoneeringnm.org",CNEWMEXICO_DownloadBeta), "USA (New Mexico)"),
	new Code(1,1,"nms", NULL, "Doug Scott's New Mexico Slot Canyons",  NULL,new NEWMEXICOS("dougscottart.com/hobbies/SlotCanyons/"), "USA (New Mexico)"),
	new Code(1,1,"SuperAmazingMap", NULL, "Super Amazing Map", NULL,new SAM("ropewiki.com/User:Super_Amazing_Map"), "USA"),
	new Code(1,0,"cond_can", NULL, "Candition.com",  NULL,new BETAC("candition.com/canyons",CANDITION_DownloadBeta, NULL, CANDITION_DownloadBeta), "USA", "", "", "From Conditions"),
	new Code(1,0,"cond_karl", NULL, "Karl Helser's NW Adventures",  NULL, new KARL("karl-helser.com"), "Pacific Northwest"),
	new Code(1,0,"cond_sixgun", NULL, "Mark Kilian's Adventures",  NULL, new SIXGUN("6ixgun.com"), "USA"),

	// UK & english
	new Code(1,1,"ukg", NULL, "UK CanyonGuides.org", "UK CanyonGuides.org",new BETAC("canyonguides.org",UKCG_DownloadBeta), "United Kingdom", "1-4 -> 2-5"),
	new Code(1,1,"kcc", NULL, "KiwiCanyons.org", "KiwiCanyons.org",new BETAC("kiwicanyons.org",KIWICANYONS_DownloadBeta), "New Zealand", "1-4 -> 2-5"),
	new Code(1,1,"ico", NULL, "Icopro.org",  NULL, new ICOPRO("icopro.org"), "World"),
	new Code(1,1,"cmag", NULL, "CanyonMag.net",  "CanyonMag.net", new CMAGAZINE("canyonmag.net/database"), "Japan / World", "1-4 -> 2-5"),

	// Spanish
	new Code(1,0,"bqn", "es", "Barranquismo.net", NULL, new BETAC("barranquismo.net" /*, BARRANQUISMO_DownloadBeta*/), "World"),
	new Code(1,1,"can", "es", "Ca\xC3\xB1onismo.com",  NULL,new BETAC("xn--caonismo-e3a.com",CANONISMO_DownloadBeta), "Mexico"),
	new Code(1,1,"jal", "es", "JaliscoVertical.Weebly.com",  NULL,new BETAC("jaliscovertical.weebly.com",JALISCO_DownloadBeta), "Mexico"),
	new Code(1,1,"alp", "es", "Altopirineo.com", "Altopirineo.com",new BETAC("altopirineo.com",ALTOPIRINEO_DownloadBeta), "Spain", "1-4 -> 2-5"),
	new Code(1,1,"ltn", "es", "Latrencanous.com",  NULL,new BETAC("latrencanous.com",TRENCANOUS_DownloadBeta), "Spain"),
	new Code(1,1,"bqo", "es", "Barrancos.org", "Barrancos.org",new BETAC("barrancos.org",BARRANCOSORG_DownloadBeta), "Spain / Europe", "1-10 -> 1-5"),
	new Code(1,1,"ddb", "es", "DescensoDeBarrancos.com",  NULL,new BETAC("descensodebarrancos.com",DESCENSO_DownloadBeta), "Spain / Europe"),
	new Code(9,1,"gua", "es", "Guara.info",  NULL,new BETAC("guara.info",GUARAINFO_DownloadBeta), "Guara (Spain)"),
	new Code(1,1,"4x4", "es", "Actionman4x4.com",  NULL,new ACTIONMAN4X4("actionman4x4.com/canonesybarrancos"), "Andalucia (Spain)"),
	new Code(1,1,"bcan", "es", "BarrancosCanarios.com",  NULL,new BCANARIOS("barrancoscanarios.com"), "Canarias (Spain)"),
	new Code(1,0,"bcue", "es", "BarrancosEnCuenca.es",  NULL,new BCUENCA("barrancosencuenca.es"), "Spain"),
	new Code(1,0,"tec", "es", "TeamEspeleoCanyons Blog",  NULL, new ESPELEOCANYONS("teamespeleocanyons.blogspot.com.es"), "Spain / Europe"),
	new Code(1,0,"sime", "es", "SiMeBuscasEstoyConLasCabras Blog",  NULL, new SIMEBUSCAS("simebuscasestoyconlascabras.blogspot.com"), "Spain / Europe", "", "From MAP"),
	new Code(1,0,"rocj", "es", "RocJumper.com",  NULL, new ROCJUMPER("rocjumper.com/barranco/"), "Spain / Europe"),
	new Code(1,1,"uno", "es", "UnoDeAventuras.com",  NULL, new UNODEAVENTURAS("unodeaventuras.com"), "World"),

	// Blogs
	new Code(1,0,"cond_nko", "es", "NKO-Extreme.com",  NULL, new NKO("nko-extreme.com"), "Spain / Europe"),
	new Code(1,0,"cond_bqe", "es", "Barranquistas.es",  NULL, new BARRANQUISTAS("barranquistas.es"), "Spain / Europe"),

	// Mallorca
	new Code(1,0,"dvc", "es", "Doblevuit.com",  NULL, new DOBLEVUIT("doblevuit.com"), "Mallorca (Spain)"),
	new Code(1,0,"nts", "es", "NeoToposSport",  NULL, new NEOTOPO("neotopossport.blogspot.com"), "Mallorca (Spain)"),
	new Code(1,0,"mve", "es", "MallorcaVerde.es",  NULL, new MALLORCAV("mallorcaverde.es"), "Mallorca (Spain)"),
	new Code(1,0,"enc", "es", "EscoNatura.com",  NULL, new ESCONAT("esconatura.com"), "Mallorca (Spain)"),
	new Code(1,0,"cond_365", "es", "365DiasDeUnaGuia",  NULL,new BETAC("http://365diasdeunaguia.wordpress.com",BOOK_DownloadBeta, NULL, COND365_DownloadBeta), "Spain", "", "", "From group updates"),
	new Code(1,0,"cond_inf", "es", "Caudal.info",  NULL,new BETAC("http://www.caudal.info",INFO_DownloadBeta, NULL, INFO_DownloadBeta), "World", "", "", "From Conditions"),

	// German
	new Code(1,1,"ccn", "de", "Canyon.Carto.net", "Canyon.Carto.net",new BETAC("canyon.carto.net",CANYONCARTO_DownloadBeta, CANYONCARTO_ExtractKML), "World", "1-7 -> 1-5", "From KML data"),

	// French
	new Code(1,1,"dcc", "fr", "Descente-Canyon.com", "Descente-Canyon.com",new BETAC("descente-canyon.com",DESCENTECANYON_DownloadBeta, DESCENTECANYON_ExtractKML, DESCENTECANYON_DownloadConditions), "World", "0-4 -> 1-5", "From Custom Data", "From Conditions"),
	new Code(1,1,"alt", "fr", "Altisud.com", "Altisud.com",new BETAC("altisud.com",ALTISUD_DownloadBeta), "Europe", "1-5"),
	new Code(1,1,"climb7", "fr", "Climbing7",  "Climbing7", new CLIMBING7("climbing7.wordpress.com"), "Europe", "1-3 -> 1.5-4.5"),
	new Code(1,1,"yad", "fr", "Yadugaz07", "Yadugaz07", new YADUGAZ("yadugaz07.com"), "France", "1-10 -> 1->5"),
	new Code(1,1,"reu", "fr", "Ligue Reunionnaise de Canyon", "Ligue Reunionnaise de Canyon", new REUNION("lrc-ffs"), "Reunion", "0-4 -> 1-5"),
	new Code(1,1,"flr", "fr", "Francois Leroux's Canyoning Reunion",  NULL, new FLREUNION("francois.leroux.free.fr/canyoning"), "Reunion"),
	new Code(1,1,"agc", "fr", "Alpes-Guide.com",  NULL, new ALPESGUIDE("alpes-guide.com/sources/topo/"), "France"),
	new Code(1,1,"tahiti", "fr", "Canyon de Tahiti", NULL,new TAHITI("canyon-a-tahiti.shost.ca"), "French Polynesia", "", "From PDF Index"),
	new Code(1,1,"mad", "fr", "An Kanion La-madinina", NULL,new MADININA("ankanionla-madinina.com"), "French Caribbean", "", "From PDF Index"),
	new Code(1,1,"mur", "fr", "Mur d'Eau Caraibe", "Mur d'Eau Caraibe",new MURDEAU("murdeau-caraibe.com"), "French Caribbean", "0-4 -> 1-5", "From KML data"),

	// Swiss
	new Code(1,1,"sch", "it", "SwissCanyon.ch", "SwissCanyon.ch",new BETAC("swisscanyon.ch",SWISSCANYON_DownloadBeta), "Switzerland", "1-4 -> 2-5"),
	new Code(1,1,"wch", "fr", "SwestCanyon.ch", "SwestCanyon.ch",new BETAC("swestcanyon.ch",SWESTCANYON_DownloadBeta), "Switzerland", "1-4 -> 2-5"),
	new Code(1,1,"sht", "de", "Schlucht.ch",  NULL,new BETAC("schlucht.ch",SCHLUCHT_DownloadBeta, NULL, SCHLUCHT_DownloadConditions), "Switzerland", "", "", "From Conditions"),

	// Italian
	new Code(1,1,"crc", "it", "CicaRudeClan.com", "CicaRudeClan.com",new BETAC("cicarudeclan.com",CICARUDECLAN_DownloadBeta), "World", "1-3 -> 1.5-4.5"),
	new Code(1,1,"est", "it", "Canyoneast.it", "Canyoneast.it",new BETAC("canyoneast.it",CANYONEAST_DownloadBeta), "Italy", "1-3 -> 3-5"),
	new Code(1,1,"tnt", "it", "TNTcanyoning.it",  NULL,new BETAC("tntcanyoning.it",TNTCANYONING_DownloadBeta), "Europe"),
	new Code(1,1,"vwc", "it", "VerticalWaterCanyoning.com", "VerticalWaterCanyoning.com",new BETAC("verticalwatercanyoning.com",VWCANYONING_DownloadBeta), "Europe", "0-4 / 1-7-> 1-5"),
	new Code(1,1,"aic", "it", "Catastoforre AIC-Canyoning.it",  NULL,new AICCATASTO("catastoforre.aic-canyoning.it"), "Italy", "", "", "From Conditions"),
	new Code(1,1,"osp", "it", "OpenSpeleo.org",  NULL, new OPENSPELEO("openspeleo.org/openspeleo/"), "Italy / Europe"),
	new Code(1,1,"gul", "it", "Gulliver.it",  NULL, new GULLIVER("gulliver.it"), "Italy / Europe"),
	new Code(1,1,"cens", "it", "Cens.it",  NULL, new CENS("cens.it"), "Italy"),
	//new Code(1,"aicf", "it", "Forum AIC-Canyoning.it",  NULL, new AICFORUM ( "aic-canyoning.it/forum"), "Italy / Europe"),
	new Code(1,1,"mac", "it", "Michele Angileri's Canyons",  NULL, new MICHELEA("micheleangileri.com"), "Italy"),

	// Others
	new Code(1,1,"ecdc", "pt", "ECDCPortugal.com",  NULL, new ECDC("media.wix.com"), "Portugal"),
	new Code(1,1,"cmad", "pt", "CaMadeira.com",  NULL, new CMADEIRA("canyoning.camadeira.com"), "Madeira"),
	new Code(1,1,"wro", "pl", "Canyoning.Wroclaw.pl", "Canyoning.Wroclaw.pl",new BETAC("canyoning.wroclaw.pl",WROCLAW_DownloadBeta), "Europe", "1-5"),

	// caves
	new Code(1,1,"sph", NULL, "Speleosphere.org",  NULL,new BETAC("speleosphere.org",SPHERE_DownloadBeta), "Guatemala"),
	new Code(1,1,"lao", NULL, "LaosCaveProject.de",  NULL,new BETAC("laoscaveproject.de",LAOS_DownloadBeta), "Laos"),
	//new Code(1,"wpc", NULL, "Wikipedia Cave List",  NULL,new BETAC( "en.wikipedia.org/wiki", WIKIPEDIA_DownloadBeta, "World" ),
	//new Code(8,"kpo", NULL, "KarstPortal.org",  NULL, KARSTPORTAL_DownloadBeta( "karstportal.org"), "World"),

	// Wikiloc
	new Code(5,0,"wik", "auto", "Wikiloc.com", "Wikiloc.com",new BETAC("wikiloc.com",WIKILOC_DownloadBeta, WIKILOC_ExtractKML), "World", "1-5", "From Custom data"),

	// manual invokation
	new Code(-1,0,"ccs", NULL, NULL, NULL,new CANYONINGCULT(NULL), "Europe"),
	new Code(-1,0,"bqnkml", NULL, NULL, NULL,new BETAC(NULL,BARRANQUISMOKML_DownloadBeta), "World"),
	new Code(-1,0,"bqndb", NULL, NULL, NULL, new BETAC("BQN:", BARRANQUISMODB_DownloadBeta), "World"),
	new Code(-1,0,"bqndbtest", NULL, NULL, NULL, new BETAC("BQN:", BARRANQUISMODB_DownloadBeta), "World"),

	new Code(-1,0, NULL,NULL,NULL,NULL,new BETAC(NULL), NULL)
};


int GetCode(const char *url) {

	// patch!
	if (strstr(url, "canyoneeringusa.com/rave"))
		return 0;

	for (int i = 1; codelist[i]->code; ++i)
	{
		const char *ubase = codelist[i]->betac->ubase;
		if (ubase && strstr(url, ubase))
		{
			// special case BQN:
			if (ubase[strlen(ubase) - 1] == ':')
				return i;

			// bad urls
			vara urla(url, "/");
			if (urla.length() < 3)
				return 0;
			return i;
		}
	}

	return 0;
}


const char *GetCodeStr(const char *url) {
	int c = GetCode(url);
	return c > 0 ? codelist[c]->code : NULL;
}


void LoadRWList()
{
	rwlist.Empty();
	rwlist.Load(filename(codelist[0]->code));
	rwlist.Sort();
}


BOOL isa(unsigned char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || static_cast<unsigned char>(c) > 127;
}


BOOL isanum(unsigned char c)
{
	return isdigit(c) || isa(c);
}


const char *skipnoalpha(const char *str)
{
	while (!isa(*str) && !(static_cast<unsigned char>(*str) > 127))
		++str;
	return str;
}


int IsImage(const char *url)
{
	const char *fileext[] = { "gif", "jpg", "png", "pdf", "tif", NULL };
	const char *ext = GetFileExt(url);
	return ext && IsMatch(vars(ext).lower(), fileext);
}


int IsGPX(const char *url)
{
	const char *fileext[] = { "gpx", "kml", "kmz", NULL };
	const char *ext = GetFileExt(url);
	return ext && IsMatch(vars(ext).lower(), fileext);
}


vars UTF8(const char *val, int cp)
{
	int len = strlen(val);
	int wlen = (len + 10) * 3;
	int ulen = (len + 10) * 2;
	void *wide = malloc(wlen);
	void *utf8 = malloc(ulen);
	int res = MultiByteToWideChar(cp, 0, val, len + 1, (LPWSTR)wide, wlen);
	WideCharToMultiByte(CP_UTF8, 0, (LPWSTR)wide, res + 1, (LPSTR)utf8, ulen, NULL, NULL);
	vars ret((const char *)utf8);
	free(wide);
	free(utf8);
	return ret;
}


vars ACP8(const char *val, int cp)
{
	int len = strlen(val);
	int wlen = (len + 10) * 3;
	int ulen = (len + 10) * 2;
	void *wide = malloc(wlen);
	void *utf8 = malloc(ulen);
	int res = MultiByteToWideChar(cp, 0, val, len + 1, (LPWSTR)wide, wlen);
	WideCharToMultiByte(CP_ACP, 0, (LPWSTR)wide, res + 1, (LPSTR)utf8, ulen, NULL, NULL);
	vars ret((const char *)utf8);
	free(wide);
	free(utf8);
	return ret;
}


vars makeurl(const char *ubase, const char *folder)
{
	if (*folder == 0)
		return "";
	if (IsSimilar(folder, "../"))
		folder += 2;
	if (IsSimilar(folder, "http"))
		return folder;
	if (IsSimilar(folder, "//"))
		return CString("http:") + folder;

	int root = *folder == '/';
	if (root) ++folder;

	vars base(ubase); base.Trim("/");
	vars url = !IsSimilar(base, "http") ? "http://" + base : base;

	vara ids(url.Trim("/"), "/");
	ids.SetSize(root || ids.length() <= 3 ? 3 : ids.length());

	return ids.join("/") + "/" + folder;
}


vars urlstr(const char *url, int stripkmlidx)
{
	// normalize url
	vars u(url);
	u.Trim(" \n\r\t\x13\x0A");
	const char *https = "https:";
	if (IsSimilar(u, https))
		u = "http:" + u.Mid(strlen(https));
	if (IsSimilar(u, "http")) {
		u = vars(url_decode(u));
		vars wiki = ExtractString(u, "//", "", WIKILOC);
		if (!wiki.IsEmpty())
			u.Replace("//" + wiki + WIKILOC, "//" WIKILOC);
		u.Replace(" ", "%20");
		u.Replace(",", "%2C");
		u.Replace("+", "%2B");
		u.Replace("/www.", "/");
		u.Replace("ajroadtrips.com", "roadtripryan.com");
		u.Replace("authors.library.caltech.edu/25057/2/advents", "brennen.caltech.edu/advents");
		u.Replace("authors.library.caltech.edu/25058/2/swhikes", "brennen.caltech.edu/swhikes");
		u.Replace("dankat.com/", "brennen.caltech.edu/");
		if (strstr(u, "translate.google.com")) {
			const char *uhttp = u;
			const char *http = strstr(uhttp + 1, "http");
			if (http) u = http;
		}

		if (stripkmlidx)
		{
			int f = u.Find("kmlidx");
			if (f > 0)
				u = u.Mid(0, f - 1);
		}

		u.Trim(" /?");
		if (strstr(u, "gulliver.it"))
			u += "/";
	}
	return u;
}


vars userurlstr(const char *url)
{
	if (strstr(url, "ropewiki.com/User:"))
		return GetToken(url, 0, '?');
	return url;
}


vars htmltrans(const char *string)
{
	vars str(string);
	str.Replace("&lt;", "<");
	str.Replace("&gt;", ">");
	str.Replace("&quot;", "\"");
	str.Replace("&apos;", "'");
	str.Replace("&amp;", "&");
	str.Replace("&nbsp;", " ");
	str.Replace("&ndash;", "-");
	str.Replace("&deg;", "o");
	str.Replace("&#039;", "\'");
	str.Replace("\\'", "\'");

	str.Replace("&ntilde;", "\xC3\xB1");
	str.Replace("&oacute;", "\xC3\xB3");
	str.Replace("&ograve;", "\xC3\xB3");
	str.Replace("&uacute;", "\xC3\xBA");
	str.Replace("&ugrave;", "\xC3\xBA");
	str.Replace("&iacute;", "\xC3\xAD");
	str.Replace("&igrave;", "\xC3\xAD");
	str.Replace("&eacute;", "\xC3\xA9");
	str.Replace("&egrave;", "\xC3\xA9");
	str.Replace("&aacute;", "\xC3\xA1");
	str.Replace("&agrave;", "\xC3\xA1");
	str.Replace("&rsquo;", "'");
	str.Replace("&#39;", "'");
	str.Replace("&#039;", "'");
	str.Replace("&#180;", "'");
	str.Replace("&#038;", "&");
	str.Replace("&#8217;", "'");
	str.Replace("&#8211;", "-");
	str.Replace("&#146;", "'");
	str.Replace("&#233;", "e");
	str.Replace("&#232;", "e");
	str.Replace("&#287;", "g");

	return str;
}


CString starstr(double stars, double ratings)
{
	if (stars == InvalidNUM)
		return "";
	if (stars > 0 && stars < 1)
		stars = 1;
	CString str = CData(stars) + "*";
	if (ratings == InvalidNUM)
		return str;
	return str + CData(ratings);
}


int Update(CSymList &list, CSym &newsym, CSym *chgsym, BOOL trackchanges)
{
	int f;
	++newsym.index;
	newsym.id = urlstr(newsym.id);

	if ((f = list.Find(newsym.id)) >= 0) {
		CSym &sym = list[f];
		++sym.index;
		vara o(sym.data), n(newsym.data);
		o.SetSize(ITEM_BETAMAX);
		n.SetSize(ITEM_BETAMAX);

		if (o.join() == n.join())
			return 0;

		if (n[ITEM_LAT][0] == '@' && o[ITEM_LAT][0] != '@' && o[ITEM_LAT][0] != 0)
			n[ITEM_LAT] = "";

		for (int i = 0; i < ITEM_BETAMAX; ++i)
			if (n[i].IsEmpty())
				n[i] = o[i];

		vars odata = o.join(), ndata = n.join();

		if (odata == ndata)
			return 0;

		// update changes
		sym = CSym(sym.id, ndata.TrimRight(","));
		++sym.index;

		// keep track of changes
		for (int i = 0; i < ITEM_BETAMAX; ++i)
			if (n[i] == o[i])
				n[i] = "";

		CSym tmpsym(sym.id, n.join().TrimRight(","));

		//ASSERT(!strstr(sym.data, "Lucky"));

		if (chgsym)
			*chgsym = tmpsym;

		if (trackchanges) {
			// test extracting kml
			if (MODE >= 0) {
				inetfile out(MkString("%u.kml", GetTickCount()));
				int kml = KMLEXTRACT ? KMLExtract(sym.id, &out, TRUE) : -1;
				Log(LOGINFO, "CHG KML:%d %s\n%s\n%s", kml, sym.id, odata, ndata);
			}
		}

		return -1;
	}

	list.Add(newsym);
	list.Sort();

	if (chgsym)
		*chgsym = newsym;

	if (trackchanges) {
		if (MODE >= 0) {
			inetfile out(MkString("%u.kml", GetTickCount()));
			int kml = KMLEXTRACT ? KMLExtract(newsym.id, &out, TRUE) : -1;

			Log(LOGINFO, "NEW kml:%d %s", kml, newsym.Line());
		}
	}

	return 1;
}


int UpdateCheck(CSymList &symlist, CSym &sym)
{
	const int found = symlist.Find(sym.id);

	if (found < 0)
		return TRUE;

	Update(symlist, sym, NULL, FALSE);
	sym = symlist[found];

	return FALSE;
}


int cmpconddate(const char *date1, const char *date2)
{
	int cmp = strncmp(date1, date2, 10);

	if (*date1 == 0)
		return -1;
	if (*date2 == 0)
		return 1;

	return cmp;
}


int UpdateOldestCond(CSymList &list, CSym &newsym, CSym *chgsym = NULL, BOOL trackchanges = TRUE)
{
	int f = list.Find(newsym.id);
	if (f >= 0)
	{
		int d = cmpconddate(newsym.GetStr(ITEM_CONDDATE), list[f].GetStr(ITEM_CONDDATE));
		if (d > 0 && d < 15) // max 0-15 days difference
			return FALSE;
	}
	Update(list, newsym, NULL, FALSE);

	return TRUE;
}


int UpdateCond(CSymList &list, CSym &newsym, CSym *chgsym, BOOL trackchanges)
{
	int f = list.Find(newsym.id);
	//ASSERT(!strstr(newsym.id, "=15546"));
	if (f >= 0 && cmpconddate(newsym.GetStr(ITEM_CONDDATE), list[f].GetStr(ITEM_CONDDATE)) < 0)
		return FALSE;
	Update(list, newsym, NULL, FALSE);

	return TRUE;
}


CString ExtractStringDel(CString &memory, const char *pre, const char *start, const char *end, int ext, int del)
{
	int f1 = memory.Find(pre);
	if (f1 < 0) return "";
	int f2 = memory.Find(start, f1 + strlen(pre));
	if (f2 < 0) return "";
	int f3 = memory.Find(end, f2 += strlen(start));
	if (f3 < 0) return "";
	int f3end = f3 + strlen(end);

	CString text = (ext > 0 ? memory.Mid(f1, f3end - f1) : (ext < 0 ? memory.Mid(f1, f3 - f1) : memory.Mid(f2, f3 - f2)));
	//Log(LOGINFO, "pre:%s start:%s end:%s", pre, start, end);
	//Log(LOGINFO, "%s", text);
	//ASSERT(!strstr(text,end));

	if (del > 0)
		memory.Delete(f1, f3end - f1);
	if (del < 0)
		memory.Delete(f1, f3 - f1);

	return text;
}


unit utime[] = { {"h", 1}, {"hr", 1}, {"hour", 1}, {"min", 1.0 / 60}, {"minute", 1.0 / 60}, { "day", 24}, {"hs", 1}, {"hrs", 1}, {"hours", 1}, {"mins", 1.0 / 60}, {"minutes", 1.0 / 60}, { "days", 24}, NULL };
unit udist[] = { {"mi", 1}, {"mile", 1}, {"km", km2mi}, {"kilometer", km2mi}, {"mis", 1}, {"miles", 1}, {"kms", km2mi}, {"kilometers", km2mi}, {"m", km2mi / 1000}, {"meter", km2mi / 1000}, {"ms", km2mi / 1000}, NULL };
unit ulen[] = { {"ft", 1}, {"feet", 1}, {"'", 1}, {"&#039;",1}, {"meter", m2ft}, {"m", m2ft}, {"fts", 1}, {"feets", 1}, {"'s", 1}, {"&#039;s",1}, {"meters", m2ft}, {"ms", m2ft}, NULL };


int matchtag(const char *tag, const char *utag)
{
	int c;

	for (c = 0; tag[c] == utag[c] && utag[c] != 0; ++c);

	if (c >= 3) return TRUE;

	return utag[c] == 0 && !isa(tag[c]);
}


int GetValues(const char *str, unit *units, CDoubleArrayList &time)
{
	CString vstr(str);
	vstr.MakeLower();
	vstr.Replace(" to ", "-");
	vstr.Replace("more than", "");

	// look for numbers
	vara list;
	vars elem;
	int mode = -1;
	for (const char *istr = vstr; *istr != 0; ++istr) {
		BOOL isnum = isdigit(*istr) || (mode > 0 && (*istr == '.' || *istr == '+'));
		BOOL issep = !isnum && strchr(" \t\n\r()[].", *istr) != NULL;
		if (mode != isnum || issep) {
			// separator
			if (!elem.IsEmpty())
				list.push(elem);
			elem = "";
		}
		if (!issep) {
			mode = isnum;
			elem += *istr;
		}
	}
	if (!elem.IsEmpty())
		list.push(elem);
	list.push("nounit");

	// process units

	for (int i = 1; i < list.length(); ++i) {
		double v = InvalidNUM, v2 = InvalidNUM;
		v = CDATA::GetNum(list[i - 1]);
		if (list[i] == "-")
			v2 = CDATA::GetNum(list[++i]), ++i;
		if (v2 != InvalidNUM && v == InvalidNUM)
			Log(LOGWARN, "Inconsistent InvalidNUM v-v2 for %s", str), v = v2;
		if (v == InvalidNUM)
			continue;
		const char *tag = list[i];

		if (!units) {
			// special case rappel# check is not unit of length
			for (int i = 0; ulen[i].unit != NULL; ++i)
				if (matchtag(tag, ulen[i].unit))
					return FALSE; // not # rappels
			if (v != InvalidNUM) time.AddTail(v);
			if (v2 != InvalidNUM) time.AddTail(v2);
			return v != InvalidNUM;
		}

		for (int u = 0; units[u].unit; ++u) {
			// find best unit match
			if (matchtag(tag, units[u].unit)) {
				if (v != InvalidNUM) time.AddTail(v*units[u].cnv);
				if (v2 != InvalidNUM) time.AddTail(v2*units[u].cnv);
			}
		}
	}
	time.Sort();
	return time.length();
}


double Avg(CDoubleArrayList &time)
{
	double avg = 0;
	int len = time.length();
	for (int i = 0; i < len; ++i)
		avg += time[i];
	return avg / len;
}


CString Pair(CDoubleArrayList &raps)
{
	if (!raps.GetSize())
		return "";
	CString str = CData(raps.Head());
	if (raps.GetSize() > 1 && raps.Head() != raps.Tail())
		str += "-" + CData(raps.Tail());
	return str;
}


int GetValues(const char *data, const char *str, const char *sep1, const char *sep2, unit *units, CDoubleArrayList &time)
{
#if 0	
	vara atime(data, str);
	for (int a = 1; a < atime.length(); ++a)
		GetValues(stripHTML(strval(atime[a], sep1, sep2)), units, time);
#else
	GetValues(stripHTML(ExtractString(data, str, sep1, sep2)), units, time);
#endif
	if (time.GetSize() == 0)
		return 0;
	return time.Tail() > 0;
}


CString GetMetric(const char *str)
{
	int i;
	vars len;
	while (*str != 0)
	{
		char c = *str++;
		switch (c)
		{
		case ',':
		case ';':
		case '\'':
			c = '.';
			break;
		case '.':
			for (i = 0; isdigit(str[i]); ++i);
			if (i == 3 || i == 0)
				continue;
			break;
		}
		len += c;
	}

	len.MakeLower();
	double length = CDATA::GetNum(len);
	if (length == InvalidNUM || length == 0)
		return "";
	if (length < 0)
		length = -length;

	const char *unit = len;
	while (!isa(*unit) && *unit != 0)
		++unit;
	if (IsSimilar(unit, "mi"))
		--unit;
	switch (*unit)
	{
	case 0:
	case 'm':
		unit = "m";
		break;
	case 'k':
		unit = "km";
		break;
	default:
		Log(LOGERR, "Invalid Metric unit '%s'", len);
		return "";
		break;
	}
	return CData(length) + unit;
}


int GetTimeDistance(CSym &sym, const char *str)
{
	// time and distance
	CDoubleArrayList dist, time;
	if (GetValues(str, utime, time)) {
		sym.SetNum(ITEM_MINTIME, time.Head());
		sym.SetNum(ITEM_MAXTIME, time.Tail());
	}

	if (GetValues(str, udist, dist))
		sym.SetNum(ITEM_HIKE, Avg(dist));

	return time.GetSize() > 0 || dist.GetSize() > 0;
}


int GetRappels(CSym &sym, const char *str)
{
	if (*str == 0)
		return FALSE;
	CDoubleArrayList raps;
	if (!GetValues(str, NULL, raps)) {
		Log(LOGWARN, "Invalid # of raps '%s'", str);
		return FALSE;
	}
	if (raps.Tail() >= 50) {
		Log(LOGERR, "Too many rappels %g from %s", raps.Tail(), str);
		return FALSE;
	}
	sym.SetStr(ITEM_RAPS, Pair(raps));
	if (raps.Tail() == 0)
		return TRUE; // non technical

	// length 
	CDoubleArrayList len;
	if (GetValues(str, ulen, len)) {
		if (len.Tail() <= 10) {
			Log(LOGERR, "Too short rappels %g from %s", len.Tail(), str);
			return FALSE;
		}
		if (len.Tail() > 400) {
			Log(LOGERR, "Too long rappels %g from %s", len.Tail(), str);
			return FALSE;
		}
		sym.SetNum(ITEM_LONGEST, len.Tail());
		return TRUE; // technical
	}

	Log(LOGWARN, "Ignoring %s raps w/o longest for %s", Pair(raps), sym.Line());
	sym.SetStr(ITEM_RAPS, "");
	return FALSE;
}


int GetNum(const char *str, int end, vars &num)
{
	num = "";

	int start;
	for (start = end; start > 0 && (isdigit(str[start - 1]) || str[start - 1] == '.'); --start);

	if (start == end)
		return FALSE;

	num = CString(str + start, end - start);
	return TRUE;
}


int GetSummary(vara &rating, const char *str)
{
	if (!str || *str == 0)
		return FALSE;

	const int extended = rating.length() == R_EXTENDED;

	vars strc(str);
	strc.Replace("&nbsp;", " ");
	if (!extended) strc.Replace(" ", "");
	strc.Replace("\"", "");
	strc.Replace("\'", "");
	strc.MakeUpper();
	strc += "  ";
	str = strc;

	int v, count = -1;
	int getaca = TRUE, getx = 0, getv = TRUE, geta = TRUE, geti = TRUE, getc = TRUE, getstar = TRUE, getmm = TRUE, gethr = TRUE, getpp = TRUE;

	for (int i = 0; str[i] != 0; ++i)
	{
		char c = str[i], c1 = str[i + 1];
		
		if (c == '%' && isanum(c1))
		{
			// %3C
			i += 2;
			continue;
		}
		
		if (c >= '1' && c <= '4')
		{
			if (getaca)
			{
				if (c1 == '(') {
					const char *end = strchr(str + i + 1, ')');
					if (end != NULL)
						c1 = end[1];
				}

				if (c1 >= 'A' && c1 <= 'C')
				{
					++i;

					if (IsSimilar(str + i, "AM")) //am/pm
					{
						i += 2;
						continue;
					}

					rating[R_T] = c;
					rating[R_W] = c1;

					// C1-C4
					//char c2 = str[i+1];
					if ((v = IsMatchN(str + i, rclassc)) >= 0)
					{
						rating[R_W] = rclassc[v];
						i += strlen(rclassc[v]) - 1;
					}

					getx = TRUE;
					getaca = FALSE;
					count = 0;
					continue;
				}
			}
		}
		
		if (getx && count < 3 && (v = IsMatchN(str + i, rxtra)) >= 0)
		{
			static const char *ignore[] = { "RIGHT", NULL };
			
			if (IsMatch(str + i, ignore))
				continue;
			
			// avoid 'for' 'or' etc
			const char *prior = "ABCIV1234+-/;";
			
			if (!strchr(prior, str[i - 1]))
				continue;
			
			rating[R_X] = rxtra[v];
			i += strlen(rxtra[v]) - 1;
			getx = FALSE;
			count = 0;
			continue;
		}

		if (c == 'V' && isdigit(c1))
		{
			if (getv && (v = IsMatchN(str + i, rverticalu)) >= 0)
			{
				rating[R_V] = rverticalu[v];
				rating[R_V].MakeLower();
				i += strlen(rverticalu[v]) - 1;
				getv = FALSE;
				count = 0;
				continue;
			}
		}

		if (c == 'A' && isdigit(c1))
		{
			if (geta && (v = IsMatchN(str + i, raquaticu)) >= 0)
			{
				rating[R_A] = raquaticu[v];
				rating[R_A].MakeLower();
				i += strlen(raquaticu[v]) - 1;
				geta = FALSE;
				count = 0;
				continue;
			}
		}

		if (!getaca && count < 3 && geti && (v = IsMatchN(str + i, rtime)) >= 0)
		{
			rating[R_I] = rtime[v];
			i += strlen(rtime[v]) - 1;
			geti = FALSE;
			count = 0;
			continue;
		}

		if ((!getv || !geta) && count < 3 && getc && (v = IsMatchN(str + i, rtime)) >= 0)
		{
			// avoid 'for' 'or' etc
			rating[R_C] = rtime[v];
			i += strlen(rtime[v]) - 1;
			getc = FALSE;
			count = 0;
			continue;
		}

		// extended summary
		if (extended)
		{
			vars num;
			
			if (c == '*' && getstar)
				if (GetNum(str, i, num))
				{
					rating[R_STARS] = num;
					getstar = FALSE;
					continue;
				}
			
			if (c == 'M' && c1 == 'M' && getmm)
			{
				if (str[i - 1] == 'X')
				{
					rating[R_TEMP] = "X";
					getmm = FALSE;
					continue;
				}
				
				if (GetNum(str, i, num))
				{
					rating[R_TEMP] = num;
					getmm = FALSE;
					continue;
				}
			}
			
			if (c == 'P' && c1 == 'P' && getpp)
			{
				if (GetNum(str, i, num))
				{
					rating[R_PEOPLE] = num;
					getpp = FALSE;
					continue;
				}
			}
			
			if (c == 'H' && c1 == 'R' && gethr)
			{
				if (GetNum(str, i, num))
				{
					rating[R_TIME] = num;
					gethr = FALSE;
					continue;
				}
			}
		}

		if (count >= 0 && (++count > 2 && !extended))
			break;
	}

	if (!getaca || !getv && !geta)
		return TRUE;
	
	if (extended && (!getstar || !getmm))
		return TRUE;
	
	return FALSE;
}


int GetSummary(CSym &sym, const char *str)
{
	vara rating; rating.SetSize(R_SUMMARY + 1);

	const int ret = GetSummary(rating, rating[R_SUMMARY] = str);

	vars sum = rating.join(";");
	sum.Replace("+", "");
	sum.Replace("-", "");
	sym.SetStr(ITEM_ACA, sum);
	return ret;
}


int GetClass(const char *type, const char *types[], int typen[])
{
	for (int i = 0; types[i] != NULL; ++i)
		if (strstr(type, types[i]) != NULL)
			return typen[i];
	//Log(LOGINFO, "Unknown type \"%s\"", type);
	return -1; // unknown
}


void SetClass(CSym &sym, int t, const char *type)
{
	CString ctype = MkString("%d:%s", t, type);
	/*
	// allow to promote offline
	int f = symlist.Find(urlstr(link));
	if (f>=0) {
		double olda = = symlist[f].GetNum(ITEM_CLASS);
		if (olda!=InvalidNUM && olda>a)
			type = symlist[f].GetStr(ITEM_CLASS);
		}
	*/
	sym.SetStr(ITEM_CLASS, ctype);
}


void SetVehicle(CSym &sym, const char *text)
{
	vars vehicle = "", road = text;
	if (strstr(road, "Passenger"))
		vehicle = "Passenger:" + road;
	if (strstr(road, "High"))
		vehicle = "High Clearance:" + road;
	if (strstr(road, "Wheel") || strstr(road, "4WD"))
		vehicle = "4WD - High Clearance:" + road;
	sym.SetStr(ITEM_VEHICLE, vehicle);
}


vars query_encode(const char *str)
{
	return vars(str).replace("+", "%2B");
}


CString GetASKList(CSymList &idlist, const char *query, rwfunc func)
{
	//rwlist.Empty();
	DownloadFile f;
	CString base = "http://ropewiki.com/api.php?action=ask&format=xml&curtimestamp&query=";

	vara fullurllist;
	double offset = 0;
	CString timestamp;
	vars dquery, dparam = "|%3FModification_date";
	BOOL moddate = strstr(query, "sort=Modification") != NULL;
	int step = moddate ? 500 : 500, n = 0;
	vars category = ExtractString(query, "Category:", "", "]");
	while (offset != InvalidNUM) {
		vars mquery = query;
		if (moddate)
			mquery = dquery + "<q>" + GetToken(query, 0, '|') + "</q>|" + GetTokenRest(query, 1, '|');
		CString url = base
			+ mquery + dparam
			+ "|limit=" + MkString("%d", step) + "|offset=" + MkString("%d", moddate ? 0 : (int)offset);
		printf("Downloading ASK %s %gO %dP (%dT)...\r", category, offset, n++, idlist.GetSize());
		//Log(LOGINFO, "%d url %s", (int)offset, url);
		if (f.Download(url))
		{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			break;
		}
		if (timestamp.IsEmpty())
			timestamp = ExtractString(f.memory, "curtimestamp=");

		vara list(f.memory, "<subject ");
		int size = list.length() - 1;
		double newoffset = ExtractNum(f.memory, "query-continue-offset=");
		if (newoffset != InvalidNUM && newoffset < offset) {
			Log(LOGERR, "Offset reset %g+%d -> %g", offset, size, newoffset);
			break;
		}
		offset = newoffset;

		if (moddate)
		{
			vars lastdate = ExtractString(list[size], "Modification date", "<value>", "</value>");
			dquery = "[[Modification_date::>" + lastdate + "]]";
		}

		vara dups;
		for (int i = 1; i < list.length(); ++i) {
			vars fullurl = ExtractString(list[i], "fullurl=");
			if (fullurllist.indexOf(fullurl) < 0)
			{
				fullurllist.push(fullurl);
				func(list[i], idlist);
				continue;
			}
			dups.push(fullurl);
		}

		if (dups.length() > 1)
			Log(LOGWARN, "Duplicated GetASKList %d urls %s", dups.length(), dups.join(";"));
		//printf("%d %doffset...             \r", idlist.GetSize(), offset);
	}
	return 	timestamp;
}


CString GetAPIList(CSymList &idlist, const char *query, rwfunc func)
{
	//rwlist.Empty();
	DownloadFile f;
	CString base = "http://ropewiki.com/api.php?action=query&format=xml&curtimestamp&"; base += query;

	int n = 0;
	vars cont = "continue=";
	CString timestamp;
	while (!cont.IsEmpty()) {
		vars mquery = query;
		printf("Downloading API %d (%dT)...\r", n++, idlist.GetSize());
		vars url = base + "&" + cont;
		//Log(LOGINFO, "%d url %s", (int)offset, url);
		if (f.Download(url))
		{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			break;
		}
		if (timestamp.IsEmpty())
			timestamp = ExtractString(f.memory, "curtimestamp=");

		// continue
		cont = ExtractString(f.memory, "<continue ", "", "/>");
		cont = cont.replace("\"", "").replace(" ", "&").Trim(" &");

		vara list(f.memory, "<page ");
		for (int i = 1; i < list.length(); ++i)
		{
			++n;
			func(list[i], idlist);
		}

	}
	return 	timestamp;
}


// ===================================== KML EXTRACT ============

void split(CStringArray &list, const char *data, int size, const char *sep)
{
	if (strlen(data) != size)
		Log(LOGERR, "inconsistency len:%d vs size:%d ", strlen(data), size);

	int lasti = 0;
	int datalen = size;
	int seplen = strlen(sep);
	list.RemoveAll();
	for (int i = 0; i < datalen - seplen; ++i)
		if (strnicmp(data + i, sep, seplen) == 0)
		{
			// flush out
			list.Add(CString(data + lasti, i - lasti));
			i = lasti = i + seplen;
		}
	// flush out
	list.Add(CString(data + lasti, datalen - lasti));
}


static int xmlid(const char *data, const char *sep, BOOL toend = FALSE)
{
	int datalen = strlen(data);
	int seplen = strlen(sep);
	if (toend && sep[seplen - 1] == '>')
		--seplen;
	for (int i = 0; i < datalen - seplen; ++i)
		if (strnicmp(data + i, sep, seplen) == 0)
		{
			if (!toend) return i;
			for (; i < datalen; ++i)
				if (data[i] == '>')
					return i + 1;
		}
	return -1;
}

vars xmlval(const char *data, const char *sep1, const char *sep2)
{
	int start = xmlid(data, sep1, TRUE);
	if (start < 0) return "";
	data += start;
	if (!sep2)
		return data;
	int end = xmlid(data, sep2);
	if (end < 0) return "";
	return vars(data, end);
}


vars strval(const char *data, const char *sep1, const char *sep2)
{
	CString ret;
	GetSearchString(data, "", ret, sep1, sep2);
	return vars(ret);
}


CString htmlnotags(const char *data)
{
	CString out;
	int ignore = FALSE;
	for (int i = 0; data[i] != 0; ++i)
	{
		if (data[i] == '?')
			continue;
		if (IsSimilar(data + i, CDATAS))
			i += strlen(CDATAS) - 1;
		else if (IsSimilar(data + i, CDATAE))
			i += strlen(CDATAE) - 1;
		else if (data[i] == '<' && ignore == 0)
		{
			// add space when dealing with <div>
			if (IsSimilar(data + i, "<div") || IsSimilar(data + i, "</div") || IsSimilar(data + i, "<br"))
				out += " ";
			++ignore;
		}
		else if (data[i] == '>' && ignore == 1)
			--ignore;
		else if (!ignore)
			out += data[i];
	}
	return out;
}


CString stripHTML(const char *data)
{
	if (!data) return "";

	CString out = htmlnotags(data);
	out = htmltrans(out);
	out.Replace("\x0D", " ");
	out.Replace("\x0A", " ");
	out.Replace("\t", " ");
	out.Replace(" #", " ");
	out.Replace(",", ";");
	out.Replace("\xC2\xA0", " ");
	out.Replace("\xC2\x96", "-");
	out.Replace("\xC2\xB4", "'");
	out.Replace("\xE2\x80\x99", "'");
	out.Replace("\xE2\x80\x93", "-");
	out.Replace("\xE2\x80\x94", "-");
	while (out.Replace("  ", " ") > 0);
	return out.Trim();
}


int CheckLL(double lat, double lng, const char *url)
{
	if (lat == InvalidNUM || lng == InvalidNUM || lat == 0 || lng == 0 || lat < -90 || lat>90 || lng < -180 || lng>180)
	{
		if (url)
			Log(LOGERR, "Invalid coordinates '%g,%g' for %s", lat, lng, url);
		return FALSE;
	}
	return TRUE;
}


vara getwords(const char *text)
{
	vars word;
	vara list;
	for (int i = 0; text[i] != 0; ++i)
	{
		char c = text[i];
		if (isanum(c))
		{
			word += c;
		}
		else
		{
			if (!word.IsEmpty())
				list.push(word), word = "";
			list.push(vars(c));
		}
	}
	if (!word.IsEmpty())
		list.push(word), word = "";
	return list;
}


BOOL TranslateMatch(const char *str, const char *pattern)
{
	int i;
	for (i = 0; str[i] != 0 && pattern[i] != 0 && pattern[i] != '*'; ++i)
	{
		if (tolower(str[i]) != tolower(pattern[i]))
			return FALSE;
	}
	return str[i] == pattern[i] || pattern[i] == '*';
}


vars Translate(const char *str, CSymList &list, const char *onlytrans)
{
	vara transwords;
	vara words = getwords(str);
	for (int i = 0; i < words.length(); ++i)
	{
		vars &word = words[i];
		const char *found = NULL;
		for (int j = 0; j < list.GetSize() && !found; ++j)
			if (TranslateMatch(word, list[j].id))
				found = list[j].data;
		if (found)
			transwords.push(word = found);
		else
			transwords.push("");
	}
	if (onlytrans)
		return transwords.join(onlytrans);
	return words.join("");
}


CString SeasonCompare(const char *season)
{
	CString str(season);
	str.Replace("-", " - ");
	str.Replace(",", " ");
	str.Replace(";", " ");
	str.Replace(".", " ");
	str.Replace("&", " ");
	str.Replace("/", " ");
	while (str.Replace("  ", " "));
	vara words(str, " ");
	for (int i = 0; i < words.length(); ++i)
	{
		for (int m = 0; m < months.length(); ++m)
			if (IsSimilar(words[i], months[m]))
			{
				words[i] = months[m];
				break;
			}
	}

	str = words.join(" ").MakeUpper();
	return str;
}


BOOL IsSeasonValid(const char *season, CString *validated)
{
	//vara sep("!?,(,but,");
	vara seasonst("Winter,Spring,Summer,Fall,Autumn");
	vara othert("Anytime,Any,time,All,year,After,rains");
	vara qualt("BEST,1,NOT,AVOID,EXCEPT,HOT,COLD,DRY,DIFFICULT,PREFERABLY");
	enum { Q_BEST = 0, Q_ONE };
	vara tempt("Early,Late");
	vara sept("-,to,through,thru,or,and");

	vara validt(months);
	validt.Append(seasonst);
	validt.Append(othert);
	vara valid(months);
	valid.Append(seasonst);
	for (int i = 0; i < othert.length(); ++i)
		valid.push((othert[i][0] == tolower(othert[i][0]) ? " " : "") + othert[i]);
	vara qual(qualt);
	for (int i = 0; i < qualt.length(); ++i)
		qual[i] = qualt[i] + " in ";
	vara sep(sept);
	for (int i = 0; i < sept.length(); ++i)
		sep[i] = " " + sept[i] + " ";
	vara temp(tempt);
	for (int i = 0; i < tempt.length(); ++i)
		temp[i] = tempt[i] + " ";

	int state = 0;
	int hasmonth = 0, best = 0;
	vars pre, presep;
	int last = 0;
	vara vlist;
	vara words = getwords(season);

	for (int i = words.length() - 1; i >= 0; --i)
	{
		// prefixes qualifiers
		if (i > 3 && i < words.length() - 4 && IsSimilar(words[i], "can") && IsSimilar(words[i + 2], "be"))
		{
			vars adj = words[i + 4];
			if (IsSimilar(adj, "very"))
			{
				words.RemoveAt(i + 4); // very
				if (i + 4 >= words.length())
					continue;
				words.RemoveAt(i + 4); // _
				if (i + 4 >= words.length())
					continue;
				adj = words[i + 4];
			}
			words.RemoveAt(i); // can
			words.RemoveAt(i); // _
			words.RemoveAt(i); // be
			words.RemoveAt(i); // _
			words.RemoveAt(i); // adj
			if (i < words.length() - 1 && IsSimilar(words[i + 1], "in"))
				words.InsertAt(i, adj);
			else
				words.InsertAt(i - 3, adj);
		}
		// prefixes qualifiers
		if (i > 3 && i < words.length() - 2 && IsSimilar(words[i], "are"))
		{
			vars adj = words[i + 2];
			if (IsSimilar(adj, "very"))
			{
				words.RemoveAt(i + 2); // very
				words.RemoveAt(i + 2); // _
				adj = words[i + 2];
			}
			words.RemoveAt(i); // are
			words.RemoveAt(i); // _
			words.RemoveAt(i); // adj
			words.InsertAt(i - 1, adj);
		}
	}
	for (int i = 0; i < words.length(); ++i)
	{
		vars &word = words[i];
		if (!words.IsEmpty())
		{
			BOOL ok = FALSE;
			// sep
			for (int v = 0; v < sept.length() && !ok; ++v)
				if (IsSimilar(word, sept[v]))
				{
					ok = TRUE;
					if (last <= 5)
					{
						presep = sep[v];
						last = 0;
					}
					pre = "";
				}
			// temp
			for (int v = 0; v < tempt.length() && !ok; ++v)
				if (IsSimilar(word, tempt[v]))
				{
					ok = TRUE;
					pre = temp[v];
					last = 0;
				}
			// qualif
			for (int v = 0; v < qualt.length() && !ok; ++v)
				if (IsSimilar(word, qualt[v]))
				{
					ok = TRUE;
					pre = qual[v];
					if (v == Q_BEST)
						best = TRUE;
					last = 0;
				}
			// seasons / months / other
			for (int v = 0; v < validt.length() && !ok; ++v)
				if (IsSimilar(word, validt[v]))
				{
					ok = TRUE;
					if (!presep.IsEmpty() && last <= 8)
						vlist.push(presep);
					if (!pre.IsEmpty() && last <= 8)
					{
						if (qual.indexOf(pre) == Q_ONE)
						{
							if (!presep.IsEmpty() && v < 12)
								v = v == 0 ? 11 : v - 1;
						}
						else
							vlist.push(pre);
					}
					presep = pre = "";
					vlist.push(valid[v]);
					if (v < 12)
						hasmonth = TRUE;
					last = 0;
				}
			++last;
		}
	}

	BOOL ok = vlist.length() > 0;

	// double check 
	int nsep = 0, nmonths = 0;
	for (int i = vlist.length() - 1; i >= 0; --i)
	{
		// trim '-'
		if (sep.indexOf(vlist[i]) >= 0)
		{
			if (i == 0 || i == vlist.length() - 1 || sep.indexOf(vlist[i + 1]) >= 0)
			{
				vlist.RemoveAt(i);
				continue;
			}
			++nsep;
			continue;
		}
		// trim qualif
		if (qual.indexOf(vlist[i]) >= 0)
		{
			if (i == vlist.length() - 1 || valid.indexOf(vlist[i + 1]) < 0)
			{
				ok = FALSE;
				vlist.RemoveAt(i);
				continue;
			}
			continue;
		}
		// trim temp
		if (temp.indexOf(vlist[i]) >= 0)
		{
			if (i == vlist.length() - 1 || seasonst.indexOf(vlist[i + 1]) < 0)
			{
				ok = FALSE;
				vlist.RemoveAt(i);
				continue;
			}
			continue;
		}
		// month and non-month
		if (hasmonth && months.indexOf(vlist[i]) < 0)
		{
			ok = FALSE;
			//vlist.RemoveAt(i);
			continue;
		}
		++nmonths;
	}

	if (hasmonth)
	{
		if (nsep > 0 && nmonths != 2 * nsep)
			ok = FALSE;
		if (nsep > 1)
			ok = FALSE;
		if (nmonths >= 2 && nsep == 0 && !best)
		{
			for (int i = 1; i < vlist.length() && ok; ++i)
			{
				int a = months.indexOf(vlist[i - 1]);
				int b = months.indexOf(vlist[i]);
				if (a >= 0 && b >= 0 && abs(a - b) > 1 && abs(a - b) < 11)
				{
					ok = FALSE;
					if (nmonths == 2)
						vlist.InsertAt(i, sep[0]);
				}
			}
		}
	}
	if (validated)
	{
		vars res = vlist.join(";");
		res.Replace(" ;", " ");
		res.Replace("; ", " ");
		res.Replace("  ", " ");
		res.Replace("BEST in After", "BEST After");
		res.Replace("Late After", "After");
		*validated = res.Trim(" -;");
		//ASSERT( !strstr(season, "early"));
	}
	return ok;
}

vars invertregion(const char *str, const char *add)
{
	vara nlist(add), list(vars(str).replace(",", ";"), ";");
	for (int i = nlist.length() - 1; i >= 0; --i)
		nlist[i].Trim();
	for (int i = list.length() - 1; i >= 0; --i)
	{
		list[i].Trim();
		if (nlist.indexOf(list[i]) < 0)
			nlist.push(list[i]);
	}
	return nlist.join(";");
}


vars regionmatch(const char *region, const char *matchregion)
{
	vars reg = GetToken(region, 0, '@').Trim();
	if (matchregion)
	{
		reg += " @ ";
		reg += matchregion;
	}
	return reg;
}

int SaveKML(const char *title, const char *credit, const char *url, vara &styles, vara &points, vara &lines, inetdata *out)
{
	// generate kml
	out->write(KMLStart());
	out->write(KMLName(MkString("%s %s", title, credit), vars(url).replace("&", "&amp;")));
	for (int i = 0; i < styles.length(); ++i)
		out->write(KMLMarkerStyle(GetToken(styles[i], 0, '='), GetToken(styles[i], 1, '='), 1, 0, GetToken(styles[i], 2, '=') == "marker"));
	for (int i = 0; i < points.GetSize(); ++i)
		out->write(points[i]);
	for (int i = 0; i < lines.length(); ++i)
		out->write(lines[i]);
	out->write(KMLEnd());
	return points.length() + lines.length();
}


vars GetKMLIDX(DownloadFile &f, const char *url, const char *search, const char *start, const char *end)
{
	vara ids(url, "kmlidx=");
	if (ids.length() > 1 && !ids[1].IsEmpty() && ids[1] != "X")
		return ids[1];

	if (DownloadRetry(f, url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return "";
	}
	if (search)
		return ExtractString(f.memory, search, start, end ? end : start);
	else
		return ExtractLink(f.memory, ".gpx");
}

int GPX_ExtractKML(const char *credit, const char *url, const char *memory, inetdata *out)
{
	if (!IsSimilar(memory, "<?xml"))
		return FALSE; // no gpx

	vara styles, points, lines;
	styles.push("dot=http://maps.google.com/mapfiles/kml/shapes/open-diamond.png");

	// process points
	vara wpt(memory, "<wpt ");
	for (int i = 1; i < wpt.length(); ++i)
	{
		vars data = wpt[i].split("</wpt>").first();
		vars id = stripHTML(xmlval(data, "<name", "</name"));
		vars desc = stripHTML(xmlval(data, "<desc", "</desc"));
		const vars cmt = stripHTML(xmlval(data, "<cmt", "</cmt"));
		if (desc.IsEmpty())
			desc = cmt;
		const double lat = CDATA::GetNum(strval(data, "lat=\"", "\""));
		const double lng = CDATA::GetNum(strval(data, "lon=\"", "\""));
		if (id.IsEmpty() || !CheckLL(lat, lng, url)) continue;
		
		// add markers
		points.push(KMLMarker("dot", lat, lng, id, desc + credit));
	}

	// process lines
	vara trk(memory, "<trkseg");
	for (int i = 1; i < trk.length(); ++i)
	{
		vara linelist;
		vars data = trk[i].split("</trkseg").first();
		vara trkpt(data, "<trkpt");
		for (int p = 1; p < trkpt.length(); ++p) {
			const double lat = CDATA::GetNum(strval(trkpt[p], "lat=\"", "\""));
			const double lng = CDATA::GetNum(strval(trkpt[p], "lon=\"", "\""));
			if (!CheckLL(lat, lng, url)) continue;
			linelist.push(CCoord3(lat, lng));
		}
		
		CString name = "Track";
		if (i > 1) name += MkString("%d", i);
		lines.push(KMLLine(name, credit, linelist, OTHER, 3));
	}

	if (points.length() == 0 && lines.length() == 0)
		return FALSE;

	// generate kml
	SaveKML("gpx", credit, url, styles, points, lines, out);
	
	return TRUE;
}


// ===============================================================================================

#define ucb(c, cmin, cmax) ((c>=cmin && c<=cmax))

#define isletter(c) ((c)>='a' && (c)<='z')


const char *nextword(register const char *pllstr)
{
	while (isletter(*pllstr) && *pllstr != 0) ++pllstr;
	while (!isletter(*pllstr) && *pllstr != 0) ++pllstr;
	return pllstr;
}


vars stripSuffixes(register const char* name)
{
	static CSymList dlist;
	if (dlist.GetSize() == 0)
	{
		dlist.Load(filename(TRANSBASIC"SYM"));
		dlist.Load(filename(TRANSBASIC"PRE"));
		dlist.Load(filename(TRANSBASIC"POST"));
		dlist.Load(filename(TRANSBASIC"MID"));
		int size = dlist.GetSize();
		for (int i = 0; i < size; ++i)
		{
			dlist[i].id.Trim(" '");
			dlist[i].index = dlist[i].id.GetLength();
		}
		dlist.Add(CSym("'"));
	}

	// basic2
	vara words = getwords(name);
	for (int w = 0; w < words.length(); ++w)
		for (int i = 0; i < dlist.GetSize(); ++i)
			if (stricmp(words[w], dlist[i].id) == 0)
			{
				words[w] = "";
				break;
			}

	vars str = words.join("");
	while (str.Replace("  ", " "));
	return str.Trim();
}


vars stripAccents(register const char* p)
{
	int makelower = FALSE;
	if (!p) return "";

	vars pdata = htmltrans(p); p = pdata;

	//   "ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ×ØÙÚÛÜÝÞßàáâãäåæçèéêëìíîïðñòóôõö÷øùúûüýþÿ"
	const char* trdup3 = "                               s                                ";
	const char* trup3 = "AAAAAAA EEEEIIIID OOOOOx0UUUUYPsaaaaaaa eeeeiiiio ooooo/0uuuuypy";
	const char* trlow3 = "aaaaaaaCeeeeiiiidNooooox0uuuuypsaaaaaaaceeeeiiiionooooo/0uuuuypy";

	//   "ĀāĂăĄąĆćĈĉĊċČčĎďĐđĒēĔĕĖėĘęĚěĜĝĞğĠġĢģĤĥĦħĨĩĪīĬĭĮįİıĲĳĴĵĶķĸĹĺĻļĽľĿ"
	const char* trdup4 = "                                                                ";
	const char* trup4 = "AaAaAaCcCcCcCcDdDdEeEeEeEeEeGgGgGgGgHhHhIiIiIiIiIiIiJjKkkLlLlLlL";
	const char* trlow4 = "aaaaaaccccccccddddeeeeeeeeeegggggggghhhhiiiiiiiiiiiijjkkklllllll";

	//   "ŀŁłŃńŅņŇňŉŊŋŌōŎŏŐőŒœŔŕŖŗŘřŚśŜŝŞşŠšŢţŤťŦŧŨũŪūŬŭŮůŰűŲųŴŵŶŷŸŹźŻżŽžſ"
	const char* trdup5 = "                                                                ";
	const char* trup5 = "lLlNnNnNnnNnOoOoOoOoRrRrRrSsSsSsSsTtTtTtUuUuUuUuUuUuWwYyYZzZzZzP";
	const char* trlow5 = "lllnnnnnnnnnoooooooorrrrrrssssssssttttttuuuuuuuuuuuuwwyyyzzzzzzp";

	const char *trups[] = { trup3, trup4, trup5 };
	const char *trlows[] = { trlow3, trlow4, trlow5 };
	const char *trdups[] = { trdup3, trdup4, trdup5 };

	vars ret;
	while (*p != 0)
	{
		unsigned char ch = (*p++);
		if (makelower && ch >= 'A' && ch <= 'Z')
			ch = ch - 'A' + 'a';
		else if (ch == 0xc3 || ch == 0xc4 || ch == 0xc5)
		{
			unsigned char ch1 = ch;
			unsigned char ch2 = *p++;

			int t = ch - 0xc3;
			const char* tr = makelower ? trlows[t] : trups[t];
			const char* trdup = trdups[t];
			if (ch2 >= 0x80)
			{
				int i = ch2 - 0x80;
				ch = tr[i];
				if (ch == ' ')
				{
					ret += ch1;
					ch = ch2;
				}
				if (trdup[i] != ' ')
					ret += trdup[i];
			}
			else
				continue; // skip
		}
		ret += ch;
	}

	ret.Replace("\xE2\x80\x99", "'");
	return ret;
}


vars stripAccentsL(register const char* p)
{
	vars ret = stripAccents(p);
	ret.MakeLower();
	return ret;
}


int IsUTF8c(const char *c)
{
	unsigned const char *uc = (unsigned const char *)c;
	/*
	U+0000..U+007F     00..7F
	U+0080..U+07FF     C2..DF     80..BF
	U+0800..U+0FFF     E0         A0..BF      80..BF
	U+1000..U+CFFF     E1..EC     80..BF      80..BF
	U+D000..U+D7FF     ED         80..9F      80..BF
	U+E000..U+FFFF     EE..EF     80..BF      80..BF
	U+10000..U+3FFFF   F0         90..BF      80..BF     80..BF
	U+40000..U+FFFFF   F1..F3     80..BF      80..BF     80..BF
	U+100000..U+10FFFF F4         80..8F      80..BF     80..BF
	*/
	if (ucb(c[0], 0, 0x7F))
		return 1;
	if (ucb(c[0], 0xC2, 0xDF))
		if (ucb(c[1], 0x80, 0xBF))
			return 2;
	if (ucb(c[0], 0xE1, 0xEC))
		if (ucb(c[1], 0x80, 0xBF))
			if (ucb(c[2], 0x80, 0xBF))
				return 3;
	if (c[0] == 0xED)
		if (ucb(c[1], 0x80, 0x9F))
			if (ucb(c[2], 0x80, 0xBF))
				return 3;
	if (c[0] == 0xE0)
		if (ucb(c[1], 0xA0, 0xBF))
			if (ucb(c[2], 0x80, 0xBF))
				return 3;
	if (ucb(c[0], 0xEE, 0xEF))
		if (ucb(c[1], 0x80, 0xBF))
			if (ucb(c[2], 0x80, 0xBF))
				return 3;
	if (c[0] == 0xF0)
		if (ucb(c[1], 0x90, 0xBF))
			if (ucb(c[2], 0x80, 0xBF))
				if (ucb(c[3], 0x80, 0xBF))
					return 4;
	if (ucb(c[0], 0xF1, 0xF3))
		if (ucb(c[1], 0x80, 0xBF))
			if (ucb(c[2], 0x80, 0xBF))
				if (ucb(c[3], 0x80, 0xBF))
					return 4;
	if (c[0] == 0xF4)
		if (ucb(c[1], 0x80, 0x8F))
			if (ucb(c[2], 0x80, 0xBF))
				if (ucb(c[3], 0x80, 0xBF))
					return 4;
	return FALSE;
}


int IsUTF8(const char *str)
{
	int utf = -1;
	while (*str != 0)
	{
		int n = IsUTF8c(str);
		if (n <= 0)
			return FALSE;
		if (n > 1)
			utf = TRUE;
		str += n;
	}
	return utf;
}


CString FixUTF8(const char *str)
{
	CString out;
	while (*str != 0)
	{
		int n = IsUTF8c(str);
		if (n <= 0)
		{
			++str;
			continue;
		}
		while (n > 0)
		{
			out += *str;
			++str;
			--n;
		}
	}
	return out;
}


CString KML_Watermark(const char *scredit, const char *memory, int size)
{
	CString credit(scredit);
	CString kml(memory, size);
	const char *desc = "</description>";
	const char *name = "</name>";
	vara names(kml, name);
	for (int i = 1; i < names.length(); ++i)
		if (names[i].Replace(desc, credit + desc) <= 0)
			names[i].Insert(0, "<description>" + credit + desc);
	return names.join(name);
}


int KMZ_ExtractKML(const char *credit, const char *url, inetdata *out, kmlwatermark *watermark)
{
	const char *desc = "</description>";
	CString newdesc = MkString("%s%s", credit, desc);
	DownloadFile f(TRUE);
	if (DownloadRetry(f, url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}
	// process KMZ
	if (strncmp(f.memory, "PK", 2) == 0)
	{
		// kmz
		//HZIP hzip = OpenZip(ZIPFILE,0, ZIP_FILENAME);
		HZIP hzip = OpenZip((void *)f.memory, f.size, ZIP_MEMORY);
		if (!hzip)
		{
			Log(LOGERR, "can't unzip memory kmz");
			return FALSE;
		}

		ZIPENTRY ze;
		for (int n = 0; GetZipItem(hzip, n, &ze) == ZR_OK; ++n)
		{
			char *ext = GetFileExt(ze.name);
			if (ext != NULL && stricmp(ext, "kml") == 0 || stricmp(ext, "kmz") == 0)
				//if (stricmp(ze.name,"doc.kml")==0)
			{
				// unzip and process
				int size = ze.unc_size;
				char *memory = (char *)malloc(size + 1);
				if (!memory)
				{
					Log(LOGERR, "can't unzip kmz %s", ze.name);
					continue;
				}
				UnzipItem(hzip, n, memory, size, ZIP_MEMORY);
				memory[size] = 0;
				out->write(KML_Watermark(credit, memory, size));
				free(memory);
			}
			else
			{
				// some other file
			}
		}

		CloseZip(hzip);
		return TRUE;
	}

	if (IsSimilar(f.memory, "<?xml")) {
		out->write(FixUTF8(watermark(credit, f.memory, f.size)));
		return TRUE;
	}

	Log(LOGERR, "Invalid KML/KMZ signature %s from %s", CString(f.memory, 4), url);
	return FALSE;
}


vars TableMatch(const char *str, vara &inlist, vara &outlist)
{
	if (!str || *str == 0)
		return "";

	for (int i = 0; i < inlist.length(); ++i)
		if (IsSimilar(str, GetToken(inlist[i], 0, '=')))
		{
			//if (water>0 && water<sizeof(waterconv)/sizeof(*waterconv))
			CString val = inlist[i].Right(1);
			ASSERT(val[0] >= '/' && val[0] <= '9');
			if (val[0] >= '0' && val[0] <= '9')
				return outlist[atoi(val)];
			return "";
		}

	ASSERT(!"Invalid value!");
	return "";
}


// ===============================================================================================

CString CoordsMemory(const char *memory)
{
	CString smemory(stripHTML(memory));
	smemory.Replace("°", "o");
	smemory.Replace("\r", "");
	smemory.Replace("\n", " ");
	smemory.Replace(";", " ");
	smemory.Replace("  ", " ");
	return smemory;
}


void GetCoords(const char *memory, float &lat, float &lng)
{
	lat = lng = InvalidNUM;
	vars text(memory);
	text.Replace("\xC2", " ");
	text.Replace("o", " ");
	text.Replace("°", " ");
	text.Replace("'", " ");
	text.Replace("\"", " ");
	text.Replace("\r", " ");
	text.Replace("\n", " ");
	text.Replace("\t", " ");
	text.Replace("  ", " ");
	text.Trim(" .;");
	int d = -1, lastalpha = -1, firstalpha = -1;
	for (d = strlen(text) - 1; d > 0 && !isdigit(text[d]); --d)
		if (isalpha(text[d]))
			lastalpha = d;
	if (lastalpha < 0) lastalpha = d;
	for (d = 0; d < lastalpha && !isdigit(text[d]); ++d)
		if (isalpha(text[d]))
			firstalpha = d;
	if (firstalpha < 0) firstalpha = d;
	text = text.Mid(firstalpha, lastalpha + 1);
	int div = -1, last = strlen(text) - 1;
	for (int i = 1; i < last && div < 0; ++i)
		if (isalpha(text[i]))
			div = i;

	const char *EW = "EeWw", *SN = "SsNn";
	if (isalpha(text[0]) && isalpha(text[last]))
		if (div >= 0 && strchr(SN, text[div]))
		{
			int skip = 0, len = strlen(text);
			while (skip < len && !isspace(text[skip]))
				++skip;
			if (skip < len) ++skip;
			div -= skip;
			text = text.Mid(skip);
		}
		else
		{
			int skip = strlen(text) - 1;
			while (skip > 0 && !isspace(text[skip]))
				--skip;
			text = text.Mid(0, skip).Trim();
		}

	const char *dtext = text;
	last = strlen(text) - 1;
	lat = lng = InvalidNUM;
	if (div >= 0 && div <= text.GetLength())
	{
		if (strchr(SN, text[0]) && strchr(EW, text[div]))
			lat = dmstodeg(text.Mid(0, div)), lng = dmstodeg(text.Mid(div));
		else if (strchr(EW, text[0]) && strchr(SN, text[div]))
			lng = dmstodeg(text.Mid(0, div)), lat = dmstodeg(text.Mid(div));
		else if (strchr(SN, text[div]) && strchr(EW, text[last]))
			lat = dmstodeg(text.Mid(0, div + 1)), lng = dmstodeg(text.Mid(div + 1));
		else if (strchr(EW, text[div]) && strchr(SN, text[last]))
			lng = dmstodeg(text.Mid(0, div + 1)), lat = dmstodeg(text.Mid(div + 1));
		else
			lng = lat = InvalidNUM;
	}
}


int ExtractCoords(const char *memory, float &lat, float &lng, CString &desc)
{
	lat = lng = InvalidNUM;
	const char *sep = " \r\t\n;.0123456789NnSsWwEe-o'\"";
	for (int i = 0; memory[i] != 0; ++i)
		if (memory[i] == 'o')
		{
			int start, end, endnum = 0, startnum = 0;
			for (end = i; memory[end] != 0 && strchr(sep, memory[end]); ++end)
				if (isdigit(memory[end]))
					endnum = TRUE;;
			if (!endnum) continue;
			for (start = i; start >= 0 && strchr(sep, memory[start]); --start)
				if (isdigit(memory[start]))
					startnum = TRUE;;
			if (!startnum) continue;

			int tstart = start;
			for (tstart = start; tstart > 0 && memory[tstart] != '.'; --tstart);
			desc = CString(memory + tstart, start - tstart + 1).Trim("(). \n\r\t");

			CString text(memory + start + 1, end - start - 1);
			GetCoords(text, lat, lng);
			if (CheckLL(lat, lng))
				return end; // continue search
		}
	return -1; // end search
}


int GetMin(const char *str, double &v2)
{
	while (isspace(*str))
		++str;
	if (isdigit(*str))
		if ((v2 = CDATA::GetNum(str)) != InvalidNUM)
			return TRUE;
	return FALSE;
}


const char *GetNumUnits(const char *str, const char *&num)
{
	num = NULL;
	while (!isdigit(*str) && *str != 0)
		++str;

	if (*str == 0)
		return NULL;

	num = str;
	while (isdigit(*str) || (*str == '.') || (*str == ' '))
		++str;

	if (*str == 0)
		return NULL;

	return str;
}


double GetHourMin(const char *ostr, const char *num, const char *unit, const char *url)
{
	char x = 0;
	if (unit) x = *unit;
	int nounit = FALSE;
	double vd = 0, vh = 0, vm = 0, v, v2;
	if ((v = CDATA::GetNum(num)) == InvalidNUM) {
		Log(LOGERR, "Invalid HourMin %s from %s", ostr, url);
		return InvalidNUM;
	}
	switch (x) {
	case 'j':
	case 'd':
		vd = v;
		break;

	case ':': // 2:15
	case 'h': // 2h45
		vh = v;
		if (GetMin(unit + 1, v2))
			vm = v2, unit = GetNumUnits(unit + 1, num);
		break;

	case 'm': // 45min
	case '\'': // 30'
		vm = v;
		if (GetMin(unit + 1, v2))
			vh = vm, vm = v2, unit = GetNumUnits(unit + 1, num);
		break;
	default:
		// autounit: >=5 min, otherwise hours
		nounit = TRUE;
		if (v >= 5)
			vm = v; // minutes
		else
			vh = v; // hours
		break;
	}

	// invalid units
	if (unit)
		if (IsSimilar(unit, "am") || IsSimilar(unit, "pm") || IsSimilar(unit, "x"))
		{
			// am/pm
			//Log(LOGERR, "Invalid HourMin units %s from %s", ostr, url);
			return InvalidNUM;
		}

	if (vd >= 5 || vd < 0 || vh >= 24 || vh < 0 || vm >= 120 || vm < 0)
	{
		Log(LOGERR, "Invalid HourMin %s = [%sd %sh %sm] from %s", ostr, CData(vd), CData(vh), CData(vm), url);
		return InvalidNUM;
	}

	if (nounit)
		Log(LOGWARN, "Possibly invalid HourMin %s = [%sh %sm]? from %s", ostr, CData(vh), CData(vm), url);

	if (vd == 0 && vh == 0 && vm == 0)
		vm = 5; // 5 min

	return vd * 24 + vh + vm / 60;
}


void GetHourMin(const char *str, double &vmin, double &vmax, const char *url)
{
	vmin = vmax = 0;
	if (*str == 0) {
		return;
	}
	/*
			if (breakutf) {
			out.Replace("\xC2", " ");
			out.Replace("\xA0", " ");
			}
	*/
	vars hmin;
	for (int i = 0; str[i] != 0; ++i)
	{
		char c = str[i];
		if (c < ' ' || c>127 || c == '/' || c == '-' || c == ',' || c == '+' || c == ';')
			c = '-';
		hmin += c;
	}

	hmin.MakeLower();
	hmin.Replace(" ou ", "-");
	hmin.Replace(" a ", "-");
	hmin.Replace("--", "-");
	hmin.Replace("--", "-");
	hmin.Replace("n-ant", "5m");
	hmin.Replace("de ", "");
	hmin.Replace(" j", "j");
	hmin.Replace(" h", "h");
	hmin.Replace(" m", "m");
	hmin.Replace("aprox", "");
	hmin.Replace("x1h", "h");
	hmin.Replace("y", "");

	//ASSERT(!strstr(url, "delika"));
	vara hmina(hmin, "-");
	CDoubleArrayList time;
	for (int i = 0; i < hmina.length(); ++i) {

		const char *num, *x, *unit;

		// get num and units (possible inherited)
		unit = GetNumUnits(hmina[i], num);
		for (int j = i + 1; !unit && j < hmina.length(); ++j)
			unit = GetNumUnits(hmina[j], x);
		// process value
		if (num != NULL)
		{
			double v = GetHourMin(hmina[i], num, unit, url);
			if (v > 0) time.AddTail(v);
		}
	}

	if (time.length() == 0)
		return;  // empty

	time.Sort();
	vmin = time[0];
	vmax = time[time.length() - 1];
}


double round2(double val)
{
	double ival = (int)val;
	double fval = val - ival;

	if (fval < 0.25) fval = 0;
	else if (fval < 0.75) fval = 0.5;
	else if (fval < 1) fval = 1.0;

	return ival + fval;
}


void GetTotalTime(CSym &sym, vara &times, const char *url, double maxtmin)  // -1 to skip tmin checks
{
	if (times.length() != 3)
	{
		Log(LOGWARN, "GETTIME: Invalid 3 times [len=%d], skipping %s", times.length(), url);
		return;
	}

	//CDoubleArrayList times;
	int error = 0;
	CDoubleArrayList tmins, tmaxs;
	double tmin = 0, tmax = 0;
	for (int i = 0; i < 3; ++i)
	{
		double vmin = 0, vmax = 0;
		if (times[i].trim().IsEmpty())
			return;
		GetHourMin(times[i], vmin, vmax, url);
		if (vmin <= 0)
		{
			++error;
			Log(LOGWARN, "GETTIME: invalid time '%s', skipping %s", times[i], url);
			return;
		}
		if (vmax <= 0) vmax = vmin;
		tmaxs.AddTail(vmax);
		tmins.AddTail(vmin);
		tmin += vmin;
		tmax += vmax;
	}


	if (tmin <= 0)
		return;

	if (maxtmin >= 0 && tmin > 0 && tmax > 0 && tmax >= tmin * 2)
	{
		Log(LOGWARN, "GETTIME: Invalid tmax (%s > 2*%s), skipping %s", CData(tmax), CData(tmin), url);
		return;
	}

	if (maxtmin >= 0 && tmin >= maxtmin)
	{
		Log(LOGWARN, "GETTIME: Very high tmin (%s > %s), skipping %s", CData(tmin), CData(maxtmin), url);
		return;
	}

	// all good, set values
	sym.SetNum(ITEM_MINTIME, tmin);
	sym.SetNum(ITEM_MAXTIME, tmax);

	int mins[] = { ITEM_AMINTIME, ITEM_DMINTIME, ITEM_EMINTIME };
	int maxs[] = { ITEM_AMAXTIME, ITEM_DMAXTIME, ITEM_EMAXTIME };
	for (int i = 0; i < 3; ++i)
	{
		sym.SetNum(mins[i], tmins[i]);
		sym.SetNum(maxs[i], tmaxs[i]);
	}
}


CString noHTML(const char *str)
{
	CString ret = stripHTML(str);
	ret.Replace(",", ";");
	ret.Trim("*");
	ret.Trim(".");
	return  ret;
}


// ===============================================================================================

vars ExtractHREF(const char *str)
{
	if (!str) return "";
	return GetToken(ExtractString(str, "href=", "", ">"), 0, " \n\r\t<").Trim("'\"");
}


vars url2url(const char *url, const char *baseurl)
{
	vara urla(url, "/");
	if (baseurl && !IsSimilar(urla[0], "http"))
	{
		urla = vara(baseurl, "/");
		urla[urla.length() - 1] = url;
	}
	return urla.join("/");
}


vars url2file(const char *url, const char *folder)
{
	vara urla(url, "/");
	if (urla.length() > 3)
	{
		urla.RemoveAt(0);
		urla.RemoveAt(0);
		urla.RemoveAt(0);
	}
	vars file = MkString("%s\\%s", folder, urla.join("_"));

	const char *subst = ":<>|?*\"";
	for (int i = 0; subst[i] != 0; ++i)
		file.Replace(subst[i], '_');
	file.Replace("%20", "_");
	return file;
}


BOOL IsBQN(const char *urlimg)
{
	return strstr(urlimg, "barranquismo.net") || strstr(urlimg, "barranquismo.org");
}


vars Download_SaveImg(const char *baseurl, const char *memory, vara &urlimgs, vara &titleimgs, int all = FALSE)
{
	CString mem(memory);
	// avoid comments
	while (!ExtractStringDel(mem, "<!--", "", "-->").IsEmpty());

	const char *astart1 = "<a";
	const char *astart2 = " href=";
	const char *aend = "</a>";
	mem.Replace("<A", astart1);
	mem.Replace(" HREF=", astart2);
	mem.Replace(" Href=", astart2);
	mem.Replace(" href=", astart2);
	mem.Replace("</A>", aend);

	vara links(mem, astart2);
	for (int i = 1; i < links.length(); ++i)
	{
		vars &link = links[i];
		vars &link1 = links[i - 1];
		vars urlimg = GetToken(link, 0, " >").Trim("\"' ");
		if (urlimg.IsEmpty())
			continue;

		urlimg = url2url(urlimg, baseurl);
		vars titleimg = stripHTML(ExtractString(link, ">", "", aend));

		// delete link
		int end = link.indexOf(aend);
		int start = link1.lastIndexOf(astart1);
		if (end < 0 || start < 0)
		{
			Log(LOGERR, "Invalid link, will get lost '%s', from %s", link, baseurl);
			continue;
		}
		link1 = link1.Mid(0, start);
		link = link.Mid(end + strlen(aend));
		if (!all && !IsBQN(urlimg))
		{
			// keep external links
			link = MkString("[%s %s]", urlimg, titleimg) + link;
			continue;
		}

		if (all || !strstr(urlimg, "xxxxx"))
			if (all || IsGPX(urlimg) || IsImage(urlimg))
				if (urlimgs.indexOf(urlimg) < 0)
				{
					// save link
					urlimgs.push(urlimg);
					titleimgs.push(titleimg);
				}

		// removed link
		link = titleimg + link;
	}
	return links.join(" ");
}


void Load_File(const char *filename, CString &memory)
{
	CFILE file;
	file.fopen(filename);
	int len = file.fsize();
	char *mem = (char *)malloc(len + 1);
	file.fread(mem, len, 1);
	mem[len] = 0;
	memory = mem;
}


int Download_Save(const char *url, const char *folder, CString &memory)
{
	vars savefile = url2file(url, folder);
	if (CFILE::exist(savefile))
	{
		Load_File(savefile, memory);
		// patch
		memory.Replace("\n", " ");
		memory.Replace("\t", " ");
		memory.Replace("\r", " ");
		memory.Replace("&nbsp;", " ");
		while (memory.Replace("  ", " "));

		// patch
		memory.Replace("<B>", "<b>");
		memory.Replace("</B>", "</b>");
		memory.Replace("</TR", "</tr");
		memory.Replace("<TR", "<tr");
		memory.Replace("<TD", "<td");
		memory.Replace("</TD", "</td");
		memory.Replace("<FONT", "<font");
		memory.Replace("</FONT", "</font");
		memory.Replace("<CENTER", "<center");
		memory.Replace("</CENTER", "</center");
		memory.Replace("<BR", "<br");
		memory.Replace("<P", "<p");
		memory.Replace("</P", "</p");
		memory.Replace("<DIV", "<div");
		memory.Replace("</DIV", "</div");
		memory.Replace("<b><font size=2>", "<font size=2><b>");
		memory.Replace("<b> <font size=2>", "<font size=2><b>");
	}

	DownloadFile f;
	if (f.Download(url))
	{
		//Log(LOGERR, "Could not download save URL %s", url);
		return FALSE;
	}

	memory = f.memory;
	CFILE file;
	if (file.fopen(savefile, CFILE::MWRITE))
	{
		file.fwrite(f.memory, f.size, 1);
		file.fclose();
	}

	vara urlimgs, titleimgs;
	Download_SaveImg(url, f.memory, urlimgs, titleimgs);
	for (int i = 0; i < urlimgs.length(); ++i)
	{
		vars urlimg = urlimgs[i];
		vars saveimg = url2file(urlimg, folder);
		if (CFILE::exist(saveimg))
			continue;
		DownloadFile f(TRUE);
		if (f.Download(urlimg))
		{
			Log(LOGERR, "Coult not download save URLIMG %s from %s", urlimg, url);
		}
		else
		{
			if (file.fopen(saveimg, CFILE::MWRITE))
			{
				file.fwrite(f.memory, f.size, 1);
				file.fclose();
			}
		}
	}
	return TRUE;
}


int KMLMAP_DownloadBeta(const char *ubase, CSymList &symlist)
{
	DownloadFile f;
	const char *maps[] = { "1JC0HCFFQxUbWxj_RiKkVANkOy7U", "1ArMXRHonoFF54K4QLl-AcXRtYb0", NULL };

	for (int m = 0; maps[m] != NULL; ++m)
	{
		vars mid = maps[m];//ExtractString(f.memory, "google.com", "mid=", "\"");
		vars url = "http://www.google.com/maps/d/u/0/kml?mid=" + mid + "&forcekml=1";
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
			vars name = stripHTML(ExtractString(list[i], "<name", ">", "</name"));
			vars name2 = ExtractString(name, "<![CDATA", "[", "]]>");
			if (!name2.IsEmpty())
				name = name2;

			name = stripHTML(name);

			//vars name = stripSuffixes(oname);
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
			vara linklist;
			vara links(list[i], "http");
			for (int i = 1; i < links.length(); ++i)
			{
				//vars pdflink = GetToken(links[i], 0, "[](){}<> ");
				vars link = GetToken(links[i], 0, "()<> ").Trim(" '\"");
				if (link.IsEmpty())
					continue;
				linklist.push(urlstr("http" + link));
			}
			CSym sym(ubase + name, name);
			sym.SetNum(ITEM_LAT, lat);
			sym.SetNum(ITEM_LNG, lng);
			sym.SetStr(ITEM_AKA, name);
			sym.SetStr(ITEM_LINKS, linklist.join(" "));

			int found;
			if (m > 0 && (found = symlist.Find(sym.id)) >= 0)
			{
				CSym &osym = symlist[found];
				int diff = 0;
				if (Distance(osym.GetNum(ITEM_LAT), osym.GetNum(ITEM_LNG), sym.GetNum(ITEM_LAT), sym.GetNum(ITEM_LNG)) > 1000)
					++diff;
				if (osym.GetStr(ITEM_LINKS) != sym.GetStr(ITEM_LINKS))
					++diff;
				if (diff)
				{
					// different
					sym.id += MkString("[%d]", m);
				}
			}

			Update(symlist, sym, NULL, FALSE);
		}
	}

	return TRUE;
}


// ===============================================================================================

double ExtractNum(const char *mbuffer, const char *searchstr, const char *startchar, const char *endchar)
{
	CString symbol;
	if (mbuffer) GetSearchString(mbuffer, searchstr, symbol, startchar, endchar);
	return CDATA::GetNum(symbol);
}


#define RWLANG "rwlang="


vars RWLANGLink(const char *purl, const char *lang)
{
	CString url = urlstr(purl);
	url += strchr(url, '?') ? '&' : '?';
	url += RWLANG;
	url += lang;
	return url;
}


vars KMLIDXLink(const char *purl, const char *pkmlidx)
{
	vars url = purl;
	vars kmlidx = pkmlidx;
	vars postfix;
	if (kmlidx.IsEmpty())
		postfix = "kmlidxna";
	else if (kmlidx != "X")
		postfix = "kmlidx=" + kmlidx;
	if (!postfix.IsEmpty())
		url += CString(strchr(url, '?') ? '&' : '?') + postfix;
	return url;
}


const char *GetPageName(const char *url)
{
	const char *sep = strrchr(url, '/');
	if (!sep) return NULL;
	return sep + 1;
}


class PLL : public LL
{
public:
	CSym *ptr;

	PLL(CSym *p = NULL) : LL()
	{
		ptr = p;
	}

	PLL(const LL &o, CSym *p = NULL)
	{
		lat = o.lat;
		lng = o.lng;
		ptr = p;
	}
};


class PLLD {
public:
	int len;
	double d;
	PLL *ll;

	PLLD(void) { d = 0; ll = NULL; };
	PLLD(double d, PLL *ll) { this->d = d; this->ll = ll; ASSERT(ll->ptr); };



	static int cmp(PLLD *b1, PLLD *b2)
	{
		if (b1->d > b2->d) return 1;
		if (b1->d < b2->d) return -1;
		return 0;
	}
};


class PLLRect : public LLRect
{
public:
	LL ll;
	CSym *ptr;
	CArrayList <PLLD> closest;

	PLLRect(CSym *p = NULL) : LLRect()
	{
		ptr = p;
	}

	PLLRect(double lat, double lng, double dist, CSym *p = NULL)
	{
		ll.lat = (float)lat;
		ll.lng = (float)lng;
		LLRect o(lat, lng, dist);
		lat1 = o.lat1;
		lat2 = o.lat2;
		lng1 = o.lng1;
		lng2 = o.lng2;
		ptr = p;
	}
};


typedef CArrayList <PLL> PLLArrayList;
typedef CArrayList <PLLRect> PLLRectArrayList;


int addmatch(PLLRect *r, PLL *lle, void *data) {
	//ASSERT( !strstr(r->ptr->GetStr(ITEM_DESC), "Tagliol") || !strstr(lle->ptr->GetStr(ITEM_DESC), "Gelli"));
	r->closest.AddTail(PLLD(lle->Distance(&r->ll, lle), lle));
	return FALSE;
}


#if 0
int BestMatch(const char *prname, const char *pllname, int &perfect) {
	//ASSERT( !strstr(prname, "usella") );
	//ASSERT( !strstr(pllname, "usella") );

	int maxm = 0; perfect = 0;
	int rlen = strlen(prname), lllen = strlen(pllname);
	if (rlen <= 0 || lllen <= 0) return 0;

	/*
	for (register int i=0; i<rlen; ++i)
	  for (register int j=0; j<lllen; ++j)
		{
		register int m;
		for (m=0; prname[i+m]!=0 && pllname[j+m]!=0 && prname[i+m]==pllname[j+m]; ++m);
		if (m>maxm)
			{
			maxm = m;
			if (maxm==rlen || maxm==lllen)
				perfect = TRUE;
			}
		}
	  */

	register const char *prstr = prname;
	while (*prstr != 0)
	{
		register const char *pllstr = pllname;
		while (*pllstr != 0)
		{
			register int m;
			for (m = 0; prstr[m] == pllstr[m] && prstr[m] != 0 && pllstr[m] != 0; ++m)
				if (!isletter(prstr[m]))
					perfect = TRUE;
			if (m > 0 && m >= maxm)
				//if (m<2 || isletter(prstr[m-2])) // avoid "Fondo d"
			{
				maxm = m;
				if (!isletter(prstr[m]) && !isletter(pllstr[m]))
					perfect = TRUE;
			}
			pllstr = nextword(pllstr);
		}
		prstr = nextword(prstr);
	}


	/*
		for (int i=0; i<lllen; ++i) {
			int m = 0;
			for (m=0; pllname[m+i]!=0 && prname[m]!=0 && prname[m]==pllname[m]; ++m);
			if (m>maxm)
				maxm = m;
		}
		if (rlen>=lllen)
			{
			if (strncmp(prname+rlen-lllen, pllname, lllen)==0)
				maxm = max(maxm, lllen);
			}
		else
			{
			if (strncmp(prname, pllname+lllen-rlen, rlen)==0)
				maxm = max(maxm, lllen);
			}

		CString extra;
		GetSearchString(pllname, "", extra, "(", ")");
		if (!extra.IsEmpty()) {
			int maxm2 = BestMatch(prname, extra+" "+GetToken(pllname, 0, '('));
			if (maxm2>maxm)
				return maxm2;
			}
	*/

	return maxm;
}
/*
int BestMatchAKA(const char *aka, const char *name)
{
		int maxi = 0;
		while (*aka!=0)
		{
			while (isspace(*aka))
				++aka;
			for (int i=0; aka[i]!=0 && aka[i]==name[i]; ++i);
			while (!isspace(*aka) && *aka!=0)
				++aka;
			if (i>maxi) maxi = i;
		}
		return maxi;
}
*/
#endif


CString rwsym(CSym *sym)
{
	return sym->id + ":" + sym->GetStr(ITEM_DESC) + ":" + sym->GetStr(ITEM_REGION) + ": [" + sym->GetStr(ITEM_AKA) + "]";
}


CString rwsym(PLLD &obj)
{
	return MkString(" %.1fkm %dL =", obj.d / 1000.0, obj.len) + rwsym(obj.ll->ptr);
}


#define MATCHLISTSEP "@;"


void MatchList(PLLRectArrayList &llrlist, PLLArrayList &lllist, Code &code)
{
	// find list of closest points
	LLMatch<PLLRect, PLL> mlist(llrlist, lllist, addmatch);
	for (int i = 0; i < llrlist.GetSize(); ++i)
	{
		PLLRect *prect = &llrlist[i];
		//CArrayList<CSym *> psymlist(size);
		//CIntListArray matchlist(size);
		printf("%d/%d Matching loc...         \r", i, llrlist.GetSize());

		vars prname = prect->ptr->GetStr(ITEM_BESTMATCH);
		vars prnamedesc = prect->ptr->GetStr(ITEM_DESC);
		vars prregion = prect->ptr->GetStr(ITEM_REGION);

		//ASSERT( !strstr(prect->ptr->id, "http://wikiloc.com/wikiloc/view.do?id=3937253"));
		//fm.fputstr(MkString("NEW: %s = %s", prname, prect->ptr->Line()));
		prect->closest.Sort(prect->closest[0].cmp);
		BOOL geoloc = strstr(prect->ptr->GetStr(ITEM_LAT), "@") != NULL;
		int mmax = -1, pmax = 0, mmaxj = -1;

		//ASSERT( !strstr(prect->ptr->data, "Escalante Natural Bridge") );
		//ASSERT( !strstr(prect->ptr->id, "http://wikiloc.com/wikiloc/view.do?id=6486258") );
		//ASSERT(!(strstr(prname, "escalante batural bridge") && ln>5));


		CArrayList <PLLD> closest;
		int size = prect->closest.GetSize();
		for (int j = 0; j < size; ++j) {
			double d = prect->closest[j].d;
			CSym *psym = prect->closest[j].ll->ptr; // rwsym
			BOOL pgeoloc = strstr(psym->GetStr(ITEM_LAT), "@") != NULL;

			//ASSERT( !strstr(prname, "basse") || !strstr(psym->GetStr(ITEM_DESC), "Basse"));

			// calc match length and keep best match
			int AdvancedBestMatch(CSym &sym, const char *prdesc, const char *prnames, const char *prregion, int &perfect);
			int p, m = AdvancedBestMatch(*psym, prnamedesc, prname, prregion, p);
			// precise loc 
			int add = d < DIST15KM && closest.length() < 15;
			// unprecise loc
			if (pgeoloc || geoloc)
				add += d < DIST150KM && closest.length() < 30 && m >= MINCHARMATCH && (m >= mmax || p);
			if (!add)
				continue;

			prect->closest[j].len = m;
			int added = closest.AddTail(prect->closest[j]);
			if ((m > mmax && !pmax) || (m > mmax && p && d < DIST15KM) || (m == mmax && p && !pmax))
				mmax = m, mmaxj = added, pmax = p;
		}

		char c = '*';
		CString match, newmatch;
		vara matchlist;
		if (mmax < 0 || closest.length() == 0) // no candidates nearby, must be new location
		{
			match = prname, newmatch = "+";
		}
		else
		{
			if (pmax) // perfect match
				match = rwsym(closest[mmaxj]), newmatch = "!" + rwsym(closest[mmaxj]);
			else	// multiple choice or not sure 
			{
				int nomatch = mmax <= MINCHARMATCH;
				if (nomatch) mmaxj = 0; // use closest match if not significant match
				match = rwsym(closest[mmaxj]);
				newmatch = (nomatch ? "?" : "~") + rwsym(closest[mmaxj]);
				//CRASHES! //Log(LOGINFO, "i=%d prname=%s", i, prname);
			}
			for (int j = 0; j < closest.length(); ++j)
			{
				CString line = geoloc ? "@" : "";
				line += MkString("%c", j == mmaxj ? '>' : '#') + rwsym(closest[j]);
				matchlist.push(line);
				//fm.fputstr(line);
			}
		}
		//ASSERT(prect->ptr->id!="http://descente-canyon.com/canyoning/canyon-description/21850");		
		//ASSERT( !strstr(prect->ptr->GetStr(ITEM_DESC), "eras"));
		prect->ptr->SetStr(ITEM_MATCH, match);
		prect->ptr->SetStr(ITEM_NEWMATCH, newmatch);
		if (matchlist.length() > 0)
			prect->ptr->SetStr(ITEM_MATCHLIST, matchlist.join(MATCHLISTSEP));
	}
}


int cmpmatchname(const void *arg1, const void *arg2)
{
	const vars *str1 = (const vars *)arg1;
	const char *num1 = *str1;
	while (!isdigit(*num1))
		++num1;
	double d1 = CDATA::GetNum(num1);

	const vars *str2 = (const vars *)arg2;
	const char *num2 = *str2;
	while (!isdigit(*num2))
		++num2;
	double d2 = CDATA::GetNum(num2);

	if (d1 < d2)
		return 1;
	if (d1 > d2)
		return -1;
	return 0;
}


void MatchName(CSym &sym, const char *region)
{

#if 0
	vars name = sym.GetStr(ITEM_BESTMATCH);
	vars match;
	vara matchlist;
	//int len = strlen(name);
	int mmax = -1, mmaxj = -1;
	//ASSERT(!strstr(name, "mascun"));
	// simple match
	for (int j = 0; j < rwlist.GetSize(); ++j) {
		CSym *psym = &rwlist[j];
		if (region && *region != 0 && *region != ';')
		{
			// must match region
			if (!strstri(GetFullRegion(psym->GetStr(ITEM_REGION), regions), region))
				continue;
		}
		//if (strstr(psym->data, "nfierno"))
		//	name = name;
		// calc match length and keep best match
		vars matchp;
		int perfect = 0, perfect2 = 0;
		int m = BestMatch(psym->GetStr(ITEM_BESTMATCH), name, perfect);
		if (m < MINCHARMATCH)
			continue;
		int m2 = BestMatchSimple(psym->GetStr(ITEM_DESC), sym.GetStr(ITEM_DESC));
		if (perfect || m >= MINCHARMATCH)
			matchlist.push(matchp = MkString("%s %2.2dL = %s", perfect ? "!" : "~", m2 > m ? m2 : m, rwsym(psym)));
		if (m > mmax)
			mmax = m, mmaxj = j, match = matchp;
	}

	// keep only best matches
	if (matchlist.length() > 1)
	{
		for (int i = matchlist.length() - 1; i >= 0; --i)
		{
			const char *mi = matchlist[i];
			if (CDATA::GetNum(mi + 1) < mmax)
				matchlist.RemoveAt(i);
		}
	}
#else

	// advance match
	CSymList reslist;
	vars name = sym.GetStr(ITEM_DESC);
	vars aka = sym.GetStr(ITEM_AKA);
	if (!strstri(name, aka))
		name += ";" + aka;
	int AdvancedMatchName(const char *linestr, const char *region, CSymList &reslist);
	int perfect = AdvancedMatchName(name, region, reslist);

	vara matchlist;
	for (int i = 0; i < reslist.GetSize(); ++i)
		matchlist.push(MkString("%s %2.2dL = %s", perfect > 0 ? "!" : "~", (int)reslist[i].GetNum(M_SCORE), rwsym(&reslist[i])));
#endif
	//ASSERT( !strstr(sym.GetStr(ITEM_DESC), "eras"));
	//if (matchlist.length()>0)
	vara addlist(sym.GetStr(ITEM_MATCHLIST), MATCHLISTSEP);
	addlist.Append(matchlist);

	/*
	for (int i=0; i<matchlist.length(); ++i)
		if (addlist.indexOf(matchlist[i])<0)
			addlist.push(matchlist[i]);
	addlist.sort(cmpmatchname);
	*/
	sym.SetStr(ITEM_MATCHLIST, addlist.join(MATCHLISTSEP));

	if (!strstr(sym.GetStr(ITEM_MATCH), RWID))
	{
		// only set if not set yet
		if (addlist.length() == 0)
		{
			sym.SetStr(ITEM_MATCH, name);
			sym.SetStr(ITEM_NEWMATCH, "+");
		}
		else
		{
			sym.SetStr(ITEM_MATCH, addlist[0]);
			sym.SetStr(ITEM_NEWMATCH, "=" + addlist[0]);
		}
	}
}


void LoadNameList(CSymList &rwnamelist)
{
	rwnamelist.Empty();
	for (int i = 0; i < rwlist.GetSize(); ++i)
		rwnamelist.Add(CSym(rwlist[i].GetStr(0), rwlist[i].id));
	rwnamelist.Sort();
}


int LoadBetaList(CSymList &bslist, int title, int rwlinks)
{
	// initialize data structures 
	if (rwlist.GetSize() == 0)
		LoadRWList();

	if (rwlist.GetSize() == 0)
		return FALSE;

	rwlist.Sort();

	// bslist
	for (int r = 0; r < rwlist.GetSize(); ++r)
	{
		vars id = title ? rwlist[r].GetStr(ITEM_DESC) : rwlist[r].id;
		vara urllist(rwlist[r].GetStr(ITEM_MATCH), ";");
		for (int u = 0; u < urllist.length(); ++u)
			bslist.Add(CSym(urlstr(urllist[u]), id));
		if (rwlinks)
		{
			vars title = rwlist[r].GetStr(ITEM_DESC);
			bslist.Add(CSym(urlstr("http://ropewiki.com/" + title.replace(" ", "_")), id));
		}
	}
	bslist.Sort();
	return TRUE;
}


int cmpsymid(const void *arg1, const void *arg2)
{
	return strcmp((*(CSym**)arg1)->id, (*(CSym**)arg2)->id);
}


vars GetBestMatch(CSym &sym);

static CSymList _bslist;


void MatchList(CSymList &symlist, Code &code)
{
	if (code.IsRW())
		return;

	printf("Matching list...         \r");

	static PLLArrayList lllist;

	CSymList &bslist = _bslist;
	if (bslist.GetSize() == 0) {
		LoadBetaList(bslist);

		// LLList
		//lllist.SetSize(rwlist.GetSize());
		for (int i = 0; i < rwlist.GetSize(); ++i) {
			printf("%d/%d Loading bslist...         \r", i, rwlist.GetSize());
			CSym &sym = rwlist[i];
			sym.SetStr(ITEM_BESTMATCH, GetBestMatch(sym));
			double lat = sym.GetNum(ITEM_LAT);
			double lng = sym.GetNum(ITEM_LNG);
			if (CheckLL(lat, lng))
				lllist.AddTail(PLL(LL(lat, lng), &sym));
		}

	}

	CArrayList <CSym *> psymlist;
	for (int s = 0; s < symlist.GetSize(); ++s)
	{
		CSym &sym = symlist[s];
		// skip manual matches
		if (!code.betac->ubase && strstr(sym.GetStr(ITEM_MATCH), RWLINK))
			continue;

		/*
		// restore mappings in ITEM_CLASS if lost
		if (!IsSimilar(sym.id, "http"))
			if (IsSimilar(sym.id, RWID))
				{
				// store map
				vars id = sym.GetStr(ITEM_INFO);
				if (!IsSimilar(id,RWID))
					sym.SetStr(ITEM_INFO, GetToken(sym.id, 0, "="));
				sym.id = GetToken(sym.id, 1, "=");
				}
		*/

		// match RWIDs
		vars id, tmp;
		if (IsSimilar(sym.id, RWID) && rwlist.Find(sym.id) >= 0)
			id = sym.id;
		tmp = sym.GetStr(ITEM_INFO);
		if (IsSimilar(tmp, RWID) && rwlist.Find(tmp) >= 0)
			id = tmp;
		if (IsSimilar(sym.id, RWTITLE))
			id = sym.id;

		// always match RWID:
		if (!id.IsEmpty() || IsSimilar(sym.id, RWTITLE))
		{
			sym.SetStr(ITEM_MATCH, id + RWLINK);
			sym.SetStr(ITEM_NEWMATCH, "");
			sym.SetStr(ITEM_MATCHLIST, "");
			sym.SetStr(ITEM_BESTMATCH, "");
			continue;
		}

		vars rwidkml = sym.GetStr(ITEM_KML);
		if (strstr(rwidkml, RWID))
		{
			sym.SetStr(ITEM_MATCH, rwidkml);
			continue;
		}
		//symlist[i].SetStr(ITEM_BETAMAX, "");
		psymlist.AddTail(&sym);
	}
	psymlist.Sort(cmpsymid);

	// map matched bslist
	PLLRectArrayList llrlist;
	int i = 0, bi = 0;
	int size = psymlist.GetSize();
	while (i < size) {
		printf("%d/%d Matching bslists...         \r", i, size);
		CSym &sym = *psymlist[i];
		sym.SetStr(ITEM_MATCH, "");
		sym.SetStr(ITEM_NEWMATCH, "");
		sym.SetStr(ITEM_MATCHLIST, "");
		sym.SetStr(ITEM_BESTMATCH, GetBestMatch(sym));
#if 1
#if 1
		if (bi < bslist.GetSize()) {
			vara rwids;
			int cmp;
			for (int dup = 0; (cmp = strcmp(sym.id, bslist[bi].id)) == 0; ++dup, ++bi)
			{
				// in bslist and symlist
				// bsym-->sym  sym-->bsym
				rwids.push(bslist[bi].GetStr(ITEM_DESC) + RWLINK);
			}
			if (rwids.length() > 0)
			{
				// in bslist and symlist
				// bsym-->sym  sym-->bsym
				sym.SetStr(ITEM_MATCH, rwids.join(";"));
				++i; continue;
			}
			if (cmp > 0)
			{
				// in bslist but not in symlist
				++bi; continue;
			}
		}
#else
		if (bi < bslist.GetSize()) {
			CSym &bsym = bslist[bi];
			int cmp = strcmp(sym.id, bsym.id);
			if (cmp == 0) {
				// in bslist and symlist
				// bsym-->sym  sym-->bsym
				sym.SetStr(ITEM_MATCH, bsym.GetStr(ITEM_DESC) + RWLINK);
				++i; ++bi; continue;
			}
			if (cmp > 0) {
				// in bslist but not in symlist
				++bi; continue;
			}
		}
#endif
#else
		vara ids;
		for (int cmp = 0; (bi < bslist.GetSize() && (cmp = strcmp(sym.id, bslist[bi].id)) >= 0); ++bi)
			if (cmp == 0)
				ids.push(bslist[++bi].GetStr(ITEM_DESC) + RWLINK);
		if (ids.length() > 0) {
			// in bslist and symlist
			// bsym-->sym  sym-->bsym
			sym.SetStr(ITEM_MATCH, ids.join(";"));
			sym.SetStr(ITEM_NEWMATCH, "");
			++i; continue;
		}
#endif

		// in symlist but not in ropewiki
		// find possible match based on location
		//ASSERT( !strstr(sym.GetStr(ITEM_DESC),"Ackersbach") );
		vars slat = sym.GetStr(ITEM_LAT);
		vars slng = sym.GetStr(ITEM_LNG);
		double lat = CDATA::GetNum(slat);
		double lng = CDATA::GetNum(slng);
		if (lat != InvalidNUM && lng == InvalidNUM)
			lng = CDATA::GetNum(GetToken(slat, 1, ';'));
		if (lat == InvalidNUM && slat[0] == '@')
			_GeoCache.Get(slat.Mid(1), lat, lng);
		if (CheckLL(lat, lng))
			llrlist.AddTail(PLLRect(lat, lng, DIST150KM, &sym)); // 150km
		++i;
	}

	MatchList(llrlist, lllist, code);

	// match by name and region
	for (int i = 0; i < symlist.GetSize(); ++i) {
		CSym &sym = symlist[i];
		if (sym.id.IsEmpty())
			continue;
		if (IsSimilar(sym.id, RWID) || IsSimilar(sym.id, RWTITLE))
			continue;
		if (strstr(sym.GetStr(ITEM_MATCH), RWLINK))
			continue;
		if (sym.GetNum(ITEM_LAT) != InvalidNUM && sym.GetNum(ITEM_LNG) != InvalidNUM)
			continue;
		// unmatched bc no coordinates
		printf("%d/%d Matching names...         \r", i, symlist.GetSize());
		MatchName(sym, code.Region(sym.GetStr(ITEM_REGION)));
	}

	//	LoadRWList();
}


double GetHourDay(const char *str)
{
	double num = CDATA::GetNum(str);
	if (num == InvalidNUM)
		return InvalidNUM;
	if (strchr(str, 'd'))
		return num * 24;
	return num;
}


BOOL InvalidUTF(const char *txt)
{
	const char *inv[] = { "\xef\xbf\xbd", NULL };
	for (int v = 0; inv[v] != NULL; ++v)
		if (strstr(txt, inv[v]))
			return TRUE;
	return FALSE;
}


vars ProcessAKA(const char *str)
{
	vars ret;
	int n = 0, i = 0, io = 0;
	const char *o[] = { " o ", " O ", "/", NULL };
	while (str[i] != 0)
	{
		if (i > 0 && str[i] == ' ' && isa(str[i - 1]) && (n = IsMatchN(str + i, o)) >= 0 && isa(str[i + strlen(o[n])]))
		{
			ret += ';';
			i += strlen(o[n]);
			continue;
		}
		ret += str[i++];
	}
	ret.Replace("Bco. ", "Barranco ");
	return ret;
}


int IsMidList(const char *str, CSymList &list)
{
	for (int i = 0; i < list.GetSize(); ++i)
	{
		const char *strid = list[i].id;
		while (*strid == ' ') ++strid;
		int l;
		for (l = 0; isanum(str[l]) && tolower(str[l]) == tolower(strid[l]); ++l);
		if (!isanum(str[l]) && !isanum(strid[l]))
			return TRUE;
	}
	return FALSE;
}


vars Capitalize(const char *str)
{
	static CSymList sublist, midlist, keeplist;
	if (sublist.GetSize() == 0)
		sublist.Load(filename(TRANSBASIC));
	if (midlist.GetSize() == 0)
		midlist.Load(filename(TRANSBASIC"MID"));
	if (keeplist.GetSize() == 0)
		keeplist.Load(filename(TRANSBASIC"SPEC"));

	vars out;
	int skip = FALSE;
	int n1 = 0;
	const char *c1 = NULL;
	while (*str != 0)
	{
		int icase = 0;
		const char *c = str;
		int n = IsUTF8c(c);
		if (isa(str[0]) || n > 1)
		{
			// letter / UTF
			if (!skip)
			{
				icase = -1;
				if (!c1 || !(isa(c1[0]) || n1 > 1))
				{
					icase = 1;
					if (IsMidList(str, sublist))
						icase = 0; // sublist = original
					else
						if (IsMidList(str, keeplist))
						{
							icase = 0; // keeplist = original ALL
							skip = TRUE;
						}
						else
							if (IsMidList(str, midlist))
								if (c1 && c1[0] == '(')
									icase = 0; // original
								else
									if (c1)
										icase = -1; // midlist = lower
				}
				if (c1 && c1[0] == '\'')  // Captain's  d'Ossule
					icase = (tolower(str[0]) == 's' && !isa(str[1]) && IsUTF8(str + 1) <= 1) ? -1 : 1;
			}
		}
		else
		{
			// simbol
			skip = FALSE;
			icase = 0;
		}

		// update
		c1 = str; n1 = n;

		if (n <= 1)
		{
			switch (icase)
			{
			case 0:
				out += str[0];
				break;
			case -1:
				out += (char)tolower(str[0]);
				break;
			case 1:
				out += (char)toupper(str[0]);
				break;
			}
			++str;
			continue;
		}

		CString tmp(str, n);
		switch (icase)
		{
		case 0:
			break;
		case -1:
			tmp = UTF8(ACP8(tmp).lower());
			break;
		case 1:
			tmp = UTF8(ACP8(tmp).upper());
			break;
		}
		out += tmp;
		str += n;
	}

	return out;
}


vars UpdateAKA(const char *oldval, const char *newval, int nohead = FALSE)
{
	vara list;
	vars added;
	static CSymList midlist;
	if (midlist.GetSize() == 0)
		midlist.Load(filename(TRANSBASIC"MID"));
	vara add(ProcessAKA(stripHTML(oldval + CString(";") + newval)).replace("' ", "'"), ";");
	for (int j = 0; j < add.length(); ++j)
		if (!add[j].Trim().IsEmpty())
		{
			vars &aka = add[j];
			if (InvalidUTF(aka))
				continue;
			if (strstr(aka, "*") && CDATA::GetNum(aka) >= 0)
				continue;
			if (isupper(aka[0]) && isupper(aka[1]))
				aka = Capitalize(aka);
			aka.Replace("\"", "\'");
			aka.Replace("{", "");
			aka.Replace("}", "");
			while (aka.Replace("\'\'", "\'"));
			vars aka1 = GetToken(aka, 0, "()").Trim();
			vars aka2 = GetToken(aka, 1, "()").Trim();
			if (strstri(added, aka1) && strstri(added, aka2))
				continue;
			added += "; " + aka;
			for (int i = 0; i < midlist.GetSize(); ++i)
				if (strstri(aka, midlist[i].id))
					added += "; " + aka.replace(midlist[i].id, " ");
			list.Add(aka);
		}
	if (nohead)
		list.RemoveAt(0);
	return list.join("; ");
}


vars skipItalics(const char *oldstr)
{
	const char *str = oldstr;
	const char *start[] = { "<i>", "&lt;i&gt;", NULL };
	const char *end[] = { "</i>", "&lt;/i&gt;", NULL };
	vars ret;
	int skip = FALSE;
	while (*str != 0) {
		int len = 0;
		if ((len = IsMatch(str, start)) > 0) {
			++skip;
			str += len;
			continue;
		}
		if ((len = IsMatch(str, end)) > 0) {
			--skip;
			str += len;
			continue;
		}
		if (!skip)
			ret += *str;
		++str;
	}
	return ret;
}


vars getfulltext(const char *line, const char *label)
{
	vars res = ExtractString(line, label);
	res.Replace(",", "%2C");
	return htmltrans(res);
}


vara getfulltextmulti(const char *line)
{
	vara list;
	vara values(line, "<value ");
	for (int i = 1; i < values.length(); ++i)
		list.push(getfulltext(values[i]));
	return list;
}


vars getfulltextorvalue(const char *line)
{
	vars res = getfulltextmulti(line).join(";");
	if (res.IsEmpty())
		res = getfulltext(line);
	if (res.IsEmpty())
		res = ExtractString(line, "<value>", "", "</value>");
	return res;
}


vars getlabel(const char *label)
{
	vars val = ExtractString(label, "", "<value>", "</value>");
	if (val.IsEmpty())
		val = ExtractString(label, "<value ", "\"", "\"");
	val.Trim(" \n\r\t\x13\x0A");
	vara vala(val = htmltrans(val));
	for (int v = 0; v < vala.length(); ++v)
		vala[v] = urlstr(vala[v], FALSE);
	return vala.join(";");
}


int rwfregion(const char *line, CSymList &regions)
{
	vara labels(line, "label=\"");
	vars id = getfulltext(labels[0]);
	//vars located = ExtractString(labels[1], "Located in region", "<value>", "</value>");
	if (id.IsEmpty()) {
		Log(LOGWARN, "Error empty ID from %.50s", line);
		return FALSE;
	}
	CSym sym(id);
	//ASSERT(!strstr(id, "Northern Ireland"));
	for (int i = 1; i < labels.length(); ++i)
		sym.SetStr(i - 1, getfulltextorvalue(labels[i]));

	Update(regions, sym, FALSE);
	return TRUE;
}


int ComparableTimes(double tmin, double tmax, double t)
{
	double range = 0.5; // 50%
	if (tmin < 0 || t < 0) // uncomparable
		return TRUE;

	if (t < tmin)
	{
		double d = fabs(tmin - t);
		if (d > 2 && d / tmin > range)
			return FALSE;
	}

	if (t > tmax)
	{
		double d = fabs(tmax - t);
		if (d > 2 && d / tmax > range)
			return FALSE;
	}

	return TRUE;
}


CString GetTime(double h)
{
	if (h <= 0)
		return "";
	//if (h<1)
	//	return MkString("%smin", CData(h*60));
	//if (h>24)
	//	return MkString("%s days", CData(h/24));
	return MkString("%sh", CData(h));
	//return MkString("%sh", CData(round(h*100)/100.0));
}


/*
int fixfloatingdot(vars &str)
{
	int ret = 0;
	int len = str.GetLength();
	for (int i=0; i<len-1; ++i)
		if (isdigit(str[i]) && str[i+1]=='.')
			if (i+4<len && isdigit(str[i+2]) && isdigit(str[i+3]) && isdigit(str[i+4]))
				{
				for (int j=i+4; j<len && isdigit(str[j]); ++j);
				str.Delete(i+4, j-i-4);
				len = str.GetLength();
				++ret;
				}
	return ret;
}
*/


double GetNumUnit(const char *str, unit *units)
{
	CDoubleArrayList list;
	if (GetValues(str, units, list))
		return list.Tail();
	return CDATA::GetNum(str);
}


class Links {
	BOOL warnkml;
	DownloadFile f;
	CSymList good, bad;

#define LINKSGOOD "linksgood"
#define LINKSBAD "linksbad"

public:
	Links()
	{
		warnkml = FALSE;
	}

	~Links()
	{
		if (good.GetSize() > 0)
		{
		}
		if (bad.GetSize() > 0)
		{
		}
		if (warnkml)
		{
			Log(LOGWARN, "***********KML FILES HAVE BEEN DOWNLOADED TO " KMLFIXEDFOLDER "!***********");
			Log(LOGWARN, "use -uploadbetakml to upload (mode -2 to forceoverwrite)");
		}
	}

	vars FilterLinks(const char *links, const char *existlinks)
	{

		vara oklinklist;
		vara linklist(links, " ");
		vara existlink(existlinks, ";");

		if (good.GetSize() == 0 && bad.GetSize() == 0)
		{
			good.Load(filename(LINKSGOOD));
			good.Sort();
			bad.Load(filename(LINKSBAD));
			bad.Sort();
		}

		printf("checking urls %dgood %dbad             \r", good.GetSize(), bad.GetSize());

		// new good links
		for (int l = 0; l < linklist.length(); ++l)
		{
			vars url = linklist[l].Trim();
			if (url.IsEmpty() || !IsSimilar(url, "http"))
				continue;

			// ignore some
			int ignore = FALSE;
			const char *ignorelist[] = { "//geocities.com", "ankanionla", "//canyoneering.net", "http://bit.ly", "http://caboverdenolimits.com",
					"//canyoning-reunion.com", "http://members.ozemail.com.au/~dnoble/canyonnames.html", "//murdeau-caraibe.org", ".brennen.",
					"googleusercontent.com/proxy/", "barrankos.blogspot.com",
					NULL };
			for (int g = 0; ignorelist[g] && !ignore; ++g)
				if (strstr(url, ignorelist[g]))
					ignore = TRUE;
			const char *code = GetCodeStr(url);
			if (ignore || code)
				continue;

			//if already listed, skip it
			if (existlink.indexOf(url) >= 0)
				continue;

			// check others
			int ok = INVESTIGATE == 0 || good.Find(url) >= 0;
			if (!ok)
			{
				if (INVESTIGATE < 1 && bad.Find(url) >= 0)
					continue;
				if (f.Download(url))
				{
					// BAD!
					bad.AddUnique(CSym(url));
					bad.Sort();
					bad.Save(filename(LINKSBAD));
					continue;
				}
				else
				{
					// GOOD!
					good.AddUnique(CSym(url));
					good.Sort();
					good.Save(filename(LINKSGOOD));
				}
			}
			oklinklist.push(url);
		}

		return oklinklist.join(" ");
	}


	int MapLink(const char *title, const char *link)
	{
		if (!strstr(link, "//maps.google") && !strstr(link, "google.com/maps") && !strstr(link, "//goo.gl/maps/"))
		{
			Log(LOGINFO, "Link %s", link);
			return FALSE;
		}

		if (strstr(link, "//goo.gl"))
			if (!f.Download(link))
				link = f.memory;

		vars msid = GetToken(ExtractString(link, "msid=", "", " "), 0, "& <>\"\'");
		vars mid = GetToken(ExtractString(link, "mid=", "", " "), 0, "& <>\"\'");

		vars url;
		if (!msid.IsEmpty())
			url = "http://maps.google.com/maps/ms?ie=UTF8&hl=en&msa=0&output=kml&forcekml=1&msid=" + msid;
		if (!mid.IsEmpty())
			url = "http://www.google.com/maps/d/u/0/kml?forcekml=1&mid=" + mid;

		if (url.IsEmpty())
		{
			Log(LOGERR, "Invalid mapid/msid in '%s'", link);
			return TRUE;
		}

		// check if KML already exist
		vars file = MkString("%s\\%s.kml", KMLFIXEDFOLDER, title);
		if (f.Download(url, file))
		{
			Log(LOGERR, "Could not download %s from %s", file, url);
			return TRUE;
		}

		// download KML and upload it to ropewiki
		warnkml = TRUE;
		Log(LOGINFO, "Download %s", file);

		return TRUE;
	}

} Links;


int TMODE = -1, ITEMLINKS = FALSE;


int CompareSym(CSym &sym, CSym &rwsym, CSym &chgsym, Code &translate)
{
	//ASSERT(!strstr(sym.data, "Chillar"));
	int update = 0;
	vara symdata(sym.data); symdata.SetSize(ITEM_BETAMAX);
	vara rwdata(rwsym.data); rwdata.SetSize(ITEM_BETAMAX);

	/*
	CSymList seasons;
	seasons.Add(CSym("Winter"));
	seasons.Add(CSym("Fall"));
	seasons.Add(CSym("Summer"));
	seasons.Add(CSym("Spring"));

	const char *ss[] = {"Winter", "Spring", "Summer", "Fall", NULL };
	*/

	// coordinates
	if (CDATA::GetNum(rwdata[ITEM_LAT]) == InvalidNUM || rwdata[ITEM_LAT].indexOf("@") >= 0)
	{
		if (CDATA::GetNum(symdata[ITEM_LAT]) != InvalidNUM) // new coordinates
			++update;
		if (CDATA::GetNum(symdata[ITEM_LNG]) != InvalidNUM) // new coordinates
			++update;
		if (symdata[ITEM_LAT].indexOf("@") >= 0 && rwdata[ITEM_LAT].indexOf("@") < 0) // new geolocation
		{
			// translation
			vara lat(symdata[ITEM_LAT], "@");
			lat[1] = translate.Region(lat[1], FALSE);
			symdata[ITEM_LAT] = lat.join("@");
			++update;
		}
	}

	// name match (for new canyons)		
	symdata[ITEM_ROCK] = translate.Rock(symdata[ITEM_ROCK]);
	symdata[ITEM_SEASON] = translate.Season(symdata[ITEM_SEASON]);
	symdata[ITEM_REGION] = translate.Region(symdata[ITEM_REGION]);
	symdata[ITEM_INFO] = Capitalize(translate.Name(symdata[ITEM_DESC])); //translate.Description(symdata[ITEM_DESC]);
	if (!strstr(symdata[ITEM_MATCH], RWID))
		symdata[ITEM_MATCH] = translate.Name(symdata[ITEM_DESC]);
	else
		symdata[ITEM_REGION] = regionmatch(symdata[ITEM_REGION], symdata[ITEM_MATCH]);

	if (translate.goodtitle)
	{
		BOOL skip = FALSE;
		vars title = stripAccents(sym.GetStr(ITEM_DESC));
		if (translate.goodtitle < 0) // capitalize if needed
			title = Capitalize(title);
		//ASSERT(!strstr(title, "parker"));
		for (int s = 0; andstr[s] != NULL && !skip; ++s)
			if (strstri(title, andstr[s]))
				skip = TRUE;
		const char *skipstr[] = { " loop",  " prospect", " area", " by ", " from ", " then ", " via ", NULL };
		for (int s = 0; skipstr[s] != NULL && !skip; ++s)
			if (strstri(title, skipstr[s]))
				skip = TRUE;
		if (!IsUTF8(title))
		{
			Log(LOGERR, "Invalid UTF8 Title for '%s' CSym:%s", sym.GetStr(ITEM_DESC), sym.Line());
			skip = TRUE;
		}
		if (!skip) // only first name (main name), to avoid region or other
			symdata[ITEM_AKA] = UpdateAKA(symdata[ITEM_AKA], GetToken(title, 0, ";,%[{"));
	}

	// compare rest of value
	for (int v = ITEM_REGION; v < ITEM_BETAMAX; ++v)
	{
		if (symdata[v].IsEmpty())
			continue;

		switch (v) {

			// display but then ignore
		case ITEM_STARS:
		case ITEM_CLASS:
		case ITEM_CONDDATE:
		case ITEM_INFO:
			continue;

		case ITEM_REGION: // translate and display but may be ignored
		case ITEM_LAT:
		case ITEM_LNG:
			//symdata[v] = ""; // display but then ignore
			if (strcmp(rwdata[v], "0") == 0 && strcmp(symdata[v], "0") != 0)
				rwdata[v] = "";
			if (rwdata[v].IsEmpty())
				if (!symdata[v].IsEmpty())
					++update;
			continue;

		case ITEM_LINKS:
			// check if links already in the match
			symdata[ITEM_LINKS] = ITEMLINKS ? Links.FilterLinks(symdata[ITEM_LINKS], rwdata[ITEM_MATCH]) : "";
			if (!symdata[ITEM_LINKS].IsEmpty())
				++update;

			continue;

		case ITEM_ACA:
		{
			// special for summary
			vara rwsum(rwdata[v], ";");
			vara symsum(symdata[v], ";");
			symsum.SetSize(R_SUMMARY);
			rwsum.SetSize(R_SUMMARY);
			int updatesum = 0;
			for (int r = 0; r < R_SUMMARY; ++r)
				if (!rwsum[r].IsEmpty())
					symsum[r] = "";
				else
					if (!symsum[r].IsEmpty())
						++updatesum;
			symdata[v] = updatesum > 0 ? symsum.join(";") : "";
			if (updatesum > 0)
				++update;
		}
		break;

		case ITEM_AKA:
		{
			vara addlist;
			vara symaka(symdata[ITEM_AKA], ";");
			for (int i = 0; i < symaka.length(); ++i) {
				vars name = translate.Name(symaka[i], FALSE);
				if (!name.IsEmpty())
				{
					name = vara(name, " - ").first();
					name = GetToken(name, 0, '/');
					name = GetToken(name, 0, '(');
				}

				//if (!strstr(rwdata[ITEM_AKA], name))
					//if (!strstr(rwdata[ITEM_DESC], name))
						//if (!strstr(symdata[ITEM_MATCH], name))
				addlist.Add(name);
			}
			vara oldaka(UpdateAKA(rwdata[ITEM_AKA], ""), ";");
			vara newaka(UpdateAKA(rwdata[ITEM_AKA], addlist.join(";")), ";");
			newaka.splice(0, oldaka.length());
			symdata[ITEM_AKA] = newaka.join(";").Trim(" ;");
			if (!symdata[ITEM_AKA].IsEmpty())
				++update;

			if (!IsUTF8(newaka.join(";")))
				Log(LOGERR, MkString("Not UTF8 compatible NEW AKA for %s [%s]", symdata[ITEM_DESC], sym.id));
			if (!IsUTF8(oldaka.join(";")))
				Log(LOGERR, MkString("Not UTF8 compatible OLD AKA for %s [%s]", rwdata[ITEM_DESC], rwsym.id));
		}
		break;

		case ITEM_LENGTH:
		case ITEM_DEPTH:
			// maximize depth/length for caves
		{
			unit *units = v == ITEM_LENGTH ? udist : ulen;
			double error = v == ITEM_LENGTH ? km2mi / 1000 : m2ft;
			if (CDATA::GetNum(rwdata[ITEM_CLASS]) == 2)
			{
				double rwv = GetNumUnit(rwdata[v], units);
				double symv = GetNumUnit(symdata[v], units);
				if (rwv + error < symv)
					rwdata[v] = "";
			}
			goto def;
		}
		break;

		case ITEM_MINTIME:
		case ITEM_AMINTIME:
		case ITEM_DMINTIME:
		case ITEM_EMINTIME:
			//ASSERT( !strstr(sym.data, "Gorgas Negras"));
			//if (!rwdata[ITEM_MINTIME].IsEmpty() && !rwdata[ITEM_MAXTIME].IsEmpty())
			//	symdata[ITEM_MINTIME] = symdata[ITEM_MAXTIME] = "";
			//else 
		{
			int _MINTIME = v, _MAXTIME = ++v; // process as a combo
			double tmin1 = GetHourDay(rwdata[_MINTIME]);
			double tmax1 = GetHourDay(rwdata[_MAXTIME]);
			double tmin2 = GetHourDay(symdata[_MINTIME]);
			double tmax2 = GetHourDay(symdata[_MAXTIME]);
			double tmin = tmin1, tmax = tmax1;
			if (tmax <= 0) tmax = tmin;
			if (tmin2 <= 0) tmin2 = tmin;
			if (tmax2 <= 0) tmax2 = tmin2;
			// integrity check							
			if (!ComparableTimes(tmin, tmax, tmin2) || !ComparableTimes(tmin, tmax, tmax2))
			{
				switch (TMODE)
				{
				case 0:
					// merge
					break;
				case -2:
					// overwrite
					tmin = tmin2;
					tmax = tmax2;
					break;
				default:
					Log(LOGWARN, "Ignoring out of range %s [%s-%s] VS [%s-%s] for %s [%s]", vara(headers)[v], CData(tmin), CData(tmax), CData(tmin2), CData(tmax2), rwsym.GetStr(ITEM_DESC), sym.GetStr(ITEM_DESC));
					continue;
					break;
				}
			}
			// compute new min-max
			if (tmin2 > 0)
				if (tmin <= 0 || tmin2 < tmin)
					tmin = tmin2;
			if (tmax2 > 0)
				if (tmax <= 0 || tmax2 > tmax)
					tmax = tmax2;
			if (tmin == tmin1 && tmax == tmax1) {
				symdata[_MINTIME] = symdata[_MAXTIME] = "";
			}
			else
			{
				symdata[_MINTIME] = GetTime(tmin);
				symdata[_MAXTIME] = GetTime(tmax);
				if (tmin == tmax || tmax <= 0) {
					symdata[_MAXTIME] = "";
					if (!rwdata[_MINTIME].IsEmpty()) {
						symdata[_MINTIME] = "";
						continue;
					}
				}
				++update;
			}
		}
		break;

		case ITEM_KML:
			if (!symdata[v].IsEmpty())
				//if (vara(rwdata[ITEM_MATCH],";").indexOf(KMLIDXLink(sym.id, symdata[v]))<0)
				if (!strstr(rwdata[ITEM_MATCH], KMLIDXLink(sym.id, symdata[v])))
				{
					rwdata[v] = "";
					++update;
					continue;
				}
			continue;

		case ITEM_SHUTTLE:
		{
			double rwlen = CDATA::GetNum(rwdata[v]);
			double symlen = CDATA::GetNum(symdata[v]);
			if (IsSimilar(symdata[v], "No"))
				symlen = 0;
			if (IsSimilar(symdata[v], "Yes"))
				symlen = 1;
			if (IsSimilar(symdata[v], "Opt"))
				symlen = 0.5;
			if (symlen < 0)
			{
				Log(LOGWARN, "Skipping invalid shuttle length '%s' for %s", symdata[v], rwsym.GetStr(ITEM_DESC));
				symdata[v] = "";
				continue;
			}
			vars sep = "!!!";
			if (rwlen >= 0) // pre-existing set
			{
				if (rwlen == 0 && symlen >= 1)
				{
					rwdata[v] = "";
					if (symlen > 1)
						symdata[v] = "Optional;" + symdata[v];
					else
						symdata[v] = "Optional;" + sep + symdata[v];
					++update;
				}
				else if (rwlen >= 1 && symlen == 0)
				{
					rwdata[v] = "";
					symdata[v] = "Optional;" + sep + symdata[v];
					++update;
				}
				else if (rwlen >= 1 && rwlen < symlen)
				{
					rwdata[v] = "";
					symdata[v] = "Required;" + symdata[v];
					++update;
				}
				else
				{
					// ==
					symdata[v] = "";
				}
			}
			else
			{
				// !symdata[v].IsEmpty()
				if (symlen >= 1) // > 1 to be required
				{
					symdata[v] = "Required;" + symdata[v];
					++update;
				}
				else if (symlen >= 0.5) // > 1 to be required
				{
					if (symlen > 1)
						symdata[v] = "Optional;" + symdata[v].replace("Optional", "");
					else
						symdata[v] = "Optional;" + sep + symdata[v];
					++update;
				}
				else
				{
					symdata[v] = "None;" + sep + symdata[v];
					++update;
				}
			}
			break;
		}

		case ITEM_PERMIT:
			if (IsSimilar(symdata[v], "Yes"))
				symdata[v] = "Yes";
			goto def;
		case ITEM_VEHICLE:
			symdata[v] = GetToken(symdata[v], 0, ':');
			goto def;
		default:
		def:
			if (strcmp(rwdata[v], "0") == 0 && strcmp(symdata[v], "0") != 0)
				rwdata[v] = "";
			if (!rwdata[v].IsEmpty())
				symdata[v] = "";
			else
				if (!symdata[v].IsEmpty())
					++update;
			break;
		}
	}

	// update Beta Site section
	if (!symdata[ITEM_NEWMATCH].IsEmpty())
		++update;

	/*
	int fixed = 0;
	for (int v=ITEM_ACA; v<ITEM_BETAMAX; ++v)
		if (fixfloatingdot(symdata[v]))
			++fixed;
	*/

	chgsym = CSym(sym.id, symdata.join().TrimRight(","));

	// set up matchlist
	CString matchlist = sym.GetStr(ITEM_MATCHLIST);
	if (!matchlist.IsEmpty())
	{
		const char *rsep = "\n,,,,";
		vara list(matchlist, MATCHLISTSEP);
		if (list.length() > 1)
			chgsym.SetStr(ITEM_MATCHLIST, rsep + list.join(rsep));
	}
	return update;
}


void BSLinkInvalid(const char *title, const char *link)
{
	//if (!codelist[c].IsCandition())
	const char *filename = "invalid.csv";
	CSymList invalids;
	vars url = urlstr(link);
	invalids.Load(filename);
	if (invalids.Find(url) < 0)
	{
		invalids.Add(CSym(url, title));
		invalids.Save(filename);
	}
	Log(LOGWARN, "INVALID BSLINK %s for %s (added to %s, run -mode=-2 -fixbeta to delete)", url, title, filename);
}


int UpdateChanges(CSymList &olist, CSymList &nlist, Code &cod)
{
	CSymList chglist;
	static CSymList allchglist;
	static CSymList ignorelist;
	if (!ignorelist.GetSize())
		ignorelist.Load(filename("ignore"));


	// match again
	if (MODE > 0)
		MatchList(nlist, cod);

	// find bad links
	for (int i = 0; i < _bslist.GetSize(); ++i)
	{
		CString &title = _bslist[i].GetStr(ITEM_DESC);
		CString &link = _bslist[i].id;
		int c = GetCode(link);
		if (c > 0 && codelist[c] == &cod)
			if (nlist.Find(link) < 0)
				BSLinkInvalid(title, link);
	}


	// find changes
	CSym usym;
	for (int i = 0; i < nlist.GetSize(); ++i)
		if (Update(olist, nlist[i], &usym, FALSE)) {
			if (ignorelist.Find(nlist[i].id) >= 0)
				continue;

			// cond_ing RWTITLE:
			if (IsSimilar(usym.id, RWTITLE))
				continue;

			// changes 
			vars id = urlstr(usym.GetStr(ITEM_MATCH));
			vars match = usym.GetStr(ITEM_NEWMATCH);

			double c = usym.GetNum(ITEM_CLASS);
			if (c != InvalidNUM) {
				if (c < 1) {
					BOOL skip = TRUE;
					// skip anything not canyoneering except for perfect matches that are not duplicate links
					if (strstr(id, RWID)) {
						if (strstr(id, RWLINK))
							skip = FALSE; // don't skip if already linked
						if (strchr(match, '!')) // || strchr(match,'~'))
							if (nlist.FindColumn(ITEM_MATCH, id + RWLINK) < 0)
								skip = FALSE;
					}
					if (skip)
						continue;
				}
			}

			CSym rwsym, chgsym;
			// if completely new just add
			if (!IsSimilar(id, RWID))
			{
				if (CompareSym(usym, rwsym, chgsym, cod))
					chglist.Add(chgsym);
			}
			else
			{
				// if matched, iterate for all matches
				vara rwids(id, RWID);
				for (int r = 1; r < rwids.length(); ++r)
				{
					vars rid = RWID + CData(CDATA::GetNum(rwids[r]));
					usym.SetStr(ITEM_MATCH, rid + RWLINK);
					int f = rwlist.Find(rid);
					if (f < 0) {
						Log(LOGERR, "Mismatched MATCH %s for %s", id, usym.Line());
						continue;
					}
					// MATCHED!
					rwsym = rwlist[f];
					if (CompareSym(usym, rwsym, chgsym, cod))
					{
						if (!cod.betac->ubase && strstr(usym.GetStr(ITEM_MATCH), RWLINK))
							continue;
						chglist.Add(chgsym);
					}
				}
			}

			//ASSERT(usym.id!="http://descente-canyon.com/canyoning/canyon-description/21414");
			//ASSERT(rwsym.id!="http://descente-canyon.com/canyoning/canyon-description/21414");
		}

	// check duplicates
	if (!cod.IsRW())
	{
		CSymList namelist, dupnamelist, rnamelist;
		for (int i = 0; i < rwlist.GetSize(); ++i) {
			CString name = GetToken(rwlist[i].GetStr(ITEM_DESC), 0, "[{(").Trim();
			rnamelist.Add(CSym(name));
		}
		for (int i = 0; i < chglist.GetSize(); ++i) {
			if (chglist[i].id.IsEmpty())
				continue;
			CString name = chglist[i].GetStr(ITEM_MATCH);
			if (IsSimilar(name, RWID))
				continue;
			namelist.Add(CSym(name));
		}
		namelist.Sort(); rnamelist.Sort();
		for (int i = 1; i < namelist.GetSize(); ++i)
			if (namelist[i - 1].id == namelist[i].id)
				dupnamelist.Add(namelist[i]);

		for (int i = 0; i < chglist.GetSize(); ++i) {
			if (chglist[i].id.IsEmpty())
				continue;
			CString name = chglist[i].GetStr(ITEM_MATCH);
			if (name.IsEmpty())
				continue;
			if (IsSimilar(name, RWID))
				continue;
			CString dup = "";
			if (dupnamelist.Find(name) >= 0)
				dup += "N";
			if (rnamelist.Find(name) >= 0)
				dup += "O";
			if (!dup.IsEmpty())
				chglist[i].SetStr(ITEM_NEWMATCH, dup + chglist[i].GetStr(ITEM_NEWMATCH));
		}
	}

	// accumulate prioretizing by match

	CSymList chglists[5][2];
	for (int i = 0; i < chglist.GetSize(); ++i)
	{
		CSym sym = chglist[i];
		CString match = sym.GetStr(ITEM_NEWMATCH);
		const char *num = match;
		while (*num != 0 && !isdigit(*num))
			++num;
		sym.SetNum(ITEM_BETAMAX, CGetNum(num));
		BOOL km = strstr(match, "km ") != NULL;
		if (match.IsEmpty())
			chglists[0][km].Add(sym);
		else if (strstr(match, "!"))
			chglists[1][km].Add(sym);
		else if (strstr(match, "~"))
			chglists[2][km].Add(sym);
		else if (strstr(match, "?"))
			chglists[3][km].Add(sym);
		else
			chglists[4][km].Add(sym);
	}

	for (int i = 0; i < sizeof(chglists) / sizeof(chglists[0]); ++i)
	{
		chglists[i][0].SortNum(ITEM_BETAMAX, -1);
		chglists[i][1].SortNum(ITEM_BETAMAX, 1);

		if (cod.IsWIKILOC() && i == 1)
		{
			int km = 1;
			vara rwids;
			for (int j = 0; j < chglists[i][km].GetSize(); ++j)
			{
				vars match = chglists[i][km][j].GetStr(ITEM_NEWMATCH);
				vars rwid = RWID + ExtractString(match, RWID, "", ":");
				// check saturated match
				int f = rwlist.Find(rwid);
				if (f >= 0)
				{
					vara wikilinks(rwlist[f].GetStr(ITEM_MATCH), "wikiloc.com");
					if (wikilinks.length() > MAXWIKILOCLINKS)
					{
						chglists[i][km].Delete(j--);
						continue;
					}
				}
				// check already matched
				if (rwids.indexOf(rwid) >= 0)
				{
					chglists[i][km].Delete(j--);
					continue;
				}
				rwids.push(rwid);
			}
		}


		allchglist.Add(chglists[i][1]);
		allchglist.Add(chglists[i][0]);
	}

	allchglist.header = headers;
	allchglist.header.Replace("ITEM_", "");
	allchglist.Save(filename(CHGFILE));
	return chglist.GetSize();
}


CString findsym(CSymList &list, const char *str)
{
	int f = list.Find(str);
	if (f > 0)
		return list[f].Line();
	else
		return MkString("ERROR: Could not find id '%s'", str);
}


int CheckBeta(int mode, const char *codeid)
{
	rwlist.Load(filename(codelist[0]->code));
	rwlist.Sort();

	int run = 0;
	for (int i = 1; codelist[i]->code != NULL; ++i) {
		if (codeid && *codeid)
		{
			if (stricmp(codeid, codelist[i]->code) != 0)
				continue;
		}
		CSymList symlist;
		symlist.Load(filename(codelist[i]->code));
		symlist.Sort();

		/*
		// PATCH Length/Depth ==================
		for (int l=0; l<symlist.GetSize(); ++l)
			{
			CSym &sym = symlist[l];
			for (int j=ITEM_AGAIN; j<ITEM_MATCHLIST; ++j)
				sym.SetStr(j, "");
			}
		symlist.header = headers;
		symlist.Save(filename(codelist[i].code));
		continue;
		//=====================================
		*/

		// match again
		MatchList(symlist, *codelist[i]);

		// find duplicates
		CSymList dup;
		for (int l = 0; l < symlist.GetSize(); ++l) {
			CString id = symlist[l].GetStr(ITEM_MATCH);
			if (!strstr(id, RWLINK))
				continue;
			dup.Add(CSym(id, symlist[l].id));
		}
		dup.Sort();
		for (int l = 1; l < dup.GetSize(); ++l)
			if (dup[l - 1].id == dup[l].id)
				Log(LOGINFO, "Duplicate link %s\n%s\n%s\n%s", dup[l].id,
					findsym(rwlist, vars(dup[l].id).replace(RWLINK, "")),
					findsym(symlist, dup[l - 1].data),
					findsym(symlist, dup[l].data));

		DownloadFile f;
		if (MODE > 0)
			for (int l = 0; l < symlist.GetSize(); ++l)
			{
				CSym &sym = symlist[l];
				if (IsSimilar(sym.id, "http"))
					if (f.Download(sym.id))
						Log(LOGERR, "Could not download %s : %s", sym.GetStr(ITEM_DESC), sym.id);
			}
	}
	return TRUE;
}


int cmpdata(const void *arg1, const void *arg2)
{
	return strcmp((*(CSym**)arg1)->data, (*(CSym**)arg2)->data);
}


int cmpclass(const void *arg1, const void *arg2)
{
	CString c1 = (*(CSym**)arg1)->GetStr(ITEM_CLASS);
	CString c2 = (*(CSym**)arg2)->GetStr(ITEM_CLASS);
	return strcmp(c2, c1);
}


int cmpinvid(const void *arg1, const void *arg2)
{
	CString c1 = (*(CSym**)arg1)->id;
	CString c2 = (*(CSym**)arg2)->id;
	int cmp = strcmp(c2, c1);
	if (!cmp)
		return strcmp((*(CSym**)arg1)->data, (*(CSym**)arg2)->data);
	return cmp;
}


int DownloadBeta(int mode, const char *codeid)
{
	if (codeid && *codeid == 0)
		codeid = NULL;
	CString symheaders = headers;
	symheaders.Replace("ITEM_", "");
	//chglist.Load(filename(CHGFILE));
	DeleteFile(filename(MATCHFILE));

	int run = 0, cnt = 0;
	for (int order = codeid ? -1 : 0; order < 10; ++order)
		for (int i = 0; codelist[i]->code != NULL; ++i) {
			if (order != codelist[i]->order)
				continue;
			if (codeid && stricmp(codeid, codelist[i]->code) != 0)
				continue;
			if (!codeid && !codelist[i]->betac->ubase)
				continue;
			if (mode == 1 && i == 0)
				continue;

			++run;
			Log(LOGINFO, "Running Beta %s MODE %d", codelist[i]->code, mode);

			// load code list
			CSymList symlist;
			symlist.header = symheaders;
			symlist.Load(filename(codelist[i]->code));
			//symlist.Sort(strcmp(codelist[i].code,WIK)==0 ? cmpclass : cmpdata);
			//symlist.Sort();

			// run process
			int ret = -1;
			int counter = 0;
			CSymList oldlist;
			if (MODE < 1)
			{
				oldlist = symlist;
				ret = codelist[i]->betac->DownloadBeta(symlist);
				if (i > 0)
				{
					for (int s = 0; s < symlist.GetSize(); ++s)
						if (symlist[s].index != 0)
							++counter;
				}
			}

			int chg = UpdateChanges(oldlist, symlist, *codelist[i]);
			Log(LOGINFO, "CHG.CSV : [%dD/%dT] %s => %d changes/additions from #%d %s [%s]", counter, symlist.GetSize(), ret ? "OK" : "ERROR!!!", chg, ++cnt, codelist[i]->code, codelist[i]->betac->ubase);
			if (i > 0 && mode < 0 && counter < symlist.GetSize() / 2)
				Log(LOGERR, "ERROR: Download malfunction for code %s [%s] (D<T/2)", codelist[i]->code, codelist[i]->betac->obase);

			// check duplicates
			symlist.Sort();
			for (int j = symlist.GetSize() - 1; j > 0; --j)
				if (symlist[j - 1].id == symlist[j].id)
				{
					Log(LOGERR, "Deleting duplicated id %s", symlist[j].id);
					symlist.Delete(j);
				}

			// save
			symlist.Sort(codelist[i]->IsWIKILOC() ? cmpclass : cmpdata);
			symlist.Save(filename(codelist[i]->code));
			//if (!codelist[i].base) // rw
			//	MoveFileEx(filename(CHGFILE), filename(CString(CHGFILE)+codelist[i].code), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH );
		}

	if (!run) {
		vara all;
		for (int i = 0; codelist[i]->code != NULL; ++i)
			all.push(codelist[i]->code);
		Log(LOGERR, "Wrong code %s [%s]", codeid, all.join(" "));
		return FALSE;
	}

	return TRUE;
}


CString GetProp(const char *name, vara &lines)
{
	if (lines.length() == 0)
	{
		Log(LOGERR, "UpdateProp of empty page");
		return "";
	}
	int i;
	for (i = 0; i < lines.length() && strncmp(lines[i], "{{", 2) != 0; ++i);

	if (i == lines.length())
	{
		Log(LOGERR, "UpdateProp of NOT {{property}} page (%s)", lines[0]);
		return "";
	}
	CString pname = "|" + CString(name) + "=";
	for (; i < lines.length() && strncmp(lines[i], "}}", 2) != 0; ++i)
		if (IsSimilar(lines[i], pname))
			return GetTokenRest(lines[i], 1, '=');
	return "";
}


int UpdateProp(const char *name, const char *value, vara &lines, int force)
{
	BOOL isnull = stricmp(value, "NULL") == 0;

	if (strchr(name, ';'))
	{
		// multi-properties
		vara names(name, ";");
		vara values(GetToken(value, 0, '!'), ";");
		values.SetSize(names.length());
		int updates = 0;
		for (int i = 0; i < names.length(); ++i)
			if (!values[i].IsEmpty())
				updates += UpdateProp(names[i], values[i], lines, force);
		return updates;
	}

	if (lines.length() < 1)
	{
		Log(LOGERR, "UpdateProp of empty page %s", name);
		return FALSE;
	}

	if (strncmp(lines[0], "{{", 2) != 0 && strncmp(lines[1], "{{", 2) != 0) {
		Log(LOGERR, "UpdateProp could not find {{");
		return FALSE;
	}

	if (value == NULL || *value == 0) {
		Log(LOGERR, "Invalid UpdateProp(%s,%s)", name, value);
		return FALSE;
	}
	if (*name == '=') {
		//Log(LOGWARN, "Invalid UpdateProp(%s,%s)", name, value);
		return FALSE;
	}

	CString pname = "|" + CString(name) + "=", pnameval = pname + value;
	if (strcmp(name, "Location type") == 0)
	{
		static vara t(",,Caving,Via Ferrata,,POI");
		int fc = t.indexOf(value);
		double c = CDATA::GetNum(value);
		if (fc > 0) c = fc;
		if (c == InvalidNUM || c < 0) c = 0;
		if (c < 2 && !isnull) return FALSE;
		pnameval = pname + t[(int)c];
	}
	if (strcmp(name, "Best season") == 0)
	{
		vars val = GetToken(value, 0, "!");
		IsSeasonValid(val, &val);
		if (val.IsEmpty())
		{
			Log(LOGERR, "Invalid season '%s' (was not posted)", value);
			return FALSE;
		}
		pnameval = pname + val;
	}
	if (strcmp(name, "AKA") == 0)
		pnameval = pname + UpdateAKA("", value);
	int i;
	for (i = 0; i < lines.length() && strncmp(lines[i], "}}", 2) != 0; ++i)
		if (IsSimilar(lines[i], pname)) {
			if (strcmp(lines[i], pnameval) == 0)
				return FALSE;
			CString val = lines[i].Mid(pname.GetLength());
			if (strcmp(val, "0") == 0 && strcmp(value, "0") != 0)
				val = "";
			if (!val.IsEmpty()) {
				// SPECIAL CASE FOR VALUE OVERRIDE
				if (!force && MODE > -2) // if not overriding values
				{

					if (strcmp(name, "AKA") == 0) // Add AKA to existing list
						pnameval = pname + UpdateAKA(val, value);
					else if (strstr(name, "Shuttle")) // silently override shuttle
						pnameval = pnameval;
					else if (strstr(name, " time")) // silently override time
					{
						pnameval = pnameval;
						double ov = CDATA::GetNum(val), nv = CDATA::GetNum(value);
						if (ov > 0 && nv > 0)
						{
							if (strstr(name, "Slowest") != NULL)
								if (ov > nv)
									pnameval = pname + val;
								else
									pnameval = pnameval;
							if (strstr(name, "Fastest") != NULL)
								if (ov < nv)
									pnameval = pname + val;
								else
									pnameval = pnameval;
						}
					}
					else
					{
						// warn that property is not empty
						Log(LOGERR, "ERROR: Property not empty, skipping %s <= %s", lines[i], pnameval);
						return FALSE;
					}
				}
			}
			// change property
			if (isnull)
			{
				lines.RemoveAt(i);
				return TRUE; // deleted
			}
			if (lines[i] == pnameval)
				return FALSE; // same
			lines[i] = pnameval;
			return TRUE; // new val
		}
	// new property
	if (strncmp(lines[i], "}}", 2) == 0) {
		if (isnull)
			return FALSE; //deleted
		// update
		lines.InsertAt(i, pnameval);
		return TRUE;
	}
	Log(LOGERR, "UpdateProp could not find }}");
	return FALSE;
}


int IsBlank(const char *str)
{
	for (; *str != 0; ++str)
		if (!isspace(*str))
			return FALSE;
	return TRUE;
}


int FindSection(vara &lines, const char *section, int *iappend)
{
	vars sec = section;
	sec.MakeLower();

	for (int i = 0; i < lines.length(); ++i)
		if (strncmp(lines[i], "==", 2) == 0)
			if (strstr(lines[i].lower(), sec))
			{
				int start = ++i, append = -1;
				for (; i < lines.length() && strncmp(lines[i], "==", 2) != 0; ++i)
					if (!lines[i].trim().IsEmpty())
						append = i + 1;
				if (iappend)
					*iappend = append;
				return start;
			}

	Log(LOGERR, "Could not find section '%s'", section);
	return -1;
}


int ReplaceLinks(vara &lines, int code, CSym &sym)
{
	vars newline;
	CSymList wiklines;
	Code &cod = *codelist[code];

	if (code > 0)
	{
		vars zpre, zpost;
		newline = cod.BetaLink(sym, zpre, zpost);
		if (cod.IsWIKILOC())
		{
			int found = cod.FindLink(newline);
			if (found < 0)
			{
				Log(LOGWARN, "Can't find invalid WIKILOC link for %s: %s", sym.id, newline);
			}
			else
			{
				wiklines.Add(CSym(cod.list[found].GetStr(ITEM_CLASS), cod.BetaLink(cod.list[found], zpre, zpost)));
				newline = "";
			}
		}
	}

	vara oldlines = lines;
	int bslastnoblank = -1, trlastnoblank = -1, firstdel = -1;
	for (int i = 0; i < lines.length(); ++i) {
		// Update Beta sites
		if (strncmp(lines[i], "==", 2) == 0)
		{
			int bs = strstr(lines[i], "Beta") != NULL;
			int tr = strstr(lines[i], "Trip") != NULL;
			if (bs || tr)
			{
				int lastnoblank = ++i;
				for (; i < lines.length() && strncmp(lines[i], "==", 2) != 0; ++i)
				{
					if (lines[i][0] == '*') // last bullet
						lastnoblank = i + 1;
					const char *httplink = strstr(lines[i], "http");
					if (!httplink)
						continue;
					int httpcode = GetCode(httplink);
					if (httpcode <= 0)
						continue;

					Code &cod = *codelist[httpcode];
					if (cod.IsWIKILOC())
					{
						// check if bulleted list
						vars pre, post;
						if (cod.IsBulletLine(lines[i], pre, post) <= 0)
						{
							Log(LOGERR, "WARNING: found non-bullet WIKILOC link at %s : %.100s", sym.id, lines[i]);
							continue;
						}
						int found = cod.FindLink(httplink);
						if (found < 0)
						{
							Log(LOGWARN, "Can't find invalid WIKILOC link for %s: %s", sym.id, lines[i]);
							continue;
						}
						if (cod.list[found].id != sym.id)
							wiklines.Add(CSym(cod.list[found].GetStr(ITEM_CLASS), cod.BetaLink(cod.list[found], pre, post)));
						lines.RemoveAt(lastnoblank = i--);
						continue;
					}

					if (httpcode == code)
					{
						// check if bulleted list
						vars pre, post;
						if (cod.IsBulletLine(lines[i], pre, post) <= 0)
						{
							Log(LOGERR, "WARNING: found non-bullet link at %s : %.100s", sym.id, lines[i]);
							newline = "";
							continue;
						}
						int found = cod.FindLink(httplink);
						if (found < 0 || cod.list[found].id == sym.id)
						{
							newline = cod.BetaLink(sym, pre, post);
							if (found < 0) Log(LOGWARN, "Replacing bad beta site link for %s: %s => %s", sym.id, lines[i], newline);
							lines.RemoveAt(lastnoblank = firstdel = i--);
						}
					}
				}
				if (bs) bslastnoblank = lastnoblank;
				if (tr) trlastnoblank = lastnoblank;
				--i;
			}
		}
	}
	// insert newline				

	int lastnoblank = bslastnoblank;
	if (cod.IsTripReport())
		lastnoblank = trlastnoblank;
	if (firstdel > lastnoblank || firstdel < 0)
		firstdel = lastnoblank;
	if (!newline.IsEmpty() && firstdel > 0)
		lines.InsertAt(firstdel, newline), ++lastnoblank;

	{
		// insert wikilines at end of bslist or trlist
		int end, start = FindSection(lines, "Trip", &end);
		int lastnoblank = end > 0 ? end : start;
		if (lastnoblank < 0)
		{
			Log(LOGERR, "MAJOR ERROR: Major wik error, lastnoblank = -1");
			lastnoblank = lines.length();
		}

		wiklines.Sort(cmpinvid);
		for (int w = 0; w < wiklines.GetSize(); ++w)
			lines.InsertAt(lastnoblank + w, wiklines[w].data);
	}

	// compare old vs new
	if (oldlines.length() != lines.length())
		return TRUE;
	for (int i = 0; i < lines.length(); ++i)
		if (oldlines[i] != lines[i])
			return TRUE;
	return FALSE;
}


int AddLinks(const char *title, vara &lines, vara&links)
{
	// process kmls
	vars mid;
	for (int l = 0; l < links.length(); ++l)
		if (Links.MapLink(title, links[l]))
			links.RemoveAt(l--);

	vars alllines = lines.join("");
	// find last non blank after Beta and before Trips
	int lastnonblank = -1;
	int i;
	for (i = 0; i < lines.length(); ++i)
	{
		if (strncmp(lines[i], "==", 2) == 0)
		{
			if (strstr(lines[i], "Trip"))
				break;
			if (strstr(lines[i], "Beta"))
				lastnonblank = -1;
			continue;
		}
		if (!lines[i].trim().IsEmpty())
			lastnonblank = i;
	}
	if (lastnonblank < 0)
		lastnonblank = i - 1;

	// process links links
	int n = 0;
	for (int l = 0; l < links.length(); ++l)
		if (!strstr(alllines, links[l]))
		{
			lines.InsertAt(++lastnonblank, "* " + links[l]);
			++n;
		}

	return n;
}


int UpdatePage(CSym &sym, int code, vara &lines, vara &comment, const char *title)
{
	int updates = 0;
	vara symdata(sym.data);
	symdata.SetSize(ITEM_BETAMAX);

	static vara rwformid(rwform, "|");
	static vara rwformacaid(rwformaca, ";");


	// modify page
	if (!IsSimilar(lines[0], "{{Canyon") && !IsSimilar(lines[1], "{{Canyon")) {
		Log(LOGERR, "Page already exists and not a {{Canyon}} for %s", sym.Line());
		return FALSE;
	}

	// update Coordinates (if needed)
	vara geoloc(symdata[ITEM_LAT], "@");
	if (geoloc.length() > 1)
	{
		if (MODE <= -2 || (GetProp("Geolocation", lines).IsEmpty() && GetProp("Coordinates", lines).IsEmpty()))
		{
			geoloc[1].Replace(";Overseas;France", "");
			geoloc[1].Replace(";Center;", ";");
			geoloc[1].Replace(";North;", ";");
			geoloc[1].Replace(";South;", ";");
			geoloc[1].Replace(";West;", ";");
			geoloc[1].Replace(";East;", ";");
			geoloc[1].Replace(";", ",");
			double lat, lng;
			if (!_GeoCache.Get(geoloc[1], lat, lng))
			{
				Log(LOGERR, "Invalid Geolocation for %s [%s]", sym.id, geoloc[1]);
				return FALSE;
			}
			if (UpdateProp("Geolocation", geoloc[1], lines))
				++updates, comment.push("Geolocation");
		}
	}
	else if (!symdata[ITEM_LAT].IsEmpty() && !symdata[ITEM_LNG].IsEmpty()) {
		if (MODE <= -2 || GetProp("Coordinates", lines).IsEmpty())
		{
			double lat = CDATA::GetNum(symdata[ITEM_LAT]), lng = CDATA::GetNum(symdata[ITEM_LAT]);
			if (!CheckLL(lat, lng, sym.id))
				Log(LOGERR, "Skipping invalid coordinates for %s %s: %s,%s", sym.id, symdata[0], symdata[ITEM_LAT], symdata[ITEM_LNG]);
			else
				if (UpdateProp("Coordinates", symdata[ITEM_LAT] + "," + symdata[ITEM_LNG], lines))
					++updates, comment.push("Coordinates");
		}
	}


	// update properties				
	for (int p = ITEM_REGION; p < symdata.length() && p < ITEM_LINKS; ++p) //p<rwformid.length(); ++p)
	{
		if (symdata[p].IsEmpty()) // skip empty field
			continue;
		// update region
		if (p == ITEM_REGION)
			symdata[p] = vara(symdata[p], ";").last();
		// update SUMMARY 
		if (p == ITEM_ACA)
			if (!IsSimilar(symdata[ITEM_ACA], RWID))
			{
				vara symsummary(symdata[ITEM_ACA], ";");
				for (int p = 0; p < symsummary.length() && p < R_SUMMARY; ++p)
					if (!symsummary[p].IsEmpty())
						if (UpdateProp(rwformacaid[p], symsummary[p], lines))
							++updates, comment.push(rwformacaid[p]);
			}
		if (p >= ITEM_RAPS && p <= ITEM_MAXTIME && CDATA::GetNum(symdata[p]) == InvalidNUM) // skip non numerical for numeric params
			continue;
		if (UpdateProp(rwformid[p], symdata[p], lines))
			++updates, comment.push(rwformid[p]);
	}

	//  process beta sites

	//if (!strstr(symdata[ITEM_MATCH], RWLINK))
	if (IsSimilar(sym.id, "http"))
		if (ReplaceLinks(lines, code, sym))
		{
			symdata[ITEM_MATCH] = RWLINK; // done
			++updates, comment.push("Beta sites");
		}

	// ITEM_LINKS stuff
	vara links(symdata[ITEM_LINKS], " ");
	if (links.length() > 0)
		if (AddLinks(title, lines, links))
			++updates, comment.push("Beta links");

	return updates;
}


void  PurgePage(DownloadFile &f, const char *id, const char *title)
{
	printf("PURGING #%s %s ...           \r", id ? id : "X", title);
	CString url = "http://ropewiki.com/api.php?action=purge";
	if (id && id[0] != 0)
	{
		if (CDATA::GetNum(id) != InvalidNUM)
			url += CString("&pageids=") + id;
		else
			url += CString("&titles=") + id;
	}
	else
		url += CString("&titles=") + title;
	url += "&forcelinkupdate";
	if (f.Download(url))
		Log(LOGERR, "ERROR: can't purge %.128s", url);
}


void  RefreshPage(DownloadFile &f, const char *id, const char *title)
{
	printf("REFRESHING #%s %s ...           \r", id ? id : "X", title);
	CString url = "http://ropewiki.com/api.php?action=sfautoedit&form=AutoRefresh&target=Votes:AutoRefresh&query=AutoRefresh[Location]=";
	if (id && id[0] != 0)
	{
		if (CDATA::GetNum(id) != InvalidNUM)
			url += title;
		else
			url += id;
	}
	if (f.Download(url))
		Log(LOGERR, "ERROR: can't refresh %.128s", url);
}


vara loginstr(RW_ROBOT_LOGIN);


int Login(DownloadFile &f)
{
	//static DWORD lasttickcnt = 0;
	//if (GetTickCount()-lasttickcnt<10*60*1000)
	//	return TRUE;
	//lasttickcnt = GetTickCount();

	extern int URLTIMEOUT; // change default timeout
	URLTIMEOUT = 60;

	int ret = 0;
	ret += f.Download(RWBASE + "index.php?title=Special:UserLogin");
	f.GetForm("name=\"userlogin\"");
	f.SetFormValue("wpName", loginstr[0]);
	f.SetFormValue("wpPassword", loginstr[1]);
	f.SetFormValue("wpRemember", "1");
	ret += f.SubmitForm();

	/*
	CString lgtoken = f.GetFormValue("wpLoginToken");
	vars login = "wpName="+loginstr[0]+"&wpPassword="+loginstr[1];
	ret += f.Download(RWBASE + "index.php?title=Special:UserLogin&action=submitlogin&type=login&returnto=Main+Page?POST?"+login+"&wpRemember=1&wpLoginAttempt=Log+in&wpLoginToken="+lgtoken, "login.htm");
	*/
	if (ret)
	{
		Log(LOGERR, "ERROR: can't login");
		return FALSE;
	}

	return TRUE;
}


int rwftitle(const char *line, CSymList &idlist)
{
	vara labels(line, "label=\"");
	//vars name = getfulltext(labels[0]);
	vars name = getfulltext(line);
	CSym sym(name);
	idlist.Add(sym);
	return TRUE;
}


int rwftitleo(const char *line, CSymList &idlist)
{
	vara labels(line, "label=\"");
	//vars name = getfulltext(labels[0]);
	vars name = htmltrans(ExtractString(line, "fulltext="));
	CSym sym(name);
	for (int i = 1; i < labels.length(); ++i)
		sym.SetStr(i - 1, getfulltextorvalue(labels[i]));
	idlist.Add(sym);
	return TRUE;
}


int rwflabels(const char *line, CSymList &idlist)
{
	vara labels(line, "label=\"");
	//vars name = getfulltext(labels[0]);
	vars name = htmltrans(ExtractString(line, "fulltext="));
	CSym sym(name.replace(",", "%2C"));
	for (int i = 1; i < labels.length(); ++i)
		sym.SetStr(i - 1, getlabel(labels[i]));
	idlist.Add(sym);
	return TRUE;
}


BOOL CheckPage(const char *page)
{
#if 0
	CSymList list;
	GetRWList(list, MkString("[[%s]]", url_encode(page)), rwftitle);
	return list.GetSize();
#else
	DownloadFile f;
	CPage p(f, NULL, page);
	return p.Exists();
#endif
}


int CreateRegions(DownloadFile &f, CSymList &regionslist)
{
	if (regionslist.GetSize() == 0)
		return FALSE;

	Log(LOGINFO, "%d regions to be created", regionslist.GetSize());
	for (int i = 0; i < regionslist.GetSize(); ++i)
		Log(LOGINFO, "#%d: %s (parent=%s)", i, regionslist[i].id, regionslist[i].data);
	for (int i = 5; i > 0; --i)
	{
		printf("Starting in %ds...   \r", i);
		Sleep(1000);
	}

	for (int i = 0; i < regionslist.GetSize(); ++i)
	{
		vars title = regionslist[i].id;
		vars parent = regionslist[i].data;

		// create region
		CPage p(f, NULL, title);
		if (p.lines.length() > 0) {
			Log(LOGERR, "ERROR: new region page not empty (title=%s)\n%.256s", title, p.lines.join("\n"));
			continue;
		}

		const char *def[] = {
				"{{Region",
				"}}",
				NULL };
		p.Set(def);;
		p.Set("Parent region", parent);
		p.comment.push("Update: new region");
		Log(LOGINFO, "POST REGION %s -> %s ", title, parent);
		p.Update();
	}

	return TRUE;
}


int AutoRegion(CSym &sym)
{
	// map region
	CString oregion = sym.GetStr(ITEM_REGION);
	if (strstr(oregion, RWID))
		oregion = GetToken(oregion, 3, ':');
	else
		oregion = regionmatch(oregion);
	sym.SetStr(ITEM_REGION, oregion);

	// AutoRegion
	vara georegions;
	int r = _GeoRegion.GetRegion(sym, georegions);
	if (r < 0)
	{
		Log(LOGWARN, "WARNING: No AutoRegion for %s [%s] (%s,%s), using '%s'", sym.id, sym.GetStr(ITEM_DESC), sym.GetStr(ITEM_LAT), sym.GetStr(ITEM_LNG), oregion);
		return FALSE;
	}

	vara aregions;
	for (int i = 0; i <= r; ++i)
		aregions.push(GetToken(georegions[i], 0, ':'));
	vars aregion = aregions.join(";");

	// check if auto region needed
	vars fregion = _GeoRegion.GetFullRegion(oregion);
	if (strstri(fregion, aregions.last()))
		return TRUE;

	// Autoregion
	Log(LOGINFO, "AutoRegion '%s'=>'%s' for %s [%s]", oregion, aregion, sym.GetStr(ITEM_DESC), sym.id);
	sym.SetStr(ITEM_REGION, aregion);
	return TRUE;
}


int UploadRegion(CSym &sym, CSymList &newregions)
{
	vars region = sym.GetStr(ITEM_REGION);
	if (region.IsEmpty())
		return FALSE;

	vara rlist(region, ";");
	vars lastregion = _GeoRegion.GetRegion(rlist.last());
	if (!lastregion.IsEmpty())
		return TRUE;


	// new region 
	Log(LOGWARN, "WARNING: Region '%s' [%s] needs to be created", lastregion, region);

	if (rlist.length() <= 1)
	{
		_GeoRegion.AddRegion(region, "");
		return TRUE;
	}

	sym.SetStr(ITEM_REGION, rlist.last());

	// build region tree  
	for (int i = 1; i < rlist.length(); ++i) {
		if (!_GeoRegion.GetRegion(rlist[i]).IsEmpty())
			continue;

		vars title = rlist[i];
		vars parent = rlist[i - 1];
		_GeoRegion.AddRegion(title, parent);
		newregions.Add(CSym(title, parent));
	}

	return TRUE;
}


int UploadRegions(int mode, const char *file)
{
	CSymList list;
	list.Load(file);

	CSymList newregionslist;
	for (int i = 0; i < list.GetSize(); ++i) {
		CSym &sym = list[i];
		if (sym.id.IsEmpty())
			continue;
		// cleanup usual stuff
		UploadRegion(sym, newregionslist);
	}

	// create regions
	DownloadFile f;
	if (!Login(f))
		return FALSE;
	CreateRegions(f, newregionslist);
	return TRUE;
}


int BQNupdate(int code, const char *id, double rwidnum)
{
	if (rwidnum <= 0)
		return FALSE;

	CSymList list;
	vars file = filename(codelist[code]->code);
	list.Load(file);
	int find = list.Find(id);
	if (find < 0)
	{
		Log(LOGERR, "Invalid SPECIAL ID %s", id);
		return FALSE;
	}

	list[find].SetStr(ITEM_INFO, RWID + CData(rwidnum));
	list.Save(file);
	return TRUE;
}


int FixBetaPage(CPage &p, int MODE = -1);


int UploadBeta(int mode, const char *file)
{
	CSymList list;
	vars chgfile = filename(CHGFILE);
	if (file != NULL && *file != 0)
		chgfile = file;
	list.Load(chgfile);
	CSymList clist[sizeof(codelist) / sizeof(*codelist)];
	rwlist.Load(filename(codelist[0]->code));
	rwlist.Sort();

	// log in as RW_ROBOT_USERNAME [RW_ROBOT_PASSWORD]
	int ret = 0;
	DownloadFile f;
	if (!Login(f))
		return FALSE;

	CSymList newregionslist;
	for (int i = 0; i < list.GetSize(); ++i) {
		CSym &sym = list[i];
		if (sym.id.IsEmpty())
			continue;
		if (!sym.GetStr(ITEM_NEWMATCH).IsEmpty())
			continue;
		// cleanup usual stuff
		CString match = sym.GetStr(ITEM_MATCH);
		if (sym.id.Find(RWID) < 0)
		{
			if (match.Find(RWID) >= 0)
			{
				if (match.Find(RWLINK, 1) < 0)
				{
					// new match, skip any data assignment
					sym.SetStr(ITEM_LAT, "");
					sym.SetStr(ITEM_LNG, "");
					for (int m = ITEM_REGION; m < ITEM_BETAMAX; ++m)
						if (m != ITEM_KML)
							sym.SetStr(m, "");
				}
			}
			else
				AutoRegion(sym);
		}
		if (!strstr(match, RWID))
			UploadRegion(sym, newregionslist);
		else
			if (MODE > -2)
				sym.SetStr(ITEM_REGION, "");
	}

	// create regions
	CreateRegions(f, newregionslist);

	// Canyons
	int error = 0;
	for (int i = 0; i < list.GetSize() && error < 3; ++i)
	{
		CSym &sym = list[i];
		if (sym.id.IsEmpty())
			continue;

		int code = GetCode(sym.id);
		const char *pagename = GetPageName(sym.GetStr(ITEM_DESC));

		if (IsSimilar(sym.id, RWID)) // id in 1st col
			sym.SetStr(ITEM_MATCH, sym.id);

		// skip new matches (manual monitoring)
		if (!sym.GetStr(ITEM_NEWMATCH).IsEmpty())
			continue;

		// check codebase
		const char *user = code >= 0 ? codelist[code]->betac->ubase : NULL;
		if (code < 0 || !user) {
			Log(LOGERR, "Invalid code/name for %s", sym.Line());
			continue;
		}

		CString url;
		int idnum = -1;
		vars id = sym.GetStr(ITEM_MATCH), title;
		const char *oldid = "";
		const char *rwid = strstr(id, RWID);
		if (rwid) {
			idnum = (int)CDATA::GetNum(rwid + strlen(RWID));
			int f = rwlist.Find(RWID + (id = MkString("%d", idnum)));
			if (idnum > 0 && f >= 0)
				title = rwlist[f].GetStr(ITEM_DESC);
			else
			{
				Log(LOGERR, "Invalid pageid=%s title=%s", id, title);
				continue;
			}
		}
		else
		{
			// create new page
			title = id; id = ""; oldid = "new";
			if (title.IsEmpty()) {
				Log(LOGERR, "Invalid title=%s for %s", title, sym.id);
				continue;
			}
		}
		printf("%d/%d %s %s    \r", i, list.GetSize(), id, title);

		// process IGNORE
		if (title.upper() == "X")
		{
			CFILE f;
			if (f.fopen(filename("ignore"), CFILE::MAPPEND))
				f.fputstr(sym.id + ",X AUTOIGNORE");
			continue;
		}

		// Process BQN:
		BOOL BQNmode = code > 0 && !IsSimilar(sym.id, "http") && !IsSimilar(sym.GetStr(ITEM_INFO), RWID);
		if (BQNmode && idnum > 0)
			BQNupdate(code, sym.id, idnum);

		CPage p(f, id, title, oldid);

		if (idnum >= 0 && p.lines.length() <= 1) {
			Log(LOGERR, "ERROR: update of empty page (#lines=%d)", p.lines.length());
			continue;
		}

		if (idnum < 0 && p.lines.length()>0) {
			Log(LOGERR, "ERROR: new page not empty (title=%s)\n%.256s", title, p.lines.join("\n"));
			PurgePage(f, id, title);
			continue;
		}

		vars basecomment = "Updated: ";
		if (idnum < 0) {
			// create new page
			const char *def[] = {
				"{{Canyon",
				"}}",
				"==Introduction==",
				"==Approach==",
				"==Descent==",
				"==Exit==",
				"==Red tape==",
				"==Beta sites==",
				"==Trip reports and media==",
				"==Background==",
				NULL };
			p.Set(def);
			if (sym.GetNum(ITEM_CLASS) == 2) // Cave
			{
				int red = p.lines.indexOf("==Red tape==");
				if (red > 0)
					p.lines.InsertAt(red + 1, "If you want to get into serious cave exploration, join some experienced local caving group (sometimes called 'grotto's).");
			}
			/*
			if (clist[code].GetSize()==0)
				clist[code].Load(filename(codelist[code].code));
			int f = clist[code].Find(sym.id);
			if (f<0) {
				Log(LOGERR, "Inconsistent symbol %s, could not find in %s", sym.id, filename(codelist[code].code));
				continue;
				}
			sym = clist[code][f];
			sym.SetStr(ITEM_MATCH, "");
			sym.SetStr(ITEM_NEWMATCH, "");
			*/			basecomment = "New page: ";

		}

		// update
		int updates = UpdatePage(sym, code, p.lines, p.comment, title);
		if (!updates || !p.NeedsUpdate())
			continue; // No need update!

		Log(LOGINFO, "POST %d/%d %s %s [#%d] U%d %s CSym:%s", i, list.GetSize(), basecomment, title, idnum, updates, p.comment.join(), sym.data);
		p.comment.InsertAt(0, basecomment);

		// since we are at it, fix it
		FixBetaPage(p);
		p.Update();

		if (BQNmode && idnum < 0)
			// created a new page, get page id!
			if (!f.Download("http://ropewiki.com/api.php?action=query&format=xml&prop=info&titles=" + title))
				BQNupdate(code, sym.id, ExtractNum(f.memory, "pageid=", "\"", "\""));
	}

	return TRUE;
}


vars SetSectionImage(int big, const char *url, const char *urlimgs, const char *titleimgs, vara &uploadfiles)
{
	vars saveimg = url2file(urlimgs, BQNFOLDER);
	if (uploadfiles.indexOf(saveimg) >= 0)
		return ""; // already embedded
	if (!CFILE::exist(saveimg))
	{
		Log(LOGERR, "LOST SAVED IMG %s from %s", urlimgs, url);
		return "";
	}
	uploadfiles.push(saveimg);
	if (big)
		return MkString("{{pic|size=X|:%s~%s}}\n", GetFileNameExt(saveimg), titleimgs);
	else
		return MkString("{{pic|:%s~%s}}\n", GetFileNameExt(saveimg), titleimgs);
}


int SetSectionContent(const char *url, CPage &p, const char *section, const char *content, vara &uploadfiles, int keepbold = TRUE)
{
	int start, end;
	start = p.Section(section, &end);
	if (start < 0)
	{
		Log(LOGERR, "Invalid section '%s', skipping %s", section, url);
		return FALSE;
	}
	if (end >= 0 && MODE > -2)
	{
		//Log(LOGERR, "Not empty Section '%s', skipping %s [MODE -2:append -3:overwrite]", section, url);
		return FALSE;
	}

	// convert content with images
	vara urlimgs, titleimgs;
	vars desc = Download_SaveImg(url, content, urlimgs, titleimgs);

	if (!keepbold)
	{
		desc.Replace("<b>", "");
		desc.Replace("</b>", "");
	}

	for (int i = 0; i < urlimgs.length(); ++i)
		desc += SetSectionImage(FALSE, url, urlimgs[i], titleimgs[i], uploadfiles);

	if (MODE < -2)
	{
		// overwrite
		for (int i = start; i < end; ++i)
			p.lines.RemoveAt(start);
		while (start < p.lines.length() && p.lines[start].Trim().IsEmpty())
			p.lines.RemoveAt(start);
		end = -1;
	}
	desc.Trim(" \n");
	if (end < 0)
		p.lines.InsertAt(start, desc);
	else
		p.lines.InsertAt(end, "\n" + desc);
	return TRUE;

}


CString ExtractBetaBQN(const char *url, CString &memory, vara &ids, int ext = FALSE, const char *start = NULL, const char *end = NULL)
{
	CString text;
	for (int i = 0; i < ids.length() && text.IsEmpty(); ++i)
		text = ExtractStringDel(memory, "<b>" + UTF8(ids[i]), !start ? "</b>" : start, !end ? "<b>" : end, ext ? -1 : FALSE, -1);

	if (text.IsEmpty())
	{
		Log(LOGWARN, "Empty section '%s' for %s", ids.join("/"), url);
		Log(memory);
		return "";
	}

	// cleanup 
	while (!ExtractStringDel(text, "<td", "", ">", TRUE).IsEmpty()); text.Replace("</td>", "");
	while (!ExtractStringDel(text, "<tr", "", ">", TRUE).IsEmpty()); text.Replace("</tr>", "");
	while (!ExtractStringDel(text, "<font", "", ">", TRUE).IsEmpty()); text.Replace("</font>", "");
	while (!ExtractStringDel(text, "<p", "", ">", TRUE).IsEmpty()); text.Replace("</p>", "");
	while (!ExtractStringDel(text, "<div", "", ">", TRUE).IsEmpty()); text.Replace("</div>", "");
	while (!ExtractStringDel(text, "<center", "", ">", TRUE).IsEmpty()); text.Replace("</center>", "");
	while (text.Replace("  ", " "));
	text.Replace("<br> ", "<br>");
	text.Replace(" <br>", "<br>");
	while (text.Replace("<br><br>", "<br>"));
	while (text.Replace("<b></b>", ""));
	while (text.Replace("<b> </b>", ""));
	while (text.Replace("</b><b>", ""));
	while (text.Replace("</b> <b>", ""));

	text.Trim();
	if (text.IsEmpty())
		return "";

	text.Replace("<br>", "");
	text.Replace("</b>", "");
	vara lines(text, "<b>");
	for (int i = 1; i < lines.length(); ++i)
	{
		vars &line = lines[i];
		int dot = line.Find(":");
		if (dot < 0 && !line.IsEmpty())
		{
			// no ':'
			Log(LOGWARN, "No <b>: line '%s' for %s", line, url);
			Log(text);
			line.Insert(0, "</b>");
		}
		if (dot >= 0)
		{
			line.Insert(dot + 1, "</b>");
			line.Trim(". \n");
			line += "</div>\n";
		}
	}
	int startindex = lines.length() > 1 ? 1 : 0;
	for (int i = startindex; i < lines.length(); ++i)
	{
		int dot = lines[i].Find(":");
		vars text = stripAccentsL(stripHTML(lines[i].Mid(dot + 1))).Trim(". ");
		if (text == "" || text == "sin datos" || text == "sans donnees")
		{
			lines.RemoveAt(i--);
			continue;
		}
	}
	vars ret = lines.join("<div><b>").Trim(" \n");
	if (!ret.IsEmpty()) ret += "\n";
	return ret;
}


int ExtractCoverBQN(const char *urlimg, const char *file, const char *album = NULL)
{
	vars url;
	DownloadFile f(TRUE);
	if (album)
	{
		url = "http://www.barranquismo.org/imagen/displayimage.php?" + vars(album) + "&pos=0";
		if (f.Download(url))
		{
			Log(LOGERR, "Could not download album page %s", url);
			return FALSE;
		}
		vars img = ExtractString(f.memory, "\"albums", "", "\"");
		if (img.IsEmpty())
		{
			Log(LOGWARN, "Could not exxxxxxxxxxxxxxxxxtract <img src= from %s", url);
			return FALSE;
		}

		url = "http://www.barranquismo.org/imagen/albums" + img;
	}
	else
	{
		if (f.Download(urlimg))
		{
			Log(LOGERR, "Could not download album page %s", url);
			return FALSE;
		}
		url = ExtractString(f.memory, "FIN DE CODIGO INICIAL", "src=\"", "\"");
		url = url2url(url, urlimg);
	}

	if (f.Download(url, file))
	{
		Log(LOGERR, "Could not download cover pic %s", url);
		return FALSE;
	}
	return TRUE;
}


int UpdateBetaBQN(int mode, const char *ynfile)
{
	CSymList list, ynlist;
	ynlist.Load(ynfile);

	vars bqnfile = filename("bqn");
	list.Load(bqnfile);
	list.Sort();

	//CSymList clist[sizeof(codelist)/sizeof(*codelist)];
	rwlist.Load(filename(codelist[0]->code));
	rwlist.Sort();

	// log in 
	loginstr = vara("Barranquismo.net,sentomillan");
	int ret = 0;
	DownloadFile f;
	if (!Login(f))
		return FALSE;

	// Canyons
	int error = 0;
	for (int i = 0; i < ynlist.GetSize(); ++i)
	{
		CSym &ynsym = ynlist[i];

		// check transfer authorization
		if (!strstr(ynsym.GetStr(6), "Y"))
			continue;

		// check if id is valid
		if (ynsym.id.IsEmpty())
			continue;
		int li = list.Find(ynsym.id);
		if (li < 0)
		{
			Log(LOGERR, "Invalid BQN id %s, skipping", ynsym.id);
			continue;
		}

		CSym &sym = list[li];

		//check if already transferred
		if (mode != -3 && !sym.GetStr(ITEM_STARS).IsEmpty())
			continue;
		//check if not transferred (overwrite)
		if (mode == -3 && !strstr(sym.GetStr(ITEM_STARS), "Y"))
			continue;

		// check if RWID is valid
		//int code = GetCode(sym.id);
		//const char *pagename = GetPageName(sym.GetStr(ITEM_DESC));

		CString url;
		int idnum = -1;
		vars id = sym.GetStr(ITEM_MATCH), title;
		const char *rwid = strstr(id, RWID);
		if (!rwid || !strstr(id, RWLINK))
		{
			Log(LOGERR, "Invalid RWLINK for %s", id);
			continue;
		}
		idnum = (int)CDATA::GetNum(rwid + strlen(RWID));
		int found = rwlist.Find(RWID + (id = MkString("%d", idnum)));
		if (idnum > 0 && found >= 0)
			title = rwlist[found].GetStr(ITEM_DESC);
		else
		{
			Log(LOGERR, "Invalid pageid=%s title=%s", id, title);
			continue;
		}

		printf("%d/%d %s %s    \r", i, list.GetSize(), id, title);

		//Throttle(barranquismoticks, 1000);
		CPage p(f, id, title);

		if (mode == -4)
		{
			int bqn = FALSE;
			int end, start = p.Section("Introduction", &end);
			for (int i = start; i < end && !bqn; ++i)
				bqn = IsSimilar(p.lines[i], "<div><b>") || IsSimilar(p.lines[i], "{{pic|size=X|:");
			if (bqn)
			{
				// beta was already transferred
				sym.SetStr(ITEM_STARS, "Y");
				list.Save(bqnfile);
				continue;
			}
			continue;
		}

		vara files;
		vars credit = MkString("<div style=\"display:none\" class=\"barranquismonet\">%s %s</div>\n", ynsym.GetStr(5), ynsym.GetStr(4));
		// cover pic
		vars bannerjpg = title.replace(" ", "_") + "_Banner.jpg";
		const char *lcover = "cover.jpg";
		const char *rcover = NULL;
		CPage up(f, NULL, NULL, NULL);
		if (up.FileExists(bannerjpg))
			lcover = NULL;

		if (IsImage(sym.id))
		{
			// pdf or jpg beta
			// update page
			int err = 0;
			DownloadFile fb(TRUE);
			vars embedded = SetSectionImage(TRUE, sym.id, sym.id, "Click on picture to open PDF document", files);
			if (fb.Download(sym.id, files[0]))
			{
				Log(LOGERR, "Could not download %s, skipping %s", sym.id, title);
				continue;
			}

			err += !SetSectionContent(sym.id, p, "Introduction", embedded, files);
			err += !SetSectionContent(sym.id, p, "Background", credit, files);

			if (err > 0)
			{
				Log(LOGERR, "%d SECTION ERRORS DETECTED, skipping %s (%s)", err, title, sym.id);
				continue;
			}
			if (lcover)
				Log(LOGWARN, "EMBEDDED cover for %s in %s", title, sym.id);
		}
		else
		{
			// html beta
			CString memory;
			if (!Download_Save(sym.id, "BQN", memory))
			{
				Log(LOGERR, "Can't download page %s", sym.id);
				continue;
			}

			// utf8
			if (!strstr(memory, "\xc3\xad") && !strstr(memory, "\xc3\x89"))
				memory = UTF8(memory);
			else
				Log(LOGINFO, "UTF8 in %s", sym.id);

			// delete sections
			vars region = ExtractBetaBQN(sym.id, memory, vara("País,Pays"), FALSE, "</b>", "<br>");
			vars region2 = ExtractBetaBQN(sym.id, memory, vara("Provincia,Province"), FALSE, "</b>", "<br>");
			// extract sections		
			vars approach = ExtractBetaBQN(sym.id, memory, vara("Aproximaci,Approche"));
			vars exit = ExtractBetaBQN(sym.id, memory, vara("Retorno,Retour"));
			vars descent = ExtractBetaBQN(sym.id, memory, vara("Descripci,Descripti"));
			vars escapes = ExtractBetaBQN(sym.id, memory, vara("Escapes,Échappatoires"), TRUE);
			vars shuttle = ExtractBetaBQN(sym.id, memory, vara("Combinación,Navette"), TRUE);
			//vars watch = ExtractBetaBQN(sym.id, memory, vara("Observaciones,Rappel"), TRUE);
			vars background = ExtractBetaBQN(sym.id, memory, vara("Historia,Histoire"), FALSE, ":", "<tr>");

			// redtape
			vars redtape = ExtractBetaBQN(sym.id, memory, vara("Restricciones,Réglementations"), FALSE, ":", "<tr>");
			redtape.Replace("No se conocen", "");
			redtape.Replace("Ils ne son pas connus / Inconnus", "");
			if (stripHTML(redtape).IsEmpty())
				redtape = "";
			else
				redtape = redtape;

			// data tables
			vars data1 = ExtractBetaBQN(sym.id, memory, vara("Datos prácticos,Données"), FALSE, "</b>", "<tr>");
			vars data2 = ExtractBetaBQN(sym.id, memory, vara("Otros datos,D'autres donnés"), FALSE, "</b>", "<tr>");
			data2.Replace("<div><b>Especies amenazadas:</b> En todos los habitats viven animales y plantas que merecen nuestro respeto</div>\n", "");

			if (data1.IsEmpty() || data2.IsEmpty())
			{
				Log(LOGERR, "Invalid data1/data2 for %s for %s", title, sym.id);
				continue;
			}

			if (!escapes.IsEmpty())
				descent += "\n" + escapes;
			vars comp = stripAccentsL(GetToken(stripHTML(shuttle), 1, ':')).Trim(" .");
			if (!shuttle.IsEmpty() && !(comp == "no" || comp == "sin combinacion" || comp == "no necesaria" || comp == "no necesario"))
				approach = shuttle + "\n" + approach;
			//data2 += shuttle;

			vars album;
			vars maps, topo, fotos;

			// topos and maps
			vara urlimgs, titleimgs, embeddedimgs;
			vars imglinks = ExtractString(memory, "TABLA IZQUIERDA", "", "OTROS DATOS");
			Download_SaveImg(sym.id, imglinks, urlimgs, titleimgs, TRUE);
			if (imglinks.IsEmpty() || urlimgs.length() == 0)
			{
				Log(LOGERR, "Invalid TABLA IZQUIERDA for %s for %s", title, sym.id);
				continue;
			}
			//continue;
			for (int u = 0; u < urlimgs.length(); ++u)
				if (IsBQN(urlimgs[u]))
				{
					vars urlimg = urlimgs[u];
					vars titleimg = titleimgs[u];
					vars search = stripAccentsL(titleimg);
					if (IsImage(urlimgs[u]))
					{
						if (u == 0)
						{
							if (!strstr(urlimg, "croquisejemplo"))
								topo += SetSectionImage(TRUE, sym.id, urlimg, titleimg, files);
						}
						else if (search.indexOf("topo") >= 0 || search.indexOf("esquema") >= 0 || search.indexOf("croquis") >= 0 || search.IsEmpty())
							topo += SetSectionImage(TRUE, sym.id, urlimg, titleimg, files);
						else if (search.indexOf("map") >= 0 || search.indexOf("ortofoto") >= 0 || search.indexOf("cabecera") >= 0 || search.indexOf("vista") >= 0 || search.indexOf("aviso") >= 0 || search.indexOf("panor") >= 0)
							maps += SetSectionImage(TRUE, sym.id, urlimg, titleimg, files);
						else if (search.indexOf("fotos") >= 0 || search.indexOf("album") >= 0)
							fotos += SetSectionImage(TRUE, sym.id, urlimg, titleimg, files);
						else
						{
							Log(LOGWARN, "Unexpected img '%s' '%s' from '%s'", titleimg, urlimg, sym.id);
							maps += SetSectionImage(TRUE, sym.id, urlimg, titleimg, files);
						}
					}

					// cover picture
					if (!lcover || rcover)
						continue;

					const char *alb = strstr(urlimg, "album=");
					if (alb)
					{
						if (ExtractCoverBQN(urlimg, lcover, GetToken(alb, 0, "&<> ")))
							rcover = bannerjpg;
						else
							Log(LOGWARN, "xxxxxxxx No cover picture for '%s' from %s", title, sym.id);
						continue;
					}
					if (search.indexOf("fotos") >= 0 || search.indexOf("album") >= 0)
					{
						if (!IsImage(urlimg))
						{
							if (ExtractCoverBQN(urlimg, lcover))
								rcover = bannerjpg;
							else
								Log(LOGERR, "NON EMBEDDED Album for #%s '%s' url %s", id, title, urlimg); // Requires manual extraction
							continue;
						}

						// Requires manual extraction
						Log(LOGWARN, "EMBEDDED Album for #%s '%s' url %s", id, title, urlimg);
					}
				}

			// update beta sites
			{
				int start, end;
				start = p.Section("Beta", &end);
				vars betasites = ExtractBetaBQN(sym.id, memory, vara("Mas información,Plus d'informations"), FALSE, "", "<tr>");
				Download_SaveImg(sym.id, betasites, urlimgs, titleimgs, TRUE);
				for (int u = 0; u < urlimgs.length(); ++u)
				{
					vars link = urlstr(urlimgs[u]);
					if (IsBQN(link))
						continue;

					int l;
					for (l = start; l < end && !strstr(p.lines[l], link); ++l);
					if (l < end) // already listed
						continue;
					p.lines.InsertAt(end++, MkString("* %s", link));
				}
			}

			// update page
			int err = 0;
			err += !SetSectionContent(sym.id, p, "Introduction", data1 + data2, files);
			err += !SetSectionContent(sym.id, p, "Approach", approach + maps, files);
			err += !SetSectionContent(sym.id, p, "Descent", descent + topo, files);
			err += !SetSectionContent(sym.id, p, "Exit", exit, files);
			err += !SetSectionContent(sym.id, p, "Red tape", redtape, files);
			err += !SetSectionContent(sym.id, p, "Trip", fotos, files);
			err += !SetSectionContent(sym.id, p, "Background", credit + background, files);

			if (err > 0)
			{
				Log(LOGERR, "%d SECTION ERRORS DETECTED, skipping %s (%s)", err, title, sym.id);
				continue;
			}
		}

		// update
		int err = 0;
		vars basecomment = "Imported from Barranquismo.net " + sym.id;
		Log(LOGINFO, "POST %d/%d %s %s [#%d]", i, list.GetSize(), basecomment, title, idnum);
		if (MODE > -1)
		{
			// do not update
			Log("\n" + p.lines.join("\n") + "\n");
			continue;
		}
		else
		{
			// upload files
			for (int u = 0; u < files.length(); ++u)
			{
				const char *rfile = GetFileNameExt(files[u]);
				if (!up.FileExists(rfile))
					err += !up.UploadFile(files[u], rfile);
			}
			// upload cover pic
			if (lcover && rcover)
				err += !up.UploadFile(lcover, rcover);

			if (err > 0)
			{
				Log(LOGERR, "%d FILE ERRORS DETECTED, skipping %s", err, sym.id);
				continue;
			}

			// update
			vars plines = p.lines.join("\n");
			p.comment.InsertAt(0, basecomment);
			if (!p.Update())
				continue;

			// double check
			CPage p2(f, id, title);
			vars p2lines = p2.lines.join("\n");
			if (htmltrans(plines) != htmltrans(p2lines))
			{
				int n, nl, ol;
				vara newlines(plines, "\n");
				vara lines(p2lines, "\n");
				for (n = 0; n < newlines.length() && n < lines.length(); ++n)
					if (newlines[n] != lines[n])
						break;
				vars dump;
				for (nl = newlines.length() - 1, ol = lines.length() - 1; nl > n && ol > n; --nl, --ol)
					if (newlines[nl] != lines[ol])
					{
						dump += MkString("#%d : ", i);
						dump += (i <= nl) ? newlines[i] : "";
						dump += MkString(" =!= ");
						dump += (i <= ol) ? lines[i] : "";
						dump += "\n";
					}
				Log(LOGERR, "INCONSISTENT UPDATE for %s! #newlines:%d =?= #oldlines:%d diff %d-%d", title, newlines.length(), n < lines.length(), n, nl);
				Log(dump);
			}

			sym.SetStr(ITEM_STARS, "Y");
			list.Save(bqnfile);
		}
	}


	// update 
	return TRUE;
}


vars ExtractStringCCS(CString &memory, const char *start)
{
	vars ret = ExtractStringDel(memory, "<b>" + vars(start), "</b>", "</body", FALSE, -1);
	if (ret.IsEmpty()) return "";
	return ret.Trim();//"<div>"+ret.split("<b>").join("</div>\n<div><b>").Trim()+"</div>";
}


int LoadCCS(const char *htmlfile, CString &memory)
{
	if (!CFILE::exist(htmlfile))
		return FALSE;

	Load_File(htmlfile, memory);
	if (!strstr(memory, "charset=utf-8"))
		memory = UTF8(memory);
	// patch
	memory.Replace("</BODY", "</body");
	memory.Replace("\t", " ");
	memory.Replace("\r", " ");
	memory.Replace("\n", " ");
	memory.Replace("<hr>\n", "");
	memory.Replace("<h2>", "<b>");
	memory.Replace("</h2>", "</b><br>");
	memory.Replace("<h1>", "<b>");
	memory.Replace("</h1>", "</b><br>");
	memory.Replace("<h3>", "<b>");
	memory.Replace("</h3>", "</b><br>");
	memory.Replace("<h4>", "<b>");
	memory.Replace("</h4>", "</b><br>");
	memory.Replace("<br/>", "<br>");

	while (memory.Replace("<br> ", "<br>"));
	while (memory.Replace(" <br>", "<br>"));
	while (memory.Replace("<br><br>", "<br>"));
	while (memory.Replace("<b><br>", "<br><b>"));
	while (memory.Replace("<br></b>", "</b><br>"));
	while (memory.Replace("<b></b>", ""));

	while (!ExtractStringDel(memory, "<IMG", "", ">").IsEmpty());

	// get bold styles
	vara bolds;
	vara styles(ExtractString(memory, "<style", "", "</style"), ".");
	for (int i = 1; i < styles.length(); ++i)
		if (strstr(ExtractString(styles[i], "{", "", "}"), "bold"))
		{
			vars bold = GetToken(styles[i], 0, " ,{");
			vars spans = "<span class=\"" + bold + "\"", spane = "</span>";
			while (TRUE)
			{
				vars span = ExtractStringDel(memory, spans, ">", spane, TRUE, FALSE);
				if (span.IsEmpty()) break;
				vars spanin = ExtractStringDel(memory, spans, ">", spane, FALSE, FALSE).Trim();
				if (spanin.IsEmpty())
					memory.Replace(span, " "); // eliminate
				else
					memory.Replace(span, "<b>" + spanin + "</b>"); // encapsulate
			}
			spans = "<p class=\"" + bold + "\"", spane = "</p>";
			while (TRUE)
			{
				vars span = ExtractStringDel(memory, spans, ">", spane, TRUE, FALSE);
				if (span.IsEmpty()) break;
				vars spanin = ExtractStringDel(memory, spans, ">", spane, FALSE, FALSE).Trim();
				if (spanin.IsEmpty())
					memory.Replace(span, " "); // eliminate
				else
					memory.Replace(span, "<b>" + spanin + "</b><br>"); // encapsulate
			}
			bolds.push(bold);
		}

	const char *check = strstr(memory, "Description of");
	memory.Replace("</b>", "</b> ");

	{
		vara lines(memory, "<b>");
		for (int i = 1; i < lines.length(); ++i)
		{
			// <b> </b> if br replace </b><br><b>
			vara blines(lines[i], "</b>");
			//ASSERT(!strstr(blines[0], "T 2m"));
			blines[0].Replace("<br>", "</b><br><b>");
			while (TRUE)
			{
				vars bs = "</b>", be = " <b>";
				vars span = ExtractStringDel(blines[0], "<span class=", ">", "</span>", TRUE, FALSE);
				vars spanin = ExtractStringDel(blines[0], "<span class=", ">", "</span>", FALSE, FALSE).Trim();
				vars sclass = ExtractString(span, "<span ", "\"", "\"");
				if (sclass.IsEmpty())
				{
					span = ExtractStringDel(blines[0], "<p class=", ">", "</p>", TRUE, FALSE);
					spanin = ExtractStringDel(blines[0], "<p class=", ">", "</p>", FALSE, FALSE).Trim();
					sclass = ExtractString(span, "<p ", "\"", "\"");
					be = "<br><b>";
				}
				if (sclass.IsEmpty())
					break;
				if (spanin.IsEmpty())
					blines[0].Replace(span, " "); // eliminate
				else if (bolds.indexOf(sclass) < 0)
					blines[0].Replace(span, bs + spanin + be); // encapsulate
			}
			lines[i] = blines.join("</b>");
		}
		memory = lines.join("<b>");
	}

	{
		vara linesb(memory, "<b>");
		for (int i = 1; i < linesb.length(); ++i)
		{
			vara blines(linesb[i], "</b>");
			blines[0].Replace(": ", ":</b> <b>");
			linesb[i] = blines.join("</b>");
		}
		memory = linesb.join("<b>");
	}

	const char *check2 = strstr(memory, "Description of");

	memory.Replace("</p>", "<br>");
	memory.Replace("<p>", "");
	while (!ExtractStringDel(memory, "<p ", "", ">").IsEmpty());

	//memory.Replace("<br>", " ");
	while (memory.Replace("  ", " "));
	while (memory.Replace("<b></b>", ""));
	while (memory.Replace("<b> </b>", " "));
	while (memory.Replace("<b>. ", ". <b>"));
	while (memory.Replace("<br><br>", "<br>"));
	//memory.Replace("<br>", "\n\n");

	memory.Replace("</span>", "");
	while (!ExtractStringDel(memory, "<span", "", ">", TRUE).IsEmpty());

	memory.Replace("<A name=1></a>", "");
	memory.Replace("<A name=2></a>", "");
	memory.Replace("<A name=3></a>", "");
	memory.Replace("<A name=4></a>", "");
	memory.Replace("<A name=5></a>", "");
	memory.Replace("<b>RETURN:", "<b>Return:");

	const char *check3 = strstr(memory, "Description of");
	return TRUE;
}


int CCS_DownloadPage(const char *memory, CSym &sym)
{

	sym.SetStr(ITEM_SEASON, stripHTML(ExtractString(memory, ">Period", " ", "<b>")));

	GetSummary(sym, stripHTML(ExtractString(memory, ">Difficulty", "", "<b>")));

	sym.SetStr(ITEM_LONGEST, GetMetric(stripHTML(ExtractString(memory, ">Rope", "x", "<b>"))));
	sym.SetStr(ITEM_LENGTH, GetMetric(stripHTML(ExtractString(memory, "Length:", "", "<b>"))));
	sym.SetStr(ITEM_DEPTH, GetMetric(stripHTML(ExtractString(memory, "Height:", "", "<b>"))));

	sym.SetStr(ITEM_SHUTTLE, stripHTML(ExtractString(memory, ">Shuttle", "", "<b>").replace(":", "").replace("Yes,", "").replace(",", ".")));

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
	const char *ids[] = { "Approach:", "Progression:", "Return:" };
	//ASSERT( !strstr(url,"/kanion.php?id=41"));
	vars timestr = ExtractString(memory, ">Time", "", "<b>");
	for (int t = 0; t < sizeof(ids) / sizeof(*ids); ++t)
		timestr.Replace(ids[t], vars("<b>") + ids[t]);
	for (int t = 0; t < sizeof(ids) / sizeof(*ids); ++t)
	{
		CString time = stripHTML(ExtractString(timestr, ids[t], "", "<b>"));
		while (!ExtractStringDel(time, "(", "", ")").IsEmpty());
		times.push(vars(time).replace("hours", "h").replace("min", "m").replace(";", "."));
	}
	GetTotalTime(sym, times, sym.id);

	return TRUE;
}


vars nobr(const char *str, int boldline = TRUE)
{
	vars ret(str);
	ret.Replace("<br>", " ");
	if (boldline)
		ret.Replace("<b>", "\n\n<b>");
	return ret.Trim(" \n\t\r");
}


int UpdateBetaCCS(int mode, const char *ynfile)
{
	CSymList filelist;
	const char *exts[] = { "JPG", NULL };
	GetFileList(filelist, CCS, exts, FALSE);

	CSymList list, ynlist;
	ynlist.Load(ynfile);

	vars bqnfile = filename("ccs");
	list.Load(bqnfile);
	list.Sort();

	//CSymList clist[sizeof(codelist)/sizeof(*codelist)];
	rwlist.Load(filename(codelist[0]->code));
	rwlist.Sort();

	// log in 
	loginstr = vara("Canyoning Cult,slovenia");
	int ret = 0;
	DownloadFile f;
	if (!Login(f))
		return FALSE;

	// Canyons
	int error = 0;
	for (int i = 0; i < ynlist.GetSize(); ++i)
	{
		CSym &ynsym = ynlist[i];
		if (!ynsym.GetStr(ITEM_NEWMATCH).IsEmpty())
			continue;

		// check if id is valid
		if (ynsym.id.IsEmpty())
			continue;
		int li = list.Find(ynsym.id);
		if (li < 0)
		{
			Log(LOGERR, "Invalid BQN id %s, skipping", ynsym.id);
			continue;
		}

		CSym &sym = list[li];
		if (sym.GetStr(ITEM_KML).IsEmpty())
			continue;

		//check if already transferred
		if (mode != -3 && !sym.GetStr(ITEM_STARS).IsEmpty())
			continue;
		//check if not transferred (overwrite)
		if (mode == -3 && !strstr(sym.GetStr(ITEM_STARS), "Y"))
			continue;

		// check if RWID is valid
		//int code = GetCode(sym.id);
		//const char *pagename = GetPageName(sym.GetStr(ITEM_DESC));

		CString url;
		int idnum = -1;
		vars id = ynsym.GetStr(ITEM_MATCH), title;
		const char *rwid = strstr(id, RWID);
		if (!rwid)// || !strstr(id, RWLINK))
		{
			Log(LOGERR, "Invalid RWLINK for %s", id);
			continue;
		}
		idnum = (int)CDATA::GetNum(rwid + strlen(RWID));
		int found = rwlist.Find(RWID + (id = MkString("%d", idnum)));
		if (idnum > 0 && found >= 0)
			title = rwlist[found].GetStr(ITEM_DESC);
		else
		{
			Log(LOGERR, "Invalid pageid=%s title=%s", id, title);
			continue;
		}

		CSym &rwsym = rwlist[found];
		printf("%d/%d %s %s    \r", i, list.GetSize(), id, title);

		//Throttle(barranquismoticks, 1000);
		CPage p(f, id, title);

		if (mode == -4)
		{
			int bqn = FALSE;
			int end, start = p.Section("Introduction", &end);
			for (int i = start; i < end && !bqn; ++i)
				bqn = IsSimilar(p.lines[i], "<b>Attraction");
			if (bqn)
			{
				// beta was already transferred
				sym.SetStr(ITEM_STARS, "Y");
				list.Save(bqnfile);
				continue;
			}
			continue;
		}

		{
			vars url = sym.GetStr(ITEM_KML);

			// html beta
			CString memory;
			vars name = sym.GetStr(ITEM_DESC);
			vars folderfile = url2file(name, CCS);
			if (!LoadCCS(folderfile + ".html", memory))
			{
				Log(LOGERR, "Can't load offline page %s", url);
				continue;
			}

			// delete sections
			//memory = ExtractString(memory, "<BODY", "", );
			vars legend;
			const char *leg = strstr(memory, " = ");
			if (leg != NULL)
				legend = ExtractStringDel(memory, CString(leg - 1, 5), "", "<b>", -1, -1);
			if (!legend.IsEmpty())
				legend = "\n\n<div style=\"font-size:x-small; background-color:#f0f0f0;\"><i>" + legend.replace("<br>", " ") + "</i></div>";
			vars background = ExtractStringCCS(memory, "Note:") + ExtractStringCCS(memory, "NOTE:");
			vars exit = ExtractStringCCS(memory, "Return:") + ExtractStringCCS(memory, "RETURN:");
			vars descent = ExtractStringCCS(memory, "Description of");
			vars approach = ExtractStringCCS(memory, "Approach:") + ExtractStringCCS(memory, "Aproach:");
			vars access = ExtractStringCCS(memory, "Access:") + ExtractStringCCS(memory, "Acess:");
			vars data = ExtractStringCCS(memory, "Description:");
			vars fotos;
			//data1 = "<div>"+data1.split("<b>").join("</div><div><b>")+"</div>";
			//descent = "<div>"+descent.split("<b>").join("</div><div><b>")+"</div>";


			// upload trip files
			{
				vara rfiles;
				CPage up(f, NULL, NULL, NULL);
				for (int j = 0; j < filelist.GetSize(); ++j)
					if (strstr(filelist[j].id, folderfile))
					{
						vars filename = filelist[j].id;
						// upload file
						vars rfile = GetFileNameExt(filename);
						if (!up.FileExists(rfile))
							if (!up.UploadFile(filename, rfile))
								Log(LOGERR, "Could not upload file %s", rfile);
						rfiles.push(rfile);
					}
				if (rfiles.length() > 0)
				{
					vars sep;
					for (int i = 1; i < rfiles.length(); ++i) sep += ";";
					fotos = "{{pic|:" + rfiles[0] + sep + "}}";
				}
			}

			vara files; // not used

			/*
			// update beta sites
			{
			int start, end;
			start = p.Section("Beta", &end);
			vars betasites = ExtractBetaBQN(sym.id, memory, vara("Mas información,Plus d'informations"), FALSE, "", "<tr>");
			Download_SaveImg(sym.id, betasites, urlimgs, titleimgs, TRUE);
			for (int u=0; u<urlimgs.length(); ++u)
				{
				vars link = urlstr(urlimgs[u]);
				if (IsBQN(link))
					continue;

				for (int l=start; l<end && !strstr(p.lines[l], link); ++l);
				if (l<end) // already listed
					continue;
				p.lines.InsertAt(end++, MkString("* %s",link));
				}
			}
			*/
			// update sym
			CSym chgsym;
			int code = GetCode(sym.id);
			CCS_DownloadPage(data, sym);
			if (CompareSym(sym, rwsym, chgsym, *codelist[code]))
			{
				chgsym.SetStr(ITEM_DESC, "");
				chgsym.SetStr(ITEM_REGION, "");
				chgsym.SetStr(ITEM_MATCH, "");
				int updates = UpdatePage(chgsym, code, p.lines, p.comment, title);
			}

			// update page
			int err = data.GetLength() < 20;
			err += !SetSectionContent(sym.id, p, "Introduction", nobr(data, TRUE), files);
			err += !SetSectionContent(sym.id, p, "Approach", nobr(access) + "\n\n" + nobr(approach), files);
			err += !SetSectionContent(sym.id, p, "Descent", descent.replace("<br>", "\n\n") + nobr(legend), files);
			err += !SetSectionContent(sym.id, p, "Exit", nobr(exit), files);
			//err += !SetSectionContent(sym.id, p, "Red tape"	, redtape, files); 
			err += !SetSectionContent(sym.id, p, "Trip", fotos, files);
			err += !SetSectionContent(sym.id, p, "Background", nobr(background), files);

			if (err > 0)
			{
				Log(LOGERR, "%d SECTION ERRORS DETECTED, skipping %s (%s)", err, title, sym.id);
				continue;
			}

		}


		// update
		int err = 0;
		vars basecomment = "Imported from AIC Infocanyon " + sym.id;
		Log(LOGINFO, "POST %d/%d %s %s [#%d]", i, list.GetSize(), basecomment, title, idnum);
		if (MODE > -1)
		{
			// do not update
			Log("\n" + p.lines.join("\n") + "\n");
			continue;
		}
		else
		{
			// update
			vars plines = p.lines.join("\n");
			p.comment.InsertAt(0, basecomment);
			if (!p.Update())
				continue;

			// double check
			CPage p2(f, id, title);
			vars p2lines = p2.lines.join("\n");
			if (plines != p2lines)
			{
				int n, nl, ol;
				vara newlines(plines, "\n");
				vara lines(p2lines, "\n");
				for (n = 0; n < newlines.length() && n < lines.length(); ++n)
					if (newlines[n] != lines[n])
						break;
				for (nl = newlines.length() - 1, ol = lines.length() - 1; nl > n && ol > n; --nl, --ol)
					if (newlines[nl] != lines[ol])
						break;
				Log(LOGERR, "INCONSISTENT UPDATE for %s! #newlines:%d =?= #oldlines:%d diff %d-%d", title, newlines.length(), n < lines.length(), n, nl);
				vars dump;
				for (int i = n; i <= nl || i <= ol; ++i)
				{
					dump += MkString("#%d : ", i);
					dump += (i <= nl) ? newlines[i] : "";
					dump += MkString(" =!= ");
					dump += (i <= ol) ? lines[i] : "";
				}
				Log(dump);
			}

			sym.SetStr(ITEM_STARS, "Y");
			sym.SetStr(ITEM_MATCH, RWID + MkString("%d", idnum) + RWLINK);
			list.Save(bqnfile);
		}
	}

	// update 
	return TRUE;
}


/*
api.php?action=login&lgname=user&lgpassword=password

// http://ropewiki.com/api.php?action=sfautoedit&form=Canyon&target=LucaTest1&query=Canyon[Slowest typical time]=9h%26Canyon[Fastest typical time]=5h

http://ropewiki.com/api.php?POST?action=edit&title=LucaTest1&summary=updated%20contents&text=article%20content

http://ropewiki.com/api.php?api.php?action=query&prop=revisions&rvprop=content&format=jsonfm&pageid=7912

7912
http://ropewiki.com/index.php?curid=7912&action=raw
		http://ropewiki.com/index.php?curid=7912&action=raw


		http://ropewiki.com/api.php?action=purge&titles=Waterholes_Canyon_(Lower)

		http://ropewiki.com/api.php?action=purge&titles=Glaucoma_Canyon&forcelinkupdate
*/


#if 0
// upload page
url = "http://ropewiki.com/api.php?POST?action=login&lgname=RW_ROBOT_USERNAME&lgpassword=RW_ROBOT_PASSWORD&format=xml";
if (f.Download(url, "login.htm")) {
	Log(LOGERR, "ERROR: can't download RWID:%d %.128s", idnum, url);
	continue;
}
vars token = ExtractString(f.memory, "token=");

char ctoken[512]; DWORD size = strlen(ctoken);
InternetGetCookie("http://ropewiki.com", "ropewikiToken", ctoken, &size);

/*
		url = "http://ropewiki.com/api.php?action=query&meta=tokens";
		if (f.Download(url, "token.htm")) {
			Log(LOGERR, "ERROR: can't download RWID:%d %.128s", idnum, url);
			continue;
			}
		token = ExtractString(f.memory, "token=");
*/
token = "04db5caf4d51196fa31cef8d58c52315+\\";
url = MkString("http://ropewiki.com/api.php?POST?action=edit&pageid=%d&token=%s", idnum, url_encode(token));
url += "&summary=updated%20contents&text=";
url += url_encode(lines.join("\n"));
if (f.Download(url, "post.htm")) {
	Log(LOGERR, "ERROR: can't download RWID:%d %.128s", idnum, url);
	continue;
}
CString str = f.memory;
#endif


int rwfstars(const char *line, CSymList &list)
{
	vara labels(line, "label=\"");
	vars id = getfulltext(labels[1]);
	vars stars = ExtractString(labels[2], "Has page rating", "<value>", "</value>");
	vars mul = ExtractString(labels[3], "Has page rating multiplier", "<value>", "</value>");
	if (id.IsEmpty()) {
		Log(LOGWARN, "Error empty ID from %.50s", line);
		return FALSE;
	}
	CSym sym(id, stars + "*" + mul);
	list.Add(sym);
	return TRUE;
}


int UploadStars(int code)
{
	const char *codestr = codelist[code]->code;
	const char *user = codelist[code]->staruser;
	if (!user) {
		Log(LOGERR, "Invalid user for code '%s'", codestr);
		return FALSE;
	}

	Log(LOGINFO, "Uploading %s stars as %s", codestr, user);
	// load beta list
	CSymList symlist, bslist, vlist;
	LoadBetaList(bslist, TRUE);
	bslist.Sort();
#ifdef DEBUG
	bslist.Save("bslist.csv");
#endif
	symlist.Load(filename(codestr));
	symlist.Sort();

	int i = 0, bi = 0;
	while (i < symlist.GetSize() && bi < bslist.GetSize()) {
		CSym &sym = symlist[i];
		CSym &bsym = bslist[bi];
		int cmp = strcmp(sym.id, bsym.id);
		if (cmp == 0) {
			// in bslist and symlist
			// bsym-->sym  sym-->bsym
			CString v = sym.GetStr(ITEM_STARS);
			//ASSERT(!strstr(sym.id, "Chute"));

			if (v[0] != 0 && v[0] != '0') {
				CSym vsym(bsym.data, v);
				double c = sym.GetNum(ITEM_CLASS);
				if (c >= 0) vsym.SetNum(1, c);
				vlist.Add(vsym);
			}
			++bi; continue;
		}
		if (cmp > 0) {
			// in bslist but not in symlist
			++bi; continue;
		}
		if (cmp < 0) {
			// in bslist but not in symlist
			++i; continue;
		}
	}
	// vlist-> title, stars:count [possible duplicates]
	vlist.Sort();
	for (int i = vlist.GetSize() - 1; i > 0; --i)
		if (vlist[i].id == vlist[i - 1].id) {
			BOOL swap = vlist[i].GetNum(1) < vlist[i - 1].GetNum(1) ||
				vlist[i].GetNum(0) < vlist[i - 1].GetNum(0);
			//ASSERT(!strstr(vlist[i].id, "Chute"));
			if (swap)
				vlist.Delete(i);
			else
				vlist.Delete(i - 1);
		}
	// vlist-> title, stars:count [highest class, highest vote per title, no duplicates]

	// query votes by user code
	CSymList rwlist;
	CString query = "[[Has page rating page::%2B]][[Has page rating user::" + url_encode(user) + "]]";
	vara prop("|Has page rating page|Has page rating|Has page rating multiplier", "|");
	GetASKList(rwlist, query + "|%3F" + prop.join("|%3F") + "|mainlabel=-", rwfstars);

	CSymList dellist;
	// compare vlist with rwlist -> list changes
	vlist.Sort();
	rwlist.Sort();
#ifdef DEBUG
	vlist.Save("vlist.csv");
	rwlist.Save("rwlist.csv");
#endif
	for (int i = vlist.GetSize() - 1, ri = rwlist.GetSize() - 1; ri >= 0 && i >= 0; ) {
		CSym &l = vlist[i];
		CSym &r = rwlist[ri];
		int cmp = strcmp(r.id, l.id);
		if (cmp == 0) {
			//ASSERT(!strstr(r.id, "Chute"));
			if (r.GetStr(0) == l.GetStr(0))
				vlist.Delete(i);
			else
				Log(LOGINFO, "Updating %s => %s", r.Line(), l.Line());
			--i, --ri;
			continue;
		}
		if (cmp > 0) {
			Log(LOGERR, "ERROR: This should never happen II ri=%d i=%d %s vs %s => Delete Vote for <", ri, i, r.Line(), l.Line());
			vara prop("|Has page rating page|Has page rating|Has page rating multiplier", "|");
			CSymList addlist, dellist;
			GetASKList(addlist, query + MkString("[[Has page rating page::%s]]", r.id) + "|%3F" + prop.join("|%3F"), rwftitleo);
			vars file = "delete.csv";
			dellist.Load(file);
			dellist.Add(addlist);
			dellist.Save(file);
			--ri;
			continue;
		}
		if (cmp < 0) {
			Log(LOGINFO, "Adding %s", l.Line());
			--i;
			continue;
		}
	}
	// list-> pageid, stars:count [no duplicates]

	// chglist = name, stars:votes
	if (vlist.GetSize() == 0) {
		Log(LOGINFO, "No star changes for %s", codestr);
		return FALSE;
	}

	// post new votes or changed votes
	// log in as RW_ROBOT_USERNAME [RW_ROBOT_PASSWORD]
	DownloadFile f;
	if (!Login(f))
		return FALSE;

	for (int i = 0; i < vlist.GetSize(); ++i)
	{
		printf("Posting %d/%d votes...\r", i, vlist.GetSize());
		CString pagename = vlist[i].id;
		pagename.Replace(" ", "_");
		pagename.Replace("%2C", ",");
		//pagename.Replace("+", "%2B");
		//pagename.Replace("&amp;", "&");
		CString stars = vlist[i].GetStr(0);
		vara starsuser(stars, "*");
		if (starsuser.length() < 2)
		{
			starsuser.SetSize(2);
			starsuser[1] = "1";
		}
		vars target = "Votes:" + url_encode(pagename) + "/" + user;
		vars url = RWBASE + "api.php?action=sfautoedit&form=Page_rating&target=" + target + "&query=Page_rating[Page]=" + url_encode(url_encode(pagename)) + "%26Page_rating[Rating]=" + starsuser[0] + "%26Page_rating[Multiplier]=" + starsuser[1] + "%26Page_rating[User]=" + url_encode(user);
		if (MODE >= 0)
			Log(LOGINFO, "Setting %.256s", url);
		if (MODE <= 0)
			if (f.Download(url))
			{
				Log(LOGERR, "ERROR: could not set url %.128s", url);
				continue;
			}
	}

	return TRUE;
}


int UploadStars(int mode, const char *codestr)
{
	for (int code = 0; codelist[code]->code != NULL; ++code)
	{
		if (codestr && strcmp(codelist[code]->code, codestr) != 0)
			continue;
		if (!codelist[code]->staruser)
			continue;
		UploadStars(code);
	}
	return TRUE;
}


int UploadCondition(CSym &sym, const char *title, const char *credit, int trackdate)
{
	vars condtxt = sym.GetStr(ITEM_CONDDATE);
	condtxt.Replace(FBCOMMA, ",");
	vara cond(condtxt, ";");
	if (cond.length() == 0)
		return FALSE;

	cond.SetSize(COND_MAX);
	double newdate = CDATA::GetDate(cond[COND_DATE]);
	if (newdate <= 0)
		return FALSE;
	if (newdate > RealCurrentDate + 1)
	{
		Log(LOGERR, "Future dates not allowed %s > %s", CDate(newdate), CDate(RealCurrentDate));
		return FALSE;
	}

	vars comments = "Updated latest condition to " + cond[COND_DATE];

	//printf("EDITING %d/%d %s %s....\r", title);
	DownloadFile f;
	vars ptitle = CONDITIONS + vars(title).replace(" ", "_") + "-" + vars(credit);
	if (trackdate)
		ptitle += "-" + cond[COND_DATE];
	CPage p(f, NULL, ptitle);

	BOOL isFacebook = strstr(credit, "@Facebook") != NULL;
	if (isFacebook)
	{
		comments = "Created new Facebook condition";
		if (cond[COND_TEXT].IsEmpty() || cond[COND_DATE].IsEmpty())
		{
			Log(LOGERR, "ERROR: Empty DATE/COMMENT for Facebook condition %s %s", cond[COND_DATE], title);
			return FALSE;
		}
		if (cond[COND_TEXT][0] != '\"' || ExtractString(cond[COND_TEXT], "[", "", "]").IsEmpty())
		{
			Log(LOGERR, "ERROR: Invalid COMMENT for Facebook condition %s %s : '%.500s'", cond[COND_DATE], title, cond[COND_TEXT]);
			return FALSE;
		}
		if (!p.Get("Date").IsEmpty())
		{
			int nonquestionable = p.Get("Questionable").IsEmpty();

			int chg = 0, cmt = 0;
			// UPDATE Conditions
			chg += p.Override("Quality condition", cond[COND_STARS]);
			chg += p.Override("Water condition", cond[COND_WATER]);
			chg += p.Override("Wetsuit condition", cond[COND_TEMP]);
			chg += p.Override("Difficulty condition", cond[COND_DIFF]);
			chg += p.Override("Team time", cond[COND_TIME]);
			chg += p.Override("Team size", cond[COND_PEOPLE]);
			chg += p.Override("Questionable", cond[COND_NOTSURE]);
			// UPDATE Comments
			vara fblist(p.Get("Comments"), FBLIST);
			if (fblist.indexOf(cond[COND_TEXT]) < 0)
			{
				BOOL joined = FALSE;
				for (int i = 1; i < fblist.length() && !joined; ++i)
					if (strstr(fblist[i], "[" + cond[COND_LINK]))
						joined = TRUE;
				if (!joined) // add line
				{
					fblist.push(cond[COND_TEXT]);
					++cmt;
				}
				p.Set("Comments", fblist.join(FBLIST), TRUE);
			}
			if (chg || cmt)
			{
				// skip the conditions that are no longer questionable
				if (MODE >= -1 && nonquestionable)
				{
					Log(LOGINFO, "Non questionable %s", ptitle);
					return FALSE;
				}
				// update
				p.comment.push(MkString("Updated Facebook condition (added %d comments, %d data change)", cmt, chg));
				if (MODE <= 0) p.Update();
				return TRUE;
			}
			return FALSE;
		}
	}

	// check date
	vars olddate = p.Get("Date");
	if (!olddate.IsEmpty())
		if (newdate <= CDATA::GetDate(olddate) && MODE >= -1)
			return FALSE;

	// set new condition
	const char *def[] = {
		"{{Condition",
		"}}",
		NULL };
	p.Set(def);

	p.Set("Location", title);
	p.Set("ReportedBy", credit);
	if (!cond[COND_USERLINK].IsEmpty())
		p.Set("ReportedByUrl", cond[COND_USERLINK]);
	p.Set("Date", cond[COND_DATE]);

	p.Override("Quality condition", cond[COND_STARS]);
	p.Override("Water condition", cond[COND_WATER]);
	p.Override("Wetsuit condition", cond[COND_TEMP]);
	p.Override("Difficulty condition", cond[COND_DIFF]);
	p.Override("Team time", cond[COND_TIME]);
	p.Override("Team size", cond[COND_PEOPLE]);
	p.Override("Questionable", cond[COND_NOTSURE]);

	if (!cond[COND_LINK].IsEmpty())
		p.Set("Url", cond[COND_LINK]);
	else
		p.Set("Url", cond[COND_LINK] = sym.id);
	if (!cond[COND_TEXT].IsEmpty())
		if (isFacebook)
			p.Set("Comments", MkString("extracts from Facebook posts, click link for full details (privacy restrictions may apply)", cond[COND_USERLINK], cond[COND_USER]) + FBLIST + cond[COND_TEXT]);
		else
			p.Set("Comments", cond[COND_TEXT]);
	else
		p.Set("Comments", MkString("Most recent conditions reported at %s", credit));
	p.comment.push(comments);
	if (MODE <= 0) p.Update();
	return TRUE;
}


int UploadConditions(int mode, const char *codestr)
{
	if (!LoadBetaList(titlebetalist, TRUE, TRUE))
		return FALSE;

	for (int code = 0; codelist[code]->code != NULL; ++code)
	{
		if (!codelist[code]->betac->cfunc)
			continue;
		if (codestr && strcmp(codelist[code]->code, codestr) != 0)
			continue;
		if (!codestr && !codelist[code]->name)
			continue;

		Code &c = *codelist[code];

		CSymList clist;
		if (MODE < -2 || MODE>0)
			clist.Load(filename(c.code));
		c.betac->cfunc(c.betac->ubase, clist);
		if (clist.GetSize() == 0)
			Log(LOGERR, "ERROR: NO CONDITIONS FOR %s (%s)!", c.code, c.name);
		for (int i = 0; i < clist.GetSize(); ++i)
		{
			printf("%s %d/%d     \r", c.code, i, clist.GetSize());
			if (IsSimilar(clist[i].id, RWTITLE))
			{
				// direct mapping
				UploadCondition(clist[i], clist[i].id.Mid(strlen(RWTITLE)), c.name);
				continue;
			}
			// possible multiple matches
			for (int b = 0; b < titlebetalist.GetSize(); ++b)
				if (clist[i].id == titlebetalist[b].id)
					UploadCondition(clist[i], titlebetalist[b].data, c.name);
		}
	}
	return TRUE;
}


int rwfpurgetitle(const char *line, CSymList &idlist)
{
	vara labels(line, "label=\"");
	vars name = getfulltext(labels[0]);
	//vars id = ExtractString(labels[1], "", "<value>", "</value>");
	CSym sym("", name);
	idlist.Add(sym);
	return TRUE;
}


int rwfpurgeid(const char *line, CSymList &idlist)
{
	vara labels(line, "label=\"");
	vars name = getfulltext(labels[0]);
	vars id = ExtractString(labels[1], "", "<value>", "</value>");
	//vars located = ExtractString(labels[1], "Located in region", "<value>", "</value>");
	if (id.IsEmpty()) {
		Log(LOGWARN, "Error empty ID from %.50s", line);
		return FALSE;
	}
	CSym sym(id, name);
	idlist.Add(sym);
	return TRUE;
}


int GetRWList(CSymList &idlist, const char *fileregion)
{
	if (!fileregion || *fileregion == 0)
		fileregion = "ALL";
	const char *ext = GetFileExt(fileregion);
	if (fileregion == "[")
	{
		GetASKList(idlist, vars(fileregion) + "|%3FHas pageid" + "|sort=Modification_date|order=asc", rwfpurgeid);
	}
	else if (!ext) {
		vars query = CString("[[Category:Canyons]]");
		if (!IsSimilar(fileregion, "ALL"))
		{
			// region or canyon 
			DownloadFile f;
			CPage p(f, NULL, fileregion);
			if (!p.Exists() || p.lines.length() < 2)
			{
				Log(LOGERR, "ERROR: Page '%s' does not exist or is empty!", fileregion);
				return FALSE;
			}
			vars subquery;
			int i;
			for (i = 0; i < p.lines.length() && !IsSimilar(p.lines[i], "{{"); ++i);
			if (IsSimilar(p.lines[i], "{{Region"))
				subquery = MkString("[[Located in region.Located in regions::%s]]", fileregion);
			if (IsSimilar(p.lines[i], "{{Canyon"))
				subquery = MkString("[[%s]]", fileregion);
			if (subquery.IsEmpty())
			{
				Log(LOGERR, "ERROR: Page '%s' is not region or canyon [%s]!", fileregion, p.lines[i]);
				return FALSE;
			}
			query += subquery;
		}
		GetASKList(idlist, query + "|%3FHas pageid" + "|sort=Modification_date|order=asc", rwfpurgeid);
	}
	else if (IsSimilar(ext, "csv"))
		idlist.Load(fileregion);
	else if (IsSimilar(ext, "log"))
	{
		CFILE fm;
		if (!fm.fopen(fileregion))
			Log(LOGERR, "could not read %s", fileregion);
		const char *line;
		while (line = fm.fgetstr())
		{
			if (strstr(line, " POST "))
			{
				CString id = ExtractString(line, "[#", "", "]");
				CString title = ExtractString(line, "Updated:", " ", "[#");
				if (!id.IsEmpty() && !title.IsEmpty())
				{
					idlist.Add(CSym(id, title.Trim()));
					continue;
				}
			}
			double num = ExtractNum(line, RWID, "", NULL);
			if (num > 0)
			{
				idlist.Add(CSym(RWID + CData(num)));
				continue;
			}
		}
	}
	for (int i = 0; i < idlist.GetSize(); ++i)
	{
		if (IsSimilar(idlist[i].id, "http"))
		{
			CString match = idlist[i].GetStr(ITEM_MATCH);
			const char *rwid = strstr(match, RWID);
			idlist[i].id = rwid ? CData(CDATA::GetNum(rwid + strlen(RWID))) : "";
		}
		idlist[i].id.Replace(RWLINK, "");
		idlist[i].id.Replace(RWID, "");
		idlist[i].data = idlist[i].GetStr(0);
		if (idlist[i].id.IsEmpty() || IsSimilar(idlist[i].id, "-----"))
			idlist.Delete(i), --i;
		//Log(LOGINFO, "#%s %s", idlist[i].id, idlist[i].GetStr(0));
	}
	CString file = filename("selection");
	idlist.Save(file);
	Log(LOGINFO, "%d pages (saved to %s)", idlist.GetSize(), file);
	for (int i = 5; i > 0; --i)
	{
		printf("Starting in %ds...   \r", i);
		Sleep(1000);
	}
	return idlist.GetSize();
}


int QueryBeta(int MODE, const char *query, const char *file)
{
	CSymList idlist;
	GetASKList(idlist, CString(query), rwftitle);
	Log(LOGINFO, "%d pages (saved to %s)", idlist.GetSize(), file);
	idlist.Save(file);
	return 0;
}


int PurgeBeta(int MODE, const char *fileregion)
{
	DownloadFile f;
	if (!Login(f))
		return FALSE;
	CSymList list;

	if (MODE == 0)
		GetASKList(list, "[[Category:Disambiguation pages]]", rwfpurgetitle);
	else if (MODE == 1)
		GetASKList(list, "[[Category:Regions]]", rwfpurgetitle);
	else
		GetRWList(list, fileregion);

	int n = list.GetSize();
	for (int i = 0; i < n; ++i)
	{
		printf("%d/%d %s:", i, n, list[i].data);
		if (MODE == -2)
			RefreshPage(f, list[i].id, list[i].GetStr(0));
		else
			PurgePage(f, list[i].id, list[i].GetStr(0));
	}
	return TRUE;
}


void RefreshBeta(int MODE, const char *fileregion)
{
	CSymList list;
	if (strchr(fileregion, '.'))
		GetRWList(list, fileregion);
	else
		list.Add(CSym("", fileregion));

	DownloadFile f;
	if (!Login(f))
		return;

	for (int i = 0; i < list.GetSize(); ++i)
	{
		vars id = list[i].id;
		vars title = list[i].GetStr(ITEM_DESC);

		Log(LOGINFO, "REFRESHING %d/%d %s [%s]", i, list.GetSize(), title, id);

		CPage p(f, id, title);
		vara lines = p.lines;
		if (lines.length() == 0)
		{
			Log(LOGERR, "ERROR: Empty page %s [%s]", title, id);
			continue;
		}
		p.lines.RemoveAll();
		p.comment = "Refreshing contents: reset";
		p.Update(TRUE);

		p.lines = lines;
		p.comment = "Refreshing contents: rebuild";
		p.Update(TRUE);

	}

}


int ReplaceBetaProp(int MODE, const char *prop, const char *fileregion)
{
	DownloadFile f;

	// get idlist
	CSymList idlist;
	GetRWList(idlist, fileregion);


	CString pname = GetToken(prop, 0, '=');
	CString pval = UTF8(GetToken(prop, 1, '='));
	Log(LOGINFO, "REPLACING %s=%s for %d", pname, pval, idlist.GetSize());

	if (!Login(f))
		return FALSE;

	if (MODE < 0)
		for (int i = 0; i < idlist.GetSize(); ++i)
		{
			CString &id = idlist[i].id.Trim("\" ");
			CString &title = idlist[i].data.Trim("\" ");
			// download page 
			printf("EDITING %d/%d %s %s....\r", i, idlist.GetSize(), id, title);
			CPage p(f, id, title);
#if 0
			vara prop("Fastest typical time|Slowest typical time|Fastest approach time|Slowest approach time|Fastest descent time|Slowest descent time|Fastest exit time|Slowest exit time", "|");
			for (int j = 0; j < prop.length(); ++j)
				UpdateProp(prop[j], "NULL", p.lines, TRUE);
			p.comment.push("Reset times");
#else
			UpdateProp(pname, pval, p.lines, TRUE);
			p.comment.push("Updated " + vars(prop));
#endif
			p.Update();
		}

	return TRUE;
}


int rwfkmlx(const char *line, CSymList &idlist)
{
	vara labels(line, "label=\"");
	//vars name = getfulltext(labels[0]);
	vars name = ExtractString(line, "fulltext=");
	name.Replace("&#039;", "'");
	CSym sym(name);
	vars kml = ExtractString(labels[1], "", "<value>", "</value>");
	vars kmlx = ExtractString(labels[2], "", "<value>", "</value>");
	sym.SetStr(ITEM_KML, kml);
	sym.SetStr(ITEM_MATCH, kmlx);
	idlist.Add(sym);
	return TRUE;
}


vars FixCheckList(const char *num, const char *str, const char *tstr)
{
	double bsnum = CDATA::GetNum(num);
	vara bslist(vars(str).Trim(" ;"), ";");
	vara bstlist(vars(tstr).Trim(" ;"), ";");
	if (bsnum > 0 && bslist.length() != bsnum)
		return MkString("%d!=%g (NUM)", bslist.length(), num); // + bslist.join(" ; ");
	if (bslist.length() < bstlist.length())
		return MkString("%d<%d (TLIST)", bslist.length(), bstlist.length());
	return "";
}


int FixCheckEmpty(const char *str1, const char *str2, int mode = 0)
{
	if (mode <= 0 && *str1 == 0 && *str2 != 0)
		return TRUE;
	if (mode >= 0 && *str1 != 0 && *str2 == 0)
		return TRUE;
	return FALSE;
}


void FixInconsistencies(void)
{
	CSymList badlist;
	LoadRWList();

	/*
		for (int i=0; i<rwlist.GetSize(); ++i)
		{
			CSym &sym = rwlist[i];

			int bad = FALSE;

			// bad geocode
			if (sym.GetNum(ITEM_LAT)==InvalidNUM && CDATA::GetNum(GetToken(sym.GetStr(ITEM_LAT),1,'@'))!=InvalidNUM)
				bad = TRUE;

			// bad info
			if (sym.GetStr(ITEM_INFO).IsEmpty())
				bad = TRUE;

			if (bad)
				badlist.Add(sym);
		}
	*/
	{
		CSymList list;
		enum {
			D_PAGEID = 0,
			D_KML, D_KMLX, D_KMLXLIST,
			D_COORDS, D_LAT, D_LNG, D_GEOCODE,
			D_SUMMARY, D_NAME, D_URL,
			D_REGMAJOR, D_REG, D_REGS, D_LOC,
			D_BANNER, D_BANNERFILE,
			D_SEASON, D_SEASONMONTHS,
			D_ACA, D_ACASUMMARY, D_ACARATING,
			D_INFO,
			D_BOOKLIST,
			D_BSLIST, D_BSTLIST, D_BSNUM,
			D_TRLIST, D_TRTLIST, D_TRNUM,
			D_COND, D_CONDINFO, D_CONDDATE,
			D_RAPINFO, D_RAPS,
			D_LONGINFO, D_LONG,
			D_HIKEINFO, D_HIKE,
			D_TIMEINFO, D_TIMEFAST, D_TIMESLOW,

			D_MODDATE,
			D_MAX
		};
		vars dstr =
			"|Has pageid"
			"|Has KML file|Has KMLX file|Has BetaSitesKML list"
			"|Has coordinates|Has latitude|Has longitude|Has geolocation"
			"|Has info summary|Has name|Has url"
			"|Has info major region|Has info region|Has info regions|Located in region"
			"|Has banner image|Has banner image file"
			"|Has best season|Has best month"
			"|Has ACA rating|Has summary|Has rating"
			"|Has info"
			"|Has book list"
			"|Has BetaSites list|Has BetaSites translist|Has BetaSites num"
			"|Has TripReports list||Has TripReports translist|Has TripReports num"
			"|Has condition|Has info condition|Has condition date"
			"|Has info rappels|Has number of rappels"
			"|Has info longest rappel|Has longest rappel"
			"|Has info length of hike|Has length of hike"
			"|Has info typical time|Has fastest typical time|Has slowest typical time"

			"|Modification date";

		vara dlist(dstr, "|");
		ASSERT(dstr.length() - 1 != D_MAX);
		GetASKList(list, "[[Category:Canyons]]" + dstr.replace("|", "|%3F") + "|sort=Modification_date|order=asc", rwflabels);

		for (int i = 0; i < list.GetSize(); ++i)
		{
			vara comment;
			vara data(list[i].data); data.SetSize(D_MAX);
			CSym sym(data[D_PAGEID], list[i].id);

			// KMLX check
			if (FixCheckEmpty(data[D_KML], data[D_KMLXLIST], -1))
				comment.push("Bad KML (KMLXLIST)");
			if (FixCheckEmpty(data[D_KML], data[D_KMLX]))
				if (!strstr(data[D_KML], "ropewiki.com"))
					comment.push("Bad KML vs KMLX");
			if (FixCheckEmpty(data[D_KMLX], data[D_KMLXLIST], 1))
				comment.push("Bad KMLX vs KMLXLIST");

			// BSList / TRList
			vars cmt;
			if (!(cmt = FixCheckList(data[D_BSNUM], data[D_BSLIST], data[D_BSTLIST])).IsEmpty())
				comment.push("Bad BSLIST " + cmt);
			if (!(cmt = FixCheckList(data[D_TRNUM], data[D_TRLIST], data[D_TRTLIST])).IsEmpty())
				comment.push("Bad TRLIST " + cmt);
			//if (FixCheckEmpty(data[D_BSLIST], data[D_KMLXLIST], -1) && FixCheckEmpty(data[D_TRLIST], data[D_KMLXLIST], -1))
			//	comment.push("Bad BSLIST (KMLXLIST)");
			if (FixCheckEmpty(data[D_BSLIST], data[D_BOOKLIST], -1))
				comment.push("Bad BSLIST (BOOKLIST)");

			// Coords / Geolocation
			if (FixCheckEmpty(data[D_COORDS], data[D_LAT]))
				comment.push("Bad Coords vs LAT");
			if (FixCheckEmpty(data[D_COORDS], data[D_LNG]))
				comment.push("Bad Coords vs LNG");
			if (FixCheckEmpty(data[D_LAT], data[D_LNG]))
				comment.push("Bad LAT vs LNG");
			double lat, lng;
			if (FixCheckEmpty(data[D_LAT], data[D_GEOCODE], -1))
				if (_GeoCache.Get(data[D_GEOCODE], lat, lng))
					comment.push("Bad Geocode LAT");

			// Summary
			if (FixCheckEmpty(data[D_ACA], vars(data[D_SUMMARY]).Trim(" ;")))
				comment.push("Bad ACA vs SUMMARY");
			if (FixCheckEmpty(data[D_ACA], data[D_ACASUMMARY], 1))
				comment.push("Bad ACA vs ACASUMMARY");
			if (FixCheckEmpty(data[D_ACA], data[D_ACARATING], 1))
				comment.push("Bad ACA vs ACARATING");

			// misc
			if (data[D_SUMMARY].IsEmpty())
				comment.push("Empty SUMMARY");
			if (data[D_NAME].IsEmpty())
				comment.push("Empty NAME");
			if (data[D_URL].IsEmpty())
				comment.push("Empty URL");
			if (data[D_PAGEID].IsEmpty())
				comment.push("Empty PAGEID");

			// region
			if (!data[D_LOC].IsEmpty())
				if (!strstr(data[D_REGMAJOR], data[D_REG]))
					comment.push("Bad REGMAJOR");
			if (FixCheckEmpty(data[D_LOC], data[D_REGMAJOR]))
				comment.push("Bad LOC vs REGMAJOR");
			if (FixCheckEmpty(data[D_LOC], data[D_REG]))
				comment.push("Bad LOC vs REG");
			if (FixCheckEmpty(data[D_LOC], data[D_REGS]))
				if (data[D_LOC] != "Mars")
					comment.push("Bad LOC vs REGS");

			if (FixCheckEmpty(data[D_BANNER], data[D_BANNERFILE]))
				comment.push("Bad BANNER vs BANNERFILE");
			if (FixCheckEmpty(data[D_SEASON], data[D_SEASONMONTHS]))
				comment.push("Bad SEASON vs SEASONMONTHS");
			if (FixCheckEmpty(data[D_INFO], data[D_BANNER], -1))
				if (!strstr(data[D_INFO].lower(), "unexplored"))
					comment.push("Bad INFO vs BANNER");
			if (FixCheckEmpty(data[D_INFO], data[D_SEASON], -1))
				if (!strstr(data[D_INFO].lower(), "unexplored"))
					comment.push("Bad INFO vs SEASON");

			if (FixCheckEmpty(data[D_COND], data[D_CONDINFO]))
				if (!strstr(data[D_CONDINFO].lower(), "unexplored"))
					comment.push("Bad COND vs CONDINFO");
			if (FixCheckEmpty(data[D_COND], data[D_CONDDATE]))
				comment.push("Bad COND vs CONDDATE");

			if (data[D_RAPS] == "0") data[D_RAPS] = "";
			if (data[D_LONG] == "0") data[D_LONG] = "";
			if (FixCheckEmpty(data[D_RAPINFO], data[D_RAPS], -1))
				comment.push("Bad RAPINFO");
			if (FixCheckEmpty(data[D_LONGINFO], data[D_LONG], -1))
				comment.push("Bad LONGINFO");
			if (FixCheckEmpty(data[D_HIKEINFO], data[D_HIKE], -1))
				comment.push("Bad HIKEINFO");
			if (FixCheckEmpty(data[D_TIMEINFO], data[D_TIMEFAST] + data[D_TIMESLOW], -1))
				comment.push("Bad TIMEINFO");
			if (FixCheckEmpty(data[D_ACASUMMARY], data[D_RAPS] + data[D_LONG] + data[D_HIKE] + data[D_TIMEFAST] + data[D_TIMESLOW], -1))
				comment.push("Bad SUMMARYINFO");

			// add bad list
			if (comment.length() > 0)
			{
				sym.SetStr(ITEM_MATCH, comment.join(";"));
				Log(LOGERR, "SYMERROR: %s [%s] : %s", sym.GetStr(ITEM_DESC), sym.id, sym.GetStr(ITEM_MATCH));
				badlist.Add(sym);
			}
		}
	}

	if (badlist.GetSize() > 0)
	{
		Log(LOGINFO, "Fixing inconsistencies in %d pages...", badlist.GetSize());
		vars file = filename(CHGFILE);
		badlist.Save(file);
		RefreshBeta(-1, file);
	}
}


int IsPicX(const char *str)
{
	vars lstr = str;
	lstr.MakeLower();
	return strstr(lstr, "topo") || strstr(lstr, "sketch") || strstr(lstr, "map") || strstr(lstr, "profile");
}


int FixBetaPage(CPage &p, int MODE)
{
	//vars id = p.Id();
	vars title = p.Title();

	vara &comment = p.comment;
	vara &lines = p.lines;

	const char *sections[] = {
	"==Introduction==",
	"==Approach==",
	"==Descent==",
	"==Exit==",
	"==Red tape==",
	"==Beta sites==",
	"==Trip reports and media==",
	"==Background==",
	NULL };

	static vara sectionscmp;
	// fix section titles
	if (sectionscmp.length() == 0)
	{
		for (int j = 0; sections[j] != NULL; ++j)
			sectionscmp.push(vars(sections[j]).lower().replace(" ", ""));
	}


	int cleanup = 0, moved = 0;
	if (lines.length() > 0) {
		// ================ FIX DUPLICATED SECTIONS
		for (int i = 0; i < lines.length(); ++i)
			if (lines[i][0] == '=')
			{
				if (lines[i].Replace("== ", "==") > 0 || lines[i].Replace(" ==", "==") > 0)
					++moved;

				int n = 0, next = -1;
				vars section = CString(lines[i]).Trim(" =");
				for (n = i + 1; n < lines.length() && next < 0; ++n)
					if (lines[n][0] == '=')
					{
						vars nsection = CString(lines[n]).Trim(" =");
						if (nsection == section)
							next = n;
					}

				if (next >= 0)
				{
					// move all to next section
					lines.RemoveAt(i), ++moved;
					while (i < lines.length() && lines[i][0] != '=')
					{
						lines.InsertAt(next, lines[i]);
						lines.RemoveAt(i);
						++moved;
					}
				}
			}
		if (moved) comment.push("Cleaned up duplicated sections");

		// ================ FIX ALSO KNOWN AS => AKA 

		vara akalist;
		const char *ALSOKNOWNAS = "Also known as";
		const char *ALSOKNOWNAS2 = "also known as";
		for (int i = 0; i < lines.length(); ++i) {
			int aka = 0;
			if (IsSimilar(lines[i], ALSOKNOWNAS) || (aka = lines[i].indexOf(ALSOKNOWNAS)) >= 0 || (aka = lines[i].indexOf(ALSOKNOWNAS2)) >= 0)
			{
				CString prefix, postfix, fix;
				int start = aka + strlen(ALSOKNOWNAS);
				int end = lines[i].Find(".", start);
				if (end < 0) end = lines[i].GetLength();
				fix = lines[i].Mid(start, end - start).Trim(" \t\r\n:.");
				if (aka > 0)
					prefix = lines[i].Mid(0, aka).Trim(" \t\r\n:.*");
				if (end > 0)
					postfix = lines[i].Mid(end).Trim(" \t\r\n:.");
				if (fix.IsEmpty())
				{
					Log(LOGERR, "Empty aka from %s", lines[i]);
					continue;
				}
				else
				{
					if (!prefix.IsEmpty() || strstr(fix, " this ") || strstr(fix, " is ")) {
						Log(LOGERR, "ERROR: Manual AKA cleanup required for %s (%s)", title, fix);
						continue;
					}
					fix.Replace(" and ", ",");
					fix.Replace(" or ", ",");
					fix.Replace(",", ";");

					vara naka(fix, ";");
					akalist.Append(naka);
				}
				vara pp;
				if (!prefix.IsEmpty())
					pp.push(prefix);
				if (!postfix.IsEmpty())
					pp.push(postfix);
				if (pp.length() == 0)
					lines.RemoveAt(i--);
				else
				{
					lines[i] = pp.join(". ");
					Log(LOGWARN, "CHECK W/O AKA LINE %s: '%s'", title, lines[i]);
				}
			}
		}
		if (akalist.length() > 0)
		{
			UpdateProp("AKA", akalist.join(";"), lines);
			comment.push(MkString("Added AKA '%s'", akalist.join(";")));
		}

		vars oldaka = GetProp("AKA", lines);
		vars newaka = UpdateAKA(oldaka, "");
		if (newaka != oldaka)
		{
			UpdateProp("AKA", newaka, lines, TRUE);
			comment.push(MkString("Fixed AKA"));
		}

		vara duplink;
		vars section;
		for (int i = 0; i < lines.length(); ++i)
		{
			if (strncmp(lines[i], "==", 2) == 0)
				section = lines[i];

			// ================ MANUAL FIXES

			const char *v = NULL;

			// fix {{pic|size=X
			//if (IsSimilar(section,"==Descent") || IsSimilar(section,"==Approach"))
			{
				vars &line = lines[i];
				int s = line.Find("{{pic");
				int e = s >= 0 ? line.Find("}}", s) : -1;
				if (s >= 0 && e >= 0)
				{
					vars str = line.Mid(s, e - s);
					if (!strstr(str, "|size=X|"))
						if (IsPicX(str))
						{
							// make size X
							vars nstr;
							vars size = ExtractString(str, "size=", "", "|");
							if (size.IsEmpty())
								line.Replace("{{pic|", "{{pic|size=X|");
							else
								line.Replace("size=" + size, "size=X");

							Log(LOGWARN, "Fixed {{pic|size=X}} '%s'->'%s' in %s", str, nstr, title);
							vars c = "Fixed {{pic}} to use {{pic|size=X}}";
							if (comment.indexOf(c) < 0)
								comment.push(c);
						}
				}
			}
			if (IsSimilar(section, "==Descent") || IsSimilar(section, "==Approach"))
			{
				int media = TRUE;
				int s = lines[i].Find("[[Media:");
				int e = s >= 0 ? lines[i].Find("]]", s) : -1;
				if (s < 0 || e < 0)
				{
					s = lines[i].Find("[[File:");
					e = s >= 0 ? lines[i].Find("]]", s) : -1;
					media = FALSE;
				}

				if (s >= 0 && e >= 0)
				{
					e += 2;
					vars str = lines[i].Mid(s, e - s);
					if (media || IsPicX(str))
					{
						vara a(strstr(str, ":"), "|");
						if (a.length() > 0)
						{
							vars file = a[0];
							vars desc = a.last();

							vars nstr = "{{pic|size=X|" + file + " ~ " + desc + "}}";
							lines[i].Replace(str, nstr);
							Log(LOGWARN, "Fixed {{pic|size=X}} '%s'->'%s' in %s", str, nstr, title);
							vars c = MkString("Fixed %s to use {{pic|size=X}}", media ? "[[Media:]]" : "[[File:]]");
							if (comment.indexOf(c) < 0)
								comment.push(c);
						}
					}
				}
			}

			// fix Balearik 2016
			if (lines[i].Replace("Balearik 2016", "Balearik Canyoning Team") || lines[i].Replace("Balearik2016", "Balearik Canyoning Team"))
			{
				Log(LOGINFO, "Cleaned Balearik 2016 credit");
				comment.push("Cleaned Balearik 2016 credit");
			}

			// fix AKA
			if (IsSimilar(lines[i], v = "|AKA="))
			{
				vars newline = v + UpdateAKA("", lines[i].Mid(5));
				if (lines[i] != newline) {
					Log(LOGINFO, "Cleaned AKA for %s: %s => %s", title, lines[i], newline);
					comment.push("Cleaned AKA");
					lines[i] = newline;
				}
			}

			// delete bad shuttle
			if (IsSimilar(lines[i], v = "|Shuttle=")) {
				CString val = GetToken(lines[i], 1, '=').Trim();
				if (!val.IsEmpty())
					if (CDATA::GetNum(val) < 0 && !IsSimilar(val, "No") && !IsSimilar(val, "Yes"))
					{
						Log(LOGWARN, "WARNING: Deleted bad Shuttle for %s: '%s'", title, val);
						lines[i] = v;
						comment.push("Fixed bad Shuttle");
					}
			}

			// delete bad permits
			if (IsSimilar(lines[i], v = "|Permits=")) {
				CString val = GetToken(lines[i], 1, '=').Trim();
				if (!val.IsEmpty())
				{
					const char *valid[] = { "No", "Yes", "Restricted", "Closed", NULL };
					if (!IsMatch(val, valid))
					{
						Log(LOGWARN, "WARNING: Deleted bad Permits for %s: '%s'", title, val);
						lines[i] = v;
						comment.push("Fixed bad Permits");
					}
				}
			}

			// delete bad season
			if (IsSimilar(lines[i], v = "|Best season=")) {
				vars val = lines[i].Mid(strlen(v)).Trim(), oval = val;
				IsSeasonValid(GetToken(val, 0, "!").Trim(), &val);
				if (SeasonCompare(val) != SeasonCompare(oval))
				{
					Log(LOGWARN, "WARNING: Cleaned bad Best Season for %s: '%s' => '%s'", title, oval, val);
					lines[i] = v + val;
					comment.push("Cleaned Best Season");
					if (InvalidUTF(val))
					{
						Log(LOGWARN, "WARNING: Deleted bad Best Season for %s: ='%s'", title, val);
						lines[i] = v;
						comment.push("Cleaned Best Season");
					}
				}
			}
			// delete manual disambiguation
			if (strstr(lines[i], "other features with similar name")) {
				CString t1 = GetToken(title, 0, '(').Trim();
				CString t2 = GetToken(GetToken(lines[i], 2, '['), 0, '(').Trim(" []");
				if (t1 == t2)
					lines.splice(i, 1), comment.push("Deleted redundant disambiguation line");
				else
					Log(LOGWARN, "WARNING: No auto-disambiguation '%s'!='%s' : %s -> %s", t1, t2, title, lines[i]);
			}
			// Ouray book
			if (strstr(lines[i], "?product=Ouray"))
				Log(LOGINFO, "%s is referencing product=Ouray", title);
			// replace < >
			if (strstr(lines[i], "&lt;")) {
				for (int j = i; j < lines.length(); ++j)
					lines[j].Replace("&lt;", "<");
				comment.push("Replaced &lt; with <");
			}
			if (strstr(lines[i], "&gt;")) {
				for (int j = i; j < lines.length(); ++j)
					lines[j].Replace("&gt;", ">");
				comment.push("Replaced &gt; with >");
			}
			if (strstr(lines[i], "&quot;")) {
				for (int j = i; j < lines.length(); ++j)
					lines[j].Replace("&quot;", "\"");
				comment.push("Replaced &quot; with \"");
			}
			vars titlelink = title;
			titlelink.Replace(",", " - ");
			titlelink.Replace(" ", "_");
			// replace AJ RoadTrip -> RoadTripRyan
			if (strstr(lines[i], "ajroadtrips.com"))
				lines[i] = lines[i].replace("ajroadtrips.com", "roadtripryan.com").replace("AJ Road Trips", "Road Trip Ryan"), comment.push("Updated old AJ Road Trips link");
			// remove grand canyoneering links
			if (strstr(lines[i], "http://www.toddshikingguide.com/GrandCanyoneering/index.htm")) {
				lines.splice(i, 1), --i, comment.push("Deleted old Grand Canyoneering link");
				if (lines[i].IsEmpty() && lines[i + 1].IsEmpty())
					lines.splice(i, 1);
			}
			// remove grand canyoneering links
			if (strstr(lines[i], "http://www.toddshikingguide.com/AZTechnicalCanyoneering/index.htm") || strstr(lines[i], "www.amazon.com/Arizona-Technical-Canyoneering-Todd-Martin")) {
				lines.splice(i, 1), --i, comment.push("Deleted old Arizona Technical Canyoneering link");
				if (lines[i].IsEmpty() && lines[i + 1].IsEmpty())
					lines.splice(i, 1);
			}
			//remove bad links
			const char *badlink = "http://ropewiki.com/User:Grand_Canyoneering_Book?id=";
			if (strstr(lines[i], badlink) && !strchr(lines[i].Mid(strlen(badlink)), '_')) {
				lines.splice(i, 1), --i, comment.push("Deleted bad link");
				if (lines[i].IsEmpty() && lines[i + 1].IsEmpty())
					lines.splice(i, 1);
			}
			// fix AIC links
			vars badaiclink = "http://catastoforre.aic-canyoning.it/index/forra/reg/";
			if (strstr(lines[i], badaiclink))
			{
				while (!ExtractStringDel(lines[i], "reg/", "", "/").IsEmpty());
				comment.push("Fixed bad link");
			}

			/*
			if (IsSimilar(lines[i], "<b>") && IsSimilar(lines[i].Right(4), "<br>"))
				{
				lines[i] = "<div>"+lines[i].Mid(0, lines[i].GetLength()-4)+"</div>";
				const char *comm = "fixed bold list";
				if (comment.indexOf(comm)<0)
					comment.push(comm);
				}
			*/

			if (IsSimilar(lines[i], UTF8("Toponímia:")) || IsSimilar(lines[i], UTF8("Toponymie:")))
			{
				lines[i] = "<div><b>" + lines[i].replace(":", ":</b>").replace("<div>", "</div>\n<div>");
				const char *comm = "fixed bold list2";
				if (comment.indexOf(comm) < 0)
					comment.push(comm);
			}
			if (lines[i] == "<div><b>Especies amenazadas:</b> En todos los habitats viven animales y plantas que merecen nuestro respeto</div>")
			{
				lines.RemoveAt(i--);
				comment.push("removed 'Especies amenazadas'");
				continue;
			}
			if (IsSimilar(lines[i], "<div><b>Combinaci"))
			{
				vars line = lines[i];
				vars comp = stripAccentsL(GetToken(stripHTML(lines[i]), 1, ':')).Trim(" .");
				if (comp == "no" || comp == "sin combinacion" || comp == "no necesaria" || comp == "no necesario")
				{
					lines.RemoveAt(i--);
					comment.push("fixed  'Combinacion vehiculos'");
					continue;
				}
				if (!IsSimilar(lines[i - 1], "==Approach"))
				{
					lines.RemoveAt(i--);
					int e;
					for (e = i; e < lines.length() && !IsSimilar(lines[e], "==Approach"); ++e);
					if (e == lines.length())
						Log(LOGERR, "Inconsisten Combinac for %s", title);
					else
						lines.InsertAt(e + 1, line + "\n");
					comment.push("fixed  'Combinacion vehiculos'");
					continue;
				}
			}

			if (strncmp(lines[i], "==", 2) == 0)
			{
				vars line = lines[i].lower().replace(" ", "");
				int s = sectionscmp.indexOf(line);
				if (s >= 0 && lines[i] != sections[s])
				{
					lines[i] = sections[s];
					const char *comm = "fixed malformed sections";
					if (comment.indexOf(comm) < 0)
						comment.push(comm);
				}
			}

			// las vegas book
			if (strstr(lines[i], "User:Las Vegas Slots Book") || strstr(lines[i], "User:Las_Vegas_Slots_Book "))
				lines[i] = MkString("* [http://ropewiki.com/User:Las_Vegas_Slots_Book?id=%s Las Vegas Slots Book by Rick Ianiello]", titlelink), comment.push("Updated old Las Vegas Slots Book link");
			// las vegas book
			if (strstr(lines[i], "User:Moab Canyoneering Book"))
				lines[i] = MkString("* [http://ropewiki.com/User:Moab_Canyoneering_Book?id=%s Moab Canyoneering Book by Derek A. Wolfe]", titlelink), comment.push("Updated old Moab Canyoneering Book link");
			// super amazing map
			if (strstr(lines[i], "http://www.bogley.com/forum/showthread.php?62130-The-Super-Amazing-Canyoneering-Map "))
				lines[i].Replace("http://www.bogley.com/forum/showthread.php?62130-The-Super-Amazing-Canyoneering-Map ", MkString("http://ropewiki.com/User:Super_Amazing_Map?id=%s ", titlelink)), comment.push("Updated old Super Amazing Map link");

			// brennen links
			if (lines[i].Replace("authors.library.caltech.edu/25057/2/advents", "brennen.caltech.edu/advents"))
				comment.push("Updated old Brennen's link");
			if (lines[i].Replace("authors.library.caltech.edu/25058/2/swhikes", "brennen.caltech.edu/swhikes"))
				comment.push("Updated old Brennen's link");
			//if (lines[i].Replace("dankat.com/", "brennen.caltech.edu/"))
			//	comment.push("Updated old Brennen's link");

			// canyoneeringusa links
			if (lines[i].Replace("canyoneeringcentral.com", "canyoneeringusa.com"))
				comment.push("Updated old CanyoneeringUSA's link");

			// region Marble Canyon
			if (IsSimilar(lines[i], "|Region=Marble Canyon"))
				lines[i] = "|Region=Grand Canyon", comment.push("Renamed Region Marble Canyon to Grand Canyon");

			if (IsSimilar(lines[i], "|Location type="))
				if (CDATA::GetNum(GetToken(lines[i], 1, '=')) != InvalidNUM)
					lines[i] = "|Location type=", comment.push("Fixed bad Location Type");

			// |Caving length= |Caving depth=
			if (IsSimilar(lines[i], "|Caving depth="))
				lines[i].Replace("Caving depth", "Depth"), comment.push("Fixed bad depth");
			if (IsSimilar(lines[i], "|Caving length="))
				lines[i].Replace("Caving length", "Length"), comment.push("Fixed bad length");

			// ================ BETASITE CLEANUP
			// check if should be bulleted list
			if (IsSimilar(section, "==Beta") || IsSimilar(section, "==Trip"))
			{
				lines[i].Trim();

				vars pre, post;
				if (lines[i][0] != '*' && Code::IsBulletLine(lines[i], pre, post) > 0)
					if (pre.IsEmpty() && post.IsEmpty())
					{
						// make bullet line
						lines[i] = "*" + lines[i];
						// delete empty lines
						while (lines[i - 1].trim().IsEmpty())
							lines.RemoveAt(--i);
						++cleanup;
					}

				// Bogley Trip Report
				if (stricmp(lines[i], "Bogley Trip Report") == 0) {
					lines.RemoveAt(i--), ++cleanup;
					continue;
				}

				// no empty lines
				if (lines[i].IsEmpty() && (lines[i - 1][0] == '*' || lines[i + 1][0] == '*') && lines[i + 1][0] != '=') {
					lines.RemoveAt(i--), ++cleanup;
					continue;
				}
			}

			const char *httplink = strstr(lines[i], "http");
			// if link present
			if (httplink)
				if (IsSimilar(section, "==Beta") || IsSimilar(section, "==Trip"))
				{
					int code = GetCode(httplink);
					if (code <= 0) continue;
					Code &cod = *codelist[code];

					// check if bulleted list
					vars pre, post;
					if (cod.IsBulletLine(lines[i], pre, post) <= 0)
					{
						Log(LOGERR, "WARNING: found non-bullet link at %s : %.100s", title, lines[i]);
						continue;
					}

					// found code! now search for id
					int found = cod.FindLink(httplink);
					if (found < 0) {
						BSLinkInvalid(title, httplink);
						if (MODE <= -2)
							lines.RemoveAt(i--), ++cleanup;
						continue; // invalid
					}

					// update BS link
					if (duplink.indexOf(cod.list[found].id) >= 0) {
						if (!pre.IsEmpty() || !post.IsEmpty())
						{
							Log(LOGERR, "CANNOT delete buplicate Beta Site link %s for %s (%.100s)", cod.list[found].id, title, lines[i]);
							continue;
						}
						Log(LOGERR, "Duplicate Beta Site link %s for %s (deleted)", cod.list[found].id, title);
						lines.RemoveAt(i--);
						++cleanup;
						continue;
					}
					duplink.push(cod.list[found].id);

					vars newline = cod.BetaLink(cod.list[found], pre, post);
					if (newline == lines[i] && IsSimilar(section, "==Trip") == cod.IsTripReport())
						continue; // no change

					// update BS link					
					Log(LOGINFO, "Cleaned up BSLink for %s: '%s' => '%s'", title, lines[i], newline);
					lines[i] = newline;

					// relocate
					if (IsSimilar(section, "==Trip") != cod.IsTripReport())
					{
						lines.RemoveAt(i--);
						int end, start = FindSection(lines, cod.IsTripReport() ? "==Trip" : "==Beta", &end);
						if (start > 0)
							lines.InsertAt(end >= 0 ? end : start, newline);
						else
							lines.InsertAt(++i, newline);
					}

					++cleanup;
				}
		}

	}

	if (ReplaceLinks(lines, 0, CSym(title)))
		++cleanup;

	if (cleanup)
		comment.push("Cleaned up Beta Site Links");

	return comment.length();
}


int FixBeta(int MODE, const char *fileregion)
{
	if (!fileregion || *fileregion == 0)
	{
		Log(LOGINFO, "FIXBETA =================== Fixing Inconsistencies...");
		FixInconsistencies();
		//Log(LOGINFO, "FIXBETA =================== Fixing disambiguations...");
		//DisambiguateBeta(1);
		Log(LOGINFO, "FIXBETA =================== Purging regions...");
		PurgeBeta(1, NULL);
	}

	Log(LOGINFO, "FIXBETA =================== Fixing Pages...");
	// query = [[Category:Canyons]][[Located in region.Located in regions::X]]
	DownloadFile f;


	// get idlist
	CSymList idlist;
	GetRWList(idlist, fileregion);

	/*
	CString pname = GetToken(prop, 0, '=');
	CString pval = UTF8(GetToken(prop, 1, '='));
	Log(LOGINFO, "REPLACING %s=%s for %d", pname, pval, idlist.GetSize());
	*/

	if (!Login(f))
		return FALSE;

	for (int i = 0; i < idlist.GetSize(); ++i)
	{
		CString &id = idlist[i].id;
		CString &title = idlist[i].data;
		CPage p(f, id, title);

		printf("%d/%d %s %s    \r", i, idlist.GetSize(), id, title);

		if (!FixBetaPage(p, MODE))
			continue;

		if (MODE < 1)
			p.Update();
	}

	return TRUE;
}


int UndoBeta(int MODE, const char *fileregion)
{
	// query = [[Category:Canyons]][[Located in region.Located in regions::X]]
	DownloadFile f;


	// get idlist
	CSymList idlist;
	GetRWList(idlist, fileregion);

	/*
	CString pname = GetToken(prop, 0, '=');
	CString pval = UTF8(GetToken(prop, 1, '='));
	Log(LOGINFO, "REPLACING %s=%s for %d", pname, pval, idlist.GetSize());
	*/

	if (!Login(f))
		return FALSE;

	if (MODE < 0)
		for (int i = 0; i < idlist.GetSize(); ++i)
		{
			CString &id = idlist[i].id;
			CString &title = idlist[i].data;

			// get oldid
			CString oldidurl = MkString("http://ropewiki.com/api.php?action=query&format=xml&prop=revisions&pageids=%s&rvlimit=1&rvprop=ids|timestamp|user|comment", id);
			if (f.Download(oldidurl)) {
				Log(LOGERR, "ERROR: can't get revision for %s %.128s, retrying", id, oldidurl);
				continue;
			}
			vars user = ExtractString(f.memory, "user=");
			vars oldid = ExtractString(f.memory, "parentid=");
			vars oldcomment = ExtractString(f.memory, "comment=");
			if ((user != RW_ROBOT_USERNAME && user != "Barranquismo.net") || oldid.IsEmpty() || IsSimilar(oldcomment, "Undo")) {
				Log(LOGERR, "ERROR: mismatched old revision for %s user=%s oldid=%s comment=%s from %.128s, retrying", title, user, oldid, oldcomment, oldidurl);
				continue;
			}

			CPage p(f, id, title, oldid);
			p.comment.push("Undo of last update");
			// download page 
			printf("EDITING %d/%d %s %s....\r", i, idlist.GetSize(), id, title);
			Log(LOGINFO, "UNDOING #%s %s : %s (%s)", id, title, p.comment.join(";"), oldcomment);
			//UpdateProp(pname, pval, lines, TRUE);
			p.Update(TRUE);
		}

	return TRUE;
}


CString KMLFolderMerge(const char *name, const char *memory, const char *id)
{
	// open folder
	int start = xmlid(memory, "<kml", TRUE);
	int end = xmlid(memory, "</kml");
	if (start >= end)
	{
		Log(LOGERR, "ERROR: no good kml %.128s", name);
		return MkString("<Folder>\n<name>%s</name>\n", name) + CString("</Folder>\n");
	}

	// output file
	//data->write(list[0]);
	CString kml(memory + start, end - start);
	if (id && id[0] != 0)
	{
		CString ids(id);
		kml.Replace("<name>", "<name>" + ids);
		kml.Replace("<NAME>", "<NAME>" + ids);
		kml.Replace("<Name>", "<Name>" + ids);
	}
	if (true)
	{
		CString newicon = "http://maps.google.com/mapfiles/kml/shapes/open-diamond.png";
		kml.Replace("http://caltopo.com/resource/imagery/icons/circle/FF0000.png", newicon);
		kml.Replace("http://caltopo.com/resource/imagery/icons/circle/000000.png", newicon);
		kml.Replace("http://caltopo.com/static/images/icons/c:ring,FF0000.png", newicon);
		kml.Replace("http://caltopo.com/static/images/icons/c:ring,000000.png", newicon);
	}
	return MkString("<Folder>\n<name>%s</name>\n", name) + kml + CString("</Folder>\n");
}


int KMLMerge(const char *url, inetdata *data)
{
	int size = 0, num = 0;
	DownloadFile f;
	if (f.Download(url))
	{
		Log(LOGERR, "ERROR: can't download main kml %.128s", url);
		return FALSE;
	}
	size += f.size;

	// split
	CStringArray list;
	split(list, f.memory, f.size, "<NetworkLink");

	// flush

	data->write(list[0]);

	for (int i = 1; i < list.GetSize(); ++i)
	{
		CStringArray line;
		split(line, list[i], list[i].GetLength(), "</NetworkLink>");

		CString name = xmlval(line[0], "<name", "</name").Trim();
		CString url = xmlval(line[0], "<href", "</href").Trim();

		// download file
		//for (int u=0; u<urls.length(); ++u) {
#ifdef DEBUG
		url.Replace(SERVER, "localhost");
#endif
		if (f.Download(url))
		{
			Log(LOGERR, "ERROR: can't download kml %.128s (%.128s)", name, url);
			return FALSE;
		}

		size += f.size;
		CString id;
		if (name[0] == '#')
		{
			++num;
			int i = 1;
			while (i < name.GetLength() && isdigit(name[i]))
				++i;
			id = CString(name, i + 1);
			// LocationID
			if (strlen(id) == 2)
			{
				name = name.Mid(2);
				id.Insert(1, name);
				id = id.Mid(1);
			}
		}
		data->write(KMLFolderMerge(name, f.memory, id));
		// close folder
		if (line.GetSize() > 1)
			data->write(line[1]);
	}
	//}

	Log(LOGINFO, "merged %d kml #%d %dKb", list.GetSize() - 1, num, size / 1024);
	return TRUE;
}


int RunExtract(DownloadKMLFunc *f, const char *base, const char *url, inetdata *out, int fx)
{
	if (!f) return FALSE;
	// find last http
	int len = strlen(url) - 1;
	while (len > 0 && !IsSimilar(url + len, "http"))
		--len;
	if (len < 0) return FALSE;
	url += len;

#ifndef DEBUG
	__try {
#endif

		// try
		return f(base, url, out, fx);

#ifndef DEBUG
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		Log(LOGALERT, "KMLExtract CRASHED!!! %.200s", url);
	}
#endif
	return FALSE;
}


int KMLAutoExtract(const char *_id, inetdata *out, int fx)
{
	vars id(_id);
	id.Trim();
	if (id.IsEmpty())
	{
		Log(LOGERR, "Empty KML Auto extraction?!?");
		return FALSE;
	}

	//auto extraction

	// {{#ask:[[Category:Canyons]][[Angel_Slot]]|?Has BetaSites list|headers=hide|mainlabel=-|format=CSV}}
	// http://ropewiki.com/Special:Ask/-5B-5BCategory:Canyons-5D-5D-5B-5BAngel_Slot-5D-5D/-3FHas-20BetaSites-20list/format%3DCSV/headers%3Dhide/mainlabel%3D-2D/offset%3D0
	DownloadFile f;
	const char *trim = "\" \t\n\r";

	CString url = "http://ropewiki.com/Special:Ask/-5B-5BCategory:Canyons-5D-5D-5B-5B" + CString(id) + "-5D-5D/-3FHas-20KML-20file/-3FHas-20BetaSites-20list/-3FHas-20TripReports-20list/format%3DCSV/headers%3Dhide/mainlabel%3D-2D/offset%3D0";
	if (f.Download(url))
		Log(LOGERR, "ERROR: can't download betasites for %s url=%.128s", id, url);

	vara bslist(vars(f.memory).Trim(trim), ",");
	if (bslist.length() == 0)
		return FALSE;

	char num = 'b';
	out->write(KMLStart());
	out->write(KMLName(id));

	for (int i = 0; i < bslist.length(); ++i)
	{
		bslist[i].Trim(trim);
		if (i == 0)
		{
			// kml
			if (strstr(bslist[0], "ropewiki.com"))
				if (!f.Download(bslist[0]))
					out->write(KMLFolderMerge(MkString("rw:%s", id), f.memory, "rw:"));
			continue;
		}

		inetmemory mem;
		int code = GetCode(bslist[i]);
		if (code > 0)
			if (RunExtract(codelist[code]->betac->kfunc, codelist[code]->betac->ubase, bslist[i], &mem, fx))
			{
				out->write(KMLFolderMerge(MkString("%s:%s", codelist[code]->code, id), mem.memory, MkString("%s:", codelist[code]->code)));
				++num;
			}
	}
	out->write(KMLEnd());
	/*
		vara bsbaselist;
		for (int b=0; b<bslist.length(); ++b)
			bsbaselist.push(GetToken(GetToken(bslist[b], 1, ':'), 2, '/'));

		for (int i=0; codelist[i].base!=NULL; ++i)
		  for (int b=0; b<bsbaselist.length(); ++b)
			if (strstr(bsbaselist[b], codelist[i].base))
			  if (RunExtract( codelist[i].extractkml, codelist[i].base, url, out, fx))
				  return TRUE;
	*/

	return TRUE;
}


int KMLExtract(const char *urlstr, inetdata *out, int fx)
{
	CString url(urlstr);
	url.Replace("&amp;", "&");
	url.Replace("%3D", "=");
	url.Replace("&ext=.kml", "");
	url.Replace("&ext=.gpx", "");
	int timestampIndex = url.Find("&timestamp=");
	if (timestampIndex > 0) url = url.Left(timestampIndex);

	//direct extraction
	if (!IsSimilar(url, "http"))
		return KMLAutoExtract(url, out, fx);

	if (strstr(url, "ropewiki.com"))
	{
		DownloadFile f(TRUE, out);
		if (f.Download(url))
			Log(LOGERR, "ERROR: can't download url %.128s", url);
		return TRUE;
	}

	for (int i = 1; codelist[i]->betac->ubase != NULL; ++i)
		if (strstr(url, codelist[i]->betac->ubase))
			if (RunExtract(codelist[i]->betac->kfunc, codelist[i]->betac->ubase, url, out, fx))
				return TRUE;

	Log(LOGWARN, "Could not extract KML for %s", url);
	return FALSE;
}


int ExtractBetaKML(int MODE, const char *url, const char *file)
{
	CString outfile = "out.kml";
	if (file != NULL && *file != 0)
		outfile = file;

	Log(LOGINFO, "%s -> %s", url, outfile);
	inetfile out = inetfile(outfile);
	KMLExtract(url, &out, MODE > 0);
	return TRUE;
}


static int codesort(Code *a1, Code *a2)
{
	if (!a1->name)
		return 1;
	if (!a2->name)
		return -1;
	return strcmp(a1->name, a2->name);
}


vars reduceHTML(const char *data)
{
	vars out = data;
	while (!ExtractStringDel(out, "<script", "", "</script>").IsEmpty());
	while (!ExtractStringDel(out, "<style", "", "</style>").IsEmpty());
	while (!ExtractStringDel(out, "<p><strong>Hits:", "", "<p>").IsEmpty());
	while (!ExtractStringDel(out, "<div class=\"systemquote\">", "", "</div>").IsEmpty());
	while (!ExtractStringDel(out, "<p class=\"small\"> WassUp", "", "\n").IsEmpty());
	while (out.Replace("\t", " "));
	while (out.Replace("\r", "\n"));
	while (out.Replace("  ", " "));
	while (out.Replace("\n\n", "\n"));
	out = htmlnotags(out);
	return out;
}


int ListBeta(void)
{
	vars outfile = "betasites.txt";

	CFILE file;
	if (!file.fopen(outfile, CFILE::MWRITE))
		return FALSE;

	int count = 0;
	for (int i = 1; codelist[i]->code != NULL; ++i)
	{
		if (!codelist[i]->name || codelist[i]->IsBook())
			continue;
		++count;
	}

	file.fputstr(MkString("<!-- %d Linked Beta Sites -->", count));

	file.fputstr(
		MkString("{| class=\"wikitable sortable\"\n"
			"! Linked Beta sites || Region || Star Extraction || Map Extraction || Condition Extraction\n"
			"|-\n", count)
	);
	/*
		int num = sizeof(codelist)/sizeof(codelist[0]);
		qsort(codelist, num, sizeof(codelist[0]), (qfunc*)codesort);
	*/
	DownloadFile f;
	for (int i = 1; codelist[i]->code != NULL; ++i)
	{
		if (!codelist[i]->name || codelist[i]->IsBook())
			continue;

		Code *c = codelist[i];
		vars rwlang = c->translink ? c->translink : "en";
		vars flag = MkString("[[File:rwl_%s.png|alt=|link=]]", rwlang);
		file.fputstr(MkString("| %s[%s  %s] || %s || %s || %s || %s", flag, c->betac->umain, c->name, c->xregion, c->xstar, c->xmap, c->xcond));
		file.fputstr("|-");

		if (!c->betac)
			Log(LOGERR, "NULL betac for %s %s", c->code, c->name);

		if ((*c->xmap == 0) != (c->betac->kfunc == NULL))
			Log(LOGERR, "Inconsistent xmap vs DownloadKML for %s %s", c->code, c->name);

		if ((*c->xcond == 0) != (c->betac->cfunc == NULL))
			Log(LOGERR, "Inconsistent xcond vs DownloadConditions for %s %s", c->code, c->name);

		if ((c->staruser == NULL || *c->staruser == 0) != (c->xstar == NULL || *c->xstar == 0))
			Log(LOGERR, "Inconsistent xstar vs staruser for %s %s", c->code, c->name);

		printf("Processing #%d %s...            \r", i, c->code);

		// check if user page exists

		if (c->staruser && *c->staruser != 0)
		{
			vars page = "User:" + vars(c->staruser);
			CPage p(f, NULL, page);
			if (!p.Exists())
			{
				Log(LOGINFO, "Creating page " + page);
				p.lines.push("#REDIRECT [[Beta Sites]]");
				p.comment.push("Created Beta Site redirect page " + page);
				p.Update();
			}
		}


		// check downloads
		double ticks;
		Throttle(ticks, 1000);
		if (f.Download(c->betac->umain))
			Log(LOGERR, "Could not download umain '%s' for code '%s'", c->betac->umain, c->code);

		CSymList list;
		list.Load(filename(c->code));
		if (list.GetSize() > 0 && MODE > 0)
		{
			vars url = list[0].id;
			Throttle(ticks, 1000);
			if (f.Download(url, "test1.html"))
			{
				Log(LOGERR, "Could not download url '%s' for code '%s'", url, c->code);
				continue;
			}

			vars memory = reduceHTML(f.memory);

			vars url2 = RWLANGLink(url, rwlang);
			Throttle(ticks, 1000);
			if (f.Download(url2, "test2.html"))
			{
				Log(LOGERR, "Could not download RWLANG url '%s' for code '%s'", url2, c->code);
				continue;
			}

			vars memory2 = reduceHTML(f.memory);
			memory2.Replace(url2, url);

			if (strcmp(memory, memory2) != 0)
			{
				CFILE file;
				if (file.fopen("test1.txt", CFILE::MWRITE))
				{
					file.fwrite(memory, memory.GetLength(), 1);
					file.fclose();
				}
				if (file.fopen("test2.txt", CFILE::MWRITE))
				{
					file.fwrite(memory2, memory2.GetLength(), 1);
					file.fclose();
				}
				system("windiff.exe test1.txt test2.txt");
				Log(LOGERR, "Inconsistent RWLANG '%s' for code '%s'", url2, c->code);
			}
		}
	}

	file.fputstr("|}\n");
	file.fclose();

	system(outfile);

	return TRUE;
}


int rwfdisambiguation(const char *line, CSymList &list)
{
	vara labels(line, "label=\"");
	vars id = getfulltext(labels[0]);
	vara titles = getfulltextmulti(labels[1]);
	//vars located = ExtractString(labels[1], "Located in region", "<value>", "</value>");
	if (id.IsEmpty()) {
		Log(LOGWARN, "Error empty ID from %.50s", line);
		return FALSE;
	}
	CSym sym(id, titles.join(";"));
	list.Add(sym);
	return TRUE;
}


CString Disambiguate(const char *title)
{
	return GetToken(title, 0, '(').Trim();
}


int DisambiguateBeta(int MODE, const char *movefile)
{
	CSymList ignorelist;
	ignorelist.Load(filename("ignoredisamb"));

	const char *disambiguation = DISAMBIGUATION;
	// query = [[Category:Canyons]][[Located in region.Located in regions::X]]
	DownloadFile f;

	// get idlist
	int error = 0;
	CSymList idlist;
	GetASKList(idlist, CString("[[Category:Disambiguation pages]]|?Needs disambiguation"), rwfdisambiguation);
	for (int i = 0; i < idlist.GetSize(); ++i) {
		if (!strstr(idlist[i].id, disambiguation)) {
			if (ignorelist.Find(idlist[i].id) < 0)
				Log(LOGWARN, "WARNING: No %s in page %s", disambiguation, idlist[i].id);
			idlist.Delete(i--);
			continue;
		}
		if (vara(idlist[i].data, ";").length() < 2)
			if (ignorelist.Find(idlist[i].id) < 0)
				Log(LOGWARN, "WARNING: Disambigation page '%s' has <2 entries '%s'", idlist[i].id, idlist[i].data);
	}

	/*
	CString pname = GetToken(prop, 0, '=');
	CString pval = UTF8(GetToken(prop, 1, '='));
	Log(LOGINFO, "REPLACING %s=%s for %d", pname, pval, idlist.GetSize());
	*/

	// add more if needed
	CSymList newidlist, rwlist;
	GetASKList(rwlist, CString("[[Category:Canyons]]") + "|%3FLocated in region" + "|sort=Modification_date|order=asc", rwfregion);
	CSymList namelist;
	for (int i = 0; i < rwlist.GetSize(); ++i)
	{
		CString name = Disambiguate(rwlist[i].id);
		int f;
		if ((f = namelist.Find(name)) < 0)
			namelist.Add(CSym(name, rwlist[i].id));
		else
		{
			namelist[f].data += "," + rwlist[i].id;
			name += disambiguation;
			if (idlist.Find(name) < 0) {
				idlist.Add(CSym(name));
				newidlist.Add(CSym(name));
			}
		}
	}

	CSymList movebeta;

	/*
	CSymList fullrwlist;
	fullrwlist.Load(filename(codelist[0].code));
	for (int i=0; i<fullrwlist.GetSize(); ++i)
		fullrwlist[i].id = fullrwlist[i].GetStr(ITEM_DESC);
	fullrwlist.Sort();
	*/

	for (int i = 0; i < namelist.GetSize(); ++i)
	{
		int id = -1;
		vars name = namelist[i].id;
		vars data = namelist[i].data;
		vara dlist(data);
		if (dlist.length() < 2)
			continue;
		if ((id = dlist.indexOf(name)) >= 0 && ignorelist.Find(name) < 0)
		{
			// base name == item => move it to item ( )?
			int f;
			if ((f = rwlist.Find(name)) < 0)
			{
				Log(LOGERR, "ERROR: Disambiguation could not find '%s' in rw.csv", name);
				continue;
			}
			vars region = rwlist[f].GetStr(0);//GetLastRegion(fullrwlist[f].GetStr(ITEM_REGION));
			if (region.IsEmpty())
			{
				Log(LOGERR, "ERROR: Disambiguation empty region for '%s' in rw.csv", name);
				continue;
			}
			vars newname = MkString("%s (%s)", name, region);
			vars problem;
			const char *problems[] = { "(Upper", "(Lower", "(Middle", "(West", "(Right", "(Left", "(Right", NULL };
			if (IsContained(data, problems))
			{
				problem = ",PROBLEM:";
				//Log(LOGWARN, "WARNING: Disambiguation problem:\n%s,%s,   PROBLEM %s", name, newname, dlist.join(";"));
				//continue;
			}
			//Log(LOGWARN, "Unbalanced Disambiguation for: %s => %s @ %s", name, newname, dlist.join(";"));
			CSym sym(name, problem + newname + ",," + data);
			movebeta.Add(sym);
		}
	}
	if (movefile)
		movebeta.Save(movefile);

	if (MODE < 1)
		idlist = newidlist;

	idlist.Sort();

	if (!Login(f))
		return FALSE;

	for (int i = 0; i < idlist.GetSize(); ++i)
	{
		CString title = idlist[i].id;
		vars redirect = "#REDIRECT [[" + title + "]]";
		for (int pass = 0; pass < 2; ++pass)
		{
			if (pass > 0)
				title.Replace(disambiguation, "");
			CPage p(f, NULL, title);
			vara &lines = p.lines;
			vara &comment = p.comment;
			printf("EDITING %d/%d %s %s....\r", i, idlist.GetSize(), "", title);
			// update lines
			if (lines.length() == 0)
			{
				// NEW DISAMBIGUATION
				if (pass == 0)
				{
					lines.push("{{Disambiguation}}");
					comment.push("Added auto-disambiguation");
				}
				else
				{
					lines.push(redirect);
					comment.push("Added auto-disambiguation-redirect");
				}
			}
			else if (pass == 0 && !IsSimilar(lines[0], "{{Disambiguation"))
			{
				// FIX DISAMBIGUATION
				int error = FALSE;
				vars pagename = Disambiguate(title);
				for (int i = 1; i < lines.length() && !error; ++i) {
					vars link = GetToken(ExtractString(lines[i], "[[", "", "]]"), 0, '|');
					if (link.IsEmpty()) continue;

					vars linkname = Disambiguate(link);
					if (linkname != pagename) {
						Log(LOGERR, "ERROR: Can't use auto-disambiguation for %s (%s!=%s)", title, pagename, linkname);
						error = TRUE;
					}
				}
				if (error)
					continue;

				if (!strstr(title, disambiguation)) {
					Log(LOGERR, "ERROR: No %s in page %s", disambiguation, title);
					continue;
				}

				lines = vara("{{Disambiguation}}\n", "\n");
				comment.push("Updated auto-disambiguation");
			}
			else if (pass == 1 && !IsSimilar(lines[0], redirect))
			{
				if (!IsSimilar(lines[0], "{{Canyon") && !IsSimilar(lines[0], "{{#set:Is Swaney exploration=true"))
					Log(LOGWARN, "WARNING: Can't set auto-disambiguation-redirect for %s (%s)", title, lines[0]);
				continue;
			}
			p.Update();
		}
	}
	return TRUE;
}


vars PageName(const char *from)
{
	vars name(from);
	name.Trim();
	name.Replace("_", " ");
	name.Replace("%2C", ",");
	return name;
}


int SimplynameBeta(int mode, const char *file, const char *movefile)
{
	CSymList symlist;
	symlist.Load(file);
	Log(LOGINFO, "Processing %d syms from %s", symlist.GetSize(), file);

	// regions
	if (MODE < 0)
	{
		CSymList regions;
		GetASKList(regions, "[[Category:Regions]]|%3FHas region level|%3FLocated In Regions", rwfregion);
		regions.SortNum(0);
		for (int i = 0; i < regions.GetSize(); ++i)
			symlist.Add(CSym(regions[i].GetStr(1), regions[i].id));
	}

	// find duplicates
	CSymList duplist = symlist;
	for (int i = 0; i < duplist.GetSize(); ++i)
		duplist[i].id = stripAccentsL(symlist[i].GetStr(ITEM_DESC));
	duplist.Sort();
	for (int i = 1; i < duplist.GetSize(); ++i)
		if (duplist[i].id == duplist[i - 1].id)
			Log(LOGERR, "NAME OVERLAP! '%s' vs '%s'", duplist[i].GetStr(ITEM_DESC), duplist[i - 1].GetStr(ITEM_DESC));

	// simplify names
	for (int i = 0; i < symlist.GetSize(); ++i)
	{
		vars dup;
		vars name = symlist[i].GetStr(ITEM_DESC);
		vars sname = stripAccents(name);
		if (MODE >= 1)
			sname = Capitalize(sname);
		if (MODE >= 2)
		{
			sname.Replace(" de ", " ");
			sname.Replace(" las ", " ");
		}
		if (name == sname) // || IsSimilar(name, UTF8("Cañón")))
		{
			symlist.Delete(i--);
		}
		else
		{
			symlist[i].SetStr(ITEM_NEWMATCH, symlist[i].id);
			symlist[i].SetStr(ITEM_DESC, sname);
			symlist[i].id = name;
		}
	}


	symlist.Save(movefile);

	return TRUE;
}


int MoveBeta(int mode, const char *file)
{
	vara done;
	DownloadFile f;
	CSymList symlist;
	symlist.Load(file);


	const char *comment = "Renamed page";

	/*
	CSymList regions;
	GetASKList(regions, "[[Category:Regions]]", rwfregion);
	regions.Sort();
	*/

	Login(f);
	for (int i = 0; i < symlist.GetSize(); ++i)
	{
		CString from = PageName(symlist[i].id);
		CString to = PageName(symlist[i].GetStr(ITEM_DESC));
		if (from.IsEmpty() || to.IsEmpty() || from == to || strstr(from, RWID))
		{
			//Log(LOGERR, "Skipping line '%s'", symlist[i]);
			continue;
		}

		if (strchr(from, ':') > 0)
		{
			CPage p(f, NULL, from, NULL);
			p.comment.push(comment);
			p.Move(to);
			continue;
		}

		// page
		{
			CSymList list;
			CSymList regionlist;
			GetASKList(regionlist, MkString("[[Located in region::%s]]", url_encode(from)) + "|%3FHas pageid", rwftitleo);
			/*
			vars query = "[[Category:Canyons]]";
			// get regionlist
			if (regionlist.GetSize()>0)
				query = "[[Category:Regions]]";
				*/

				// move page
			GetASKList(list, MkString("[[%s]]", url_encode(from)), rwftitleo);
			BOOL move = TRUE;
			if (list.GetSize() != 1)
			{
				Log(LOGERR, "ERROR MOVE FROM %s does not exist", from);
				move = FALSE;
			}
			if (CheckPage(to))
			{
				Log(LOGERR, "ERROR MOVE TO %s already exists", to);
				move = FALSE;
			}
			if (!move && MODE < 1)
				continue;
			if (move)
			{
				CPage p(f, NULL, from);
				p.comment.push(comment);
				p.Move(to);
			}

			// regionlist
			for (int r = 0; r < regionlist.GetSize(); ++r)
			{
				CString &title = regionlist[r].id;
				vars prop = !regionlist[r].GetStr(0).IsEmpty() ? "Region" : "Parent region";
				// download page 

				printf("UPDATING %d/%d %s %s....\r", r, regionlist.GetSize(), "", title);
				CPage p(f, NULL, title);
				UpdateProp(prop, to, p.lines, TRUE);
				p.comment.push("Updated " + prop + "=" + to);
				p.Update();
			}
			if (regionlist.GetSize() > 0)
				continue;
		}

		// votes
		{
			CSymList list;
			GetASKList(list, MkString("[[Category:Page ratings]][[Has page rating page::%s]]", url_encode(from)), rwftitleo);
			for (int l = 0; l < list.GetSize(); ++l)
			{
				vars lfrom = list[l].id;
				vars lto = lfrom.replace(from, to);
				CPage p(f, NULL, lfrom);
				p.Set("Page", to, TRUE);
				p.comment.push(comment);
				p.Update();
				p.Move(lto);
			}
		}

		// conditions
		{
			CSymList list;
			GetASKList(list, MkString("[[Category:Conditions]][[Has condition location::%s]]", url_encode(from)), rwftitleo);
			for (int l = 0; l < list.GetSize(); ++l)
			{
				vars lfrom = list[l].id;
				vars lto = lfrom.replace(from, to);
				CPage p(f, NULL, lfrom);
				p.Set("Location", to, TRUE);
				p.comment.push(comment);
				p.Update();
				p.Move(lto);
			}
		}

		// references
		{
			CSymList list;
			GetASKList(list, MkString("[[Category:References]][[Has condition location::%s]]", url_encode(from)), rwftitleo);
			for (int l = 0; l < list.GetSize(); ++l)
			{
				vars lfrom = list[l].id;
				vars lto = lfrom.replace(from, to);
				CPage p(f, NULL, lfrom);
				p.Set("Location", to, TRUE);
				p.comment.push(comment);
				p.Update();
				p.Move(lto);

				vars ffrom = lfrom.replace("References:", "File:") + ".jpg";
				vars fto = ffrom.replace(from, to);
				CPage pf(f, NULL, ffrom);
				p.comment.push(comment);
				p.Move(fto);
			}
		}

		// files (banner, kml, pics, etc)
		const char *assfiles[] = { ".kml", ".pdf", "_Banner.jpg", NULL };
		for (int n = 0; assfiles[n] != NULL; ++n)
		{
			CSymList list;
			GetASKList(list, MkString("[[File:%s%s]]", url_encode(from), assfiles[n]), rwftitleo);
			for (int l = 0; l < list.GetSize(); ++l)
			{
				vars lfrom = list[l].id;
				vars lto = lfrom.replace(from, to);
				CPage p(f, NULL, lfrom, NULL);
				p.comment.push(comment);
				p.Move(lto);
				done.Add(lto);
			}
		}

		//other stuff
		if (f.Download("http://ropewiki.com/Special:ListFiles?limit=50&ilsearch=" + url_encode(from)))
		{
			Log(LOGERR, "ERROR: can't download Special:ListFiles %s", from);
			continue;
		}

		vara files(f.memory, "<a href=\"/File:");

		if (files.length() > 1)
			Log(LOGWARN, "MOVING RESIDUE Files...");
		for (int l = 1; l < files.length(); ++l)
		{
			vars file = "File:" + url_decode(ExtractString(files[l], "", "", "\""));
			file.Replace("_", " ");
			file.Replace("&#039;", "'");
			//if (MODE<1)
			//else
			if (done.indexOf(file) < 0)
				if (!strstr(file, "_Banner.jpg") && !strstr(file, ".kml"))
				{
					done.push(file);
					vars lfrom = file;
					vars lto = lfrom.replace("File:" + from, "File:" + to);
					if (lfrom != lto)
					{
						CPage p(f, NULL, lfrom, NULL);
						p.comment.push(comment);
						p.Move(lto);
					}
				}
		}
	}

	return TRUE;
}


int DeleteBeta(int mode, const char *file, const char *comment)
{
	DownloadFile f;
	CSymList symlist;
	symlist.Load(file);

	Login(f);
	for (int i = 0; i < symlist.GetSize(); ++i)
	{
		CString from = PageName(symlist[i].id);
		CPage p(f, NULL, from);
		if (p.lines.length() > 0 && IsSimilar(p.lines[0], "{{Canyon"))
		{
			// delete votes
			CSymList list;
			GetASKList(list, MkString("[[Category:Page ratings]][[Has page rating page::%s]]", url_encode(from)), rwftitleo);
			symlist.Add(list);
		}

		p.comment.push(comment);
		p.Delete();
	}

	return TRUE;
}


int cmprwid(CSym **arg1, CSym **arg2)
{
	register double rdiff = (*arg1)->index - (*arg2)->index;
	if (rdiff < 0) return -1;
	if (rdiff > 0) return 1;
	rdiff = GetCode((*arg1)->id) - GetCode((*arg2)->id);
	if (rdiff < 0) return -1;
	if (rdiff > 0) return 1;
	return 0;
}


int TestBeta(int mode, const char *column, const char *code)
{
	vara hdr(vars(headers).replace("ITEM_", "").replace(" ", ""));
	int col = hdr.indexOf(column) - 1;
	if (col < 0)
	{
		Log(LOGERR, "Invalid column '%s'", column);
		return FALSE;
	}

	// set NULL column
	CSymList bslist;
	LoadBetaList(bslist);
	for (int i = 0; i < rwlist.GetSize(); ++i)
	{
		CSym &sym = rwlist[i];
		sym.SetStr(ITEM_NEWMATCH, sym.GetStr(col));
		sym.SetStr(col, "");
	}

	// -getbeta [code]
	DownloadBeta(MODE = 1, code);

	// load results
	vars file = filename(CHGFILE);
	CSymList chgfile;
	chgfile.Load(file);
	// sort results by RWID
	for (int i = 0; i < chgfile.GetSize(); ++i)
	{
		CSym &sym = chgfile[i];
		vars match = sym.GetStr(ITEM_MATCH);
		if (!IsSimilar(match, RWID) || !strstr(match, RWLINK))
		{
			// skip NON matches
			chgfile.Delete(i--);
			continue;
		}

		sym.index = CDATA::GetNum(match.Mid(strlen(RWID)));
		if (sym.index == InvalidNUM)
		{
			Log(LOGERR, "Unexpected invalid RWID %s", match);
			chgfile.Delete(i--);
			continue;
		}
	}
	chgfile.Sort((qfunc *)cmprwid);
	chgfile.Save(file);

	// process sorted list by RWID
	for (int i = 0; i < chgfile.GetSize();)
	{
		double index = chgfile[i].index;
		int f = rwlist.Find(RWID + CData(index));
		if (f < 0)
		{
			Log(LOGERR, "Inconsistend RWID:%s", CData(index));
			for (int n = 0; index == chgfile[i].index; ++n)
				chgfile.Delete(i);
			continue;
		}

		for (int n = 0; i < chgfile.GetSize() && index == chgfile[i].index; ++n, ++i)
		{
			CSym &sym = chgfile[i];
			vars vvalue, value = sym.GetStr(col);
			vars vovalue, ovalue = rwlist[f].GetStr(ITEM_NEWMATCH);
			switch (col)
			{
			case ITEM_SEASON:
			{
				vars val = GetToken(value, 0, "!");
				IsSeasonValid(val, &val);
				vvalue = SeasonCompare(val);
				vovalue = SeasonCompare(ovalue);
			}
			break;
			}

			BOOL match = vvalue == vovalue;
			if (MODE > 0 && strstr(value, "?"))
				match = FALSE;
			if (n == 0 && match)
			{
				// first order match! move on!
				while (index == chgfile[i].index)
					chgfile.Delete(i);
				break;
			}

			sym.SetStr(ITEM_NEWMATCH, MkString("#%d: %s %s %s", n, vovalue, match ? "==" : "!=", vvalue));
			sym.SetStr(ITEM_REGION, rwlist[f].GetStr(ITEM_REGION));
			sym.SetStr(ITEM_LAT, "");
			sym.SetStr(ITEM_LNG, "");
		}
	}

	chgfile.Save(file);
	return TRUE;
}


int rwfgeocode(const char *line, CSymList &regions)
{
	vara labels(line, "label=\"");
	vars id = getfulltext(labels[0]);
	//vars located = ExtractString(labels[1], "Located in region", "<value>", "</value>");
	if (id.IsEmpty()) {
		Log(LOGWARN, "Error empty ID from %.50s", line);
		return FALSE;
	}
	CSym sym(RWID + ExtractString(line, "Has pageid", "<value>", "</value>"));
	//ASSERT(!strstr(id, "Northern Ireland"));
	sym.SetStr(ITEM_DESC, id);
	sym.SetStr(ITEM_LAT, ExtractString(line, "lat=", "\"", "\""));
	sym.SetStr(ITEM_LNG, ExtractString(line, "lon=", "\"", "\""));
	sym.SetStr(ITEM_MATCH, ExtractString(ExtractString(line, "Has geolocation", "", "<property"), "<value>", "", "</value>").replace(",", ";"));
	regions.Add(sym);
	return TRUE;
}


vars GetFullGeoRegion(const char *georegion)
{
	//CSymList &regions = GeoRegion.Regions();

	vara list;
	vara glist(georegion, ";");
	_GeoRegion.Translate(glist);
	for (int i = 0; i < glist.length(); ++i)
	{
		vars reg = GetToken(glist[i], 0, ':');
		if (list.indexOf(reg) < 0)
			list.push(reg);
	}
	return list.join(";");
}


int CheckRegion(const char *region, CSymList &regions)
{
	vara majoryes, majorno;
	static CSymList nomajor;
	if (nomajor.GetSize() == 0)
	{
		nomajor.Load(filename(GEOREGIONNOMAJOR));
		nomajor.Sort();
	}

	for (int i = 0; i < regions.GetSize(); ++i)
		if (stricmp(regions[i].GetStr(0), region) == 0)
		{
			// check children
			vars children = regions[i].id;
			vars major = regions[i].GetStr(1);
			if (IsSimilar(major, "T"))
				majoryes.push(children);
			else
				if (nomajor.Find(children) < 0)
					majorno.push(children);
			CheckRegion(children, regions);
		}

	if (majoryes.length() > 0 && majorno.length() > 0)
	{
		Log(LOGERR, "Inconsisten major for '%s' : YES [%s] != NO [%s]", region, majoryes.join(";"), majorno.join(";"));
		return FALSE;
	}

	return TRUE;
}


int CheckRegions(CSymList &rwlist, CSymList &chglist)
{
	// check region structure
	CSymList regions;
	GetASKList(regions, "[[Category:Regions]]|%3FLocated in region|%3FIs major region|sort=Modification_date|order=asc", rwfregion);

	CheckRegion("World", regions);

	// check geocodes match
	CSymList geolist;
	GetASKList(geolist, "[[Category:Canyons]]|%3FHas pageid|%3FHas coordinates|%3FHas geolocation|sort=Modification_date|order=asc", rwfgeocode);
	for (int i = 0; i < geolist.GetSize(); ++i)
	{
		CSym &sym = geolist[i];
		printf("%d/%d Geocode %s                     \r", i, geolist.GetSize(), sym.GetStr(ITEM_DESC));
		double lat = sym.GetNum(ITEM_LAT);
		double lng = sym.GetNum(ITEM_LNG);
		double glat, glng;
		vars geocode = sym.GetStr(ITEM_MATCH);
		if (!CheckLL(lat, lng))
		{
			Log(LOGERR, "Invalid lat/lng %s %s: %s,%s (geocode:%s)", sym.id, sym.GetStr(ITEM_DESC), CData(lat), CData(lng), geocode);
			continue;
		}
		if (_GeoCache.Get(geocode.replace(";", ","), glat, glng))
		{
			double dist = Distance(lat, lng, glat, glng);
			if (dist > MAXGEOCODEDIST)
			{
				vars err = MkString("Geocode '%s' (%g;%g) = %dkm", geocode, lat, lng, (int)(dist / 1000));
				Log(LOGERR, "Mismatched geocode %s %s: %s", sym.id, sym.GetStr(ITEM_DESC), err);
				sym.SetStr(ITEM_NEWMATCH, err);
				chglist.Add(sym);
			}
		}
	}

	CSymList fakelist, dellist;
	fakelist.Add(CSym("Location"));
	fakelist.Add(CSym("No region specified"));

	// check empty regions
	DownloadFile f;
	for (int i = 0; i < regions.GetSize(); )
	{
		printf("%d/%d Region %s                     \r", i, regions.GetSize(), regions[i].id);
		vara regions10;
		for (int j = 0; j < 10 && i < regions.GetSize(); ++j, ++i)
			regions10.push(regions[i].id);

		// check empty
		vars url = "http://ropewiki.com/index.php?title=Template:RegionCount&action=raw&templates=expand&ctype=text/x-wiki&region=" + url_encode(regions10.join(";"));
		if (f.Download(url))
		{
			Log(LOGERR, "Could not download url %s", url);
			continue;
		}

		vara res(f.memory, ";");
		for (int j = 0; j < res.length(); ++j)
			if (res[j] == "(0)" && fakelist.Find(regions10[j]) < 0)
			{
				Log(LOGWARN, "WARNING: '%s' is an empty region (use -deletebeta delete.csv to delete)", _GeoRegion.GetFullRegion(regions10[j]));
				dellist.Add(CSym(regions10[j]));
			}
	}

	// write delete file
	{
		vars deletefile = "delete.csv";
		DeleteFile(deletefile);
		CFILE df;
		if (df.fopen(deletefile, CFILE::MAPPEND))
		{
			df.fputstr("pagename");
			for (int i = 0; i < dellist.GetSize(); ++i)
				df.fputstr(dellist[i].id);
		}
	}

	// build distance map
	PLLArrayList lllist;
	for (int i = 0; i < rwlist.GetSize(); ++i)
	{
		CSym &sym = rwlist[i];
		double lat = sym.GetNum(ITEM_LAT);
		double lng = sym.GetNum(ITEM_LNG);
		if (CheckLL(lat, lng))
			lllist.AddTail(PLL(LL(lat, lng), &sym));
	}
	
	PLLRectArrayList llrlist;
	for (int i = 0; i < rwlist.GetSize(); ++i)
	{
		CSym &sym = rwlist[i];
		double lat = sym.GetNum(ITEM_LAT);
		double lng = sym.GetNum(ITEM_LNG);
		if (lat != InvalidNUM && lat != 0 && lng != InvalidNUM && lng != 0)
			llrlist.AddTail(PLLRect(lat, lng, MAXGEOCODEDIST, &sym)); // 150km
	}
	
	LLMatch<PLLRect, PLL> mlist(llrlist, lllist, addmatch);
	for (int i = 0; i < llrlist.GetSize(); ++i)
	{
		PLLRect *prect = &llrlist[i];
		CSym &sym = *prect->ptr;
		vars title = sym.GetStr(ITEM_DESC);
		printf("Processing %s %d/%d        \r", title, i, llrlist.GetSize());

		// check GeoRegion
		vara georegions;
		if (_GeoRegion.GetRegion(sym, georegions) >= 0)
			continue;

		prect->closest.Sort(prect->closest[0].cmp);

		// check 5 closest regions
		int matchedregion = 0;
		CArrayList <PLLD> &closest = prect->closest;
		vars nearregion;
		vars region = sym.GetStr(ITEM_REGION), mregion = GetToken(region, 0, ';');
		if (!region.IsEmpty())
			for (int j = 1; j < MAXGEOCODENEAR && j < closest.GetSize() && !matchedregion; ++j)
			{
				//double d = prect->closest[j].d;
				//CSym *psym = prect->closest[j].ll->ptr;
				//BOOL pgeoloc = strstr(psym->GetStr(ITEM_LAT), "@")!=NULL;
				CSym *csym = closest[j].ll->ptr;
				vars cregion = csym->GetStr(ITEM_REGION);
				if (region == cregion)
					++matchedregion;
				if (nearregion.IsEmpty()) // closer same Main Region
					if (GetToken(cregion, 0, ';') == mregion)
						nearregion = cregion;
			}
		
		if (!matchedregion && !region.IsEmpty() && !nearregion.IsEmpty())
		{
			vara aregion(_GeoRegion.GetSubRegion(region, mregion), ";");
			vara anearregion(_GeoRegion.GetSubRegion(nearregion, mregion), ";");
			if (anearregion.length() < aregion.length())
				continue; // avoid de-refining

			vars aregions = aregion.join(";");
			vars anearregions = anearregion.join(";");
			ASSERT(aregions != anearregions);
			BOOL refined = anearregion.length() > aregion.length() && strstri(anearregions, aregions);

			CSym chgsym(sym.id, title);
			chgsym.SetStr(ITEM_MATCH, refined ? "REFINED REG " : "WRONG REG " + aregions);
			chgsym.SetStr(ITEM_REGION, anearregions);
			//sym.SetStr(ITEM_REGION, anearregions);
			chglist.Add(chgsym);
			Log(LOGWARN, "WARNING: no matching nearby region? '%s' nearby:%s '%s' -> %s", aregions, sym.id, title, anearregions);
		}
	}

	// check invalid regions
	for (int i = 0; i < rwlist.GetSize(); ++i)
	{
		CSym &sym = rwlist[i];
		vars title = sym.GetStr(ITEM_DESC);
		vars region = _GeoRegion.GetRegion(sym.GetStr(ITEM_REGION));

		if (region.IsEmpty() || regions.Find(region) < 0)
		{
			vars url = "http://ropewiki.com/api.php?action=query&prop=info&titles=" + url_encode(title);
			if (f.Download(url))
			{
				Log(LOGERR, "Could not download url %s", url);
				continue;
			}
			else
			{
				if (strstr(f.memory, "redirect="))
					continue;
			}

			// official region
			vara georegions;
			int r = _GeoRegion.GetRegion(sym, georegions);
			vars georegion = r < 0 ? "?" : _GeoRegion.GetFullRegion(georegions[r]);

			// closest match
			int llr = -1;
			vars nearregion;
			for (int r = 0; r < llrlist.GetSize() && llr < 0; ++r)
				if (llrlist[r].ptr == &sym)
				{
					CArrayList <PLLD> &closest = llrlist[r].closest;
					for (int c = 1; c < closest.length() && nearregion.IsEmpty(); ++c)
					{
						CSym *csym = llrlist[r].closest[c].ll->ptr;
						nearregion = _GeoRegion.GetFullRegion(csym->GetStr(ITEM_REGION));
					}
				}

			CSym chgsym(sym.id, title);
			chgsym.SetStr(ITEM_MATCH, MkString("BAD REGION! SET TO GEOREGION (NEAREST: %s)", nearregion));
			chgsym.SetStr(ITEM_REGION, georegion);
			sym.SetStr(ITEM_REGION, georegion);
			chglist.Add(chgsym);

			Log(LOGWARN, "WARNING: invalid region '%s' for %s '%s' -> GEOREGION:%s NEAREST:%s", region, sym.id, title, georegion, nearregion);
		}
	}

	// fix empty regions http://ropewiki.com/index.php?title=Template:RegionCount&action=raw&templates=expand&ctype=text/x-wiki&region=Ticino;Sondrio;Como

	return TRUE;
}


int sortgeomatch(const void *arg1, const void *arg2)
{
	vars s1 = ((CSym**)arg1)[0]->GetStr(ITEM_LAT);
	vars s2 = ((CSym**)arg2)[0]->GetStr(ITEM_LAT);
	return strcmp(s1, s2);
}


int FixBetaRegionGetFix(CSymList &idlist)
{
	int error = 0;
	CSymList geomatch, rwmatch;

	// find optimal MODE
	// process 
	for (int i = 0; i < idlist.GetSize(); ++i)
	{
		CSym &sym = idlist[i];
		printf("Processing %s %d/%d [err:%d]       \r", idlist[i].GetStr(ITEM_DESC), i, idlist.GetSize(), error);

		vars symregion = sym.GetStr(ITEM_REGION);
		vars tomatch = _GeoRegion.GetRegion(symregion);
		if (tomatch.IsEmpty())
		{
			Log(LOGWARN, "Possible #REDIRECT for %s [%s] (ignored)", sym.GetStr(ITEM_DESC), sym.id);
			continue; // #REDIRECT!
		}

		vara georegions;
		int maxlev = -1;
		int maxr = _GeoRegion.GetRegion(sym, georegions, &maxlev);
		if (maxr < 0)
		{
			++error;
			continue;
		}

		// shorten georegions on demand

		vars georegion;
		vara georegions2;
		if (MODE > 0) maxr = MODE; // MAX auto level
		for (int i = 0; i <= maxr && i < georegions.length(); ++i)
			georegions2.push(georegion = georegions[i]);

		// shorten (change) the region too if max is forced
		if (maxlev >= 0)
		{
			int curlev = (int)CDATA::GetNum(GetToken(georegion, 1, ':'));
			if (curlev == maxlev)
			{
				//int ni = i + maxlev - curlev;
				if (!_GeoRegion.GetSubRegion(tomatch, GetToken(georegion, 0, ':'), TRUE).IsEmpty()) // && ni<afullregion.length()) 
					continue;
				else
					tomatch = tomatch;
			}
		}

		// process match

		int g = geomatch.Find(georegion);
		if (g < 0)
		{
			CSym sym(georegion, georegions2.join(";"));
			sym.SetStr(ITEM_NEWMATCH, "X");
			geomatch.Add(sym);
			g = geomatch.Find(georegion);
		}


		geomatch[g].data += ";" + tomatch + "=" + sym.GetStr(ITEM_DESC);
	}

	// collapse results
	vara subst;
	int diff = 0, rdiff = 0;
	geomatch.Sort();
	for (int i = 0; i < geomatch.GetSize(); ++i)
	{
		CSym &sym = geomatch[i];
		vara list(sym.GetStr(ITEM_NEWMATCH), ";");
		list.RemoveAt(0);
		list.sort();


		vars georegion = GetToken(sym.id, 0, ':');
		vars region = GetToken(list[list.length() / 2], 0, '=');
		vars fullgeoregion = GetFullGeoRegion(sym.GetStr(ITEM_DESC));
		vars fullregion = _GeoRegion.GetFullRegion(region);

		sym.SetStr(ITEM_LAT, fullregion);
		if (georegion != region)
		{
			// inconsistency detected
			sym.SetStr(ITEM_LNG, georegion);
			sym.SetStr(ITEM_MATCH, fullgeoregion);
			rdiff++;
		}
		else
			if (!strstri(fullregion, fullgeoregion))
			{
				int idiff = -1, icount = 0;
				// find possible translations
				vara aregion(fullregion, ";");
				vara ageoregion(fullgeoregion, ";");
				int start = aregion.indexOf(ageoregion[0]);
				if (start >= 0)
				{
					for (int i = 0, j = start; i < ageoregion.length() && j < aregion.length(); ++i, ++j)
						if (ageoregion[i] != aregion[j])
						{
							const char *a = ageoregion[i];
							const char *b = aregion[j];
							if (strstri(ageoregion[i], "|"))
								continue; // ignore
							if (i + 1 < ageoregion.length() && ageoregion[i + 1] == aregion[j])
							{
								++i;
								continue;
							}
							if (j + 1 < aregion.length() && ageoregion[i] == aregion[j + 1])
							{
								++j;
								continue;
							}

							idiff = i;
							++icount;
						}
					if (icount == 1)
					{
						vars num, desc = sym.GetStr(ITEM_DESC);
						const char *str = strstri(desc, ageoregion[idiff]);
						if (str)
							num = ":" + GetToken(str, 1, ":;");
						else
							Log(LOGERR, "Inconsistent georegion %s not in %s", desc, ageoregion[idiff]);
						vars chg = ageoregion[idiff] + num + "," + aregion[start + idiff] + num;
						if (subst.indexOf(chg) < 0)
							subst.push(chg);
					}

				}
				if (icount > 0 || start < 0)
				{
					sym.SetStr(ITEM_MATCH, fullgeoregion);
					rdiff++;
				}
			}

		vara elist, dlist;
		for (int l = 0; l < list.length(); ++l)
			if (GetToken(list[l], 0, '=') == region)
				elist.Add(GetToken(list[l], 1, '='));
			else
				dlist.Add(list[l]);

		diff += dlist.length();

		int col = ITEM_NEWMATCH;
		sym.SetNum(col++, elist.length());
		sym.SetStr(col++, elist.join("; "));
		sym.SetNum(col++, dlist.length());
		sym.SetStr(col++, dlist.join("; "));
	}

	Log(LOGINFO, "%d Loc x %d Reg inconsistencies, saved to file %s", diff, rdiff, filename(GEOREGIONFIX));
	if (subst.length() > 0)
		Log(LOGWARN, "%d possible transtlations for file %s :\n%s", subst.length(), filename(GEOREGIONTRANS), subst.join("\n"));
	geomatch.header = "Sub Region,GEORegion,RWRegion (<-Fix),Chg,Fix,In,InList,Out,OutList";
	geomatch.Sort(sortgeomatch);
	geomatch.Save(filename(GEOREGIONFIX));
	if (_GeoRegion.overlimit)
	{
		Log(LOGERR, "ERROR: OVER LIMIT reached (%d errors), change IP to continue", _GeoRegion.overlimit);
		//return FALSE;
	}
	// Output: GeoRegionFix.csv file
	// check file then run "rwr -fixbetaregion REGION" to get chg.csv

	return diff > 0 || rdiff > 0;
}


int FixBetaRegionUseFix(CSymList &idlist)
{
	// uses georegionfix!!!

	CSymList geomatch, chglist;
	geomatch.Load(filename(GEOREGIONFIX));
	geomatch.Sort();

	for (int i = 0; i < idlist.GetSize(); ++i)
	{
		CSym &rwsym = idlist[i];
		CSym sym(rwsym.id, rwsym.GetStr(ITEM_DESC));

		vars geocode = GetToken(rwsym.GetStr(ITEM_LAT), 1, '@');
		if (!geocode.IsEmpty())
		{
			// check geocode
			double lat, lng;
			if (_GeoCache.Get(geocode, lat, lng))
			{
				double dist = Distance(lat, lng, rwsym.GetNum(ITEM_LAT), rwsym.GetNum(ITEM_LNG));
				if (dist > MAXGEOCODEDIST)
				{
					vars err = MkString("Geocode '%s' (%g;%g) = %dkm", geocode, rwsym.GetNum(ITEM_LAT), rwsym.GetNum(ITEM_LNG), (int)(dist / 1000));
					Log(LOGERR, "Mismatched geocode %s %s: %s", rwsym.id, rwsym.GetStr(ITEM_DESC), err);
					sym.SetStr(ITEM_NEWMATCH, err);
				}
			}
		}

		vara georegions;
		if (_GeoRegion.Get(rwsym, georegions))
		{
			// check georegion
			vars georegion = georegions.join(";");
			for (int j = 0; j < geomatch.GetSize(); ++j)
			{
				CSym &geosym = geomatch[j];
				if (georegions.indexOf(geosym.id) >= 0)
				{
					vars rwreg = rwsym.GetStr(ITEM_REGION);
					vars georeg = geosym.GetStr(ITEM_LAT);
					vars fixreg = geosym.GetStr(ITEM_MATCH);
					if (!fixreg.IsEmpty())
						georeg = fixreg;
					vara rwregions(rwreg, ";");
					vara georegions(georeg, ";");
					if (rwregions.last() != georegions.last())
					{
						vara fullregions(_GeoRegion.GetFullRegion(rwregions.last()), ";");
						if (fullregions.indexOfi(georegions.last()) < 0)
						{
							vara diffregions;
							for (int d = 0; d < fullregions.length(); ++d)
								if (d >= georegions.length() || fullregions[d] != georegions[d])
									diffregions.push(fullregions[d]);

							vars code = (diffregions.length() > 0 && !geocode.IsEmpty()) ? "@" : "";
							sym.SetStr(ITEM_REGION, georegions.join(";"));
							sym.SetStr(ITEM_MATCH, diffregions.join(";") + code);
						}
					}
					break;
				}
			}
		}

		if (*GetTokenRest(sym.data, 1) != 0)
			chglist.Add(sym);
	}

	Log(LOGINFO, "%d changes saved to file %s", chglist.GetSize(), filename(CHGFILE));
	chglist.header = headers;
	chglist.header.Replace("ITEM_", "");
	chglist.Save(filename(CHGFILE));
	//system(filename(CHGFILE));
	return TRUE;
}


int FixBetaRegion(int MODE, const char *fileregion)
{
	if (fileregion != NULL && *fileregion == 0)
		fileregion = NULL;

	LoadRWList();

	// select syms belonging to region or ALL if not specified
	CSymList idlist;
	BOOL all = TRUE;
	if (fileregion != NULL && *fileregion != 0)
	{
		all = FALSE;
		CSymList sellist;
		GetRWList(sellist, fileregion);
		for (int i = 0; i < sellist.GetSize(); ++i)
		{
			int f = rwlist.Find(RWID + vars(sellist[i].id).replace(RWID, ""));
			if (f < 0)
			{
				Log(LOGERR, "Mismatched pageid %s", sellist[i].id);
				continue;
			}
			idlist.Add(rwlist[f]);
		}
	}
	else
	{
		idlist = rwlist;
		for (int i = 0; i < idlist.GetSize(); ++i)
			if (idlist[i].GetStr(ITEM_REGION) == "Mars")
				idlist.Delete(i--);
	}

	// generic check and fix ALL regions
	if (all && MODE < 0)
	{
		CSymList chglist;
		CheckRegions(idlist = rwlist, chglist);

		Log(LOGINFO, "%d changes saved to file %s", chglist.GetSize(), filename(CHGFILE));
		chglist.header = headers;
		chglist.header.Replace("ITEM_", "");
		chglist.Save(filename(CHGFILE));
		return TRUE;
	}

	// check and fix specific regions
	if (MODE < 0)
		// apply fix
		return FixBetaRegionUseFix(idlist);

	// compute fix
	Log(LOGINFO, "Computing georegionfix.csv....");
	// get and apply fixes
	if (!FixBetaRegionGetFix(idlist))
		return FALSE;

	system(filename(GEOREGIONFIX));

	Log(LOGINFO, "Applying georegionfix.csv -> chg.csv....");
	if (!FixBetaRegionUseFix(idlist))
		return FALSE;

	return TRUE;
}


#if 0

//translate text
//https://translate.googleapis.com/translate_a/single?client=gtx&sl=auto&tl=en&hl=en-US&dt=t&dt=bd&dj=1&source=input&tk=534018.534018&q=basses%20eaux

// refresh
//http://ropewiki.com/api.php?action=sfautoedit&form=AutoRefresh&target=Votes:AutoRefresh&query=AutoRefresh[Location]=


//This file contains extremely crude C source code to extract plain text
//from a PDF file. It is only intended to show some of the basics involved
//in the process and by no means good enough for commercial use.
//But it can be easily modified to suit your purpose. Code is by no means
//warranted to be bug free or suitable for any purpose.
//
//Adobe has a web site that converts PDF files to text for free,
//so why would you need something like this? Several reasons:
//
//1) This code is entirely free including for commericcial use. It only
//   requires ZLIB (from www.zlib.org) which is entirely free as well.
//
//2) This code tries to put tabs into appropriate places in the text,
//   which means that if your PDF file contains mostly one large table,
//   you can easily take the output of this program and directly read it
//   into Excel! Otherwise if you select and copy the text and paste it into
//   Excel there is no way to extract the various columns again.
//
//This code assumes that the PDF file has text objects compressed
//using FlateDecode (which seems to be standard).
//
//This code is free. Use it for any purpose.
//The author assumes no liability whatsoever for the use of this code.
//Use it at your own risk!


//PDF file strings (based on PDFReference15_v5.pdf from www.adobve.com:
//
//BT = Beginning of a text object, ET = end of a text object
//5 Ts = superscript
//-5 Ts = subscript
//Td move to start next line

//No precompiled headers, but uncomment if need be:
#include "stdafx.h"

#include <stdio.h>
#include <windows.h>

//YOur project must also include zdll.lib (ZLIB) as a dependency.
//ZLIB can be freely downloaded from the internet, www.zlib.org
//Use 4 byte struct alignment in your project!

#include "zlib.h"


//Find a string in a buffer:
size_t FindStringInBuffer(char* buffer, char* search, size_t buffersize)
{
	char* buffer0 = buffer;

	size_t len = strlen(search);
	bool fnd = false;
	while (!fnd)
	{
		fnd = true;
		for (size_t i = 0; i < len; i++)
		{
			if (buffer[i] != search[i])
			{
				fnd = false;
				break;
			}
		}
		if (fnd) return buffer - buffer0;
		buffer = buffer + 1;
		if (buffer - buffer0 + len >= buffersize) return -1;
	}
	return -1;
}


//Keep this many previous recent characters for back reference:
#define oldchar 15


//Convert a recent set of characters into a number if there is one.
//Otherwise return -1:
float ExtractNumber(const char* search, int lastcharoffset)
{
	int i = lastcharoffset;
	while (i > 0 && search[i] == ' ') i--;
	while (i > 0 && (isdigit(search[i]) || search[i] == '.')) i--;
	float flt = -1.0;
	char buffer[oldchar + 5]; ZeroMemory(buffer, sizeof(buffer));
	strncpy(buffer, search + i + 1, lastcharoffset - i);
	if (buffer[0] && sscanf(buffer, "%f", &flt))
	{
		return flt;
	}
	return -1.0;
}


//Check if a certain 2 character token just came along (e.g. BT):
bool seen2(const char* search, char* recent)
{
	if (recent[oldchar - 3] == search[0]
		&& recent[oldchar - 2] == search[1]
		&& (recent[oldchar - 1] == ' ' || recent[oldchar - 1] == 0x0d || recent[oldchar - 1] == 0x0a)
		&& (recent[oldchar - 4] == ' ' || recent[oldchar - 4] == 0x0d || recent[oldchar - 4] == 0x0a)
		)
	{
		return true;
	}
	return false;
}


//This method processes an uncompressed Adobe (text) object and extracts text.
void ProcessOutput(FILE* file, char* output, size_t len)
{
	//Are we currently inside a text object?
	bool intextobject = false;

	//Is the next character literal (e.g. \\ to get a \ character or \( to get ( ):
	bool nextliteral = false;

	//() Bracket nesting level. Text appears inside ()
	int rbdepth = 0;

	//Keep previous chars to get extract numbers etc.:
	char oc[oldchar];
	int j = 0;
	for (j = 0; j < oldchar; j++) oc[j] = ' ';

	for (size_t i = 0; i < len; i++)
	{
		char c = output[i];
		if (intextobject)
		{
			if (rbdepth == 0 && seen2("TD", oc))
			{
				//Positioning.
				//See if a new line has to start or just a tab:
				float num = ExtractNumber(oc, oldchar - 5);
				if (num > 1.0)
				{
					fputc(0x0d, file);
					fputc(0x0a, file);
				}
				if (num < 1.0)
				{
					fputc('\t', file);
				}
			}
			if (rbdepth == 0 && seen2("ET", oc))
			{
				//End of a text object, also go to a new line.
				intextobject = false;
				fputc(0x0d, file);
				fputc(0x0a, file);
			}
			else if (c == '(' && rbdepth == 0 && !nextliteral)
			{
				//Start outputting text!
				rbdepth = 1;
				//See if a space or tab (>1000) is called for by looking
				//at the number in front of (
				int num = ExtractNumber(oc, oldchar - 1);
				if (num > 0)
				{
					if (num > 1000.0)
					{
						fputc('\t', file);
					}
					else if (num > 100.0)
					{
						fputc(' ', file);
					}
				}
			}
			else if (c == ')' && rbdepth == 1 && !nextliteral)
			{
				//Stop outputting text
				rbdepth = 0;
			}
			else if (rbdepth == 1)
			{
				//Just a normal text character:
				if (c == '\\' && !nextliteral)
				{
					//Only print out next character no matter what. Do not interpret.
					nextliteral = true;
				}
				else
				{
					nextliteral = false;
					if (((c >= ' ') && (c <= '~')) || ((c >= 128) && (c < 255)))
					{
						fputc(c, file);
					}
				}
			}
		}
		//Store the recent characters for when we have to go back for a number:
		for (j = 0; j < oldchar - 1; j++) oc[j] = oc[j + 1];
		oc[oldchar - 1] = c;
		if (!intextobject)
		{
			if (seen2("BT", oc))
			{
				//Start of a text object:
				intextobject = true;
			}
		}
	}
}


int _tmain(int argc, _TCHAR* argv[])
{
	//Discard existing output:
	FILE* fileo = fopen("c:\\pdf\\output2.txt", "w");
	if (fileo) fclose(fileo);
	fileo = fopen("c:\\pdf\\output2.txt", "a");

	//Open the PDF source file:
	FILE* filei = fopen("c:\\pdf\\somepdf.pdf", "rb");

	if (filei && fileo)
	{
		//Get the file length:
		int fseekres = fseek(filei, 0, SEEK_END);   //fseek==0 if ok
		long filelen = ftell(filei);
		fseekres = fseek(filei, 0, SEEK_SET);

		//Read ethe ntire file into memory (!):
		char* buffer = new char[filelen]; ZeroMemory(buffer, filelen);
		size_t actualread = fread(buffer, filelen, 1, filei);  //must return 1

		bool morestreams = true;

		//Now search the buffer repeated for streams of data:
		while (morestreams)
		{
			//Search for stream, endstream. We ought to first check the filter
			//of the object to make sure it if FlateDecode, but skip that for now!
			size_t streamstart = FindStringInBuffer(buffer, "stream", filelen);
			size_t streamend = FindStringInBuffer(buffer, "endstream", filelen);
			if (streamstart > 0 && streamend > streamstart)
			{
				//Skip to beginning and end of the data stream:
				streamstart += 6;

				if (buffer[streamstart] == 0x0d && buffer[streamstart + 1] == 0x0a) streamstart += 2;
				else if (buffer[streamstart] == 0x0a) streamstart++;

				if (buffer[streamend - 2] == 0x0d && buffer[streamend - 1] == 0x0a) streamend -= 2;
				else if (buffer[streamend - 1] == 0x0a) streamend--;

				//Assume output will fit into 10 times input buffer:
				size_t outsize = (streamend - streamstart) * 10;
				char* output = new char[outsize]; ZeroMemory(output, outsize);

				//Now use zlib to inflate:
				z_stream zstrm; ZeroMemory(&zstrm, sizeof(zstrm));

				zstrm.avail_in = streamend - streamstart + 1;
				zstrm.avail_out = outsize;
				zstrm.next_in = (Bytef*)(buffer + streamstart);
				zstrm.next_out = (Bytef*)output;

				int rsti = inflateInit(&zstrm);
				if (rsti == Z_OK)
				{
					int rst2 = inflate(&zstrm, Z_FINISH);
					if (rst2 >= 0)
					{
						//Ok, got something, extract the text:
						size_t totout = zstrm.total_out;
						ProcessOutput(fileo, output, totout);
					}
				}
				delete[] output; output = 0;
				buffer += streamend + 7;
				filelen = filelen - (streamend + 7);
			}
			else
			{
				morestreams = false;
			}
		}
		fclose(filei);
	}
	if (fileo) fclose(fileo);
	return 0;
}

#endif


vars simplifyTitle(const char *str, BOOL super = TRUE)
{
	vars ret(str);
	ret.MakeUpper();
	ret.Replace("&AMP;", "&");
	ret.Replace("&#39;", "'");
	ret.Replace(",", " ");
	ret.Replace(";", " ");
	ret.Replace(".", " ");
	ret.Replace("MP3", " ");
	ret.Replace("+", " ");
	ret.Replace(" FT ", " FEAT ");
	ret.Replace(" FEATURING ", " FEAT ");
	ret.Replace(" AND ", "&");
	if (super)
	{
		ret.Replace("-", " ");
		ret.Replace("&", " ");
		ret.Replace("'", "");
		ret.Replace(" THE ", " ");
		if (IsSimilar(ret, "THE "))
			ret = ret.Mid(4);
	}

	int ft = ret.indexOf(" FEAT ");
	if (ft >= 0)
		ret = ret.Mid(0, ft);

	while (ret.Replace("  ", " "));
	return ret.Trim();
}


void AddTrack(CSymList &symlist, const char *_title, const char *_artist, double date, const char *_group)
{
	if (!_title || !*_title)
		return;

	vars file = simplifyTitle(vars(_title), FALSE) + " - " + simplifyTitle(vars(_artist), FALSE);
	vars link = "http://musicpleer.co/#!" + simplifyTitle(file);
	CSym sym(link.replace(" ", "+"), file);
	sym.SetStr(ITEM_LAT, simplifyTitle(_title));
	sym.SetStr(ITEM_LNG, simplifyTitle(_artist));
	sym.SetDate(ITEM_NEWMATCH, date);
	sym.SetStr(ITEM_MATCH, _group);
	sym.SetStr(ITEM_ACA, "Hyperlink(\"" + link + "\")");
	//ASSERT(strstr(sym.GetStr(ITEM_DESC), "-"));
	//ASSERT(!strstr(sym.GetStr(ITEM_DESC), "HOW DEEP"));
	int found = symlist.Find(sym.id);
	if (found < 0)
		symlist.Add(sym);
	else
	{
		// update
		CSym &osym = symlist[found];
		if (osym.GetDate(ITEM_NEWMATCH) < date)
			osym.SetDate(ITEM_NEWMATCH, date);
		if (osym.GetStr(ITEM_LAT).IsEmpty())
			osym.SetStr(ITEM_LAT, sym.GetStr(ITEM_LAT));
		if (osym.GetStr(ITEM_LNG).IsEmpty())
			osym.SetStr(ITEM_LNG, sym.GetStr(ITEM_LNG));
		vara g(osym.GetStr(ITEM_MATCH), ";");
		if (g.indexOf(_group) < 0)
		{
			g.push(_group);
			osym.SetStr(ITEM_MATCH, g.join(";"));
		}
	}
}


int GetTop40US(CSymList &symlist, const char *year)
{
	const char *group = "Top40US";

	DownloadFile f;
	// get list
	vars ubase = "www.at40.com";
	vars url = burl(ubase, year ? MkString("top-40/%s", year) : "top-40");
	if (f.Download(url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}

	vara table(ExtractString(f.memory, "id=\"pagtable\"", "", "</table"), "href=");

	for (int t = 0; t < table.length(); ++t)
	{
		if (t > 0)
		{
			url = burl(ubase, ExtractString(table[t], "", "\"", "\""));
			if (f.Download(url))
			{
				Log(LOGERR, "ERROR: can't download url %.128s", url);
				return FALSE;
			}
		}

		vara urls(f.memory, "class=\"chartintlist2\"");
		for (int u = 1; u < urls.length(); ++u)
		{
			vars url = ExtractString(urls[u], "href=", "\"", "\"");
			vars date = ExtractString(urls[u], "href=", ">", "</a");
			double vdate = CDATA::GetDate(date, "MMM D, YYYY");
			if (url.IsEmpty())
				continue;


			url = burl(ubase, url);
			if (f.Download(url))
			{
				Log(LOGERR, "ERROR: can't download url %.128s", url);
				continue;
			}

			vara list(ExtractString(f.memory, "\"charttableint\"", "", "</table"), "chart_song");
			printf("%d : %s %d %s [%s]     \r", symlist.GetSize(), group, list.length(), CDate(vdate), date);
			for (int i = 1; i < list.length(); ++i)
			{
				vars artist = stripHTML(ExtractString(list[i], ">", "", "<br"));
				vars title = stripHTML(ExtractString(list[i], "<br", ">", "</td"));
				vars link = "http://musicpleer.co/#!" + artist + " " + title;
				AddTrack(symlist, title, artist, vdate, group);
			}
		}
	}

	return TRUE;
}


int GetTop100US(CSymList &symlist, const char *year)
{
	const char *group = "Top100US";

	DownloadFile f;
	// get list
	vars ubase = "www.billboard.com";
	vars url = burl(ubase, "charts/hot-100");

	vars pyear = "XXX";
	if (year != NULL)
	{
		int y = atoi(year);
		if (y < 2000)
		{
			Log(LOGERR, "Bad year specified %s", year);
			return FALSE;
		}
		pyear = MkString("%d", y - 1);
	}

	while (TRUE)
	{
		if (f.Download(url))
		{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			return FALSE;
		}

		vars date = ExtractString(f.memory, "datetime=", "\"", "\"");
		double vdate = CDATA::GetDate(date, "YYYY-MM-DD");
		if (strstr(date, pyear))
			break;

		vara list(f.memory, "class=\"chart-row__title\"");
		printf("%d : %s %d %s [%s]     \r", symlist.GetSize(), group, list.length(), CDate(vdate), date);
		for (int i = 1; i < list.length(); ++i)
		{
			vars artist = stripHTML(ExtractString(list[i], "<h3", ">", "</h3"));
			vars title = stripHTML(ExtractString(list[i], "<h2", ">", "</h2"));
			AddTrack(symlist, title, artist, vdate, group);
		}

		if (!year)
			break;

		// next url
		vars nav = ExtractString(f.memory, "class=\"chart-nav\"", ">", "</nav");
		url = burl(ubase, ExtractString(nav, "href=", "\"", "\""));
	}

	return TRUE;
}


int GetTopUK(CSymList &symlist, const char *year, const char *uurl, const char *group)
{
	DownloadFile f;
	// get list
	vars ubase = "www.officialcharts.com";
	vars url = burl(ubase, uurl);

	vars pyear = "XXX";
	if (year != NULL)
	{
		int y = atoi(year);
		if (y < 2000)
		{
			Log(LOGERR, "Bad year specified %s", year);
			return FALSE;
		}
		pyear = MkString("%d", y - 1);
	}

	while (TRUE)
	{
		if (f.Download(url))
		{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			return FALSE;
		}

		vars date = stripHTML(ExtractString(f.memory, "class=\"article-date\"", ">", "<"));
		double vdate = CDATA::GetDate(date, "D MMM YYYY");
		if (strstr(date, pyear))
			break;

		vara list(f.memory, "class=\"track\"");
		printf("%d : %s %d %s [%s]     \r", symlist.GetSize(), group, list.length(), CDate(vdate), date);
		for (int i = 1; i < list.length(); ++i)
		{
			vars artist = stripHTML(ExtractString(list[i], "class=\"artist\"", ">", "</div"));
			vars title = stripHTML(ExtractString(list[i], "class=\"title\"", ">", "</div"));
			AddTrack(symlist, title, artist, vdate, group);
		}

		if (!year)
			break;

		// next url
		vara urls(f.memory, "href=");
		for (int i = 1; i < urls.length(); ++i)
			if (strstr(urls[i], ">prev<"))
				url = burl(ubase, ExtractString(urls[i], "", "\"", "\""));

	}

	return TRUE;
}


int GetTopDance(CSymList &symlist, const char *year, const char *uurl, const char *group)
{
	vars global = "DanceTop40";
	DownloadFile f;
	// get list
	vars ubase = "www.dancetop40.com";
	vars url = burl(ubase, uurl);
	if (year)
	{
		url = burl(ubase, MkString("history/year/%s", year));
		group = global;
	}

	int page = 1;
	while (TRUE)
	{
		if (f.Download(url))
		{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			return FALSE;
		}

		vara list(f.memory, "<h4");
		for (int i = 1; i < list.length(); ++i)
		{
			vars artist = stripHTML(ExtractString(list[i], "by <a ", ">", "</a"));
			vars title = stripHTML(ExtractString(list[i], "", ">", "</h4"));
			vars ago = stripHTML(ExtractString(list[i], ">released", ">", "</"));
			double nago = atoi(ago);
			if (strstr(ago, "year"))
				nago *= 12 * 30;
			else if (strstr(ago, "month"))
				nago *= 30;
			else if (strstr(ago, "week"))
				nago *= 7;
			else if (strstr(ago, "day"))
				nago *= 1;
			else
			{
				Log(LOGERR, "unknown unit '%s' for %s - %s", ago, title, artist);
			}
			double vdate = Date(COleDateTime::GetCurrentTime() - COleDateTimeSpan((int)nago, 0, 0, 0));
			printf("%d : %s %d %s     \r", symlist.GetSize(), group, list.length(), CDate(vdate));
			AddTrack(symlist, title, artist, vdate, group);
		}

		if (!year)
			break;

		// next url
		++page;
		url = burl(ubase, MkString("history/year/%s/%d", year, page));
		if (page > 5)
			break;
	}

	return TRUE;
}


int DownloadTop40(const char *mp3file, const char *mp3title, const char *mp3artist)
{
	DownloadFile f;
	vars txt = vars(mp3title) + " " + vars(mp3artist);
	vars search = "http://databrainz.com/api/search_api.cgi?qry=" + txt.replace(" ", "+") + "&format=json&mh=50&where=mpl";
	if (f.Download(search))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", search);
		return FALSE;
	}

	vara urls(f.memory, "\"url\":");
	for (int u = 1; u < urls.length() && !CFILE::exist(mp3file); ++u)
	{
		vars id = ExtractString(urls[u], "", "\"", "\"");
		vars url = "http://databrainz.com/api/data_api_new.cgi?id=" + id + "&r=mpl&format=json";
		if (f.Download(url))
		{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			continue;
		}

		vars furl = ExtractString(f.memory, "\"url\":", "\"", "\"");
		vars title = stripHTML(ExtractString(f.memory, "\"title\":", "\"", "\""));
		vars artist = stripHTML(ExtractString(f.memory, "\"artist\":", "\"", "\""));
		if (!IsSimilar(simplifyTitle(title), simplifyTitle(mp3title)) || !IsSimilar(simplifyTitle(artist), simplifyTitle(mp3artist)))
			continue;

		// download mp3
		DownloadFile ff(TRUE);
		if (ff.Download(furl))
		{
			printf("ERROR: Trying another download source for %.128s", mp3file);
			continue;
		}

		// save mp3
		CFILE cf;
		if (cf.fopen(mp3file, CFILE::MWRITE))
		{
			cf.fwrite(ff.memory, 1, ff.size);
			cf.fclose();
			return TRUE;
		}
		return FALSE;
	}

	return FALSE;
}


int cmptrack(const void *arg1, const void *arg2)
{
	CString c1 = (*(CSym**)arg1)->GetStr(ITEM_NEWMATCH);
	CString c2 = (*(CSym**)arg2)->GetStr(ITEM_NEWMATCH);
	return strcmp(c2, c1);
}


int ExistTop40(CSymList &filelist, const char *mp3file)
{
	int found = filelist.Find(GetFileName(mp3file));
	if (found >= 0 && strcmp(filelist[found].data, mp3file) != 0)
	{
		MoveFile(filelist[found].data, mp3file);
		return TRUE;
	}
	return found >= 0;
}


int GetTop40(const char *year, const char *pattern)
{
	vars folder = "D:\\Music\\00+Top\\";
	if (year != NULL && *year != 0 && CDATA::GetNum(year) <= 0)
		folder = year;

	vars file = folder + "top40.csv";

	CSymList symlist;
	symlist.Load(file);

	if (MODE == -1 || MODE == 0)
	{
		// get list
		GetTopDance(symlist, year, "ES", "DanceTop40ES");
		GetTopDance(symlist, year, "UK", "DanceTop40UK");
		GetTopDance(symlist, year, "US", "DanceTop40US");
		GetTopDance(symlist, year, "IT", "DanceTop40IT");

		GetTop40US(symlist, year);
		//GetTop100US(symlist, year);
		//GetTopUK(symlist, year, "charts", "Top100UK");
		GetTopUK(symlist, year, "charts/uk-top-40-singles-chart/", "Top40UK");
		GetTopUK(symlist, year, "charts/dance-singles-chart/", "TopDance40UK");
	}


	symlist.Sort(cmptrack);
	symlist.Save(file);

	if (MODE == -1 || MODE == 1)
	{

		CSymList filelist;
		const char *exts[] = { "MP3", NULL };
		GetFileList(filelist, folder, exts, TRUE);
		for (int j = 0; j < filelist.GetSize(); ++j)
		{
			vars filename = filelist[j].id.MakeUpper();
			vars id = GetFileName(filename);
			vars data = folder + id + ".mp3";
			filelist[j] = CSym(id, data);
			if (filename != data)
				MoveFile(filename, data);
		}
		filelist.Sort();

		DownloadFile f;
		// download mp3

		for (int p = 0; p < 2; ++p)
			for (int i = 0; i < symlist.GetSize(); ++i)
			{
				CSym &sym = symlist[i];
				vars mp3desc = sym.GetStr(ITEM_DESC);
				double mp3date = sym.GetDate(ITEM_NEWMATCH);
				vara mp3sdate(sym.GetStr(ITEM_NEWMATCH), "-");
				if (mp3sdate.length() < 3)
				{
					Log(LOGERR, "Invalid date '%s' for %s'", sym.GetStr(ITEM_NEWMATCH), sym.id);
					continue;
				}
				double index = 12 * 2050 - 12 * atoi(mp3sdate[0]) - atoi(mp3sdate[1]);
				vars match = sym.GetStr(ITEM_MATCH);
				vars group = strstr(match, "Dance") ? "Dance40" : (strstr(match, "40") ? "Top40" : "TopMore");
				vars mp3folder = folder + group + "\\";
				CreateDirectory(mp3folder, NULL);
				mp3folder += MkString("%03d %s %s", (int)index, CDATA::GetMonth(atoi(mp3sdate[1])), mp3sdate[0]) + "\\";
				CreateDirectory(mp3folder, NULL);
				vars mp3file = mp3folder + mp3desc + ".mp3";
				vars mp3title = sym.GetStr(ITEM_LAT);
				vars mp3artist = sym.GetStr(ITEM_LNG);

				if (!ExistTop40(filelist, mp3file) && p > 0)
				{
					if (pattern && !strstr(match, pattern))
						continue;
					printf("%d/%d Downloading %s        \r", i, symlist.GetSize(), mp3file);
					if (!CFILE::exist(mp3file))
						if (!DownloadTop40(mp3file, mp3title, mp3artist))
							Log(LOGERR, "Could not download %s", mp3file);
				}

				if (CFILE::exist(mp3file))
				{
					// set date
					COleDateTime dt(mp3date);
					SYSTEMTIME sysTime;
					dt.GetAsSystemTime(sysTime);
					FILETIME time;
					int err = !SystemTimeToFileTime(&sysTime, &time);
					if (!err) err = !CFILE::settime(mp3file, &time);
					if (err)
						Log(LOGERR, "Error setting file time for %s", mp3file);
				}
			}
	}
	return TRUE;
}


int DownloadBetaKML(int mode, const char *filecsv)
{
	CSymList rwnamelist;
	DownloadFile f;
	LoadRWList();
	LoadNameList(rwnamelist);
	vars folder = vars(KMLFOLDER) + "\\";

	// load from file
	if (filecsv)
	{
		CSymList list;
		list.Load(filecsv);
		for (int i = 0; i < list.GetSize(); ++i)
		{
			if (!list[i].GetStr(ITEM_NEWMATCH).Trim().IsEmpty())
				continue;

			vars kmlfile = list[i].GetStr(ITEM_KML);
			if (kmlfile.Trim().IsEmpty())
				continue;

			// map kmlfile
			vars match = list[i].GetStr(ITEM_MATCH);
			const char *rwid = strstr(match, RWID);
			if (!rwid)
			{
				Log(LOGERR, "Invalid RWID '%s'", match);
				continue;
			}
			vars id = RWID + GetToken(rwid, 1, ": ");
			int found = rwlist.Find(id);
			if (found < 0)
			{
				Log(LOGERR, "Invalid RWID '%s'", match);
				continue;
			}

			// download kmlfile
			vars lockmlfile = folder + rwlist[found].GetStr(ITEM_DESC) + ".kml";
			if (IsSimilar(kmlfile, "http"))
			{
				if (f.Download(kmlfile, lockmlfile))
					Log(LOGERR, "Could not download kml from %s to %s", kmlfile, lockmlfile);
				continue;
			}

			// download msid
			printf("Downloading %s         \r", lockmlfile);
			inetfile out(lockmlfile);
			KMZ_ExtractKML("", "http://maps.google.com/maps/ms?ie=UTF8&hl=en&msa=0&output=kml&msid=" + kmlfile, &out);
		}
		return TRUE;
	}

	// download new kmls
	vars file = filename("file");

	CSymList filelist;
	filelist.header = "FILE,MODIFICATION DATE";
	filelist.Load(file);

	CSymList newlist;
	newlist.header = filelist.header;
	ROPEWIKI_DownloadKML(newlist);
	filelist.header = newlist.header;

	for (int i = 0; i < newlist.GetSize(); ++i)
		if (filelist.Find(newlist[i].id) < 0)
			filelist.Add(newlist[i]);
	filelist.Save(file);

	if (MODE < -1)
		newlist = filelist;

	vara specials("DEM,Null,Scott Swaney's Death Valley Canyons,Fire closure area,Mount Wilson mountain biking");

	if (MODE < 0)
	{
		for (int i = 0; i < newlist.GetSize(); ++i)
		{
			CString name = newlist[i].id.Mid(5, newlist[i].id.GetLength() - 5 - 4);
			if (specials.indexOf(name) >= 0)
				continue;
			int found = rwnamelist.Find(name);
			if (found < 0)
			{
				Log(LOGERR, "Orphaned KML file '%s'", ROPEWIKIFILE + name + ".kml");
				continue;
			}
			int found2 = rwlist.Find(rwnamelist[found].data);
			if (found2 < 0)
			{
				Log(LOGERR, "Orphaned2 KML file '%s'", name);
				continue;
			}
			vars kmlfile = rwlist[found2].GetStr(ITEM_KML);
			if (!strstr(kmlfile, "ropewiki.com"))
			{
				Log(LOGERR, "Bad ITEM_KMLFILE for '%s' (%s)", name, kmlfile);
				continue;
			}
			vars lockmlfile = folder + GetFileNameExt(kmlfile.replace("/", "\\"));
			printf("Downloading %s         \r", lockmlfile);
			if (f.Download(kmlfile, lockmlfile) || !CFILE::exist(lockmlfile))
			{
				Log(LOGERR, "Could not download %s to %s", kmlfile, lockmlfile);
				continue;
			}
		}

	}

	return TRUE;
}


int FixBetaKML(int mode, const char *transfile)
{
	//@@@@ TODO
	return FALSE;

	CSymList rwnamelist;

	LoadRWList();
	LoadNameList(rwnamelist);

	CSymList trans, ctrans;
	ctrans.Load(filename("trans-colors"));

	if (IsSimilar(transfile, "0x"))
	{
		//@@@transcolor = transfile+2;
		transfile = NULL;
	}

	if (transfile)
		trans.Load(transfile);

	CSymList files, kmlrwlist;
	const char *exts[] = { "KML", NULL };
	//if (filecsv)
	//	files.Load(filecsv);
	//else
	GetFileList(files, KMLFOLDER, exts, FALSE);

	// list of kmls
	for (int n = 0; n < files.GetSize(); ++n)
	{
		vars file = files[n].id;

		CFILE f;
		vars memory;
		if (!f.fopen(file))
		{
			Log(LOGERR, "Can't read %s", file);
			continue;
		}

		int len = f.fsize();
		char *mem = (char *)malloc(len + 1);
		f.fread(mem, len, 1);
		mem[len] = 0;
		memory = mem;
		free(mem);
		f.fclose();

		// process
		vars omemory = memory;
		vars memory2 = memory.replace("\"", "'");
		LL dse[2], ase[2], ese[2];

		enum { D_A, D_D, D_E, D_MAX };
		const char *names[] = { "Approach" , "Descent" , "Exit", NULL };

		LL se[3][2];
		int done[D_MAX];
		double length[D_MAX], depth[D_MAX];
		for (int d = 0; d < D_MAX; ++d)
		{
			length[d] = depth[d] = InvalidNUM;
			done[d] = 0;
		}

		if (TMODE == 1) // GBR -> GRY
		{
			vars sep = "<LineStyle>", sep2 = "</LineStyle>";
			vara placemarks(memory, sep);
			for (int p = 1; p < placemarks.length(); ++p)
			{
				vara pl(placemarks[p], sep2);
				double w = ExtractNum(pl[0], "<width>", "", "</width>");
				vars color = ExtractString(pl[0], "<color>", "", "</color>");
				int t;
				for (t = 0; t < ctrans.GetSize(); ++t)
				{
					if (ctrans[t].id[0] != ';')
					{
						CString &cstart = ctrans[t].id;
						if (IsSimilar(color, cstart))
						{
							// translate
							color = ctrans[t].GetStr(0);
							break;
						}
						CString cend = ctrans[t].GetStr(0);
						if (IsSimilar(color, cend))
						{
							// already translated
							break;
						}
					}
				}
				if (t >= ctrans.GetSize())
					Log(LOGERR, "No color translation: '%s' from %s", color, file);
				pl[0] = MkString("<color>%s</color><width>%g</width>", color, (w > 5 || w <= 0) ? 5 : w);
				placemarks[p] = pl.join(sep2);
			}
			memory = placemarks.join(sep);
		}

		vars sep = "<Placemark>", sep2 = "</Placemark>";
		vara placemarks(memory, sep);
		for (int p = 1; p < placemarks.length(); ++p)
		{
			vara pl(placemarks[p], sep2);

			if (transfile)
			{
				// translate name
				vars sep = "<name>", sep2 = "</name>";
				vara names(pl[0], sep);

				for (int i = 1; i < names.length(); ++i)
				{
					int tr = 0;
					vara line(names[i], sep2);
					if (names[0].IsEmpty())
						continue;

					int t;
					for (t = 0; t < trans.GetSize(); ++t)
						if (!trans[t].id.Trim().IsEmpty())
						{
							if (IsSimilar(line[0], trans[t].data))
							{
								// already translated
								break;
							}
							if (IsSimilar(line[0], trans[t].id))
							{
								// translate
								line[0] = trans[t].GetStr(0) + line[0].Mid(trans[t].id.GetLength());
								break;
							}
						}

					if (t >= trans.GetSize())
						Log(LOGWARN, "No name translation: '%s' from %s", line[0], file);

					names[i] = line.join(sep2);
				}
				pl[0] = names.join(sep);
			}

			// eliminate styles for lines
			if (pl[0].Find("<LineString>") > 0)
			{
				vars style = ExtractStringDel(pl[0], "<styleUrl>", "#", "</styleUrl>");
				if (!style.IsEmpty())
				{
					vars styledef, ostyle = style;
					while (styledef.IsEmpty() && !style.IsEmpty())
					{
						styledef = ExtractString(memory2, "<Style id='" + style + "'", ">", "</Style>");
						vars stylemap = ExtractString(memory2, "<StyleMap id='" + style + "'", ">", "</StyleMap>");
						if (!stylemap.IsEmpty())
							style = ExtractString(stylemap, "<styleUrl>", "#", "</styleUrl>");
					}
					if (styledef.IsEmpty())
					{
						Log(LOGERR, "Could not remap LineString style #%s", ostyle);
						styledef = ostyle;
					}
					else
						styledef = "<Style>" + styledef + "</Style>";
					int linestring = pl[0].Find("<LineString>");
					pl[0].Insert(linestring, styledef.Trim());
				}

				// check colors are appropriate
				vars name = ExtractString(pl[0], "<name>", "", "</name>");
				// get standard color
				vars color = ExtractString(pl[0], "<color>", "", "</color>");
				unsigned char abgr[8];
				FromHex(color, abgr);
				for (int c = 0; c < 4; ++c)
				{
					double FFCLASS = 2;
					double v = round(abgr[c] / 255.0*FFCLASS);
					double iv = round(255 * v / FFCLASS);
					abgr[c] = iv > 0xFF ? 0xFF : (int)iv;
				}
				vars scolor = ToHex(abgr, 4);

				int ok = FALSE, fixed = FALSE;
				int standards = ctrans.Find(";STANDARD");
				// check
				for (int i = standards + 1; !ok && i < ctrans.GetSize(); ++i)
				{
					if (IsSimilar(scolor.Right(6), ctrans[i].id.Right(6)))
					{
						vara match(ctrans[i].data);
						for (int m = 1; !ok && m < match.length(); ++m)
							if (match[m] == "*" || strstr(name, match[m]) || strstr(name, match[m].lower()))
								ok = TRUE;
					}
				}
				// fix
				for (int i = standards + 1; !ok && !fixed && i < ctrans.GetSize(); ++i)
				{
					vara match(ctrans[i].data);
					for (int m = 1; !ok && !fixed && m < match.length(); ++m)
						if (strstr(name, match[m]) || strstr(name, match[m].lower()))
							fixed = pl[0].Replace("<color>" + color + "</color>", "<color>" + ctrans[i].id + "</color>");
				}

				if (!ok)
					Log(fixed ? LOGWARN : LOGERR, "Inconsistent color/name %s %s (%s) %s in %s", fixed ? "FIXED!" : "?", scolor, color, name, file);

				// @@@check proper direction approach/exit
				// @@@compute elevation
				// @@@compute distance
				// @@@compute and fix coordinates
				for (int d = 0; d < 3; ++d)
				{
					if (IsSimilar(name, names[d]) && done[d] >= 0)
					{
						++done[d];
						if (name == names[d])
							done[d] = -1;
						//ASSERT(!strstr(file, "Ringpin"));
						if (!ComputeCoordinates(pl[0], length[d], depth[d], se[d]))
							Log(LOGERR, "Could not comput %s coords for %s", names[d], file);
					}
				}
			}

			pl[0].Replace("<description></description>", "");
			placemarks[p] = pl.join(sep2);
		}
		memory = placemarks.join(sep);

		if (omemory != memory)
		{
			// WRITE
			vars name = GetFileNameExt(file);
			file = vars(KMLFIXEDFOLDER) + "\\" + name;
			if (!f.fopen(file, CFILE::MWRITE))
			{
				Log(LOGERR, "Can't write %s", file);
				continue;
			}
			f.fwrite(memory, 1, memory.GetLength());
			f.fclose();

			printf("Fixed %s       \r", file);
		}

		// accumunate stats
		vars name = GetFileName(file);
		name.Replace("_", " ");
		int found = rwnamelist.Find(name);
		if (found < 0)
		{
			Log(LOGERR, "Orphaned KML file '%s'", ROPEWIKIFILE + name + ".kml");
			continue;
		}
		int found2 = rwlist.Find(rwnamelist[found].data);
		if (found2 < 0)
		{
			Log(LOGERR, "Orphaned2 KML file '%s'", name);
			continue;
		}
		CSym &osym = rwlist[found2];
		CSym sym(osym.id, osym.GetStr(0));
		for (int d = 0; d < D_MAX; ++d)
			if (done[d] > 1)
				length[d] = depth[d] = InvalidNUM;
		if (length[D_D] != InvalidNUM && osym.GetNum(ITEM_LENGTH) == InvalidNUM)
			sym.SetStr(ITEM_LENGTH, MkString("%sm", CData(round(length[D_D]))));
		if (depth[D_D] != InvalidNUM && osym.GetNum(ITEM_DEPTH) == InvalidNUM)
			sym.SetStr(ITEM_DEPTH, MkString("%sm", CData(round(depth[D_D]))));
		if (osym.GetNum(ITEM_HIKE) == InvalidNUM)
			if (length[D_D] != InvalidNUM && length[D_A] != InvalidNUM && length[D_E] != InvalidNUM)
				sym.SetStr(ITEM_HIKE, MkString("%sm", CData(round(length[D_D] + length[D_A] + length[D_E]))));
		if (strlen(GetTokenRest(sym.data, 2)) > 0)
			kmlrwlist.Add(sym);
	}

	kmlrwlist.Save(filename("kml"));

	return TRUE;
}


int UploadBetaKML(int mode, const char *filecsv)
{
	CSymList files;
	const char *exts[] = { "KML", NULL };
	//if (filecsv)
	//	files.Load(filecsv);
	//else
	GetFileList(files, KMLFIXEDFOLDER, exts, FALSE);

	//loginstr = vara("Barranquismo.net,sentomillan");
	DownloadFile f;
	if (!Login(f))
		return FALSE;

	for (int i = 0; i < files.GetSize(); ++i)
	{
		CPage up(f, NULL, NULL, NULL);
		vars rfile = GetFileNameExt(files[i].id);
		if (up.FileExists(rfile))
			if (MODE > -2)
			{
				Log(LOGWARN, "%s already exists, skipping", rfile);
				continue;
			}
			else
			{
				Log(LOGWARN, "%s already exists, OVERWRITING", rfile);
			}

		if (!up.UploadFile(files[i].id, rfile))
			Log(LOGERR, "ERROR uploading %s", rfile);
	}

	return TRUE;
}


int cmpprice(const void *arg1, const void *arg2)
{
	const char *s1 = strstr(((CSym**)arg1)[0]->data, "$");
	const char *s2 = strstr(((CSym**)arg2)[0]->data, "$");
	double p1 = s1 ? CGetNum(s1) : 0;
	double p2 = s2 ? CGetNum(s2) : 0;
	if (p1 > p2) return 1;
	if (p1 < p2) return -1;
	return 0;
}


int GetCraigList(const char *keyfile, const char *htmfile)
{

	CSymList creglist, keylist, donelist;
	vars donefile = GetFileNoExt(keyfile) + ".his";
	vars outfile = GetFileNoExt(keyfile) + ".htm";
	if (htmfile)
		outfile = htmfile;

	keylist.Load(keyfile);
	donelist.Load(donefile);

	/*
		reglist.Load(regfile ? regfile : filename("craigregion"));
		for (int r=reglist.GetSize()-1; r>=0; --r)
			{
			vars reg = ExtractString(reglist[r].id, "//", "", ".craig");
			if (reg.IsEmpty() || reglist.Find(reg)>=0)
				reglist.Delete(r);
			else
				reglist[r].id = reg;
			}
	*/

	DownloadFile f;
	if (f.Download("http://www.craigslist.org/about/areas.json"))
	{
		Log(LOGERR, "ERROR: can't download craiglist area info");
		return FALSE;
	}
	vara list(f.memory, "{");
	for (int i = 1; i < list.length(); ++i)
	{
		vars &item = list[i];
		vars name = ExtractString(item, "\"name\"", "\"", "\"");
		vars host = ExtractString(item, "\"hostname\"", "\"", "\"");
		double lat = ExtractNum(item, "\"lat\"", "\"", "\"");
		double lng = ExtractNum(item, "\"lon\"", "\"", "\"");
		if (host.IsEmpty() || !CheckLL(lat, lng))
		{
			Log(LOGERR, "Invalid hostname/coords for %s", list[i]);
			continue;
		}

		CSym sym(host, name);
		sym.SetNum(ITEM_LAT, lat);
		sym.SetNum(ITEM_LNG, lng);
		if (creglist.Find(host) < 0)
			creglist.Add(sym);
	}

	// get list
	vars start, end;
	CSymList idlist;
	for (int k = 0; k < keylist.GetSize(); ++k)
	{
		CSymList ridlist, reglist;

		// match regions to geoloc < dist
		vara geolist(keylist[k].data);
		for (int g = 1; g < geolist.length(); ++g)
		{
			double lat, lng, dist;
			vara geo(geolist[g], "<");
			if (geo.length() < 2 || (dist = CDATA::GetNum(geo[1])) <= 0 || !_GeoCache.Get(geo[0], lat, lng))
			{
				Log(LOGERR, "Invalid geoset 'loc < dist' %s", geolist[g]);
				continue;
			}

			LLRect rect(lat, lng, dist / km2mi * 1000);
			for (int r = 0; r < creglist.GetSize(); ++r)
			{
				CSym &sym = creglist[r];
				if (rect.Contains(sym.GetNum(ITEM_LAT), sym.GetNum(ITEM_LNG)))
					if (reglist.Find(sym.id) < 0)
						reglist.Add(sym);
			}
		}

		// search regions
		for (int r = 0; r < reglist.GetSize(); ++r)
		{
			static double ticks = 0;
			printf("%d %d%% %s %d/%d %s %d/%d       \r", idlist.GetSize() + ridlist.GetSize(), ((k*reglist.GetSize() + r) * 100) / (reglist.GetSize()*keylist.GetSize()), keylist[k].id, k, keylist.GetSize(), reglist[r].id, r, reglist.GetSize());

			vars url = MkString("http://%s.craigslist.org/search/sss?query=%s", reglist[r].id, keylist[k].id); //&sort=rel&min_price=100&max_price=3000

			Throttle(ticks, 1000);
			if (f.Download(url))
			{
				Log(LOGERR, "ERROR: can't download url %.128s", url);
				continue;
			}

			vars START = "<p", END = "</p>";

			vars memory = f.memory;
			int r1 = memory.Replace("href=\"//", "href=\"http://");
			int r2 = memory.Replace("href=\"/", MkString("href=\"http://%s.craigslist.org/", reglist[r].id));

			vars defloc = ExtractString(memory, "https://post.craigslist.org/c/", "", "\"");

			vara list(memory, START);
			if (list.length() <= 1)
				continue;

			int mid = 0;
			start = list[0];
			for (int i = 1; i < list.length(); ++i)
			{
				mid = list[i].Find(END);
				if (mid < 0)
				{
					Log(LOGERR, "ERROR: malformed item %.128s", list[i]);
					continue;
				}
				mid += END.length();
				vars item = START + list[i].Mid(0, mid);
				vars id = ExtractString(item, "data-pid=", "\"", "\"");
				if (idlist.Find(id) >= 0 || ridlist.Find(id) >= 0)
					continue;

				vars pic;
				vara data(ExtractString(item, "data-ids=", "\"", "\""), ",");
				if (data.length() > 0)
				{
					pic = GetToken(data[0], 1, ':');

					vars a = "</a>";
					int found = item.Find(a);
					if (found > 0)
						item.Insert(found, MkString("<img src=\"https://images.craigslist.org/%s_300x300.jpg\"</img>", pic));
				}

				// avoid repeated ads with same text and same pic
				vara locs(stripHTML(ExtractString(item, "<small>", "", "</small>")).Trim("() "), ">");
				vars loc = locs.last().trim();
				if (loc.IsEmpty())
					loc = defloc;
				vars text = pic + ":" + loc + ":" + stripHTML(ExtractString(item, "data-id=", ">", "<"));
				if (keylist[k].GetNum(0) < 0)
					if (MODE <= 0 && donelist.Find(text) >= 0)
						continue;

				vars link = ExtractString(item, "href=", "\"", "\"");
				donelist.Add(CSym(text, link));

				ridlist.Add(CSym(id, item));
			}
			end = list.last().Mid(mid);
		}

		// sort by price
		ridlist.Sort(cmpprice);
		idlist.Add(ridlist);
	}

	donelist.Save(donefile);

	CFILE cf;
	if (cf.fopen(outfile, CFILE::MWRITE))
	{
		cf.fwrite(start, start.GetLength(), 1);
		for (int i = 0; i < idlist.GetSize(); ++i)
			cf.fwrite(idlist[i].data, idlist[i].data.GetLength(), 1);
		cf.fwrite(end, end.GetLength(), 1);
		cf.fclose();
	}

	if (!htmfile)
		system(outfile);

	return TRUE;
}
