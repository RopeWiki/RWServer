#pragma once


#ifdef DEBUG
#define SERVER "localhost"
#define RWBASE vars("http://localhost:8080/")
#else
#define server "ropewiki.com/luca"
#define RWBASE vars("https://ropewiki.com/")
//#define server "localhost"
#endif

enum { SITE_DESC, SITE_LAT, SITE_LNG, SITE_MODE, SITE_EXTRA, SITE_EQUIV, SITE_RESERVED, SITE_WATERSHEDDIST, SITE_WATERSHED, SITE_NUM };
enum { ITEM_DESC, ITEM_LAT, ITEM_LNG, ITEM_LAT2, ITEM_LNG2, ITEM_SUMMARY, ITEM_WATERSHED, ITEM_CONN,
	   ITEM_TAG, ITEM_COORD, ITEM_NUM, ITEM_EXIT, ITEM_LATPARKING, ITEM_LNGPARKING, ITEM_LATSHUTTLE,
	   ITEM_LNGSHUTTLE, ITEM_BANNERPHOTO, ITEM_INTRO, ITEM_REDTAPE, ITEM_TRIPREPORT, ITEM_BACKINFO};
enum { DEM_DESC, DEM_LAT, DEM_LNG, DEM_LAT2, DEM_LNG2, DEM_DATE, DEM_SIZE, DEM_PROJ, DEM_RES, DEM_ELEV, DEM_X, DEM_NUM };


#define GPX ".gpx"
#define KML ".kml"
#define KMZ ".kmz"
#define PNG ".png"
#define CSV ".csv"

#define ucheck(q,x) ((strstr(q,x)!=NULL) ? strstr(q,x)+strlen(x) : NULL)

const char *AuthenticateUser(const char *key);

vars cleanhref(const char *str, const char *tag = "");
vars addmode(const char *mode, const char *newmode);

int jsonarray(const char *data, vara &list);
int jsonarray(vara &data, vara &list);
vars htmltrans(const char *string);

class CKMLOut;
typedef void transfunc(CKMLOut &out, const char *line);

class CKMLOut {
	BOOL deldata;
	inetdata *data;
	int clink;
	int started;
	int extension;
public:
	CString name;
	CKMLOut(const char *kmlfile);
	CKMLOut(inetdata *dataout, const char *name, int clink = FALSE);
	CKMLOut& operator+=(const CString &str);
	~CKMLOut(void);
	static int Exist(const char *kmlfile);
	int Load(const char *kmlfile, transfunc *f = NULL);
	void SetExtensions(void) { extension = TRUE; }
};

#define KMLEMPTY "<?xml>"
CString stripHTML(const char *str);
int KMLMerge(const char *url, inetdata *data);
int KMLExtract(const char *url, inetdata *data, int fx);


vars xmlval(const char *data, const char *sep1, const char *sep2 = NULL);
vars strval(const char *data, const char *sep1, const char *sep2 = NULL);
int CheckLL(double lat, double lng, const char *url = NULL);

void WaterflowTestSites(const char *name);
void WaterflowSaveSites(const char *name);
void WaterflowSaveForecastSites(void);
void WaterflowSaveUSGSSites(void);
void WaterflowSaveKMLSites(CKMLOut &out, const char *link = NULL);
void WaterflowLoadSites(int usgs = FALSE);
void WaterflowFlush(void);
void WaterflowUnloadSites(void);
int WaterflowQuery(VOID *request);

void WatershedSaveSites(const char *name);

int PictureQuery(const char *location, inetdata &data);


double Distance(double lat1, double lng1, double lat2, double lng2);


#define ValidLL(lat, lng) ((lat)!=0 && (lng)!=0 && (lat)>=-90 && (lat)<=90 && (lng)>=-180 && (lng)<=180)

class LL
{
public:
	float lat, lng;

	LL()
	{
		lat = lng = 0;
	}

	LL(double olat, double olng)
	{
		lat = (float)olat;
		lng = (float)olng;
	}

	
	inline static double Distance(const LL *A, const LL *B)
	{
		return ::Distance(A->lat, A->lng, B->lat, B->lng);
	}

