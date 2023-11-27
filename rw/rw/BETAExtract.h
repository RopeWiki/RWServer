#pragma once

#include "stdafx.h"
#include "TradeMaster.h"
#include "rw.h"
#include "GeoRegion.h"

#define BETA "BETA"

#define DESCENT RGB(0xFF, 0x00, 0x00)
#define EXIT RGB(0xFF, 0xFF, 0x00)
#define APPROACH RGB(0x00, 0xFF, 0x00)
#define ROAD RGB(0x00, 0x00, 0x00)
#define OTHER RGB(0xFF, 0x00, 0xFF)

#define ITEM_MATCH ITEM_LAT2
#define ITEM_NEWMATCH ITEM_LNG2

#define RWREDIR "rwredir"
#define RWID "RWID:"
#define RWTITLE "RWTITLE:"

#define TTRANSLATED "(Translated)" 
#define TRANSBASIC "trans-basic"
#define TORIGINAL "(Original)"

#define MINCHARMATCH 3

#define FBLIST "\n*"
#define FBCOMMA "|"

#define CONDITIONS "Conditions:"

#define KMLFOLDER "BETAKML"
#define KMLFIXEDFOLDER "BETAKML\\FIXED"
#define ROPEWIKIFILE "http://ropewiki.com/File:"

#define RWLINK "@"

#define BQNFOLDER "BQN"

#define DownloadRetry(f, url) f.Download(url)

#define CHGFILE "CHG"
#define MATCHFILE "MATCH"
#define DIST15KM (25*1000)
#define DIST150KM (150*1000)
#define MAXGEOCODEDIST 50000 // 50km
#define MAXGEOCODENEAR 5 // nearby
#define REPLACEBETALINK FALSE

#define DISAMBIGUATION " (disambiguation)"

#define KMLEXTRACT 1


typedef CString kmlwatermark(const char *scredit, const char *memory, int size);

typedef struct { const char *unit; double cnv; } unit;
extern unit utime[];
extern unit udist[];
extern unit ulen[];

extern int MODE;
extern int INVESTIGATE;


class CPage;

static CSymList rwlist, titlebetalist;


static const char *andstr[] = { " and ", " y ", " et ", " e ", " und ", "&", "+", "/", ":", " -", "- ", NULL };
static const char *headers = "ITEM_URL, ITEM_DESC, ITEM_LAT, ITEM_LNG, ITEM_MATCH, ITEM_MODE, ITEM_INFO, ITEM_REGION, ITEM_ACA, ITEM_RAPS, ITEM_LONGEST, ITEM_HIKE, ITEM_MINTIME, ITEM_MAXTIME, ITEM_SEASON, ITEM_SHUTTLE, ITEM_VEHICLE, ITEM_CLASS, ITEM_STARS, ITEM_AKA, ITEM_PERMIT, ITEM_KML, ITEM_CONDDATE, ITEM_AGAIN, ITEM_EGAIN, ITEM_LENGTH, ITEM_DEPTH, ITEM_AMINTIME, ITEM_AMAXTIME, ITEM_DMINTIME, ITEM_DMAXTIME, ITEM_EMINTIME, ITEM_EMAXTIME, ITEM_ROCK, ITEM_LINKS, ITEM_EXTRA, https://maps.googleapis.com/maps/api/geocode/xml?address=,\"=+HYPERLINK(+CONCATENATE(\"\"http://ropewiki.com/index.php/Location?jform&locsearchchk=on&locname=\"\",C1,\"\",\"\",D1,\"\"&locdist=5mi&skinuser=&sortby=-Has_rank_rating&kmlx=\"\",A1),\"\"@check\"\")\", ";
static const char *raquaticu[] = { "A1", "A2-", "A2+", "A2", "A3", "A4-", "A4+", "A4", "A5", "A6", "A7", NULL };
static const char *rclassc[] = { "B+", "B-", "C+", "C-", "B/C", "C0", "C1+", "C1-", "C1", "C2", "C3", "C4", NULL };
static const char *rtech[] = { "1", "2", "3", "4", NULL };
static const char *rtime[] = { "IV", "VI", "III", "II", "I", "V", NULL };
static const char *rverticalu[] = { "V1", "V2", "V3", "V4", "V5", "V6", "V7", NULL };
static const char *rwform = "=|=|=|=|=|=|Region|=ACA|Number of rappels|Longest rappel|Hike length|Fastest typical time|Slowest typical time|Best season|NeedsShuttle;Shuttle|Vehicle|Location type|=Stars|AKA|Permits|=KML|=COND|Approach elevation gain|Exit elevation gain|Length|Depth|Fastest approach time|Slowest approach time|Fastest descent time|Slowest descent time|Fastest exit time|Slowest exit time|Rock type|=SKETCH"; //|Has user counter";
static const char *rwformaca = "Technical rating;Water rating;Time rating;Extra risk rating;Vertical rating;Aquatic rating;Commitment rating";
static const char *rwprop = "Has pageid|Has coordinates|Has geolocation|Has BetaSites list|Has TripReports list|Has info|Has info major region|Has info summary|Has info rappels|Has longest rappel|Has length of hike|Has fastest typical time|Has slowest typical time|Has best season|Has shuttle length|Has vehicle type|Has location class|Has user counter|Has AKA|Requires permits|Has KML file|Has condition date|Has approach elevation gain|Has exit elevation gain|Has length|Has depth|Has fastest approach time|Has slowest approach time|Has fastest descent time|Has slowest descent time|Has fastest exit time|Has slowest exit time|Has rock type|Has SketchList"; //|Has user counter";
static const char *rwater[] = { "A", "B", "C", NULL };
static const char *rxtra[] = { "R-", "R+", "R", "PG-", "PG+", "PG", "XX", "X", NULL };

