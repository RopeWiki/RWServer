
#include <afxinet.h>
#include <atlimage.h>
#include "stdafx.h"
#include "trademaster.h"
#include "rw.h"
#include "math.h"
#include "Xunzip.h"
#include "BetaC.h"
#include "BETAExtract.h"
#include "BluuGnome.h"
#include "Books.h"
#include "CBrennen.h"
#include "Code.h"
#include "DescenteCanyon.h"
#include "GeoRegion.h"
#include "RoadTripRyan.h"
#include "UKCanyonGuides.h"
#include "SuperAmazingMap.cpp"
#include "ToddMartin.cpp"

#include "passwords.h"

extern int INVESTIGATE;

#define KMLFOLDER "BETAKML"
#define KMLFIXEDFOLDER "BETAKML\\FIXED"
#define ROPEWIKIFILE "http://ropewiki.com/File:"

#define WIKILOC "wikiloc.com"

#define CHGFILE "CHG"
#define MATCHFILE "MATCH"
#define DIST15KM (25*1000)
#define DIST150KM (150*1000)
#define MAXGEOCODEDIST 50000 // 50km
#define MAXGEOCODENEAR 5 // nearby
#define REPLACEBETALINK FALSE
#define MAXWIKILOCLINKS 5

#define MINCHARMATCH 3


#define RWTITLE "RWTITLE:"
#define RWLINK "@"
#define RWBASE vars("http://ropewiki.com/")
#define DISAMBIGUATION " (disambiguation)"

#define KMLEXTRACT 1

const char *andstr[] = { " and ", " y ", " et ", " e ", " und ", "&", "+", "/", ":", " -", "- ", NULL };


//UTF:\xEF\xBB\xBF         to add columns use CheckBeta() 
static const char *headers ="ITEM_URL, ITEM_DESC, ITEM_LAT, ITEM_LNG, ITEM_MATCH, ITEM_MODE, ITEM_INFO, ITEM_REGION, ITEM_ACA, ITEM_RAPS, ITEM_LONGEST, ITEM_HIKE, ITEM_MINTIME, ITEM_MAXTIME, ITEM_SEASON, ITEM_SHUTTLE, ITEM_VEHICLE, ITEM_CLASS, ITEM_STARS, ITEM_AKA, ITEM_PERMIT, ITEM_KML, ITEM_CONDDATE, ITEM_AGAIN, ITEM_EGAIN, ITEM_LENGTH, ITEM_DEPTH, ITEM_AMINTIME, ITEM_AMAXTIME, ITEM_DMINTIME, ITEM_DMAXTIME, ITEM_EMINTIME, ITEM_EMAXTIME, ITEM_ROCK, ITEM_LINKS, ITEM_EXTRA, https://maps.googleapis.com/maps/api/geocode/xml?address=,\"=+HYPERLINK(+CONCATENATE(\"\"http://ropewiki.com/index.php/Location?jform&locsearchchk=on&locname=\"\",C1,\"\",\"\",D1,\"\"&locdist=5mi&skinuser=&sortby=-Has_rank_rating&kmlx=\"\",A1),\"\"@check\"\")\"";
static const char *rwprop =                     "Has pageid|Has coordinates|Has geolocation|Has BetaSites list|Has TripReports list|Has info|Has info major region|Has info summary|Has info rappels|Has longest rappel|Has length of hike|Has fastest typical time|Has slowest typical time|Has best season|Has shuttle length|Has vehicle type|Has location class|Has user counter|Has AKA|Requires permits|Has KML file|Has condition date|Has approach elevation gain|Has exit elevation gain|Has length|Has depth|Has fastest approach time|Has slowest approach time|Has fastest descent time|Has slowest descent time|Has fastest exit time|Has slowest exit time|Has rock type|Has SketchList"; //|Has user counter";
static const char *rwform =                     "=|=|=|=|=|=|Region|=ACA|Number of rappels|Longest rappel|Hike length|Fastest typical time|Slowest typical time|Best season|NeedsShuttle;Shuttle|Vehicle|Location type|=Stars|AKA|Permits|=KML|=COND|Approach elevation gain|Exit elevation gain|Length|Depth|Fastest approach time|Slowest approach time|Fastest descent time|Slowest descent time|Fastest exit time|Slowest exit time|Rock type|=SKETCH"; //|Has user counter";

static vara cond_stars("0 - Unknown,1 - Poor,2 - Ok,3 - Good,4 - Great,5 - Amazing");
enum { W_DRY=0, W_WADING=1, W_SWIMMING=2, W_VERYLOW=3, W_LOW=4, W_MODLOW=5, W_MODERATE=6, W_MODHIGH=7, W_HIGH=8, W_VERYHIGH=9, W_EXTREME=10 };
//static vara cond_water("0 - Dry,1 - Low flow,2 - Moderate flow,3 - High flow,4 - Very High flow,5 - Extreme flow");
//static vara cond_water("a1 - A - Dry,a2 - B - Very Low flow,a2+ - B+ - Deep pools,a2 - B - Very Low flow,a3 - B/C - Low flow,a4- - C1- - Moderate Low flow,a4 - C1 - Moderate flow,a4+ - C1+ - Moderate High flow,a5 - C2 - High flow,a6 - C3 - Very High flow,a7 - C4 - Extreme flow");


static vara cond_temp("0 - None,1 - Rain jacket (1mm-2mm),2 - Thin wetsuit (3mm-4mm),3 - Full wetsuit (5mm-6mm),4 - Thick wetsuit (7mm-10mm),5 - Drysuit");
//static vara cond_temp("0mm - None,1mm - Rain jacket,3mm - Thin wetsuit,5mm - Full wetsuit,7mm - Thick wetsuit,Xmm - Drysuit");


static vara cond_diff("0 - Nontechnical,1 - Easy,2 - Normal,3 - Special challenges,4 - Advanced,5 - Extreme");

enum { R_T = 0, R_W, R_I, R_X, R_V, R_A, R_C, R_SUMMARY, R_STARS, R_TEMP, R_TIME, R_PEOPLE, R_LINE, R_EXTENDED };
static const char *rwformaca =                  "Technical rating;Water rating;Time rating;Extra risk rating;Vertical rating;Aquatic rating;Commitment rating";

enum { M_MATCH=ITEM_ACA, M_SCORE, M_LEN, M_NUM, M_RETRY };

GeoCache _GeoCache;
GeoRegion _GeoRegion;

int iReplace(vars &line, const char *ostr, const char *nstr)
{
	int n = 0;
	vars nline;
	register int olen = strlen(ostr), nlen = strlen(nstr);
	for (register int i=0; line[i]!=0; ++i)
	{
		if (strnicmp(((const char*)line)+i, ostr, olen)==0)
			{
			line.Delete(i, olen);
			if (*nstr!=0)
				line.Insert(i, nstr);  // 3  4
			i += nlen-1;
			++n;
			}
	}

	return n;
}

BOOL isa(unsigned char c)
{
	return (c>='a' && c<='z') ||  (c>='A' && c<='Z')  || (unsigned char)c>127;
}

BOOL isanum(unsigned char c)
{
	return isdigit(c) || isa(c);
}

const char *skipnoalpha(const char *str)
{
	while (!isa(*str) && !(((unsigned char)*str)>127)) 
		++str;
	return str;
}

const char *skipalphanum(const char *str)
{
	while (isanum(*str)) 
		++str;
	return str;
}

const char *skipnotalphanum(const char *str)
{
	while (*str!=0 && !isanum(*str)) 
		++str;
	return str;
}

int countnotalphanum(const char *str)
{
	int cnt = 0;
	while (*str!=0 && !isanum(*str)) 
		++str, ++cnt;
	return cnt;
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

static CSymList rwlist, titlebetalist;
int LoadBetaList(CSymList &bslist, int title = FALSE, int rwlinks = FALSE);

// ===============================================================================================


vars UTF8(const char *val, int cp)
{
	int len = strlen(val);
	int wlen = (len+10)*3;
	int ulen = (len+10)*2;
	void *wide = malloc(wlen);
	void *utf8 = malloc(ulen);
	int res = MultiByteToWideChar(cp , 0, val,  len+1, (LPWSTR)wide, wlen);
	WideCharToMultiByte(CP_UTF8, 0, (LPWSTR)wide, res+1, (LPSTR)utf8, ulen, NULL, NULL);
	vars ret((const char *)utf8);
	free(wide);
	free(utf8);
	return ret;
}

vars ACP8(const char *val, int cp = CP_UTF8)
{
	int len = strlen(val);
	int wlen = (len+10)*3;
	int ulen = (len+10)*2;
	void *wide = malloc(wlen);
	void *utf8 = malloc(ulen);
	int res = MultiByteToWideChar(cp , 0, val,  len+1, (LPWSTR)wide, wlen);
	WideCharToMultiByte(CP_ACP, 0, (LPWSTR)wide, res+1, (LPSTR)utf8, ulen, NULL, NULL);
	vars ret((const char *)utf8);
	free(wide);
	free(utf8);
	return ret;
}

vars makeurl(const char *ubase, const char *folder)
	{
	if (*folder==0)
		return "";
	if (IsSimilar(folder, "../"))
		folder += 2;
	if (IsSimilar(folder, "http"))
		return folder;
	if (IsSimilar(folder, "//"))
		return CString("http:")+folder;

	int root = *folder =='/';
	if (root) ++folder;

	vars base(ubase); base.Trim("/");
	vars url = !IsSimilar(base, "http") ? "http://"+base : base;

	vara ids(url.Trim("/"), "/");
	ids.SetSize(root || ids.length()<=3 ? 3 : ids.length());

	return ids.join("/")+"/"+folder;
	}


vars urlstr(const char *url, int stripkmlidx)
{
	// normalize url
	vars u(url);
	u.Trim(" \n\r\t\x13\x0A");
	const char *https = "https:";
	if (IsSimilar(u, https))
		u = "http:"+u.Mid(strlen(https));
	if (IsSimilar(u, "http")) {
		u = vars(url_decode(u));
		vars wiki = ExtractString(u, "//", "", WIKILOC);
		if (!wiki.IsEmpty())
			u.Replace("//"+wiki+WIKILOC, "//" WIKILOC);
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
			const char *http = strstr(uhttp+1, "http");
			if (http) u = http;
		}

		if (stripkmlidx)
			{
			int f = u.Find("kmlidx");
			if (f>0) 
				u = u.Mid(0,f-1);
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
	if (stars==InvalidNUM)
		return "";
	if (stars>0 && stars<1)
		stars = 1;
	CString str = CData(stars)+"*";
	if (ratings==InvalidNUM)
		return str;
	return str+CData(ratings);
}


int Update(CSymList &list, CSym &newsym, CSym *chgsym, BOOL trackchanges)
{
	int f;
	++newsym.index;
	newsym.id = urlstr(newsym.id); 
	if ((f=list.Find(newsym.id))>=0) {
		CSym &sym = list[f];
		++sym.index;
		vara o(sym.data), n(newsym.data);
		o.SetSize(ITEM_BETAMAX);
		n.SetSize(ITEM_BETAMAX);
		if (o.join()==n.join())
			return 0;
		if (n[ITEM_LAT][0]=='@' && o[ITEM_LAT][0]!='@' && o[ITEM_LAT][0]!=0)
			n[ITEM_LAT] = "";
		for (int i=0; i<ITEM_BETAMAX; ++i)
			if (n[i].IsEmpty())
				n[i] = o[i];
		vars odata = o.join(), ndata = n.join();
		if (odata==ndata)
			return 0;

		// update changes
		sym = CSym(sym.id, ndata.TrimRight(","));
		++sym.index;

		// keep track of changes
		for (int i=0; i<ITEM_BETAMAX; ++i)
			if (n[i]==o[i])
				n[i] ="";
		CSym tmpsym(sym.id, n.join().TrimRight(","));
		//ASSERT(!strstr(sym.data, "Lucky"));
		if (chgsym)
			*chgsym = tmpsym;
		if (trackchanges) {
			// test extracting kml
			if (MODE>=0) {
				inetfile out( MkString("%u.kml", GetTickCount()) );
				int kml = KMLEXTRACT ? KMLExtract(sym.id, &out, TRUE) : -1;
				Log(LOGINFO, "CHG KML:%d %s\n%s\n%s", kml, sym.id, odata, ndata);
				}
		}
			return -1;
	}
	else {
		list.Add(newsym);
		list.Sort();
		if (chgsym)
			*chgsym = newsym;
		if (trackchanges) {
			if (MODE>=0) {
				inetfile out( MkString("%u.kml", GetTickCount()) );
				int kml = KMLEXTRACT ? KMLExtract(newsym.id, &out, TRUE) : -1;
				Log(LOGINFO, "NEW kml:%d %s", kml, newsym.Line());
				}
		}
		return 1;
	}
}

int UpdateCheck(CSymList &symlist, CSym &sym)
{
	int found = symlist.Find(sym.id);
	if (found<0)
		return TRUE;

	Update(symlist, sym, NULL, FALSE);
	sym = symlist[found];
	return FALSE;
}



int cmpconddate(const char *date1, const char *date2)
{
	int cmp = strncmp(date1, date2, 10);
	if (*date1==0)
		return -1;
	if (*date2==0)
		return 1;
	return cmp;
}



int UpdateOldestCond(CSymList &list, CSym &newsym, CSym *chgsym = NULL, BOOL trackchanges = TRUE)
{
	int f = list.Find(newsym.id);
	if (f>=0) 
		{
		int d = cmpconddate(newsym.GetStr(ITEM_CONDDATE), list[f].GetStr(ITEM_CONDDATE));
		if (d>0 && d<15) // max 0-15 days difference
			return FALSE;
		}
	Update(list, newsym, NULL, FALSE);
	return TRUE;
}

int UpdateCond(CSymList &list, CSym &newsym, CSym *chgsym, BOOL trackchanges)
{
	int f = list.Find(newsym.id);
	//ASSERT(!strstr(newsym.id, "=15546"));
	if (f>=0 && cmpconddate(newsym.GetStr(ITEM_CONDDATE), list[f].GetStr(ITEM_CONDDATE))<0)
		return FALSE;
	Update(list, newsym, NULL, FALSE);
	return TRUE;
}


CString ExtractStringDel(CString &memory, const char *pre, const char *start, const char *end, int ext = FALSE, int del = TRUE)
{
		int f1 = memory.Find(pre);
		if (f1<0) return "";
		int f2 = memory.Find(start, f1+strlen(pre));
		if (f2<0) return "";
		int f3 = memory.Find(end, f2+=strlen(start));
		if (f3<0) return "";
		int f3end = f3 + strlen(end);

		CString text = (ext>0 ? memory.Mid(f1, f3end-f1) : (ext<0 ? memory.Mid(f1, f3-f1) : memory.Mid(f2,f3-f2)));
		//Log(LOGINFO, "pre:%s start:%s end:%s", pre, start, end);
		//Log(LOGINFO, "%s", text);
		//ASSERT(!strstr(text,end));
		if (del>0) 
			memory.Delete(f1, f3end-f1);
		if (del<0) 
			memory.Delete(f1, f3-f1);
		return text;
}

unit utime[] = { {"h", 1}, {"hr", 1}, {"hour", 1}, {"min", 1.0 / 60}, {"minute", 1.0 / 60}, { "day", 24}, {"hs", 1}, {"hrs", 1}, {"hours", 1}, {"mins", 1.0 / 60}, {"minutes", 1.0 / 60}, { "days", 24}, NULL };
unit udist[] = { {"mi", 1}, {"mile", 1}, {"km", km2mi}, {"kilometer", km2mi}, {"mis", 1}, {"miles", 1}, {"kms", km2mi}, {"kilometers", km2mi}, {"m", km2mi / 1000}, {"meter", km2mi / 1000}, {"ms", km2mi / 1000}, NULL };
unit ulen[] = { {"ft", 1}, {"feet", 1}, {"'", 1}, {"&#039;",1}, {"meter", m2ft}, {"m", m2ft}, {"fts", 1}, {"feets", 1}, {"'s", 1}, {"&#039;s",1}, {"meters", m2ft}, {"ms", m2ft}, NULL };

int matchtag(const char *tag, const char *utag)
{
		int c;
		for (c=0; tag[c]==utag[c] && utag[c]!=0; ++c);
		if (c>=3) return TRUE;
		return utag[c]==0 && !isa(tag[c]);
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
	for (const char *istr = vstr; *istr!=0; ++istr) {
		BOOL isnum = isdigit(*istr) || (mode>0 && (*istr=='.' || *istr=='+'));
		BOOL issep = !isnum && strchr(" \t\n\r()[].", *istr)!=NULL;
		if (mode!=isnum || issep) {
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
/*
	if (!units) {
		// special case for raps


		while (*str!=0 && !isdigit(*str))
			++str;
		if (*str==0) return FALSE;
		const char *str2 = str;
		while (*str2!=0 && *str2!='-')
			++str2;
		while (*str2!=0 && !isdigit(*str2))
			++str2;
		

		double v = CDATA::GetNum(str);
		double v2 = CDATA::GetNum(str2);
		CString unit = GetToken(str, 1, ' ');
		CString unit2 = GetToken(str2, 1, ' ');
		for (int i=0; ulen[i].unit!=NULL; ++i) {
			if (IsSimilar(unit, ulen[i].unit))
				return FALSE; // not # rappels
			if (IsSimilar(unit2, ulen[i].unit))
				v2=InvalidNUM; // not # rappels
		}

		if (v!=InvalidNUM) time.AddTail(v);
		if (v2!=InvalidNUM) time.AddTail(v2);
		return v!=InvalidNUM;
	}
	*/

	// process units
	
	for (int i=1; i<list.length(); ++i) {
		double v = InvalidNUM, v2 = InvalidNUM;
		v = CDATA::GetNum(list[i-1]);
		if (list[i]=="-")
			v2 = CDATA::GetNum(list[++i]), ++i;
		if (v2!=InvalidNUM && v==InvalidNUM)
			Log(LOGWARN, "Inconsistent InvalidNUM v-v2 for %s", str), v=v2;
		if (v==InvalidNUM) 
			continue;
		const char *tag = list[i];

		if (!units) {
			// special case rappel# check is not unit of length
			for (int i=0; ulen[i].unit!=NULL; ++i)
				if (matchtag(tag, ulen[i].unit))
					return FALSE; // not # rappels
			if (v!=InvalidNUM) time.AddTail(v);
			if (v2!=InvalidNUM) time.AddTail(v2);
			return v!=InvalidNUM;
		}

		for (int u=0; units[u].unit; ++u) {
			// find best unit match
			if (matchtag(tag, units[u].unit)) {
				if (v!=InvalidNUM) time.AddTail(v*units[u].cnv);
				if (v2!=InvalidNUM) time.AddTail(v2*units[u].cnv);
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
	for (int i=0; i<len; ++i)
		avg += time[i];
	return avg/len;
}

CString Pair(CDoubleArrayList &raps)
{
	if (!raps.GetSize())
		return "";
	CString str = CData(raps.Head());
	if (raps.GetSize()>1 && raps.Head()!=raps.Tail())
		str+="-"+CData(raps.Tail());
	return str;
}




int GetValues(const char *data, const char *str, const char *sep1, const char *sep2, unit *units, CDoubleArrayList &time)
{
#if 0	
	vara atime(data, str);
	for (int a=1; a<atime.length(); ++a)
		GetValues(stripHTML(strval(atime[a], sep1, sep2)), units, time);
#else
	GetValues(stripHTML(ExtractString(data, str, sep1, sep2)), units, time);
#endif
	if (time.GetSize()==0)
		return 0;
	return time.Tail()>0;
}


CString GetMetric(const char *str)
{
	int i;
	vars len;
	while (*str!=0)
		{
		char c = *str++;
		switch (c)
			{
			case ',': 
			case ';': 
			case '\'': 
				c='.'; 
				break;
			case '.': 
				for (i=0; isdigit(str[i]); ++i);
				if (i==3 || i==0) 
					continue;
				break;
			}
		len += c;
		}

	len.MakeLower();
	double length = CDATA::GetNum(len);
	if (length==InvalidNUM || length==0)
		return "";
	if (length<0)
		length = -length;
	
	const char *unit = len;
	while (!isa(*unit) && *unit!=0)
		++unit;
	if (IsSimilar(unit,"mi"))
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
	return CData(length)+unit;
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

			return time.GetSize()>0 || dist.GetSize()>0;
}


int GetRappels(CSym &sym, const char *str)
{
			if (*str==0)
				return FALSE;
			CDoubleArrayList raps;
			if (!GetValues(str, NULL, raps)) {
				Log(LOGWARN, "Invalid # of raps '%s'", str);
				return FALSE;
			}
			if (raps.Tail()>=50) {
				Log(LOGERR, "Too many rappels %g from %s", raps.Tail(), str);
				return FALSE;
			}
			sym.SetStr(ITEM_RAPS, Pair(raps));
			if (raps.Tail()==0) 
				return TRUE; // non technical

			// length 
			CDoubleArrayList len;
			if (GetValues(str, ulen, len)) {
				if (len.Tail()<=10) {
					Log(LOGERR, "Too short rappels %g from %s", len.Tail(), str);
					return FALSE;
				}
				if (len.Tail()>400) {
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

static const char *rtech[] = { "1", "2", "3", "4", NULL };
static const char *rwater[] = { "A", "B", "C", NULL };
static const char *rxtra[] = { "R-", "R+", "R", "PG-", "PG+", "PG", "XX", "X", NULL };
static const char *rclassc[] = { "B+", "B-", "C+", "C-", "B/C", "C0", "C1+", "C1-", "C1", "C2", "C3", "C4", NULL };
static const char *rverticalu[] = { "V1", "V2", "V3", "V4", "V5", "V6", "V7", NULL };
static const char *raquaticu[] = { "A1", "A2-", "A2+", "A2", "A3", "A4-", "A4+", "A4", "A5", "A6", "A7", NULL };
static const char *rtime[] = { "IV", "VI", "III", "II", "I", "V", NULL };

int GetNum(const char *str, int end, vars &num)
{
	num = "";

	int start;
	for (start=end; start>0 && (isdigit(str[start-1]) || str[start-1]=='.'); --start);

	if (start==end)
		return FALSE;
		
	num = CString(str+start, end-start);
	return TRUE;
}

int GetSummary(vara &rating, const char *str)
{
	if (!str || *str==0)
		return FALSE;

	int extended = rating.length()==R_EXTENDED;

	vars strc(str); 
	strc.Replace("&nbsp;", " ");
	if (!extended)
		strc.Replace(" ", "");
	strc.Replace("\"", "");
	strc.Replace("\'", "");
	strc.MakeUpper();
	strc += "  ";
	str = strc;

	int v = -1, count = -1;
	int getaca= TRUE, getx = 0, getv = TRUE, geta = TRUE, geti = TRUE, getc = TRUE, getstar=TRUE, getmm=TRUE, gethr=TRUE, getpp=TRUE;
	for (int i=0; str[i]!=0; ++i) {
		register char c = str[i], c1 = str[i+1];		
		if (c=='%' && isanum(c1))
			{
			// %3C
			i+=2;
			continue;
			}
		if (c>='1' && c<='4') 
		  if (getaca)
			{
			if (c1=='(') {
				const char *end = strchr(str+i+1, ')');
				if (end!=NULL)
					c1 = end[1];
				}
			if (c1>='A' && c1<='C')
				{
				++i;
				if (IsSimilar(str+i,"AM")) //am/pm
					{
					i += 2;
					continue;
					}
				rating[R_T] = c;
				rating[R_W] = c1;
				// C1-C4
				//char c2 = str[i+1];
				if ((v=IsMatchN(str+i, rclassc))>=0)
					{
					rating[R_W] = rclassc[v];
					i += strlen(rclassc[v])-1;
					}
				getx = TRUE;
				getaca = FALSE;
				count = 0;
				continue;
				}
			}
		if (getx && count<3 && (v=IsMatchN(str+i, rxtra))>=0) {
			static const char *ignore[] = { "RIGHT", NULL };
			if (IsMatch(str+i, ignore))
				continue;
			// avoid 'for' 'or' etc
			const char *prior = "ABCIV1234+-/;";
			if (!strchr(prior, str[i-1]))
				continue;
			rating[R_X] = rxtra[v];
			i += strlen(rxtra[v])-1;
			getx = FALSE;
			count = 0;
			continue;
			}

		if (c=='V' && isdigit(c1))
		  if (getv && (v=IsMatchN(str+i, rverticalu))>=0) {
			rating[R_V] = rverticalu[v];
			rating[R_V].MakeLower();
			i += strlen(rverticalu[v])-1;
			getv = FALSE;
			count = 0;
			continue;
			}

		if (c=='A' && isdigit(c1))
		  if (geta && (v=IsMatchN(str+i, raquaticu))>=0) {
			rating[R_A] = raquaticu[v];
			rating[R_A].MakeLower();
			i += strlen(raquaticu[v])-1;
			geta = FALSE;
			count = 0;
			continue;
			}

		if (!getaca && count<3 && geti && (v=IsMatchN(str+i, rtime))>=0) {
			rating[R_I] = rtime[v];
			i += strlen(rtime[v])-1;
			geti = FALSE;
			count = 0;
			continue;
			}

		if ((!getv||!geta) && count<3 && getc && (v=IsMatchN(str+i, rtime))>=0) 
			{
			// avoid 'for' 'or' etc
			rating[R_C] = rtime[v];
			i += strlen(rtime[v])-1;
			getc = FALSE;
			count = 0;
			continue;
			}

		// extended summary
		if (extended)
			{
			vars num;
			if (c=='*' && getstar)
				if (GetNum(str, i, num))
					{
					rating[R_STARS] = num;
					getstar = FALSE;
					continue;
					}
			if (c=='M' && c1=='M' && getmm)
				if (str[i-1]=='X')
					{
					rating[R_TEMP] = "X";
					getmm = FALSE;
					continue;
					}
				else
				if (GetNum(str, i, num))
					{
					rating[R_TEMP] = num;
					getmm = FALSE;
					continue;
					}
			if (c=='P' && c1=='P' && getpp)
				if (GetNum(str, i, num))
					{
					rating[R_PEOPLE] = num;
					getpp = FALSE;
					continue;
					}
			if (c=='H' && c1=='R' && gethr)
				if (GetNum(str, i, num))
					{
					rating[R_TIME] = num;
					gethr = FALSE;
					continue;
					}
			}


		if (count>=0)
			if (++count>2)
				if (!extended)
					break;
	}

	if (!getaca || (!getv && !geta))
		return TRUE;
	if (extended)
		if (!getstar || !getmm)
			return TRUE;
	return FALSE;
}

int GetSummary(CSym &sym, const char *str)
{

	vara rating; rating.SetSize(R_SUMMARY+1);
	int ret = GetSummary(rating, rating[R_SUMMARY] = str);

	vars sum = rating.join(";");
	sum.Replace("+","");
	sum.Replace("-","");
	sym.SetStr(ITEM_ACA, sum);	
	return ret;
}



int GetClass(const char *type, const char *types[], int typen[])
{
		for (int i=0; types[i]!=NULL; ++i)
			if (strstr(type, types[i])!=NULL)
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


void SetVehicle(CSym &sym,  const char *text)
{
	vars vehicle = "", road = text;
	if (strstr(road, "Passenger"))
		vehicle = "Passenger:"+road;
	if (strstr(road, "High"))
		vehicle = "High Clearance:"+road;
	if (strstr(road, "Wheel") || strstr(road, "4WD"))
		vehicle = "4WD - High Clearance:"+road;
	sym.SetStr(ITEM_VEHICLE, vehicle);
}

vars query_encode(const char *str)
{
	return vars(str).replace("+", "%2B");
}

//int GetRWList(CSymList &idlist, const char *fileregion);

CString GetASKList(CSymList &idlist, const char *query, rwfunc func)
	{
	//rwlist.Empty();
	DownloadFile f;
	CString base = "http://ropewiki.com/api.php?action=ask&format=xml&curtimestamp&query=";

	vara fullurllist;
	double offset = 0;
	CString timestamp;
	vars dquery, dparam = "|%3FModification_date";
	BOOL moddate = strstr(query, "sort=Modification")!=NULL;
	int step = moddate ? 500 : 500, n = 0;
	vars category = ExtractString(query, "Category:", "", "]");
	while (offset!=InvalidNUM) {
		vars mquery = query;
		if (moddate)
			mquery = dquery+"<q>"+GetToken(query, 0, '|')+"</q>|"+GetTokenRest(query, 1, '|');
		CString url = base 
			+ mquery + dparam
			+ "|limit="+MkString("%d",step)+"|offset="+MkString("%d", moddate ? 0 : (int)offset);
		printf( "Downloading ASK %s %gO %dP (%dT)...\r", category, offset, n++, idlist.GetSize());
		//Log(LOGINFO, "%d url %s", (int)offset, url);
		if (f.Download(url))
			{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			break;
			}
		if (timestamp.IsEmpty())
			timestamp = ExtractString(f.memory, "curtimestamp=");

		vara list(f.memory, "<subject ");
		int size = list.length()-1;
		double newoffset = ExtractNum(f.memory, "query-continue-offset=");
		if (newoffset!=InvalidNUM && newoffset<offset) {
			Log(LOGERR, "Offset reset %g+%d -> %g", offset, size, newoffset);
			break;
			}
		offset = newoffset;

		if (moddate)
			{
			vars lastdate = ExtractString(list[size], "Modification date", "<value>", "</value>");
			dquery = "[[Modification_date::>"+lastdate+"]]";
			}

		vara dups;
		for (int i=1; i<list.length(); ++i) {
			vars fullurl = ExtractString(list[i],"fullurl=");
			if (fullurllist.indexOf(fullurl)<0) 
				{
				fullurllist.push(fullurl);
				func(list[i], idlist);
				continue;
				}
			dups.push(fullurl);
			}

		if (dups.length()>1)
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
		printf( "Downloading API %d (%dT)...\r", n++, idlist.GetSize());
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
		cont = cont.replace("\"","").replace(" ","&").Trim(" &");

		vara list(f.memory, "<page ");
		for (int i=1; i<list.length(); ++i)
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
		if (strlen(data)!= size)
			Log(LOGERR, "inconsistency len:%d vs size:%d ", strlen(data), size);

		int lasti = 0;
		int datalen = size;
		int seplen = strlen(sep);
		list.RemoveAll();
		for (int i=0; i<datalen-seplen; ++i)
		  if (strnicmp(data+i, sep, seplen)==0)
			{
			// flush out
			list.Add(CString(data+lasti, i-lasti));
			i = lasti = i+seplen;
			}
		// flush out
		list.Add(CString(data+lasti, datalen-lasti));
	 }


static int xmlid(const char *data, const char *sep, BOOL toend = FALSE)
{
		int datalen = strlen(data);
		int seplen = strlen(sep);
		if (toend && sep[seplen-1]=='>')
			--seplen;
		for (int i=0; i<datalen-seplen; ++i)
		  if (strnicmp(data+i, sep, seplen)==0)
			{
			if (!toend) return i;
			for (; i<datalen; ++i)
				if (data[i]=='>')
					return i+1;
			}
		return -1;
}

vars xmlval(const char *data, const char *sep1, const char *sep2)
{
	int start = xmlid(data, sep1, TRUE);
	if (start<0) return "";
	data += start;
	if (!sep2)
		return data;
	int end = xmlid(data, sep2);
	if (end<0) return "";
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
	for (int i=0; data[i]!=0; ++i)
		{
		if (data[i]=='?')
			continue;
		if (IsSimilar(data+i,CDATAS))
			i+=strlen(CDATAS)-1;
		else if (IsSimilar(data+i,CDATAE))
			i+=strlen(CDATAE)-1;
		else if (data[i]=='<' && ignore==0)
			{
			// add space when dealing with <div>
			if (IsSimilar(data+i, "<div") || IsSimilar(data+i, "</div") || IsSimilar(data+i, "<br"))
				out += " ";
			++ignore;
			}
		else if (data[i]=='>' && ignore==1)
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
	out.Replace("\x0D"," ");
	out.Replace("\x0A"," ");
	out.Replace("\t"," ");	
	out.Replace(" #", " ");
	out.Replace(",", ";");
	out.Replace("\xC2\xA0", " ");
	out.Replace("\xC2\x96", "-");
	out.Replace("\xC2\xB4", "'");
	out.Replace("\xE2\x80\x99", "'");
	out.Replace("\xE2\x80\x93", "-");
	out.Replace("\xE2\x80\x94", "-");
	while (out.Replace("  ", " ")>0);
	return out.Trim();
}

int CheckLL(double lat, double lng, const char *url)
{
	if (lat==InvalidNUM || lng==InvalidNUM || lat==0 || lng==0 || lat<-90 || lat>90 || lng<-180 || lng>180)
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
	for (int i=0; text[i]!=0; ++i)
		{
		char c = text[i];
		if (isanum(c))
			{
			word += c;
			}
		else
			{
			if (!word.IsEmpty())
				list.push(word), word="";
			list.push(vars(c));
			}
		}
	if (!word.IsEmpty())
		list.push(word), word="";
	return list;
}

BOOL TranslateMatch(const char *str, const char *pattern)
{
	int i;
	for (i=0; str[i]!=0 && pattern[i]!=0 && pattern[i]!='*'; ++i)
		{
		if (tolower(str[i])!=tolower(pattern[i]))
			return FALSE;
		}
	return str[i]==pattern[i] || pattern[i]=='*';
}

vars Translate(const char *str, CSymList &list, const char *onlytrans) 
{
	vara transwords;
	vara words = getwords(str);
	for (int i=0; i<words.length(); ++i)
		{
		vars &word = words[i];
		const char *found = NULL;
		for (int j=0; j<list.GetSize() && !found; ++j)
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

static vara months("Jan,Feb,Mar,Apr,May,Jun,Jul,Aug,Sep,Oct,Nov,Dec");

CString SeasonCompare(const char *season)
{
	CString str(season);
	str.Replace("-"," - ");
	str.Replace(","," ");
	str.Replace(";"," ");
	str.Replace("."," ");
	str.Replace("&"," ");
	str.Replace("/"," ");
	while(str.Replace("  "," "));
	vara words(str, " ");
	for (int i=0; i<words.length(); ++i)
	  {
	  for (int m=0; m<months.length(); ++m)
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
	enum { Q_BEST=0, Q_ONE };
	vara tempt("Early,Late");
	vara sept("-,to,through,thru,or,and");

	vara validt(months); 
	validt.Append(seasonst);
	validt.Append(othert);
	vara valid(months);
	valid.Append(seasonst);
	for (int i=0; i<othert.length(); ++i)
		valid.push((othert[i][0]==tolower(othert[i][0]) ? " " : "") + othert[i]);
	vara qual(qualt);
	for (int i=0; i<qualt.length(); ++i)
		qual[i] = qualt[i] + " in ";
	vara sep(sept);
	for (int i=0; i<sept.length(); ++i)
		sep[i] = " " + sept[i] + " ";
	vara temp(tempt);
	for (int i=0; i<tempt.length(); ++i)
		temp[i] = tempt[i] + " ";

	int state = 0;
	int hasmonth = 0, best = 0;
	vars pre, presep;
	int last = 0;
	vara vlist;
	vara words = getwords(season);

	for (int i=words.length()-1; i>=0; --i)
		{
		// prefixes qualifiers
		if (i>3 && i<words.length()-4 && IsSimilar(words[i],"can") && IsSimilar(words[i+2],"be"))
			{
			vars adj = words[i+4];
			if (IsSimilar(adj, "very"))
				{
				words.RemoveAt(i+4); // very
				if (i+4>=words.length())
					continue;
				words.RemoveAt(i+4); // _
				if (i+4>=words.length())
					continue;
				adj = words[i+4];
				}
			words.RemoveAt(i); // can
			words.RemoveAt(i); // _
			words.RemoveAt(i); // be
			words.RemoveAt(i); // _
			words.RemoveAt(i); // adj
			if (i<words.length()-1 && IsSimilar(words[i+1],"in"))
				words.InsertAt(i, adj);
			else
				words.InsertAt(i-3, adj);
			}
		// prefixes qualifiers
		if (i>3 && i<words.length()-2 && IsSimilar(words[i],"are"))
			{
			vars adj = words[i+2];
			if (IsSimilar(adj, "very"))
				{
				words.RemoveAt(i+2); // very
				words.RemoveAt(i+2); // _
				adj = words[i+2];
				}
			words.RemoveAt(i); // are
			words.RemoveAt(i); // _
			words.RemoveAt(i); // adj
			words.InsertAt(i-1, adj);
			}
		}
	for (int i=0; i<words.length(); ++i)
		{
		vars &word = words[i];
		if (!words.IsEmpty())
			{
			BOOL ok = FALSE;
			// sep
			for (int v=0; v<sept.length() && !ok; ++v)
				if (IsSimilar(word, sept[v]))
					{
					ok = TRUE;
					if (last<=5)
						{
						presep = sep[v];
						last = 0;
						}
					pre = "";
					}
			// temp
			for (int v=0; v<tempt.length() && !ok; ++v)
				if (IsSimilar(word, tempt[v]))
					{
					ok = TRUE;
					pre = temp[v];
					last = 0;
					}
			// qualif
			for (int v=0; v<qualt.length() && !ok; ++v)
				if (IsSimilar(word, qualt[v]))
					{
					ok = TRUE;
					pre = qual[v];
					if (v == Q_BEST)
						best = TRUE;
					last = 0;
					}
			// seasons / months / other
			for (int v=0; v<validt.length() && !ok; ++v)
				if (IsSimilar(word, validt[v]))
					{
					ok = TRUE;
					if (!presep.IsEmpty() && last<=8)
						vlist.push(presep);
					if (!pre.IsEmpty() && last<=8)
						{
						if (qual.indexOf(pre)==Q_ONE)
							{
							if (!presep.IsEmpty() && v<12)
								v = v==0 ? 11 : v-1;							
							}
						else
							vlist.push(pre);
						}
					presep = pre = "";
					vlist.push(valid[v]);
					if (v<12)
					  hasmonth = TRUE;
					last = 0;
					}
			++last;
			}
		}

	BOOL ok = vlist.length()>0;

	// double check 
	int nsep = 0, nmonths = 0;
	for (int i=vlist.length()-1; i>=0; --i)
		{
		// trim '-'
		if (sep.indexOf(vlist[i])>=0)
		  {
		  if (i==0 || i==vlist.length()-1 || sep.indexOf(vlist[i+1])>=0) 
			{
			vlist.RemoveAt(i);
			continue;
			}
		  ++nsep;
		  continue;
		  }
		// trim qualif
		if (qual.indexOf(vlist[i])>=0)
		  {
		  if (i==vlist.length()-1 || valid.indexOf(vlist[i+1])<0) 
			{
			ok = FALSE;
			vlist.RemoveAt(i);
			continue;
			}
		  continue;
		  }
		// trim temp
		if (temp.indexOf(vlist[i])>=0)
		  {
		  if (i==vlist.length()-1 || seasonst.indexOf(vlist[i+1])<0) 
			{
			ok = FALSE;
			vlist.RemoveAt(i);
			continue;
			}
		  continue;
		  }
		// month and non-month
		if (hasmonth && months.indexOf(vlist[i])<0) 
			{
			ok = FALSE;
			//vlist.RemoveAt(i);
			continue;
			}
		++nmonths;
		}

	if (hasmonth)
		{
		if (nsep>0 && nmonths!=2*nsep) 
			ok = FALSE;
		if (nsep>1)
			ok = FALSE;
		if (nmonths>=2 && nsep==0 && !best)
			{
			for (int i=1; i<vlist.length() && ok; ++i)
				{
				int a = months.indexOf(vlist[i-1]);
				int b = months.indexOf(vlist[i]);
				if (a>=0 && b>=0 && abs(a-b)>1 && abs(a-b)<11)
					{
					ok = FALSE;
					if (nmonths==2)
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
	vara nlist(add), list(vars(str).replace(",",";"), ";");
	for (int i=nlist.length()-1; i>=0; --i)
		nlist[i].Trim();
	for (int i=list.length()-1; i>=0; --i)
		{
		list[i].Trim();
		if (nlist.indexOf(list[i])<0)
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
	out->write( KMLStart() );
	out->write( KMLName( MkString("%s %s", title, credit), vars(url).replace("&", "&amp;")) );
	for (int i=0; i<styles.length(); ++i)
		out->write( KMLMarkerStyle( GetToken(styles[i],0,'='), GetToken(styles[i],1,'=')) );
	for (int i=0; i<points.GetSize(); ++i)
		out->write( points[i] );
	for (int i=0; i<lines.length(); ++i)
		out->write( lines[i] );
	out->write( KMLEnd() );
	return points.length()+lines.length();
}


// ===============================================================================================


vars GetKMLIDX(DownloadFile &f, const char *url, const char *search, const char *start, const char *end)
{
	vara ids(url, "kmlidx=");
	if (ids.length()>1 && !ids[1].IsEmpty() && ids[1]!="X")
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
	for (int i=1; i<wpt.length(); ++i)
		{
			vars data = wpt[i].split("</wpt>").first();
			vars id = stripHTML(xmlval(data, "<name", "</name"));
			vars desc = stripHTML(xmlval(data, "<desc", "</desc"));
			vars cmt = stripHTML(xmlval(data, "<cmt", "</cmt"));
			if (desc.IsEmpty())
				desc = cmt;
			double lat = CDATA::GetNum(strval(data, "lat=\"", "\""));
			double lng = CDATA::GetNum(strval(data, "lon=\"", "\""));
			if (id.IsEmpty() || !CheckLL(lat, lng, url)) continue;


		// add markers
		points.push( KMLMarker("dot", lat, lng, id, desc+credit ) );
		}

	// process lines
	vara trk(memory, "<trkseg");
	for (int i=1; i<trk.length(); ++i)
		{
			vara linelist;
			vars data = trk[i].split("</trkseg").first();
			vara trkpt(data, "<trkpt");
			for (int p=1; p<trkpt.length(); ++p) {
				double lat = CDATA::GetNum(strval(trkpt[p], "lat=\"", "\""));
				double lng = CDATA::GetNum(strval(trkpt[p], "lon=\"", "\""));
				if (!CheckLL(lat, lng, url)) continue;
				linelist.push(CCoord3(lat, lng));
			}
		CString name = "Track";
		if (i>1) name+=MkString("%d",i);
		lines.push( KMLLine(name, credit, linelist, OTHER, 3) );
		}

	if (points.length()==0 && lines.length()==0)
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
	while (isletter(*pllstr) && *pllstr!=0) ++pllstr;
	while (!isletter(*pllstr) && *pllstr!=0) ++pllstr;
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
		for (int i=0; i<size; ++i)
			{
			dlist[i].id.Trim(" '");
			dlist[i].index = dlist[i].id.GetLength();
			}
		dlist.Add(CSym("'"));
		}

	// basic2
	vara words = getwords(name);
	for (int w=0; w<words.length(); ++w)
		for (int i=0; i<dlist.GetSize(); ++i)
			if (stricmp(words[w], dlist[i].id)==0)
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
	const char* trup3  = "AAAAAAA EEEEIIIID OOOOOx0UUUUYPsaaaaaaa eeeeiiiio ooooo/0uuuuypy";
	const char* trlow3 = "aaaaaaaCeeeeiiiidNooooox0uuuuypsaaaaaaaceeeeiiiionooooo/0uuuuypy";

					//   "ĀāĂăĄąĆćĈĉĊċČčĎďĐđĒēĔĕĖėĘęĚěĜĝĞğĠġĢģĤĥĦħĨĩĪīĬĭĮįİıĲĳĴĵĶķĸĹĺĻļĽľĿ"
	const char* trdup4 = "                                                                ";
	const char* trup4  = "AaAaAaCcCcCcCcDdDdEeEeEeEeEeGgGgGgGgHhHhIiIiIiIiIiIiJjKkkLlLlLlL";
	const char* trlow4 = "aaaaaaccccccccddddeeeeeeeeeegggggggghhhhiiiiiiiiiiiijjkkklllllll";

					//   "ŀŁłŃńŅņŇňŉŊŋŌōŎŏŐőŒœŔŕŖŗŘřŚśŜŝŞşŠšŢţŤťŦŧŨũŪūŬŭŮůŰűŲųŴŵŶŷŸŹźŻżŽžſ"
	const char* trdup5 = "                                                                ";
	const char* trup5  = "lLlNnNnNnnNnOoOoOoOoRrRrRrSsSsSsSsTtTtTtUuUuUuUuUuUuWwYyYZzZzZzP";
	const char* trlow5 = "lllnnnnnnnnnoooooooorrrrrrssssssssttttttuuuuuuuuuuuuwwyyyzzzzzzp";

	const char *trups[] = { trup3, trup4, trup5 };
	const char *trlows[] = { trlow3, trlow4, trlow5 };
	const char *trdups[] = { trdup3, trdup4, trdup5 };

	vars ret;
	while (*p!=0)
		{
		unsigned char ch = (*p++);
		if (makelower && ch>='A' && ch<='Z')
			ch = ch-'A'+'a';
		else if ( ch ==0xc3 || ch==0xc4 || ch==0xc5)
			{
			unsigned char ch1 = ch;
			unsigned char ch2 = *p++;

			int t = ch-0xc3;
			const char* tr = makelower ? trlows[t] : trups[t];
			const char* trdup = trdups[t];
			if (ch2>=0x80)
				{
				int i = ch2 - 0x80;
				ch = tr[i];
				if (ch==' ')
					{
					ret += ch1;
					ch = ch2;
					}
				if (trdup[i]!=' ')
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

vars stripAccents2(register const char* p) 
{
	vars ret = stripAccents(p);

	// special characters
	ret.Replace("\xc3\x91", "Ny");
	ret.Replace("\xc3\xb1", "ny");
	ret.Replace("\xc3\x87", "S");
	ret.Replace("\xc3\xa7", "s");

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
if (ucb(c[0],0,0x7F)) 
	return 1;
if (ucb(c[0],0xC2,0xDF))
	if (ucb(c[1],0x80,0xBF))
		return 2;
if (ucb(c[0],0xE1,0xEC))
	if (ucb(c[1],0x80,0xBF))
		if (ucb(c[2],0x80,0xBF))
			return 3;
if (c[0]==0xED)
	if (ucb(c[1],0x80,0x9F))
		if (ucb(c[2],0x80,0xBF))
			return 3;
if (c[0]==0xE0)
	if (ucb(c[1],0xA0,0xBF))
		if (ucb(c[2],0x80,0xBF))
			return 3;
if (ucb(c[0],0xEE,0xEF))
	if (ucb(c[1],0x80,0xBF))
		if (ucb(c[2],0x80,0xBF))
			return 3;
if (c[0]==0xF0)
	if (ucb(c[1],0x90,0xBF))
		if (ucb(c[2],0x80,0xBF))
			if (ucb(c[3],0x80,0xBF))
			return 4;
if (ucb(c[0],0xF1,0xF3))
	if (ucb(c[1],0x80,0xBF))
		if (ucb(c[2],0x80,0xBF))
			if (ucb(c[3],0x80,0xBF))
			return 4;
if (c[0]==0xF4)
	if (ucb(c[1],0x80,0x8F))
		if (ucb(c[2],0x80,0xBF))
			if (ucb(c[3],0x80,0xBF))
			return 4;
return FALSE;
}

int IsUTF8(const char *str)
{
	int utf = -1;
	while (*str!=0)
		{
		int n = IsUTF8c(str);
		if (n<=0) 
			return FALSE;
		if (n>1)
			utf = TRUE;
		str += n;
		}
	return utf;
}


CString FixUTF8(const char *str)
{
	CString out;
	while (*str!=0)
		{
		int n = IsUTF8c(str);
		if (n<=0) 
			{
			++str;
			continue;
			}
		while (n>0)
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
	for (int i=1; i<names.length(); ++i)
		if (names[i].Replace(desc, credit+desc)<=0)
			names[i].Insert(0, "<description>"+credit+desc);
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
	if (strncmp(f.memory, "PK", 2)==0)
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
			for (int n = 0; GetZipItem(hzip, n, &ze)==ZR_OK; ++n)
				{
				char *ext = GetFileExt(ze.name);
				if (ext!=NULL && stricmp(ext,"kml")==0 || stricmp(ext,"kmz")==0)
				//if (stricmp(ze.name,"doc.kml")==0)
						{
						// unzip and process
						int size = ze.unc_size;
						char *memory = (char *)malloc(size+1);
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

	Log(LOGERR, "Invalid KML/KMZ signature %s from %s", CString(f.memory,4), url);
	return FALSE;
}

int CCOLLECTIVE_ExtractKML(const char *ubase, const char *url, inetdata *out, int fx)
{
	DownloadFile f;
	vars id = GetKMLIDX(f, url, "KmlLayer(", "", ")");
	id.Trim(" '\"");
	if (id.IsEmpty())
		return FALSE; // not available

	// fix issues with bad link
	id.Replace("&amp;", "&");
	int find  = id.Find("http", 5);
	if (find>0) id = id.Mid(find);
	vars msid = ExtractString(id, "msid=", "", "&");
	
	CString credit = " (Data by " + CString(ubase) + ")";
	if (msid.IsEmpty())
		KMZ_ExtractKML(credit, id, out);
	else
		KMZ_ExtractKML(credit, "http://maps.google.com/maps/ms?ie=UTF8&hl=en&msa=0&output=kml&msid="+msid, out);
	return TRUE;
}

vars TableMatch(const char *str, vara &inlist, vara &outlist)
{
	if (!str || *str==0)
		return "";

	for (int i=0; i<inlist.length(); ++i)
		if (IsSimilar(str, GetToken(inlist[i],0,'=')))
			{
			//if (water>0 && water<sizeof(waterconv)/sizeof(*waterconv))
			CString val = inlist[i].Right(1);
			ASSERT(val[0]>='/' && val[0]<='9');
			if (val[0]>='0' && val[0]<='9')
				return outlist[atoi(val)];
			return "";
			}

	ASSERT(!"Invalid value!");
	return "";
}


int CCOLLECTIVE_DownloadPage(DownloadFile &f, const char *url, CSym &sym) 
{
		// get full stats
		if (f.Download(url))
			{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			return FALSE;
			}

		
		vars stime = stripHTML(ExtractString(f.memory, "Time Commitment:", "<dd>", "</dd"));
		CDoubleArrayList time;
		if (GetValues(stime, utime, time) || GetValues(stime, NULL, time)) {
			sym.SetNum(ITEM_MINTIME, time.Head());
			sym.SetNum(ITEM_MAXTIME, time.Tail());
			}

		//vars area = stripHTML(ExtractString(f.memory, ">Area:", "<dd>", "</dd"));
		//sym.SetStr(ITEM_REGION, reg + ";" + area);
		vara reg;
		int betabase = FALSE;
		vara crumbs( ExtractString(f.memory, "\"breadcrumb\"", "", "</field") ,"\"crust");
		for (int i=1; i<crumbs.length(); ++i)
			{
			vars str = stripHTML(ExtractString(crumbs[i], "href=", ">", "</a"));
			if (betabase)
				reg.push(str);
			if (str=="Betabase")
				betabase = TRUE;
			}
		sym.SetStr(ITEM_REGION, reg.join(";"));


		vars season = stripHTML(ExtractString(f.memory, ">Best Seasons:", "<dd>", "</dd"));
		sym.SetStr(ITEM_SEASON, season.replace("Mid-","").replace("Year","All year"));
		
		vars vehicle = stripHTML(ExtractString(f.memory, ">Vehicle:", "<dd>", "</dd"));
		SetVehicle(sym, vehicle);

		//vars shuttle = stripHTML(ExtractString(f.memory, ">Shuttle:", "<dd>", "</dd"));
		//sym.SetStr(ITEM_SHUTTLE, shuttle);

		vars coord = stripHTML(ExtractString(f.memory, ">Coordinates:", "<dd>", "</dd"));
		double lat = CDATA::GetNum(GetToken(coord, 0, ';'));
		double lng = CDATA::GetNum(GetToken(coord, 1, ';'));
		if (CheckLL(lat,lng))
			{
			sym.SetNum(ITEM_LAT, lat);
			sym.SetNum(ITEM_LNG, lng);
			}

		// kml
		vars kmlidx = ExtractString(f.memory, "KmlLayer(", "", ")").Trim("'\" ");
		if (!kmlidx.IsEmpty())
			sym.SetStr(ITEM_KML, "X");

		vara conds(f.memory, "/calendar.png");
		if (conds.length()>1)
			{
			const char *memory = conds[1];
			vars date = stripHTML(ExtractString(memory, "", ">", "</span"));
			double vdate = CDATA::GetDate(date, "MMM D; YYYY" );
			if (vdate>0)
				{
				//enum { W_DRY=0, W_WADING=1, W_SWIMMING=2, W_VERYLOW=3, W_LOW=4, W_MODLOW=5, W_MODERATE=6, W_MODHIGH=7, W_HIGH=8, W_VERYHIGH=9, W_EXTREME=10 };
				static vara watercc("NA=/,Dry=0,Muddy=0,Shallow=1,Deep=2,Swimming=2,Light=4,Moderate=6,Heavy flow=8,Extreme flow=10");
				
				//static vara cond_temp("0 - None,1 - Rain jacket,2 - Thin wetsuit,3 - Full wetsuit,4 - Thick wetsuit,5 - Drysuit");
				static vara tempcc("NA=/,None=0,Waterproof=1,Fleece=1,1-3mm=2,4-5mm=3,6mm+=4,Drysuit=5");

				//static vara cond_diff("0 - Nontechnical,1 - Easy,2 - Normal,3 - Special challenges,4 - Advanced,5 - Extreme");
				static vara diffcc("NA=/,Easy=1,Moderate=2,Difficult=3,Strenuous=4,Very strenuous=5");
											
				vara cond; 
				cond.SetSize(COND_MAX);

				cond[COND_DATE] = CDate(vdate);
				cond[COND_WATER] = TableMatch(stripHTML(ExtractString(memory, ">Water:", "", "</br")), watercc, cond_water);
				cond[COND_TEMP] = TableMatch(stripHTML(ExtractString(memory, ">Thermal:", "", "</br")), tempcc, cond_temp);
				cond[COND_DIFF] = TableMatch(stripHTML(ExtractString(memory, ">Difficulty:", "", "</br")), diffcc, cond_diff);

				sym.SetStr(ITEM_CONDDATE, cond.join(";"));
				}
			}
	
		return TRUE;
}

int CCOLLECTIVE_Empty(CSym &sym)
{
	vars summary = sym.GetStr(ITEM_ACA);
	summary.Replace(";","");
	return summary.IsEmpty() && sym.GetStr(ITEM_LONGEST).IsEmpty() && sym.GetStr(ITEM_RAPS).IsEmpty() && sym.GetStr(ITEM_STARS).IsEmpty() && sym.GetStr(ITEM_CONDDATE).IsEmpty();
}

int CCOLLECTIVE_DownloadBeta(const char *ubase, CSymList &symlist) 
{
	// ALL map 
	ubase = "canyoncollective.com";

	DownloadFile f;
	CString url = burl(ubase, "betabase");
	if (f.Download(url))
		{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
		}

	// tables for ratings
	vars table = ExtractString(f.memory, "<h3>Areas</h3>", "<ol", "</ol");
	vara list(table, "href=");
	if (list.length()<=1)
		Log(LOGERR, "Could not extract sections from url %.128s", url);
	for (int i=1; i<list.length(); ++i) {
			vars lnk = ExtractString(list[i], "\"", "", "\"");
			vars reg = ExtractString(list[i], ">", "", "<");
			CString url = burl(ubase, lnk);
			double page = 1;
			while (page!=InvalidNUM)
				{
				if (f.Download(url+MkString("?page=%g", page)))
					{
					Log(LOGERR, "ERROR: can't download url %.128s", url);
					continue;
					}

				double last = ExtractNum(f.memory, "data-last=", "\"", "\"");
				if (++page>last)
					page = InvalidNUM;

				// Scan area
				vara jlist(f.memory, "showcaseListItem");
				for (int j=1; j<jlist.length(); ++j) {
					vars &item = jlist[j];
					vars link = burl(ubase, ExtractLink(item, "betabase/"));
					vars name = stripHTML(ExtractString(item, "<h3", ">", "</h3"));
					
					vars stars = stripHTML(ExtractString(item, "class=\"RatingValue\"", ">", "/5"));
					vars ratings = stripHTML(ExtractString(item, "class=\"Hint\"", ">", "</"));
					vars summary = stripHTML(ExtractString(item, "Canyon Rating:", "<dd>", "</dd"));
					vars raplong = ExtractString(item, "Rappels, Longest", "<dd>", "</dd");

					CSym sym(urlstr(link), name);
					double vstars = CDATA::GetNum(stars);
					double vratings = CDATA::GetNum(ratings);
					if ((vstars>0 && vratings<0) || (vstars<0 && vratings>0))
						Log(LOGERR, "Invalid star/ratings %s (%s) for %s", stars, ratings, link);
					else
						if (vratings>0)
							sym.SetStr(ITEM_STARS, starstr(vstars, vratings));
					GetSummary(sym, summary);
					CDoubleArrayList raps, len;
					if (GetValues(GetToken(raplong, 0, '|'), NULL, len))
						sym.SetNum(ITEM_LONGEST, len.Tail());
					if (GetValues(GetToken(raplong, 1, '|'), NULL, raps))
						sym.SetStr(ITEM_RAPS, Pair(raps));

					if (CCOLLECTIVE_Empty(sym) && MODE>-2)
						continue; // skip empty canyons

					printf("Downloading %s/%s %d/%d   \r", CData(page-1), CData(last), i, list.length());
					//if (!Update(symlist, sym, NULL, FALSE) && MODE>=-1)
					//	continue;

					if (CCOLLECTIVE_DownloadPage(f, sym.id, sym))
						if (!CCOLLECTIVE_Empty(sym))
							Update(symlist, sym, NULL, FALSE);
					}
				}
		}

	return TRUE;
}

int CCOLLECTIVE_DownloadConditions(const char *ubase, CSymList &symlist) 
{
	// ALL map 
	ubase = "canyoncollective.com";

	DownloadFile f;
	CString url = burl(ubase, "betabase");
	if (f.Download(url))
		{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
		}

	// tables for ratings
	vars table = ExtractString(f.memory, "<h3>Recent Condition Reports</h3>", "", "<script");
	vara list(table, "<li");
	for (int i=1; i<list.length(); ++i) 
		{
		vars title = ExtractString(list[i], "\"title\"", "", "</li");
		vars link = ExtractString(title, "href=", "\"", "\"");
		vars name = stripHTML(ExtractString(title, "href=", ">", "</a"));
		if (link.IsEmpty() || !strstr(link, "betabase"))
			continue;

		CSym sym(urlstr(burl(ubase, link)), name);
		if (CCOLLECTIVE_DownloadPage(f, sym.id, sym))
			{
			vars redirect = ExtractString(f.memory, "\"redirect\"", "value=\"", "\"");
			if (!redirect.IsEmpty())
				sym.id = urlstr(burl(ubase, redirect));
			UpdateCond(symlist, sym, NULL, FALSE);
			}
		}

	return TRUE;
}





// ===============================================================================================


// ===============================================================================================
vara visited;

int CNORTHWEST_DownloadSelect(DownloadFile &f, const char *ubase, const char *url, CSymList &symlist, vara &region)
{			
	//ASSERT(!strstr(url,"Cascade"));
	if (f.Download(burl(ubase, url)))
		{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
		}

	// tables for ratings
	vars name = stripHTML(ExtractString(f.memory, "<strong", ">", "</strong>"));
	vara list(ExtractString(f.memory, "<table", "", "</table>"), "href=");
	if (list.length()<=1)
		return FALSE;


for (int i=1; i<list.length(); ++i) {
	vars lnk = ExtractString(list[i], "\"", "", "\"");
	vars name = stripHTML(ExtractString(list[i], ">", "", "</a"));
	if (IsImage(lnk))
		continue;

	BOOL add = FALSE;
	if (TRUE) //GetToken(lnk, 1, '/').IsEmpty())
	{
			// folders
			if (IsSimilar(lnk, "http") || IsSimilar(lnk, "mailto"))
				continue;
			if (visited.indexOf(lnk)>=0)
				continue;
			visited.push(lnk);
			vars reg = stripHTML(ExtractString(list[i], ">", "", "</a"));
			vara oldregion = region; region.push(reg);
			if (!CNORTHWEST_DownloadSelect(f, ubase, lnk, symlist, region))
				add = TRUE;
				
			region = oldregion;
	}
	else
	{
			// canyons
			add = TRUE;
	}
	//add or not to add?
	if (name.IsEmpty() || region.IsEmpty()) 
		 continue;
/*
	 if (strstr(name, ".jpg") || strstr(name, ".JPG"))
		 continue;
	 if (strstr(name, ".png") || strstr(name, ".PNG"))
		 continue;
	 if (strstr(name, ".pdf") || strstr(name, ".PDF"))
		 continue;
*/
	 //patch!
	lnk.Replace("French_Cabin/French_Cabin_Creek.php", "French_Cabin/French_Cabin_Creek.php");

	CSym sym(urlstr(burl(ubase, lnk)));
	sym.SetStr(ITEM_DESC, UTF8(name));
	sym.SetStr(ITEM_ACA, ";;;;;;;;;;"+lnk);
	sym.SetStr(ITEM_REGION, region.join(";"));
	Update(symlist, sym, NULL, FALSE);
}

	return TRUE;
}

int CNORTHWEST_DownloadBeta(const char *ubase, CSymList &symlist) 
{
	// ALL map 

	DownloadFile f;
	vara region;
	CNORTHWEST_DownloadSelect(f, ubase, "beta.php", symlist, region);

	return TRUE;
}

// ===============================================================================================
int CUSA_DownloadBeta(DownloadFile &f, const char *ubase, const char *lnk, const char *region, CSymList &symlist)
{
		vars url = burl(ubase, lnk);
		if (f.Download(url))
			{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			return FALSE;
			}

		vara tables(f.memory, "<table");
		for (int t=1; t<tables.length(); ++t) {
		CString table = ExtractString(tables[t], "", ">", "</table");
		vara lines(table, "href");
			for (int l=1; l<lines.length(); ++l) {
				vars tr = lines[l];
				vars lnk = ExtractString(tr, "", "\"", "\"");
				vars name = ExtractString(tr, "", ">", "</a>");
				vars summary = ExtractString(tr, "</a>", "", "</tr>");
				if (lnk.IsEmpty() || strstr(name, "Frylette"))
					continue;

				vars lnkurl = burl(ubase, lnk);
				lnkurl.Replace("canyoneeringcentral.com", "canyoneeringusa.com");
				//lnkurl.Replace("//canyoneeringusa.com//", "//");
				CString ext = lnkurl.Right(4);
				if (IsSimilar(ext, ".php") || IsSimilar(ext, ".htm"))
					{
					if (f.Download(lnkurl))
						{
						Log(LOGERR, "ERROR: can't download url %.128s", lnkurl);
						return FALSE;
						}
					vars basehref = ExtractString(f.memory, "<base href=", "\"", "\"");
					if (!basehref.IsEmpty())
						lnkurl = basehref;
					}

				CSym sym(urlstr(lnkurl));
				sym.SetStr(ITEM_DESC, stripHTML(name));
				sym.SetStr(ITEM_REGION, "Utah;"+stripHTML(region));
				int nstars = 0;
				double stars = max(vara(summary, "/yellowStar").length(), vara(summary, "/yellow_star").length())-1;
				if (stars==1) stars=1.5;
				else if (stars==2) stars=3;
				else if (stars==3) stars=4.5;
				if (strstr(name,"Corral Hollow"))
					stars = 1;
				//ASSERT(!strstr(name,"Wild"));
				//ASSERT(!strstr(name,"Knotted Rope Ridge"));
				sym.SetStr(ITEM_STARS, starstr(stars, 1));
				GetSummary(sym, stripHTML(summary));
				if (sym.GetNum(ITEM_ACA)==InvalidNUM)
					sym.SetNum(ITEM_CLASS, 0);
				Update(symlist, sym, NULL, FALSE);
				}
			}
		return TRUE;
}

/*
int CUSA_DownloadRaves(const char *ubase2, CSymList &symlist)
{
	DownloadFile f;
	vara visitedlist;
	CString ubase = "canyoneeringusa.com";
	CString url = burl(ubase, "rave");
	visitedlist.push(url);
	int page = 1;
	double maxpage = -1;
	do
	{
	if (f.Download(url))
		{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
		}

	if (maxpage<0)
		maxpage = ExtractNum(f.memory, "Page 1 of", "", "<");

	vara list(f.memory, "class=\"nine");
	for (int i=1; i<list.length(); ++i) {

		}
	}
	while (page<=maxpage);
	return TRUE;
}
*/

int CUSA_DownloadBeta(const char *ubase2, CSymList &symlist)
{
	DownloadFile f;
	vara visitedlist;
	CString ubase = "canyoneeringusa.com";
	CString url = burl(ubase, "utah");
	visitedlist.push(url);
	if (f.Download(url))
		{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
		}

	// tables for ratings
	vara list(ExtractString(f.memory, ">Main Menu<", "<ul", "</ul"), "href=");
	for (int i=1; i<list.length(); ++i) {
		vars lnk = ExtractString(list[i], "", "\"", "\"");
		vars region = ExtractString(list[i], "", ">", "</a>");
		if (strstr(lnk, "escalante"))
			lnk = "utah/escalante/archives";
		if (strstr(lnk, "zion"))
			lnk = "utah/zion/technical";		
		vars url = burl(ubase, lnk);
		if (visitedlist.indexOf(url)>=0)
			continue;
		visitedlist.push(url);

		CUSA_DownloadBeta(f, ubase, lnk, region, symlist);
		}

	CUSA_DownloadBeta(f, ubase, "utah/zion/trails", "Zion National Park", symlist);
	CUSA_DownloadBeta(f, ubase, "utah/zion/off-trail", "Zion National Park", symlist);

	// patch for choprock
	CSym sym("http://www.canyoneeringusa.com/utah/escalante/choprock/","Choprock Canyon");
	sym.SetStr(ITEM_STARS, starstr(4.5,1));
	Update(symlist, sym, NULL, FALSE);

	// raves
	
	return TRUE;
}


// ===============================================================================================
int ASOUTHWEST_DownloadLL(DownloadFile &f, CSym &sym, CSymList &symlist)
{
	if (!UpdateCheck(symlist, sym) && sym.GetNum(ITEM_LAT)!=InvalidNUM && MODE>-2)
		return TRUE;
	const char *url = sym.id;
	if (f.Download(url))
		{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
		}
	double lat, lng;
	vara list(f.memory, "lat=");
	for (int i=1; i<list.length(); ++i)
		{
		lat = CDATA::GetNum(list[i].Trim("\"' "));
		lng = CDATA::GetNum(ExtractString(list[i],"lon=","","\"").Trim("\"' "));
		if (CheckLL(lat,lng)) {
			sym.SetNum(ITEM_LAT, lat);
			sym.SetNum(ITEM_LNG, lng);
			return TRUE;
			}
		}
	return FALSE;
}


int ASOUTHWEST_DownloadBeta(DownloadFile &f, const char *ubase, const char *sub, const char *region, CSymList &symlist) 
{
	CString url = burl(ubase, sub);
	if (f.Download(url))
		{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
		}

	// tables for ratings
	vara list(f.memory, "class=\"ag\"");
	for (int i=1; i<list.length(); ++i) {
		vars lnk = ExtractString(list[i], "href=", "\"", "\"").replace("slot_canyons/","");
		vars name = stripHTML(ExtractString(list[i], "href=", ">", "<"));
		vara stars(ExtractString(list[i], "class=\"star\"", ">", "<"), ";");
		CSym sym( urlstr(burl(ubase, lnk)) );
		sym.SetStr(ITEM_DESC, name);
		sym.SetStr(ITEM_REGION, region);
		if (stars.length()>0)
			sym.SetStr(ITEM_STARS, starstr(stars.length(),1));
		ASOUTHWEST_DownloadLL(f, sym, symlist);
		Update(symlist, sym, NULL, FALSE);
		}
  // tables for ratings
  vara slots(f.memory, "class=\"slot\"");
  for (int s=1; s<slots.length(); ++s) {
	vara list(slots[s], "<li>");
	for (int i=1; i<list.length(); ++i) {
		vars lnk = ExtractString(list[i], "href=", "\"", "\"").replace("slot_canyons/","");
		vars name = stripHTML(ExtractString(list[i], "href=", ">", "<"));
		vara stars(ExtractString(list[i], "class=\"star\"", ">", "<"), ";");
		CSym sym( urlstr(burl(ubase, lnk)) );
		sym.SetStr(ITEM_DESC, name);
		sym.SetStr(ITEM_REGION, region);
		if (stars.length()>0)
			sym.SetStr(ITEM_STARS, starstr(stars.length(),1));
		ASOUTHWEST_DownloadLL(f, sym, symlist);
		Update(symlist, sym, NULL, FALSE);
		}
	}

	// tables for regions
	vara urls(f.memory, "href=");
	for (int i=1; i<urls.length(); ++i) {
		vars lnk = ExtractString(urls[i], "", "\"", "\"");
		vars name = stripHTML(ExtractString(urls[i], "", ">", "<"));
		if (strstr(urls[i], "class=\"hotelbold\""))
			ASOUTHWEST_DownloadBeta(f, ubase, lnk, name, symlist);
		}

	return TRUE;
}


int ASOUTHWEST_DownloadBeta(const char *ubase, CSymList &symlist) 
{
	DownloadFile f;
	ASOUTHWEST_DownloadBeta(f, ubase, "index.html", "", symlist);
	return TRUE;
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
		for (d=strlen(text)-1; d>0 && !isdigit(text[d]); --d)
			if (isalpha(text[d]))
				lastalpha = d;
		if (lastalpha<0) lastalpha = d;
		for (d=0; d<lastalpha && !isdigit(text[d]); ++d)
			if (isalpha(text[d]))
				firstalpha = d;
		if (firstalpha<0) firstalpha = d;
		text = text.Mid( firstalpha, lastalpha+1 );
		int div = -1, last = strlen(text)-1;
		for (int i=1; i<last && div<0; ++i)
			if (isalpha(text[i]))
				div = i;

		const char *EW = "EeWw", *SN = "SsNn";
		if (isalpha(text[0]) && isalpha(text[last]))
			if (div>=0 && strchr(SN, text[div]))
				{
				int skip = 0, len = strlen(text);
				while (skip<len && !isspace(text[skip]))
					++skip;
				if (skip<len) ++skip;
				div -= skip;
				text = text.Mid(skip);
				}
			else
				{
				int skip = strlen(text)-1;
				while (skip>0 && !isspace(text[skip]))
					--skip;
				text = text.Mid(0, skip).Trim();
				}

		const char *dtext = text;
		last = strlen(text)-1;
		lat = lng = InvalidNUM;
		if (div>=0 && div<=text.GetLength())
			{
			if (strchr(SN, text[0]) && strchr(EW, text[div]))
				lat = dmstodeg(text.Mid(0, div)), lng = dmstodeg(text.Mid(div));
			else if (strchr(EW, text[0]) && strchr(SN, text[div]))
				lng = dmstodeg(text.Mid(0, div)), lat = dmstodeg(text.Mid(div));
			else if (strchr(SN, text[div]) && strchr(EW, text[last]))
				lat = dmstodeg(text.Mid(0, div+1)), lng = dmstodeg(text.Mid(div+1));
			else if (strchr(EW, text[div]) && strchr(SN, text[last]))
				lng = dmstodeg(text.Mid(0, div+1)), lat = dmstodeg(text.Mid(div+1));
			else
				lng = lat = InvalidNUM;
			}
}


int ExtractCoords(const char *memory, float &lat, float &lng, CString &desc)
{
	lat = lng = InvalidNUM;
	const char *sep = " \r\t\n;.0123456789NnSsWwEe-o'\"";
	for (int i=0; memory[i]!=0; ++i)
		if (memory[i]=='o')
			{
				int start, end, endnum = 0, startnum = 0;
				for (end=i; memory[end]!=0 && strchr(sep, memory[end]); ++end)
					if (isdigit(memory[end]))
						endnum = TRUE;;
				if (!endnum) continue;
				for (start=i; start>=0 && strchr(sep, memory[start]); --start)
					if (isdigit(memory[start]))
						startnum = TRUE;;
				if (!startnum) continue;

				int tstart = start;
				for ( tstart=start; tstart>0 && memory[tstart]!='.'; --tstart);
				desc = CString(memory+tstart, start-tstart+1).Trim("(). \n\r\t");

				CString text(memory+start+1, end-start-1);
				GetCoords(text, lat, lng);
				if (CheckLL(lat,lng))
					return end; // continue search
			}
	return -1; // end search
}


// ===============================================================================================
int DAVEPIMENTAL_DownloadBeta(const char *ubase, CSymList &symlist) 
{
	// ALL map 

	DownloadFile f;
	CString url = burl(ubase, "canyons.php");
	if (f.Download(url))
		{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
		}

	// tables for ratings
	vara list(f.memory, "/canyons/");
	if (list.length()<=1)
		Log(LOGERR, "Could not extract sections from url %.128s", url);
	for (int i=1; i<list.length(); ++i) {
			vars lnk = ExtractString(list[i], "?", "", "\"");
			CString url = burl(ubase, "/canyons/?"+lnk);
			if (f.Download(url))
				{
				Log(LOGERR, "ERROR: can't download url %.128s", url);
				continue;
				}

			

			vars link = url;
			vars name = stripHTML(ExtractString(f.memory, "class=\"ttl\"", ">", "<"));
			vars region = stripHTML(ExtractString(f.memory, "class=\"subttl\"", ">", "<"));
			vars summary = stripHTML(ExtractString(f.memory, "ACA Rating:", "", "<"));
			vars rating = ExtractString(f.memory, "Quality Rating:", "img/", "star");

			CSym sym(urlstr(link));
			sym.SetStr(ITEM_DESC, name);
			sym.SetStr(ITEM_REGION, region);
			GetSummary(sym, summary);
			double stars = CDATA::GetNum(rating);
			if (stars>0)
				sym.SetStr(ITEM_STARS, starstr(stars, 1));

			Update(symlist, sym, NULL, FALSE);
			printf("Downloading %d/%d   \r", i, list.length());
			}

	return TRUE;
}

// ===============================================================================================


int ONROPE_ExtractKML(const char *ubase, const char *url, inetdata *out, int fx)
{
	//if (!fx)
	//	return FALSE;


	DownloadFile f;
	vars id = GetKMLIDX(f, url);	
	if (id.IsEmpty())
		return FALSE; // not available
	
	CString credit = " (Data by " + CString(ubase) + ")";

	//alternate go/gpx/download/578
	CString url2 = burl(ubase,id);
	if (DownloadRetry(f,url2))
		{
		Log(LOGERR, "ERROR: can't download url %.128s", url2);
		return FALSE;
		}

	return GPX_ExtractKML(credit, url, f.memory, out);
}




int ONROPE_DownloadBeta(const char *ubase, CSymList &symlist) 
{
	// ALL map 

	DownloadFile f;
	CString url = burl(ubase, "about-canyon-beta");
	if (f.Download(url))
		{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
		}

	// tables for ratings
	vara list(f.memory, "class=\"external-link\"");
	if (list.length()<=1)
		Log(LOGERR, "Could not extract sections from url %.128s", url);
	for (int i=1; i<list.length(); ++i) {
		vars lnk = ExtractString(list[i], "href=");
		CString url = burl(ubase, lnk);
		if (f.Download(url))
			{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			continue;
			}
		vara jlist(f.memory, "class=\"page-collection\"");
		if (jlist.length()<=1)
			Log(LOGERR, "Could not extract sections from url %.128s", url);
		for (int j=1; j<jlist.length(); ++j) {
			vars lnk = ExtractString(jlist[j], "href=");
			CString url = burl(ubase, lnk);
			printf("Downloading %d/%d %d/%d   \r", i, list.length(), j, jlist.length());

			if (f.Download(url))
				{
				Log(LOGERR, "ERROR: can't download url %.128s", url);
				continue;
				}

			vars link = url;
			vars name = GetToken(ExtractString(f.memory, "<h2", ">", "<"),0,',').Trim();
			vars region = stripHTML(ExtractString(f.memory, ">Location:", "", "<br"));
			vars longest = ExtractString(f.memory, ">Longest Rappel:", "", "<").Trim();
			vars summary = ExtractString(f.memory, ">Rating:", "", "<").Trim();
			//ASSERT(!strstr(url, "pearl"));
			vars kmlidx = ExtractLink(f.memory, ".gpx");

			CSym sym(urlstr(link));
			sym.SetStr(ITEM_DESC, name);
			sym.SetStr(ITEM_REGION, region);
			sym.SetStr(ITEM_KML, kmlidx);
			CDoubleArrayList len;
			GetSummary(sym, summary);
			if (GetValues(longest, ulen, len))
				sym.SetNum(ITEM_LONGEST, len.Tail());

			Update(symlist, sym, NULL, FALSE);
			}
		}

	return TRUE;
}


int GetMin(const char *str, double &v2)
{
	while (isspace(*str))
		++str;
	if (isdigit(*str))
			if ((v2=CDATA::GetNum(str))!=InvalidNUM)
				return TRUE;
	return FALSE;
}

const char *GetNumUnits(const char *str, const char *&num)
{
	num = NULL;
	while (!isdigit(*str) && *str!=0)
		++str;

	if (*str==0)
		return NULL;

	num = str;
	while (isdigit(*str) || (*str=='.') || (*str==' '))
		++str;

	if (*str==0)
		return NULL;

	return str;
}


double GetHourMin(const char *ostr, const char *num, const char *unit, const char *url)
{
	char x = 0;
	if (unit) x = *unit;
	int nounit = FALSE;
	double vd = 0, vh = 0, vm = 0, v, v2;
	if ((v = CDATA::GetNum(num))==InvalidNUM) {
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
				if (GetMin(unit+1, v2))
					vm = v2, unit = GetNumUnits(unit+1, num);
				break;

			case 'm': // 45min
			case '\'': // 30'
				vm = v;
				if (GetMin(unit+1, v2))
					vh = vm, vm = v2, unit = GetNumUnits(unit+1, num);
				break;
			default:
				// autounit: >=5 min, otherwise hours
				nounit = TRUE;
				if (v>=5) 
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

	if (vd>=5 || vd<0 || vh>=24 || vh<0 || vm>=120 || vm<0)
		{
		Log(LOGERR, "Invalid HourMin %s = [%sd %sh %sm] from %s", ostr, CData(vd), CData(vh), CData(vm), url);
		return InvalidNUM;
		}

	if (nounit)
		Log(LOGWARN, "Possibly invalid HourMin %s = [%sh %sm]? from %s", ostr, CData(vh), CData(vm), url);

	if (vd==0 && vh==0 && vm==0)
		vm = 5; // 5 min

	return vd*24 + vh + vm/60;
}


void GetHourMin(const char *str, double &vmin, double &vmax, const char *url)
{
	vmin = vmax = 0;
	if (*str==0) {
		return;
		}
/*
		if (breakutf) {
		out.Replace("\xC2", " ");
		out.Replace("\xA0", " ");
		}
*/
	vars hmin;
	for (int i=0; str[i]!=0; ++i)
		{
		char c = str[i];
		if (c<' ' || c>127 || c=='/' || c=='-' || c==',' || c=='+' || c==';')
			c='-';
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
	for (int i=0; i<hmina.length(); ++i) {

		const char *num, *x, *unit; 

		// get num and units (possible inherited)
		unit = GetNumUnits(hmina[i], num);
		for (int j=i+1; !unit && j<hmina.length(); ++j)
			unit = GetNumUnits(hmina[j], x);
		// process value
		if (num!=NULL)
			{
			double v = GetHourMin(hmina[i], num, unit, url);
			if (v>0) time.AddTail(v);
			}
		}
		
	if (time.length()==0)
		return;  // empty

	time.Sort();
	vmin = time[0];
	vmax = time[time.length()-1];
}


double round2(double val)
{
	double ival = (int)val;
	double fval = val - ival;

	if (fval<0.25) fval = 0;
	else if (fval<0.75) fval = 0.5;
	else if (fval<1) fval = 1.0;

	return ival + fval;
}

void GetTotalTime(CSym &sym, vara &times, const char *url, double maxtmin)  // -1 to skip tmin checks
{
		if (times.length()!=3)
			{
			Log(LOGWARN, "GETTIME: Invalid 3 times [len=%d], skipping %s", times.length(), url);
			return;
			}

		//CDoubleArrayList times;
		int error = 0;
		CDoubleArrayList tmins, tmaxs;
		double tmin = 0, tmax = 0;
		for (int i=0; i<3; ++i)
			{
			double vmin = 0, vmax = 0;
			if (times[i].trim().IsEmpty())
				return;
			GetHourMin(times[i], vmin, vmax, url);
			if (vmin<=0)
				{
				++error;
				Log(LOGWARN, "GETTIME: invalid time '%s', skipping %s", times[i], url);
				return;
				}
			if (vmax<=0) vmax = vmin;
			tmaxs.AddTail(vmax);
			tmins.AddTail(vmin);
			tmin += vmin;
			tmax += vmax;
			}


		if (tmin<=0)
			return;

		if (maxtmin>=0 && tmin>0 && tmax>0 && tmax>=tmin*2)
			{
			Log(LOGWARN, "GETTIME: Invalid tmax (%s > 2*%s), skipping %s", CData(tmax), CData(tmin), url);
			return;
			}

		if (maxtmin>=0 && tmin>=maxtmin)
			{
			Log(LOGWARN, "GETTIME: Very high tmin (%s > %s), skipping %s", CData(tmin), CData(maxtmin), url);
			return;
			}

		// all good, set values
		sym.SetNum(ITEM_MINTIME, tmin);
		sym.SetNum(ITEM_MAXTIME, tmax);

		int mins[] = { ITEM_AMINTIME, ITEM_DMINTIME, ITEM_EMINTIME };
		int maxs[] = { ITEM_AMAXTIME, ITEM_DMAXTIME, ITEM_EMAXTIME };
		for (int i=0; i<3; ++i)
			{
			sym.SetNum(mins[i], tmins[i]);
			sym.SetNum(maxs[i], tmaxs[i]);
			}
}


CString noHTML(const char *str)
{
	CString ret = stripHTML(str);
	ret.Replace(",",";");
	ret.Trim("*");
	ret.Trim(".");
	return  ret;
}

// ===============================================================================================
#define CANYONCARTOTICKS 3000
static double canyoncartoticks;
double CANYONCARTO_Stars(double stars)
{
	if (stars==InvalidNUM)
		return InvalidNUM;
	
	stars = stars/7*5;
	return floor(stars*10+0.5)/10;
}

int CANYONCARTO_DownloadRatings(CSymList &symlist) 
{
	DownloadFile f;
	vars url = "http://canyon.carto.net/cwiki/bin/view/Canyons/BewertungsResultate";
	Throttle(canyoncartoticks, CANYONCARTOTICKS);
	if (f.Download(url))
		{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
		}

	// gather data
	CSymList votes;
	vara list(ExtractString(f.memory, "<table", "", "</table"), "<tr");
	for (int i=1; i<list.length(); ++i)
		{
		vars id = ExtractString(list[i], "href=", ">", "<");
		vars user = ExtractString(list[i], "/bin/view/Main/", "", "\"");
		double stars = ExtractNum(list[i], "Bewertung=", "", NULL);
		if (stars==InvalidNUM)
			stars = ExtractNum(list[i], "Bewertung</a>", "=", NULL);
		if (id.IsEmpty() || user.IsEmpty() || stars==InvalidNUM)
			{
			Log(LOGERR, "Invalid vote entry #%d %.256s", i, list[i]);
			continue;
			}

		CSym sym( id+"@"+user, urlstr("http://canyon.carto.net/cwiki/bin/view/Canyons/"+id));
		sym.SetNum(ITEM_LAT, stars);

		int f = votes.Find(sym.id);
		if (f<0)
			votes.Add(sym); // add
		else
			votes[f] = sym; // update
		}	

	votes.Sort();

	// compute stars
	for (int i=0; i<votes.GetSize(); )
		{
		// compute average
		double sum = 0, num = 0;
		vars id = votes[i].GetStr(ITEM_DESC);
		while (i<votes.GetSize() && votes[i].GetStr(ITEM_DESC)==id)
			{
			sum += votes[i].GetNum(ITEM_LAT);
			++num;
			++i;
			}
		// add to list
		int f = symlist.Find(id);
		if (f<0)
			{
			Log(LOGWARN, "Unmatched CANYONCARTO vote for %s", id);
			continue;
			}

		symlist[f].SetStr(ITEM_STARS, starstr(CANYONCARTO_Stars(sum/num), num));
		}

	return TRUE;
}

int CANYONCARTO_DownloadPage(DownloadFile &f, const char *url, CSym &sym, CSymList &symlist) 
{
	// Using CANYONCARTO_Downloadratings instead!
/*
// download list of recent additions
	Throttle(canyoncartoticks, CANYONCARTOTICKS);
	if (f.Download(url))
		{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
		}

	// current stars
	double oldstars = sym.GetNum(ITEM_STARS);
	double newstars = CANYONCARTO_Stars(ExtractNum(f.memory, "Gesamt:", "", "<"));
	double ratings = ExtractNum(f.memory, "Bew total:", "", "<");
	if (ratings>0 && abs(newstars - oldstars)>0.5)
		Log(LOGWARN, "Inconsisten stars %s != %s for %s", CData(oldstars), CData(newstars), sym.id);
	if (ratings!=InvalidNUM) //stars!=InvalidNUM && 
		{
		sym.SetStr(ITEM_STARS, starstr(oldstars, ratings));
		return TRUE;
		}
	return FALSE;
*/
	return TRUE;
}

int CANYONCARTO_DownloadBeta(const char *ubase, CSymList &symlist) 
{
	//vara recentlist, urllist;
	//const char *canyonlink = "href=\"/canyoning/canyon/";
	//const char *canyondesclink = "/canyoning/canyon-description/";
	//varas months("janvier=Jan,février=Feb,mars=Mar,avril=Apr,mai=May,juin=Jun,juillet=Jul,août=Aug,septembre=Sep,octobre=Oct,novembre=Nov,décembre=Dec");

	DownloadFile f;
	CString url = "http://canyon.carto.net/cwiki/bin/custom/cmap2kml.pl";

	// download list of recent additions
	if (f.Download(url))
		{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
		}

	vars err;
	vara list( ExtractString(f.memory, "", "<Folder>", "</Folder>"), "<Placemark>");
	for (int i=1; i<list.length(); ++i)
		{
		const char *memory = list[i];
		vars link = ExtractString(memory, "href=", "'", "'").replace("http:/canyon", "http://canyon");
		vars name = UTF8(stripHTML(ExtractString(memory, "", "<name>", "</name>")));
		CSym sym(urlstr(link), name);

		vara coords(ExtractString(memory, "", "<coordinates>", "</coordinates>"));
		if (coords.length()>1)
		{
		double lat = CDATA::GetNum(coords[1]);
		double lng = CDATA::GetNum(coords[0]);
		if (CheckLL(lat,lng))
			{
			sym.SetNum(ITEM_LAT, lat);
			sym.SetNum(ITEM_LNG, lng);
			}
		}
		
		vars summary = stripHTML(ExtractString(memory, ">Bewertung", "<td>", "</td>"));
		GetSummary(sym, summary);

		vars aka = UTF8(stripHTML(ExtractString(memory, ">Aliasname", "<td>", "</td>")));
		sym.SetStr(ITEM_AKA, name+";"+aka.replace(",",";"));

		double tmax, tmin;
		vars time = stripHTML(ExtractString(memory, ">Gesamtzeit", "<td>", "</td>").replace("ka",""));
		GetHourMin(time, tmin, tmax, link);
		if (tmin>0)
			sym.SetNum(ITEM_MINTIME, tmin);
		if (tmax>0)
			sym.SetNum(ITEM_MINTIME, tmax);

		sym.SetStr(ITEM_DEPTH, GetMetric(stripHTML(ExtractString(memory, ">Höhendiff.", "<td>", "</td>"))));

		CDoubleArrayList max;
		if (GetValues(memory, ">max.", "<td>", "</td>", ulen, max))
			sym.SetNum(ITEM_LONGEST, max.Tail());

		sym.SetStr(ITEM_KML, "X");

		vars votes = stripHTML(ExtractString(memory, ">subj.", "<td>", "</td>"));
		double stars = CANYONCARTO_Stars(CDATA::GetNum(votes));
		//double ratings = CDATA::GetNum(GetToken(votes, 2, ' '));
		if (stars>0 && err.IsEmpty()) 
			{
#if 0
			sym.SetStr(ITEM_STARS, starstr(stars*5/7, 1));
#else
			sym.SetStr(ITEM_STARS, starstr(stars, 0));
			double newstars = sym.GetNum(ITEM_STARS);
			if (newstars>0)
			  {
			  int find = symlist.Find(link);
			  if (find>=0)
				{
				CString o = symlist[find].GetStr(ITEM_STARS);
				CString n = sym.GetStr(ITEM_STARS);
				if (GetToken(o, 0, '*')!=GetToken(n, 0, '*'))
					find = -1;
				}
			  if (find<0 || MODE<=-2)
				{
				// download new stars
				if (!CANYONCARTO_DownloadPage(f, link, sym, symlist)) {
					err = "ERROR";
					Log(LOGERR, "CANYONCARTO_DownloadBeta blocked for %s ???", sym.id);
					}
				}
			  else
				{
				// preserve old stars
				sym.SetStr(ITEM_STARS, symlist[find].GetStr(ITEM_STARS));
				}
			  }
#endif
			}
		printf("%d %d/%d %s   \r", symlist.GetSize(), i, list.length(), err);
		Update(symlist, sym, NULL, FALSE);			
		}

	CANYONCARTO_DownloadRatings(symlist);
	return TRUE;
}

CString CANYONCARTO_Watermark(const char *scredit, const char *memory, int size)
{
	CString credit(scredit);
	CString kml(memory, size);
	kml.Replace("Einstiege", "Start");
	kml.Replace("Einstieg", "Start");
	kml.Replace("Ausstiege", "End");
	kml.Replace("Ausstieg", "End");
	kml.Replace("ParkPla\xE3\xB3\xBA\x65", "Parking");
	kml.Replace("ParkPlatz", "Parking");

	const char *desc = "</description>";
	const char *name = "</name>";
	vara names(kml, name);
	for (int i=1; i<names.length(); ++i)
		if (names[i].Replace(desc, credit+desc)<=0)
			names[i].Insert(0, "<description>"+credit+desc);
	return names.join(name);
}


//http://canyon.carto.net/cwiki/bin/view/Canyons/Bodengo3Canyon
int CANYONCARTO_ExtractKML(const char *ubase, const char *url, inetdata *out, int fx)
{
	CString credit = " (Data by " + CString(ubase) + ")";
	CString kmlurl = "http://canyon.carto.net/cwiki/bin/custom/cmap2kml.pl?canyon="+GetToken(GetToken(url, GetTokenCount(url, '/')-1, '/'), 0, '&');
	Throttle(canyoncartoticks, CANYONCARTOTICKS);
	return KMZ_ExtractKML(credit, kmlurl, out, CANYONCARTO_Watermark);
}

// ===============================================================================================

int WROCLAW_DownloadPage(DownloadFile &f, const char *url, CSym &sym) 
{
	// download list of recent additions
	Throttle(canyoncartoticks, 1000);
	if (f.Download(url))
		{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
		}

	//double stars = ExtractNum(f.memory, "Gesamt:", "", "<");
	vars desc;
	float lat = InvalidNUM, lng = InvalidNUM;
	vars gps = stripHTML(ExtractString(f.memory, ">GPS", ":", "</table"));
	gps.Replace("&#176;","o");
	ExtractCoords(gps, lat, lng, desc);
	if (CheckLL(lat,lng))
		{
		sym.SetNum(ITEM_LAT, lat);
		sym.SetNum(ITEM_LNG, lng);
		}

	vara list(ExtractString(f.memory, ">Wasze opinie", "", "</table"), "<td");
	int n = 0;
	double sum = 0;
	for (int i=1; i<list.length(); ++i)
		{
		vara len(list[i], "star.gif");
		if (len.length()>1)
			{
			sum += len.length()-1;
			++n;
			}
		}
	if (n>0)
		sym.SetStr(ITEM_STARS, starstr(sum/n, n));


	sym.SetStr(ITEM_LONGEST, GetMetric(stripHTML(ExtractString(f.memory, "max kaskada:", "", "<"))));

	vars depthlength = stripHTML(ExtractString(f.memory, "deniwelacja:", "", "</td"));
	sym.SetStr(ITEM_DEPTH, GetMetric(GetToken(depthlength, 0, ':')));
	sym.SetStr(ITEM_LENGTH, GetMetric(GetToken(depthlength, 1, ':')));

	vars rock = stripHTML(ExtractString(strstr(f.memory, "ska\xC5\x82"), ":", "", "</td"));
	sym.SetStr(ITEM_ROCK, rock);

	vara times;
	vars memory = f.memory;
	memory.Replace("\xc5\x9b","s");
	memory.Replace("\xc3\xb3","o");
	const char *ids[] = { "dojscie:", "kanion:", "powrot:" };
	//ASSERT( !strstr(url,"/kanion.php?id=41"));
	for (int t=0; t<sizeof(ids)/sizeof(*ids); ++t)
		{
		vars time = stripHTML(ExtractString(memory, ids[t], "", "<"));
		if (!time.Replace("min", "m"))
			time+="m";
		times.push(time);	
		}
	GetTotalTime(sym, times, url);

	return TRUE;
}

int WROCLAW_DownloadBeta(const char *ubase, CSymList &symlist) 
{
	//vara recentlist, urllist;
	//const char *canyonlink = "href=\"/canyoning/canyon/";
	//const char *canyondesclink = "/canyoning/canyon-description/";
	//varas months("janvier=Jan,février=Feb,mars=Mar,avril=Apr,mai=May,juin=Jun,juillet=Jul,août=Aug,septembre=Sep,octobre=Oct,novembre=Nov,décembre=Dec");

	DownloadFile f;
	CString url = "http://www.canyoning.wroclaw.pl/bylismy.php?r=0&s=a";

	// download list of recent additions
	if (f.Download(url))
		{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
		}

	vara list( ExtractString(f.memory, "class=\"lista\"", "", "</table>"), "<tr");
	for (int i=1; i<list.length(); ++i)
		{
		const char *memory = list[i];
		vars link = ExtractString(memory, "href=", "\"", "\"");
		vars name = UTF8(stripHTML(ExtractString(memory, "href=", ">", "</a>")));
		if (link.IsEmpty())
			continue;

		CSym sym(urlstr("http://www.canyoning.wroclaw.pl/"+link), name);
		printf("%d/%d   \r", i, list.length());
		if (!UpdateCheck(symlist, sym) && MODE>-2)
			continue;
		if (WROCLAW_DownloadPage(f, sym.id, sym))
			Update(symlist, sym, NULL, FALSE);
		//ASSERT(!strstr(sym.GetStr(ITEM_DESC),"Corredora"));

		}

	return TRUE;
}



// ===============================================================================================
void LoadRWList();

vars ExtractHREF(const char *str)
{
	if (!str) return "";
	return GetToken(ExtractString(str, "href=", "", ">"), 0, " \n\r\t<").Trim("'\"");
}

int INFO_DownloadBeta(const char *ubase, CSymList &symlist) 
{
	//vara recentlist, urllist;
	//const char *canyonlink = "href=\"/canyoning/canyon/";
	//const char *canyondesclink = "/canyoning/canyon-description/";
	//varas months("janvier=Jan,février=Feb,mars=Mar,avril=Apr,mai=May,juin=Jun,juillet=Jul,août=Aug,septembre=Sep,octobre=Oct,novembre=Nov,décembre=Dec");

	// initialize data structures 
	if (rwlist.GetSize()==0)
		LoadRWList();

	DownloadFile f;
	CString url = "http://www.caudal.info/informe_ropewiki.php";

	// download list of recent additions
	if (f.Download(url))
		{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
		}

	vara rlist( ExtractString(f.memory,"<table","","</table"), "<tr");
	for (int r=2; r<rlist.length(); ++r)
	  {
		vara td(rlist[r],"<td");
		if (td.length()<6)
			continue;

		vars num = stripHTML(ExtractString(td[1], ">", "", "</td"));
		vars date = stripHTML(ExtractString(td[3], ">", "", "</td"));
		double vdate = CDATA::GetDate(date, "DD/MM/YYYY" );
		if (vdate<=0)
			{
			Log(LOGERR, "Invalid date '%s'", date);
			continue;
			}

		vars name = stripHTML(ExtractString(td[4], ">", "", "</td"));
		vars id = urlstr(ExtractHREF(td[5]));
		vars rwlink = urlstr(ExtractHREF(td[6]));
		if (!rwlink.Replace("http://ropewiki.com/", ""))
			continue;
		rwlink.Replace("_", " ");
		if (id.IsEmpty())
			{
			Log(LOGERR, "Invalid infid '%s'", id);
			continue;
			}

		CSym sym(urlstr(id), name);

		for (int i=0; i<rwlist.GetSize(); ++i)
			if (rwlist[i].GetStr(ITEM_DESC)==rwlink)
				{
				sym.id = RWTITLE+rwlink;
				sym.SetStr(ITEM_MATCH, rwlist[i].id);
				sym.SetStr(ITEM_INFO, rwlist[i].id);
				break;
				}

		//enum { W_DRY=0, W_WADING=1, W_SWIMMING=2, W_VERYLOW=3, W_LOW=4, W_MODLOW=5, W_MODERATE=6, W_MODHIGH=7, W_HIGH=8, W_VERYHIGH=9, W_EXTREME=10 };
		static vara watercc("9=/,0=0,1=1,2=2,3=3,4=4,5=6,6=8,7=9,8=10,9=/");
		
		//static vara cond_temp("0 - None,1 - Rain jacket,2 - Thin wetsuit,3 - Full wetsuit,4 - Thick wetsuit,5 - Drysuit");
		//static vara tempcc("NA=/,None=0,Waterproof=1,Fleece=1,1-3mm=2,4-5mm=3,6mm+=4,Drysuit=5");

		//static vara cond_diff("0 - Nontechnical,1 - Easy,2 - Normal,3 - Special challenges,4 - Advanced,5 - Extreme");
		//static vara diffcc("NA=/,Easy=1,Moderate=2,Difficult=3,Strenuous=4,Very strenuous=5");
									
		vara cond; 
		cond.SetSize(COND_MAX);
		cond[COND_DATE] = CDate(vdate);
		cond[COND_WATER] = TableMatch(num, watercc, cond_water);
		cond[COND_LINK] = id;
		//cond[COND_TEMP] = TableMatch(stripHTML(ExtractString(memory, ">Thermal:", "", "</br")), tempcc, cond_temp);
		//cond[COND_DIFF] = TableMatch(stripHTML(ExtractString(memory, ">Difficulty:", "", "</br")), diffcc, cond_diff);

		sym.SetStr(ITEM_CONDDATE, cond.join(";"));
		UpdateCond(symlist, sym, NULL, FALSE);
	  }

	// avoid "malfunction" message
	for (int i=0; i<symlist.GetSize(); ++i)
			++symlist[i].index;

	return TRUE;
}

// ===============================================================================================
int COND365_DownloadBeta(const char *ubase, CSymList &symlist) 
{
	DownloadFile f;
	CString url = burl(ubase, "");

	// download list of recent additions
	if (f.Download(url))
		{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
		}

	double date = CDATA::GetDate(ExtractString( f.memory, "datetime=", "\"", "\""));
	vars title = stripHTML(ExtractString( f.memory, "datetime=", "title=\"", "\""));
	if (date>0 && IsSimilar(title,"Info Caudales"))
		for (int i=0; i<rwlist.GetSize(); ++i)
			if (strstr(rwlist[i].GetStr(ITEM_MATCH), ubase))
				{
				vars title = rwlist[i].GetStr(ITEM_DESC);
				vara cond; cond.SetSize(COND_MAX);
				cond[COND_DATE] = CDate(date);
				cond[COND_LINK] = ubase;
				CSym sym(RWTITLE+title, title);
				sym.SetStr(ITEM_CONDDATE, cond.join(";"));
				Update(symlist, sym, NULL, FALSE);
				}
	return TRUE;
}

int CANDITION_DownloadBeta(const char *ubase, CSymList &symlist) 
{
	//vara recentlist, urllist;
	//const char *canyonlink = "href=\"/canyoning/canyon/";
	//const char *canyondesclink = "/canyoning/canyon-description/";
	//varas months("janvier=Jan,février=Feb,mars=Mar,avril=Apr,mai=May,juin=Jun,juillet=Jul,août=Aug,septembre=Sep,octobre=Oct,novembre=Nov,décembre=Dec");

	DownloadFile f;
	CString url = "http://candition.com";

	// download list of recent additions
	if (f.Download(url))
		{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
		}

	vara rlist( f.memory, "showonlyone(");
	for (int r=1; r<rlist.length(); ++r)
	  {
	  vars region = stripHTML(ExtractString(rlist[r], "'", "", "'"));
	  vara list( rlist[r], "href=");
	  for (int i=1; i<list.length(); ++i)
		{
		const char *memory = list[i];
		vars link = ExtractString(memory, "\"", "", "\""); 
		vars name = stripHTML(ExtractString(memory, "", ">", "</a>"));
		if (link.IsEmpty() || !strstr(link, "/canyons/"))
			continue;
			
		CSym sym(urlstr(url+link), name);
		ASSERT(!name.IsEmpty());
		sym.SetStr(ITEM_REGION, region.replace(" - ", ";"));

		// condition
		vars date = stripHTML(ExtractString(memory, "Last Candition:", "", "</div"));
		double vdate = CDATA::GetDate(date, "MMM D; YYYY" );
		if (vdate<=0) // ignore if never posted any condition
			continue;

		vars stars;
		double vstars = CDATA::GetNum(stripHTML(ExtractString(memory, "rating_box", ">", "</div")));
		if (vstars>=0 && vstars<=cond_stars.length())
			stars = cond_stars[(int)vstars];
		//sym.SetStr(ITEM_CONDDATE, CDate(vdate)+";"+stars);
		// don't use stars
		sym.SetStr(ITEM_CONDDATE, CDate(vdate));
		Update(symlist, sym, NULL, FALSE);
		}
	  }
	return TRUE;
}


// ===============================================================================================


// ===============================================================================================

int CANYONEAST_DownloadPage(DownloadFile &f, const char *url, CSym &sym) 
{
	// download list of recent additions
	static double ticks;
	Throttle(ticks, 1000);
	if (f.Download(url))
		{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
		}

	//double stars = ExtractNum(f.memory, "", "", "<");
	vars desc;
	float  lat = InvalidNUM, lng = InvalidNUM;
	vars gps = stripHTML(ExtractString(f.memory, "punto di accesso al greto", ":", "<br"));
	gps.Replace("°","o");
	gps.Replace(";",".");
	ExtractCoords(gps, lat, lng, desc);
	if (CheckLL(lat,lng))
		{
		sym.SetNum(ITEM_LAT, lat);
		sym.SetNum(ITEM_LNG, lng);
		}

	sym.SetStr(ITEM_SEASON, stripHTML(ExtractString(f.memory, ">Periodo", ":", "<br")));
	
	GetSummary(sym, stripHTML(ExtractString(f.memory, "Difficoltà:", "", "<br")));

	sym.SetStr(ITEM_LONGEST, GetMetric(stripHTML(ExtractString(f.memory, " alta:", "", "<br"))));
	sym.SetStr(ITEM_LENGTH, GetMetric(stripHTML(ExtractString(f.memory, "Sviluppo:", "", "<br"))));
	sym.SetStr(ITEM_DEPTH, GetMetric(stripHTML(ExtractString(f.memory, "Dislivello:", "", "<br"))));
	
	sym.SetStr(ITEM_SHUTTLE, stripHTML(ExtractString(f.memory, "Navetta:", "", "<br").replace(" ","").replace("Circa","").replace("No/","Optional ").replace(",",".")));

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

	vara times;
	const char *ids[] = { "Avvicinamento:", "Progressione:", "Ritorno:" };
	//ASSERT( !strstr(url,"/kanion.php?id=41"));
	for (int t=0; t<sizeof(ids)/sizeof(*ids); ++t)
		{
		CString time = stripHTML(ExtractString(f.memory, ids[t], "", "<br"));
		while (!ExtractStringDel(time, "(", "", ")").IsEmpty());
		times.push(vars(time).replace("ore", "h").replace("giorni", "j").replace(";","."));	
		}
	GetTotalTime(sym, times, url);

	return TRUE;
}

int CANYONEAST_DownloadBeta(const char *ubase, CSymList &symlist) 
{
	//vara recentlist, urllist;
	//const char *canyonlink = "href=\"/canyoning/canyon/";
	//const char *canyondesclink = "/canyoning/canyon-description/";
	//varas months("janvier=Jan,février=Feb,mars=Mar,avril=Apr,mai=May,juin=Jun,juillet=Jul,août=Aug,septembre=Sep,octobre=Oct,novembre=Nov,décembre=Dec");

	DownloadFile f;
	CString url = "http://www.canyoneast.it/target.php?w=1920&lng=it&target=Elenco";

	// download list of recent additions
	if (f.Download(url))
		{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
		}

	vara list( ExtractString(f.memory, "Elenco Canyons", "", "\"endele\""), "<li>");
	for (int i=1; i<list.length(); ++i)
		{
		const char *memory = list[i];
		vars link = ExtractString(ExtractString(memory, "href=", "", "</a>"), "\"", "", "\""); 
		vars name = UTF8(stripHTML(ExtractString(memory, "href=", ">", "</a>")));
		if (link.IsEmpty())
			continue;

		CSym sym(urlstr(link.replace("www.","WWW.")), name);
		printf("%d/%d   \r", i, list.length());
		if (!UpdateCheck(symlist, sym) && MODE>-2)
			continue;
		if (CANYONEAST_DownloadPage(f, sym.id, sym))
			Update(symlist, sym, NULL, FALSE);
		//ASSERT(!strstr(sym.GetStr(ITEM_DESC),"Corredora"));

		}

	return TRUE;
}

// ===============================================================================================

int TNTCANYONING_DownloadPage(DownloadFile &f, const char *url, CSym &sym) 
{
	// download list of recent additions
	static double ticks;
	Throttle(ticks, 1000);
	if (f.Download(url))
		{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
		}

	GetSummary(sym, stripHTML(ExtractString(f.memory, ">Difficolt", ":", "</p")));

	sym.SetStr(ITEM_SEASON, stripHTML(ExtractString(f.memory, ">Periodo", ":", "</p")));
	
	sym.SetNum(ITEM_RAPS, CDATA::GetNum(stripHTML(ExtractString(f.memory, "Numero calate", ":", "</p"))));
	sym.SetStr(ITEM_LONGEST, GetMetric(stripHTML(ExtractString(f.memory, "Calata max", ":", "</p"))));
	sym.SetStr(ITEM_LENGTH, GetMetric(stripHTML(ExtractString(f.memory, ">Sviluppo", ":", "</p"))));
	sym.SetStr(ITEM_DEPTH, GetMetric(stripHTML(ExtractString(f.memory, ">Dislivello", ":", "</p"))));
	
	sym.SetStr(ITEM_SHUTTLE, stripHTML(ExtractString(f.memory, ">Navetta", ":", "</p").replace(" ","").replace("Circa","").replace("Si,","").replace("si,","").replace(",",".")));

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
	const char *ids[] = { "Tempo Avvicinamento", "Tempo Discesa", "Tempo Ritorno" };
	//ASSERT( !strstr(url,"/kanion.php?id=41"));
	for (int t=0; t<sizeof(ids)/sizeof(*ids); ++t)
		{
		CString time = stripHTML(ExtractString(f.memory, ids[t], ":", "</p"));
		while (!ExtractStringDel(time, "(", "", ")").IsEmpty());
		time.Trim("\t -");
		if (!time.IsEmpty()) time = vars(time).split("Tempo").first();
		times.push(vars(time).replace("ore", "h").replace("giorni", "j").replace(";","."));	
		}
	GetTotalTime(sym, times, url);

	return TRUE;
}

int TNTCANYONING_DownloadBeta(const char *ubase, CSymList &symlist) 
{
	//vara recentlist, urllist;
	//const char *canyonlink = "href=\"/canyoning/canyon/";
	//const char *canyondesclink = "/canyoning/canyon-description/";
	//varas months("janvier=Jan,février=Feb,mars=Mar,avril=Apr,mai=May,juin=Jun,juillet=Jul,août=Aug,septembre=Sep,octobre=Oct,novembre=Nov,décembre=Dec");

	DownloadFile f;
	CString url = "http://www.tntcanyoning.it/";

	// download list of recent additions
	if (f.Download(url))
		{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
		}

	vara list( ExtractString(f.memory, ">Canyon Percorsi", "", "</ul"), "canyon-percorsi/");
	for (int i=1; i<list.length(); ++i)
		{
		const char *memory = list[i];
		vars link = ExtractString(memory, "", "", "\""); 
		vars name = UTF8(stripHTML(ExtractString(memory, "", ">", "</a>")));
		if (link.IsEmpty())
			continue;

		CSym sym(urlstr("http://www.tntcanyoning.it/canyon-percorsi/"+link), name);
		printf("%d/%d   \r", i, list.length());
		if (!UpdateCheck(symlist, sym) && MODE>-2)
			continue;
		if (TNTCANYONING_DownloadPage(f, sym.id, sym))
			Update(symlist, sym, NULL, FALSE);
		//ASSERT(!strstr(sym.GetStr(ITEM_DESC),"Corredora"));

		}

	return TRUE;
}

// ===============================================================================================


vars VWExtractString(const char *memory, const char *id)
{
	vars cell = ExtractString(strstr(memory, "<table"), id, "", "</tr");
	return ExtractString(cell, "<t", ">", "</t").replace(",",".");
}

double VWExtractStars(const char *memory, const char *id)
{
	vars cell = VWExtractString(memory, id);

	double num = CDATA::GetNum(stripHTML(cell));
	if (num>0) return num+1; // 0-4 scale


	int n = 0, r = 0;
	vars str = ExtractString(cell, "color:", ">", "<");
	for (int i=0; str[i]!=0; ++i)
		if (str[i]=='O')
			++n;
		else
			++r;

	return (n + (r>0 ? 0.5 : 0))*5.0/7.0; // 0-7 scale
}


int VWCANYONING_DownloadPage(DownloadFile &f, const char *url, CSym &sym) 
{
	// download list of recent additions
	static double ticks;
	Throttle(ticks, 1000);
	if (f.Download(url))
		{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
		}

	if (!strstr(f.memory, "AVVICINAMENTO") && !strstr(f.memory, "Avvicinamento")  )
		return FALSE;

	GetSummary(sym, stripHTML(VWExtractString(f.memory, ">DIFFICOLTA")));

	vars season = stripHTML(VWExtractString(f.memory, ">PERIODO"));
	sym.SetStr(ITEM_SEASON, season.replace("(","; BEST ").replace(")",""));

	
	vars raps = stripHTML(VWExtractString(f.memory, ">CALATE"));
	sym.SetNum(ITEM_RAPS, CDATA::GetNum(raps));
	sym.SetStr(ITEM_LONGEST, GetMetric(GetToken(raps, 1, '(')));

	sym.SetStr(ITEM_LENGTH, GetMetric(stripHTML(VWExtractString(f.memory, ">SVILUPPO"))));
	sym.SetStr(ITEM_DEPTH, GetMetric(stripHTML(VWExtractString(f.memory, ">DISLIVELLO"))));
	
	vars shuttle = stripHTML(VWExtractString(f.memory, ">NAVETTA"));
	sym.SetStr(ITEM_SHUTTLE, shuttle.replace(" ","").replace("Circa","").Trim("SsIi,.()"));
	
	sym.SetStr(ITEM_ROCK, GetMetric(stripHTML(VWExtractString(f.memory, ">GEOLOGIA"))));
	double stars = VWExtractStars(f.memory, ">BELLEZZA");
	double fun = VWExtractStars(f.memory, ">DIVERTIMENTO");
	if (fun>stars)
		stars = fun;
	if (stars>0)
		{
		if (stars>5) stars = 5;
		sym.SetStr(ITEM_STARS, starstr(stars,1));
		}

	vars region;
	vars loc = stripHTML(VWExtractString(f.memory, ">LOCALITA"));
	vara aregion = vara(sym.id, "/");
	if (aregion.length()>4)
		sym.SetStr(ITEM_REGION, region = aregion[4]);
	if (!loc.IsEmpty())
		sym.SetStr(ITEM_LAT, "@"+loc+"."+region);
	
		
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
	const char *ids[] = { ">AVVICINAMENTO", ">TEMPI", ">RIENTRO" };
	//ASSERT( !strstr(url,"/kanion.php?id=41"));
	for (int t=0; t<sizeof(ids)/sizeof(*ids); ++t)
		{
		CString time = stripHTML(VWExtractString(f.memory, ids[t]));
		while (!ExtractStringDel(time, "(", "", ")").IsEmpty());
		times.push(vars(time).replace("ore", "h").replace("giorni", "j").replace(";","."));	
		}
	GetTotalTime(sym, times, url);

	return TRUE;
}

int VWCANYONING_DownloadBeta(const char *ubase, CSymList &symlist) 
{
	//vara recentlist, urllist;
	//const char *canyonlink = "href=\"/canyoning/canyon/";
	//const char *canyondesclink = "/canyoning/canyon-description/";
	//varas months("janvier=Jan,février=Feb,mars=Mar,avril=Apr,mai=May,juin=Jun,juillet=Jul,août=Aug,septembre=Sep,octobre=Oct,novembre=Nov,décembre=Dec");

	DownloadFile f;
	CString url = "http://www.verticalwatercanyoning.com";

	// download list of recent additions
	if (f.Download(url))
		{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
		}

	vara done;
	vara list( f.memory, "href=");
	for (int i=1; i<list.length(); ++i)
		{
		const char *memory = list[i];
		vars link = ExtractString(memory, "\"", "", "\""); 
		vars name = UTF8(stripHTML(ExtractString(memory, "", ">", "</a>")));
		const char *plink = strstr(link, "profili/");
		if (link.IsEmpty() || !plink || strstr(link, "ztemplate"))
			continue;

		if (done.indexOf(link)>=0)
			continue;
		done.push(link);

		vara plinka(plink, "/");
		if (plinka.length()<=2 || plinka[2].IsEmpty())
			{
			// region
			if (f.Download(link))
				{
				Log(LOGERR, "ERROR: can't download url %.128s", link);
				continue;
				}

			// add possible links
			vara list2( f.memory, "href=");
			for (int j=1; j<list2.GetSize(); ++j)
				list.push(list2[j]);
			continue;
			}


		// profili/region/somewhere
		CSym sym(urlstr(link), name);
		printf("%d/%d   \r", i, list.length());
		if (!UpdateCheck(symlist, sym) && MODE>-2)
			continue;
		if (VWCANYONING_DownloadPage(f, sym.id, sym))
			Update(symlist, sym, NULL, FALSE);
		//ASSERT(!strstr(sym.GetStr(ITEM_DESC),"Corredora"));

		}

	return TRUE;
}

// ===============================================================================================
int HIKEAZ_ExtractKML(const char *ubase, const char *url, inetdata *out, int fx)
{
	//if (!fx)
	//	return FALSE;


	DownloadFile f;
	vars id = GetKMLIDX(f, url, "'Download GPX Route'", "data-gps='", "'");	
	if (id.IsEmpty())
		return FALSE; // not available
	
	CString credit = " (Data by " + CString(ubase) + ")";

	//alternate go/gpx/download/578
	CString url2 = "http://hikearizona.com/location_ROUTE.php?ID="+id;
	if (DownloadRetry(f, url2))
		{
		Log(LOGERR, "ERROR: can't download url %.128s", url2);
		return FALSE;
		}

	if (!IsSimilar(f.memory, "<?xml"))
		return FALSE; // no pt data

	vara styles, points, lines;
	styles.push("dot=http://maps.google.com/mapfiles/kml/shapes/open-diamond.png");

	vara linelist;
	vara data(f.memory, "<PT>");
	for (int i=1; i<data.length(); ++i)
		{
		CString name = ExtractString(data[i], "<TN>", "", "<");
		double lat = ExtractNum(data[i], "<LA>", "", "<");
		double lng = ExtractNum(data[i], "<LO>", "", "<");
		if (!CheckLL(lat,lng))
			{
			Log(LOGERR, "Invalid PT wpt='%s' for %s", data[i], url);
			continue;
			}
		if (!name.IsEmpty())
			points.push( KMLMarker("dot", lat, lng, name, credit ) );
		linelist.push(CCoord3(lat, lng));
		}
	lines.push( KMLLine("Track", credit, linelist, OTHER, 3) );

	return SaveKML("data", credit, url, styles, points, lines, out);
}


int HIKEAZ_GetCondition(const char *memory, CSym &sym)
{
			vara cond; cond.SetSize(COND_MAX);

			vars date = ExtractString(memory, "itemprop='dateCreated'", "content='", "'");
			double vdate = CDATA::GetDate(date, "MMM D YYYY" );
			if (vdate<=0)
				return FALSE;
			cond[COND_DATE] = CDate(vdate);

			double stars = ExtractNum(memory, "itemprop='ratingValue'", "content='", "'");
			if (stars>0 && stars<=5)
				cond[COND_STARS] = cond_stars[(int)stars];

			vars trip = ExtractString(memory, "<input", "//hikearizona.com/trip=", ">");
			trip = GetToken(trip.Trim("'\" "), 0, "'\" ");
			if (!trip.IsEmpty())
				cond[COND_LINK] = "http://hikearizona.com/trip="+trip;
			else
				Log(LOGERR, "Invalid trip id for %s %s", sym.GetStr(ITEM_DESC), sym.id);

			sym.SetStr(ITEM_CONDDATE, cond.join(";"));
			return TRUE;
}


int HIKEAZ_DownloadBeta(const char *ubase, CSymList &symlist) 
{
	DownloadFile f;
	vars base = "http://hikearizona.com/interest.php?SHOW=YES";
	vars url = base;
	if (f.Download(url))
		{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
		}

	vars optsel = ExtractString(f.memory, "name='STx'", "", "</select>");
	vara opt(optsel, "<option");


	for (int o=1; o<opt.length(); ++o)
	{
		const char *option = opt[o];
		double optval = ExtractNum(option, "value=", "'", "'");
		if (optval<0) {
			Log(LOGERR, "ERROR: Invalid STx");
			continue;
			}
		const char *aztypes[] = { "&TYP=12", "&TYP=13", NULL };
	for (int aztype=0; aztypes[aztype]!=NULL; ++aztype)
		{
		vars url = base+aztypes[aztype]+"&STx="+CData(optval);
		vars state = ExtractString(option, "value=", ">", "<").trim();
		if (state.IsEmpty())
			continue;

		int start = 0;
		while (true)
			{
			if (f.Download(url+MkString("&start=%d",start)))
				{
				Log(LOGERR, "ERROR: can't download url %.128s", url);
				continue;
				}
			int maxstart = 0;
			vara starts(f.memory, "start=");
			for (int i=1; i<starts.length(); ++i)
				{
					double num = CDATA::GetNum(starts[i]);
					if (num>maxstart) maxstart = (int)num;
				}
	/*
			vars content = ExtractString(f.memory, "<table", "<table", "</table");
			content = content.split(" Adventure Tales ").first();
			content.Replace("href=\"../WM/saratoga.htm\"", "");

			const char *types[] = { 
					"Soaking", 
					"Spelunking", 
					"Slot Canyon", "Canyoneering", "Peekaboo", 
					"Hiking", "Backpacking", "Exploring", "Family", "Picknick", 
					//"Roadside Attraction",  "Ruins", "Rock Art", "Petroglyphs", "Climb", "Scrambl",
					//"Hunting",  "Rafting", "Dinosaur", "Salt lake", 
					//"Swimming", "Mining", 
					NULL };
			int typen[] = { 5, 2, 1, 1, 1, 0, 0, 0, 0, 0 };
	*/
			vara list(f.memory, aztype>0 ? "title='Caving'" : "title='Canyoneering'");
			for (int i=1; i<list.length(); ++i)
					{
						vars href = ExtractString(list[i], "href=", "", "</a");
						vars link = "http:"+ExtractString(href, "http", ":", "\"");
						vars name = stripHTML(ExtractString(href, "title=", "'", "'"));
						vars region = stripHTML(ExtractString(list[i], "href=", "</div>", "</div>"));
						CSym sym(urlstr(link), name);
						if (!IsSimilar(state, "WW"))
							sym.SetStr(ITEM_REGION, state + ";" + region.replace(">", ";"));
						if (!UpdateCheck(symlist, sym) && MODE>-2)
							continue;

						// get details
						if (f.Download(link))
							{
							Log(LOGERR, "ERROR: can't download url %.128s", link);
							continue;
							}

						// summary
						vara aca;
						aca.push(stripHTML(ExtractString(f.memory, "class='STP'>Grade", "</td>", "</td>")));
						aca.push(stripHTML(ExtractString(f.memory, "class='STP'>Water", "</td>", "</td>")));
						aca.push(stripHTML(ExtractString(f.memory, "class='STP'>Time", "</td>", "</td>")));
						aca.push(stripHTML(ExtractString(f.memory, "class='STP'>Risk", "</td>", "</td>")));
						
						const char **check[] = { rtech, rwater, rtime, rxtra };
						for (int a=0; a<4; ++a)
							if (!aca[a].IsEmpty() && IsMatch(aca[a],check[a])<0)
								Log(LOGERR, "Invalid ACA%d %s", a, aca[0]);
						sym.SetStr(ITEM_ACA, aca.join(";"));					

						// distance / time
						vars sdist = stripHTML(ExtractString(f.memory, "class='STP'>Distance", "</td>", "</td>"));
						vars stime = stripHTML(ExtractString(f.memory, "class='STP'>Avg Time", "</td>", "</td>"));
						CDoubleArrayList dist, time;
						if (GetValues(stime, utime, time) || GetValues(stime, NULL, time)) {
							sym.SetNum(ITEM_MINTIME, time.Head());
							sym.SetNum(ITEM_MAXTIME, time.Tail());
						}
						if (GetValues(sdist, udist, dist))
							sym.SetNum(ITEM_HIKE, Avg(dist));

						// season
						vars season = stripHTML(ExtractString(f.memory, "<b>Seasons</b>", "", "</td>"));
						vars preferred = stripHTML(ExtractString(f.memory, "<b>Preferred</b>", "", "</td>"));						
						if (!preferred.IsEmpty())
							season += "; BEST in "+preferred;
						if (!season.IsEmpty())
							sym.SetStr(ITEM_SEASON, season);
						
						// lat lng
						vara ll(ExtractString(f.memory, "gmap.setCenter(", "LatLng(", ")"));
						if (ll.length()!=2)
							Log(LOGERR, "Inconsistent LL %s", ll.join());
						else
							{
							double lat = CDATA::GetNum(ll[0]), lng = CDATA::GetNum(ll[1]);
							if (!CheckLL(lat,lng))
								Log(LOGERR, "Invalid LL %s", ll.join());
							else
								{
									sym.SetNum(ITEM_LAT, lat);
									sym.SetNum(ITEM_LNG, lng);
								}
							}

						// ratings
						double stars = ExtractNum(f.memory, "itemprop='ratingValue'", ">", "<");
						double ratings = ExtractNum(f.memory, "itemprop='reviewCount'", ">", "<");
						if (stars>=0)
							if (ratings<=0)
								Log(LOGERR, "Invalid reviewCount=%g", ratings);
							else
								sym.SetStr(ITEM_STARS, starstr(stars, ratings));

						// kml
						vars kmlidx = ExtractString(f.memory, "'Download Route'", "data-gps='", "'");
						sym.SetStr(ITEM_KML, kmlidx.IsEmpty() ? "" : "X");

						// class					
						vars desc = ExtractString(f.memory, "itemprop='text'", ">", "<table");
						CString type = "-1";
						//if (stars>=4.5)
						//	type = "1:Stars";
						if (strstr(desc, "hike") || strstr(desc, "backpack"))
							type = "0:Hike";
						if (strstr(desc, "Slot") || strstr(desc, "slot") || strstr(desc, "fissure"))
							type = "0:Slot";
						if (strstr(desc, " rope") || strstr(desc, " webbing"))
							type = "1:Rope";
						if (strstr(desc, "Rappel") || strstr(desc, "rappel") || strstr(desc, "abseil") || strstr(desc, " rap ") || strstr(desc, " raps "))
							type = "1:Rappel";
						if (!aca[0].IsEmpty())
							type = "1:Summary";
						if (aztype>0)
							type = "2:Cave";
						vars title = sym.GetStr(ITEM_DESC);
						if (strstr(title.lower()," to ")) // A to B
							type = "-1:ToHike";
						sym.SetStr(ITEM_CLASS, type);

						// Conditions
						vars id = ExtractString(sym.id, "ZTN=", "", 0);
						if (!id.IsEmpty())
							{
							vars curl = MkString("http://hikearizona.com/TLG_BASE.php?ZTN=%s&UID=0&m=0&_=1472613646479", id);
							if (f.Download(curl))
								Log(LOGERR, "Could not download %s", curl);
							else
								{
								vara list(f.memory, "itemtype='http://schema.org/Review'");
								if (list.length()>1)
									HIKEAZ_GetCondition(list[1], sym);
								}
							}


						Update(symlist, sym, NULL, FALSE);
						//Log(LOGINFO, "%s", sym.Line());
					}
			printf("Downloading %s %d ...    \r", state, symlist.GetSize());
			start += list.length()-1;
			if (start>maxstart || maxstart==0)
				break;
			}
		}
	}

	return TRUE;
}



int HIKEAZ_DownloadConditions(const char *ubase, CSymList &symlist) 
{

	const char *urls[] = {
		"http://hikearizona.com/TLG_BASE.php?UID=&O=2&ZTN=&G=&C=12&m=0&_=1472613270790",
		"http://hikearizona.com/TLG_BASE.php?UID=&O=2&ZTN=&G=&C=13&m=0&_=1472613270795",
		NULL
		};

	DownloadFile f;
	// if asked to process full, process 500
	int maxstart = (MODE<-2 || MODE>1) ? 3000 : 1;
	for (int u=0; urls[u]!=NULL; ++u)
	  for (int start=0; start<maxstart; start+=20)
		{
		vars url = urls[u];
		if (start>0) 
			url += MkString("&start=%d", start);
		if (f.Download(url))
			{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			return FALSE;
			}

		vara list(f.memory, "itemtype='http://schema.org/Review'");
		for (int i=1; i<list.length(); ++i)
			{
			const char *memory = list[i];
			vars link = ExtractString(memory, "itemprop='url'", "content='", "'");
			vars name = ExtractString(memory, "itemprop='itemReviewed'", "content='", "'");
			if (link.IsEmpty())
				continue;

			CSym sym(urlstr(link), name);
			if (!HIKEAZ_GetCondition(memory, sym))
				continue;
			UpdateCond(symlist, sym, NULL, FALSE);
			}
		//ASSERT(!strstr(f.memory, "=15546"));
		}

	return TRUE;
}

// ===============================================================================================



int SUMMIT_DownloadPage(DownloadFile &f, const char *url, CSym &sym) 
{
	// download list of recent additions
	static double ticks;
	Throttle(ticks, 1000);
	if (f.Download(url))
		{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
		}

	/*
	vars memory = f.memory;
	memory.Replace("<strong>","<b>");
	memory.Replace("</strong>","</b>");
	memory.Replace("<STRONG>","<b>");
	memory.Replace("</STRONG>","</b>");
	memory.Replace("<B>","<b>");
	memory.Replace("</B>","</b>");
	*/

	float lat, lng;
	vars latlng = stripHTML(ExtractString(f.memory, ">Lat/Lon:", "", "</p"));
	GetCoords(latlng.replace("/", ","), lat, lng);
	if (CheckLL(lat,lng))
		{
		sym.SetNum(ITEM_LAT, lat);
		sym.SetNum(ITEM_LNG, lng);
		}

	vars region = invertregion(stripHTML(ExtractString(f.memory, ">Location:", "", "</p")), "");
	sym.SetStr(ITEM_REGION, region);


	vars type = "-1:Unknown";
	vars rtype = stripHTML(ExtractString(f.memory, ">Activities:", "", "</p"));
	if (strstr(rtype, "Hiking"))
		type = "0:Hiking";
	if (strstr(rtype, "Scrambling"))
		type = "0:Scrambling";
	if (strstr(rtype, "Climbing"))
		type = "0:Climbing";
	if (strstr(rtype, "Canyoneering"))
		type = "1:Canyoneering";
	sym.SetStr(ITEM_CLASS, type);

	//GetSummary(sym, stripHTML(ExtractString(f.memory, "Difficoltà:", "", "<br")));
	sym.SetStr(ITEM_SEASON, stripHTML(ExtractString(f.memory, ">Season:", "", "</p")));

	return TRUE;
}


int SUMMIT_DownloadBeta(const char *ubase, CSymList &symlist) 
{
	//vara recentlist, urllist;
	//const char *canyonlink = "href=\"/canyoning/canyon/";
	//const char *canyondesclink = "/canyoning/canyon-description/";
	//varas months("janvier=Jan,février=Feb,mars=Mar,avril=Apr,mai=May,juin=Jun,juillet=Jul,août=Aug,septembre=Sep,octobre=Oct,novembre=Nov,décembre=Dec");

	DownloadFile f;

	for (int page=1; TRUE; ++page)
	{
	// download list of recent additions
	CString url = "http://www.summitpost.org/object_list.php?search_in=name_only&order_type=DESC&object_type=9&orderby=object_name&page="+MkString("%d", page);
	if (f.Download(url))
		{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
		}

	/*
	memory.Replace("<strong>","<b>");
	memory.Replace("</strong>","</b>");
	memory.Replace("<STRONG>","<b>");
	memory.Replace("</STRONG>","</b>");
	memory.Replace("<B>","<b>");
	memory.Replace("</B>","</b>");
	*/

	  vara list(f.memory, "srch_results_lft");
	  for (int i=1; i<list.length(); ++i)
		{
		vars line = ExtractString(list[i], "", "", "</td>");

		vars link = ExtractString(line, "href=", "'", "'" );
		vars name = stripHTML(ExtractString(line, "href=", "alt=\"", "\""	));
		if (link.IsEmpty() || name.IsEmpty())
			continue;

		CSym sym( burl(ubase, link), name);

		printf("%d/%d %d/%d   \r", page, -1, i, list.length());
		if (!UpdateCheck(symlist, sym) && MODE>-2)
			continue;
		if (SUMMIT_DownloadPage(f, sym.id, sym))
			Update(symlist, sym, NULL, FALSE);
		}
	
	  if (list.length()<=1)
		break;
	}

	return TRUE;
}


// ===============================================================================================


int CLIMBUTAH_DownloadDetails(DownloadFile &f, CSym &sym, CSymList &symlist)
{
	const char *url = sym.id;
	if (f.Download(url))
		{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
		}
	
	vara akalist;
	vara aka(vars(f.memory).replace("aka:","AKA:").replace("Aka:","AKA:"), ">AKA:");
	for (int i=1; i<aka.length(); ++i)
		akalist.push(vars(stripHTML(GetToken(aka[i].replace("\n", " ").replace("<br>","\n").replace("</p>","\n"), 0, '\n'))).replace("&", ";"));
	sym.SetStr(ITEM_AKA, akalist.join(";"));
	
	CString desc;
	float lat, lng;
	CString fmemory = CoordsMemory(f.memory);
	if (ExtractCoords(fmemory, lat, lng, desc)>0)
		{
		sym.SetNum(ITEM_LAT, lat);
		sym.SetNum(ITEM_LNG, lng);
		}

	Update(symlist, sym, NULL, FALSE);
	return FALSE;
}


int CLIMBUTAH_DownloadBeta(const char *ubase, CSymList &symlist) 
{
	DownloadFile f;
	vars base = "http://www.climb-utah.com/";
	vars url = base;
	if (f.Download(url))
		{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
		}

	vars optsel = ExtractString(f.memory, "", "<select", "</select>");
	vara opt(optsel, "<option");
	for (int o=1; o<opt.length(); ++o)
	{
		vars url = base+ExtractString(opt[o], "value=", "\"", "\"");
		vars region = ExtractString(opt[o], "value=", ">", "<").trim();
		if (region.IsEmpty())
			continue;
		if (f.Download(url))
			{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			continue;
			}

		vars content = ExtractString(f.memory, "<table", "<table", "</table");
		content = content.split(" Adventure Tales ").first();
		content.Replace("href=\"../WM/saratoga.htm\"", "");

		const char *types[] = { 
				"Soaking", 
				"Spelunking", 
				"Slot Canyon", "Canyoneering", "Peekaboo", 
				"Hiking", "Backpacking", "Exploring", "Family", "Picknick", 
				//"Roadside Attraction",  "Ruins", "Rock Art", "Petroglyphs", "Climb", "Scrambl",
				//"Hunting",  "Rafting", "Dinosaur", "Salt lake", 
				//"Swimming", "Mining", 
				NULL };
		int typen[] = { 5, 2, 1, 1, 1, 0, 0, 0, 0, 0 };

		vara list(content, "href=");
		for (int i=1; i<list.length(); ++i)
				{
					vara ids(url, "/");
					ids[ids.length()-1] = ExtractString(list[i], "", "\"", "\"");
					vars link = ids.join("/");
					vars name = stripHTML(ExtractString(list[i], "", ">", "</a>"));
					vars type = stripHTML(ExtractString(list[i], "", "</a>", "<p>"));
					//ASSERT( strstr(id, "shay.htm")==NULL );
					CSym sym(urlstr(link), name);
					sym.SetStr(ITEM_REGION, region);
					int t = GetClass(type, types, typen);
					SetClass(sym, t, type);
					printf("Downloading %d/%d %d/%d...\r", i, list.length(), o, opt.length());
					if (!Update(symlist, sym, NULL, FALSE) && MODE>=-1)
						continue;
					CLIMBUTAH_DownloadDetails(f, sym, symlist);
				}
	}

	return TRUE;
}

// ===============================================================================================

#define CCHRONICLESTICKS 1000
static double cchroniclesticks;


int CCHRONICLES_ExtractKML(const char *ubase, const char *url, inetdata *out, int fx)
{
	DownloadFile f;
	vars id = GetToken(GetKMLIDX(f, url, "google.com", "msid=","\""), 0, "&");
	if (id.IsEmpty())
		return FALSE; // not available

	CString credit = " (Data by " + CString(ubase) + ")";
	Throttle(cchroniclesticks, CCHRONICLESTICKS);

	KMZ_ExtractKML(credit, "http://maps.google.com/maps/ms?ie=UTF8&hl=en&msa=0&output=kml&msid="+id, out);
	return TRUE;
}


int CCHRONICLES_DownloadBeta(const char *ubase, CSymList &symlist) 
{
	DownloadFile f;

	vars url = "http://canyonchronicles.com";
	if (DownloadRetry(f,url))
		{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
		}

	vara donelist;
	vara regions(f.memory, "<li class=");
	for (int r=1; r<regions.length(); ++r)
		{
			vars li = ExtractString(regions[r], "", "", "</li");
			vars url = urlstr(ExtractString(li, "href=", "\"",  "\""));
			vars region = stripHTML(ExtractString(li, "href=", ">", "</a"));
			if (url.IsEmpty() || donelist.indexOf(url)>=0)
				continue;
			donelist.push(url);

			Throttle(cchroniclesticks, CCHRONICLESTICKS);
			if (DownloadRetry(f,url))
				{
				Log(LOGERR, "ERROR: can't download url %.128s", url);
				continue;
				}

			vara list(f.memory, "entry-title");
			for (int i=1; i<list.length(); ++i) {
				vars url = "http"+ExtractString(list[i], "http", "", "\"");
				vars name = ExtractString(list[i], "http", ">", "<");
				if (name.IsEmpty() || !strstr(url, "canyonchronicles.com") || strstr(url,".php") || strstr(url,".jpg"))
					continue;

				CSym sym(urlstr(url), name);
				sym.SetStr(ITEM_REGION, region);

				printf("Downloading %d/%d %d/%d     \r", i, list.length(), r, regions.length());
				if (!UpdateCheck(symlist, sym) && MODE>-2)
					continue;

				Throttle(cchroniclesticks, CCHRONICLESTICKS);
				if (DownloadRetry(f, url))
					{
					Log(LOGERR, "ERROR: can't download url %.128s", url);
					continue;
					}
				vars kmlidx = GetToken(ExtractString(f.memory, "google.com", "msid=","\""), 0, "&");
				if (kmlidx.IsEmpty() && strstr(f.memory, "google.com")) kmlidx = "MAP?";
				sym.SetStr(ITEM_KML, kmlidx);
				Update(symlist, sym, NULL, FALSE);
			}
		}
	return TRUE;
}

// ===============================================================================================


// ===============================================================================================

int KARSTPORTAL_DownloadPage(DownloadFile &f, const char *id, CSymList &symlist, const char *type)
{
		// get coordinates
		CString nurl = MkString("http://WWW.karstportal.org/taxonomy/term/%s", id);
		if (!UpdateCheck(symlist, CSym(nurl)) && MODE>-2)
			return TRUE;
		if (f.Download(nurl))
			{
			//Log(LOGERR, "Can't download from %s", nurl);
			return FALSE;
			}

		CString coords;
		double lat = ExtractNum(f.memory, "\"lat\":", "\"", "\"");
		double lng = ExtractNum(f.memory, "\"lng\":", "\"", "\"");		
		if (lat==InvalidNUM || lng==InvalidNUM)
			{
			//Log(LOGERR, "Invalid coord for url %s", nurl); 
			return FALSE;
			}

		vars title = ExtractString(f.memory, "id=\"page-title\"", ">", "<");
		title.Replace("\"", "");
		vars name = stripHTML(GetToken(title, 0, "()")).Trim();
		vars region = invertregion(stripHTML(ExtractString(title, "", "(", ")"))).Trim();

		CSym sym(urlstr(nurl), name);
		sym.SetStr(ITEM_LAT, MkString("@%g;%g", lat, lng));
		sym.SetStr(ITEM_LNG, "");
		sym.SetStr(ITEM_CLASS, type);
		sym.SetStr(ITEM_REGION, region);
		Update(symlist, sym, NULL, FALSE);
		return TRUE;
}

int KARSTPORTAL_DownloadBeta(const char *ubase, CSymList &symlist) 
{
	vara regions;
	DownloadFile f;

	// download base
	CString url = "http://www.karstportal.org/browse?taxonomy=cave_name";
	if (f.Download(url))
		{
		Log(LOGERR, "Can't download base url %s", url);
		return FALSE;
		}
	vara list(f.memory, "/taxonomy/term/");
	double nmin = 1e16, nmax = -1;
	for (int i=1; i<list.length(); ++i)
		{
		vara vals;
		double n = CDATA::GetNum(list[i]);
		if (n>0)
			{
			nmin = min(n, nmin);
			nmax = max(n, nmax);
			printf("%d %d/%d   \r", symlist.GetSize(), i, list.length());
			KARSTPORTAL_DownloadPage(f, MkString("%d",(int)n), symlist, "2:Cave");
			}
		}
/*
	if (MODE==0)
		{
		for (int i=(int)nmin; i<(int)nmax; ++i) {
			printf("%d %d/%d   \r", symlist.GetSize(), i, (int)(nmax-nmin));
			KARSTPORTAL_DownloadPage(f, MkString("%d",i), symlist, "0:Cave Hidden");
			}
		}
*/
	return TRUE;
}

// ===============================================================================================




		// Convert CH y/x to WGS lat
		double CHtoWGSlat(double y, double x)
		{
			// Converts military to civil and  to unit = 1000km
			// Auxiliary values (% Bern)
			double y_aux = (y - 600000.0) / 1000000.0;
			double x_aux = (x - 200000.0) / 1000000.0;

			// Process lat
			double lat = 16.9023892
				+ 3.238272 * x_aux
				- 0.270978 * pow(y_aux, 2)
				- 0.002528 * pow(x_aux, 2)
				- 0.0447 * pow(y_aux, 2) * x_aux
				- 0.0140 * pow(x_aux, 3);

			// Unit 10000" to 1 " and converts seconds to degrees (dec)
			lat = lat * 100 / 36;

			return lat;
		}

		// Convert CH y/x to WGS long
		double CHtoWGSlng(double y, double x)
		{
			// Converts military to civil and  to unit = 1000km
			// Auxiliary values (% Bern)
			double y_aux = (y - 600000.0) / 1000000.0;
			double x_aux = (x - 200000.0) / 1000000.0;

			// Process long
			double lng = 2.6779094
				+ 4.728982 * y_aux
				+ 0.791484 * y_aux * x_aux
				+ 0.1306 * y_aux * pow(x_aux, 2)
				- 0.0436 * pow(y_aux, 3);

			// Unit 10000" to 1 " and converts seconds to degrees (dec)
			lng = lng * 100.0 / 36.0;

			return lng;
		}


int SWISSCANYON_GetSummary(CSym &sym, const char *summary)
{
		while (*summary!=0 && !isanum(*summary))
			++summary;

		//ASSERT(!strstr(sym.data, "Gribbiasca")); 

		if (tolower(summary[0])=='v')
			return GetSummary(sym, summary);

		if (!isdigit(summary[0]))
			return FALSE;

		vara slist2(",,,"), slist(summary, ";");
		int len = strlen(summary);
		for (int s=0; s<len; ++s)
		  {
		  if (!isdigit(summary[s]))
			{
			if (summary[s+1]=='I' || summary[s+1]=='V') 
				{
				slist2[2] = GetToken(summary+s, 0, " ;,/.abc");
				s = len;
				}
			}
		  else
		  switch (summary[s+1])
			{
			case 'a':
				slist2[1] = vars("a")+summary[s++];
				break;
			case 'b':
				slist2[0] = vars("v")+summary[s++];
				break;
			case 'c':	
				++s;
				break;
			}
		  }
		
		return GetSummary(sym, slist2.join(" "));
}


int SWISSCANYON_DownloadPage(DownloadFile &f, CSym &sym)
{
	if (f.Download(sym.id))
		{
		Log(LOGERR, "Can't download page url %s", sym.id);
		return FALSE;
		}

	vars memory(f.memory);
	memory.Replace("<TD", "<td");
	memory.Replace("</TD", "</td");
	memory.Replace("<TR", "<tr");
	memory.Replace("<TABLE", "<table");
	memory.Replace("<BR", "<br");

	// coordinates
	vars startp = stripHTML(ExtractString(strstr(memory,"Partenza<"), "<td", ">", "</td>"));
	startp.Replace(".", "");
	vara div(startp, "/");
	if (div.length()>=2)
		{
		double east = CDATA::GetNum(div[0].trim().Right(6));
		double north = CDATA::GetNum(div[1].trim().Left(6));
		double lat = CHtoWGSlat(east, north);
		double lng = CHtoWGSlng(east, north);
		if (CheckLL(lat,lng))
			{
			sym.SetNum(ITEM_LAT, lat);
			sym.SetNum(ITEM_LNG, lng);
			}
		}

	// best time
	vars season = UTF8(stripHTML(ExtractString(strstr(memory,"riode id"), "<td", ">", "</td>")));
	season.Replace("&#150;","-");
	if (!season.IsEmpty())
		sym.SetStr(ITEM_SEASON, season);

	// shuttle
	vars shuttle = UTF8(stripHTML(ExtractString(strstr(memory,">Navett"), "<td", ">", "</td>")));
	if (!shuttle.IsEmpty())
		{
		int f = shuttle.Find("min");
		if (f>0) shuttle = shuttle.Mid(0, f+3);
		shuttle.Replace("Pas", "No");
		shuttle.Replace("(No indispensable)", "Optional");
		if (IsSimilar(shuttle,"min"))
			shuttle="";
		sym.SetStr(ITEM_SHUTTLE, shuttle);
		}

	sym.SetStr(ITEM_DEPTH, GetMetric(stripHTML(ExtractString(strstr(memory,"Dislivello<"), "<td", ">", "</td>"))));
	sym.SetStr(ITEM_LENGTH, GetMetric(stripHTML(ExtractString(strstr(memory,"Lunghezza<"), "<td", ">", "</td>"))));

	// summary
	vars summary = UTF8(stripHTML(ExtractString(strstr(memory,">Difficolt"), "<td", ">", "</td>")));
	SWISSCANYON_GetSummary(sym, summary);
	
	return TRUE;
}



int SWISSCANYON_DownloadTable(const char *url, const char *base, CSymList &symlist)
{
	DownloadFile f;
	if (f.Download(url))
		{
		Log(LOGERR, "Can't download base url %s", url);
		return FALSE;
		}
	vars memory(f.memory);
	memory.Replace("<TD", "<td");
	memory.Replace("</TD", "</td");
	memory.Replace("<TR", "<tr");
	memory.Replace("<TABLE", "<table");
	memory.Replace("<BR", "<br");
	vara tables(memory, "teschio");
	for (int t=1; t<tables.length(); ++t)
	{
	vars table = ExtractString(tables[t], "", "", "</table");
	vara list(table, "<tr");
	for (int i=1; i<list.length(); ++i)
		{
		vars &line = list[i];
		vara td(line, "<td");
		vars &td1 = td[1];
		vars link = ExtractString(td[1], "href", "\"", "\"");
		vars name = UTF8(stripHTML(ExtractString(td[1], ">", "", "<td").split("<br").first())).trim();
		// fake url
		BOOL fake = link.IsEmpty();
		if (fake)
			link = url+vars("?")+url_encode(name);
		else
			link = burl(base, link);

		CSym sym(urlstr(link), name);

		vars region = UTF8(stripHTML(ExtractString(td[2], ">", "", "</td")));
		sym.SetStr(ITEM_REGION, "Switzerland;"+region);
		sym.SetStr(ITEM_LAT, "@"+region+";Switzerland");

		double stars = ExtractNum(td[4], "stella-", "", ".");
		double smiles = ExtractNum(td[5], "smile-mini-", "", ".");
		if (smiles>stars) 
			stars = smiles;
		if (stars>0)
			sym.SetStr(ITEM_STARS, starstr(stars+1, 1));

		vars summary = stripHTML(ExtractString(td[6], ">", "", "</td"));
		SWISSCANYON_GetSummary(sym, summary);

		if (Update(symlist, sym, NULL, FALSE) || MODE<=-2)
			if (!fake)
				{
				SWISSCANYON_DownloadPage(f, sym);
				Update(symlist, sym, NULL, FALSE);
				}
		}
	}
	return TRUE;
}

int SWISSCANYON_DownloadBeta(const char *ubase, CSymList &symlist) 
{
	vara regions;
	DownloadFile f;
	const char *urls[] = { "http://www.swisscanyon.ch/canyons/lista-ti.htm", "http://www.swisscanyon.ch/canyons/lista-ch-west.htm", NULL };
	for (int u=0; urls[u]!=NULL; ++u)
		SWISSCANYON_DownloadTable(urls[u], "swisscanyon.ch/canyons", symlist);
	return TRUE;
}

// ===============================================================================================

int SWESTCANYON_DownloadBeta(const char *ubase, CSymList &symlist) 
{
	vara regions;
	DownloadFile f;
	const char *urls[] = { "http://www.swestcanyon.ch/www_swisscanyon_ch_fichiers/lista-ch-west.htm",  NULL };
	for (int u=0; urls[u]!=NULL; ++u)
	{
	// download base
	SWISSCANYON_DownloadTable(urls[u], ubase, symlist);
	}
	return TRUE;
}

// ===============================================================================================
int CICARUDECLAN_DownloadPage(DownloadFile &f, const char *url, CSym &sym)
{
		if (f.Download(url))
			{
			Log(LOGERR, "Can't download page %s", url);
			return FALSE;
			}

		vars memory(f.memory);
		memory.Replace("<TD","<td");
		memory.Replace("</TD","</td");

		vars summary = stripHTML(ExtractString(strstr(memory, ">Diffic"), "<td", ">", "</td"));
		if (summary.IsEmpty())
			summary = stripHTML(ExtractString(strstr(memory, ">Diffic"), "<td", ">", "</td"));
		GetSummary(sym, summary);

		vars season = UTF8(stripHTML(ExtractString(strstr(memory, ">Period"), "<td", ">", "</td")));
		if (season.IsEmpty())
			season = UTF8(stripHTML(ExtractString(strstr(memory, ">Period"), "<td", ">", "</td")));
		sym.SetStr(ITEM_SEASON, season);

		vars rappels = stripHTML(ExtractString(strstr(memory, ">Calate"), "<td", ">", "</td"));
		if (rappels.IsEmpty())
			rappels = stripHTML(ExtractString(strstr(memory, ">Rope"), "<td", ">", "</td"));
		GetRappels(sym, rappels.replace("nessuna", "0"));

		vars time = stripHTML(ExtractString(strstr(memory, ">Tempi"), "<td", ">", "</td"));
		if (time.IsEmpty())
			time = stripHTML(ExtractString(strstr(memory, ">Duration"), "<td", ">", "</td"));
#if 1
		vara otimes(time, "+"), times;
		vara order("avvicinamento,discesa,rientro");
		for (int i=0; i<order.length(); ++i)
			{
			vars tval;
			for (int t=0; t<otimes.length(); ++t)
				if (strstr(otimes[t],order[i]))
					tval = otimes[t].replace(order[i],"");
			times.push(tval);
			}
#else
		time.Replace("avvicinamento", "");
		time.Replace("accesso", "");
		time.Replace("discesa", "");
		time.Replace("rientro", "");
		vara times(time, "+");
#endif
		GetTotalTime(sym, times, url);

		vars shuttle = stripHTML(ExtractString(strstr(memory, ">Navetta"), "<td", ">", "</td"));
		if (shuttle.IsEmpty())
			shuttle = stripHTML(ExtractString(strstr(memory, ">Shuttle"), "<td", ">", "</td"));
		shuttle.Replace(";",".");
		shuttle.Replace("circa","");
		shuttle = GetToken(shuttle, 0, 'k').Trim() + " km";
		if (shuttle.Left(1)=="0")
			shuttle = "No";
		sym.SetStr(ITEM_SHUTTLE, shuttle);

		sym.SetStr(ITEM_DEPTH, GetMetric(stripHTML(ExtractString(strstr(memory,">Dislivello"), "<td", ">", "</td>").replace("circa","").replace("chilo","k"))));
		sym.SetStr(ITEM_LENGTH, GetMetric(stripHTML(ExtractString(strstr(memory,">Lunghezza"), "<td", ">", "</td>").replace("circa","").replace("chilo","k"))));


		//ASSERT(!strstr(sym.data, "Consusa"));
		return TRUE;
}

int CICARUDECLAN_DownloadBeta(const char *ubase, CSymList &symlist) 
{
	vara regions;
	DownloadFile f;

	// download base
	vars base = "cicarudeclan.com/ita";
	CString url = burl(base, "schede.htm"); //"http://www.cicarudeclan.com/eng/data.htm";
	if (f.Download(url))
		{
		Log(LOGERR, "Can't download base url %s", url);
		return FALSE;
		}
	vara list(f.memory, "<tr");
	for (int i=1; i<list.length(); ++i)
		{
		vars region = UTF8(ExtractString(list[i], "swapImage(", "'", "'"));
		vars rurl = ExtractString(list[i], "href=", "\"", "\"");
		if (rurl.IsEmpty() || IsSimilar(rurl,"javascript"))
			continue;

		url = burl(base, rurl);
		if (f.Download(url))
			{
			Log(LOGERR, "Can't download region url %s", url);
			return FALSE;
			}
		// page
		vara list(f.memory, "<img src");
		for (int i=1; i<list.length(); ++i)
			{
			vars line = list[i].replace("<!--<a href", "<!--<trash");
			vars link = ExtractString(line, "href=", "\"", "\"");
			if (link.IsEmpty())
				continue;

			link = burl(base, link);
			vars name = UTF8(stripHTML(ExtractString(line, "href=", ">", "</a")));
			
			CSym sym(urlstr(link), name);
			sym.SetStr(ITEM_REGION, region);

			double stars = 0;
			vars pallino = ExtractString(line, "=", "\"", "\"");
			if (strstr(pallino, "red"))
				stars = 1.5;
			if (strstr(pallino, "yellow"))
				stars = 3;
			if (strstr(pallino, "green"))
				stars = 4.5;
			ASSERT(stars>0);
			if (stars>0)
				sym.SetStr(ITEM_STARS, starstr(stars,1));

			if (Update(symlist, sym, NULL, FALSE) || MODE<=-2)
				{
				CICARUDECLAN_DownloadPage(f, link, sym);
				Update(symlist, sym, NULL, FALSE);
				}
			}
		}
	return TRUE;
}

// ===============================================================================================
int SCHLUCHT_DownloadBeta(const char *ubase, CSymList &symlist) 
{
	DownloadFile f;

	// download base
	vars url = burl(ubase, "index.php?option=com_canyons&view=category&id=39&format=json");
	if (f.Download(url))
		{
		Log(LOGERR, "Can't download base url %s", url);
		return FALSE;
		}
	vara list(f.memory, "\"Point\"");
	for (int i=1; i<list.length(); ++i)
		{
		vars line = list[i];
		vars name = ExtractString(line, "\"name\"", "\"", "\"");
		vars link = ExtractString(ExtractString(line, "\"descriptionlink\"", "", ","), "\\\"", "", "\\\"");
		if (link.IsEmpty())
			continue;

		CSym sym(urlstr(burl(ubase,link.replace("\\/","/"))), name);
		sym.SetStr(ITEM_REGION, "Switzerland");
		sym.SetStr(ITEM_AKA, name);

		vars coords = ExtractString(line, "\"coordinates\"", "[", "]");
		double lng = CDATA::GetNum(GetToken(coords, 0, ','));
		double lat = CDATA::GetNum(GetToken(coords, 1, ','));
		if (!CheckLL(lat,lng))
			{
			Log(LOGERR, "Invalid coordinates %s for %s", coords, sym.id);
			continue;
			}
		sym.SetNum(ITEM_LAT, lat);
		sym.SetNum(ITEM_LNG, lng);

		if (Update(symlist, sym, NULL, FALSE) || MODE<=-2)
			{
			//SCHLUCHT_DownloadBeta(f, link, sym);
			Update(symlist, sym, NULL, FALSE);
			}
		}
	return TRUE;
}


int SCHLUCHT_DownloadConditions(const char *ubase, CSymList &symlist) 
{
	DownloadFile f;

	vara urls;
	urls.push("https://www.schlucht.ch/index.php?option=com_canyons&task=displayComments&format=json");

	if (MODE>0 || MODE<-2)
		{
		CSymList list;
		SCHLUCHT_DownloadBeta(ubase, list);
		for (int i=0; i<list.GetSize(); ++i)
			{
			vars id = GetToken(list[i].id, 1, "=");
			if (id.IsEmpty())
				continue;
			urls.push("https://www.schlucht.ch/index.php?option=com_canyons&task=displayComments&format=json&cid="+id);
			}
		}

	for (int u=0; u<urls.length(); ++u)
		{
		// download base
		if (f.Download(urls[u]))
			{
			Log(LOGERR, "Can't download base url %s", urls[u]);
			return FALSE;
			}
		vara list(f.memory, "\"canyon\"");
		for (int i=1; i<list.length(); ++i)
			{
			vars line = list[i];
			vars id = ExtractString(line, ":", "\"", "\"");
			vars date = ExtractString(line, "\"comment_date\"", "\"", "\"");
			double vdate = CDATA::GetDate(date, "D.MM.YYYY" );
			if (id.IsEmpty() || vdate<=0)
				continue;

			CSym sym(urlstr("http://schlucht.ch/schluchten-der-schweiz-navbar.html?cid="+id));
			vara cond; cond.SetSize(COND_MAX);
			cond[COND_DATE] = CDate(vdate);
			cond[COND_LINK] = sym.id;
			sym.SetStr(ITEM_CONDDATE, cond.join(";"));
			UpdateCond(symlist, sym, NULL, FALSE);
			}
		}
	return TRUE;
}


vars url2url(const char *url, const char *baseurl)
{
	vara urla(url, "/");
	if (baseurl && !IsSimilar(urla[0], "http"))
		{
		urla = vara(baseurl, "/");
		urla[urla.length()-1] = url;
		}
	return urla.join("/");
}

vars url2file(const char *url, const char *folder)
{
	vara urla(url, "/");
	if (urla.length()>3)
		{
		urla.RemoveAt(0);
		urla.RemoveAt(0);
		urla.RemoveAt(0);
		}
	vars file = MkString("%s\\%s", folder, urla.join("_"));

	const char *subst = ":<>|?*\"";
	for (int i=0; subst[i]!=0; ++i)
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

		vara links(mem,astart2);
		for (int i=1; i<links.length(); ++i)
			{
			vars &link = links[i];
			vars &link1 = links[i-1];
			vars urlimg = GetToken(link, 0, " >").Trim("\"' ");
			if (urlimg.IsEmpty())
				continue;

			urlimg = url2url( urlimg, baseurl);
			vars titleimg = stripHTML(ExtractString(link, ">", "", aend));

			// delete link
			int end = link.indexOf(aend);
			int start = link1.lastIndexOf(astart1);
			if (end<0 || start<0)
				{
				Log(LOGERR, "Invalid link, will get lost '%s', from %s", link, baseurl);
				continue;
				}
			link1 = link1.Mid(0, start);
			link = link.Mid(end+strlen(aend));
			if (!all && !IsBQN(urlimg))
				{
				// keep external links
				link = MkString("[%s %s]", urlimg, titleimg) + link;
				continue;
				}

			if (all || !strstr(urlimg, "xxxxx"))
				if (all || IsGPX(urlimg) || IsImage(urlimg))
					if (urlimgs.indexOf(urlimg)<0)
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
			char *mem = (char *)malloc(len+1);
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
			while(memory.Replace("  ", " "));

			// patch
			memory.Replace("<B>","<b>");
			memory.Replace("</B>","</b>");
			memory.Replace("</TR","</tr");
			memory.Replace("<TR","<tr");
			memory.Replace("<TD","<td");
			memory.Replace("</TD","</td");
			memory.Replace("<FONT","<font");
			memory.Replace("</FONT","</font");
			memory.Replace("<CENTER","<center");
			memory.Replace("</CENTER","</center");
			memory.Replace("<BR","<br");
			memory.Replace("<P","<p");
			memory.Replace("</P","</p");
			memory.Replace("<DIV","<div");
			memory.Replace("</DIV","</div");
			memory.Replace("<b><font size=2>","<font size=2><b>");
			memory.Replace("<b> <font size=2>","<font size=2><b>");
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
		for (int i=0; i<urlimgs.length(); ++i)
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


#define BQNFOLDER "BQN"

void ReplaceIfDigit(CString &str, const char *match, const char *rmatch)
{
	while (TRUE)
	{
	int pos = str.Find(match);
	if (pos<=0) 
		return;
	if (!isdigit(str[pos-1]))
		return;
	str.Replace(match, rmatch);
	}
}

int BARRANQUISMO_DownloadPage(DownloadFile &fout, const char *url, CSym &sym)
{
		//Throttle(barranquismoticks, 1000);
		CString memory;
		if (!Download_Save(url, BQNFOLDER, memory))
			{
			Log(LOGERR, "Can't download page %s", url);	
			return FALSE;
			}	
	
		//ASSERT(!strstr(url, "cuichat"));
		if (IsImage(url))
			{
			sym.SetStr(ITEM_CONDDATE, "!"+vars(GetFileExt(url)));
			return TRUE;
			}


		vars loc3 = UTF8(stripHTML(ExtractString(memory, "País:", "",  "<br")).MakeUpper().Trim());
		vars loc2 = UTF8(stripHTML(ExtractString(memory, "Departamento:", "",  "<br")).MakeUpper().Trim());
		vars loc1 = UTF8(stripHTML(ExtractString(memory, "Acceso desde:", "",  "<br")).MakeUpper().Trim());

		vars loc; 
		double glat = InvalidNUM, glng = InvalidNUM;

		loc1.Replace("DESDE ", "");
		loc1 = GetToken(loc1, 0, ".(,;/").Trim();
		loc2 = GetToken(loc2, 0, ".(,;/").Trim();
		if (sym.GetNum(ITEM_LAT)==InvalidNUM && !loc3.IsEmpty())
			if (!_GeoCache.Get(loc = loc1!=loc2 ? loc1+"."+loc2+"."+loc3 : loc1+"."+loc3, glat, glng))
				if (!_GeoCache.Get(loc = loc1+"."+loc3, glat, glng))
					//if (!_GeoCache.Get(loc = loc2+"."+loc3, glat, glng))
						//if (!_GeoCache.Get(loc = GetToken(sym.GetStr(ITEM_LAT), 1, '@'), glat, glng))
						Log(LOGERR, "Bad Geocode for %s : '%s'", sym.id, loc);
		if (sym.GetNum(ITEM_LAT)==InvalidNUM && !loc.IsEmpty())
			sym.SetStr(ITEM_LAT, "@"+loc);

		double longest = CDATA::GetNum(stripHTML(ExtractString(memory, "mas largo:", "",  "<br")));
		if (longest!=InvalidNUM)
			sym.SetNum(ITEM_LONGEST, longest*m2ft);
		vars shuttle = UTF8(stripHTML(ExtractString(memory, "vehículos:", "",  "<br")));
		//if (shuttle!=InvalidNUM)
		shuttle.Trim();
		if (!shuttle.IsEmpty())
			{
			const char *no[] = { "no necesari", "inneces", "sin combi", "imposible", "no posible", "no hay combi", NULL };
			const char *opt[] = { "posible", "no impresc", "no obligatori", "opcional", "factible", "uno o dos", "no recomend", "no totalmente", "conveniente", "aconsej", NULL };
			const char *yes[] = { "recomend", "obliga", NULL };			
			vars oshuttle = shuttle = stripAccentsL(shuttle);
			shuttle.Replace("sin datos", "");
			shuttle.Replace(" es ", " ");
			shuttle.Replace("vehiculo", "coche");
			shuttle.Replace("solo 1", "1 solo");
			shuttle.Replace("solo un", "1 solo");
			shuttle.Replace("solo se necesita", "1 solo");
			shuttle.Replace("un solo", "1 solo");
			shuttle.Replace("un coche solo", "1 solo coche");
			shuttle.Trim();
			BOOL bno = FALSE, bopt = FALSE, byes = FALSE;
			for (int i=0; !bno && no[i]!=NULL; ++i)
				if (strstr(shuttle, no[i]))
					bno = TRUE, shuttle.Replace(no[i],vars(no[i]).upper());
			if (IsSimilar(shuttle, "No") || IsSimilar(shuttle, "1"))
					bno = TRUE;
			for (int i=0; !bopt && opt[i]!=NULL; ++i)
				if (strstr(shuttle, opt[i]))
					bopt = TRUE, shuttle.Replace(opt[i],vars(opt[i]).upper());
			for (int i=0; !bopt && yes[i]!=NULL; ++i)
				if (strstr(shuttle, yes[i]))
					byes = TRUE, shuttle.Replace(yes[i],vars(yes[i]).upper());
			if (!shuttle.IsEmpty())
				{
				if (bopt && !byes)
					shuttle = "Optional ! " + shuttle;
				else if (bno && !byes)
					shuttle = "No ! " + shuttle;
				else 
					shuttle = "Yes !" + shuttle;
				}
			sym.SetStr(ITEM_SHUTTLE, shuttle); //*km2mi
			}		

		sym.SetStr(ITEM_SEASON, UTF8(stripHTML(ExtractString(memory, "Epoca:", "",  "<br"))));
		
		//ASSERT(!strstr(sym.id, "_brujas"));
		//ASSERT(!strstr(sym.id, "transit"));

		vars gps = stripHTML(ExtractString(memory, "Coord. GPS", ":", "<font"));
		vars gps2 = stripHTML(ExtractString(memory, "Coord. GPS", "final:", "<font"));
		if (gps.IsEmpty())
			gps = gps2;
		gps.Replace("Sin datos", "");
		gps.Trim();
		if (gps.IsEmpty())
			gps = stripHTML(ExtractString(memory, "UTM", " ", "<"));
		if (sym.GetNum(ITEM_LAT)!=InvalidNUM)
			gps = "";

		gps.Replace("Sin datos", "");
		if (!gps.IsEmpty())
			{
			// don't worry about location if already known
			static CSymList bslist;
			if (bslist.GetSize()==0)
				LoadBetaList(bslist);
			
			int found = bslist.Find(sym.id);
			if (found>=0)
				{
				found = rwlist.Find(bslist[found].data);
				if (found>=0 && rwlist[found].GetNum(ITEM_LAT)!=InvalidNUM)
					gps = "";
				}
			}

		if (!gps.IsEmpty())
			{
			char gzone[10] = "30T";
			double gx, gy;						
			if (glat!=InvalidNUM && glng!=InvalidNUM)
				LL2UTM(glat, glng, gx, gy, gzone);

			float lat = InvalidNUM, lng = InvalidNUM;
			gps.Replace(" y ", " ");
			gps.Replace(" - ", " ");
			gps.Replace("/", " ");
			gps.Replace("WGS84", "");
			gps.Replace("WGS 84", "");
			gps.Replace("ED50", "");			
			gps.Replace("ED 50", "");			
			gps.Replace("EU50", "");	
			gps.Replace("MAZANDÚ","");
			gps.Replace("(Datum European 1979)","");
			gps.Replace("(Datum European 1979)","");
			gps.Replace("(ETRS89)","");
			gps.Replace("grados", "o");
			gps.Replace("&deg;", "o");
			gps.Replace("&ordm;", "o");
			gps.Replace("°","o");
			gps.Replace("\xba", "o");
			gps.Replace("\x92", "'");
			gps.Replace("\x94", "\"");
			gps.Replace("minutos", "'");
			gps.Replace("segundos", "\"");
			gps.Replace("norte", "N");
			gps.Replace("oeste", "O");
			gps.Replace("este", "E");
			gps.Replace("sur", "S");

			// utm
			if (strstr(gps,"Lat") || strstr(gps,"o") || gps[0]=='N')
				{
				// LAT/LNG 3
				gps.Replace("Lat.", " ");
				gps.Replace("Lng.", " ");
				gps.Replace("Long.", " ");
				gps.Replace("Lat:", " ");
				gps.Replace("Lng:", " ");
				gps.Replace("Long:", " ");
				gps.Replace("Longitude", " ");
				gps.Replace("LAT:", " ");
				gps.Replace("LWNG:", " ");
				gps.Replace("Latitude", " ");
				gps.Replace("||", " ");
				gps.Replace("O","W");
				gps.Replace("N;","N");
				gps.Replace("''","\"");
				gps.Replace("´", "'");
				gps.Replace("MAZAND\xD8\xA0", " ");
				while (gps.Replace("  ", " "));
				vars ogps;
				gps = gps.split("X").first();
				ExtractCoords(gps.replace(";","."), lat, lng, ogps);
				if (!CheckLL(lat,lng))
					ExtractCoords(gps.replace(";"," "), lat, lng, ogps);
				}
			else
				{
				// UTM
				if (gps.Replace("X-",""))
					{
					gps.Replace(" ", "");
					gps.Replace("Y-"," ");
					}
				gps.Replace("=", " ");
				gps.Replace("X:", " ");
				gps.Replace("Y:", " ");
				gps.Replace("X", " ");
				gps.Replace("Y", " ");
				gps.Replace("x:", " ");
				gps.Replace("y:", " ");
				gps.Replace("x", " ");
				gps.Replace("y", " ");
				gps.Replace("m;", " ");
				gps.Replace("UTM", " ");
				gps.Replace("Datum:", " ");
				gps.Replace("Datum", " ");
				gps.Replace(" T67 ", " ");
				gps.Replace(";", " ");
				gps.Replace("WAPOINT:", "");
				if (gps.Replace("31T", " "))
					gps = "31T "+gps;
				while (gps.Replace("  ", " "));
				vara coord(gps.Trim(), " ");
				if (coord.length()==1) // X-Y
					coord = vara(coord[0],"-");
				int c = 0;
				double z = CDATA::GetNum(coord[c++]);
				if (z ==(int)z || z>180)
					{
					if (z>=1 && z<=60)
						if (isdigit(coord[0][strlen(coord[0])-1]))
							if (c<coord.length() && strlen(coord[c])==1)
								coord[0] += coord[c++];
							else
								coord[0] += UTMLetterDesignator(glat);
					if (z>60)
						coord.InsertAt(0, gzone);
					if (c<coord.length()-1)
						{
						static int dzone[] = { 0, 1, -1, 0 };
						double z = CDATA::GetNum(coord[0]);
						vars l = coord[0].Right(1);
						if (!l.IsEmpty() && z>=1 && z<=60)
						  for (int r=0; r<=3 && !(fabs(glat-lat)<1 && fabs(glng-lng)); ++r)
							{
								UTM2LL(CData(z+dzone[r])+l, coord[c].replace(".", ""), coord[c+1].replace(".", ""), lat, lng);
								if (!CheckLL(lat,lng))
									UTM2LL(coord[0], coord[c], coord[c+1], lat, lng);
							
							}
						}
					}
				}
			const char *special = "Coordenadas lat; lon ():";
			if (gps.Replace(special, ""))
				{
				gps.Replace(";",".");
				gps.Trim();
				lat = (float)CDATA::GetNum(GetToken(gps, 0, ' '));
				lng = (float)CDATA::GetNum(GetToken(gps, 1, ' '));
				}
			if (!CheckLL(lat,lng))
				{
				Log(LOGERR, "Invalid coordintates '%s' for %s", gps, url);
				}
			else
				{
				//ASSERT(lat>35 && lat<43 && lng>-10 && lng<5);
				if (fabs(glat-lat)<1 && fabs(glng-lng)<1)
					{
					sym.SetNum(ITEM_LAT, lat);	
					sym.SetNum(ITEM_LNG, lng);
					}
				else
					{
					vars locgps = "UNKNOWN";
					if (glat!=InvalidNUM && glng!=InvalidNUM)
						locgps = MkString("%s,%s UTM %s %s %s", CData(glat), CData(glng), gzone, CData(gx), CData(gy));
					Log(LOGERR, "Inconsistent coordinates %s '%s' = %s,%s <> %s = '%s'", sym.id, gps, CData(lat),  CData(lng), locgps, loc);
					}
				}
			}


		vara times;
		const char *ids[] = { "Horario de aproximaci", "Horario de descenso:", "Horario de retorno:" };
		const char *ids2[] = { "d'approche:", "de parcours:", "de retour:" };
		//ASSERT(sym.id!="http://barranquismo.net/paginas/barrancos/riu_glorieta.htm");
		//ASSERT(sym.id!="http://barranquismo.net/paginas/barrancos/canyon_de_oilloki.htm");
		for (int i=0; i<3; ++i)
			{
			vars time = stripHTML(ExtractString(memory, ids[i], "</b>",  "<br"));
			if (time.IsEmpty())
				time = stripHTML(ExtractString(memory, ids2[i], "</b>",  "<br"));
			if (time.IsEmpty())
				{
				Log(LOGWARN, "Missing A+D+E times '%s' from %s", ids[i], url);
					continue;
				}

			time.MakeLower();
			time.Replace(" a ", "-");
			time.Replace(" o ", "-");
			time.Replace("y media", ".5h");
			time.Replace("media hora", "0.5h");
			time.Replace("una hora", "1h");
			time.Replace(" y ", "-");
			time.Replace("dos ","2");
			time.Replace("tres ","3");
			time.Replace("cuatro ","4");
			time.Replace("cinco ","5");
			time.Replace("seis ","6");
			time.Replace("siete ","7");
			time.Replace("ocho ","8");
			time.Replace("diez ","10");
			time.Replace("de", "");
			time.Replace(" ", "");	
			time.Replace("\x92", "'");
			time.Replace("\x45", "'");
			time.Replace("\xb4", "'");
			time.Replace("\x60", "'");
			time.Replace("\xba", "x");
			time.Replace("inmediat","-5m-");
			time.Replace("imediat","-5m-");			
			time.Replace("nada","-5m-");
			time.Replace("nihil","-5m-");			
			time.Replace("alpi","-5m-");			
			time.Replace("horas","h");
			time.Replace("hora","h");
			time.Replace("min","m-");
			time.Replace("h.","h");
			time.Replace("m.","m-");
			time.Replace("veh","coche");
			time.Replace("2da","");
			time.Replace("2x","");
			time.Replace("1x","");
			time.Replace("con1","");
			time.Replace("con2","");
			time.Replace("1coche","");
			time.Replace("2coche","");
			time.Replace("4pers","");
			ReplaceIfDigit(time, "'30",".5h");
			ReplaceIfDigit(time, ".30",".5h");
			ReplaceIfDigit(time, "'5",".5h");
			ReplaceIfDigit(time, ";5",".5h");
			ReplaceIfDigit(time, ";30",".5h");
			ReplaceIfDigit(time, ";",".");
			time.Replace("1/2", ".5h");
			time.Replace("/", "-");
			time.Replace(";", "-");
			time.Replace("sindatos", "");
			time = GetToken(time, 0, '(');
			times.push(time.Trim());
			}
		if (times.length()==3)
			GetTotalTime(sym, times, url, 20);

		// length & depth
		int idlen[] = { ITEM_LENGTH, ITEM_DEPTH };
		const char *idslen[] = { "<b>Longitud", "<b>Desnivel" }; 
		for (int i=0; i<2; ++i)
			{
			vars len = GetMetric( stripHTML(ExtractString(memory, idslen[i], "</b>", "<br")) );
			if (!len.IsEmpty())
				sym.SetStr(idlen[i], len);
			}

		// Rock
		vars rock = stripAccentsL(stripHTML(UTF8(ExtractString(memory, "<b>Tipo de roca", "</b>", "<br")))).Trim(" .");
		if (rock!="sin datos")
			sym.SetStr(ITEM_ROCK, rock.replace(" y ", ";"));

		// user
		vars cdate = stripHTML(ExtractString(memory, "CREADOR DE PAGINA-->", "<b>",  "</b"));
		vars cuser = stripHTML(ExtractString(memory, "CREADOR DE PAGINA-->", "<i>",  "</i"));
		if (cuser.IsEmpty())
			cuser = stripHTML(ExtractString(memory, "CREADOR DE PAGINA-->", "por:",  "</tr"));
		sym.SetStr(ITEM_CONDDATE, UTF8(cuser +" ; "+cdate));
		return TRUE;
}

//http://api.bing.com/qsml.aspx?query=b&maxwidth=983&rowheight=20&sectionHeight=160&FORM=IESS02&market=en-US

int IsUpper(const char *word)
{
	while (*word)
		{
		if (*word>='a' && *word<='z')
			return FALSE;
		++word;
		}
	return TRUE;
}

/*
int istrash(char c)
{
	return c==' ' || c=='\'';
}
*/

static double barranquismoticks;


int BARRANQUISMO_GetName(const char *title, vars &name, vars &aka) 
{
	//ASSERT( !strstr(title, "GIRONA"));
	//ASSERT( !strstr(title, "ARGAT"));
	vara uwords;
	vara words = getwords(vars(title).replace(" o ", ";").replace(" O ", ";").replace("\xE2\x80\x99", "'"));
	static vara lower, skip;
	if (lower.length()==0)
		{
		CSymList list;
		list.Load(filename(TRANSBASIC"SYM"));
		list.Load(filename(TRANSBASIC"PRE"));
		list.Load(filename(TRANSBASIC"POST"));
		list.Load(filename(TRANSBASIC"MID"));
		list.Add(CSym("Cañon ")); // no UTF8
		list.Add(CSym("Cañón ")); // no UTF8

		for (int i=0; i<list.GetSize(); ++i)
			{
			vars word = list[i].id;
			word = word.MakeUpper().Trim();
			if (!isa(word[0]))
				continue;
			if (list[i].id[0]==' ')
				if (list[i].id[strlen(list[i].id)-1]==' ' || list[i].id[strlen(list[i].id)-1]=='\'')
					lower.push(word);
			skip.push(word);
			}
		}

	// process '
	for (int i=words.length()-1; i>0; --i)
		if (words[i]=="\'")
			{
			words[i-1] += "\'";
			words.RemoveAt(i);
			}

	for (int i=0; i<words.length(); ++i)
		{
		vars &word = words[i];
		if (IsUpper(word) || word==";")
			{
			if (lower.indexOf(word)<0)
				word = word[0]+word.Mid(1).MakeLower();
			else
				word = word.MakeLower();
			if (skip.indexOf(word.upper())<0)
				uwords.push(word);
			}
		}
	vara akaa(words.join(""), ";");
	for (int i=akaa.length()-1; i>=0; --i)
		if (skip.indexOf(akaa[i].upper().trim())>=0)
			akaa.RemoveAt(i);
	aka = akaa.join(";");

	// trim
	while (uwords.length()>0 && lower.indexOf(uwords[0].upper())>=0)
		uwords.RemoveAt(0);
	while (uwords.length()>0 && uwords[0]<' ')
		uwords.RemoveAt(0);
	while (uwords.length()>0 && uwords[uwords.length()-1][0]<' ')
		uwords.RemoveAt(uwords.length()-1);
	name = uwords.join("").Trim(" ;'");
	name.Replace(";'", ";");
	name.Replace(" '", " ");
	while (name.Replace("  ", " "));
	if (name.IsEmpty())
		name = "(empty)";
	return TRUE;
}

int BARRANQUISMOKML_DownloadBeta(const char *ubase, CSymList &symlist) 
{
	ubase = "barranquismo.net";

	CFILE file;
	CString memory;
	if (file.fopen(vars(filename("bqn")).replace(".csv", ".kml")))
		{
		const char *line;
		while (line=file.fgetstr())
			memory += line;
		file.fclose();
		vara list(memory, "<Placemark");
		for (int i=1; i<list.length(); ++i)
			{
			vars name, aka;
			vars title = ExtractString(list[i], "<name>", "", "</name>");
			BARRANQUISMO_GetName(title, name, aka);
			vara coords(ExtractString(list[i], "<coordinates>", "", "</coordinates>"));
			vars msid = GetToken(ExtractString(list[i], "&msid=", "", "&"), 0, "]><");
			const char *str = "http://www.barranquismo.net/paginas";
			vars id = ExtractString(list[i], str, "", "]");
			if (!id.IsEmpty()) 
				id = str + id;
			else
				id = MkString("http://www.barranquismo.net?mapid=%d", i);
			CSym sym(urlstr(id), title);
			//sym.SetStr(ITEM_AKA, aka);
			sym.SetStr(ITEM_LAT, coords[1]);
			sym.SetStr(ITEM_LNG, coords[0]);
			sym.SetStr(ITEM_KML, msid);
			Update(symlist, sym, NULL, FALSE);
			}
		}
	return TRUE;
}


varas BARRANQUISMO_trans(
"http://barranquismo.net/paginas1/barrancos/canon_de_taourarte.htm=http://barranquismo.net/paginas/barrancos1/canon_de_taourarte.htm,"
"http://barranquismo.net/paginas1/barrancos/trevelez.htm=http://barranquismo.net/paginas/barrancos1/trevelez.htm,"
"http://barranquismo.net/paginas/barrancos/barranc_de_ensegur.htn=http://barranquismo.net/paginas/barrancos/barranc_de_ensegur.htm,"
"http://barranquismo.net/paginas/barrancos/ríu_de_cuesta_yanu.htm=http://www.barranquismo.net/paginas/barrancos/riu_de_cuesta_yanu.htm,"
"http://barranquismo.net/paginas/informacion/barranc_del_mas_del_coix.htm=http://www.barranquismo.net/paginas/barrancos/barranc_del_mas_del_coix.htm," 
"http://barranquismo.net/paginas/informacion/barranc_de_la_pinella.htm=http://www.barranquismo.net/paginas/barrancos/barranc_de_la_pinella.htm,"
"http://barranquismo.net/paginas/informacion/barranc_de_les_olles.htm=http://www.barranquismo.net/paginas/barrancos/barranc_de_les_olles.htm,"
"http://barranquismo.net/paginas/barranco_del_mas_de_fuentes.htm=http://www.barranquismo.net/paginas/barrancos/barranco_del_mas_de_fuentes.htm," 
		);

int BARRANQUISMO_DownloadBeta(const char *ubase, CSymList &symlist, int (*DownloadPage)(DownloadFile &f, const char *url, CSym &sym)) 
{
	DownloadFile f;
	if (MODE>=-2) // work offline
		{
		for (int i=0; i<symlist.GetSize(); ++i)
			{
			printf("%d/%d  \r", i, symlist.GetSize());
			CSym sym = symlist[i];
			if (!UpdateCheck(symlist, sym) && MODE>-2)
				continue;
			if (DownloadPage(f, sym.id, sym))
				Update(symlist, sym, NULL, FALSE);
			}
		return TRUE;
		}

	// load kml id,name,coords
	CSymList kmllist;
	kmllist.Sort();



	vara regions;
	{
	// download main list
	vars url = "http://www.barranquismo.net/nuevaspaginas.js";
	if (f.Download(url))
		{
		Log(LOGERR, "Can't download base url %s", url);
		return FALSE;
		}

	int findid = 0, findname = 0;
	vara list(f.memory, "<OPTION");
	for (int i=1; i<list.length(); ++i)
		{
		vars link = ExtractString(list[i], "", "VALUE=", ">").Trim("\"' ");
		vara name(ExtractString(list[i], "", ">", "\""), ".");
		if (!IsSimilar(link, "http"))
			continue;

		CSym sym(urlstr(link), UTF8(name[0]));
		if (BARRANQUISMO_trans[0].indexOf(sym.id)>=0)
			sym.id = BARRANQUISMO_trans[1][BARRANQUISMO_trans[0].indexOf(sym.id)];

		//sym.SetStr(ITEM_AKA, UTF8(name + ";" + AKA));
		//sym.SetStr(ITEM_REGION, UTF8(region));
		//sym.SetStr(ITEM_LAT, "@"+UTF8(loc));
		int found = kmllist.Find(sym.id);
		if (found>=0) ++findid;
		vars title = stripAccents(UTF8(name[0])).replace(" o ", ";");
		//ASSERT(!strstr(sym.GetStr(ITEM_DESC),"gedre"));
		//ASSERT(!strstr(sym.id,"gedre"));
		for (int k=0; found<0 && k<kmllist.GetSize(); ++k)
			{
			vars desc = stripAccents(kmllist[k].GetStr(ITEM_DESC)).replace(" o ", ";");
			if (IsSimilar(title, desc) || IsSimilar(desc, title))
				found = k, ++findname;
			}
		if (found>0)
			{
			// found locations
			sym.SetStr(ITEM_LAT, kmllist[found].GetStr(ITEM_LAT));
			sym.SetStr(ITEM_LNG, kmllist[found].GetStr(ITEM_LNG));
			}

		printf("%d %d/%d  \r", symlist.GetSize(), i, list.length());
		if (!UpdateCheck(symlist, sym))
			continue;
		if (DownloadPage(f, sym.id, sym))
			Update(symlist, sym, NULL, FALSE);
		//ASSERT(!strstr(sym.GetStr(ITEM_DESC),"Corredora"));
		Log(LOGINFO, "NEW REF %s,%s", sym.id, sym.GetStr(ITEM_DESC));
		}

	Log(LOGINFO, "#id:%d #name:%d  #tot:%d  #kmllist:%d ", findid, findname, findid+findname, kmllist.GetSize());
	}

	// download base
	vars url = "http://www.barranquismo.net/BD/cuadro_paises.js";
	if (f.Download(url))
		{
		Log(LOGERR, "Can't download base url %s", url);
		return FALSE;
		}
	vara list(f.memory, "href=");
	for (int i=1; i<list.length(); ++i)
		{
		vars region = stripHTML(ExtractString(list[i], "", ">", "</a"));
		vars rurl = ExtractString(list[i], "", "", " ").Trim("\"' ");
		if (rurl.IsEmpty())
			continue;

		url = rurl;
		Throttle(barranquismoticks, 1000);
		if (f.Download(url))
			{
			Log(LOGERR, "Can't download region url %s", url);
			return FALSE;
			}
		// page
		vars data = ExtractString(f.memory, "", "<hr", "Encontradas");
		vara dlist(data, "<hr");
		for (int d=0; d<dlist.length(); ++d)
			{
			//ASSERT( !strstr(dlist[d].lower(), "sorrosal"));
			vara lines(dlist[d], "<font");//.replace("<!--<a href", "<!--<trash");
			if (lines.length()<4)
				continue;
			vars title = stripHTML(ExtractString(lines[1], "", ">", "</font"));
			vars loc = stripHTML(ExtractString(lines[2], "", ">", "</font"));
			vara links(ExtractString(dlist[d],"","","</table"), "href=");
			for (int l=1; l<links.length(); ++l)
				{
				vars link = GetToken(ExtractString(links[l], "", "", ">"),0,' ').Trim("\"' ");
				if (!strstr(link, ubase) || strstr(link, "bibliografia.php") || strstr(link, "/reglamentos/"))
					continue;
				//ASSERT(!strstr(title, "BARRANCO DEL AGUA"));
				vars name, AKA;
				BARRANQUISMO_GetName(title, name, AKA);
				CSym sym(urlstr(link), UTF8(name));
				if (BARRANQUISMO_trans[0].indexOf(sym.id)>=0)
					sym.id = BARRANQUISMO_trans[1][BARRANQUISMO_trans[0].indexOf(sym.id)];

				sym.SetStr(ITEM_AKA, UTF8(name + ";" + AKA));
				sym.SetStr(ITEM_REGION, UTF8(region));
				sym.SetStr(ITEM_LAT, "@"+UTF8(loc));

				printf("%d %d/%d %d/%d  \r", symlist.GetSize(), i, list.length(), d, dlist.length());
				if (!UpdateCheck(symlist, sym) && MODE>-2)
					continue;
				if (DownloadPage(f, sym.id, sym))
					Update(symlist, sym, NULL, FALSE);
				//ASSERT(!strstr(sym.GetStr(ITEM_DESC),"Corredora"));
				}
			}
		}



	return TRUE;
}

int BARRANQUISMO_DownloadBeta(const char *ubase, CSymList &symlist) 
{
	ubase = "barranquismo.net";
	return BARRANQUISMO_DownloadBeta(ubase, symlist, BARRANQUISMO_DownloadPage);
}


int BARRANQUISMODB_DownloadBeta(const char *ubase, CSymList &symlist) 
{
	DownloadFile f;

	vara sum; sum.SetSize(R_SUMMARY);
	if (MODE<=-2 || symlist.GetSize()==0)
	{
	int num = 0;
	// download base
	vars url = "http://www.barranquismo.net/BD/cuadro_paises.js";
	if (f.Download(url))
		{
		Log(LOGERR, "Can't download base url %s", url);
		return FALSE;
		}
	vara list(f.memory, "href=");
	for (int i=1; i<list.length(); ++i)
		{
		vars region = stripHTML(ExtractString(list[i], "", ">", "</a"));
		vars rurl = ExtractString(list[i], "", "", " ").Trim("\"' ");
		if (rurl.IsEmpty())
			continue;

		url = rurl;
		Throttle(barranquismoticks, 1000);
		if (f.Download(url))
			{
			Log(LOGERR, "Can't download region url %s", url);
			return FALSE;
			}
		// page
		vars data = ExtractString(f.memory, "", "<hr", "Encontradas");
		vara dlist(data, "<hr");
		for (int d=0; d<dlist.length(); ++d)
			{
			//ASSERT( !strstr(dlist[d].lower(), "sorrosal"));
			vara lines(dlist[d], "<font");//.replace("<!--<a href", "<!--<trash");
			if (lines.length()<4)
				continue;
			vars loc;
			vars title = stripHTML(ExtractString(lines[1], "", ">", "</font"));
			vara locs( ExtractString(lines[2], "", ">", "</font"), "<br>");
			for (int l=0;l<locs.length(); ++l)
				{
				locs[l] = stripHTML(locs[l]);
				if (!locs[l].IsEmpty())
					loc = locs[l];
				}

			dlist[d].Replace("HREF=","href=");
			vara links(dlist[d], "href=");

			vara linklist;
			for (int l=1; l<links.length(); ++l)
				{
				vars link = GetToken(ExtractString(links[l], "", "", ">"),0,' ').Trim("\"' ");
				if (strstr(link, "/reglamentos/"))
					continue;
				//ASSERT(!strstr(title, "BARRANCO DEL AGUA"));
				link = urlstr(link);
				if (BARRANQUISMO_trans[0].indexOf(link)>=0)
					link = BARRANQUISMO_trans[1][BARRANQUISMO_trans[0].indexOf(link)];
				if (linklist.indexOf(link)<0)
					linklist.push(link);
				//ASSERT(!strstr(sym.GetStr(ITEM_DESC),"Corredora"));
				}

			//vars name, AKA;
			//BARRANQUISMO_GetName(title, name, AKA);
			//vars id = stripAccents(title);
			CSym sym(MkString("BQN:%d",num++), UTF8(title));

			//sym.SetStr(ITEM_KML, UTF8(title));
			//sym.SetStr(ITEM_AKA, UTF8(name + ";" + AKA));
			sym.SetStr(ITEM_REGION, UTF8(region));
			sym.SetStr(ITEM_LAT, "@"+UTF8(loc));
			//sym.SetStr(ITEM_LNG, UTF8(title));
			sym.SetStr(ITEM_EXTRA, linklist.join(" "));
			sym.SetStr(ITEM_CONDDATE, UTF8(locs.join(";")));

			printf("%d %d/%d %d/%d  \r", symlist.GetSize(), i, list.length(), d, dlist.length());
			Update(symlist, sym, NULL, FALSE);
			}
		}
	}

	CSymList bslist;
	LoadBetaList(bslist);

	const char *GetCodeStr(const char *url);
	for (int i=0; i<symlist.GetSize(); ++i)
		{
		CSym &sym = symlist[i];

		vars name = sym.GetStr(ITEM_DESC);
		name.Replace(" o ", ";");
		name.Replace(" O ", ";");
		vars Capitalize(const char *oldval);
		sym.SetStr(ITEM_DESC, name);
		sym.SetStr(ITEM_AKA, Capitalize(name));




		// try mapping
		//if (!IsSimilar(sym.GetStr(ITEM_INFO),RWID))
			{
			vara linklist( sym.GetStr(ITEM_EXTRA), " " );

			int b;
			vara codes, nocodes;
			vars matched;
			for (int l=0; l<linklist.length() && matched.IsEmpty(); ++l)
				{
				vars url, ourl;
				ourl = url = linklist[l];
				if (url=="-")
					{
					linklist.RemoveAt(l--);
					continue;
					}
				if (!IsSimilar(url,"http"))
					continue;
				if (strstr(url,"barranquismo.net/BD/bibliografia"))
					continue;
				//http://canyon.carto.net/cwiki/bin/view/Canyons/?topic=AlmbachklammCanyon
				if (strstr(url, "carto.net"))
					{
					url.Replace("/?topic=","");
					url.Replace("/view/Canyons","/view/Canyons/");
					url.Replace("Canyons//", "Canyons/");
					}
				//http://descente-canyon.com/canyoning.php/208-21304-canyons.html 
				//http://descente-canyon.com/canyoning/canyon-description/22332
				if (strstr(url, "descente-canyon.com"))
					{
					vars id = ExtractString(url,".php/","-","-");
					if (id.IsEmpty())
						id = ExtractString(url,".php/","/","/");
					if (strstr(url, "/canyoning/canyon/2"))
						id = ExtractString(url,"/canyoning/canyon/","","/");
					if (strstr(url, "reglementation/"))
						id = ExtractString(url,"reglementation/","","/");				
					if (!id.IsEmpty())
						url = "http://descente-canyon.com/canyoning/canyon-description/"+id;
					}// "http://descente-canyon.com/canyoning/canyon-description/2852"

				if (strstr(url, "ankanionla"))
					{
					url.Replace("/Files/Other","");
					url.Replace(".pdf",".htm");
					}

				if (strstr(url, "micheleangileri.com"))
					{
					url.Replace("scheda.cgi","schedap.cgi");
					}
				if (strstr(url, "ankanionla"))
					{
					url.Replace("ankanionla","ankanionla-madinina.com");
					}
				if ((b=bslist.Find(url))>=0)
					{
					matched = bslist[b].data;
					}
				else
					{
					const char *code = GetCodeStr(url);
					if (code)
						codes.push(code);
					else
						nocodes.push(url);
					//Log(LOGWARN, "Could not map url %s", url);
					}
				}

			sym.SetStr(ITEM_LINKS, nocodes.join(" "));

			// update INFO if needed
			if (!matched.IsEmpty())
				if (!IsSimilar(sym.GetStr(ITEM_INFO),RWID))
					sym.SetStr(ITEM_INFO, matched);

			// separate loc and see if 
			vara region;
			int geoloc = FALSE;
			vars loc = sym.GetStr(ITEM_LAT).Trim("@");
			if (!loc.IsEmpty())
			{
			vara locs(loc.replace(".", ";"), ";");
			for (int l=0; l<locs.length(); ++l)
				{
				int low = FALSE;
				const char *str = locs[l];
				for (const char *s =str; !low && *s!=0; ++s)
					if (islower(*s))
						low = TRUE;
				// is it lowercase?
				if (!low)
					region.push(str);
				else
					geoloc = TRUE;
				}
			sym.SetStr(ITEM_REGION, invertregion(region.join(";")));
			if (!geoloc)
				sym.SetStr(ITEM_LAT, "");
			}
			
			vars c = "-1:NoLinks";
			if (codes.length()>0)
				c = "-2:DupLinks"+codes.join("-");
			if (nocodes.length()>0)
				c = "0:Links";
			if (geoloc)
				c = "1:Geoloc";
			sym.SetStr(ITEM_CLASS, c);	
			}
		
		}




	return TRUE;
}



int BARRANCOSORG_DownloadBeta(const char *ubase, CSymList &symlist) 
{
	//BARRANQUISMO_DownloadBeta(ubase, symlist, BARRANCOSORG_DownloadPage);

	const char *urls[] = { 
		"http://barrancos.org/guara.htm",
		"http://barrancos.org/pirineoh.htm",
		"http://barrancos.org/cataluny.htm",
		"http://barrancos.org/andalucia.htm",
		"http://barrancos.org/Euskadi.htm",
		"http://barrancos.org/Asturias.htm",
		"http://barrancos.org/C_Corcega.htm",
		"http://barrancos.org/Ariege/index.htm",
		"http://barrancos.org/alpes/index.htm",
		"http://barrancos.org/lombardia/index.htm",
		"http://barrancos.org/Madeira/index.htm",
		"http://barrancos.org/Eslovenia/index.htm",
		NULL};

	vars url;
	DownloadFile f;
	for (int u=0; urls[u]!=NULL; ++u)
		{
		if (f.Download(url=urls[u]))
			{
			Log(LOGERR, "Can't download base url %s", url);
			continue;
			}

		vara list(f.memory, "<tr");
		int inota = -1, icuerda = -1, icoches = -1, ihoras = -1;
		for (int i=1; i<list.length(); ++i)
			{	
			vara td(ExtractString(list[i], ">", "", "</tr"), "<td");
			if (i==1)
				{
				for (int i=1; i<td.length(); ++i)
					{
					if (strstr(td[i], "Cuerda")) icuerda = i;
					if (strstr(td[i], "Nota")) inota = i;
					if (strstr(td[i], "Coches")) icoches = i;
					if (strstr(td[i], "Horas")) ihoras = i;
					}
				}

			vars link = ExtractString(td[1], "href=", "\"", "\"");
			if (link.IsEmpty())
				link = ExtractString(td[2], "href=", "\"", "\"");
			if (link.IsEmpty())
				continue;
			if (!IsSimilar(link, "http") || strstr(link, ".zip"))
				{
				vara path(url, "/");
				path[path.length()-1] = link;
				link = path.join("/");
				}
			vars name = stripHTML(ExtractString(td[1], ">", "", "</td"));
			CSym sym(urlstr(link), UTF8(skipnoalpha(name)));

			if (inota>0)
				{
				if (td.length()<=inota) continue;
				vars nota = stripHTML(ExtractString(td[inota], ">", "", "</td"));
				double stars = CDATA::GetNum(nota);
				if (stars>0)
					sym.SetStr(ITEM_STARS, starstr(stars/2,1));
				}
			if (icuerda>0)
				{
				double longest = -1;
				vars cuerda = stripHTML(ExtractString(td[icuerda], ">", "", "</td"));
				if (IsSimilar(cuerda, "2x"))
					longest = CDATA::GetNum(GetToken(cuerda, 1, 'x'));
				else if (IsSimilar(cuerda, "1x"))
					longest = CDATA::GetNum(GetToken(cuerda, 1, 'x'))/2;
				else 
					longest = CDATA::GetNum(cuerda);
				if (longest>=0)
					sym.SetStr(ITEM_LONGEST, MkString("%.0fm", longest));
				}
			if (ihoras>0)
				{	
				vars horas = stripHTML(ExtractString(td[7], ">", "", "</td"));
				if (!horas.IsEmpty())
					{
					if (!horas.Replace("minutos","'"))
						horas += "h";
					double vmin = 0, vmax = 0;
					GetHourMin(horas, vmin, vmax, url);
					if (vmin>0)
						sym.SetNum(ITEM_MINTIME, vmin);
					if (vmax>0)
						sym.SetNum(ITEM_MAXTIME, vmax);
					}
				}			

			Update(symlist, sym, NULL, FALSE);
			}
		
		}

	return TRUE;
}


// ===============================================================================================
	
static double altisudticks;

int ALTISUD_DownloadPage(DownloadFile &f, const char *url, CSym &sym)
{
	Throttle(altisudticks, 1000);
	if (f.Download(url))
		{
		Log(LOGERR, "Can't download base url %s", url);
		return FALSE;
		}

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

	// coords
	/*
	vars ogps;
	double lat = InvalidNUM, lng = InvalidNUM;
	vars coords = stripHTML(ExtractString(memory, ">point gps", "<span", "</span"));
	ExtractCoords(gps, lat, lng, ogps);
	*/
	double lat = ExtractNum(memory, "\"latitude\"", "=\"", "\"");
	double lng = ExtractNum(memory, "\"longitude\"", "=\"", "\"");
	if (CheckLL(lat,lng))
		{
		sym.SetNum(ITEM_LAT, lat);
		sym.SetNum(ITEM_LNG, lng);
		}

	sym.SetStr(ITEM_DEPTH, GetMetric(stripHTML(ExtractString(strstr(memory, UTF8(">Dénivelé")), "<div", ">", "</div").replace(" ",""))));
	sym.SetStr(ITEM_LENGTH, GetMetric(stripHTML(ExtractString(strstr(memory, UTF8(">Longueur")), "<div", ">", "</div").replace(" ",""))));	

	return TRUE;
}

int ALTISUD_DownloadBeta(const char *ubase, CSymList &symlist) 
{
	vara regions;
	DownloadFile f;

	// download base
	int u = 0;
	vars url;
	vara urllist, visited;
	urllist.push("http://www.altisud.com/canyoning.html");
	
	while (urllist.GetSize()>u)
	{
	Throttle(altisudticks, 1000);
	if (f.Download(url=urllist[u++]))
		{
		Log(LOGERR, "Can't download base url %s", url);
		return FALSE;
		}
	vars sep = "<a href=\"/canyoning/";
	vara list(f.memory, sep);
	for (int i=1; i<list.length(); ++i)
		{
		vars link = "http://www.altisud.com/canyoning/" + ExtractString(list[i], "", "", "\"");
		if (visited.indexOf(link)>=0)
			continue;
		visited.push(link);

		if (!strstr(link, "-fiche.html"))
			{
			urllist.push(link);
			continue;
			}
	
		// download fiche
		CSym sym(urlstr(link));
		printf("%d %d/%d %d/%d  \r", symlist.GetSize(), u, urllist.length(), i, list.length());
		if (!UpdateCheck(symlist, sym) && MODE>-2)
			continue;
		if (ALTISUD_DownloadPage(f, link, sym))
			Update(symlist, sym, NULL, FALSE);
		}

	}

	return TRUE;
}

// ===============================================================================================
	

class REUNION : public BETAC
{
public:

	REUNION(const char *base) : BETAC(base)
	{
		ticks = 1000;
		umain = "http://www.lrc-ffs.fr/";
		//urls.push("http://www.gulliver.it/canyoning/");
		//x = BETAX("<tr", "</tr", ">nome itinerario", "</table");

	}

	int DownloadPage(const char *url, CSym &sym)
	{
		CSymList lines;
		DownloadUrl(url, lines, BETAX("class=", NULL, "<h1", ">Statistiques"), D_DATA);

		vara sketch;
		for (int i=0; i<lines.GetSize(); ++i)
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

	int DownloadBeta(CSymList &symlist) 
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

		vara rlist( ExtractString(f.memory, "<map", "", "</ul"), "href=");
		for (int r=1; r<rlist.length(); ++r)
			{
			vars link = GetToken(rlist[r], 0, " <>").Trim("'\"");
			if (link.IsEmpty())
				continue;
			link = burl(ubase, link);
			if (done.indexOf(link)>=0)
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
			for (int i=1;i<geotags.length(); ++i)
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

			vara markers( ExtractString(f.memory, "lmm-listmarkers", "", "</table"), "<tr");
			for (int i=1;i<markers.length(); ++i)
				{
				vars topolink;
				vars name = stripHTML(ExtractString(markers[i], "lmm-listmarkers-markername", ">", "<"));
				vara links(markers[i], "href=");
				for (int l=1; l<links.length() && topolink.IsEmpty(); ++l)
					{
					if (IsSimilar(links[l],"\"\""))
						continue;
					vars link = ExtractString(links[l], "\"", "", "\"").Trim();
					if (link.IsEmpty())
						link = ExtractString(links[l], "'", "", "'");
					if (link.IsEmpty() || !IsSimilar(link, "http"))
						Log(LOGWARN, "Empty link for %s in %s", name, url);
					if (strstr(link, "page_id"))
						topolink = burl(ubase, link.replace("\xC2\xA0",""));
					}
				if (topolink.IsEmpty())
					continue;

				const char *sname = name;
				while (isdigit(*sname) || isspace(*sname))
					++sname;

				CSym sym(urlstr(topolink), sname);
				sym.SetStr(ITEM_REGION, "Reunion");

				int find = clist.Find(name);
				if (find>=0)
					{
					double lat = clist[find].GetNum(ITEM_LAT);
					double lng = clist[find].GetNum(ITEM_LNG);
					if (CheckLL(lat,lng))				 
					  if (lat>-21.429 && lat<-20.801 && lng>55.127 && lng<55.918) //-21.429,55.127,-20.801,55.918
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

};

// ===============================================================================================
	
//static double altisudticks;

int GUARAINFO_DownloadBeta(const char *ubase, CSymList &symlist) 
{
	vara regions;
	DownloadFile f;

	// download base
	vars url = "http://www.guara.info/guara-base-de-datos/descenso-de-barrancos/";	
	if (f.Download(url))
		{
		Log(LOGERR, "Can't download base url %s", url);
		return FALSE;
		}
	vars region;
	vars memory = ExtractString(f.memory, "\""+url+"\"", "", "\"http://www.guara.info/guara-base-de-datos/escalada/\"");
	vara list(memory, "<li");
	for (int i=1; i<list.length(); ++i)
		{	
		vars link = ExtractString(list[i], "href=", "\"", "\"");
		vars name = stripHTML(ExtractString(list[i], "<a ", ">", "</a"));
		if (strstr(list[i], "menu-item-has-children") && !IsSimilar(name, "Barranco"))
			{
			region = name;
			continue;
			}
		if (link.IsEmpty())
			continue;
		if (strstr(link, "wp-json"))
			continue;
		if (!strstr(link, "base-de-datos"))
			continue;
	
		// download fiche
		CSym sym(urlstr(link), name);
		sym.SetStr(ITEM_REGION, region);
		sym.SetStr(ITEM_AKA, name);

		if (IsSimilar(region, "cuenca"))
			sym.SetStr(ITEM_LAT, "@Sierra de Guara.Huesca.Spain");
		Update(symlist, sym, NULL, FALSE);
		}

	return TRUE;
}

	
// ===============================================================================================

//static double altisudticks;

int TRENCANOUS_DownloadBeta(const char *ubase, CSymList &symlist) 
{
	vara regions;
	DownloadFile f;

	// download base
	//vars url = "http://www.latrencanous.com/barrancos/barrancos.html";	
const char *urls[] = { "http://www.latrencanous.com/?page_id=480", "http://www.latrencanous.com/?page_id=490", 
			"http://www.latrencanous.com/?page_id=660", "http://www.latrencanous.com/?page_id=644", NULL };
for (int u=0; urls[u]!=NULL; ++u)
{
	vars url = urls[u];
	if (f.Download(url))
		{
		Log(LOGERR, "Can't download base url %s", url);
		return FALSE;
		}
	vars region;
	vars memory = ExtractString(f.memory, "<table", "", "</table");
	vara list(memory, "<tr");
	for (int i=1; i<list.length(); ++i)
		{	
		vara td(list[i], "<td");
		if (td.length()<3)
			continue;
		vars name = stripHTML(ExtractString(td[1], "", ">", "</td"));

		vars link;
		for (int t=3; t<td.length() && link.IsEmpty(); ++t)
			{
			vars tlink = ExtractString(td[t], "href=", "\"", "\"");
			if (tlink.IsEmpty())
				continue;
			if (!IsSimilar(tlink,"http"))
				tlink = "http://latrencanous.com/barrancos/"+tlink;
			const char *ext = GetFileExt(tlink);
			if (!ext || stricmp(ext,"pdf")!=0)
				continue;
			if (!strstr(tlink, "latrencanous.com"))
				continue;
			link = tlink;
			break;
			}
		/*
		vars link = ExtractString(td[3], "href=", "\"", "\"");
		if (!link.IsEmpty() && !IsSimilar(link,"http"))
			link = "http://latrencanous.com/barrancos/"+link;
		if (link.IsEmpty() || !strstr(link,ubase))
			{
			link = ExtractString(td[8], "href=", "\"", "\"");
			if (!link.IsEmpty() && !IsSimilar(link,"http"))
				link = "http://latrencanous.com/barrancos/"+link;
			}
		*/
		if (link.IsEmpty())
			continue;

		vars subregion, region;
		region = stripHTML(ExtractString(td[2], "", ">", "</td"));
		if (td.length()>3)
			{
			subregion = stripHTML(ExtractString(td[3], "", ">", "</td"));
			if (!IsSimilar(subregion,"http"))
				subregion = "";
			if (!subregion.IsEmpty())
				if (region.IsEmpty())
					region = subregion;
				else
					region += ";" + subregion;
			}

		// download fiche
		int ferrata = u>1;
		if (ferrata) name = "Via Ferrata "+name;
		CSym sym(urlstr(link), name);
		sym.SetStr(ITEM_REGION, region);
		sym.SetStr(ITEM_AKA, name);
		sym.SetStr(ITEM_CLASS, ferrata ? "3:Ferrata" : "1:Canyon");

		printf("%d %d/%d   \r", symlist.GetSize(), i, list.length());
		if (!UpdateCheck(symlist, sym) && MODE>-2)
			continue;
		Update(symlist, sym, NULL, FALSE);
		}
}

// MAPA
#if 0
{
const char *urls[] ={ "http://www.latrencanous.com/?page_id=63", "http://www.latrencanous.com/?page_id=69", NULL};
for (int u=0; urls[u]!=NULL; ++u)
	{
	vars url = urls[u];
	if (f.Download(url))
		{
		Log(LOGERR, "Can't download base url %s", url);
		return FALSE;
		}

	vars mid = ExtractString(f.memory, "google.com", "mid=", "\"");
	url = "http://www.google.com/maps/d/u/0/kml?mid="+mid+"&forcekml=1";
	if (f.Download(url))
		{
		Log(LOGERR, "Can't download base url %s", url);
		return TRUE;
		}
	if (!strstr(f.memory, "<Placemark>"))
		Log(LOGERR, "ERROR: Could not download kml %s", url);
	vara list(f.memory, "<Placemark");
	for (int i=1; i<list.length(); ++i)
		{
		vars oname = ExtractString(list[i], "<name", ">", "</name");
		vars name = stripSuffixes(oname);
		if (name.IsEmpty())
			{
			//Log(LOGWARN, "Could not map %s", oname);
			continue;
			}
		vara coords(ExtractString(list[i], "<coordinates", ">", "</coordinates"));
		if (coords.length()<2)
			continue;
		double lat = CDATA::GetNum(coords[1]);
		double lng = CDATA::GetNum(coords[0]);
		if (!CheckLL(lat,lng))
			{
			Log(LOGERR, "Invalid KML coords %s from %s", coords.join(), url);
			continue;
			}
		int matched = 0;
		vara idlist;
		vara links(list[i], "http");
		for (int i=1; i<links.length(); ++i)
			{
			//vars pdflink = GetToken(links[i], 0, "[](){}<> ");
			int len = links[i].Find(".pdf");
			if (len<0)
				len = links[i].Find(".PDF");
			if (len<0)
				continue;
			vars pdflink = "http"+links[i].Left(len+4);
			if (pdflink.IsEmpty() || !strstr(pdflink, "latrencanous.com"))
				continue;
			idlist.push(urlstr(pdflink));
			}

		int maxn = symlist.GetSize();
		for (int n=0; n<maxn; ++n)
			{
			CSym &sym = symlist[n];
			if (idlist.length()>0)
				{
				// match by id
				if (idlist.indexOf(sym.id)>=0)
					{
					sym.SetNum(ITEM_LAT, lat);
					sym.SetNum(ITEM_LNG, lng);
					++matched;
					}
				}
			//if (!matched)
				{
				// match by class and name
				int c = sym.GetNum(ITEM_CLASS)==3;
				if (u!=c)
					continue;
				vars symname = stripSuffixes(sym.GetStr(ITEM_DESC));
				//ASSERT( !strstr(name, "Berche") || !strstr(symname, "Berche"));
				if (!symname.IsEmpty())
					if (IsSimilar(symname, name) || strstr(symname, name) || strstr(name, symname))
					{
					sym.SetNum(ITEM_LAT, lat);
					sym.SetNum(ITEM_LNG, lng);
					++matched;
					}
				}
			}

		if (!matched)
			{
			//Log(LOGERR, "Invalid LTN name match '%s'", name);
			}
		}
	}
}
#endif

	return TRUE;
}

/*
int TRENCANOUS_DownloadPage(DownloadFile &f, const char *url, CSym &sym)
{
	return TRUE;
}

int TRENCANOUS_DownloadBeta(const char *ubase, CSymList &symlist) 
{
	return TRENCANOUS_DownloadBeta(ubase, symlist, TRENCANOUS_DownloadPage);
}
*/




int KMLMAP_DownloadBeta(const char *ubase, CSymList &symlist) 
{
	DownloadFile f;
	const char *maps[] = { "1JC0HCFFQxUbWxj_RiKkVANkOy7U", "1ArMXRHonoFF54K4QLl-AcXRtYb0", NULL };

	for (int m=0; maps[m]!=NULL; ++m)
	{
	vars mid = maps[m];//ExtractString(f.memory, "google.com", "mid=", "\"");
	vars url = "http://www.google.com/maps/d/u/0/kml?mid="+mid+"&forcekml=1";
	if (f.Download(url))
		{
		Log(LOGERR, "Can't download base url %s", url);
		return TRUE;
		}
	if (!strstr(f.memory, "<Placemark>"))
		Log(LOGERR, "ERROR: Could not download kml %s", url);
	vara list(f.memory, "<Placemark");
	for (int i=1; i<list.length(); ++i)
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
		if (coords.length()<2)
			continue;
		double lat = CDATA::GetNum(coords[1]);
		double lng = CDATA::GetNum(coords[0]);
		if (!CheckLL(lat,lng))
			{
			Log(LOGERR, "Invalid KML coords %s from %s", coords.join(), url);
			continue;
			}
		vara linklist;
		vara links(list[i], "http");
		for (int i=1; i<links.length(); ++i)
			{
			//vars pdflink = GetToken(links[i], 0, "[](){}<> ");
			vars link = GetToken(links[i], 0, "()<> ").Trim(" '\"");
			if (link.IsEmpty())
				continue;
			linklist.push(urlstr("http"+link));
			}
		CSym sym(ubase+name, name );			
		sym.SetNum(ITEM_LAT, lat);
		sym.SetNum(ITEM_LNG, lng);
		sym.SetStr(ITEM_AKA, name);
		sym.SetStr(ITEM_LINKS, linklist.join(" "));

		int found;
		if (m>0 && (found=symlist.Find(sym.id))>=0)
			{
			CSym &osym = symlist[found];
			int diff = 0;
			if (Distance(osym.GetNum(ITEM_LAT),osym.GetNum(ITEM_LNG), sym.GetNum(ITEM_LAT),sym.GetNum(ITEM_LNG))>1000)
				++diff;
			if (osym.GetStr(ITEM_LINKS) != sym.GetStr(ITEM_LINKS))
				++diff;
			if (diff)
				{
				// different
				sym.id += MkString("[%d]",m);
				}
			}

		Update(symlist, sym, NULL, FALSE);
		}
	}

	return TRUE;
}





class FLREUNION : public BETAC
{
public:
	FLREUNION(const char *base) : BETAC(base)
	{
		x = BETAX("<p", NULL, "</table",  "<table");
		urls.push("http://francois.leroux.free.fr/canyoning/canyions.htm");
		urls.push("http://francois.leroux.free.fr/canyoning/classics.htm");
	}

	int DownloadInfo(const char *data, CSym &sym)
	{
		sym.SetStr(ITEM_DESC, UTF8(sym.GetStr(ITEM_DESC)));
		sym.SetStr(ITEM_REGION, "Reunion");
		return TRUE;
	}

};



class ZIONCANYONEERING : public BETAC
{
	vars ctitle, cmore, cmore2;

public:
	ZIONCANYONEERING(const char *base): BETAC(base)
	{
		umain = "http://zioncanyoneering.com";
	}

	vars ExtractVal(const char *xml, const char *id)
	{
		vars item = *id!=0 ? MkString("\"%s\"", id) : "";
		vars str = ExtractString(xml, item, ":", ",");
		str.Trim("\" ");
		if (str=="null")
			str = "";
		return str;
	}

	int DownloadBeta(CSymList &symlist)
	{
		if (f.Download("http://zioncanyoneering.com/api/CanyonAPI/GetAllCanyons"))
			return FALSE;

		vara list(f.memory, "\"Name\"");
		for (int i=1; i<list.GetSize(); ++i)
			{
			vars &xml = list[i];
			vars name = ExtractVal(xml, "");
			vars desc = ExtractVal(xml, "Description");
			vars url = ExtractVal(xml, "URLName");
			vars region = ExtractVal(xml, "State");

			double n = CDATA::GetNum(ExtractVal(xml, "NumberOfVoters"));
			double score = CDATA::GetNum(ExtractVal(xml, "PublicRating"));

			if (n>0 || !desc.IsEmpty())
				{
				// add only if voted of desc not empty
				CSym sym(urlstr("http://zioncanyoneering.com/Canyon/"+url), name);
				sym.SetStr(ITEM_REGION, region);
				if (n>0 && score>0)
					sym.SetStr(ITEM_STARS, starstr(score/n, n));

				double lat = CDATA::GetNum(ExtractVal(xml, "Latitude"));
				double lng = CDATA::GetNum(ExtractVal(xml, "Longitude"));
				if (CheckLL(lat,lng))
					sym.SetStr(ITEM_LAT, MkString("@%s;%s", CData(lat), CData(lng)));

				Update(symlist, sym, NULL, 0);
				}

			}

		return TRUE;
	}

};



class BETAWPC : public BETAC
{
	vars ctitle, cmore, cmore2;

public:
	BETAWPC(const char *base, const char *ctitle = "class='post-title", 
		const char *cmore = "class='blog-pager-older-link",
		const char *cmore2 = "</a"): BETAC(base)
	{
		ticks = 500;
		x = BETAX(this->ctitle = ctitle); //, "</h3");
		this->cmore = cmore;
		this->cmore2 = cmore2;
	}


	vars DownloadMore(const char *memory) 
	{
		return ExtractHREF(ExtractString(f.memory, cmore, "", cmore2));
	}

};


class ESPELEOCANYONS : public BETAWPC
{
public:
	ESPELEOCANYONS(const char *base) : BETAWPC(base)
	{
		urls.push("http://teamespeleocanyons.blogspot.com.es");
	}

	int DownloadInfo(const char *data, CSym &sym)
	{
		vars id = ExtractString(sym.id, ".blogspot.", "", "/");
		if (id != "com.es")
			sym.id.Replace(id, "com.es");

		// check if empty
		vars text = stripHTML(ExtractString(data, "</h3>", "", "<div class='post-footer"));
		if (text.length()<20)
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
		urls.push(umain="http://www.barranquistas.es/");
	}

	int DownloadInfo(const char *data, CSym &sym)
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
		urls.push(umain="http://www.rocjumper.com/barranco/");
		x = BETAX("<h3", "</h3");
		//x = BETAX("itemprop=\"blogPost\"", NULL);
	}
	
	int DownloadInfo(const char *data, CSym &sym)
	{
		sym.id = urlstr(GetToken(sym.id, 0, '#'));
		sym.SetStr(ITEM_CLASS, strstr(sym.id,"/barranco/") ? "1:Barranco" : "-1:NoBarranco");
		return strstr(sym.id,"/barranco/")!=NULL;
	}

	vars DownloadMore(const char *memory) 
	{
		double maxp = ExtractNum(f.memory, "'pagination-meta'", " de ", "<span");
		if (maxp<0 || p>=maxp)
			return "";
		return umain+MkString("page/%d/",++p);
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

	int DownloadInfo(const char *data, CSym &sym)
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

	vars DownloadMore(const char *memory) 
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

	int DownloadInfo(const char *data, CSym &sym)
	{
		vars name = sym.GetStr(ITEM_DESC);
		int f = name.indexOf("Canyonee");
		//int c = name.indexOf("Creek");
		//if (c>=0) f = name.indexOf(" ", c);
		if (f<0) // && c<0)
			return FALSE;

		//name = name.Mid(0,f).Trim();
		//sym.SetStr(ITEM_DESC, name);

		//sym.id = vars(sym.id).replace("www.", "").replace("WWW.", "").replace("//", "//WWW.");
		//sym.SetStr(ITEM_REGION,"United States");
		return TRUE;
	}
};




class CMAGAZINE : public BETAC
{
public:
	int p;

	CMAGAZINE(const char *base) : BETAC(base)
	{
		p = 1;
		urls.push(umain="https://canyonmag.net/wordpress/wp-admin/admin-ajax.php?POST?action=wpas_ajax_load&page=1&form_data=paged%3D1%26wpas_id%3Ddatabase_location%26wpas_submit%3D1%26paged%3D1%26wpas_id%3Ddatabase_vertical%26wpas_submit%3D1%26paged%3D1%26wpas_id%3Ddatabase_aquatic%26wpas_submit%3D1%26paged%3D1%26wpas_id%3Ddatabase_rating%26wpas_submit%3D1");

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
		while ( (find= memory.Find(unicode))>=0)
			{
			vars code = memory.Mid(find+unicode.GetLength(), 2);
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


	vars country = stripHTML(ExtractString(f.memory, ">Country",">","</tr"));
	vars prefecture = stripHTML(ExtractString(f.memory, ">Prefecture",">","</tr"));
	vars city = stripHTML(ExtractString(f.memory, ">City",">","</tr"));

	sym.SetStr(ITEM_REGION, country+";"+prefecture);


	sym.SetStr(ITEM_DEPTH, GetMetric(stripHTML(ExtractString(f.memory, ">Difference",">","</tr"))));
	sym.SetStr(ITEM_LENGTH, GetMetric(stripHTML(ExtractString(f.memory, ">Length",">","</tr"))));

	sym.SetNum(ITEM_RAPS, CDATA::GetNum(stripHTML(ExtractString(f.memory, ">Rappels",">","</tr"))));	
	sym.SetStr(ITEM_LONGEST, GetMetric(stripHTML(ExtractString(f.memory, ">Max",">","</tr"))));

	vars sum = ExtractString(f.memory, ">Rating",">","</tr");
	GetSummary(sym, stripHTML(sum));

	int stars = sum.Replace("class=\"fa fa-star\"","");
	if (stars>0)
		sym.SetStr(ITEM_STARS, starstr(stars+1,1));

	vars coords = stripHTML(ExtractString(f.memory, ">Entry",">","</tr"));
	double lat = CDATA::GetNum(GetToken(coords,0,';'));
	double lng = CDATA::GetNum(GetToken(coords,1,';'));
	if (CheckLL(lat,lng))
		{
		sym.SetNum(ITEM_LAT, lat);
		sym.SetNum(ITEM_LNG, lng);
		}
	else
		{
		sym.SetStr(ITEM_LAT, "@"+city+"."+prefecture+"."+country);
		}

	return TRUE;
	}

};


class YADUGAZ : public BETAC
{
public:

	YADUGAZ(const char *base) : BETAC(base)
	{
		urls.push(umain="http://www.yadugaz07.com/canyon-ardeche-lozere-drome.php");
		x = BETAX("<tr", "</tr", ">Liste des ", "</table");
	}

	int DownloadInfo(const char *data, CSym &sym)
	{		
		double val = CDATA::GetNum(stripHTML(ExtractString(data, "smiley", ">", "</td")));
		if (val>0)
			sym.SetStr(ITEM_STARS, starstr(val/2, 1));
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
		umain="http://climbing7.wordpress.com/";
		urls.push("http://climbing7.wordpress.com/category/canyoning/");
		urls.push("http://climbing7.wordpress.com/category/via-ferrata-2/");

		x = BETAX("<!-- .preview -->", "<!-- #post");
	}

	int DownloadInfo(const char *data, CSym &sym)
	{		
		const char *star = "\xE2\x9C\xAF";

		sym.id = GetToken(sym.id, 0, 0xE2);

		vars desc = sym.GetStr(ITEM_DESC);
		int stars = desc.Replace(star,"");
		double val[] = { 0, 1.5, 3, 4.5, 5 };
		if (stars>3) stars=4;
		sym.SetStr(ITEM_STARS, starstr(val[stars], 1));

		vara list(desc, ";");
		sym.SetStr(ITEM_DESC, list[0]);
		sym.SetStr(ITEM_AKA, list[0]);
		sym.SetStr(ITEM_REGION, list.last());

		list.RemoveAt(0);
		sym.SetStr(ITEM_LAT, "@"+list.join("."));

		if (strstr(curl,"ferrata"))
			{
			sym.SetStr(ITEM_DESC, "Via Ferrata "+sym.GetStr(ITEM_DESC));
			sym.SetStr(ITEM_CLASS, "3:Ferrata");
			}
		return TRUE;
	}

	vars DownloadMore(const char *memory) 
	{
		vars next= ExtractHREF(ExtractString(memory, "class=\"nav-previous\"", "", "</div>"));
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
		urls.push(umain="http://www.nko-extreme.com/category/cronicas/");
		x = BETAX("<h2", "</h2");
	}
	
	vars DownloadMore(const char *memory) 
	{
		double maxp = ExtractNum(f.memory, ">&rsaquo;<", "page/", ">");
		if (maxp<0 || p>maxp)
			return "";
		return umain+MkString("page/%d/",++p);
	}
};


class ACTIONMAN4X4 : public BETAC
{
public:
	ACTIONMAN4X4(const char *base) : BETAC(base)
	{
		utf = TRUE;
		urls.push(umain="http://www.actionman4x4.com/canonesybarrancos/localiza.htm");
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
		urls.push(umain="http://simebuscasestoyconlascabras.blogspot.com");
		x = BETAX("<li", NULL, "<h2>Barrancos", "<h2");
		kfunc = MAP_ExtractKML;
		region = "Spain";
	}

	virtual int DownloadPage(const char *url, CSym &sym)
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
		urls.push(umain="http://barrancosencuenca.es/barrancos.html");
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
		urls.push(umain="http://www.barrancoscanarios.com");
		x = BETAX("<li", NULL, "<strong>Barrancos</strong>", "<strong>");
		//region = "Spain";
		//aregion.push(region);
	}

	int DownloadInfo(const char *data, CSym &sym)
	{

		sym.SetStr(ITEM_REGION, aregion.join(";"));

		if (strstr(data, "</ul") && aregion.length()>0)
			aregion.RemoveAt(aregion.length()-1);

		if (strstr(data, "<ul"))
			{
			aregion.push( sym.GetStr(ITEM_DESC) );
			return FALSE;
			}

		vara aregion(sym.id, "/");
		aregion.splice(0,4);
		if (aregion.length()>0)
			aregion.RemoveAt(aregion.length()-1);
		sym.SetStr(ITEM_REGION, aregion.join(";"));

		if (tregions.GetSize()==0)
			{
			tregions.Load(filename("trans-bcanR"));
			tregions.Sort();
			}

		int f = 0;
		for (int i=0; i<aregion.length(); ++i)
			if ((f=tregions.Find(aregion[i]))>=0)
				aregion[i] = tregions[f].data;
		vars geoloc = invertregion( aregion.join(";"));
		if (!geoloc.IsEmpty())
			sym.SetStr(ITEM_LAT, "@"+geoloc);

		vars name = sym.GetStr(ITEM_DESC);
		sym.SetStr(ITEM_AKA, name.replace(" o ",";").replace(" O ",";"));

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
		x = BETAX("<tr", "</tr", ">nome itinerario", "</table");

	}

	int DownloadInfo(const char *data, CSym &sym)
	{
		const char *str = strstr(data, "href=");
		if (str) str = strstr(str, "<td");
		if (str) str += 2;
		
		CSym condsym;
		if (ExtractLink(str, curl, condsym))
			{
			double vdate = CDATA::GetDate(condsym.data, "DD/MM/YY" );
			if (vdate<=0)
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

	virtual int DownloadPage(const char *url, CSym &sym)
	{
		Throttle(tickscounter, ticks);
		if (f.Download(url))
			{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			return FALSE;
			}
		double lat = ExtractNum(f.memory, "place:location:latitude", "=\"", "\"");
		double lng = ExtractNum(f.memory, "place:location:longitude", "=\"", "\"");
		if (CheckLL(lat,lng))
			sym.SetStr(ITEM_LAT, MkString("@%s;%s", CData(lat), CData(lng)));

		return TRUE;
	}

	vars DownloadMore(const char *memory) 
	{
		vars pagination = ExtractString(memory, "class=\"pagination\"", "", "</ul");
		vars url = burl("www.gulliver.it", ExtractHREF(strstr(pagination, "ref=\"#\"")));
		if (!strstr(url, "?"))
			return "";
		return url;
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

	int DownloadInfo(const char *data, CSym &sym)
	{
		vara td(data, "<td");

		sym.SetStr(ITEM_REGION, "Madeira");
		vars summary = stripHTML(ExtractString(td[2], ">", "", "</td"));
		GetSummary(sym, summary);

		//sym.SetStr(ITEM_LONGEST, stripHTML(ExtractString(td[3], ">", "", "</td")));
		return TRUE;
	}

	virtual int DownloadPage(const char *url, CSym &sym)
	{
		Throttle(tickscounter, ticks);
		if (f.Download(url))
			{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			return FALSE;
			}

		vars coords = stripHTML(ExtractString(f.memory, "<h3>Coordenadas", ">", "<h3"))+":";		
		double lat = ExtractNum(coords, ":", "", ";");
		double lng = ExtractNum(coords, ":", ";", ":");
		if (CheckLL(lat,lng))
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
		for (int i=0; i<topolinks.GetSize(); ++i)
			atopo.push(topolinks[i].id);
		sym.SetStr(ITEM_LINKS, atopo.join(" "));

		// time
		vara times;
		const char *ids[] = { "Aproxima", "Percurso", "Regresso" };
		for (int t=0; t<sizeof(ids)/sizeof(*ids); ++t)
			times.push( noHTML(ExtractString(f.memory, MkString("<td>%s",ids[t]), "</td>", "</td")) );	
		GetTotalTime(sym, times, url, -1);

		return TRUE;
	}

};



// download and convert PDF
int DownloadPDFHTML(const char *url, CString &str)
{
	char buffer[MAX_PATH];
	GetCurrentDirectory(sizeof(buffer), buffer);

	vars path = vars(buffer) + "\\pdf2html\\"; 
	vars pdffile =  path + "pdfhtml.pdf";
	vars htmlfile = path + "pdfhtml.html";
	vars pdf2htmlexe = path + "pdfhtml.vbs"; //"pdf2htmlex\\pdf2htmlex.exe --optimize-text 0 --embed-font 0 --embed-javascript 0 --embed-outline 0 --embed-image 0 --embed-css 0 --process-nontext 0 ";

	DeleteFile(pdffile);
	DeleteFile(htmlfile);

	DownloadFile fb(TRUE);
	if (fb.Download(url, pdffile))
		{
		Log(LOGERR, "Could not download PDF from %s",url);
		return FALSE;
		}

	vars cmd = pdf2htmlexe + " " + pdffile + " " + htmlfile; 
	system(cmd);

	CFILE f;
	if (f.fopen(htmlfile))
		{
		str = "";
		int bread = 0;
		char buff[8*1024];
		while ( bread = f.fread(buff, 1, sizeof(buff)))
			str += CString(buff, bread);
		return TRUE;
		}
	
	return FALSE;
}





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
	if (strstr(x,"°"))
		GetCoords( vars(vars(x).replace(";",".")+" "+vars(y).replace(";",".")).replace(". "," "), lat, lng);
	else
		UTM2LL(zone, vars(x).replace(";","").replace(".",""), vars(y).replace(";","").replace(".",""), lat, lng);
	}


	virtual vars simplify(const char *sname)
	{
		vars name = stripAccents(sname);
		name.Replace("'","");
		name.Replace(".","");
		name.Replace("-","");
		name.Replace("La ","");
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
		if (!DownloadPDFHTML(pdfurl,pdf))
			return FALSE;

		// process table
		double tcheck = 1;
		vara list(pdf, "<tr");
		CSymList tlist;
		for (int i=1;i<list.length(); ++i)
			{
			vara line(list[i], "<td");
			vars tid = stripHTML(ExtractString(line[1], ">", "", "</td"));
			vara tida(tid, "-");
			if (tida.length()!=2 || strlen(tida[0])==0 || CDATA::GetNum(tida[1])<=0)
				continue;

			int col = 2;
			vars name = stripHTML(ExtractString(line[col++], ">", "", "</td")).Trim("; .");
			if (name.IsEmpty())
				continue;

			if (line.length()!=lastcol)
				{
				Log(LOGERR, "ERROR: INDEX PDF Table is wrong size (%d!=%d), skipping %s for %s", line.length(), lastcol, tid, pdfurl);
				continue;
				}

			double tn = CDATA::GetNum(tida[1]);
			if (tn!=1 && (tn!=tcheck && tn!=tcheck-1))
				Log(LOGERR, "INDEX PDF Table Out of Sync %s != %s", tid, CData(tcheck));
			tcheck = tn+1;

			CSym sym(tid, name);

			if (regcol>=0)
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
			if (!CheckLL(lat,lng))
				{
				Log(LOGWARN, "WARNING: Bad startcoord %s zn:%s x:%s y:%s in %s", tid, zone, x, y, pdfurl);
				lat = lat2, lng = lng2;
				}
			if (!CheckLL(lat2,lng2))
				{
				Log(LOGWARN, "WARNING: Bad endcoord %s zn:%s x:%s y:%s in %s", tid, zone, x2, y2, pdfurl);
				lat2 = lat, lng2 = lng;
				}
			if (CheckLL(lat,lng))
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
			sym.SetStr(ITEM_DEPTH, GetMetric(depth.replace(" ","")));
			vars length = stripHTML(ExtractString(line[col++], ">", "", "</td"));
			sym.SetStr(ITEM_LENGTH, GetMetric(length.replace(" ","")));

			ASSERT(sym.GetStr(ITEM_LENGTH)!="1m");

			vara times;
			times.push(stripHTML(ExtractString(line[col+1], ">", "", "</td")));
			times.push(stripHTML(ExtractString(line[col], ">", "", "</td")));
			times.push(stripHTML(ExtractString(line[col+2], ">", "", "</td")));
			col += 3;
			for (int t=0; t<times.length(); ++t)
				{
				times[t] = times[t].replace(" h \xC2½",".5h").replace(" h \xC2¾", ".75h").replace(" h \xC2¼",".25h");
				times[t] = times[t].replace(" h\xC2½",".5h").replace(" h\xC2¾", ".75h").replace(" h\xC2¼",".25h");
				times[t] = times[t].replace("\xC2½ h","0.5h").replace("\xC2¾ h","0.75h").replace("\xC2¼ h","0.25h");
				if (strstr(times[t],"½") || strstr(times[t],"¾") || strstr(times[t],"¼"))
					Log(LOGERR, "ERROR: Invalid time for %s %s : %s", tid, sym.id, times[t]);
				}
			GetTotalTime(sym, times, tid, -1);

			vars shuttle = stripHTML(ExtractString(line[col++], ">", "", "</td"));
			if (CDATA::GetNum(shuttle)==0)
				sym.SetStr(ITEM_SHUTTLE, "No");
			else
				sym.SetStr(ITEM_SHUTTLE, shuttle);

			double num = CDATA::GetNum(stripHTML(ExtractString(line[col++], ">", "", "</td")));
			if (num>=0) sym.SetNum(ITEM_RAPS, num);
		
			vars longest = stripHTML(ExtractString(line[col++], ">", "", "</td"));
			int fx = longest.indexOf("x");
			if (fx>=0) longest = longest.Mid(fx+1);
			sym.SetStr(ITEM_LONGEST, GetMetric(longest));

			sym.SetStr(ITEM_AKA, sym.GetStr(ITEM_DESC));

			Update(tlist, sym, NULL, 0);
			}
#if 1
	// do not match
	for (int i=0; i<tlist.GetSize(); ++i)
		{
		CSym sym( tlist[i].id, tlist[i].data);
		sym.SetStr(ITEM_DESC, mkdesc2(sym.GetStr(ITEM_DESC), sym.id)); 
		sym.id = vars(pdfurl)+"?id="+sym.id;
		Update(symlist, sym, NULL, 0);
		}

#else
	// match
	// patch for duplicates
	for (int i=0; i<tlist.GetSize(); ++i)
		{
		vars newname = tlist[i].GetStr(ITEM_DESC);
		for (int j=i+1; j<tlist.GetSize(); ++j)
			if (tlist[i].GetStr(ITEM_DESC)==tlist[j].GetStr(ITEM_DESC))
				{
				newname = mkdesc2(tlist[i].GetStr(ITEM_DESC), tlist[i].id); 
				tlist[j].SetStr(ITEM_DESC, mkdesc2(tlist[j].GetStr(ITEM_DESC), tlist[j].id)); 
				}
		tlist[i].SetStr(ITEM_DESC, newname);
		}

	vara matched;
	CSymList unmatched;
	for (int i=0; i<tlist.GetSize(); ++i)
		{
		vars tid = tlist[i].id;
		vars name = tlist[i].GetStr(ITEM_DESC);

		// trans
		vars tname = name;
		vars tname2 = mkdesc2(name, tid);

		int ti = trans.Find(name);
		if (ti>=0)
			tname = trans[ti].GetStr(ITEM_DESC);

		int match = -1;
		vars cname = simplify(tname);
		vars cname2 = simplify(tname2);
		if (IsSimilar(tname, "http"))
			{
			for (int s=0; s<symlist.GetSize() && match<0; ++s)
				if (symlist[s].id == tname)
					match = s;
			}
		else
			{
			for (int s=0; s<symlist.GetSize() && match<0; ++s)
				{
				vars sname = simplify(symlist[s].GetStr(ITEM_DESC));
				//sname.Replace("riv ", "riviere ");
				if (strcmp(cname,sname)==0 || strcmp(cname2, sname)==0)
					match = s;
				}
			}

		if (match<0)
			{
			unmatched.Add(tlist[i]);
			continue;
			}


		CSym &sym = symlist[match];
		//ASSERT(!strstr(sym.data, "Teruvea"));
		if (matched.indexOf(sym.id)>=0)
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
	for (int i=0; i<symlist.GetSize(); ++i)
		if (matched.indexOf(symlist[i].id)<0)
			unmatchedsymlist.push(symlist[i].GetStr(ITEM_DESC));

	// match summary
	if (unmatchedsymlist.length()>0)
		{
		Log(LOGERR, "UNMATCHED SYMLIST: %d/%d (see log, ed beta\trans-*code*.csv) %s", unmatchedsymlist.length(), symlist.GetSize(), pdfurl);
		Log(","+unmatchedsymlist.join("\n,"));
		Log(LOGERR, "UNMATCHED INDEX PDF: %d/%d (see log, ed beta\trans-*code*.csv)", unmatched.GetSize(), tlist.GetSize()); 
		if (unmatchedsymlist.length()>0) 
			{
			vara unmatchedstr;
			for (int i=0; i<unmatched.GetSize(); ++i)
				unmatchedstr.push(unmatched[i].GetStr(ITEM_DESC)+","+unmatched[i].id);
			Log(unmatchedstr.join("\n"));
			}
		}
	else
		{
		// Add unmatched
		for (int i=0; i<unmatched.GetSize(); ++i)
			{
			vars id = urlstr(pdfurl)+"?id="+unmatched[i].id;
			vars desc = mkdesc2( unmatched[i].GetStr(ITEM_DESC), unmatched[i].id);
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

		for (int u=0; urls[u]!=NULL; ++u)
		{
			CSymList symlist;

			// get page list
			x = BETAX("<a", NULL, ">Inventaire<", "<style");
			vara urla(urls[u], "/");
			urla.SetSize(urla.length()-1);
			ubase = umain = urla.join("/");
			DownloadUrl(urls[u], symlist, x );
			for (int i=0; i<symlist.GetSize(); ++i)
				symlist[i].id = GetToken(symlist[i].id, 0, '#');


			// get table coords
			vars invurl = ::ExtractLink(f.memory,">Inventaire<");
			vars pdfpageurl = makeurl(ubase, invurl);
			if (f.Download(pdfpageurl))
				{
				Log(LOGERR, "ERROR: Could not download Inventaire page %s", invurl);
				continue;
				}

			vars pdffile = ::ExtractLink(f.memory,".pdf");
			if (!DownloadIndex(symlist, makeurl(ubase, pdffile), "20P", -1, 3, 18))
				Log(LOGERR, "ERROR: Could not find/download INDEX PDF for MADININA");

			for (int i=0; i<symlist.GetSize(); ++i)
				Update(gsymlist, symlist[i], 0, 0);
		}

		return gsymlist.GetSize()>0;
	}
};


class TAHITI : public MADININA
{
public:

	TAHITI(const char *base) : MADININA(base)
	{
		kfunc = SELL_ExtractKML;
	}

	virtual int DownloadPage(const char *url, CSym &sym)
	{
		Throttle(tickscounter, ticks);
		if (f.Download(url))
			{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			vars name = GetFileName(vars(sym.id).replace("/", "\\").replace("%20", " "));
			sym.SetStr(ITEM_DESC, name);
			return TRUE;
			}

		vars name = stripHTML(ExtractString(f.memory, "class=\"ContentBox\"", ">", "</span"));
		if (name.IsEmpty())
			{
			Log(LOGWARN, "WARNING: using title for name of url %.128s", url);
			name = stripHTML(ExtractString(f.memory, "<title", ">", "</title"));
			}
		sym.SetStr(ITEM_DESC, name);
		return TRUE;
	}

	virtual int DownloadInfo(const char *data, CSym &sym)
	{
		//name.Replace("riv ", "Riviere ");
		if(!strstr(sym.id,"/Topo/"))
			return FALSE;
		return TRUE;
	}

	virtual void DownloadPatch(vars &memory)
	{
		memory.Replace("href=\"#\"", "");
	}


	virtual int DownloadBeta(CSymList &symlist) 
	{

		// get page list
		CSymList clist;
		BETAX xmain("<div", "</div", ">Liste canyons par commune<", ">Les Lavatubes<");
		DownloadUrl(umain="http://canyon-a-tahiti.shost.ca/", clist, xmain, 0 );
		vars memory = f.memory;
		vars base;
		x = BETAX("<a");
		for (int i=0; i<clist.GetSize(); ++i)
			{	
			vara urla(clist[i].id, "/");
			urla.SetSize(urla.length()-1);
			ubase = base = urla.join("/");
			DownloadUrl(clist[i].id, symlist, x );
			}

		// get table coords
		trans.Load(filename("trans-tahiti"));
		trans.Sort();

		DownloadPatch(memory);
		//vars pdffile = ExtractHREF(ExtractString(memory,">Inventaire<", "", ">Liste canyons par commune<"));
		vars pdffile = ::ExtractLink(memory, "ContentBox\">Inventaire");
		if (pdffile.IsEmpty() || !DownloadIndex(symlist, makeurl(umain, pdffile), "6K", 3, 4, 19))
			Log(LOGERR, "ERROR: Could not find/download INDEX PDF for TAHITI");

		return symlist.GetSize()>0;
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
		x = BETAX("<tr", "</tr", "</th","</table");
	}

	virtual int DownloadInfo(const char *data, CSym &sym)
	{
		sym.id.Replace("//murdeau","//WWW.murdeau");
		if(strstr(sym.id,"/dominique/"))
			sym.SetStr(ITEM_REGION, "Dominica");
		if(strstr(sym.id,"/martinique/"))
			sym.SetStr(ITEM_REGION, "Martinique");
		return TRUE;
	}

	virtual int DownloadPage(const char *url, CSym &sym)
	{
		Throttle(tickscounter, ticks);
		if (f.Download(url))
			{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			return TRUE;
			}

		vars cot = stripHTML(ExtractString(f.memory, ">Cotation", ":", "</td"));
		vara sum( GetToken(cot,0,'-'), ".");
		if (sum.length()>=3)
			GetSummary(sym, "v"+sum[0]+"a"+sum[1]+sum[2]);

		int stars = cot.Replace("*", "x");
		sym.SetStr(ITEM_STARS, starstr(stars+1,1));

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
		urls.push(umain="http://alpes-guide.com/sources/topo/index.asp?pmotcle%22=&psecteur=&pacti=1%2C14");
		x = BETAX("class=\'liste\'", "En savoir plus...");
	}

	int DownloadInfo(const char *data, CSym &sym)
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

	virtual int DownloadLink(const char *data, CSym &sym)
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
		sym = CSym( urlstr(link), name );
		return TRUE;
	}

	virtual vars DownloadMore(const char *memory) 
	{
		vars page = MkString("&ppage=%d", ++p);
		if (!strstr(f.memory, page))
			return "";
		return "http://alpes-guide.com/sources/topo/index.asp?pacti=1%2C14&psecteur=&pmotcle="+page;
	}
	
};



class OPENSPELEO : public BETAC
{
public:

	int p;
	OPENSPELEO(const char *base) : BETAC(base)
	{
		p = 1;
		urls.push(umain="http://www.openspeleo.org/openspeleo/canyons.html");
		x = BETAX("class=\"searchresult2\"");
	}

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

		SWISSCANYON_GetSummary(sym, stripHTML(ExtractString(f.memory, ">Difficolt&agrave;<", ">", "</span>")));

		sym.SetStr(ITEM_SEASON, stripHTML(ExtractString(f.memory, ">Periodo dell'anno<", ">", "</span>")));
		sym.SetStr(ITEM_SHUTTLE, vars(stripHTML(ExtractString(f.memory, ">Navetta<", ">", "</span>"))).replace("si","Yes").replace("Si","Yes").replace("Nessun","No").replace("circa",""));

		sym.SetStr(ITEM_AKA, stripHTML(ExtractString(f.memory, ">Sinonimi<", ">", "</span>")));
		
		/*
		double lat = ExtractNum(f.memory, "place:location:latitude", "=\"", "\"");
		double lng = ExtractNum(f.memory, "place:location:longitude", "=\"", "\"");
		if (CheckLL(lat,lng))
			sym.SetStr(ITEM_LAT, MkString("@%s;%s", CData(lat), CData(lng)));
		*/

		return TRUE;
	}

	virtual vars DownloadMore(const char *memory) 
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

	virtual int DownloadPage(const char *url, CSym &sym)
	{
		CSymList links;
		DownloadUrl(url, links, BETAX("<h2"), D_LINK);

		vara sketch;
		for (int i=0; i<links.GetSize(); ++i)
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

	int DownloadBeta(CSymList &symlist) 
	{	
		return DownloadHTML("beta\\ecdc.html", symlist, x );
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


	int DownloadPage(const char *url, CSym &sym)
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
		if (!_GeoCache.Get(oaddress = address = city+";"+GetToken(region,0,",;-")+";Italy", lat, lng))
			{
			while (!ExtractStringDel(address,"(", "", ")").IsEmpty());
			if (!_GeoCache.Get(address, lat, lng))
			  if (!_GeoCache.Get(address = city+";"+region+";Italy", lat, lng))
				if (!_GeoCache.Get(address = city+";Italy", lat, lng))
					if (!_GeoCache.Get(address = city+";"+region+";Italy", lat, lng))
						address = address;
			}

		//if (CheckLL(lat,lng))
		//	if (address!=oaddress)
		//		address=MkString("%s;%s", CData(lat), CData(lng));
		sym.SetStr(ITEM_LAT, "@"+address);

		sym.SetStr(ITEM_AKA, sym.GetStr(ITEM_DESC));

		sym.SetStr(ITEM_REGION, "Italy;"+region);
		sym.SetStr(ITEM_DEPTH, GetMetric(depth));
		sym.SetStr(ITEM_LENGTH, GetMetric(length));
		sym.SetStr(ITEM_LONGEST, GetMetric(longest));
		sym.SetStr(ITEM_ROCK, rock);
		sym.SetStr(ITEM_SHUTTLE, shuttle.replace(" ","").replace("Circa","").replace("Possibile","Optional ").replace("Si","Yes").replace("---","").replace(",",".").replace("Indispensabile","Yes").replace("Consigliabile","Optional").replace("Consigl","Optional").replace("Necessaria","Yes")
			.replace("Necessaria","Yes").replace("Decisa","Yes").replace("Forte","Yes").replace("Poco","No").replace("Raccom","Yes").replace("Sconsig","No").replace("Senepu","No"));
		return TRUE;
	}

	vara done;
	int DownloadRegion(const char *url, CSymList &symlist)
		{
			CSymList urllist;
			DownloadUrl(url, urllist, x, D_LINK);

			for (int i=0; i<urllist.GetSize(); ++i)
				{
				vars url = urllist[i].id;
				if (done.indexOf(url)>=0)
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


	int DownloadBeta(CSymList &symlist)
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
		urls.push( "http://www.icopro.org/canyon/map-data/p/1" );
		x = BETAX("content");
		umain = "http://www.icopro.org/canyon";
	}

	
	void DownloadPatch(vars &memory)
	{		
		memory.Replace("\\/", "/");
		memory.Replace("\\\"", "\"");		
		memory.Replace("\\\'", "\'");

		int find;
		vars unicode = "\\u00";
		while ( (find= memory.Find(unicode))>=0)
			{
			vars code = memory.Mid(find+unicode.GetLength(), 2);
			vars ostr = unicode + code;
			unsigned char nstr[8] = "X";
			FromHex(code, nstr);
			memory.Replace(ostr, CString(nstr));
			}

		memory = UTF8(memory);
	}

	int DownloadInfo(const char *data, CSym &sym)
	{
		sym.data = sym.data;
		vars summary = ExtractString(strstr(data, "<dt>Rate"), "<dd", ">", "</dd");
		vars region = stripHTML(ExtractString(strstr(data, "<dt>Country"), "<dd", ">", "</dd"));
		vars geoloc = stripHTML(ExtractString(strstr(data, "<dt>Location"), "<dd", ">", "</dd")) + "." + region;
		double lat, lng;
		lat = ExtractNum(data, "\"lat\"", "\"", "\"");
		lng = ExtractNum(data, "\"lng\"", "\"", "\"");
		if (CheckLL(lat,lng) && Distance(lat, lng, -8.3183, 114.8607)>100)
			{
			//sym.SetNum(ITEM_LAT, lat);
			//sym.SetNum(ITEM_LNG, lng);
			sym.SetStr(ITEM_LAT, MkString("@%s;%s", CData(lat), CData(lng)));
			}
		else
		if (!geoloc.IsEmpty())
			{
			sym.SetStr(ITEM_LAT, "@"+geoloc);
			}

		sym.SetStr(ITEM_REGION, region);
		GetSummary(sym, summary);
		return TRUE;
	}

	vars DownloadMore(const char *memory) 
	{
		vars last = ExtractString(memory, "\"isLast\"", ":", ",");
		if (IsSimilar(last, "true"))
			return "";
		return MkString("http://www.icopro.org/canyon/map-data/p/%d",++page);
	}

};


// ===============================================================================================

//static double altisudticks;

int ALTOPIRINEO_DownloadPage(DownloadFile &f, const char *url, CSym &sym)
{
	if (f.Download(url))
		{
		Log(LOGERR, "Can't download base url %s", url);
		return FALSE;
		}
	vars memory = stripAccentsL(f.memory);
	vars season = stripHTML(ExtractString(memory, ">epoca", ":", "</li"));
	sym.SetStr(ITEM_SEASON, season);
	vars longest = stripHTML(ExtractString(memory, ">altura", ":", "</li"));
	longest.Replace("metros", "meters");
	sym.SetStr(ITEM_LONGEST, longest.Trim(" ."));
	vars rapnum = stripHTML(ExtractString(memory, ">numero", ":", "</li"));
	sym.SetNum(ITEM_RAPS, CDATA::GetNum(rapnum));

	double paisaje = CDATA::GetNum(ExtractString(memory, ">paisaje", "alt=\"", "\""));
	double ludico = CDATA::GetNum(ExtractString(memory, ">caracter", "alt=\"", "\""));
	double stars = 0;
	if (paisaje>0) stars = paisaje;
	if (ludico>paisaje) stars = ludico;
	if (stars>0)
		sym.SetStr(ITEM_STARS, starstr(stars+1,1));

	vara times;
	const char *timelab[] = { ">aproximacion", ">descenso", ">retorno", NULL };
	for (int i=0; timelab[i]!=NULL; ++i)
		{
		vars tstr = stripHTML(ExtractString(memory, timelab[i], ":", "</li"));
		tstr.Replace("horas", "h");
		tstr.Replace("hora", "h");
		tstr.Replace("minutos", "m");
		tstr.Replace("minuto", "m");
		tstr.Replace("&#8242", "m");
		tstr.Replace(" y ", "");
		tstr.Replace("unas", "");
		tstr.Replace("inmediato", "5min");
		tstr.Replace("inmediata", "5min");
		tstr.Trim(" .");
		times.push(tstr);
		}
	GetTotalTime(sym, times, url);

	return TRUE;
}

int ALTOPIRINEO_DownloadBeta(const char *ubase, CSymList &symlist) 
{
	vara regions;
	DownloadFile f;

	vara urllist("http://altopirineo.com/category/barrancos-pirineos/,http://altopirineo.com/category/barrancos-sierra-de-guara/");

	vars url = "http://altopirineo.com/alto-pirineo/la-base/";
	if (f.Download(url))
		{
		Log(LOGERR, "Can't download base url %s", url);
		return FALSE;
		}
	vara ulist(ExtractString(f.memory, ">Archivos", "<ul", "</ul"), "href=");
	for (int u=1; u<ulist.length(); ++u)
		{
		vars link = ExtractString(ulist[u], "", "", ">").Trim(" '\"");
		if (!link.IsEmpty())
			urllist.push(link);
		}


	for (int u=0; u<urllist.length(); ++u)
	{
	// download base
	if (f.Download(url = urllist[u]))
		{
		Log(LOGERR, "Can't download base url %s", url);
		return FALSE;
		}
	vara list(f.memory, "<h4");
	for (int i=1; i<list.length(); ++i)
		{	
		vars link = ExtractString(list[i], "href=", "\"", "\"");
		vara regionname(stripHTML(ExtractString(list[i], "", ">", "</h4")), ".");
		vars name = regionname[0];
		vars region = regionname.length()>1 ? regionname[1] : "";
		if (link.IsEmpty() || region.IsEmpty())
			continue;

		// download fiche
		CSym sym(urlstr(link), name);
		sym.SetStr(ITEM_REGION, "Spain;"+region);
		printf("%d %d/%d %d/%d  \r", symlist.GetSize(), u, urllist.length(), i, list.length());
		if (!UpdateCheck(symlist, sym) && MODE>-2)
			continue;
		if (ALTOPIRINEO_DownloadPage(f, link, sym))
			Update(symlist, sym, NULL, FALSE);
		}
	}

	return TRUE;
}

// ===============================================================================================
class OREGONCAVES4U : public BETAC
{
public:

	OREGONCAVES4U(const char *base) : BETAC(base)
	{
		ubase = base;
		umain = "http://www.oregoncaves4u.com";
	}

	virtual int DownloadBeta(CSymList &symlist) 
	{
	vara regions;
	DownloadFile f;

	// download base
	CString url = umain;
	if (f.Download(url))
		{
		Log(LOGERR, "Can't download OregonCaves4U base url %s", url);
		return FALSE;
		}
	vara headers;
	vara list(f.memory, "<tr");
	for (int i=1; i<list.length(); ++i)
		{
		vara vals;
		vara line = list[i].split("<td");
		for (int l=1; l<line.length(); ++l)
			{
			CString val;
			GetSearchString(line[l], "", val, ">", "</td");
			vals.push(val);
			}

		if (vals.length()<1)
			continue;

		// get url
		CString nurl;
		GetSearchString(vals[0], "href=", nurl, "\"", "\"");
		if (nurl.IsEmpty())
			{
			Log(LOGERR, "Can't find OregonCaves4U url in %s", vals[0]);
			continue;
			}
		if (strnicmp(nurl, "http", 4)!=0)
			{
			CString lurl = nurl;
			nurl = url + lurl;
			vals[0].Replace(lurl, nurl);
			}

		// get coordinates
		if (f.Download(nurl))
			{
			Log(LOGERR, "Can't download OregonCaves4U from %s", nurl);
			continue;
			}

		CString coords;
		const char *sep = ", \t\a\n";
		GetSearchString(f.memory, "<caption", coords, ":", "</caption");
		coords.Trim(sep);
		double lat = CGetNum(GetToken(coords, 0, sep));
		double lng = CGetNum(GetToken(coords, 1, sep));
		
		if (lat==InvalidNUM || lng==InvalidNUM)
			{
			Log(LOGERR, "Invalid coord for OregonCaves4U url %s", nurl); 
			continue;
			}

		// add to list
		if (lng>0) lng = -lng;

		CSym sym(urlstr(nurl), stripHTML(vals[0]));
		sym.SetNum(ITEM_LAT, lat);
		sym.SetNum(ITEM_LNG, lng);
		sym.SetStr(ITEM_CLASS, "2:Cave");
		Update(symlist, sym, NULL, FALSE);
		}

	return TRUE;
	}

};

// ===============================================================================================

int ZIONCANYON_DownloadBeta(const char *ubase, CSymList &symlist) 
{
	vara regions;
	DownloadFile f;
	//const char *region = { "Arizona", "Utah", NULL };

	for (int n=1; n<=2; ++n)
	{
	// download base
	CString url = MkString("http://zioncanyoneering.com/api/CanyonAPI/GetCanyonsByState?stateId=%d", n);
	if (f.Download(url))
		{
		Log(LOGERR, "Can't download base url %s", url);
		continue;
		}
	vara list(f.memory, "\"Name\":");
	for (int i=1; i<list.length(); ++i)
		{
		const char *memory = list[i];
		// add to list
		vars name = ExtractString(memory, "", "\"", "\"");				
		vars state = ExtractString(memory, "\"State\":", "\"", "\"");		
		vars nurl = "http://zioncanyoneering.com/Canyons/State/" + url_encode(state) +"/" + url_encode(ExtractString(memory, "\"URLName\":", "\"", "\""));

		CSym sym(urlstr(nurl), stripHTML(name));
		if (nurl.IsEmpty() || name.IsEmpty())
			continue;

		double ratings = ExtractNum(memory, "\"NumberOfVoters\":", "", ",");
		double stars = ExtractNum(memory, "\"PublicRating\":", "", ",");
		if (stars!=InvalidNUM && ratings!=InvalidNUM)
			sym.SetStr(ITEM_STARS, starstr(stars/ratings, ratings));
		vars region = ExtractString(memory, "\"Region\"", "\"", "\"");		
		sym.SetStr(ITEM_REGION, state);

		double t = ExtractNum(memory, "\"CanyonRating\":", "", ",");		
		vars w = ExtractString(memory, "\"WaterVolumeValue\":", "\"", "\"");
		vars m = ExtractString(memory, "\"TimeCommitmentValue\":", "\"", "\"");		
		sym.SetStr(ITEM_ACA, CData(t)+";"+w+";"+m);

		Update(symlist, sym, NULL, FALSE);
		}
	}
	return TRUE;
}

// ===============================================================================================

/*
South 
http://www.kiwicanyons.org/?geo_mashup_content=render-map&map_content=global&width=650&height=500&zoom=6&background_color=c0c0c0&show_future=false&marker_select_info_window=true&marker_select_center=true&marker_select_highlight=false&marker_select_attachments=false&center_lat=-43.612217&center_lng=170.727539&auto_info_open=false
http://www.kiwicanyons.org/?geo_mashup_content=render-map&map_content=global&width=650&height=500&zoom=5&background_color=c0c0c0&show_future=false&marker_select_info_window=true&marker_select_center=true&marker_select_highlight=false&marker_select_attachments=false&center_lat=-41.442726&center_lng=173.496094&auto_info_open=false&marker_min_zoom=8

http://www.kiwicanyons.org/?geo_mashup_content=geo-query&object_name=post&object_ids=867&template=info-window-max
http://www.kiwicanyons.org/?geo_mashup_content=geo-query&object_name=post&object_ids=867  ->
http://www.kiwicanyons.org/?geo_mashup_content=geo-query&object_name=post
http://www.kiwicanyons.org/?p=558
*/

int KIWICANYONS_DownloadBeta(const char *ubase, CSymList &symlist) 
{
	//vara recentlist, urllist;
	//const char *canyonlink = "href=\"/canyoning/canyon/";
	//const char *canyondesclink = "/canyoning/canyon-description/";

	DownloadFile f;
	CString kc = "http://www.kiwicanyons.org";
	CString infourl = "http://www.kiwicanyons.org/?p=";
	CString nz = "New Zealand";

	// download map
	CString url = kc+"/?geo_mashup_content=geo-query&object_name=post";
	if (f.Download(url))
			{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			return FALSE;
			}
	vara list(f.memory, "{\"object_name\"");
	for (int i=1; i<list.length(); ++i) {
		vars id = ExtractString(list[i], "\"object_id\"");
		if (id.IsEmpty()) {
			Log(LOGERR, "Invalid id from %s", list[i]);
			continue;
			}
		double lat = ExtractNum(list[i], "\"lat\"");
		double lng = ExtractNum(list[i], "\"lng\"");
		if (!CheckLL(lat,lng)) {
			Log(LOGERR, "Invalid lat/lng for %s from %s", list[i], url);
			continue;
			}
		// split title in 4
		vars title = ExtractString(list[i], "\"title\"");
		vars name, rating, stars;
		vars *dst = &name;
		for (int t=0; title[t]!=0; ++t) {
			if (title[t]=='v' && isdigit(title[t+1]))
				dst = &rating;
			if (title[t]=='*')
				stars += title[t];
			*dst += title[t];
			}
		name.Trim(); rating.Trim(); stars.Trim();
		CSym sym(urlstr(infourl+id));
		sym.SetStr(ITEM_DESC, GetToken(name.replace(",",";"), 0, '(').Trim());
		sym.SetNum(ITEM_LAT, lat);
		sym.SetNum(ITEM_LNG, lng);
		GetSummary(sym, rating);
		//sym.SetNum(ITEM_LNG, lng);
		vara cat(ExtractString(list[i], "\"categories\"", "[", "]"));
		//"16":{"name":"0 Star Canyons","parent_id":"15","color_name":"black"}, -> 2
		//"15":{"name":"1 Star Canyons","parent_id":"14","color_name":"yellow"}, -> 3
		//"14":{"name":"2 Star Canyons","parent_id":"9","color_name":"orange"}, -> 4
		//"9":{"name":"3 Star Canyons","parent_id":"","color_name":"red"}, -> 5
		//"7":{"name":"Beta-less Canyons","parent_id":"16","color_name":"purple"}, -> 0
		//"17":{"name":"Potential Canyons","parent_id":"7","color_name":"blue"},
		//"63":{"name":"Trip Reports","parent_id":"","color_name":"aqua"},
		//"1":{"name":"Uncategorized","parent_id":"","color_name":"blue"}} });	
		int starv = -1;
		if (cat.indexOf("\"7\"")>=0) starv = 0;
		if (cat.indexOf("\"16\"")>=0) starv = 2;
		if (cat.indexOf("\"15\"")>=0) starv = 3;
		if (cat.indexOf("\"14\"")>=0) starv = 4;
		if (cat.indexOf("\"9\"")>=0) starv = 5;
		if (strlen(stars)>0) {
			int nstarv = strlen(stars)+2;
			if (nstarv!=starv)
				Log(LOGWARN, "Inconsistent stars for %s [%s] => Max(%d,%d) ", title, cat.join(), nstarv, starv);
			if (nstarv>starv)
				starv = nstarv;
			if (starv>5) {
				Log(LOGWARN, "Inconsistent stars>5 for %s [%s]", title, cat.join());
				starv = 5;
			}
		}
		sym.SetStr(ITEM_REGION, nz);
		if (starv<0) 
			sym.SetNum(ITEM_CLASS, -1); // unclassified
		else
			sym.SetStr(ITEM_STARS, MkString("%s*1", CData(starv)));
		Update(symlist, sym, NULL, FALSE);
		}

	// download list
	//CString url = kc + "/?geo_mashup_content=geo-query&object_name=post";
	url = kc;
	if (f.Download(url))
		{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
		}

	const char *region[2] = { "South Island", "North Island" };
	const char *sep[2] = { "http://www.kiwicanyons.org/legend/test/south-island/", "http://www.kiwicanyons.org/legend/test/north-island/" };
	for (int n=0; n<2; ++n) 
		{
		vara list(f.memory, sep[n]);
		for (int i=1; i<list.length(); ++i) 
			{
			vars subregion = ExtractString(list[i], "", ">", "<");
			vars urlex = ExtractString(list[i], "", "", "\"");
			if (urlex.IsEmpty())
				continue;

			DownloadFile f;
			vars urlr = sep[n]+urlex;
			if (f.Download(urlr))
					{
					Log(LOGERR, "ERROR: can't download url %.128s", urlr);
					return FALSE;
					}
			char *table = strstr(f.memory, "<table");
			if (!table) {
				Log(LOGERR, "ERROR: No table detected from %.128s", urlr);
				continue;
				}
			vara list(table, infourl);
			for (int i=1; i<list.length(); ++i) 
				{
				vars id = ExtractString(list[i], "", "", "\"");
				if (id.IsEmpty()) 
					{
					Log(LOGERR, "Invalid id from %s", list[i]);
					continue;
					}
				vara td(list[i], "<td");
				vars name = ExtractString(td[0], "", ">", "<");
				vars rating = ExtractString(td[1], "", ">", "<");
				double stars = ExtractNum(td[2], "", ">", "<");

				CSym sym(urlstr(infourl+id));
				if (!name.IsEmpty())
					sym.SetStr(ITEM_DESC, name.replace(",",";"));
				GetSummary(sym, rating);
				if (stars>=0) {
					stars += 2;
					if (stars>5) {
						Log(LOGWARN, "Inconsistent stars>5 for %s [%s]", CData(stars), sym.data);
						stars = 5;
					}

					sym.SetStr(ITEM_CLASS, "1");
					sym.SetStr(ITEM_STARS, MkString("%s*1", CData(stars)));
				}
				
				//time & longest
				CDoubleArrayList longest, time;
				vars stime = ExtractString(td[3], "", ">", "<").replace("m", " m").replace("h", " h");;
				if (GetValues(stime, utime, time)) 
					{
					sym.SetNum(ITEM_MINTIME, time.Head());
					sym.SetNum(ITEM_MAXTIME, time.Tail());
					}
				vars slongest = ExtractString(td[4], "", ">", "<").replace("m", " m");;
				if (GetValues(slongest, ulen, longest))
					sym.SetNum(ITEM_LONGEST, longest.Tail());

				sym.SetStr(ITEM_REGION, nz+";"+region[n]+";"+subregion);

				Update(symlist, sym, NULL, FALSE);
				}
			}
		}
	return TRUE;
}

// ===============================================================================================

#define WIKILOCTICKS 1000
	

int WIKILOC_ExtractKML(const char *ubase, const char *url, inetdata *out, int fx)
{
	DownloadFile f;
	static double wikilocticks;
	Throttle(canyoncartoticks, WIKILOCTICKS);
	if (DownloadRetry(f, url))
		{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
		}

	vars user = stripHTML(ExtractString(f.memory, ">Author:", "", "</div"));
	CString credit = " (Data by " + user + " at " + CString(ubase) + ")";
	vars desc = stripHTML(ExtractString(f.memory, "<h1", ">", "<a")) + " " + credit;

	vara styles, points, lines;
	styles.push("dot=http://maps.google.com/mapfiles/kml/shapes/open-diamond.png");

	vara wpt(f.memory, "maps.LatLng(");
	for (int i=2; i<wpt.length(); ++i)
		{
			vara coords(ExtractString(wpt[i], "", "", ");"));
			vara attr(ExtractString(wpt[i], "buildAndCache(", "", ");"));
			if (coords.length()<2 || attr.length()<6)
				continue;
			vars id = stripHTML(attr[5].Trim("'\""));
			double lat = CDATA::GetNum(coords[0]);
			double lng = CDATA::GetNum(coords[1]);
			if (id.IsEmpty() || !CheckLL(lat, lng, url)) 
				continue;

		// add markers
		points.push( KMLMarker("dot", lat, lng, id, credit ) );
		}

	// process lines
	vara linelist;
	vara lats(ExtractString(f.memory, "lat:", "[", "]"));
	vara lngs(ExtractString(f.memory, "lng:", "[", "]"));
	for (int i=0; i<lats.length() && i<lngs.length(); ++i)
		{
		double lat = CDATA::GetNum(lats[i]);
		double lng = CDATA::GetNum(lngs[i]);
		linelist.push(CCoord3(lat, lng));
		}
	lines.push( KMLLine("Track", desc, linelist, OTHER, 3) );

	if (points.length()==0 && lines.length()==0)
		return FALSE;

	// generate kml
	SaveKML("WikiLoc", desc, url, styles, points, lines, out);
	return TRUE;
}


int WIKILOC_DownloadPage(DownloadFile &f, const char *url, CSym &sym)
{
	if (strstr(url,"imgServer"))
		{
		Log(LOGERR, "ERROR wikiloc.com ImgServer reference: %s", url);
		return FALSE;
		}
	if (f.Download(url))
			{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			return FALSE;
			}
	
	vara coords(ExtractString(f.memory, "maps.LatLng(","",")"));
	if (coords.length()>1)
		{
		sym.SetStr(ITEM_LAT, coords[0]);
		sym.SetStr(ITEM_LNG, coords[1]);
		}
/*
	vara rating(ExtractString(f.memory, "title=\"Rated:","","\""), "(");
	double stars = 0, num = 0;
	if (rating.length()>1)
		{
		stars = CDATA::GetNum(rating[0]);
		num = CDATA::GetNum(rating[1]);
		sym.SetStr(ITEM_STARS, starstr(stars, num));
		}
*/
	sym.SetStr(ITEM_KML, "X");

	vara wpt(f.memory, "maps.LatLng(");
	vars description = stripHTML(ExtractString(f.memory, "class=\"description\"",">","</div"));
	description.Replace(sym.GetStr(ITEM_DESC),"");
	int n = wpt.length()-2;
	int len = description.length();
	int score = n*100 + len;
	vars desc = MkString("%05d:%db+%dwpt", score, len, n);
	if (score<300) sym.SetStr(ITEM_CLASS, "0:"+desc);
	else sym.SetStr(ITEM_CLASS, "1:"+desc);

	return TRUE;
}

/*
int WIKI_DownloadBeta(const char *ubase, CSymList &symlist, int (*DownloadPage)(DownloadFile &f, const char *url, CSym &sym)) 
{
	int n = 0, step = 20;
	DownloadFile f;
	int from=0;
	vars url = "http://www.wikiloc.com/trails/canyoneering";
	while (!url.IsEmpty())
	{
		if (f.Download(url))
			{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			return FALSE;
			}
		url = ExtractLink(f.memory, ">Next&nbsp;&rarr;");
		double count = ExtractNum(f.memory, "&nbsp;of&nbsp;", "", "&nbsp;");
		if (count<=0 || count>10000)
			{
			Log(LOGERR, "Bad count %s!", CData(count));
			return FALSE;
			}

		vara list(f.memory, "wikiloc/view.do?id=");
		for (int i=1; i<list.length(); ++i)
			{
			vars id = ExtractString(list[i], "", "", "\"").Trim(" \"'");
			if (id.IsEmpty())
				continue;

			vars name = stripHTML(ExtractString(list[i], ">", "", "<"));
			vars loc = stripHTML(ExtractString(list[i], "\"near\"", "\"", "\""));
			double lat = ExtractNum(list[i], "\"lat\"", ":", ",");
			double lng = ExtractNum(list[i], "\"lon\"", ":", ",");
			if (!CheckLL(lat,lng))
				{
				Log(LOGERR, "Invalid WIKILOC coords for %s", list[i]);
				continue;
				}

			vars link = "http://wikiloc.com/wikiloc/view.do?id="+id;

			++n;
			CSym sym(urlstr(link), name);
			vars country = stripHTML(ExtractString(loc, "(", "", ")"));
			vars region = invertregion( GetToken(loc, 0, '('), country + " @ ");
			sym.SetNum(ITEM_LAT, lat);
			sym.SetNum(ITEM_LNG, lng);
			sym.SetStr(ITEM_REGION, region);
			sym.SetStr(ITEM_KML, "X");
			vara rated(name,"*");
			double stars = rated[0][strlen(rated[0])-1]-(double)'0';
			if (stars>=1 && stars<=5)
				sym.SetStr(ITEM_STARS, starstr(stars, 1));

			printf("%d %d/%d %d/%d/%d    \r", symlist.GetSize(), i, list.length(), from, n, (int)count);
			if (!UpdateCheck(symlist, sym) && MODE>-2)
				continue;

			if (DownloadPage(f, sym.id, sym))
				Update(symlist, sym, NULL, FALSE);
			}

		// next
		from += step;
		if (from>count)
			break;
	}

	return TRUE;
}

*/

int WIKI_DownloadBeta(const char *ubase, CSymList &symlist, int (*DownloadPage)(DownloadFile &f, const char *url, CSym &sym)) 
{
	int n = 0, step = 20;
	DownloadFile f;
	if (f.Download("http://www.wikiloc.com/wikiloc/start.do"))
		{
		Log(LOGERR, "ERROR: can't login in wikiloc");
		return FALSE;
		}
	f.GetForm();
	f.SetFormValue("email", WIKILOC_EMAIL);
	f.SetFormValue("password", WIKILOC_PASSWORD);
	if (f.SubmitForm())
		{
		Log(LOGERR, "ERROR: can't login in wikiloc");
		return FALSE;
		}

	int from=0;
	while (true)
	{
		//vars url = "https://www.wikiloc.com/wikiloc/find.do?act=46%2C&uom=mi&s=id"+MkString("&from=%d&to=%d", from, from+step); 
		vars url = "https://www.wikiloc.com/wikiloc/find.do?a=46&z=2&sw=-90%2C-180&ne=90%2C180"+MkString("&from=%d&to=%d", from, from+step); 
		//vars url = https://www.wikiloc.com/wikiloc/find.do?a=46&z=2&from=0&to=15&sw=-90%2C-180&ne=90%2C180
		if (f.Download(url)) // sw=-90%%2C-180&ne=90%%2C180&tr=0 https://www.wikiloc.com/wikiloc/find.do?act=46%2C&uom=mi&from=10&to=20
			{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			return FALSE;
			}

		//double count = ExtractNum(f.memory, "&nbsp;of&nbsp;", "", "</a");
		double count = ExtractNum(f.memory, "count", ":", ",");
		if (count<=0 || count>10000)
			{
			Log(LOGERR, "Count too big %s>10K", CData(count));
			return FALSE;
			}

		vara mem(f.memory,"\"tiles\"");
		vara list(mem.length()>0 ? mem[0] : f.memory, "{\"id\"");
		for (int i=1; i<list.length(); ++i)
			{
			vars id = ExtractString(list[i], "", ":", ",").Trim(" \"'");
			if (id.IsEmpty())
				continue;

			vars name = stripHTML(ExtractString(list[i], "\"name\"", "\"", "\""));
			vars loc = stripHTML(ExtractString(list[i], "\"near\"", "\"", "\""));
			double lat = ExtractNum(list[i], "\"lat\"", ":", ",");
			double lng = ExtractNum(list[i], "\"lon\"", ":", ",");
			if (!CheckLL(lat,lng))
				{
				Log(LOGERR, "Invalid WIKILOC coords for %s", list[i]);
				continue;
				}

			vars link = "http://wikiloc.com/wikiloc/view.do?id="+id;

			++n;
			CSym sym(urlstr(link), name);
			vars country = stripHTML(ExtractString(loc, "(", "", ")"));
			vars region = invertregion( GetToken(loc, 0, '('), country + " @ ");
			sym.SetNum(ITEM_LAT, lat);
			sym.SetNum(ITEM_LNG, lng);
			sym.SetStr(ITEM_REGION, region);
			sym.SetStr(ITEM_KML, "X");
			vara rated(name,"*");
			double stars = rated[0][strlen(rated[0])-1]-(double)'0';
			if (stars>=1 && stars<=5)
				sym.SetStr(ITEM_STARS, starstr(stars, 1));

			printf("%d %d/%d %d/%d/%d    \r", symlist.GetSize(), i, list.length(), from, n, (int)count);
			if (!UpdateCheck(symlist, sym) && MODE>-2)
				continue;

			if (DownloadPage(f, sym.id, sym))
				Update(symlist, sym, NULL, FALSE);
			}

		// next
		from += step;
		if (from>count)
			break;
	}

	return TRUE;
}

int WIKILOC_DownloadBeta(const char *ubase, CSymList &symlist) 
{
	/*
	if (MODE==-2)
		{
		//TRENCANOUS_DownloadBeta(ubase, symlist, WIKILOC_DownloadPage);
		BARRANQUISMO_DownloadBeta(ubase, symlist, WIKILOC_DownloadPage);
		}
	*/
	WIKI_DownloadBeta(ubase, symlist, WIKILOC_DownloadPage);
	return TRUE;
}


// ===============================================================================================


class AICCATASTO : public BETAC
{
public:
	AICCATASTO (const char *base) : BETAC(base)
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

		double lat = ExtractNum(f.memory, "var lat","=",";");
		double lng = ExtractNum(f.memory, "var lon","=",";");
		if (CheckLL(lat,lng))
			{
			vars marker = ExtractString(f.memory, "google.maps.Marker", "(", ")");
			if (strstr(marker, "icon:"))
				{
				sym.SetStr(ITEM_LAT, MkString("@%s;%s", CData(lat),CData(lng)) );
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
		if (!strstr(interest,"SI"))
			sym.SetStr(ITEM_CLASS, "-1:" + interest);

		vars table = ExtractString(f.memory, ">Data discesa", "<tr", "</table");
		vars date = stripHTML(ExtractString(table, "<td", ">", "</td"));
		if (!date.IsEmpty())
			{
			double vdate = CDATA::GetDate(date);
			if (vdate>0)
				sym.SetStr(ITEM_CONDDATE, CDate(vdate));
			}

		sym.SetStr(ITEM_AKA, PatchName(stripHTML( vars(ExtractString(strstr(f.memory,"class=\"page\""), "<h1", ">", "</h1")).replace("\n", "; ") )));

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

	int DownloadBeta(CSymList &symlist) 
	{
		vars rurl = "http://catastoforre.aic-canyoning.it/index/regioni";
		if (f.Download(rurl))
			{
			Log(LOGERR, "ERROR: can't download url %.128s", rurl);
			return FALSE;
			}

		vara region(ExtractString(f.memory, "id=\"mappa\"", "", "</div"), "href=");
		for (int r=1; r<region.length(); ++r)
			{
			vars url = burl( ubase, ExtractString(region[r], "", "\"", "\""));
			vars regname = ExtractString(region[r], "title", "\"", "\"");
			regname = "Italy; " + regname;
			if (f.Download(url))
				{
				Log(LOGERR, "ERROR: can't download url %.128s", url);
				continue;
				}

			vara list(f.memory, "google.maps.Marker(");
			for (int i=1; i<list.length(); ++i)
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
				if (!UpdateCheck(symlist, sym) && MODE>-2)
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
		for (int i=1; i<list.length(); ++i)
			{
			vars line = list[i];
			vars link = ExtractString(line, "href=", "\"", "\"");
			if (link.IsEmpty())
				continue;
			CSym sym(urlstr(burl(ubase,link)));
	#if 0
			vars date = stripHTML(GetToken(line, 0, ":"));
			double vdate = CDATA::GetDate(date, "MMM D; YYYY" );
			if (vdate<=0)
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

// ===============================================================================================
class AICFORUM : public BETAC
{
public:
	AICFORUM (const char *base) : BETAC(base)
	{
	}

	int DownloadBeta(CSymList &symlist) 
	{
		ubase = "win.aic-canyoning.it/forum";

		DownloadFile f;

		const char *rurls[] = {
			"http://win.aic-canyoning.it/forum/default.asp?CAT_ID=3",
			"http://win.aic-canyoning.it/forum/default.asp?CAT_ID=8",
			NULL
		};

		for (int ru=0; rurls[ru]!=NULL; ++ru)
		{
		vars rurl = rurls[ru];
		if (f.Download(rurl))
			{
			Log(LOGERR, "ERROR: can't download url %.128s", rurl);
			return FALSE;
			}
		vara region(ExtractString(f.memory, ">Discussioni<", "", "<script"), "href=");
		for (int r=1; r<region.length(); ++r)
			{
			vars url = ExtractString(region[r], "\"", "", "\"");
			if (url.IsEmpty() || !strstr(url, "forum.asp"))
				continue;
			vars regname = stripHTML(ExtractString(region[r], ">", "", "</a"));
			if (regname.IsEmpty())
				continue;

			regname = !ru ? "Italy; " + regname : regname; 

			double maxpg = 1;
			for (int pg=1; pg<=maxpg; ++pg)
			  {
			  vars urlpg = burl(ubase, url);  
			  if (pg>1) urlpg += MkString("&whichpage=%d", pg);
			  if (f.Download(urlpg))
				{
				Log(LOGERR, "ERROR: can't download url %.128s", urlpg);
				continue;
				}

			  maxpg = ExtractNum(f.memory, ">Pagina:", " di ", "</");

			  vara list(ExtractString(f.memory, ">Discussione<", "", ">Mostra Discussioni"), "<tr ");
			  for (int i=1; i<list.length(); ++i)
				{
				vara links, names, llist(list[i], "href=");
				for (int l=1; l<llist.length(); ++l)
					{
					vars link = ExtractString(llist[l], "\"", "", "\"");
					vars name = stripHTML(ExtractString(llist[l], ">", "", "</a"));
					if (name.IsEmpty() || link.IsEmpty())
						continue;
					links.push(burl(ubase, link));
					names.push(name);
					}

				if (links.length()<=0)
					continue;


				CSym sym(urlstr(links[0]), names[0].replace("\"","").replace("0xC2","").replace("0","Z").replace("1","A").replace("2","B").replace("3","C").replace("4","D"));
				sym.SetStr(ITEM_REGION, ";"+regname);

				printf("%d %d/%d %d/%d    \r", symlist.GetSize(), i, list.length(), r, region.length());
				if (!UpdateCheck(symlist, sym) && MODE>-2)
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
	#define CCS "CCS"
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



	int DownloadPage(const char *url, CSym &sym)
	{ 
	  //ASSERT(!strstr(sym.data, "Brezzi"));
	  double maxpg = 1;
	  int img = 0, pdf = 0;
	  //vars folderfile = CCS"\\" + sym.GetStr(ITEM_DESC);
	  vars folderfile = url2file(sym.GetStr(ITEM_DESC), CCS);
	  for (int pg=1; pg<=maxpg; ++pg)
		{
		vars urlpg = url;
		if (pg>1) urlpg += MkString("&whichpage=%d", pg);
		if (f.Download(urlpg))
		  {
		  Log(LOGERR, "ERROR: can't download url %.128s", urlpg);
		  return FALSE;
		  }

		maxpg = ExtractNum(f.memory, ">Pagina:", " di ", "</");

		{
		vara links(f.memory, "href=");
		for (int i=1; i<links.length(); ++i)
			{
			vars link = ExtractString(links[i], "\"", "", "\"");
			if (strstr(link,"/davidov/") || strstr(link,"/gebu/"))
				Download_File(burl("win.aic-canyoning.it", link), MkString("%s.pdf", folderfile), TRUE), ++pdf;
			}
		}

		{
		vara links(f.memory, "src=");
		for (int i=1; i<links.length(); ++i)
			{
			vars link = ExtractString(links[i], "\"", "", "\"");
			if (strstr(link,"/davidov/") || strstr(link,"/gebu/"))
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

// ===============================================================================================

int CANONISMO_DownloadPage(DownloadFile &f, const char *url, CSym &sym)
{
	//ASSERT( !strstr(url, "chuveje") );
	if (f.Download(url))
			{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			return FALSE;
			}

	vars name = stripHTML(ExtractString(f.memory, ">Nombre", ":", "</div"));
	vara aka(stripHTML(ExtractString(f.memory, ">Conocido", ":", "</div"))); aka.push(name);
	vars country = stripHTML(ExtractString(f.memory, ">Pa\xC3\xADs", ":", "</div"));
	vars region = stripHTML(ExtractString(f.memory, ">Estado", ":", "</div"));
	sym.SetStr(ITEM_DESC, name);
	sym.SetStr(ITEM_AKA, aka.join(";"));
	sym.SetStr(ITEM_REGION, country + "; "+region);
	
	vars summary = stripHTML(ExtractString(f.memory, ">Clasificac", ":", "</div"));
	GetSummary(sym, summary);

	vars season = stripHTML(ExtractString(f.memory, ">Temporada", ":", "</div"));
	sym.SetStr(ITEM_SEASON, season);

	// coordenates or geocode
	vars geoloc;
	float lat = InvalidNUM, lng = InvalidNUM;
	vars town = stripHTML(ExtractString(f.memory, ">Poblac", ":", "</div"));
	if (town.IsEmpty())
		town = stripHTML(ExtractString(f.memory, ">Municipio", ":", "</div"));
	if (!town.IsEmpty())
		 geoloc = MkString("@%s. %s. %s.", town, region, country);
	vars coords = stripHTML(ExtractString(f.memory, ">De inicio", ":", "</div"));
	if (!coords.IsEmpty())
		{
		coords.Replace("O", "W");
		coords.Replace("\xC2\xB0", "o");
		coords.Replace("\xC2\xBA", "o");
		coords.Replace("\xE2\x80\x9D", "\"");
		GetCoords(coords, lat, lng);
		if (!CheckLL(lat,lng))
			{
			if (IsSimilar(region, "Jalisco"))
				UTM2LL("13Q", GetToken(coords,0,' '), GetToken(coords,1,' '), lat, lng);
			if (!CheckLL(lat,lng))
				Log(LOGERR, "Invalid coords '%s' from %s", coords, url);
			}
		}
	if (CheckLL(lat,lng))
		{
		sym.SetNum(ITEM_LAT, lat);
		sym.SetNum(ITEM_LNG, lng);
		}
	else
		{
		sym.SetStr(ITEM_LAT, geoloc);
		sym.SetStr(ITEM_LNG, "");
		}


	double longest = CDATA::GetNum(stripHTML(ExtractString(f.memory, ">Rappel", ":", "</div")));
	if (longest>=0)
		sym.SetStr(ITEM_LONGEST, MkString("%.0fm", longest));

	double raps = CDATA::GetNum(stripHTML(ExtractString(f.memory, ">No. de rappeles", ":", "</div")));
	if (raps>=0)
		sym.SetNum(ITEM_RAPS, raps);

	vara times;
	const char *ids[] = { ">De acceso<", ">De recorrido<", ">De salida<" };
	for (int t=0; t<sizeof(ids)/sizeof(*ids); ++t)
		{
		vars time = stripHTML(ExtractString(f.memory, ids[t], ":", "</div"));
		time.Replace("minutos", "m");
		time.Replace("horas", "h");
		time.Replace("hora", "h");
		time.Replace("hrs.", "h");
		time.Replace(" o ", " - ");
		time.Replace(" y ", " ");
		times.push(time);	
		}
	GetTotalTime(sym, times, url);

	return TRUE;
}

int CANONISMO_DownloadBeta(const char *ubase, CSymList &symlist) 
{
	DownloadFile f;
	vars rurl = burl( ubase, "index.php/canones/canones-mexico");
	if (f.Download(rurl))
		{
		Log(LOGERR, "ERROR: can't download url %.128s", rurl);
		return FALSE;
		}

	vara list(ExtractString(f.memory, "data-options=", "'", "'"), "href=");
	for (int i=1; i<list.length(); ++i)
		{
		vars link = ExtractString(list[i], "", "\\\"", "\\\"");
		link.Replace("\\","");
		link.Replace("cau00f1onismo.com", ubase);
		if (link.IsEmpty())
			continue;

		CSym sym(urlstr(link));

		printf("%d %d/%d    \r", symlist.GetSize(), i, list.length());
		if (!UpdateCheck(symlist, sym) && MODE>-2)
			continue;

		if (CANONISMO_DownloadPage(f, sym.id, sym))
			Update(symlist, sym, NULL, FALSE);
		}


	return TRUE;
}


// ===============================================================================================

class NEWMEXICOS : public BETAC
{
public:

	NEWMEXICOS(const char *base) : BETAC(base, BOOK_DownloadBeta)
	{
		umain = "http://dougscottart.com/hobbies/SlotCanyons.htm";
	}

	#define AKAMARK "*AKA*"
	int DownloadPage(const char *url, CSym &sym)
	{
		//ASSERT( !strstr(url, "chuveje") );
		if (f.Download(url))
				{
				Log(LOGERR, "ERROR: can't download url %.128s", url);
				return FALSE;
				}


		vara aka;
		vara akalist(ExtractString(f.memory, "class=WordSection", ">", ">ACA ").replace(">Spanish<", ">" AKAMARK "<").replace(">English<", ">" AKAMARK "<"), "<p");
		for (int i=0; i<akalist.length(); ++i)		
			{
			vars name = stripHTML(UTF8(ExtractString(akalist[i], ">", "", "</p")));
			name.Replace("English = ", AKAMARK);
			name.Replace("Spanish = ", AKAMARK);
			name.Replace(" or ", ";");
			//name.Replace(" & ", ";");
			name = GetToken(name, 0, "()");
			if (i<=1 || name.Replace(AKAMARK,""))
				if (!name.IsEmpty() && !strstr(name,"http"))
					{
					while (name.Replace(AKAMARK,""));
					aka.push(name);
					}
			}
		if (aka.length()>0)
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
		while (isspace(*sseason) || *sseason=='-')
			++sseason;
		sym.SetStr(ITEM_SEASON, sseason);

		vars coord = stripHTML(ExtractString(f.memory, "GPS ", "", "</p"));
		const char *start = coord;
		while (*start!=0 && !isdigit(*start))
			++start;
		if (*start!=0)
			{
			float lat =InvalidNUM, lng = InvalidNUM;
			GetCoords(start, lat, lng);
			if (CheckLL(lat,lng))
				{
				sym.SetNum(ITEM_LAT, lat);
				sym.SetNum(ITEM_LNG, lng);
				}
			else
				Log(LOGWARN, "Invalid GPS coords for %s", sym.id);
			}

		return TRUE;
	}


	
	int DownloadBeta(CSymList &symlist) 
	{
		DownloadFile f;
		vars rurl = burl( ubase, umain);
		if (f.Download(rurl))
			{
			Log(LOGERR, "ERROR: can't download url %.128s", rurl);
			return FALSE;
			}

			vara list(f.memory, "hobbies/SlotCanyons/");
			for (int i=1; i<list.length(); ++i)
				{
				vars link = GetToken(list[i], 0, "\"");
				vars name = stripHTML(UTF8(ExtractString(list[i], ">", "", "</a")));
				if (link.IsEmpty())
					continue;

				CSym sym(urlstr("http://dougscottart.com/hobbies/SlotCanyons/"+link), name);
				sym.SetStr(ITEM_REGION, "New Mexico");
				printf("%d %d/%d    \r", symlist.GetSize(), i, list.length());
				if (!UpdateCheck(symlist, sym) && MODE>-2)
					continue;

				if (DownloadPage(sym.id, sym))
					Update(symlist, sym, NULL, FALSE);
				}

		return TRUE;
	}
};

// ===============================================================================================

int CNEWMEXICO_DownloadPage(DownloadFile &f, const char *url, CSym &sym)
{
	//ASSERT( !strstr(url, "chuveje") );
	if (f.Download(url))
			{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			return FALSE;
			}

	vars coord = stripHTML(ExtractString(f.memory, "<h1>The Canyon", "coord", "<"));
	int find = coord.indexOf(" N");
	if (find>0)
		{
		float lat =InvalidNUM, lng = InvalidNUM;
		GetCoords(coord.Mid(find), lat, lng);
		if (CheckLL(lat,lng))
			{
			sym.SetNum(ITEM_LAT, lat);
			sym.SetNum(ITEM_LNG, lng);
			}
		}

	return TRUE;
}

int CNEWMEXICO_DownloadBeta(const char *ubase, CSymList &symlist) 
{
	DownloadFile f;
	vars rurl = burl( ubase, "http://canyoneeringnm.org/technical-canyoneering");
	if (f.Download(rurl))
		{
		Log(LOGERR, "ERROR: can't download url %.128s", rurl);
		return FALSE;
		}

	vara rlist(f.memory, "/technical-canyoneering/");
	for (int r=1; r<rlist.length(); ++r)
		{
		vars link = GetToken(rlist[r], 0, "'\" ><");
		if (link.IsEmpty())
			continue;

		rurl = urlstr("http://canyoneeringnm.org/technical-canyoneering/" + link);
		if (f.Download(rurl))
			{
			Log(LOGERR, "ERROR: can't download url %.128s", rurl);
			return FALSE;
			}

		vara list(ExtractString(f.memory, "<div id=\"main\">", "", "<!-- end main"), "href=");
		for (int i=1; i<list.length(); ++i)
			{
			vars link = ExtractString(list[i], "\"", "", "\"");
			vars namesum = stripHTML(ExtractString(list[i], ">", "", "<"));
			if (link.IsEmpty())
				continue;

			vars name = GetToken(namesum, 0, "()");
			vars sum = GetToken(namesum, 1, "()");

			CSym sym(urlstr(burl(ubase,link)), name);
			GetSummary(sym, sum);

			if (sym.id==rurl)
				continue;

			printf("%d %d/%d    \r", symlist.GetSize(), r, rlist.length());
			if (!UpdateCheck(symlist, sym) && MODE>-2)
				continue;

			if (CNEWMEXICO_DownloadPage(f, sym.id, sym))
				Update(symlist, sym, NULL, FALSE);
			}
		}

	return TRUE;
}


// ===============================================================================================

int SPHERE_DownloadPage(DownloadFile &f, const char *url, CSym &sym)
{
	//ASSERT( !strstr(url, "chuveje") );
	if (f.Download(url))
			{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			return FALSE;
			}

	//vars name = stripHTML(ExtractString(f.memory, "<h2", "", "h2"));
	//vars type = stripHTML(ExtractString(f.memory, ">Feature type:", "</b>", "<br"));
	//sym.SetStr(ITEM_DESC, name);
	
	vars loc = stripHTML(ExtractString(f.memory, ">Location (UTM):", "</b>", "<br"));
	loc.Replace( "Datum: WGS84", "");
	vara coord(loc.Trim(), " ");
	if (coord.length()==3)
		{
		float lat = InvalidNUM, lng =InvalidNUM;
		UTM2LL(coord[0], coord[1], coord[2], lat, lng);
		if (CheckLL(lat,lng))
			{
			sym.SetNum(ITEM_LAT, lat);
			sym.SetNum(ITEM_LNG, lng);
			}
		}


	return TRUE;
}


int SPHERE_DownloadBeta(const char *ubase, CSymList &symlist) 
{
	vara regions;
	DownloadFile f;

	// download base
	CString url = "http://speleosphere.org/index.php?content=spdatabase";
	if (f.Download(url))
		{
		Log(LOGERR, "Can't download url %s", url);
		return FALSE;
		}

	vara deps( ExtractString(f.memory, ">Name of Department", "", "</tr>"), "<option");
	for (int d=1; d<deps.length(); ++d)
	{
	vars val = ExtractString(deps[d], "value=", "\"", "\"");
	vars dep = stripHTML(ExtractString(deps[d], ">", "", "<"));
	

	CString url = MkString("http://speleosphere.org/index.php?content=guatesearch&checkdept=on&whatdept=%s&search=Search", val);
	if (f.Download(url))
		{
		Log(LOGERR, "Can't download url %s", url);
		continue;
		}

	vara list(ExtractString(f.memory, ">Cave Name", "", "</table"), "<tr");
	for (int i=1; i<list.length(); ++i)
		{
		vars link = ExtractHREF(list[i]);
		vars name = stripHTML(UTF8(ExtractString(list[i], "href=", ">", "<")));
		if (link.IsEmpty())
			continue;


		CSym sym(urlstr(burl(ubase, link)), name);
		//vars region = stripHTML(ExtractString(f.memory, ">Department:", "</b>", "<br"));
		vara aka(stripHTML(UTF8(ExtractString(list[i], "Also known as ", "", "<")))); 
		aka.push(name);
		sym.SetStr(ITEM_AKA, aka.join(";"));
		sym.SetStr(ITEM_REGION, "Guatemala; " +dep);
		sym.SetStr(ITEM_CLASS, "2:Cave");

		// name transform
		const char *trans[] = { 
			"Resurgence=Cueva Resumidero del ,Resurgence of ,Rising of ,Resumidero en el , resurgence",
			"Insurgence=Cueva Sumidero del ,Insurgence of , infeeder, insurgence,sumidero en ",
			NULL
			};
		for (int t=0; trans[t]!=NULL; ++t)
			{
			vars out = GetToken(trans[t],0,'=');
			vara rep( GetToken(trans[t],1,'='));
			for (int r=0; r<rep.length(); ++r)
				if (name.Replace(rep[r],""))
					{
					sym.SetStr(ITEM_DESC, name.Trim() + " ("+out+")");
					break;
					}
			}

		printf("%d %d/%d %d/%d   \r", symlist.GetSize(), i, list.length(), d, deps.length());
		if (!UpdateCheck(symlist, sym) && MODE>-2)
			continue;

		if (SPHERE_DownloadPage(f, sym.id, sym))
			Update(symlist, sym, NULL, FALSE);
		}

	}

	return TRUE;
}

// ===============================================================================================
/*
int WIKIPEDIA_DownloadPage(DownloadFile &f, const char *url, CSym &sym)
{
	//ASSERT( !strstr(url, "chuveje") );
	if (f.Download(url))
			{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			return FALSE;
			}

	//vars name = stripHTML(ExtractString(f.memory, "<h2", "", "h2"));
	//vars type = stripHTML(ExtractString(f.memory, ">Feature type:", "</b>", "<br"));
	//sym.SetStr(ITEM_DESC, name);
	
	vars loc = stripHTML(ExtractString(f.memory, ">Location (UTM):", "</b>", "<br"));
	loc.Replace( "Datum: WGS84", "");
	vara coord(loc.Trim(), " ");
	if (coord.length()==3)
		{
		double lat = InvalidNUM, lng =InvalidNUM;
		UTM2LL(coord[0], coord[1], coord[2], lat, lng);
		if (CheckLL(lat,lng))
			{
			sym.SetNum(ITEM_LAT, lat);
			sym.SetNum(ITEM_LNG, lng);
			}
		}


	return TRUE;
}


int WIKIPEDIA_DownloadBeta(const char *ubase, CSymList &symlist) 
{
	vara regions;
	DownloadFile f;

	// download base
	CString url = "http://en.wikipedia.org/wiki/List_of_caves";
	if (f.Download(url))
		{
		Log(LOGERR, "Can't download url %s", url);
		return FALSE;
		}

	vara dep( ExtractString(f.memory, "<h2>Contents", "", "id=\"See_also\""), "<h2");
	for (int d=1; d<deps.length(); ++d)
	{
	vars mreg = stripHTML(ExtractString(deps[d], "id=", "\"", "\""));
	vara reg( deps[d], "<h3");
	for (int r=1; r<reg.length(); ++r)
		{

		}
	

	CString url = MkString("http://speleosphere.org/index.php?content=guatesearch&checkdept=on&whatdept=%s&search=Search", val);
	if (f.Download(url))
		{
		Log(LOGERR, "Can't download url %s", url);
		continue;
		}

	vara list(ExtractString(f.memory, ">Cave Name", "", "</table"), "<tr");
	for (int i=1; i<list.length(); ++i)
		{
		vars link = ExtractHREF(list[i]);
		vars name = stripHTML(UTF8(ExtractString(list[i], "href=", ">", "<")));
		if (link.IsEmpty())
			continue;


		CSym sym(urlstr(burl(ubase, link)), name);
		//vars region = stripHTML(ExtractString(f.memory, ">Department:", "</b>", "<br"));
		vara aka(stripHTML(UTF8(ExtractString(list[i], "Also known as ", "", "<")))); 
		aka.push(name);
		sym.SetStr(ITEM_AKA, aka.join(";"));
		sym.SetStr(ITEM_REGION, "Guatemala; " +dep);
		sym.SetStr(ITEM_CLASS, "2:Cave");

		// name transform
		const char *trans[] = { 
			"Resurgence=Cueva Resumidero del ,Resurgence of ,Rising of ,Resumidero en el , resurgence",
			"Insurgence=Cueva Sumidero del ,Insurgence of , infeeder, insurgence,sumidero en ",
			NULL
			};
		for (int t=0; trans[t]!=NULL; ++t)
			{
			vars out = GetToken(trans[t],0,'=');
			vara rep( GetToken(trans[t],1,'='));
			for (int r=0; r<rep.length(); ++r)
				if (name.Replace(rep[r],""))
					{
					sym.SetStr(ITEM_DESC, name.Trim() + " ("+out+")");
					break;
					}
			}

		printf("%d %d/%d %d/%d   \r", symlist.GetSize(), i, list.length(), d, deps.length());
		if (!UpdateCheck(symlist, sym) && MODE>-2)
			continue;

		if (SPHERE_DownloadPage(f, sym.id, sym))
			Update(symlist, sym, NULL, FALSE);
		}

	}

	return TRUE;
}
*/

// ===============================================================================================

int LAOS_DownloadBetaList(const char *url, const char *dep, CSymList &symlist) 
{
	DownloadFile f;
	if (f.Download(url))
		{
		Log(LOGERR, "Can't download url %s", url);
		return FALSE;
		}
	//ASSERT( !strstr(dep,  "Vieng Phouka"));
	vars memory(f.memory);
	//memory.Replace("</TABLE", "</table");
	//memory.Replace("<TR", "<tr");
	//memory.Replace("<TD", "<td");
	//memory.Replace("</TD", "</td");
	vara list(ExtractString(memory, "No.<", "", "</TBODY"), "<tr");
	int xlen = 7;
	for (int i=0; i<list.length(); ++i)
		{
		int header = FALSE;
		vara td(list[i], "<td");
		for (int t=1; t<td.length(); ++t)
			if (strstr(stripHTML(ExtractString(td[t], ">", "", "</td")), "Length (m)"))
				{
				header = TRUE;
				xlen = t + !i;
				break;
				}
		if (header)
			continue;


		if (td.length()<=2)
			continue;

		int x = 0;
		if (i>0 && strstr(list[i-1],"rowspan=\"2\""))
			x = 1;

		vars name = stripHTML(UTF8(ExtractString(td[2-x], ">", "", "</td")));
		//name.Replace(" cave)", ")");
		if (name.IsEmpty())
			continue;
		vars link = url+vars("?id=")+name.replace(" ","_");

		CSym sym(urlstr(link), name);
		sym.SetStr(ITEM_REGION, "Laos; "+vars(dep));
		sym.SetStr(ITEM_CLASS, "2:Cave");

		if (td.length()>4-x)
			{
			float lat = InvalidNUM, lng = InvalidNUM;
			vars slat = stripHTML(ExtractString(td[3-x], ">", "", "</td"));
			vars slng = stripHTML(ExtractString(td[4-x], ">", "", "</td"));
			GetCoords(slat+" "+slng, lat, lng);
			if (CheckLL(lat,lng))
				{
				sym.SetNum(ITEM_LAT, lat);
				sym.SetNum(ITEM_LNG, lng);
				}
			}
		if (td.length()>xlen-x)
			{
			vars slen = stripHTML(ExtractString(td[xlen-x], ">", "", "</td"));
			double len = CDATA::GetNum(slen);
			if (len!=InvalidNUM)
				sym.SetStr(ITEM_LENGTH, CData(len)+"m");
			}

		Update(symlist, sym, NULL, FALSE);
		}
	return FALSE;
}

int LAOS_DownloadBeta(const char *ubase, CSymList &symlist) 
{
	vara regions;
	DownloadFile f;

	// download base
	CString url = "http://www.laoscaveproject.de/cave_areas.htm";
	if (f.Download(url))
		{
		Log(LOGERR, "Can't download url %s", url);
		return FALSE;
		}

	vara deps( ExtractString(f.memory, ">Cave by Area", "", "</TABLE>"), "<TR");
	for (int d=1; d<deps.length(); ++d)
	{
	vars val = ExtractHREF(deps[d]);
	vars dep = stripHTML(ExtractString(deps[d], "href=", ">", "<"));

	printf("%d %d/%d    \r", symlist.GetSize(), d, deps.length());
	LAOS_DownloadBetaList(burl(ubase, val), dep, symlist);
	}

	CSymList deepest;
	LAOS_DownloadBetaList(burl(ubase, "deep_and_long.htm"), "", symlist);
	/*
	for (int d=0; d<deepest.GetSize(); ++d)
		for (int i=0; i<symlist.GetSize(); ++i)
			{
			CSym &sym = symlist[i];
			CSym &dsym = deepest[d];
			if (sym.GetStr(ITEM_DESC)!=dsym.GetStr(ITEM_DESC))
				continue;
			sym.id = dsym.id;
			sym.SetNum(ITEM_LENGTH, max(dsym.GetNum(ITEM_LENGTH),sym.GetNum(ITEM_LENGTH)) );
			}

	symlist.Sort();
	for (int d=0; d<deepest.GetSize(); ++d)
			Update(symlist, deepest[d], NULL, FALSE);
	*/

	return TRUE;
}

// ===============================================================================================

int JALISCO_DownloadPage(DownloadFile &f, const char *url, CSym &sym)
{
	//ASSERT( !strstr(url, "chuveje") );
	if (f.Download(url))
			{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			return FALSE;
			}
	vars memory = f.memory;
	memory.Replace("\n", " ");
	memory.Replace("\r", " ");
	memory.Replace("\t", " ");
	memory.Replace("&ndash;", "-");	
	memory.Replace("&nbsp;", " ");	
	while (memory.Replace("  ", " "));

	vars name = stripHTML(ExtractString(memory, ">Nombre", "- ", "<br"));

	vars country = "Mexico";
	vara aka(stripHTML(ExtractString(memory, ">Toponimia", "-", "<br").replace("(conocido tambi&eacute;n como)","")), ";");
	if (!name.IsEmpty()) aka.push(name);
	sym.SetStr(ITEM_AKA, aka.join(";"));

	if (aka.length()==0)
		return FALSE; // empty page

	vars region = stripHTML(ExtractString(memory, ">Estado", "-", "<br"));
	sym.SetStr(ITEM_REGION, country + "; "+region);
	
	vars summary = stripHTML(ExtractString(memory, ">Clasificaci&oacute;n", "", "<br"));
	GetSummary(sym, summary);

	vars season = stripHTML(ExtractString(memory, ">&Eacute;poca", "-", "<br"));
	sym.SetStr(ITEM_SEASON, season);

	//ASSERT( !strstr(url, "nogal") );
	vars shuttle = stripHTML(ExtractString(memory, ">Combinaci", "-", "<br"));
	shuttle.Trim(" .");
	if (!shuttle.IsEmpty())
		{
		if (IsSimilar(shuttle, "No") || strstr(shuttle, "egresa"))
			shuttle = "No !" + shuttle;
		else
			shuttle = "Yes !" + shuttle;
		sym.SetStr(ITEM_SHUTTLE, shuttle);
		}

	// coordenates or geocode
	vars geoloc;
	double lat = InvalidNUM, lng = InvalidNUM;
	vars town = stripHTML(ExtractString(memory, ">Municipio", "-", "<br"));
	if (!town.IsEmpty())
		 geoloc = MkString("@%s. %s. %s.", town, region, country);
	/*
	vars coords = stripHTML(ExtractString(memory, ">De inicio", ":", "</div"));
	if (!coords.IsEmpty())
		{
		coords.Replace("O", "W");
		coords.Replace("\xC2\xB0", "o");
		coords.Replace("\xC2\xBA", "o");
		coords.Replace("\xE2\x80\x9D", "\"");
		GetCoords(coords, lat, lng);
		if (!CheckLL(lat,lng))
			{
			if (IsSimilar(region, "Jalisco"))
				UTM2LL("13Q", GetToken(coords,0,' '), GetToken(coords,1,' '), lat, lng);
			if (!CheckLL(lat,lng))
				Log(LOGERR, "Invalid coords '%s' from %s", coords, url);
			}
		}
	*/
	if (CheckLL(lat,lng))
		{
		sym.SetNum(ITEM_LAT, lat);
		sym.SetNum(ITEM_LNG, lng);
		}
	else
		{
		sym.SetStr(ITEM_LAT, geoloc);
		sym.SetStr(ITEM_LNG, "");
		}


	double longest = CDATA::GetNum(stripHTML(ExtractString(memory, "Rapel mas alto", "-", "<br")));
	if (longest>=0)
		sym.SetStr(ITEM_LONGEST, MkString("%.0fm", longest));

	//double raps = CDATA::GetNum(stripHTML(ExtractString(memory, ">No. de rappeles", ":", "</div")));
	//if (raps>=0)
	//	sym.SetNum(ITEM_RAPS, raps);
	//ASSERT( !strstr(url, "matatlan"));
	//if (strstr(url, "matatlan"))
	//		Log(memory);

	vara times;
	memory.Replace("Aproximaci","aproximaci");
	memory.Replace("Descenso","descenso");
	memory.Replace("Retorno","retorno");
	const char *ids[] = { ">Horario de aproximaci", ">Horario de descenso", ">Horario de retorno" };
	for (int t=0; t<sizeof(ids)/sizeof(*ids); ++t)
		{
		vars time = stripHTML(ExtractString(memory, ids[t], "-", "<br"));
		time.Replace("minutos", "m");
		time.Replace("horas", "h");
		time.Replace("hora", "h");
		time.Replace("hrs.", "h");
		time.Replace(" o ", " - ");
		time.Replace(" y ", " ");
		times.push(time);	
		}
	GetTotalTime(sym, times, url);

	sym.SetStr(ITEM_DEPTH, GetMetric(stripHTML(ExtractString(memory, ">Desnivel", "-", "<br"))));
	sym.SetStr(ITEM_LENGTH, GetMetric(stripHTML(ExtractString(memory, ">Longitud", "-", "<br"))));

	sym.SetStr(ITEM_ROCK, stripHTML(ExtractString(memory, ">Tipo de Roca", "-", "<br")));


	return TRUE;
}

int JALISCO_DownloadBeta(const char *ubase, CSymList &symlist) 
{
	DownloadFile f;
	vars rurl = burl(ubase, "");
	if (f.Download(rurl))
		{
		Log(LOGERR, "ERROR: can't download url %.128s", rurl);
		return FALSE;
		}

	vara list(ExtractString(f.memory, "/equipo-de-cantildeonismo.html", "", "/escalada.html"), "href=");
	for (int i=1; i<list.length(); ++i)
		{
		vars link = ExtractString(list[i], "", "\"", "\"");
		vars name = stripHTML(ExtractString(list[i], ">", "", "</a"));
		if (link.IsEmpty())
			continue;

		link = burl(ubase, link);
		CSym sym(urlstr(link), name);

		printf("%d %d/%d    \r", symlist.GetSize(), i, list.length());
		if (!UpdateCheck(symlist, sym) && MODE>-2)
			continue;

		if (JALISCO_DownloadPage(f, sym.id, sym))
			Update(symlist, sym, NULL, FALSE);
		}


	return TRUE;
}


// ===============================================================================================

int DESCENSO_DownloadBeta(const char *ubase, CSymList &symlist) 
{
	DownloadFile f;
	vara urls;
	urls.push(burl(ubase, "barrancos.html"));
	urls.push(burl(ubase, "ferratas/ferratas.html"));

	for (int u=0; u<urls.length(); ++u)
	{
	vars url = urls[u];
	if (f.Download(url))
		{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		continue;
		}

	printf("%d %d/%d    \r", symlist.GetSize(), u, urls.length());

	// extract page
	if (strstr(f.memory, ">Acceso") && strstr(f.memory, ">Retorno"))
	{
	vars name = stripHTML(ExtractString(f.memory, "<title", ">", "</title"));

	CSym sym(urlstr(url), name);

/*
	if (!UpdateCheck(symlist, sym) && MODE>-2)
		continue;

	if (JALISCO_DownloadPage(f, sym.id, sym))
*/
		Update(symlist, sym, NULL, FALSE);
		continue;
	}

	// queue links
	vara list(f.memory, "class=\"telist\"");
	for (int i=1; i<list.length(); ++i)
		{
		vars link = ExtractString(list[i], "href=", "", ">");
		link = GetToken(link, 0, ' ');
		link.Trim(" \"'");
		if (link.IsEmpty())
			continue;
		if (!IsSimilar(link, "http"))
			{
			vara path(url, "/");
			path[path.length()-1] = link;
			link = path.join("/");
			}
		link = urlstr(link);
		if (strstr(link, "php.cgi") || strstr(link, "mailto:") || strstr(link, "file:") || strstr(link, ".css"))
			continue;
		if (strstr(link, "/fotos/") || strstr(link, "../") || strstr(link, "/otras.html"))
			continue;
		if (!strstr(link, ubase))
			continue;
		if (urls.indexOf(link)>=0)
			continue;
		//ASSERT( !strstr(link,"centelles"));
		urls.push(link);
		}


	}


	return TRUE;
}



// ===============================================================================================


class DOBLEVUIT : public BETAC
{
public:
	DOBLEVUIT(const char *base) : BETAC(base)
	{
		urls.push( "http://doblevuit.com/blog/barrancs-i-torrents/" );
		x = BETAX("<h4", "</tr");
	}

	int DownloadPage(const char *url, CSym &sym)
	{
			if (strstr(url, "/wp.me") || strstr(url, "?p=") )
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
		urls.push( "http://neotopossport.blogspot.com.es/p/barranquismo.html" );
		x = BETAX("<br", NULL, ">REALIZADOS", ">PENDIENTES");
	}

	int DownloadPage(const char *url, CSym &sym)
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
		urls.push( "http://www.mallorcaverde.es/menu-barrancos.htm" );
		x = BETAX("<td", "</td", "banner-barrancos.jpg", "</table");
		region = "Spain";
	}

};

class ESCONAT : public BETAC
{
public:

	ESCONAT(const char *base) : BETAC(base)
	{
		urls.push(  "http://www.esconatura.com/barranquismo-listado.htm" );
		x = BETAX("<tr", "</tr", ">Nombre", "</table");
		region = "Spain";
	}

};


// ===============================================================================================
 


double ExtractNum(const char *mbuffer, const char *searchstr, const char *startchar, const char *endchar)
{
	CString symbol;
	if (mbuffer) GetSearchString(mbuffer, searchstr, symbol, startchar, endchar);
	return CDATA::GetNum(symbol);
}



//https://www.google.com/maps/d/u/0/viewer?mid=1SXfd6dNXXIVNgnna4bKq_4SOK4c
#define AZORESKML "http://www.google.com/maps/d/u/0/kml?mid=1SXfd6dNXXIVNgnna4bKq_4SOK4c&forcekml=1";


int GetKMLCoords(const char *str, double &lat, double &lng)
{
	vara coords(str);
	lat = lng = InvalidNUM;
	if (coords.length() >= 2)
	{
		lat = CDATA::GetNum(coords[1]);
		lng = CDATA::GetNum(coords[0]);
	}
	return CheckLL(lat, lng);
}


int AZORES_DownloadPage(const char *memory, CSym &sym) 
{
	vars url = sym.id;

	double lat = InvalidNUM, lng = InvalidNUM;
	if (!GetKMLCoords(ExtractString(memory, "<coordinates", ">", "</coordinates"), lat, lng))
		{
		Log(LOGERR, "Invalid KML Start coords from %s", url);
		return FALSE;
		}
	sym.SetNum(ITEM_LAT, lat);
	sym.SetNum(ITEM_LNG, lng);

	//sym.SetStr(ITEM_SEASON, stripHTML(ExtractString(f.memory, ">Periodo", ":", "<br")));
	
	GetSummary(sym, stripHTML(ExtractString(memory, "Grade:", "", "<br")));

	sym.SetNum(ITEM_RAPS, CDATA::GetNum(stripHTML(ExtractString(memory, " rappels:", "", "<br"))));
	sym.SetStr(ITEM_LONGEST, GetMetric(stripHTML(ExtractString(memory, " rappel:", "", "<br"))));
	//sym.SetStr(ITEM_LENGTH, GetMetric(stripHTML(ExtractString(f.memory, "Sviluppo:", "", "<br"))));
	sym.SetStr(ITEM_DEPTH, GetMetric(stripHTML(ExtractString(memory, "level:", "", "<br"))));
	
	//sym.SetStr(ITEM_SHUTTLE, stripHTML(ExtractString(memory, "Navetta:", "", "<br").replace(" ","").replace("Circa","").replace("No/","Optional ").replace(",",".")));

	vars interest = stripHTML(ExtractString(memory, "Quality:", "", "<br"));
	double stars = CDATA::GetNum(interest.replace(";","."));
	if (stars>0)
		sym.SetStr(ITEM_STARS, starstr(stars,1));

	vara times;
	const char *ids[] = { "Access:", "Time:", "Return:" };
	//ASSERT( !strstr(url,"/kanion.php?id=41"));
	for (int t=0; t<sizeof(ids)/sizeof(*ids); ++t)
		{
		CString time = stripHTML(ExtractString(memory, ids[t], "", "<br"));
		while (!ExtractStringDel(time, "(", "", ")").IsEmpty());
		times.push(vars(time).replace("hours", "h").replace("days", "j").replace(";","."));	
		}
	GetTotalTime(sym, times, url);

	return TRUE;
}

int BOOKAZORES_DownloadKML(CSym &sym, vara &descents, const char *file)
{
			vars url = sym.id;
			vars name = sym.GetStr(ITEM_DESC); //ExtractString(start, "<name", ">", "</name");
			//ASSERT(!strstr(name,"Cedro"));

			vara lines, linelist;
			double slat = sym.GetNum(ITEM_LAT), slng = sym.GetNum(ITEM_LNG);
			double elat = InvalidNUM, elng = InvalidNUM;
			int imatch = -1, match = -1, distn = 0;
			double idist = 500;
			for (int i=1; i<descents.length(); ++i)
				//if (strstr(descents[i], name))
					{
					double eps = 1e-5;
					double lat = InvalidNUM, lng = InvalidNUM;
					vara coords(ExtractString(descents[i], "<coordinates", ">", "</coordinates"), " ");
					if (coords.length()<3)
						{
						vars invalid = descents[i];
						continue;
						}
					double dist;
					for (int j=0; j<1; ++j)
						{
						if (GetKMLCoords(coords[j], lat, lng))
							if ((dist=Distance(lat, lng, slat, slng))<=idist)
								{
								distn = dist==idist;
								imatch=i, idist=dist, match=0;
								}
						if (GetKMLCoords(coords[coords.length()-1-j], lat, lng))
							if ((dist=Distance(lat, lng, slat, slng))<=idist)
								{
								distn = dist==idist;
								imatch=i, idist=dist, match=1;
								}
						}
					}
			if (distn>0)
				return FALSE;

			if (imatch<0)
				return FALSE;

			vara coords(ExtractString(descents[imatch], "<coordinates", ">", "</coordinates"), " ");
			descents.RemoveAt(imatch);

			if (!match)
				{
				for (int i=0; i<coords.length(); ++i)
					if (GetKMLCoords(coords[i], elat, elng))
						linelist.push(CCoord3(elat, elng));
				}
			else
				{
				for (int i=coords.length()-1; i>=0; --i)
					if (GetKMLCoords(coords[i], elat, elng))
						linelist.push(CCoord3(elat, elng));
				}

			if (!CheckLL(elat, elng, NULL))
				{
				Log(LOGERR, "AZORES KML no valid descent for %s", name);
				return FALSE;
				}

			vara styles, points;
			styles.push( "start=http://maps.google.com/mapfiles/kml/paddle/grn-blank.png" );
			styles.push( "end=http://maps.google.com/mapfiles/kml/paddle/red-blank.png" );

			points.push( KMLMarker( "start", slat, slng, "Start") );
			points.push( KMLMarker( "end", elat, elng, "End") );

			lines.push( KMLLine("Descent", NULL, linelist, RGB(0xFF,0,0), 5) );

			inetfile out(file);
			return SaveKML("Azores Canyoning Book", "Desnivel", url, styles, points, lines, &out);
}

int BOOKAZORES_DownloadBeta(const char *ubase, CSymList &symlist) 
{
	vars url = AZORESKML;
	DownloadFile f;
	if (f.Download(url))
		{
		Log(LOGERR, "Can't download base url %s", url);
		return TRUE;
		}	
	 if (!strstr(f.memory, "<Placemark>"))
		Log(LOGERR, "ERROR: Could not download kml %s", url);
	vars memory = f.memory;
	vars placemark = "<Placemark>";
	vara starts( ExtractStringDel(memory, "<Folder>", "", "</Folder>", TRUE, TRUE), placemark);
	vara ends( ExtractStringDel(memory, "<Folder>", "", "</Folder>", TRUE, TRUE), placemark);
	vara descents( ExtractStringDel(memory, "<Folder>", "", "</Folder>", TRUE, TRUE), placemark);

	 if (rwlist.GetSize()==0)
	   LoadRWList();

	 vara duplist;
	 for (int i=1; i<starts.length(); ++i)
		{
		vars oname = ExtractString(starts[i], "<name", ">", "</name");
		vars name = oname;
		if (duplist.indexOf(name)>=0)
			name += MkString(" #%d", i);
		duplist.push(name);
		CSym sym(urlstr(MkString("http://ropewiki.com/User:Azores_Canyoning_Book?id=%s.", name.replace(" ", "_"))), oname);
		sym.SetStr(ITEM_AKA, oname);
		sym.SetStr(ITEM_REGION, "Azores");

		// update sym
		if (UpdateCheck(symlist, sym) || MODE<=-2)
			if (AZORES_DownloadPage(starts[i], sym))
				Update(symlist, sym, NULL, FALSE);
		}

	for (int i=0; i<symlist.GetSize(); ++i)
		symlist[i].SetStr(ITEM_KML, "");
	
	vara order("Upper,Middle,Lower");
	for (int o=0; o<=5; ++o)
	 {
	 for (int i=0; i<symlist.GetSize(); ++i)
	   {
		// save kml
		CSym &sym = symlist[i];
		vars rwid = ExtractString(sym.GetStr(ITEM_MATCH), RWID, "", RWLINK);
		if (!rwid.IsEmpty() && sym.GetStr(ITEM_KML).IsEmpty())
			{
			int found = rwlist.Find(RWID+rwid);
			if (found<0)
				{
				Log(LOGERR, "Mismatched RWID %s for %s", rwid, sym.id);
				continue;
				}
			CSym &rwsym = rwlist[found];

			if (o<order.length())
				if (!strstr(rwsym.GetStr(ITEM_DESC), order[o]))
					continue;

			vars file = MkString("%s\\%s.kml", KMLFIXEDFOLDER, rwsym.GetStr(ITEM_DESC));
			if (BOOKAZORES_DownloadKML(sym, descents, file))
				sym.SetStr(ITEM_KML, "X");
			else if (o==5)
				Log(LOGERR, "AZORES KML invalid matched descent for %s %s,%s", sym.GetStr(ITEM_DESC), sym.GetStr(ITEM_LAT), sym.GetStr(ITEM_LNG));

			if (strstr(rwsym.GetStr(ITEM_KML), "ropewiki.com"))
				DeleteFile(file); // do not overwrite if already exists
			}
		}
	 }

	for (int i=1; i<descents.length(); ++i)
		Log(LOGERR, "Unmatched descent for %s", ExtractString(descents[i], "<name", ">", "</name"));

	return TRUE;
}








#define RWLANG "rwlang="

vars RWLANGLink(const char *purl, const char *lang)
{
		CString url = urlstr(purl);
		url += strchr(url,'?') ? '&' : '?';
		url +=RWLANG;
		url +=lang;
		return url;
}

vars KMLIDXLink(const char *purl, const char *pkmlidx)
{
		vars url = purl;
		vars kmlidx = pkmlidx;
		vars postfix;
		if (kmlidx.IsEmpty())
			postfix = "kmlidxna";
		else if (kmlidx!="X")
			postfix = "kmlidx=" + kmlidx;
		if (!postfix.IsEmpty())
			url += CString(strchr(url,'?') ? '&' : '?')+postfix;
		return url;
}

/*
typedef int tbeta(const char *base, CSymList &symlist);
typedef int textract(const char *base, const char *url, inetdata *out, int fx);
typedef int tcond(const char *base, CSymList &symlist);
*/

#define FCHK strstr

#define F_MATCHMANY "M"
#define F_MATCHMORE "+"
#define F_MATCHNODATE "!"
#define F_MATCHANY "*"
#define F_MATCHPIC "P"
#define F_NOMATCH "X"
#define F_GROUP "G"
#define F_REGSTRICT "R"


// ===============================================================================================
int ROPEWIKI_DownloadBeta(const char *ubase, CSymList &symlist);
int ROPEWIKI_DownloadRedirects(const char *ubase, CSymList &symlist);

static Code codelist[] = {
	Code(0,0,"rw", NULL, "Ropewiki", NULL,new BETAC(RWID, ROPEWIKI_DownloadBeta), NULL ),
	Code(0,0,RWREDIR, NULL, "Redirects", NULL,new BETAC(RWTITLE, ROPEWIKI_DownloadRedirects), NULL ),

	// local databases
	Code(0,1,"kmlmap", NULL, NULL, NULL, new BETAC( "KML:", KMLMAP_DownloadBeta), "World" ),
	Code(0,1,"book_GrandCanyon", NULL, "Grand Canyoneering Book by Todd Martin", "Grand Canyoneering Book", new BOOK("ropewiki.com/User:Grand_Canyoneering_Book"), "USA (Arizona)" ),
	Code(0,1,"book_Arizona", NULL, "Arizona Technical Canyoneering Book by Todd Martin", "Arizona Technical Canyoneering Book", new BOOK("ropewiki.com/User:Arizona_Technical_Canyoneering_Book"), "USA (Grand Canyon)"),
	Code(0,1,"book_Zion", NULL, "Zion Canyoneering Book by Tom Jones", "Zion Canyoneering Book", new BOOK("ropewiki.com/User:Zion_Canyoneering_Book"), "USA (Zion)"),
	Code(0,1,"book_Vegas", NULL, "Las Vegas Slots Book by Rick Ianiello", "Las Vegas Slots Book", new BOOK("ropewiki.com/User:Las_Vegas_Slots_Book"), "USA (Las Vegas)"),
	Code(0,1,"book_Moab", NULL, "Moab Canyoneering Book by Derek A. Wolfe", "Moab Canyoneering Book", new BOOK("ropewiki.com/User:Moab_Canyoneering_Book"), "USA (Moab)"),
	Code(0,1,"book_Ossola", NULL, "Canyons du Haut-Piemont Italien Book by Speleo Club de la Valle de la Vis", "Canyons du Haut-Piemont Italien Book", new BOOK("ropewiki.com/User:Canyons_du_Haut-Piemont_Italien_Book"), "Italy (Piemonte)"),
	Code(0,1,"book_Alps", NULL, "Canyoning in the Alps Book by Simon Flower", "Canyoning in the Alps Book ", new BOOK("ropewiki.com/User:Canyoning_in_the_Alps_Book"), "Europe (Alps)"),
	Code(0,1,"book_SwissAlps", NULL, "Canyoning in the Swiss Alps Book by Association Openbach", "Canyoning in the Swiss Alps Book ", new BOOK( "ropewiki.com/User:Canyoning_in_the_Swiss_Alps_Book"), "Switzerland"),
	Code(0,1,"book_NordItalia", NULL, "Canyoning Nord Italia Book by Pascal van Duin", "Canyoning Nord Italia Book ", new BOOK( "ropewiki.com/User:Canyoning_Nord_Italia_Book"), "Italy (North)"),
	//Code(0,"book_Azores", NULL, "Azores Canyoning Book by Desnivel", "Azores Canyoning Book ", new BETAC( "ropewiki.com/User:Azores_Canyoning_Book"), BOOKAZORES_DownloadBeta, "Azores" ),

	// USA kml extract
	Code(1,1,"rtr", NULL, "RoadTripRyan.com", "RoadTripRyan.com", new BETAC( "roadtripryan.com",ROADTRIPRYAN_DownloadBeta, ROADTRIPRYAN_ExtractKML), "USA", "1-5", "Disabled"),
	Code(1,1,"cco", NULL, "CanyonCollective.com", "CanyonCollective.com", new BETAC( "canyoncollective.com/betabase",CCOLLECTIVE_DownloadBeta, CCOLLECTIVE_ExtractKML, CCOLLECTIVE_DownloadConditions), "World", "1-5", "From KML data", "From Conditions"),
	Code(1,1,"cch", NULL, "CanyonChronicles.com",  NULL, new BETAC( "canyonchronicles.com",CCHRONICLES_DownloadBeta, CCHRONICLES_ExtractKML), "World", "", "From KML data" ), 
	Code(1,1,"haz", NULL, "HikeArizona.com", "HikeArizona.com",new BETAC( "hikearizona.com",HIKEAZ_DownloadBeta, HIKEAZ_ExtractKML, HIKEAZ_DownloadConditions), "USA", "1-5", "From Custom Data", "From Trip Reports"),
	Code(1,1,"orc", NULL, "OnRopeCanyoneering.com",  NULL,new BETAC( "onropecanyoneering.com",ONROPE_DownloadBeta, ONROPE_ExtractKML), "USA", "", "From GPX data"),
	Code(1,1,"blu", NULL, "BluuGnome.com",  NULL,new BETAC( "bluugnome.com",BLUUGNOME_DownloadBeta, BLUUGNOME_ExtractKML), "USA", "", "From HTML data"),
	Code(1,1,"cba", NULL, "Chris Brennen's Adventure Hikes (St Gabriels)", "Chris Brennen's Adventure Hikes (St Gabriels)", new CBRENNEN( "brennen.caltech.edu/advents"), "USA", "1-3 -> 1.5-4.5", "From HTML data"),
	Code(1,1,"cbs", NULL, "Chris Brennen's Adventure Hikes (Southwest)", "Chris Brennen's Adventure Hikes (Southwest)", new CBRENNEN( "brennen.caltech.edu/swhikes"), "USA", "1-3 -> 1.5-4.5", "From HTML data"),
	Code(1,1,"cbw", NULL, "Chris Brennen's Adventure Hikes (World)", "Chris Brennen's Adventure Hikes (World)", new CBRENNEN( "brennen.caltech.edu/world"), "World", "1-3 -> 1.5-4.5", "From HTML data"),

	// USA star extract
	Code(1,1,"cus", NULL, "CanyoneeringUSA.com", "CanyoneeringUSA.com",new BETAC( "canyoneeringusa.com",CUSA_DownloadBeta), "USA", "1-3 -> 1.5-4.5" ),
	Code(1,1,"zcc", NULL, "ZionCanyoneering.com", "ZionCanyoneering.com",new ZIONCANYONEERING( "zioncanyoneering.com"), "USA", "1-5" ),
	Code(1,1,"asw", NULL, "AmericanSouthwest.net", "AmericanSouthwest.net",new BETAC( "americansouthwest.net/slot_canyons",ASOUTHWEST_DownloadBeta),"USA", "1-5" ),
	Code(1,1,"thg", NULL, "ToddsHikingGuide.com", "ToddsHikingGuide.com",new TODDMARTIN("toddshikingguide.com/Hikes"), "USA", "1-5" ),
	Code(1,1,"dpm", NULL, "Dave Pimental's Minislot Guide", "Dave Pimental's Minislot Guide",new BETAC( "math.utah.edu/~sfolias/minislot",DAVEPIMENTAL_DownloadBeta), "USA", "1-5" ),

	// USA no extract
	Code(1,1,"cnw", NULL, "CanyoneeringNorthwest.com",  NULL,new BETAC( "canyoneeringnorthwest.com",CNORTHWEST_DownloadBeta), "USA / Canada" ),
	Code(1,1,"cut", NULL, "Climb-Utah.com",  NULL,new BETAC( "climb-utah.com",CLIMBUTAH_DownloadBeta), "USA (Utah)" ),
	Code(1,1,"spo", NULL, "SummitPost.org",  NULL,new BETAC( "summitpost.org",SUMMIT_DownloadBeta), "USA / World" ),
	Code(1,1,"cnm", NULL, "CanyoneeringNM.org",  NULL,new BETAC( "canyoneeringnm.org",CNEWMEXICO_DownloadBeta), "USA (New Mexico)" ),
	Code(1,1,"nms", NULL, "Doug Scott's New Mexico Slot Canyons",  NULL,new NEWMEXICOS( "dougscottart.com/hobbies/SlotCanyons/"), "USA (New Mexico)" ),
	Code(1,1,"SuperAmazingMap", NULL, "Super Amazing Map", NULL,new SAM("ropewiki.com/User:Super_Amazing_Map"), "USA" ),
	Code(1,0,"cond_can", NULL, "Candition.com",  NULL,new BETAC( "candition.com/canyons",CANDITION_DownloadBeta, NULL, CANDITION_DownloadBeta), "USA", "", "", "From Conditions"),
	Code(1,0,"cond_karl", NULL, "Karl Helser's NW Adventures",  NULL, new KARL( "karl-helser.com"), "Pacific Northwest"),
	Code(1,0,"cond_sixgun", NULL, "Mark Kilian's Adventures",  NULL, new SIXGUN( "6ixgun.com"), "USA"),
	// UK & english
	Code(1,1,"ukg", NULL, "UK CanyonGuides.org", "UK CanyonGuides.org",new BETAC( "canyonguides.org",UKCG_DownloadBeta), "United Kingdom", "1-4 -> 2-5" ),
	Code(1,1,"kcc", NULL, "KiwiCanyons.org", "KiwiCanyons.org",new BETAC( "kiwicanyons.org",KIWICANYONS_DownloadBeta), "New Zealand", "1-4 -> 2-5"),
	Code(1,1,"ico", NULL, "Icopro.org",  NULL, new ICOPRO( "icopro.org"), "World"),
	Code(1,1,"cmag", NULL, "CanyonMag.net",  "CanyonMag.net", new CMAGAZINE( "canyonmag.net/database"), "Japan / World", "1-4 -> 2-5" ),

	// Spanish
	Code(1,0,"bqn", "es", "Barranquismo.net", NULL, new BETAC( "barranquismo.net" /*, BARRANQUISMO_DownloadBeta*/ ), "World" ),
	Code(1,1,"can", "es", "Ca\xC3\xB1onismo.com",  NULL,new BETAC( "xn--caonismo-e3a.com",CANONISMO_DownloadBeta), "Mexico"  ),
	Code(1,1,"jal", "es", "JaliscoVertical.Weebly.com",  NULL,new BETAC( "jaliscovertical.weebly.com",JALISCO_DownloadBeta), "Mexico" ),
	Code(1,1,"alp", "es", "Altopirineo.com", "Altopirineo.com",new BETAC( "altopirineo.com",ALTOPIRINEO_DownloadBeta), "Spain", "1-4 -> 2-5" ),
	Code(1,1,"ltn", "es", "Latrencanous.com",  NULL,new BETAC( "latrencanous.com",TRENCANOUS_DownloadBeta), "Spain" ),
	Code(1,1,"bqo", "es", "Barrancos.org", "Barrancos.org",new BETAC( "barrancos.org",BARRANCOSORG_DownloadBeta), "Spain / Europe", "1-10 -> 1-5" ),
	Code(1,1,"ddb", "es", "DescensoDeBarrancos.com",  NULL,new BETAC( "descensodebarrancos.com",DESCENSO_DownloadBeta), "Spain / Europe" ),
	Code(9,1,"gua", "es", "Guara.info",  NULL,new BETAC( "guara.info",GUARAINFO_DownloadBeta), "Guara (Spain)" ),
	Code(1,1,"4x4", "es", "Actionman4x4.com",  NULL,new ACTIONMAN4X4( "actionman4x4.com/canonesybarrancos"), "Andalucia (Spain)" ),
	Code(1,1,"bcan", "es", "BarrancosCanarios.com",  NULL,new BCANARIOS( "barrancoscanarios.com"), "Canarias (Spain)" ),
	Code(1,0,"bcue", "es", "BarrancosEnCuenca.es",  NULL,new BCUENCA( "barrancosencuenca.es"), "Spain" ),
	Code(1,0,"tec", "es", "TeamEspeleoCanyons Blog",  NULL, new ESPELEOCANYONS( "teamespeleocanyons.blogspot.com.es"), "Spain / Europe"),
	Code(1,0,"sime", "es", "SiMeBuscasEstoyConLasCabras Blog",  NULL, new SIMEBUSCAS( "simebuscasestoyconlascabras.blogspot.com"), "Spain / Europe", "", "From MAP"),
	Code(1,0,"rocj", "es", "RocJumper.com",  NULL, new ROCJUMPER( "rocjumper.com/barranco/"), "Spain / Europe"),


	// Blogs
	Code(1,0,"cond_nko", "es", "NKO-Extreme.com",  NULL, new NKO( "nko-extreme.com"), "Spain / Europe"),
	Code(1,0,"cond_bqe", "es", "Barranquistas.es",  NULL, new BARRANQUISTAS( "barranquistas.es"), "Spain / Europe"),

	// Mallorca
	Code(1,0,"dvc", "es", "Doblevuit.com",  NULL, new DOBLEVUIT( "doblevuit.com"), "Mallorca (Spain)"),
	Code(1,0,"nts", "es", "NeoToposSport",  NULL, new NEOTOPO( "neotopossport.blogspot.com"), "Mallorca (Spain)"),
	Code(1,0,"mve", "es", "MallorcaVerde.es",  NULL, new MALLORCAV( "mallorcaverde.es"), "Mallorca (Spain)"),
	Code(1,0,"enc", "es", "EscoNatura.com",  NULL, new ESCONAT( "esconatura.com"), "Mallorca (Spain)"),
	Code(1,0,"cond_365", "es", "365DiasDeUnaGuia",  NULL,new BETAC( "http://365diasdeunaguia.wordpress.com",BOOK_DownloadBeta, NULL, COND365_DownloadBeta), "Spain", "", "", "From group updates"),
	Code(1,0,"cond_inf", "es", "Caudal.info",  NULL,new BETAC( "http://www.caudal.info",INFO_DownloadBeta, NULL, INFO_DownloadBeta), "World", "", "", "From Conditions"),

	// German
	Code(1,1,"ccn", "de", "Canyon.Carto.net", "Canyon.Carto.net",new BETAC( "canyon.carto.net",CANYONCARTO_DownloadBeta, CANYONCARTO_ExtractKML), "World", "1-7 -> 1-5", "From KML data" ),

	// French
	Code(1,1,"dcc", "fr", "Descente-Canyon.com", "Descente-Canyon.com",new BETAC( "descente-canyon.com",DESCENTECANYON_DownloadBeta, DESCENTECANYON_ExtractKML, DESCENTECANYON_DownloadConditions), "World", "0-4 -> 1-5", "From Custom Data", "From Conditions" ),
	Code(1,1,"alt", "fr", "Altisud.com", "Altisud.com",new BETAC( "altisud.com",ALTISUD_DownloadBeta), "Europe", "1-5" ),
	Code(1,1,"climb7", "fr", "Climbing7",  "Climbing7", new CLIMBING7( "climbing7.wordpress.com"), "Europe", "1-3 -> 1.5-4.5"),
	Code(1,1,"yad", "fr", "Yadugaz07", "Yadugaz07", new YADUGAZ( "yadugaz07.com"), "France", "1-10 -> 1->5" ),
	Code(1,1,"reu", "fr", "Ligue Reunionnaise de Canyon", "Ligue Reunionnaise de Canyon", new REUNION("lrc-ffs"), "Reunion", "0-4 -> 1-5" ),
	Code(1,1,"flr", "fr", "Francois Leroux's Canyoning Reunion",  NULL, new FLREUNION( "francois.leroux.free.fr/canyoning"), "Reunion"),
	Code(1,1,"agc", "fr", "Alpes-Guide.com",  NULL, new ALPESGUIDE( "alpes-guide.com/sources/topo/"), "France"),
	Code(1,1,"tahiti", "fr", "Canyon de Tahiti", NULL,new TAHITI( "canyon-a-tahiti.shost.ca"), "French Polynesia", "", "From PDF Index" ),
	Code(1,1,"mad", "fr", "An Kanion La-madinina", NULL,new MADININA( "ankanionla-madinina.com"), "French Caribbean", "", "From PDF Index" ),
	Code(1,1,"mur", "fr", "Mur d'Eau Caraibe", "Mur d'Eau Caraibe",new MURDEAU( "murdeau-caraibe.com"), "French Caribbean", "0-4 -> 1-5", "From KML data" ),

	// Swiss
	Code(1,1,"sch", "it", "SwissCanyon.ch", "SwissCanyon.ch",new BETAC( "swisscanyon.ch",SWISSCANYON_DownloadBeta), "Switzerland", "1-4 -> 2-5"),
	Code(1,1,"wch", "fr", "SwestCanyon.ch", "SwestCanyon.ch",new BETAC( "swestcanyon.ch",SWESTCANYON_DownloadBeta), "Switzerland", "1-4 -> 2-5" ),
	Code(1,1,"sht", "de", "Schlucht.ch",  NULL,new BETAC( "schlucht.ch",SCHLUCHT_DownloadBeta, NULL, SCHLUCHT_DownloadConditions), "Switzerland", "", "", "From Conditions" ),

	// Italian
	Code(1,1,"crc", "it", "CicaRudeClan.com", "CicaRudeClan.com",new BETAC( "cicarudeclan.com",CICARUDECLAN_DownloadBeta), "World", "1-3 -> 1.5-4.5" ),
	Code(1,1,"est", "it", "Canyoneast.it", "Canyoneast.it",new BETAC( "canyoneast.it",CANYONEAST_DownloadBeta), "Italy", "1-3 -> 3-5" ),
	Code(1,1,"tnt", "it", "TNTcanyoning.it",  NULL,new BETAC( "tntcanyoning.it",TNTCANYONING_DownloadBeta), "Europe" ),
	Code(1,1,"vwc", "it", "VerticalWaterCanyoning.com", "VerticalWaterCanyoning.com",new BETAC( "verticalwatercanyoning.com",VWCANYONING_DownloadBeta), "Europe", "0-4 / 1-7-> 1-5" ),
	Code(1,1,"aic", "it", "Catastoforre AIC-Canyoning.it",  NULL,new AICCATASTO( "catastoforre.aic-canyoning.it"), "Italy", "", "", "From Conditions" ),
	Code(1,1,"osp", "it", "OpenSpeleo.org",  NULL, new OPENSPELEO( "openspeleo.org/openspeleo/"), "Italy / Europe"),
	Code(1,1,"gul", "it", "Gulliver.it",  NULL, new GULLIVER( "gulliver.it"), "Italy / Europe"),
	Code(1,1,"cens", "it", "Cens.it",  NULL, new CENS( "cens.it"), "Italy"),
	//Code(1,"aicf", "it", "Forum AIC-Canyoning.it",  NULL, new AICFORUM ( "aic-canyoning.it/forum"), "Italy / Europe"),
	Code(1,1,"mac", "it", "Michele Angileri's Canyons",  NULL, new MICHELEA( "micheleangileri.com"), "Italy"),

	// Others
	Code(1,1,"ecdc", "pt", "ECDCPortugal.com",  NULL, new ECDC( "media.wix.com"), "Portugal"),
	Code(1,1,"cmad", "pt", "CaMadeira.com",  NULL, new CMADEIRA( "canyoning.camadeira.com"), "Madeira"),
	Code(1,1,"wro", "pl", "Canyoning.Wroclaw.pl", "Canyoning.Wroclaw.pl",new BETAC( "canyoning.wroclaw.pl",WROCLAW_DownloadBeta), "Europe", "1-5" ),
	
	// caves
	Code(1,1,"oc4", NULL, "OregonCaves4U.com",  NULL,new OREGONCAVES4U("oregoncaves4u.com"), "USA"),
	Code(1,1,"sph", NULL, "Speleosphere.org",  NULL,new BETAC( "speleosphere.org",SPHERE_DownloadBeta), "Guatemala" ),
	Code(1,1,"lao", NULL, "LaosCaveProject.de",  NULL,new BETAC( "laoscaveproject.de",LAOS_DownloadBeta), "Laos" ),
	//Code(1,"wpc", NULL, "Wikipedia Cave List",  NULL,new BETAC( "en.wikipedia.org/wiki", WIKIPEDIA_DownloadBeta, "World" ),
	//Code(8,"kpo", NULL, "KarstPortal.org",  NULL, KARSTPORTAL_DownloadBeta( "karstportal.org"), "World"),

	// Wikiloc
	Code(5,0,"wik", "auto", "Wikiloc.com", "Wikiloc.com",new BETAC( "wikiloc.com",WIKILOC_DownloadBeta, WIKILOC_ExtractKML), "World", "1-5", "From Custom data" ),

	// manual invokation
	Code(-1,0,"ccs", NULL, NULL, NULL,new CANYONINGCULT ( NULL), "Europe"),
	Code(-1,0,"bqnkml", NULL, NULL, NULL,new BETAC( NULL,BARRANQUISMOKML_DownloadBeta), "World" ),
	Code(-1,0,"bqndb", NULL, NULL, NULL, new BETAC( "BQN:", BARRANQUISMODB_DownloadBeta ), "World" ),
	Code(-1,0,"bqndbtest", NULL, NULL, NULL, new BETAC( "BQN:", BARRANQUISMODB_DownloadBeta ), "World" ),
	

	Code(-1,0, NULL,NULL,NULL,NULL,new BETAC(NULL), NULL)
};

//CSymList csvlist[sizeof(codelist)/sizeof(*codelist)];




int GetCode(const char *url) { 

	// patch!
	if (strstr(url, "canyoneeringusa.com/rave"))
		return 0;

	for (int i=1; codelist[i].code; ++i)
		{
		const char *ubase = codelist[i].betac->ubase;
		if (ubase && strstr(url, ubase ))
			{
			// special case BQN:
			if (ubase[strlen(ubase)-1]==':')
				return i;

			// bad urls
			vara urla(url, "/");
			if (urla.length()<3)
				return 0;
			return i;
			}
		}

	return 0;
}

const char *GetCodeStr(const char *url) {
	int c = GetCode(url);
	return c>0 ? codelist[c].code : NULL;
}






void LoadRWList()
{
	rwlist.Empty();
	rwlist.Load(filename(codelist[0].code));
	rwlist.Sort();

}

const char *GetPageName(const char *url)
{
	const char *sep = strrchr(url, '/');
	if (!sep) return NULL;
	return sep+1;
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
		if (b1->d>b2->d) return 1;
		if (b1->d<b2->d) return -1;
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
	if (rlen<=0 || lllen<=0) return 0;

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
	  while (*prstr!=0)
	  {
		register const char *pllstr = pllname;
		while (*pllstr!=0)
			{
			register int m;
			for (m=0; prstr[m]==pllstr[m] && prstr[m]!=0 && pllstr[m]!=0; ++m)
				if (!isletter(prstr[m]))
					perfect = TRUE;
			if (m>0 && m>=maxm)
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
	return sym->id + ":" + sym->GetStr(ITEM_DESC) + ":" + sym->GetStr(ITEM_REGION) + ": ["+ sym->GetStr(ITEM_AKA) +"]";
}

CString rwsym(PLLD &obj)
{
	return MkString(" %.1fkm %dL =", obj.d/1000.0, obj.len) + rwsym(obj.ll->ptr);
}






#define MATCHLISTSEP "@;"



void MatchList(PLLRectArrayList &llrlist, PLLArrayList &lllist, Code &code)
{
	// find list of closest points
	LLMatch<PLLRect,PLL> mlist(llrlist, lllist, addmatch);
	for (int i=0; i<llrlist.GetSize(); ++i)
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
		BOOL geoloc = strstr(prect->ptr->GetStr(ITEM_LAT), "@")!=NULL;
		int mmax = -1, pmax = 0, mmaxj = -1;

		//ASSERT( !strstr(prect->ptr->data, "Escalante Natural Bridge") );
		//ASSERT( !strstr(prect->ptr->id, "http://wikiloc.com/wikiloc/view.do?id=6486258") );
		//ASSERT(!(strstr(prname, "escalante batural bridge") && ln>5));


		CArrayList <PLLD> closest;
		int size = prect->closest.GetSize();
		for (int j=0; j<size; ++j) {
			double d = prect->closest[j].d;
			CSym *psym = prect->closest[j].ll->ptr; // rwsym
			BOOL pgeoloc = strstr(psym->GetStr(ITEM_LAT), "@")!=NULL;

			//ASSERT( !strstr(prname, "basse") || !strstr(psym->GetStr(ITEM_DESC), "Basse"));

			// calc match length and keep best match
			int AdvancedBestMatch(CSym &sym, const char *prdesc, const char *prnames, const char *prregion, int &perfect);
			int p, m = AdvancedBestMatch(*psym, prnamedesc, prname, prregion, p);
			// precise loc 
			int add = d<DIST15KM && closest.length()<15;
			// unprecise loc
			if (pgeoloc || geoloc) 
				add += d<DIST150KM && closest.length()<30 && m>=MINCHARMATCH && (m>=mmax || p);
			if (!add)
				continue;

			prect->closest[j].len = m;
			int added = closest.AddTail(prect->closest[j]);
			if ((m>mmax && !pmax) || (m>mmax && p && d<DIST15KM) || (m==mmax && p && !pmax) ) 
				mmax = m, mmaxj = added, pmax = p;
			}

		char c = '*';
		CString match, newmatch;
		vara matchlist;
		if (mmax<0 || closest.length()==0) // no candidates nearby, must be new location
			{
			match = prname, newmatch = "+";
			}
		else
			{	
			if (pmax) // perfect match
				match = rwsym(closest[mmaxj]), newmatch = "!" + rwsym(closest[mmaxj]);
			else	// multiple choice or not sure 
				{
				int nomatch = mmax<=MINCHARMATCH;
				if (nomatch) mmaxj = 0; // use closest match if not significant match
				match = rwsym(closest[mmaxj]);
				newmatch = (nomatch ? "?" : "~") + rwsym(closest[mmaxj]);
				//CRASHES! //Log(LOGINFO, "i=%d prname=%s", i, prname);
				}
			for (int j=0; j<closest.length(); ++j)
				{
				CString line = geoloc ? "@" : "";
				line += MkString("%c", j==mmaxj ? '>' : '#') + rwsym(closest[j]);
				matchlist.push(line);
				//fm.fputstr(line);
				}
			}
		//ASSERT(prect->ptr->id!="http://descente-canyon.com/canyoning/canyon-description/21850");		
		//ASSERT( !strstr(prect->ptr->GetStr(ITEM_DESC), "eras"));
		prect->ptr->SetStr(ITEM_MATCH, match);
		prect->ptr->SetStr(ITEM_NEWMATCH, newmatch);
		if (matchlist.length()>0)
			prect->ptr->SetStr(ITEM_MATCHLIST, matchlist.join(MATCHLISTSEP));
		}
}


int cmpmatchname( const void *arg1, const void *arg2)
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

	if (d1<d2) 
		return 1;
	if (d1>d2) 
		return -1;
	return 0;
}


int BestMatchSimple(const char *prname, const char *pllname, int *perfects = NULL) 
{
	int n = 0, m = 0, nskip = 0, mskip = 0;
	int nperfect = 0, npartial = 0;
	int mperfect = 0, mpartial = 0;
	while (prname[n]!=0 && pllname[m]!=0)
		{
		if (tolower(prname[n])!=tolower(pllname[m]))
			{
			// skip 1 char if both are  hel-lo hel lo
			if (!isanum(prname[n]) && !isanum(pllname[m]))
				{
				++n; ++m; 
				continue;
				}

			// skip 1 char on either one
			//if (!isanum(prname[n]) && !nskip)
			//	if (prname[n+1]==pllname[m])
			if (!isanum(prname[n]) && isanum(pllname[m]))
					{
					// skip 1 char
					++n; //++nskip;
					continue;
					}
			//if (!isanum(pllname[m]) && !mskip)
			//	if (pllname[m+1]==prname[n])
			if (isanum(prname[n]) && !isanum(pllname[m]))
					{
					// skip 1 char
					++m; //++mskip;
					continue;
					}

			break;
			}

		// equal chars
		nskip = mskip = 0;
		if (isanum(prname[n]))
			{
			if (!isanum(prname[n+1]) && !isanum(pllname[m+1]))
				nperfect = n+1, mperfect = m+1;
			npartial = n+1;
			}
		++n; ++m;
		}

	if (perfects)
		{
		int pr0 = prname[n]==0, pl0 = pllname[m]==0;
		pr0 = pr0 || !isanum(prname[nperfect]);
		pl0 = pl0 || !isanum(pllname[mperfect]);
		perfects[0] = pr0 ? nperfect : 0;
		perfects[1] = pl0 ? mperfect : 0;
		}
	return npartial;
}
/*
	int pr0 = prname[n]==0, pl0 = pllname[n]==0;
	if (wantperfect)
		{
		pr0 = pr0 || strchr(wantperfect, prname[n]);
		pl0 = pl0 || strchr(wantperfect, pllname[n]);
		if (perfects)
			{
			perfects[0] = pr0;
			perfects[1] = pl0;
			}
		if (pr0 && pl0)
			while (strchr(wantperfect, prname[n]))
				--n;
		}
	if (pr0 && pl0)
		{
		perfect = ++n;
		*perfectn = perfect;
		}
*/


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
		for (int j=0; j<rwlist.GetSize(); ++j) {
			CSym *psym = &rwlist[j];
			if (region && *region!=0 && *region!=';')
				{
				// must match region
				if(!strstri(GetFullRegion(psym->GetStr(ITEM_REGION), regions), region))
					continue;
				}
			//if (strstr(psym->data, "nfierno"))
			//	name = name;
			// calc match length and keep best match
			vars matchp;
			int perfect = 0, perfect2 = 0;
			int m = BestMatch(psym->GetStr(ITEM_BESTMATCH), name, perfect);
			if (m<MINCHARMATCH)
				continue;
			int m2 = BestMatchSimple(psym->GetStr(ITEM_DESC), sym.GetStr(ITEM_DESC));
			if (perfect || m>=MINCHARMATCH)
				matchlist.push( matchp = MkString("%s %2.2dL = %s", perfect ? "!" : "~", m2>m ? m2 : m, rwsym(psym)) );
			if (m>mmax)
				mmax = m, mmaxj = j, match = matchp;
			}

		// keep only best matches
		if (matchlist.length()>1)
			{
			for (int i=matchlist.length()-1; i>=0; --i)
				{
				const char *mi = matchlist[i]; 
				if (CDATA::GetNum(mi+1)<mmax)
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
		for (int i=0; i<reslist.GetSize(); ++i)
			matchlist.push( MkString("%s %2.2dL = %s", perfect>0 ? "!" : "~", (int)reslist[i].GetNum(M_SCORE), rwsym(&reslist[i])) );
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
			if (addlist.length()==0)
				{
				sym.SetStr(ITEM_MATCH, name);
				sym.SetStr(ITEM_NEWMATCH, "+");
				}
			else
				{
				sym.SetStr(ITEM_MATCH, addlist[0]);
				sym.SetStr(ITEM_NEWMATCH, "="+addlist[0]);
				}
			}
}




void LoadNameList(CSymList &rwnamelist)
{
	rwnamelist.Empty();
	for (int i=0; i<rwlist.GetSize(); ++i)
		rwnamelist.Add(CSym(rwlist[i].GetStr(0), rwlist[i].id));
	rwnamelist.Sort();
}

int LoadBetaList(CSymList &bslist, int title, int rwlinks)
{
		// initialize data structures 
		if (rwlist.GetSize()==0)
			LoadRWList();

		if (rwlist.GetSize()==0)
			return FALSE;

		rwlist.Sort();
		
		// bslist
		for (int r=0; r<rwlist.GetSize(); ++r) 
		{
			vars id = title ? rwlist[r].GetStr(ITEM_DESC) : rwlist[r].id;
			vara urllist(rwlist[r].GetStr(ITEM_MATCH), ";");
			for (int u=0; u<urllist.length(); ++u)
				bslist.Add(CSym(urlstr(urllist[u]), id));
			if (rwlinks)
				{
				vars title = rwlist[r].GetStr(ITEM_DESC);
				bslist.Add(CSym(urlstr("http://ropewiki.com/"+title.replace(" ","_")), id));
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
	if (bslist.GetSize()==0) {
		LoadBetaList(bslist);

		// LLList
		//lllist.SetSize(rwlist.GetSize());
		for (int i=0; i<rwlist.GetSize(); ++i) {
			printf("%d/%d Loading bslist...         \r", i, rwlist.GetSize());
			CSym &sym = rwlist[i];
			sym.SetStr(ITEM_BESTMATCH, GetBestMatch(sym));
			double lat = sym.GetNum(ITEM_LAT);
			double lng = sym.GetNum(ITEM_LNG);		
			if (CheckLL(lat,lng))
				lllist.AddTail( PLL(LL(lat, lng), &sym) );
		}

	}

	CArrayList <CSym *> psymlist;
	for (int s=0; s<symlist.GetSize(); ++s)
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
		if (IsSimilar(sym.id, RWID) && rwlist.Find(sym.id)>=0)
			id = sym.id;
		tmp = sym.GetStr(ITEM_INFO);
		if (IsSimilar(tmp, RWID) && rwlist.Find(tmp)>=0)
			id = tmp;
		if (IsSimilar(sym.id, RWTITLE))
			id = sym.id;

		// always match RWID:
		if (!id.IsEmpty() || IsSimilar(sym.id, RWTITLE))
			{
			sym.SetStr(ITEM_MATCH, id+RWLINK);			
			sym.SetStr(ITEM_NEWMATCH, "");			
			sym.SetStr(ITEM_MATCHLIST,"");
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
	while (i<size ) {
		printf("%d/%d Matching bslists...         \r", i, size);
		CSym &sym = *psymlist[i];
		sym.SetStr(ITEM_MATCH,"");
		sym.SetStr(ITEM_NEWMATCH,"");
		sym.SetStr(ITEM_MATCHLIST,"");
		sym.SetStr(ITEM_BESTMATCH, GetBestMatch(sym));
#if 1
#if 1
		if (bi<bslist.GetSize()) {
			vara rwids;
			int cmp;
			for (int dup=0; (cmp=strcmp(sym.id, bslist[bi].id))==0; ++dup, ++bi)
				{
				// in bslist and symlist
				// bsym-->sym  sym-->bsym
				rwids.push(bslist[bi].GetStr(ITEM_DESC)+RWLINK);
				}
			if (rwids.length()>0)
				{
				// in bslist and symlist
				// bsym-->sym  sym-->bsym
				sym.SetStr(ITEM_MATCH, rwids.join(";"));
				++i; continue;
				}
			if (cmp>0) 
				{
				// in bslist but not in symlist
				++bi; continue;
				}
		}
#else
		if (bi<bslist.GetSize()) {
			CSym &bsym = bslist[bi];
			int cmp = strcmp(sym.id, bsym.id);
			if (cmp==0) {
				// in bslist and symlist
				// bsym-->sym  sym-->bsym
				sym.SetStr(ITEM_MATCH, bsym.GetStr(ITEM_DESC)+RWLINK);
				++i; ++bi; continue;
			}
			if (cmp>0) {
				// in bslist but not in symlist
				++bi; continue;
			}
		}
#endif
#else
		vara ids;			
		for (int cmp=0; (bi<bslist.GetSize() && (cmp=strcmp(sym.id, bslist[bi].id))>=0); ++bi)
			if (cmp==0) 
				ids.push(bslist[++bi].GetStr(ITEM_DESC)+RWLINK);
		if (ids.length()>0) {
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
		if (lat!=InvalidNUM && lng==InvalidNUM)
			lng = CDATA::GetNum(GetToken(slat,1,';'));
		if (lat==InvalidNUM && slat[0]=='@')
			_GeoCache.Get(slat.Mid(1), lat, lng);
		if (CheckLL(lat,lng))
			llrlist.AddTail( PLLRect(lat, lng, DIST150KM, &sym) ); // 150km
		++i;
	}

	MatchList(llrlist, lllist, code);

	// match by name and region
	for (int i=0; i<symlist.GetSize(); ++i) {
		CSym &sym = symlist[i];
		if (sym.id.IsEmpty())
			continue;
		if (IsSimilar(sym.id, RWID) || IsSimilar(sym.id, RWTITLE))
			continue;
		if (strstr(sym.GetStr(ITEM_MATCH), RWLINK))
			continue;
		if (sym.GetNum(ITEM_LAT)!=InvalidNUM && sym.GetNum(ITEM_LNG)!=InvalidNUM)
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
	if (num==InvalidNUM)
		return InvalidNUM;
	if (strchr(str, 'd'))
		return num*24;
	return num;
}


BOOL InvalidUTF(const char *txt)
{
	const char *inv[] = { "\xef\xbf\xbd", NULL };
	for (int v=0; inv[v]!=NULL; ++v)
		if (strstr(txt, inv[v]))
			return TRUE;
	return FALSE;
}


vars ProcessAKA(const char *str)
{
	vars ret;
	int n = 0, i = 0, io = 0;
	const char *o[] = { " o ", " O ", "/", NULL };
	while (str[i]!=0)
		{
		if (i>0 && str[i]==' ' && isa(str[i-1]) && (n=IsMatchN(str+i, o))>=0 && isa(str[i+strlen(o[n])])) 
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
	for (int i=0; i<list.GetSize(); ++i)
		{
		const char *strid = list[i].id;
		while (*strid==' ') ++strid;
		int l;
		for (l=0; isanum(str[l]) && tolower(str[l])==tolower(strid[l]); ++l);
		if (!isanum(str[l]) && !isanum(strid[l]))
			return TRUE;
		}
	return FALSE;
}

vars Capitalize(const char *str)
{
	static CSymList sublist, midlist, keeplist;
	if (sublist.GetSize()==0)
		sublist.Load(filename(TRANSBASIC));
	if (midlist.GetSize()==0)
		midlist.Load(filename(TRANSBASIC"MID"));
	if (keeplist.GetSize()==0)
		keeplist.Load(filename(TRANSBASIC"SPEC"));

	vars out;
	int skip = FALSE;
	int n1 = 0;
	const char *c1 = NULL;
	while (*str!=0)
		{
		int icase = 0;
		const char *c = str;
		int n = IsUTF8c(c);
		if (isa(str[0]) || n>1)
			{
			// letter / UTF
			if (!skip)
				{
				icase = -1;
				if (!c1 || !(isa(c1[0]) || n1>1))
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
							if (c1 && c1[0]=='(')
								icase = 0; // original
							else
								if (c1)
									icase = -1; // midlist = lower
					}
				if (c1 && c1[0]=='\'')  // Captain's  d'Ossule
					icase = (tolower(str[0])=='s' && !isa(str[1]) && IsUTF8(str+1)<=1) ? -1 : 1;
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

		if (n<=1)
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
	if (midlist.GetSize()==0)
		midlist.Load(filename(TRANSBASIC"MID"));
	vara add(ProcessAKA(stripHTML(oldval+CString(";")+newval)).replace("' ","'"), ";");		
	for (int j=0; j<add.length(); ++j)
		if (!add[j].Trim().IsEmpty())
				{
				vars &aka = add[j];
				if (InvalidUTF(aka))
					continue;
				if (strstr(aka, "*") && CDATA::GetNum(aka)>=0)
					continue;
				if (isupper(aka[0]) && isupper(aka[1]))
					aka = Capitalize(aka);
				aka.Replace("\"", "\'");
				aka.Replace("{", "");
				aka.Replace("}", "");
				while (aka.Replace("\'\'", "\'"));
				vars aka1 = GetToken(aka,0,"()").Trim();
				vars aka2 = GetToken(aka,1,"()").Trim();
				if (strstri(added, aka1) && strstri(added, aka2))
					continue;
				added += "; "+aka;
				for (int i=0; i<midlist.GetSize(); ++i)
					if (strstri(aka, midlist[i].id))
						added += "; "+aka.replace(midlist[i].id," ");
				list.Add(aka);
				}
	if (nohead)
		list.RemoveAt(0);
	return list.join("; ");
}






vars skipItalics(const char *oldstr)
{
	const char *str = oldstr;
	const char *start[] =  { "<i>", "&lt;i&gt;", NULL };
	const char *end[] =  { "</i>", "&lt;/i&gt;", NULL };
	vars ret;
	int skip = FALSE;
	while (*str!=0) {
		int len = 0; 
		if ((len=IsMatch(str, start))>0) {
			++skip;
			str += len;
			continue;
			}
		if ((len=IsMatch(str, end))>0) {
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

vars getfulltext(const char *line, const char *label = "fulltext=")
{
	vars res = ExtractString(line,label);
	res.Replace(",", "%2C");
	return htmltrans(res);
}

vara getfulltextmulti(const char *line)
{
		vara list;
		vara values(line, "<value ");
		for (int i=1; i<values.length(); ++i)
			list.push(getfulltext(values[i]));
		return list;
}

vars getfulltextorvalue(const char *line)
{
	vars res =  getfulltextmulti(line).join(";");
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
	for (int v=0; v<vala.length(); ++v)
		vala[v] = urlstr(vala[v], FALSE);
	return vala.join(";");
}


int ROPEWIKI_DownloadItem(const char *line, CSymList &symlist)
			{
			vara labels(line, "label=\"");
			vars id = ExtractString(labels[1], "Has pageid", "<value>", "</value>");
			if (id.IsEmpty()) {
				Log(LOGWARN, "Error empty ID from %.50s", line);
				return FALSE;
				}
			CSym sym(RWID+id);
			vars name = getfulltext( line );
			sym.SetStr(ITEM_DESC, name);
			//ASSERT(!IsSimilar(sym.GetStr(ITEM_DESC), "Goblin"));
			sym.SetStr(ITEM_LAT, ExtractString(line,"lat=") );
			sym.SetStr(ITEM_LNG, ExtractString(line,"lon=") );
			for (int l=3, m=ITEM_LNG; l<labels.length(); ++l, ++m) {
				vars val = getlabel(labels[l]);
				switch (m)
				{
				case ITEM_LNG:
					// geolocation
					if (!val.IsEmpty())
						sym.SetStr(ITEM_LAT, sym.GetStr(ITEM_LAT)+"@"+val);
					continue;
				case ITEM_NEWMATCH:
					// trip reports
					if (!val.IsEmpty())
						sym.SetStr(ITEM_MATCH, sym.GetStr(ITEM_MATCH)+";"+val);
					continue;
				case ITEM_RAPS:
					val.Trim("r");
					break;
				case ITEM_REGION:
					val.Replace("[[","");
					val.Replace("]]","");
					val.Replace(":","");
					val = GetToken(val, 0, '|');
					val = GetToken(val, 1, '(') + ";" + GetToken(val, 0, '(');
					val.Trim(" ;");
					break;
				case ITEM_AKA:
					val = name + (val.IsEmpty() ? "" : "; " + val);
					break;
				default:
					break;
				}

				sym.SetStr(m, val );
			}
			//ASSERT(strstr(sym.data, "Snowflake")==NULL);
			// processing
			//GetSummary(sym, stripHTML(skipItalics(sym.GetStr(ITEM_ACA))) );
		
			Update(symlist, sym, FALSE);
			return TRUE;
			}

vars ROPEWIKI_DownloadList(const char *timestamp, CSymList &symlist, const char *q, const char *prop, rwfunc func)
{
	CString query = MkString("%s[[Max_Modification_date::>%s]] OR %s[[Modification_date::>%s]]", q, timestamp, q, timestamp);
	CString querysort = "|sort=Modification_date|order=asc";
	return GetASKList(symlist, query+prop+querysort, func);
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
		for (int i=1; i<labels.length(); ++i)
			sym.SetStr(i-1, getfulltextorvalue(labels[i]));

		Update(regions, sym, FALSE);
		return TRUE;
		}


int ROPEWIKI_DownloadBeta(const char *ubase, CSymList &symlist) 
{
	/*
	CString timestampurl= base+"|%3FModification_date|sort=Modification_date|order=desc|limit=1";
	if (f.Download(timestampurl))
		{
		Log(LOGERR, "ERROR: can't download url %.128s", timestampurl);
		return FALSE;
		}
	double timestamp = ExtractNum(f.memory, "<results>", "<value>", "</value>");
	*/	

	// sym
	const char *STAMP0 = "2000-01-01T00:00:00Z";
	CString timestamp = STAMP0;
	CString hdr = GetToken(symlist.header,0);
	if (IsSimilar(hdr, "TS:"))
		timestamp = hdr.Mid(3);

	vars newtimestamp = ROPEWIKI_DownloadList(timestamp, symlist, "[[Category:Canyons]]", "|%3F"+vara(rwprop,"|").join("|%3F"), ROPEWIKI_DownloadItem);
	if (!newtimestamp.IsEmpty())
		symlist.header = "TS:"+newtimestamp+","+GetTokenRest(symlist.header,1);		

	// regions
	CSymList regionlist;
	regionlist.Load(filename(RWREGIONS));
	if (!CFILE::exist(filename(RWREGIONS)))
		timestamp = STAMP0;;
	ROPEWIKI_DownloadList(timestamp, regionlist, "[[Category:Regions]]", "|%3FLocated in region|%3FIs major region", rwfregion);
	regionlist.Save(filename(RWREGIONS));

	return TRUE;
}


int ROPEWIKI_DownloadKML(const char *line, CSymList &symlist)
{
			vars file = getfulltext(line);
			const char *ext = GetFileExt(file);
			if (!IsSimilar(file.Right(4),".kml"))
				return TRUE;

			vars mod = ExtractString(line, "<value>", "", "</value>");
			symlist.Add(CSym(file,mod));
			return TRUE;
}

int ROPEWIKI_DownloadKML(CSymList &symlist) 
{
	CString timestamp = "1";
	CString hdr = GetToken(symlist.header,0);
	if (IsSimilar(hdr, "TS:"))
		timestamp = hdr.Mid(3);

	CString query = MkString("[[File:%%2B]][[Modification_date::>%s]]|%%3FModification_date", timestamp);
	timestamp = GetASKList(symlist, query+"|sort=Modification_date|order=asc", ROPEWIKI_DownloadKML);

	if (!timestamp.IsEmpty())
		symlist.header = "TS:"+timestamp+","+GetTokenRest(symlist.header,1);
	return TRUE;
}






int rwxredirect(const char *line, CSymList &redirects)
		{
		vars id = htmltrans(ExtractString(line, "", "pageid=\"", "\""));
		vars name = htmltrans(ExtractString(line, "", "title=\"", "\""));
		if (name.IsEmpty() || id.IsEmpty()) {
			Log(LOGWARN, "Error empty ID/name from %.50s", line);
			return FALSE;
			}
		if (strstri(name, DISAMBIGUATION))
			return FALSE;
		vara aka;
		vara list(line, "<lh ");
		for (int i=1; i<list.length(); ++i)
			aka.push(ExtractString(list[i], "", "title=\"", "\""));

		CSym sym(RWID+id, name);
		sym.SetStr(ITEM_CLASS, "-1:redirect");
		sym.SetStr(ITEM_AKA, aka.join(";"));
		Update(redirects, sym, FALSE);
		return TRUE;
		}

int ROPEWIKI_DownloadRedirects(const char *ubase, CSymList &symlist) 
{
	CSymList redirects;
	GetAPIList(redirects, "generator=allredirects&garunique&prop=linkshere&lhshow=redirect&lhnamespace=0&garnamespace=0&lhlimit=1000&garlimit=1000", rwxredirect);
	//GetAPIList(redirects, "generator=recentchanges&grcshow=redirect&grctoponly&grcnamespace=0&prop=links&grclimit=1000&grcend="+timestamp, rwxredirect);  Max 30 days!?!
	//GetAPIList(redirects, "generator=allredirects&garnamespace=0&garlimit=100&prop=links&pllimit=100&plnamespace=0", rwxredirect);

	redirects.Sort();
	for (int i=0; i<redirects.GetSize(); ++i)
		{
		CSym &isym = redirects[i];
		vars title = isym.GetStr(ITEM_DESC);
		for (int j=0; j<redirects.GetSize(); ++j)
			if (i!=j)
				{
				CSym &jsym = redirects[j];
				vara aka( jsym.GetStr(ITEM_AKA), ";" );
				if (aka.indexOf(title)<0)
					continue;

				// merge syms
				aka.push( isym.GetStr(ITEM_AKA) );
				jsym.SetStr(ITEM_AKA, aka.join(";"));
				redirects.Delete(i--);
				}
		}

	// always download new
	if (redirects.GetSize()<1)
		return FALSE;
	
	symlist = redirects;
	return TRUE;
}













int ComparableTimes(double tmin, double tmax, double t)
{
	double range = 0.5; // 50%
	if (tmin<0 || t<0) // uncomparable
		return TRUE;

	if (t<tmin)
		{
		double d = fabs(tmin-t);
		if (d>2 && d/tmin>range)
			return FALSE;
		}

	if (t>tmax)
		{
		double d = fabs(tmax-t);
		if (d>2 && d/tmax>range)
			return FALSE;
		}

	return TRUE;
}



CString GetTime(double h)
{
	if (h<=0)
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
		if (good.GetSize()>0)
			{
			}
		if (bad.GetSize()>0)
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

		if (good.GetSize()==0 && bad.GetSize()==0)
			{
				good.Load(filename(LINKSGOOD));
				good.Sort();
				bad.Load(filename(LINKSBAD));
				bad.Sort();
			}

		printf("checking urls %dgood %dbad             \r", good.GetSize(), bad.GetSize());

		// new good links
		for (int l=0; l<linklist.length(); ++l)
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
			for (int g=0; ignorelist[g] && !ignore; ++g)
				if (strstr(url, ignorelist[g]))
					ignore = TRUE;
			const char *code = GetCodeStr(url);
			if (ignore || code)
				continue;

			//if already listed, skip it
			if (existlink.indexOf(url)>=0)
				continue;

			// check others
			int ok = INVESTIGATE==0 || good.Find(url)>=0;
			if (!ok)
				{
				if (INVESTIGATE<1 && bad.Find(url)>=0)
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
		 url = "http://maps.google.com/maps/ms?ie=UTF8&hl=en&msa=0&output=kml&forcekml=1&msid="+msid;
	   if (!mid.IsEmpty())
		 url = "http://www.google.com/maps/d/u/0/kml?forcekml=1&mid="+mid;

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
		if (CDATA::GetNum(rwdata[ITEM_LAT])==InvalidNUM || rwdata[ITEM_LAT].indexOf("@")>=0)
			{
			if (CDATA::GetNum(symdata[ITEM_LAT])!=InvalidNUM) // new coordinates
				++update;
			if (CDATA::GetNum(symdata[ITEM_LNG])!=InvalidNUM) // new coordinates
				++update;
			if (symdata[ITEM_LAT].indexOf("@")>=0 && rwdata[ITEM_LAT].indexOf("@")<0) // new geolocation
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
		if (!strstr(symdata[ITEM_MATCH],RWID))
			symdata[ITEM_MATCH] = translate.Name(symdata[ITEM_DESC]);
		else
			symdata[ITEM_REGION] = regionmatch(symdata[ITEM_REGION], symdata[ITEM_MATCH]);

		if (translate.goodtitle)
			{
			BOOL skip = FALSE;
			vars title = stripAccents(sym.GetStr(ITEM_DESC));
			if (translate.goodtitle<0) // capitalize if needed
				title = Capitalize(title);
			//ASSERT(!strstr(title, "parker"));
			for (int s=0; andstr[s]!=NULL && !skip; ++s)
				if (strstri(title, andstr[s]))
					skip = TRUE;
			const char *skipstr[] = { " loop",  " prospect", " area", " by ", " from ", " then ", " via ", NULL };
			for (int s=0; skipstr[s]!=NULL && !skip; ++s)
				if (strstri(title, skipstr[s]))
					skip = TRUE;
			if (!IsUTF8(title))
				{
				Log(LOGERR, "Invalid UTF8 Title for '%s' CSym:%s", sym.GetStr(ITEM_DESC), sym.Line());
				skip = TRUE;
				}
			if (!skip) // only first name (main name), to avoid region or other
				symdata[ITEM_AKA] = UpdateAKA(symdata[ITEM_AKA], GetToken(title,0, ";,%[{"));
			}
	
		// compare rest of value
		for (int v=ITEM_REGION; v<ITEM_BETAMAX; ++v)
			{
			if (symdata[v].IsEmpty())
				continue;
			
			switch (v)  {

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
						if (strcmp(rwdata[v],"0")==0 && strcmp(symdata[v],"0")!=0)
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
							for (int r=0; r<R_SUMMARY; ++r)
								if (!rwsum[r].IsEmpty())
									symsum[r] = "";
								else
									if (!symsum[r].IsEmpty())
										++updatesum;
							symdata[v] = updatesum>0 ? symsum.join(";") : "";
							if (updatesum>0)
								++update;
						}
						break;

					case ITEM_AKA:
						{
						vara addlist;
						vara symaka(symdata[ITEM_AKA], ";");
						for (int i=0; i<symaka.length(); ++i) {
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
						vara oldaka(UpdateAKA( rwdata[ITEM_AKA], "" ), ";");
						vara newaka(UpdateAKA( rwdata[ITEM_AKA], addlist.join(";")), ";");
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
						unit *units = v==ITEM_LENGTH ? udist : ulen;
						double error = v==ITEM_LENGTH ? km2mi/1000 : m2ft;
						if (CDATA::GetNum(rwdata[ITEM_CLASS])==2)
							{
							double rwv = GetNumUnit(rwdata[v], units);
							double symv = GetNumUnit(symdata[v], units);
							if (rwv+error < symv)
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
							if (tmax<=0) tmax = tmin;
							if (tmin2<=0) tmin2 = tmin;
							if (tmax2<=0) tmax2 = tmin2;
							// integrity check							
							if (!ComparableTimes(tmin,tmax,tmin2) || !ComparableTimes(tmin,tmax,tmax2))
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
							if (tmin2>0)
								if (tmin<=0 || tmin2<tmin) 
									tmin=tmin2;
							if (tmax2>0)
								if (tmax<=0 || tmax2>tmax) 
									tmax=tmax2;
							if (tmin==tmin1 && tmax==tmax1) {
								symdata[_MINTIME] = symdata[_MAXTIME] = "";
								}
							else 
								{
								symdata[_MINTIME] = GetTime(tmin);
								symdata[_MAXTIME] = GetTime(tmax);
								if (tmin==tmax || tmax<=0) {
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
							if (!strstr(rwdata[ITEM_MATCH],KMLIDXLink(sym.id, symdata[v])))
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
						if (IsSimilar(symdata[v],"No"))
							symlen = 0;
						if (IsSimilar(symdata[v],"Yes"))
							symlen = 1;
						if (IsSimilar(symdata[v],"Opt"))
							symlen = 0.5;
						if (symlen<0)
							{
							Log(LOGWARN, "Skipping invalid shuttle length '%s' for %s", symdata[v], rwsym.GetStr(ITEM_DESC));
							symdata[v] = "";
							continue;
							}
						vars sep = "!!!";
						if (rwlen>=0) // pre-existing set
							{
							if (rwlen==0 && symlen>=1)
								{
								rwdata[v] = "";
								if (symlen>1)
									symdata[v] = "Optional;"+symdata[v];
								else
									symdata[v] = "Optional;"+sep+symdata[v];
								++update;
								}
							else if (rwlen>=1 && symlen==0)
								{
								rwdata[v] = "";
								symdata[v] = "Optional;"+sep+symdata[v];
								++update;
								}
							else if (rwlen>=1 && rwlen<symlen)
								{
								rwdata[v] = "";
								symdata[v] = "Required;"+symdata[v];
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
							if (symlen>=1) // > 1 to be required
								{
								symdata[v] = "Required;"+symdata[v];
								++update;
								}
							else if (symlen>=0.5) // > 1 to be required
								{
								if (symlen>1)
									symdata[v] = "Optional;"+symdata[v].replace("Optional","");
								else
									symdata[v] = "Optional;"+sep+symdata[v];
								++update;
								}
							else
								{
								symdata[v] = "None;"+sep+symdata[v];
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
						if (strcmp(rwdata[v],"0")==0 && strcmp(symdata[v],"0")!=0)
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
			if (list.length()>1)
				chgsym.SetStr(ITEM_MATCHLIST, rsep+list.join(rsep));
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
	if (invalids.Find(url)<0)
		{
		invalids.Add(CSym(url,title));
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
	if (MODE>0)
		MatchList(nlist, cod);

	// find bad links
	for (int i=0; i<_bslist.GetSize(); ++i)
		{
		CString &title = _bslist[i].GetStr(ITEM_DESC);
		CString &link = _bslist[i].id;
		int c = GetCode(link);
		if (c>0 && &codelist[c]==&cod)
			if (nlist.Find(link)<0)
				BSLinkInvalid(title, link);
		}


	// find changes
	CSym usym;
	for (int i=0; i<nlist.GetSize(); ++i)
		if (Update(olist, nlist[i], &usym, FALSE)) {
			if (ignorelist.Find(nlist[i].id)>=0)
				continue;

			// cond_ing RWTITLE:
			if (IsSimilar(usym.id,RWTITLE))
				continue;

			// changes 
			vars id = urlstr(usym.GetStr(ITEM_MATCH));
			vars match = usym.GetStr(ITEM_NEWMATCH);

			double c = usym.GetNum(ITEM_CLASS);
			if (c!=InvalidNUM) {
				if (c<1) {
					BOOL skip = TRUE;
					// skip anything not canyoneering except for perfect matches that are not duplicate links
					if (strstr(id, RWID)) {
						if (strstr(id, RWLINK))
							skip = FALSE; // don't skip if already linked
						if (strchr(match,'!')) // || strchr(match,'~'))
								if (nlist.FindColumn(ITEM_MATCH, id+RWLINK)<0)
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
				for (int r=1; r<rwids.length(); ++r)
					{
					vars rid = RWID+CData(CDATA::GetNum(rwids[r]));
					usym.SetStr(ITEM_MATCH, rid+RWLINK);
					int f = rwlist.Find(rid); 
					if (f<0) {
						Log(LOGERR, "Mismatched MATCH %s for %s", id, usym.Line());
						continue;
					}
					// MATCHED!
					rwsym = rwlist[f];
					if (CompareSym(usym, rwsym, chgsym, cod))
						{
						if (!cod.betac->ubase && strstr(usym.GetStr(ITEM_MATCH),RWLINK))
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
		CSymList namelist, dupnamelist,  rnamelist;
		for (int i=0; i<rwlist.GetSize(); ++i) {
			CString name = GetToken(rwlist[i].GetStr(ITEM_DESC),0,"[{(").Trim();
			rnamelist.Add(CSym(name));
		}
		for (int i=0; i<chglist.GetSize(); ++i) {
			if (chglist[i].id.IsEmpty())
				continue;
			CString name = chglist[i].GetStr(ITEM_MATCH);
			if (IsSimilar(name, RWID))
				continue;
			namelist.Add(CSym(name));
		}
		namelist.Sort(); rnamelist.Sort();
		for (int i=1; i<namelist.GetSize(); ++i)
			if (namelist[i-1].id==namelist[i].id)
				dupnamelist.Add(namelist[i]);

		for (int i=0; i<chglist.GetSize(); ++i) {
			if (chglist[i].id.IsEmpty())
				continue;
			CString name = chglist[i].GetStr(ITEM_MATCH);
			if (name.IsEmpty())
				continue;
			if (IsSimilar(name, RWID))
				continue;
			CString dup = "";
			if (dupnamelist.Find(name)>=0)
				dup += "N";
			if (rnamelist.Find(name)>=0)
				dup += "O";
			if (!dup.IsEmpty())
				chglist[i].SetStr( ITEM_NEWMATCH, dup + chglist[i].GetStr(ITEM_NEWMATCH) );
			}
		}


	// accumulate prioretizing by match

	CSymList chglists[5][2];
	for (int i=0; i<chglist.GetSize(); ++i)
		{
		CSym sym = chglist[i];
		CString match = sym.GetStr(ITEM_NEWMATCH);
		const char *num = match;
		while (*num!=0 && !isdigit(*num))
			++num;
		sym.SetNum(ITEM_BETAMAX, CGetNum(num));
		BOOL km = strstr(match, "km ")!=NULL;
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
	for (int i=0; i<sizeof(chglists)/sizeof(chglists[0]); ++i)
		{
		chglists[i][0].SortNum(ITEM_BETAMAX, -1);
		chglists[i][1].SortNum(ITEM_BETAMAX, 1);

		if (cod.IsWIKILOC() && i==1)
			{
			int km = 1;
			vara rwids;
			for (int j=0; j<chglists[i][km].GetSize(); ++j)
				{	
				vars match = chglists[i][km][j].GetStr(ITEM_NEWMATCH);
				vars rwid = RWID+ExtractString(match, RWID, "", ":");
				// check saturated match
				int f = rwlist.Find(rwid);
				if (f>=0)
					{
					vara wikilinks(rwlist[f].GetStr(ITEM_MATCH), "wikiloc.com");
					if (wikilinks.length()>MAXWIKILOCLINKS)
						{
						chglists[i][km].Delete(j--);
						continue;
						}
					}
				// check already matched
				if (rwids.indexOf(rwid)>=0)
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
	if (f>0)
		return list[f].Line();
	else
		return MkString("ERROR: Could not find id '%s'", str);
}

int CheckBeta(int mode, const char *codeid)
{
	rwlist.Load(filename(codelist[0].code));
	rwlist.Sort();

	int run = 0;
	for (int i=1; codelist[i].code!=NULL; ++i) {
		if (codeid && *codeid)
			{
			if (stricmp(codeid, codelist[i].code)!=0)
				continue;
			}
		CSymList symlist;
		symlist.Load(filename(codelist[i].code));
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
		MatchList(symlist, codelist[i]);

		// find duplicates
		CSymList dup;
		for (int l=0; l<symlist.GetSize(); ++l) {
			CString id = symlist[l].GetStr(ITEM_MATCH);
			if (!strstr(id, RWLINK))
				continue;
			dup.Add(CSym(id, symlist[l].id));
		}
		dup.Sort();
		for (int l=1; l<dup.GetSize(); ++l)
			if (dup[l-1].id==dup[l].id)
				Log(LOGINFO, "Duplicate link %s\n%s\n%s\n%s", dup[l].id, 
				findsym(rwlist, vars(dup[l].id).replace(RWLINK,"")),
				findsym(symlist, dup[l-1].data), 
				findsym(symlist, dup[l].data));

		DownloadFile f;
		if (MODE>0)
			for (int l=0; l<symlist.GetSize(); ++l) 
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
	if (codeid && *codeid==0)
		codeid = NULL;
	CString symheaders = headers;
	symheaders.Replace("ITEM_", "");
	//chglist.Load(filename(CHGFILE));
	DeleteFile(filename(MATCHFILE));
	
	int run = 0, cnt = 0;
	for (int order=codeid ? -1 : 0; order<10; ++order)
	  for (int i=0; codelist[i].code!=NULL; ++i) {
		if (order!=codelist[i].order)
			continue;
		if (codeid && stricmp(codeid, codelist[i].code)!=0)
			continue;
		if (!codeid && !codelist[i].betac->ubase)
			continue;
		if (mode==1 && i==0)
			continue;
 
		++run;
		Log(LOGINFO, "Running Beta %s MODE %d", codelist[i].code, mode);

		// load code list
		CSymList symlist;
		symlist.header = symheaders;
		symlist.Load(filename(codelist[i].code));
		//symlist.Sort(strcmp(codelist[i].code,WIK)==0 ? cmpclass : cmpdata);
		//symlist.Sort();

		// run process
		int ret = -1;
		int counter = 0;
		CSymList oldlist;
		if (MODE<1)
			{
			oldlist = symlist;
			ret = codelist[i].betac->DownloadBeta(symlist);
			if (i>0)
				{
				for (int s=0; s<symlist.GetSize(); ++s)
					if (symlist[s].index!=0)
						++counter;
				}
			}

		int chg = UpdateChanges(oldlist, symlist, codelist[i]);
		Log(LOGINFO, "CHG.CSV : [%dD/%dT] %s => %d changes/additions from #%d %s [%s]", counter, symlist.GetSize(), ret ? "OK" : "ERROR!!!", chg, ++cnt, codelist[i].code, codelist[i].betac->ubase);
		if (i>0 && mode<0 && counter<symlist.GetSize()/2)
			Log(LOGERR, "ERROR: Download malfunction for code %s [%s] (D<T/2)", codelist[i].code, codelist[i].betac->obase);

		// check duplicates
		symlist.Sort();
		for (int j=symlist.GetSize()-1; j>0; --j)
			if (symlist[j-1].id==symlist[j].id)
				{
				Log(LOGERR, "Deleting duplicated id %s", symlist[j].id);
				symlist.Delete(j);
				}

		// save
		symlist.Sort(codelist[i].IsWIKILOC() ? cmpclass : cmpdata);
		symlist.Save(filename(codelist[i].code));
		//if (!codelist[i].base) // rw
		//	MoveFileEx(filename(CHGFILE), filename(CString(CHGFILE)+codelist[i].code), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH );
		}

	if (!run) {
		vara all;
		for (int i=0; codelist[i].code!=NULL; ++i)
			all.push(codelist[i].code);
		Log(LOGERR, "Wrong code %s [%s]", codeid, all.join(" "));
		return FALSE;
	}

	return TRUE;
}



CString GetProp(const char *name, vara &lines)
{
	if (lines.length()==0)
		{
		Log(LOGERR, "UpdateProp of empty page");
		return "";
		}
	int i;
	for (i=0; i<lines.length() && strncmp(lines[i], "{{", 2)!=0; ++i);

	if (i==lines.length()) 
		{
		Log(LOGERR, "UpdateProp of NOT {{property}} page (%s)", lines[0]);
		return "";
		}
	CString pname = "|"+CString(name)+"=";
	for (; i<lines.length() && strncmp(lines[i], "}}", 2)!=0; ++i)
		if (IsSimilar(lines[i], pname))
			return GetTokenRest(lines[i], 1, '=');
	return "";
}



int UpdateProp(const char *name, const char *value, vara &lines, int force = FALSE)
{
	BOOL isnull = stricmp(value, "NULL")==0;

	if (strchr(name,';'))
		{
		// multi-properties
		vara names(name,";");
		vara values(GetToken(value, 0, '!'),";"); 
		values.SetSize(names.length());
		int updates = 0;
		for (int i=0;i<names.length(); ++i)
			if (!values[i].IsEmpty())
				updates += UpdateProp(names[i], values[i], lines, force);
		return updates;
		}

	if (lines.length()<1)
		{
		Log(LOGERR, "UpdateProp of empty page %s", name);
		return FALSE;
		}

	if (strncmp(lines[0], "{{", 2)!=0 && strncmp(lines[1], "{{", 2)!=0) {
		Log(LOGERR, "UpdateProp could not find {{");
		return FALSE;
	}

	if (value==NULL || *value==0) {
		Log(LOGERR, "Invalid UpdateProp(%s,%s)", name, value);
		return FALSE;
		}
	if (*name=='=') {
		//Log(LOGWARN, "Invalid UpdateProp(%s,%s)", name, value);
		return FALSE;
	}

	CString pname = "|"+CString(name)+"=", pnameval = pname + value;
	if (strcmp(name, "Location type")==0)
		{
		static vara t(",,Caving,Via Ferrata,,POI");
		int fc = t.indexOf(value);
		double c = CDATA::GetNum(value);
		if (fc>0) c = fc;
		if (c==InvalidNUM || c<0) c=0;
		if (c<2 && !isnull) return FALSE;
		pnameval = pname + t[(int)c];
		}
	if (strcmp(name, "Best season")==0)
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
	if (strcmp(name, "AKA")==0)
		pnameval = pname + UpdateAKA("", value);
	int i;
	for (i=0; i<lines.length() && strncmp(lines[i], "}}", 2)!=0; ++i)
		if (IsSimilar(lines[i], pname)) {
			if (strcmp(lines[i], pnameval)==0)
				return FALSE;
			CString val = lines[i].Mid(pname.GetLength());
			if (strcmp(val,"0")==0 && strcmp(value,"0")!=0)
				val = "";			
			if (!val.IsEmpty()) {
				// SPECIAL CASE FOR VALUE OVERRIDE
				if (!force && MODE>-2) // if not overriding values
				{
					
					if (strcmp(name, "AKA")==0) // Add AKA to existing list
						pnameval = pname + UpdateAKA(val, value); 
					else if (strstr(name, "Shuttle")) // silently override shuttle
						pnameval = pnameval;
					else if (strstr(name, " time")) // silently override time
						{
						pnameval = pnameval; 
						double ov = CDATA::GetNum(val), nv = CDATA::GetNum(value);
						if (ov>0 && nv>0)
							{
							if (strstr(name, "Slowest")!=NULL)
								if (ov>nv)
									pnameval = pname + val;
								else
									pnameval = pnameval; 
							if (strstr(name, "Fastest")!=NULL)
								if (ov<nv)
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
			if (lines[i]==pnameval)
				return FALSE; // same
			lines[i] = pnameval;
			return TRUE; // new val
			}
	// new property
	if (strncmp(lines[i], "}}", 2)==0) {
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
	for (; *str!=0; ++str)
		if (!isspace(*str))
			return FALSE;
	return TRUE;
}

int FindSection(vara &lines, const char *section, int *iappend)
{
	vars sec = section;
	sec.MakeLower();

	for (int i=0; i<lines.length(); ++i)
		if (strncmp(lines[i], "==", 2)==0)
			if (strstr(lines[i].lower(), sec))
				{
				int start = ++i, append = -1;
				for (; i<lines.length() && strncmp(lines[i], "==", 2)!=0; ++i)
					if (!lines[i].trim().IsEmpty())
						append = i+1;
				if (iappend)
					*iappend = append;
				return start;
				}

	Log(LOGERR,"Could not find section '%s'", section);
	return -1;
}

int ReplaceLinks(vara &lines, int code, CSym &sym)
		{
		vars newline;
		CSymList wiklines;
		Code &cod = codelist[code];

		if (code>0)
			{
			vars zpre, zpost;
			newline = cod.BetaLink(sym , zpre, zpost);
			if (cod.IsWIKILOC())
				{
				int found = cod.FindLink(newline);
				if (found<0)
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
		for (int i=0; i<lines.length(); ++i) {
			// Update Beta sites
			if (strncmp(lines[i], "==", 2)==0)
				{
				int bs = strstr(lines[i], "Beta")!=NULL;
				int tr = strstr(lines[i], "Trip")!=NULL;
				if (bs || tr)
				{
				int lastnoblank = ++i;
				for (; i<lines.length() && strncmp(lines[i], "==", 2)!=0; ++i) 
					{
					 if (lines[i][0]=='*') // last bullet
						 lastnoblank=i+1;
					 const char *httplink = strstr(lines[i], "http");
					 if (!httplink)
						 continue;
					 int httpcode = GetCode(httplink);
					 if (httpcode<=0)
						 continue;

					 Code &cod = codelist[httpcode];
					 if (cod.IsWIKILOC())
						{
							// check if bulleted list
							vars pre, post;
							if (cod.IsBulletLine(lines[i], pre, post)<=0)
								{
								Log(LOGERR, "WARNING: found non-bullet WIKILOC link at %s : %.100s", sym.id, lines[i]);
								continue;
								}
							int found = cod.FindLink(httplink);
							if (found<0)
								{
								Log(LOGWARN, "Can't find invalid WIKILOC link for %s: %s", sym.id, lines[i]);
								continue;
								}
							if (cod.list[found].id!=sym.id)
								wiklines.Add(CSym(cod.list[found].GetStr(ITEM_CLASS), cod.BetaLink(cod.list[found], pre, post)));
							lines.RemoveAt(lastnoblank = i--);
							continue;
						}

					 if (httpcode==code) 
						{			
							// check if bulleted list
							vars pre, post;
							if (cod.IsBulletLine(lines[i], pre, post)<=0)
								{
								Log(LOGERR, "WARNING: found non-bullet link at %s : %.100s", sym.id, lines[i]);
								newline = "";
								continue;
								}
							int found = cod.FindLink(httplink);
							if (found<0 || cod.list[found].id==sym.id)
								{
								newline = cod.BetaLink(sym, pre, post);
								if (found<0) Log(LOGWARN, "Replacing bad beta site link for %s: %s => %s", sym.id, lines[i], newline);
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
		if (firstdel>lastnoblank || firstdel<0)
			firstdel = lastnoblank;
		if (!newline.IsEmpty() && firstdel>0)
			lines.InsertAt(firstdel, newline), ++lastnoblank;

		{
		// insert wikilines at end of bslist or trlist
		int end, start = FindSection(lines, "Trip", &end);
		int lastnoblank = end>0 ? end : start;
		if (lastnoblank<0)
			{
			Log(LOGERR, "MAJOR ERROR: Major wik error, lastnoblank = -1");
			lastnoblank = lines.length();
			}
			
		wiklines.Sort(cmpinvid);
		for (int w=0; w<wiklines.GetSize(); ++w)
			lines.InsertAt(lastnoblank+w, wiklines[w].data);
		}

		// compare old vs new
		if (oldlines.length()!=lines.length())
			return TRUE;
		for (int i=0; i<lines.length(); ++i)
			if (oldlines[i]!=lines[i])
				return TRUE;
		return FALSE;
		}



int AddLinks(const char *title, vara &lines, vara&links)
{
			// process kmls
			vars mid;
			for (int l=0; l<links.length(); ++l)
				 if (Links.MapLink(title, links[l]))
					 links.RemoveAt(l--);
		 
			 vars alllines = lines.join("");
			 // find last non blank after Beta and before Trips
			 int lastnonblank = -1;
			 int i;
			 for (i=0; i<lines.length(); ++i) 
				{
				if (strncmp(lines[i], "==", 2)==0)
					{
					if (strstr(lines[i], "Trip"))
						break;
					if (strstr(lines[i], "Beta"))
						lastnonblank = -1;
					continue;
					}
				if (!lines[i].trim().IsEmpty())
					lastnonblank =  i;
				}
			 if (lastnonblank<0)
				 lastnonblank = i-1;

			 // process links links
			 int n = 0;
			 for (int l=0; l<links.length(); ++l)
				 if (!strstr(alllines, links[l]))
					{
					lines.InsertAt(++lastnonblank, "* "+links[l]);
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
		if (geoloc.length()>1)
			{
			if (MODE<=-2 || (GetProp("Geolocation", lines).IsEmpty() && GetProp("Coordinates", lines).IsEmpty()))
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
			if (MODE<=-2 || GetProp("Coordinates", lines).IsEmpty())
				{
				double lat = CDATA::GetNum(symdata[ITEM_LAT]), lng = CDATA::GetNum(symdata[ITEM_LAT]);
				if (!CheckLL(lat, lng, sym.id))
					Log(LOGERR, "Skipping invalid coordinates for %s %s: %s,%s", sym.id, symdata[0],symdata[ITEM_LAT], symdata[ITEM_LNG]);
				else
				if (UpdateProp("Coordinates", symdata[ITEM_LAT]+","+symdata[ITEM_LNG], lines))
					++updates, comment.push("Coordinates");
				}
			}


		// update properties				
		for (int p=ITEM_REGION; p<symdata.length() && p<ITEM_LINKS; ++p) //p<rwformid.length(); ++p)
			{
			if (symdata[p].IsEmpty()) // skip empty field
				continue;
			// update region
			if (p==ITEM_REGION)
				symdata[p] = vara(symdata[p], ";").last();
			// update SUMMARY 
			if (p==ITEM_ACA)
			  if (!IsSimilar(symdata[ITEM_ACA], RWID))
				{
				vara symsummary(symdata[ITEM_ACA], ";");
				for (int p=0; p<symsummary.length() && p<R_SUMMARY; ++p)
					if (!symsummary[p].IsEmpty())
						if (UpdateProp(rwformacaid[p], symsummary[p], lines))
							++updates, comment.push(rwformacaid[p]);
				}
			if (p>=ITEM_RAPS && p<=ITEM_MAXTIME && CDATA::GetNum(symdata[p])==InvalidNUM) // skip non numerical for numeric params
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
		 if (links.length()>0)
			 if (AddLinks(title, lines, links))
				++updates, comment.push("Beta links");


		return updates;
}



void  PurgePage(DownloadFile &f, const char *id, const char *title)
{
	printf("PURGING #%s %s ...           \r", id ? id : "X" , title);
	CString url = "http://ropewiki.com/api.php?action=purge";
	if (id && id[0]!=0) 
		{
		if (CDATA::GetNum(id)!=InvalidNUM)
			url += CString("&pageids=")+id;
		else
			url += CString("&titles=")+id;
		}
	else
		url += CString("&titles=")+title;
	url += "&forcelinkupdate";
	if (f.Download(url))
		Log(LOGERR, "ERROR: can't purge %.128s", url);
}

void  RefreshPage(DownloadFile &f, const char *id, const char *title)
{
	printf("REFRESHING #%s %s ...           \r", id ? id : "X" , title);
	CString url = "http://ropewiki.com/api.php?action=sfautoedit&form=AutoRefresh&target=Votes:AutoRefresh&query=AutoRefresh[Location]=";
	if (id && id[0]!=0) 
		{
		if (CDATA::GetNum(id)!=InvalidNUM)
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
	ret += f.Download("http://ropewiki.com/index.php?title=Special:UserLogin");
	f.GetForm("name=\"userlogin\"");
	f.SetFormValue("wpName",loginstr[0]);
	f.SetFormValue("wpPassword",loginstr[1]);
	f.SetFormValue("wpRemember","1");
	ret += f.SubmitForm();

	/*
	CString lgtoken = f.GetFormValue("wpLoginToken");
	vars login = "wpName="+loginstr[0]+"&wpPassword="+loginstr[1];
	ret += f.Download("http://ropewiki.com/index.php?title=Special:UserLogin&action=submitlogin&type=login&returnto=Main+Page?POST?"+login+"&wpRemember=1&wpLoginAttempt=Log+in&wpLoginToken="+lgtoken, "login.htm");
	*/
	if (ret)
		{
		Log(LOGERR, "ERROR: can't login");
		return FALSE;
		}

	return TRUE;
}


BOOL IsID(const char *id)
{
		// all digits
		if (!id) return FALSE;
		for (int i = 0; id[i]!=0; ++i)
			if (!isdigit(id[i]))
				return FALSE;
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
		for (int i=1; i<labels.length(); ++i)
			sym.SetStr(i-1, getfulltextorvalue(labels[i]));
		idlist.Add(sym);
		return TRUE;
}

int rwflabels(const char *line, CSymList &idlist)
{
		vara labels(line, "label=\"");
		//vars name = getfulltext(labels[0]);
		vars name = htmltrans(ExtractString(line, "fulltext="));
		CSym sym(name.replace(",", "%2C"));
		for (int i=1; i<labels.length(); ++i)
			sym.SetStr(i-1, getlabel(labels[i]));
		idlist.Add(sym);
		return TRUE;
}



class CPage
{
BOOL open;
vars id, title;
vars pageurl, textname, oldlines;
DownloadFile *fp;

int Open(DownloadFile &f, const char *url, int retries = 3, const char *action = "action=submit")
{
	for (int retry=0; retry<retries; ++retry)
		{
		// download page 
#ifdef DEBUG
			if (f.Download(url, "testpage.html")) {
#else
			if (f.Download(url)) {
#endif
			//Log(LOGERR, "ERROR: can't EDIT %.128s, retrying", pageurl);
			if (retries>1) 
				Login(f);
			continue;
			}

		// modify page
		vars memory(htmltrans(f.memory));
		textname = ExtractString(memory, "<textarea", " name=\"", "\""); 
		lines = vara(oldlines = ExtractString(memory, "<textarea", ">", "</textarea"), "\n");

		// group multilines
		BOOL properties = FALSE;
		for (int i=0; i<lines.length(); ++i)
			{
			if (IsSimilar(lines[i],"{{"))
				{
				properties = TRUE;
				continue;
				}
			if (IsSimilar(lines[i],"}}"))
				{
				properties = FALSE;
				continue;
				}
			if (properties)
			  if (lines[i-1][0]=='|' && lines[i][0]!='|')
				{
				lines[i-1] += "\n"+lines[i];
				lines.RemoveAt(i--);
				}
			}

		// get values
		if (!f.GetForm(action)) {
			//Log(LOGERR, "ERROR: could not edit/submit, retrying %s", title);
			Login(f);
			continue;
			}
		return TRUE;
		}

	pageurl = "";
	return FALSE;
}


public:
vara lines;
vara comment;

const char *Title()
{
	return title;
}

const char *Id()
{
	return id;
}

BOOL Exists()
{
	return open>0 && lines.length()>0;
}

CPage(DownloadFile &f, const char *_id, const char *_title, const char *_oldid = "")
{
	fp = &f;

	if (!_id) _id = "";
	if (!_title) _title = "";
	id = _id;
	title = _title;

	if (!id.IsEmpty() && !IsID(id))
		{
		// id = title
		title = id; 
		id = "";
		}

	open = -1;
	if (_oldid)
		{
		// Edit
		pageurl = CString("http://ropewiki.com/index.php?");
		if (id.IsEmpty()) 
			pageurl += CString("title=")+url_encode(title);
		else
			pageurl += CString("curid=")+id;
		if (_oldid=="new")
			open = Open(f, pageurl+"&action=edit");
		else
			open = Open(f, pageurl+"&action=edit"+(*_oldid!=0 ? CString("&oldid=")+_oldid : ""));
		}
}

int NeedsUpdate()
{
	int ret = !pageurl.IsEmpty() && comment.length()!=0 && oldlines.trim()!=lines.join("\n").trim();
#ifdef DEBUGXXX
	if (ret)
		{
		CFILE of, nf;
		if (of.fopen("testo.log", CFILE::MWRITE))
			of.fputstr(oldlines);
		if (nf.fopen("testn.log", CFILE::MWRITE))
			nf.fputstr(lines.join("\n"));
		}
#endif
	return ret;
}

void Set(const char *name[])
{
	lines = vara(name);
}

vars Get(const char *name)
{
	vars prop = "|"+vars(name)+"=";
	int len = prop.GetLength();
	for (int i=0; i<lines.length(); ++i)
		if (strncmp(prop, lines[i], len)==0)
			{
			const char *line = lines[i];
			return line + len;
			}
	return "";
}

void Set(const char *name, const char *value, int force = FALSE)
{
	UpdateProp(name, value, lines, force);
}

int Override(const char *name, const char *value)
{
	if (value!=NULL && *value!=0)
		if (Get(name)!=value)
			{
			Set(name, value, TRUE);
			return TRUE;
			}
	return FALSE;
}

int Section(const char *section, int *iappend)
{
	return FindSection(lines, section, iappend);
}


void Purge(void)
{
		DownloadFile &f = *fp;
		PurgePage(f, id, title);
}



int Update(int forceupdate = FALSE)
{
		if (open<=0)
			{
			Log(LOGINFO, "COULD NOT OPEN! SKIPPING UPDATING %s %s : %s", id, title, comment.join(";"));
			return FALSE;
			}

		if (!forceupdate && !NeedsUpdate())
			return FALSE;

		//UpdateProp(pname, pval, lines, TRUE);

		DownloadFile &f = *fp;
		Log(LOGINFO, "UPDATING %s %s : %s", id, title, comment.join(";"));
		f.SetFormValue(textname, lines.join("\n"));
		f.SetFormValue("wpSummary", comment.join(";"));
		f.SetFormValue("wpWatchthis", "0");
		f.SetFormValue("wpPreview", NULL);
		f.SetFormValue("wpDiff", NULL);
#ifdef DEBUGXXX
		for (int i=0; i<lines.GetSize(); ++i) Log(LOGINFO, "'%s'", lines[i]);
#endif
		/*
		CString url = pageurl;
		f.SetPostFile(PostPage);
		url += "&action=submit?POSTFILE?";
		url += f.SetFormValue(textname, lines.join("\n"));
		url += f.SetFormValues(lists);
		//printf("SUBMITTING %s ...\r", title);
		for (int retry=0; retry<3; ++retry)
			{
			if (f.Download(url)) {
				Login(f);
				continue;
				}
			Purge();
			return TRUE;
			}
		*/
		for (int retry=0; retry<3; ++retry)
			{
			if (f.SubmitForm()) {
				Login(f);
				continue;
				}
			Purge();
			return TRUE;
			}

		Log(LOGERR, "ERROR: can't submit form for %s %.128s", id, title);
		return FALSE;
}

void Delete(void)
{
		if (comment.length()<=0)
			return;
		
		if (open<=0)
			{
			Log(LOGINFO, "ALREADY DELETED %s %s : %s", id, title, comment.join(";"));
			return;
			}


		Log(LOGINFO, "DELETING %s %s : %s", id, title, comment.join(";"));
		for (int i=0; i<lines.length(); ++i)
			Log(LOGINFO, "%s", lines[i]);

		// Delete
		DownloadFile &f = *fp;
		vars url = "http://ropewiki.com/index.php?action=delete&title="+url_encode(title.replace(" ", "_"));
		if (!Open(f, url, 1, "action=delete"))
			{
			Log(LOGERR, "ERROR: could not open submit page %s (3 retries)", url);
			return;
			}

		//wpOldTitle		
		//wpNewTitleNs
		//f.SetFormValue(lists, "wpOldTitle", url_encode(title));

		//f.SetFormValue(lists, "wpDeleteReasonList", "other");
		f.SetFormValue("wpReason", comment.join(";"));
		f.SetFormValue("wpWatch", "0");

#ifdef DEBUGXXX
		for (int i=0; i<lists[0].GetSize(); ++i) Log(LOGINFO, "'%s' = '%s'", lists[0][i], lists[1][i]);
#endif
		/*
		url += "?POST?" + f.SetFormValues(lists, FALSE);
		*/
		//printf("SUBMITTING %s ...\r", title);
		for (int retry=0; retry<3; ++retry)
			{
			if (f.SubmitForm()) {
				Login(f);
				continue;
				}
			DownloadFile f;
			if (Open(f, pageurl, 1)) {
				Log(LOGERR, "FAILED DELETE %s", title);
				return;
				}

			Purge();
			return;
			}
		Log(LOGERR, "ERROR: can't delete %.128s", url);
		// manual chek
		//if (MODE>=0) 
		//	system("start \"check\" \"+url+"&action=history\"");
}

void Move(const char *newtitle)
{
		if (comment.length()<=0)
			return;

		vars pageurl = this->pageurl;
		Log(LOGINFO, "MOVING %s %s -> %s", id, title, newtitle);

		// Move
		DownloadFile &f = *fp;
		vars url = "http://ropewiki.com/Special:MovePage/"+url_encode(title.replace(" ", "_"));
		if (!Open(f, url)) {
			Log(LOGERR, "ERROR: could not open submit page %s (3 retries)", url);
			return;
			}

		//wpOldTitle		
		//wpNewTitleNs
		//f.SetFormValue(lists, "wpOldTitle", url_encode(title));

		vars nstitle = newtitle;
		const char *ns = f.GetFormValue2("wpNewTitleNs");
		if (ns && strcmp(ns,"0")!=0)
			nstitle = GetTokenRest(newtitle, 1, ':');

		f.SetFormValue("wpNewTitleMain", nstitle);
		f.SetFormValue("wpReason", comment.join(";"));
		f.SetFormValue("wpWatch", "0");
		f.SetFormValue("wpLeaveRedirect", 0);

#ifdef DEBUGXXX
		for (int i=0; i<lists[0].GetSize(); ++i) Log(LOGINFO, "'%s' = '%s'", lists[0][i], lists[1][i]);
#endif
		/*
		url = "http://ropewiki.com/index.php?title=Special:MovePage&action=submit?POST?";
		url += f.SetFormValues(lists, FALSE);
		*/
		//printf("SUBMITTING %s ...\r", title);
		for (int retry=0; retry<3; ++retry)
			{
			if (f.SubmitForm()) {
				Login(f);
				continue;
				}
			DownloadFile f;
			if (Open(f, pageurl,1 )) {
				Log(LOGERR, "FAILED MOVE %s -> %s", title, newtitle);
				return;
				}

			id = "";
			title = newtitle;
			Purge();
			return;
			}
		Log(LOGERR, "ERROR: can't move %.128s", url);
		// manual chek
		//if (MODE>=0) 
		//	system("start \"check\" \"+url+"&action=history\"");
}


#if 0
static int UploadFileHandler(DownloadFile *df, const char *lrfile, inetdata &data)
{
		/*
		// get token from previous page
		CString token = df->GetFormValue("wpEditToken", "id");
		*/
		
		CString lfile = GetToken(lrfile, 0, '>');
		CString rfile = GetToken(lrfile, 1, '>');
	



		data.write( df->SetFormFile("wpUploadFile", rfile, mime[1][found]) );
		CFILE f;
		if (!f.fopen(lfile, CFILE::MREAD))
			{
			Log(LOGERR, "ERROR: %s file not exist\n", lfile);
			return FALSE;
			}
		int bread = 0;
		char buff[8*1024];
		while ( bread = f.fread(buff, 1, sizeof(buff)))
			data.write(buff, bread);
		data.write("\n");

		form +=df->SetFormValue("wpDestFile", rfile);
		form +=df->SetFormValue("wpUploadDescription",""); // comment
		form +=df->SetFormValue("wpWatchthis","0");
		form +=df->SetFormValue("wpIgnoreWarning","1"); // ignore warns
		form +=df->SetFormValue("wpEditToken", token);
		form +=df->SetFormValue("title", "Special:Upload");
		form +=df->SetFormValue("wpDestFileWarningAck", "");
		form +=df->SetFormValue("wpForReUpload", "1");
		form +=df->SetFormValue("wpUpload","Upload file");
		//url += f.SetFormValues(lists);

		data.write(form);
		return TRUE;		
}
#endif


int FileExists(const char *rfile)
{
		DownloadFile &f = *fp;
		vars ufile = url_encode(vars(rfile).replace(" ", "_"));
		vars url = "http://ropewiki.com/File:"+ufile;
		if (!f.Download(url))
			if (!strstr(f.memory,"No file by this name exists"))
				return TRUE;

		return FALSE;
}

int UploadFile(const char *lfile, const char *rfile, const char *desc = NULL)
{
		DownloadFile &f = *fp;
		Log(LOGINFO, "UPLOADING FILE %s -> File:%s", lfile, rfile);

		// Upload
		vars ufile = url_encode(vars(rfile).replace(" ", "_"));
		vars url = "http://ropewiki.com/index.php?title=Special:Upload&wpDestFile="+ufile+"&wpForReUpload=1";
		if (!Open(f, url, 3, "action=\"/Special:Upload\"")) {
			Log(LOGERR, "ERROR: could not open upload page %s (3 retries)", url);
			return FALSE;
			}

		/*
		f.SetPostFile(UploadFileHandler);
		url = "http://ropewiki.com/index.php/Special:Upload?POSTFILE?";
		url += lfile +vars(">")+rfile;
		*/
		//printf("SUBMITTING %s ...\r", title);
		vars ext = GetFileExt(lfile);
		varas mime("kml=application/vnd.google-earth.kml+xml,pdf=application/pdf,jpg=image/jpeg,gif=image/gif,png=image/png,tif=image/tiff");
		int found = mime[0].indexOf(ext);
		if (found<0)
			{
			Log(LOGERR, "Invalid mime for file %s", lfile);
			return FALSE;
			}
		f.SetFormFile("wpUploadFile", rfile, mime[1][found], lfile);
		f.SetFormValue("wpDestFile", rfile);
		if (desc && *desc!=0)
			f.SetFormValue("wpUploadDescription",desc); // comment
		f.SetFormValue("wpWatchthis","0");
		f.SetFormValue("wpIgnoreWarning","1"); // ignore warns
		//f.SetFormValue("wpEditToken", token);
		//f.SetFormValue("title", "Special:Upload");
		//f.SetFormValue("wpDestFileWarningAck", "");
		//f.SetFormValue("wpForReUpload", "1");
		//f.SetFormValue("wpUpload","Upload file");
		for (int retry=0; retry<3; ++retry)
			{
			if (f.SubmitForm()) {
				Login(f);
				continue;
				}
			if (FileExists(rfile))
				return TRUE;
			}


		Log(LOGERR, "UPLOAD ERROR for file '%s', skipping File:%s", lfile, rfile);
		return FALSE;
}

};





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
	if (regionslist.GetSize()==0)
		return FALSE;

	Log(LOGINFO, "%d regions to be created", regionslist.GetSize());
	for (int i=0; i<regionslist.GetSize(); ++i)
		Log(LOGINFO, "#%d: %s (parent=%s)", i, regionslist[i].id, regionslist[i].data);
	for (int i=5; i>0; --i)
		{
		printf("Starting in %ds...   \r", i);
		Sleep(1000);
		}

	for (int i=0; i<regionslist.GetSize(); ++i)
		{
		vars title = regionslist[i].id;
		vars parent = regionslist[i].data;

		// create region
		CPage p(f, NULL, title);
		if (p.lines.length()>0) {
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
	  oregion = regionmatch( oregion );
	sym.SetStr(ITEM_REGION, oregion);

	// AutoRegion
	vara georegions;
	int r = _GeoRegion.GetRegion(sym, georegions);
	if (r<0)
		{
		Log(LOGWARN, "WARNING: No AutoRegion for %s [%s] (%s,%s), using '%s'", sym.id, sym.GetStr(ITEM_DESC), sym.GetStr(ITEM_LAT), sym.GetStr(ITEM_LNG), oregion);
		return FALSE;
		}

	vara aregions;
	for (int i=0; i<=r; ++i)
		aregions.push(GetToken(georegions[i],0,':'));
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
	for (int i=0; i<list.GetSize(); ++i) {
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
		if (rwidnum<=0)
			return FALSE;

		CSymList list;
		vars file = filename(codelist[code].code);
		list.Load(file);
		int find = list.Find(id);
		if (find<0)
			{
			Log(LOGERR, "Invalid SPECIAL ID %s", id);
			return FALSE;
			}

		list[find].SetStr(ITEM_INFO, RWID+CData(rwidnum));
		list.Save(file);
		return TRUE;
}


int FixBetaPage(CPage &p, int MODE = -1);

int UploadBeta(int mode, const char *file)
{
	CSymList list;
	vars chgfile = filename(CHGFILE);
	if (file!=NULL && *file!=0)
		chgfile = file;
	list.Load(chgfile);
	CSymList clist[sizeof(codelist)/sizeof(*codelist)];
	rwlist.Load(filename(codelist[0].code));
	rwlist.Sort();

	// log in as RW_ROBOT_USERNAME [RW_ROBOT_PASSWORD]
	int ret = 0;	
	DownloadFile f;
	if (!Login(f))
		return FALSE;

	CSymList newregionslist;
	for (int i=0; i<list.GetSize(); ++i) {
		CSym &sym = list[i];
		if (sym.id.IsEmpty())
			continue;
		if (!sym.GetStr(ITEM_NEWMATCH).IsEmpty())
			continue;
		// cleanup usual stuff
		CString match = sym.GetStr(ITEM_MATCH);
		if (sym.id.Find(RWID)<0)
			{
			if (match.Find(RWID)>=0) 
				{
				if (match.Find(RWLINK, 1)<0)
					{
					// new match, skip any data assignment
					sym.SetStr(ITEM_LAT, "");
					sym.SetStr(ITEM_LNG, "");
					for (int m=ITEM_REGION; m<ITEM_BETAMAX; ++m)
						if (m!=ITEM_KML)
							sym.SetStr(m, "");
					}
				}
			else
				AutoRegion(sym);
			}
		if (!strstr(match,RWID))
			UploadRegion(sym, newregionslist);
		else
			if (MODE>-2)
				sym.SetStr(ITEM_REGION,"");
		}

	// create regions
	CreateRegions(f, newregionslist);

	// Canyons
	int error = 0;
	for (int i=0; i<list.GetSize() && error<3; ++i)
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
		const char *user = code>=0 ? codelist[code].betac->ubase : NULL;
		if (code<0 || !user) {
			Log(LOGERR, "Invalid code/name for %s", sym.Line());
			continue;
		}

		CString url;
		int idnum = -1;
		vars id = sym.GetStr(ITEM_MATCH), title;
		const char *oldid = "";
		const char *rwid = strstr(id, RWID);
		if (rwid) {
			idnum = (int)CDATA::GetNum(rwid+strlen(RWID));
			int f = rwlist.Find(RWID+(id=MkString("%d", idnum)));
			if (idnum>0 && f>=0)
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
			title = id; id =""; oldid="new";
			if (title.IsEmpty()) {
				Log(LOGERR, "Invalid title=%s for %s", title, sym.id);
				continue;
				}
			}
		printf("%d/%d %s %s    \r", i, list.GetSize(), id, title);

		// process IGNORE
		if (title.upper()=="X")
			{
			CFILE f;
			if (f.fopen(filename("ignore"), CFILE::MAPPEND))
				f.fputstr(sym.id + ",X AUTOIGNORE");
			continue;
			}

		// Process BQN:
		BOOL BQNmode = code>0 && !IsSimilar(sym.id, "http") && !IsSimilar(sym.GetStr(ITEM_INFO), RWID);
		if (BQNmode && idnum>0)
			BQNupdate(code, sym.id, idnum);

		CPage p(f, id, title, oldid);

		if (idnum>=0 && p.lines.length()<=1) {
			Log(LOGERR, "ERROR: update of empty page (#lines=%d)", p.lines.length());
			continue;
			}

		if (idnum<0 && p.lines.length()>0) {
			Log(LOGERR, "ERROR: new page not empty (title=%s)\n%.256s", title, p.lines.join("\n"));
			PurgePage(f, id, title);
			continue;
			}

		vars basecomment = "Updated: ";
		if (idnum<0) {
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
			if (sym.GetNum(ITEM_CLASS)==2) // Cave
				{
				int red = p.lines.indexOf("==Red tape==");
				if (red>0)
					p.lines.InsertAt(red+1, "If you want to get into serious cave exploration, join some experienced local caving group (sometimes called 'grotto's).");
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

		if (BQNmode && idnum<0)
			// created a new page, get page id!
			if (!f.Download("http://ropewiki.com/api.php?action=query&format=xml&prop=info&titles="+title))
				BQNupdate(code, sym.id, ExtractNum(f.memory, "pageid=", "\"", "\""));
		}

	return TRUE;
}


vars SetSectionImage(int big, const char *url, const char *urlimgs, const char *titleimgs, vara &uploadfiles)
{
	vars saveimg = url2file(urlimgs, BQNFOLDER);
	if (uploadfiles.indexOf(saveimg)>=0)
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
	if (start<0)
		{
		Log(LOGERR, "Invalid section '%s', skipping %s", section, url);
		return FALSE;
		}
	if (end>=0 && MODE>-2)
		{
		//Log(LOGERR, "Not empty Section '%s', skipping %s [MODE -2:append -3:overwrite]", section, url);
		return FALSE;
		}
	
	// convert content with images
	vara urlimgs, titleimgs;
	vars desc = Download_SaveImg(url, content, urlimgs, titleimgs);	

	if (!keepbold)
		{	
		desc.Replace("<b>",""); 		
		desc.Replace("</b>",""); 		
		}

	for (int i=0; i<urlimgs.length(); ++i)
		desc += SetSectionImage(FALSE, url, urlimgs[i], titleimgs[i], uploadfiles);
	
	if (MODE<-2)
		{
		// overwrite
		for (int i=start; i<end; ++i)
			p.lines.RemoveAt(start);
		while (start<p.lines.length() && p.lines[start].Trim().IsEmpty())
			p.lines.RemoveAt(start);
		end = -1;
		}
	desc.Trim(" \n");
	if (end<0)
		p.lines.InsertAt(start, desc);
	else
		p.lines.InsertAt(end, "\n"+desc);
	return TRUE;

}




CString ExtractBetaBQN(const char *url, CString &memory, vara &ids, int ext = FALSE, const char *start = NULL, const char *end = NULL)
{
	CString text;
	for (int i=0; i<ids.length() && text.IsEmpty(); ++i)
		text = ExtractStringDel(memory, "<b>"+UTF8(ids[i]), !start ? "</b>" : start, !end ? "<b>" : end, ext ? -1 : FALSE, -1);

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
	while (text.Replace("  "," "));
	text.Replace("<br> ", "<br>");
	text.Replace(" <br>", "<br>");
	while (text.Replace("<br><br>","<br>"));
	while (text.Replace("<b></b>",""));
	while (text.Replace("<b> </b>",""));
	while (text.Replace("</b><b>",""));
	while (text.Replace("</b> <b>",""));
	
	text.Trim();
	if (text.IsEmpty())
		return "";

	text.Replace("<br>", "");
	text.Replace("</b>",""); 
	vara lines(text,"<b>");
	for (int i=1; i<lines.length(); ++i)
		{
			vars &line = lines[i];
			int dot = line.Find(":");
			if (dot<0 && !line.IsEmpty()) 
				{
				// no ':'
				Log(LOGWARN, "No <b>: line '%s' for %s", line, url);
				Log(text);
				line.Insert(0, "</b>");
				}
			if (dot>=0)
				{
				line.Insert(dot+1, "</b>");			
				line.Trim(". \n");
				line += "</div>\n";
				}
		}
	int startindex = lines.length()>1 ? 1 : 0;
	for (int i=startindex; i<lines.length(); ++i)
		{
			int dot = lines[i].Find(":");
			vars text = stripAccentsL(stripHTML(lines[i].Mid(dot+1))).Trim(". ");
			if (text=="" || text=="sin datos" || text=="sans donnees")
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
		url = "http://www.barranquismo.org/imagen/displayimage.php?"+vars(album)+"&pos=0";
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

		url = "http://www.barranquismo.org/imagen/albums"+img;
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
	rwlist.Load(filename(codelist[0].code));
	rwlist.Sort();

	// log in 
	loginstr = vara("Barranquismo.net,sentomillan");
	int ret = 0;	
	DownloadFile f;
	if (!Login(f))
		return FALSE;

	// Canyons
	int error = 0;
	for (int i=0; i<ynlist.GetSize(); ++i)
		{
		CSym &ynsym = ynlist[i];

		// check transfer authorization
		if (!strstr(ynsym.GetStr(6),"Y"))
			continue;

		// check if id is valid
		if (ynsym.id.IsEmpty())
			continue;
		int li = list.Find(ynsym.id);
		if (li<0)
			{
			Log(LOGERR, "Invalid BQN id %s, skipping", ynsym.id);
			continue;
			}

		CSym &sym = list[li];

		//check if already transferred
		if (mode!=-3 && !sym.GetStr(ITEM_STARS).IsEmpty())
			continue;
		//check if not transferred (overwrite)
		if (mode==-3 && !strstr(sym.GetStr(ITEM_STARS),"Y"))
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
		idnum = (int)CDATA::GetNum(rwid+strlen(RWID));
		int found = rwlist.Find(RWID+(id=MkString("%d", idnum)));
		if (idnum>0 && found>=0)
			title = rwlist[found].GetStr(ITEM_DESC);
		else
			{
			Log(LOGERR, "Invalid pageid=%s title=%s", id, title);
			continue;
			}

		printf("%d/%d %s %s    \r", i, list.GetSize(), id, title);

		//Throttle(barranquismoticks, 1000);
		CPage p(f, id, title);

		if (mode==-4)
			{
			int bqn = FALSE;
			int end, start = p.Section("Introduction", &end);
			for (int i=start; i<end && !bqn; ++i)
				bqn = IsSimilar(p.lines[i], "<div><b>") || IsSimilar(p.lines[i],"{{pic|size=X|:");
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
		vars bannerjpg = title.replace(" ","_")+"_Banner.jpg";
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

		if (err>0)
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
		redtape.Replace("No se conocen","");
		redtape.Replace("Ils ne son pas connus / Inconnus","");
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
			descent += "\n"+escapes;
		vars comp = stripAccentsL(GetToken(stripHTML(shuttle), 1, ':')).Trim(" .");
		if (!shuttle.IsEmpty() && !(comp=="no" || comp=="sin combinacion" || comp=="no necesaria" || comp=="no necesario"))
			 approach = shuttle+"\n"+approach;
			 //data2 += shuttle;

		vars album;
		vars maps, topo, fotos;

		// topos and maps
		vara urlimgs, titleimgs, embeddedimgs;
		vars imglinks = ExtractString(memory, "TABLA IZQUIERDA", "", "OTROS DATOS");
		Download_SaveImg(sym.id, imglinks, urlimgs, titleimgs, TRUE);
		if (imglinks.IsEmpty() || urlimgs.length()==0)
			{
			Log(LOGERR, "Invalid TABLA IZQUIERDA for %s for %s", title, sym.id);
			continue;
			}
		//continue;
		for (int u=0; u<urlimgs.length(); ++u)
			if (IsBQN(urlimgs[u]))
			{
			  vars urlimg = urlimgs[u];
			  vars titleimg = titleimgs[u];
			  vars search = stripAccentsL(titleimg);
			  if (IsImage(urlimgs[u]))
				{
				if (u==0)
					{
					if (!strstr(urlimg, "croquisejemplo"))			
						topo += SetSectionImage(TRUE, sym.id, urlimg, titleimg, files);					
					}
				else if (search.indexOf("topo")>=0 || search.indexOf("esquema")>=0 || search.indexOf("croquis")>=0 ||search.IsEmpty())
						topo += SetSectionImage(TRUE, sym.id, urlimg, titleimg, files);					
				else if (search.indexOf("map")>=0 || search.indexOf("ortofoto")>=0 || search.indexOf("cabecera")>=0 || search.indexOf("vista")>=0 || search.indexOf("aviso")>=0 || search.indexOf("panor")>=0 )
						maps += SetSectionImage(TRUE, sym.id, urlimg, titleimg, files);
				else if (search.indexOf("fotos")>=0 || search.indexOf("album")>=0)
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
			  if (search.indexOf("fotos")>=0 || search.indexOf("album")>=0)
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
		for (int u=0; u<urlimgs.length(); ++u)
			{
			vars link = urlstr(urlimgs[u]);
			if (IsBQN(link))
				continue;

			int l;
			for (l=start; l<end && !strstr(p.lines[l], link); ++l);
			if (l<end) // already listed
				continue;
			p.lines.InsertAt(end++, MkString("* %s",link));
			}
		}
		
		// update page
		int err = 0;
		err += !SetSectionContent(sym.id, p, "Introduction", data1+data2, files);
		err += !SetSectionContent(sym.id, p, "Approach", approach+maps, files);
		err += !SetSectionContent(sym.id, p, "Descent", descent+topo, files); 
		err += !SetSectionContent(sym.id, p, "Exit", exit, files); 
		err += !SetSectionContent(sym.id, p, "Red tape"	, redtape, files); 
		err += !SetSectionContent(sym.id, p, "Trip", fotos, files); 
		err += !SetSectionContent(sym.id, p, "Background", credit+background, files); 

		if (err>0)
			{
			Log(LOGERR, "%d SECTION ERRORS DETECTED, skipping %s (%s)", err, title, sym.id);
			continue;
			}		
		}


		// update
		int err = 0;
		vars basecomment = "Imported from Barranquismo.net "+sym.id;
		Log(LOGINFO, "POST %d/%d %s %s [#%d]", i, list.GetSize(), basecomment, title, idnum);
		if (MODE>-1)
			{
			// do not update
			Log("\n"+p.lines.join("\n")+"\n");
			continue;
			}
		else
			{
			// upload files
			for (int u=0; u<files.length(); ++u)
				{
				const char *rfile = GetFileNameExt(files[u]);
				if (!up.FileExists(rfile))
					err += !up.UploadFile(files[u], rfile);						
				}
			// upload cover pic
			if (lcover && rcover)
				err += !up.UploadFile(lcover, rcover);

			if (err>0)
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
			if (htmltrans(plines)!=htmltrans(p2lines))
				{
				int n, nl,ol;
				vara newlines(plines,"\n");
				vara lines(p2lines,"\n");
				for (n=0; n<newlines.length() && n<lines.length(); ++n)
					if (newlines[n]!=lines[n])
						break;
				vars dump;
				for (nl=newlines.length()-1, ol=lines.length()-1; nl>n && ol>n; --nl, --ol)
					if (newlines[nl]!=lines[ol])
						{
						dump += MkString("#%d : ", i);
						dump += (i<=nl) ? newlines[i] : "";
						dump += MkString(" =!= ");
						dump += (i<=ol) ? lines[i] : "";
						dump += "\n";
						}
				Log(LOGERR, "INCONSISTENT UPDATE for %s! #newlines:%d =?= #oldlines:%d diff %d-%d", title, newlines.length(), n<lines.length(), n, nl);
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
	vars ret = ExtractStringDel(memory, "<b>"+vars(start), "</b>", "</body", FALSE, -1);
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

	while(memory.Replace("<br> ", "<br>"));
	while(memory.Replace(" <br>", "<br>"));
	while(memory.Replace("<br><br>", "<br>"));
	while(memory.Replace("<b><br>", "<br><b>"));
	while(memory.Replace("<br></b>", "</b><br>"));
	while(memory.Replace("<b></b>", ""));

	while(!ExtractStringDel(memory, "<IMG", "", ">").IsEmpty());

	// get bold styles
	vara bolds;
	vara styles( ExtractString(memory, "<style", "", "</style"), ".");
	for (int i=1; i<styles.length(); ++i)
		if (strstr(ExtractString(styles[i], "{", "", "}"), "bold"))
			{
			vars bold = GetToken(styles[i], 0, " ,{");
			vars spans = "<span class=\""+bold+"\"", spane = "</span>";
			while (TRUE)
				{
				vars span = ExtractStringDel(memory, spans, ">", spane, TRUE, FALSE);
				if (span.IsEmpty()) break;
				vars spanin = ExtractStringDel(memory, spans, ">", spane, FALSE, FALSE).Trim();
				if (spanin.IsEmpty())
					memory.Replace(span, " "); // eliminate
				else
					memory.Replace(span, "<b>"+spanin+"</b>"); // encapsulate
				}
			spans = "<p class=\""+bold+"\"", spane = "</p>";
			while (TRUE)
				{
				vars span = ExtractStringDel(memory, spans, ">", spane, TRUE, FALSE);
				if (span.IsEmpty()) break;
				vars spanin = ExtractStringDel(memory, spans, ">", spane, FALSE, FALSE).Trim();
				if (spanin.IsEmpty())
					memory.Replace(span, " "); // eliminate
				else 
					memory.Replace(span, "<b>"+spanin+"</b><br>"); // encapsulate
				}
			bolds.push(bold);
			}


	const char *check = strstr(memory, "Description of");
	memory.Replace("</b>", "</b> ");

	{
	vara lines(memory, "<b>");
	for (int i=1; i<lines.length(); ++i)
		{
		// <b> </b> if br replace </b><br><b>
		vara blines(lines[i], "</b>");
		//ASSERT(!strstr(blines[0], "T 2m"));
		blines[0].Replace("<br>","</b><br><b>");
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
			else if (bolds.indexOf(sclass)<0)
				blines[0].Replace(span, bs+spanin+be); // encapsulate
			}
		lines[i] = blines.join("</b>");
		}
	memory = lines.join("<b>");
	}

	{
	vara linesb(memory, "<b>");
	for (int i=1; i<linesb.length(); ++i)
		{
		vara blines(linesb[i], "</b>");
		blines[0].Replace(": ",":</b> <b>");
		linesb[i] = blines.join("</b>");
		}
	memory = linesb.join("<b>");
	}

	const char *check2 = strstr(memory, "Description of");

	memory.Replace("</p>", "<br>");
	memory.Replace("<p>", "");
	while(!ExtractStringDel(memory, "<p ", "", ">").IsEmpty());

	//memory.Replace("<br>", " ");
	while(memory.Replace("  ", " "));
	while(memory.Replace("<b></b>", ""));
	while(memory.Replace("<b> </b>", " "));
	while(memory.Replace("<b>. ", ". <b>"));
	while(memory.Replace("<br><br>", "<br>"));
	//memory.Replace("<br>", "\n\n");

	memory.Replace("</span>", "");
	while(!ExtractStringDel(memory, "<span", "", ">", TRUE).IsEmpty());

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
	
	sym.SetStr(ITEM_SHUTTLE, stripHTML(ExtractString(memory, ">Shuttle", "", "<b>").replace(":","").replace("Yes,","").replace(",",".")));

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
	for (int t=0; t<sizeof(ids)/sizeof(*ids); ++t)
		timestr.Replace(ids[t], vars("<b>")+ids[t]);
	for (int t=0; t<sizeof(ids)/sizeof(*ids); ++t)
		{
		CString time = stripHTML(ExtractString(timestr, ids[t], "", "<b>"));
		while (!ExtractStringDel(time, "(", "", ")").IsEmpty());
		times.push(vars(time).replace("hours", "h").replace("min", "m").replace(";","."));	
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
	rwlist.Load(filename(codelist[0].code));
	rwlist.Sort();

	// log in 
	loginstr = vara("Canyoning Cult,slovenia");
	int ret = 0;	
	DownloadFile f;
	if (!Login(f))
		return FALSE;

	// Canyons
	int error = 0;
	for (int i=0; i<ynlist.GetSize(); ++i)
		{
		CSym &ynsym = ynlist[i];
		if (!ynsym.GetStr(ITEM_NEWMATCH).IsEmpty())
			continue;

		// check if id is valid
		if (ynsym.id.IsEmpty())
			continue;
		int li = list.Find(ynsym.id);
		if (li<0)
			{
			Log(LOGERR, "Invalid BQN id %s, skipping", ynsym.id);
			continue;
			}

		CSym &sym = list[li];
		if (sym.GetStr(ITEM_KML).IsEmpty())
			continue;

		//check if already transferred
		if (mode!=-3 && !sym.GetStr(ITEM_STARS).IsEmpty())
			continue;
		//check if not transferred (overwrite)
		if (mode==-3 && !strstr(sym.GetStr(ITEM_STARS),"Y"))
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
		idnum = (int)CDATA::GetNum(rwid+strlen(RWID));
		int found = rwlist.Find(RWID+(id=MkString("%d", idnum)));
		if (idnum>0 && found>=0)
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

		if (mode==-4)
			{
			int bqn = FALSE;
			int end, start = p.Section("Introduction", &end);
			for (int i=start; i<end && !bqn; ++i)
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
		if (!LoadCCS(folderfile+ ".html", memory))
			{
			Log(LOGERR, "Can't load offline page %s", url);	
			continue;
			}	

		// delete sections
		//memory = ExtractString(memory, "<BODY", "", );
		vars legend;
		const char *leg = strstr(memory, " = ");
		if (leg!=NULL)
		  legend = ExtractStringDel(memory, CString(leg-1,5), "", "<b>", -1, -1);
		if (!legend.IsEmpty())
			legend = "\n\n<div style=\"font-size:x-small; background-color:#f0f0f0;\"><i>"+legend.replace("<br>", " ")+"</i></div>";
		vars background = ExtractStringCCS(memory, "Note:")+ExtractStringCCS(memory, "NOTE:");
		vars exit = ExtractStringCCS(memory, "Return:")+ExtractStringCCS(memory, "RETURN:");
		vars descent = ExtractStringCCS(memory, "Description of");
		vars approach = ExtractStringCCS(memory, "Approach:")+ExtractStringCCS(memory, "Aproach:");
		vars access = ExtractStringCCS(memory, "Access:")+ExtractStringCCS(memory, "Acess:");
		vars data =  ExtractStringCCS(memory, "Description:");
		vars fotos;
		//data1 = "<div>"+data1.split("<b>").join("</div><div><b>")+"</div>";
		//descent = "<div>"+descent.split("<b>").join("</div><div><b>")+"</div>";
		

		// upload trip files
		{
		vara rfiles;
		CPage up(f, NULL, NULL, NULL);
		for (int j=0; j<filelist.GetSize(); ++j)
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
		if (rfiles.length()>0)
			{
			vars sep;
			for (int i=1; i<rfiles.length(); ++i) sep += ";";
			fotos = "{{pic|:"+rfiles[0]+sep+"}}";
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
		if (CompareSym(sym, rwsym, chgsym, codelist[code]))
			{
			chgsym.SetStr(ITEM_DESC, "");
			chgsym.SetStr(ITEM_REGION, "");
			chgsym.SetStr(ITEM_MATCH, "");
			int updates = UpdatePage(chgsym, code, p.lines, p.comment, title);
			}
				
		// update page
		int err = data.GetLength()<20;
		err += !SetSectionContent(sym.id, p, "Introduction", nobr(data, TRUE), files);
		err += !SetSectionContent(sym.id, p, "Approach", nobr(access)+"\n\n"+nobr(approach), files);
		err += !SetSectionContent(sym.id, p, "Descent", descent.replace("<br>", "\n\n")+nobr(legend), files); 
		err += !SetSectionContent(sym.id, p, "Exit", nobr(exit), files); 
		//err += !SetSectionContent(sym.id, p, "Red tape"	, redtape, files); 
		err += !SetSectionContent(sym.id, p, "Trip", fotos, files); 
		err += !SetSectionContent(sym.id, p, "Background", nobr(background), files); 

		if (err>0)
			{
			Log(LOGERR, "%d SECTION ERRORS DETECTED, skipping %s (%s)", err, title, sym.id);
			continue;
			}

		}


		// update
		int err = 0;
		vars basecomment = "Imported from AIC Infocanyon "+sym.id;
		Log(LOGINFO, "POST %d/%d %s %s [#%d]", i, list.GetSize(), basecomment, title, idnum);
		if (MODE>-1)
			{
			// do not update
			Log("\n"+p.lines.join("\n")+"\n");
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
			if (plines!=p2lines)
				{
				int n, nl,ol;
				vara newlines(plines,"\n");
				vara lines(p2lines,"\n");
				for (n=0; n<newlines.length() && n<lines.length(); ++n)
					if (newlines[n]!=lines[n])
						break;
				for (nl=newlines.length()-1, ol=lines.length()-1; nl>n && ol>n; --nl, --ol)
					if (newlines[nl]!=lines[ol])
						break;
				Log(LOGERR, "INCONSISTENT UPDATE for %s! #newlines:%d =?= #oldlines:%d diff %d-%d", title, newlines.length(), n<lines.length(), n, nl);
				vars dump;
				for (int i=n; i<=nl || i<=ol; ++i)
					{
					dump += MkString("#%d : ", i);
					dump += (i<=nl) ? newlines[i] : "";
					dump += MkString(" =!= ");
					dump += (i<=ol) ? lines[i] : "";
					}
				Log(dump);
				}

			sym.SetStr(ITEM_STARS, "Y");
			sym.SetStr(ITEM_MATCH, RWID+MkString("%d",idnum)+RWLINK);
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
			InternetGetCookie( "http://ropewiki.com", "ropewikiToken", ctoken, &size);

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
		CSym sym(id, stars+"*"+mul);
		list.Add(sym);
		return TRUE; 
}

int UploadStars(int code)
{
	const char *codestr = codelist[code].code;
	const char *user = codelist[code].staruser;
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
	while (i<symlist.GetSize() && bi<bslist.GetSize()) {
		CSym &sym = symlist[i];
		CSym &bsym = bslist[bi];
		int cmp = strcmp(sym.id, bsym.id);
		if (cmp==0) {
			// in bslist and symlist
			// bsym-->sym  sym-->bsym
			CString v = sym.GetStr(ITEM_STARS);
			//ASSERT(!strstr(sym.id, "Chute"));

			if (v[0]!=0 && v[0]!='0') {
				CSym vsym(bsym.data, v);
				double c = sym.GetNum(ITEM_CLASS);
				if (c>=0) vsym.SetNum(1, c);
				vlist.Add(vsym);
				}
			++bi; continue;
			}
		if (cmp>0) {
			// in bslist but not in symlist
			++bi; continue;
			}
		if (cmp<0) {
			// in bslist but not in symlist
			++i; continue;
			}
		}
	// vlist-> title, stars:count [possible duplicates]
	vlist.Sort();
	for (int i=vlist.GetSize()-1; i>0; --i) 
		if (vlist[i].id==vlist[i-1].id) {
			BOOL swap = vlist[i].GetNum(1)<vlist[i-1].GetNum(1) ||
				vlist[i].GetNum(0)<vlist[i-1].GetNum(0);
			//ASSERT(!strstr(vlist[i].id, "Chute"));
			if (swap)
				vlist.Delete(i);
			else
				vlist.Delete(i-1);
		}
	// vlist-> title, stars:count [highest class, highest vote per title, no duplicates]
	
	// query votes by user code
	CSymList rwlist;
	CString query = "[[Has page rating page::%2B]][[Has page rating user::"+url_encode(user)+"]]";
	vara prop("|Has page rating page|Has page rating|Has page rating multiplier", "|"); 
	GetASKList(rwlist, query+"|%3F"+prop.join("|%3F")+"|mainlabel=-", rwfstars);

	CSymList dellist;
	// compare vlist with rwlist -> list changes
	vlist.Sort();
	rwlist.Sort();
#ifdef DEBUG
	vlist.Save("vlist.csv");
	rwlist.Save("rwlist.csv");
#endif
	for (int i=vlist.GetSize()-1, ri=rwlist.GetSize()-1; ri>=0 && i>=0; ) {
		CSym &l = vlist[i];
		CSym &r = rwlist[ri];
		int cmp = strcmp(r.id, l.id);
		if (cmp==0) {
			//ASSERT(!strstr(r.id, "Chute"));
			if (r.GetStr(0)==l.GetStr(0))
				vlist.Delete(i);
			else
				Log(LOGINFO, "Updating %s => %s", r.Line(), l.Line());
			--i, --ri;
			continue;
			}
		if (cmp>0) {
			Log(LOGERR, "ERROR: This should never happen II ri=%d i=%d %s vs %s => Delete Vote for <", ri, i, r.Line(), l.Line());
			vara prop("|Has page rating page|Has page rating|Has page rating multiplier", "|"); 
			CSymList addlist, dellist;
			GetASKList(addlist, query+MkString("[[Has page rating page::%s]]", r.id)+"|%3F"+prop.join("|%3F"), rwftitleo);
			vars file = "delete.csv";
			dellist.Load(file);
			dellist.Add(addlist);
			dellist.Save(file);
			--ri;
			continue;
			}
		if (cmp<0) {
			Log(LOGINFO, "Adding %s", l.Line());
			--i;
			continue;
			}
		}
	// list-> pageid, stars:count [no duplicates]

	// chglist = name, stars:votes
	if (vlist.GetSize()==0) {
		Log(LOGINFO, "No star changes for %s", codestr);
		return FALSE;
	}

	// post new votes or changed votes
	// log in as RW_ROBOT_USERNAME [RW_ROBOT_PASSWORD]
	DownloadFile f;
	if (!Login(f))
		return FALSE;

	for (int i=0; i<vlist.GetSize(); ++i)
		{
		printf("Posting %d/%d votes...\r", i, vlist.GetSize());
		CString pagename = vlist[i].id; 
		pagename.Replace(" ", "_");
		pagename.Replace("%2C", ",");
		//pagename.Replace("+", "%2B");
		//pagename.Replace("&amp;", "&");
		CString stars = vlist[i].GetStr(0);
		vara starsuser(stars, "*");
		if (starsuser.length()<2)
			{
			starsuser.SetSize(2);
			starsuser[1] = "1";
			}
		vars target = "Votes:"+url_encode(pagename)+"/"+user; 
		vars url = RWBASE+"api.php?action=sfautoedit&form=Page_rating&target="+target+"&query=Page_rating[Page]="+url_encode(url_encode(pagename))+"%26Page_rating[Rating]="+starsuser[0]+"%26Page_rating[Multiplier]="+starsuser[1]+"%26Page_rating[User]="+url_encode(user);
		if (MODE>=0)
			Log(LOGINFO, "Setting %.256s", url);			
		if (MODE<=0)
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
	for (int code = 0; codelist[code].code!=NULL; ++code) 
		{
		if (codestr && strcmp(codelist[code].code, codestr)!=0)
			continue;
		if (!codelist[code].staruser)
			continue;
		UploadStars(code);
		}
	return TRUE;
}


#define FBLIST "\n*"
#define FBCOMMA "|"

#define CONDITIONS "Conditions:"

int UploadCondition(CSym &sym, const char *title, const char *credit, int trackdate = FALSE)
{
		vars condtxt = sym.GetStr(ITEM_CONDDATE);
		condtxt.Replace(FBCOMMA, ",");
		vara cond(condtxt,";");
		if (cond.length()==0)
			return FALSE;

		cond.SetSize(COND_MAX);		
		double newdate = CDATA::GetDate(cond[COND_DATE]);
		if (newdate<=0)
			return FALSE;
		if (newdate>RealCurrentDate+1)
			{
			Log(LOGERR, "Future dates not allowed %s > %s", CDate(newdate), CDate(RealCurrentDate));
			return FALSE;
			}

		vars comments = "Updated latest condition to "+cond[COND_DATE];

		//printf("EDITING %d/%d %s %s....\r", title);
		DownloadFile f;
		vars ptitle = CONDITIONS+vars(title).replace(" ","_")+"-"+vars(credit);
		if (trackdate)
			ptitle += "-"+cond[COND_DATE];
		CPage p(f, NULL, ptitle);

		BOOL isFacebook = strstr(credit,"@Facebook")!=NULL;
		if (isFacebook)
		  {
		  comments = "Created new Facebook condition";
		  if (cond[COND_TEXT].IsEmpty() || cond[COND_DATE].IsEmpty())
			  {
			  Log(LOGERR, "ERROR: Empty DATE/COMMENT for Facebook condition %s %s", cond[COND_DATE], title);
			  return FALSE;
			  }
		  if (cond[COND_TEXT][0]!='\"' || ExtractString(cond[COND_TEXT], "[", "", "]").IsEmpty())
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
			chg += p.Override("Water condition",	cond[COND_WATER]);
			chg += p.Override("Wetsuit condition", cond[COND_TEMP]);
			chg += p.Override("Difficulty condition", cond[COND_DIFF]);								
			chg += p.Override("Team time", cond[COND_TIME]);
			chg += p.Override("Team size", cond[COND_PEOPLE]);
			chg += p.Override("Questionable", cond[COND_NOTSURE]);
			// UPDATE Comments
			vara fblist(p.Get("Comments"), FBLIST);
			if (fblist.indexOf(cond[COND_TEXT])<0)
				{
				BOOL joined = FALSE;
				for (int i=1; i<fblist.length() && !joined; ++i)
					if (strstr(fblist[i], "["+cond[COND_LINK]))
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
				if (MODE>=-1 && nonquestionable)
					{
					Log(LOGINFO, "Non questionable %s", ptitle);
					return FALSE;
					}
				// update
				p.comment.push(MkString("Updated Facebook condition (added %d comments, %d data change)", cmt, chg));
				if (MODE<=0) p.Update();
				return TRUE;
				}
			return FALSE;
			}
		  }

		// check date
		vars olddate = p.Get("Date");
		if (!olddate.IsEmpty())
			if (newdate<=CDATA::GetDate(olddate) && MODE>=-1)
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
		p.Override("Water condition",	cond[COND_WATER]);
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
				p.Set("Comments", MkString("extracts from Facebook posts, click link for full details (privacy restrictions may apply)", cond[COND_USERLINK], cond[COND_USER])+FBLIST+cond[COND_TEXT]);
			else
				p.Set("Comments", cond[COND_TEXT]);
		else
			p.Set("Comments", MkString("Most recent conditions reported at %s", credit));
		p.comment.push(comments);
		if (MODE<=0) p.Update();
		return TRUE;
}


int UploadConditions(int mode, const char *codestr)
{
	if (!LoadBetaList(titlebetalist, TRUE, TRUE))
		return FALSE;

	for (int code = 0; codelist[code].code!=NULL; ++code) 
		{
		if (!codelist[code].betac->cfunc)
			continue;
		if (codestr && strcmp(codelist[code].code, codestr)!=0)
			continue;
		if (!codestr && !codelist[code].name)
			continue;

		Code &c = codelist[code];

		CSymList clist;
		if (MODE<-2 || MODE>0)
			clist.Load(filename(c.code));
		c.betac->cfunc(c.betac->ubase, clist);
		if (clist.GetSize()==0)
			Log(LOGERR, "ERROR: NO CONDITIONS FOR %s (%s)!", c.code, c.name);
		for (int i=0; i<clist.GetSize(); ++i)
			{
			printf("%s %d/%d     \r", c.code, i, clist.GetSize());
			if (IsSimilar(clist[i].id, RWTITLE))
				{
				// direct mapping
				UploadCondition(clist[i], clist[i].id.Mid(strlen(RWTITLE)), c.name);
				continue;
				}
			// possible multiple matches
			for (int b=0; b<titlebetalist.GetSize(); ++b)
			  if (clist[i].id==titlebetalist[b].id)			
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
	if (!fileregion || *fileregion==0)
		fileregion = "ALL";
	const char *ext = GetFileExt(fileregion);
	if (fileregion=="[")
		{
		GetASKList(idlist, vars(fileregion)+"|%3FHas pageid"+"|sort=Modification_date|order=asc", rwfpurgeid);
		}
	else if (!ext) {
		vars query = CString("[[Category:Canyons]]");
		if (!IsSimilar(fileregion, "ALL"))
			{
			// region or canyon 
			DownloadFile f;
			CPage p(f, NULL, fileregion);
			if (!p.Exists() || p.lines.length()<2)
				{
				Log(LOGERR, "ERROR: Page '%s' does not exist or is empty!", fileregion);
				return FALSE;
				}
			vars subquery;
			int i;
			for (i=0; i<p.lines.length() && !IsSimilar(p.lines[i],"{{"); ++i);
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
		GetASKList(idlist, query+"|%3FHas pageid"+"|sort=Modification_date|order=asc", rwfpurgeid);
		}
	else if (IsSimilar(ext,"csv"))
		idlist.Load(fileregion);
	else if (IsSimilar(ext,"log"))
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
			if (num>0)
				{
				idlist.Add(CSym(RWID+CData(num)));
				continue;
				}
			}
		}
	for (int i=0; i<idlist.GetSize(); ++i)
		{
		if (IsSimilar(idlist[i].id, "http"))
			{
			CString match = idlist[i].GetStr(ITEM_MATCH);
			const char *rwid = strstr(match, RWID);
			idlist[i].id = rwid ? CData(CDATA::GetNum(rwid+strlen(RWID))) : "";
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
	for (int i=5; i>0; --i)
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
		
	if (MODE==0)
		GetASKList(list, "[[Category:Disambiguation pages]]", rwfpurgetitle);
	else if (MODE==1)
		GetASKList(list, "[[Category:Regions]]", rwfpurgetitle);
	else
		GetRWList(list, fileregion);

	int n = list.GetSize();
	for (int i=0; i<n; ++i)
		{
		printf("%d/%d %s:", i, n, list[i].data);
		if (MODE==-2)
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
		list.Add(CSym("",fileregion));

	DownloadFile f;
	if (!Login(f))
		return;

	for (int i=0; i<list.GetSize(); ++i)
		{
		vars id = list[i].id;
		vars title = list[i].GetStr(ITEM_DESC);

		Log(LOGINFO, "REFRESHING %d/%d %s [%s]", i, list.GetSize(), title, id);

		CPage p(f, id, title);
		vara lines = p.lines;
		if (lines.length()==0)
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

	if (MODE<0)
	  for (int i=0; i<idlist.GetSize(); ++i)
		{
		CString &id = idlist[i].id.Trim("\" ");
		CString &title = idlist[i].data.Trim("\" ");
		// download page 
		printf("EDITING %d/%d %s %s....\r", i, idlist.GetSize(), id, title);
		CPage p(f, id, title);
#if 0
		vara prop("Fastest typical time|Slowest typical time|Fastest approach time|Slowest approach time|Fastest descent time|Slowest descent time|Fastest exit time|Slowest exit time","|");
		for (int j=0; j<prop.length(); ++j)
			UpdateProp(prop[j], "NULL", p.lines, TRUE);
		p.comment.push("Reset times");
#else
		UpdateProp(pname, pval, p.lines, TRUE);
		p.comment.push("Updated "+vars(prop));
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
		if (bsnum>0 && bslist.length()!=bsnum)
			return MkString("%d!=%g (NUM)", bslist.length(), num); // + bslist.join(" ; ");
		if (bslist.length()<bstlist.length())
			return MkString("%d<%d (TLIST)", bslist.length(), bstlist.length());
		return "";
}

int FixCheckEmpty(const char *str1, const char *str2, int mode  = 0)
{
	if (mode<=0 && *str1==0 && *str2!=0)
		return TRUE;
	if (mode>=0 && *str1!=0 && *str2==0)
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
	enum { D_PAGEID = 0, 
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
		D_TRLIST, D_TRTLIST,D_TRNUM,
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
	ASSERT( dstr.length()-1 != D_MAX);
	GetASKList(list, "[[Category:Canyons]]"+dstr.replace("|","|%3F")+"|sort=Modification_date|order=asc", rwflabels);

 


 

	for (int i=0; i<list.GetSize(); ++i)
		{
		vara comment;
		vara data( list[i].data ); data.SetSize(D_MAX);
		CSym sym( data[D_PAGEID], list[i].id);

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
		if (!(cmt=FixCheckList(data[D_BSNUM], data[D_BSLIST], data[D_BSTLIST])).IsEmpty())
			comment.push("Bad BSLIST " + cmt);
		if (!(cmt=FixCheckList(data[D_TRNUM], data[D_TRLIST], data[D_TRTLIST])).IsEmpty())
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
			if (data[D_LOC]!="Mars")
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

		if (data[D_RAPS]=="0") data[D_RAPS]="";
		if (data[D_LONG]=="0") data[D_LONG]="";
		if (FixCheckEmpty(data[D_RAPINFO], data[D_RAPS],-1))
			comment.push("Bad RAPINFO");
		if (FixCheckEmpty(data[D_LONGINFO], data[D_LONG],-1))
			comment.push("Bad LONGINFO");
		if (FixCheckEmpty(data[D_HIKEINFO], data[D_HIKE],-1))
			comment.push("Bad HIKEINFO");
		if (FixCheckEmpty(data[D_TIMEINFO], data[D_TIMEFAST]+data[D_TIMESLOW],-1))
			comment.push("Bad TIMEINFO");
		if (FixCheckEmpty(data[D_ACASUMMARY], data[D_RAPS]+data[D_LONG]+data[D_HIKE]+data[D_TIMEFAST]+data[D_TIMESLOW],-1))
			comment.push("Bad SUMMARYINFO");

		// add bad list
		if (comment.length()>0)
			{
			sym.SetStr(ITEM_MATCH, comment.join(";"));
			Log(LOGERR, "SYMERROR: %s [%s] : %s", sym.GetStr(ITEM_DESC), sym.id, sym.GetStr(ITEM_MATCH));
			badlist.Add(sym);
			}
		}
	}

	if (badlist.GetSize()>0)
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
		if (sectionscmp.length()==0)
			{
			for (int j=0; sections[j]!=NULL; ++j)
				sectionscmp.push(vars(sections[j]).lower().replace(" ",""));
			}


		int cleanup = 0, moved = 0;
		if (lines.length()>0) {
			// ================ FIX DUPLICATED SECTIONS
			for (int i=0; i<lines.length(); ++i) 
				if (lines[i][0]=='=')
				{
					if (lines[i].Replace("== ", "==")>0 || lines[i].Replace(" ==", "==")>0)
						++moved;

					int n = 0, next = -1;
					vars section = CString(lines[i]).Trim(" =");
					for (n=i+1; n<lines.length() && next<0; ++n)
						if (lines[n][0]=='=')
							{
							vars nsection = CString(lines[n]).Trim(" =");
							if (nsection==section)
								next = n;
							}

					if (next>=0)
						{
						// move all to next section
							lines.RemoveAt(i), ++moved;
							while (i<lines.length() && lines[i][0]!='=') 
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
			for (int i=0; i<lines.length(); ++i) {
				int aka = 0;
				if (IsSimilar(lines[i], ALSOKNOWNAS) || (aka = lines[i].indexOf(ALSOKNOWNAS))>=0 || (aka = lines[i].indexOf(ALSOKNOWNAS2))>=0)
				{
					CString prefix, postfix, fix;
					int start = aka+strlen(ALSOKNOWNAS);
					int end = lines[i].Find( ".",  start);
					if (end<0) end = lines[i].GetLength();
					fix = lines[i].Mid(start, end-start).Trim(" \t\r\n:.");
					if (aka>0)
						prefix = lines[i].Mid(0, aka).Trim(" \t\r\n:.*");
					if (end>0)
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
						akalist.Append( naka );
						}
					vara pp;
					if (!prefix.IsEmpty())
						pp.push(prefix);
					if (!postfix.IsEmpty())
						pp.push(postfix);
					if (pp.length()==0)
						lines.RemoveAt(i--);
					else
						{
						lines[i] =  pp.join(". ");
						Log(LOGWARN, "CHECK W/O AKA LINE %s: '%s'", title, lines[i]);
						}
				}
			  }
			if (akalist.length()>0) 
				{
				UpdateProp("AKA", akalist.join(";"), lines);
				comment.push(MkString("Added AKA '%s'",akalist.join(";")));
				}

			vars oldaka = GetProp("AKA", lines);
			vars newaka = UpdateAKA(oldaka, "");
			if (newaka!=oldaka) 
				{
				UpdateProp("AKA", newaka, lines, TRUE);
				comment.push(MkString("Fixed AKA"));
				}


			vara duplink;
			vars section;
			for (int i=0; i<lines.length(); ++i) 
				{
				if (strncmp(lines[i], "==", 2)==0)
					section = lines[i];

				// ================ MANUAL FIXES

				const char *v = NULL;

				// fix {{pic|size=X
				//if (IsSimilar(section,"==Descent") || IsSimilar(section,"==Approach"))
					{
					vars &line = lines[i];
					int s = line.Find("{{pic");
					int e = s>=0 ? line.Find("}}", s) : -1;
					if (s>=0 && e>=0)
						{
						vars str = line.Mid(s, e-s);
						if (!strstr(str, "|size=X|"))
							if (IsPicX(str))
							{
							// make size X
							vars nstr;
							vars size = ExtractString(str, "size=", "", "|");
							if (size.IsEmpty())
								line.Replace("{{pic|", "{{pic|size=X|");
							else
								line.Replace("size="+size, "size=X");

							Log(LOGWARN, "Fixed {{pic|size=X}} '%s'->'%s' in %s", str, nstr, title);
							vars c = "Fixed {{pic}} to use {{pic|size=X}}";
							if (comment.indexOf(c)<0) 
								comment.push(c);
							}
						}
					}
				if (IsSimilar(section,"==Descent") || IsSimilar(section,"==Approach"))
					{
					int media = TRUE;
					int s = lines[i].Find("[[Media:");
					int e = s>=0 ? lines[i].Find("]]", s) : -1;
					if (s<0 || e<0)
						{
						s = lines[i].Find("[[File:");
						e = s>=0 ? lines[i].Find("]]", s) : -1;
						media = FALSE;
						}

					if (s>=0 && e>=0)
						{
						e += 2;
						vars str = lines[i].Mid(s, e-s);
						if (media || IsPicX(str))
							{
							vara a(strstr(str,":"), "|");
							if (a.length()>0)
								{
								vars file = a[0];
								vars desc = a.last();

								vars nstr = "{{pic|size=X|"+file+" ~ "+desc+"}}";
								lines[i].Replace(str, nstr);
								Log(LOGWARN, "Fixed {{pic|size=X}} '%s'->'%s' in %s", str, nstr, title);
								vars c = MkString("Fixed %s to use {{pic|size=X}}", media ? "[[Media:]]" : "[[File:]]");
								if (comment.indexOf(c)<0) 
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
				if (IsSimilar(lines[i], v="|AKA="))
					{
						vars newline = v+UpdateAKA("", lines[i].Mid(5));
						if (lines[i]!=newline) {
							Log(LOGINFO, "Cleaned AKA for %s: %s => %s", title, lines[i], newline);
							comment.push("Cleaned AKA");
							lines[i] = newline;
							}
					}

				// delete bad shuttle
				if (IsSimilar(lines[i], v="|Shuttle=")) {
					CString val = GetToken(lines[i], 1, '=').Trim();
					if (!val.IsEmpty())
						if (CDATA::GetNum(val)<0 && !IsSimilar(val, "No") && !IsSimilar(val, "Yes"))
							{
							Log(LOGWARN, "WARNING: Deleted bad Shuttle for %s: '%s'", title, val);
							lines[i] = v; 
							comment.push("Fixed bad Shuttle");
							}
					}

				// delete bad permits
				if (IsSimilar(lines[i], v="|Permits=")) {
					CString val = GetToken(lines[i], 1, '=').Trim();
					if (!val.IsEmpty())
						{
						const char *valid[] = { "No", "Yes", "Restricted", "Closed", NULL};
						if (!IsMatch(val, valid))
								{
								Log(LOGWARN, "WARNING: Deleted bad Permits for %s: '%s'", title, val);
								lines[i] = v; 
								comment.push("Fixed bad Permits");
								}
						}
					}

				// delete bad season
				if (IsSimilar(lines[i], v="|Best season=")) {
					vars val = lines[i].Mid(strlen(v)).Trim(), oval = val;
					IsSeasonValid(GetToken(val, 0, "!").Trim(), &val);
					if (SeasonCompare(val)!=SeasonCompare(oval))
						{
						Log(LOGWARN, "WARNING: Cleaned bad Best Season for %s: '%s' => '%s'", title, oval, val);
						lines[i]= v + val; 
						comment.push("Cleaned Best Season");
						if (InvalidUTF(val))
							{
							Log(LOGWARN, "WARNING: Deleted bad Best Season for %s: ='%s'", title, val);
							lines[i]= v; 
							comment.push("Cleaned Best Season");
							}
						}
					}
				// delete manual disambiguation
				if (strstr(lines[i], "other features with similar name")) {
					CString t1 = GetToken(title, 0, '(').Trim();
					CString t2 = GetToken(GetToken(lines[i], 2, '['), 0, '(').Trim(" []");
					if (t1==t2)
						lines.splice(i, 1), comment.push("Deleted redundant disambiguation line");
					else
						Log(LOGWARN, "WARNING: No auto-disambiguation '%s'!='%s' : %s -> %s", t1, t2, title, lines[i]);
				}
				// Ouray book
				if (strstr(lines[i], "?product=Ouray"))
					Log(LOGINFO, "%s is referencing product=Ouray", title);
				// replace < >
				if (strstr(lines[i],"&lt;"))  {
					for (int j=i; j<lines.length(); ++j)
						lines[j].Replace("&lt;", "<");
					comment.push("Replaced &lt; with <");
				}
				if (strstr(lines[i],"&gt;"))  {
					for (int j=i; j<lines.length(); ++j)
						lines[j].Replace("&gt;", ">");
					comment.push("Replaced &gt; with >");
				}
				if (strstr(lines[i],"&quot;"))  {
					for (int j=i; j<lines.length(); ++j)
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
					lines.splice(i, 1),--i,comment.push("Deleted old Grand Canyoneering link");
					if (lines[i].IsEmpty() && lines[i+1].IsEmpty())
						lines.splice(i, 1);
				}
				// remove grand canyoneering links
				if (strstr(lines[i], "http://www.toddshikingguide.com/AZTechnicalCanyoneering/index.htm") || strstr(lines[i], "www.amazon.com/Arizona-Technical-Canyoneering-Todd-Martin")) {
					lines.splice(i, 1),--i,comment.push("Deleted old Arizona Technical Canyoneering link");
					if (lines[i].IsEmpty() && lines[i+1].IsEmpty())
						lines.splice(i, 1);
				}
				//remove bad links
				const char *badlink = "http://ropewiki.com/User:Grand_Canyoneering_Book?id=";
				if (strstr(lines[i], badlink) && !strchr(lines[i].Mid(strlen(badlink)), '_')) {
					lines.splice(i, 1),--i,comment.push("Deleted bad link");
					if (lines[i].IsEmpty() && lines[i+1].IsEmpty())
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
						lines[i] = "<div><b>"+lines[i].replace(":",":</b>").replace("<div>", "</div>\n<div>");
					const char *comm = "fixed bold list2";
					if (comment.indexOf(comm)<0)
						comment.push(comm);
					}
				if (lines[i]=="<div><b>Especies amenazadas:</b> En todos los habitats viven animales y plantas que merecen nuestro respeto</div>")
					{
					lines.RemoveAt(i--);
					comment.push("removed 'Especies amenazadas'");
					continue;
					}
				if (IsSimilar(lines[i], "<div><b>Combinaci"))
					{
					vars line = lines[i];
					vars comp = stripAccentsL(GetToken(stripHTML(lines[i]), 1, ':')).Trim(" .");
					if (comp=="no" || comp=="sin combinacion" || comp=="no necesaria" || comp=="no necesario")
						{
						lines.RemoveAt(i--);
						comment.push("fixed  'Combinacion vehiculos'");
						continue;
						}
					if (!IsSimilar(lines[i-1],"==Approach"))
						{
						lines.RemoveAt(i--);
						int e;
						for (e=i; e<lines.length() && !IsSimilar(lines[e],"==Approach"); ++e);
						if (e==lines.length())
							Log(LOGERR, "Inconsisten Combinac for %s", title);
						else
							lines.InsertAt(e+1, line+"\n");
						comment.push("fixed  'Combinacion vehiculos'");
						continue;
						}
					}

				if (strncmp(lines[i],"==",2)==0)
					{
					vars line = lines[i].lower().replace(" ","");
					int s = sectionscmp.indexOf(line);
					if (s>=0 && lines[i]!=sections[s])
						{
						lines[i] = sections[s];
						const char *comm = "fixed malformed sections";
						if (comment.indexOf(comm)<0)
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
					if (CDATA::GetNum(GetToken(lines[i],1,'='))!=InvalidNUM)
						lines[i] = "|Location type=", comment.push("Fixed bad Location Type");

				// |Caving length= |Caving depth=
				if (IsSimilar(lines[i], "|Caving depth="))
						lines[i].Replace("Caving depth","Depth"), comment.push("Fixed bad depth");
				if (IsSimilar(lines[i], "|Caving length="))
						lines[i].Replace("Caving length","Length"), comment.push("Fixed bad length");

				// ================ BETASITE CLEANUP
				// check if should be bulleted list
				if (IsSimilar(section,"==Beta") || IsSimilar(section,"==Trip"))
					{
					lines[i].Trim();

					vars pre, post;
					if (lines[i][0]!='*' && Code::IsBulletLine(lines[i], pre, post)>0)
						if (pre.IsEmpty() && post.IsEmpty())
							{
							// make bullet line
							lines[i] = "*"+lines[i];
							// delete empty lines
							while (lines[i-1].trim().IsEmpty())
								lines.RemoveAt(--i);
							++cleanup;
							}

					// Bogley Trip Report
					if (stricmp(lines[i], "Bogley Trip Report")==0) {
						lines.RemoveAt(i--), ++cleanup;
						continue;
						}

					// no empty lines
					if (lines[i].IsEmpty() && (lines[i-1][0]=='*' || lines[i+1][0]=='*') && lines[i+1][0]!='=') {
						lines.RemoveAt(i--), ++cleanup;
						continue;
						}
					}

				const char *httplink = strstr(lines[i], "http");
				// if link present
				if (httplink) 
				  if (IsSimilar(section,"==Beta") || IsSimilar(section,"==Trip"))
					{
					int code = GetCode(httplink);
					if (code<=0) continue;
					Code &cod = codelist[code];

					// check if bulleted list
					vars pre, post;
					if (cod.IsBulletLine(lines[i], pre, post)<=0)
						{
						Log(LOGERR, "WARNING: found non-bullet link at %s : %.100s", title, lines[i]);
						continue;
						}

					// found code! now search for id
					int found = cod.FindLink(httplink);
					if (found<0) {
						BSLinkInvalid(title, httplink);
						if (MODE<=-2)
							lines.RemoveAt(i--), ++cleanup;
						continue; // invalid
						}

					// update BS link
					if (duplink.indexOf(cod.list[found].id)>=0) {
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
					if (newline==lines[i] && IsSimilar(section,"==Trip")==cod.IsTripReport())
						continue; // no change

					// update BS link					
					Log(LOGINFO, "Cleaned up BSLink for %s: '%s' => '%s'", title, lines[i], newline);
					lines[i] = newline;

					// relocate
					if (IsSimilar(section,"==Trip")!=cod.IsTripReport())
						{
						lines.RemoveAt(i--);
						int end, start = FindSection(lines, cod.IsTripReport() ? "==Trip" : "==Beta", &end);
						if (start>0)
							lines.InsertAt(end>=0 ? end : start, newline);
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
	if (!fileregion || *fileregion==0)
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

	for (int i=0; i<idlist.GetSize(); ++i)
		{
		CString &id = idlist[i].id;
		CString &title = idlist[i].data;
		CPage p(f, id, title);

		printf("%d/%d %s %s    \r", i, idlist.GetSize(), id, title);

		if (!FixBetaPage(p, MODE))
			continue;

		if (MODE<1)
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

	if (MODE<0)
	  for (int i=0; i<idlist.GetSize(); ++i)
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
		if ((user!=RW_ROBOT_USERNAME  && user!="Barranquismo.net") || oldid.IsEmpty() || IsSimilar(oldcomment, "Undo")) {
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
		if (start>=end)
			{
			Log(LOGERR, "ERROR: no good kml %.128s", name);
			return MkString("<Folder>\n<name>%s</name>\n", name)+CString("</Folder>\n");
			}

		// output file
		//data->write(list[0]);
		CString kml(memory+start, end-start);
		if (id && id[0]!=0)
			{
			CString ids(id);
			kml.Replace("<name>", "<name>"+ids);
			kml.Replace("<NAME>", "<NAME>"+ids);
			kml.Replace("<Name>", "<Name>"+ids);
			}
		if (true)
			{
			CString newicon = "http://maps.google.com/mapfiles/kml/shapes/open-diamond.png";
			kml.Replace("http://caltopo.com/resource/imagery/icons/circle/FF0000.png", newicon);
			kml.Replace("http://caltopo.com/resource/imagery/icons/circle/000000.png", newicon);
			kml.Replace("http://caltopo.com/static/images/icons/c:ring,FF0000.png", newicon);
			kml.Replace("http://caltopo.com/static/images/icons/c:ring,000000.png", newicon);						
			}
		return MkString("<Folder>\n<name>%s</name>\n", name)+kml+CString("</Folder>\n");
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

				for (int i=1; i<list.GetSize(); ++i)
					{
					CStringArray line;
					split(line, list[i], list[i].GetLength(), "</NetworkLink>");

					CString name = xmlval(line[0], "<name", "</name").Trim();
					CString url = xmlval(line[0], "<href", "</href").Trim();

					// download file
					//for (int u=0; u<urls.length(); ++u) {
#ifdef DEBUG
					url.Replace(server, "localhost");
#endif
						if (f.Download(url))
							{
							Log(LOGERR, "ERROR: can't download kml %.128s (%.128s)", name, url);
							return FALSE;
							}

						size += f.size;
						CString id;
						if (name[0]=='#')
							{
							++num;
							int i=1;
							while (i<name.GetLength() && isdigit(name[i]))
								++i;
							id = CString(name, i+1); 
							// LocationID
							if (strlen(id)==2)
								{
								name = name.Mid(2);
								id.Insert(1, name);
								id = id.Mid(1);
								}
							}
						data->write(KMLFolderMerge(name, f.memory, id));
						// close folder
						if (line.GetSize()>1)
							data->write(line[1]);
						}
					//}

				Log(LOGINFO, "merged %d kml #%d %dKb", list.GetSize()-1, num, size/1024);
				return TRUE;
}








int RunExtract(DownloadKMLFunc *f, const char *base, const char *url, inetdata *out, int fx)
{
	if (!f) return FALSE;
	// find last http
	int len  = strlen(url)-1;
	while (len>0 && !IsSimilar(url+len, "http"))
		--len;
	if (len<0) return FALSE;
	url += len;

	#ifndef DEBUG
		__try { 
	#endif

			// try
	return f(base, url, out, fx);

	#ifndef DEBUG
	} __except(EXCEPTION_EXECUTE_HANDLER) 
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

	CString url = "http://ropewiki.com/Special:Ask/-5B-5BCategory:Canyons-5D-5D-5B-5B"+CString(id)+"-5D-5D/-3FHas-20KML-20file/-3FHas-20BetaSites-20list/-3FHas-20TripReports-20list/format%3DCSV/headers%3Dhide/mainlabel%3D-2D/offset%3D0";
	if (f.Download(url))
		Log(LOGERR, "ERROR: can't download betasites for %s url=%.128s", id, url);

	vara bslist(vars(f.memory).Trim(trim),",");
	if (bslist.length()==0)
		return FALSE;

	char num = 'b';
	out->write( KMLStart() );
	out->write( KMLName(id) );

	for (int i=0; i<bslist.length(); ++i)
		{
			bslist[i].Trim(trim);
			if (i==0)
				{
				// kml
				if (strstr(bslist[0], "ropewiki.com"))
					if (!f.Download(bslist[0]))
						out->write(KMLFolderMerge(MkString("rw:%s",id), f.memory, "rw:"));
				continue;
				}

			inetmemory mem;
			int code = GetCode(bslist[i]);
			if (code>0)
				if (RunExtract( codelist[code].betac->kfunc, codelist[code].betac->ubase, bslist[i], &mem, fx))
					{
						out->write(KMLFolderMerge(MkString("%s:%s", codelist[code].code, id), mem.memory, MkString("%s:", codelist[code].code)));
					++num;
					}
		}
	out->write( KMLEnd() );
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
	url.Replace("&amp;","&");
	url.Replace("%3D","=");
	url.Replace("&ext=.kml","");
	url.Replace("&ext=.gpx","");
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

	for (int i=1; codelist[i].betac->ubase!=NULL; ++i) 
	  if (strstr(url, codelist[i].betac->ubase))
		  if (RunExtract( codelist[i].betac->kfunc, codelist[i].betac->ubase, url, out, fx))
			  return TRUE;

	Log(LOGWARN, "Could not extract KML for %s", url);
	return FALSE;
}



int ExtractBetaKML(int MODE, const char *url, const char *file)
{
	CString outfile = "out.kml";
	if (file!=NULL && *file!=0)
		outfile = file;

	Log(LOGINFO, "%s -> %s", url, outfile);
	inetfile out = inetfile(outfile);
	KMLExtract(url, &out, MODE>0);
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
	while(!ExtractStringDel(out, "<script", "", "</script>").IsEmpty());
	while(!ExtractStringDel(out, "<style", "", "</style>").IsEmpty());
	while(!ExtractStringDel(out, "<p><strong>Hits:", "", "<p>").IsEmpty());
	while(!ExtractStringDel(out, "<div class=\"systemquote\">", "", "</div>").IsEmpty());
	while(!ExtractStringDel(out, "<p class=\"small\"> WassUp", "", "\n").IsEmpty());
	while(out.Replace("\t"," "));
	while(out.Replace("\r","\n"));
	while(out.Replace("  "," "));
	while(out.Replace("\n\n","\n"));
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
	for (int i=1; codelist[i].code!=NULL; ++i) 
		{
		if (!codelist[i].name || codelist[i].IsBook())
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
	for (int i=1; codelist[i].code!=NULL; ++i) 
		{
		if (!codelist[i].name || codelist[i].IsBook())
			continue;

		Code *c = &codelist[i];
		vars rwlang = c->translink ? c->translink : "en";
		vars flag = MkString("[[File:rwl_%s.png|alt=|link=]]", rwlang );
		file.fputstr(MkString("| %s[%s  %s] || %s || %s || %s || %s", flag, c->betac->umain, c->name, c->xregion, c->xstar, c->xmap, c->xcond));
		file.fputstr("|-");	

		if ( !c->betac )
			Log(LOGERR, "NULL betac for %s %s", c->code, c->name);

		if ( (*c->xmap==0) != (c->betac->kfunc==NULL))
			Log(LOGERR, "Inconsistent xmap vs DownloadKML for %s %s", c->code, c->name);

		if ( (*c->xcond==0) != (c->betac->cfunc==NULL))
			Log(LOGERR, "Inconsistent xcond vs DownloadConditions for %s %s", c->code, c->name);

		if ( (c->staruser==NULL || *c->staruser==0) != (c->xstar==NULL || *c->xstar==0))
			Log(LOGERR, "Inconsistent xstar vs staruser for %s %s", c->code, c->name);

		printf("Processing #%d %s...            \r", i, c->code);

		// check if user page exists

		if (c->staruser && *c->staruser!=0)
			{
			vars page = "User:"+vars(c->staruser);
			CPage p(f, NULL, page);
			if (!p.Exists())
				{
				Log(LOGINFO, "Creating page "+page);
				p.lines.push("#REDIRECT [[Beta Sites]]");
				p.comment.push("Created Beta Site redirect page "+page);
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
		if (list.GetSize()>0 && MODE>0)
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
			if (f.Download(url2,"test2.html"))
				{
				Log(LOGERR, "Could not download RWLANG url '%s' for code '%s'", url2, c->code);
				continue;
				}

			vars memory2 = reduceHTML(f.memory);
			memory2.Replace(url2, url);

			if (strcmp(memory, memory2)!=0)
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
	for (int i=0; i<idlist.GetSize(); ++i) {
		if (!strstr(idlist[i].id, disambiguation)) {
			if (ignorelist.Find(idlist[i].id)<0)
				Log(LOGWARN, "WARNING: No %s in page %s", disambiguation, idlist[i].id);
			idlist.Delete(i--);
			continue;
		}
		if (vara(idlist[i].data, ";").length()<2)
			if (ignorelist.Find(idlist[i].id)<0)
				Log(LOGWARN, "WARNING: Disambigation page '%s' has <2 entries '%s'", idlist[i].id, idlist[i].data);
	}

	/*	
	CString pname = GetToken(prop, 0, '=');
	CString pval = UTF8(GetToken(prop, 1, '='));
	Log(LOGINFO, "REPLACING %s=%s for %d", pname, pval, idlist.GetSize());
	*/

	// add more if needed
	CSymList newidlist, rwlist;
	GetASKList(rwlist, CString("[[Category:Canyons]]")+"|%3FLocated in region"+"|sort=Modification_date|order=asc", rwfregion);
	CSymList namelist;
	for (int i=0; i<rwlist.GetSize(); ++i)
		{
		CString name = Disambiguate(rwlist[i].id);
		int f;
		if ((f=namelist.Find(name))<0)
			namelist.Add(CSym(name,rwlist[i].id));
		else
			{
			namelist[f].data += ","+rwlist[i].id;
			name += disambiguation;
			if (idlist.Find(name)<0) {
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

	for (int i=0; i<namelist.GetSize(); ++i)
		{
		int id = -1;
		vars name = namelist[i].id;
		vars data = namelist[i].data;
		vara dlist(data);
		if (dlist.length()<2)
			continue;
		if ((id=dlist.indexOf(name))>=0 && ignorelist.Find(name)<0)
			{
			// base name == item => move it to item ( )?
			int f;
			if ((f=rwlist.Find(name))<0)
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
			CSym sym(name, problem+newname+",,"+data);
			movebeta.Add(sym);
			}
		}
	if (movefile)
		movebeta.Save(movefile);

	if (MODE<1)
		idlist = newidlist;

	idlist.Sort();

	if (!Login(f))
		return FALSE;

   for (int i=0; i<idlist.GetSize(); ++i)
	{
	CString title = idlist[i].id;
	vars redirect = "#REDIRECT [["+title+"]]";
	for (int pass=0; pass<2; ++pass)
		{
		if (pass>0)
			title.Replace(disambiguation, "");
		CPage p(f, NULL, title);
		vara &lines = p.lines;
		vara &comment = p.comment;
		printf("EDITING %d/%d %s %s....\r", i, idlist.GetSize(), "", title);
		// update lines
		if (lines.length()==0) 
			{
			// NEW DISAMBIGUATION
			 if (pass==0)
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
		else if (pass==0 && !IsSimilar(lines[0],"{{Disambiguation")) 
			{
			// FIX DISAMBIGUATION
			int error = FALSE;
			vars pagename = Disambiguate(title);
			for (int i=1; i<lines.length() && !error; ++i) {
				vars link = GetToken(ExtractString(lines[i], "[[", "", "]]"), 0, '|');
				if (link.IsEmpty()) continue;

				vars linkname = Disambiguate(link);
				if (linkname!=pagename) {
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

			lines = vara("{{Disambiguation}}\n","\n");
			comment.push("Updated auto-disambiguation");
			}
		else if (pass==1 && !IsSimilar(lines[0],redirect)) 
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
	name.Replace("_"," ");
	name.Replace("%2C",",");
	return name;
}


int SimplynameBeta(int mode, const char *file, const char *movefile)
{
	CSymList symlist;
	symlist.Load(file);
	Log(LOGINFO, "Processing %d syms from %s", symlist.GetSize(), file);

	// regions
	if (MODE<0)
	{
	CSymList regions;
	GetASKList(regions, "[[Category:Regions]]|%3FHas region level|%3FLocated In Regions", rwfregion); 
	regions.SortNum(0);
	for (int i=0; i<regions.GetSize(); ++i)
		symlist.Add(CSym(regions[i].GetStr(1),regions[i].id));
	}

	// find duplicates
	CSymList duplist = symlist;
	for (int i=0; i<duplist.GetSize(); ++i)
		duplist[i].id = stripAccentsL(symlist[i].GetStr(ITEM_DESC));
	duplist.Sort();
	for (int i=1; i<duplist.GetSize(); ++i)
		if (duplist[i].id == duplist[i-1].id)
			Log(LOGERR, "NAME OVERLAP! '%s' vs '%s'", duplist[i].GetStr(ITEM_DESC), duplist[i-1].GetStr(ITEM_DESC));

	// simplify names
	for (int i=0; i<symlist.GetSize(); ++i)
		{
		vars dup;
		vars name = symlist[i].GetStr(ITEM_DESC);
		vars sname = stripAccents(name);
		if (MODE>=1)
			sname = Capitalize(sname);
		if (MODE>=2)
			{
			sname.Replace(" de ", " ");
			sname.Replace(" las ", " ");
			}
		if (name==sname) // || IsSimilar(name, UTF8("Cañón")))
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
	for (int i=0; i<symlist.GetSize(); ++i)
	{
		CString from = PageName(symlist[i].id);
		CString to = PageName(symlist[i].GetStr(ITEM_DESC));
		if (from.IsEmpty() || to.IsEmpty() || from==to || strstr(from, RWID))
			{
			//Log(LOGERR, "Skipping line '%s'", symlist[i]);
			continue;
			}

		if (strchr(from, ':')>0)
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
		GetASKList(regionlist, MkString("[[Located in region::%s]]", url_encode(from))+"|%3FHas pageid", rwftitleo); 
		/*
		vars query = "[[Category:Canyons]]";
		// get regionlist
		if (regionlist.GetSize()>0)
			query = "[[Category:Regions]]";
			*/

		// move page
		GetASKList(list, MkString("[[%s]]", url_encode(from)), rwftitleo);
		BOOL move = TRUE;
		if (list.GetSize()!=1)
			{
			Log(LOGERR, "ERROR MOVE FROM %s does not exist", from);
			move = FALSE;
			}
		if (CheckPage(to))
			{
			Log(LOGERR, "ERROR MOVE TO %s already exists", to);
			move = FALSE;
			}
		if (!move && MODE<1)
			continue;
		if (move)
			{
			CPage p(f, NULL, from);
			p.comment.push(comment);
			p.Move(to);
			}

		// regionlist
		for (int r=0; r<regionlist.GetSize(); ++r)
			{
			CString &title = regionlist[r].id;
			vars prop = !regionlist[r].GetStr(0).IsEmpty() ? "Region" : "Parent region";
			// download page 
			
			printf("UPDATING %d/%d %s %s....\r", r, regionlist.GetSize(), "", title);
			CPage p(f, NULL, title);
			UpdateProp(prop, to, p.lines, TRUE);
			p.comment.push("Updated "+prop+"="+to);
			p.Update();
			}
		if (regionlist.GetSize()>0)
			continue;
		}

		// votes
		{
		CSymList list;
		GetASKList(list, MkString("[[Category:Page ratings]][[Has page rating page::%s]]", url_encode(from)), rwftitleo);
		for (int l=0; l<list.GetSize(); ++l)
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
		for (int l=0; l<list.GetSize(); ++l)
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
		for (int l=0; l<list.GetSize(); ++l)
			{
			vars lfrom = list[l].id;
			vars lto = lfrom.replace(from, to);
			CPage p(f, NULL, lfrom);
			p.Set("Location", to, TRUE);
			p.comment.push(comment);
			p.Update();
			p.Move(lto);

			vars ffrom = lfrom.replace("References:","File:")+".jpg";
			vars fto = ffrom.replace(from, to);
			CPage pf(f, NULL, ffrom);
			p.comment.push(comment);
			p.Move(fto);
			}
		}

		// files (banner, kml, pics, etc)
		const char *assfiles[] = { ".kml", ".pdf", "_Banner.jpg", NULL };
		for (int n=0; assfiles[n]!=NULL; ++n)
		{
		CSymList list;
		GetASKList(list, MkString("[[File:%s%s]]", url_encode(from), assfiles[n]), rwftitleo);
		for (int l=0; l<list.GetSize(); ++l)
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
		if (f.Download("http://ropewiki.com/Special:ListFiles?limit=50&ilsearch="+url_encode(from)))
			{
			Log(LOGERR, "ERROR: can't download Special:ListFiles %s", from);
			continue;
			}

		vara files(f.memory, "<a href=\"/File:");

		if (files.length()>1)
			Log(LOGWARN, "MOVING RESIDUE Files...");
		for (int l=1; l<files.length(); ++l)
			{
			vars file = "File:"+url_decode(ExtractString(files[l], "", "", "\""));
			file.Replace("_", " ");
			file.Replace("&#039;", "'");
			//if (MODE<1)
			//else
			if (done.indexOf(file)<0)
			if (!strstr(file, "_Banner.jpg") && !strstr(file, ".kml"))
				{
				done.push(file);
				vars lfrom = file;
				vars lto = lfrom.replace("File:"+from, "File:"+to);
				if (lfrom!=lto)
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
	for (int i=0; i<symlist.GetSize(); ++i)
	{
		CString from = PageName(symlist[i].id);
		CPage p(f, NULL, from);
		if (p.lines.length()>0 && IsSimilar(p.lines[0],"{{Canyon"))
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
	if (rdiff<0) return -1;
	if (rdiff>0) return 1;
	rdiff = GetCode((*arg1)->id) - GetCode((*arg2)->id);
	if (rdiff<0) return -1;
	if (rdiff>0) return 1;
	return 0;
}


int TestBeta(int mode, const char *column, const char *code)
{
	vara hdr(vars(headers).replace("ITEM_", "").replace(" ",""));
	int col = hdr.indexOf(column)-1;
	if (col<0)
		{
		Log(LOGERR, "Invalid column '%s'", column);
		return FALSE;
		}

	// set NULL column
	CSymList bslist;
	LoadBetaList(bslist);
	for (int i=0; i<rwlist.GetSize(); ++i)
		{
		CSym &sym = rwlist[i];
		sym.SetStr(ITEM_NEWMATCH, sym.GetStr(col) );
		sym.SetStr(col, "");
		}

	// -getbeta [code]
	DownloadBeta(MODE=1, code);

	// load results
	vars file = filename(CHGFILE);
	CSymList chgfile;
	chgfile.Load(file);
	// sort results by RWID
	for (int i=0; i<chgfile.GetSize(); ++i)
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
		if (sym.index==InvalidNUM)
			{
			Log(LOGERR, "Unexpected invalid RWID %s", match);
			chgfile.Delete(i--);
			continue;
			}
		}
	chgfile.Sort((qfunc *)cmprwid);
	chgfile.Save(file);

	// process sorted list by RWID
	for (int i=0; i<chgfile.GetSize();)
		{
		double index = chgfile[i].index;
		int f = rwlist.Find(RWID+CData(index));
		if (f<0) 
			{
			Log(LOGERR, "Inconsistend RWID:%s", CData(index));
			for (int n=0; index == chgfile[i].index; ++n)
				chgfile.Delete(i);
			continue;
			}

		for (int n=0; i<chgfile.GetSize() && index == chgfile[i].index; ++n, ++i)
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

			BOOL match = vvalue==vovalue;
			if (MODE>0 && strstr(value, "?"))
				match = FALSE;
			if (n==0 && match)
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
		CSym sym(RWID+ExtractString(line, "Has pageid", "<value>", "</value>"));
		//ASSERT(!strstr(id, "Northern Ireland"));
		sym.SetStr(ITEM_DESC, id);
		sym.SetStr(ITEM_LAT, ExtractString(line, "lat=", "\"", "\""));
		sym.SetStr(ITEM_LNG, ExtractString(line, "lon=", "\"", "\""));
		sym.SetStr(ITEM_MATCH, ExtractString(ExtractString(line, "Has geolocation", "", "<property"), "<value>", "", "</value>").replace(",",";"));
		regions.Add(sym);
		return TRUE;
}


vars GetFullGeoRegion(const char *georegion)
{
	//CSymList &regions = GeoRegion.Regions();

	vara list;
	vara glist(georegion, ";");
	_GeoRegion.Translate(glist);
	for (int i=0; i<glist.length(); ++i)
		{
		vars reg = GetToken(glist[i], 0, ':');
		if (list.indexOf(reg)<0)
			list.push(reg);
		}
	return list.join(";");
}

int CheckRegion(const char *region, CSymList &regions)
{
	vara majoryes, majorno;
	static CSymList nomajor;
	if (nomajor.GetSize()==0)
		{
		nomajor.Load(filename(GEOREGIONNOMAJOR));
		nomajor.Sort();
		}

	for (int i=0; i<regions.GetSize(); ++i)
		if (stricmp(regions[i].GetStr(0),region)==0)
			{
			// check children
			vars children = regions[i].id;
			vars major = regions[i].GetStr(1);
			if (IsSimilar(major,"T"))
				majoryes.push(children);
			else
				if (nomajor.Find(children)<0)
					majorno.push(children);
			CheckRegion(children, regions);
			}

	if (majoryes.length()>0 && majorno.length()>0)
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
	for (int i=0; i<geolist.GetSize(); ++i)
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
			if (dist>MAXGEOCODEDIST)
				{
				vars err = MkString("Geocode '%s' (%g;%g) = %dkm", geocode, lat, lng, (int)(dist/1000));
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
	for (int i=0; i<regions.GetSize(); )
		{
		printf("%d/%d Region %s                     \r", i, regions.GetSize(), regions[i].id);
		vara regions10;
		for (int j=0; j<10 && i<regions.GetSize(); ++j, ++i)
			regions10.push(regions[i].id);

		// check empty
		vars url = "http://ropewiki.com/index.php?title=Template:RegionCount&action=raw&templates=expand&ctype=text/x-wiki&region="+url_encode(regions10.join(";"));
		if (f.Download(url))
			{
			Log(LOGERR, "Could not download url %s", url);
			continue;
			}

		vara res(f.memory, ";");
		for (int j=0; j<res.length(); ++j)
			if (res[j]=="(0)" && fakelist.Find(regions10[j])<0)
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
		for (int i=0; i<dellist.GetSize(); ++i)
			df.fputstr(dellist[i].id);
		}
	}

	// build distance map
	PLLArrayList lllist;
	for (int i=0; i<rwlist.GetSize(); ++i) 
		{
		CSym &sym = rwlist[i];
		double lat = sym.GetNum(ITEM_LAT);
		double lng = sym.GetNum(ITEM_LNG);		
		if (CheckLL(lat,lng))
			lllist.AddTail( PLL(LL(lat, lng), &sym) );
		}
	PLLRectArrayList llrlist;
	for (int i=0; i<rwlist.GetSize(); ++i) 
		{
		CSym &sym = rwlist[i];
		double lat = sym.GetNum(ITEM_LAT);
		double lng = sym.GetNum(ITEM_LNG);		
		if (lat!=InvalidNUM && lat!=0 && lng!=InvalidNUM && lng!=0)
			llrlist.AddTail( PLLRect(lat, lng, MAXGEOCODEDIST, &sym) ); // 150km
		}
	LLMatch<PLLRect,PLL> mlist(llrlist, lllist, addmatch);
	for (int i=0; i<llrlist.GetSize(); ++i)
		{
		PLLRect *prect = &llrlist[i];
		CSym &sym = *prect->ptr;		
		vars title = sym.GetStr(ITEM_DESC);
		printf("Processing %s %d/%d        \r", title, i, llrlist.GetSize());

		// check GeoRegion
		vara georegions;
		if (_GeoRegion.GetRegion(sym, georegions)>=0)
			continue;

		prect->closest.Sort(prect->closest[0].cmp);

		// check 5 closest regions
		int matchedregion = 0;
		CArrayList <PLLD> &closest = prect->closest;
		vars nearregion;
		vars region = sym.GetStr(ITEM_REGION), mregion = GetToken(region, 0, ';');
		if (!region.IsEmpty())
		  for (int j=1; j<MAXGEOCODENEAR && j<closest.GetSize() && !matchedregion; ++j) 
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
			ASSERT(aregions!=anearregions);
			BOOL refined = anearregion.length() > aregion.length() && strstri(anearregions, aregions);

			CSym chgsym(sym.id, title);
			chgsym.SetStr(ITEM_MATCH, refined ? "REFINED REG " : "WRONG REG "+aregions);
			chgsym.SetStr(ITEM_REGION, anearregions);
			//sym.SetStr(ITEM_REGION, anearregions);
			chglist.Add(chgsym);
			Log(LOGWARN, "WARNING: no matching nearby region? '%s' nearby:%s '%s' -> %s", aregions, sym.id, title, anearregions);
			}
		}

	// check invalid regions
	for (int i=0; i<rwlist.GetSize(); ++i)
		{
		CSym &sym = rwlist[i];
		vars title = sym.GetStr(ITEM_DESC);
		vars region = _GeoRegion.GetRegion(sym.GetStr(ITEM_REGION));

		if (region.IsEmpty() || regions.Find(region)<0)
			{
			vars url = "http://ropewiki.com/api.php?action=query&prop=info&titles="+url_encode(title);
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
			vars georegion = r<0 ? "?" : _GeoRegion.GetFullRegion(georegions[r]);

			// closest match
			int llr = -1;
			vars nearregion;
			for (int r=0; r<llrlist.GetSize() && llr<0; ++r)
				if (llrlist[r].ptr==&sym)
					{
					CArrayList <PLLD> &closest = llrlist[r].closest;
					for (int c=1; c<closest.length() && nearregion.IsEmpty(); ++c)
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




int sortgeomatch( const void *arg1, const void *arg2 )
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
		for (int i=0; i<idlist.GetSize(); ++i)
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
			if (maxr<0)
				{
				++error;
				continue;
				}

			// shorten georegions on demand

			vars georegion;
			vara georegions2;
			if (MODE>0) maxr = MODE; // MAX auto level
			for (int i=0; i<=maxr && i<georegions.length(); ++i)
				georegions2.push(georegion=georegions[i]);

			// shorten (change) the region too if max is forced
			if (maxlev>=0)
				{
				int curlev = (int)CDATA::GetNum(GetToken(georegion,1,':'));
				if (curlev==maxlev)
					{
					//int ni = i + maxlev - curlev;
					if (!_GeoRegion.GetSubRegion(tomatch, GetToken(georegion,0,':'), TRUE).IsEmpty()) // && ni<afullregion.length()) 
						continue;
					else
						tomatch = tomatch;
					
					}
				}

			// process match

			int g = geomatch.Find(georegion);
			if (g<0)
				{
				CSym sym(georegion, georegions2.join(";"));
				sym.SetStr(ITEM_NEWMATCH, "X");
				geomatch.Add(sym); 
				g = geomatch.Find(georegion);
				}


			geomatch[g].data += ";"+tomatch+"="+sym.GetStr(ITEM_DESC);
			}


		// collapse results
		vara subst;
		int diff = 0, rdiff = 0;
		geomatch.Sort();
		for (int i=0; i<geomatch.GetSize(); ++i)
			{
			CSym &sym = geomatch[i];
			vara list(sym.GetStr(ITEM_NEWMATCH), ";");
			list.RemoveAt(0);
			list.sort();
			

			vars georegion = GetToken(sym.id,0,':');
			vars region = GetToken(list[list.length()/2], 0, '=');
			vars fullgeoregion = GetFullGeoRegion(sym.GetStr(ITEM_DESC));
			vars fullregion = _GeoRegion.GetFullRegion(region);

			sym.SetStr(ITEM_LAT, fullregion);
			if (georegion!=region)
				{
				// inconsistency detected
				sym.SetStr(ITEM_LNG, georegion );
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
				if (start>=0)
					{
					for (int i = 0, j = start; i<ageoregion.length() && j<aregion.length(); ++i, ++j)
					  if (ageoregion[i]!=aregion[j])
						{
							const char *a = ageoregion[i];
							const char *b = aregion[j];
							if (strstri(ageoregion[i],"|"))
								continue; // ignore
							if (i+1<ageoregion.length() && ageoregion[i+1]==aregion[j])
								{
								++i;
								continue;
								}
							if (j+1<aregion.length() && ageoregion[i]==aregion[j+1])
								{
								++j;
								continue;
								}

						idiff = i;
						++icount;
						}
					if (icount==1)
						{
						vars num, desc = sym.GetStr(ITEM_DESC);
						const char *str = strstri(desc,ageoregion[idiff]);
						if (str)
							num = ":"+GetToken(str,1,":;");
						else
							Log(LOGERR, "Inconsistent georegion %s not in %s", desc, ageoregion[idiff]);
						vars chg = ageoregion[idiff]+num+","+aregion[start+idiff]+num;
						if (subst.indexOf(chg)<0)
							subst.push(chg);
						}

					}
				if (icount>0 || start<0)
					{
					sym.SetStr(ITEM_MATCH, fullgeoregion);
					rdiff++;
					}
				}

			vara elist, dlist;
			for (int l=0; l<list.length(); ++l)
				if (GetToken(list[l], 0, '=')==region)
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
		if (subst.length()>0)
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

		return diff>0 || rdiff>0;
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
	if (fileregion!=NULL && *fileregion==0)	
		fileregion=NULL;

	LoadRWList();

	// select syms belonging to region or ALL if not specified
	CSymList idlist;
	BOOL all = TRUE;
	if (fileregion!=NULL && *fileregion!=0)	
		{
		all = FALSE;
		CSymList sellist;
		GetRWList(sellist, fileregion);
		for (int i=0; i<sellist.GetSize(); ++i)
			{
			int f = rwlist.Find(RWID+vars(sellist[i].id).replace(RWID,""));
			if (f<0) 
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
		for (int i=0; i<idlist.GetSize(); ++i)
			if (idlist[i].GetStr(ITEM_REGION)=="Mars")
				idlist.Delete(i--);
		}

	// generic check and fix ALL regions
	if (all && MODE<0)
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
	if (MODE<0)
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
size_t FindStringInBuffer (char* buffer, char* search, size_t buffersize)
{
	char* buffer0 = buffer;

	size_t len = strlen(search);
	bool fnd = false;
	while (!fnd)
	{
		fnd = true;
		for (size_t i=0; i<len; i++)
		{
			if (buffer[i]!=search[i])
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
	while (i>0 && search[i]==' ') i--;
	while (i>0 && (isdigit(search[i]) || search[i]=='.')) i--;
	float flt=-1.0;
	char buffer[oldchar+5]; ZeroMemory(buffer,sizeof(buffer));
	strncpy(buffer, search+i+1, lastcharoffset-i);
	if (buffer[0] && sscanf(buffer, "%f", &flt))
	{
		return flt;
	}
	return -1.0;
}

//Check if a certain 2 character token just came along (e.g. BT):
bool seen2(const char* search, char* recent)
{
if (    recent[oldchar-3]==search[0] 
	 && recent[oldchar-2]==search[1] 
	 && (recent[oldchar-1]==' ' || recent[oldchar-1]==0x0d || recent[oldchar-1]==0x0a) 
	 && (recent[oldchar-4]==' ' || recent[oldchar-4]==0x0d || recent[oldchar-4]==0x0a)
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
	int j=0;
	for (j=0; j<oldchar; j++) oc[j]=' ';

	for (size_t i=0; i<len; i++)
	{
		char c = output[i];
		if (intextobject)
		{
			if (rbdepth==0 && seen2("TD", oc))
			{
				//Positioning.
				//See if a new line has to start or just a tab:
				float num = ExtractNumber(oc,oldchar-5);
				if (num>1.0)
				{
					fputc(0x0d, file);
					fputc(0x0a, file);
				}
				if (num<1.0)
				{
					fputc('\t', file);
				}
			}
			if (rbdepth==0 && seen2("ET", oc))
			{
				//End of a text object, also go to a new line.
				intextobject = false;
				fputc(0x0d, file);
				fputc(0x0a, file);
			}
			else if (c=='(' && rbdepth==0 && !nextliteral) 
			{
				//Start outputting text!
				rbdepth=1;
				//See if a space or tab (>1000) is called for by looking
				//at the number in front of (
				int num = ExtractNumber(oc,oldchar-1);
				if (num>0)
				{
					if (num>1000.0)
					{
						fputc('\t', file);
					}
					else if (num>100.0)
					{
						fputc(' ', file);
					}
				}
			}
			else if (c==')' && rbdepth==1 && !nextliteral) 
			{
				//Stop outputting text
				rbdepth=0;
			}
			else if (rbdepth==1) 
			{
				//Just a normal text character:
				if (c=='\\' && !nextliteral)
				{
					//Only print out next character no matter what. Do not interpret.
					nextliteral = true;
				}
				else
				{
					nextliteral = false;
					if ( ((c>=' ') && (c<='~')) || ((c>=128) && (c<255)) )
					{
						fputc(c, file);
					}
				}
			}
		}
		//Store the recent characters for when we have to go back for a number:
		for (j=0; j<oldchar-1; j++) oc[j]=oc[j+1];
		oc[oldchar-1]=c;
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
		int fseekres = fseek(filei,0, SEEK_END);   //fseek==0 if ok
		long filelen = ftell(filei);
		fseekres = fseek(filei,0, SEEK_SET);

		//Read ethe ntire file into memory (!):
		char* buffer = new char [filelen]; ZeroMemory(buffer, filelen);
		size_t actualread = fread(buffer, filelen, 1 ,filei);  //must return 1

		bool morestreams = true;

		//Now search the buffer repeated for streams of data:
		while (morestreams)
		{
			//Search for stream, endstream. We ought to first check the filter
			//of the object to make sure it if FlateDecode, but skip that for now!
			size_t streamstart = FindStringInBuffer (buffer, "stream", filelen);
			size_t streamend   = FindStringInBuffer (buffer, "endstream", filelen);
			if (streamstart>0 && streamend>streamstart)
			{
				//Skip to beginning and end of the data stream:
				streamstart += 6;

				if (buffer[streamstart]==0x0d && buffer[streamstart+1]==0x0a) streamstart+=2;
				else if (buffer[streamstart]==0x0a) streamstart++;

				if (buffer[streamend-2]==0x0d && buffer[streamend-1]==0x0a) streamend-=2;
				else if (buffer[streamend-1]==0x0a) streamend--;

				//Assume output will fit into 10 times input buffer:
				size_t outsize = (streamend - streamstart)*10;
				char* output = new char [outsize]; ZeroMemory(output, outsize);

				//Now use zlib to inflate:
				z_stream zstrm; ZeroMemory(&zstrm, sizeof(zstrm));

				zstrm.avail_in = streamend - streamstart + 1;
				zstrm.avail_out = outsize;
				zstrm.next_in = (Bytef*)(buffer + streamstart);
				zstrm.next_out = (Bytef*)output;

				int rsti = inflateInit(&zstrm);
				if (rsti == Z_OK)
				{
					int rst2 = inflate (&zstrm, Z_FINISH);
					if (rst2 >= 0)
					{
						//Ok, got something, extract the text:
						size_t totout = zstrm.total_out;
						ProcessOutput(fileo, output, totout);
					}
				}
				delete[] output; output=0;
				buffer+= streamend + 7;
				filelen = filelen - (streamend+7);
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
	if (ft>=0) 
		ret = ret.Mid(0, ft);

	while (ret.Replace("  ", " "));
	return ret.Trim();
}

void AddTrack(CSymList &symlist, const char *_title, const char *_artist, double date, const char *_group)
{		
		if (!_title || !*_title)
			return;

		vars file = simplifyTitle(vars(_title), FALSE) + " - " + simplifyTitle(vars(_artist), FALSE);
		vars link = "http://musicpleer.co/#!"+simplifyTitle(file);
		CSym sym(link.replace(" ", "+"), file);			
		sym.SetStr(ITEM_LAT, simplifyTitle(_title));
		sym.SetStr(ITEM_LNG, simplifyTitle(_artist));
		sym.SetDate(ITEM_NEWMATCH, date);
		sym.SetStr(ITEM_MATCH, _group);
		sym.SetStr(ITEM_ACA,"Hyperlink(\""+link+"\")");
		//ASSERT(strstr(sym.GetStr(ITEM_DESC), "-"));
		//ASSERT(!strstr(sym.GetStr(ITEM_DESC), "HOW DEEP"));
		int found = symlist.Find(sym.id);
		if (found<0)
			symlist.Add(sym);
		else
			{
			// update
			CSym &osym = symlist[found];
			if (osym.GetDate(ITEM_NEWMATCH)<date)
				osym.SetDate(ITEM_NEWMATCH, date);
			if (osym.GetStr(ITEM_LAT).IsEmpty())
				osym.SetStr(ITEM_LAT, sym.GetStr(ITEM_LAT));
			if (osym.GetStr(ITEM_LNG).IsEmpty())
				osym.SetStr(ITEM_LNG, sym.GetStr(ITEM_LNG));
			vara g(osym.GetStr(ITEM_MATCH), ";");
			if (g.indexOf(_group)<0)
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
	
	for (int t=0; t<table.length(); ++t)
	{
	if (t>0)
		{
		url = burl(ubase, ExtractString(table[t], "", "\"", "\""));
		if (f.Download(url))
			{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			return FALSE;
			}
		}

	vara urls(f.memory, "class=\"chartintlist2\"");
	for (int u=1; u<urls.length(); ++u)
		{
		vars url = ExtractString(urls[u], "href=", "\"", "\"");
		vars date = ExtractString(urls[u], "href=", ">", "</a");
		double vdate = CDATA::GetDate(date, "MMM D, YYYY" );
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
		for (int i=1; i<list.length(); ++i)
			{
			vars artist = stripHTML(ExtractString(list[i], ">", "", "<br"));
			vars title = stripHTML(ExtractString(list[i], "<br", ">", "</td"));
			vars link = "http://musicpleer.co/#!"+artist+" "+title;
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
	if (year!=NULL)
		{
		int y = atoi(year);
		if (y<2000) 
			{
			Log(LOGERR, "Bad year specified %s", year);
			return FALSE;
			}
		pyear = MkString("%d", y-1);
		}

	while (TRUE)
		{
		if (f.Download(url))
			{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			return FALSE;
			}

		vars date = ExtractString(f.memory, "datetime=", "\"", "\"");
		double vdate = CDATA::GetDate(date, "YYYY-MM-DD" );
		if (strstr(date, pyear))
			break;

		vara list(f.memory, "class=\"chart-row__title\"");
		printf("%d : %s %d %s [%s]     \r", symlist.GetSize(), group, list.length(), CDate(vdate), date);
		for (int i=1; i<list.length(); ++i)
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
	if (year!=NULL)
		{
		int y = atoi(year);
		if (y<2000) 
			{
			Log(LOGERR, "Bad year specified %s", year);
			return FALSE;
			}
		pyear = MkString("%d", y-1);
		}

	while (TRUE)
		{
		if (f.Download(url))
			{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			return FALSE;
			}

		vars date = stripHTML(ExtractString(f.memory, "class=\"article-date\"", ">", "<"));
		double vdate = CDATA::GetDate(date, "D MMM YYYY" );
		if (strstr(date, pyear))
			break;

		vara list(f.memory, "class=\"track\"");
		printf("%d : %s %d %s [%s]     \r", symlist.GetSize(), group, list.length(), CDate(vdate), date);
		for (int i=1; i<list.length(); ++i)
			{
			vars artist = stripHTML(ExtractString(list[i], "class=\"artist\"", ">", "</div"));
			vars title = stripHTML(ExtractString(list[i], "class=\"title\"", ">", "</div"));
			AddTrack(symlist, title, artist, vdate, group);
			}

		if (!year) 
			break;			

		// next url
		vara urls(f.memory, "href=");
		for (int i=1; i<urls.length(); ++i)
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
		for (int i=1; i<list.length(); ++i)
			{
			vars artist = stripHTML(ExtractString(list[i], "by <a ", ">", "</a"));
			vars title = stripHTML(ExtractString(list[i], "", ">", "</h4"));
			vars ago = stripHTML(ExtractString(list[i], ">released", ">", "</"));
			double nago = atoi(ago); 
			if (strstr(ago, "year"))
				nago *= 12*30;
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
			double vdate = Date( COleDateTime::GetCurrentTime() - COleDateTimeSpan( (int)nago, 0, 0, 0 ) );
			printf("%d : %s %d %s     \r", symlist.GetSize(), group, list.length(), CDate(vdate));
			AddTrack(symlist, title, artist, vdate, group);
			}

		if (!year) 
			break;			

		// next url
		++page;
		url = burl(ubase, MkString("history/year/%s/%d", year, page));
		if (page>5)
			break;
		}

	return TRUE;
	}



int DownloadTop40(const char *mp3file, const char *mp3title, const char *mp3artist)
{
		DownloadFile f;
		vars txt = vars(mp3title) + " " + vars(mp3artist);
		vars search = "http://databrainz.com/api/search_api.cgi?qry="+txt.replace(" ","+")+"&format=json&mh=50&where=mpl";
		if (f.Download(search))
			{
			Log(LOGERR, "ERROR: can't download url %.128s", search);
			return FALSE;
			}

		vara urls(f.memory, "\"url\":");
		for (int u=1; u<urls.length() && !CFILE::exist(mp3file); ++u)
			{
			vars id = ExtractString(urls[u], "", "\"", "\"");
			vars url = "http://databrainz.com/api/data_api_new.cgi?id="+id+"&r=mpl&format=json";
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
				cf.fwrite(ff.memory,1,ff.size);
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
	if (found>=0 && strcmp(filelist[found].data, mp3file)!=0)
		{
		MoveFile(filelist[found].data, mp3file);
		return TRUE;
		}
	return found>=0;
}


int GetTop40(const char *year, const char *pattern)
{
	vars folder = "D:\\Music\\00+Top\\";
	if (year!=NULL && *year!=0 && CDATA::GetNum(year)<=0)
		folder = year;

	vars file = folder+"top40.csv";

	CSymList symlist;
	symlist.Load(file);

	if (MODE==-1 || MODE==0)
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

	if (MODE==-1 || MODE==1)
	{

	CSymList filelist;
	const char *exts[] = { "MP3", NULL };
	GetFileList(filelist, folder, exts, TRUE);
	for (int j=0; j<filelist.GetSize(); ++j)
		{
		vars filename = filelist[j].id.MakeUpper();
		vars id = GetFileName(filename);
		vars data = folder+id+".mp3";
		filelist[j] = CSym(id, data);
		if (filename!=data)
			MoveFile(filename, data);
		}
	filelist.Sort();

	DownloadFile f;
	// download mp3
	
	for (int p=0; p<2; ++p)
	  for (int i=0; i<symlist.GetSize(); ++i)
		{
		CSym &sym = symlist[i];
		vars mp3desc = sym.GetStr(ITEM_DESC);
		double mp3date = sym.GetDate(ITEM_NEWMATCH);
		vara mp3sdate(sym.GetStr(ITEM_NEWMATCH), "-");
		if (mp3sdate.length()<3)
			{
			Log(LOGERR, "Invalid date '%s' for %s'", sym.GetStr(ITEM_NEWMATCH), sym.id);
			continue;
			}
		double index = 12*2050 - 12*atoi(mp3sdate[0]) - atoi(mp3sdate[1]);
		vars match = sym.GetStr(ITEM_MATCH);
		vars group = strstr(match, "Dance") ? "Dance40" : (strstr(match, "40") ? "Top40" : "TopMore");
		vars mp3folder = folder+group+"\\";
		CreateDirectory(mp3folder, NULL);
		mp3folder += MkString("%03d %s %s", (int)index, CDATA::GetMonth(atoi(mp3sdate[1])), mp3sdate[0])+"\\";
		CreateDirectory(mp3folder, NULL);
		vars mp3file = mp3folder+mp3desc+".mp3";
		vars mp3title = sym.GetStr(ITEM_LAT);
		vars mp3artist = sym.GetStr(ITEM_LNG);

		if (!ExistTop40(filelist, mp3file) && p>0)
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
	vars folder = vars(KMLFOLDER)+"\\";

	// load from file
	if (filecsv)
		{
		CSymList list;
		list.Load(filecsv);
		for (int i=0; i<list.GetSize(); ++i)
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
			vars id = RWID+GetToken(rwid, 1, ": ");
			int found = rwlist.Find(id);
			if (found<0) 
				{
				Log(LOGERR, "Invalid RWID '%s'", match);
				continue;
				}

			// download kmlfile
			vars lockmlfile = folder+rwlist[found].GetStr(ITEM_DESC)+".kml";
			if (IsSimilar(kmlfile, "http"))
				{
				if (f.Download(kmlfile, lockmlfile))
					Log(LOGERR, "Could not download kml from %s to %s", kmlfile, lockmlfile);
				continue;
				}

			// download msid
			printf("Downloading %s         \r", lockmlfile);
			inetfile out(lockmlfile);
			KMZ_ExtractKML("", "http://maps.google.com/maps/ms?ie=UTF8&hl=en&msa=0&output=kml&msid="+kmlfile, &out);
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

	for (int i=0; i<newlist.GetSize(); ++i)
		if (filelist.Find(newlist[i].id)<0)
			filelist.Add(newlist[i]);
	filelist.Save(file);

	if (MODE<-1)
		newlist = filelist;

	vara specials("DEM,Null,Scott Swaney's Death Valley Canyons,Fire closure area,Mount Wilson mountain biking");

	if (MODE<0)
		{
		for (int i=0; i<newlist.GetSize(); ++i)
			{
			CString name = newlist[i].id.Mid(5, newlist[i].id.GetLength()-5-4);
			if (specials.indexOf(name)>=0)
				continue;
			int found = rwnamelist.Find(name);
			if (found<0)
				{
				Log(LOGERR, "Orphaned KML file '%s'", ROPEWIKIFILE+name+".kml");
				continue;
				}
			int found2 = rwlist.Find( rwnamelist[found].data );
			if (found2<0)
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
			vars lockmlfile = folder+GetFileNameExt(kmlfile.replace("/", "\\"));			
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

	if (IsSimilar(transfile,"0x"))
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
	GetFileList(files, KMLFOLDER , exts, FALSE);



	// list of kmls
	for (int n=0; n<files.GetSize(); ++n)
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
		char *mem = (char *)malloc(len+1);
		f.fread(mem, len, 1);
		mem[len]=0;
		memory = mem;
		free(mem);
		f.fclose();

		// process
		vars omemory = memory;
		vars memory2 = memory.replace("\"", "'");
		LL dse[2], ase[2], ese[2];
		
		enum {D_A, D_D, D_E, D_MAX };
		const char *names[] = { "Approach" , "Descent" , "Exit", NULL };

		LL se[3][2];
		int done[D_MAX];
		double length[D_MAX], depth[D_MAX];
		for (int d=0; d<D_MAX; ++d)
			{
			length[d] = depth[d] = InvalidNUM;
			done[d] = 0;
			}
		
		if (TMODE==1) // GBR -> GRY
		{
			vars sep = "<LineStyle>", sep2 = "</LineStyle>";
			vara placemarks(memory, sep);
			for (int p=1; p<placemarks.length(); ++p)
			{
			vara pl(placemarks[p], sep2);
			double w = ExtractNum(pl[0], "<width>", "", "</width>");
			vars color = ExtractString(pl[0], "<color>", "", "</color>");
			int t;
			for (t=0; t<ctrans.GetSize(); ++t)
			  {
			  if (ctrans[t].id[0]!=';')
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
			if (t>=ctrans.GetSize())
				Log(LOGERR, "No color translation: '%s' from %s", color, file);
			pl[0] = MkString("<color>%s</color><width>%g</width>", color, (w>5 || w<=0) ? 5 : w);
			placemarks[p] = pl.join(sep2);
			}
			memory = placemarks.join(sep);
		}

		vars sep = "<Placemark>", sep2 = "</Placemark>";
		vara placemarks(memory, sep);
		for (int p=1; p<placemarks.length(); ++p)
		{
		vara pl(placemarks[p], sep2);

		if (transfile)
		{
		// translate name
		vars sep = "<name>", sep2 = "</name>";
		vara names(pl[0], sep);

		 for (int i=1; i<names.length(); ++i)
			{
			int tr = 0;
			vara line(names[i], sep2);
			if (names[0].IsEmpty())
				continue;

			int t;
			for (t=0; t<trans.GetSize(); ++t)
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

			if (t>=trans.GetSize())
				Log(LOGWARN, "No name translation: '%s' from %s", line[0], file);

			names[i] = line.join(sep2);
			}
		pl[0] = names.join(sep);
		}

		// eliminate styles for lines
		if (pl[0].Find("<LineString>")>0)
			{
				vars style = ExtractStringDel(pl[0],"<styleUrl>","#","</styleUrl>");
				if (!style.IsEmpty())
					{
					vars styledef, ostyle = style;
					while (styledef.IsEmpty() && !style.IsEmpty())
						{
						styledef = ExtractString(memory2, "<Style id='"+style+"'", ">", "</Style>");
						vars stylemap = ExtractString(memory2, "<StyleMap id='"+style+"'", ">", "</StyleMap>");
						if (!stylemap.IsEmpty())
							style = ExtractString(stylemap,"<styleUrl>","#","</styleUrl>");
						}
					if (styledef.IsEmpty())
						{
						Log(LOGERR, "Could not remap LineString style #%s", ostyle);
						styledef = ostyle;
						}
					else
						styledef = "<Style>"+styledef+"</Style>";
					int linestring = pl[0].Find("<LineString>");
					pl[0].Insert(linestring, styledef.Trim());
					}

			// check colors are appropriate
			vars name = ExtractString(pl[0], "<name>","","</name>");
			// get standard color
			vars color = ExtractString(pl[0], "<color>","","</color>");
			unsigned char abgr[8];
			FromHex(color, abgr);
			for (int c=0; c<4; ++c)
				{
				double FFCLASS = 2;
				double v = round(abgr[c]/255.0*FFCLASS);
				double iv = round(255*v/FFCLASS);
				abgr[c] = iv>0xFF ? 0xFF : (int)iv;
				}
			vars scolor = ToHex(abgr,4);

			int ok = FALSE, fixed = FALSE;
			int standards = ctrans.Find(";STANDARD");
			// check
			for (int i=standards+1; !ok && i<ctrans.GetSize(); ++i)
				{
				if (IsSimilar(scolor.Right(6),ctrans[i].id.Right(6)))
					{
					vara match(ctrans[i].data);
					for (int m=1; !ok && m<match.length(); ++m)
						if (match[m]=="*" || strstr(name, match[m]) || strstr(name, match[m].lower()))
							ok = TRUE;
					}
				}
			// fix
			for (int i=standards+1; !ok && !fixed && i<ctrans.GetSize(); ++i)
				{
				vara match(ctrans[i].data);
				for (int m=1; !ok && !fixed && m<match.length(); ++m)
					if (strstr(name, match[m]) || strstr(name, match[m].lower()))
						fixed = pl[0].Replace("<color>"+color+"</color>", "<color>"+ctrans[i].id+"</color>");
				}

			if (!ok)
				Log(fixed ? LOGWARN : LOGERR, "Inconsistent color/name %s %s (%s) %s in %s", fixed ? "FIXED!" : "?", scolor, color, name, file);

			// @@@check proper direction approach/exit
			// @@@compute elevation
			// @@@compute distance
			// @@@compute and fix coordinates
			for (int d=0; d<3; ++d)
				{
				if (IsSimilar(name, names[d]) && done[d]>=0)
					{
					++done[d];
					if (name==names[d])
						done[d] = -1;
					//ASSERT(!strstr(file, "Ringpin"));
					if (!ComputeCoordinates(pl[0], length[d], depth[d], se[d]))
						Log(LOGERR, "Could not comput %s coords for %s", names[d], file);
					}
				}
			}


		pl[0].Replace("<description></description>","");
		placemarks[p] = pl.join(sep2);
		}
		memory = placemarks.join(sep);

		if (omemory != memory)
			{
			// WRITE
			vars name = GetFileNameExt(file);
			file = vars(KMLFIXEDFOLDER)+"\\"+name;
			if (!f.fopen(file,CFILE::MWRITE))
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
		name.Replace("_"," ");
		int found = rwnamelist.Find(name);
		if (found<0)
			{
			Log(LOGERR, "Orphaned KML file '%s'", ROPEWIKIFILE+name+".kml");
			continue;
			}
		int found2 = rwlist.Find( rwnamelist[found].data );
		if (found2<0)
			{
			Log(LOGERR, "Orphaned2 KML file '%s'", name);
			continue;
			}
		CSym &osym = rwlist[found2];
		CSym sym(osym.id, osym.GetStr(0));
		for (int d=0; d<D_MAX; ++d)
			if (done[d]>1)
				length[d] = depth[d] = InvalidNUM;
		if (length[D_D]!=InvalidNUM && osym.GetNum(ITEM_LENGTH)==InvalidNUM)
			sym.SetStr(ITEM_LENGTH, MkString("%sm", CData(round(length[D_D]))));
		if (depth[D_D]!=InvalidNUM && osym.GetNum(ITEM_DEPTH)==InvalidNUM)
			sym.SetStr(ITEM_DEPTH, MkString("%sm", CData(round(depth[D_D]))));
		if (osym.GetNum(ITEM_HIKE)==InvalidNUM)
			if (length[D_D]!=InvalidNUM && length[D_A]!=InvalidNUM && length[D_E]!=InvalidNUM)
				sym.SetStr(ITEM_HIKE, MkString("%sm", CData(round(length[D_D]+length[D_A]+length[D_E]))));			
		if (strlen(GetTokenRest(sym.data,2))>0)
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
	GetFileList(files, KMLFIXEDFOLDER , exts, FALSE);

	//loginstr = vara("Barranquismo.net,sentomillan");
	DownloadFile f;
	if (!Login(f))
		return FALSE;

	for (int i=0; i<files.GetSize(); ++i)
	{
		CPage up(f, NULL, NULL, NULL);
		vars rfile = GetFileNameExt(files[i].id);
		if (up.FileExists(rfile))
			if (MODE>-2)
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



int cmpprice( const void *arg1, const void *arg2 )
{
	const char *s1 = strstr(((CSym**)arg1)[0]->data, "$");
	const char *s2 = strstr(((CSym**)arg2)[0]->data, "$");
	double p1 = s1 ? CGetNum(s1) : 0;
	double p2 = s2 ? CGetNum(s2) : 0;
	if (p1>p2) return 1;
	if (p1<p2) return -1;
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
	vara list( f.memory, "{");
	for (int i=1; i<list.length(); ++i)
		{
		vars &item = list[i];
		vars name = ExtractString(item, "\"name\"", "\"", "\"");
		vars host = ExtractString(item, "\"hostname\"", "\"", "\"");
		double lat = ExtractNum(item, "\"lat\"", "\"", "\"");
		double lng = ExtractNum(item, "\"lon\"", "\"", "\"");
		if (host.IsEmpty() || !CheckLL(lat,lng))
			{
			Log(LOGERR, "Invalid hostname/coords for %s", list[i]);
			continue;
			}

		CSym sym(host, name);
		sym.SetNum(ITEM_LAT, lat);
		sym.SetNum(ITEM_LNG, lng);
		if (creglist.Find(host)<0)
			creglist.Add(sym);
		}


	// get list
	vars start, end;
	CSymList idlist;
	for (int k=0; k<keylist.GetSize(); ++k)
	{
	CSymList ridlist, reglist;

	// match regions to geoloc < dist
	vara geolist( keylist[k].data );
	for (int g=1; g<geolist.length(); ++g)
		{
		double lat, lng, dist;
		vara geo(geolist[g], "<");
		if (geo.length()<2 || (dist=CDATA::GetNum(geo[1]))<=0 || !_GeoCache.Get(geo[0], lat, lng))
			{
			Log(LOGERR, "Invalid geoset 'loc < dist' %s", geolist[g]);
			continue;
			}

		LLRect rect(lat, lng, dist/km2mi*1000);		
		for (int r=0; r<creglist.GetSize(); ++r)
			{
			CSym &sym = creglist[r];
			if (rect.Contains(sym.GetNum(ITEM_LAT), sym.GetNum(ITEM_LNG)))
				if (reglist.Find(sym.id)<0)
					reglist.Add(sym);
			}
		}

	// search regions
	for (int r=0; r<reglist.GetSize(); ++r)
		{
		static double ticks = 0;
		printf("%d %d%% %s %d/%d %s %d/%d       \r", idlist.GetSize()+ridlist.GetSize(), ((k*reglist.GetSize()+r)*100)/(reglist.GetSize()*keylist.GetSize()), keylist[k].id, k, keylist.GetSize(), reglist[r].id, r, reglist.GetSize());

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
		if (list.length()<=1)
			continue;

		int mid = 0;
		start = list[0];
		for (int i=1; i<list.length(); ++i)
			{
			mid = list[i].Find(END);
			if (mid<0) 
				{
				Log(LOGERR, "ERROR: malformed item %.128s", list[i]);
				continue;
				}				
			mid += END.length();
			vars item = START + list[i].Mid(0, mid);
			vars id = ExtractString(item, "data-pid=", "\"", "\"");
			if (idlist.Find(id)>=0 || ridlist.Find(id)>=0)
				continue;

			vars pic;
			vara data(ExtractString(item, "data-ids=", "\"", "\""), ",");
			if (data.length()>0)
				{
				pic = GetToken(data[0], 1, ':');

				vars a = "</a>";
				int found = item.Find(a);
				if (found>0)
					item.Insert(found, MkString("<img src=\"https://images.craigslist.org/%s_300x300.jpg\"</img>",pic));				
				}

			// avoid repeated ads with same text and same pic
			vara locs( stripHTML(ExtractString(item, "<small>", "", "</small>")).Trim("() "), ">");
			vars loc = locs.last().trim();
			if (loc.IsEmpty())
				loc = defloc;
			vars text = pic+":"+loc+":"+stripHTML(ExtractString(item, "data-id=", ">", "<"));
			if (keylist[k].GetNum(0)<0)
				if (MODE<=0 && donelist.Find(text)>=0)
					continue;

			vars link = ExtractString(item, "href=", "\"", "\"");
			donelist.Add(CSym(text,link));

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
		for (int i=0; i<idlist.GetSize(); ++i)
			cf.fwrite(idlist[i].data, idlist[i].data.GetLength(), 1);
		cf.fwrite(end, end.GetLength(), 1);
		cf.fclose();
		}

	if (!htmfile)
		system(outfile);

	return TRUE;
	}














#ifdef DEBUG
#define FBLog if(TRUE) Log
#else
#define FBLog if(FALSE) Log
#endif


#define FBDEBUG "    DEBUG: "
#define FACEBOOKTICKS 1100
static double facebookticks;

int fburlcheck(const char *link)
{
	if (!link || *link==0)
		return FALSE;
	if (strstr(link, "/profile.php"))
		return FALSE;
	if (strstr(link, "/story.php"))
		return FALSE;
	if (!IsSimilar(link, "http"))
		return FALSE;
	return TRUE;
}

vars fburlstr(const char *link, int noparam = FALSE)
{

	vars url = htmltrans(link).Trim();
	if (url.IsEmpty()) 
		return "";

	if (noparam)
		{
		if (strstr(url,"/profile.php?"))
			{
			vars id = ExtractString(url+"&", "id=", "", "&");
			if (id.IsEmpty())
				{
				Log(LOGERR, "ERROR: Cannot convert profile.php link %s", url);
				return "";
				}
			url = "/"+id;
			}
		if (strstr(url, "/story.php"))
			{
			//https://m.facebook.com/story.php?story_fbid=10208789220476568&id=1494703586&_rdr
			//https://www.facebook.com/1494703586/posts/10208789220476568
			vars url2 = url+"&";
			vars story = ExtractString(url2, "?story_fbid=", "", "&");
			vars user = ExtractString(url2, "&id=", "", "&");
			if (story.IsEmpty() || user.IsEmpty())
				{
				Log(LOGERR, "ERROR: Cannot convert story_fbid link %s", url);
				return "";
				}
			url = "/"+user+"/posts/"+story;
			}
		if (noparam<0)
			url = vars(url).split("&refid=").first().split("?refid=").first();
		else
			url = GetToken(url,0,'?');
		}

	url = burl("m.facebook.com", url);
	url.Replace("//www.facebook.com","//m.facebook.com");
	url.Replace("//facebook.com","//m.facebook.com");
	url.Replace("http:", "https:");
	return url.Trim(" /");
}


vars fburlstr2(const char *link, int noparam = FALSE)
{
	vars url = fburlstr(link, noparam);
	return url.replace("m.facebook.com", "facebook.com");
}

int FACEBOOK_Login(DownloadFile &f, const char *name, const char *user, const char *pass)
{
	Throttle(facebookticks, FACEBOOKTICKS);
	vars url = "https://m.facebook.com";
	if (f.Download(url))
		{
		Log(LOGALERT, "Could not logout www.facebook.com");
		}
	vars username = ExtractString(f.memory, ">Logout", "(", ")");
	vars logout = fburlstr(ExtractLink(f.memory, ">Logout"));
	if (!logout.IsEmpty())
		{
		if (username==name)
			return TRUE; // already logged in
		Throttle(facebookticks, FACEBOOKTICKS);
		if (f.Download(logout))
			{
			Log(LOGALERT, "Could not logout www.facebook.com");
			}
		}
	
	Throttle(facebookticks, FACEBOOKTICKS);
	if (f.Download(url))
		{
		Log(LOGALERT, "Could not download www.facebook.com");
		return FALSE;
		}

	vars pickaccount = fburlstr(ExtractLink(f.memory, "another account"));
	if (!pickaccount.IsEmpty())
		{
		Throttle(facebookticks, FACEBOOKTICKS);
		if (f.Download(pickaccount))
			{
			Log(LOGALERT, "Could not logout www.facebook.com");
			}
		}

	if (!f.GetForm("login_form"))
		{
		vars user = ExtractString(f.memory, "\"fb_welcome_box\"", ">", "<");
		Log(LOGALERT, "Cant find facebook login form (%s)", user);
		return FALSE;
		}

	f.SetFormValue("email", user);
	f.SetFormValue("pass", pass);	
	Throttle(facebookticks, FACEBOOKTICKS);
	if (f.SubmitForm())
		{
		Log(LOGALERT, "Could not login on facebook");
		return FALSE;
		}
	return TRUE;
}



const char *skipblanks(const char *str)
{
	while (*str==' ') ++str;
	return str;
}


static int cmplength( const void *arg1, const void *arg2 )
{
	return ((CSym**)arg2)[0]->id.GetLength()- ((CSym**)arg1)[0]->id.GetLength();
//	return CSymID::Compare(((CSym**)arg1)[0]->id, ((CSym**)arg2)[0]->id);
}



void PrepMatchList(CSymList &list)
{
	// prepare to use IsMatchList
	for (int i=0; i<list.GetSize(); ++i)
		list[i].id = stripAccents2(list[i].id).Trim();
	list.Sort(cmplength);
}


int IsMatchList(const char *str, CSymList &list, BOOL noalpha = TRUE, int *matchi = NULL, int minmatch = -1)
// noalpha = TRUE common string must terminate in a non alpha char, FALSE can end in any char
// minmatch = minimum chars to match, if more are present they have to match too
{
	str =skipnotalphanum(str);
	for (int i=0; i<list.GetSize(); ++i)
		{
		const char *strid = list[i].id;
		ASSERT(*strid!=' ');
		int l;
		for (l=0; str[l]!=0 && tolower(str[l])==tolower(strid[l]); ++l);
		int  match = strid[l]==0;
		if (minmatch>0 && l>=minmatch)
			match = TRUE;
		if (match)
			if (!noalpha || !isa(str[l]))
				{
				if (matchi)
					*matchi = i;
				return l;
				}
		}
	return FALSE;
}




static double CheckDate(double d, double m, double y)
{
	if (y<100) y+=2000;
	if (d<0 || d>31 || m<0 || m>12 || y<0 || y<=2012)
		return -1;
	return Date( (int)d, (int)m, (int)y); 
}

typedef struct {
	int start;
	int end;
	double val;
} tnums;



class FACEBOOKDATE
{

int FACEBOOK_CheckDate(double d, double m, double y, double &mind, double &mindate, double pdate)
{
	double date = CheckDate( d, m, y); 
	if (date<=0)
		return FALSE;
	double diff = fabs(date - pdate);
	if (diff<mind) 
		{
		mind=diff;
		mindate=date;
		return TRUE;
		}
	return FALSE;
}



#define MAXNUMS 4


int FACEBOOK_AdjustDate(int num, tnums nums[], double vdate, double &date, int &start, int &end)
{
	double vdatey = CDATA::GetNum(CDate(vdate)), vdatey1 = vdatey-1, vdatey2 = vdatey+1;
	double mindate = 0, mind = 1000;
	for (int n=num; n<MAXNUMS; ++n)
		nums[n].val = -1;
	for (int n=0; n<2; ++n)
		{
		double v0 = nums[n].val, v1 = nums[n+1].val, v2 = nums[n+2].val;
		if (v2>=0)
			{
			if (FACEBOOK_CheckDate( v0, v1, v2, mind, mindate, vdate))
				start = nums[n].start, end = nums[n+2].end;
			if (FACEBOOK_CheckDate( v1, v0, v2, mind, mindate, vdate))
				start = nums[n].start, end = nums[n+2].end;
			if (FACEBOOK_CheckDate( v2, v1, v0, mind, mindate, vdate))
				start = nums[n].start, end = nums[n+2].end;
			if (FACEBOOK_CheckDate( v2, v0, v1, mind, mindate, vdate))
				start = nums[n].start, end = nums[n+2].end;
			}
		else
			{
			if (FACEBOOK_CheckDate( v0, v1, vdatey, mind, mindate, vdate))
				start = nums[n].start, end = nums[n+1].end;
			if (FACEBOOK_CheckDate( v1, v0, vdatey, mind, mindate, vdate))
				start = nums[n].start, end = nums[n+1].end;
			if (FACEBOOK_CheckDate( v0, v1, vdatey1, mind, mindate, vdate))
				start = nums[n].start, end = nums[n+1].end;
			if (FACEBOOK_CheckDate( v1, v0, vdatey1, mind, mindate, vdate))
				start = nums[n].start, end = nums[n+1].end;
			if (FACEBOOK_CheckDate( v0, v1, vdatey2, mind, mindate, vdate))
				start = nums[n].start, end = nums[n+1].end;
			if (FACEBOOK_CheckDate( v1, v0, vdatey2, mind, mindate, vdate))
				start = nums[n].start, end = nums[n+1].end;
			}
		if (nums[3].val<=0)
			break;
		}
	if (mindate<=0)
		return FALSE;

	date = mindate;
	return TRUE;
}


CSymList months, weekdays, reldate;

void FACEBOOK_Init(void)
{
	static int init = 0;
	if (init)
		return;
	init = TRUE;

	// months
	{
	CSymList tmp;
	tmp.Load(filename(TRANSBASIC"MONTH"));
	for (int i=0; i<tmp.GetSize(); ++i)
		if (months.Find(tmp[i].id)<0)
			{
			double m = tmp[i].GetNum(0);
			if (m<1 || m>12)
				Log(LOGERR, "Invalid month %s=%s", tmp[i].id, CData(m));
			else
				months.Add(CSym(stripAccents2(tmp[i].id), CData(m)));
			}
	}

	// date & week
	{
	CSymList tmp;
	tmp.Load(filename(TRANSBASIC"DATE"));
	for (int i=0; i<tmp.GetSize(); ++i)
		{
		if (tmp[i].id.GetLength()>1)
			{
			// weekdays
			double n = tmp[i].GetNum(0);
			if (n<0 || n>7)
				Log(LOGERR, "Invalid weekday %s=%s", tmp[i].id, tmp[i].data);
			else
				weekdays.Add(CSym(stripAccents2(tmp[i].id), CData(n)));
			}
		else
		if (tmp[i].id.GetLength()==1)
			{
			// match
			vara list(tmp[i].data);
			for (int l=0; l<list.length(); ++l)
				reldate.AddUnique(CSym(stripAccents2(list[l]),tmp[i].id));
			}
		}
	}

}


double FACEBOOK_GetMonth(const char *m)
{
	for (int i=0; i<months.GetSize(); ++i)
		if (IsSimilar(m,months[i].id))
			return months[i].GetNum(0);
	return 0;
}

#define DELDATE " < : > "
int FACEBOOK_MatchRelativeDate(double odate, vars &line, double &date, vars &decipher)
{
	FACEBOOK_Init();
	if (line.IsEmpty())
		return FALSE;

	/*
	// hoy, yesterday
	static const char *match[][10] = {
		//{ "I", "year ago", "years ago", NULL },
		{ "0", "today", "this morning", "this afternoon", "hoy", "esta mañana", "esta tarde", "avui", "ahir", NULL },
		{ "1", "yesterday", "last night", "ayer", "anoche", NULL },
		{ "L", "last", "pasado", NULL },
		{ "A", "days ago", NULL },
		{ "P", "pasado", "pasada", "passat", NULL },
		NULL
		};
	*/


	for (int f=0; f<line.GetLength(); ++f)
	  {
	  int n = -1;
	  f += countnotalphanum(line);
	  if (IsMatchList((const char*)line + f, reldate, TRUE, &n))
		{
		vars m = reldate[n].id;
		int flen = strlen(m);
		if (f>0 && isa(line[f-1]))
			continue; // only full words

		int iw;
		double daynum = -1, dayago = -1;
		char mc = reldate[n].data[0];
		switch (mc)
			{
			case '0':
			case '1':
				daynum = mc-'0';
				break;

			case 'I': // ignore
				dayago = daynum = -1;
				break;
			
			case 'A':
				{
				static const char *numbers[] = { "today", "one", "two", "three", "four", "five", "six", "seven", NULL };			
				int l;
				for (l=f-1; l>=0 && !isanum(line[l]); --l);
				for (; l>=0 && isanum(line[l]); --l); ++l;
				if (l>=0)
					if ((daynum = IsMatchNC(((const char *)line)+l, numbers))>0 || (daynum=CDATA::GetNum(((const char *)line)+l))>0)
						flen = f+flen - l, f = l;
				}
				break;

			case 'L':
				{
				// last sunday
				flen += countnotalphanum((const char *)line+f+flen);
				if (IsMatchList(((const char *)line)+f+flen, weekdays, TRUE, &iw, 3))
					{
					dayago = weekdays[iw].GetNum(0);
					// make full word
					while (isa(line[f+flen])) ++flen;
					}
				}
				break;

			case 'P':
				{
				// pasado domingo OR domingo pasado
				flen += countnotalphanum((const char *)line+f+flen);
				//if ((dayago = IsMatchNC(((const char *)line)+f+flen, weekdays))>=0)
				if (IsMatchList(((const char *)line)+f+flen, weekdays, TRUE, &iw, 3))
					{
					dayago = weekdays[iw].GetNum(0);
					// make full word
					while (isa(line[f+flen])) ++flen;
					}
				else
					{
					int l;
					for (l=f-1; l>=0 && !isa(line[l]); --l);
					for (; l>=0 && isa(line[l]); --l); ++l;
					if (l>=0 && IsMatchList(((const char *)line)+l, weekdays, TRUE, &iw, 3))
						{
						dayago = weekdays[iw].GetNum(0);
						flen = f+flen - l, f = l;
						}
					}
				}
				break;
			}
	
		if (dayago>=0)
			{
			// adjust for weekend
			if (dayago>=7)
				dayago = 5;
			// adjust for last 'day'
			int day = ((int)odate)%7;  // 3
			daynum = day - dayago;  // 3 - 5 = -2
			if (daynum<0) daynum += 7; // 5
			}

		if (daynum>=0)
			{
			date = odate - daynum;
			vars datestr = line.Mid(f, flen); line.Delete(f, flen); line.Insert(f, DELDATE);
			decipher = MkString("'%s' = -%g = %s", datestr, daynum, CDate(date));
			return TRUE;
			}
		}
	  }

	return FALSE;
}

int CheckYear(vars &line, const char *year)
{
	int f = line.Find(year);
	if (f<0) return FALSE;
	return (!isdigit(line[f+strlen(year)]) && (f==0 || !isdigit(line[f-1])));
}

public:

int FACEBOOK_MatchDate(double odate, vars &oline, double &date, vars &decipher)
{
	FACEBOOK_Init();

	// detect
	static const char *numof[] = { " of ", " de ", NULL };
	static const char *numbering[] = { "st", "nd", "rd", "th", NULL };
	static const char *numignore[] = { "\"", "'", "am", "pm", NULL };
	static const char *sep = " \t\r\n_.;,/-"; // no : time

	const char *str = oline;

	tnums nums[MAXNUMS];
	int retry = 0;
	int start = -1, num = 0, nummonth = 0;
	int i=0; 
	while (TRUE)
		{
		// ignore sep
		//while (strchr(sep, line[i]) && line[i]!=0)
		//	++i;

		if (isdigit(str[i]))
			{
			// found number
			nums[num].start = i;
			nums[num].val = CDATA::GetNum(str+i);
			while (isdigit(str[i]))
				++i;
			if (IsMatch(str+i, numignore))
				{
				FBLog(LOGINFO, FBDEBUG"Ignoring #' number in %.500s", str);
				continue;
				}
			if (IsMatch(str+i, numbering))
				while (isa(str[i]))
					++i;
			if (IsMatch(str+i, numof))
				while (isa(str[++i]))
					++i;
			while (strchr(sep, str[i]) && str[i]!=0)
				++i;

			nums[num].end = i;
			++num;
			if (num==MAXNUMS)
				goto datecheck;
			continue;
			}

		// ie: Dec 
		double m = FACEBOOK_GetMonth(str+i);
		if (m>0)
			{
			// found month
			if (++nummonth>1)
				goto datecheck; // too many months
			nums[num].start = i;
			nums[num].val = m;
			// August2016 ok
			while (!strchr(sep, str[i]) && !isdigit(str[i]) && str[i]!=0)
				++i;
			if (IsMatch(str+i, numof))
				while (isa(str[++i]))
					++i;
			while (strchr(sep, str[i]) && str[i]!=0)
				++i;
			nums[num].end = i;
			++num;
			if (num==MAXNUMS)
				goto datecheck;
			continue;
			}

		// ie: Jueves 30
		int iw;
		if (IsMatchList(str+i, weekdays, TRUE, &iw, 3))
			{
			int start = i;
			while (isa(str[i])) ++i;
			while (!isanum(str[i])) ++i;
			double dayn = CDATA::GetNum(str+i);
			if (dayn<=0)
				continue;

			// skip num
			int end = i;
			while (isdigit(str[end])) ++end;

			// adjust for weekend
			double dayago = weekdays[iw].GetNum(0);
			if (dayago>=7)
				dayago = 5;
			// adjust for last 'day'
			int day = ((int)odate)%7;  // 3
			double daynum = day - dayago;  // 3 - 5 = -2
			if (daynum<0) daynum += 7; // 5
			double ddate = odate - daynum;

			double ddayn = CDATA::GetNum(GetToken(CDate(ddate),2,'-'));
			if (ddayn!=dayn)
				continue;

			date = ddate;
			vars datestr = oline.Mid(start, end-start); oline.Delete(start, end-start); oline.Insert(start, DELDATE);
			decipher = MkString("'%s' = %s", datestr, CDate(date));
			return TRUE;
			}




datecheck:
		// bla bla
		if (num>=2)
			{
			// try to decipher
			int start = -1, end = -1;
			if (FACEBOOK_AdjustDate(num, nums, odate, date, start, end))
				{
				vars datestr = oline.Mid(start, end-start); oline.Delete(start, end-start); oline.Insert(start, DELDATE);
				decipher = MkString("'%s' = %s", datestr, CDate(date));
				return num>2 ? 1 : -1;
				}
			}

		// skip words
		//if (++retry>10)
		//	break;
		num = nummonth = 0;
		while (!strchr(sep, str[i]) && !isdigit(str[i]) && str[i]!=0)
			++i;
		while (strchr(sep, str[i]) && str[i]!=0)
			++i;
		if (str[i]==0)
			break;
		}


	// find 
	if (!IsSimilar(str, "http"))
		if (CheckYear(oline, "17") || CheckYear(oline, "16") || CheckYear(oline, "2017") || CheckYear(oline, "2016"))
			FBLog(LOGWARN, "? Missed date? '16/17'? '%.500s'",  oline);

	// try relative
	return FACEBOOK_MatchRelativeDate(odate, oline, date, decipher);
}

} FBDATE;




int FACEBOOK_MatchLink(const char *line, CSymList &matchlist)
{
	int match = 0;
	vara links(line, "href=");
	for (int i=1; i<links.length(); ++i)
		{
		vars link = GetToken(links[i], 0, " >");
		vars url = url_decode(ExtractString(link.Trim(" '\""),"u=http", "", "&"));
		if (url.IsEmpty())
			continue;
		url = urlstr("http"+url);
		if (strstr(url, "ropewiki.com"))
			url.Replace(" ", "_");
		int f = titlebetalist.Find(url);

		if (f>=0)
			{
			++match;
			matchlist.AddUnique(titlebetalist[f]);
			}
		}
	return match;
}







class PSYMAKA {

public:
CSym *sym;
vara aka;
};

static char *PICMODE = NULL;
static const char *equivlist[] = { " (Upper)"," (Middle)"," (Lower)"," (Full)"," I "," II "," III "," IV "," V "," 1 "," 2 "," 3 "," 4 "," 5 ", "1", "2", "3", "4", "5", NULL };

class FACEBOOKMATCH
{
CArrayList <PSYMAKA> akalist;
CSymList mainsublist, uplowsublist, uplowsublist2, prelist, presublist, postsublist, midlist, midsublist, shortsublist, typosublist, typo2sublist, startlist, endlist, canyonlist, commonlist, akacommonlist, ignorelistpre, ignorelistpost, reglist;

BOOL  AKA_IsSaved(CSym &sym, CSym &asym)
{
	vars aaka = asym.GetStr(ITEM_AKA);
	vars aka = sym.GetStr(ITEM_AKA);
	vars adesc = asym.GetStr(ITEM_DESC);
	vars desc = sym.GetStr(ITEM_DESC);
	if (adesc!=desc)
		return FALSE;
	if (aaka!=aka)
		return FALSE;
	return TRUE;
}


void AKA_Save(const char *file)
{
	CSymList savelist;
	for (int i=0; i<akalist.GetSize(); ++i)
		{
		CSym sym(akalist[i].sym->id,akalist[i].sym->GetStr(ITEM_DESC));
		sym.SetStr(ITEM_AKA, akalist[i].sym->GetStr(ITEM_AKA));
		sym.SetStr(ITEM_MATCH, akalist[i].aka.join(";"));
		savelist.Add(sym);
		}
	savelist.Save(file);
}

void AKA_Push(vara &aka, const char *str)
{
	str = skipnotalphanum(str);
	if (strlen(str)<3)
		return;
	if (aka.indexOf(str)>=0)
		return;
	if (akacommonlist.Find(str)>=0)
		return;
	ASSERT(isanum(*str));
	aka.push(str);
}

void AKA_Add(vara &aka, const char *_str)
{
	// NO NEED TO ASSERT, OPEN AKA.CSV!!! [DEBUG]
	//ASSERT(!strstr(_str, "Consusa"));
	//ASSERT(!strstr(_str, "Cueva de la Finca Siguanha"));
	
	// normal name
	vars st = stripAccents2(_str); st.Replace("%2C",",");

	// substitution lists
	vars ostr = st;
	CSymList *sublists[] = { &mainsublist, &midsublist, &uplowsublist, &postsublist, &presublist, &shortsublist, &typosublist, &typo2sublist, NULL };
	for (int i=0; sublists[i]; ++i)
		{
		vars str = FACEBOOK_SubstList(ostr, *(sublists[i]));
		if (i==0 || str!=ostr)
			{
			ostr = str;
			AKA_Push(aka, str);
			/*
			// no spaces version for hash
			if (hash)
				if (str.Replace(" ",""))
					AKA_Push(aka, str);
			*/
			}
		}

	// (Devils Jumpoff)
	//ASSERT(!strstr(st, "jumpoff"));
	vars parenthesis = GetToken(st, 1, "()").Trim();
	if (!parenthesis.IsEmpty())
		{
		vars noparenthesis = GetToken(st, 0, "()").Trim();
		AKA_Add(aka, noparenthesis);
		AKA_Add(aka, parenthesis);
		AKA_Add(aka, parenthesis+vars(" ")+noparenthesis);
		AKA_Add(aka, noparenthesis+vars(" ")+parenthesis);
		}
	else
		{
		// AND / OR
		int subst = 0;
		for (int s=0; andstr[s]!=NULL; ++s)
				if (st.Replace(andstr[s], ";"))
					++subst;
		if (subst)
			{
			vara stlist(st, ";");
			for (int i=0; i<stlist.length(); ++i)
				AKA_Add(aka, stlist[i]);
			}
		}
}

void AKA_Load(vara &aka, CSym &sym)
{
		AKA_Add(aka, sym.GetStr(ITEM_DESC));
		vara akalist(sym.GetStr(ITEM_AKA), ";");
		for (int i=0; i<akalist.length(); ++i)
			AKA_Add(aka, akalist[i]);
#ifdef DEBUGXXX
		if (strstr(sym.data, "Lytle"))
			{
			Log(LOGINFO, "%s", sym.GetStr(ITEM_DESC));
			for (int i=0; i<aka.aka.length(); ++i)
				Log(LOGINFO, "#%d %s", i, aka.aka[i]);
			i = i+1;
			}
#endif
		// set full region	
}



vars FACEBOOK_SubstList(const char *str, CSymList &list)
{
	vars line = " " + vars(str) + " ";
	for (int i=0; i<list.GetSize(); ++i)
	  iReplace(line, list[i].id, list[i].data);

	while(line.Replace("  "," "));
	return line.Trim(" .,;");
}



#define PUNCTCHARS ".:,;/!?@#$%^&*=+//[]{}<>"

int FACEBOOK_MatchName(int &l, vars &lmatch, const char *matchline, const char *matchname, const char *strflags)
{
			matchline = skipnotalphanum(matchline);
			if (*matchline==0)
				return 0;
			ASSERT(isanum(matchline[0]));
			ASSERT(isanum(matchname[0]));

			//ASSERT(!(IsSimilar(matchline, "barrancomascun") && IsSimilar(matchname, "barranco mascun")));

			// best match on simple name
			int perfects[2] = { 0, 0 };
			//const char *matchsep = " -()"PUNCTCHARS;
			int lold = l;
			int lnew = BestMatchSimple(matchname, matchline, perfects);
			if (lnew<MINCHARMATCH || lnew<=lold)
				return 0;

			// acept partial match if no flags
			if (!strflags) 
				{
				//if (FACEBOOK_CommonNames(matchname, mainname)>=perfects[0])
				//	return 0;
				//ASSERT(!IsSimilar(matchline, "Escalante"));
				//l = perfects[0]>0 ? lnew+1: lnew;
				l = lnew;
				lmatch = CString(matchname, lnew);
				}

			// perfect matches
			if (perfects[0]<MINCHARMATCH || perfects[0]<=lold)
				return 0;
			// accept full match only
			if (perfects[0]<(int)strlen(matchname))
				return 0;

			int lpnew = perfects[0];

			// avoid unwanted matches (ALL common words, empty meaning)
			//if (FACEBOOK_CommonNames(matchname, mainname)>=perfects[0])
			//	return 0;
			if (IsMatchList(matchname, commonlist)==perfects[0])
				return 0;
			if (lpnew==MINCHARMATCH && !IsSimilar(matchline, matchname))
				return 0;

			//const char *matchnamell = skipblanks(matchname+ll);
			const char *postmatch = skipblanks(matchline+perfects[1]);

			// avoid unwanted matches (ignore list)
			if (IsMatchList(postmatch, ignorelistpost))
				return 0;

			// empty meaning
			vara words(matchname, " ");

			/*
			if (!mainname || !IsSimilar(match, mainname))
				{
				vars &last = words.last();
				// check if "Torrrente de ...", "Baranco de San ..." ... 
				if (IsMatchList(last, midlist)==last.GetLength())
					return 0;
				// check if "Barranco del Rio ...", "Baranco Arroyo ..."
				BOOL significant = FALSE;
				for (int i=0; i<words.length(); ++i)
					if (!IsMatchList(words[i], prelist) && !IsMatchList(words[i], midlist))
						significant = TRUE;
				if (!significant)
					return 0;
				// weird stuff: more than 2 char per word
				if (strlen(match)/words.length()<=2)
					return 0;
				// one word must be a total match
				if (words.length()<2)
					if (!IsSimilar(match, matchname))
						return 0;
				}
			*/

			int nn = words.length();
			if (nn<2)
				{
				// if not MATCH MORE, skip single words unless 
				if (strflags && !FCHK(strflags,F_MATCHMORE))
					{
					int match = 0;
					// no one word match unless its 'X canyon' 'X caudal' or 'X [date]' 'X :'
					if (IsMatchList(postmatch, canyonlist))
						++match;
					if (IsSimilar(postmatch, vars(DELDATE).Trim()))
						++match;
					//if (*postmatch==':')
					if (!isanum(*postmatch))
						++match;
					if (!match)
						return 0;
					}
				}

			// GOT IT!
			l = lpnew;
			lmatch = matchname;
			return TRUE;
}

private:
#define HASHTAG "[#]" 
//#define HASHTAG2 "#"

typedef CArrayList <vars> CPLineArrayList;

void FACEBOOK_AddLine(vara *pline, const char *line)
{
	vars line1(line, 80); // 80 char max
	pline->push(line1);

	// barranco de X vs barranco X
	vars line2 = FACEBOOK_SubstList(line1, midsublist);
	if (line1!=line2 && pline->indexOf(line2)<0)
		pline->push(line2);

	// barranco X inferior vs barranco X
	vars line3 = FACEBOOK_SubstList(line2, uplowsublist);
	if (line2!=line3 && pline->indexOf(line3)<0)
		pline->push(line3);

	vars line4 = FACEBOOK_SubstList(line3, uplowsublist2);
	if (line3!=line4 && pline->indexOf(line4)<0)
		pline->push(line4);

	// process hashtags: barrancodelabolulla vs delabolulla vs bolulla
	int len = 0;
	while ((len=IsMatchList(line, prelist, FALSE))>0 && isanum(line[len]))
		pline->push(line += len);
	while ((len=IsMatchList(line, midlist, FALSE))>0 && isanum(line[len]))
		pline->push(line += len);
}


int FACEBOOK_MatchLine(const char *linestr, CSymList &matchlist, vara &matchregions, const char *strflags, int retry)
{
	linestr = skipnotalphanum(linestr);
	if (!linestr || *linestr==0)
		return FALSE;

	int maxl = MINCHARMATCH;
   // try taking out the prefix
	
	vara pline;
	FACEBOOK_AddLine(&pline, linestr);	//FACEBOOK_ScanLine(linestr, &pline, hash);

	// scan name database
	CSymList matched;
	for (int i=0; i<akalist.GetSize(); ++i)
		{
		int l = 1;
		vars lmatch;
		CSym &sym = *akalist[i].sym;
		vara &aka = akalist[i].aka;

		// region block
		//if (FCHKstrflags,F_REGSTRICT) || FCHK(strflags,F_GROUP))
		if (matchregions.length()>0)
			{
			vars fullregion = sym.GetStr(ITEM_REGION);
			// force to belong to a specific region set
			BOOL matched = matchregions[0].IsEmpty();
			for (int r=0; r<matchregions.length() && !matched; ++r)
				if (strstri(fullregion, matchregions[r]))
					matched = TRUE;
			if (!matched)
				continue;
			}

		// must match partially
		for (int a=0; a<aka.length(); ++a) // aka
			for (int p=0; p<pline.length(); ++p) // plines
				FACEBOOK_MatchName(l, lmatch, pline[p], aka[a], strflags);

		if (l>=maxl)
			{
			if (l>maxl) 
				{
				// reset list for better matches
				maxl=l;
				matched.Empty();
				}

			// keep tracks of all equal lenght matches
			CSym msym(sym.id, sym.data);
			double score = FACEBOOK_Score(sym, retry, linestr, lmatch, matchregions);
			msym.SetNum(M_LEN, l);
			msym.SetStr(M_NUM, "X"); //***simple for barrancodelsol vs barranco del sol
			msym.SetStr(M_MATCH, lmatch);
			msym.SetNum(M_RETRY, retry);
			msym.SetNum(M_SCORE, score);
			matched.Add(msym);
			}
		}

	if (matched.GetSize()==0)
		return FALSE;

	for (int i=0; i<matched.GetSize(); ++i)
		{
		int found = matchlist.Find(matched[i].id);
		if (found<0 || matched[i].GetStr(M_MATCH)!=matchlist[found].GetStr(M_MATCH))
			matchlist.Add(matched[i]);
		}

   return TRUE;
}



public:

#if 0
const char *FACEBOOK_ScanLine(const char *line, vara *pline = NULL, int hash = FALSE)
{
	// skip startlist words
	while (IsMatchList(line = skipnotalphanum(line), startlist, !hash))
		{
		// SHOULD SKIP, BUT JUST TO BE SURE WE DONT SKIP ANYTHING IMPORTANT! ie:"Running springs"
		FACEBOOK_AddLine(pline, line); 
		line = skipalphanum(line);
		}

	// scan all prelist words +1
	while (IsMatchList(line = skipnotalphanum(line), prelist, !hash) || IsMatchList(line = skipnotalphanum(line), midlist, !hash))
		{
		FACEBOOK_AddLine(pline, line);
		line = skipalphanum(line);
		}

	if (*line!=0)
		FACEBOOK_AddLine(pline, line);

	/*
	// scan all prelist words +1 FOR HASHTAGS!!!
	int len = 0;
	while ((len=IsMatchList(line, prelist, FALSE)))
		{
		line += len;
		if (pline) pline->AddUnique(line);
		}
	*/

	// if there is a () after the name, scan that too  
	// Chorros (Gavilanes) [Y]
	// Agua mansa (Gavilanes) [Y]
	// Helada. (no se) [N]
	const char *extra = line;
	for (int r=0; r<3; ++r)
		{
		extra = skipblanks(skipalphanum(extra));
		if (*extra=='(')
			FACEBOOK_AddLine(pline, skipnotalphanum(extra));
		}

	return line;
}

#endif


void FACEBOOK_MatchLists(const char *linestr, CSymList &matchlist, vara &matchregions, const char *strflags)
{
	// MATCH MORE WHEN NEEDED!!!
	int scannext = TRUE;
	const char *scanchr = "#:-/=([\"";

	int words = 0;
	int retry = 0;
	int maxretry = FCHK(strflags,F_MATCHMORE) ? 5 : 2;

	// try N first Capitalized words: me and erl gave a shot at Eaton Canyon
	for (const char *linescan = skipnotalphanum(linestr), *nextscan = NULL; *linescan!=0 && retry<maxretry; linescan = nextscan, ++words)
	{
		int skipretry = FALSE;
		int scan = scannext || words<3; scannext = FALSE;

		// find next word (and special symbols coming up)
		const char *linescanend;
		for (linescanend=linescan; isanum(*linescanend); ++linescanend); // skip alpha
		for (nextscan=linescanend; *nextscan!=0 && !isanum(*nextscan); ++nextscan) // skip not alpha
			if (strchr(scanchr, *nextscan))
				++scannext;

		// end scan at ignorelist words
		if (IsMatchList(linescan, ignorelistpre))
			return;


		// try ALL pre or mid words and do not count retries (more restrictive with mid bc shorter)
		if (IsMatchList(linescan, prelist, FALSE) || IsMatchList(linescan, midlist, TRUE))
			scan++, scannext++, skipretry=TRUE;

		// try all the words in the line
		if (!scan && FCHK(strflags,F_MATCHANY)) 
			scan++;

		// try uppercase words
		if (!scan && isupper(*linescan))
			scan++;

		// try numbers (7 Teacups 12 cascadas)
		if (!scan && isdigit(*linescan) && *linescanend==' ')
			scan++;

		// try X Creek or X Canyon
		if (!scan && IsMatchList(nextscan, canyonlist))
			scan++;

		// try startlist word // SHOULD SKIP, BUT JUST TO BE SURE WE DONT SKIP ANYTHING IMPORTANT! ie:"Running springs"
		if (IsMatchList(linescan, startlist))
			scan++, scannext++, skipretry=TRUE;

		// scan
		if (!scan)
			continue;

		// scan and move forward
		FACEBOOK_MatchLine(linescan, matchlist, matchregions, strflags, retry);
		if (!skipretry) 
			++retry;
	}

	// in God we trusted!
}



public:



vars GetMatch(CSym &sym)
{
	return MkString("'%s' : '%s' %sS %sR %sL %s*", sym.GetStr(ITEM_DESC), sym.GetStr(M_MATCH), sym.GetStr(M_SCORE), sym.GetStr(M_RETRY), sym.GetStr(M_LEN), sym.GetStr(ITEM_STARS));
}

enum { C_EQUAL = 0, C_STRSTR };
int CmpMatch(const char *main, const char *match, int cmp)
{
	if (*main==0 || *match==0)
		return FALSE;

	vars smain = vars(main).replace(" ", "  ");
	vars smatch = vars(match).replace(" ", "  ");
	switch (cmp)
	{
		case C_EQUAL:
			return stricmp(main,match)==0 || stricmp(smain, smatch);
		case C_STRSTR:
			return strstri(main, match) || strstri(smain, smatch);
	}

	return FALSE;
}


void FACEBOOK_InitAKA(void)
{
	static int initialized = FALSE;
	if (initialized)
		return;

	initialized = TRUE;

	FACEBOOK_Init();
	if (rwlist.GetSize()==0)
		LoadRWList();

	if (akalist.GetSize()>0)
		return;

	CSymList loadlist;
	vars file = filename("AKA");
	if (CFILE::exist(file))
		{
		FBLog(LOGINFO, "Loading pre-existing %s", file);
		loadlist.Load(file);
		loadlist.Sort();
		}

	FBLog(LOGINFO, "Created new file %s", file);
	akalist.SetSize(rwlist.GetSize());
	for (int i=0; i<akalist.GetSize(); ++i)
		{
		CSym &sym = rwlist[i];
		akalist[i].sym = &sym;
		printf("%d/%d building AKA list...        \r", i, akalist.GetSize());
	
		int f = loadlist.Find(sym.id);
		if (f<0 || !AKA_IsSaved(sym, loadlist[f]))
			f = -1;
		if (f<0)
			AKA_Load(akalist[i].aka, sym); 
		else
			akalist[i].aka = vara(loadlist[f].GetStr(ITEM_MATCH), ";");

		sym.SetStr(ITEM_REGION, _GeoRegion.GetFullRegion(sym.GetStr(ITEM_REGION)));
		}
	AKA_Save(file);
}


int BestMatch(CSym &sym, const char *prdesc, const char *prnames, const char *prregion, int &perfect)
{
	// 
	vars match;
	int l = 1;
	perfect = FALSE;

	// must match partially
	vara pline(prnames, ";");
	vars akastr = sym.GetStr(ITEM_BESTMATCH);
	vara aka(akastr, ";");
	//aka.push(sym.GetStr(ITEM_DESC));

	// add every single word source word that is not
	for (const char *pr = skipnotalphanum(prdesc); *pr!=0; pr = skipnotalphanum(skipalphanum(pr)))
		pline.push(pr);		

	for (int a=0; a<aka.length(); ++a) // aka
		for (int p=0; p<pline.length(); ++p) // plines			
			if (FBMATCH.FACEBOOK_MatchName(l, match, pline[p], aka[a], NULL))
				perfect = TRUE;
	
	double score = FACEBOOK_Score(sym, 0, prdesc, match, vara(prregion, ";"));
	return (int)score;
}


vars GetBestMatch(CSym &sym)
{
	FACEBOOK_InitAKA();

	vara aka;
	int f = rwlist.Find(sym.id);
	if (f>=0)
		aka = akalist[f].aka;
	else
		AKA_Load(aka, sym); 

	// allow no common words
	//vars name = sym.GetStr();
	for (int i=0; i<aka.length(); ++i)
		if (IsMatchList(aka[i], prelist))
			aka.RemoveAt(i--);

	return aka.join(";");
}




#if 0

		// prepare name list to countermatch common list check
		CSymList namelist;
		vars name1 = GetToken(sym.GetStr(ITEM_DESC), 0, "()[]").Trim();
		if (!name1.IsEmpty()) namelist.Add(CSym(name1));
		vars name2 = GetToken(sym.GetStr(ITEM_DESC), 1, "()[]").Trim();
		if (!name2.IsEmpty()) namelist.Add(CSym(name2));
		vara symaka( sym.GetStr(ITEM_AKA), ";" );
		for (int a=0; a<symaka.length(); ++a)
			namelist.Add(CSym(symaka[a].Trim()));
		namelist.Sort(cmplength);

#endif



int FACEBOOK_CommonNames(const char *matchname, CSymList *mainname = NULL, BOOL noalpha = TRUE)
{
		int len;
		const char *ptr = matchname;
		while ((len=IsMatchList(ptr, commonlist, noalpha))>0)
			{
			if (ptr>matchname)
				if (mainname && IsMatchList(ptr, *mainname, noalpha))
					break;
			ptr = skipnotalphanum(ptr + len);
			}
		return ptr-matchname;
}

double FACEBOOK_Score(CSym &sym, int retry, const char *line, const char *match, vara &prefregions)
{
		int len = strlen(match); 
		//int nwords = vara(match, " ").length();
		vars name = vars(sym.GetStr(ITEM_DESC));

		// initial score
		int commlen = FACEBOOK_CommonNames(match);
		double score = (len - commlen/2) / (retry+1);

		// advantage for region mentioned in line
		vara matchregions(_GeoRegion.GetFullRegion(sym.GetStr(ITEM_REGION)), ";" );
		for (int r=0; r<matchregions.length(); ++r)
			if (prefregions.indexOfi(matchregions[r])>=0)
				score += score/2; // 50% more

		// advantage for perfect title match
		//if (strlen(match)<=len) 
		//	score += 1; 
		
		// advantage for simplified title match
		vars omatch = match, oname = name, oline = line;
		CSymList *sublists[] = { &mainsublist, &midsublist, &postsublist, &presublist, &shortsublist, &typosublist, &typo2sublist, NULL };
		for (int i=0; sublists[i]; ++i)
			{
			vars line = FACEBOOK_SubstList(oline, *(sublists[i]));
			vars name = FACEBOOK_SubstList(oname, *(sublists[i]));
			vars match = FACEBOOK_SubstList(omatch, *(sublists[i]));
			vars name1 = GetToken(name, 0, "()");
			vars name2 = GetToken(name, 1, "()");

			double scoreadd = 0, scorediv = 1;
			// advantage for perfect match
			if (CmpMatch(name, match, C_EQUAL))
				scoreadd += 0.5;

			// advantage for semi match
			if (CmpMatch(name1, match, C_STRSTR))
				scoreadd += name2.IsEmpty() ? 0.9 : 0.75;
			if (CmpMatch(name2, match, C_STRSTR))
					scoreadd += 0.5;

			// advantage for name mentioned in line
			if (CmpMatch(line, name, C_STRSTR))
				scoreadd += 0.5;
			if (CmpMatch(line, name1, C_STRSTR))
				scoreadd += 0.4;
			if (CmpMatch(line, name2, C_STRSTR))
				scoreadd += 0.3;

			// penalize Torrente de, Barranco de, etc.
			if (match.IsEmpty())
				scorediv = 2;

			oname = line;
			oname = name;
			omatch = match;
			score = (score + scoreadd / (i+1)) / scorediv;
			}

		return score*100;
}



int FACEBOOK_Match(const char *_line, CSymList &reslist, const char *region, const char *strflags)
{
	_line = skipnotalphanum(_line);
	if (*_line==0)
		return FALSE;

	FACEBOOK_InitAKA();

	vars line = FACEBOOK_SubstList(_line, mainsublist);

	// try to find possible matches
	CSymList matchlist;
	vara prefregions(region,";");
	FACEBOOK_MatchLists(line, matchlist, prefregions, strflags);
	if (matchlist.GetSize()==0) // no match
		return FALSE;

if (!FCHK(strflags, F_MATCHMANY))
{
	// maximum match
	vara equiv;
	double maxe = -1;
	matchlist.SortNum(M_SCORE, -1);
#ifdef DEBUG
	if (matchlist.GetSize()>0)
	  for (int i=0; i<matchlist.GetSize(); ++i)
		Log(LOGINFO, "     BM#%d %s", i, GetMatch(matchlist[i]));
#endif
	for (int i=0; i<matchlist.GetSize(); ++i)
		{
		CSym &sym = matchlist[i];
		double e = sym.GetNum(M_SCORE);
		if (e<maxe)
			{
			matchlist.Delete(i--);
			continue;
			}
		maxe = e;
		vars match = " "+sym.GetStr(ITEM_DESC)+" "; 
		for (int j=0; equivlist[j]!=NULL; ++j)
			iReplace(match, equivlist[j], " ");
		while (match.Replace("  "," "));
		if (equiv.indexOfi(match)<0)
			equiv.push(match);
		}

	// so... how good is the result?
	vars match = matchlist[0].GetStr(M_MATCH);
	vars title = equiv[0]; //.GetStr(ITEM_DESC);

	/*
	if (!IsSimilar(title, match))
		{
		vara words(match, " ");
		// if NOT EXACTLY matching the title
		//if last word is a middle word "Torrente de", "Barranco del" etc. reject it, unless the official name matches itexactly
		vars last = words.last();
		if (IsMatchList(last, midlist)==last.GetLength())
			{
			Log(LOGWARN, "Rejecting inconclusive match [last word is a middle word] '%s' <=X= '%s'", title, match);
			return FALSE;
			}

		// if the match its all prefixes
		BOOL significant = FALSE;
		for (int i=0; i<words.length(); ++i)
			if (!IsMatchList(words[i], prelist) && !IsMatchList(words[i], midlist))
				significant = TRUE;
		if (!significant)
			{
			Log(LOGWARN, "Rejecting insignificant match [all words are pre words] '%s' <=X= '%s'", title, match);
			return FALSE;
			}
		}
	*/

	if (equiv.length()>1)
		{
		vars wlist;
		int MAXOVERMATCH = 10;
		// report overmatch
		Log(LOGWARN, "FBOVERMATCHED '%.256s' [%s,%s... #%d]", line, equiv[0], equiv[1], equiv.length());
		matchlist.SortNum(ITEM_STARS,-1);
		for (int i=0; i<MAXOVERMATCH && i<matchlist.GetSize(); ++i)
			Log(LOGWARN, "OM#%d %s", i, GetMatch(matchlist[i]));
		//ASSERT(!strstr(linestr, "Barranco del chorro"));
		//ASSERT(!strstr(linestr, "Parker Creek Canyon"));
		if (PICMODE)
			{
			// report overmatches
			for (int i=0; i<MAXOVERMATCH && i<matchlist.GetSize(); ++i)
				reslist.Add(matchlist[i]);
			return -1;
			}
		// pick best canyon if 3 overmatched
		if (matchlist.GetSize()<=3)
			{
			reslist.Add(matchlist[0]);
			return -1;
			}
		return FALSE;
		}
}

	reslist.Add(matchlist);
	return matchlist[0].GetNum(M_RETRY)>0 ? -1 : 1;
}


vara FACEBOOK_CheckUsed(vara &usedlist, CSymList &matchlist)
{
	vara dellist;
	  if (usedlist.length()>0) 
		{
		// already used date, delete any canyon that does not match
		for (int m=0; m<matchlist.GetSize(); ++m)
			{
			vars title = matchlist[m].GetStr(ITEM_DESC);
			if (usedlist.indexOf(title)<0)
				{
				dellist.push(title);
				matchlist.Delete(m--);
				}
			}
		}
	  else
		{
		// first time using it
		for (int m=0; m<matchlist.GetSize(); ++m)
			{
			vars title = matchlist[m].GetStr(ITEM_DESC);
			usedlist.push(title);
			}
		}
  return dellist;
}


vars FACEBOOK_MatchRegion(const char *text, const char *preregions)
{
	vara list;
	FACEBOOK_Init();

	while (*(text=skipnotalphanum(text))!=0)
		{
		int m;
		// region must start with Upper case and be in the database
		if (IsMatchList(text, reglist, TRUE, &m))
			if (list.indexOfi(reglist[m].id)<0)
				{
				vars reg = reglist[m].id;
				if (!reglist[m].data.IsEmpty())
					reg = reglist[m].data;
				if (reg=="X")
					continue; // skip some regions 'Como'

				list.push(reg);
				FBLog(LOGINFO, FBDEBUG"Detected region '%s' <= '%.30s...'", reg, text);
				}
		text = skipalphanum(text);
		}
	vara more(preregions, ";");
	for (int i=0; i<more.length(); ++i)
		if (list.indexOfi(more[i])<0)
			list.push(more[i]);

	return list.join(";");
}


private:



void PrepSublist(CSymList &list, const char *def = NULL)
{
	CSymList newlist;
	for (int i=0; i<list.GetSize(); ++i)
		{
		vars id = " "+stripAccents2(list[i].id).Trim()+" ";
		while(id.Replace("  ", " "));
		vars data = " "+stripAccents2(list[i].data).Trim()+" ";
		while(data.Replace("  ", " "));
		if (def)
			data = def;
		newlist.AddUnique(CSym(id, data));		
		}
	newlist.Sort(cmplength);
	list = newlist;
}


void FACEBOOK_Init()
{
	static int init = FALSE;
	if (init) return;
	init = TRUE;

	// ---------- LISTS ---------
	if (mainsublist.GetSize()==0)
		{
		CSymList basic, list, sublist, sublistp;

		// main synonims = standardized comparisons
		list.Load(filename(TRANSBASIC"PRE"));
		for (int i=0; i<list.GetSize(); ++i)
			{
			vars trans = list[i].data.Trim(" ()");
			if (!trans.IsEmpty())
				sublist.AddUnique(CSym(list[i].id, trans));
			}

		basic.Load(filename(TRANSBASIC));
		sublist.Add(basic);

		PrepSublist(sublist);

		// punct chars -> '.' => ' . '
		const char *blanks = "\t\r\n";
		for (int i=0; blanks[i]!=0; ++i)
			sublistp.AddUnique(CSym(CString(blanks[i])," "));
		const char *punct = PUNCTCHARS;
		for (int i=0; punct[i]!=0; ++i)
			sublistp.AddUnique(CSym(CString(punct[i])," "+CString(punct[i])+" "));
		sublistp.AddUnique(CSym("&#8217;","'"));
		sublistp.AddUnique(CSym("&#8217","'"));
		sublistp.Sort(cmplength);	

		//ignorelistpre.Load(filename("facebookignorepre"));
		//PrepSublist(ignorelistpre, "XXXX");

		// inferiore, superiore, etc.
		mainsublist.Add(sublistp);
		mainsublist.Add(sublist);
		mainsublist.Add(ignorelistpre);

		// uplow list (Upper) (Lower)
		mainsublist.AddUnique(CSym(")"," "));
		mainsublist.AddUnique(CSym("("," "));

		// uplow list for hash
		for (int i=0; i<basic.GetSize(); ++i)
			{
			vars id = basic[i].id; id.Trim();
			vars data = basic[i].data; data.Trim(" ()");
			if (id.GetLength()>=5)
				uplowsublist2.AddUnique(CSym(id, " "+data));
			}

		PrepSublist(sublist, " ");
		uplowsublist.AddUnique(sublist);
		}
	if (midsublist.GetSize()==0)
		{
		midsublist.Load(filename(TRANSBASIC"MID"));
		// extend with Devil's or "The Force"
		midsublist.AddUnique(CSym("'"));
		midsublist.AddUnique(CSym("\""));
		for (int i=0; i<midsublist.GetSize(); ++i)
			midsublist[i].data = " ";
		//PrepSublist(midsublist, " ");
		}
	if (postsublist.GetSize()==0)
		{
		postsublist.Load(filename(TRANSBASIC"POST"));
		PrepSublist(postsublist, " ");
		}
	if (presublist.GetSize()==0)
		{
		presublist.Load(filename(TRANSBASIC"PRE"));
		PrepSublist(presublist, " ");
		}
	if (shortsublist.GetSize()==0)
		{
		shortsublist.Load(filename(TRANSBASIC"SHORT"));
		PrepSublist(shortsublist);
		}
	if (typosublist.GetSize()==0)
		{
		typosublist.Load(filename(TRANSBASIC"TYPO"));
		//typosublist.Sort(cmplength);
		}
	if (typo2sublist.GetSize()==0)
		{
		typo2sublist.Load(filename(TRANSBASIC"TYPO2"));
		//typo2sublist.Sort(cmplength);
		}
	// IsMatchList lists
	if (prelist.GetSize()==0)
		{
		prelist.AddUnique(presublist);
		PrepMatchList(prelist);
		}
	if (midlist.GetSize()==0)
		{
		midlist.AddUnique(midsublist);
		PrepMatchList(midlist);
		}
	if (canyonlist.GetSize()==0)
		{
		const char *list[] = { "Canyon", "Creek", "Falls", "caudal", NULL };
		for (int i=0; list[i]!=NULL; ++i)
			canyonlist.AddUnique(CSym(list[i]));
		PrepMatchList(canyonlist);
		}
	if (startlist.GetSize()==0)
		{
		CSymList list;
		list.Load(filename("facebookstart"));
		for (int i=0; i<list.GetSize(); ++i)
			startlist.AddUnique(CSym(list[i].id));
		startlist.Add(CSym(HASHTAG));
		PrepMatchList(startlist);
		}
	if (endlist.GetSize()==0)
		{
		CSymList list;
		list.Load(filename("facebookend"));
		for (int i=0; i<list.GetSize(); ++i)
			endlist.AddUnique(CSym(list[i].id));	
		endlist.AddUnique(postsublist);
		for (int i=0; equivlist[i]!=NULL; ++i)
			endlist.AddUnique(CSym(equivlist[i]));
		PrepMatchList(endlist);
		}
	if (ignorelistpre.GetSize()==0)
		{
		ignorelistpre.Load(filename("facebookignorepre"));
		PrepMatchList(ignorelistpre);
		}
	if (ignorelistpost.GetSize()==0)
		{
		ignorelistpost.Load(filename("facebookignorepost"));
		PrepMatchList(ignorelistpost);
		}
	if (reglist.GetSize()==0)
		{
		// set up proper region detections
		CSymList redir;
		redir.Load(filename(RWREDIR));
		redir.Load(filename("facebookredir"));

		// main regions
		CSymList &regionlist = _GeoRegion.Regions();
		for (int i=0; i<regionlist.GetSize(); ++i)
			//if (!IsMatchList(regionlist[i].id, commonlist))
				{
				vars oreg = regionlist[i].id;
				reglist.Add(CSym(oreg));
				// synonims automatic list
				vars sreg = stripAccents2(oreg).Trim();
				if (sreg!=oreg)
					regionlist.AddUnique(CSym(sreg, oreg));
				if (iReplace(sreg = oreg, " national park", ""))
					regionlist.AddUnique(CSym(sreg.Trim(), oreg));
				if (iReplace(sreg = oreg, " national forest", ""))
					regionlist.AddUnique(CSym(sreg.Trim(), oreg));
				if (iReplace(sreg = oreg, " NRA", ""))
					regionlist.AddUnique(CSym(sreg.Trim(), oreg));
				if (iReplace(sreg = oreg, "Islas ", ""))
					regionlist.AddUnique(CSym(sreg.Trim(), oreg));
				}
		reglist.iSort();

		// redirections
		for (int i=0; i<redir.GetSize(); ++i)
			{
			vars name = redir[i].GetStr(ITEM_DESC);
			int f = regionlist.Find(name);
			if (f<0) continue;

			// set up synonims
			vara aka( redir[i].GetStr(ITEM_AKA), ";" );
			for (int a=0; a<aka.length(); ++a)
				//if (!IsMatchList(aka[a], commonlist))
					reglist.AddUnique(CSym(aka[a], name));
			}

		PrepMatchList(reglist);
		}

	if (commonlist.GetSize() ==0)
		{
		CSymList list;
		list.Load(filename("facebookcommon"));
		for (int i=0; i<list.GetSize(); ++i)
			commonlist.AddUnique(CSym(list[i].id));
		commonlist.AddUnique(presublist);
		commonlist.AddUnique(midsublist);
		commonlist.AddUnique(uplowsublist);
		commonlist.AddUnique(shortsublist);
		commonlist.AddUnique(postsublist);
		commonlist.AddUnique(startlist);
		commonlist.AddUnique(endlist);
		commonlist.AddUnique(ignorelistpre);
		commonlist.AddUnique(ignorelistpost);

		commonlist.AddUnique(reglist);
		commonlist.AddUnique(_GeoRegion.Regions());
		commonlist.AddUnique(_GeoRegion.Redirects());
		
		CSymList tmp;
		tmp.Load(filename(TRANSBASIC"DATE"));
		commonlist.AddUnique(tmp);
		//commonlist.AddUnique(months);
		PrepMatchList(commonlist);
		}

	// ---------- AKA LIST ---------
	akacommonlist = commonlist;
	akacommonlist.Sort();

#ifdef DEBUG
	mainsublist.Save("SUBMAINLIST.CSV");
	midsublist.Save("SUBMIDLIST.CSV");
	uplowsublist.Save("SUBUPLOWLIST.CSV");
	postsublist.Save("SUBPOSTLIST.CSV");
	presublist.Save("SUBPRELIST.CSV");
	shortsublist.Save("SUBSHORTLIST.CSV");
	typosublist.Save("SUBTYPOLIST.CSV");
	typo2sublist.Save("SUBTYPO2LIST.CSV");
	commonlist.Save("COMMON.CSV");
	reglist.Save("REGLIST.CSV");
#endif
}


} FBMATCH;


vars GetBestMatch(CSym &sym)
{
	return FBMATCH.GetBestMatch(sym);
}

int AdvancedBestMatch(CSym &sym, const char *prdesc, const char *prname, const char *prregion, int &perfect)
{
	//ASSERT(!strstri(prdesc, "Barranco del Estret") || !strstri(sym.data, "Estret") );
	return FBMATCH.BestMatch(sym, prdesc, prname, prregion, perfect);
}

int AdvancedMatchName(const char *linestr, const char *region, CSymList &reslist)
{
	int ret = FBMATCH.FACEBOOK_Match(stripAccents2(linestr), reslist, region, F_MATCHMORE F_MATCHMANY)>0;

	for (int i=1; i<reslist.GetSize(); ++i)
		if (reslist[i].GetStr(ITEM_DESC)==reslist[i-1].GetStr(ITEM_DESC))
			reslist.Delete(i--);


	return reslist.GetSize()>0;
}



int FACEBOOK_MatchWater(const char *line, vars &water, const char *label = NULL)
{
	//enum { W_DRY=0, W_WADING=1, W_SWIMMING=2, W_VERYLOW=3, W_LOW=4, W_MODLOW=5, W_MODERATE=6, W_MODHIGH=7, W_HIGH=8, W_VERYHIGH=9, W_EXTREME=10 };


	int n = -1;
	static CSymList waterlev;
	if (waterlev.GetSize()==0)
		{
		waterlev.Load(filename("facebookwater"));
		waterlev.Sort(cmplength);
		}

	if (!label)
		{
		/*
		too erratic

		// set up ignore list
		vara ignore;
		for (int m=0; m<matchlist.GetSize(); ++m)
			{
			vars l = stripAccents(matchlist[m].id);
			for (const char *str= skippunct(l); *str!=0; str=skippunct(skipalphanum(str)))
				if (IsMatchList(str, waterlev, TRUE, &n))
					ignore.push(waterlev[n].id);
			}

		// find water level EVERYWHERE
		vars l = stripAccents(line);
		const char *str = skippunct(l);
		for (int i=0; i<20 && *str!=0; ++i, str=skippunct(skipalphanum(str)))
			if (IsMatchList(str, waterlev, TRUE, &n))
				if (ignore.indexOf(waterlev[n].id)<0)
					{
					water = cond_water[atoi(waterlev[n].data)];
					return TRUE; ;
					}
		*/
		}
	else
		{
		const char *tomatch = strstri(line, label);
		if (tomatch)
			{
			int n = -1;
			while (*tomatch!=0 && isa(*tomatch)) 
				++tomatch;
			tomatch = skipnotalphanum(tomatch);
			if (IsMatchList(tomatch, waterlev, TRUE, &n))
				{
				water = cond_water[atoi(waterlev[n].data)];
				return TRUE; ;
				}
			else
				{
				FBLog(LOGWARN, "? Missed '%s'? '%.250s'", label, tomatch);
				}
			}
		}

	return FALSE;
}

vars GetFBSummary(vara &summary)
{
	vars v;
	vara out;
	if (!(v= GetToken(summary[R_STARS],0,'-')).IsEmpty())
		out.push("*:"+v.Trim());
	if (!(v= GetToken(summary[R_T],0,'-')).IsEmpty())
		out.push("D:"+v.Trim());
	if (!(v= GetToken(summary[R_W],0,'-')).IsEmpty())
		out.push("W:"+v.Trim());
	if (!(v= GetToken(summary[R_TEMP],0,'-')).IsEmpty())
		out.push("MM:"+v.Trim());
	if (!(v= GetToken(summary[R_TIME],0,'-')).IsEmpty())
		out.push("HR:"+v.Trim());
	if (!(v= GetToken(summary[R_PEOPLE],0,'-')).IsEmpty())
		out.push("PP:"+v.Trim());
	return out.join("|");
}


int FACEBOOK_MatchSummary(const char *line, vara &orating)
{

	int matched = 0;
	const char *match = NULL;

	vara rating; rating.SetSize(R_EXTENDED);
	if (!GetSummary(rating, line))
		{
		if (FACEBOOK_MatchWater(line, orating[R_W], "caudal"))
			{
			orating[R_SUMMARY] = line;
			return TRUE;
			}
		return FALSE;
		}

	vars ratingstr = rating.join(";");

	//enum { W_DRY=0, W_WADING=1, W_SWIMMING=2, W_VERYLOW=3, W_LOW=4, W_MODLOW=5, W_MODERATE=6, W_MODHIGH=7, W_HIGH=8, W_VERYHIGH=9, W_EXTREME=10 };
	static varas mwaterstrstr("a1=0,a2-=1,a2+=2,a2=3,a3=4,a4-=5,a4+=7,a4=6,a5=8,a6=9,a7=10,A=0,B-=1,B+=2,B/C=4,B=3,C0=4,C1-=5,C1+=7,C1=6,C2=8,C3=9,C4=10,C5=10,C-=5,C+=7,C=6");
	if ((match = mwaterstrstr.strstr(ratingstr)))			
		orating[R_W] = cond_water[atoi(match)];
	
	//static vara cond_diff("0 - Nontechnical,1 - Easy,2 - Normal,3 - Special challenges,4 - Advanced,5 - Extreme");
	static varas mdiffT("1=0,2=0,3=1,4=2");
	static varas mdiffV("v1=0,v2=1,v3=1,v4=2,v5=3,v6=4,v7=5");
	static varas mdiffX("PG=2,R=3,XX=5,X=4");
	static varas mdiffW("C1=2,C2=2,C3=2,C4=2");
	int matchnum = max(max(max(mdiffV.matchnum(rating[R_V]), mdiffX.matchnum(rating[R_X])), mdiffT.matchnum(rating[R_T])), mdiffW.matchnum(rating[R_W]));
	if (matchnum>=0)			
		orating[R_T] = cond_diff[matchnum], ++matched;

	//static vara cond_temp("0 - None,1 - Rain jacket,2 - Thin wetsuit,3 - Full wetsuit,4 - Thick wetsuit,5 - Drysuit");
	static varas mtempstrstr("15MM=4,14MM=4,13MM=4,12MM=4,11MM=4,10MM=4,0MM=0,1MM=1,2MM=1,3MM=2,4MM=2,5MM=3,6MM=3,7MM=4,8MM=4,9MM=4,XMM=5");
	if (match = mtempstrstr.strstr(rating[R_TEMP]+"MM"))
		orating[R_TEMP] = cond_temp[atoi(match)], ++matched;

	//static vara cond_stars("0 - Unknown,1 - Poor,2 - Ok,3 - Good,4 - Great,5 - Amazing");
	double num = CDATA::GetNum(rating[R_STARS]);
	if (num>=0 && num<6)
		orating[R_STARS] = cond_stars[(int)min(5, ceil(num))], ++matched;

	if ((num=CDATA::GetNum(rating[R_TIME]))>0)
		orating[R_TIME] = rating[R_TIME], ++matched;

	if ((num=CDATA::GetNum(rating[R_PEOPLE]))>0)
		orating[R_PEOPLE] = rating[R_PEOPLE], ++matched;

	orating[R_SUMMARY] = line;
	return matched;
}


//"0mm=0,1mm=1,2mm=1,3mm=2,4mm=2,5mm=3,6mm=3,7mm=4,8mm=4,7mm=4,10mm	Xmm"

double FACEBOOK_PostDate(const char *strdate, double year)
{
	const char *hrmin = "hms";
	  double pdate = -1;
	  vara datea(strdate, " ");
	  if (pdate<0 && datea.length()>=3)
			pdate = CheckDate( CDATA::GetNum(datea[1]), CDATA::GetMonth(datea[0]), CDATA::GetNum(datea[2], year));
	  if (pdate<0 && datea.length()>=1 && IsSimilar(datea[0], "Yesterday"))
		  pdate = RealCurrentDate-1;
	  if (pdate<0 && datea.length()>=2 && CDATA::GetMonth(datea[0])>0)
		  pdate = CheckDate( CDATA::GetNum(datea[1]), CDATA::GetMonth(datea[0]), CDATA::GetNum(CDate(RealCurrentDate)));
	  if (pdate<0 && datea.length()>=2 && CDATA::GetNum(datea[0])>=0)
		if (strchr(hrmin, datea[1][0]))
			pdate = RealCurrentDate;
	  if (pdate<0 && datea.length()>=2 && datea[0]=="Just")
		  pdate = RealCurrentDate;

	  return pdate;			
}


CSymList fbgrouplist, fbuserlist, fbgroupuserlist, fbnewfriendslist;

#define FBMAXTEXT 1024
enum { FBL_NAME = 0, FBL_REGION, FBL_FLAGS, FBL_GLINKS, FBL_GNAMES };

const char *fbheaders = "FB_ID,FB_SOURCE,FB_USER,FB_TEST,FB_TEXT,FB_NAME,FB_AKA,FB_REGION,FB_SUREDATE,FB_SURELOC,FB_SURECOND,FB_SUMMARY,FB_SIGN,FB_LINK,FB_ULINK,FB_GLINK,FB_LIKE, FB_DETREG";
enum { FB_SOURCE, FB_USER, FB_RWID, FB_TEXT, FB_NAME, FB_AKA, FB_REGION, FB_SUREDATE, FB_SURELOC, FB_SURECOND, FB_SUMMARY, FB_SIGN, FB_LINK, FB_ULINK, FB_GLINK, FB_LIKE, FB_MAX, FB_DATE, FB_PIC = ITEM_KML };

void MFACEBOOK_Like(DownloadFile &f, const char *link)
{
#ifndef DEBUGXXXX
  if (link && *link!=0)
	{
	Throttle(facebookticks, FACEBOOKTICKS);
	if (f.Download(link))
		Log(LOGERR, "ERROR: can't like with url %.500s", link);
	}
  else
	Log(LOGERR, "ERROR: invalid likeurl %s", link ? link : "(null)");
#endif
}


vars MFACEBOOK_GetAlbumText(DownloadFile &f, const char *aurl)
{
	static CSymList atext;

	int a;
	vars url = fburlstr(aurl, -1);
	if ((a=atext.Find(url))>=0)
		return atext[a].data;

	//ASSERT(!strstr(url, "CanyoningCultSlovenia/albums/1361894213853121"));
	//ASSERT(!strstr(url, "tiffanie.lin?lst=100013183368572:10710383:1490650849"));
	Throttle(facebookticks, FACEBOOKTICKS);
	if (f.Download(url, "testfb.html"))
		{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return "";
		}

	vars title = stripHTML(ExtractString(f.memory, "<title", ">", "</title"));
	vars text = stripHTML(ExtractString(strstr(f.memory, "</abbr"), "<div", ">", "</div"));
	vars likelink = fburlstr(ExtractLink(f.memory, ">Like</"));

	if (text.IsEmpty())
		FBLog(LOGWARN, "EMPTY ALBUMTXT for %s", url);
	FBLog(LOGINFO, "    ALBUMTXT '%s' [%s] : %.500s", title, url, text);	

	vars titletext = "<FBTITLE:"+title+">";
	titletext += "<FBURL:"+url+">";
	titletext += "<FBLIKE:"+likelink+">";
	titletext += title + " : " + text;
	atext.Add(CSym(url, titletext));
	
	return titletext;
}

#define MAXCDATE 30

typedef struct {
	double cdate;
	vars decipher;
	CSymList matchlist;
	vara summary; 
	int matchdate;
	int matchsummary;
	int matchlink;
	int matchname;
	const char *htmlline;
	void Reset(const char *_htmlline)
		{
		cdate = 0;
		decipher = "";
		matchlist.Empty();
		summary.RemoveAll();
		summary.SetSize(R_EXTENDED);
		matchdate = matchsummary = matchlink = matchname = 0;
		htmlline = _htmlline;
		};
} FBMatch;

int FBCheckDate(int matchdate, double pdate, double cdate, vars &err)
{
	if (PICMODE)
		return TRUE;
	if (!matchdate)
		{
		err =  "(nodate)";
		return FALSE;
		}
	if (cdate>pdate+1)
		{
		err = MkString("FUTURE DATE %s>%s", CDate(cdate), CDate(pdate));
		return FALSE;
		}
	if (cdate<pdate-MAXCDATE)
		{
		err = MkString("VERY OLD DATE %s<%s-%d", CDate(cdate), CDate(pdate), MAXCDATE);
		return FALSE;
		}
	return TRUE;
}

void FACEBOOK_LogUserGroup(const char *_userlink, const char *_fblink, const char *user, const char *fbname)
{
	  vars userlink = fburlstr(_userlink);
	  vars fblink = fburlstr(_fblink);
	  
	  int fgu = fbgroupuserlist.Find(userlink);
	  if (fgu<0)
		{
		CSym user(fburlstr(userlink), user); 
		user.SetStr(FBL_GLINKS, fblink);
		user.SetStr(FBL_GNAMES, fbname);
		fbgroupuserlist.Add(user);
		}
	  else
		{
		CSym &user = fbgroupuserlist[fgu];
		vara glinks(user.GetStr(FBL_GLINKS), ";");
		if (glinks.indexOf(fblink)<0)
			{
			glinks.push(fblink);
			user.SetStr(FBL_GLINKS, glinks.join(";"));
			}
		vara gnames(user.GetStr(FBL_GNAMES), ";");
		if (gnames.indexOf(fbname)<0)
			{
			gnames.push(fbname);
			user.SetStr(FBL_GNAMES, gnames.join(";"));
			}
		}
}


vars FACEBOOK_PreferredRegions(const char *user)
{
	// determine regions from groups belongs to
	vara rlist;
	int u = fbgroupuserlist.Find(fburlstr(user));
	if (u>=0)
		{
		vara list(fbgroupuserlist[u].GetStr(FBL_GLINKS), ";");
		for (int l=0; l<list.length(); ++l)
			{
			int g = fbgrouplist.Find(fburlstr(list[l]));
			if (g<0)
				{
				// complain if link is not in group list
				FBLog(LOGWARN, "Mismatched groupuser for groupuser '%s' grouplink '%s'", user, list[l]);
				continue;
				}
			vara grlist( fbgrouplist[g].GetStr(FBL_REGION), ";");
			for (int l=0; l<grlist.length(); ++l)
				if (rlist.indexOf(grlist[l])<0)
					rlist.push(grlist[l]);
			}
		}
	return rlist.join(";");
}


#define BR "<BR>"
// return maximum posts date
double MFACEBOOK_ProcessPage(DownloadFile &f, CSymList &fbsymlist, const char *url, const char *memory, const char *fblink, const char *fbname, const char *strregions, const char *strflags)
{
	// initialize
	static double year = 0;
	if (year==0)
		year = CDATA::GetNum(CDate(RealCurrentDate));

	static CSymList translist;
	if (translist.GetSize()==0)
		translist.Load(filename("facebooktrans"));

	const char* truncate[] = { ">More</", ">See Translation</", ">See More Photos</", ">Play Video</", ">Suggested Groups</", ">Friend Requests</", ">People You May Know</", NULL };

	double maxpdate = -1;
	vara list(memory, "<div><div><h3"); // main stories
	for (int i=1; i<list.length(); ++i)
	  {
	  // cleanup story
	  const char *memory = strchr(list[i],'>')+1;

	  // LIKE
	  vars likelink = fburlstr(ExtractLink(memory, ">Like</"));

	  // LINK
	  vars urllink = fburlstr2(ExtractLink(memory, ">Full Story</a"), -1);
	  if (urllink.IsEmpty())
		{
		// not a story
		vars text = stripHTML(ExtractString(strstr(memory, "</h3>"), "<div", ">", "<abbr"));
		if (text.IsEmpty())
			continue;
		Log(LOGWARN, "Invalid facebook permalink in %s '%.256s'", url, text);
		continue;
		}


	  // USER
	  vars usertag = ExtractString(memory, "<strong", ">", "</strong");
	  vars user = stripHTML(usertag);
	  vars userlink = fburlstr2(ExtractString(usertag, "href=", "\"", "\""), TRUE);
	  if (user.IsEmpty())
		{
		// no user
		Log(LOGWARN, "Invalid facebook user in %s '%.256s'", url, stripHTML(memory));
		continue;
		}
	  if (userlink.IsEmpty())
		  userlink = "#";

	  // DATE
	  vars postdate = ExtractString(memory, "<abbr", ">", "</abbr");
	  double pdate = FACEBOOK_PostDate(postdate, year);
	  maxpdate = max(pdate, maxpdate);
	  if (pdate<=0)
		{
		Log(LOGWARN, "Invalid facebook utime '%s' in %.256s", postdate, stripHTML(memory));
		continue;
		}

	  // keep track of users posting on groups
	  if (FCHK(strflags, F_GROUP))
		  FACEBOOK_LogUserGroup(userlink, fblink, user, fbname);

	  // capture first pic
	  vars pic;
	  if (PICMODE)
		{
		if (strstr(memory, "<img"))
			if (!strstr(pic = fburlstr(ExtractLink(memory, "<img")), "/photo"))
				pic = "";

		// skip if pic is empty
		if (pic.IsEmpty())
			continue;

		// if not filtering and not ropewiki link, filter users
		if (*PICMODE==0 && !strstr(memory, "ropewiki.com/"))
			{
			// skip if user did not authorize pics and not mentioning ropewiki
			int f = fbuserlist.Find(GetToken(fburlstr(userlink), 0, '?'));
			if (f<0 || !strchr(fbuserlist[f].GetStr(FBL_FLAGS), 'P'))
				continue;
			}
		}

	  double cdate = pdate;
	  vara list2(list[i], "<div><h3"); // secondary stories

	  for (int i2=0; i2<list2.length(); ++i2)
		{
		vars h3 = ExtractString(list2[i2], ">", "", "</h3");
		vars text = ExtractString(list2[i2], "</h3", ">", "<abbr");

		// cut it short
		for (int t=0; truncate[t]; ++t)
			text = text.split(truncate[t]).first();

		 // avoid bad posts
		  {
		  int skip = FALSE;
		  if (strstr(h3, " updated "))
			  if (strstr(h3, "cover photo") || strstr(h3, "profile picture"))
			  skip = TRUE;
		  if (strstr(h3, "shared a memory"))
			  skip = TRUE;
		  if (strstr(h3, ">Suggested Groups</"))
			  skip = TRUE;
		  if (strstr(h3, "/caudal.info/"))
			  skip = TRUE;

		  if (skip)
			{
			FBLog(LOGINFO, FBDEBUG"Skipping h3:'%s' text:%.500s url:%s", stripHTML(h3), stripHTML(text), urllink);
			continue;
			}
		  }

		// multiple lines
		text.Replace("<br","<p");
		text.Replace("</br","<p");
		text.Replace("</p","<p");
		text.Replace("<p", "(LINEBREAK)<p");
		//int hash = text;

		// avoid articles on accidents
		//while (!ExtractStringDel(text, "<table", "", "</table>").IsEmpty());
		
		// avoid people name's
		//while (!ExtractStringDel(text, "href=\"/profile.php?id=", ">", "</a>").IsEmpty());

		// html lines 
		vara htmllines(text, "(LINEBREAK)");

		// post text
		vars posttext;
		for (int i=0; i<htmllines.length(); ++i)
			{
			vars line = stripHTML(htmllines[i].replace(">#<", ">" HASHTAG "<")).Trim();
			if (line.IsEmpty() || IsSimilar(line, "http"))
				continue;
			// add '. ' separator
			if (!posttext.IsEmpty())
				if (posttext[strlen(posttext)-1]!='.')
					posttext += ".";
			posttext += " " BR + line;
			}

		// add albums titles
		const char *albumsep[] = { " album: <a", "<a", NULL };
		for (int a=0; albumsep[a]!=NULL; ++a)
			{
			vara albums(h3, albumsep[a]);
			for (int l=1; l<albums.length(); ++l)
				{
				vars albumlink = ExtractHREF(albums[l]);
				vars album = stripHTML(ExtractString(albums[l], ">", "", "</a"));
				if (album.IsEmpty() || albumlink.IsEmpty())
					{
					Log(LOGWARN, "empty album title from url %s", urllink);
					continue;
					}

				if (albumsep[a][0]=='<')
					if (!strstr(albumlink, "/albums/"))
						continue; // skip nonalbums for general case

				// insert album title
				//if (htmllines.indexOf(album)<0)
				//	htmllines.InsertAt(1, album);

				// insert album title : description
				vars txt = MFACEBOOK_GetAlbumText(f, albumlink);
				if (htmllines.indexOf(txt)<0)
					htmllines.InsertAt(0, txt);
				}
			}

		// process text
		vara olines;
		vara lines = htmllines;
		for (int l=0; l<lines.length(); ++l)
		  {
		  // transform text
		  vars &line = lines[l];

		  // eliminate profiles
		  vars aref;
		  while (!ExtractStringDel(line, "href=\"/hashtag", "", ">", FALSE, -1).IsEmpty());
		  while (!ExtractStringDel(line, "href=\"/", "", "</a>", FALSE, -1).IsEmpty());

		  line = stripAccents2(stripHTML(line));
		  if (IsSimilar(line,"http"))
			  line = "";
		  if (line.IsEmpty())
			  continue;

		  // original lines
		  if (l>0) 
			  olines.push(line);

		  // translate line
		  for (int t=0; t<translist.GetSize(); ++t)
			line.Replace(translist[t].id, translist[t].data);
		  }

		// try to catch global references
		vars alllines = lines.join(" ").Trim();
		if (alllines.IsEmpty())
			continue;
		vars allhtmllines = htmllines.join(" ");

		// parse lines
		int nlist = 0;
		#define MAXFBMATCH 10
		FBMatch llist[MAXFBMATCH+1];

		FBMatch g; g.Reset(allhtmllines);
		g.matchdate = FBDATE.FACEBOOK_MatchDate(pdate, alllines, g.cdate, g.decipher);
		vars err;
		if (!FBCheckDate(g.matchdate, pdate, g.cdate, err))
			{
			FBLog(LOGINFO, FBDEBUG"L#* %s : '%s' %s <= '%.500s'=>", err, user, g.decipher, alllines);
			continue;
			}
		g.matchsummary = FACEBOOK_MatchSummary(alllines, g.summary);
		g.matchlink = FACEBOOK_MatchLink(allhtmllines, g.matchlist);

		// prepare to run match
		int matchperfect = 0, matchsome = 0;
		vars matchregions = FBMATCH.FACEBOOK_MatchRegion(alllines, strregions);

		// match text by lines
		for (int nl=0; nl<lines.length(); ++nl)
		 if (!lines[nl].IsEmpty())
		  {
		  vars line = " "+lines[nl]+" ";
		  const char *htmlline = htmllines[nl];

		  // try to catch local references
		  FBMatch &l = llist[nlist]; l.Reset(htmlline);
		  l.matchdate = FBDATE.FACEBOOK_MatchDate(pdate, line, l.cdate, l.decipher);
		  if (line.Trim().IsEmpty())
			  continue;

		  l.matchsummary = FACEBOOK_MatchSummary(line, l.summary);
		  l.matchlink = FACEBOOK_MatchLink(htmlline, l.matchlist);
		  if (l.matchlink)
			l.matchname = TRUE;
		  else
			l.matchname = FBMATCH.FACEBOOK_Match(line, l.matchlist, matchregions, strflags);

		  //ASSERT(!strstr(line, "Arteta"));
		  if (!l.matchname)
			  {
				FBLog(LOGINFO, FBDEBUG"L#%d NO GOOD NAME by '%s' <= '%.500s'=>", nl, user, line);
				//ASSERT(!strstr(linestr,"Avello"));
				//ASSERT(!strstr(linestr, "Lucas"));
				//ASSERT(!strstr(linestr, "Mortix"));
				//ASSERT(!strstr(linestr, "Bolulla"));
				//ASSERT(!strstr(linestr, "Rubio"));
			  continue;
			  }

		  // detect matches
		  ++matchsome;
		  if (l.matchdate && l.matchname)
			  ++matchperfect;

		  // match found
		  FBLog(LOGINFO, FBDEBUG"L#%d %dMATCH! [#%dPERF #%dSOME] '%s'... <= '%.500s'=>", nl, l.matchlist.GetSize(), matchperfect, matchsome, l.matchlist[0].GetStr(ITEM_DESC), line);
		  if (nlist<MAXFBMATCH)
			++nlist;
		  }
	
	//
	// PROCESS MATCHES: try to make sense of the mess...
	//
	//  Name  Date   GDate  GName
	//    X    X                  => PERFECT!, GO
	//    X            X          => GO BEST MATCH
	//         X             X    => Skip

	int used = 0;
	if (matchsome)
	  for (int n=0; n<nlist; ++n)
		{
		FBMatch &m = llist[n];
		FBMatch *mm = NULL, *md = NULL, *ms = NULL;
		if (matchperfect && m.matchdate && m.matchname)
		  {
		  mm = md = &m;
		  ms = m.matchsummary ? &m : &g;
		  }
		if (!matchperfect && matchsome && m.matchname)
		  {
		  // process just one, the first to match
		  mm = &m; md = &g;
		  ms = m.matchsummary ? &m : &g;
		  nlist = 0; // force break
		  }

		// Final error check
		if (!mm || !md || !ms)
			continue;

		vars err;
		if (!FBCheckDate(md->matchdate, pdate, md->cdate, err))
			{
			FBLog(LOGINFO, FBDEBUG"L#? %s : '%s' %s <= '%.500s'=>", err, user, md->decipher, stripHTML(md->htmlline));
			continue;
			}

		// detect shaky business and play it safe
		if (used>0)
			{
			if (mm->matchname<0)
				{
				FBLog(LOGWARN, FBDEBUG"L#? %s : '%s' %s <= '%.500s'=>", "SHAKY#2NAME", user, mm->decipher, stripHTML(mm->htmlline));
				continue;
				}
			if (md->matchdate<g.matchdate)
				{
				FBLog(LOGWARN, FBDEBUG"L#? %s : '%s' %s <= '%.500s'=>", "SHAKY#2DATE", user, md->decipher, stripHTML(md->htmlline));
				continue;
				}
			}

		// OUTPUT FBSYMs
		++used;
		for (int m=0; m<mm->matchlist.GetSize(); ++m)
			  {
			  // fb mapping
			  vars date = CDate(md->cdate);
			  CSym &msym = mm->matchlist[m];
			  vars title = msym.GetStr(ITEM_DESC);
			  

			  CSym sym("Conditions:"+title+"-"+user.replace(" ","")+"@Facebook-"+date, title);
			  sym.SetStr(FB_DATE, "\'"+date);
			  sym.SetStr(FB_NAME, title);
			  sym.SetStr(FB_AKA, msym.GetStr(ITEM_AKA));
			  sym.SetStr(FB_RWID, msym.id);
			  sym.SetStr(FB_REGION, invertregion(msym.GetStr(ITEM_REGION)).replace(";","."));
			  sym.SetStr(FB_MAX, matchregions);

			  // Like it!
			  vara likelist;
			  likelist.push(likelink);
			  vars fblike = fburlstr(ExtractString(mm->htmlline, "<FBLIKE:", "", ">"));
			  if (!fblike.IsEmpty())
				  likelist.push(fblike);

			  vars fburl = fburlstr2(ExtractString(mm->htmlline, "<FBURL:", "", ">"));
			  if (fburl.IsEmpty())
				  fburl = urllink;
			  vars fbtitle = ExtractString(mm->htmlline, "<FBTITLE:", "", ">");
			  if (fbtitle.IsEmpty())
				 fbtitle = strstr(fburl,"/posts/") ? user+"'s timeline" : fbname;
			  vars fbtext = posttext;
			  if (fbtext.IsEmpty())
				  fbtext = stripHTML(mm->htmlline);
			  fbtext = fbtext.replace("\"","'");
			  if (fbtext.GetLength()>FBMAXTEXT)
				fbtext = fbtext.Mid(0, FBMAXTEXT)+"...";

			  vars fbuser = user.replace(";"," ");
			  fbtitle = fbtitle.replace(";"," ");

			  sym.SetStr(FB_TEXT, fbtext.replace(";", FBCOMMA));
			  sym.SetStr(FB_SIGN, "["+fburl+" "+fbtitle+"]");
			  
			  sym.SetStr(FB_USER, fbuser);
			  sym.SetStr(FB_SOURCE, fbtitle);
			  sym.SetStr(FB_LINK, fburl);
			  sym.SetStr(FB_ULINK, userlink);
			  sym.SetStr(FB_LIKE, likelist.join(" "));

			  if (!fburlcheck(userlink) || !fburlcheck(fburl))
				{
				Log(LOGERR, "ERROR: bad POST / USER url in fbsym %s by %s %s", fbtitle, user, date);
				Log(fburl);
				Log(userlink);
				continue;
				}
			  /*
			  if (PICMODE)
				{
				// skip matched with banner pic
				if (strstr(matchlist[m].GetStr(ITEM_INFO), "vxp"))
					continue;
				// set pic info
				CSym sym(matchlist[m].id, matchlist[m].GetStr(ITEM_DESC));
				sym.SetStr(ITEM_KML, pic);
				sym.SetStr(ITEM_LAT, user);
				sym.SetStr(ITEM_LNG, userlink);
				sym.SetStr(ITEM_CONDDATE, CDate(cdate));
				vars cnt = matchlist.GetSize()>1 ? MkString("[#%d/%d]", m+1, matchlist.GetSize()) : "";
				ASSERT(!olines.join("").Trim().IsEmpty());
				sym.SetStr(ITEM_MATCH, cnt + " " + olines.join(" "));
				sym.SetStr(ITEM_NEWMATCH, matchlist[m].GetStr(ITEM_AKA));
				sym.SetStr(ITEM_REGION, matchlist[m].GetStr(ITEM_REGION));
				Log(LOGINFO, "FBC PIC : '%s' by '%s' = '%.256s' url:%s", title, user, line, urllink);
				fbsymlist.Add(sym);
				continue;
				}
			  */

			  if (md->matchdate>0)
				sym.SetStr(FB_SUREDATE, "X");
			  if (mm->matchlink>0 || mm->matchname>0)
				sym.SetStr(FB_SURELOC, "X");
			  if (ms->matchsummary && !mm->summary[R_W].IsEmpty())
				sym.SetStr(FB_SURECOND, "X");

			  vars sum = GetFBSummary(ms->summary);
			  FBLog(LOGINFO, FBDEBUG"FBC DATE [%s] --- SUM [%s] <='%s'", md->decipher, sum, ms->summary[R_SUMMARY]);
			  Log(LOGINFO, ">>>"+(used>1 ? MkString("#%d", used): "")+" SYM(%d):\"%s\" <='%.500s'=> DATE(%d):%s SUM(%d):[%s] REG:[%s] by '%s@%s' url:%s", mm->matchname, title, fbtext, md->matchdate, date, ms->matchsummary, sum, matchregions, user, fbtitle, fburl);


			  // summary
			  ms->summary[R_SUMMARY] = ""; 
			  if (!ms->summary[R_W].IsEmpty())
				sym.SetStr(FB_SUMMARY, ms->summary.join(";"));

			  fbsymlist.Add(sym);

			  //ASSERT(!strstr(title, "Eaton Canyon (Upper)"));
			  //ASSERT(!strstr(title, "Heaps"));
			  //ASSERT(!strstr(title, "New Day Canyon"));
			  //ASSERT(!strstr(title, "One "));
			  //ASSERT(!strstr(title, "San Julian"));
			  //ASSERT(!strstr(rwname, "Simon (Upper)"));
			  //ASSERT(!strstr(lines.join(" "), "Rio Bianco"));			  	
			  }
		// loop nlist
		}

	// check for unused matches
	if (!used)
		{
		ASSERT(!matchsome);
		Log(LOGWARN, "??? FBUNUSED DATE %s REG:[%s]<='%.500s' url:%s", g.decipher, matchregions, posttext, urllink);

		CSym sym;
		vars text = posttext;
		if (posttext.IsEmpty())
			text = stripHTML(htmllines.join("; "));
		text.Replace("\"","'");
		if (text.GetLength()>FBMAXTEXT) 
			text.Truncate(FBMAXTEXT);
		sym.SetStr(FB_TEXT, text);
		sym.SetStr(FB_NAME, "UNUSED");
		sym.SetStr(FB_REGION, matchregions);
		sym.SetStr(FB_USER, user);
		sym.SetStr(FB_LINK, urllink);
		sym.SetStr(FB_SOURCE, fbname);
		fbsymlist.Add(sym);
#ifdef DEBUGXXX
		CSymList matchlist;
		FBMATCH.FACEBOOK_Match(posttext, matchlist, strregions, strflags);
		const char *warn[] = { "Eaton", "Devils Jumpoff", "Monkeyface", "Shaver Cut", "fall creek", "salmon creek", "Vivian", "Rubio", "Seven Teacups", "7 Teacups", NULL };
		for (int i=0; warn[i]!=NULL; ++i)
			if (strstr(posttext, stripAccents(warn[i])))
				{
				ASSERT(!"FBUNUSED DATE WARNING");
				CSymList matchlist;
				FBMATCH.FACEBOOK_Match(posttext, matchlist, strregions, strflags);
				}
#endif
		}
	 }
	}
	 return maxpdate;
}


int MFACEBOOK_DownloadConditionsDate(DownloadFile &f, CSymList &fbsymlist, const char *ubase, const char *name, const char *strregions = "", const char *strflags = "") 
{
	//ASSERT(FB_MAX<FB_DATE);
	//ASSERT(ITEM_AKA>=FB_MAX);

	if (!IsSimilar(ubase, "http"))
		return FALSE;

	CString url = fburlstr(ubase);

	if (PICMODE && *PICMODE)
		if (!strstr(name, PICMODE) && !strstr(ubase, PICMODE))
			return FALSE;
	

	// number of pages	
	int umax = 1000;
	double postdate = 0;
	double minpostdate = 0;

	if (INVESTIGATE>0)
		{
		// num pages
		umax = INVESTIGATE;
		}
	else
		{
		// num days
		minpostdate = RealCurrentDate + INVESTIGATE;
		}


int retry = 0;
int morelistn = 0;
vara morelist;
vars oldurl = url;
int u;
for (u=0; u<umax && retry<3 && !url.IsEmpty(); ++u)
	{
	// download list of recent additions
	Throttle(facebookticks, FACEBOOKTICKS);
	if (f.Download(url, "testfb.html"))
		{
		Log(LOGERR, "ERROR: can't download url %.128s, REPEAT!", url);
		Sleep(5000);
		url = oldurl;
		++retry;
		continue;
		}

	retry = 0;
	oldurl = url;
	vars title = stripHTML(ExtractString(f.memory, "<title", ">", "</title"));
	Log(LOGINFO, "--------------- #%d %s<%s %s REG:%s FL:%s [%s]", u, CDate(postdate), CDate(minpostdate), title, strregions, strflags, url);
	
	vars more, seemore;
	const char *seemores[] = { ">See More Posts</", ">See More Stories</", ">Show more</", NULL };
	for (int mode=0; seemores[mode]!=NULL && more.IsEmpty(); ++mode)
		more = fburlstr(ExtractLink(f.memory, seemore = seemores[mode]));
	if (more.IsEmpty())
		{
		if (morelist.length()==0)
			{
			vara list(f.memory, "<div class=\"i\"");
			for (int i=1; i<list.length(); ++i)
				morelist.push( fburlstr(ExtractHREF(list[i])) );
			}
		if (morelistn<morelist.length())
			more = morelist[morelistn++];
		}
	
	vars memory = ExtractString(f.memory, "<body", "", seemore);
	postdate = MFACEBOOK_ProcessPage(f, fbsymlist, url, memory, ubase, title, strregions, strflags);
	if (postdate>0 && postdate < minpostdate)
		break;

	url = more;
	}

	if (retry>=3)
		Log(LOGERR, "====================== DOWNLOAD ERROR FOR %s PG#%d ====================", name, u);

	return TRUE;
}



int FACEBOOK_DownloadList(DownloadFile &f, CSymList &symlist, const char *ourl, const char *match, const char *start, const char *end, const char *region = NULL, const char *flags = NULL)
{
	// check proper urls
	for (int i=0; i<symlist.GetSize(); ++i)
		{
		CSym &sym = symlist[i];
		if (IsSimilar(sym.id, "http"))
			sym.id = fburlstr(sym.id);
		}

	int n = 0;
	vars url = ourl;
	while (!url.IsEmpty())
	{
		if (f.Download(url))
			{
			Log(LOGERR, "Cound not download friends url %s", url);
			return 0;
			}
		vars memory = ExtractString(f.memory, start, "", end);

		url = ExtractLink(f.memory, ">See More Friends</");
		if (!url.IsEmpty())
			{
			memory.Truncate(memory.Find(url));
			url = fburlstr(url);
			}
		
		vara friends(memory, match); 
		for (int i=1; i<friends.length(); ++i)
			{
			const char *line = friends[i];
			vars name = stripHTML(ExtractString(line, "href=", ">", "<"));
			vars link = fburlstr(ExtractString(line, "href=", "\"", "\""), TRUE);
			if (link.IsEmpty() || name.IsEmpty())
				continue;
			if (strstr(link, "/privacy"))
				continue;
			++n;
			if (!fburlcheck(link))
				{
				Log(LOGERR, "Invalid profile url for user '%s' url:%s", name, link);
				continue;
				}
			int found = symlist.Find(link);
			if (found<0)
				{
				CSym sym(link, name);
				sym.SetStr(FBL_REGION, region);
				sym.SetStr(FBL_FLAGS, flags);
				symlist.Add(sym);
				}
			else
				{
				symlist[found].SetStr(0, name);
				}
			}
	}
	return n;
}

static int cmpgeoregion( const void *arg1, const void *arg2)
	{
		const char *s1 = *((const char **)arg1);
		const char *s2 = *((const char **)arg2);
		const char *a1 = strrchr(s1, ':');
		const char *a2 = strrchr(s2, ':');
		/*
		int ok = okgeoregion(s1)-okgeoregion(s2);
		if (ok!=0)
			return ok;
		*/
		int a = -strcmp(a1, a2);
		if (a!=0)
			return a;
		/*
		int s = -strcmp(s1, s2);
		if (s!=0)
			return s;
		*/
		return 0;
	}

int invcmpconddate( const void *arg1, const void *arg2 )
{
	return ((CSym**)arg1)[0]->GetStr(ITEM_CONDDATE).Compare(((CSym**)arg2)[0]->GetStr(ITEM_CONDDATE));
}


int FACEBOOK_TestLine(int MODE, const char *_line, const char *result = NULL)
{
		const char *strflags = MODE==0 ? "+G" : "";
		const char *strregions = MODE==0? "Europe" : "";

		static CSymList translist;
		if (translist.GetSize()==0)
			translist.Load(filename("facebooktrans"));

		vars line = stripAccents2(_line);
		line.Replace(BR,".");
		// translate line
		for (int t=0; t<translist.GetSize(); ++t)
			line.Replace(translist[t].id, translist[t].data);

		CSymList matchlist;
		vars decipher;
		vara useddate;
		double cdate;
		line.Replace(FBCOMMA, ";");
		vara summary; summary.SetSize(R_EXTENDED);
		Log(LOGINFO, "TESTIN:%s", line);
		vars matchregions = FBMATCH.FACEBOOK_MatchRegion(line, strregions);
		int matchdate = FBDATE.FACEBOOK_MatchDate(RealCurrentDate, line, cdate, decipher);
		int matchsum = FACEBOOK_MatchSummary(line, summary);
		int matchname = FBMATCH.FACEBOOK_Match(line, matchlist, matchregions, strflags);
		vars alert = "";
		if (result)
			{
				if (matchlist.GetSize()>0)
					{
					if (matchlist[0].GetStr(ITEM_DESC)!=result)
						alert = MkString("%s", result);
					}
				else
					{
					if (*result!=0)
						alert = MkString("(empty)", result);
					}
			}
		Log(alert.IsEmpty() ? LOGINFO : LOGERR, ">>> SYM(%d):'%s' DATE(%d):%s SUM(%d):%s REG:[%s]", matchname, matchname ? matchlist[0].GetStr(ITEM_DESC) : "", matchdate, matchdate ? decipher : "", matchsum, matchsum ? GetFBSummary(summary) : "", matchregions);
		matchname = matchname+1;
		return TRUE;
}

#define FBPOWERUSERS "facebookpulist"
#define FBGROUPUSERS "facebookgulist"
#define FBGROUPS "facebookglist"
#define FBUSERS "facebookulist"
#define FBFRIENDS "facebookflist"
#define FBNEWFRIENDS "facebooknflist"

int FACEBOOK_Download(CSymList &fbsymlist, const char *FBNAME)
{
	CSymList symlist;
	vars ubase = "https://m.facebook.com";


	DownloadFile f;
	//COMMENT REMOVED FROM PUBLIC
	FACEBOOK_Login(f);	

#ifdef DEBUGXXXX
	{
	CSymList tmp;
	tmp.Load(filename(FBGROUPUSERS));
	for (int i=0; i<tmp.GetSize(); ++i)
		{
		CSym &sym = tmp[i];
		vars name = sym.GetStr(FBL_NAME);
		vara gllist(sym.GetStr(FBL_GLINKS),";");
		vara gnlist(sym.GetStr(FBL_GNAMES),";");
		for (int g=0; g<gllist.length(); ++g)
			FACEBOOK_LogUserGroup(sym.id, gllist[g], name, gnlist[g]);
		}
	fbgroupuserlist.Save(filename(FBGROUPUSERS));
	}
#endif

#ifdef DEBUGXXXX
	for (int u=0; u<fbgroupuserlist.GetSize(); ++u)
		{
		CSym &usym = fbgroupuserlist[u];
		vara list(usym.GetStr(FBL_GLIST),";");
		usym.SetStr(FBL_GLIST+1, list.join(";"));
		if (list.GetSize()==0)
			continue;
		for (int i=0; i<fbgrouplist.GetSize(); ++i)
			{
				CSym &sym = fbgrouplist[i];
				if (sym.data.Trim(", ").IsEmpty())
					continue;
				int f = list.indexOf(sym.GetStr(FBL_NAME));
				if (f>=0)
					list[f] = sym.id;
			}
		usym.SetStr(FBL_GLIST, list.join(";"));
		}
	fbgroupuserlist.Save(filename(FBGROUPUSERS));
#endif


	// group & user lists
	fbuserlist.Load(filename(FBUSERS));
	fbgrouplist.Load(filename(FBGROUPS));
	fbgroupuserlist.Load(filename(FBGROUPUSERS));

	// groups: add facebook groups, by default disabled
	int ng =FACEBOOK_DownloadList(f, fbgrouplist, burl(ubase, "groups/?seemore"), "<li",">Groups You", "</ul", "", "X");
	fbgrouplist.Save(filename(FBGROUPS));

	// users: add facebook friends, implicit allows use of canyon pics
	CSymList fblist;
	int nu = FACEBOOK_DownloadList(f, fblist, burl(ubase, "profile.php?v=friends"), "<table", "<h3", "\"footer\"", "", "P");
	fbuserlist.AddUnique(fblist);

	// keep track of old/new friends
	CSymList fbnewfriendslist, fboldfriendslist;
	fbnewfriendslist.Load(filename(FBNEWFRIENDS));
	fboldfriendslist.Load(filename(FBFRIENDS));
	fboldfriendslist.Sort();
	fblist.Save(filename(FBFRIENDS)); // respect people that unfriended RC
	for (int i=0; i<fblist.GetSize(); ++i)
		if (fboldfriendslist.Find(fblist[i].id)<0)
			fbnewfriendslist.AddUnique(fblist[i]);
	fbnewfriendslist.Save(filename(FBNEWFRIENDS));

	if (MODE==3)
		{
		// new friends only
		fbuserlist = fbnewfriendslist;
		}

	if (MODE==2)
		{
		// posting users only
		CSymList tmp;
		tmp.Load(filename(FBPOWERUSERS));
		tmp.Sort();

		for (int i=0; i<fbuserlist.GetSize(); ++i)
			if (tmp.Find(fbuserlist[i].id)<0)
				fbuserlist.Delete(i--);
		}

	Log(LOGINFO, "FACEBOOK processing %d Users [RC %d Groups %d Friends]", fbuserlist.GetSize(), ng, nu);

	// login as unfriendly user
	//https://www.facebook.com/luke.tester.144#
	//FACEBOOK_Login(f, "Testing", UNFRIENDLY_USERNAME, UNFRIENDLY_PASSWORD);	

		// test
	if (FBNAME && strstr(FBNAME, ".log"))
		{
		//ASSERT(!"TESTING FBTEST.CSV");

		Log(LOGINFO, "---------------------TEST START---------------------------");
		// Test MATCHNAME
		CLineList list;
		list.Load(FBNAME, FALSE);

		for (int i=0; i<list.GetSize(); ++i)
			if (IsSimilar(list[i],"*["))
				FACEBOOK_TestLine(MODE, ExtractString(list[i], "<=", "'", "'=>"));

		Log(LOGINFO, "---------------------TEST END---------------------------");
		return TRUE;
		}

	if (FBNAME && IsSimilar(FBNAME, "http"))
		{
		//vars url = "https://m.facebook.com/groups/583118321723884?bacr=1465249813%3A1048425145193197%3A%3A7&multi_permalink_ids&refid=18";
		//vars url = "https://m.facebook.com/groups/58689255788?bacr=1465080305%3A10154230987145789%3A%3A7&multi_permalink_ids&refid=18";
		//ASSERT(!"TESTING FBTEST.CSV");
		const char *strflags = F_MATCHMORE;
		const char *strregions = "";
		// Test page
		CSymList tmplist;
		Log(LOGINFO, "---------------------TEST START---------------------------");
		INVESTIGATE = 1;
		MFACEBOOK_DownloadConditionsDate(f, tmplist, FBNAME, "TEST", strregions, strflags);
		Log(LOGINFO, "---------------------TEST END---------------------------");
		return TRUE;
		}


	// GROUPS
	if (MODE<0 || MODE==0)
	  for (int i=0; i<fbgrouplist.GetSize(); ++i)
		{
		CSym &sym = fbgrouplist[i];
		if (IsSimilar(sym.id, "http"))
			if (FBNAME==NULL || strstr(sym.GetStr(FBL_NAME), FBNAME)) 
				if (FBNAME!=NULL || !strstr(sym.GetStr(FBL_FLAGS),F_NOMATCH)) 
					MFACEBOOK_DownloadConditionsDate(f, fbsymlist, sym.id, sym.GetStr(FBL_NAME), sym.GetStr(FBL_REGION), sym.GetStr(FBL_FLAGS)+F_GROUP);
		fbgroupuserlist.Save(filename(FBGROUPUSERS));
		}

	// USERS
	if (MODE<0 || MODE>=1)
	  for (int i=0; i<fbuserlist.GetSize(); ++i)
		{
		CSym &sym = fbuserlist[i];
		if (IsSimilar(sym.id, "http"))
			if (FBNAME==NULL || strstr(sym.GetStr(FBL_NAME), FBNAME)) 
				if (FBNAME!=NULL || !strstr(sym.GetStr(FBL_FLAGS),F_NOMATCH)) 
					{					
					vars regions = sym.GetStr(FBL_REGION);
					if (regions.IsEmpty())
						regions = FACEBOOK_PreferredRegions(sym.id);
					MFACEBOOK_DownloadConditionsDate(f, fbsymlist, sym.id+"?v=timeline", sym.GetStr(FBL_NAME), regions, sym.GetStr(FBL_FLAGS));
					}
		}

	if (MODE==3)
		DeleteFile(filename(FBNEWFRIENDS));

	return TRUE;
}


int cmpfbsym(CSym **arg1, CSym **arg2)
{
	register int cmp = -strcmp((*arg1)->GetStr(FB_DATE), (*arg2)->GetStr(FB_DATE));
	if (!cmp)
		cmp = strcmp((*arg1)->id, (*arg2)->id);
	if (!cmp)
		cmp = strcmp((*arg1)->GetStr(FB_LINK), (*arg2)->GetStr(FB_LINK));
	return cmp;
}

int FBSymCmp(CSym &sym, CSym &sym2)
{
	if (vars(sym.GetStr(FB_LINK)) != sym2.GetStr(FB_LINK))
		return FALSE;
	if (sym.GetStr(FB_NAME) != sym2.GetStr(FB_NAME))
		return FALSE;
	if (sym.GetStr(FB_SOURCE) != sym2.GetStr(FB_SOURCE))
		return FALSE;
	return TRUE;
}

int DownloadFacebookConditions(int mode, const char *file, const char *fbname)
{
	if (!strstr(file, ".csv"))
		{
		Log(LOGERR, "Invalid csv file '%s'", file);
		return FALSE;
		}

	if (!LoadBetaList(titlebetalist, TRUE, TRUE)) 
		return FALSE;

		if (fbname && IsSimilar(fbname,"TEST"))
			{
			// TEST
			CSymList list;
			list.Load(file);

			Log(LOGINFO, "---------------------TEST START---------------------------");
			// Test MATCHNAME

			for (int i=0; i<list.GetSize(); ++i)
				{
				CSym &sym = list[i];
				vars id = sym.GetStr(FB_RWID);
				if (IsSimilar(id, RWID) || id.IsEmpty())
					continue;
				vars line(sym.GetStr(FB_TEXT));
				line.replace(FBCOMMA, ",");
				FACEBOOK_TestLine( strstr(sym.GetStr(FB_SOURCE), "timeline") ? 1 : 0, line, sym.GetStr(FB_NAME) );
				sym.id = "";
				}
			Log(LOGINFO, "---------------------TEST END---------------------------");
			return TRUE;
			}

		if (fbname && strstr(fbname,".csv"))
			{
			vars file2 = GetFileName(file) + "." + GetFileName(fbname) + ".csv";
			Log(LOGINFO, "COMPARING %s <<>> %s => %s ", file, fbname, file2);

			CSymList list, list2;
			list.Load(file);
			list2.Load(fbname);

			// set =
			#define EQ " "
			for (int i=0; i<list.GetSize(); ++i)
				{
				int eq = -1;
				for (int i2=0; i2<list2.GetSize() && eq<0; ++i2)
					{
					if (FBSymCmp(list[i], list2[i2]))
						if (list2[i2].GetStr(FB_RWID)!=EQ)
							eq = i2;
					}
				if (eq<0)
					list[i].SetStr(FB_RWID, "<<<<");
				else
					{
					list[i].SetStr(FB_RWID, EQ);
					list2[eq].SetStr(FB_RWID, EQ);
					}
				}
			for (int i2=0; i2<list2.GetSize(); ++i2)
				if (list2[i2].GetStr(FB_RWID)!=EQ)
					{
					list2[i2].SetStr(FB_RWID, ">>>>");
					list.Add(list2[i2]);
					}
			/*
			for (int i=0; i<list.GetSize(); ++i)
				if (list[i].GetStr(FB_RWID)==EQ)
					list[i].SetStr(FB_RWID,"");
			*/
			list.Sort((qfunc *)cmpfbsym);
			list.header = fbheaders;
			list.Save(file2);
			return TRUE;
			}

		// load existing file
		CSymList fbsymlist;
		fbsymlist.Load(file);
		
		FACEBOOK_Download(fbsymlist, fbname);

		// sort and remove duplicates
		for (int i=0; i<fbsymlist.GetSize(); ++i)
			for (int j=i+1; j<fbsymlist.GetSize(); ++j)
				if (fbsymlist[i].id==fbsymlist[j].id )
					if (fbsymlist[i].GetStr(FB_LINK)==fbsymlist[j].GetStr(FB_LINK))
						fbsymlist.Delete(j--);	

		// save file
		fbsymlist.Sort((qfunc *)cmpfbsym);
		fbsymlist.header = fbheaders;
		fbsymlist.Save(file);

	#ifdef DEBUG
		//if (fbname)
		//DeleteFile(file+".log");
		//MoveFile("rw.log", file+".log");
		//system(file+".log");
		//system(file+".csv");
	#endif 
		return TRUE;
}





int UploadFacebookConditions(int mode, const char *fbname)
{
		if (!LoadBetaList(titlebetalist, TRUE, TRUE)) 
			return FALSE;

		CSymList fbsymlist;

		DownloadFile f;
		// COMMENT REMOVED FROM PUBLIC
		FACEBOOK_Login(f);	

		// load file
		fbsymlist.Load(fbname, FALSE);
		int count = 0;
		for (int i=0; i<fbsymlist.GetSize(); ++i)
			if (IsSimilar(fbsymlist[i].id, CONDITIONS))
				++count;

		CSymList fbuserposting;
		fbuserposting.Load(filename(FBPOWERUSERS));

		Log(LOGINFO, "%s %d Facebook condition reports...", MODE>=0 ? "Testing " : "Updating ", count);
		Sleep(3000);
		
		// UPLOAD (Consolidates while uploading)
		for (int i=0; i<fbsymlist.GetSize(); ++i)
			{
			CSym &fbsym = fbsymlist[i];
			if (!IsSimilar(fbsym.id, CONDITIONS))
				continue;

			vara cond; cond.SetSize(COND_MAX);
			cond[COND_DATE] = fbsym.GetStr(FB_DATE).Trim("'");
			cond[COND_USER] = fbsym.GetStr(FB_USER);
			cond[COND_USERLINK] = fbsym.GetStr(FB_ULINK);
			cond[COND_LINK] = fbsym.GetStr(FB_LINK);
			vars posttext = fbsym.GetStr(FB_TEXT);
			posttext.Replace(BR, "");
			cond[COND_TEXT] = "\""+posttext+"\""+" '''"+fbsym.GetStr(FB_SIGN)+"'''";
			cond[COND_TEXT].Replace(";",FBCOMMA);
			vara summary(fbsym.GetStr(FB_SUMMARY), ";"); 
			summary.SetSize(R_EXTENDED);
			cond[COND_DIFF] = summary[R_T];
			cond[COND_WATER] = summary[R_W];
			cond[COND_TEMP] = summary[R_TEMP];
			cond[COND_STARS] = summary[R_STARS];
			cond[COND_TIME] = summary[R_TIME];
			cond[COND_PEOPLE] = summary[R_PEOPLE];

			/*
			fbsym.SetStr(FB_SUMMARY,"");
			if (strstr(fbsym.data,";"))
				{
				Log(LOGERR, "ERROR: ';' in fbsym %s : %s", fbsym.id, fbsym.data);
				continue;
				}
			*/

			if (!fburlcheck(cond[COND_USERLINK]) || !fburlcheck(cond[COND_LINK]))
				{
				Log(LOGERR, "ERROR: Invalid url for POST / USER for %s by %s %s", fbsym.GetStr(ITEM_DESC), cond[COND_USER], cond[COND_DATE]);
				Log(cond[COND_LINK]);
				Log(cond[COND_USERLINK]);
				continue;
				}

			cond[COND_NOTSURE] = "data";
			if (fbsym.GetStr(FB_SURELOC).IsEmpty())
				cond[COND_NOTSURE] = "location";
			else if (fbsym.GetStr(FB_SUREDATE).IsEmpty())
				cond[COND_NOTSURE] = "date";

			CSym sym(fbsym.id, fbsym.GetStr(FB_NAME) );		
			sym.SetStr(ITEM_CONDDATE, cond.join(";"));

			vars credit = fbsym.GetStr(FB_USER)+"@Facebook";
			printf("%d/%d %s            \r", i, fbsymlist.GetSize(), fbsym.id);
			if (!UploadCondition(sym, sym.GetStr(ITEM_DESC), credit.replace(" ",""), TRUE))
				fbsym.id = "=";

			if (MODE<0)
				{
				// add likes
				vara likelist(fbsym.GetStr(FB_LIKE), " ");
				for (int l=0; l<likelist.length(); ++l)
					MFACEBOOK_Like(f, likelist[l]);
				}

			fbuserposting.AddUnique(CSym(fburlstr(fbsym.GetStr(FB_ULINK)), fbsym.GetStr(FB_USER) ));
		}

	fbuserposting.Save(filename(FBPOWERUSERS));
	fbsymlist.Save(fbname);

	return TRUE;
}








int DownloadFacebookPics(int mode, const char *fbname)
{
	if (!LoadBetaList(titlebetalist, TRUE, TRUE))
		return FALSE;

	CSymList fbsymlist;
	PICMODE = fbname ? fbname : "";
	FACEBOOK_Download(fbsymlist, fbname);

	fbsymlist.header = headers;
	fbsymlist.header.Replace("ITEM_","");
	//outlist.Sort(invcmpconddate);
	fbsymlist.Save("fbpics.csv");

	return TRUE;
}

vars GetPics(const char *opic)
{
	  DownloadFile f;
	  vars pic = opic, imgurl;
	  if (strstr(pic, "//flickr.com/"))
		{
		// flickr
		pic = ExtractString(pic, "?u=", "", "&"); 
		if (f.Download(pic))
			return "";
		vara imgs(f.memory, "<img ");
		for (int i=1; i<imgs.length() && imgurl.IsEmpty(); ++i)
			if (strstr(imgs[i], "class=\"main-photo\""))
				imgurl = ExtractString(imgs[i], "src=", "\"", "\"");
		if (!imgurl.IsEmpty() && !IsSimilar(imgurl, "http"))
			imgurl = "http:"+imgurl;
		return imgurl;
		}

	  if (strstr(pic, "ropewiki.com/File:"))
		{
		  if (f.Download(pic))
			{
			Log(LOGERR, "ERROR: Could not download url %.128s", pic);
			return "";
			}
		  imgurl = ExtractLink(f.memory, "alt=\"File:");
		  if (!imgurl.IsEmpty() && !IsSimilar(imgurl, "http"))
			  imgurl = "http://ropewiki.com"+imgurl;
		  return imgurl;
		}

	  // facebook
	  while (!pic.IsEmpty() && imgurl.IsEmpty())
	  {
	  if (f.Download(pic))
		  return "";
	  imgurl = fburlstr(ExtractLink(f.memory, ">View Full Size</a"));
	  if (imgurl.IsEmpty())
		pic = fburlstr(ExtractLink(f.memory, "alt=\"Image"));
	  }
	
	  return imgurl;
}



int UploadPics(int mode, const char *file)
{
	DownloadFile f;

	// upload pictures
	Login(f);
	CPage up(f, NULL, NULL, NULL);
	DownloadFile fb(TRUE);

	CSymList fbsymlist;
	fbsymlist.Load(file);


	for (int i=0; i<fbsymlist.GetSize(); ++i)
	  {
	  CSym &fbsym = fbsymlist[i];
	  vars pic = fbsym.GetStr(FB_PIC);
	  if (pic.IsEmpty())
		  continue;

	  vars name = fbsym.GetStr(ITEM_DESC).Trim();
	  vars author = fbsym.GetStr(ITEM_LAT).Trim();
	  vars authorlink = fbsym.GetStr(ITEM_LNG).Trim();
	  if (name.IsEmpty() || author.IsEmpty())
		  continue;

	  if (authorlink.IsEmpty())
		  Log(LOGWARN, "WARNING: No author link for %s", author);

	  vars comment = "Picture by "+author;//fburlstr2(pic);
	  if (!authorlink.IsEmpty())
		  comment += " "+fburlstr2(authorlink);
	  vars rfile = name.replace(" ", "_") + "_Banner.jpg";
	  printf("%d/%d %s         \r", i, fbsymlist.GetSize(), rfile);
	  if (MODE>-2 && MODE<2) 
		  if (up.FileExists(rfile))
			continue;
	  if (MODE>1)
		{
		ShellExecute(NULL, "open", ACP8(pic), NULL, NULL, SW_SHOWNORMAL);
		Sleep(1000);
		continue;
		}

	  // Get new banner pic
	  Log(LOGINFO, "FBPIC: %s : %s : %s", fbsym.id, name, author);
	  vars imgurl = GetPics(pic);
	  if (imgurl.IsEmpty())
		  {
		  Log(LOGERR, "Could not download pic for %s from %.128s", name, pic);
		  continue;
		  }

	  vars file = "banner.jpg";
	  if (fb.Download(imgurl, file))
		{
		Log(LOGERR, "ERROR: Could not download url %.128s", imgurl);
		continue;
		}

	  // upload banner file
	  up.UploadFile(file, rfile, comment);
	  PurgePage(f, NULL, name.replace(" ", "_"));
	  DeleteFile(file);

	  ShellExecute(NULL, "open", "http://ropewiki.com/File:"+ACP8(rfile), NULL, NULL, SW_SHOWNORMAL);
	  }
	return TRUE;
}










/*
int FACEBOOK_DownloadConditions(const char *ubase, CSymList &symlist) 
{
	int code = GetCode(ubase);
	Code &c = codelist[code];


	if (strstr(ubase, "m.facebook.com"))
		return MFACEBOOK_DownloadConditionsDate(ubase, symlist, c.region, c.flags);
	DownloadFile f;
	CString url = ubase;

	// download list of recent additions
	Throttle(facebookticks, FACEBOOKTICKS);
	if (f.Download(url, "testfb.html"))
		{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
		}

	vara list(f.memory, "userContentWrapper");
	for (int i=1; i<list.length(); ++i)
		{
		const char *memory = list[i];
		vars permalink = "/permalink/";
		vars link = ExtractString(memory, permalink, "", "\"");
		if (link.IsEmpty())
			{
			Log(LOGWARN, "Invalid facebook permalink in %s", url);
			continue;
			}

		vars urllink = ubase + permalink + link;

		double d = ExtractNum(memory, "data-utime=", "\"", "\"");				
		vars pdate = d<=0 ? -1 : CDate(Date(COleDateTime((__time64_t)d), TRUE));
		if (pdate.IsEmpty())
			{
			Log(LOGWARN, "Invalid facebook utime for %s in %s", urllink, url);
			continue;
			}

		vars text = ExtractString(memory, "userContent", ">", "</div>");
		text.Replace("<br","<p");
		text.Replace("</br","<p");
		text.Replace("</p","<p");
		int hash = text.Replace(">#<", ">(#)<");

		vara lines(text, "<p");

		// albums
		vara albums(memory, " album: <a");
		for (int l=1; l<albums.length(); ++l)
		  {
		  vars album = ExtractString(albums[l], "href=", "", "<");
		  if (album.IsEmpty())
			  continue;
		  lines.push(album);
		  }

		// links
		vara medialinks;
		vara media(list[i], "facebook.com/media/set");
		for (int l=1; l<media.length(); ++l)
			{
			vars link = GetToken(media[l], 0, "\"'>");
			if (link.IsEmpty())
				continue;
			vars medialink = "https://www.facebook.com/media/set"+htmltrans(link);
			if (medialinks.indexOf(medialink)>=0)
				continue;

			medialinks.push(medialink);
			Throttle(facebookticks, FACEBOOKTICKS);
			if (f.Download(medialink, "testfbm.htm"))
				{
				Log(LOGERR, "Could not download url %s", medialink);
				continue;
				}

			vars title = ExtractString(f.memory, "fbPhotoAlbumTitle", ">", "<");
			double d = ExtractNum(memory, "data-utime=", "\"", "\"");				
			vars pdate = d<=0 ? -1 : CDate(Date(COleDateTime((__time64_t)d), TRUE));
			if (pdate.IsEmpty())
				{
				Log(LOGWARN, "Invalid facebook utime for %s in %s", title, medialink);
				continue;
				}

			lines.push(pdate+" : "+title);
			}

		// text
		for (int l=0; l<lines.length(); ++l)
		  {
		  vars line = (l>0 ? "<p" : "") + lines[l];
		  line.Replace(",", " ");
		  line.Replace(";", " ");
		  line.Replace(".", " ");
		  line.Replace(":", " ");
		  line = stripHTML(line);

		  vara matchlist;
		  vars date = pdate;
		  if (!FACEBOOK_MatchDate(line, date, forcedate))
			  continue;
		  if (!FACEBOOK_MatchName(line, hash, matchlist))
			  continue; // too many matches or too little

		  Log(LOGINFO, "%s %s = '%s'", date, matchlist.join(";"), line);
		  for (int m=0; m<matchlist.length(); ++m)
			  {
			  CSym sym(RWTITLE+matchlist[m], line);
			  vara cond; cond.SetSize(COND_MAX);
			  cond[COND_DATE] = date;
			  cond[COND_LINK] = urllink;
			  sym.SetStr(ITEM_CONDDATE, cond.join(";"));
			  UpdateOldestCond(symlist, sym, NULL, FALSE);
			  }
		  }
		}
	return TRUE;
}
*/



/*
* [[File:waterflowA.png]] [[Allows value::]] : Completely dry or all pools avoidable 
* [[File:waterflowB.png]] [[Allows value::]] : Not flowing but requires wading in pools of water 
* [[File:waterflowB.png]] [[Allows value::]] : Not flowing but requires deep wading or swimming 
* [[File:waterflowC0.png]] [[Allows value::]] : Water flow should not represent any danger 
* [[File:waterflowC½.png]] [[Allows value::]] : Not as fun, but should be safe even for beginners 
* [[File:waterflowC1.png]] [[Allows value::]] : A bit low, still fun but not very challenging 
* [[File:waterflowC1.png]] [[Allows value::]] : Challenging but not dangerous for intermediate canyoneers 
* [[File:waterflowC1.png]] [[Allows value::]] : A bit high, quite challenging but not too dangerous 
* [[File:waterflowC2.png]] [[Allows value::]] : High water, only for experienced swift water canyoneers 
* [[File:waterflowC3.png]] [[Allows value::]] : Dangerously high water, only for expert swift water canyoneers 
* [[File:waterflowC4.png]] [[Allows value::]] : Extremely dangerous high water, may be unsafe even for experts


enum { W_DRY=0, W_WADING=1, W_SWIMMING=2, W_VERYLOW=3, W_LOW=4, W_MODLOW=5, W_MODERATE=6, W_MODHIGH=7, W_HIGH=8, W_VERYHIGH=9, W_EXTREME=10 };


*/
//const char *wconv[] = {"A - Dry", "B - Wading", "B - Swimming", "C0 - Very low", "CÂ½ - Low flow", "C1 - Moderate Low flow", "C1 - Moderate flow", "C1 - Moderate High flow", "C2 - High flow", "C3 - Very High flow", "C4 - Extreme flow", NULL };

//const char *wconv[] = {"0mm ","1mm ","3mm ","5mm ","7mm ","Xmm ", NULL};

//static vara cond_temp("0mm - None,1mm - Rain jacket,3mm - Thin wetsuit,5mm - Full wetsuit,7mm - Thick wetsuit,Xmm - Drysuit");

const char *wconv[] = {"DRY", "WADING", "SWIMMING", "LOW", "MODLOW", "3 - Significant flow (class C)", "MODHIGH","4 - High flow features (class C2)]","VERYHIGH", "5 - Extreme (class C3+)", NULL };

int FixConditions(int MODE, const char *query)
{
	CSymList idlist;
	if (!query || *query==0)
		query = "[[Category:Conditions]][[Has condition water::~3 -*]]";
	GetASKList(idlist, CString(query), rwftitle);

	DownloadFile f;
	for (int i=0; i<idlist.GetSize(); ++i)
		{
		int changes = 0;
		const char *title = idlist[i].id;
		CPage p(f, NULL, title);
		vars prop = "Wetsuit condition";
		vars w, ow = w = p.Get(prop);
		int m = IsMatchNC(w, wconv);
		if (m>=0)
			w = cond_temp[m];
		if (w[0]=='C')
			if (w[1]<'0' || w[1]>'4')
				w = w; //ASSERT(!w);

		if (w!=ow)
			{
			Log(LOGINFO, "%s =>%s for %s", ow, w, title);
				p.Set(prop, w, TRUE);
			p.comment.push("Fixed wetsuit values");
			p.Update();
			}
		else
			{
			Log(LOGINFO, "Skipping '%s' for %s", ow, title);
			}
		printf("%d/%d %s            \r", i, idlist.GetSize(), title);
		}
	return TRUE;
}

int ResetBetaColumn(int MODE, const char *value)
{
	vara hdr(vars(headers).replace(" ",""),",");
	int col = hdr.indexOf(value)-1;
	if (col<0)
		{
		Log(LOGERR, "Could not find %s in %s", value, headers);
		return FALSE;
		}

	for (int i=0; codelist[i].code!=NULL; ++i)
		{
		//if (value && *value!=0 && strcmp(codelist[i].code, value)!=0)
		//	continue;

		CSymList list;
		vars file = filename(codelist[i].code);
		list.Load(file);
		for (int l=0; l<list.GetSize(); ++l)
			{
			if (MODE==1)
				{
				// insert
				for (int j=ITEM_BETAMAX; j>=col; --j)
					list[l].SetStr(j+1, list[l].GetStr(j));
				list[l].SetStr(col, "");
				}
			if (MODE==2)
				{
				// move columns
				vars region = list[l].GetStr(ITEM_REGION);
				for (int j=ITEM_REGION-1; j>=ITEM_ACA; --j)
					list[l].SetStr(j+1, list[l].GetStr(j));
				list[l].SetStr(ITEM_ACA, region);
				}
			if (MODE==-1)
				list[l].SetStr(col, "");
			}
		list.header = GetToken(list.header, 0) + "," + GetTokenRest(headers, 1);
		list.header.Replace("ITEM_","");
		list.Save(file);
		}

	return TRUE;
}


//-mode 1 -setfbconditions infocaudales



#if 0 
void Test(const char *geomatchfile)
{
/*
	//DownloadFile f;
	//f.Download("http://ropewiki.com/LucaTest2?aka="+url_encode("Destéou"), "test.htm");
Code bq("bqnlist", "Barranquismo.net", NULL, NULL, BARRANQUISMO_DownloadBeta, NULL, FALSE, "es" );
	CSymList symlist;
	bq.downloadbeta("", symlist);
	symlist.Save("bqnlist.csv");
*/
/*
	extern CString InstagramAuth(void);
	vars res = InstagramAuth();
*/


	int l, perfects[2];

	//int ln;
	vars lmatch;
	//FBMATCH.FACEBOOK_Lists(l, ln, lmatch, "barranco de salt o fleix.", "barranco de saltococino", NULL, "+");
	
	l = BestMatchSimple("barrancoMascun Descendido  < : > Caudal normal;", "Barranco Mascun", perfects);
	l = BestMatchSimple("barrancoMascun Descendido  < : > Caudal normal;", "barranco mascun", perfects);
	l = BestMatchSimple("salt o fleix.", "saltococino", perfects);
	l = BestMatchSimple("coa negra", "coanegra", perfects);
	l = BestMatchSimple("coa negra", "coanegra", perfects);
	l = BestMatchSimple("coa . negra", "coanegra", perfects);
	l = BestMatchSimple("coa...negra", "coanegra", perfects);
	l = BestMatchSimple("coa[#]negra", "coanegra", perfects);
	l = BestMatchSimple("coa'ne'gra", "coanegra", perfects);
	l = BestMatchSimple("coa'ne'gra", "coane tro", perfects);
	l = BestMatchSimple("[#]coa negra", "coanegra", perfects);
	l = BestMatchSimple("[#]coanegra", "coanegra", perfects);
	l = BestMatchSimple("coa-ne-gra", "coanegra", perfects);
	l = BestMatchSimple("coa-negra", "coa negra", perfects);
	l = BestMatchSimple("coanegra", "coa negra", perfects);
	l = BestMatchSimple("coanegrita", "coa negra", perfects);
	l = BestMatchSimple("coanegri ta", "coanegra", perfects);
	l = BestMatchSimple("coanegr ita", "coanegr-la", perfects);
	l = l+1;

	l = l+1;

	/*
	vars stra = Capitalize("La Bendola");

	CString utf8("TRIÃˆGE");
	vars str2 = Capitalize("TRIÃˆGE");
	vars str3 = Capitalize("LE III IIIIII II CANYON CAPTAIN'S DU TRIÃˆGE");
	vars str4 = Capitalize("le IV canyon d'avignone l'ossule captain's du Ãˆcer (middle)");
	vars str = CString("TRIÈGE");
	vars utf = UTF8(str);
	vars utf8b = UTF8(ACP8(utf8).MakeLower());
	str.MakeLower();
	utf.MakeLower();
	//vars acp = ACP8(utfw);
	str += str;
	*/
/*
// insert one more column in beta files
	for (int i=0; codelist[i].code; ++i)
		{
		vars file = filename(codelist[i].code);
		CSymList list;
		list.Load(file);
		for (int l=0; l<list.GetSize(); ++l)
			{
			vara adata(list[i].data);
			adata.InsertAt(5,"X");
			list[i] = CSym(list[i].id, adata.join());
			}

		list.Save(file);
		}
*/
}


#endif