	static int cmp(LL *lle1, LL *lle2)
	{
		if (lle1->lat>lle2->lat) return 1;
		if (lle1->lat<lle2->lat) return -1;
		if (lle1->lng>lle2->lng) return 1;
		if (lle1->lng<lle2->lng) return -1;
		return 0;
	}

	CString Summary()
	{
		return MkString("%.6f %.6f", lat, lng);
	}

	BOOL IsValid()
	{
		return ValidLL(lat, lng);
	}
};

class LLRect {
public:
float lat1;
float lng1;
float lat2;
float lng2;


inline LLRect(void)
	{
	lat1 = lat2 = lng1 = lng2 = 0;
	}

inline LLRect(double lat, double lng, double dist)
	{
	LLRect LLDistance(double lat, double lng, double dist);
	*this = LLDistance(lat, lng, dist);
	}

inline LLRect(double lat1, double lng1, double lat2, double lng2)
	{
		this->lat1 = (float)min(lat1, lat2);
		this->lat2 = (float)max(lat1, lat2);
		this->lng1 = (float)min(lng1, lng2);
		this->lng2 = (float)max(lng1, lng2);
	}

inline int Intersects(LLRect const& other) const
    {
        return lat1 < other.lat2 && lat2 > other.lat1 && 
               lng1 < other.lng2 && lng2 > other.lng1;
    }

inline int Contains(LLRect const& other) const
    {
        return lat1 < other.lat1 && lng1 < other.lng1 &&
               lat2 > other.lat2 && lng2 > other.lng2;
    }

inline int Contains(double lat, double lng) const
    {
        return lat1 <= lat && lng1 <= lng &&
               lat2 >= lat && lng2 >= lng;
    }

inline int Contains(LL &lle) const
{
	return Contains(lle.lat, lle.lng);
}

void Expand(double lat, double lng)
	{
		if (!ValidLL(lat,lng))
			return;
		if (!IsValid())
			{
			// start expansion
			lat1 = lat2 = (float)lat;
			lng1 = lng2 = (float)lng;
			return;
			}
		if (lat<lat1) lat1=(float)lat;		
		if (lat>lat2) lat2=(float)lat;
		if (lng<lng1) lng1=(float)lng;		
		if (lng>lng2) lng2=(float)lng;
	}

inline void Expand(double dist)
	{
	Expand(LLRect(lat1, lng1, dist));
	Expand(LLRect(lat2, lng2, dist));
	}

inline void Expand(LLRect const &other)
	{
		Expand(other.lat1, other.lng1);
		Expand(other.lat2, other.lng2);
	}

inline void Intersect(LLRect &other)
    {
        lat1 = max(lat1, other.lat1);
        lat2 = min(lat2, other.lat2);
        lng1 = max(lng1, other.lng1);
        lng2 = min(lng2, other.lng2);
	}

inline int IsNull(void) { return lat1==lat2 && lng1==lng2; }

inline int IsValid(void) { return !(lat1==0 && lat2==0 && lng1==0 && lng2==0); }

	static int cmp(LLRect *b1, LLRect *b2)
	{
		//const LLRect *b1 = *a1;
		//const LLRect *b2 = *a2;
		if (b1->lat1>b2->lat1) return 1;
		if (b1->lat1<b2->lat1) return -1;
		if (b1->lng1>b2->lng1) return 1;
		if (b1->lng1<b2->lng1) return -1;
		return 0;
	}

CString Summary();

};

// quick match of rectangles vs points
// rect around point to have distance buffer
template<class LLRTYPE,class LLTYPE>
class LLMatch {
public:
typedef int rproc(LLRTYPE *r, LLTYPE *lle, void *data); // TRUE to get only 1 match, 0 to get all matches
LLMatch(CArrayList <LLRTYPE> &rlist, CArrayList <LLTYPE> &llelist, rproc *proc, void *data = NULL)
{
	int match = 0;
	rlist.Sort(LLRTYPE::cmp);
	llelist.Sort(LLTYPE::cmp);
	int nbox = rlist.GetSize(), nlle = llelist.GetSize();
	for (int i=0, j=0; i<nbox; ++i)
	  {
	  const LLRect *box = &rlist[i];
	  //skip out of box
	  while (j<nlle && llelist[j].lat < box->lat1)
		  ++j;
	  // process inside box
	  for (int k=j; k<nlle; ++k) 
		{
		PLL *lle = &llelist[k];
		if (lle->lat > box->lat2)
			break; // finish box
		if (lle->lng < box->lng1 || lle->lng > box->lng2)
			continue; // out of bounds

		// match, single or multiple
		if (proc(&rlist[i], lle, data))
			break;
		}
	  }
}

};






