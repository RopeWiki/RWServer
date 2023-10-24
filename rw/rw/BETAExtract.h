#pragma once

#include "stdafx.h"
#include "trademaster.h"
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

#define TTRANSLATED "(Translated)" 
#define TRANSBASIC "trans-basic"
#define TORIGINAL "(Original)"

#define MINCHARMATCH 3

#define FBLIST "\n*"
#define FBCOMMA "|"

#define CONDITIONS "Conditions:"

extern int MODE;

#if 0
int DownloadRetry(DownloadFile &f, const char *url, int retrywait = 5)
{

	for (int r = 0; r < 2; ++r) {
		if (!f.Download(url))
			return FALSE;
		Sleep(retrywait * 1000);
	}
	return TRUE;
}
#else
#define DownloadRetry(f, url) f.Download(url)
#endif

typedef struct { const char *unit; double cnv; } unit;
extern unit utime[];
extern unit udist[];
extern unit ulen[];

typedef CString kmlwatermark(const char *scredit, const char *memory, int size);

enum { ITEM_INFO = ITEM_SUMMARY, ITEM_REGION, ITEM_ACA, ITEM_RAPS, ITEM_LONGEST, ITEM_HIKE, ITEM_MINTIME, ITEM_MAXTIME, ITEM_SEASON, ITEM_SHUTTLE, ITEM_VEHICLE, ITEM_CLASS, ITEM_STARS, ITEM_AKA, ITEM_PERMIT, ITEM_KML, ITEM_CONDDATE, ITEM_AGAIN, ITEM_EGAIN, ITEM_LENGTH, ITEM_DEPTH, ITEM_AMINTIME, ITEM_AMAXTIME, ITEM_DMINTIME, ITEM_DMAXTIME, ITEM_EMINTIME, ITEM_EMAXTIME, ITEM_ROCK, ITEM_LINKS, ITEM_EXTRA, ITEM_BETAMAX, ITEM_BESTMATCH, ITEM_MATCHLIST }; //, ITEM_BOAT, , ITEM_AELEV, ITEM_EELEV,  };
enum { COND_DATE, COND_STARS, COND_WATER, COND_TEMP, COND_DIFF, COND_LINK, COND_USER, COND_USERLINK, COND_TEXT, COND_NOTSURE, COND_TIME, COND_PEOPLE, COND_MAX };
static vara cond_water("0 - Dry (class A = a1),1 - Very Low (class B = a2),2 - Deep pools (class B+ = a2+),1 - Very Low (class B = a2),3 - Low (class B/C = a3),4 - Moderate Low (class C1- = a4-),5 - Moderate (class C1 = a4),6 - Moderate High (class C1+ = a4+),7 - High (class C2 = a5),8 - Very High (class C3 = a6),9 - Extreme (class C4 = a7)");

enum { T_NAME = 0, T_REGION, T_SEASON, T_TROCK, T_MAX };

enum { M_MATCH = ITEM_ACA, M_SCORE, M_LEN, M_NUM, M_RETRY };
enum { R_T = 0, R_W, R_I, R_X, R_V, R_A, R_C, R_SUMMARY, R_STARS, R_TEMP, R_TIME, R_PEOPLE, R_LINE, R_EXTENDED };

static vara cond_temp("0 - None,1 - Rain jacket (1mm-2mm),2 - Thin wetsuit (3mm-4mm),3 - Full wetsuit (5mm-6mm),4 - Thick wetsuit (7mm-10mm),5 - Drysuit");
//static vara cond_temp("0mm - None,1mm - Rain jacket,3mm - Thin wetsuit,5mm - Full wetsuit,7mm - Thick wetsuit,Xmm - Drysuit");

static vara cond_diff("0 - Nontechnical,1 - Easy,2 - Normal,3 - Special challenges,4 - Advanced,5 - Extreme");

static vara cond_stars("0 - Unknown,1 - Poor,2 - Ok,3 - Good,4 - Great,5 - Amazing");