static vara cond_diff("0 - Nontechnical,1 - Easy,2 - Normal,3 - Special challenges,4 - Advanced,5 - Extreme");
static vara cond_stars("0 - Unknown,1 - Poor,2 - Ok,3 - Good,4 - Great,5 - Amazing");
static vara cond_temp("0 - None,1 - Rain jacket (1mm-2mm),2 - Thin wetsuit (3mm-4mm),3 - Full wetsuit (5mm-6mm),4 - Thick wetsuit (7mm-10mm),5 - Drysuit");
static vara cond_water("0 - Dry (class A = a1),1 - Very Low (class B = a2),2 - Deep pools (class B+ = a2+),1 - Very Low (class B = a2),3 - Low (class B/C = a3),4 - Moderate Low (class C1- = a4-),5 - Moderate (class C1 = a4),6 - Moderate High (class C1+ = a4+),7 - High (class C2 = a5),8 - Very High (class C3 = a6),9 - Extreme (class C4 = a7)");
static vara months("Jan,Feb,Mar,Apr,May,Jun,Jul,Aug,Sep,Oct,Nov,Dec");

enum { ITEM_INFO = ITEM_SUMMARY, ITEM_REGION, ITEM_ACA, ITEM_RAPS, ITEM_LONGEST, ITEM_HIKE, ITEM_MINTIME, ITEM_MAXTIME, ITEM_SEASON, ITEM_SHUTTLE, ITEM_VEHICLE, ITEM_CLASS, ITEM_STARS, ITEM_AKA, ITEM_PERMIT, ITEM_KML, ITEM_CONDDATE, ITEM_AGAIN, ITEM_EGAIN, ITEM_LENGTH, ITEM_DEPTH, ITEM_AMINTIME, ITEM_AMAXTIME, ITEM_DMINTIME, ITEM_DMAXTIME, ITEM_EMINTIME, ITEM_EMAXTIME, ITEM_ROCK, ITEM_LINKS, ITEM_EXTRA, ITEM_BETAMAX, ITEM_BESTMATCH, ITEM_MATCHLIST,  };
enum { COND_DATE, COND_STARS, COND_WATER, COND_TEMP, COND_DIFF, COND_LINK, COND_USER, COND_USERLINK, COND_TEXT, COND_NOTSURE, COND_TIME, COND_PEOPLE, COND_MAX };
enum { T_NAME = 0, T_REGION, T_SEASON, T_TROCK, T_MAX };
enum { M_MATCH = ITEM_ACA, M_SCORE, M_LEN, M_NUM, M_RETRY };
enum { R_T = 0, R_W, R_I, R_X, R_V, R_A, R_C, R_SUMMARY, R_STARS, R_TEMP, R_TIME, R_PEOPLE, R_LINE, R_EXTENDED };
enum { W_DRY = 0, W_WADING = 1, W_SWIMMING = 2, W_VERYLOW = 3, W_LOW = 4, W_MODLOW = 5, W_MODERATE = 6, W_MODHIGH = 7, W_HIGH = 8, W_VERYHIGH = 9, W_EXTREME = 10 };