LLRect GetBox(CKMLOut &out, const char *region);
void WaterflowMarkerList(CKMLOut &out, const char *name, const char *rwicon, const char *linkref, LLRect *bbox);

#define CDATAS "<![CDATA["
#define CDATAE "]]>"
void Throttle(double &last, int ms);

#define CCoord(val) MkString("%.6lf", val)
#define CCoord2(lat,lng) MkString("%.6lf,%.6lf", lat, lng)
#define CCoord3(lat,lng) MkString("%.6lf,%.6lf,0", lng, lat)
#define CCoord4(lat,lng,elev,ang) MkString("%.6lf,%.6lfA%.6lf,%.2lf", lng, lat, ang, elev)

#define RGBA(a,r,g,b) (((int)(a)<<24)|RGB(r,g,b))

CString KMLStart(const char *name = NULL, int clink = FALSE, int extension = FALSE);
CString KMLEnd(int clink = FALSE);
CString KMLFolderStart(const char *name, const char *icon = NULL, int hidechildren = FALSE, int visibility = TRUE);
CString KMLFolderEnd(void);
CString KMLMarker(const char *style, double lat, double lng, const char *name, const char *desc = NULL);
CString KMLMarkerStyle(const char *name, const char *url = NULL, double scale = 1, double textscale = 0, int ismarker = 0, const char *extra = NULL);
CString KMLLineStyle(COLORREF color, int width = -1);
CString KMLLine(const char *name, const char *desc, vara &list, int color, int width = 1, const char *extra = NULL, const char *extrastyle = NULL);
CString KMLBox(const char *name, const char *desc, LLRect &box, int color, int width, const char *extra = NULL, const char *extrastyle = NULL);
CString KMLRect(const char *name, const char *desc, LLRect &box, int color, int width = 1, int areacolor = -1);
CString KMLPoly(const char *name, const char *desc, vara &list, int color, int width = 1, int areacolor = -1);
CString KMLName(const char *name, const char *desc = NULL, int snippet = FALSE);
CString KMLLink(const char *name, const char *desc, const char *url);
CString KMLClickMe(const char *param);
CString KMLScanBox(const char *param, const char *cookies = NULL, const char *status = NULL);

#define M_PI 3.14159265358979323846
#define m2ft 3.28084
#define km2mi 0.621371
#define mi2ft 5280

double Distance(double lat1, double lng1, double lat2, double lng2);

CString ElevationFilename(const char *url);
CString ElevationPath(const char *name);

int ElevationQuery(const char *params, CKMLOut &out, CStringArrayList &filelist);
int ElevationPoint(const char *params, CKMLOut &out);
void ElevationProfile(TCHAR *argv[]);
void ElevationLegend(const char *file);
void ElevationSaveDEM(const char *file);
void ElevationSaveScan(const char *file);
void ElevationSaveScan(CKMLOut &out);
void ElevationFlush(void);

int FACEBOOK_Login(DownloadFile &f, const char *name = "Ropewiki Conditions", const char *user = "ropewikirobot@gmail.com", const char *pass = "conditions4all");

void DownloadPanoramioUrl(int runmode);
void DownloadFlickrUrl(int runmode);

class CSym;
class CSymList;
void DownloadRiversUS(int mode, const char *folder);
void DownloadRiversUSUrl(int mode, const char *folder);
typedef BOOL gmlread(const char *str, CSym &sym, LLRect &bbox);
typedef BOOL gmlfix(const char *str, CSymList &sym);
void DownloadRiversGML(const char *poifolder, const char *gmlfolder, gmlread read, gmlfix fix = NULL);
void CalcRivers(const char *poifolder, const char *folder, int join, int simplify, int reorder, const char *startfile = NULL);
void TransRivers(const char *outfolder, const char *folder, int mode);

void LoadWaterflow(CSymList &list, int active = TRUE);

void GetFileList(CSymList &list, const char *folder, const char *exts[], int recurse = FALSE);