static const char *headers = "ITEM_URL, ITEM_DESC, ITEM_LAT, ITEM_LNG, ITEM_MATCH, ITEM_MODE, ITEM_INFO, ITEM_REGION, ITEM_ACA, ITEM_RAPS, ITEM_LONGEST, ITEM_HIKE, ITEM_MINTIME, ITEM_MAXTIME, ITEM_SEASON, ITEM_SHUTTLE, ITEM_VEHICLE, ITEM_CLASS, ITEM_STARS, ITEM_AKA, ITEM_PERMIT, ITEM_KML, ITEM_CONDDATE, ITEM_AGAIN, ITEM_EGAIN, ITEM_LENGTH, ITEM_DEPTH, ITEM_AMINTIME, ITEM_AMAXTIME, ITEM_DMINTIME, ITEM_DMAXTIME, ITEM_EMINTIME, ITEM_EMAXTIME, ITEM_ROCK, ITEM_LINKS, ITEM_EXTRA, https://maps.googleapis.com/maps/api/geocode/xml?address=,\"=+HYPERLINK(+CONCATENATE(\"\"http://ropewiki.com/index.php/Location?jform&locsearchchk=on&locname=\"\",C1,\"\",\"\",D1,\"\"&locdist=5mi&skinuser=&sortby=-Has_rank_rating&kmlx=\"\",A1),\"\"@check\"\")\"";

class CPage;

static CSymList rwlist, titlebetalist;

CString noHTML(const char *str);
vars UTF8(const char *val, int cp = CP_ACP);
vars Translate(const char *str, CSymList &list, const char *onlytrans = NULL);
vars TableMatch(const char *str, vara &inlist, vara &outlist);
int GetSummary(CSym &sym, const char *str);
int GetSummary(vara &rating, const char *str);
vars invertregion(const char *str, const char *add = "");
void GetTotalTime(CSym &sym, vara &times, const char *url, double maxtmin = 24);
vars urlstr(const char *url, int stripkmlidx = TRUE);
int Update(CSymList &list, CSym &newsym, CSym *chgsym = NULL, BOOL trackchanges = TRUE);
int UpdateCond(CSymList &list, CSym &newsym, CSym *chgsym = NULL, BOOL trackchanges = TRUE);
int SaveKML(const char *title, const char *credit, const char *url, vara &styles, vara &points, vara &lines, inetdata *out);
CString stripHTML(const char *data);
int UpdateCheck(CSymList &symlist, CSym &sym);
int GetValues(const char *str, unit *units, CDoubleArrayList &time);
int GetValues(const char *data, const char *str, const char *sep1, const char *sep2, unit *units, CDoubleArrayList &time);
CString Pair(CDoubleArrayList &raps);
int GPX_ExtractKML(const char *credit, const char *url, const char *memory, inetdata *out);
vars GetKMLIDX(DownloadFile &f, const char *url, const char *search = NULL, const char *start = NULL, const char *end = NULL);
int GetTimeDistance(CSym &sym, const char *str);
int GetRappels(CSym &sym, const char *str);
int GetClass(const char *type, const char *types[], int typen[]);
void SetClass(CSym &sym, int t, const char *type);
CString starstr(double stars, double ratings);
CString KML_Watermark(const char *scredit, const char *memory, int size);
int KMZ_ExtractKML(const char *credit, const char *url, inetdata *out, kmlwatermark *watermark = KML_Watermark);
vars makeurl(const char *ubase, const char *folder);
CString CoordsMemory(const char *memory);
int ExtractCoords(const char *memory, float &lat, float &lng, CString &desc);
int GetKMLCoords(const char *str, double &lat, double &lng);
void SetVehicle(CSym &sym, const char *text);
vars KMLIDXLink(const char *purl, const char *pkmlidx);
vars regionmatch(const char *region, const char *matchregion = NULL);
vars stripSuffixes(register const char* name);
vars stripAccents(register const char* p);
vars stripAccentsL(register const char* p);
BOOL IsSeasonValid(const char *season, CString *validated = NULL);
vars ACP8(const char *val, int cp = CP_UTF8);
vars ExtractHREF(const char *str);
const char *skipnoalpha(const char *str);
CString GetMetric(const char *str);
void GetCoords(const char *memory, float &lat, float &lng);
BOOL isanum(unsigned char c);
CString ExtractStringDel(CString &memory, const char *pre, const char *start, const char *end, int ext = FALSE, int del = TRUE);
BOOL isa(unsigned char c);
int LoadBetaList(CSymList &bslist, int title, int rwlinks);
int UploadCondition(CSym &sym, const char *title, const char *credit, int trackdate = FALSE);
int Login(DownloadFile &f);
int rwftitle(const char *line, CSymList &idlist);
void  PurgePage(DownloadFile &f, const char *id, const char *title);
int UpdateProp(const char *name, const char *value, vara &lines, int force = FALSE);
int FindSection(vara &lines, const char *section, int *iappend);
vars url2file(const char *url, const char *folder);

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