vars ACP8(const char *val, int cp = CP_UTF8);
double Avg(CDoubleArrayList &time);
const char *skipnoalpha(const char *str);
CString CoordsMemory(const char *memory);
int Download_Save(const char *url, const char *folder, CString &memory);
int ExtractCoords(const char *memory, float &lat, float &lng, CString &desc);
vars ExtractHREF(const char *str);
CString ExtractStringDel(CString &memory, const char *pre, const char *start, const char *end, int ext = FALSE, int del = TRUE);
int FindSection(vara &lines, const char *section, int *iappend);
int GetClass(const char *type, const char *types[], int typen[]);
void GetCoords(const char *memory, float &lat, float &lng);
vars getfulltext(const char *line, const char *label = "fulltext=");
void GetHourMin(const char *str, double &vmin, double &vmax, const char *url);
int GetKMLCoords(const char *str, double &lat, double &lng);
vars GetKMLIDX(DownloadFile &f, const char *url, const char *search = NULL, const char *start = NULL, const char *end = NULL);
vars getlabel(const char *label);
CString GetMetric(const char *str);
int GetRappels(CSym &sym, const char *str);
int GetSummary(CSym &sym, const char *str);
int GetSummary(vara &rating, const char *str);
int GetTimeDistance(CSym &sym, const char *str);
void GetTotalTime(CSym &sym, vara &times, const char *url, double maxtmin = 24);
int GetValues(const char *str, unit *units, CDoubleArrayList &time);
int GetValues(const char *data, const char *str, const char *sep1, const char *sep2, unit *units, CDoubleArrayList &time);
vara getwords(const char *text);
int GPX_ExtractKML(const char *credit, const char *url, const char *memory, inetdata *out);
vars invertregion(const char *str, const char *add = "");
BOOL isa(unsigned char c);
BOOL isanum(unsigned char c);
int IsImage(const char *url);
BOOL IsSeasonValid(const char *season, CString *validated = NULL);
CString KML_Watermark(const char *scredit, const char *memory, int size);
vars KMLIDXLink(const char *purl, const char *pkmlidx);
int KMLMAP_DownloadBeta(const char *ubase, CSymList &symlist);
int KMZ_ExtractKML(const char *credit, const char *url, inetdata *out, kmlwatermark *watermark = KML_Watermark);
int LoadBetaList(CSymList &bslist, int title = FALSE, int rwlinks = FALSE);
void LoadRWList();
int RWLogin(DownloadFile &f);
vars makeurl(const char *ubase, const char *folder);
CString noHTML(const char *str);
CString Pair(CDoubleArrayList &raps);
void PurgePage(DownloadFile &f, const char *id, const char *title);
vars regionmatch(const char *region, const char *matchregion = NULL);
int rwfregion(const char *line, CSymList &regions);
int rwftitle(const char *line, CSymList &idlist);
int SaveKML(const char *title, const char *credit, const char *url, vara &styles, vara &points, vara &lines, inetdata *out);
void SetClass(CSym &sym, int t, const char *type);
void SetVehicle(CSym &sym, const char *text);
CString starstr(double stars, double ratings);
vars stripAccents(register const char* p);
vars stripAccentsL(register const char* p);
CString stripHTML(const char *data);
vars stripSuffixes(register const char* name);
vars TableMatch(const char *str, vara &inlist, vara &outlist);
vars Translate(const char *str, CSymList &list, const char *onlytrans = NULL);
int Update(CSymList &list, CSym &newsym, CSym *chgsym = NULL, BOOL trackchanges = TRUE);
int UpdateCheck(CSymList &symlist, CSym &sym);
int UpdateCond(CSymList &list, CSym &newsym, CSym *chgsym = NULL, BOOL trackchanges = TRUE);
int UpdateProp(const char *name, const char *value, vara &lines, int force = FALSE);
int UploadCondition(CSym &sym, const char *title, const char *credit, int trackdate = FALSE);
vars url2file(const char *url, const char *folder);
vars urlstr(const char *url, int stripkmlidx = TRUE);
vars UTF8(const char *val, int cp = CP_ACP);


//static functions

static CString filename(const char *name)
{
	return BETA + vars("\\") + vars(name) + ".csv";
}


static CString burl(const char *base, const char *folder, bool useHttps)
{
	if (IsSimilar(folder, "../"))
		folder += 2;
	if (IsSimilar(folder, "http"))
		return folder;
	if (folder[0] == '/' && folder[1] == '/')
		return CString("http:") + folder;
	CString str;
	if (!IsSimilar(base, "http")) {
		if (!useHttps)
			str += "http://";
		else
			str += "https://";
	}
	str += base;
	if (folder[0] != '/')
		str += "/";
	return str + folder;
}


static CString burl(const char *base, const char *folder)
{
	return burl(base, folder, false);
}