int GetCacheMaxDays(const char *kmzfile);

void DownloadPOI(void);
void DownloadRWLOC(const char *strdays);

void DownloadDEM();

void SaveKML(const char *name);
void SaveCSVKML(const char *name);

void CheckCSV(void);
void CheckKMZ(void);
void CompareCSV(const char *a, const char *b);
void CopyCSV(const char *a, const char *b);
void CopyDEM(const char *a, const char *b);
void FixRivers(const char *folder);
void CheckRiversID(const char *folder);
void GetRiversElev(const char *folder);
void GetRiversWG(const char *folder);
void FixJLink(const char *folder, const char *match);
void FixPanoramio(const char *outfolder, const char *folder);

CString NetworkLink(const char *name, const char *url);

CString alink(const char *link, const char *text = NULL, const char *attr = NULL);
CString jlink(const char *link, const char *text = NULL);

void IdCanyons(LLRect *bbox, const char *id);
void ScoreCanyons(const char *config, const char *cookies, CKMLOut *out, inetdata *data);
void ScanCanyons(const char *file, LLRect *bbox);
void ScanCanyons(void);
void ScanAdd(const char *file, const char *user, int priority = 1);


vara GetRiversPointSummary(const char *coords);

int MoveBeta(int mode, const char *file);
int PurgeBeta(int mode, const char *file);
void RefreshBeta(int mode, const char *file);
int UploadPics(int mode, const char *file);
int UploadBeta(int mode, const char *file);
int UploadRegions(int mode, const char *file);
int UploadStars(int mode, const char *code);
int UploadConditions(int mode, const char *code);
int DownloadFacebookPics(int mode, const char *fbname);
int UploadFacebookConditions(int mode, const char *fbname);
int DownloadFacebookConditions(int mode, const char *file, const char *fbname);
int UpdateBetaBQN(int mode, const char *ynfile);
int UpdateBetaCCS(int mode, const char *ynfile);
int DownloadBeta(int mode, const char *code);
int DownloadBetaKML(int mode, const char *file);
int FixBetaKML(int mode, const char *file);
int UploadBetaKML(int mode, const char *file);

int CheckBeta(int mode, const char *code);
int UndoBeta(int mode, const char *logfile);
int ExtractBetaKML(int MODE, const char *url, const char *file);
int FixBeta(int MODE, const char *query);
int FixBetaRegion(int MODE, const char *fileregion);
int FixConditions(int MODE, const char *query);
int QueryBeta(int MODE, const char *query, const char *file);
int ListBeta(void);
int DisambiguateBeta(int MODE, const char *movefile);
int ReplaceBetaProp(int MODE, const char *value, const char *query);
int ResetBetaColumn(int MODE, const char *value);
int SimplynameBeta(int mode, const char *file, const char *movefile);
int DeleteBeta(int mode, const char *file, const char *comment);
int TestBeta(int mode, const char *column, const char *code);
int ConvertPDFHTML(const char *pdffile, const char *html);

typedef int rwfunc(const char *line, CSymList &list);
CString GetASKList(CSymList &idlist, const char *query, rwfunc func);
CString GetAPIList(CSymList &idlist, const char *query, rwfunc func);

float dmstodeg(double dd, double mm, double ss);
float dmstodeg(const char *str, char spc = ' ');


double ExtractNum(const char *mbuffer, const char *searchstr, const char *startchar  = "\"", const char *endchar = "\"");
//void convertUTMToLatLong(const char *szone, const char *utmx, const char *utmy, double &latitude, double &longitude);
//void UTM2LL(const char *utmZone, double utmX, double utmY, double &latitude, double &longitude);
void UTM2LL(const char *utmZone, const char *utmX, const char *utmY, float &latitude, float &longitude);
char UTMLetterDesignator(double Lat);
void UTM2LL(const char* UTMZone, const double UTMNorthing, const double UTMEasting, float& Lat,  float& Long );
void LL2UTM(const double Lat, const double Long, double &UTMNorthing, double &UTMEasting, char* UTMZone);
int ComputeCoordinates(const char *coords, double &length, double &depth, LL se[2]);

int GetTop40(const char *year, const char *pattern);
int GetCraigList(const char *keyfile, const char *htmfile);



vars htmltrans(const char *string);
