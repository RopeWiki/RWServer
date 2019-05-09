#include <afxinet.h>
#include <atlimage.h>
#include "stdafx.h"
#include "trademaster.h"
#include "rw.h"
#include "math.h"
#ifdef GDAL
#include "gdal_priv.h"
#include "cpl_conv.h" // for CPLMalloc()
#include "gdiplusimaging.h"
//ogr_spatialref.h"
#include "ogr_srs_api.h"
#endif

#include "passwords.h"

#define USEDIRECT // direct (up down) rivers as a group
#define SHOWSCAN	// use scan info to display scan raps
#define SCANRAPS	// include scan rappels in output

#define SCANVERSION 9
#define GOOGDAYS (30*12) // google data good for 6months
#define SCANDAYS (30*12) // scan data good for 6months

// PROBLEMS & TEST CASES
// [ok] Lake messup 46.297730 -122.168399
// [ok] Moved Up 46.335420 -122.123376 -> 46.333540 -122.124094    Rd34d3a99a2c9eb41
// [ok] Flow down  46.087674, -122.207340
// [ok] Move up too much 46.202576,-121.431510
// [ok] Lake overlap 46.520952,-121.485013
// [ok] Weird shit with lakes 46.333138 -122.107888
// [?] rap at fork joint  46.327852°, -122.095396°
// 44.160114 -121.884125 76m1
// 44.160110 -121.884148 86m2


#ifdef DEBUG
//#define NOGOOGLEELEV
#endif
//#define FORCEGOOGLEELEV

//#define CRASHPROTECTION

extern int INVESTIGATE, TMODE;

static int groundHeightCnt;

int CRASHID = -1;
#define CRASHTEST(n) /*CRASHID=n*/

#define CTEST(str) /*printf("%s  \r", str)*/

// -getriversus RIVERSUS\GML
// -getriversmx RIVERSMX\GML
// -getriversca RIVERSCA\GML
// -simplify -calcrivers RIVERSCA\GML POI\RIVERSCA

/*
void CRASHDEMARRAY(tcoord *coord)
{
#ifndef DEBUG
	__try { 
#endif

				DEMARRAY.height(coord);
#ifndef DEBUG
} __except(EXCEPTION_EXECUTE_HANDLER) 
	{ 
	Log(LOGALERT, "DEMARRAY CRASHED!!! ID=%d", CRASHID); 	
	}
#endif
}
*/

//#define DEBUGPLANB	// disable PlanB unless specified
//#define DEBUGX		// Xtra DEBUG traces and marks
//#define DEBUGTHERMAL

const char *AMP = "&amp;";

#define USEPROJECTION  // use projected DEMs?

#define SCANTMP "SCAN\\TMP"
#define SCANFINAL "SCAN\\FINAL"
#define SCANTRK "SCAN\\TRK"
#define scanname(file, path) MkString("%s\\%s.csv", path, file)
#define SCANFILE "SCAN.csv"
#define TRK ".trk"

#ifdef DEBUG
#define SCANFLUSHTIME (60) // 60s
#else
#define SCANFLUSHTIME (60*5) // 5 min
#endif

#define DEMLIST "DEM.csv"
#define DEMCREDIT "DEMCREDIT.csv"
#define DEMFOLDER "DEM,D:\\DEM" //,T:\\DEM"

//Download cache
#define MAXCACHE (1000*1024.0*1024.0)
#define CACHE "CACHE"
#define CACHESUBNUM (1<<10)

#define RWZ "KMZ"
#define RWZSUBNUM (1<<8)

#define RWLOC "RWLOC"
#define RWLOCOPT "GLC"
#define POIOPT "GPC"

#define MAXELEV 9000	// 9000m

#define MAXTHREADRIVERS 5
#define RIVERSBASE "RIVERS"
#define RIVERSUS RIVERSBASE"US"
#define RIVERSCA RIVERSBASE"CA"
#define RIVERSMX RIVERSBASE"MX"
static const char *RIVERSALL[] = { RIVERSUS, RIVERSCA, RIVERSMX, NULL };
#define MINELEVDIFF 10 //10m

#define POI "POI"
#define POIGRIDSIZE 5
#define MAXDIRDAYS 30
#define MAXPANORAMIOQUERY 50000
#define MAXFLICKRQUERY 50000
#define MAXRIVERSUSDAYS 60
#define MAXEXPIRE (365*10)	// never expire
#define MINHIGHRES 7

#define deg2g(x) (tan((x)*M_PI/180))
#define g2deg(x) (atan((x))/M_PI*180)

//#define MINALT		// minimize altitude
#define MINSTEP ((double)5)  // 5m

//#define BSHARP
//#define BSMOOTH
//#define BDELETE
#define BORIG
#define BREGRADE
//#define BINTERP
//#define BORIG2
//#define BDEM0
//#define BLAT


#define MAXCONNECTION 50
#define MAXCLIMBING 3    // 5 steps
#define MAXESCAPING 30    // 25 steps
#define MAXSTALLDIST 100    // 100m going around in circles (<
#define MAXGUIDEDIST 150 // 150m
#define MAXSPLITDIST MAXGUIDEDIST 
#define MAXSPLITS 10	// stream can split up to 10 times
#define MAXPRERAPPEL 150 // 100m
#define MAXUPRIVER 50
#define MAXDOWNRIVER (MAXGUIDEDIST*2)
#define MINRIVERPOINT  500  // 500m
#define MINRIVERJOIN  20  // 20m
#define MAXRIVERJOIN  1500 // 1mi
#define MAXCONNDIST 10000	// 6mi
#define MAXSCOREDIST 2000 // 3mi
#define MAXWGDIST 100 // 100m
#define MAXONRAPDIST (MAXWGDIST/2) // 50m
//#define MAXREGIONSIZE 160000 // 100mi
//#define MAXSTEPS 10
#define AUTODISTANCE 2000 // 2.0km
#define AUTOMAXDISTANCE 10000 // 10km
#define AUTOMINGRADE deg2g(7)
//#define MINALTM 0
#define MINRAP 5
//#define MINRAPDIST (MINSTEP*2)
enum { AMIN = -1, APIXEL, ABILINEAL, ABICUBIC, ASPLINE, GF1, GF2, GF3 };
#define ALGORITHM 2 // 0:Pixel 1:Bilineal 2:Bicubic 3:spline (not working very well)
#define MAXOPENDEM 20
#define MAXOPENSYM 10
// minimum high angle slope

#if 0
#define ANGM 2
#define ANG0 27
#define ANG1 30
#define ANG2 45
#define ANG3 60
#define ANG4 75
#else
#define ANGM 2
#define ANG0 25
#define ANG1 30
#define ANG2 40
#define ANG3 60
#define ANG4 80
#endif

enum { WSORDER, WSLEVEL, WSCFS1, WSCFS2, WSAREA1, WSAREA2, WSTEMP1, WSTEMP2, WSPREP1, WSPREP2, WSMAX };

double FLOATVAL = 0;
static CString filename(const char *name, const char *subfolder = NULL)
{
	const char *fileext[] = { KML, CSV, PNG, NULL };
	CString path = POI;
	if (subfolder && IsSimilar(subfolder, RIVERSBASE))
		path = RIVERSBASE;
	if (subfolder)
		path += "\\"+vars(subfolder);
	if (name && *name)
		{
		path += "\\"+vars(name);
		const char *ext = GetFileExt(name);
		if (!ext || !IsMatchSimilar(ext-1,fileext,TRUE))
			path += ".csv";
		}
	return path;
}


int LoadFile(CString &data, const char *file)
{
	CFILE f;
	if (f.fopen(file))
		{
		int size = f.fsize();
		char *buffer = data.GetBufferSetLength( size );
		f.fread(buffer, size, 1); 
		}
	return data.GetLength();
}

CString alink(const char *link, const char *text, const char *attr)
{
	if (text==NULL)
		text = link;
	return MkString("<a href=\"%s\" %s>%s</a>", link, attr ? attr : NULL, text);
}

CString jref(const char *link)
{
#if 1
	return link;
#else
	//\"about:blank\" onclick=\"window.open('"+url1+"', '_blank')
	//desc += "<li><a href=\"about:blank\" onclick=\"window.open('"+url3+"', '_blank');\"></a></li>";		
	//CString ret("javascript:window.open('');\" onclick=\"window.open('");
	CString ret("javascript:window.open('");
	ret += link;
	ret +="');";
	return ret;
#endif
}

CString jlink(const char *link, const char *text)
{
	if (text==NULL)
		text = link;
	CString ret = "<a href=\"";
	ret += jref(link);
	ret += "\">";
	ret += text;
	ret += "</a>";
	return ret;
}


CString cdatajlink(const char *link, const char *text = NULL)
{
	return CDATAS+jlink(link)+CDATAE;
}


const double GRADE[] = { deg2g(ANG0), deg2g(ANG1), deg2g(ANG2), deg2g(ANG3), deg2g(ANG4) };
const double GRADEM[] = { deg2g(ANG0), deg2g(ANG1-ANGM), deg2g(ANG2-ANGM), deg2g(ANG3-ANGM), deg2g(ANG4-ANGM) };


#define isNaN(v) (v==InvalidNUM)
#define InvalidAlt -1000
COLORREF hsv2rgb(double h, double s, double v, double a = 0);


static CString numtoi(double val, double mul = 1, const char *units = NULL)
{
	if (val==InvalidNUM)
		return "";
	CString str = MkString("%d", (int)(val*mul+0.5));
	if (units) str += units;
	return str;
}


inline int resnum(double res) { return (int)max(0, min(2, (res+3)/10)); }   // (r+3)/10  1-7 H 7-17 M 17+ L

DWORD resColor(double res)
{
	if (res<=0)
		return RGB( 0, 0, 0);
	const COLORREF colors[] = { RGB(0, 0xFF,0), RGB(0xFF, 0xFF,0), RGB(0xFF, 0, 0) };
	return colors[ resnum(res) ];
}


#define EARTHRADIUS 6378137

CString CCoordS(const char *str)
{
	vara coords(str, ";");
	return coords[1]+","+coords[0];
}

class vector
{
public:
	double x, y;
	vector(void) { x = y =0; }
	vector(double x, double y) { this->x = x, this->y = y; }
	vector operator*(double m) { return vector( this->x * m, this->y * m); }
	vector operator+(const vector &other) { return vector(this->x + other.x, this->y + other.y); }
	vector operator-(const vector &other) { return vector(this->x - other.x, this->y - other.y); }
};

vector operator*(double m, const vector &other) { return vector( other.x * m, other.y * m); }


double BiCubicKernel(register double dfVal)
{
		if ( dfVal > 2.0 )
			return 0.0;
		
		double a, b, c, d;
		double xm1 = dfVal - 1.0;
		double xp1 = dfVal + 1.0;
		double xp2 = dfVal + 2.0;
		
		a = ( xp2 <= 0.0 ) ? 0.0 : xp2 * xp2 * xp2;
		b = ( xp1 <= 0.0 ) ? 0.0 : xp1 * xp1 * xp1;
		c = ( dfVal   <= 0.0 ) ? 0.0 : dfVal * dfVal * dfVal;
		d = ( xm1 <= 0.0 ) ? 0.0 : xm1 * xm1 * xm1;
		
		return ( 0.16666666666666666667 * ( a - ( 4.0 * b ) + ( 6.0 * c ) - ( 4.0 * d ) ) );
		// 1/6 * [xp2^3- 4*xp1^3 + 6*x0^3 - 4*xm1^3]

}



typedef double tsmooth(double t, double P0, double P1, double P2, double P3);
	
double bicubic(double t, double P0, double P1, double P2, double P3)
{
	return BiCubicKernel(-1)*P0+BiCubicKernel(0)*P1+BiCubicKernel(1)*P2+BiCubicKernel(2)*P3;
}


double bezier(double t, double P0, double P1, double P2, double P3)
{
	double t1 = (1-t);
	return t1*t1*t1*P0 + 3*t1*t1*t*P1 + 3*t1*t*t*P2 + t*t*t*P3;
}

double spline(double s, double P0, double P1, double P2, double P3)
{
  // http://cubic.org/docs/hermite.htm 
// double s : from 0 to 1 
  double s2 = s*s;
  double s3 = s*s2;
  double h1 =  2*s3 - 3*s2 + 1;          // calculate basis function 1
  double h2 = -2*s3 + 3*s2;              // calculate basis function 2
  double h3 =   s3 - 2*s2 + s;         // calculate basis function 3
  double h4 =   s3 -  s2;              // calculate basis function 4
  double t,c,b; t=c=b=0;
  double T1 = ((1-t)*(1-c)*(1+b))/2*(P1-P0) + ((1-t)*(1+c)*(1-b))/2*(P2-P1);
  double T2 = ((1-t)*(1+c)*(1+b))/2*(P2-P1) + ((1-t)*(1-c)*(1-b))/2*(P3-P2);
  //double T2 = ((1-t)*(1+c)*(1+b))/2*(P1-P0) + ((1-t)*(1-c)*(1-b))/2*(P2-P1);
  return h1*P1 +                    // multiply and sum all funtions
             h2*P2 +                    // together to build the interpolated
             h3*T1 +                    // point along the curve.
             h4*T2;
  //return p;                            // draw to calculated point on the curve
}


tsmooth *_spline = spline;


void _splinevv(double s, vector P[], vector &p)
{
  // http://cubic.org/docs/hermite.htm
  p.x = _spline(s, P[-1].x, P[0].x, P[1].x, P[2].x);
  p.y = _spline(s, P[-1].y, P[0].y, P[1].y, P[2].y);
  //return p;                            // draw to calculated point on the curve
}


class LLE : public LL
{
public:
	float elev;

	LLE()
	{
		lat = lng = 0;
		elev = -1;
	}

	LLE(double olat, double olng, double oelev = -1)
	{
		lat = (float)olat;
		lng = (float)olng;
		elev = (float)oelev;
	}

	inline LLE(const LLE &other) 
	{ 
		*this = other; 
	}

	float GetElev(void);

	double Id(void)
	{
		return -(lat+90+(lng+180)*1e8);
	}

	int Scan(const char *ll)
	{
		return Scan(GetToken(ll,0,' '), GetToken(ll,1,' '));
	}

	int Scan(const char *slat, const char *slng)
	{
		double lat = CDATA::GetNum(slat);
		double lng = CDATA::GetNum(slng);
		if (lat==InvalidNUM || lng==InvalidNUM)
			return FALSE;
		this->lat = (float)lat;
		this->lng = (float)lng;
		return TRUE;
	}

	CString Summary(BOOL showelev = FALSE)
	{
		CString out = MkString("%.6f %.6f", lat, lng);
		if (showelev) out += MkString(" %.0fm", elev);
		return out;
	}

	inline static double Distance(const LLE *A, const LLE *B)
	{
		return ::Distance(A->lat, A->lng, B->lat, B->lng);
	}
#ifdef DEBUGXXX
	
	inline static int Match(double v1, double v2, double m)
	{
		return abs(v2-v1)<m; // ~100m  1km = 0.0093 ~ 0.001  100m = 0.0001 1m = 0.000001
	}

	inline static int Match(const LLE *A, LLE *B, double m = 1e-4)
	{
		return Match(A->lat, B->lat, m) && Match(A->lng, B->lng, m);
	}
#endif

	/*
	inline static int Direction(const LLE *A, const LLE *B)
	{
		return B->elev > A->elev;
	}
	*/

	inline static int Directional(LLE *A, LLE *B, register float mind = MINELEVDIFF)
	{
		return A->elev+mind>B->elev;
	}
	
	static int cmp(LLE **lle1, LLE **lle2)
	{
		if ((*lle1)->lat>(*lle2)->lat) return 1;
		if ((*lle1)->lat<(*lle2)->lat) return -1;
		if ((*lle1)->lng>(*lle2)->lng) return 1;
		if ((*lle1)->lng<(*lle2)->lng) return -1;
		return 0;
	}

	static int cmpelev(LLE **lle1, LLE **lle2)
	{
		if ((*lle1)->elev > (*lle2)->elev) return -1;
		if ((*lle1)->elev < (*lle2)->elev) return 1;
		return 0;
	}

};

typedef CArrayList<LL> LLLIST;

class tpoint;
class LLEA : public LLE
{
	public:
	 float ang;

	 LLEA() : LLE()
	 {
		 ang = 0;
	 }

/*
	LLEA(double lat, double lng, double elev = -1, double ang = 0) : LLE(lat,lng,elev)
	 {
		this->ang = (float)ang;
	 }

	inline LLEA(const LLEA &other) 
	{ 
		*this = other; 
	}
*/
	inline LLEA(const tpoint &other);
};

#define SIDEGRADE 20  // also 10 may work
#define GRADESIDE(elev) ((elev) /  (SIDEGRADE/2+(SIDEGRADE/2)/MINSTEP))   //10+10/5 = 10+2



class tcoord : public LLEA {
public:
	float res, resg;
	float sidegrade;
	signed char credit;
	//double elevC, elevL, elevR;
	//tcoord& operator=(const tcoord &other) { *this = other; return *this; }
	inline tcoord(const tcoord &other) { *this = other; }
	inline tcoord(double olat, double olng, double oelev = -1, double oang = 0, double ores = -1) { 
		lat=(float)olat; lng=(float)olng; elev=(float)oelev; ang=(float)oang; 
		res=(float)ores; resg=-1; 
		credit=-1;
		sidegrade = 0;
		//elevC = elevR = elevL = -1;
		}
	//inline tcoord(const LLEA &other) : tcoord(other.lat, other.lng, other.elev, other.ang) { }
	tcoord(void) { *this = tcoord(0,0); }

	static int cmpelev(tcoord *lle1, tcoord *lle2)
	{
		if ((lle1)->elev > (lle2)->elev) return -1;
		if ((lle1)->elev < (lle2)->elev) return 1;
		return 0;
	}
};


int Rap(double g);
DWORD RapColor(int g);

class tpoint : public tcoord {
	void Init(void)
	{
		dist = tdist = 0; grade = 0; grade2 =0; rapnum = -1;
	}
public:
	tpoint(void) : tcoord() { Init(); }
	tpoint(tcoord &coord) : tcoord(coord) { Init(); }
	inline tpoint(const tpoint &other) { *this = other; }
	float dist, tdist;
	float grade, grade2;
	int rapnum;

};

int KMLSym(const char *name, const char *desc, const char *coordinates, LLRect &bb, CSym &sym);

typedef CArrayList<tcoord> tcoords;
typedef CArrayList<tpoint> tpoints;

CString KMLCoords(tpoints &coords, int start = 0)
{
	vara kmlcoord;
	for (int i=start; i<coords.GetSize(); ++i)
		kmlcoord.push(CCoord3(coords[i].lat, coords[i].lng));
	return kmlcoord.join(" ");
}


void _splinepv(double s, tpoint P[], vector &p)
{
  // http://cubic.org/docs/hermite.htm
  p.x = _spline(s, P[-1].tdist, P[0].tdist, P[1].tdist, P[2].tdist);
  p.y = _spline(s, P[-1].elev, P[0].elev, P[1].elev, P[2].elev);
  //return p;                            // draw to calculated point on the curve
}


class trap {
public:
	double id;
	float ft, m;
	float dist;
	float mingrade, maxgrade, avggrade, sidegrade;
	int start, end;
};
typedef CArrayList<trap> traplist;


inline BOOL isBetterResG(double res, double resg)
	{
#ifdef FORCEGOOGLEELEV
		return resg>0;
#else
		return resg>0 && res-resg>2; // at least 2m better resolution difference
#endif
	}



inline double rnd(double x, double factor = 1, double round = 0.5)
{
	//int r1 = round(x), r2 =(int)(x+0.5);
	//ASSERT(r1==r2);
	return (floor(x*factor+round))/factor;
}

double Distance(double lat1, double lng1, double lat2, double lng2)
{ 
   register double ra = M_PI/180; 
   register double b = lat1 * ra, c = lat2 * ra, d = b - c; 
   register double g = lng1 * ra - lng2 * ra; 
   register double sd2 = sin(d/2), sg2 = sin(g/2);
   register double f = 2 * asin(sqrt(sd2*sd2 + cos(b) * cos(c) * sg2*sg2)); 
   return f * EARTHRADIUS; 
} 

inline double Distance(const tcoord &coord1, const tcoord &coord2)
{
	return Distance(coord1.lat, coord1.lng, coord2.lat, coord2.lng);
}

inline double sqr(double x) { return x * x; }
inline double dist2(const LLE &v, const LLE &w) { return sqr(v.lng - w.lng) + sqr(v.lat - w.lat); }
inline double DistanceToSegment(const LLE &p, const LLE &v, const LLE &w, double &t) 
{
  LLE prj;
  prj = v; t = 0;
  double l2 = dist2(v, w);
  if (l2 != 0) 
	{
	t = ((p.lng - v.lng) * (w.lng - v.lng) + (p.lat - v.lat) * (w.lat - v.lat)) / l2;
	if (t < 0) 
	  prj = v;
	else
	if (t > 1) 
	  prj = w;
	else
	  prj = tcoord(v.lat + t * (w.lat - v.lat), v.lng + t * (w.lng - v.lng));
	}
  return Distance(p.lat, p.lng, prj.lat, prj.lng);
  //return sqrt(dist2(p,prj));
}


double DistanceToLine(double lat, double lng, tcoords &coords)
{
		double mind = 1e30;
		//if (!Box().Contains(lat, lng))
		//	return mind;

		double t;
		tcoord p(lat, lng);
		for (int i=coords.length()-1; i>0; --i)
			{
			double d = DistanceToSegment(p, coords[i], coords[i-1], t);
			if (d<mind)
				mind = d;
			}
		return mind;
}

double DistanceToLine(double lat, double lng, tpoints &coords)
{
		double mind = 1e30;
		//if (!Box().Contains(lat, lng))
		//	return mind;

		double t;
		tcoord p(lat, lng);
		for (int i=coords.length()-1; i>0; --i)
			{
			double d = DistanceToSegment(p, coords[i], coords[i-1], t);
			if (d<mind)
				mind = d;
			}
		return mind;
}



int IntersectionToLine(tpoints &trk, tpoints &coords, double mind)
{
		double tij;
		int imax = coords.length(), jmax = trk.length();
		int j=1, i=1; 
		while (i<imax && j<jmax)
			{
			if (DistanceToSegment(coords[i], trk[j-1], trk[j], tij)<mind)
				return j;

			if (trk[j].elev>coords[i].elev)
				++j;
			else
				++i;
/*
			if (DistanceToSegment(trk[j], coords[i-1], coords[j], tji)<mind)
				return j;

			if (tij>1)
				++j;
			if (tij<0)
				++i;
			if (tji>1)
				++i;
			if (tji<0)
				++j;
*/
			}
		return -1;
}

void CoordDirection(const tcoord &coord, double theta, double dist, tcoord &newcoord) {
  double dE = dist*sin(theta);
  double dN = dist*cos(theta);
  double c = 2 * M_PI * EARTHRADIUS;
  double newlat = coord.lat + 360 * dN / c;
  double newlng = coord.lng + 360 * dE / (c * cos(newlat * M_PI / 180));
  newcoord = tcoord(newlat, newlng, -1, theta);
}



double Bearing(double lat1, double lng1, double lat2, double lng2)
{
register double ra = M_PI/180; 
register double p1 = lat1 * ra, p2 = lat2 * ra, y1 = lng1 * ra, y2 = lng2 * ra; 
register double y = sin(y2-y1) * cos(p2);
register double x = cos(p1)*sin(p2) - sin(p1)*cos(p2)*cos(y2-y1);
return atan2(y, x);
}

inline double Bearing(const tcoord &coord1, const tcoord &coord2)
{
	return Bearing(coord1.lat, coord1.lng, coord2.lat, coord2.lng);
}





double Distance(tcoords &trkcoords, int imax = -1)
{
	double dist = 0, d = 0;
	int size1 = trkcoords.GetSize()-1;
	if (imax>0) size1 = imax;
	for (int i=1; i<size1; ++i, dist+=d)
		d=Distance(trkcoords[i],trkcoords[i+1]);
	return dist;
}

int DistantPoint(tcoords &trkcoords, double maxdist, tcoord *newpoint = NULL, double *remainer = NULL)
{
	double dist = 0, d = 0;
	int size1 = trkcoords.GetSize()-1;
	for (int i=0; i<size1; ++i, dist+=d)
		if ((dist+(d=Distance(trkcoords[i],trkcoords[i+1])))>maxdist)
			break;
	if (remainer)
		*remainer = maxdist - dist;
	
	int p = i;
	if (newpoint)
		{
		double angle = Bearing(trkcoords[p], trkcoords[p==size1 ? p-1 : p+1]);
		CoordDirection(trkcoords[p], angle, maxdist - dist, *newpoint);
		}
	return p;
}


int DistantPoint(tpoints &trkcoords, double maxdist, tpoint *newpoint = NULL, double *remainer = NULL)
{
	double dist = 0, d = 0;
	int size1 = trkcoords.GetSize()-1;
	for (int i=0; i<size1; ++i, dist+=d)
		if ((dist+(d=Distance(trkcoords[i],trkcoords[i+1])))>maxdist)
			break;
	if (remainer)
		*remainer = maxdist - dist;
	
	int p = i;
	if (newpoint)
		{
		double angle = Bearing(trkcoords[p], trkcoords[p==size1 ? p-1 : p+1]);
		CoordDirection(trkcoords[p], angle, maxdist - dist, *newpoint);
		}
	return p;
}


int ReverseDistantPoint(tcoords &trkcoords, double maxdist, tpoint *newpoint = NULL, double *remainer = NULL)
{
	double dist = 0, d = 0;
	int size1 = trkcoords.GetSize()-1;
	for (int i=size1; i>0; --i, dist+=d)
		if ((dist+(d=Distance(trkcoords[i],trkcoords[i-1])))>maxdist)
			break;
	if (remainer)
		*remainer = maxdist - dist;
	int p = i;
	if (newpoint)
			{
			double angle = Bearing(trkcoords[p], trkcoords[p==0 ? p+1 : p-1]);
			CoordDirection(trkcoords[p], angle, maxdist - dist, *newpoint);
			}
	return p;
}



int ReverseDistantPoint(tpoints &trkcoords, double maxdist, tpoint *newpoint = NULL, double *remainer = NULL)
{
	double dist = 0, d = 0;
	int size1 = trkcoords.GetSize()-1;
	for (int i=size1; i>0; --i, dist+=d)
		if ((dist+(d=Distance(trkcoords[i],trkcoords[i-1])))>maxdist)
			break;
	if (remainer)
		*remainer = maxdist - dist;
	int p = i;
	if (newpoint)
			{
			double angle = Bearing(trkcoords[p], trkcoords[p==0 ? p+1 : p-1]);
			CoordDirection(trkcoords[p], angle, maxdist - dist, *newpoint);
			}
	return p;
}




CString LLRect::Summary()
{
	return MkString( "%s,%s - %s,%s", CCoord(lat1), CCoord(lng1), CCoord(lat2), CCoord(lng2));
}



LLRect LLDistance(double lat, double lng, double dist)
{
	tcoord coord(lat, lng), newcoord;
	LLRect box(lat, lng, lat, lng);
	for (int i=0; i<4; ++i)
		{
		CoordDirection(coord, 2*M_PI*i/4.0, dist, newcoord);
		box.Expand(newcoord.lat, newcoord.lng);
		}
	return box;
}


static int logging = 0;


LLRect *Savebox;


unsigned long crc_32_table[] =
{
    0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
    0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
    0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
    0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
    0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
    0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
    0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
    0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
    0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
    0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
    0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
    0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
    0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
    0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
    0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
    0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
    0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
    0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
    0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
    0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
    0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
    0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
    0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
    0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
    0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
    0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
    0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
    0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
    0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
    0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
    0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
    0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
    0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
    0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
    0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
    0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
    0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
    0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
    0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
    0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
    0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
    0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
    0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
    0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
    0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
    0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
    0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
    0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
    0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
    0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
    0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
    0x2d02ef8dL
}; /* end crc_32_tab[] */

/* #define CRC(crc, v) { crc ^= (v); crc += (v); } */
#define CRC(crc, v) (crc_32_table[((unsigned char) crc ^ (v)) & 0xff] ^ (crc >> 8))

DWORD crc32(const char *buffer, int size, DWORD old = 0)
    {
	int i;
	unsigned char *b = (unsigned char*)buffer;
	for (i=0; i<size; ++i)
		old = CRC(old,b[i]);
	return old;
    }


//
// Map Tiles
//

 /// Determines the map width and height (in pixels) at a specified level
/// of detail.
/// </summary>
/// <param name="levelOfDetail">Level of detail, from 1 (lowest detail)
/// to 23 (highest detail).</param>
/// <returns>The map width and height in pixels.</returns>
inline double MapSize(int levelOfDetail)
{
			if (levelOfDetail<0)
				return (DWORD)EARTHRADIUS*2*M_PI;
            return (DWORD) 256 << levelOfDetail;
}

/// <summary>
/// Clips a number to the specified minimum and maximum values.
/// </summary>
/// <param name="n">The number to clip.</param>
/// <param name="minValue">Minimum allowable value.</param>
/// <param name="maxValue">Maximum allowable value.</param>
/// <returns>The clipped value.</returns>
inline double Clip(double n, double minValue, double maxValue)
{
    return min(max(n, minValue), maxValue);
}

double MinLatitude = -85.05112878;
double MaxLatitude = 85.05112878;
double MinLongitude = -180;
double MaxLongitude = 180;

/// <summary>
/// Determines the ground resolution (in meters per pixel) at a specified
/// latitude and level of detail.
/// </summary>
/// <param name="latitude">Latitude (in degrees) at which to measure the
/// ground resolution.</param>
/// <param name="levelOfDetail">Level of detail, from 1 (lowest detail)
/// to 23 (highest detail).</param>
/// <returns>The ground resolution, in meters per pixel.</returns>
double GroundResolution(double latitude, int levelOfDetail)
{
    latitude = Clip(latitude, MinLatitude, MaxLatitude);
    return cos(latitude * M_PI / 180) * 2 * M_PI * EARTHRADIUS / MapSize(levelOfDetail);
}
		
/// <summary>
/// Converts a pixel from pixel XY coordinates at a specified level of detail
/// into latitude/longitude WGS-84 coordinates (in degrees).
/// </summary>
/// <param name="pixelX">X coordinate of the point, in pixels.</param>
/// <param name="pixelY">Y coordinates of the point, in pixels.</param>
/// <param name="levelOfDetail">Level of detail, from 1 (lowest detail)
/// to 23 (highest detail).</param>
/// <param name="latitude">Output parameter receiving the latitude in degrees.</param>
/// <param name="longitude">Output parameter receiving the longitude in degrees.</param>
void Pix2LL(int levelOfDetail, int pixelX, int pixelY, double &latitude, double &longitude)
{
    double mapSize = MapSize(levelOfDetail);
    double x = (Clip(pixelX, 0, mapSize - 1) / mapSize) - 0.5;
    double y = 0.5 - (Clip(pixelY, 0, mapSize - 1) / mapSize);

    latitude = 90 - 360 * atan(exp(-y * 2 * M_PI)) / M_PI;
    longitude = 360 * x;
}

inline void Pix2LL(int levelOfDetail, int pixelX, int pixelY, float &latitude, float &longitude)
{
	double lat, lng;
	Pix2LL(levelOfDetail, pixelX, pixelY, lat, lng);
	latitude = (float)lat, longitude = (float)lng;
}

/// <summary>
/// Converts a point from latitude/longitude WGS-84 coordinates (in degrees)
/// into pixel XY coordinates at a specified level of detail.
/// </summary>
/// <param name="latitude">Latitude of the point, in degrees.</param>
/// <param name="longitude">Longitude of the point, in degrees.</param>
/// <param name="levelOfDetail">Level of detail, from 1 (lowest detail)
/// to 23 (highest detail).</param>
/// <param name="pixelX">Output parameter receiving the X coordinate in pixels.</param>
/// <param name="pixelY">Output parameter receiving the Y coordinate in pixels.</param>
void LL2Pix(int levelOfDetail, double latitude, double longitude, int &pixelX, int &pixelY)
{

    latitude = Clip(latitude, MinLatitude, MaxLatitude);
    longitude = Clip(longitude, MinLongitude, MaxLongitude);

    double x = (longitude + 180) / 360; 
    double sinLatitude = sin(latitude * M_PI / 180);
    double y = 0.5 - log((1 + sinLatitude) / (1 - sinLatitude)) / (4 * M_PI);

    double mapSize = MapSize(levelOfDetail);
    pixelX = (int) Clip(x * mapSize + 0.5, 0, mapSize - 1);
    pixelY = (int) Clip(y * mapSize + 0.5, 0, mapSize - 1);
}

/// <summary>
/// Converts tile XY coordinates into a QuadKey at a specified level of detail.
/// </summary>
/// <param name="tileX">Tile X coordinate.</param>
/// <param name="tileY">Tile Y coordinate.</param>
/// <param name="levelOfDetail">Level of detail, from 1 (lowest detail)
/// to 23 (highest detail).</param>
/// <returns>A string containing the QuadKey.</returns>
CString QuadKey(int tileX, int tileY, int levelOfDetail)
{
    CString quadKey;
    for (int i = levelOfDetail; i > 0; i--)
    {
        char digit = '0';
        int mask = 1 << (i - 1);
        if ((tileX & mask) != 0)
        {
            digit++;
        }
        if ((tileY & mask) != 0)
        {
            digit++;
            digit++;
        }
        quadKey += digit;
    }
    return quadKey;
}


void Wms(int epsg, double lat, double lng, double &x, double &y)
{
	if (epsg)
	{
	x = lng * 20037508.34 / 180;
	double sy = log(tan((90 + lat) * M_PI / 360)) / (M_PI / 180);
	y = sy * 20037508.34 / 180;
	}
	else
	{
	x = lat;
	y = lng;
	}
}

CString WmsBox(LLRect &b, int epsg, int invert)
{
	double x1, y1, x2, y2;
	if (!invert)
		{
		Wms(epsg, b.lat1, b.lng1, x1, y1);
		Wms(epsg, b.lat2, b.lng2, x2, y2);
		}
	else
		{
		Wms(epsg, b.lat1, b.lng1, y1, x1);
		Wms(epsg, b.lat2, b.lng2, y2, x2);
		}
	//LLRect xy(x1, y1, x2, y2);
	return MkString("%.2f,%.2f,%.2f,%.2f", x1<x2 ? x1 : x2, y1<y2 ? y1 : y2, x1<x2 ? x2 : x1, y1<y2 ? y2 : y1);
}



typedef CString TileFunc(double lat, double lng, int &px, int &py);

CString BingTile(const char *_url, int LOD, double lat, double lng, int &px, int &py)
{
	int x, y;
	LL2Pix(LOD, lat, lng, x, y);
	px = x % 256;
	py = y % 256;
	CString url(_url);
	url.Replace("{Q}", QuadKey(x/256, y/256, LOD));
	return url;
}

CString GoogleTile(const char *_url, int LOD, double lat, double lng, int &px, int &py)
{
	int x, y;
	LL2Pix(LOD, lat, lng, x, y);
	px = x % 256;
	py = y % 256;
	CString url(_url);
	url.Replace("{X}", MkString("%d", x/256));
	url.Replace("{Y}", MkString("%d", y/256));
	url.Replace("{Z}", MkString("%d", LOD));
	return url;
}

CString GoogleFixed(const char *_url, int LOD, double lat, double lng, int &px, int &py)
{
	
	int x, y;
	LL2Pix(LOD, lat, lng, x, y);
	int cd = 640-512;
	px = x % 1024 + cd;
	py = y % 1024 + cd;
	int cx = (x/1024)*1024+512;
	int cy = (y/1024)*1024+512;
	Pix2LL(LOD, cx, cy, lat, lng);

	CString url(_url);
	url += "&size=640x640&scale=2";
	url.Replace("{LAT}", CCoord(lat));
	url.Replace("{LNG}", CCoord(lng));
	url.Replace("{Z}", MkString("%d", LOD-1));
	return url;
}
			


class TileCache {
int modes;
CStringArrayList urls;
CArrayList<CImage*> imgs;

public:
TileCache(void)
{
	modes = 0;
}

~TileCache(void)
{
	for (int i=0; i<imgs.GetSize(); ++i)
		delete imgs[i];
}


#if 0
		f.headers = "Accept: text/html, application/xhtml+xml, */*\n"
			"Accept-Language: en-US,en;q=0.5\n"
			"User-Agent: Mozilla/5.0 (Windows NT 6.3; WOW64; Trident/7.0; Touch; rv:11.0) like Gecko\n"
			"Accept-Encoding: gzip, deflate\n"
			"Connection: Keep-Alive\n"
			"Pragma: no-cache\n";
#endif

int SetModes(int n)
{
	modes = n;
}

int GetModes(void)
{
	return min(modes, 5);
}

COLORREF GetPixel(double lat, double lng, int mode)
{
	int px, py;
	CString url;
	

switch (mode)
	{
		case 1: // Bing
			url = BingTile("http://ak.t0.tiles.virtualearth.net/tiles/a{Q}.jpeg?g=3262&n=z", 17, lat, lng, px, py);
			break;

		case 0: // Google
			//url = BingTile("http://kh.google.com/flatfile?f1-030131233222233310022-i.586
			//url = GoogleTile("http://khm0.google.com/kh/v=142&src=app&hl=en&x={X}&y={Y}&z={Z}", 15, lat, lng, px, py);
			//http://khm0.googleapis.com/kh?v=106&hl=en-US&x={X}&y={Y}&z={Z}&token=121361";
			url = GoogleFixed("http://maps.googleapis.com/maps/api/staticmap?center={LAT},{LNG}&zoom={Z}&maptype=satellite", 17, lat, lng, px, py);
			break;

		case 2: // ESRI
			url = GoogleTile("http://services.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{Z}/{Y}/{X}.png", 17, lat, lng, px, py);
			break;

		case 3: // Nokia
			//url = GoogleTile("http://maptile.mapplayer1.maps.svc.ovi.com/maptiler/maptile/newest/satellite.day/{Z}/{X}/{Y}/256/png8", 17, lat, lng, px, py);
			url = GoogleTile("https://4.aerial.maps.api.here.com/maptile/2.1/maptile/4b91ab9d1b/hybrid.day/{Z}/{X}/{Y}/256/png8?app_id=xWVIueSv6JL0aJ5xqTxb&app_code=djPZyynKsbTjIUDOBcHZ2g&lg=eng&ppi=72", 17, lat, lng, px, py);			
			break;
			
		case 4: // MapBox
			url = GoogleTile("http://b.tiles.mapbox.com/v4/openstreetmap.map-inh7ifmo/{Z}/{X}/{Y}.png?access_token=pk.eyJ1Ijoib3BlbnN0cmVldG1hcCIsImEiOiJhNVlHd29ZIn0.ti6wATGDWOmCnCYen-Ip7Q", 17, lat, lng, px, py);			
			break;

		//case 3: // MapQuest, too low res
			//url = GoogleTile("http://otile1.mqcdn.com/tiles/1.0.0/sat/{Z}/{X}/{Y}.jpg", 17, lat, lng, px, py);			
			//break;			 
	}


	int i = urls.Find(url);
	if (i<0)
		{
		// add image
		i = imgs.GetSize();
		CImage *img = new CImage;
		imgs.AddTail(img);
		urls.AddTail(url);

		DownloadFile f;
		CString file = MkString("TILE.JPG", i);
		if (f.Download(url, file))
			Log(LOGERR, "Error downloading tile %s", url);
		else
			if (img->Load(file))
				Log(LOGERR, "Error loading tile %s", url);
		}
	if (imgs[i]->IsNull())
		return RGB(0,0,0);
	if (px>=imgs[i]->GetWidth() || py>=imgs[i]->GetHeight())
		return RGB(0,0,0);
	return imgs[i]->GetPixel(px, py);
}

} tilecache;

/*
http://ak.t1.tiles.virtualearth.net/tiles/a023013201333221.jpeg?g=3262&n=z
https://khms1.google.com/kh/v=166&src=app&expIds=201527&rlbl=1&x=1431&y=3298&z=13&s=Galileo
*/


typedef CDoubleArrayList vard;

static CSymList demcredits; //Digital Elevation Data by 

class GDALFILE
{ 
  //CString file;
  GDALDataset *poDataSet;
  OGRSpatialReferenceH poSpace, poLatLong;
  OGRCoordinateTransformationH poTransform2LL, poTransform2Pix;

public:
  double G[6]; 
  int projected;
  GDALRasterBand  *band;	
  int invalid;
  int nRasterXSize, nRasterYSize, nRasterCount;
  float xmpp, ympp;

  inline int Invalid(void) { return invalid; }

  GDALFILE(void)
  {
  Init();
  }

  BOOL Open(const char *file, int credit = -1, int project = TRUE)
  {
  Destroy();

  poDataSet = (GDALDataset *) GDALOpen(file, GA_ReadOnly );
  if( poDataSet == NULL )
	  {
	  Log(LOGERR, "ERROR: Invalid IMG file %s ", file);
	  return FALSE;
	  }

  if( poDataSet->GetGeoTransform( G ) != CE_None )
	  {
		Log(LOGERR, "%s irregular transform", file);
		return FALSE;
	  }
   if (credit>0)
		{
		// adjustments
		double dlat = demcredits[credit].GetNum(ITEM_LAT);
		if (dlat!=InvalidNUM)
			G[3] += dlat;
		double dlng = demcredits[credit].GetNum(ITEM_LNG);
		if (dlng!=InvalidNUM)
			G[0] += dlng;
		}


      //OGRSpatialReference 
      //OGRCoordinateTransformation *poTransform = OGRCreateCoordinateTransformation( &poSpace, &poLatLong );
	  //poSpace.importFromWkt(&proj);
	  //poLatLong.SetWellKnownGeogCS( "WGS84" );

   nRasterCount = poDataSet->GetRasterCount();
   band = poDataSet->GetRasterBand( 1 );
   if (!band)
		{
		Log(LOGERR, "DEM: bad band in %s", file);
		return FALSE;
		}

   nRasterXSize = band->GetXSize();
   nRasterYSize = band->GetYSize();

   if(project && poDataSet->GetProjectionRef() != NULL )
	  {
	  poSpace = OSRNewSpatialReference( NULL );
	  poLatLong = OSRNewSpatialReference( NULL );
	  char *proj = (char *)poDataSet->GetProjectionRef();
	  OSRImportFromWkt(poSpace, &proj);
	  OSRSetWellKnownGeogCS(poLatLong, "WGS84"); 
	  
	  poTransform2LL = OCTNewCoordinateTransformation( poSpace, poLatLong);
	  poTransform2Pix = OCTNewCoordinateTransformation( poLatLong, poSpace);
	  if( poTransform2LL == NULL || poTransform2Pix == NULL)
		{
		Log(LOGERR, "%s invalid transform", file);
		}
	  else
	  {
	  register const double eps = 1e-5;
	  double x, y, ox, oy;
	  ox = x = 0, oy = y = 0;
	  //if( !poTransform->Transform( 1, &x, &y ) ) {
	  if( poTransform2LL!=NULL)
	   if (!OCTTransform(poTransform2LL, 1, &x, &y, NULL)) {
		Log(LOGWARN, "%s invalid transform 0,0", file);
		}
	  if (abs(x-ox)>eps || abs(y-oy)>eps)
		 ++projected;
	  ox = x = nRasterXSize, oy = y = nRasterYSize;
	  //if( !poTransform->Transform( 1, &x, &y ) ) {
	  if( poTransform2LL!=NULL)
	   if (!OCTTransform(poTransform2LL, 1, &x, &y, NULL)) {
		Log(LOGWARN, "%s invalid transform max,max", file);
		}
	  if (abs(x-ox)>eps || abs(y-oy)>eps)
		 ++projected;

	  if (!projected)
		{
  		if (poTransform2LL) OCTDestroyCoordinateTransformation( poTransform2LL );
		if (poTransform2Pix) OCTDestroyCoordinateTransformation( poTransform2Pix );
		poTransform2LL = poTransform2Pix = NULL;
		}
	  }
	  }

     // lat/lng
  double lat1, lng1, lat2, lng2;
  Pix2LL(0, 0, lat1, lng1);
  Pix2LL(nRasterXSize, nRasterYSize, lat2, lng2);

  if (!CheckLL(lat1,lng1,NULL) || !CheckLL(lat2,lng2,NULL))
	{
	Log(LOGERR, "ERROR: Invalid IMG file %s, invalid bounding box %g,%g - %g,%g", file, lat1, lng1, lat2, lng2);
	return FALSE;
	}

   double latm = Distance(lat1, lng1, lat2, lng1);
   double lngm = Distance(lat1, lng1, lat1, lng2);
   ympp = (float)(latm/nRasterYSize);
   xmpp = (float)(lngm/nRasterXSize);

   invalid = FALSE;
   return TRUE;
   }

   void Init(void)
   {
      //file = "";
	  projected = 0;
	  poSpace = poLatLong = NULL;
	  poTransform2LL = poTransform2Pix = NULL;
	  poDataSet = NULL;
	  band = NULL;
	  nRasterXSize = nRasterYSize = nRasterCount = 0;
      invalid = TRUE;
   }

   void Destroy(void)
   {
  	  if (poTransform2LL) OCTDestroyCoordinateTransformation( poTransform2LL );
	  if (poTransform2Pix) OCTDestroyCoordinateTransformation( poTransform2Pix );
	  if (poSpace) OSRDestroySpatialReference(poSpace);
	  if (poLatLong) OSRDestroySpatialReference(poLatLong);
	  if (poDataSet) GDALClose(poDataSet);
	  Init();
	}


   ~GDALFILE(void)
   {
	   Destroy();
   }

void inline Pix2LL(double Xpixel, double Yline, double &lat, double &lng)
{
   double x = Xpixel+0.5;
   double y = Yline+0.5;
   lng = G[0] + x*G[1] + y*G[2];
   lat = G[3] + x*G[4] + y*G[5];
#ifdef USEPROJECTION
   if (poTransform2LL)
	   OCTTransform(poTransform2LL, 1, &lng, &lat, NULL);
#endif
}

void inline LL2Pix(double lat, double lng, int &Xpixel, int &Yline, double *Xp = NULL, double *Yp = NULL)
{
  //lng/G[1] - G[0]/G[1] = Xpixel;
  //lat/G[5] - G[3]/G[5] = Yline;
#if 1
#ifdef USEPROJECTION
  if (poTransform2Pix)
	  OCTTransform(poTransform2Pix, 1, &lng, &lat, NULL);
#endif
//  double x =( G[5]/2 + ( lng - G[0]  - lat*G[2]/G[5] + G[3]*G[2]/G[5] )  / (G[1] - G[4]*G[2]/G[5]));
//  double y = ( G[1]/2 + ( lat - G[3] - lng*G[4]/G[1] + G[0]*G[4]/G[1] ) / (G[5] - G[2]*G[4]/G[1]));  
  register double G1524 = G[1]*G[5] - G[2]*G[4];
  double x = /* G[5]/2 + */ ( lng*G[5] - G[0]*G[5]  - lat*G[2] + G[3]*G[2] )  / G1524;
  double y = /*G[1]/2 +*/ ( lat*G[1] - G[1]*G[3] - lng*G[4] + G[0]*G[4] ) / G1524;  
//  double y = G[1]/2 + ( lat*G[1] - G[3]*G[1] - lng*G[4] + G[0]*G[4] ) / (G[5]*G[1] - G[2]*G[4]);  
  Xpixel = (int)x;
  Yline = (int)y;
  if (Xp)
	  *Xp = x - Xpixel;
  if (Yp)
	  *Yp = y - Yline;
#else
  Xpixel =(int) ( G[5]/2 + lng/G[1] - G[0]/G[1]);
  Yline = (int) ( G[1]/2 + lat/G[5] - G[3]/G[5]);
#endif
}



int Check(const char *file)
{
  int err = 0, err2 = 0;
  int maxx = 100; //nRasterXSize;
  int maxy = 100; //nRasterYSize;

  double lat, lng, dlat, dlng;
  Pix2LL(maxx/2, maxy/2, lat, lng);
  Pix2LL(maxx/2+1, maxy/2+0, dlat, dlng);
  double dfixed = Distance(lat, lng, dlat, dlng);

  for (int y=0; y<maxy; ++y)
    for (int x=0; x<maxx; ++x)
	 {
	 double xlat, ylng;
	 int x2, y2;
     Pix2LL(x, y, xlat, ylng);
     LL2Pix(xlat, ylng, x2, y2);
	 if (x2!=x || y2!=y)
		 ++err;
	 }
/*
  int S = 1;
  for (int y=S; y<maxy-S; ++y)
    for (int x=S; x<maxx-S; ++x)
	  {
	  double xlat, ylng;
      Pix2LL(DEMTransform, x, y, xlat, ylng);
	  for (int dy=-S; dy<=S; ++dy)
        for (int dx=-S; dx<=S; ++dx)
		   {
		   double xclat, yclng;
 		   int xc=x+dx, yc=y+dy;
           Pix2LL(DEMTransform, xc, yc, xclat, yclng);
		   double d1 = Distance(xlat, ylng, xclat, yclng);
		   double d2 = Distance(xclat, yclng, xlat, ylng);
		   double d12 = abs(d1-d2);
		   if (d12>1e-5)
			   ++err2;
		   }
	   }
*/
  if (err+err2>0)
	  Log(LOGALERT, "inconsistent transform in %s err:%d err2:%d", file, err, err2);

   return (err + err2);
}

int getval(int dX, int dY, float &h)
{
	return band->RasterIO(GF_Read, dX, dY, 1, 1, &h, 1, 1, GDT_Float32, 0, 0)== CE_None;
}

int SaveThermal(const char *_file, int res = 100)
{
  CString file(_file);
  int err = 0, err2 = 0;
  int maxx = poDataSet->GetRasterXSize();
  int maxy = poDataSet->GetRasterYSize();

  double lat1, lng1, lat2, lng2;
  Pix2LL(0, 0, lat1, lng1);
  Pix2LL(maxx, maxy, lat2, lng2);

  double minlat = min(lat1, lat2);
  double maxlat = max(lat1, lat2);
  double minlng = min(lng1, lng2);
  double maxlng = max(lng1, lng2);

  CImage c;
  double hmax, hlat, hlng;
  hmax = hlat = hlng = 0;
  int maxpx, maxpy;
  maxpx = maxpy = res;
  double lngstep = ((maxlng-minlng)/maxpx);
  double latstep = ((maxlat-minlat)/maxpy);
  c.Create(maxpx, maxpy, 24);
  for (int x=0; x<maxpx; ++x)
		for (int y=0; y<maxpy; ++y)
			{
			int dX, dY;
			float h = -1;
			double lng = minlng + x * lngstep;
			double lat = minlat + y * latstep;
			LL2Pix(lat, lng, dX, dY);
			if (dX >= 0 && dY >= 0 && dX < maxx && dY < maxy)
				if (!getval(dX, dY, h))
					h = -1;
			if (h>hmax) hmax = h, hlat = lat, hlng = lng;
			h = max(min(h/5000, 1), -1);
			c.SetPixel(x, maxpy-y-1, hsv2rgb(360*h, 100, (h<0 || h>1) ? 0 : 100) );
			}
  //c.ReleaseDC();
  c.Save(file+PNG);

  CKMLOut out(file+KML);
  out += "<GroundOverlay>";
  out += MkString("<Icon><href>%s</href><viewRefreshMode>onRegion</viewRefreshMode></Icon><drawOrder>10</drawOrder>", file+PNG);
  out += MkString("<LatLonBox><south>%.6f</south><west>%.6f</west><north>%.6f</north><east>%.6f</east></LatLonBox>", minlat, minlng, maxlat, maxlng);
  out += "</GroundOverlay>";
/*
  for (int r=0; r<5; ++r)
	{
	double rng = step, rlat = hlat, rlng = hlng;
	step = step / 10;
	for (double lat=rlat-rng; lat<rlat+rng; lat+=step)
			for (double lng=rlng-rng; lng<rlng+rng; lng+=step)
				{
				int dX, dY;
				float h = -1;
				LL2Pix(lat, lng, dX, dY);
				if (dX >= 0 && dY >= 0 && dX < maxx && dY < maxy)
					{
					CPLErr eErr = band->RasterIO(GF_Read, dX, dY, 1, 1, &h, 1, 1, GDT_Float32, 0, 0);
					if(eErr != CE_None)
						h = -1;
					}

				if (h>hmax) hmax = h, hlat = lat, hlng = lng;
				}
	}
*/

  Log(LOGINFO, "Dumping THERMAL %s hmax:%g @ %g,%g", file, hmax, hlat, hlng);

  return TRUE;
}

	inline BOOL InvalidElev(register int n, float data[])
	{
		for (register int i =0; i<n; ++i)
			if (data[i]>=MAXELEV || data[i]<=InvalidAlt)
				return TRUE;
		return FALSE;
	}


	double heightval(double dfX, double dfY, int alg = ALGORITHM)
	{
				if (alg==0)
					dfX += 0.5, dfY += 0.5;
                int dX = int(dfX);
                int dY = int(dfY);
                double dfDeltaX = dfX - dX;
                double dfDeltaY = dfY - dY;

				if (alg>=GF1)
					{
                    int dXNew = dX - 1;
                    int dYNew = dY - 1;
                    if (dXNew >= 0 && dYNew >= 0 && dXNew + 3 <= nRasterXSize && dYNew + 3 <= nRasterYSize)
						{
							//cubic interpolation
							float anElevData[9] = {0};
							CPLErr eErr = band->RasterIO(GF_Read, dXNew, dYNew, 3, 3,
																	  &anElevData, 3, 3,
																	  GDT_Float32, 0, 0);
							if(eErr != CE_None || InvalidElev(9, anElevData))
								return 0;
						// gradient
						double fx = 0 , fy =0, v2 = sqrt(2.0);
						float gx = xmpp, gy = ympp;
						float &z1 = anElevData[0];
						float &z2 = anElevData[1];
						float &z3 = anElevData[2];
						float &z4 = anElevData[3];
						float &z5 = anElevData[4];
						float &z6 = anElevData[5];
						float &z7 = anElevData[6];
						float &z8 = anElevData[7];
						float &z9 = anElevData[8];
						// 123
						// 456
						// 789
						switch (alg)
							{
							case GF1:
								fy=(z8-z2)/(2*gy); 
								fx=(z6-z4)/(2*gx);
								break;
							case GF2:
								fx=(z3-z1+v2*(z6-z4)+z9-z7)/((4+2*v2)*gx);
								fy=(z7-z1+v2*(z8-z2)+z9-z3)/((4+2*v2)*gy);
								break;
							case GF3:
								fx=(z3-z1+z6-z4+z9-z7)/(6*gx);
								fy=(z7-z1+z8-z2+z9-z3)/(6*gx); 
								break;
							}
						return sqrt(fy*fy+fx*fx);
						}

					return 0;
					}

                
                if(alg == AMIN)
                {
                    int dXNew = dX - 1;
                    int dYNew = dY - 1;
                    if (dXNew >= 0 && dYNew >= 0 && dXNew + 3 <= nRasterXSize && dYNew + 3 <= nRasterYSize)
                    {
						//cubic interpolation
						float anElevData[9] = {0};
						CPLErr eErr = band->RasterIO(GF_Read, dXNew, dYNew, 3, 3,
																  &anElevData, 3, 3,
																  GDT_Float32, 0, 0);
						
						if(eErr != CE_None || InvalidElev(9, anElevData))
							return InvalidAlt;

						double dfSumH(MAXELEV);
						for ( int i = 0; i < 9; i++)
							if (dfSumH>0 && anElevData[i]<dfSumH)
								dfSumH = anElevData[i];
						if (dfSumH==MAXELEV)
							return InvalidAlt;
						return dfSumH;
					}
					alg = ABILINEAL;
                }
                if(alg == ASPLINE)
                {
                    int dXNew = dX - 1;
                    int dYNew = dY - 1;
                    if (dXNew >= 0 && dYNew >= 0 && dXNew + 4 <= nRasterXSize && dYNew + 4 <= nRasterYSize)
                    {
						//cubic interpolation
						float anElevData[16] = {0};
						CPLErr eErr = band->RasterIO(GF_Read, dXNew, dYNew, 4, 4,
																  &anElevData, 4, 4,
																  GDT_Float32, 0, 0);
						if(eErr != CE_None || InvalidElev(16, anElevData))
							return InvalidAlt;
#if						0
						// check pic mapping
						float anElev;
						for ( int x = 0; x < 4; x++ )
							for ( int y = 0; y < 4; y++ )
								{
								CPLErr eErr = band->RasterIO(GF_Read, dXNew+x, dYNew+y, 1, 1,
																		  &anElev, 1, 1,
																		  GDT_Float32, 0, 0);
								ASSERT( anElev== anElevData[y*4+x]);
								}
						
							
						float anElevDataTest[16] = {
							1, 2, 4, 8,
						    3, 6, 12, 24,
						    6, 12, 24, 48, 
						    12, 24, 48, 96};

						memcpy(anElevData, anElevDataTest, sizeof(anElevDataTest));

#endif

						double ret, P[4];
						for ( int i = 0; i < 4; i++ )
							{
							float *p = &anElevData[i*4];
							P[i] = bezier(dfDeltaX, p[0], p[1], p[2], p[3]);
							}
						ret = bezier(dfDeltaY, P[0], P[1], P[2], P[3]);
						return ret;
					}
					alg = ABILINEAL;
                }
                if(alg == ABICUBIC)
                {
                    int dXNew = dX - 1;
                    int dYNew = dY - 1;
                    if (dXNew >= 0 && dYNew >= 0 && dXNew + 4 <= nRasterXSize && dYNew + 4 <= nRasterYSize)
                    {
						//cubic interpolation
						float anElevData[16] = {0};
						CPLErr eErr = band->RasterIO(GF_Read, dXNew, dYNew, 4, 4,
																  &anElevData, 4, 4,
																  GDT_Float32, 0, 0);
						if(eErr != CE_None || InvalidElev(16, anElevData))
							return InvalidAlt;

						double dfSumH(0);
						for ( int i = 0; i < 4; i++ )
						{
							// Loop across the X axis
							for ( int j = 0; j < 4; j++ )
							{
								// Calculate the weight for the specified pixel according
								// to the bicubic b-spline kernel we're using for
								// interpolation
								int dKernIndX = j - 1;
								int dKernIndY = i - 1;
								double dfPixelWeight = BiCubicKernel(dKernIndX - dfDeltaX) * BiCubicKernel(dKernIndY - dfDeltaY);

								//  K(j - 1-deltaX) * K(j-i-deltaY) * P[j + i *4]

								// Create a sum of all values
								// adjusted for the pixel's calculated weight
								float val = anElevData[j + i * 4];

								dfSumH += val * dfPixelWeight;
							}
						}
						return dfSumH;
					}
					alg = ABILINEAL;
                }
                if(alg == ABILINEAL)
                {
                    if (dX >= 0 && dY >= 0 && dX + 2 <= nRasterXSize && dY + 2 <= nRasterYSize)
                    {
						//bilinear interpolation
						float anElevData[4] = {0,0,0,0};
						CPLErr eErr = band->RasterIO(GF_Read, dX, dY, 2, 2,
																  &anElevData, 2, 2,
																  GDT_Float32, 0, 0);
						if(eErr != CE_None || InvalidElev(4, anElevData))
							return InvalidAlt;

						double dfDeltaX1 = 1.0 - dfDeltaX;                
						double dfDeltaY1 = 1.0 - dfDeltaY;

						double dfXZ1 = anElevData[0] * dfDeltaX1 + anElevData[1] * dfDeltaX;
						double dfXZ2 = anElevData[2] * dfDeltaX1 + anElevData[3] * dfDeltaX;
						double dfYZ = dfXZ1 * dfDeltaY1 + dfXZ2 * dfDeltaY;
						return dfYZ;
					}
					alg = APIXEL;
                }
				if (alg == APIXEL)
				{
					if (dX >= 0 && dY >= 0 && dX < nRasterXSize && dY < nRasterYSize)
						{
						float dfDEMH(0);
						CPLErr eErr = band->RasterIO(GF_Read, dX, dY, 1, 1,
																  &dfDEMH, 1, 1,
																  GDT_Float32, 0, 0);
						if(eErr != CE_None || InvalidElev(1, &dfDEMH))
							return InvalidAlt;

						return dfDEMH;
						}              
                }                
			return InvalidAlt;
	}

};




// -1:error 0:ok 1:changed
typedef int symfunc(CSym &sym);

class CSymListThread
{
	//CSection CC;
	int rindex, windex, mindex;
	CSymList *list;
	symfunc *proc;

	public:
	int errors, changes;

	CSymListThread(CSymList &_list, symfunc *_proc, int nthreads)
	{
		this->list = &_list;
		this->proc = _proc;
		errors = changes = 0;
		rindex = 0, windex = 0;
		mindex = list->GetSize();

		if (mindex==0)
			return;

		// fire up threads 
		for (int i=0; i<nthreads; ++i)
			AfxBeginThread(thread,(void*)this);

		// wait to finish
		while (windex<mindex)
			Sleep(1000);
	}

inline void threadloop(void)
	{
	// scan items in the list
	while (TRUE)
		{
		int index;

		// get index
		ThreadLock();
		index = rindex++;
		ThreadUnlock();
		if (index>=mindex)
			break;

		int ret = proc((*list)[index]);

		// done
		ThreadLock();
		if (ret<0) ++errors;
		if (ret>0) ++changes;
		windex++;
		ThreadUnlock();
		}
	}

static UINT thread(LPVOID pParam)
{
	CSymListThread *data = (CSymListThread *)pParam;	
	data->threadloop();
	return 0;
}

};



class DEMFILE;
typedef CArrayList<DEMFILE*> DEMList;
typedef DEMList *DEMListPtr;

class tang {
public:
	int dx, dy; 
	double ang;

	tang(void)
	{
		dx = dy = 0;
		ang = 0;
	}
	tang(int dx, int dy, double ang)
	{
		this->dx = dx, this->dy = dy;
		this->ang = ang;
	}

	static int cmpang(tang *a1, tang *a2)
	{
		if (a2->ang<a1->ang) return 1;
		if (a2->ang>a1->ang) return -1;
		return 0;
	}

	double operator-(const tang &other)
	{
		return other.ang-ang;
	}
};
typedef CArrayList<tang> AngList;

#define VCHECK 0x6666

#ifndef GDAL

class {
 public:
	void height(tcoord *coord, int alg = ALGORITHM, int highres = 1)
	{
	}
} DEMARRAY;

#else
class DEMFILE {
	GDALFILE handle;
	AngList anglist;
	static CArrayList<DEMFILE *> openlist;

 public:
	CString file;
	double res;
	double lat1, lng1, lat2, lng2;
	double date;
	double lasttime;
	int vcheck;
	signed char credit, projected;

	DEMFILE(CSym sym)
	{
		file = sym.id;
		//file = sym.id;
		res = sym.GetNum(ITEM_DESC);
		lat1 = sym.GetNum(ITEM_LAT);
		lng1 = sym.GetNum(ITEM_LNG);
		lat2 = sym.GetNum(ITEM_LAT2);
		lng2 = sym.GetNum(ITEM_LNG2);
		date = sym.GetNum(DEM_DATE);
		projected = sym.GetNum(DEM_PROJ)>0;
		lasttime = 0;
		vcheck = VCHECK;

		// load DEMCREDITS
		credit = -1;
		if (demcredits.GetSize()==0)
			demcredits.Load(filename(DEMCREDIT));
		for (int i=1; i<demcredits.GetSize() && credit<0; ++i)
			if (strstr(file, demcredits[i].id))
				credit = i;
	}

	~DEMFILE(void)
	{
		Close();
	}
	
	int Open(void)
	{
		CRASHTEST(51);	
		if (openlist.GetSize()>MAXOPENDEM)
			Purge();

		CRASHTEST(52);	
		if (!handle.Open(file, credit, projected))
			{
			Log(LOGERR, "DEM: can't open %s", file);
			return FALSE;
			}
		// cache opened DEM
		openlist.AddTail(this);

		CRASHTEST(55);	
		double clat, clng;
		handle.Pix2LL(0, 0, clat, clng);
		for (int dx=-1; dx<=1; ++dx)
			for (int dy=-1; dy<=1; ++dy)
				if (dx!=0 || dy!=0)
					{
					double plat, plng, pang;
					handle.Pix2LL(dx, dy, plat, plng);
					pang = Bearing(clat, clng, plat, plng);
					if (pang<0) pang += 2*M_PI;
					tang t(dx, dy, pang);
					anglist.AddTail(t);
					}
		anglist.Sort(tang::cmpang);
		CRASHTEST(56);	
				
#ifdef DEBUG // test consistency of transformation
		handle.Check(file);
#endif
		return TRUE;
	}

	void Close(void)
	{
		handle.Destroy();
	}

	static int cmptime(DEMFILE **a1, DEMFILE **a2)
	{
		if ((*a1)->lasttime > (*a2)->lasttime) return -1;
		if ((*a1)->lasttime < (*a2)->lasttime) return 1;
		return 0;
	}

	void Purge(void)
	{
#ifdef DEBUG
		//Log(LOGINFO, "purging DEMARRAY %d -> %d", openlist.GetSize(), MAXOPENDEM);
#endif
		openlist.Sort(cmptime);
		ASSERT(openlist[0]->lasttime >= openlist[openlist.GetSize()-1]->lasttime);
		for (int i=openlist.GetSize()-1; i>=0 && i>=MAXOPENDEM; --i)
			{
			openlist[i]->Close();
			openlist.Delete(i);
			}
	}


	inline int inside(register double lat, register double lng)
	{
		CRASHTEST(411);
		return lat>=lat1 && lng>=lng1 && lat<lat2 && lng<lng2;
	}



	inline int height(tcoord *coord, int alg = ALGORITHM)
	 {
	 CRASHTEST(41);
	 if (!inside(coord->lat, coord->lng))
		return FALSE;

	 // open if needed
	 CRASHTEST(42);	
	 lasttime = GetTickCount();
	 if (handle.Invalid())
		if (!Open())
			return FALSE;

	  CRASHTEST(43);	
	  int x, y;
	  double Xp, Yp;
	  handle.LL2Pix(coord->lat, coord->lng, x, y, &Xp, &Yp);
	  CRASHTEST(44);	
	  double elev = handle.heightval(x+Xp, y+Yp, alg);  // add min elev diff for sea level 
	  if (elev<0 || elev>MAXELEV) 
		  return FALSE;

	  // return result
	  CRASHTEST(45);	
	  coord->elev = (float)rnd(elev,10);
	  coord->res = (float)res;
	  coord->credit = credit;
	  //if (alg==0)
	  //  Pix2LL(trans, x, y, coord->lat, coord->lng);

	  return TRUE;

	}


	void CoordDirection(float M[3][3], int x, int y, double ang, double dist, tcoord &newcoord)
	{
		if (ang<0) ang += 2*M_PI;
		int n = (int)(round( ang / (2*M_PI) * 8 )) % 8;
		ASSERT(n>=0 && n<8);
		tang &a = anglist[n];
		ASSERT(abs(a.ang-ang)<M_PI/8 || abs(a.ang+2*M_PI-ang)<M_PI/8);
		x += a.dx;
		y += a.dy;
		double lat, lng;
		handle.Pix2LL(x, y, lat, lng);
		newcoord = tcoord(lat, lng, M[1+a.dy][1+a.dx], a.ang, res);
	}

	int minheight(tcoord &coord, double dirang, double searchang, double dist, tcoords &newcoords)
	{
	 if (!inside(coord.lat, coord.lng))
		return -1;

	 // open if needed
	 lasttime = GetTickCount();
	 if (handle.Invalid())
		if (!Open())
			return -1;

	  int x, y;
	  double Xp, Yp;
	  handle.LL2Pix(coord.lat, coord.lng, x, y, &Xp, &Yp);

	  // check boundaries
	  int d = 1;
	  int d2 = d+d+1;
	  if (x-d<0 || x+d>=handle.nRasterXSize)
		  return -1;
	  if (y-d<0 || y+d>=handle.nRasterYSize)
		  return -1;

	  //double elev = heightval(x+Xp, y+Yp, alg);
	  //if (elev<=0) 
	  //	  return FALSE;

	  //if (alg==0)
	  // Pix2LL(trans, x, y, coord->lat, coord->lng);
	  // get data & interpolate
	  float M[3][3];
	  CPLErr eErr = handle.band->RasterIO( GF_Read, x-d, y-d, d2, d2, M, d2, d2, GDT_Float32, 0, 0 );
	  if(eErr != CE_None)
		return -1;

	newcoords.SetSize(0);
	if (coord.elev<0)
		{
		coord.elev = M[1][1];
		coord.res = (float)res;
		}
    int n = (int)(searchang / (2*M_PI/8));
    tcoord newcoord;
	CoordDirection(M, x, y, dirang, d, *newcoords.push(newcoord));
	//ASSERT(Distance(coord.lat, coord.lng, newcoord.lat, newcoord.lng)>1);
	for (register int i=1, ni=1, m = n/2; i<=m; ++i)
		{
		CoordDirection(M, x, y, dirang + searchang/2*(i/(double)m), d, *newcoords.push(newcoord));
		//ASSERT(Distance(coord.lat, coord.lng, newcoord.lat, newcoord.lng)>1);
		CoordDirection(M, x, y, dirang - searchang/2*(i/(double)m), d, *newcoords.push(newcoord));
		//ASSERT(Distance(coord.lat, coord.lng, newcoord.lat, newcoord.lng)>1);
		}

	// process elevations
	int mini;
	double minelev = newcoords[mini=0].elev;
	double ming = 0; 
	for (int i=1; i<newcoords.length(); ++i)
		{
		ASSERT(newcoords[i].elev>=0);
		//double g = (newcoords[i].elev - minelev) / Distance(coord.lat, coord.lng, newcoords[i].lat, newcoords[i].lng);
		if (newcoords[i].elev<minelev)
		//if (g<ming)
			{
			minelev = newcoords[mini = i].elev;
			//ming = g;
			}
		}
	
	return mini;
	}

};

class LLE;
DEMList DEMFILE::openlist;

class DEMArray {
int init;
DEMList demlist;
DEMList **demarray;
int minlat, minlng, maxlat, maxlng;
CSection CC;

public:
/*
DEMList &list(double lat, double lng)
	{
		return demarray[(int)lat - minlat][(int)lng-minlng];
	}
*/
	inline DEMList &dem(double lat, double lng)
	{
		Init();
		static DEMList empty;
		if (lat<minlat || lat>maxlat || lng<minlng || lng>maxlng)
			return empty;
		return demarray[(int)floor(lat) - minlat][(int)floor(lng) - minlng];
	}

	double DEMDate(LLRect &box)
	{
		Init();
		DEMList &dlist = dem((box.lat1+box.lat2)/2, (box.lng1+box.lng2)/2);
		double date = 0;
		for (int j=0; j<dlist.length(); ++j)
			{
			DEMFILE &f = *dlist[j];
			if (box.Intersects(LLRect(f.lat1, f.lng1, f.lat2, f.lng2)))
				if (f.date>date)
					date = f.date;
			}		
		return date;
	}

	void height(LLE *lle, int alg = ALGORITHM, int highres = 1);
	void height(tcoord *coord, int alg = ALGORITHM, int highres = 1)
	{
		CRASHTEST(0);
		CC.Lock();
		Init();

		coord->elev = InvalidAlt;
		coord->res = -1;
		float &lat = coord->lat;
		float &lng = coord->lng;
		if (lat>=minlat && lat<maxlat && lng>=minlng && lng<maxlng)
		{
		DEMList &dlist = dem(lat, lng);
		for (int j=0; j<dlist.length(); ++j)
			{
			if (!dlist[j]->height(coord,alg))
				continue;
			if (highres)
				break;
			}
		}
		CC.Unlock();
	}

	int minheight(tcoord &coord, double dirang, double searchang, double diststep, tcoords &newcoords, int highres = 1)
	{
		CC.Lock();
		int ret = -1;
		Init();

		int gret = -1;
		double lat = coord.lat;
		double lng = coord.lng;
		if (lat>=minlat && lat<maxlat && lng>=minlng && lng<maxlng)
		{
		DEMList &dlist = dem(lat, lng);
		for (int j=0; j<dlist.length(); ++j)
			{
			int ret = dlist[j]->minheight(coord,dirang,searchang,diststep,newcoords);
			if (ret<0)
				continue;
			gret = ret;			
			if (highres)
				break;
			}		
		}

		coord.lat = (float)lat;
		coord.lng = (float)lng;
		CC.Unlock();
		return gret;
	}


	int Load(CSymList &dlist)
	{
	// create DEM list
	minlat = 1000, minlng = 1000, maxlat = -1000, maxlng = -1000;
	for (int i=0; i<dlist.GetSize(); ++i)
		{
		if (dlist[i].GetStr(ITEM_NUM)=="X")
			continue; // ignore file
		DEMFILE *df = new DEMFILE(dlist[i]);
		demlist.AddTail(df);
		int ilat1 = (int)floor(df->lat1);
		int ilng1 = (int)floor(df->lng1);
		int ilat2 = (int)ceil(df->lat2);
		int ilng2 = (int)ceil(df->lng2);
		if (ilat1<minlat) minlat = ilat1;
		if (ilng1<minlng) minlng = ilng1;
		if (ilat2>maxlat) maxlat = ilat2;
		if (ilng2>maxlng) maxlng = ilng2;
		}

	// create DEM quickarray
	int latsize = (maxlat-minlat);
	int lngsize = (maxlng-minlng);
	if (latsize<=0 || lngsize<=0)
		return FALSE;

	demarray = new DEMListPtr[latsize];	
	for (int i=latsize-1; i>=0; --i)
		demarray[i] = new DEMList[lngsize];

	for (int i=0; i<demlist.GetSize(); ++i)
		{
		DEMFILE *df = demlist[i];
		int ilat1 = (int)floor(df->lat1);
		int ilng1 = (int)floor(df->lng1);
		int ilat2 = (int)ceil(df->lat2);
		int ilng2 = (int)ceil(df->lng2);
		for (int y=ilat1; y<ilat2; ++y)
			for (int x=ilng1; x<ilng2; ++x)
				demarray[y-minlat][x-minlng].AddTail(df);
		}

	return TRUE;
	}

	DEMArray(void)
	{
	init = FALSE;
	GDALAllRegister();  
	}

	void Init()
	{
	  if (init)
		  return;
	  CC.Lock();		
	  if (!init)
	  {
	  CSymList dlist;
	  dlist.Load(filename(DEMLIST));
	  //if (dlist.GetSize()>0)
	  //tilecache.SetModes(2);
	  if (!DEMARRAY.Load(dlist))
		Log(LOGERR, "Could not load DEM list from %s", DEMLIST);
	  init = TRUE;
	  }
	  CC.Unlock();		
	}

	~DEMArray(void)
	{
	for (int i=0; i<demlist.GetSize(); ++i)
		delete demlist[i];

	int ysize = (maxlat-minlat);
	for (int i=ysize-1; i>=0; --i)
		delete [] demarray[i];
	delete [] demarray;
	}

} DEMARRAY;

#endif


//
// CRiver
//
static inline double StrId(const char *str)
	{
	double id = 0;
	if (str && *str!=0)
		FromHex(str+1, &id);
	return id;
	}

static inline CString IdStr(double id)
	{
		return (id<0 ? "P" : "R") + ToHex(&id, sizeof(id) );
	}

class LLEC;
class SEG;
class CRiver;
class CRiverList;

typedef CArrayList<LLEC*> LLECLIST;

void DEMArray::height(LLE *lle, int alg, int highres)
	{
		tcoord c(lle->lat, lle->lng);
		DEMARRAY.height(&c, alg, highres);
		lle->elev = (float)c.elev;
	}

float LLE::GetElev(void)
	{
		if (elev>=0)
			return elev;

		// get new elev
		DEMARRAY.height(this);
		return elev;
	}

class LLER : public LLE
{
	public:
	 float res;

};


class trappel;
class LLEC : public LLE
{ 
public:
	LLEC *lle;
	LLECLIST conn;
	void *ptr;
	LLEC() : LLE()
	{
		lle = NULL;
		ptr = NULL;
	}

	inline CRiver *river() { return (CRiver *)ptr; }
	inline CSym *sym() { return (CSym *)ptr; }
	inline trappel *rappel() { return (trappel *)ptr; }


	LLEC(const LLE &o)
	{
		lat = o.lat;
		lng = o.lng;
		elev = o.elev;
		lle = NULL;
		ptr = NULL;
		ASSERT( lat==o.lat && lng==o.lng);
	}
	

	inline static void Link(LLEC *A, LLEC *B, CRiver *river)
	{
		A->lle = B;
		B->lle = A;
		A->ptr = B->ptr = river;

	}




};



int RiverLL(const char *llstr, LLE &lle)
{
		lle.lat = lle.lng = 0;
		const char *sep = strchr(llstr, ';') ? ";" : ",";
		vara ll(llstr, sep);
		if (ll.length()<2) 
			return FALSE;
		double lat = CGetNum(ll[1]); 
		double lng = CGetNum(ll[0]);
		if (lat==InvalidNUM || lng==InvalidNUM)
			return FALSE;
		lle = LLE(rnd(lat, 1e6), rnd(lng, 1e6), ll.length()>2 ? CGetNum(ll[2]) : -1);
		return TRUE;
}

inline int RiverLL(const char *llstr, float &lat, float &lng)
{
	LLE lle;
	if (!RiverLL(llstr, lle))
		return FALSE;
	lat = lle.lat;
	lng = lle.lng;
	return TRUE;
}



//
// Rappels
//
enum { EWR=0, EWC, EGA, EGX, EWGNUM }; // ETHIS/EUP/EDN
enum { ETHIS=0, EUP, EDN, EERRORSPLIT }; // flags
enum { RTHIS, RSIDEG, RGROUPED }; // rflags

enum { EERROK = 0, ERRUNK, EERRGOOG };

#define FLAG(x) (1<<(x))
#define setflag(f, x) f = ((f)|FLAG(x))
#define getflag(f, x) (((f)&FLAG(x))!=0)

#define getewcflag(f, x) (((f)&x)!=0)

static CString FlagStr(unsigned char flag)
	{
	CString res;
	for (int i=0; i<8; ++i)
		res += getflag(flag, i) ? "1" : "0";
	return res + "B";
	}


#ifdef DEBUG
const char *testscan = NULL; // COMMENT REMOVED FROM PUBLIC
#else
const char *testscan = NULL;
#endif





/*
Users of the free API:
2,500 requests per 24 hour period.
512 locations per request.
5 requests per second.
*/
// gooogle key handling
#define MAXALTPERCALL 512
#define MAXCALLPERSEC 5

class gkey {
int k;
vara keys;

	typedef CArrayList <tcoord*>tcoordsp;

	CString encodeGOOG(tcoords &coordsp, int start, int end, const char *url = "")
	{

	#if 0
		// 20 chars per coordinate (10+1+9)
		end = min( start + 75, end);

		vara list;
		for (int i=start; i<end; ++i)
			list.push(CCoord2(coordsp[i].lat, coordsp[i].lng));
		return list.join("|");
	#else
	  CString out =  url;
	  out += "enc:";

	  int ll[2], pll[2];
	  

	  pll[0] = pll[1] = 0;
	  for (int i=start; i<end && out.GetLength()<2000-20; ++i) 
		{
		ll[0] = (int)(coordsp[i].lat*1e5 + 0.5);
		ll[1] = (int)(coordsp[i].lng*1e5 + 0.5);

		double d  = i<end-1 ? Distance(coordsp[i], coordsp[i+1]) : 0;
	    
		for (register int l=0; l<2; ++l)
			{
			//if (ll[l]<0) ll[l]-=1;
			// Encode latitude
			register int val = (ll[l] - pll[l]);
			val = (val < 0) ? ~(val<<1) : (val <<1);
			while (val >= 0x20) {
			  int value = (0x20|(val & 31)) + 63;
			  out += (char)value;
			  val >>= 5;
			}
			out+= (char)(val + 63);
			pll[l] = ll[l];
			}
		}
	  
	  return out;
	#endif
	}

	void next()
	{
		++k;
		if (k>=keys.length())
			k = 0;
	}

public:
	int calls;
	double disabled;

//#define RMAP
#define MAXLOWRES -100
#ifdef RMAP // use Resolution Map to save google calls
#define rmapn 1024
#define rmapsize (rmapn*rmapn*sizeof(*rmap))
	enum { RMLOW = -1, RMUNKNOWN = 0, RMHIGH = 1 };
	signed char *rmap;

	~gkey(void)
	{
		free(rmap);
	}

	#define rmapx(x) (int)((x-floor(x))*100)
	inline signed char &getrmap(tcoord &c)
	{
		register int x = rmapx(c.lat);
		register int y = rmapx(c.lng);
		return rmap[x+rmapn*y];
	}

	signed char setrmap(tcoord &c)
	{
		BOOL isbetter = isBetterResG(c.res, c.resg);
		signed char &rm = getrmap(c);
		if (rm==RMHIGH || isbetter)
			{
			if (rm<=MAXLOWRES)
				Log(LOGALERT,"wrong rmap detected for %.5f,%.5f", c.lat, c.lng);
			return rm = RMHIGH;
			}
		else 
			{
			return rm = max(rm-1,MAXLOWRES);
			}
	}

	BOOL Load(CFILE &f)
	{
		int ret = f.fread(rmap, rmapsize, 1);
		if (!ret) memset(rmap, 0, rmapsize);
		return ret;
	}

	BOOL Save(CFILE &f)
	{
		return f.fwrite(rmap, rmapsize, 1);
	}
#else
	BOOL Load(CFILE &f)
	{
		return TRUE;
	}

	BOOL Save(CFILE &f)
	{
		return TRUE;
	}

	#define getrmap(c) 0
	#define setrmap(c)
#endif

	gkey(const char *keys = NULL)
	{
		k = 0;
		calls = 0;
		if (keys)
			this->keys = vara(keys);


#ifdef RMAP
	    rmap = (signed char *)malloc(rmapsize);
		memset(rmap, 0, rmapsize);
#endif
	}

	const char *key()
	{
		return keys[k];
	}



	static inline int needs(tcoords &coords, int start, int &end)
	{
		end = min(start + MAXALTPERCALL-1, coords.GetSize());
		double maxres = 0;
		double maxelev = 0, minelev = 1e10;
		signed char rmmax = MAXLOWRES;
		for (int i=start; i<end; ++i)
			{
			if (coords[i].res<0) 
				return TRUE;
			if (coords[i].res>maxres)
				maxres = coords[i].res;
			if (coords[i].elev>maxelev)
				maxelev = coords[i].elev;
			if (coords[i].elev<minelev)
				minelev = coords[i].elev;
			rmmax = max(rmmax, getrmap(coords[i]));
			}
		if (rmmax<=MAXLOWRES)
			return FALSE;
		if (minelev<0)
			minelev = 0;
		if (maxelev<0 || maxres<=0)
			return FALSE; // no good area, no river
		if (maxelev-minelev<MINRAP)
			return FALSE;
		if (maxres<MINHIGHRES)
			return FALSE;
		if (rmmax>0)
			return TRUE;
		return TRUE;
	}

	int getelev(tcoords &coords, BOOL forced = FALSE)
	{
	static DownloadFile f;
	static double tickscounter;

	int n = coords.GetSize();;
	/*
	// remap coordinates
	tcoordsp coordsp(n);
	
	int cluster = (int)ceil(n*1.0/MAXALTPERCALL);
	if (cluster<1 || cluster>2) cluster = 1;
	for (int i=0, j=0, s=0; i<n; i++, j+=cluster)
		{
		if (j>=n) 
			j = ++s;
		coordsp[i] = &_coordsp[j];
		}
	*/
	
	// break in chunks
	int start=0, end=0;
	int	error = 0;
	while (start<n)
	{
		CString clist;
		int end = 0;

		int betterres = 0;
		++groundHeightCnt;
		// check if more is resolution really needed
		if (needs(coords, start, end) || forced)
		{
		vars url = encodeGOOG(coords, start, end, "https://maps.googleapis.com/maps/api/elevation/json?locations=");
		url += MkString("&key=%s", key());

		Throttle(tickscounter, 1000/MAXCALLPERSEC);
	#if 1 // EXECUTE GOOGLE CALLS
		BOOL errornow = FALSE;
		if (f.Download(url))
			{
			Log(LOGWARN,"elevationGOOG: failed download");
			errornow = TRUE;
			}
	#else // SIMULATE GOOGLE CALLS
		// check timer
		static int last = 0;
		int cur = GetTickCount();
		if (cur-last<1000/MAXCALLPERSEC)
			Log(LOGERR, "TOO FAST %d - %d = %d", cur, last, cur-last);
		last = cur;
		if (end-start>=MAXALTPERCALL)
			Log(LOGERR, "TOO MANY ALTs %d", end-start);
		// fuill up memory
		CString memory;
		for (int i = start; i<end; ++i)
			{
			tcoord c(coordsp[i]->lat, coordsp[i]->lng);
			DEMARRAY.height(&c);
			memory += MkString("{\"elevation\":%g \"resolution\":%g }\n", c.elev, 5.0);
			}
		f.memory = (char *)((const char *)memory);
	#endif

		if (!errornow && strstr(f.memory, "OVER_QUERY_LIMIT"))
			{
			Log(LOGWARN,"elevationGOOG: Over quota key#%d", k);
			errornow = TRUE;
			}
		vara olist(f.memory, "\"elevation\"");
		if (errornow)
			{
			next();
			if (++error>=keys.length())
				{
				// disable for 10 minutes
				Log(LOGERR, "getelevationGOOG: %d errors, disabled for 24h", error);
				disabled = GetTickCount()+24*60*60*1000;
				return FALSE; // abort
				}
			Sleep(1000);
			continue;
			}

		++calls;
		error = 0;
		
		CArrayList <LLER>llelist(olist.length()-1);
		for (int j=1, i=0; j<olist.length() && i<end; ++i)
			{
			llelist[i].elev = (float)ExtractNum(olist[j], "", ":", ",");
			llelist[i].res = (float)ExtractNum(olist[j], "\"resolution\"", ":", ",");
			llelist[i].lat = (float)ExtractNum(olist[j], "\"lat\"", ":", ",");
			llelist[i].lng = (float)ExtractNum(olist[j], "\"lng\"", ":", ",");
			++j;
			}
		
		//if (llelist.length()!=end-start)
		//	Log(LOGWARN, "Google elevation incompleteresponse! %d ask != %d rcv", end-start, llelist.length());
		for (int j=0; j<llelist.length() && start<end; j++, start++)
			{
			//if (error)
			//	Log(LOGINFO, "IGNORING #%d %s,%s JS%d %s,%s", i, CCoord(coordsp[i].lat), CCoord(coordsp[i].lng),  j, CCoord(lat), CCoord(lng));
			//else
			LLER *lle = &llelist[j];
			tcoord &c = coords[start];

			#define LLEPS 1e-5
			#define equiv(lle,c) (Distance(lle->lat, lle->lng, c.lat, c.lng)<MINSTEP/2)
			if (!equiv(lle,c))
				{
				// mismatch!!
				LLER *newlle = NULL;
				for (int k=0; k<llelist.length(); ++k)
					{
					LLER *tmp = &llelist[k];
					if (equiv(tmp, c))
						{
						newlle = lle;
						break;
						}
					}

				if (!newlle)
					{
					Log(LOGALERT, "Google elevation missing for point %s: %d ask != %d rcv", lle->Summary(), i-start, j);
					continue;
					}
				lle = newlle;
				}

			c.resg = lle->res;
			if (lle->elev>0 && lle->res>0)
			  {
			  setrmap(c);
			  if (forced || c.res<0 || isBetterResG(c.res, c.resg))
				{
				++betterres;
				c.elev = lle->elev;			
				}
			  }
			}
		}
		else
		{
		start = end;
		}
		
		// might not have gotten all points we asked for but start should take care

		/*
		// if no high res found on first cluster, skip second one
		if (cluster==2 && start>=n/2-1 && betterres==0)
			{
			for (int i=0; start<n; ++start, ++i)
				coordsp[start]->resg = coordsp[i]->resg;
			}
		*/
	}

	return TRUE;
	}


	BOOL IsDisabled(void)
	{
		return  keys.length()==0 || disabled>GetTickCount();
	}

	int getelevationGOOG(tcoords &coordsp, int forced)
	{
	static CSection CC;

	// skip if temporally disabled 
#ifdef NOGOOGLEELEV
	return FALSE;
#endif

	if (IsDisabled())
		return FALSE;

	CC.Lock();
	if (disabled>0)
		{
		// reset calls
		calls = 0;
		disabled = 0;
		}
#ifdef DEBUGXXX
	double dist = 0;
	for (int i=1; i<coordsp.length(); ++i)
		dist += Distance(coordsp[i-1], coordsp[i]);
	double diststep = dist / MINSTEP;
#endif

	int ret = getelev(coordsp, forced);
	CC.Unlock();
	return ret;
	}

};



gkey maingkey(MAIN_GOOGLE_KEY);
gkey scangOFFkey;
gkey scangONkey(OTHER_GOOGLE_KEYS);
gkey *scangkey = &scangOFFkey;



class CScan;
class CScanList;
typedef CArrayListPtr<CScan> CScanPtr;
typedef CArrayList<CScanPtr> CScanPtrList;
//typedef CArrayListPtr<CRiver> CRiverPtr;
typedef CArrayList<CScan*> CScanListPtr;
typedef CArrayList<CRiver*> CRiverListPtr;
//typedef CArrayList<CRiverPtr> CRiverPtrList;


class CRiver {
protected:
	LLRect box;
	tcoords coords;
public:
	CSym *sym;
	CScanPtr scan;
	CScanPtrList splits;
	double id;
	int num;
	short int level, order;
	short int fixed;
	double length;

	LLE SE[2];
	SEG *seg;

	short int dir;
	short int connected;
	//CDoubleArrayList conn[2];


	CRiver(void)
	{
		Init();
	}

	CRiver(CSym &sym)
	{
		Init();
		Set(&sym, 0);
	}

	CRiver(LLRect *bbox)
	{
		Init();
		box = *bbox;
	}

	BOOL IsValid(void)
	{
		return coords.GetSize()>1 && box.IsValid();
	}

	CRiver(tpoints &trk, CScanPtr &scan)
	{
		Init();
		if (trk.GetSize()<2)
			{
			Log(LOGALERT, "Invalid trk (%d POINT) at %s", trk.GetSize(), trk.GetSize()<1 ? "?" : trk[0].Summary());
			return;
			}
		for (int i=0; i<trk.GetSize(); ++i)
			{
			coords.push(trk[i]);
			box.Expand(trk[i].lat, trk[i].lng);
			}
		if (trk.GetSize()>1)
		{
		SE[0] = trk.Head();
		SE[1] = trk.Tail();
		}

		this->scan = scan;
	}

	/*
	~CRiver(void)
	{
	}
	*/

	void Init()
	{
		id = 0;
		length = 0;
		level = order = -1;
		fixed = FALSE;
		dir = 0;
		num = -1;
		sym = NULL;
		connected = FALSE;
		seg = NULL;
		//scan = NULL;
	}

	BOOL operator==(const CRiver &other)
	{
		return other.id == id;
	}
	int operator-(const CRiver &other)
	{
		return (int)(id - other.id);
	}

/*	
	CRiver& operator=(const CRiver &other)
	{
		id = other.id;
		level = other.level;

		SE[0] = other.SE[0];
		SE[1] = other.SE[1];
		//LLEC::Link(&SE[0], &SE[1], this);

		box = other.box;
		sym = other.sym;
		num = other.num;
		//pri = other.pri;
		coords = other.coords;
		fixed = other.fixed;

		return *this;
	}
*/

	static inline double Id(const char *S, const char *E, const char *dist)
	{
		return Id(S, E, CGetNum(dist));
	}
		
	static double Id(const char *S, const char *E, double d)
	{
		/*
		LLEC lle;
		if (!RiverLL(S, lle) || d==InvalidNUM )
			{
			Log(LOGERR, "Invalid ID for %s,%s,%s", S,E,dist);
			return 0;
			}
		*/
		double a = crc32(E, GetTokenPos(E,2), crc32(S, GetTokenPos(S,2), 0));
		ASSERT(a>0);
		return a+d/1000.0;
		//double b = crc32(S, strlen(S), crc32(E, strlen(E), crc32(dist, strlen(dist), 0)));
		///return crc32(ToHex(&a, sizeof(a))+ToHex(&b, sizeof(b)), 0);
		//double id = (lle.lat+90)*360+(lle.lng+180)+d/1e4;
		//return id;
		//float idf = (float)(a/b);
		//return *((DWORD *)(&id));
	}

	static inline CString Summary(const char *S, const char *E, const char *dist)
	{		
		return MkString("%s %s %s", S, E, dist);
	}

	static inline CString Summary(const char *S, const char *E, double dist)
	{
		return Summary(S, E, MkString("%.0f", dist));
	}


	void Set(CSym *s, int n)
	{
		Init();
		sym = s;
		num = n;
		connected = FALSE;
		coords.Reset();
		box = LLRect();
		vara summaryp(s->GetStr(ITEM_SUMMARY), " ");
		if (summaryp.length()==3)
			{
			if (!RiverLL(summaryp[0], SE[0]) || !RiverLL(summaryp[1], SE[1]))
				{
				Log(LOGERR, "Invalid river summary %s", summaryp.join(" "));
				return;
				}
			length = CGetNum(summaryp[2]);
			id = Id(summaryp[0], summaryp[1], length);
			return;
			}

		if (summaryp.length()==2)
			{
			LLE lle;
			if (!lle.Scan(summaryp[0], summaryp[1]))
				{
				Log(LOGERR, "Invalid lleriver summary %s", summaryp.join(" "));
				return;
				}
			id = lle.Id();
			SE[0]=SE[1]=lle;
			return;
			}

		Log(LOGERR, "Invalid river/lleriver summary");

		/*
		void LLECheck(LLEC *lle);
		LLEC::Link(&SE[0], &SE[1], this);

		LLEC *lle = &SE[0];
			LLEC SE2[2];
			vara sump(GetStr(ITEM_SUMMARY), " ");
			if (!RiverLL(sump[0], SE2[0]) || !RiverLL(sump[1], SE2[1]))
				{
				Log(LOGERR, "Bad res coordinates %s", sump.join(" "));
				}
			double d0 = LLEC::Distance(&SE2[0], lle);
			double d1 = LLEC::Distance(&SE2[1], lle->lle);
			double d2 = LLEC::Distance(&SE2[0], lle);
			double d3 = LLEC::Distance(&SE2[1], lle->lle);
			int ok = (d0<=0 && d1<=0) || (d2<=0 && d3<=0);
			if (!ok)
				{
				void LLEPrint(const char *id, LLEC *lle);
				Log(LOGALERT, "Inconsistent merged seg %g %g %g %g <> %s", d0, d1, d2, d3, sump.join(" "));
				LLEPrint("Bad", lle);
				}

		LLECheck(&SE[0]);
		LLECheck(&SE[1]);		
		*/
		
	}

	
	//  return coord index >=0 or -1 if not found
	int FindPoint(double lat, double lng, double mind = MINRIVERJOIN)
	{
		// check box
		//if (!Box().Contains(lat, lng))
		//	return -1;
		
		int j = -1;
		tcoords &coords = Coords();
		for (int i=0, n=coords.length(); i<n; ++i)
			{
			double d = ::Distance(lat, lng, coords[i].lat, coords[i].lng);
			if (d<mind)
				j = i, mind = d;
			}

		return j;
	}	

	inline double DistanceToLine(double lat, double lng)
	{
		return ::DistanceToLine(lat, lng, Coords());
	}

	inline CString GetStr(int n)
	{
		return sym->GetStr(n);
	}

	inline double GetNum(int n)
	{
		return sym->GetNum(n);
	}

	inline int Level(void)
	{
		if (level<0)
			{
			level = atoi(GetToken(sym->GetStr(ITEM_WATERSHED), WSLEVEL, ';'));
			if (level<0) level = 0;
			}
		return level;
	}

	inline int Order(void)
	{
		if (order<0)
			{
			order = atoi(GetToken(sym->GetStr(ITEM_WATERSHED), WSORDER, ';'));
			if (order<0) order = 0;
			}
		return order;
	}


	inline const LLRect &Box(void)
	{
		if (!box.IsValid())
			box = LLRect(GetNum(ITEM_LAT), GetNum(ITEM_LNG), GetNum(ITEM_LAT2), GetNum(ITEM_LNG2) );
		return box;		
	}

	static inline tcoord Coords(const char *point)
	{
		vara c(point, ";");
		if (c.length()>=2)
			return tcoord(CGetNum(c[1]), CGetNum(c[0]));
		else
			{
			Log(LOGALERT, "Invalid river coords '%s' detected", point);
			return tcoord(0,0);
			}
	}

	inline tcoords &Coords()
	{
		if (coords.GetSize()==0)
			{
			vara scoords( sym->GetStr(ITEM_COORD), " ");
			coords.SetSize(scoords.length());
			for (int i=0; i<coords.GetSize(); ++i)   
				coords[i] = Coords(scoords[i]);
			}
		return coords;
	}
/*
	CDoubleArrayList &Conn(int se)
	{
		if (!connected)
		{
		connected = TRUE;
		vara connp(sym->GetStr(ITEM_CONN), " ");
		if (connp.length()>0)
			{
#ifdef DEBUG
			double id2 = 0;
			FromHex(connp[0], &id2);
			if (id2!=id)
				Log(LOGALERT, "Inconsistent id2 %s != %s", ToHex(&id2, sizeof(id2)), ToHex(&id, sizeof(id)));
#endif
			for (int i=0, c=1; i<2 && c<connp.length(); ++i, ++c)
				{
				vara connlist(connp[c], ";");
				int n = connlist.length();
				conn[i].SetSize(n);
				for (int j=0; j<n; ++j)
					FromHex(connlist[j], &conn[i][j]);
				}
			}
		else
			{
			id = id;
			}	
		}
		return conn[se];
	}
*/

};


class trapcore {
public:
	LLEA lle;
	LLEA end;
	float avgg;
	float res, resg;
	unsigned char rflags[4];
};


class trappel;
typedef CArrayList<trappel *> CRapList;

class trappel : public trapcore {
public:
	float score, scoreg;
	CScanPtr scan;
	CRapList scorelist;
	double upscoreid;
#ifdef DEBUG
	vara logscoreg, logscore;
#endif

	static CString Header();

	trappel(void)
	{
		score = scoreg = 0;
		upscoreid = 0;
	}

	inline float m() 
	{ 
		return (float)rnd(lle.elev - end.elev); 
	}


	int IsValid(void)
	{
		if (score<=0)
			return FALSE;
		if (!scan)
			return FALSE;
		if (rflags[RGROUPED])
			return FALSE;
		return TRUE;
	}

	inline CString Id() { return IdStr(lle.Id()); }

	CString Summary(CScan *scan = NULL, BOOL extra = FALSE);

};

class scancorold 
{
public:
	double id;
	double oid;
	LLEA mappoint;
	float cfs;
	float temp;
	unsigned char flags[4];
	float date;
	int trktell;
	unsigned char level, order, res, version;
};

class scancore 
{
public:
	double id;
	double oid;
	LLEA mappoint;
	float cfs;
	float temp;
	unsigned char flags[4];
	float date;
	int trktell;
	unsigned char level, order, res, version;
	float km2; 
};

class scancorev2 : public scancore 
{
public:	// <-- add new fields, after conv move to scancore class
	
	scancorev2()
	{
	//drainage = InvalidNUM;
	}
};

class scanheadersave : public scancore // <-- change to v2 to upgrade scancore when saving
{
public:
short int nrap, ndown, iname;
};

class scanheader : public scancore
{
public:
short int nrap, ndown, iname;
};


class scantrkheader
{
public:
double id;
int num;
};


static int cmpsymdesc(LLEC **lle1, LLEC **lle2)
{
		CString data1 = (*lle1)->sym()->GetStr(0);
		CString data2 = (*lle2)->sym()->GetStr(0);
		int ret = strcmp(data1, data2);
		if (ret!=0) return ret;
		
		// if equal source, sort by elev
		float elev1 = (*lle1)->GetElev();
		float elev2 = (*lle2)->GetElev();
		if (elev1 > elev2) return -1;
		if (elev1 < elev2) return 1;

		return 0;
}

class CRapGroup;
class CanyonMapper;

class LLGOOG : public LL
{
  public:
	unsigned int encoded;
};



enum { INVERROR = -2, INVGOOG = -1, INVVERSION = 1, INVALL = 0 };


int LoadTrk(CFILE &tf, tpoints &list, double id, double minelev);


class CScan : public scancore 
{

	public:
	const char *name;
	CRapList rappels;
	CArrayList<trappel> rapdata;
	CDoubleArrayList downconn;
	CRiver *trk;
	LLECLIST *wrlist;
	const char *file;

	CRapGroup *group() { return (CRapGroup *)trk; }

	int geterror() { return flags[EERRORSPLIT]&0x7; }
	void seterror(int err) { flags[EERRORSPLIT] = (flags[EERRORSPLIT]&(~0x7)) | (err&0x7); }

	int getsplit() { return flags[EERRORSPLIT]>>4; }
	void setsplit(int num) { flags[EERRORSPLIT] = (flags[EERRORSPLIT]&0x0F) | (num<<4); }

	int getvalid() { return (flags[EERRORSPLIT]&0x8)!=0; }
	void setvalid() { flags[EERRORSPLIT] |= 0x8; }

	int operator-(const CScan &other)
	{
		return (int) (id - other.id);
	}

	BOOL operator==(const CScan &other)
	{
		return id == other.id;
	}

	double GetScore(void)
	{
		double maxscore = 0;
		for (int i=0; i<rapdata.GetSize(); ++i)
			if (rapdata[i].score>maxscore)
				maxscore = rapdata[i].score;
		return maxscore;
	}

	double GetLongest(void)
	{
		double maxlen = 0;
		for (int i=0; i<rapdata.GetSize(); ++i)
			if (rapdata[i].m()>maxlen)
				maxlen = rapdata[i].m();
		return maxlen;
	}

	CScan& operator=(const CScan &other)
	{
		id = other.id;
		oid = other.oid;
		mappoint = other.mappoint;
		name = other.name;
		cfs = other.cfs;
		temp = other.temp;
		memcpy(flags, other.flags, sizeof(flags));
		rappels = other.rappels;
		downconn = other.downconn;
		trktell = other.trktell;
		file = other.file;

		//ASSERT(!trk && !other.trk);
		//ASSERT(!wrlist && !other.wrlist);
		/*
		not transferable!

		if (trk) 
			delete trk, trk = NULL;
		if (other.trk) 
			trk = new CRiver, *trk = *other.trk;
		if (wrlist) 
			delete wrlist, wrlist = NULL;
		if (other.wrlist) 
			rwlist = new LLECLIST, *rwlist = other.wrlist;
		*/

		return *this;
	}

	CScan()
	{
		id = oid = 0;
		//lle = LLEA();
		name = NULL;
		cfs = temp = InvalidNUM;
		ZeroMemory(flags, sizeof(flags));
		trk = NULL;
		wrlist = NULL;
	}

	~CScan()
	{
		if (trk) delete trk;
		if (wrlist) delete wrlist;
	}

	int RapNum(trappel *rap)
	{
		int nraps = rappels.GetSize();
		for (int i=0; i<nraps; ++i)
			if (rappels[i]->lle.lat==rap->lle.lat && rappels[i]->lle.lng==rap->lle.lng)
				return i;
		return -1;
	}

	CString Summary(void);


	vara Merge(CScan *other)
	{
		vara ret;
		if (version>other->version)
			return ret;
		if (version<other->version)
			{
			*this = *other;
			return ret;
			}
		if (geterror()!=EERROK && other->geterror()==EERROK)
			{
			*this = *other;
			return ret;
			}
		if (geterror()==EERROK && other->geterror()!=EERROK)
			return ret;
		/*
		if (date>other->date)
			return ret;
		if (date<other->date)
			{
			*this = *other;
			return ret;
			}
		*/

		ASSERT(IdStr(this->id)!="R7b141a70fc36ed41");
		ASSERT(IdStr(other->id)!="R7b141a70fc36ed41");

		// report
		if (rappels.GetSize()!=other->rappels.GetSize())
			ret.push(MkString("%draps!=%draps", rappels.GetSize(), other->rappels.GetSize()));
		if (downconn.GetSize()!=other->downconn.GetSize())
			ret.push(MkString("%dconn!=%dconn", downconn.GetSize(), other->downconn.GetSize()));
		// merge
		for (int i=0; i<sizeof(flags); ++i)
			flags[i] |= other->flags[i];
		if (rappels.GetSize()<other->rappels.GetSize())
			rappels = other->rappels;
		if (downconn.GetSize()<other->downconn.GetSize())
			downconn = other->downconn;
		return ret;
	}


	int Load(CFILE &f, CFILE &tf, CStringArrayList &namelist) 
	{
		scanheader check;
		if (!f.fread(&check, sizeof(check), 1))
			return FALSE;
		if (check.id==0 || check.id==InvalidNUM || check.ndown<0 || check.nrap<0 || check.iname<0 || check.iname>namelist.GetSize())
			{
			Log(LOGALERT, "Corrupted SCAN file %s: mismatched scan header", f.name());
			id = oid = 0;
			return FALSE;
			}

		//ASSERT REMOVED FROM PUBLIC

		memcpy(this, &check, sizeof(scancore));
		this->name = namelist[check.iname];
		if (oid<0) 
			{
			Log(LOGALERT, "Corrupted SCAN file %s: Invalid oid=%s", f.name(), IdStr(oid));
			id = oid = 0;
			}

		if (check.nrap>0)
		{
		rapdata.SetSize(check.nrap);
		rappels.SetSize(check.nrap);
		for (int i=0, n=check.nrap; i<n; ++i)
			if (!f.fread(rappels[i]=&rapdata[i], sizeof(trapcore), 1))
				{
				Log(LOGALERT, "Corrupted SCAN file %s: cannot read rappels", f.name());
				return FALSE;
				}
		}
		if (check.ndown>0)
		{
		downconn.SetSize(check.ndown);
		if (f.fread(downconn.Data(), sizeof(*downconn.Data()), check.ndown)!=check.ndown)
			{
			Log(LOGALERT, "Corrupted SCAN file %s: cannot read downcon", f.name());
			return FALSE;
			}
		}

		// load track
		if (tf.isopen())
			{
			tpoints list;
			if (!LoadTrk(tf, list, id, -1))
				return FALSE;
			
			// build trk
			trk = new CRiver(list, CScanPtr(NULL, 0));
			// not loaded
			wrlist = new LLECLIST;
			}

		return TRUE;
	}

#define ENCODE(x) ((unsigned int)max(0, min(0xFFFFFF, (x)*(1<<10))))
#define DECODE(x) (((float)(x))/(1<<10))

	static unsigned int encode(float elev, float resg)
	{
		unsigned int enc = (ENCODE(elev)<<8) |  // 24-10 = 14.10 fixed precision int 2^14 = 16Km
				((unsigned char)max(0, min(0xFF, resg*(1<<2))));  // 6.2 fixed precision 2^6 = 64m
#ifdef DEBUG
		float celev, cresg;
		decode(enc, celev, cresg);
		ASSERT(abs(celev-elev)<1e-3);
		ASSERT(abs(max(cresg,0)-max(0,resg))<0.5);
#endif
		return enc;
	}

	static BOOL decode(unsigned int encoded, float &elev, float &resg)
	{
		elev = DECODE(encoded>>8);
		resg = (float)(encoded & 0xFF)/(1<<2);
		return (elev>=0 && elev<MAXELEV);
	}


	int Save(CFILE &f, CFILE &tf, int iname)
	{
		// save performed scans
		scanheadersave check;
		memcpy(&check, this, sizeof(scancore));
		check.nrap = rappels.GetSize();
		check.ndown = downconn.GetSize();
		check.iname = iname;
		check.trktell = tf.ftell();

		if (!f.fwrite(&check, sizeof(check), 1))
		 	return FALSE;

		int nrap = rappels.GetSize();
		if (nrap>0)
		for (int i=0, n=rappels.GetSize(); i<n; ++i)
		  if (!f.fwrite(rappels[i], sizeof(trapcore), 1))
			return FALSE;
		int ndown = downconn.GetSize();
		if (ndown>0)
		  if (f.fwrite(downconn.Data(), sizeof(*downconn.Data()), ndown)!=ndown)
			return FALSE;

		// save track
		if (tf.isopen())
			{
			tcoords &trk = this->trk->Coords();
			scantrkheader trkcheck;
			trkcheck.id = id;
			trkcheck.num = trk.GetSize();
			if (!tf.fwrite(&trkcheck, sizeof(trkcheck), 1))
		 		return FALSE;
			for (int i=0, n=trk.GetSize(); i<n; ++i)
			  {
			  tcoord &p = trk[i];
			  LLGOOG lle;
			  //LL lle;
			  lle.lat = p.lat;
			  lle.lng = p.lng;
			  lle.encoded = encode(p.elev, p.resg);
			  if (!tf.fwrite(&lle, sizeof(lle), 1))
				return FALSE;
			  }
			}

		return TRUE;
	}

	void Set(CScanPtr ptr, double id, double oid, int split, CanyonMapper &cm, const char *name = "");

	void SetRiver(CRiver *r);

	BOOL GetName(void)
	{
		if (name!=NULL && *name!=0)
			return FALSE;

		LLECLIST &wrlist = *this->wrlist;

		if (wrlist.GetSize()==0)
			return FALSE;

		CIntArrayList counts;
		vara names;
		LLECLIST llelist;
		int maxcount = 0, maxfind = 0;
		const char *del[] = { "Middle ", "Upper ", "Lower ", "Big ", "Little ", NULL };

		wrlist.Sort(cmpsymdesc);

		for (int i=0; i<wrlist.GetSize(); ++i)
			{
			const char *data = wrlist[i]->sym()->data;
			vars name = htmltrans(wrlist[i]->sym()->id);
#if 0
			Log(LOGINFO, "#%d %p %p %s [%s]", i, wrlist[i], wrlist[i]->sym(), name, data);
#endif
			for (int d=0; del[d]!=NULL; ++d)
				if (IsSimilar(name, del[d]))
					name.Delete(0, strlen(del[d]));

			name.Replace(" falls", " Falls");
			name.Replace(" FALLS", " Falls");
			vara falls(name, " Falls");
			int find = names.indexOfi(falls[0]);
			if (find<0)
				{
				counts.push(0);
				names.push(falls[0]);
				llelist.push(wrlist[i]);
				}
			else
				{
				int count = ++counts[find];
				if (count>maxcount)
					{
					maxcount = count;
					maxfind = find;
					}
				}
			}

		name = llelist[maxfind]->sym()->id;
#if 0
		Log(LOGINFO, "Named: %s --> segment %s @ %s ",  llelist[maxfind]->sym()->Line(), IdStr(id), lle.Summary(), lle.);
#endif
		return TRUE;
	}

	// -1:google errors only +1:except google errors  0:all
	BOOL IsValid(int mode = 0)  
	{
	   CScan *scan = this;
	   if (!scan)
		   return FALSE; //compute
	   if (scan->id==0) 
		   return FALSE; // not valid

	   // extra checks
	   extern int MODE;
	   if (MODE<-1) // special: check EERROR is properly reported
	   {
		  // check resolution & force recalc
		  for (int i=0; i<scan->rappels.GetSize(); ++i)
			{
			  trappel *rap = scan->rappels[i];
			  if (!scan->geterror())
			   if (rap->res>MINHIGHRES && rap->resg<=0)
				{
				Log(LOGERR, "Invalid ERROROK [Res:%.2f ResG:%.2f ERROR:%d] for %s", rap->res, rap->resg, scan->geterror(), rap->Summary(scan));
				return FALSE;
				}
			  if (rap->res<=0)
				{
				Log(LOGERR, "Invalid Res [Res:%.2f ResG:%.2f] for %s", rap->res, rap->resg, rap->Summary(scan));
				return FALSE;
				}
			  if (!ValidLL(rap->lle.lat, rap->lle.lng))
				{
				Log(LOGERR, "Invalid START LL [%s] for %s", rap->lle.Summary(), rap->Summary(scan));
				return FALSE;
				}
			  if (!ValidLL(rap->end.lat, rap->end.lng))
				{
				Log(LOGERR, "Invalid END LL [%s] for %s", rap->end.Summary(), rap->Summary(scan));
				return FALSE;
				}
			if (scan->oid!=0) 
				if (rap->lle.ang==InvalidNUM)
				{
				Log(LOGERR, "Invalid START Ang [InvalidNUM] for %s", rap->Summary(scan));
				return FALSE;
				}
			if (rap->lle.ang!=InvalidNUM)
			  if (abs(rap->lle.ang)>2*M_PI)
				{
				Log(LOGERR, "Invalid START Ang [%s] for %s", CData(rap->lle.ang), rap->Summary(scan));
				return FALSE;
				}
			}
	   }

	   if (mode>=0)
	   {
	   // invalid checks
	   if (scan->version<SCANVERSION)
			return FALSE; // engine version
	   if (scan->date+SCANDAYS<CurrentDate)
			return FALSE; // outdated

	   /*
	   if (scan->rappels.length()>0)
		if (scan->rappels[0]->lle.Id() == scan->mappoint.Id())
			if (scan->oid!=0)
				if (scan->rappels[0]->lle.ang == 0)
					return FALSE;
	   */
	   }
	   
	   if (mode<=0)
	   {
	   // google
	   if (scan->geterror()!=EERROK)
			{
			if (mode<0)
			   return FALSE;

			// auto when goog disabled
			if (TMODE!=0 && !scangkey->IsDisabled())
				return FALSE; // goog error
			}
	   }


	   return TRUE;
	}
};


	int LoadTrk(CFILE &tf, tpoints &list, double id, double minelev) 
	{
			scantrkheader trkcheck;
			if (!tf.fread(&trkcheck, sizeof(trkcheck), 1))
		 		return FALSE;
			if ((id!=0 && trkcheck.id!=id) || trkcheck.num<0)
				{
				Log(LOGALERT, "Corrupted TRK file %s: mismatched header", tf.name());
				return FALSE;
				}

			list.SetSize(trkcheck.num);
			for (int i=0; i<trkcheck.num; ++i)
			  {
			  tcoord &p = list[i];
			  LLGOOG lle;
			  //LL lle;
			  if (!tf.fread(&lle, sizeof(lle), 1))
				{
				Log(LOGALERT, "Corrupted TRK file %s: cannot read trks", tf.name());
				return FALSE;
				}
			  //LL lle;
			  p.lat = lle.lat;
			  p.lng = lle.lng;
			  if (!CScan::decode(lle.encoded, p.elev, p.resg))
				{
				Log(LOGALERT, "Corrupted TRK file %s: invalid GOOGDATA", tf.name());
				return FALSE;
				}
			  if (p.elev<minelev)
				{
				list.SetSize(i);
				break;
				}
			  }
		return TRUE;
	}

CString TrkFile(const char *file, const char *filepath = NULL)
	{
		if (filepath && strstr(filepath, SCANTMP))
			filepath = SCANTMP;
		else
			filepath = SCANTRK;
		return scanname(GetFileName(file)+TRK, filepath);
	}

int LoadTrk(const char *trkfile, tcoords &trkcoords, double minelev) 
	{
		CFILE tf;
		if (!tf.fopen(TrkFile(GetToken(trkfile, 0, ':'))))
			{
			Log(LOGALERT, "Cannot open TRK file %s", trkfile);
			return FALSE;
			}

		double seek = CGetNum(GetToken(trkfile, 1, ':'));
		if (seek<0)
			{
			Log(LOGALERT, "Cannot seek TRK file %s", trkfile);
			return FALSE;
			}
		tf.fseek( (int)seek );

		tpoints list;
		if (!LoadTrk(tf, list, 0, minelev))
			return FALSE;

		trkcoords.SetSize(list.GetSize());
		for (int i=list.GetSize()-1; i>=0; --i)
			trkcoords[i] = list[i];
		return TRUE;
	}







static int cmprapelev(trappel **rap1, trappel **rap2)
	{
		if ((*rap1)->lle.elev > (*rap2)->lle.elev) return -1;
		if ((*rap1)->lle.elev < (*rap2)->lle.elev) return 1;
		return 0;
	}


enum { SPIVOT = 0, SGSUM, SGAVG, SRSUM, SRAVG, SMAX };
#define SDESC "PIVOT#,GRPSUM#,GRPAVG#,RAPSUM#,RAPAVG#"
int SPRIMARY = SGSUM;


class CRapGroup {
public:
	int rank;
	double id, oid;
	int score[SMAX];
	trappel *pivot;
	float length;
	float depth;
	float maxlen;
	BOOL valid;
	unsigned char order, level;
	float km2, cfs, temp;
	CRapList raplist;
	const char *name;
	CSym *rwsym;

	CRapGroup(void)
	{
		rank = -1;
		name = NULL;
		for (int i=0; i<SMAX; ++i)
			score[i] = 0;
		length = depth = maxlen = 0;
		order = level = 0;
		cfs = temp = km2 = InvalidNUM;
		pivot = NULL;
		valid = FALSE;
		oid = id = 0;
	}

	int AddRap(trappel *rap, LLRect *bbox)
	{
		//ASSERT REMOVED FROM PUBLIC
		//ASSERT REMOVED FROM PUBLIC
		//ASSERT(rap->scan!=NULL && rap->score>0);

		// check not added already
		if (raplist.Find(rap)<0)
			{
			rap->rflags[RGROUPED] = TRUE;
			raplist.AddTail(rap);
			//if (rap->scan->getvalid())
			if (bbox->Contains(rap->lle.lat, rap->lle.lng))
				valid = TRUE;
			return TRUE;
			}
		return FALSE;
	}

	void Set(trappel *last, int ewc, LLRect *bbox)
	{
		// create new group from rap list and allowed connections
		AddRap(last, bbox);
		while (last!=NULL)
		{
			trappel *next = NULL;
			CRapList &scorelist = last->scorelist;
			scorelist.Sort(cmprapelev);
			for (int i=0; i<scorelist.GetSize(); ++i)
				{
				//double id = scorelist[i]->scan->id;
				//if (scan->id==id || scan->downconn.Find(id)>=0)
				if (scorelist[i]->lle.elev < last->lle.elev)
					AddRap(next = scorelist[i], bbox);
				}
			last = next;
		}
		Compute(ewc);
	}


	void Compute(int ewc)
	{
		raplist.Sort(cmprapelev);
		for (int i=0; i<SMAX; ++i)
			score[i] = 0;
		length = depth = 0;
		maxlen = 0;

		// compute stats
		pivot = NULL;
		int n = raplist.GetSize();
		if (n==0) return;
		trappel *last = NULL;
		//double mincfs, maxcfs, mintemp, maxtemp;
		//mincfs = maxcfs = mintemp = maxtemp = InvalidNUM;
		for (int i=0; i<n; ++i)
			{
			trappel *rap = raplist[i];
			if (raplist[i]->m() > maxlen)
				maxlen = rap->m();
			score[SRSUM] += (int)rap->score;
			score[SGSUM] += (int)rap->scoreg;
			if (!pivot || rap->scoreg > pivot->scoreg)
				pivot = rap, score[SPIVOT] = (int)rap->scoreg;
			/*
			if (rap->scan->cfs!=InvalidNUM)
				if (mincfs==InvalidNUM || mincfs > rap->scan->cfs)
					mincfs = rap->scan->cfs;
			if (rap->scan->cfs!=InvalidNUM)
				if (maxcfs==InvalidNUM || maxcfs < rap->scan->cfs)
					maxcfs = rap->scan->cfs;
			if (rap->scan->temp!=InvalidNUM)
				if (mintemp==InvalidNUM || mintemp > rap->scan->temp)
					mintemp = rap->scan->temp;
			if (rap->scan->temp!=InvalidNUM)
				if (maxtemp==InvalidNUM || maxtemp < rap->scan->temp)
					maxtemp = rap->scan->temp;
			*/
			if (last)
				length += (float)LLE::Distance(&rap->lle, &last->lle);
			last = rap;
			}
		score[SRAVG] = score[SRSUM]/n;
		score[SGAVG] = score[SGSUM]/n;
		length += (float)LLE::Distance(&raplist[i-1]->lle, &raplist[i-1]->end);
		depth = (float)(raplist.Head()->lle.elev - raplist.Tail()->end.elev);
		name = pivot->scan->name;	
		if (!name || *name==0)
			name = "unnamed";
		order = pivot->scan->order;
		level = pivot->scan->level;
		km2 = pivot->scan->km2;
		cfs = pivot->scan->cfs;
		temp = pivot->scan->temp;
		id = raplist.Head()->scan->id;
		oid = raplist.Head()->scan->oid;
	}

	static CString Header(BOOL metric);
	CString Summary(int rapinfo, BOOL metric);
};


int SCMP0(CRapGroup **A, CRapGroup **B) { return (*B)->score[0] - (*A)->score[0]; }
int SCMP1(CRapGroup **A, CRapGroup **B) { return (*B)->score[1] - (*A)->score[1]; }
int SCMP2(CRapGroup **A, CRapGroup **B) { return (*B)->score[2] - (*A)->score[2]; }
int SCMP3(CRapGroup **A, CRapGroup **B) { return (*B)->score[3] - (*A)->score[3]; }
int SCMP4(CRapGroup **A, CRapGroup **B) { return (*B)->score[4] - (*A)->score[4]; }
void *SCMP[] = { SCMP0, SCMP1, SCMP2, SCMP3, SCMP4 };



//
// GEOTOOLS
//


typedef int tgetelevation(tcoords &coordsp);

int DownloadUrl(DownloadFile &f, const char *url, int retry = 2, int delay = 1)
{
	for (int i=0; i<retry; ++i)
		{
		if (i>0) 
			Sleep(delay*1000);
		if (!f.Download(url)) 
			return FALSE;
		}
	return -1;
}


int getelevationGPSV(tcoords &coordsp)
{
	int error = 0;
	static DownloadFile f;
	int n = coordsp.GetSize();
	for (int i=0; i<n; ++i)
	  if (coordsp[i].elev<0)
		{
		double lat = coordsp[i].lat, lng = coordsp[i].lng;
		++groundHeightCnt;
		vars url = MkString("http://www.gpsvisualizer.com/elevation_data/elev.js?coords=%s,%s", CCoord(lat), CCoord(lng));
		if (!DownloadUrl(f, url))
			{
			vars elev;
			vars data(f.memory);
			GetSearchString(data, "LocalElevationCallback", elev, "(", ")");
			coordsp[i].elev = (float)(1/CGetNum(elev));
			}
		else
			++error;
		}
	return !error;
}


int getelevationNED(tcoords &coordsp)
{
	static DownloadFile f;
	int n = coordsp.GetSize();
	for (int i=0; i<n; ++i)
	  if (coordsp[i].elev<0)
		{
		double lat = coordsp[i].lat, lng = coordsp[i].lng;
		++groundHeightCnt;
		vars url = MkString("http://ned.usgs.gov/epqs/pqs.php?x=%s&y=%s&units=Meters&output=json",CCoord(lng), CCoord(lat));
		if (!DownloadUrl(f, url))
			{
			vars elev;
			vars data(f.memory);
			GetSearchString(data, "\"Elevation\"", elev, ":", ",");
			coordsp[i].elev = (float)CGetNum(elev);
			}
		}
	  return TRUE;
}


int getelevationGEO(tcoords &coordsp)
{
	int error = 0;
	int n = coordsp.GetSize();
	static DownloadFile f;
	for (int i=0; i<n; ++i)
	  if (coordsp[i].elev<0)
		{
		double lat = coordsp[i].lat, lng = coordsp[i].lng;
		++groundHeightCnt;
		vars url = MkString("http://api.geonames.org/astergdemJSON?lat=%s&lng=%s&username=Administrator", CCoord(lat), CCoord(lng)); //&username=nelson
		if (!DownloadUrl(f, url))
			{
			vars elev;
			vars data(f.memory);
			GetSearchString(data, "\"astergdem\"", elev, ":", ",");
			coordsp[i].elev = (float)CGetNum(elev);
			}
		else
			++error;
		}
	  return !error;
}

int getelevationEARTH(tcoords &coordsp)
{
	int error = 0;
	int n = coordsp.GetSize();
	static DownloadFile f;
	for (int i=0; i<n; ++i)
	  if (coordsp[i].elev<0)
		{
		double lat = coordsp[i].lat, lng = coordsp[i].lng;
		++groundHeightCnt;
		vars url = MkString("http://www.earthtools.org/height/%s/%s", CCoord(lat), CCoord(lng));
		if (!DownloadUrl(f, url))
			{
			vars elev;
			vars data(f.memory);
			GetSearchString(data, "<location>", elev, "<meters>", "</meters>");
			coordsp[i].elev = (float)CGetNum(elev);
			}
		else
			++error;
		}
	return !error;
}




int getelevationGOOGSCAN(tcoords &coordsp)
{
	return scangkey->getelevationGOOG(coordsp, FALSE);
}

int getelevationGOOGMAIN(tcoords &coordsp)
{
	return maingkey.getelevationGOOG(coordsp, FALSE);
}

int getelevationGOOGMAINFORCED(tcoords &coordsp)
{
	return maingkey.getelevationGOOG(coordsp, TRUE);
}



int getelevationDEM(tcoords &coordsp)
{
	int n = coordsp.GetSize();
	for (int i=0; i<n; ++i)
		{
		++groundHeightCnt;
		DEMARRAY.height(&coordsp[i]);
		}
	return TRUE;
}

int getelevationDEM0(tcoords &coordsp)
{
	int n = coordsp.GetSize();
	for (int i=0; i<n; ++i)
		{
		++groundHeightCnt;
		DEMARRAY.height(&coordsp[i], 0);
		}
	return TRUE;
}

int getelevationDEM1(tcoords &coordsp)
{
	int n = coordsp.GetSize();
	for (int i=0; i<n; ++i)
		{
		++groundHeightCnt;
		DEMARRAY.height(&coordsp[i], 1);
		}
	return TRUE;
}

int getelevationDEM2(tcoords &coordsp)
{
	int n = coordsp.GetSize();
	for (int i=0; i<n; ++i)
		{
		++groundHeightCnt;
		DEMARRAY.height(&coordsp[i], 2);
		}
	return TRUE;
}

int getelevationDEM3(tcoords &coordsp)
{
	int n = coordsp.GetSize();
	for (int i=0; i<n; ++i)
		{
		++groundHeightCnt;
		DEMARRAY.height(&coordsp[i], 3);
		}
	return TRUE;
}

int getelevationDEMMIN(tcoords &coordsp)
{
	int n = coordsp.GetSize();
	for (int i=0; i<n; ++i)
		{
		++groundHeightCnt;
		DEMARRAY.height(&coordsp[i], -1);
		}
	return TRUE;
}

//
// COLOR
//

double colorInterp(double v, double y0, double x0, double y1, double x1) {
  return (v - x0) * (y1 - y0) / (x1 - x0) + y0;
}

double jetComponent(double v) {
  if (v <= -0.75)
    return 0;
  else if (v <= -0.25)
    return colorInterp(v, 0, -0.75, 1.0, -0.25);
  else if (v <= 0.25)
    return 1.0;
  else if (v <= 0.75)
    return colorInterp(v, 1.0, 0.25, 0, 0.75);
  else
    return 0.0;
}

DWORD hexColor(double r, double g, double b) {
  return RGB((int)r,(int)g,(int)b);
}


DWORD jet(double v) {
  return hexColor(jetComponent(v * 2 - 1 - 0.5) * 255, jetComponent(v * 2 - 1) * 255, jetComponent(v * 2 - 1 + 0.5) * 255);
}


COLORREF hsv2rgb(double h, double s, double v, double a) {
	double r, g, b;
	int i;
	double f, p, q, t;
 
	// Make sure our arguments stay in-range
	h = max(0, min(360, h));
	s = max(0, min(100, s));
	v = max(0, min(100, v));
	//ASSERT(h<=50 || h>=300);
 
	// We accept saturation and value arguments from 0 to 100 because that's
	// how Photoshop represents those values. Internally, however, the
	// saturation and value are calculated from a range of 0 to 1. We make
	// That conversion here.
	s /= 100;
	v /= 100;
 
	if(s == 0) {
		// Achromatic (grey)
		r = g = b = v;
		return RGB(r*0xFF, g*0xFF, b*0xFF);
	}
 
	h /= 60; // sector 0 to 5
	i = (int)h;
	f = h - i; // factorial part of h
	p = v * (1 - s);
	q = v * (1 - s * f);
	t = v * (1 - s * (1 - f));
 
	switch(i) {
		case 0:
			r = v;
			g = t;
			b = p;
			break;
 
		case 1:
			r = q;
			g = v;
			b = p;
			break;
 
		case 2:
			r = p;
			g = v;
			b = t;
			break;
 
		case 3:
			r = p;
			g = q;
			b = v;
			break;
 
		case 4:
			r = t;
			g = p;
			b = v;
			break;
 
		default: // case 5:
			r = v;
			g = p;
			b = q;
			break;
	}
	return RGBA(a*0xFF, r*0xFF, g*0xFF, b*0xFF);
}

//double smodes[] = {	MAXSTEPS, MAXSTEPS, MAXSTEPS, MAXSTEPS,	3,		3,		3,		3,			3, NULL };
const char *modes[] = { "DEMA",	"DEM0",	"DEM1",	"DEM2", "DEM3"		"GPSV",	"NED",	"GEO",	"EARTH",	"GOOG", NULL };
tgetelevation *fmodes[] = { getelevationDEM, getelevationDEM0, getelevationDEM1, getelevationDEM2, getelevationDEM3, getelevationGPSV, getelevationNED, getelevationGEO, getelevationEARTH, getelevationGOOGMAINFORCED, NULL };



const char *copt = "TAFC@GPWRL""01-2B""MO$KNX""S567";
enum { OTEST = 0, OAREA, OFULLDIST, OCACHED, OCROSSHAIR, OGRID, OPOI, OWATERFLOW, ORIVERS, ORWLOC,   
O0,		//pixrep
O1,		//bilin	
OMINUS,	// no highres	
O2PASS,	// -1:skip 0:auto 1:forced			
OPLANB,	// only foe debug
OMAPPER, OOVERLAY, OLINK, OKML, ONOLINES, ONOLINES2, 
OSCANNER, OFLOWDOWN, OFLOWUP, ORAIN, ONUM };

int GetCacheMaxDays(const char *kmzfile)
{
	if (strstr(kmzfile,"\\" RIVERSBASE)) // rivers
		return MAXEXPIRE;
	const char *opt=strrchr(kmzfile, ')');
	if (opt && strchr(opt, copt[OOVERLAY])) // overlays
		return MAXEXPIRE;
	return 30; // for anything else
}



static CSection MCC;

class CRiver;

typedef struct {short int x, y; float elev; float dist; } smap;
static int cmpsmap(smap *a1, smap *a2)
	{
		if (a2->elev>a1->elev) return -1;
		if (a2->elev<a1->elev) return 1;
		if (a2->dist>a1->dist) return -1;
		if (a2->dist<a1->dist) return 1;
		return 0;
	}

#define ESCAPEMINELEV 1 // at least 1m lower
#define ESCAPESTEPS (2*MAXESCAPING*2) // 50*5m = 250m radius
#define ESCAPEELEV 100
#define cmap(x,y) CMAP[(x)+ESCAPESTEPS*(y)]

#define filled ang
typedef struct {
	int i, n;
	smap coords[8];
} smap8;

class CEscapeMap {
tcoord *CMAP;
float minelev;
float maxalt, minalt;
int newx, newy;
int propagate, dump;
smap8 *coords;

public:

int ConnectDistance(int x, int y, double dist, tcoords &newcoords)
{
	if (x==ESCAPESTEPS/2 && y==ESCAPESTEPS/2)
		return -1; // found it!
	tcoord &c = cmap(x, y);
	if (c.filled<=0)
		return TRUE;
	ASSERT(c.filled>0);
	c.filled = 0;
	// propagate
	smap8 sm;
	sm.n = 0;
	for (int i=-1; i<=1; ++i)
		for (int j=-1; j<=1; ++j)
			if (i!=0 || j!=0)
				{
				register int cx = x+i;
				register int cy = y+j;
				if (cx<0 || cx>=ESCAPESTEPS || cy<0 || cy>=ESCAPESTEPS || cmap(cx,cy).filled<=0)
					continue;
				smap &sc = sm.coords[sm.n++];
				sc.x = cx;
				sc.y = cy;
				double dx = ESCAPESTEPS/2-cx;
				double dy = ESCAPESTEPS/2-cy;
				sc.elev = 0;
				sc.dist =(float)( sqrt(dx*dx+dy*dy)+sqrt((double)(i*i+j*j))+dist );
				}
	if (sm.n==0) return TRUE;
	qsort(sm.coords, sm.n, sizeof(sm.coords[0]), (qfunc *)cmpsmap);
	ASSERT(sm.coords[0].dist<=sm.coords[sm.n-1].dist);
	for (int i=0; i<sm.n; ++i)
		if (ConnectDistance(sm.coords[i].x, sm.coords[i].y, sm.coords[i].dist, newcoords)<0)
			{
			tcoord &c = cmap(sm.coords[i].x, sm.coords[i].y);
			ASSERT( CheckLL(c.lat, c.lng) );
			newcoords.push( c );
			c.filled = 1;
			return -1;
			}
	return TRUE;
}

int FillCell(int x, int y, float fillelev)
{
	if (x<=0 || x>=ESCAPESTEPS-1 || y<=0 || y>=ESCAPESTEPS-1)
		{
		// not found / overflow
		minelev = MAXELEV;
		return -1;
		}

	tcoord &c = cmap(x,y);
	// no data
	if (c.elev<=0)
		return -1;
	// too high
	if (c.elev>fillelev)
		return 1;
	// already filled
	if (c.filled>0)
		return 1;
	// fill cell
	c.filled = fillelev;
	if (c.elev < minelev - ESCAPEMINELEV)
		{
		// found it!
		minelev = c.elev;
		newx = x, newy = y;
		return -1;
		}
	return 0;
}

#if 0
int PropagateCell(short int x, short int y, double fillelev)
{
  if (fillelev>0)
	{
	int ret = FillCell(x, y, fillelev);
	if (ret<0)
		return -1;
	if (ret>0)
		return 1;
	}
  else
	{
	fillelev = -fillelev;
	}

	++propagate;
	smap8 &sm = coords[propagate];
	if (propagate>1000)
		{
		//ASSERT(propagate<ESCAPESTEPS*ESCAPESTEPS/2);
		dump = TRUE;
		if (propagate>ESCAPESTEPS*ESCAPESTEPS)
			{
			Log(LOGERR, "Escape propagation maxed out!");
			--propagate;
			return -1;
			}
		//--propagate;
		//return TRUE;
		}
	// propagate
	sm.n = 0;
	for (int i=-1; i<=1; ++i)
		for (int j=-1; j<=1; ++j)
			if (i!=0 || j!=0)
				{
				register short int cx = x+i;
				register short int cy = y+j;
				if (cx<0 || cx>=ESCAPESTEPS || cy<0 || cy>=ESCAPESTEPS || cmap(cx, cy).filled>0)
					continue;

				smap &c = sm.coords[sm.n++];
				c.x = cx; c.y = cy;
				c.elev = (float)cmap(cx, cy).elev;
				double dx = ESCAPESTEPS/2-cx;
				double dy = ESCAPESTEPS/2-cy;
				c.dist =(float)( sqrt(dx*dx+dy*dy) );
				}
	ASSERT(sm.n<=8);
	if (sm.n>0)
		{
		qsort(sm.coords, sm.n, sizeof(sm.coords[0]), (qfunc *)cmpsmap);
		//coords.Sort(cmpsmap);
		ASSERT(sm.coords[0].elev<=sm.coords[sm.n-1].elev);
		for (int i=0; i<coords[propagate].n; ++i)
		  //if (cmap(x,y).filled<=0)
			if (PropagateCell(coords[propagate].coords[i].x, coords[propagate].coords[i].y, fillelev)<0)
				{
				/*
				tcoord &c = cmap(coords[i].x, coords[i].y);
				if (c.filled == MinFill(coords[i].x, coords[i].y))
					c.elev = minelev;
				//if (minelev<MAXELEV)
				//	res.push( c );
				*/
				--propagate;
				return -1;
				}
		}
	--propagate;
	return TRUE;
}
#else

BOOL getsmap(short int x, short int y, smap8 &sm)
{
	// propagate
	sm.i = 0;
	sm.n = 0;
	for (int i=-1; i<=1; ++i)
		for (int j=-1; j<=1; ++j)
			if (i!=0 || j!=0)
				{
				register short int cx = x+i;
				register short int cy = y+j;
				if (cx<0 || cx>=ESCAPESTEPS || cy<0 || cy>=ESCAPESTEPS || cmap(cx, cy).filled>0)
					continue;

				smap &c = sm.coords[sm.n++];
				c.x = cx; c.y = cy;
				c.elev = (float)cmap(cx, cy).elev;
				double dx = ESCAPESTEPS/2-cx;
				double dy = ESCAPESTEPS/2-cy;
				c.dist =(float)( sqrt(dx*dx+dy*dy) );
				}
	if (sm.n>1)
		{
		qsort(sm.coords, sm.n, sizeof(sm.coords[0]), (qfunc *)cmpsmap);
		ASSERT(sm.coords[0].elev<=sm.coords[sm.n-1].elev);
		return TRUE;
		}
	return FALSE;
}


int PropagateCell(short int x, short int y, float fillelev)
{
  int propagate = 0;
  while (propagate>=0)
  {
	if (propagate>1000)
		{
		//ASSERT(propagate<ESCAPESTEPS*ESCAPESTEPS/2);
		//dump = TRUE;
		if (propagate>ESCAPESTEPS*ESCAPESTEPS)
			{
			Log(LOGERR, "Escape propagation maxed out!");
			return -1;
			}
		//--propagate;
		//return TRUE;
		}

	int ret = 0;
	if (fillelev>0)
		{
		ret = FillCell(x, y, fillelev);
		}
	else
		{
		ret = 0;
		fillelev = -fillelev;
		}
	if (ret<0) // abort
		return -1; 
	if (ret==0) // dive down
		getsmap(x, y, coords[++propagate]);
	smap8 &sm = coords[propagate];
	if (sm.i < sm.n)
		x = sm.coords[sm.i].x, y = sm.coords[sm.i].y, sm.i++;
	else	
		--propagate; // return
	}
  return TRUE;
}

#endif




int FillCMAP(float elev)
{
CTEST("A");
	for (short int m=1; m<ESCAPEELEV; ++m)
		for (int x=0; x<ESCAPESTEPS; ++x)
			for (int y=0; y<ESCAPESTEPS; ++y)
				if (cmap(x,y).filled>0)
					if (PropagateCell(x, y, -(elev+m))<0)
						return -1;
CTEST("B");
	return FALSE;
}

CEscapeMap(const tcoord &coord)
{
#ifdef DEBUGTHERMAL
	dump = TRUE;
#else
	dump = FALSE;
#endif
	propagate = 0;
	minelev = coord.elev;
	newx = newy = -1;
	CMAP = NULL;
	coords = NULL;

	if (minelev<=0)
		return;

	//EscapeMinElev
	LLRect box = LLDistance(coord.lat, coord.lng, MINSTEP*ESCAPESTEPS);
	
	int ncmap = ESCAPESTEPS*ESCAPESTEPS;
	CMAP = new tcoord[ncmap];
	coords = new smap8[ncmap+1];
	coords[0].i = coords[0].n = 0;

	maxalt = coord.elev, minalt = coord.elev;
	for (int x=0; x<ESCAPESTEPS; ++x)
		for (int y=0; y<ESCAPESTEPS; ++y)
			{
			tcoord &c = cmap(x,y) = tcoord(box.lat1 + y*(box.lat2-box.lat1)/ESCAPESTEPS, box.lng1 + x*(box.lng2-box.lng1)/ESCAPESTEPS);
			DEMARRAY.height(&c);
			if (c.elev<0) 
				c.elev = MAXELEV;
			else
				{
				if (c.elev>maxalt) maxalt = c.elev;
				if (c.elev<minalt) minalt = c.elev;
				}
			c.filled = -1;
			}

}

void Dump(BOOL B)
{
	static int nfile = 0;
#ifdef DEBUGX
	for (int i=0; i<newcoords.GetSize(); ++i)
		Log(LOGINFO, "%d %.6f,%.6f,%.6f", i, newcoords[i].lat, newcoords[i].lng, newcoords[i].elev);
#endif
	if (!B) ++nfile;
    CImage c;
    c.Create(ESCAPESTEPS, ESCAPESTEPS, 24);
	for (int x=0; x<ESCAPESTEPS; ++x)
		for (int y=0; y<ESCAPESTEPS; ++y)
			{
			tcoord &p = cmap(x,y);
			double h = (p.elev-minalt)/(maxalt-minalt);
			c.SetPixel(x, ESCAPESTEPS-y-1,hsv2rgb(360*h, 100, (h<0 || h>1) ? 0 : (p.filled>=0 ? 50 : 100)) );
			if (p.filled==1)
				c.SetPixel(x, ESCAPESTEPS-y-1, RGB(0,0,0xff) );
			}
	if (B>0) c.SetPixel(newx, ESCAPESTEPS-newy-1, RGB(0x80,0x80,0x80) );
	c.SetPixel(ESCAPESTEPS/2, ESCAPESTEPS-ESCAPESTEPS/2-1, RGB(0xff,0xff,0xff));
	vars name = MkString("CMAP%0.2d%s.png", nfile, !B ? "A" : "B");
	c.Save(name);
	Log(LOGINFO, "Dumping THERMAL #%d : %s", nfile, filename);
}

BOOL Escape(tcoord &newcoord, tcoords &newcoords)
{
	// if invalid elev don't bother
	if (minelev<=0)
		return FALSE;
	// if very low diff elevation don't bother
	if (maxalt-minalt<=MINELEVDIFF)
		return FALSE;
	

	float elev = minelev;
	if (dump) Dump(FALSE);
	if (FillCell(ESCAPESTEPS/2, ESCAPESTEPS/2, elev)<0)
		return FALSE; // no data
	FillCMAP(elev);
	if (dump) Dump(minelev>=elev ? -1 : 1);

	if (minelev>=elev)
		return FALSE;

CTEST("D");
	
	ConnectDistance(newx, newy, 0, newcoords);
CTEST("E  \r");

	newcoord = cmap(newx, newy);
	newcoords.push(newcoord);
	ASSERT(newcoords[newcoords.length()-1].elev==newcoord.elev);
	ASSERT(newcoord.elev == minelev);
CTEST("F  \r");


	return TRUE;
}

~CEscapeMap(void)
{
	if (CMAP)
		delete [] CMAP;
	if (coords)
		delete [] coords;
}

};

static int Rap(double g)
{
	if (g>=GRADEM[4])
		return 4;
	if (g>=GRADEM[3])
		return 3;
	if (g>=GRADEM[2])
		return 2;
	if (g>=GRADEM[1])
		return 1;
	if (g>=GRADEM[0])
		return 0;
	return -1;
}

static DWORD RapColor(int n)
{
	#define RAPINVERT RGB(0x66,0,0)
	const COLORREF rapcolors[] = { RGB(0x00,0xFF,0xFF), RGB(0xFF,0xFF,0), RGB(0xFF,0x7F,0), RGB(0xFF,0,0), RAPINVERT };
	return rapcolors[n];
}



CString resText(double res)
{
	DWORD c = resColor(res);
	const char *quality[] = { "High", "Med", "Low" };
	return MkString("<span style='background-color:#%2.2X%2.2X%2.2X'>%.0fm (%s)</span>", (c>>0)&0xFF, (c>>8)&0xFF, (c>>16)&0xFF, res, quality[resnum(res)]);
}

CString resCredit(int credit)
{
	return credit<0 ? "Unknown" : demcredits[credit].GetStr(ITEM_DESC);
}

typedef CArrayList<tpoints> trklist;
typedef BOOL tisbetterres(double res, double resg);







// ================ REUSE GOOGLE DATA =============


class CGOOGData {
// Google points
CArrayList<tcoord *> list;
CArrayList<unsigned int> tmp;

public: 
int calls, missed;

BOOL Add(CScan *scan)
	{
	//ASSERT(!IsSimilar(scan->name, "Cripple"));
	if (!scan->trk)
		return TRUE;
	tcoords &coords = scan->trk->Coords();
	if (scan->date+GOOGDAYS<GetTime()) // valid only 6 months
		return TRUE;

	for (int i=0; i<coords.GetSize(); ++i)
		{
		tcoord &c = coords[i];
		if (c.resg>0)
			{
			if (c.elev>MAXELEV)
				return FALSE; // error
			//c.lat = (float)rnd(c.lat, 1e6);
			//c.lng = (float)rnd(c.lng, 1e6);
			list.AddTail(&c);
			}
		}
	return TRUE;
	}

inline unsigned int Find(tcoord &c)
	{
	//int aprox = -1;
	//tcoord f;
	//f.lat = (float)rnd(c.lat, 1e6);
	//f.lng = (float)rnd(c.lng, 1e6);
	//if (Distance(f,c)>1)
	//	Log(LOGALERT, "GOOGDATA Rounding too big!!!");
	register int found = list.Find(&c);
	if (found<0) 
		{
		/*
		if (aprox<0 || aprox>=list.GetSize())
			return FALSE;

		// check if within <1m
		if (Distance(c.lat, c.lng, list[aprox]->lat, list[aprox]->lng)>1)		
			if (++aprox>=list.GetSize()-1 || Distance(c.lat, c.lng, list[aprox]->lat, list[aprox]->lng)>1)
				return FALSE;

		// found good approx
		found = aprox;
		*/
		return 0;
		}

	// actually found!
	return CScan::encode(list[found]->elev, list[found]->resg);
	}

void Sort(void)
	{
	list.Sort(LLE::cmp);
	}

void Reset(void)
	{
	calls = missed = 0;
	list.Reset();
	}




int getelevationGOOG(tcoords &coords)
	{
	if (list.GetSize()==0)
		return FALSE;


	register int n = coords.length();
	tmp.SetSize(n);
	for (register int i=0; i<n; ++i)
		{
		tcoord &c = coords[i];
		if (!(tmp[i]=Find(coords[i])))
			{
			int end = 0;
			if (gkey::needs(coords, i, end))
				{
				// abort
				++missed;
				return FALSE;
				}
			else
				{
				// skip
				for ( ;i<end; ++i)
					tmp[i] = 0;
				--i;
				}
			}
		}
	
	++calls;
	for (int i=0; i<n; ++i)
		if (tmp[i]!=0)
			{
			register float elev, resg;
			CScan::decode(tmp[i], elev, resg);
			// get elevation ONLY if GOOG is better
			tcoord &c = coords[i];
			if (isBetterResG(c.res, resg))
				c.elev = elev;
			c.resg = resg;
			}
	return TRUE;
	}

} GOOGDATA;





class CanyonMapper 
{
DownloadFile f;
int *iopts;
const char *query;
CStringArrayList *filelist;
CKMLOut *out;
tgetelevation *getelevationGOOG;

//double maxstep;
tgetelevation *getelevation;

//int abortcalc;
CString name, fname;
tcoords trkcoords;
CIntArrayList raps;

void Output(CKMLOut &out, const char *networklink = NULL)
{
  if (trk.length()<=0)
	{
	//out += KMLName("ERROR", MkString("Elevation not available! Check coverage at http://%s/rwr/?rwz=DEM,0,0,0,KC", server), 2);
	out += "<Placemark>";
	out += KMLName("ERROR", MkString("Elevation not available! Check coverage at http://%s/rwr/?rwz=DEM,0,0,0,KC", server));
	out += MkString("<Style><BalloonStyle><text>" CDATAS "<b><font size=\"+1\">$[name]</font></b><br/><br/><div style='width:auto'>$[description]</div>" CDATAE "</text></BalloonStyle><IconStyle><Icon><href>http://sites.google.com/site/rwicons/%s.png</href></Icon><scale>%g</scale><hotSpot x=\"0.5\" y=\"0.5\" xunits=\"fraction\" yunits=\"fraction\"/></IconStyle><LabelStyle><scale>%g</scale></LabelStyle></Style>", "error", 0.5, 0.6);
	out += MkString("<Point><coordinates>%s</coordinates></Point>", CCoord3(mappoint.lat,mappoint.lng));
	out += "</Placemark>\n";
	return;
	}

  if (trk.length()<=1)
	  return;

  // output results
  createLineString(out, networklink);
  drawHeightMap();

  // Thermal altitude imaging
  if (iopts[OTEST])
	  drawThermal();
}

int iopt[ONUM];

public:

inline tcoords &Trk() { return trkcoords; }  // unprocessed track

CanyonMapper(CKMLOut *out, const char *name, const char *mode = NULL, int *customiopt = NULL, const char *query = NULL, CStringArrayList *filelist = NULL) 
{
  int m = 0;
  if (customiopt)
	memcpy(iopt, customiopt, sizeof(iopt));
  else
	memset(iopt, 0, sizeof(iopt));

  if (mode!=NULL)
		{
		if (IsSimilar(mode, "nog"))
			iopt[O2PASS] = -1;
		else
			{
			m = IsMatchN(mode, modes);
			if (m<0) 
				{
				Log(LOGERR, "Invalid compute mode %s", mode);
				return;
				}
			}
		}
  getelevation = fmodes[ m ];
  //maxstep = smodes[m]*MINSTEP;
  this->iopts = iopt;
  this->query = query;
  this->filelist = filelist;
  this->out = out;

  // choose google key
  //gkey = out ? &maingkey : scangkey;
  if (out)
	getelevationGOOG = iopts[O2PASS]>0 ? getelevationGOOGMAINFORCED : getelevationGOOGMAIN;
  else
	getelevationGOOG = getelevationGOOGSCAN;

  Reset(name);
}


void Reset(const char *name)
{
  // init
  this->name = name;
  fname = ElevationFilename(name);
  //abortcalc = 0;
  groundHeightCnt = 0;
  googerror = 0;
  googcache = 0;
  trkcoords.Reset();
  raps.Reset();
}


/*
double Grade(int i) {
#if 0
  if (i==0 || i>trk.length()-3)
    return (alts[i+1]-trk[i].elev)/dists[i+1];
  else {
    return (trk[i].dist*(trk[i].elev+alts[i+1]+alts[i+2]-3*alts[i-1]) + 2*dists[i+1]*(alts[i+1]+alts[i+2]-alts[i-1]-trk[i].elev) - dists[i+2]*(alts[i-1]+trk[i].elev+alts[i+1]-3*alts[i+2])) /
            (3*trk[i].dist*trk[i].dist + 4*dists[i+1]*dists[i+1] + 4*dists[i+1]*dists[i+2] + 3*dists[i+2]*dists[i+2] + 2*trk[i].dist*(2*dists[i+1]+dists[i+2]));
  }
#else
  if (i<=0)
	  return -1;
  if (i>=trk.length()-1)
	  return 0;
  return (alts[i+1]-trk[i].elev)/dists[i+1];
  //double g2 = (alts[i+1]-alts[i-1])/(dists[i+1]+trk[i].dist);
  //double g2 = 0; //(alts[i+2]-trk[i].elev)/(dists[i+1]+dists[i+2]);
  //return min( 0, min(g1, g2));
#endif
}
*/
/*
int issamerappel(int i, int j)
{
if (Rap(trk[j-1].grade)>1 && Rap(trk[j+1].grade)>1 && Rap(trk[j].grade)<=1)
	return FALSE;
if (Rap(trk[j].grade)>0)
	return TRUE;

#if 0
if (dists[j+1]<MINRAPDIST)
 if (Rap(grades[j+1])>0)
	return TRUE;
#endif

return FALSE;
}
*/

// process segment

int width, height, ntiles, heighttotal;
int maxcnt;
float maxdist, maxft;
float minalt, maxalt;
float minres, maxres;
float minresg, maxresg;

tcoord mappoint;
tpoints trk;
traplist rappels;
int googerror, googcache;

#define FLAGSIZE 16
#define TEXTSIZE 16
#define TILEHEIGHT 60

inline int ycoord(double h) { return (int)(FLAGSIZE+height*(1-((h)-minalt)/(maxalt-minalt))); }
inline int xcoord(double d) { return (int)(width*((d)/maxdist)); }


void RappelInfo(trap &r, CString &icon, CString &name, CString &desc)
{
			int rap = Rap(r.avggrade);
			double m = trk[r.start].tdist; 
			icon = MkString("rg%d", rap);
			name = MkString("%.0fft", r.ft);
			desc += MkString("<img style=\"margin-left:-8px;margin-bottom:-5px;width:20px;height:20px\" src=\"http://sites.google.com/site/rwicons/%s.png\"/>", icon);
			desc += MkString(" %.0fft @%.2fmi ", r.ft, m*km2mi/1000);
			desc += MkString("[%.0fm @%.2fkm] ", r.m, m/1000);
			desc += MkString(": Angle:%.0f&#186; - %.0f&#186; ", g2deg(r.maxgrade), g2deg(r.mingrade));
			desc += MkString("Grade:%.0f%% - %.0f%% ", r.maxgrade*100, r.mingrade*100);
			desc += MkString("ID:%s", IdStr(r.id));
}

//typedef void(CanyonMapper::*getelevation)(int n, tcoords *coordsp);
void cascadeMinimum()
{
	// force cascading minimum
	float minelev = trkcoords[0].elev;
	if (minelev<0) minelev = 0;
	for (int i=0; i<trkcoords.length(); ++i)
		{
		if (trkcoords[i].elev>0)
			minelev = min(trkcoords[i].elev, minelev);
		trkcoords[i].elev = minelev;
		}
	//smoothHeight(bezier);
}

void sharpHeight()
{
	// transition to 10m minstep
	CArrayList<float> elev(trkcoords.length());
	for (int i=1; i<trkcoords.length()-1; ++i)
		elev[i] = 3*trkcoords[i].elev - trkcoords[i-1].elev - trkcoords[i+1].elev;
	for (int i=1; i<trkcoords.length()-1; ++i)
		trkcoords[i].elev = elev[i];
}



void doubleStep()
{
	// transition to 10m minstep
	tcoords newtrkcoords;
	for (int i=0; i<trkcoords.length(); i+=2)
		newtrkcoords.push(trkcoords[i]);
	trkcoords = newtrkcoords;
}


int  halfTrack()
{
	int removed = 0;
	// transition to 10m minstep
	tcoords newtrkcoords;
	for (int i=0; i<trkcoords.length()-1; i+=2)
			newtrkcoords.push(trkcoords[i]);
	trkcoords = newtrkcoords;
	return removed;
}

int  removeEqualHeight()
{
	int removed = 0;
	// transition to 10m minstep
	tcoords newtrkcoords;
	for (int i=0; i<trkcoords.length()-1; ++i)
		if (trkcoords[i].elev!=trkcoords[i+1].elev)
			newtrkcoords.push(trkcoords[i]);
		else
			++removed;
	trkcoords = newtrkcoords;
	return removed;
}

int replaceEqualHeight()
{
	// transition to 10m minstep
	int replaced = 0;
	for (int i=1; i<trkcoords.length()-2; ++i)
		if (trkcoords[i].elev==trkcoords[i+1].elev)
			{
			trkcoords[i].elev = (trkcoords[i-1].elev+trkcoords[i+1].elev)/2;
			++replaced;
			}
	return replaced;
}




void smoothHeight(tsmooth func)
{
	// smooth
	int n = trkcoords.length()-2;
	if (n<=1) return;
	CArrayList<float> newe(n);
	for (int i=1; i<n; ++i)
		newe[i] = (float)func(0.5, trkcoords[i-1].elev, trkcoords[i].elev, trkcoords[i+1].elev, trkcoords[i+2].elev);
	for (int i=1; i<n; ++i)
		trkcoords[i].elev = newe[i];
}

#if 0

inline double gboost(tpoint &p)
{
	double res = isBetterResG(p.res, p.resg) ? p.resg : p.res;
	return res>MINSTEP ? MINSTEP : 0; //log(res)/log(MINSTEP)
	//return res*BOOSTMUL;
}


void computeGrade() 
{
  CArrayList<vector> tangents;
  int n = trk.GetSize();
  // set a default on a 4 pix window
  for (int i=1; i<n-2; ++i)
	{
	tpoint &trki2 = trk[i+2];
	tpoint &trki1 = trk[i-1];
	trk[i].grade2 = trk[i].grade = -(trki2.elev-trki1.elev)/(trki2.tdist-trki1.tdist-gboost(trk[i]));
    }

#ifdef BREGRADE
  // recompute grade
  CTEST("ProcTrackREG");
  for (int i=1; i<n-2; ++i)
#if 0
  if (trk[i].res>MINSTEP)
	{
	//for (int j=i; j<n-1 && d<8; ++j)
	//	d += trk[j].dist;
	//trk[i].grade = -(trkcoords[i].elev-trkcoords[j].elev)/(d-2);
	double d = trk[i].dist + trk[i-1].dist/2 + trk[i+1].dist/2;
	double newg = -(trk[i-1].elev-trk[i+1].elev)/(d-gboost(trk[i]));
	if (newg < trk[i].grade)
		trk[i].grade = trk[i].grade2 = newg;
	double g = -(trk[i].grade - trk[i-1].grade);
	if (g<0)
		trk[i].grade += g;
	}
#else
  {
#if 1
	  
	  tangents.SetSize(trk.length());
	  //CArrayList<vector> trkvec(trk.length());
	  //for (int i=0; i<trk.length(); ++i)
	  //  trkvec[i] = vector(trk[i].tdist, trk[i].elev);	 
/*
	  vector tests[][4] = 
	  { 
			{vectIN
			or(0,1), vector(1,1), vector(2,0), vector(3,0) },
			{vector(0,1), vector(1,0.75), vector(2,0.25), vector(3,0) },
			{vector(0,1), vector(1,0.66), vector(2,0.33), vector(3,0) },
			{vector(0,1), vector(1,0.33), vector(2,0.33), vector(3,0) },
			{vector(0,3), vector(1,2), vector(2,1), vector(3,0) },
			{vector(0,0), vector(1,0), vector(2,0), vector(3,0) },
	  };
*/

	  //for (int i=0; i<6; ++i)
	  for (int i=2; i<trk.length()-2; ++i)
		  {
		  double DIFF = 0.25;
		  //vector *vtrk = &tests[i][1];
		  tpoint *vtrk = &trk[i];
		  vector p, p25, p50, p75, p100;
		  _splinepv(0, vtrk, p);
		  _splinepv(0.50-DIFF, vtrk, p25);
		  _splinepv(0.50, vtrk, p50);
		  _splinepv(0.50+DIFF, vtrk, p75);
		  _splinepv(1, vtrk, p100);

		  double g = (p100.y-p.y)/(p100.x-p.x);
		  double g25 = (p75.y-p25.y)/(p75.x-p25.x);
		  double g50a = (p.y-p50.y)/(p.x-p50.x);
		  double g50b = (p100.y-p50.y)/(p100.x-p50.x);
		  tangents[i] = p75-p25;
		  trk[i].grade = -tangents[i].y/tangents[i].x;
		  //trk[i].grade = min(min(g50a, g50b), g25);
	      }
	  for (int i=1; i<trk.length()-1; ++i)
		{
		vector t1 = 0.5 * (tangents[i-1]+tangents[i]);
		vector t2 = 0.5 * (tangents[i+1]+tangents[i]);
		trk[i].grade = max( trk[i].grade2, max( trk[i].grade, max(-t1.y/t1.x, -t2.y/t2.x)));
		}
	  /*
	  double g1 = (trkcoords[i+2].elev-trkcoords[i].elev)/(trk[i].dist + trk[i+1].dist - gboost(trk[i])*2);
	  double g2 = (trkcoords[i+1].elev-trkcoords[i-1].elev)/(trk[i-1].dist + trk[i].dist - gboost(trk[i])*2);
	  trk[i].grade = min(g1, g2);
	  */
#else
	  double d = 0;
	  for (int j=0; i+j<n-1 && d<trk[i].res; ++j)
		  d += trk[i+j].dist - gboost(trk[i+j]);	
	  trk[i].grade =  -(trkcoords[i+j].elev-trkcoords[i].elev) / d;
#endif
  }
#endif
#endif

#ifdef BORIG2
	// delete
	{
	CDoubleArrayList grade(trk.length());
	for (int i=0; i<trk.length(); ++i)
		grade[i] = trk[i].grade;
	for (int i=1; i<trk.length()-2; ++i)
		{			
		if (trk[i].elev == trk[i+1].elev)
			{
#if 1
			 double g1 = (trk[i-1].elev-trkcoords[i].elev)/(trk[i-1].dist+trk[i].dist/2);
			 double g2 = (trk[i+1].elev-trkcoords[i+2].elev)/(trk[i+1].dist+trk[i].dist/2);
			 grade[i-1] = g1; grade[i+1] =  g2;
			 grade[i] = (g1+g2)/2;
#else
			 //trk[i+1].grade = (trk[i].elev-trkcoords[i+1].elev)/(trk[i+1].dist+trk[i].dist/2);
			 grade[i] = (trk[i-1].grade+trk[i+1].grade)/2;
			 //double g1 = g2deg(trk[i-1].grade); 
			 //double g2 = g2deg(trk[i+1].grade);
			 //grade[i] = deg2g((g1+g2)/2);
#endif
			 //if (g1>=GRADEM[0] && g2>=GRADEM[0])
				{
				//trk[i].elev = trk[i-1].

				//trkcoords[i+1].lat = (trkcoords[i].lat + trkcoords[i+1].lat)/2;
				//trkcoords[i+1].lng = (trkcoords[i].lng + trkcoords[i+1].lng)/2;
				//trk[i].dist
				//trk[i].dist
				//trk.Delete(i+1);
				//--i;
				}
			}
			
			//double booster = trk[i].res/MINSTEP;
			//if (booster>1)
			//	trk[i].grade *= 1+booster/4;
		}
	for (int i=0; i<trk.length(); ++i)
		trk[i].grade = grade[i];
	}
#endif
}
#endif



double grade2D(tpoint &p, int alg)
{
	LLE lle(p);
	DEMARRAY.height(&lle, alg);
	return lle.elev;
}

double grade1D(tpoint *trk, int alg) 
{
		double fx = 0 , fy =0, v2 = sqrt(2.0);
		double gx = trk[0].res, gy = trk[0].res;
		float &z1 = trk[-1].elev;
		float &z2 = trk[-1].elev;
		float &z3 = trk[-1].elev;
		float &z4 = trk[0].elev;
		float &z5 = trk[0].elev;
		float &z6 = trk[0].elev;
		float &z7 = trk[1].elev;
		float &z8 = trk[1].elev;
		float &z9 = trk[1].elev;
		// 123
		// 456
		// 789
		switch (alg)
			{
			case GF1:
				fy=(z8-z2)/(2*gy); 
				fx=(z6-z4)/(2*gx);
				break;
			case GF2:
				fx=(z3-z1+v2*(z6-z4)+z9-z7)/((4+2*v2)*gx);
				fy=(z7-z1+v2*(z8-z2)+z9-z3)/((4+2*v2)*gy);
				break;
			case GF3:
				fx=(z3-z1+z6-z4+z9-z7)/(6*gx);
				fy=(z7-z1+z8-z2+z9-z3)/(6*gx); 
				break;
			}
		return -fy; //sqrt(fy*fy+fx*fx);

}


#ifdef DEBUG
void computeGradeDeriv() 
{
 int n = trk.GetSize();
 CDoubleArrayList g(n);
 for (int i=1; i<n-3; ++i)
    {
    g[i] = grade2D(trk[i], GF3);
	//trk[i].grade2 = trk[i].grade;
    //trk[i].grade2 = bezier(0.5, trk[i-1].grade, trk[i].grade, trk[i+1].grade, trk[i+2].grade);
    //trk[i].grade2 = bicubic(0.5, trk[i-1].grade, trk[i].grade, trk[i+1].grade, trk[i+2].grade);
    }

 for (int i=1; i<n-3; ++i)
	 {
	 trk[i].grade2 = (float)bicubic(0.5, g[i-1], g[i], g[i+1], g[i+2]);
     }

}

void testDeriv() 
{
  int n = trk.GetSize();
  CDoubleArrayList deriv2(n), height(n), rap(n), rapn(n), gr(n), sharp(n);

  // smooth grade
  for (int i=2; i<n-2; ++i)
    //gr[i] = (0.25*trk[i-1].grade+0.25*trk[i+1].grade+0.5*trk[i].grade);
	{
	tpoint &trki2 = trk[i];
	tpoint &trki1 = trk[i-1];
	//trk[i].grade2 = trk[i].grade; // spline(0.5, trk[i-1].grade, trk[i].grade, trk[i+1].grade, trk[i+2].grade);
	//trk[i].grade2 = trk[i].grade - trk[i-1].grade;
	//trk[i].grade2 = -(trki2.elev-trki1.elev)/(trki2.tdist-trki1.tdist-gboost(trk[i]));
	//sharp[i] = 3*trk[i].elev-trk[i-1].elev-trk[i+1].elev;
    }

  for (int i=1; i<n-2; ++i)
    {
	//trk[i].grade = gr[i];
	deriv2[i] = trk[i].grade-trk[i-1].grade;
	rap[i] = Rap(trk[i].grade);
	height[i] = 0;
    }
  // fill holes
//  for (int i=1; i<n-2; ++i)
//	  if (deriv2[i]<=0 && deriv2[i-1]>0 && deriv2[i+1]>0)
//		  deriv2[i]  = 0.5;

  double height1, height2;
  height1 = height2 = trk[0].elev;
  int istart = 0, irapmax = 0;
  for (int i=1; i<n-2; ++i)
    {
	if ((deriv2[i]>0 && deriv2[i-1]<=0 && deriv2[i+1]>0) || rap[i]<1)
		{
		double h = 0;
		if (rap[irapmax]>0)
			for (int j=istart; j<i; ++j)
				height[j] = (trk[istart].elev-trk[i].elev)*m2ft;
		// finish prior
	    istart = i;
		irapmax = 0;
		}
	if (rap[i]>0 && rap[i]>irapmax)
		irapmax = i;
	//if (rap[i]>rap[i+1])


	//if (trk[i].grade>0.5 && trk[i-1].grade<=0.5)
	//if (trk[i].grade<0.5)
	//if (deriv2[i]<=0)
	//	height1 = height2 = 0;	
	//double hdiff  = trk[istart].elev-trk[i].elev;
	//height[i] = (hdiff>0 ? hdiff : 0) * m2ft;
	}

 CSymList glist;
 glist.header = "N,ELEV,DIST,D1,D2,H";
 for (int i=2; i<n-2; ++i)
   {
   vara str;
   str.push(CData(trk[i].elev));
   str.push(CData(trk[i].dist));
   str.push(CData(trk[i].grade));
   str.push(CData(trk[i].grade2));
   //str.push(CData(height[i]));
   str.push(CData(trk[i].elev-trk.Tail().elev));
   str.push(trk[i].rapnum>=0 && rappels.GetSize()>0 ? CData(rappels[trk[i].rapnum].m*m2ft) : "0");
   //str.push(CData(trk[i].elev-trk.Tail().elev));
/*
   bez[i] = bezier(0.5, trk[i-1].elev, trk[i].elev, trk[i+1].elev, trk[i+2].elev);
	spl[i] = spline(0.5, trk[i-1].elev, trk[i].elev, trk[i+1].elev, trk[i+2].elev);
	bic[i] = bicubic(0.5, trk[i-1].elev, trk[i].elev, trk[i+1].elev, trk[i+2].elev);
 str.push(CData(bez[i]));
   str.push(CData(bic[i]));
   str.push(CData(spl[i]));
*/
   str.push("");

   //str.push(CData(grade1D(&trk[i], GF1)));
   //str.push(CData(grade1D(&trk[i], GF2)));
   str.push(CData(grade1D(&trk[i], GF3)));

   str.push("");

   //str.push(CData(grade2D(trk[i], GF1)));
   //str.push(CData(grade2D(trk[i], GF2)));
   str.push(CData(grade2D(trk[i], GF3)));
   str.push("Grades");

	trk[i].grade2 = trk[i].grade; // spline(0.5, trk[i-1].grade, trk[i].grade, trk[i+1].grade, trk[i+2].grade);
	//trk[i].grade2 = trk[i].grade - trk[i-1].grade;
	str.push(CData((trk[i-2].elev-trk[i+2].elev)/(4*MINSTEP-MINSTEP))); // <<<< ----
	str.push(CData((trk[i-1].elev-trk[i+2].elev)/(3*MINSTEP-MINSTEP))); //<<<< ====
	str.push(CData((trk[i-1].elev-trk[i+1].elev)/(2*MINSTEP-MINSTEP))); //<<<< ++++
	str.push(CData((trk[i-1].elev-trk[i].elev)/(1*MINSTEP)));

	//str.push("Sides");
	//double c, l, r;
	//str.push(CData(c=(trk[i-1].elev-trk[i].elev)/(trk[i].res)));
	//str.push(CData(r=(trk[i].elevR-trk[i].elev)/(trk[i].res)));
	//str.push(CData(l=(trk[i].elevL-trk[i].elev)/(trk[i].res)));
	//str.push(CData(l+r));

   CSym sym(MkString("%d", i), str.join());
   glist.Add(sym);
   }
 vars name = "trk.csv";
 if (out) name = out->name+name;
 glist.Save(name);

}

#endif

int processLat(tgetelevation getelev, tisbetterres *isbetterres = NULL)
{
	// lateral passes to improve location
	int error = 0;
	tcoords gcoords2;
	for (int i=0; i<trkcoords.length()-1; ++i)		
	if (!isbetterres || isbetterres(trkcoords[i].res,trkcoords[i].resg)) 
		{
		double dist = MINSTEP/2;
		//if (dist<0) dist = trkcoords[i].resg * 1;
		double theta = Bearing(trkcoords[i],trkcoords[i+1]);
		tcoord coord1(trkcoords[i].lat, trkcoords[i].lng), coord2(coord1);
		moveInDirection(coord1, theta-M_PI/2, dist);
		moveInDirection(coord2, theta+M_PI/2, dist);
		gcoords2.push(coord1);
		gcoords2.push(coord2);
		}

	if (!getelev(gcoords2))
		return FALSE;

#ifdef DEBUG
//#define DEBUGLAT
#endif
#ifdef DEBUGLAT
	if (out) 
		{
		*out += KMLFolderStart("Lat", "Lat", TRUE);
		*out+=KMLMarkerStyle("dot", "http://maps.google.com/mapfiles/kml/shapes/placemark_circle.png");
		}
#endif
	for (int i=0, i2=0; i<trkcoords.length()-1; ++i)		
	if (!isbetterres || isbetterres(trkcoords[i].res,trkcoords[i].resg)) 
		{
		unsigned char patched = 0;
		//trkcoords[i].elevL = gcoords2[i2].elev;
		//trkcoords[i].elevR = gcoords2[i2+1].elev;
		//trkcoords[i].elevC = trkcoords[i].elev;
		double oelev = trkcoords[i].elev;
		double oresg = trkcoords[i].resg;
		if (!isbetterres || isbetterres(trkcoords[i].res,gcoords2[i2].resg) )
			if (gcoords2[i2].elev<trkcoords[i].elev)
				{
				++patched;
				trkcoords[i].elev = gcoords2[i2].elev;
				trkcoords[i].resg = gcoords2[i2].resg;
				}
		if (!isbetterres || isbetterres(trkcoords[i].res,gcoords2[i2+1].resg))
			if (gcoords2[i2+1].elev<trkcoords[i].elev)
				{
				++patched;
				trkcoords[i].elev = gcoords2[i2+1].elev;
				trkcoords[i].resg = gcoords2[i2+1].resg;
				}
#ifdef DEBUGLAT
		if (out) 
			{
			vara list;
			list.push(CCoord3(gcoords2[i2].lat, gcoords2[i2].lng));
			list.push(CCoord3(trkcoords[i].lat, trkcoords[i].lng));
			*out += KMLLine("Lat", NULL, list, isBetterResG(trkcoords[i].res,gcoords2[i2].resg) ? RGB(0, 0xFF, 0) : RGB(0, 0, 0xFF));
			list.push(CCoord3(gcoords2[i2+1].lat, gcoords2[i2+1].lng));
			list.push(CCoord3(trkcoords[i].lat, trkcoords[i].lng));
			*out += KMLLine("Lat", NULL, list, isBetterResG(trkcoords[i].res,gcoords2[i2+1].resg) ? RGB(0, 0xFF, 0) : RGB(0, 0, 0xFF));
			vars desc = MkString( "%d patches, elev %.1f -> %.1f, res %.1f -> %.1f", patched, oelev, trkcoords[i].elev, oresg, trkcoords[i].resg);
			*out += KMLMarker("dot", trkcoords[i].lat, trkcoords[i].lng, MkString("%.0f", trkcoords[i].resg));
			*out += KMLMarker("dot", gcoords2[i2].lat, gcoords2[i2].lng, MkString("%.0f", gcoords2[i2].resg));
			*out += KMLMarker("dot", gcoords2[i2+1].lat, gcoords2[i2+1].lng, MkString("%.0f", gcoords2[i2+1].resg));
			}
#endif		
		i2+=2;
		}
#ifdef DEBUGLAT
	  if (out) *out += KMLFolderEnd();
#endif
    return TRUE;
}


int processSides()
{
#ifdef SIDEGRADE
	// lateral passes to improve location
	tcoords newcoords;
	for (int i=0; i<trkcoords.length()-1; ++i)		
		{
#if 0
		//if (dist<0) dist = trkcoords[i].resg * 1;
		double theta = Bearing(trkcoords[i],trkcoords[i+1]);
		tcoord &coord = trkcoords[i];
		tcoord coord1, coord2;
		coord1 = coord2 = coord;
		moveInDirection(coord1, theta-M_PI/2, SIDEGRADE); DEMARRAY.height(&coord1); 
		moveInDirection(coord2, theta+M_PI/2, SIDEGRADE); DEMARRAY.height(&coord2);
		coord.sides = max(coord1.elev, coord2.elev);
#else
		tcoord &coord = trkcoords[i+1];
		double theta = Bearing(trkcoords[i+1],trkcoords[i]);
		//mini = FindMinElev(coord, theta, 3*M_PI/2, 12, SIDEGRADE, newcoords);
		//mini = FindMinElev(coord, theta, M_PI/2, 8, SIDEGRADE, newcoords); // OK! to reinforce rappels
		//mini = FindMinElev(coord, theta, 3*M_PI/4, 12, SIDEGRADE, newcoords);
		int mini = FindMinElev(coord, theta, M_PI, 8, SIDEGRADE, newcoords); //!!!GREAT!!!
		trkcoords[i].sidegrade = (float)GRADESIDE(newcoords[mini].elev - coord.elev);
#endif
		}
#endif
    return TRUE;
}

#define GRADE(elev,dist, res) ((elev)/((dist) + ((res)<MINSTEP ? 1-MINSTEP/2 : 1-MINSTEP)))
#define GRADEI(i,trk) GRADE(trk[i-1].elev-trk[i+1].elev, Distance(trk[i-1], trk[i+1]), isBetterResG(trk[i].res, trk[i].resg) ? trk[i].resg : trk[i].res)

static inline double NORMANG(double ang)
{
	if (ang==InvalidNUM)
		return InvalidNUM;
	while (ang>M_PI)
		ang -= 2*M_PI;
	while (ang<-M_PI)
		ang += 2*M_PI;
	return ang;
}

#define ABSNORMANG(ang) abs(NORMANG(ang))


int processGuide(tpoints &coords, BOOL continuation)
{
	int ret = 0;
	tcoords &trk = trkcoords;

	int size = coords.GetSize();
	tcoord &cstart = coords[0], &cend = coords[size-1];
	LLE start = cstart, end = cend;

	//if (continuation)
	{
	// process bad rappel at rap start
	double cang = Bearing(coords[0], coords[1]);

	int skip = -1;
	for (int i=1; i<trk.GetSize()-1; ++i)
		{
		if (Rap(GRADEI(i,trk))<=0)
			break;
		double ang = Bearing(trk[i-1], trk[i]);
		if(ABSNORMANG(cang-ang)<M_PI/4)
			break;
		skip = i;
		}
	for (int i=0; i<=skip; ++i)
		trk.Delete(0);
	}

	{
	// trim track
	int start = -1, end = -1;
	int trksize = trk.GetSize();
#if 1
	double startdist = MAXSPLITDIST, enddist = MAXSPLITDIST;
	int size = trk.length();
	double seang = Bearing(cstart, cend);
	for (int i=0; i<size; ++i)
		{
		double dist;
		dist = Distance(trk[i], cstart);
		if (startdist>dist)
			startdist = dist, start = i;
		dist = Distance(trk[i], cend);
		if (enddist>dist)
			enddist = dist, end = i;
		}
	/*
	// move down end to align with angle
	if (end>0)
	  for (end=end; end<size; ++end)
		{
		//double ang = Bearing(trk[end-1], trk[end]);
		if (ABSNORMANG(seang-trk[end].ang)<M_PI/4)
			break;
		}
	*/
#else
  	register double t;
	for (int i=0; i<trk.GetSize(); ++i)
		if (DistanceToSegment(trk[i], coords[0], coords[1], t)<MAXGUIDEDIST*2 && t>0)
			{
			start = i-1;
			break;
			}
	for (int i=0; i<trk.GetSize(); ++i)
		if (DistanceToSegment(trk[i], coords[size-2], coords[size-1], t)<MAXGUIDEDIST*2 && t>1)
			{
			end = i;
			break;
			}
#endif

	#define MINPOINTS 10
	int trknewsize = (end>0 ? end : trk.GetSize()) - (start>0 ? start : 0);
	if (trknewsize<MINPOINTS && trksize>MINPOINTS && Distance(cstart,cend)>MAXGUIDEDIST)
		{
		if (start>0 && start>trksize/2 && coords.GetSize()>2 && abs(cstart.elev-trk[start-1].elev)>MINELEVDIFF) 
			Log(ret=LOGALERT, "Massive START trimming of %d points [%d - %d]: %s --> %s ", trksize, start, end, cstart.Summary(TRUE), cend.Summary(TRUE));
		if (end>0 && end<trksize/2 && coords.GetSize()>2 && abs(trk[end].elev-cend.elev)>MINELEVDIFF) 
			Log(ret=LOGALERT, "Massive END trimming of %d points [%d - %d]: %s --> %s ", trksize, start, end, cstart.Summary(TRUE), cend.Summary(TRUE));
		}

	if (end>0) // do not allow end=0
		{
		// avoid clipping off rappels from the end
		while (end<size-1 && Rap(GRADEI(end, trk))>1)
			++end;
		trk.SetSize(end+1);
		}
	if (start>0)
		{
		// avoid clipping off rappels
		if (continuation)
		  {
		  int size = trk.length();
		  while (start<size-1 && Rap(GRADEI(start, trk))>1)
				++start;
		  }
		for (int s=0; s<start; ++s)
			trk.Delete(0);
		}
	}

	return ret;
}



void processSteps(tcoords &trkcoords, double minstep = -1, double maxstep = -1)
{
	if (minstep>0)
		for (int i=1; i<trkcoords.length(); ++i)
			if (Distance(trkcoords[i-1], trkcoords[i])<minstep)
				trkcoords.Delete(i--);
	
	// identify long steps and break them down
	if (maxstep>0)
	  for (int i=1; i<trkcoords.GetSize(); ++i)
		{
		double dist = Distance(trkcoords[i-1], trkcoords[i]);
		if (dist>maxstep)
			{			
			tcoord trkcoordO = trkcoords[i-1];
			double angle = Bearing(trkcoordO, trkcoords[i]);
			for (double newdist = maxstep; newdist<dist; newdist+=maxstep, ++i)
				{
				tcoord newcoord;
				CoordDirection(trkcoordO, angle, newdist, newcoord);
				DEMARRAY.height(&newcoord);
				trkcoords.InsertAt(newcoord, i);
				}
			}
		}
}



void processElevation(void)
{

#ifdef DEBUGPLANB
  if (iopts[OPLANB])
#endif
	{
#ifdef BORIG
	// original values
	int alg = ALGORITHM, hres = 1;
    if (iopts[O0]) alg = 0;
    if (iopts[O1]) alg = 1;
    if (iopts[OMINUS]) hres = 0;
	if (alg!=ALGORITHM || hres!=1)
	  for (int i=0; i<trkcoords.length(); ++i)
		{
		double elev = trkcoords[i].elev;
		DEMARRAY.height(&trkcoords[i], alg, hres);
		}
#endif
#if 0 //TRY TO IMPROVE LOW RESOLUTION
	// Low quality alternative
	// get original value
	getelevationDEM0(trkcoords);
	smoothHeight(bezier);
	// process laterals
	//processLat(getelevationDEM1);
#endif
#ifdef BINTERP
	// delete
	{
	CDoubleArrayList elev(trkcoords.length());
	double minelev = trkcoords[0].elev;
	for (int i=0; i<trkcoords.length(); ++i)
		elev[i] = trkcoords[i].elev = minelev = min(trkcoords[i].elev, minelev);
	for (int i=1; i<elev.length()-2; ++i)
		{
		if (trkcoords[i].elev == trkcoords[i+1].elev)
			{
			 double e = trkcoords[i].elev;
			 elev[i] = (trkcoords[i-1].elev + e)/2;
			 elev[i+1] = (trkcoords[i+2].elev + e)/2;
			}
		}
	for (int i=0; i<trkcoords.length(); ++i)
		trkcoords[i].elev = elev[i];

	}
#endif
#ifdef BDELETE
	// delete
	{
	CDoubleArrayList newe;
	//tcoords newc;
	for (int i=1; i<trkcoords.length()-2; ++i)
		{
		if (abs(trkcoords[i].elev - trkcoords[i+1].elev) < 0.1)
			{
			 //double g1 = -(trkcoords[i-1].elev - trkcoords[i].elev)/Distance(trkcoords[i-1], trkcoords[i]);
			 //double g2 = -(trkcoords[i+1].elev - trkcoords[i+2].elev)/Distance(trkcoords[i+1], trkcoords[i+2]);
			 //f (g1<=GRADEM[0] && g2<=GRADEM[0])
				{
				trkcoords[i].lat = (trkcoords[i].lat + trkcoords[i+1].lat)/2;
				trkcoords[i].lng = (trkcoords[i].lng + trkcoords[i+1].lng)/2;
				trkcoords.Delete(i+1);
				--i;
				}
			}
		}
	for (int i=1; i<trkcoords.length()-2; ++i)
		if (trkcoords[i].elev == trkcoords[i+1].elev)
			ASSERT( !"NO" );
	}
#endif
#ifdef BSMOOTH
	// smooth
	{
	CDoubleArrayList t(trkcoords.length());
	for (int i=0; i<trkcoords.length(); ++i) t[i] = trkcoords[i].elev;

	for (int i=1; i<trkcoords.length()-1; ++i)
		//if (t[i] == t[i+1])
			trkcoords[i].elev =  (2*t[i] + t[i-1] + t[i+1])/4;
	}
#endif
#ifdef BSHARP
	// sharpen
	{
	CDoubleArrayList newe;
	for (int i=1; i<trkcoords.length()-1; ++i)
		newe.push( 3*trkcoords[i].elev - trkcoords[i-1].elev - trkcoords[i+1].elev );
	for (int i=1; i<trkcoords.length()-1; ++i)
		trkcoords[i].elev = newe[i-1];
	}
#endif
#if 0
	// edge
	CDoubleArrayList newe;
	for (int i=1; i<trkcoords.length()-1; ++i)
		newe.push( trkcoords[i].elev - trkcoords[i+1].elev );
	for (int i=1; i<trkcoords.length()-1; ++i)
		if (abs(newe[i-1])<1)
			trkcoords[i].elev = 10000;
	trkcoords[0] = trkcoords[1];
	trkcoords[trkcoords.length()-1] = trkcoords[trkcoords.length()-2];
#endif
#if 0
	// propagate minelev
	for (int i=1; i<trkcoords.length()-1; ++i)
		if (trkcoords[i].elev == trkcoords[i+1].elev)
			trkcoords[i].elev = (trkcoords[i-1].elev+trkcoords[i+1].elev)/2;
#endif
	}

  // processing to get minimum elevation
  //getelevationDEMMIN(trkcoords);

  // google 2nd pass
  if (!iopts[O0] && iopts[O2PASS]>=0)
	if (!GOOGDATA.getelevationGOOG(trkcoords))
		{
		if (!getelevationGOOG(trkcoords))
			++googerror;
		if (!processLat(getelevationGOOG, isBetterResG))
			++googerror;
		}

  // smoothHeight(bezier);
	
}


void processTrk(void)
{
  // roundup elevations for consistency
  for (int i=0; i<trkcoords.length(); ++i)
	  {
#ifdef DEBUG
	  ASSERT( abs(trkcoords[i].elev - DECODE(ENCODE(trkcoords[i].elev))) < 1e-3);
#endif
	  trkcoords[i].elev = DECODE(ENCODE(trkcoords[i].elev));
      }

  if (INVESTIGATE>=0)
	{
	CSymList list;
	for (int i=0; i<trkcoords.length(); ++i)
		list.Add(CSym(MkString("s%d", i), trkcoords[i].Summary(TRUE)));
	list.Save(fname+".trkcoords.csv");
	}

  cascadeMinimum();

  // process gradients & minmax
  int n = trkcoords.length();
  trk.SetSize(n);

  maxdist = 0;
  minalt = maxalt = trkcoords[0].elev;
  minres = maxres = trkcoords[0].res;
  minresg = maxresg = trkcoords[0].resg;
  for (int i=0; i<n; ++i)
	{
	trk[i] = trkcoords[i];

	if (trk[i].elev<minalt)
		minalt = trk[i].elev;
	if (trk[i].elev>maxalt)
		maxalt = trk[i].elev;
	if (trk[i].elev<minres)
		minres = trk[i].res;
	if (trk[i].elev>maxres)
		maxres = trk[i].res;
	if (trk[i].elev<minresg)
		minresg = trk[i].resg;
	if (trk[i].elev>maxresg)
		maxresg = trk[i].resg;
	float d = i<n-1 ? (float)Distance(trkcoords[i].lat, trkcoords[i].lng, trkcoords[i+1].lat, trkcoords[i+1].lng) : 0;
	//ASSERT(d<10);
	if (d>250 && i<n-1) 
		if (trkcoords[i].elev>0 && trkcoords[i+1].elev>0 && abs(trkcoords[i].elev-trkcoords[i+1].elev)>MINELEVDIFF)
			Log(LOGWARN, "Elevation long shot %g>250m for %s: %s ---> %s", d, fname, trkcoords[i].Summary(TRUE), trkcoords[i+1].Summary(TRUE));
	trk[i].dist = d;
	trk[i].grade = trk[i].grade2 = 0;
	// compute gradients
	trk[i].tdist = maxdist;
	maxdist += d;
	}
  width = (int)(maxdist+1);
  height = (int)((maxalt-minalt)+1);
  ASSERT(width>0 && height>0);
  ntiles = tilecache.GetModes();
  heighttotal = height + 2 + TEXTSIZE*2 + TILEHEIGHT*ntiles + FLAGSIZE+3;

  for (int i = 1; i<n-1; ++i)
    {
	trk[i].grade = (float)GRADEI(i,trk); //GRADE(trk[i-1].elev-trk[i+1].elev, trk[i+1].tdist-trk[i-1].tdist);
	trk[i].grade2 = trk[i].grade-trk[i-1].grade;
    }
  trk[0].grade = trk[1].grade;
  trk[0].grade2 = trk[1].grade2;
  trk[i].grade = trk[i-1].grade;
  trk[i].grade2 = trk[i-1].grade2;

	// compute special gradients
	// computeGrade();
}

int processMerge(trklist &trks)
{
	for (int t=0; t<trks.GetSize(); ++t)
		{
		int itrk = IntersectionToLine(trk, trks[t], MINSTEP/2);
		if (itrk>=0)
			trk.SetSize(itrk+1);
		}
	trks.AddTail(trk);
	return TRUE;
}

void processTrack(trklist *trks = NULL)
{
	processSteps(trkcoords, MINSTEP/2, MINSTEP*2);

	if (trkcoords.length()<2)
		return;

	processSides();

	processElevation();

	processTrk();

	if (trks) 
		processMerge(*trks);

	if (trk.length()<2)
		return;
	
	makeRappels();

#ifdef DEBUG
	if (INVESTIGATE>=0)
		testDeriv();
#endif
}

void makeRappels(void)
{
  raps.SetSize(5);
  for (int i=0; i<raps.length(); ++i)
	  raps[i]=0;

  int n = trk.GetSize();
  CDoubleArrayList g2(n);
  g2[0] = 0;
  for (int i=1; i<n; i++) 
   {
   ASSERT(trk[i].rapnum<0);
   g2[i] = trk[i].grade-trk[i-1].grade; //trk[i].grade2 
   }

  maxcnt = 0; maxft = 0;
  for (int i=0; i<n-1; ++i)
	{
	if (Rap(trk[i].grade)>0) // && trk[i].dist<maxstep)  // avoid long shots to escape potholes
		{
		// find rappel extent
		// 0 0 0 1 1 1 1 2 2 2 1 1 1 2 2 3 3 2 1 0 0  rap
		//      +1      +1    -1    +1+1  -1-1      delta
		//       v         ^     v         ^   v      grade
		//       |               |             |       extents
		//       m m M m m m M M M 
		int wanttobreak = 0, readytobreak = 0;
		double oldg = trk[i].grade;
		double maxg = oldg;
		//int maxj, minj; double mingrade, maxgrade;
		//mingrade = maxgrade = trk[lastdelta=minj=maxj=i].grade;
		
		// do not start too early
		int lastdelta=i;
		for (int j=i+1; j<n-1 && Rap(trk[j].grade)>0; ++j)
			{
			int deltaup = Rap(trk[j].grade+0.05) - Rap(oldg);
			int deltadn = Rap(trk[j].grade-0.05) - Rap(oldg);

			/*
			if (trk[j].grade>=maxgrade)
				maxgrade = trk[maxj=j].grade;
			else
				mingrade = trk[minj=j].grade;

			if (trk[j].grade<=mingrade)
				mingrade = trk[minj=j].grade;
			else
				maxgrade = trk[maxj=j].grade;

			if (delta>0 && minj-i>1 && minj!=j)
				{		
				j = minj;
				//break; 
				}
			*/

			// note the possible breakpoint is local minimum (0 in second derivative)
			//if (trk[j].g2<=0 && trk[j].g2<trk[j+1].g2 && rap<=1)
			//if (trk[j].grade+0.1<maxg && 
			if (g2[j]<=0 && g2[j+1]>0) // local min
				{
				float maxg2 = 0;
				for (int k=j+1; k<n-1 && g2[k]>0; ++k)
					maxg2 = max(maxg2, trk[k].grade);
				if (wanttobreak || deltadn<0) maxg2 = 1e10;
				if (Rap(maxg)!=Rap(maxg2) || (maxg-trk[j].grade>0.1 && maxg2-trk[j].grade>0.1)) // significant min or min separating two <> max raps
					{
					readytobreak++;
					lastdelta = j;
					}
				}

			// break chain when this conditionsa are met
			if //(delta>0 && oldrap==1) // step up from Yellow
				(deltadn<0 // step down
				|| (deltaup>0 && readytobreak)) // step up
				//g2[j]<=0 && g2[j+1]<0)
				{
				wanttobreak++;
				//lastdelta = j;
				}

			// overdue break
			if (wanttobreak>0 && readytobreak>0)
				{
				// do not break up short legs 
				if (lastdelta-i>=2)
					{
						j = lastdelta;
						break;
					}
				//readytobreak = FALSE;
				}

			oldg = trk[j].grade;
			maxg = max(oldg, maxg);
			}
	
		// avoid rappels with long tails
		//if (j-minj>2)
		//{
		//	j = minj;
		//}
		// long enough to start for itself
		//if (lastdelta-i>2 && j-lastdelta>=2)
		//{
		//	j = lastdelta;
		//}

		// for short rappels add transition at start and at end
		if (j-i<=2 && trk[i].elev-trk[j].elev<MINRAP)
		{
		if (i>0 && trk[i-1].rapnum<0) // Rap(trk[i-1].grade)==0)
			--i;
		if (j<n-1 && trk[j].rapnum<0)
			++j;
#ifdef DEBUGXXX
		if (trk[i].elev-trk[j].elev>=MINRAP)
			Log(LOGWARN, "Extended %s R#%d to %gm", fname, rappels.length(), trk[i].elev-trk[j].elev);
#endif
		}

		if (i==1) 
			i=0; // patch!

		//if (maxrap>=0) ++raps[maxrap]; // count all rappels
		trap rap;
		rap.end = j;
		rap.start = i;
		rap.id = LLE(trk[i]).Id();
		rap.m = (float)rnd(trk[i].elev-trk[j].elev);
		rap.ft = (float)m2ft*rap.m;
		rap.dist = trk[j].tdist-trk[i].tdist;
		//if (i>0) // skip drop-in 
			if (rap.m>=MINRAP)  // skip small drops & low gradient
				{				
				if (rap.ft>maxft) 
					maxft = rap.ft;
				int num = rappels.length();
				float dist = 0;
				float mingrade, maxgrade, maxsidesgrade;
				mingrade = maxgrade = trk[i].grade;
				maxsidesgrade = trk[i].sidegrade;
				for (int k=i; k<j; ++k)
					{
					trk[k].rapnum = num;
					mingrade = min(trk[k].grade, mingrade);
					maxgrade = max(trk[k].grade, maxgrade);
					maxsidesgrade = max(trk[k].sidegrade, maxsidesgrade);
					dist += trk[k].dist;
					}
				rap.maxgrade = maxgrade;
				rap.mingrade = mingrade;
				++raps[Rap(maxgrade)];
				rap.avggrade = maxgrade; // rap.m/(rap.dist-gboost(trk[i])); 
				rap.sidegrade = maxsidesgrade;
				if ( abs(dist-rap.dist)>1)
					Log(LOGALERT, "Inconsistency distance %f <> %f", dist, rap.dist);
				rap.dist = dist;
				rappels.AddTail(rap);
				}
		// jump to next
		for (i = j+1; i<n-1 && g2[i]<0; ++i);
		--i;
		}
	}
}



#if 0 // BEN
/*
double groundHeight(double lat, double lng) {

	tcoordsp list;
	tcoord coord(lat, lng);
	list.push(&coord);
	getelevation(list);
	return coord.elev;
  //return geglobe.getGroundAltitude(lat, lng);
}

void MapBEN(double lat, double lng, double totaldist) 
{
  if (totaldist <=0)
	  totaldist = 4000;
  //double tstart = (new Date()).getTime();
  tcoord coord(lat, lng);

  lats.push(lat);
  lngs.push(lng);
  alts.push(groundHeight(lat, lng));
  dists.push(0);

  double dist = 0;
  double theta;
  double thetaprev = InvalidNUM;
  double altprev = 1e6;
  double directioncount1 = 10;
  double directioncount2 = 8;
  double searchangle1 = M_PI/2;
  double searchangle2 = searchangle1 / directioncount1;
  double stepangle = M_PI/3;
  double steplimit = 40;
  double diststep = MINSTEP;
  
  while (dist < totaldist && !abortcalc) {
    //Find a point that is strictly at a lower altitude than the current point
    double steps = 1;
    double downhilldist;
    double dc1 = directioncount1;
    double dc2 = directioncount2;
    double sa1 = searchangle1;
    double sa2 = searchangle2;
    while (steps < steplimit && !abortcalc) {
      theta = downhillDirection(coord, steps*diststep, dc1, thetaprev, sa1, InvalidNUM);
      theta = downhillDirection(coord, steps*diststep, dc2, theta, sa2, altprev);
      if (isNaN(theta)) {
        steps++;
        dc1 = round(dc1*steps/(steps-1));
        dc2 = round(dc2*steps/(steps-1));
        if (steps == 2) {
          sa1 = 2*M_PI;
          sa2 = sa1/dc1;
        }
      } else {
        break;
      }
    }
    if (steps >= steplimit || abortcalc)
      break;
    downhilldist = steps*diststep;
    
    //Assign intermediate (some uphill) steps to the ultimate altitude-loss point
    if (steps > 1) {
      double targetE = steps*diststep*sin(theta);
      double targetN = steps*diststep*cos(theta);
      //alert("Supplementing for " + steps + " steps; target is " + targetE + ", " + targetN);
      double x = 0, y = 0, d = diststep, dx, dy;
      tcoord current = coord;
      double steptheta = theta;
      double stepalt = altprev;
      //alert("Starting loop");
      while (steps > 1) {
        steptheta = downhillDirection(current, d, directioncount1, steptheta, stepangle, InvalidNUM);
        steptheta = downhillDirection(current, d, directioncount2, steptheta, stepangle/directioncount1, InvalidNUM);
        moveInDirection(current, steptheta, d);
        lats.push(current.lat);
        lngs.push(current.lng);
        //createPlacemark(current["lat"], current["lng"], true);
        stepalt = groundHeight(current.lat, current.lng);
        alts.push(stepalt);
        dists.push(d);
        steps--;
        x += d*sin(steptheta);
        y += d*cos(steptheta);
        //alert("Moved in direction " + steptheta + " for " + d + " to height " + stepalt + "; x and y are now " + x + ", " + y + ".  steptheta adjusted to " + atan2(targetN-y, targetE-x) + " relative to original theta of " + theta);
        steptheta = atan2(targetE-x, targetN-y);
        dx = targetE - x;
        dy = targetN - y;
        d = sqrt(dx*dx + dy*dy)/steps;
      }
      steps = d/diststep;
      //alert("Finished loop with " + steps + " steps remaining");
    }
    
    //Add the ultimate altitude-loss point
    moveInDirection(coord, theta, downhilldist);
    lats.push(coord.lat);
    lngs.push(coord.lng);
    //createPlacemark(coord["lat"], coord["lng"], false);
    altprev = groundHeight(coord.lat, coord.lng);
    alts.push(altprev);
    dists.push(steps*diststep);
	dist += steps*diststep;
    thetaprev = theta;
  }

}


double downhillDirection(tcoord &coord, double dist, double n, double thetaprev, double searchangle, double altprev) {
  double bestangle;
  double lowestalt = 1e6;
  double minbound, maxbound;
  if (isNaN(thetaprev)) {
    minbound = 0;
    maxbound = 2*M_PI;
  } else {
    minbound = thetaprev - searchangle;
    maxbound = thetaprev + searchangle + 0.001;
  }
  double dtheta = (maxbound-minbound)/(n-1);
  for (double theta=minbound; theta<maxbound; theta += dtheta) {
    tcoord newcoord = coord;
    moveInDirection(newcoord, theta, dist);
    double newalt = groundHeight(newcoord.lat, newcoord.lng);
    //createPlacemark(newcoord["lat"], newcoord["lng"]);
    if (newalt < lowestalt) {
      lowestalt = newalt;
      bestangle = theta;
    }
  }
  if (isNaN(altprev))
    return bestangle;
  else
    if (lowestalt >= altprev) return InvalidNUM; else return bestangle;
}
*/
#endif


static DWORD rapColor(double grade)
{
	int n = Rap(grade);
	if (n<0) n = 0;
	return RapColor(n);

}

static double gcolor(double g, double g1, double g2, double h1, double h2)
{
	double v = h1 + (g-g1)/(g2-g1)*(h2-h1);
	return v;
}

static DWORD pathColor(double g, double alpha = 0) 
{
#if 0
  if (grade > 0)
    return RGB(0,0,0);
  else
    return jet(-max(grade, -0.7)/0.7);
#else
	if (g>=GRADE[3])
		return hsv2rgb(0, 100, gcolor(g, GRADE[3], GRADE[4], 100, 80), alpha);
	if (g>=GRADE[2])
		return hsv2rgb(gcolor(g, GRADE[2], GRADE[3], 30, 0), 100, 100, alpha);
	if (g>=GRADE[1])
		return hsv2rgb(gcolor(g, GRADE[1], GRADE[2], 60, 30), 100, 100, alpha);
	//if (g<=GRADE[0])
	//	return hsv2rgb(gcolor(g, GRADE0, GRADE1, 180, 60), 100, 100, alpha);
	if (g>0)
		return hsv2rgb(gcolor(g, 0, GRADE[1], 180, 60), 100, 100, alpha);
	return RGBA(alpha*0xFF, 0, 0xFF, 0xFF);
#endif
}


void translateCoord(tcoord &coord, double dE, double dN) {
  double EARTH_RADIUS = 6371000;
  double c = 2 * M_PI * EARTH_RADIUS;
  coord.lat = (float)(coord.lat + 360 * dN / c);
  coord.lng = (float)(coord.lng + 360 * dE / (c * cos(coord.lat * M_PI / 180)));
}

void moveInDirection(tcoord &coord, double theta, double dist) {
  translateCoord(coord, dist*sin(theta), dist*cos(theta));
  coord.ang = (float)theta;
}





CString PicLink(void)
{
	if (iopts[OLINK] || iopts[OKML])
		return MkString("http://%s/rwr/?profile=%s", server, fname);
	else
		return fname+PNG;
}

void createLineString(CKMLOut &out, const char *networklink) 
{
  CString download;
  if (iopts[OLINK] && query!=NULL)
	  download = jlink( MkString("http://%s/rwr/?rwzdownload=%s", server, CString(query, strlen(query)-1)), "Download KML File");

  double rapdist = 0;
  int rlast = rappels.GetSize()-1;
  if (rlast>0)
	  {
	  int start = rappels[0].start;
	  int end = rappels[rlast].end;
	  rapdist = trk[end].tdist - trk[start].tdist;
      }
  CString summary = MkString("%d rappels (max %.0fft) over %.2f miles [(max %.0fm) over %.2fkm]", rappels.GetSize(), maxft, rapdist*km2mi/1000, maxft/m2ft, rapdist/1000);
  CString desc = "Summary: "+summary+"</br>";
	
  CString res;
  if (maxres>0)
	  res += resCredit(trk[0].credit) + " Data Resolution: " + resText(maxres);
  if (minres>0 && round(minres)!=round(maxres))
	  res += " - " +resText(minres);
  CString resg;
  if (minresg<minres)
	{
	if (maxresg>0)
		resg += "   " + resCredit(0) + " 2nd pass: "+ resText(maxresg);
	if (minresg>0 && round(minresg)!=round(maxresg))
		  resg += " - " +resText(minresg);
	}

#ifdef DEBUGX
  desc += MkString("R Grades: %d+%d+%d+%d = %d<br>", raps[1], raps[2], raps[3], raps[4], raps[1]+raps[2]+raps[3]+raps[4]);
  desc += MkString("Segments: %d x %gm = %.0fm<br>", trk.length(), MINSTEP, maxdist);
  desc += MkString("Height Calls: %d<br>", groundHeightCnt);
#endif
  //desc += "<br/>Click on the profile below to display full screen. Click on R markers for rappel info. <br/>";
  //desc += MkString("<a href=\"http://%s/rwr/?profile=%s\"><img src=\"http://%s/rwr/?profile=%s\" width=\"100%%\"/></a><br/>", server, fname, server, fname);
  //desc += "<br>";
  for (int i=0; i<rappels.length(); ++i)
	{
	CString ricon, rname, rdesc;
	RappelInfo(rappels[i], ricon, rname, rdesc);
	desc += rdesc;
	desc += MkString(" <a href=\"#%sR%d;balloon\">[Profile]</a><br/>",fname,i);
	}
  desc += /*MkString("<a href=\"%s\">Download Full Profile</a> ", PicLink())+*/ download+" <br/>";
  desc += MkString("<br><div style=\"float:right\"><a href=\"http://%s/rwr/?profile=legend\">Color Scale</a></div>", server);
  desc += "<div style=\"float:left\">" + res + resg + "</div>";

  summary.Replace("<br>", " ");

  CString id;
  GetSearchString(fname, "", id, '(', ')');

  CString summaryid = id+"summary";   
  CString plotstyle = "<styleUrl>#"+summaryid+"</styleUrl>";
  out += "<Style id=\""+summaryid+"\"><ListStyle><listItemType>checkHideChildren</listItemType></ListStyle><BalloonStyle><text>" CDATAS "<b><font size=\"+1\">"+fname+"</font></b><br/><br/><div style='width:45em'>"+desc+"</div>" CDATAE "</text></BalloonStyle></Style>";
//<Style id=\"rwresstyle\"><ListStyle><listItemType>checkHideChildren</listItemType></ListStyle></Style><styleUrl>#rwresstyle</styleUrl>
  out += KMLName(fname, summary, 1);  //+ MkString(" <a href=\"#%s;balloon\">more</a>"
  out += plotstyle;
/*
  // Rap Marker Styles
  for (int i=0; i<=3; ++i)
	{
	CString extra = MkString("<ListStyle><listItemType>checkHideChildren</listItemType><ItemIcon><href>http://sites.google.com/site/rwicons/rg%d.png</href></ItemIcon></ListStyle>", i);
	out += KMLMarkerStyle(MkString("rg%d",i), 0.8, 0.8, FALSE, extra);
	}
*/
  /*
  // full line
  vara list; 
  for (int i=0; i<trk.length(); ++i) 
	  list.push(CCoord3(trk[i].lat, trk[i].lng));
  out += KMLLine(fname, NULL, list, RGB(0, 0xFF, 0xFF), 1);
*/
  vara norap;
  double dist = 0;
  for (int i=0, rapnum=-1; i<trk.length()-1; i++, dist+=trk[i].dist) 
	{
#ifdef SIDEGRADE
			{
#if 1
			// 	trk[i].grade = -(trk[i+1].elev-trk[i-1].elev)/(trk[i+1].tdist-trk[i-1].tdist-MINSTEP+1);
			float gr = trk[i].sidegrade;
			int xw = Rap( gr );	
			if (xw>0)
				{
				vara list; 
				list.push(CCoord3(trk[i].lat, trk[i].lng));
				list.push(CCoord3(trk[i+1].lat, trk[i+1].lng));
				norap.push( KMLLine(NULL, NULL, list, pathColor(gr, 0.5), 5+xw*5) );  // 4, 8, 12, 16 => 8 10 12
				}
#endif
			}
#endif

		if (trk[i].rapnum<0)
			{
			// non rappel segments
			vara list; 
			list.push(CCoord3(trk[i].lat, trk[i].lng));
			list.push(CCoord3(trk[i+1].lat, trk[i+1].lng));
			norap.push( KMLLine(NULL, NULL, list, pathColor(trk[i].grade), 2, plotstyle) );
			continue;
			}

		// rappel segments
		if (trk[i].rapnum!=rapnum && rappels.length()>0)
			{
			// Rappel!
			trap &r = rappels[rapnum=trk[i].rapnum];

			CString desc  = "<div>", icon, name;
			RappelInfo(r, icon, name, desc);
			desc += MkString(" [<a href=\"#%s;balloon\">Summary</a>]<br/>", fname) + "</div>";
			CString resg;
			resg += resCredit(trk[r.start].credit) + " Data Resolution: " + resText(trk[r.start].res) + " ";
			if (isBetterResG(trk[r.start].res, trk[r.start].resg))
				resg += "   " + resCredit(0) + " 2nd pass: "+resText(trk[r.start].resg);
			int x = xcoord(dist), y = ycoord(trk[i].elev); 
			desc += MkString("<div style=\"width:100%%;overflow:auto\"><img style=\"margin-left:-%dpx;margin-top:-%dpx\" width=\"%d\" height=\"%d\" alt=\"Chart\" src=\"%s\"/></div>", x, y, width, heighttotal, PicLink());
			desc += MkString("<div style=\"float:right\"><a href=\"http://%s/rwr/?profile=legend\">Color Scale</a></div>", server);
            desc += "<div style=\"float:left\">"+ resg  + "</div>";

#ifdef DEBUGX
			desc += MkString("Segment: s%d-s%d<br>", r.start, r.end);
#endif
			COLORREF c = rapColor(r.avggrade);
			CString rapstyle = MkString("%sR%d", id, rapnum), style;
			style += MkString("<LabelStyle><scale>0.8</scale><color>ff%6.6X</color></LabelStyle>", c==RAPINVERT ? RGB(0xFF,0xFF,0xFF) : c);
			style += MkString("<IconStyle><Icon><href>http://sites.google.com/site/rwicons/%s.png</href></Icon><scale>1.0</scale></IconStyle>", icon);
		    out += MkString("<Style id=\"%s\">", rapstyle); out+=style;
			out += MkString("<ListStyle><listItemType>checkHideChildren</listItemType><ItemIcon><href>http://sites.google.com/site/rwicons/%s.png</href></ItemIcon></ListStyle>", icon);
			out += MkString("<BalloonStyle><text>" CDATAS "<b><font size=\"+1\">%s</font></b><br/><br/><div style='width:45em;height:auto'>%s</div>", name, desc);
			out += MkString(CDATAE"</text></BalloonStyle>");
			out += "</Style>";
			out += MkString("<Folder id=\"%sR%d\"><name>%s</name><styleUrl>#%s</styleUrl>", fname, rapnum, name, rapstyle);
			CString Rstyle;
			Rstyle += MkString("<styleUrl>#%s</styleUrl>", rapstyle);
			Rstyle += "<Style>"+style+"</Style>";
			out += KMLMarker(Rstyle, trk[i].lat, trk[i].lng, name, NULL);
			if (iopts[ONOLINES])
				plotstyle = ""; 
			if (!iopts[ONOLINES2])
			  for (int k=r.start; k<r.end; ++k)
				{
				vara list; 
				list.push(CCoord3(trk[k].lat, trk[k].lng));
				list.push(CCoord3(trk[k+1].lat, trk[k+1].lng));
				out += KMLLine(NULL, NULL, list, pathColor(trk[k].grade), 5, plotstyle); 
				}
				
			out += "</Folder>";

			//for (int k=i; k<=j; ++k)
			//	out += KMLLine(name, desc, rlist, colors[rap], 5+rap);
			}
	}


	if (!iopts[ONOLINES] && !iopts[ONOLINES2])
	{
	out += "<Folder><name>Non Technical</name><Style><ListStyle><listItemType>checkHideChildren</listItemType></ListStyle></Style>";
	for (int i=0; i<norap.GetSize(); ++i)
		out += norap[i];
	if (networklink)
		out += networklink;
	out += "</Folder>"; 
	}

#ifdef DEBUG
  CFILE f;
  if (INVESTIGATE>=0)
    if (f.fopen(fname+".csv", CFILE::MWRITE))
	{
	for (int i=1; i<trk.length()-2; ++i)
		{
		double h = trk[i].elev-trk[i+1].elev, h1 = trk[i-1].elev-trk[i+1].elev, h2 = trk[i].elev-trk[i+2].elev;
		double d = trk[i].dist, d1 = trk[i-1].dist+trk[i].dist, d2 = trk[i].dist+trk[i+1].dist;

		CString str = MkString("s%d:%.fm,rapnum:,%d,g:,%d,elev:,%g",i, trk[i].tdist, trk[i].rapnum, Rap(trk[i].grade), trk[i].elev);
		str += MkString(",m:,%g,ft:,%g,res:%g,resg:%g,dist:,%g,grade:,%g,deg:,%g,grade2:,%g", h, h*m2ft, trk[i].res, trk[i].resg, trk[i].dist, trk[i].grade, g2deg(trk[i].grade), trk[i].grade2);
		str += MkString(",coord:,%s,%s", CCoord(trk[i].lat), CCoord(trk[i].lng));
		//ASSERT( trk[i].grade == -h/d);
		str += MkString(",CALCP-1: m:,%g,ft:,%g,dist:,%g,grade:,%g,deg:,%g", h1, h1*m2ft, d1, h1/d1, g2deg(h1/d1));
		str += MkString(",CALCP+1: m:,%g,ft:,%g,dist:,%g,grade:,%g,deg:,%g", h2, h2*m2ft, d2, h2/d2, g2deg(h2/d2));
		f.fputstr(str);
		}
	}
#endif
}



void drawHeightMap(void) 
{
  
  CImage c;
  c.Create(width, heighttotal, 24);
  CDC dc; dc.Attach( c.GetDC() );
  dc.FillSolidRect(0, 0, width, heighttotal, RGB(0xA0,0xA0,0xA0));
  dc.SetBkMode(TRANSPARENT);

  {
  CFont font;
  font.CreatePointFont(80, "System", &dc);
  CFont* def_font = dc.SelectObject(&font);
  dc.SetTextColor(RGB(0,0,0));

  //Draw 100ft/30m contours & 300ft/100m steps
  double diststep = 100; // 100m
  double contourstep = 30; //100/m2ft;
  CPen pen( PS_SOLID, 1, RGB(0xCC,0xCC,0xCC));
  //CPen thick( PS_SOLID , 3, RGB(0x00,0x00,0x00));
  //CPen thickred( PS_SOLID , 3, RGB(0xFF,0x00,0x00));
  CPen* pOldPen = dc.SelectObject( &pen );
  for (double y=ceil(minalt/contourstep)*contourstep; y<maxalt; y+=contourstep) 
  {
	int py = ycoord(y);
	dc.MoveTo(0, py);
	dc.LineTo(width, py);
	dc.TextOut(0, py, MkString("%.0fm", y));
	CString ft = MkString("%.0fft", y/3*10);
	dc.TextOut(width-TEXTSIZE*ft.GetLength()/2, py, ft);
  }
  for (double x=0; x<maxdist; x+=diststep) 
  if (x>0)
  {
    int px = xcoord(x);
    dc.MoveTo(px, 0);
    dc.LineTo(px, heighttotal);
  }
  CPen pen2( PS_DOT, 1, RGB(0xCC,0xCC,0xCC));
  dc.SelectObject( &pen2 );
  int q = 0;
  double mistep = 1/4.0/km2mi*1000;
  const char *quarts[] = {"", "¼","½", "¾"};
  for (double x=0; x<maxdist; x+=mistep, ++q) 
  if (x>0)
  {
    int px = xcoord(x);
    dc.MoveTo(px, 0);
    dc.LineTo(px, heighttotal);
	int qn = q % 4 ;
	CString label = MkString("%d%smi", q/4, quarts[qn]);
	if (q/4==0) label = label.Mid(1);
	dc.TextOut(px,height+TEXTSIZE+FLAGSIZE, label);
  }
  for (double x=0; x<maxdist; x+=diststep) 
  if (x>0)
	dc.TextOut(xcoord(x),height+FLAGSIZE, MkString("%.0fm", x));
  
  //Draw height profile
  double dist = 0;
  for (int i=0; i<trk.length()-1; i++) {

	// draw tiles
	if (ntiles>0)
		{
		int distance = (int)(Distance(trk[i],trk[i+1])+0.5);
		double theta = Bearing(trk[i],trk[i+1]);
		double thetay = theta - M_PI/2;
		register double stx = sin(theta), ctx = cos(theta);
		register double sty = sin(thetay), cty = cos(thetay);

		for (int y=0; y<TILEHEIGHT-1; ++y)
			 {
				tcoord coordy(trk[i].lat,trk[i].lng);
				double y2 = y - TILEHEIGHT/2;
				translateCoord(coordy, y2*sty, y2*cty);
				for (int x=0; x<distance; ++x)
					{
					tcoord coord = coordy;
					translateCoord(coord, x*stx, x*ctx);
					for (int t=0; t<ntiles; ++t)
						c.SetPixel(xcoord(dist+x), height+(t+1)*TILEHEIGHT-y-1, tilecache.GetPixel(coord.lat, coord.lng, t));
					}
			}
		}
	/*
	if (i%10==0)
		{
		CPen* pOldPen = dc.SelectObject( &pen );
		dc.MoveTo(xcoord(dist), 0);
		dc.LineTo(xcoord(dist), height);
		dc.SelectObject( pOldPen );
		}
	*/
	// draw resolution
	int x = xcoord(dist), y = ycoord(trk[i].elev);
	int x2 = xcoord(dist+trk[i].dist), y2 = ycoord(trk[i+1].elev);
	if (trk[i].res>0)
		dc.FillSolidRect(x, heighttotal-3, x2-x, 3, resColor(trk[i].res));
	if (trk[i].resg>0)
		dc.FillSolidRect(x, heighttotal-6, x2-x, 3, resColor(trk[i].resg));
    
    DWORD segcolor = pathColor(trk[i].grade);
	LOGBRUSH brush; brush.lbColor = segcolor; brush.lbStyle = BS_SOLID; brush.lbStyle = 0;
	CPen pen( PS_SOLID|PS_GEOMETRIC|PS_ENDCAP_ROUND, trk[i].rapnum>=0 ? 4 : 2, segcolor);	
	dc.SelectObject( &pen );
	dc.MoveTo(x, y);
    dc.LineTo(x2, y2);
    dist+=trk[i].dist;
  }
  dc.SelectObject( pOldPen );
  dc.SelectObject(&def_font);
  }

  // draw rappel flags
  {
  CFont font;
  // Courier Regular
  font.CreatePointFont(62, "Terminal Bold", &dc);
  CFont* def_font = dc.SelectObject(&font);
  dc.SetTextColor(RGB(0,0,0));
  for (int rapnum=0; rapnum<rappels.length(); ++rapnum)
		{
		int fsize = FLAGSIZE, fsize2 = fsize/2;
		COLORREF c = rapColor(rappels[rapnum].avggrade);
		//CPen rappel( PS_SOLID );
		int px, py, x = xcoord(trk[rappels[rapnum].start].tdist), y = ycoord(trk[rappels[rapnum].start].elev);
		dc.FillSolidRect(px=x-1, py=y-fsize-1, 1+2, fsize+2, RGB(0, 0, 0));
		dc.FillSolidRect(px+=1, py+=1, 1, fsize, RGB(0, 0xFF, 0xFF));
		dc.FillSolidRect(px+=1, py-=1, fsize2+2, fsize2+2, RGB(0, 0, 0));
		dc.FillSolidRect(px+=1, py+=1, fsize2, fsize2, c);
		CString ftext = MkString("R %.0fft", rappels[rapnum].ft);
		dc.TextOut(px+1,py-1, ftext);
		if (c == RAPINVERT)
			{
			dc.SetTextColor(RGB(0xFF,0xFF,0xFF));
			dc.TextOut(px+1,py-1, "R");
			dc.SetTextColor(RGB(0,0,0));
			}
		//dc.SelectObject( &rappel );
		//dc.MoveTo(x, y-8);
		//dc.LineTo(x, y+8);
		}
  dc.SelectObject(&def_font);
  }

  dc.Detach();
  c.ReleaseDC();

  // save
  vars pngfile = ElevationPath(fname)+PNG;
  c.Save(pngfile);
  if (!iopts[OLINK] && filelist!=NULL)
	  filelist->AddTail(pngfile);
}

static void drawLegend(const char *file) 
{
  CImage c;
  int W = 3;
  int width = 90*W, height = TEXTSIZE*3;
  c.Create(width, height, 24);
  CDC dc; dc.Attach( c.GetDC() );
  dc.FillSolidRect(0, 0, width, height, RGB(0x80,0x80,0x80));
  dc.SetBkMode(TRANSPARENT);
  //CFont font;
  //font.CreatePointFont(80, "Arial", &dc);
  //CFont* def_font = dc.SelectObject(&font);

  CPen pen( PS_SOLID , 1, RGB(0xCC,0xCC,0xCC));
  CPen* pOldPen = dc.SelectObject( &pen );

  for (int i=1; i<sizeof(GRADE)/sizeof(GRADE[0]); ++i)
    {
	COLORREF c = rapColor(GRADE[i]);
	dc.SetTextColor(c==RAPINVERT ? RGB(0xFF,0xFF,0xFF) : c);
	double deg = g2deg(GRADE[i]);
	int x = (int)rnd(W*deg);
	//if (x % 10 == 0)
	  dc.MoveTo(x, 0);
      dc.LineTo(x, height);
	  dc.TextOut(x,height-TEXTSIZE, MkString("%.0f%%", -GRADE[i]*100));
	  dc.TextOut(x, 0, MkString("%.0fº", deg));
    }


  double deg=0;
  for (int x=0, r=-1; x<width; x+=W, ++deg) 
  {
    double g = deg2g(deg);

	int nr = Rap(g);
    if (nr>0 && nr!=r)
	{
	  int msize = 4;
	  dc.MoveTo(x, TEXTSIZE-msize);
      dc.LineTo(x, height-TEXTSIZE+msize);
	  r = nr;
	}

	dc.FillSolidRect(x, TEXTSIZE, W, TEXTSIZE, pathColor(g) );
  }

  dc.SelectObject( &pOldPen );
  dc.SetTextColor(0);
  dc.TextOut(0,TEXTSIZE*2,"Grade:");
  dc.TextOut(0,0,"Angle:");

  //dc.SelectObject(def_font);
  dc.Detach();
  c.ReleaseDC();
  c.Save(file);
}

void drawThermal(void)
  {
  int alg = 2, res = 1;
  if (iopts[O0]) alg = 0;
  if (iopts[O1]) alg = 1;
  if (iopts[OMINUS]) res = 0;

  double minlat, maxlat, minlng, maxlng, minalt, maxalt;
  minlat = maxlat = trk[0].lat;
  minlng = maxlng = trk[0].lng;
  minalt = maxalt = trk[0].elev;
  for (int i=0; i<trk.length(); ++i)
	{
	if (trk[i].lat>maxlat) maxlat = trk[i].lat;
	if (trk[i].lat<minlat) minlat = trk[i].lat;
	if (trk[i].lng>maxlng) maxlng = trk[i].lng;
	if (trk[i].lng<minlng) minlng = trk[i].lng;
	if (trk[i].elev>maxalt) maxalt = trk[i].elev;
	if (trk[i].elev<minalt) minalt = trk[i].elev;
	}
  int x1, y1, x2, y2;
  // 1km = 0.0093 ll
  //minlat = rnd(minlat,0.5e3,-1);
  //minlng = rnd(minlng,0.5e3,-1);
  //maxlat = rnd(maxlat,0.5e3,1);
  //maxlng = rnd(maxlng,0.5e3,1);
  LL2Pix(-1, minlat, minlng, x1, y1);
  LL2Pix(-1, maxlat, maxlng, x2, y2);
  int rx = min(x1,x2), w = abs(x1-x2);
  int ry = min(y1,y2), h = abs(y1-y2);

  CImage c;
  #define PIXPERM 1
  #define px(x) (int)(((x-rx)+0.5)/PIXPERM)
  #define py(y) (int)(((y-ry)+0.5)/PIXPERM)
  int maxx, maxy;
  c.Create(maxx=px(rx+w)+2, maxy=py(ry+h)+2, 24);
  
  for (int y=ry; y<ry+h; ++y)
	for (int x=rx; x<rx+w; ++x)
		{
		tcoord coord; 
		Pix2LL(-1, x, y, coord.lat, coord.lng);
		DEMARRAY.height(&coord, alg, res);
		double alt = coord.elev;
		//int g = (int)((alt-minalt)/(20+maxalt-minalt)*0xFF);
		//c.SetPixel(x-rx, y-ry, RGB(g, g, g));
		//c.SetPixel(px(x), py(y), jet((alt-minalt)/(maxalt-minalt)) );
		double h = (alt-minalt)/(maxalt-minalt);
		c.SetPixel(px(x), py(y), hsv2rgb(360*h, 100, (h<0 || h>1) ? 0 : 100) );
		}
  //c.Save(fname+"alt.png");
  for (int i=0; i<trk.length(); ++i)
		{
		int x,y;
		ASSERT(trk[i].lat>=minlat && trk[i].lat<=maxlat);
		ASSERT(trk[i].lng>=minlng && trk[i].lng<=maxlng);
		LL2Pix(-1, trk[i].lat, trk[i].lng, x, y);
		double h = (trk[i].elev-minalt)/(maxalt-minalt);
		c.SetPixel(px(x), py(y), hsv2rgb(360*h, 100, (h<0 || h>1) ? 0 : 50) );
		x=y;
		}
  CString tname = "T"+fname+"altx";
  c.Save(tname+PNG);

  CKMLOut out(tname+KML);

	out += "<GroundOverlay>";
    out += MkString("<Icon><href>%s</href><viewRefreshMode>onRegion</viewRefreshMode></Icon><drawOrder>10</drawOrder>", tname+PNG);
	out += MkString("<LatLonBox><south>%.6f</south><west>%.6f</west><north>%.6f</north><east>%.6f</east></LatLonBox>", minlat, minlng, maxlat, maxlng);
	out += "</GroundOverlay>";
}



int FindMinElev(const tcoord &coord, double dirang, double searchang, int n, double dist, tcoords &newcoords)
{
	// search for elevation in direction of 'dirang' covering 'searchang' sector with 'n' increments

	// get elevations as bulk
	//tcoords newcoords(n+1);
	tcoord newcoord;
	newcoords.SetSize(0);
	ASSERT(dirang != InvalidNUM);
	dirang = dirang==InvalidNUM ? 0 : NORMANG(dirang);
	CoordDirection(coord, dirang, dist,* newcoords.push(newcoord));
	for (register int i=1, m = n/2; i<=m; ++i)
		{
		CoordDirection(coord, dirang + searchang/2*(i/(double)m), dist, *newcoords.push(newcoord));
		CoordDirection(coord, dirang - searchang/2*(i/(double)m), dist, *newcoords.push(newcoord)); 
		}
	n = newcoords.length();
	//ASSERT( newcoords.length() == n );

	/*
	// get elev for coord too?
	tcoord *coordptr = NULL;
	if (coord.elev<=0)
		coordptr = newcoords.push(coord);
	*/

	// get elevations
	getelevation(newcoords);

	/*
	// coord
	if (coordptr)
		{
		coord = *coordptr;
		if (coord.elev<=0)
			{
			Log(LOGERR, "ERROR: No height for base point (%s)", CCoord2(coord.lat,coord.lng));
			return -1;
			}
		}
	*/

	// newcoords
	int err = 0;
	for (int i=0; i<n; ++i)
		if (newcoords[i].elev<0)
			{
			++err;
			newcoords[i].elev = MAXELEV; // high elevation
			}
	if (err)
		{
		if (INVESTIGATE>=0)
			Log(LOGWARN, "FindMinElev: getelevation failed (%d/%d) around %s,%s", err, newcoords.length(), CCoord(coord.lat), CCoord(coord.lat));
		//abortcalc = err==newcoords.length();
		return -1;
		}
	   
	// process elevations
	int mini = 0;
	double minelev = newcoords[mini=0].elev; // -1 force at least 1m difference
	//coords.SetSize(newcoords.length());
	for (int i=1; i<n; ++i)
		{
		ASSERT(newcoords[i].elev>=0);
		//coords[i] = newcoords[i];
		if (newcoords[i].elev<minelev)
			minelev = newcoords[mini = i].elev;
		}
	
	return mini;
}



int FindMinElevLL(const tpoint &coord, double dirang, double searchang, int n, double dist, tcoords &newcoords)
{
	int mini = FindMinElev(coord, coord.ang, searchang, n, dist, newcoords);	
	for (int i=1; i<3 && mini>=0; ++i)
		{
		searchang = searchang / n * 2;
		mini = FindMinElev(coord, newcoords[mini].ang, searchang, n = 8, dist, newcoords);
		}
	return mini;
}

	/*
int FindMinElevDEM0(tcoord &coord, double dirang, double searchang, int n, double dist, tcoords &newcoords)
{	
	getelevation = getelevationDEM0;
	tcoord old = coord;
	//if (coord.elev<0)
	if (coord.elev<0 || coord.res<0)
		DEMARRAY.height(&coord, 0);
	double d = Distance(coord.lat, coord.lng, old.lat, old.lng);
	ASSERT(d<15);
	if (coord.elev<=0)
		{
		Log(LOGERR, "ERROR: No height for base point (%s)", CCoord2(coord.lat,coord.lng));
		abortcalc = TRUE;
		return -1;
		}
	dist = coord.res;
	ASSERT(dist>0 && dist<20);
	int mini = FindMinElev(coord, dirang, searchang, n, dist, newcoords);
	if (mini>=0)
		{
		tcoord &nc = newcoords[mini];
		nc.ang = Bearing(coord.lat, coord.lng, nc.lat, nc.lng);
		d = Distance(coord.lat, coord.lng, nc.lat, nc.lng);
		ASSERT(d<15);
		}
	else
		mini = -1;
	return mini;
}
	*/

int ConnectMinElev(tcoord &coord1, tcoord &coord2, tcoords &coords)
{
#ifdef DEBUGX
	for (int i=0; i<newcoords.GetSize(); ++i)
		Log(LOGINFO, "%d %s", i, CCoord2(newcoords[i].lat, newcoords[i].lng));
#endif
	int nang = 8;
	double searchang = M_PI/2;

	int count = 0;
	tcoords newcoords, newcoords1, newcoords2;
	while (Distance(coord1, coord2)>2*MINSTEP && count<MAXCONNECTION)
		{
		double ang = Bearing(coord1, coord2);
#ifdef DEBUGX
		Log(LOGINFO, "%d c1:%s (a:%.2f) c2:%s d:%.0f a:%.2f", count, CCoord2(coord1.lat, coord1.lng), coord1.ang, CCoord2(coord2.lat, coord2.lng), Distance(coord1, coord2), ang);
#endif
		int mini1 = FindMinElev(coord1, ang, searchang, nang, MINSTEP, newcoords);
		ASSERT(mini1>=0 && newcoords[mini1].elev>0 && abs(newcoords[mini1].ang-ang)<M_PI/4);		
		if (mini1<0 || newcoords[mini1].elev<0)
			break;
		newcoords1.push(coord1 = newcoords[mini1]);
		++count;
			
		//int mini2 = FindMinElev(coord2, -ang, searchang, nang, MINSTEP, newcoords);
		//if (mini2>=0)
		//	newcoords2.push(coord2 = newcoords[mini2]);
		//if (mini1<0 && mini2<0)
		//	break;		
		}

	// report connection points
	for (int i=0; i<newcoords1.length(); ++i)
		trkcoords.push(newcoords1[i]);
	//for (int i=newcoords2.length()-1; i>=0; --i)
	//	trkcoords.push(newcoords2[i]);
	// guide connection		
	if (coord2.elev>0)
		trkcoords.push(coord2);
	return TRUE;
}


int EscapeMinElev2(const tcoord &coord, tcoord &newcoord)
{
		int nang = 2*8;
		double searchang = 2*M_PI;
		int found = -1;
		CDoubleArrayList minelev, maxelev;
		minelev.SetSize(nang);
		maxelev.SetSize(nang);
		for (int i=0; i<nang; ++i)
			{
			minelev[i] = 1e15;
			maxelev[i] = -1;
			}
		//newcoords.SetSize(nang);
		// find spot with minimum
		tcoords newcoords;
		for (int n=0; n<MAXESCAPING && found<0; ++n)
			{
			// compute max and min
			double minringelev = 1e15;
			int mini = FindMinElev(coord, 0, searchang, nang, n*MINSTEP, newcoords);
			if (mini<0) 
				return FALSE; // failed
			for (int i=0; i<newcoords.length(); ++i)
				{
				if (newcoords[i].elev<minelev[i])
					minelev[i] = newcoords[i].elev;
				if (newcoords[i].elev>maxelev[i])
					maxelev[i] = newcoords[i].elev;
				if (maxelev[i]<minringelev)
					minringelev = maxelev[i];
				}
			if (newcoords[mini].elev<coord.elev && maxelev[mini]<=minringelev)
				found = mini;
			}

		if (found<0)
			return FALSE; // failed

		newcoord = newcoords[found];
		return TRUE;
}



int Guide(const tcoord &coord, int &guide, tpoints *guiding, double distance = MAXGUIDEDIST, float tmax = 1.0, double *tp = NULL)
{
		if (guide>=guiding->GetSize())
			return FALSE;

		double t;
		double d = DistanceToSegment(coord, (*guiding)[guide-1], (*guiding)[guide], t);
		if (d>distance)
			{
			// check if other segments match better
			for (int g=guide+1; g<guiding->length() && (d = DistanceToSegment(coord, (*guiding)[g-1], (*guiding)[g], t))>distance; ++g);
			if (g>=guiding->length())
				return FALSE; // out of track
			guide = max(g, guide);
			}
		if (t>tmax)
			{
#if 1
			for (++guide; tp!=NULL && guide<guiding->length() && DistanceToSegment(coord, (*guiding)[guide-1], (*guiding)[guide], t)<distance && t>tmax; ++guide);
			if (guide>=guiding->length())
				return FALSE;	// finished
#else
			if (++guide>=guiding->length())
				return FALSE;	// finished
#endif
			}
		if (tp) *tp = t;
		return TRUE;
}
int ReverseGuide(const tcoord &coord, int &guide, tpoints *guiding, double distance = MINRIVERJOIN)
{
		if (guide<=1)
			return FALSE;

		double t;
		double d = DistanceToSegment(coord, (*guiding)[guide-1], (*guiding)[guide], t);
		if (d<distance)
			return FALSE;
		//for (; guide>0 && DistanceToSegment(coord, (*guiding)[guide-1], (*guiding)[guide], t))<d; --guide);
		if (t<0)
			if (--guide<=1)
				return FALSE;	// finished
		return TRUE;
}

int MapPoint(double lat, double lng, double totaldist = 0, double minelev = 1, double initang = InvalidNUM, tpoints *guiding = NULL, BOOL strictguide = FALSE) 
{
  //double tstart = (new Date()).getTime();
  
  CDoubleArrayList trkdists;
  
  tcoord coord(lat, lng);
  DEMARRAY.height(&coord);
  this->mappoint = coord;

  //if (fulldist)
  //	  maxstep = 100*MINSTEP;

  tcoords newcoords;
  int lastgood = 0, lastgoodmin = 0;
  int climbing = 0, guide = 1, backflowguide;
  double dist = 0;
  BOOL addcoord = TRUE;
  int usedguide = 0;
  //int angmulti = max(1, MINSTEP/5);

runagain:

  // get initial elevation
  if (coord.elev<=0)
		{
		DEMARRAY.height(&coord);
		if (coord.elev<0)
			{
			Log(LOGERR, "MapPoint invalid elev @ %s", coord.Summary(TRUE));
			return usedguide;
			}
		}
   //ASSERT(coord.elev > 5400/m2ft);
   // add coord
   BOOL moving = FALSE;
   if (initang!=InvalidNUM)
	  {
	  coord.ang = (float)initang; 
	  initang = InvalidNUM;
	  }

   if (addcoord)
		{
		addcoord = FALSE;
		int n = trkcoords.length();
		if (n>0)
			dist += Distance(coord, trkcoords[n-1]);
		trkdists.push(dist);
		lastgoodmin = lastgood = trkcoords.length();
		trkcoords.push(coord);
		}
	//ASSERT( Distance(coord, tcoord(  46.059209,-122.205469))>10 );

#ifdef DEBUG
  // check distance calc is good
  double dist2 = 0;
  for (int i=1; i<trkcoords.length(); ++i)
	  dist2 += Distance(trkcoords[i], trkcoords[i-1]);
  ASSERT( abs(dist-dist2)<1e-3);
#endif

  while (TRUE) {
    
	//find a point that is strictly at a lower altitude than the current point
	double diststep = MINSTEP;
	// 180 search
	int nang = 8;
	double searchang = M_PI;
	if (!moving)
		{
		if (guiding)
			{
			double t;
			if (Guide(coord, guide, guiding, MAXGUIDEDIST, 0.9f, &t))
				if (t<1)
					coord.ang = (float)Bearing(coord, (*guiding)[guide]);
				//else
					// muast be ended // Log(LOGALERT, "Inconsistent !moving guide at %s : [%d] %s -> [%d] %s", coord.Summary(), guide-1, (*guiding)[guide-1].Summary(), guide, (*guiding)[guide].Summary());
			}
		else
			{
			// 360 search
			nang *= 2;
			searchang *= 2;
			}
		}
	moving = TRUE;

	// find next lowest point 
#ifdef BDEM0
	int mini = DEMARRAY.minheight(coord, coord.ang, searchang, diststep, newcoords);
#else
	int mini = FindMinElevLL(coord, coord.ang, searchang, nang, diststep, newcoords);	
#endif

	// end prematurely if no next point available
	if (mini<0) 
		return usedguide;

	// minimal elevation
	if (newcoords[mini].elev<minelev)
		return usedguide;

	// climbing / sliding
	if (coord.elev>newcoords[mini].elev)
		climbing=0;
	else
		if (++climbing>MAXCLIMBING)
			break;

	// guiding
	if (guiding)
		{
		// check if following guide 
		if (!Guide(coord, guide, guiding))
			{
			if (strictguide)
				goto runguide;
			}
		else
			{
			if (climbing && strictguide)
			  {
				// help find next coord 
				double ang = Bearing(coord, (*guiding)[guide]), minang = M_PI;
				mini = FindMinElev(coord, ang, M_PI/4, nang, diststep, newcoords);
			  }
			}
		}
	// newcoord
	tcoord &newcoord = newcoords[mini];
	diststep = Distance(coord, newcoord);
	ASSERT(diststep>1);

	// distance / autodistance
	if (!guiding)
		{
		if (totaldist>0 && dist>totaldist)
			return usedguide;	// finished
		if (totaldist<=0)
			{
			// autodistance
			if (dist>AUTOMAXDISTANCE)
				return usedguide;
			int n = trkcoords.length()-(int)(AUTODISTANCE/MINSTEP);
			if (n>0)
				{
				double a = AUTOMINGRADE;
				double g = (trkcoords[n].elev-coord.elev)/(dist-trkdists[n]);
				ASSERT(abs(dist-trkdists[n])>Distance(coord, trkcoords[n]));
				if (g<a)
					return usedguide; // cut off
				}
			}
		}

	// going around in circles
	int last = (int)(trkcoords.length()-MAXSTALLDIST/MINSTEP);
	if (last>0 && Distance(newcoord, trkcoords[last])<MAXSTALLDIST/2)
		break;

	// accumulate
	dist += diststep;
	trkdists.push(dist);
	lastgood = trkcoords.length();
	trkcoords.push(coord = newcoord);
	//ASSERT( Distance(coord, tcoord(  46.059209,-122.205469))>10 );


	if (trkcoords[lastgoodmin].elev>coord.elev)
		lastgoodmin = lastgood;
  }

    // finished guiding
	if (guiding && guide>=guiding->length())
		return usedguide;

    // resume from last good point
	coord = trkcoords[lastgoodmin];
	dist = trkdists[lastgoodmin];
	trkcoords.SetSize(lastgoodmin+1);
	trkdists.SetSize(lastgoodmin+1);
	BOOL lastgoodminguided = guiding ? Guide(trkcoords[lastgoodmin], guide, guiding) : FALSE;

	if (trkcoords.GetSize()>10000)
		{
		Log(LOGALERT, "Possible cyclic loop in MapPoint (>10000 point) at %s", mappoint.Summary());
		trkcoords.SetSize(100);
		return usedguide;
		}

	// stalled
	{
	tcoord newcoord;
	tcoords newcoords;
	CTEST("EE1   \r");					
	CEscapeMap cm(coord);
	int escaped = cm.Escape(newcoord, newcoords);
	CTEST("EE1   \r");					
	/*
	// check if guiding better
	if (escaped && guiding)
		if (Distance(coord, newcoord) > Distance(coord, (*guiding)[guide]))
			escaped = -1;
	*/
#ifdef DEBUGXXX
	Log(LOGINFO, "EscapingMinElev %s s%d %fm = %s (+%d) => %s,%s %sm G%d", fname, trkcoords.length(), dist, escaped>0 ? "ESCAPED!" : escaped<0 ? "IGNORED" : "FAILED", newcoords.length(), CCoord(newcoord.lat), CCoord(newcoord.lng), CData(newcoord.elev), guide);
#endif
	if (escaped>0)
		{
		// coord, dist, trkcoords, trkdists
		// last time was still good
		int newguide = guide;
		for (int i=1; i<newcoords.length()-1; ++i)
			{
			tcoord &newcoord = newcoords[i];
			ASSERT( CheckLL(newcoord.lat, newcoord.lng) );

			if (guiding)
				if (!Guide(newcoord, newguide, guiding))
					{
					if (strictguide)
						goto runvalidguide;
					if (lastgoodminguided && guide>1) 
						goto runvalidguide;
					}
			ASSERT( CheckLL(newcoord.lat, newcoord.lng) );
			trkdists.push( dist += Distance(coord, newcoord) );
			trkcoords.push( coord = newcoord );  
			//ASSERT( Distance(coord, tcoord(  46.059209,-122.205469))>10 );
			}
		guide  = newguide;
		coord = newcoord;
		addcoord = TRUE;
		goto runagain;
		}
	}

	// completely lost 
	if (!guiding)
		return usedguide;

	// if not too far from the guide, guide forward, otherwise quit
	// also takes into note backflow, when it flows back to previous guide points
	if (!lastgoodminguided) // gone too far
		return usedguide;
					
runvalidguide:
	// clean backflow, if any
	if (Guide(coord, backflowguide=1, guiding)) // has backflow, need to clean up
		{
#if 0
		// guide is good, so go back to closest point to guideline
		double t = 1;
		double mindist = MAXGUIDEDIST;
		for (int pos = trkcoords.GetSize()-1, minpos = pos; pos>0 && t>0; --pos)
			{
			double d = DistanceToSegment(trkcoords[pos], (*guiding)[guide-1], (*guiding)[guide], t);
			if (d<mindist)
				minpos = pos, mindist = d;
			}
		lastgoodmin = minpos;
#else
		//if (backflowguide>0)
		{
		#define MAXBACKFLOW (int)((MAXGUIDEDIST*2)/MINSTEP)
		int nguide = max(1, guide-MAXBACKFLOW), npos = max(0, trkcoords.GetSize()-MAXBACKFLOW);
		if (!IsValidGuide(*guiding, MAXGUIDEDIST, &nguide, &npos))
			{
			// resume from last good guide position
			//guide = nguide+1;
			lastgoodmin = min(lastgoodmin, npos);
			}
		}
#endif
		}

runguide:
	if (++guide>=guiding->length())
		return usedguide; // finished

	// resume guide from last good point
	coord = trkcoords[lastgoodmin];
	dist = trkdists[lastgoodmin];
	trkcoords.SetSize(lastgoodmin+1);
	trkdists.SetSize(lastgoodmin+1);

	++usedguide;
	coord = (*guiding)[guide-1];
	addcoord = TRUE;
	goto runagain;
}



static int cmpcoord(tcoord *a, tcoord *b)
	{
	register double diff = a->elev - b->elev;
	if (diff>0)
		return -1;
	else if (diff<0)
		return 1;
	else return 0;
	}




void Map(double lat, double lng, double totaldist = 0, double minelev = 1, double ang = InvalidNUM, const char *trkfile = NULL) 
{
MCC.Lock();
if (trkfile && *trkfile!=0)
	{
	if (!LoadTrk(trkfile, trkcoords, minelev))
		{
		Log(LOGALERT, "Bad Trk for %s", trkfile);
		MapPoint(lat, lng, totaldist, minelev, ang);
		}
	}
else
	{
	MapPoint(lat, lng, totaldist, minelev, ang);
	}
processTrack();
if (out) Output(*out);
MCC.Unlock();
}


BOOL IsValidGuide(tpoints &coords, double distance = MAXGUIDEDIST, int *guideptr = NULL, int *posptr = NULL, int maxpos = -1)
{
  // check track vs guide, find split if any
  int guide = 1, pos = 0;
  if (guideptr) guide = *guideptr;
  if (posptr) pos = *posptr;

  if (maxpos<0) maxpos = trkcoords.length();
  while (pos<maxpos && Guide(trkcoords[pos], guide, &coords, distance))
	  ++pos;

  if (guideptr) *guideptr = guide;
  if (posptr) *posptr = pos;

  /// make sure difference is significant
  return !(guide<coords.length()-1 && pos<trkcoords.length()-1);
}



int MapSplit(CRiver *river, CRiverList *rlist, double endelev, tpoints &coords, CSym *split)
{
  int guide = 1, pos = 0;
  if (IsValidGuide(coords, MAXSPLITDIST, &guide, &pos))
	if (guide>=coords.GetSize()-1)
	  return FALSE;
  
  int minguide = guide; 
  int minpos = pos;
  // go back as far as possible to avoid split
  if (pos>=trkcoords.GetSize()) pos = trkcoords.GetSize()-1;
  while (pos>0 && ReverseGuide(trkcoords[pos], guide, &coords, MINRIVERJOIN))
	  --pos;

  if (guide<2) guide = 2;

  // find guide point
  tpoint &bestjoin1 = coords[guide];
  //tcoord &bestjoin2 = trkcoords[pos];

  // we got pos as the closest point
  //DEMARRAY.height(&coords[minguide]);
  for (;guide<=coords.GetSize()-2; ++guide)
		{
		CanyonMapper cm(NULL, "Check River");
		double dist = MAXSPLITDIST*3;
		if (guide<minguide)
			{
			// compute distante up till breach
			//dist = MAXSPLITDIST*3;
			for (int i=guide; i<minguide; ++i)
				dist += Distance(coords[i], coords[i+1]);
			}
		cm.MapPoint(coords[guide].lat, coords[guide].lng, dist);
		if (cm.IsValidGuide(coords, MAXSPLITDIST))
			break;
		}
  
  tpoint &newstart = coords[guide];

  /*
  // if too close to start, move it forward
  double dist = 0;
  int nextguide = guide;
  for (guide=1; (guide<nextguide || dist<MAXSPLITDIST) && guide<coords.length(); ++guide)
	  dist += Distance(coords[guide-1], coords[guide]);

  // if too close to the end, not worth it
  dist = 0;
  for (int nextguide=guide; nextguide<coords.length() && dist<MAXSPLITDIST; ++nextguide)
	  dist += Distance(coords[nextguide-1], coords[nextguide]);
  */

  if (guide<coords.length()-2)
	  {
	  // real split detected!
		//Log(LOGINFO, "Splitted ID %s [%s] guide %d/%d at %s", IdStr(river->id), river->sym->id, guide, coords.length(), LLE(coords[guide]).Summary());

		LLRect bbox;
		if (!KMLSym(river->sym->id, "SPLIT@"+LLE(coords[guide]).Summary(), KMLCoords(coords, guide), bbox, *split))
			Log(LOGERR, "Could not make river split for RiverID %s", IdStr(river->id));
		//Log(split->Line());

		return TRUE;
	  }

  return FALSE;
}

  /*
  // interpolate
  tcoord l;
  for (int i=0; i<list.GetSize(); ++i)   
	{
	tcoord c(CGetNum(GetToken(list[i],1,';')), CGetNum(GetToken(list[i],0,';')));
    if (i>0 && interpolate)
		{
		double dist = Distance(l.lat, l.lng, c.lat, c.lng);
		double theta = Bearing(l.lat, l.lng, c.lat, c.lng);
		int n = (int)(dist / MINSTEP);
		for (int i = 1; i<n; ++i)
			{
			tcoord nc;
			CoordDirection(l, theta, i*dist/n, nc);
			coords.push(nc);
			}
		}
	coords.push(l=c);	
	}
  for (int i=0; i<coords.GetSize(); ++i)   
    coordsp.push(&coords[i]);
  */
/*
  // use coords as guideline
  if (flip)
	{
    coords[0] = rcoords[N];
	for (int i=0; i<size; ++i)
		coords [i+ONE] = rcoords[size-i-1];
	}
  else
	{
    coords[0] = rcoords[0];
	for (int i=0; i<size; ++i)
		coords [i+ONE] = rcoords[i];
	}
*/



static int cmpelev(float selev, float eelev, float mindiff = 1)
{
	if (selev-eelev>mindiff) 
		return -1;
	if (eelev-selev>mindiff) 
		return 1;
	return 0;
}


void MapElev(tcoord &end, float &eminelev, double &emindist, double &eminangle, float dist)
{
		trkcoords.Reset();
		MapPoint(end.lat, end.lng, dist);  
		if (trkcoords.length()<2) 
			{
			if (eminelev - end.elev > 1)
				{
				eminelev = end.elev;
				emindist = 0;
				eminangle = InvalidNUM;
				}
			}
		else
			{
			tpoint np;
			int p = DistantPoint(trkcoords, dist, &np);
			float pelev = trkcoords[p].elev;
			if (eminelev - pelev > 1)
				{
				eminelev = pelev;
				emindist = Distance(trkcoords, p);
				eminangle = Bearing(end, np);
				}
			}
}



int MapRiver(CRiver *river, CRiverList *rlist, double minelev = -1, CSym *split = NULL, int retry = 0, trklist *trks = NULL)
{
  double id = river->id;
  tcoords rc = river->Coords();
  processSteps(rc, MINSTEP/2, MAXUPRIVER);

  if (retry>0)
	  fname = ElevationFilename(MkString("Fork%d ", retry)+name);

  MCC.Lock();
  BOOL splitted = FALSE;

  tpoints coords;
  for (int i=0; i<rc.GetSize(); ++i)
	  {
	  tpoint p = rc[i];
	  DEMARRAY.height(&p);
	  coords.push(p);
      }

  // delete invalid elevation and duplicates
  for (int i=0; i<coords.length(); ++i)
	if (coords[i].elev==InvalidAlt || (i>0 && Distance(coords[i], coords[i-1])<1))
		coords.Delete(i--);

  int nullsev = LOGERR;
  const char *nulldesc = NULL;
  if (coords.GetSize()<2)
	{
	nullsev = LOGWARN;
	nulldesc = "<2POINT";
	goto nulltrack;
	}
	  
  // get sense of heights
  int mini, maxi;
  float maxe, mine;
  double sedist = 0;
  DEMARRAY.height(&coords.Head());
  DEMARRAY.height(&coords.Tail());
  maxe = mine = coords[mini = maxi = 0].elev;
  for (int i=1; i<coords.GetSize(); ++i)
		{
		sedist += Distance(coords[i], coords[i-1]);
		if (coords[i].elev>0 && coords[i].elev<MAXELEV)
			{
			if (coords[i].elev<mine)
				mine = coords[mini = i].elev;
			if (coords[i].elev>maxe)
				maxe = coords[maxi = i].elev;
			}
		}

  if (maxe<=0)
	{
	nullsev = LOGWARN;
	nulldesc = "INVALID ELEV";
	goto nulltrack;
	}

  {
  tcoord start = coords.Head();
  tcoord end = coords.Tail();

  if (maxe<0) maxe = 0;
  if (mine<0) mine = 0;


  // flow down from the end for some distance first to get the real end elevation
  if (!retry || minelev<0) // autoelevation
	{
	// check if is a flat track
	if (abs(maxe-mine)<MINELEVDIFF && Distance(start, end)>MINRIVERPOINT)
		{
		nullsev = LOGWARN;
		nulldesc = "FLATBIG";
		goto nulltrack;
		}
	// check if is a flat track
	if (abs(maxe-mine)<1)
		{
		nullsev = LOGWARN;
		nulldesc = "FLATSMALL";
		goto nulltrack;
		}
	// establish direction
	tpoint start2, end2;
	DistantPoint(coords, sedist/4, &start2);
	DEMARRAY.height(&start2);
	ReverseDistantPoint(coords, sedist/4, &end2);
	DEMARRAY.height(&end2);

	int count  = cmpelev(start.elev, end.elev);
	count += cmpelev(start2.elev, end2.elev);
	count += cmpelev(start.elev, start2.elev);
	count += cmpelev(end2.elev, end.elev);
	//count += Distance(coords[mini], coords.Tail()) < Distance(coords[mini], coords.Head()) ? 1 : -1;
	//count += Distance(coords[maxi], coords.Head()) < Distance(coords[maxi], coords.Tail()) ? 1 : -1;

  //double dse = Distance(start, end);
	// find lowest possible end elevation
	float eminelev = end.elev;
	float sminelev = start.elev;
	double smindist = 0, emindist = 0;
	double sminangle = InvalidNUM, eminangle = InvalidNUM;

	float dist = MAXDOWNRIVER;
	tpoint endflow = end, startflow = start;
	//double angle = Bearing(end, start);
	MapElev(end, eminelev, emindist, eminangle, dist);
	MapElev(end2, eminelev, emindist, eminangle, dist);
	ASSERT(Distance(end,end2)<=sedist/2);
	//double angle = Bearing(start, end);
	MapElev(start, sminelev, smindist, sminangle, dist);
	MapElev(start2, sminelev, smindist, sminangle, dist);
	ASSERT(Distance(start,start2)<=sedist/2);

	minelev = min(mine, min(eminelev, sminelev))-MINELEVDIFF*2;
	if (!retry)
		{
#ifdef USEDIRECT
		BOOL flip = FALSE;
		if (river->dir!=0)
			{
			if (river->dir<0)
				flip = TRUE;
			}
		else
#endif
			{
#ifdef DEBUG
			Log(LOGWARN, "No direction pre-set for %s", fname);
#endif
			int fliptrue = 0, flipfalse = 0;
			flip = count>0;
			if (eminelev-sminelev>1)
				flip = TRUE, fliptrue++;
			if (sminelev-eminelev>1)
				flip = FALSE, flipfalse++;
			if (abs(eminelev-sminelev)<1)
				{
				if (emindist-smindist>MINSTEP)
					flip = TRUE, fliptrue++;
				if (smindist-emindist>MINSTEP)
					flip = FALSE, flipfalse++;
				}

			if (sminangle!=InvalidNUM && eminangle!=InvalidNUM)
			{
			double sseang = ABSNORMANG(Bearing(start, end) - sminangle);
			double eseang = ABSNORMANG(Bearing(start, end) - eminangle);
			if (sseang<M_PI/2 && eseang<M_PI/2)
				flip = FALSE, flipfalse++;
			double sesang = ABSNORMANG(Bearing(end, start) - sminangle);
			double eesang = ABSNORMANG(Bearing(end, start) - eminangle);
			if (sesang<M_PI/2 && eesang<M_PI/2)
				flip = TRUE, fliptrue++;
			}

			if ((flip && flipfalse>fliptrue) || (!flip && fliptrue>flipfalse))
				Log(LOGWARN, "Unclear flip [%d vs %dN<>%dY] for %s [%s]", flip, flipfalse, fliptrue, IdStr(river->id), river->sym->GetStr(ITEM_SUMMARY));
			}

		if (flip)
		{
		//if (!retry && Distance(elast, end)>Distance(elast, start)) 
		//if (!retry && Distance(elast, end)>Distance(elast, start)) 
		// something is wrong! check if start has joins, if does NOT then it's real start
		//int RiverJoins(LLE &lle, CRiverList *rlist);
		//if (RiverJoins(start, rlist)>1)
			{
			// go ahead and switch
			coords.Flip();
			start = coords.Head();
			end = coords.Tail();
			}
		}
		}
	}


  // go up river some distance to make sure we don't miss anything, will fix later
  int continuation = FALSE;
  if (!retry)
	  {
	  // find good start
	  tpoint newstart;
	  tpoints newstarts;

	  // pre-start
	  int GetUpRiver(CRiver *r, tpoints &coords, tpoint &newstart);
	  continuation = GetUpRiver(river, coords, newstart)>0;
	  if (continuation) // don't move location of springs
		 newstarts.push(newstart);

	  // normal start
	  newstarts.push(coords[0]);

	  // post-start
	  DistantPoint(coords, MAXUPRIVER, &newstart);
	  newstarts.push(newstart);

	  // find best start
	  start = coords.Head();
	  end = coords.Tail();
	  for (int i=0; i<newstarts.GetSize(); ++i)
		{
		start = newstarts[i];
		trkcoords.Reset();
		MapPoint(start.lat, start.lng, MAXGUIDEDIST*2);
		if (IsValidGuide(coords, MAXGUIDEDIST))
			break;
		}
	  }

  trkcoords.Reset();
  if (retry) id = start.Id();
  MapPoint(start.lat, start.lng, 1e10, minelev, InvalidNUM, &coords, !split);  
#ifdef DEBUG  
  for (int i=0; i<trkcoords.length(); ++i)
		ASSERT(trkcoords[i].ang != InvalidNUM);
#endif
	
  if (split)
	  splitted = MapSplit(river, rlist, minelev, coords, split);

  // adjust ends of guide
  if (processGuide(coords, continuation)>0)
	{
	vars sum = river->sym->GetStr(ITEM_SUMMARY);
	Log(LOGINFO, "RIVER TEST SPLIT%d:%s,%s", retry, IdStr(id), sum.replace(" ",","));
	}

  // process track
  processTrack(trks);
  }

  // new mappoint
  //if (trk.GetSize()>0)
  // mappoint = trk[0];
  
  // avoid empty river segments
  if (!retry && trk.GetSize()<=2)
	  {
	  nulldesc = "BAD?";
	  if (maxe-mine<MINELEVDIFF || coords.Head().elev-coords.Tail().elev<MINELEVDIFF)
		{
		nullsev = LOGWARN;
		nulldesc = "FLAT";
		}
	  if (coords.GetSize()<=2)
		{
		nullsev = LOGWARN;
		nulldesc = "2POINT";
		}

nulltrack:
	  if (nullsev>LOGWARN || INVESTIGATE>=0)
			{
			Log(nullsev, "%s Track %s [%dorig %drc %dcoords] [%.0fm ^%.0fm] [%s --> %s] '%s'", nulldesc, IdStr(river->id), river->Coords().GetSize(), rc.GetSize(), coords.GetSize(), Distance(coords.Head(), coords.Tail()), maxe-mine, coords.Head().Summary(TRUE), coords.Tail().Summary(TRUE), river->sym->GetStr(ITEM_SUMMARY));
			vars sum = river->sym->GetStr(ITEM_SUMMARY);
			Log(LOGINFO, "RIVER TEST SPLIT%d:%s,%s", retry, IdStr(id), sum.replace(" ",","));
			}

	  trkcoords = river->Coords();
	  for (int i=0; i<trkcoords.GetSize(); ++i)
		 trkcoords[i].elev = 0;

	  processTrk();
	  makeRappels();
      }

  if (out) Output(*out);

  MCC.Unlock();
  return splitted;
}




};

class LLEProcess;
int LocateRogue(LLE &lle, CKMLOut *out = NULL);
void RainSimulation(double lat, double lng, CKMLOut *out, LLEProcess *proc);
void WaterSimulation(double lat, double lng, CKMLOut *out, LLEProcess *proc);

/*
GDALInvGeoTransform	(	double * 	gt_in,
double * 	gt_out	 
)

GDALGCPsToGeoTransform	(	int 	nGCPCount,
const GDAL_GCP * 	pasGCPs,
double * 	padfGeoTransform,
int 	bApproxOK	 
)	

oid GDALApplyGeoTransform	(	double * 	padfGeoTransform,
double 	dfPixel,
double 	dfLine,
double * 	pdfGeoX,
double * 	pdfGeoY	 
)		

};



int GDALGetRasterCount	(	GDALDatasetH 	hDS	 ) 	
Fetch the number of raster bands on this dataset.

See also:
GDALDataset::GetRasterCount().

*/

int cmpsymmarkers(CSym *arg1, CSym *arg2, BOOL mergedesc = FALSE)
{
	// compare id
	int cmp = strcmp(arg1->id, arg2->id); 
	if (cmp!=0 || mergedesc<0)
		return cmp;
	// compare coordinates
	int arg1p1 = GetTokenPos(arg1->data, ITEM_LAT);
	int arg1p2 = GetTokenPos(arg1->data, ITEM_SUMMARY+1);
	int arg2p1 = GetTokenPos(arg2->data, ITEM_LAT);
	int arg2p2 = GetTokenPos(arg2->data, ITEM_SUMMARY+1);
	int n = min(arg1p2-arg1p1, arg2p2-arg2p1);
	return strncmp((const char *)arg1->data+arg1p1, (const char *)arg2->data+arg2p1, n);
	//if (cmp!=0)
	//	return cmp;
	//return strcmp( arg1->GetStr(ITEM_SUMMARY), arg2->GetStr(ITEM_SUMMARY));
}

int cmpmarkers( const void *arg1, const void *arg2 )
{
	return cmpsymmarkers(((CSym**)arg1)[0],((CSym**)arg2)[0]); 
}

int UpdateListCheck(CSymList &list)
{
	int error = 0;
	// check sort order
	for (int i=1; i<list.GetSize(); ++i)
		{
		CSym &sym1 = list[i-1];
		CSym &sym2 = list[i];
		int cmpid =strcmp(sym1.id,sym2.id);
		if (cmpid>0)
			{
			if (INVESTIGATE>0)
				Log(LOGALERT, "Inconsistent UpdateList order ID %s > %s", sym1.id,sym2.id);
			++error;
			}
		if (cmpid==0)
			{
			vara attr1, attr2;
			for (int l=ITEM_LAT; l<=ITEM_SUMMARY; ++l)
				{
				attr1.push(sym1.GetStr(l));
				attr2.push(sym2.GetStr(l));
				}
			vars attrs1 = attr1.join();
			vars attrs2 = attr2.join();
			if (strcmp(attrs1,attrs2)>0)
				{
				if (INVESTIGATE>0)
					Log(LOGALERT, "Inconsistent UpdateList order ID %s ATTR %s > %s", sym1.id, attrs1, attrs2);
				++error;
				}
			}
		}
	return error;
}

CString SymInfo(CSym sym)
{
	sym.SetStr(ITEM_COORD, "");
	return sym.Line();
}


#define DESCLISTBR "</BR>"
vara desclist(const char *desc)
{
	int ncdatas = strlen(CDATAS);
	int ncdatae = strlen(CDATAE);
	int ndesc = strlen(desc);
	if (strncmp(desc, CDATAS, ncdatas)==0)
		{
		desc += ncdatas;
		ndesc -= ncdatae+ncdatas;
		}
	vara list( vars(desc, ndesc), DESCLISTBR );
	list.sort();
	return list;
}

CString desctext(vara &descp)
{
	descp.sort();
	return CDATAS+descp.join(DESCLISTBR)+CDATAE;
}

CString ampfix(CString &id)
{
const char *stdsubst[] = { 
"&amp;", "&",
"\\u0026", "&",
"\\u003e", "&gt;",
"\\u003c", "&lt;",
"&quot;", "\"",
"&#39;", "\'",
"\\\"", "\"",
"\\\'", "\'",
NULL
};

	int i=0;
	while (stdsubst[i]!=NULL)
		{
		id.Replace(stdsubst[i], stdsubst[i+1]);
		i+=2;
		}

	for (int i=0; i<id.GetLength(); ++i)
		if (id[i]=='&')
			{
			for (int j=0; j<=5 && id[i+j]!=0 && id[i+j]!=';'; ++j);
			if (id[i+j]!=';')
				id.Insert(i+1, AMP+ 1);
			}
	return id.Trim();
}


CString adddesc(const char *desc, const char *newdesc)
{
	//decompose 
	vara descp = desclist(desc);
	vara newdescp = desclist(newdesc);

	// accumulate
	for (int i=0; i<newdescp.length(); ++i)
		if (descp.indexOf(newdescp[i])<0)
			descp.push(newdescp[i]);

	//recompose
	return desctext(descp);
}

// mergedesc = -1:no merge 0:merge tags 1:merdge tag&desc
void SymMerge(CSym &lsym, CSym &sym, int mergedesc)
{
	// accumulate mode if different
	if (mergedesc>=0)
		sym.SetStr( ITEM_TAG, addmode(lsym.GetStr(ITEM_TAG), sym.GetStr(ITEM_TAG)));
	// accumulate description if different
	if (mergedesc>0)
		sym.SetStr( ITEM_DESC, adddesc(lsym.GetStr(ITEM_DESC), sym.GetStr(ITEM_DESC)));

	// merge
	for (int i=0; i<ITEM_NUM; ++i)
		{
		CString str = sym.GetStr(i);
		if (str.IsEmpty())
			continue;
		lsym.SetStr(i, str);
		}
}

int UpdateList(CSymList &list, CSymList &newlist = CSymList(), const char *msg = NULL, int mergedesc = FALSE)
{
	int oldn = list.GetSize(), newn = newlist.GetSize();
	int changes = 0, additions =0, deletions = 0;
	list.Sort(cmpmarkers);
	newlist.Sort(cmpmarkers);

	int l = 0, n = 0, maxl = list.GetSize(), maxn = newlist.GetSize();
	while (n<maxn)
		{
		CSym &sym = newlist[n];
		int cmp = -1;
		if (l<maxl)
			cmp = cmpsymmarkers(&newlist[n], &list[l], mergedesc);
		if (cmp>0)
			{
			// skip existing
			++l;
			continue;
			}
		if (cmp==0)
			{
			CSym &lsym = list[l];
			// update any field if != null
			if (strcmp(lsym.data, sym.data)!=0)
				{
				if (INVESTIGATE>0)
					Log(LOGINFO, "Changed:\n%s\n%s", SymInfo(lsym), SymInfo(sym));
				SymMerge(lsym, sym, mergedesc);
				++changes;
				}
			++n;
			continue;
			}
		// add new
		if (INVESTIGATE>0)
			Log(LOGINFO, "Added:\n%s", SymInfo(newlist[n]));
		list.Add(newlist[n++]);
		++additions;
		}
	
	list.Sort(cmpmarkers);


	for (int i=list.GetSize()-1; i>0; --i)
		{
		if (cmpsymmarkers(&list[i], &list[i-1])==0)
			{
			if (INVESTIGATE>0)
				{
				list[i].data.TrimRight(",");
				list[i-1].data.TrimRight(",");
				if (list[i].data!=list[i-1].data)
					Log(LOGINFO, "Different Duplicate:\n%s\n%s", SymInfo(list[i-1]), SymInfo(list[i]));
				}
			SymMerge(list[i-1], list[i], mergedesc);
			list.Delete(i), ++deletions;
			}
		}

#ifdef DEBUG
	int error = UpdateListCheck(list);
	if (error>0)
		Log(LOGALERT, "Inconsistent UpdateList order, %d errors", error);
#endif
	/*
	if (detail)
		{
		detail[0] = additions;
		detail[1] = changes;
		detail[2] = deletions;
		}
	*/
	if (msg)
		Log(LOGINFO, "Updated %s: %d items <- %d+%d (%d add, %d chng, %d del)", msg, list.GetSize(), oldn, newn, additions, changes, deletions);

	return changes+additions+deletions;
}

/*
vara fragment(const char *str)
{
	vara list;
	while (*str!=0)
		{
		for ( ; *str!=0 && !isalpha(*str); ++str);
		for (int i= 0; str[i]!=0 && isalpha(str[i]); ++i); 
		list.push( vars(str, i) ); str+=i;
		}
	return list;
}
*/

inline BOOL istext(char c)
{
	return (c>='A' && c<='Z') || (c>='a' && c<='z');
}

int matchtag(const char *desc, const char *tag, int tlen)
{
		register const char *str = desc;
		while (*str!=0)
			{
			if (strnicmp(str, tag, tlen)==0)
				{
				// check that it's a word
				// 4mine = YES
				// mineral = NO
				// abominet = NO
				BOOL start = str==desc || !istext(str[-1]);
				BOOL end = str[tlen]==0 || !istext(str[tlen]);
				if (start && end)
					return TRUE;
				else
					start = end;
				}
			// jump to next word
			while (*str!=0 && istext(*str))
				++str;
			while (*str!=0 && !istext(*str))
				++str;
			}
		return FALSE;
}

int matchtags(const char *desc, vara &tags)
{
	for (int t=0; t<tags.length(); ++t)
		if (matchtag(desc, tags[t], tags[t].length()))
			return TRUE;
	return FALSE;
}


int FilterList(CSymList &list, char tag)
{
	static vara tags;
	static int init = FALSE;
	if (!init)
		{
		init = TRUE;
		CSymList ilist; 
		ilist.Load(filename("PicIgnore.csv")); 
		ilist.Sort();
		for (int i=0; i<ilist.GetSize(); ++i)
			tags.push(ilist[i].id);
		}

	int deleted =0;
	for (int p=list.GetSize()-1; p>=0; --p)
		{
		if (matchtags(list[p].id, tags) || !strchr(list[p].GetStr(ITEM_TAG), tag))
			list.Delete(p), ++deleted;
		}

	return deleted;
}

void UpdateList(const char *file, CSymList &newlist, const char *msg = "", int mergedesc = FALSE)
{
	CSymList list;
	if (CFILE::exist(file))
		list.Load(file);
	if (msg!=NULL && *msg==0)
		msg = file;
	UpdateList(list, newlist, msg, mergedesc);
	// save anyway for update
	list.Save(file);
#ifdef DEBUGX
	CSymList list2;
	list2.Load(file);
	int error = UpdateListCheck(list2);
	if (error>0)
		Log(LOGALERT, "Inconsistent UpdateList order, %d errors", error);
#endif
	newlist.Empty();
	newlist.Add(list);
}




void SetFileError(const char *file)
{
	COleDateTime dt(GetTime()-365); 
	SYSTEMTIME sysTime;
	dt.GetAsSystemTime(sysTime);
	FILETIME time;
	int err = !SystemTimeToFileTime(&sysTime, &time);
	if (!err) err = !CFILE::settime(file, &time);
	if (err)
		Log(LOGERR, "Error setting file time for %s", file);
}


class CBox {
	public:
	float step;
	float overlap;
	LLRect bbox;
	LLRect nbox, obox;

		CBox(LLRect *bbox, double step = 1, double overlap = 0)
		{
			static LLRect maxbox(MinLatitude, MinLongitude, MaxLatitude, MinLongitude);
			if (!bbox)
				bbox = &maxbox;
			this->step = (float)step;
			this->bbox.lat1 = (float)floor(bbox->lat1/step)*this->step;
			this->bbox.lng1 = (float)floor(bbox->lng1/step)*this->step;
			this->bbox.lat2 = (float)ceil(bbox->lat2/step)*this->step;
			this->bbox.lng2 = (float)ceil(bbox->lng2/step)*this->step;
			this->overlap = (float)overlap;
		}

		static int box(const char *file, LLRect &box, double step = 1)
		{
			CString lat, lng;
			int  i = 0;

			CString name = GetFileName(file);
			int slat = name[i++]=='s' ? -1 : 1;
			while (isdigit(name[i]))
				lat += name[i++];
			int slng = name[i++]=='w' ? -1 : 1;
			while (isdigit(name[i]))
				lng += name[i++];

			float vlat = (float)CGetNum(lat);
			float vlng = (float)CGetNum(lng);
			if (vlat==InvalidNUM || vlng==InvalidNUM)
				return FALSE;

			box.lat1 = slat * vlat;
			box.lng1 = slng * vlng;
			box.lat2 = box.lat1 +1;
			box.lng2 = box.lng1 +1;
			return TRUE;
		}

		static CString file(double lat, double lng)
		{
			CString str = MkString("%c%g%c%g", lat<0 ? 's' : 'n', abs(lat), lng<0 ? 'w' : 'e', abs(lng));
			str.Replace(".", "");
			return str;
		}

		CString file(void) { return file(nbox.lat1, nbox.lng1); }

		int DaysOld(const char *filename)
		{
			FILETIME time;
			CFILE::gettime(filename, &time);
			if (time.dwHighDateTime==0 && time.dwLowDateTime==0)
				return 1000;
			double cur = GetTime();
			double tfile = COleDateTime(time);
			return (int) (cur - tfile);
		}

		void Update(const char *file, CSymList &list, int errors, int mergedesc = FALSE)
		{
			UpdateList(file, list, CString(file) + (errors>0 ? MkString(" [%d errors]", errors) : ""), mergedesc);
			if (errors>0)
				SetFileError(file);
		}

		LLRect *Start()
		{
			nbox.lat1 = bbox.lat1;
			nbox.lat2 = nbox.lat1+step;
			nbox.lng1 = bbox.lng1;
			nbox.lng2 = nbox.lng1+step;

			return &nbox;
		}

		LLRect *Next()
		{
			nbox.lat1 = nbox.lat1+step;
			nbox.lat2 = nbox.lat1+step;
			if (nbox.lat1>=bbox.lat2)
				{
				nbox.lat1 = bbox.lat1;
				nbox.lat2 = nbox.lat1+step;
				nbox.lng1 = nbox.lng1+step;
				nbox.lng2 = nbox.lng1+step;
				if (nbox.lng1>=bbox.lng2)
					return NULL;
				}
			obox = nbox;
			obox.lat1 -= overlap;
			obox.lng1 -= overlap;
			obox.lat2 += overlap;
			obox.lng2 += overlap;
			return &obox;
		}

		double Progress(double combop = 0)
		{
			double latstep = step, lngstep = step;
			double latp = (nbox.lat1-bbox.lat1 + combop * latstep) / (bbox.lat2-bbox.lat1);
			return (nbox.lng1-bbox.lng1 + latp*lngstep) / (bbox.lng2-bbox.lng1);
		}

		static double Progress(LLRect &nbox, LLRect &bbox, double combop = 0)
		{
			double latstep = nbox.lat2-nbox.lat1;
			double lngstep = nbox.lng2-nbox.lng1;
			double latp = (nbox.lat1-bbox.lat1 + combop * latstep) / (bbox.lat2-bbox.lat1);
			return (nbox.lng1-bbox.lng1 + latp*lngstep) / (bbox.lng2-bbox.lng1);
		}

};



void GetFileList(CSymList &list, const char *folder, const char *exts[], int recurse)
{
	// check if file (or files) match expected header
	// if they don't, destroy them to prevent wrong header usage
	CString path = MkString("%s\\*", folder);

	BOOL match = TRUE;
	CFileFind finder;
	BOOL bWorking = finder.FindFile(path);
    while (bWorking)
    {
		bWorking = finder.FindNextFile();
		if (finder.IsDots()) continue;
		CString file = finder.GetFilePath();

		if (finder.IsDirectory())
			{
			if (recurse)
				GetFileList(list, file, exts, recurse);
			continue;
			}

		if (exts)
			{
			const char *ext = GetFileExt(file);
			if (!ext)
				continue;
			if (!IsMatch(CString(ext).MakeUpper(), exts))
				continue;
			}

		list.Add(CSym(file));
	}
}


void ScanMinMax(GDALFILE &gdal, CSym &sym, CSymList &oldlist, double &minh, double &maxh)
{
#ifdef GDAL
	int successmax = 0, successmin = 0;
	maxh = gdal.band->GetMaximum(&successmax); 
	minh = gdal.band->GetMinimum(&successmin); 
	if (!successmax) maxh = InvalidAlt;
	if (!successmin) minh = InvalidAlt;

	BOOL compute = !successmax || !successmin;
    if (compute)
     {
	 // find cached data
	 int found = -1;
	 int datalen = strlen(sym.data);
	 vars filename = GetFileNameExt(sym.id);
	 for (int i=0; i<oldlist.GetSize() && found<0; ++i)
		 if (strncmp(oldlist[i].data, sym.data, datalen)==0)
			 if (filename==GetFileNameExt(oldlist[i].id))
				 found = i;

     if (found>=0)
		{
		// cached
		vara list(GetToken(oldlist[found].GetStr(ITEM_COORD), 1, ':'), " - ");
		if (list.length()==2)
			{
			minh = CDATA::GetNum(list[0]);
			maxh = CDATA::GetNum(list[1]);
			compute = FALSE;
			}
		}
	if (compute)
		{
		// compute
		 minh = 1e100;
		 maxh = -1e-100;
		 Log(LOGINFO, "Computing %dx%d MinMax for %s", gdal.nRasterXSize, gdal.nRasterYSize, sym.id);
		 for (int x=0; x<gdal.nRasterXSize; ++x)
			 for (int y=0; y<gdal.nRasterYSize; ++y)
				{
				float h = InvalidAlt;
				if (!gdal.getval(x,y, h))
					continue;
				if (h<=InvalidAlt)
					continue;
				if (h<minh)
					minh = h;
				if (h>maxh)
					maxh = h;
				}
		}
     }
#endif
}

void ScanDEM(const char *folder, CSymList &list, CSymList &oldlist)
{
#ifdef GDAL
extern int MODE;
CSymList dellist;
CSymList filelist;
const char *exts[] = { "ADF", "BIL", "HGT", "IMG", "TIF", "DEM", NULL };
GetFileList(filelist, folder, exts, TRUE);


for (int f=0; f<filelist.GetSize(); ++f)
{
  CString &file = filelist[f].id;
  printf("Processing %s %d%% %d/%d %s          \r", folder, (f*100)/filelist.GetSize(), f, filelist.GetSize(), filelist[f].id);

  // get file time
  double filetime = 0, filesize = 0;
  {
  CFILE f;
  if (f.fopen(file))
	  {
	  filetime = f.ftime();
	  filesize = f.fsize();
      }
  }

  // IMG file
  GDALFILE gdal;
  
  if (!gdal.Open(file))
	{
	Log(LOGERR, "ERROR: Invalid IMG file %s", file);
	continue;
	}
	
  if (gdal.projected)
	{
	Log(LOGWARN, "%s uses a projection (can use gdalwarp to optimize the file)", file);
	}

  // lat/lng
  double lat1, lng1, lat2, lng2;
  gdal.Pix2LL(0, 0, lat1, lng1);
  gdal.Pix2LL(gdal.nRasterXSize, gdal.nRasterYSize, lat2, lng2);

   // desc
  double res = max(gdal.xmpp, gdal.ympp);
  CString desc = MkString( "%2.2lfm Resolution (%.6fx%.6f) %dx%dx%d Pixels", res, gdal.G[1], gdal.G[5], gdal.nRasterXSize, gdal.nRasterYSize, gdal.nRasterCount );

  CSym sym(file, desc);
  sym.SetStr(ITEM_LAT, CCoord(min(lat1,lat2)) );
  sym.SetStr(ITEM_LNG, CCoord(min(lng1,lng2)) );
  sym.SetStr(ITEM_LAT2, CCoord(max(lat1,lat2)) );
  sym.SetStr(ITEM_LNG2, CCoord(max(lng1,lng2)) );
  sym.SetNum(ITEM_SUMMARY, filetime);
  sym.SetNum(ITEM_WATERSHED, filesize);
  sym.SetNum(ITEM_CONN, gdal.projected);
  sym.SetNum(ITEM_TAG, res);

  if (MODE==0 && res<10) // reject irrelevant high res data
  {
  // get min max
  double maxh = InvalidAlt, minh = InvalidAlt;
  ScanMinMax(gdal, sym, oldlist, minh, maxh);

  // save MinMax
  sym.SetStr(ITEM_COORD, MkString("%s: %s - %s", CData(maxh-minh), CData(minh), CData(maxh)));
  if (minh<=InvalidAlt)
	minh = 0;
  if (maxh<=0 || maxh>=MAXELEV)
	{
	Log(LOGERR, "Invalid maxh %g (BAD DATA?) in %s", maxh, file);
	maxh=0;
	}
  if (maxh-minh<=60)
	{
	sym.SetStr(ITEM_NUM, "X");
	dellist.Add(sym);
    }
 }

  list.Add(sym);

#if 1  // test consistency of transformation
  gdal.Check(file);
#endif

 if (MODE>0)
	gdal.SaveThermal(file);			
 }

if (MODE==0)
{
vars xdir = "D:\\DEMX";
CreateDirectory(xdir, NULL);
for (int i=0; i<dellist.GetSize(); ++i)
	{
	Log(LOGINFO, "XDeleting %d/%d %s [%sm %s]", i, dellist.GetSize(), dellist[i].id, dellist[i].GetStr(ITEM_TAG), dellist[i].GetStr(ITEM_COORD));
	MoveFile(dellist[i].id, xdir + "\\"+ GetFileNameExt(dellist[i].id));
	}
}
#endif
}



void ElevationSaveKMZ(void)
{
	CreateDirectory(CACHE, NULL);

}

int cmpdem(CSym **arg1, CSym **arg2)
{
	register double rdiff = (*arg1)->GetNum(ITEM_DESC) - (*arg2)->GetNum(ITEM_DESC);
	if (rdiff<0) return -1;
	if (rdiff>0) return 1;
	rdiff = (*arg1)->GetNum(ITEM_SUMMARY) - (*arg2)->GetNum(ITEM_SUMMARY);
	if (rdiff<0) return 1;
	if (rdiff>0) return -1;
	return 0;
}

void ElevationSaveDEM(const char *arg)
{	
	CSymList list, oldlist;
	vars file = filename(DEMLIST);
	oldlist.Load(file);
	if (arg!=NULL && *arg!=0)
		oldlist.Load(arg);

	vara path(DEMFOLDER);
	for (int i=0;i<path.length(); ++i)
		ScanDEM(path[i], list, oldlist);

	list.Sort((qfunc *)cmpdem);
	list.header = "FILE,DESC,LAT1,LNG1,LAT2,LNG2,DATE,SIZE,PROJ,RES,ELEV,X";
	list.Save(file);

	CKMLOut out(file.replace(".csv",""));
	out += KMLName("Digital Elevation Coverage", "This map shows the coverage and resolution of the Digital Elevation data.<ul><li>Red : Low Resolution (~20m)</li><li>Yellow : Medium Resolution (~10m)</li><li>Green : High Resolution (~5m)</li></ul>");
	for (int i=list.GetSize()-1; i>=0; --i)
		{		
		LLRect box(list[i].GetNum(ITEM_LAT), list[i].GetNum(ITEM_LNG), list[i].GetNum(ITEM_LAT2), list[i].GetNum(ITEM_LNG2));
		out += KMLRect(GetFileName(list[i].id), list[i].GetStr(ITEM_DESC), box, resColor(list[i].GetNum(ITEM_DESC))); //jet(res));
		}
}


//
// River List
//
typedef BOOL tprogress(CRiverList *list, int i, int n);


class CRiverList {

	CSymList list;
	CRiverListPtr rplist;
	CRiverListPtr rblist; LLRect rbbox;
	CArrayList<CRiver> rdlist;
	LLRect bbox;
public:
	CString file;
	BOOL dirty;
	//BOOL dirtyscan;
	void *sorted;

	static int cmpelev(CRiver **a1, CRiver **a2)
	{
		const LLE *b1 = (*a1)->SE;
		const LLE *b2 = (*a2)->SE;
		double h1 = max(b1[0].elev,b1[1].elev);
		double h2 = max(b2[0].elev,b2[1].elev);
		if (h1>h2) return -1;
		if (h1<h2) return 1;
		return 0;
	}

	static int cmpbox(CRiver **a1, CRiver **a2)
	{
		const LLRect *b1 = &(*a1)->Box();
		const LLRect *b2 = &(*a2)->Box();
		if (b1->lat1>b2->lat1) return 1;
		if (b1->lat1<b2->lat1) return -1;
		if (b1->lng1>b2->lng1) return 1;
		if (b1->lng1<b2->lng1) return -1;
		return 0;
	}

	static int cmpriver(CRiver **a1, CRiver **a2)
	{
		if ((*a2)->id < (*a1)->id) return -1;
		if ((*a2)->id > (*a1)->id) return 1;
		return 0;
	}

	inline void SetSize(int n)
	{
		rplist.SetSize(n);
	}


	inline int GetSize(void)
	{
		return rplist.GetSize();
	}

	inline CRiver& operator[](int i) 
	{ 
		return *rplist[i]; 
	}

	inline CArrayList<CRiver> &Data()
	{
		return rdlist;
	}

	CRiver *Add(CRiver &r)
	{
		dirty = TRUE;
		sorted = FALSE;
		list.Add(*r.sym);
		Sort();
		return Find(r.id);
	}
/*
	CRiver *Add(tpoints &trk)
	{
		dirty = TRUE;
		return rplist.push(rdlist.push(CRiver(trk)));
	}
*/
	BOOL Save(const char *file)
	{
		CSymList symlist;
		for (int i=0; i<list.GetSize(); ++i)
			if (!list[i].data.IsEmpty())
				symlist.Add(list[i]);
		return symlist.Save(file);
	}

	/*
	void CheckSE(void)
	{
		for (int i=0; i<rplist.GetSize(); ++i)
			{
			if (rplist[i]->SE[0].lle!=&rplist[i]->SE[1] || &rplist[i]->SE[0]!=rplist[i]->SE[1].lle)
				Log(LOGALERT, "Inconsistent LLEC link");
			if (rplist[i]->SE[0].river==NULL || rplist[i]->SE[1].river==NULL)
				Log(LOGALERT, "Inconsistent LLEC river link");
			}
	}
	*/

	void Sort(void)
	{
		typedef int tcmp(CRiver **a1, CRiver **a2);
		tcmp *cmp = cmpriver;

		if (sorted==cmp && !dirty)
			return;
		if (dirty)
			Clean();

		rplist.Sort(sorted = cmp);
		// consistency check
		for (int i=rplist.GetSize()-1; i>0; --i)
			if (rplist[i-1]->id==rplist[i]->id)
				{
				CString sum1 = rplist[i-1]->GetStr(ITEM_SUMMARY);
				CString sum2 = rplist[i]->GetStr(ITEM_SUMMARY);
				if (sum1 != sum2)
					{
					Log(LOGALERT, "ERROR: Inconsistent CRiver ID %X for %s != %s", rplist[i]->id, sum1, sum2); 
					ASSERT(sum1 == sum2);
					continue;
					}
				rplist.Delete(i);
				}

		Direct(bbox);
	}


	CRiverListPtr SortList(int cmp(CRiver **a1, CRiver **a2))
	{
		// map elist
		int nsize = rplist.GetSize();
		CRiverListPtr elist(nsize);
		for (int i=0; i<nsize; ++i)
			elist[i] = rplist[i];
		elist.Sort(cmp);
		return elist;
	}


	CRiverList(const char *file)
	{
		dirty = TRUE;
		//dirtyscan = FALSE;
		Load(file);
		Sort();
	}

	CRiverList(CSymList &newlist)
	{
		dirty = TRUE;
		//dirtyscan = FALSE;
		list = newlist;
		Sort();
	}

	CRiverList(void)
	{
		dirty = TRUE;
		//dirtyscan = FALSE;
	}

	void Clean(void)
	{
		dirty = FALSE;
		//dirtyscan = FALSE;
		int n = list.GetSize();
		int rn = rdlist.GetSize();
		if (rn<n)
			{
			rdlist.SetSize(n);
			rplist.SetSize(n);
			for (int i=rn; i<n; ++i)
				rdlist[i].Set(&list[i], i);	
			}
		for (int i=0; i<n; ++i)
			rdlist[i].sym = &list[i];
		for (int i=0; i<rplist.GetSize(); ++i)
			rplist[i] = &rdlist[i];
	}

	~CRiverList(void)
	{
	}

	/*
	CRiver *Find(CRiver &search)
	{
		Sort();
		int f = rlist.Find(search);
		if (f<0) return NULL;
		return &rlist[f];
	}
	*/
	CRiver *Find(double id)
	{
		if (id==0)
			return NULL;

		CRiver fid;
		fid.id = id;
		int n = rplist.Find(&fid);
		if (n<0) return NULL;
		return rplist[n];
	}

	CRiver *Find(const char *summary)
	{
		CRiver search;
		FromHex(summary, &search.id);
		int f = rplist.Find(&search);
		if (f<0) return NULL;
		return rplist[f];
	}
	
	CRiver *Find(LLE *SE);

	CRiver *FindPoint(double lat, double lng, double mind = MINRIVERJOIN);

	void Load(const char *file)
	{
		dirty = TRUE;
		this->file = file;
		if (strchr(file,'\\')!=NULL)
			{
			// direct file load for -getrivers
			list.Load(file);
			}
		else
			{
			// group file load for runtime
			for (int i=0; RIVERSALL[i]!=NULL; ++i)
				{
				CString filepath = filename(file, RIVERSALL[i]);
				if (CFILE::exist(filepath))
					list.Load(filepath);
				}
			}
		if (!CBox::box(file, bbox))
			Log(LOGERR, "BAD RIVER FILE %s", file);
		Sort();
	}
	
	CSymList &SymList()
	{
		return list;
	}


	/*
	int Flush()
	{
		if (!dirtyscan)
			return TRUE;

		dirtyscan = FALSE;
		if (!SaveScan())
			{
			Log(LOGERR, "Could not write SCAN file %s", file);
			dirtyscan = TRUE;
			return FALSE;
			}
		return TRUE;
	}
	*/


	void Direct(LLRect &bbox);

};



class CCachedObj { 
public:
		CString id;
		int lasttime; 
		void *obj;
		int lock;
		CCachedObj(const char *id, int lasttime = 0, void *obj = NULL) 
		{ 
			this->id = id; 
			this->lasttime = lasttime; 
			this->obj = obj; 
			this->lock = 0;
		}

/*
		const int CCachedObj::operator-(const cobj &b) 
		{ 
			return id.Compare(b.id); 
		}
*/
};


class CSymListCache
{
private:
	CArrayList<CCachedObj *> list;
	double csize, maxsize;
	CSection CC;

public:
	/*
	virtual void *Create(const char *str) { return NULL; };
	virtual void Destroy(void *obj) { };
	*/

	int Size(void *obj) { return 1; }
		

	void *Create(const char *filepath)
	{
		CRiverList *rlist = new CRiverList(filepath);
		return rlist;
	}

	void Destroy(void *list)
	{
		//((CRiverList *)list)->Flush();
		delete (CRiverList *)list;
	}

	/*
	void Flush(void *list)
	{
		((CRiverList *)list)->Flush();
	}
	*/

	// cache functions
public:

	static int cmptime(CCachedObj **a1, CCachedObj **a2)
	{
		return (int)((*a2)->lasttime-(*a1)->lasttime);
	}

	static int cmpid(CCachedObj **a1, CCachedObj **a2)
	{
		return (*a1)->id.Compare((*a2)->id);
	}

	/*
	void Flush(void)
	{
		for (int i=0; i<list.GetSize(); i++)
		  Flush(list[i]->obj);
	}
	*/

	void Purge(void)
	{
		list.Sort(cmptime);
		for (int i=list.GetSize()-1; i>=0 && csize>maxsize; --i)
		  if (list[i]->lock==0)
			{
			csize -= Size(list[i]->obj);
			Destroy(list[i]->obj);
			delete list[i];
			list.Delete(i);
			}
	    if (csize>maxsize)
			Log(LOGERR, "Purge not purging %g>%g", csize, maxsize);
		list.Sort(cmpid);
	}

	CCachedObj *Lock(const char *filepath)
	{
	CCachedObj *obj;
	CC.Lock();
	int n = list.Find(&CCachedObj(filepath));
	if (n>=0)
		{
		// cached
		obj = list[n];
		++obj->lock;
		list[n]->lasttime = GetTickCount();
		}
	else
		{
		obj = new CCachedObj(filepath, GetTickCount(), Create(filepath));
		if (obj!=NULL)
			{
			++obj->lock;
			n = list.AddTail(obj);
			list[n]->lasttime = GetTickCount();
			if ((csize+=Size(obj))>maxsize)
				Purge();
			list.Sort(cmpid);
			}
		}
	CC.Unlock();
	return obj;
	}

	void Unlock(CCachedObj *obj)
	{
	if (obj)
		{
		CC.Lock();
		obj->lock--;
		CC.Unlock();
		}
	}


	void SetMaxsize(int maxsize)
	{
		this->maxsize = maxsize;
	}

	CSymListCache(int maxsize = MAXOPENSYM)
	{
		csize = 0;
		this->maxsize = maxsize;
	}

	~CSymListCache()
	{
		maxsize = 0;
		Purge();
	}
};


CSymListCache rivercache;


class CRiverCache
{
	CCachedObj *obj;
	
public:
	CRiverList *rlist;
	
	CRiverCache(const char *param, CRiver *&r)
	{
		r = NULL;

		vara argv(param);
		LLE SE[2];
		if (!RiverLL(argv[1], SE[0]))
			return;
		if (!RiverLL(argv[2], SE[1]))
			return;

		Lock(SE[0]);

		if (!rlist)
			return;

		r = rlist->Find(SE);
	}
	

	CRiverCache(LLE &lle)
	{
		Lock(lle);
	}

	CRiverCache(const char *file)
	{
		Lock(file);
	}

	void Lock(LLE &lle)
	{
		Lock(CBox::file(floor(lle.lat), floor(lle.lng)));
	}

	void Lock(const char *file)
	{
		rlist = NULL;
		obj = rivercache.Lock(file);
		if (obj)
			rlist = (CRiverList *)obj->obj;
	}
	
	~CRiverCache(void)			
	{
		rivercache.Unlock(obj);
	}
};






typedef CArrayList<CRiverList *> CCacheRiverList;

class CRiversCache {
		CArrayList<CCachedObj *> cachelist;

public:
		CCacheRiverList rlists;

		CRiversCache(LLRect *ebox, const char *folder = NULL)
		{
		CBox cb(ebox);
		for (LLRect *box =cb.Start(); box!=NULL; box=cb.Next())
			{
			CString file = cb.file();
			if (folder)
				{
				file = MkString("%s\\%s%s",folder, cb.file(), CSV);
				if (!CFILE::exist(file))
					continue;
				}
			CCachedObj *obj = rivercache.Lock(file);
			cachelist.AddTail(obj);
			if (obj && obj->obj)
				rlists.AddTail( (CRiverList *)obj->obj );
			else
				Log(LOGFATAL, "Could not cache file %s", file);			
			}
		}

		// release lock
		~CRiversCache(void)
		{
		for (int i=0; i<cachelist.GetSize(); ++i)
			rivercache.Unlock(cachelist[i]);
		}
};

/*
static int cmpriver(CRiverSym *a1, CRiverSym *a2)
{
	if (a2->id < a1->id) return -1;
	if (a2->id > a1->id) return 1;
	return 0;
}
*/


void CreateRWZ(void)
{
	CreateDirectory(RWZ, NULL);
	CreateDirectory(RWZ"\\0", NULL);
	for (int i=0; i<RWZSUBNUM; ++i)
		{
		CString dir = MkString(RWZ"\\%3.3X\\", i);
		CreateDirectory(dir, NULL);
		}
}

inline BOOL IsCached(const char *name)
{
	return name[strlen(name)-1]=='C';
}

CString ElevationFilename(const char *usig)
{
	CString sig = url_decode(usig);
	CString name = GetToken(sig, 0, ",;.(");
	// / ? < > \ : * | "
	name.Replace("/","");
	name.Replace("?","");
	name.Replace("<","");
	name.Replace(">","");
	name.Replace("\\","");
	name.Replace("#","");
	name.Replace(":","");
	name.Replace("*","");
	name.Replace("|","");
	name.Replace("&","");
	name.Replace("\"","");
	name.Replace(".", "");
	name.Replace(";", "");
	name.Replace(" ", "");

	// eliminate non ascii chars
	for (int i=name.GetLength()-1; i>=0; --i)
		if (name[i]<' ' || name[i]>'~')
			name.Delete(i);

	int pos = GetTokenPos(sig,4);
	if (pos>0 && sig[pos-1]==',')
		--pos;
	DWORD crc = crc32(sig, pos);
	name += MkString("(%8.8X)", crc);
	int len = strlen(sig);
	if (pos>0)
		{
		const char *mode = sig; mode +=pos;
		if (strchr(mode, copt[OMAPPER])) // Map
			name += copt[OMAPPER];
		if (strchr(mode, copt[OOVERLAY])) // Overlay
			name += copt[OOVERLAY];
		if (strchr(mode, copt[OGRID])) // Grid
			name += copt[OGRID] + GetToken(sig, 3);
		}
	if (IsCached(sig))
		name += "C";
	return name;
}

CString ElevationPath(const char *name)
{
	static int init = FALSE;
	if (!init) 
		CreateRWZ();
	if (IsCached(name)) 
		{
		const char *crcstr = strrchr(name, '(');
		if (!crcstr)
			{
			Log(LOGERR, "Invalid ElevationPath for %s", name);
			return name;
			}
		DWORD crc = 0;
		while (*crcstr!=0 && *crcstr!=')')
			crc = (crc<<4) | from_hex(*crcstr++);
		return MkString(RWZ"\\%3.3X\\", crc & (RWZSUBNUM-1)) + name;
		}
	else
#ifdef DEBUG
		return name;
#else
		return MkString(RWZ"\\0\\") + name;
#endif
}


//int l = list->FindColumn(ITEM_SUMMARY, argv[1]);

CString RiverSummary(CSym &sym)
{
		vars summary = sym.GetStr(ITEM_SUMMARY); 
		summary.Replace(" ",",");
		CRiver river(sym);
		return sym.id+" "+IdStr(river.id)+","+summary;
}



/*
int RiverBox(LLRect *bbox, CSymList &olist)
{
	CBox cb(bbox);
	for (LLRect *box =cb.Start(); box!=NULL; box=cb.Next())
		{
			CRiverList *list; CRiverCache obj(cb.file(), list);
			if (!list)
				{
				Log(LOGERR, "Missing river file %s for box %s - %s", cb.file(), CCoord2(bbox->lat1, bbox->lng1), CCoord2(bbox->lat2, bbox->lng2));
				continue;
				}
			for (int i=0; i<list->rlist.GetSize(); ++i)
				{
				CRiver &r = list->rlist[i];
				if (bbox->Intersects(r.Box()))
					AddRiver(olist, r);
				}
		}
	return TRUE;
}
*/

typedef void treefunc(CRiver *r, CRiverListPtr &rlist, int dir, void *data);


inline int isRiverJoined(const LLE &lle, CRiver *r, short int dir = 0, int *SE = NULL) // -1 up +1 down 0 any
{
			const LLRect &box = r->Box();
			if (box.lat2<lle.lat)
				return FALSE;
			if (box.lng2<lle.lng)
				return FALSE;
			if (box.lat1>lle.lat)
				return FALSE;  //break;  could make it a bit faster
			if (box.lng1>lle.lng)
				return FALSE;

			// reject disjoint segments
			double d[2];
			d[0] = LLE::Distance(&lle, &r->SE[0]);
			d[1] = LLE::Distance(&lle, &r->SE[1]);
			if ( d[0]>MINRIVERJOIN && d[1]>MINRIVERJOIN)
				return FALSE;

			// conn 
			short int conn = d[1]<d[0];
			// when going downhill stop if NOT going down conn -> !conn
			if (dir>0 && !LLE::Directional(&r->SE[conn], &r->SE[!conn]))
				return FALSE;
			// when going hphill stop if NOT going up !conn -> conn
			if (dir<0 && !LLE::Directional(&r->SE[!conn], &r->SE[conn]))
				return FALSE;

			// found!!!
			if (SE) *SE=conn;
			return TRUE;
}

class CRiverTree {
CRiverListPtr rblist;

public:
CSymList *outlist;
CDoubleArrayList idlist;
CRiverListPtr rlist;

int AddRiver(CRiver *r)
{		
	int n = idlist.GetSize();
	register double id = r->id;
	for (int i=0; i<n && idlist[i]!=id; ++i);
	if (i<n)
		return FALSE;
	idlist.AddTail(id);
	rlist.AddTail(r);
	if (outlist)
		(*outlist).Add(*r->sym);
	return TRUE;
}



int Recurse(const LLE &lle, int dir)
{
		/*
		if (MatchCoord(lat, 49.5062642) && MatchCoord(lng, -123.2025603))
			lat = lat;
		*/
		// parse list
		for (int i=0; i<rblist.GetSize(); ++i)
			{
			int SE;
			if (!isRiverJoined(lle, rblist[i], dir, &SE))
				continue;

			// recurse upstream or downstream only if not already added
			if (AddRiver(rblist[i]))
				Recurse(rblist[i]->SE[!SE], dir);
			}
		return TRUE;
}

CRiverTree(CSymList *olist = NULL)
{
		outlist = olist;
}


void GetBox(LLRect *bbox)
{
		CRiversCache cache(bbox);
		for (int j=0; j<cache.rlists.GetSize(); ++j)
			{
			CRiverList &rlist = *cache.rlists[j];
			for (int i=0; i<rlist.GetSize(); ++i)
				if ( bbox->Intersects( rlist[i].Box() ) )
					AddRiver(&rlist[i]);
			}
}

int GetTree(CRiver *r, LLRect *bbox, int dir, treefunc *func = NULL, void *data = NULL)
{

		CRiversCache cache(bbox);
		for (int j=0; j<cache.rlists.GetSize(); ++j)
			{
			CRiverList &rlist = *cache.rlists[j];
			for (int i=0; i<rlist.GetSize(); ++i)
				if ( bbox->Intersects( rlist[i].Box() ) )
					if (bbox->Contains(rlist[i].SE[0]) || bbox->Contains(rlist[i].SE[1]))
						rblist.AddTail(&rlist[i]);				
			}
		rblist.Sort(CRiverList::cmpbox);

		  // single (main river first)
		AddRiver(r);
		for (int se=0; se<2; ++se)
			  if (LLE::Directional(&r->SE[se], &r->SE[!se]))
				{
				if (dir<=0) 
					Recurse(r->SE[se], -1);
				if (dir>=0) 
					Recurse(r->SE[!se], 1);
				}

		if (func)
		  func(r, rlist, dir, data);

		return rlist.GetSize();
}

/*
REALLY NEEDED? maybe current good enough

int GetTreeDir(CRiver *r, LLRect *bbox, int dir, treefunc *func = NULL, void *data = NULL)
{
		CRiverListPtr rblist;
		rblist.AddTail(r); // first one

		CRiversCache cache(bbox);
		for (int j=0; j<cache.rlists.GetSize(); ++j)
			{
			CRiverList &rlist = *cache.rlists[j];
			for (int i=0; i<rlist.GetSize(); ++i)
				if ( bbox->Intersects( rlist[i].Box() ) )
					rblist.AddTail(&rlist[i]);				
			}


		if (r->dir==0)
			return;
		int s;
		if (r->dir>0)
			s = dir>0 ? 1 : 0;
		else
			s = dir>0 ? 0 : 1;
		LLRect bbox( r->SE[s].lat, r->SE[s].lng, MINRIVERJOIN);
		for (int i=0; i<rblist.GetSize(); ++i)
				{
				CRiver *ri = &rblist[i];
				if ((bbox.Contains(ri->SE[0]) && ri->dir==dir) || (bbox.Contains(rlist[i].SE[1]) && ri->dir==-dir))
					if (ri->id != r->id)
						rlist.AddTail(&rlist[i]);				
				}
			}

		if (func)
		  func(r, rblist, dir, data);

}
*/
};


int RiverJoins(LLE &lle, CRiverList *rlist)
{
	int joins = 0;
	CRiverList &rblist =  *rlist;
	int size = rblist.GetSize();
	for (int i=0; i<size; ++i)
		if (isRiverJoined(lle, &rblist[i]))
			++joins;
	return joins;
}


#if 1
void GetStartRiver(CRiver *r, CRiverListPtr &rlist, int dir, void *data)
{
	tpoint &start = *((tpoint *)data);
	tpoint origin = start;

	// upstream (0,1,2...)
	for (int i=1; i<rlist.GetSize(); ++i)
		{
		tcoords &coords = rlist[i]->Coords();
		if (coords.length()<2)
			continue;

		tpoint newstart;
		if (Distance(coords.Head(), origin) < Distance(coords.Tail(), origin))
			DistantPoint(coords, MAXUPRIVER, &newstart);
		else
			ReverseDistantPoint(coords, MAXUPRIVER, &newstart);
		DEMARRAY.height(&newstart);
		if (newstart.elev<start.elev)
			start = newstart;

		//ASSERT(!"keep trying...");
#ifdef DEBUGXXX
		Log(LOGINFO, "Not moved %s", start.Summary());
#endif
		}

}

int GetTree(CRiver *r, LLRect *bbox, int dir, treefunc *func = NULL, void *data = NULL)
{

		CRiverListPtr rblist;
		rblist.AddTail(r); // first one

		CRiversCache cache(bbox);
		for (int j=0; j<cache.rlists.GetSize(); ++j)
			{
			CRiverList &rlist = *cache.rlists[j];
			for (int i=0; i<rlist.GetSize(); ++i)
				{
				CRiver *ri = &rlist[i];
				if ((bbox->Contains(ri->SE[0]) && ri->dir==dir) || (bbox->Contains(rlist[i].SE[1]) && ri->dir==-dir))
					if (ri->id != r->id)
						rblist.AddTail(&rlist[i]);	// should check for duplicates (at quad edges) but not needed
				}
			}

		if (func)
		  func(r, rblist, dir, data);

		return rblist.GetSize();
}


int GetUpRiver(CRiver *r, tpoints &coords, tpoint &newstart)
{
		ASSERT( IdStr(r->id) != "R91eda03f0a71eb41");

		// set newstart
		double dist = Distance(coords[1], coords[0]);
		double angle = Bearing(coords[1], coords[0]);
		CoordDirection(coords[0], angle, MAXUPRIVER, newstart);
		DEMARRAY.height(&newstart);

		// default newstart
#if 1
		LLRect bbox( coords[0].lat, coords[0].lng, MINRIVERJOIN);
		int ujoins = GetTree(r, &bbox, -1, GetStartRiver, &newstart)-1;
#else
		// ERROR not precise enought 
		//int dir = Distance(coords[0].lat, coords[0].lng, r->SE[1].lat, r->SE[1].lng) < Distance(coords[0].lat, coords[0].lng, r->SE[0].lat, r->SE[0].lng);
		//LLRect bbox = LLDistance(r->SE[dir].lat, r->SE[dir].lng, MINRIVERJOIN*2);
		// int ujoins = utree.GetTree(r, &bbox, -1, GetStartRiver, &newstart)-1;
#endif

		// return number of joins upstream
		return ujoins;

		 /*
		// downstream
		//CRiverTree dtree(NULL);
		//if (rlist.GetSize()>1)
		 //if (dtree.GetTree(rlist[1], &bbox, 1)>1)
		if (join && coords.GetSize()>3)
			{
			// when there are river joins
			int skip = -1;
			for (int i=0; i<3; ++i)
				{
				CanyonMapper cm(NULL, "Check River");
				//double d  = Distance(coords[i], coords[i+1]);
				cm.MapPoint(coords[i].lat, coords[i].lng, MAXGUIDEDIST*2);
				if (cm.IsValidGuide(coords, MAXGUIDEDIST))
					break;
				// delete guide coord
				skip = i;
				}
			for (int i=0; i<skip; ++i)
				coords[i] = coords[skip+1];
			}
		*/
}
#else
int GetUpRiver(CRiver *r, CRiverList &rlist, tcoord &start, double distance)
{
	double distance = MAXGUIDEDIST*2;
	LLE lle(start);
	tcoord origin = start;
	for (int i=1; i<rlist.GetSize(); ++i)
		{
		if (!isRiverJoined(lle, &rlist[i], -1))
			continue;
		
		tcoords &coords = rlist[i].Coords();
		if (coords.length()==0)
			continue;

		if (Distance(coords[0], origin)>MINRIVERJOIN)
			{
			// starts from back
			for (int j=coords.length()-2; j>=0; --j)
				{
				if (Distance(coords[j], start)>distance)
					{
					// found
					start = coords[j];
					return TRUE;
					}
				}
			start = coords.Head();
			}
		else
			{
			// starts from back
			for (int j=1; j<coords.GetSize(); ++j)
				if (Distance(coords[j], start)>distance)
					{
					// found
					start = coords[j];
					return TRUE;
					}
			start = coords.Tail();
			}

		//ASSERT(!"keep trying...");
		}
	return FALSE;
}
#endif



int RiverListPoint(const char *param, CSym *sym, CSymList *ulist, CSymList *dlist)
{	
		LLE lle;
		if (!RiverLL(param, lle))
			return FALSE;
		CString dist = GetToken(param, 2, ';');
		double distance = CGetNum( dist );
		if (distance==InvalidNUM)
			distance = 20/km2mi*1000;

		LLRect bbox; 
		bbox = LLDistance(lle.lat, lle.lng, distance);

		for (int i=0; isdigit(dist[i]); ++i);
		if (dist[i]=='A')
			{
			// area
			CRiverTree utree(dlist);
			utree.GetBox(&bbox);
			return TRUE;
			}

		CRiver *r = NULL;
		CRiverCache obj(lle);
		if (obj.rlist)
			r = obj.rlist->FindPoint(lle.lat, lle.lng, MINRIVERPOINT);
		if (!r)
			{
			Log(LOGWARN, "Invalid river point %s (no river at %s,%s)", param, GetToken(param, 1, ';'), GetToken(param, 0, ';'));
			return FALSE;
			}
		if (sym)
			*sym = *r->sym;
		if (ulist)
			{
			CRiverTree utree(ulist);
			utree.GetTree(r, &bbox, -1);
			}
		if (dlist)
			{
			CRiverTree dtree(dlist);
			dtree.GetTree(r, &bbox, 1);
			}
		return TRUE;
}


BOOL isScanner(vara &argv)
{
	return argv.length()>4 && strstr(argv[4], "S"); // NOT USED
}

BOOL isScanner2(const char *param)
{
	return strstr(GetToken(param, 3), "S")!=NULL;
}

int RiverListSummary(const char *param, CSymList &ulist, CSymList &dlist) 
{
		CRiver *r = NULL; 
		CRiverCache obj(param, r);
		if (!r)
			{
			Log(LOGERR, "Invalid river summary %s", param);
			return FALSE;
			}
		
		vara argv(param);
		BOOL scanner = isScanner(argv);

		// get box of possible joints
		LLRect bbox; 
		double distance = scanner ? MAXCONNDIST : MAXRIVERJOIN;
		bbox = LLDistance(r->SE[0].lat, r->SE[0].lng, distance);
		bbox.Expand( LLDistance(r->SE[1].lat, r->SE[1].lng, distance) );

		if (scanner)
			{
			CRiverTree dtree(&dlist);
			dtree.GetTree(r, &bbox, 1);
			double minelev = CGetNum(argv[3]);
			// filter out below certain elevation
			for (int i=0; i<dlist.GetSize(); ++i)
				{
				CRiver r(dlist[i]);
				for (int j=0; j<2; ++j)
					DEMARRAY.height(&r.SE[j]);
				if (r.SE[0].elev<=minelev && r.SE[1].elev<=minelev)
					dlist.Delete(i--);
				}
			}
		else
			{
			/*
			if (area) // get from params
				return RiverBox(&bbox, olist);
			*/
			CRiverTree utree(&ulist);
			utree.GetTree(r, &bbox, -1);
			CRiverTree dtree(&dlist);
			dtree.GetTree(r, &bbox, 1);
			}
		return TRUE;
}


int RiverListScan(const char *param, vara &list) 
{
		vara argv(param);
		if (argv.length()<=4)
			return FALSE;

		LLE scan1, scan2, endrap;
		if (!RiverLL(argv[1], scan1))
			return FALSE;
		if (!RiverLL(argv[2], scan2))
			return FALSE;
		if (!RiverLL(argv[3], endrap))
			return FALSE;


#ifdef SHOWSCAN
		// simple scan map
		vars last = CCoord(endrap.elev);
		vara seg3(argv[2], ";");
		for (int i=0; i<seg3.length(); i+=3)
			{
			int inext = i+2+3;
			const char *elev = inext<seg3.length() ? seg3[inext] : last;
			list.push(MkString("Scan,%s,%s,%sE", seg3[i+1], seg3[i], elev));
			}
		return TRUE;
#else
		// full scan maps

		BOOL waterfall = scan1.elev>0; 
		if (waterfall)
			// add waterfall startand end elev (0 if no river join)
			list.push(MkString("Waterfall,%s,%s,%sE", CCoord(scan1.lat), CCoord(scan1.lng), CCoord(scan2.elev)));

		double ver = CGetNum(GetToken(argv[3], 1, 'S'));
		if (ver<SCANVERSION || TRUE)
			{
			// obsolete scan, so our best to match old raps
			list.push(MkString("River,%s,%s,%sE", CCoord(scan2.lat), CCoord(scan2.lng), CCoord(endrap.elev)));
			return TRUE;
			}

		// find river near point
		CRiver *r = NULL;
		CRiverCache obj(scan2);
		if (obj.rlist)
			r = obj.rlist->FindPoint(scan2.lat, scan2.lng, MAXUPRIVER*2);
		if (!r)
			{
			if (scan2.elev>0)
				Log(LOGERR, "Invalid scan river point %s (no river at %s)", param, scan2.Summary());
			return FALSE;
			}

		// get box of possible joints
		LLRect bbox; 
		double distance = CGetNum(argv[3]);
		bbox = LLRect(scan2.lat, scan2.lng, endrap.lat, endrap.lng);

		// find down river list
		CSymList dlist;
		CRiverTree dtree(&dlist);
		dtree.GetTree(r, &bbox, 1);
		for (int i=0; i<dlist.GetSize(); ++i)
			list.push(RiverSummary(dlist[i]));		

		/*
		double minelev = CGetNum(argv[3]);
		// filter out below certain elevation
		for (int i=0; i<dlist.GetSize(); ++i)
			{
			CRiver r(dlist[i]);
			for (int j=0; j<2; ++j)
				DEMARRAY.height(&r.SE[j]);
			if (r.SE[0].elev<=minelev && r.SE[1].elev<=minelev)
				dlist.Delete(i--);
			}
		*/
		return TRUE;
#endif
}


//
// KML
//
CString POIMarkerName(const char *file)
{
	// World Wide => WW
	// Panoramio => Pan
	// World of Waterfalls => WoW
	CString name = GetToken(file,0,'-');
	return "P"+name.MakeUpper();
}


void DIRMarkerList(CSymList &list, vara &paths, LLRect *bbox, char tag)
{
	CSymList dlist;
	CBox cb(bbox, POIGRIDSIZE);
	for (LLRect *box =cb.Start(); box!=NULL; box=cb.Next())
		{
		CString cbfile = cb.file();
		for (int i=0; i<paths.length(); ++i)
			{
			CString filepath = filename(cbfile, paths[i]);
			if (CFILE::exist(filepath))
				dlist.Load(filepath);
			}
		}
	// merge folders and merge descriptions
	FilterList(dlist, tag);
	UpdateList(list, dlist, NULL, TRUE); 
}

void POIMarkerList(CSymList &list, const char *file, const char *marker, LLRect *bbox = NULL)
{
	if (GetFileExt(file))
		{
		// CSV file
		list.Load(file);
		return;
		}

	// DIR file
	if (!bbox) bbox = Savebox;
	if (!bbox) return;

	vara paths( file, " ");
	DIRMarkerList(list, paths, bbox, marker[1]);
}


void POIMarkerList(CKMLOut &out, const char *file, const char *marker, LLRect *bbox = NULL)
{
		CSymList list;
		POIMarkerList(list, file, marker, bbox);
		out += KMLMarkerStyle(marker, NULL, 1);
		for (int j=0; j<list.GetSize(); ++j)
			{
			CSym &sym = list[j];
			double lat = sym.GetNum(ITEM_LAT);
			double lng = sym.GetNum(ITEM_LNG);
			if (bbox && !bbox->Contains(lat, lng))
				continue;
			out += KMLMarker(marker, lat, lng, sym.id, sym.GetStr(ITEM_DESC));
			}
}

void RWLocMarkertList(CKMLOut &out, LLRect *bbox)
{
		out +=
"<Style id=\"000\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/7/75/Starn00.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>0.0</scale></LabelStyle>"
"</Style><Style id=\"001\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/7/75/Starn00.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>1.1</scale></LabelStyle>"
"</Style><StyleMap id=\"00\"><Pair><key>normal</key><styleUrl>#000</styleUrl></Pair><Pair><key>highlight</key><styleUrl>#001</styleUrl></Pair></StyleMap><Style id=\"100\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/8/87/Starn10.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>0.0</scale></LabelStyle>"
"</Style><Style id=\"101\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/8/87/Starn10.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>1.1</scale></LabelStyle>"
"</Style><StyleMap id=\"10\"><Pair><key>normal</key><styleUrl>#100</styleUrl></Pair><Pair><key>highlight</key><styleUrl>#101</styleUrl></Pair></StyleMap><Style id=\"200\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/1/15/Starn20.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>0.0</scale></LabelStyle>"
"</Style><Style id=\"201\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/1/15/Starn20.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>1.1</scale></LabelStyle>"
"</Style><StyleMap id=\"20\"><Pair><key>normal</key><styleUrl>#200</styleUrl></Pair><Pair><key>highlight</key><styleUrl>#201</styleUrl></Pair></StyleMap><Style id=\"300\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/d/d3/Starn30.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>0.0</scale></LabelStyle>"
"</Style><Style id=\"301\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/d/d3/Starn30.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>1.1</scale></LabelStyle>"
"</Style><StyleMap id=\"30\"><Pair><key>normal</key><styleUrl>#300</styleUrl></Pair><Pair><key>highlight</key><styleUrl>#301</styleUrl></Pair></StyleMap><Style id=\"400\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/a/a0/Starn40.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>0.0</scale></LabelStyle>"
"</Style><Style id=\"401\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/a/a0/Starn40.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>1.1</scale></LabelStyle>"
"</Style><StyleMap id=\"40\"><Pair><key>normal</key><styleUrl>#400</styleUrl></Pair><Pair><key>highlight</key><styleUrl>#401</styleUrl></Pair></StyleMap><Style id=\"500\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/c/cc/Starn50.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>0.0</scale></LabelStyle>"
"</Style><Style id=\"501\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/c/cc/Starn50.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>1.1</scale></LabelStyle>"
"</Style><StyleMap id=\"50\"><Pair><key>normal</key><styleUrl>#500</styleUrl></Pair><Pair><key>highlight</key><styleUrl>#501</styleUrl></Pair></StyleMap><Style id=\"010\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/b/b6/Starn01.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>0.0</scale></LabelStyle>"
"</Style><Style id=\"011\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/b/b6/Starn01.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>1.1</scale></LabelStyle>"
"</Style><StyleMap id=\"01\"><Pair><key>normal</key><styleUrl>#010</styleUrl></Pair><Pair><key>highlight</key><styleUrl>#011</styleUrl></Pair></StyleMap><Style id=\"110\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/1/12/Starn11.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>0.0</scale></LabelStyle>"
"</Style><Style id=\"111\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/1/12/Starn11.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>1.1</scale></LabelStyle>"
"</Style><StyleMap id=\"11\"><Pair><key>normal</key><styleUrl>#110</styleUrl></Pair><Pair><key>highlight</key><styleUrl>#111</styleUrl></Pair></StyleMap><Style id=\"210\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/b/b7/Starn21.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>0.0</scale></LabelStyle>"
"</Style><Style id=\"211\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/b/b7/Starn21.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>1.1</scale></LabelStyle>"
"</Style><StyleMap id=\"21\"><Pair><key>normal</key><styleUrl>#210</styleUrl></Pair><Pair><key>highlight</key><styleUrl>#211</styleUrl></Pair></StyleMap><Style id=\"310\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/2/2e/Starn31.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>0.0</scale></LabelStyle>"
"</Style><Style id=\"311\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/2/2e/Starn31.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>1.1</scale></LabelStyle>"
"</Style><StyleMap id=\"31\"><Pair><key>normal</key><styleUrl>#310</styleUrl></Pair><Pair><key>highlight</key><styleUrl>#311</styleUrl></Pair></StyleMap><Style id=\"410\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/1/1d/Starn41.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>0.0</scale></LabelStyle>"
"</Style><Style id=\"411\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/1/1d/Starn41.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>1.1</scale></LabelStyle>"
"</Style><StyleMap id=\"41\"><Pair><key>normal</key><styleUrl>#410</styleUrl></Pair><Pair><key>highlight</key><styleUrl>#411</styleUrl></Pair></StyleMap><Style id=\"510\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/f/fe/Starn51.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>0.0</scale></LabelStyle>"
"</Style><Style id=\"511\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/f/fe/Starn51.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>1.1</scale></LabelStyle>"
"</Style><StyleMap id=\"51\"><Pair><key>normal</key><styleUrl>#510</styleUrl></Pair><Pair><key>highlight</key><styleUrl>#511</styleUrl></Pair></StyleMap><Style id=\"020\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/3/3a/Starn02.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>0.0</scale></LabelStyle>"
"</Style><Style id=\"021\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/3/3a/Starn02.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>1.1</scale></LabelStyle>"
"</Style><StyleMap id=\"02\"><Pair><key>normal</key><styleUrl>#020</styleUrl></Pair><Pair><key>highlight</key><styleUrl>#021</styleUrl></Pair></StyleMap><Style id=\"120\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/a/a4/Starn12.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>0.0</scale></LabelStyle>"
"</Style><Style id=\"121\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/a/a4/Starn12.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>1.1</scale></LabelStyle>"
"</Style><StyleMap id=\"12\"><Pair><key>normal</key><styleUrl>#120</styleUrl></Pair><Pair><key>highlight</key><styleUrl>#121</styleUrl></Pair></StyleMap><Style id=\"220\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/1/13/Starn22.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>0.0</scale></LabelStyle>"
"</Style><Style id=\"221\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/1/13/Starn22.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>1.1</scale></LabelStyle>"
"</Style><StyleMap id=\"22\"><Pair><key>normal</key><styleUrl>#220</styleUrl></Pair><Pair><key>highlight</key><styleUrl>#221</styleUrl></Pair></StyleMap><Style id=\"320\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/3/32/Starn32.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>0.0</scale></LabelStyle>"
"</Style><Style id=\"321\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/3/32/Starn32.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>1.1</scale></LabelStyle>"
"</Style><StyleMap id=\"32\"><Pair><key>normal</key><styleUrl>#320</styleUrl></Pair><Pair><key>highlight</key><styleUrl>#321</styleUrl></Pair></StyleMap><Style id=\"420\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/7/77/Starn42.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>0.0</scale></LabelStyle>"
"</Style><Style id=\"421\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/7/77/Starn42.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>1.1</scale></LabelStyle>"
"</Style><StyleMap id=\"42\"><Pair><key>normal</key><styleUrl>#420</styleUrl></Pair><Pair><key>highlight</key><styleUrl>#421</styleUrl></Pair></StyleMap><Style id=\"520\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/1/11/Starn52.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>0.0</scale></LabelStyle>"
"</Style><Style id=\"521\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/1/11/Starn52.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>1.1</scale></LabelStyle>"
"</Style><StyleMap id=\"52\"><Pair><key>normal</key><styleUrl>#520</styleUrl></Pair><Pair><key>highlight</key><styleUrl>#521</styleUrl></Pair></StyleMap><Style id=\"030\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/b/bd/Starn03.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>0.0</scale></LabelStyle>"
"</Style><Style id=\"031\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/b/bd/Starn03.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>1.1</scale></LabelStyle>"
"</Style><StyleMap id=\"03\"><Pair><key>normal</key><styleUrl>#030</styleUrl></Pair><Pair><key>highlight</key><styleUrl>#031</styleUrl></Pair></StyleMap><Style id=\"130\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/0/09/Starn13.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>0.0</scale></LabelStyle>"
"</Style><Style id=\"131\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/0/09/Starn13.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>1.1</scale></LabelStyle>"
"</Style><StyleMap id=\"13\"><Pair><key>normal</key><styleUrl>#130</styleUrl></Pair><Pair><key>highlight</key><styleUrl>#131</styleUrl></Pair></StyleMap><Style id=\"230\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/9/98/Starn23.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>0.0</scale></LabelStyle>"
"</Style><Style id=\"231\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/9/98/Starn23.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>1.1</scale></LabelStyle>"
"</Style><StyleMap id=\"23\"><Pair><key>normal</key><styleUrl>#230</styleUrl></Pair><Pair><key>highlight</key><styleUrl>#231</styleUrl></Pair></StyleMap><Style id=\"330\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/0/07/Starn33.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>0.0</scale></LabelStyle>"
"</Style><Style id=\"331\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/0/07/Starn33.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>1.1</scale></LabelStyle>"
"</Style><StyleMap id=\"33\"><Pair><key>normal</key><styleUrl>#330</styleUrl></Pair><Pair><key>highlight</key><styleUrl>#331</styleUrl></Pair></StyleMap><Style id=\"430\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/f/fb/Starn43.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>0.0</scale></LabelStyle>"
"</Style><Style id=\"431\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/f/fb/Starn43.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>1.1</scale></LabelStyle>"
"</Style><StyleMap id=\"43\"><Pair><key>normal</key><styleUrl>#430</styleUrl></Pair><Pair><key>highlight</key><styleUrl>#431</styleUrl></Pair></StyleMap><Style id=\"530\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/d/dc/Starn53.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>0.0</scale></LabelStyle>"
"</Style><Style id=\"531\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/d/dc/Starn53.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>1.1</scale></LabelStyle>"
"</Style><StyleMap id=\"53\"><Pair><key>normal</key><styleUrl>#530</styleUrl></Pair><Pair><key>highlight</key><styleUrl>#531</styleUrl></Pair></StyleMap><Style id=\"040\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/2/25/Starn04.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>0.0</scale></LabelStyle>"
"</Style><Style id=\"041\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/2/25/Starn04.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>1.1</scale></LabelStyle>"
"</Style><StyleMap id=\"04\"><Pair><key>normal</key><styleUrl>#040</styleUrl></Pair><Pair><key>highlight</key><styleUrl>#041</styleUrl></Pair></StyleMap><Style id=\"140\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/7/73/Starn14.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>0.0</scale></LabelStyle>"
"</Style><Style id=\"141\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/7/73/Starn14.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>1.1</scale></LabelStyle>"
"</Style><StyleMap id=\"14\"><Pair><key>normal</key><styleUrl>#140</styleUrl></Pair><Pair><key>highlight</key><styleUrl>#141</styleUrl></Pair></StyleMap><Style id=\"240\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/e/ea/Starn24.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>0.0</scale></LabelStyle>"
"</Style><Style id=\"241\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/e/ea/Starn24.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>1.1</scale></LabelStyle>"
"</Style><StyleMap id=\"24\"><Pair><key>normal</key><styleUrl>#240</styleUrl></Pair><Pair><key>highlight</key><styleUrl>#241</styleUrl></Pair></StyleMap><Style id=\"340\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/6/6a/Starn34.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>0.0</scale></LabelStyle>"
"</Style><Style id=\"341\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/6/6a/Starn34.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>1.1</scale></LabelStyle>"
"</Style><StyleMap id=\"34\"><Pair><key>normal</key><styleUrl>#340</styleUrl></Pair><Pair><key>highlight</key><styleUrl>#341</styleUrl></Pair></StyleMap><Style id=\"440\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/3/31/Starn44.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>0.0</scale></LabelStyle>"
"</Style><Style id=\"441\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/3/31/Starn44.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>1.1</scale></LabelStyle>"
"</Style><StyleMap id=\"44\"><Pair><key>normal</key><styleUrl>#440</styleUrl></Pair><Pair><key>highlight</key><styleUrl>#441</styleUrl></Pair></StyleMap><Style id=\"540\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/2/27/Starn54.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>0.0</scale></LabelStyle>"
"</Style><Style id=\"541\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/2/27/Starn54.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>1.1</scale></LabelStyle>"
"</Style><StyleMap id=\"54\"><Pair><key>normal</key><styleUrl>#540</styleUrl></Pair><Pair><key>highlight</key><styleUrl>#541</styleUrl></Pair></StyleMap><Style id=\"050\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/2/29/Starn05.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>0.0</scale></LabelStyle>"
"</Style><Style id=\"051\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/2/29/Starn05.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>1.1</scale></LabelStyle>"
"</Style><StyleMap id=\"05\"><Pair><key>normal</key><styleUrl>#050</styleUrl></Pair><Pair><key>highlight</key><styleUrl>#051</styleUrl></Pair></StyleMap><Style id=\"150\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/d/d9/Starn15.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>0.0</scale></LabelStyle>"
"</Style><Style id=\"151\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/d/d9/Starn15.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>1.1</scale></LabelStyle>"
"</Style><StyleMap id=\"15\"><Pair><key>normal</key><styleUrl>#150</styleUrl></Pair><Pair><key>highlight</key><styleUrl>#151</styleUrl></Pair></StyleMap><Style id=\"250\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/e/e0/Starn25.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>0.0</scale></LabelStyle>"
"</Style><Style id=\"251\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/e/e0/Starn25.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>1.1</scale></LabelStyle>"
"</Style><StyleMap id=\"25\"><Pair><key>normal</key><styleUrl>#250</styleUrl></Pair><Pair><key>highlight</key><styleUrl>#251</styleUrl></Pair></StyleMap><Style id=\"350\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/0/09/Starn35.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>0.0</scale></LabelStyle>"
"</Style><Style id=\"351\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/0/09/Starn35.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>1.1</scale></LabelStyle>"
"</Style><StyleMap id=\"35\"><Pair><key>normal</key><styleUrl>#350</styleUrl></Pair><Pair><key>highlight</key><styleUrl>#351</styleUrl></Pair></StyleMap><Style id=\"450\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/8/81/Starn45.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>0.0</scale></LabelStyle>"
"</Style><Style id=\"451\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/8/81/Starn45.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>1.1</scale></LabelStyle>"
"</Style><StyleMap id=\"45\"><Pair><key>normal</key><styleUrl>#450</styleUrl></Pair><Pair><key>highlight</key><styleUrl>#451</styleUrl></Pair></StyleMap><Style id=\"550\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/3/37/Starn55.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>0.0</scale></LabelStyle>"
"</Style><Style id=\"551\">"
"<IconStyle><scale>1.0</scale><Icon><href>http://ropewiki.com/images/3/37/Starn55.png</href></Icon></IconStyle>"
"<LabelStyle><color>ffc0c0ff</color><scale>1.1</scale></LabelStyle>"
"</Style><StyleMap id=\"55\"><Pair><key>normal</key><styleUrl>#550</styleUrl></Pair><Pair><key>highlight</key><styleUrl>#551</styleUrl></Pair></StyleMap>";



		CSymList list;
		CString file = filename(RWLOC);
		list.Load(file);
		for (int j=0; j<list.GetSize(); ++j)
			{
			CSym &sym = list[j];
			double lat = sym.GetNum(ITEM_LAT);
			double lng = sym.GetNum(ITEM_LNG);
			if (bbox && !bbox->Contains(lat, lng))
				continue;
			out += KMLMarker(sym.GetStr(ITEM_TAG), lat, lng, sym.GetStr(ITEM_WATERSHED), sym.GetStr(ITEM_DESC));
			}
}


void POIKMLSites(CKMLOut &out, const char *link = NULL)
{
	//out += KMLFolderStart( POI );

	CSymList filelist;
	const char *exts[] = { "DIR", "CSV", NULL };
	GetFileList(filelist, POI, exts, FALSE);

	const vara groups("W-Waterfalls,K-Kayaking,C-Canyons,V-Caves,M-Mines,H-Hot Springs,F-Ferratas");
	for (int g=0; g<groups.GetSize(); ++g)
	  {
      CString title = GetToken(groups[g], 1, '-');
	  out += KMLFolderStart(title, vars("P")+groups[g][0]);
	  for (int i=0; i<filelist.GetSize(); ++i)
		{
		CString name = GetFileName(filelist[i].id);
		if (name[0]!=groups[g][0])
			continue;
		CString marker = POIMarkerName(name);
		const char *minus = strchr(name, '-');
		CString fname = !minus ? name : minus+1;
		CString desc;
		if (strchr(fname,'.'))
			{
			// autoname
			desc = cdatajlink("http://www."+GetToken(fname,0," ").MakeLower()); 
			}
		else
			{
			// custom name
			CString header;
			CFILE::getheader(filelist[i].id, header);
			CString url = GetToken(header, 1);
			if (strncmp(url,"http",4)==0)
				desc = cdatajlink(url);
			}
		out += KMLFolderStart(fname, marker, TRUE, FALSE );
		out += KMLName(NULL, desc, -1);
		if (link)
			{
			CString path;
			if (IsSimilar(GetFileExt(filelist[i].id),"CSV"))
				path = filename(GetFileName(filelist[i].id)); // CSV file
			else				
				CFILE::getheader(filelist[i].id, path); // DIR file
			out += KMLLink(name, NULL, MkString("http://%s/rwr/?rwz=%s;%s,0,0,0,%s", server, path, marker, link));
			}
		else
			POIMarkerList(out, filelist[i].id, marker);
		out += KMLFolderEnd();
		}
	  out += KMLFolderEnd();
	  }

	//out += KMLFolderEnd();
}

vara GetWatershed(const char *str)
{
		vara stats(str, ";"); 
		while (stats.length()<WSMAX) stats.push("");
		// use catchment by default, but use watersimhed if available
		if (stats[WSAREA2]!="")
			stats[WSAREA1]=stats[WSAREA2];
		// consolidate cfs
		if (stats[WSCFS1]=="")
			stats[WSCFS1]=stats[WSCFS2];
		// consolidate temp
		if (stats[WSTEMP1]=="")
			stats[WSTEMP1]=stats[WSTEMP2];
		// consolidate prep
		if (stats[WSPREP1]=="")
			stats[WSPREP1]=stats[WSPREP2];
		return stats;
}



vara WatershedSummary(CSym &sym, int item = ITEM_WATERSHED)
{
		vara sum;

		// build stats
		vara stats = GetWatershed(sym.GetStr(item));
	
		const int titleid[] = { WSORDER, WSLEVEL, WSAREA1, WSCFS1, WSPREP1, WSTEMP1 };
		const char *title[] = { "O%d", "L%d", "Drainage:%dkm2", "AvgFlow:%dcfs", "AvgPrecip:%dmm", "AvgTemp:%dC", NULL };
		for (int i=0; i<stats.length() && title[i]!=NULL; ++i)
		  {
		  int id = titleid[i];
		  if (!stats[id].IsEmpty())
			  {
			  double v = CDATA::GetNum(stats[id]);
			  if (v!=InvalidNUM)
				  {
				  if (id==WSTEMP1 || id==WSPREP1) 
					  v = v/100;
				  sum.push( MkString( title[i], (int)(v+0.5) ) );
			      }
		      }
		  }


		return sum;
}

static vars htmlcleanup(const char *_str)
{
const char *stdsubst[] = { 
"&amp;", "&",
NULL
};

vars str(_str);
int i=0;
while (stdsubst[i]!=NULL)
	{
	str.Replace(stdsubst[i], stdsubst[i+1]);
	i+=2;
	}
return str.trim();
}

CString WatershedURL(CSym &sym)
{
		// get watershed info
		const char *match = "watershed_characterization";
		vara httplist(sym.GetStr(ITEM_DESC), "http:");
		for (int h=0; h<httplist.length() && !strstr(httplist[h], match); ++h);
		if (h>=httplist.length())
			return "";

		// get url
		CString url = GetToken(httplist[h], 0, "\"'");
		if (url.IsEmpty())
			return "";

		//if (!INVESTIGATE)  // do not update
		//	return -1;

		return "http:"+htmlcleanup(url);
}

void POIRiverList(CKMLOut &out, const char *folder, const char *file, LLRect *clip, int major = 0)
{
	    // skip files that do not exist
		CString riverfile = filename(file, folder);
		if (!CFILE::exist(riverfile))
			return;

		//CString postfix = folder + strlen(RIVERSBASE);

		CSymList list; list.Load(riverfile);	
		//CRiverList *plist; CRiverCache obj(file, plist);
		//if (!plist) return;
		//CSymList &list = *plist;
		
		for (int l=list.GetSize()-1; l>=0; --l)
			{
			CSym &sym = list[l];
			LLRect bbox(sym.GetNum(ITEM_LAT), sym.GetNum(ITEM_LNG), sym.GetNum(ITEM_LAT2), sym.GetNum(ITEM_LNG2));
			if (clip && !clip->Intersects(bbox))
				continue;
			
			vars coords(sym.GetStr(ITEM_COORD));
			coords.Replace(";", ",");
			vara list = coords.split(" ");
			CString desc = sym.GetStr(ITEM_DESC);
		
			CString url, button, button2;
			//if (!postfix.IsEmpty())
			//	name += '_' + postfix + '_';
			url = MkString("http://%s/rwr/?rwl=%s,R$", server, RiverSummary(sym));
			//CString url = MkString("http://%s/rwr/?rwz=%s,%s,%s,%s,R",server, sym.id, info[0], info[1], info[2]);
			button = MkString("<b><input type=\"button\" id=\"map\" onclick=\"window.open('%s', '_blank');\" value=\"Run Canyon Mapper on this stream\"></b><br>", url);
			LLE lle;
			if (RiverLL(list[list.length()/2], lle))
				{					
				url = MkString("http://ropewiki.com/index.php/Waterflow?location=%g,%g&watershed=on", lle.lat, lle.lng );
				button2 = MkString("<b><input type=\"button\" id=\"waterflow\" onclick=\"window.open('%s', '_blank');\" value=\"Run Waterflow Analysis for this stream\"></b><br>", url);
				//button2 = MkString("<b><input type=\"button\" id=\"map\" onclick=\"window.open('%s', '');\" value=\"Waterflow Analysis for this stream\"></b><br><br>", url);
				}


			// encapsulate additional tools
			CRiver r(sym);
			desc.Replace("<ul", "<br>Additional tools:<ul");
			desc.Replace(CDATAS, CDATAS"<div style='width:22em'>" + button + button2 + "<br>" +WatershedSummary(sym).join("<br>") + MkString("<br>RiverID:%s<br>", IdStr(r.id)));
			desc.Replace(CDATAE, "</div>" CDATAE );
			//MkString("<li><a href=\"%s\">Canyon Explorer Analysis</a></li></ul>", url));			

			// min 1 max 5
			double width = min(max(1, sym.GetNum(ITEM_WATERSHED)/2), 5);
			if (major)
				{
				double v = 2;
				if (major>0 && width<v)
					continue;
				if (major<0 && width>=v)
					continue;
				//ASSERT(!(major>0 && width>v));
				}
			out+=  KMLLine(sym.id, desc, list, RGB(0,0,0xFF), (int)width);
			}
}

void POIKMLRivers(CKMLOut &out, const char *folder, const char *link = NULL)
{
	//out+= KMLFolderStart( RIVERS );

	// Rivers
	//CBox cb(
	static CSymList rivers;
	GetFileList(rivers, filename("", folder), NULL);
	for (int i=0; i<rivers.GetSize(); ++i)
		{
		CString &file = rivers[i].id;

		LLRect bbox;
		if (!CBox::box(file, bbox))
			continue;
		// got file and bbox
		if (Savebox)
			if (!Savebox->Intersects(bbox))
				continue;

		// within range
		POIRiverList(out, folder, GetFileExt(file), Savebox);		
		}

	//out+= KMLFolderEnd();
}

/*
void POISaveKMLSites(void)
{
	CKMLOut out;
	POIKMLSites(out);
	KMLSave( POI, out);
}

void POISaveKMLRivers(void)	
{
	CKMLOut out;
	POIKMLRivers(out);
	KMLSave( RIVERS, out);
}
*/

void SaveCSVKML(const char *name)
{
	CSymList list;
	list.Load(name);

	CKMLOut out(GetFileName(name));
	for (int i=0; i<list.GetSize(); ++i)
		out += KMLMarker("pin", list[i].GetNum(ITEM_LAT), list[i].GetNum(ITEM_LNG), list[i].id, list[i].data);
}

void SaveKML(const char *name)
{
	CKMLOut out(GetFileName(name));
	out += KMLFolderStart(POI);
	POIKMLSites(out);
	out += KMLFolderEnd();
	
	out += KMLFolderStart(RIVERSUS);
	POIKMLRivers(out, RIVERSUS);
	out += KMLFolderEnd();
	
	out += KMLFolderStart(RIVERSCA);
	POIKMLRivers(out, RIVERSCA);
	out += KMLFolderEnd();
	
	out += KMLFolderStart("GAGES");
	WaterflowSaveKMLSites(out); 
	out += KMLFolderEnd();
}


CString KMLRegion(LLRect &box, int minlod = 512, int maxlod = -1)
{
	CString out;
	out += "<Region>";
	out += MkString("<LatLonAltBox><south>%.6f</south><west>%.6f</west><north>%.6f</north><east>%.6f</east></LatLonAltBox>", box.lat1, box.lng1, box.lat2, box.lng2);
	//double alt = Distance(box.lat1, box.lng1, box.lat2, box.lng2); 
	//out += MkString("<LatLonAltBox><south>%.6f</south><west>%.6f</west><north>%.6f</north><east>%.6f</east><minAltitude>0</minAltitude><maxAltitude>%.0f</maxAltitude><altitudeMode>absolute</altitudeMode></LatLonAltBox>", box.lat1, box.lng1, box.lat2, box.lng2, 500);
	out += MkString("<Lod><minLodPixels>%d</minLodPixels><maxLodPixels>%d</maxLodPixels></Lod>", minlod, maxlod); //<minFadeExtent>0</minFadeExtent><maxFadeExtent>0</maxFadeExtent>";
	out += "</Region>";
	return out;
}

int GridRegion(CKMLOut &out, const char *name, const char *p1, const char *p2)
{
	LLRect box(-90, -180, 90, 180);
	if (!RiverLL(p1, box.lat1, box.lng1) || !RiverLL(p2, box.lat2, box.lng2))
		{
		Log(LOGERR, "Grid error %s %s %s L2", name, p1, p2);
		return FALSE;
		}
	out += KMLRegion(box);
	return TRUE;
}

void DeleteKMZ(const char *gridname)
{
	CString cfile = ElevationPath(ElevationFilename(gridname))+KMZ;
#ifdef DEBUG
	Log(LOGINFO, "Deleted KMZ %s", cfile);
#endif
	DeleteFile(cfile);	
}

inline CString GridName(const char *name, const char *opt, double lat, double lng, double step = 10, double n = 2)
{
	LLRect ll(lat, lng, lat, lng);
	CBox cb(&ll, step); 
	LLRect *box = cb.Start();
	return MkString("%s,%g;%g,%g;%g,%g,%s", name, box->lng1, box->lat1, box->lng2, box->lat2, n, opt);
}

typedef BOOL tgrid(const char *name, double lat, double lng, double n);

int GridList(CKMLOut &out, const char *name, const char *p1, const char *p2, double n, const char *o, tgrid valid = NULL)
{
	int step;
	LLRect box(-90, -180, 90, 180);
	switch ((int)n)
	{
	  case 0:
			// 60  => 3x3x2 = 18 cells of 60x60
			step = 60;
			break;
	  case 1:
			// 10  => 6x6 = 36 cells of 10x10
			step = 10;
			if (!RiverLL(p1, box.lat1, box.lng1) || !RiverLL(p2, box.lat2, box.lng2))
				{
				Log(LOGERR, "Grid error %s %s %s L1", name, p1, p2);
				return FALSE;
				}
			break;
	  case 2:
			// 1  => 10x10 = 100 cells of 1x1
			step = 1;
			if (!RiverLL(p1, box.lat1, box.lng1) || !RiverLL(p2, box.lat2, box.lng2))
				{
				Log(LOGERR, "Grid error %s %s %s L2", name, p1, p2);
				return FALSE;
				}
			break;
	  default:
		  Log(LOGERR, "Invalid Grid level %g %s %s %s", n, name, p1, p2);
		  return FALSE;
	}

	COLORREF c[4] = { RGB(0xFF, 0, 0), RGB(0x00, 0xFF, 0), RGB(0x00, 0, 0xFF), RGB(0xFF, 0xFF, 0xFF) };
	for (float lat=box.lat1; lat<box.lat2; lat+=step)
		for (float lng=box.lng1; lng<box.lng2; lng+=step)			
			{
			if (valid && !valid(name, lat, lng, n+1))
				continue;
			CString id = MkString("%gx%gx%g", lat, lng, n);
			CString url = MkString("http://%s/rwr/?rwz=%s", server, GridName(name, o, lat, lng, step, n+1));
			out += "<NetworkLink>";
			out += KMLName(id);
			out += KMLRegion(LLRect(lat, lng, lat+step, lng+step));
			out += MkString("<Link><href>%s</href><viewRefreshMode>onRegion</viewRefreshMode></Link></NetworkLink>", url);
			}

	return TRUE;
}


	//http://tile.thunderforest.com/outdoors/5/6/12.png 4corners
	// http://maptile.mapplayer1.maps.svc.ovi.com/maptiler/maptile/newest/satellite.day/16/11438/39072
	//http://maptile.mapplayer1.maps.svc.ovi.com/maptiler/maptile/newest/satellite.day/15/5719/19536
	//http://services.arcgisonline.com/arcgis/rest/services/NGS_Topo_US_2D/MapServer/tile/13/2606/2857

typedef struct {
	int z;
	double res;
	CString url;
 } resolution;

/*

void UTM2LL(const char *utmZone, double utmX, double utmY, double &latitude, double &longitude)
	{
		if (!utmZone) utmZone = "0N";

		BOOL isNorthHemisphere = utmZone[strlen(utmZone)-1] >= 'N';

        const double diflat = -0.00066286966871111111111111111111111111;
        const double diflon = -0.0003868060578;

        double zone = atoi(utmZone);
        double c_sa = 6378137.000000;
        double c_sb = 6356752.314245;
        double e2 = pow((pow(c_sa,2) - pow(c_sb,2)),0.5)/c_sb;
        double e2cuadrada = pow(e2,2);
        double c = pow(c_sa,2) / c_sb;
        double x = utmX - 500000;
        double y = isNorthHemisphere ? utmY : utmY - 10000000;

        double s = ((zone * 6.0) - 183.0);
        double lat = y / (c_sa * 0.9996);
        double v = (c / pow(1 + (e2cuadrada * pow(cos(lat), 2)), 0.5)) * 0.9996;
        double a = x / v;
        double a1 = sin(2 * lat);
        double a2 = a1 * pow((cos(lat)), 2);
        double j2 = lat + (a1 / 2.0);
        double j4 = ((3 * j2) + a2) / 4.0;
        double j6 = ((5 * j4) + pow(a2 * (cos(lat)), 2)) / 3.0;
        double alfa = (3.0 / 4.0) * e2cuadrada;
        double beta = (5.0 / 3.0) * pow(alfa, 2);
        double gama = (35.0 / 27.0) * pow(alfa, 3);
        double bm = 0.9996 * c * (lat - alfa * j2 + beta * j4 - gama * j6);
        double b = (y - bm) / v;
        double epsi = ((e2cuadrada * pow(a, 2)) / 2.0) * pow((cos(lat)), 2);
        double eps = a * (1 - (epsi / 3.0));
        double nab = (b * (1 - epsi)) + lat;
        double senoheps = (exp(eps) - exp(-eps)) / 2.0;
        double delt  = atan(senoheps/(cos(nab) ) );
        double tao = atan(cos(delt) * tan(nab));

        longitude = ((delt * (180.0 / M_PI)) + s) + diflon;
        latitude = ((lat + (1 + e2cuadrada * pow(cos(lat), 2) - (3.0 / 2.0) * e2cuadrada * sin(lat) * cos(lat) * (tao - lat)) * (tao - lat)) * (180.0 / M_PI)) + diflat;
	}

char UTMZone(double Lat)
{
//This routine determines the correct UTM letter designator for the given latitude
//returns 'Z' if latitude is outside the UTM limits of 84N to 80S
	//Written by Chuck Gantz- chuck.gantz@globalstar.com
	char LetterDesignator;

	if((84 >= Lat) && (Lat >= 72)) LetterDesignator = 'X';
	else if((72 > Lat) && (Lat >= 64)) LetterDesignator = 'W';
	else if((64 > Lat) && (Lat >= 56)) LetterDesignator = 'V';
	else if((56 > Lat) && (Lat >= 48)) LetterDesignator = 'U';
	else if((48 > Lat) && (Lat >= 40)) LetterDesignator = 'T';
	else if((40 > Lat) && (Lat >= 32)) LetterDesignator = 'S';
	else if((32 > Lat) && (Lat >= 24)) LetterDesignator = 'R';
	else if((24 > Lat) && (Lat >= 16)) LetterDesignator = 'Q';
	else if((16 > Lat) && (Lat >= 8)) LetterDesignator = 'P';
	else if(( 8 > Lat) && (Lat >= 0)) LetterDesignator = 'N';
	else if(( 0 > Lat) && (Lat >= -8)) LetterDesignator = 'M';
	else if((-8> Lat) && (Lat >= -16)) LetterDesignator = 'L';
	else if((-16 > Lat) && (Lat >= -24)) LetterDesignator = 'K';
	else if((-24 > Lat) && (Lat >= -32)) LetterDesignator = 'J';
	else if((-32 > Lat) && (Lat >= -40)) LetterDesignator = 'H';
	else if((-40 > Lat) && (Lat >= -48)) LetterDesignator = 'G';
	else if((-48 > Lat) && (Lat >= -56)) LetterDesignator = 'F';
	else if((-56 > Lat) && (Lat >= -64)) LetterDesignator = 'E';
	else if((-64 > Lat) && (Lat >= -72)) LetterDesignator = 'D';
	else if((-72 > Lat) && (Lat >= -80)) LetterDesignator = 'C';
	else LetterDesignator = 'Z'; //This is here as an error flag to show that the Latitude is outside the UTM limits

	return LetterDesignator;
}
*/

/*Reference ellipsoids derived from Peter H. Dana's website- 
http://www.utexas.edu/depts/grg/gcraft/notes/datum/elist.html
Department of Geography, University of Texas at Austin
Internet: pdana@mail.utexas.edu
3/22/95

Source
Defense Mapping Agency. 1987b. DMA Technical Report: Supplement to Department of Defense World Geodetic System
1984 Technical Report. Part I and II. Washington, DC: Defense Mapping Agency
*/


const double PI = 3.14159265;
const double FOURTHPI = PI / 4;
const double deg2rad = PI / 180;
const double rad2deg = 180.0 / PI;
#define ELLIPSOIDRADIUS 6378137
#define ECCENTRICITY 0.00669438
/*
static Ellipsoid ellipsoid[] = 
{//  id, Ellipsoid name, Equatorial Radius, square of eccentricity	
	Ellipsoid( 23, "WGS-84", 6378137, 0.00669438)
	Ellipsoid( -1, "Placeholder", 0, 0),//placeholder only, To allow array indices to match id numbers
	Ellipsoid( 1, "Airy", 6377563, 0.00667054),
	Ellipsoid( 2, "Australian National", 6378160, 0.006694542),
	Ellipsoid( 3, "Bessel 1841", 6377397, 0.006674372),
	Ellipsoid( 4, "Bessel 1841 (Nambia) ", 6377484, 0.006674372),
	Ellipsoid( 5, "Clarke 1866", 6378206, 0.006768658),
	Ellipsoid( 6, "Clarke 1880", 6378249, 0.006803511),
	Ellipsoid( 7, "Everest", 6377276, 0.006637847),
	Ellipsoid( 8, "Fischer 1960 (Mercury) ", 6378166, 0.006693422),
	Ellipsoid( 9, "Fischer 1968", 6378150, 0.006693422),
	Ellipsoid( 10, "GRS 1967", 6378160, 0.006694605),
	Ellipsoid( 11, "GRS 1980", 6378137, 0.00669438),
	Ellipsoid( 12, "Helmert 1906", 6378200, 0.006693422),
	Ellipsoid( 13, "Hough", 6378270, 0.00672267),
	Ellipsoid( 14, "International", 6378388, 0.00672267),
	Ellipsoid( 15, "Krassovsky", 6378245, 0.006693422),
	Ellipsoid( 16, "Modified Airy", 6377340, 0.00667054),
	Ellipsoid( 17, "Modified Everest", 6377304, 0.006637847),
	Ellipsoid( 18, "Modified Fischer 1960", 6378155, 0.006693422),
	Ellipsoid( 19, "South American 1969", 6378160, 0.006694542),
	Ellipsoid( 20, "WGS 60", 6378165, 0.006693422),
	Ellipsoid( 21, "WGS 66", 6378145, 0.006694542),
	Ellipsoid( 22, "WGS-72", 6378135, 0.006694318),
};
*/

char UTMLetterDesignator(double Lat)
{
//This routine determines the correct UTM letter designator for the given latitude
//returns 'Z' if latitude is outside the UTM limits of 84N to 80S
	//Written by Chuck Gantz- chuck.gantz@globalstar.com
	char LetterDesignator;

	if((84 >= Lat) && (Lat >= 72)) LetterDesignator = 'X';
	else if((72 > Lat) && (Lat >= 64)) LetterDesignator = 'W';
	else if((64 > Lat) && (Lat >= 56)) LetterDesignator = 'V';
	else if((56 > Lat) && (Lat >= 48)) LetterDesignator = 'U';
	else if((48 > Lat) && (Lat >= 40)) LetterDesignator = 'T';
	else if((40 > Lat) && (Lat >= 32)) LetterDesignator = 'S';
	else if((32 > Lat) && (Lat >= 24)) LetterDesignator = 'R';
	else if((24 > Lat) && (Lat >= 16)) LetterDesignator = 'Q';
	else if((16 > Lat) && (Lat >= 8)) LetterDesignator = 'P';
	else if(( 8 > Lat) && (Lat >= 0)) LetterDesignator = 'N';
	else if(( 0 > Lat) && (Lat >= -8)) LetterDesignator = 'M';
	else if((-8> Lat) && (Lat >= -16)) LetterDesignator = 'L';
	else if((-16 > Lat) && (Lat >= -24)) LetterDesignator = 'K';
	else if((-24 > Lat) && (Lat >= -32)) LetterDesignator = 'J';
	else if((-32 > Lat) && (Lat >= -40)) LetterDesignator = 'H';
	else if((-40 > Lat) && (Lat >= -48)) LetterDesignator = 'G';
	else if((-48 > Lat) && (Lat >= -56)) LetterDesignator = 'F';
	else if((-56 > Lat) && (Lat >= -64)) LetterDesignator = 'E';
	else if((-64 > Lat) && (Lat >= -72)) LetterDesignator = 'D';
	else if((-72 > Lat) && (Lat >= -80)) LetterDesignator = 'C';
	else LetterDesignator = 'Z'; //This is here as an error flag to show that the Latitude is outside the UTM limits

	return LetterDesignator;
}


void LL2UTM(const double Lat, const double Long, double &UTMEasting, double &UTMNorthing, char* UTMZone)
{
//converts lat/long to UTM coords.  Equations from USGS Bulletin 1532 
//East Longitudes are positive, West longitudes are negative. 
//North latitudes are positive, South latitudes are negative
//Lat and Long are in decimal degrees
	//Written by Chuck Gantz- chuck.gantz@globalstar.com

	double a = ELLIPSOIDRADIUS; //ellipsoid[ReferenceEllipsoid].EquatorialRadius;
	double eccSquared = ECCENTRICITY; //ellipsoid[ReferenceEllipsoid].eccentricitySquared;
	double k0 = 0.9996;

	double LongOrigin;
	double eccPrimeSquared;
	double N, T, C, A, M;
	
//Make sure the longitude is between -180.00 .. 179.9
	double LongTemp = (Long+180)-int((Long+180)/360)*360-180; // -180.00 .. 179.9;

	double LatRad = Lat*deg2rad;
	double LongRad = LongTemp*deg2rad;
	double LongOriginRad;
	int    ZoneNumber;

	ZoneNumber = int((LongTemp + 180)/6) + 1;
  
	if( Lat >= 56.0 && Lat < 64.0 && LongTemp >= 3.0 && LongTemp < 12.0 )
		ZoneNumber = 32;

  // Special zones for Svalbard
	if( Lat >= 72.0 && Lat < 84.0 ) 
	{
	  if(      LongTemp >= 0.0  && LongTemp <  9.0 ) ZoneNumber = 31;
	  else if( LongTemp >= 9.0  && LongTemp < 21.0 ) ZoneNumber = 33;
	  else if( LongTemp >= 21.0 && LongTemp < 33.0 ) ZoneNumber = 35;
	  else if( LongTemp >= 33.0 && LongTemp < 42.0 ) ZoneNumber = 37;
	 }
	LongOrigin = (ZoneNumber - 1)*6 - 180 + 3;  //+3 puts origin in middle of zone
	LongOriginRad = LongOrigin * deg2rad;

	//compute the UTM Zone from the latitude and longitude
	sprintf(UTMZone, "%d%c", ZoneNumber, UTMLetterDesignator(Lat));

	eccPrimeSquared = (eccSquared)/(1-eccSquared);

	N = a/sqrt(1-eccSquared*sin(LatRad)*sin(LatRad));
	T = tan(LatRad)*tan(LatRad);
	C = eccPrimeSquared*cos(LatRad)*cos(LatRad);
	A = cos(LatRad)*(LongRad-LongOriginRad);

	M = a*((1	- eccSquared/4		- 3*eccSquared*eccSquared/64	- 5*eccSquared*eccSquared*eccSquared/256)*LatRad 
				- (3*eccSquared/8	+ 3*eccSquared*eccSquared/32	+ 45*eccSquared*eccSquared*eccSquared/1024)*sin(2*LatRad)
									+ (15*eccSquared*eccSquared/256 + 45*eccSquared*eccSquared*eccSquared/1024)*sin(4*LatRad) 
									- (35*eccSquared*eccSquared*eccSquared/3072)*sin(6*LatRad));
	
	UTMEasting = (double)(k0*N*(A+(1-T+C)*A*A*A/6
					+ (5-18*T+T*T+72*C-58*eccPrimeSquared)*A*A*A*A*A/120)
					+ 500000.0);

	UTMNorthing = (double)(k0*(M+N*tan(LatRad)*(A*A/2+(5-T+9*C+4*C*C)*A*A*A*A/24
				 + (61-58*T+T*T+600*C-330*eccPrimeSquared)*A*A*A*A*A*A/720)));
	if(Lat < 0)
		UTMNorthing += 10000000.0; //10000000 meter offset for southern hemisphere
}

void UTM2LL(const char *utmZone, const char *utmX, const char *utmY, float &latitude, float &longitude)
{
	latitude = longitude = InvalidNUM;
	double x = CDATA::GetNum(utmX);
	double y = CDATA::GetNum(utmY);
	if (x==InvalidNUM || y==InvalidNUM)
		return;
	double z = CDATA::GetNum(utmZone);
	if (z<1 || z>60)
		return;
	UTM2LL(utmZone, x, y, latitude, longitude);
}


void UTM2LL(const char* UTMZone, const double UTMEasting, const double UTMNorthing, float& lat,  float& lng )
{
//converts UTM coords to lat/long.  Equations from USGS Bulletin 1532 
//East Longitudes are positive, West longitudes are negative. 
//North latitudes are positive, South latitudes are negative
//Lat and Long are in decimal degrees. 
	//Written by Chuck Gantz- chuck.gantz@globalstar.com

	double k0 = 0.9996;
	double a = ELLIPSOIDRADIUS; //ellipsoid[ReferenceEllipsoid].EquatorialRadius;
	double eccSquared = ECCENTRICITY; //ellipsoid[ReferenceEllipsoid].eccentricitySquared;
	double eccPrimeSquared;
	double e1 = (1-sqrt(1-eccSquared))/(1+sqrt(1-eccSquared));
	double N1, T1, C1, R1, D, M;
	double LongOrigin;
	double mu, phi1, phi1Rad;
	double x, y;
	int ZoneNumber;
	char* ZoneLetter;
	int NorthernHemisphere; //1 for northern hemispher, 0 for southern

	x = UTMEasting - 500000.0; //remove 500,000 meter offset for longitude
	y = UTMNorthing;

	ZoneNumber = strtoul(UTMZone, &ZoneLetter, 10);
	if((*ZoneLetter - 'N') >= 0)
		NorthernHemisphere = 1;//point is in northern hemisphere
	else
	{
		NorthernHemisphere = 0;//point is in southern hemisphere
		y -= 10000000.0;//remove 10,000,000 meter offset used for southern hemisphere
	}

	LongOrigin = (ZoneNumber - 1)*6 - 180 + 3;  //+3 puts origin in middle of zone

	eccPrimeSquared = (eccSquared)/(1-eccSquared);

	M = y / k0;
	mu = M/(a*(1-eccSquared/4-3*eccSquared*eccSquared/64-5*eccSquared*eccSquared*eccSquared/256));

	phi1Rad = mu	+ (3*e1/2-27*e1*e1*e1/32)*sin(2*mu) 
				+ (21*e1*e1/16-55*e1*e1*e1*e1/32)*sin(4*mu)
				+(151*e1*e1*e1/96)*sin(6*mu);
	phi1 = phi1Rad*rad2deg;

	N1 = a/sqrt(1-eccSquared*sin(phi1Rad)*sin(phi1Rad));
	T1 = tan(phi1Rad)*tan(phi1Rad);
	C1 = eccPrimeSquared*cos(phi1Rad)*cos(phi1Rad);
	R1 = a*(1-eccSquared)/pow(1-eccSquared*sin(phi1Rad)*sin(phi1Rad), 1.5);
	D = x/(N1*k0);

	double Lat = phi1Rad - (N1*tan(phi1Rad)/R1)*(D*D/2-(5+3*T1+10*C1-4*C1*C1-9*eccPrimeSquared)*D*D*D*D/24
					+(61+90*T1+298*C1+45*T1*T1-252*eccPrimeSquared-3*C1*C1)*D*D*D*D*D*D/720);
	Lat = Lat * rad2deg;

	double Long = (D-(1+2*T1+C1)*D*D*D/6+(5-2*C1+28*T1-3*C1*C1+8*eccPrimeSquared+24*T1*T1)
					*D*D*D*D*D/120)/cos(phi1Rad);
	Long = LongOrigin + Long * rad2deg;

	lat = (float)Lat;
	lng = (float)Long;
}


class CMapSym  {
public:
	//double wkid;
	double xmin, ymin, xmax, ymax, zmax;
	double ox, oy;
	int tsize;
	CString id, utm;
public:
	CArrayList<resolution> resolutions;

	inline int IsUTM(void)
	{
		return utm[0]!=0;
	}


	CMapSym(void)
	{
		zmax = 0;
	}

	inline int IsLast(int z)
	{
		if (zmax<=0) 
			return TRUE;
		if (z>=zmax)
			return TRUE;
		return FALSE;
	}

	inline int GetSubTiles(int x, int y, int z, int &ix1, int &iy1, int &ix2, int &iy2)
	{
		int z1 = z+1;
		double x1, x2, y1, y2;
		if (z<0)
			{
			x1 = xmin, x2 = xmax, y1 = ymin, y2 = ymax;
			}
		else
			{
			double mul = tsize*resolutions[z].res;
			x1 = max(xmin, ox+x*mul);
			x2 = min(xmax, ox+(x+1)*mul);
			y1 = max(ymin, oy-(y+1)*mul);
			y2 = min(ymax, oy-y*mul);
			}
		if (x1>x2) { double aux = x1; x1 = x2; x2 = aux; }
		if (y1>y2) { double aux = y1; y1 = y2; y2 = aux; }
		double div = tsize*resolutions[z1].res;
		ix1 = (int)floor((x1-ox)/div);
		iy1 = (int)floor((oy-y2)/div);
		ix2 = (int)ceil((x2-ox)/div-1e-3);
		iy2 = (int)ceil((oy-y1)/div-1e-3);
		return z1;
	}




	BOOL Setup(CSym &sym)
	{
	id = sym.id;
	if (IsSimilar(id,";"))
		return FALSE;
	CString url = sym.GetStr(0);
	url.Replace("&amp;", "&");
	url.Replace("&", "&amp;");
	CString zoom = sym.GetStr(1);
	zmax = CDATA::GetNum(zoom);
	if (zoom[0]=='U')
		utm = zoom.Mid(1);
	double size = sym.GetNum(2);
	tsize = size<=0 ? 256 : (int)size;

	if (zmax>0)
		{
		// traditional google tiles
		double r = M_PI * EARTHRADIUS;
		xmin = -r; ymin = -r;
		xmax = r; ymax = r;
		ox = -r;
		oy = r;
		int chz = 3;
		double chgz=sym.GetNum(chz++); 
		for (int z=0; z<=zmax; ++z) {
			resolution res;
			res.z = z;
			res.res = (xmax-xmin)/tsize/pow(2.0,z);
			while (chgz!=InvalidNUM && z>=chgz)
				{
				url = sym.GetStr(chz++);
				chgz=sym.GetNum(chz++); 
				url.Replace("&amp;", "&");
				url.Replace("&", "&amp;");
				}
		
			res.url = url;
			resolutions.AddTail(res);
			}
		}
	else
		{
		// Mapserver
		zmax = 0;
		CString link = url;
		const char *MapServer = "/MapServer";
		int pos = link.Find(MapServer);
		if (pos<0)
			{
			Log(LOGERR, "No Mapserver url %s", link);
			return FALSE;
			}
		link = link.Left(pos+strlen(MapServer)) + "?f=json";
		DownloadFile f;
		if (f.Download(link)) {
			Log(LOGERR, "Could not download Mapserver url %s", link);
			return FALSE;
			}

		//wkid = ExtractNum(f.memory, "\"wkid\"", ":", "}");
		double rows = ExtractNum(f.memory, "\"rows\"", ":", ",");
		double cols = ExtractNum(f.memory, "\"cols\"", ":", ",");
		if (rows!=cols || rows<=0 || cols<=0) {
			Log(LOGERR, "Invalid MapServer cols!=rows %g!=%g", cols, rows);
			return FALSE;
			}
		
		ox = ExtractNum(f.memory, "\"x\"", ":", ",");
		oy = ExtractNum(f.memory, "\"y\"", ":", ",");
		vars extent = ExtractString(f.memory, "\"fullExtent\"", "{", "}");
		xmin = ExtractNum(extent, "\"xmin\"", ":", ",");
		xmax = ExtractNum(extent, "\"xmax\"", ":", ",");
		ymin = ExtractNum(extent, "\"ymin\"", ":", ",");
		ymax = ExtractNum(extent, "\"ymax\"", ":", ",");
		tsize = (int)rows;
		vars lods = ExtractString(f.memory, "\"lods\"", "[", "]");
		vara levels(lods, "\"level\"");
		for (int i=1; i<levels.length(); ++i) { //&& resolutions.GetSize()<=3
			double level = ExtractNum(levels[i], "", ":", ",");
			double reslevel = ExtractNum(levels[i], "\"resolution\"", ":", ",");
			if (level<0 || reslevel<0) {
				Log(LOGERR, "Invalid level resolution [%g] = %g", level, reslevel);
				continue;
				}
			resolution res;
			res.z = (int)level;
			res.res = reslevel;
			res.url = url;
			resolutions.AddAt(res, (int)level);
			}
		for (int r=1; r<resolutions.length(); ++r)
			if (resolutions[r-1].res/resolutions[r].res<1.75)
				resolutions.Delete(r--);
		zmax = resolutions.length()-1;
		}
	return TRUE;
	}

/*
	CMapSym *Set(CSym &sym)
	{
	if (id==sym.id)
		return zmax>0 ? this : NULL;
		
	ThreadLock();
	if (id==sym.id)
		return zmax>0 ? this : NULL;
	BOOL ret = Setup(sym);
	ThreadUnlock();
	return ret ? this : NULL;
	}
*/
	void zxy2LL(int size, int pixelZ, int pixelX, int pixelY, float &latitude, float &longitude)
	{
		double mapSize =  size *(1<<pixelZ);
		double tx = (Clip(size*pixelX, 0, mapSize - 1) / mapSize) - 0.5;
		double ty = 0.5 - (Clip(size*pixelY, 0, mapSize - 1) / mapSize);

		latitude = (float)(90 - 360 * atan(exp(-ty * 2 * M_PI)) / M_PI);
		longitude = (float)(360 * tx);
	}

	LLRect GetBox(int px, int py, int pz, LL rot[4])
	{
	if (pz<0) pz = 0;
	
	if (IsUTM())
		{
		double mul = tsize*resolutions[pz].res;
		double tx = ox+px*mul;
		double ty = oy-py*mul;
		UTM2LL(utm, tx, ty, rot[3].lat, rot[3].lng);
		UTM2LL(utm, tx+mul, ty-mul, rot[1].lat, rot[1].lng);
		UTM2LL(utm, tx+mul, ty, rot[2].lat, rot[2].lng);
		UTM2LL(utm, tx, ty-mul, rot[0].lat, rot[0].lng);
		return LLRect(rot[3].lat, rot[3].lng, rot[1].lat, rot[1].lng);
		}
	else
		{
		float lat1, lat2, lng1, lng2;
		zxy2LL(tsize, pz, px, py, lat1, lng1);
		zxy2LL(tsize, pz, px+1, py+1, lat2, lng2);
		LLRect box(lat1, lng1, lat2, lng2);
		rot[0].lat = rot[1].lat = lat1;
		rot[2].lat = rot[3].lat = lat2;
		rot[0].lng = rot[2].lng = lng1;
		rot[1].lng = rot[3].lng = lng2;
		return box;
		}	
	}

	CString GetLink(int x, int y, int z, LLRect &box)
	{
	CString link = resolutions[z].url;
	for (int i=0; link[i]!=0 && link[i+1]!=0 && link[i+2]!=0; ++i)
		 if (link[i]=='{' && link[i+2]=='}') {
			CString s;
			switch (link[i+1]) { // {X}, {Y}, {Z}, {T} = TMS Y, {W} = WMS Box, {M} = MapServer Box, {Q} = Quadkey
				case 'X':
					s = CData(x);
					break;
				case 'Y':
					s = CData(y);
					break;
				case 'Z':
					if (IsUTM())
						s = CData(resolutions[z].z);
					else
						s = CData(z);
					break;
				case 'Q': 
					s = QuadKey(x, y, z);
					break;
				case 'W':
					s = WmsBox(box, strstr(link, "900913")!=NULL, FALSE);
					break;
				case 'M':
					s = WmsBox(box, strstr(link, "900913")!=NULL, TRUE);
					break;
				case 'T':
					s = CData((1<<z)-y-1);
					break;
				default:
					continue;
				}
			link.Delete(i, 3);
			link.Insert(i, s);
			}
	 return link;
	}

};

CSymList maplist;
CArrayList<CMapSym> maps;





int KMLOverlay(CKMLOut &out, CMapSym *m, int x, int y, int z, LLRect &box, LL rot[4], int minlod = -1, int maxlod = -1)
{
	CString link = m->GetLink(x, y, z, box);
	out += MkString("<GroundOverlay><drawOrder>%d</drawOrder>", z);
	out += MkString("<name>%d-%d-%d</name>", x, y, z);
	if (minlod>-1 || maxlod>-1)
		out += KMLRegion(box, minlod, maxlod);
    out += MkString("<Icon><href>%s</href><viewRefreshMode>onRegion</viewRefreshMode></Icon>", link);
      //<refreshMode>onInterval</refreshMode><refreshInterval>86400</refreshInterval><viewBoundScale>0.75</viewBoundScale>
	if (m->IsUTM())
		out += MkString("<gx:LatLonQuad><coordinates>%.6f,%.6f %.6f,%.6f %.6f,%.6f %.6f,%.6f</coordinates></gx:LatLonQuad>",
			rot[0].lng, rot[0].lat, rot[1].lng, rot[1].lat, rot[2].lng, rot[2].lat, rot[3].lng, rot[3].lat);
	else
		out += MkString("<LatLonBox><south>%.6f</south><west>%.6f</west><north>%.6f</north><east>%.6f</east></LatLonBox>", 
			box.lat1, box.lng1, box.lat2, box.lng2);
	out += "</GroundOverlay>";
	return TRUE;
}



#if 1
int OverlayRecurse(CKMLOut &out, CMapSym *m, int px, int py, int pz, int lev = 0)
{
	if (!m) return FALSE;
	// calc region
	//LLRect pbox = OverlayBox(px>>1, py>>1, pz-1); 
	LL rotation[4];
	LLRect box = m->GetBox(px, py, pz, rotation);

	BOOL last = m->IsLast(pz);

	int minx, miny, maxx, maxy;
	int z = last ? pz : m->GetSubTiles(px, py, pz, minx, miny, maxx, maxy);
	int MINOVERLAY = pz<=0 ? -1 : (int)(m->tsize / (m->resolutions[pz-1].res/m->resolutions[pz].res));
	int MAXOVERLAY = last ? -1 : (int)(m->tsize * 1.5);

	if (lev>3)
		{
		// set network link
		out += "<NetworkLink>";
		//out += KMLName(id);
		out += KMLRegion(box, MINOVERLAY);
		CString url = MkString("http://%s/rwr/?rwz=%s,%d,%d,%d,OC", server, m->id, px, py, pz);
		out += "<Link><href>";
		out += url;
		out +="</href><viewRefreshMode>onRegion</viewRefreshMode></Link>";
		out +="</NetworkLink>";
		return TRUE;
		}

	// display overlay
	if (pz>=0)
		{		
		KMLOverlay(out, m, px, py, pz, box, rotation, MINOVERLAY, MAXOVERLAY);
		if (last) 
			return TRUE;
		}
	// overlay children
	//out += KMLFolderStart( MkString("%d-%d-%d+", px, py, pz)); //, maps[m].name, TRUE );
	//out += "<viewRefreshMode>onRegion</viewRefreshMode>";
	//out += KMLRegion(box, MAXOVERLAY, -1);
	//out += "<viewRefreshMode>onRegion</viewRefreshMode>";	
	for (int y=miny; y<maxy; ++y)
		for (int x=minx; x<maxx; ++x)
			OverlayRecurse(out, m, x, y, z, lev+1);
	//out += KMLFolderEnd();

	//out += "</Document>";
	return TRUE;
}
#else

int OverlayRecurse(CKMLOut &out, int m, int px, int py, int pz, int lev = 0)
{
	// calc region
	//LLRect pbox = OverlayBox(px>>1, py>>1, pz-1); 
	LLRect box = OverlayBox(px, py, pz);

	if (pz >6)
		{
		return TRUE;
		}

	out += "<Folder>";
	out += KMLRegion(box, MINOVERLAY, -1);
	//out += "<visibility>0</visibility>";
	//out += "<viewRefreshMode>onRegion</viewRefreshMode>";

	// overlay children
	//out += KMLFolderStart( MkString("%d-%d-%d", px, py, pz)); //, maps[m].name, TRUE );
	//out += KMLRegion(box.lat1, box.lng1, box.lat2, box.lng2, MAXOVERLAY, -1 );
	//out += "<viewRefreshMode>onRegion</viewRefreshMode>";
	int sz = pz+1, sx = px<<1, sy = py<<1;
	//ex <<= 1; ey <<= 1; 
	//LLRect cbox = OverlayBox(sx+x, sy+y, sz);
	CString children = MkString("%d-%d-%d+", px, py, pz);
	out += MkString("<Folder id=\"%s\">", children);
	out += KMLRegion(box, MAXOVERLAY, -1);
	//out += "<visibility>0</visibility>";
	//out += "<viewRefreshMode>onRegion</viewRefreshMode>";
	for (int y=0; y<2; ++y)
		for (int x=0; x<2; ++x)
			{
#if 1
			OverlayRecurse(out, m, sx+x, sy+y, sz, lev+1);
#else
			// set network link
			out += "<NetworkLink>";
			out += KMLName(id);
			out += KMLRegion(box, MAXOVERLAY, -1);
			CString url = MkString("http://%s/rwr/?rwz=%s,%d,%d,%d,OC", server, maps[m].id, sx+x, sy+y, sz);
			out += MkString("<Link><href>%s</href><viewRefreshMode>onRegion</viewRefreshMode></Link></NetworkLink>", url);
#endif
			}
	out += "</Folder>";

	// display overlay
	if (pz!=0)
		{
		// set overlay
		BOOL last = pz>=maps[m].index;
		out += "<Folder>";
		//out += KMLFolderStart( MkString("%d-%d-%d", px, py, pz)); //, maps[m].name, TRUE );
		//out += KMLRegion(box, 5, MAXOVERLAY);
		out += KMLRegion(box, MINOVERLAY, last ? -1 : MAXOVERLAY);
		//out += "<visibility>0</visibility>";
		KMLOverlay(out, m, px, py, pz, box);

		out += MkString("<NetworkLinkControl><Update><targetHref>http://%s/rwr/?rwz=CTOPO,0,0,0,O</targetHref>\n", server);
		out += MkString("<Change><Folder targetId=\"%s\"><visibility>0</visibility></Folder></Change>", children);
		out += MkString("</Update></NetworkLinkControl>");
		out += "</Folder>";
		if (last) 
			return TRUE;
		}

	out += "</Folder>";
	return TRUE;
}

#endif


int OverlayList(CKMLOut &out, const char *name, double px, double py, double pz)
{
	static int loaded = FALSE;
	out.SetExtensions();
	if (!loaded)
		{
		ThreadLock();
		if (!loaded)
			{			
			maplist.Load(filename("maps.csv"));
			maplist.Sort();
			maps.SetSize(maplist.GetSize());
			for (int i=0; i<maps.GetSize(); ++i)
				maps[i].Setup(maplist[i]);
			loaded=TRUE;
			}
		ThreadUnlock();
		}
	int f = maplist.Find(name);
	if (f>=0 && maps[f].zmax>0)
		return OverlayRecurse(out, &maps[f], (int)px, (int)py, (int)pz);
	// map not found
	Log(LOGERR, "Invalid Overlay map name %s px=%s py=%s pz=%s", name, CData(px), CData(py), CData(pz));
	return TRUE;
}


void RWE(CKMLOut &out, const char *line)
{
	// Ropewiki Explorer folders

	if (strstr(line, "#POI#"))
		{
		POIKMLSites(out, POIOPT);
		return;
		}

	if (strstr(line, "#WATERFLOW#"))
		{
		out += KMLFolderStart("Waterflow", "waterflow", FALSE, FALSE);
		out += KMLName(NULL, "http://ropewiki.com/Waterflow", -1 );
		WaterflowSaveKMLSites(out, "GWC");
		out += KMLFolderEnd();
		return;
		}

	if (strstr(line, "#RIVERS#"))
		{
		const char *name = "US Streams";
		out += KMLFolderStart(name, "rivers", TRUE, FALSE);
		out += KMLName(NULL, CDATAS "<ul style=\"width:45em\">http://water.epa.gov/scitech/datait/tools/waters</ul>" CDATAE, -1 );
		out += KMLLink(name, NULL, MkString("http://%s/rwr/?rwz=" RIVERSUS ",0,0,0,GRC", server));
		out += KMLFolderEnd();
		return;
		}

	if (strstr(line, "#RIVERSCA#"))
		{
		const char *name = "Canada Streams";
		out += KMLFolderStart(name, "rivers", TRUE, FALSE);
		out += KMLName(NULL, CDATAS "<ul style=\"width:45em\">http://ftp2.cits.rncan.gc.ca/pub/geobase/official/nhn_rhn/doc/NHN.pdf</ul>" CDATAE, -1 );
		out += KMLLink(name, NULL, MkString("http://%s/rwr/?rwz=" RIVERSCA ",0,0,0,GRC", server));
		out += KMLFolderEnd();
		return;
		}

	if (strstr(line, "#RIVERSMX#"))
		{
		const char *name = "Mexico Streams";
		out += KMLFolderStart(name, "rivers", TRUE, FALSE);
		out += KMLName(NULL, CDATAS"<ul style=\"width:45em\">http://www.inegi.org.mx/geo/contenidos/topografia/regiones_hidrograficas.aspx</ul>" CDATAE, -1 );
		out += KMLLink(name, NULL, MkString("http://%s/rwr/?rwz=" RIVERSMX ",0,0,0,GRC", server));
		out += KMLFolderEnd();
		return;
		}

	if (strstr(line, "#TEST#"))
		{
		// Grid Test
		out += KMLFolderStart("Grid");
		out += KMLLink("Grid", NULL, MkString("http://%s/rwr/?rwz=Grid,0,0,0,GTC", server));
		out += KMLFolderEnd();
		return;
		}

	if (strstr(line, "#RWLOC#"))
		{
		// Grid Test
		out += KMLFolderStart("Ropewiki Locations");
		out += KMLLink(RWLOC, NULL, MkString("http://%s/rwr/?rwz=%s,0,0,0,%s", server, RWLOC, RWLOCOPT));
		out += KMLFolderEnd();
		return;
		}


	const char *localhost = "http://localhost";
	if (strstr(line, localhost))
		{
		CString nline(line);
		nline.Replace(localhost, vars("http://")+server);
		out += nline;
		return;
		}
	
	out += line;
}


void RWEPLUS(CKMLOut &out, const char *line)
{
	if (strstr(line, "#RWS#"))
		{
		vars str = 
		"<NetworkLink targetId=\"activescanner\">"
		"<name>Ropewiki Explorer+</name>"
		"<Style><ListStyle><ItemIcon><href>http://ropewiki.com/favicon.ico</href></ItemIcon></ListStyle></Style>"
		"<visibility>0</visibility>"
		"<open>1</open>"
		"<Snippet maxLines=\"0\"></Snippet>"
		"<flyToView>0</flyToView>"
		"<Link>"
		"<href>http://"+vars(server)+"/rwr</href>"
		"<refreshMode>onChange</refreshMode>"
		"<viewRefreshMode>onStop</viewRefreshMode>"
		"<viewRefreshTime>0.5</viewRefreshTime>"
		"<viewBoundScale>0.5</viewBoundScale>"
		"<viewFormat>rws=*_[bboxSouth]_[bboxWest]_[bboxNorth]_[bboxEast].kml</viewFormat>"
		"</Link>"
		"</NetworkLink>";
		out += str;
		return;
		}
	return RWE(out, line);
}

BOOL rvalid(const char *name, double lat, double lng, double n)
{
	CString file = filename(CBox::file(lat, lng), name);
	return CFILE::exist(file);
}

/*
vars ScannerMap(const char *param)
{
	// SCANNER MAP: decide on the fly if map to segment or point

	vara argv(param);
	if (!isScanner(argv))
		return param;
	if (!strstr(argv[4], "SMC"))
		return param;

	double lat = (float)CGetNum( argv[1] );
	double lng = (float)CGetNum( argv[2] );
	if (!CheckLL(lat, lng))
		{
		Log(LOGERR, "invalid SCANNER param '%s'", param);
		return param;
		}
#if 0  
		return param;
#else
	// try to map to river
	CRiver *r = NULL;
	CRiverCache obj(LLE(lat,lng));
	if (obj.rlist)
		r = obj.rlist->FindPoint(lat, lng);
	if (!r)
		return param;

	vara summary(RiverSummary(*r->sym));
	if (summary.length()<4)
		return param;

	argv[0] = summary[0];
	argv[1] = summary[1];
	argv[2] = summary[2];
	argv[4].Replace("SMC","SR");

	vars newparam = argv.join();
	Log(LOGINFO, "Mapping SCANNER POINT %s --> %s", param, newparam);
	return newparam;
#endif
}
*/

int CanyonMap(const char *param, CKMLOut &out, BOOL map, CStringArrayList *filelist = NULL)
{
	vara argv(param);

	const char *ext = GetFileExt(argv[0]);
	if (ext)
	 if (stricmp(ext, "+KC")==0 || stricmp(ext, "KC")==0 || stricmp(ext, "K")==0 || stricmp(ext, "KML")==0)
		{
		if (strchr(ext, '+')!=NULL)
			out.Load( filename(GetFileName(argv[0])+KML), RWEPLUS );
		else
			out.Load( filename(GetFileName(argv[0])+KML), RWE );
		return TRUE;		
		}	


	// basic stuff
	static int cnt = 0;
	double n[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	BOOL err = FALSE;
	for (int i=1; i<4 && i<argv.length(); ++i)
		err |= (n[i] = CGetNum(argv[i]) ) == InvalidNUM;
	if (err || i<3)
		{
		Log(LOGERR, "invalid coordinates parameter %s", param);
		return FALSE;
		}

	// options
	CString opt;
	if (argv.length()>4)
		opt = argv[4].MakeUpper();
	int iopts[ONUM];
	for (int i=0; i<ONUM; ++i)
		iopts[i] = strchr(opt, copt[i])!=NULL;

	if (map) iopts[OMAPPER] = TRUE;

	if (iopts[OGRID])
		{	
		// Point of interest files
		//Log(LOGINFO, "Grid %s", param);
		vars name = url_decode(argv[0]);
		vara namep = name.split(";");

		// regions bounding box
		LLRect bbox;
		if (n[3]>0)
		   if (!RiverLL(argv[1], bbox.lat1, bbox.lng1) || !RiverLL(argv[2], bbox.lat2, bbox.lng2))
			{
			Log(LOGERR, "Invalid bbox %s , %s", argv[1], argv[2]);
			return FALSE;
			}

		if (iopts[OTEST])
			{
			if (n[3]<2)
				GridList(out, argv[0], argv[1], argv[2], n[3], opt);
			else
				{
				GridRegion(out, argv[0], argv[1], argv[2]);
				out += KMLRect("item", "", bbox, RGB(0xFF, 0xFF, 0xFF));
				}
			return TRUE;
			}

		if (iopts[OPOI] && namep.length()>=2)
			{
			if (n[3]<2)
				GridList(out, argv[0],argv[1],argv[2], n[3], opt);
			else
				{
				GridRegion(out, argv[0], argv[1], argv[2]);
				POIMarkerList(out, namep[0], namep[1], &bbox);
				}
			return TRUE;
			}

		if (iopts[OWATERFLOW] && namep.length()>=3)
			{
			if (n[3]<2)
				GridList(out, argv[0],argv[1],argv[2], n[3], opt);
			else
				{
				GridRegion(out, argv[0], argv[1], argv[2]);
				WaterflowMarkerList(out, namep[0], namep[1], namep[2], &bbox);
				}
			return TRUE;
			}

		if (iopts[ORWLOC])
			{
			if (n[3]<2)
				GridList(out, argv[0],argv[1],argv[2], n[3], opt);
			else
				{
				GridRegion(out, argv[0], argv[1], argv[2]);
				RWLocMarkertList(out, &bbox);
				}
			return TRUE;
			}

		if (iopts[ORIVERS])
			{
			if (n[3]<2)
				GridList(out, argv[0],argv[1],argv[2], n[3], opt);
			else if (n[3]<3)
				{				
				/*
				CBox cb(&bbox);
				GridRegion(out, argv[0], argv[1], argv[2]);
				for (LLRect *box =cb.Start(); box!=NULL; box=cb.Next())
					POIRiverList(out, argv[0], cb.file(), &bbox, 1);
				*/
				GridList(out, argv[0],argv[1],argv[2], n[3], opt, rvalid);
				}
			else
				{
				GridRegion(out, argv[0], argv[1], argv[2]);
				POIRiverList(out, argv[0], CBox::file(bbox.lat1, bbox.lng1), &bbox);
				}
			return TRUE;
			}
		Log(LOGERR, "Invalid Grid mode %s", param);
		return FALSE;
		}
	
	if (iopts[OOVERLAY])
		{
		//Log(LOGINFO, "Ground Overlay %s", param);
		OverlayList(out, argv[0], n[1], n[2], n[3]);
		return TRUE;
		}

	if (iopts[OMAPPER])
		{
		const char *mode = NULL;
		if (argv.length()>5)
			mode = argv[5];
		CanyonMapper cm(&out, param, mode, iopts, param, filelist);

		/*
		if (iopts[OSCANNER])
			{
			const char *idr = NULL;
			if (argv.length()>3 && (idr=strstr(argv[3],";R")))
				{
				// river scan
				CRiver *river = NULL;
				CRiverCache obj(LLE(n[1],n[2]));
				if (obj.rlist && (river=obj.rlist->Find(idr+2)))
					{
					Log(LOGINFO, "Mapping RIVER SCAN %s", param);
					cm.MapRiver(river, obj.rlist);
					return TRUE;
					}
				Log(LOGERR, "RiverFind error for %s", param);
				}
			else 
			if (argv.length()>3 && (idr=strstr(argv[3],";P")))
				{
				// river scan
				CRiver *river = NULL;
				CRiverCache obj(scanname(CBox::file(floor(n[1]), floor(n[2])), EXTRA));
				if (obj.rlist && (river=obj.rlist->Find(idr+2)))
					{
					Log(LOGINFO, "Mapping EXTRA SCAN %s", param);
					cm.MapRiver(river, obj.rlist, -1, NULL, 1);
					return TRUE;
					}
				//Log(LOGERR, "RiverFind error for %s", param);
				}

			// point scan
			Log(LOGINFO, "Mapping POINT SCAN %s", param);
			cm.Map(n[1], n[2], 0);
			return TRUE;
			}
		*/

		if (argv[1].indexOf(";")<0)  //if (argv[0]=="@")
			{
			// Map a specific point; Lat,Lng,Dist
			// "http://%s/rwr/?rwl=@,%s,%s,0/minelev,MC$"
			if (iopts[ORAIN])
				RainSimulation(n[1], n[2], &out, NULL);
			else
			if (iopts[OFLOWUP])
				LocateRogue(LLE(n[1], n[2]), &out);
			else
			if (iopts[OFLOWDOWN])
				WaterSimulation(n[1], n[2], &out, NULL);
			else 
				{
				Log(LOGINFO, "Mapping POINT %s", param);
				BOOL elevation = strchr(argv[3], 'E')!=NULL;
				float ang = (float)CGetNum(GetToken(argv[1], 1, 'A'));
				cm.Map(n[1], n[2], elevation ? 0 : n[3], elevation ? n[3] : 1, ang, GetToken(argv[2], 1, '@')); // maxdist or minelev
				return TRUE;
				}
			}

		else
			{
			// Map a specific point segment S;E;D  S/E=Lat;Lng;Dist
			// MkString("http://%s/rwr/?rwl=%s,R$", server, RiverSummary(sym))
			CRiver *river;
			CRiverCache obj(param, river);
			if (!river)
				{
				Log(LOGERR, "RiverFind error for %s", param);
				return FALSE;
				}
#if 0
			Log(LOGINFO, "Mapping RIVER SEGMENT %s", param);
			// expect user to do something about splits
			cm.MapRiver(river, obj.rlist);
#else
	      CSym split;		  
		  CRiver stub;
		  trklist trks;
		  for (int retry=0; river!=NULL && retry<=MAXSPLITS; ++retry)
		  {
			  Log(LOGINFO, "Mapping RIVER SEGMENT#%d %s", retry, param);
			  CanyonMapper cm(&out, param, mode, iopts, param, filelist);
			  BOOL splitted = cm.MapRiver(river, obj.rlist, -1, &split, retry, &trks);

			  // no splits
			  if (!splitted)
				  break;

			  // compute split too
			  stub.Set(&split, 0);
			  river = &stub;
		  }
#endif
			return TRUE;
			}

		return TRUE;
		}


	if (iopts[OAREA] || iopts[ORIVERS])	
		{
		// Map a whole region
		Log(LOGINFO, "Mapping to KML %s", param);
		//out += KMLName(name + " " + rmode);
	
		// only used for KML download
			
		vara list;
		CString GetRiverSRSUMmary(const char *summary, vara &list);
		GetRiverSRSUMmary(param, list);		

#ifdef DEBUGX // output river group
		out += KMLRect("bbox", "", bbox, RGB(0x00, 0x00, 0xFF));
#if 0
		for (int i=0; i<list.GetSize(); ++i)
			{
			CSym sym = list[i];
			vars coords(sym.GetStr(ITEM_COORD));
			coords.Replace(";", ",");
			vara list = coords.split(" ");
			CString desc = sym.GetStr(ITEM_SUMMARY);
			desc.Replace(" ",",");
			out +=  KMLLine(sym.id, desc, list, RGB(0,0,0xFF));
			}
#endif
#endif

		for (int i=0; i<list.length(); ++i)
			{
			vars link = list[i]+","+opt;
			vars name = ElevationFilename(link);
#if 0 // embed as network link
			out += KMLLink(name, NULL, MkString("http://%s/rwr/?rwz=%s,MC", server, link));
#else // embed as true data
			out += MkString("<Document id=\"%s\">", name);
			CanyonMap(link, out, TRUE, filelist);
			out += "</Document>";
#endif
			}

		return TRUE;
		}
	
	Log(LOGERR, "INVALID kmz=%s", param);
	return FALSE;
}


void ElevationProfile(TCHAR *argv[])
{

  // specific coordinates lat lng dist name
  if (argv!=NULL && argv[0]!=NULL)
	{
	vara param;
	for (int i=0; argv[i]!=NULL; ++i)
		{
		vars str = vars(argv[i]).trim();
		str.Replace(",", "");
		param.push(str);
		}
	CKMLOut out(ElevationFilename(param.join()));
	CanyonMap(param.join(), out, TRUE);
	return;
	}

  // as a group
  CFILE f;
  if (f.fopen("canyons.txt"))
	{
	const char *line;
	while (line=f.fgetstr())
	  {
	  if (strncmp(line, "--",2)==0)
		  break;
	  if (line[0]!='/' && line[0]!=';' && line[0]!=0)
		{
		CKMLOut out(ElevationFilename(line));
		CanyonMap(line, out, TRUE);
		}
	  }
	}

}


void ElevationLegend(const char *file)
{
	CanyonMapper::drawLegend(file);
}

int ElevationQuery(const char *params, CKMLOut &out, CStringArrayList &filelist)
{
int ret = 0;	
#ifndef DEBUG
	__try { 
#endif
  //Log(LOGINFO, "Elevationquery: %s", params);
  ret = CanyonMap(params, out, FALSE, &filelist);
  

#ifndef DEBUG
} __except(EXCEPTION_EXECUTE_HANDLER) 
	{ 
	Log(LOGALERT, "ELEVATION CRASHED!!! %.200s", params); 	
	}
#endif
return ret;
}


/*
int ElevationPoint(const char *params, CKMLOut &out)
{
	vara bbox = vars(params).split(",");
	double west = CGetNum(bbox[0]);
	double south = CGetNum(bbox[1]);
	double east = CGetNum(bbox[2]);
	double north = CGetNum(bbox[3]);

	double center_lng = ((east - west) / 2) + west;
	double center_lat = ((north - south) / 2) + south;

	out += MkString( 
	   "<Placemark>\n"
	   "<name>View-centered placemark</name>\n"
	   "<Point>\n"
	   "<coordinates>%.6f,%.6f</coordinates>\n"
	   "</Point>\n"
	   "</Placemark>\n", 
	   center_lng, center_lat);

	return TRUE;
}
*/

//
// CANYONS
//
/*
Rubio, 34.209, -118.115, 1000
Eaton, 34.199, -118.099, 2000
lsa, 34.181, -118.045
little santa anita, 34.184, -118.0468
supercloud, 34.253, -118.116
josephine, 34.298, -118.173
fox, 34.312, -118.179
suicide, 34.276, -118.222
imlay, 37.316, -112.964
heaps, 37.261, -112.976
mystery, 37.301, -112.933
pine creek, 37.213, -112.943
spry, 37.234, -112.955
keyhole, 37.226, -112.902
orderville, 37.333, -112.861
behunin, 37.271, -112.967
water canyon, 37.054, -112.967
hidden falls, 36.148, -115.511
ice cube, 36.131, -115.515
ice box, 36.136, -115.513
cerberus, 36.207, -116.726
hades, 36.228, -116.738
styx, 36.190, -116.719
coffin, 36.154, -116.747
jump, 36.903, -119.051

*/




static vars cleanup(const char *_str)
{
static const char *trim = " '\"";
static const char *stdsubst[] = { 
"\r", " ",
"\n", " ",
"\t", " ",
",", " ",
"  ", " ",
NULL
};

vars str(_str);
int i=0;
while (stdsubst[i]!=NULL)
	{
	str.Replace(stdsubst[i], stdsubst[i+1]);
	i+=2;
	}
return vars(str.Trim(trim));
}










static int Cachehits = 0;

/*

class DownloadCache
{
	double csize;
	CSymList list;
	CSection CC;

public:

	DownloadCache(void)
	{
	}

	~DownloadCache(void)
	{
	}

	CString CFile(const char *url)
	{
	CString file(url);
	// / ? < > \ : * | "
	file.Replace("/","(S)");
	file.Replace("?","(Q)");
	file.Replace("<","(L)");
	file.Replace(">","(G)");
	file.Replace("\\","(B)");
	file.Replace(":","(C)");
	file.Replace("*","(A)");
	file.Replace("|","(P)");
	file.Replace("\"","(M)");
	return file;
	}

	CString PathFile(const char *file)
	{
		char sub[] = "XXX\\";
		DWORD CRC = crc32(file, strlen(file)) & (CACHESUBNUM-1);
		CString pfile = MkString(CACHE"\\%0.4d\\", CRC) + file;
		return pfile;
	}

	void Initialize(void)
	{
		GetFileList(list, CACHE, NULL, TRUE);
		if (list.GetSize()==0)
			{
			CreateDirectory(CACHE, NULL);
			for (int i=0; i<CACHESUBNUM; ++i)
				{
				CString path = MkString(CACHE"\\%0.4d", i);
				CreateDirectory(path, NULL);
				}
			}

		list.Sort();
		Cachehits = 0;
		csize = 0;
		for (int i=0; i<list.GetSize(); ++i)
			{
			CFILE a;
			list[i].index = 0;
			CString t = list[i].id;
			list[i].id = GetFileNameExt(list[i].id);
			if (a.fopen(PathFile(list[i].id)))
				{
				int size = a.fsize();
				list[i].data = CData(size);
				csize += size;
				}
			}
	}

	void Purge(double size)
	{
		double t1 = list[0].index;
		double t2 = list[list.GetSize()/2].index;
		double t3 = list[list.GetSize()-1].index;
		list.SortIndex();
		for (int i=0; i<list.GetSize() && csize>size; ++i)
			{
			csize -= list[i].GetNum(0);
			DeleteFile(PathFile(list[i].id));
			list.Delete(i--);
			}
	}

	int DownloadUrl(DownloadFile &f, const char *url, int retry = 2, int delay = 1)
	{
	CString cfile = CFile(url);

	CC.Lock();
	if (list.GetSize()==0)
		Initialize();
	int n = list.Find(cfile);
	CC.Unlock();

	if (n>=0)
		{
		// cached
		list[n].index = GetTickCount();
		if (f.Load(PathFile(cfile)))
			{
			++Cachehits;
			return FALSE; // ok
			}
		}

	// download with restry
	for (int i=0; i<retry; ++i)
		{
		if (i>0) 
			Sleep(delay*1000);
		if (!f.Download(url, PathFile(cfile))) 
			{
			// got file
			CC.Lock();
			n = list.GetSize();
			list.Add(CSym(cfile, CData(f.size)));
			list[n].index = GetTickCount();
			csize += f.size;
			if (csize>MAXCACHE*1.10)
				Purge(MAXCACHE);
			list.Sort();
			CC.Unlock();
			return FALSE; // ok
			}
		}
    // error
	return -1;
	}



} Cache;
*/


int KMLSym(const char *name, const char *desc, const char *coordinates, LLRect &bb, CSym &sym)
{
			vars coords(coordinates);
			coords = coords.trim();
			coords.Replace(",",";");
			coords.Replace("\n"," ");
			coords.Replace("  "," ");
			vara clist = coords.split(" ");
			if (clist.length()<1)
				return FALSE;
			
			float dist = 0, plat = 0, plng = 0;
			for (int c=0; c<clist.length(); ++c)
				{
				vara num(clist[c], ";");
				float lat = InvalidNUM, lng = InvalidNUM;
				if (num.length()>=2)
					{
					lng = (float)CGetNum(num[0]);
					lat = (float)CGetNum(num[1]);
					}
				if (lat==InvalidNUM || lng==InvalidNUM)
					{
					//Log(LOGERR, "Invalid coords for %s", name);
					return -1;
					}
				if (c==0 || lat<bb.lat1)
					bb.lat1 = lat;
				if (c==0 || lat>bb.lat2)
					bb.lat2 = lat;
				if (c==0 || lng<bb.lng1)
					bb.lng1 = lng;
				if (c==0 || lng>bb.lng2)
					bb.lng2 = lng;
				if (c>0)
					dist += (float)Distance(lat,lng,plat,plng);
				plat = lat;
				plng = lng;
				}

			sym = CSym(cleanup(name), cleanup(desc));
			if (clist.length()>1)
				{
				// multiple coordinates (rivers)

				// expand bbox for rivers
				LLRect aux = bb;
				bb = LLDistance(aux.lat1, aux.lng1, MINRIVERJOIN);
				bb.Expand( LLDistance(aux.lat2, aux.lng2, MINRIVERJOIN) );

				sym.SetStr(ITEM_LAT, CCoord(bb.lat1));
				sym.SetStr(ITEM_LNG, CCoord(bb.lng1));
				sym.SetStr(ITEM_LAT2, CCoord(bb.lat2));
				sym.SetStr(ITEM_LNG2, CCoord(bb.lng2));
				sym.SetStr(ITEM_SUMMARY, CRiver::Summary(clist[0], clist[clist.length()-1], dist));
				sym.SetStr(ITEM_COORD, cleanup(coords));
				}
			else
				{
				// POIs
				sym.SetStr(ITEM_LAT, CCoord(bb.lat1));
				sym.SetStr(ITEM_LNG, CCoord(bb.lng1));
				}
			//sym.SetStr(ITEM_NUM, plist[i]);
			return TRUE;
}


#include "Xunzip.h"

#define ZIPFILE "KML.ZIP"


typedef int fdescription(const char *line, CSym &sym);

class DownloadKML {
	const char *msg;
	LLRect *bbox;
	int minlodzoom;
	int numthreads;
	CSection CC;
	int u, tu;
	vara urls;
	fdescription *cdescription;
	int error;
	LLRect gbox;
	CSymList *ptrlist;
public:

LLRect *GlobalBox()
{
	return &gbox;
}

static UINT DownloadThread(LPVOID pParam)
{
		DownloadKML *t = (DownloadKML *)pParam;
		t->DownloadNext();
		return 0;
}

void DownloadNext(void)
{

	{
	DownloadFile f(TRUE);

	int u;
	while ((u = Next())>=0)
		{

		CString &url = urls[u];
		if (strnicmp(url, "http", 4)==0)
		{
			// try to download twice
			if (DownloadUrl(f, url, 3))
				{
				Log(LOGERR, "Could not download #%d %s", u, url);
				++error;
				}
			else
				GetKML(u, f.memory, f.size);

		}
		else
		{			
			// try to load file
			if (!f.Load(url))
				{
				Log(LOGERR, "Could not load urlfile %s", url);
				++error;
				}
			else
				GetKML(u, f.memory, f.size);
		}


		CC.Lock();
		++tu;
		CC.Unlock();
		}
	}

	CC.Lock();
	--numthreads;
	CC.Unlock();
}

int Next(void)
{
	#define INVNEXT -2
	int nu = INVNEXT;
	while (nu==INVNEXT)
		{
		CC.Lock();
		if (u<urls.length())
			{
			nu = u++;
			fprintf(stdout, "%s KMZ Downloading %d/%d (%d%%) Cachehits:%d Threads:%d => %d items     \r", msg, nu, urls.length(), (nu*100)/urls.length(), Cachehits, numthreads, ptrlist->GetSize());
			}
		if (tu==urls.length())
			nu = -1;
		CC.Unlock();
		if (nu==INVNEXT)
			Sleep(100);
		}

	return nu;
}

DownloadKML(void)
{
	bbox = NULL;
	minlodzoom = 0;
	cdescription = NULL;
}

int Download(CSymList &list, const char *url, LLRect *bbox = NULL, int minlodzoom = 18, fdescription *cdescription = NULL, const char *msg = NULL)
{
	vara urllist;
	urllist.push(url);
	return Download(list, urllist, bbox, minlodzoom, cdescription, msg);
}


int Download(CSymList &list, vara &urllist, LLRect *bbox = NULL, int minlodzoom = 18, fdescription *cdescription = NULL, const char *msg = NULL)
{
	this->bbox = bbox;
	this->minlodzoom = minlodzoom;
	this->cdescription = cdescription;
	this->msg = msg;

	tu = u = 0;
	error = 0;
	Cachehits = 0;
	ptrlist = &list;
	for (int i=0; i<urllist.length(); ++i)
		urls.push( urllist[i] );

#if 0
	DownloadThread(this);
#else
	// start multithread ramping up if needed
	numthreads = 0;
	int maxthreads = 10;
	 
	do
		{
		if (numthreads<maxthreads && u<urls.length())
			{
			CC.Lock();
			++numthreads;
			CC.Unlock();
			AfxBeginThread( DownloadThread, this );
			}
		Sleep(1000);
		}
	while (numthreads>0);
#endif
	return error;
}


void GetKML(int ufrom, const char *memory, int size)
{
		// process networked kml
		if (strncmp(memory, "PK", 2)==0)
			{
			// kmz
			//HZIP hzip = OpenZip(ZIPFILE,0, ZIP_FILENAME);
			HZIP hzip = OpenZip((void *)memory, size, ZIP_MEMORY);
			if (!hzip)
				{
				Log(LOGERR, "can't unzip memory kmz");
				++error;
				return;
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
						GetKML(ufrom, memory, size);
						free(memory);
						}
				else
						{
						// some other file
						}
				}

			CloseZip(hzip);
			return;
			}

		int links =0, addedlinks = 0, placemarks = 0, placeerrors = 0, addedplacemarks = 0, urlslen = urls.length();

		vara ulist(memory, "<NetworkLink");
		for (int i=1; i<ulist.length(); ++i)
			{
			++links;
			vars region, nurl;
			GetSearchString(ulist[i], "", region, "<Region", "</Region>");
			if (region!="")
				{
				// check BB (if any)
				CString north, south, east, west, minlod;
				GetSearchString(region, "", north, "<north>", "</north>");
				GetSearchString(region, "", south, "<south>", "</south>");
				GetSearchString(region, "", west, "<west>", "</west>");
				GetSearchString(region, "", east, "<east>", "</east>");
				LLRect bb(CGetNum(north), CGetNum(west), CGetNum(south), CGetNum(east));
				if (bbox && !bb.IsNull() && !bbox->Intersects(bb))
					continue;

				GetSearchString(region, "", minlod, "<minLodPixels>", "</minLodPixels>");
				if (minlod!="")
					{
					if (bb.IsNull())
						{
						Log(LOGERR, "LOD of null BB %s", region);
						}
					else
						{
						// check LOD
						int x1, y1, x2, y2;
						LL2Pix(minlodzoom, bb.lat1, bb.lng1, x1, y1);
						LL2Pix(minlodzoom, bb.lat2, bb.lng2, x2, y2);
						int mlod = (int)CGetNum(minlod);
						int lod = max(abs(x2-x1), abs(y2-y1));
						if (lod<mlod)
							continue;
						}
					}
				}

			GetSearchString(ulist[i], "", nurl, "<href>", "</href>");
			if (nurl!="") 
				{
				nurl.Replace("&amp;", "&");
				if (strnicmp(nurl,"http",4)!=0)
					{
					vara url = urls[ufrom].split("/");
					url[url.length()-1] = nurl;
					nurl = url.join("/");
					}
				CC.Lock();
				//ASSERT(urls.length() != 2870 );
				//Log(LOGINFO, "Queuing #%d -> #%d %s", ufrom, urls.length(), ulist[i]);
				urls.push( nurl );
				CC.Unlock();
				++addedlinks;
				}
			}

		LLRect bb;
		placeerrors = 0;
		vara plist(memory, "<Placemark");
		for (int i=1; i<plist.length(); ++i)
			{
			vars &line = plist[i];
			vars name, desc, coords;
			GetSearchString(plist[i], "", name, "<name>", "</name>");
			GetSearchString(plist[i], "", desc, "<description>", "</description>");
			desc.Replace("www.", "http://www.");
			desc.Replace("http://http://","http://");
			desc.Replace("https://http://","https://");
			GetSearchString(plist[i], "", coords, "<coordinates>", "</coordinates>");


			CSym sym;
			int ret = KMLSym(name, desc, coords, bb, sym);
			if (ret<0) 
				++placeerrors;
			if (ret<=0) 
				continue;

			if (cdescription)
				if (!cdescription(plist[i], sym))
					continue; // skip

			++placemarks;
			if (bbox)
				if (!bbox->Intersects(bb))
					continue;

			CC.Lock();
			ptrlist->Add(sym);
			gbox.Expand(bb);
			CC.Unlock();
			++addedplacemarks;
		}
	
		if (placeerrors)
		  Log(LOGINFO, "#%d Links +%d/%d [%d-%d] Placemarks +%d/%d PlaceErrors %d", ufrom, addedlinks, links, urlslen, urls.length(), addedplacemarks, placemarks, placeerrors);
}

};


CSym FindBox(const char *region)
{
	if (region)
		{
		CSymList regions;
		regions.Load(filename("regions.csv"));
		for (int i=0; i<regions.GetSize(); ++i)
			if (stricmp(regions[i].id, region)==0)
				return regions[i];
		}

	return CSym("");
}


LLRect GetBox(CKMLOut &out, const char *region)
{
	LLRect bbox;
	CSym sym = FindBox(region);
	if (sym.id=="")
		{
		Log(LOGERR, "Invalid region %s", region);
		return bbox;
		}

	double n[] = { 0, 0, 0, 0 }; //San Jacinto
	for (int j=0; j<4; ++j)
		{
		n[j] = sym.GetNum(j);
		if (n[j]==InvalidNUM)
			{
			Log(LOGERR, "Invalid bounding box parameters %s", sym.Line());
			return bbox;
			}
		}

	bbox = LLRect(n[0], n[1], n[2], n[3]);
	CString desc = MkString("[%s - %s]", CCoord2(bbox.lat1, bbox.lng1), CCoord2(bbox.lat2, bbox.lng2));
	out+= KMLRect(region, desc, bbox, RGB(0x00, 0x00, 0xFF));
	return bbox;
}


//
// KML Output
//





CKMLOut& CKMLOut::operator+=(const CString &str)
	{
		if (!started) {
			started = TRUE;
			CString start = KMLStart(name, clink, extension);
			data->write(start);
			}
		data->write(str);
		return *this;
	}		

CKMLOut::CKMLOut(inetdata *data, const char *name, int nclink)
	{
	clink = extension = started = FALSE;
	this->data = data;
	this->deldata = FALSE;
	this->clink = nclink;
	this->name = name;
	}

CKMLOut::CKMLOut(const char *kmlfile)
	{
	clink = extension = started = FALSE;
	data = new inethandle(CreateFile(vars(kmlfile)+KML, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL));
	deldata = TRUE;
	name = kmlfile;
	}

CKMLOut::~CKMLOut(void)
	{
	*this += KMLEnd(clink);
	if (deldata) 
		delete ((inethandle *)data);
	}

	

int CKMLOut::Load(const char *kmlfile, transfunc *trans)
	{
	CFILE f;
	vars file(kmlfile);
	if (!GetFileExt(file))
		file += KML;
	if (f.fopen(file))
		{
		// header
		CString end = KMLEnd();
		static const char *match[] = { "<kml", "</kml", "<?xml", "<Document", "</Document", NULL };
		for(char *line  = f.fgetstr(); line!=NULL; line=f.fgetstr())
			{
			if (IsMatchSimilar(line, match, TRUE))
				continue;
			if (trans)
				trans(*this, line);
			else
				*this += line;
			*this += "\n";
			}
		return TRUE;
		}
	return FALSE;
	}



CString KMLStart(const char *name, int clink, int extension)
{
	CString out = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n";
	if (extension)
		out += "<kml xmlns=\"http://www.opengis.net/kml/2.2\" xmlns:gx=\"http://www.google.com/kml/ext/2.2\">\r\n";
	else
		out += "<kml xmlns=\"http://www.opengis.net/kml/2.2\">\r\n";
	
	if (!clink)
		if (name)
			out += MkString("<Document id=\"%s\">\n", name);
		else
			out += MkString("<Document>\n");
	if (name)
		out += KMLName(name);
	return out;
}

CString KMLName(const char *name, const char *desc, int snippet)
{
	CString out;
	if (name)
		{
		CString nname(name);
		nname.Replace("&amp;", "&");
		nname.Replace("&", "&amp;");
		nname.Replace("<", "&lt;");
		nname.Replace(">", "&gt;");
		ASSERT(strlen(nname)<1024);
		out += "<name>"; 
		out += nname;
		out += "</name>";
		}
	if (desc)
		{
		//ASSERT(strlen(desc)<3*1024);
		out += "<description>";
		vars d = desc;
		if (d.Replace("window.open('');\" onclick=\"window.open('", "window.open('"))
			d = d;
		//d.Replace("window.open('')","window.open('_blank')");
		//d.Replace("window.open('about:blank')","window.open('_blank')");
		out += d;
		out += "</description>";
		}
	if (snippet)
		if (desc && snippet>0)
			out += MkString("<Snippet maxLines=\"%d\">%s</Snippet>", snippet, desc);
		else
			out += "<Snippet maxLines=\"0\"></Snippet>";
	
	return out;
}

CString KMLEnd(int clink)
{
	CString out;
	if (!clink)
		out += "</Document>\n";
	out += "</kml>\n";
	return out;
}






CString KMLFolderStart(const char *name, const char *icon, int hidechildren, int visibility)
{
	CString out = MkString("<Folder><name>%s</name>\n", name);
	if (icon)
		{
		CString check; 
		if (hidechildren) check = "<listItemType>checkHideChildren</listItemType>";
		out+= MkString("<Style id=\"%s\"><ListStyle>%s<ItemIcon><href>http://sites.google.com/site/rwicons/%s.png</href></ItemIcon></ListStyle></Style><styleUrl>#%s</styleUrl>", icon, check, icon, icon);	
		if (!visibility)
			out += "<visibility>0</visibility>";
		}
	return out;
}

CString KMLFolderEnd(void)
{
	return "</Folder>\n";
}

CString KMLMarkerStyle(const char *name, const char *url, double scale, double textscale, int hotcenter, const char *extra)
{
   BOOL hidetext = textscale==0;
   CString defurl = "http://sites.google.com/site/rwicons/"+CString(name)+".png";
   if (!url) url = defurl;

   CString style;
   for (int i=0; i<=hidetext; ++i)
   {
   style += hidetext ? MkString("<Style id=\"%s%d\">", name, i) : MkString("<Style id=\"%s\">", name);
   style += MkString("<LabelStyle><scale>%g</scale></LabelStyle>", hidetext ? (!i ? 0.0 : 1.1) : textscale);
   style += MkString("<IconStyle><Icon><href>%s</href></Icon><scale>%g</scale>", url, scale);
   if (hotcenter)
	   style += MkString("<hotSpot x=\"%g\" y=\"%g\" xunits=\"fraction\" yunits=\"fraction\"/>", 0.5, 0.5);
   style += "</IconStyle>";
   if (extra)
	   style += extra;
   style += "</Style>\n";
   }
   // 2 styles
   if (hidetext)
      style += MkString("<StyleMap id=\"%s\"><Pair><key>normal</key><styleUrl>#%s0</styleUrl></Pair><Pair><key>highlight</key><styleUrl>#%s1</styleUrl></Pair></StyleMap>\n", name, name, name);
   return style;
}

CString KMLMarkerNC(const char *icon, double lat, double lng, const char *name, const char *desc = NULL)
{
	CString out = "<Placemark>";
	out += KMLName(name, desc);
	if (icon[0]=='<')
		out += icon; // custom style
	else
		out += MkString("<styleUrl>#%s</styleUrl>", icon);
  	out += MkString("<Point><coordinates>%s</coordinates></Point>",CCoord3(lat,lng));
	out += "</Placemark>\n";
	return out;
}

CString KMLMarker(const char *icon, double lat, double lng, const char *name, const char *desc)
{
	if (Savebox)
		if (!Savebox->Contains(lat, lng))
			return "";
	return KMLMarkerNC(icon, lat, lng, name, desc);
}


#define needsalpha(color)  ((((color)>>24)&0xFF) == 0)

CString KMLPoly(const char *name, const char *desc, vara &list, int color, int width, int areacolor)
{
	CString out = "<Placemark>";
	out += KMLName(name, desc);
	//<styleUrl>#%s</styleUrl>
	if (areacolor==-1)
		areacolor = 0x4f000000 | (color & 0xFFFFFF);
	if (needsalpha(color)) // no invisible
		color |= 0xFF000000; 
	out += MkString("<Style><PolyStyle><color>%8.8X</color></PolyStyle><LineStyle><color>%8.8X</color></LineStyle></Style>", areacolor, color);
	out += "<Polygon><tessellate>0</tessellate><outerBoundaryIs><LinearRing><coordinates>";
	out += list.join(" ");
	out += "</coordinates></LinearRing></outerBoundaryIs></Polygon></Placemark>\n";
	return out;
}


CString KMLBox(const char *name, const char *desc, LLRect &bbox, int color, int width, const char *extra, const char *extrastyle)
{
	vara list;
	list.push(CCoord3(bbox.lat1, bbox.lng1));
	list.push(CCoord3(bbox.lat1, bbox.lng2));
	list.push(CCoord3(bbox.lat2, bbox.lng2));
	list.push(CCoord3(bbox.lat2, bbox.lng1));
	list.push(CCoord3(bbox.lat1, bbox.lng1));

	return KMLLine(name, desc, list, color, width, extra, extrastyle);
}


CString KMLRect(const char *name, const char *desc, LLRect &box, int color, int width, int areacolor)
{
		vara plist; 
		plist.push(CCoord3(box.lat1,box.lng1));
		plist.push(CCoord3(box.lat1,box.lng2));
		plist.push(CCoord3(box.lat2,box.lng2));
		plist.push(CCoord3(box.lat2,box.lng1));
		plist.push(CCoord3(box.lat1,box.lng1));
		return KMLPoly(name, desc, plist, color, width, areacolor); //jet(res));
}

CString KMLLine(const char *name, const char *desc, vara &list, int color, int width, const char *extra, const char *extrastyle)
{
	CString out = "<Placemark>";
	out += "<Style>";
	if (extrastyle)
		out += extrastyle;
	out += "<LineStyle>";
	if (needsalpha(color)) // no invisible
		color |= 0xFF000000; 
	out += MkString("<color>%8.8X</color>", color);
	if (width>0)
		out += MkString("<width>%d</width>", width);
	out += "</LineStyle></Style>";
	if (extra)
		out += extra;
	out += KMLName(name, desc); /*<Snippet maxLines="0"></Snippet> */
	out += "<LineString><tessellate>0</tessellate><coordinates>";
	out += list.join(" ");
	out += "</coordinates></LineString></Placemark>\n";
	return out;
}


CString KMLLink(const char *name, const char *desc, const char *url)
{
	 return MkString("<NetworkLink>%s<open>0</open><Link><href>%s</href></Link></NetworkLink>", KMLName(name, desc), url); //<visibility>1</visibility>
}
/*
      <visibility>0</visibility>
      
      <description>A simple server-side script that generates a new random
        placemark on each call</description>
      <refreshVisibility>0</refreshVisibility>
      <flyToView>0</flyToView>
	   <Link><href>http://yourserver.com/cgi-bin/randomPlacemark.py</href></Link>
*/



//
// Panoramio
// 
const char *pictags[] ={ "waterfalls,waterfall,falls,water falling,water fall,cascada,cascadas", "slots,slot canyon,canyoneering,canyoning,rappels,rappel,abseil", "caves,cave,caving,cuevas,cueva,sotano", "mines,mine,minas,mina", "hotsprings,hotspring,hot springs,hot spring,hotspring", "ferratas,ferrata,zipline,suspension", NULL };
vara PANORAMIODIR( "PANOR,PANORTAG" );
const char *panoramios[] = {"WPA-Panoramio.com Waterfalls", "CPA-Panoramio.com Canyons", "VPA-Panoramio.com Caves", "MPA-Panoramio.com Mines", "HPA-Panoramio.com HotSprings", "FPA-Panoramio.com Ferratas", NULL };
vara FLICKRDIR( "FLICKR,FLICKRTAG" );
const char *flickrs[] = {"WFL-Flickr.com Waterfalls", "CFL-Flickr.com Canyons", "VFL-Flickr.com Caves", "MFL-Flickr.com Mines", "HFL-Flickr.com HotSprings", "FFL-Flickr.com Ferratas", NULL };

#if 0
void Throttle(double &last, int ms)
{
register double now = GetTickCount();
register double diff = now - last;
while (now < last + ms)
{
  if (diff>0 && diff<ms)
	Sleep((DWORD)(ms-diff));
  now = GetTickCount();
}
last = now;
}
#else
void Throttle(double &last, int ms)
{
	register double now = 0, sleep = 0;
	while (TRUE)
	{	   
	  now = GetTickCount();
	  sleep = last + ms - now;
	  if (sleep<=0)
		  break;
	  Sleep((DWORD)sleep);
	}
    last = now;
}

#endif



typedef int dirfunc(const char *file, CSymList &glist, LLRect *bbox, int usetags);

void DownloadDirUrl(int runmode, vara &DIR, dirfunc *getdir, const char *files[] = NULL)
{
	// update POI lists
   if (files)
    {
    CSymList csv;
	csv.header = vara(DIR).join(" ");
	for (int ver=0; pictags[ver]!=NULL; ++ver)
		{
		CString csvfile = filename(files[ver]);
		csv.Save(csvfile);
		CString file = csvfile;
		file.Truncate( file.GetLength() -3 );
		file += "dir";

		DeleteFile(file);
		MoveFile(csvfile, file);
		}
    }

   if (!Savebox)
		{
		Log(LOGERR, "Need -bbox to capture properly");
		return;
		}

	// update cached folder
	CBox cb(Savebox, POIGRIDSIZE);
   for (int usetags=1; usetags>=0; --usetags)
    {
	if (runmode==1 && usetags)
		continue;
	if (runmode==0 && !usetags)
		continue;
	for (LLRect *box =cb.Start(); box!=NULL; box=cb.Next())
		{
		CString file = filename(cb.file(), DIR[usetags] );
		if (INVESTIGATE>=0 || cb.DaysOld(file)>MAXDIRDAYS) // refresh every X days
			{
			CSymList list;
			int err = getdir(file, list, box, usetags);
			int size = list.GetSize();
			cb.Update(file, list, err, TRUE);
			
			// delete cached file
			//vars name = DIR.join(" ");
			//DeleteKMZ( GridName(name, POIOPT, box->lat1, box->lng1) ); 
			//Log(LOGINFO, "%s URL: [%s - %s] => %d->%d items", DIR[usetags], CCoord2(box->lat1, box->lng1), CCoord2(box->lat2, box->lng2), size, list.GetSize());	
			}
		}
   }

}





CString jdecode(const char *str)
{
	CString out;
	
	while (*str!=0)
		{
		if (*str=='&' && strnicmp(str, AMP, 5)!=0)
			{
			out += AMP, str+=1;
			continue;
			}
		if (*str=='\'')
			{
			out += "&#39;", str+=1;
			continue;
			}

		if (*str=='\\')
			{			
			switch (str[1])
				{
				case 't':
				case 'r':
				case 'n':
					out += " ", str+=2;
					break;
				case '\"':
					out += "&quot;", str+=2;
					break;
				case '\'':
					out += "&#39;", str+=2;
					break;
				case '\\':
					out += "\\", str+=2;
					break;					
				case 'u':
					{
					//\u0026 = &
					if (strncmp(str, "\\u0026", 6)==0)
						out += AMP;
					else
						out += "\xFF\xFF";
					//char chars[10];
					//wchar_t uc = from_hex(str[2])<<12 | from_hex(str[3])<<8 | from_hex(str[4])<<4 | from_hex(str[5])<<0;
					//int len = wcstombs(chars, &uc, 2);
					//if (len>0)
					str+=6;
					}
					break;
				default:
					Log(LOGERR, "invalid \\%c", str[1]);
					str+=2;
					break;
				}
			continue;
			}
		out += *str++;
		}

	return out;
}

int jmatch(const char *str1, const char *str2)
{
	int match = FALSE;
	while (*str1!=0 && str2!=0)
		{
		if (*str1>=128 || isspace(*str1))
			{
			++str1;
			continue;
			}
		if (*str2>=128 || isspace(*str2))
			{
			++str2;
			continue;
			}
		if (*str1!=*str2)
			return FALSE;
		match = TRUE;
		++str1, ++str2; 
		}
	return match && *str1==*str2;;
}

vars panoramiodesc(const char *line)
{
	return "";
}

static double panoramioticks = 0;

int PanoramioFix(CSym &sym)
{
		// find proper link (fix panoramio bug)
		DownloadFile f;

		if (INVESTIGATE>0)
			Log(LOGINFO, "Fixing:\n%s", SymInfo(sym));
		double d = 1e-3;
		double lat = sym.GetNum(ITEM_LAT);
		double lng = sym.GetNum(ITEM_LNG);
		CString url = MkString("http://www.panoramio.com/map/get_panoramas?set=full&from=0&to=400&size=thumbnail&mapfilter=false");
		url += MkString("&miny=%s&minx=%s&maxy=%s&maxx=%s", CCoord(lat-d), CCoord(lng-d), CCoord(lat+d), CCoord(lng+d));
		Throttle(panoramioticks, 1000);
		if (DownloadUrl(f, url))
			{
			Log(LOGERR, "Could not download %s", url);
			return FALSE;
			}

		int found = FALSE;
		//CString id = nospecialchars(
		vara plist(f.memory, "},");
		for (int i=1; i<plist.length() && !found; ++i)
			{
			CString id, desc;
			CString line = jdecode(plist[i]);
			GetSearchString(line, "\"photo_title\"", id, ":\"", "\"");
			GetSearchString(line, "\"photo_url\"", desc, ":\"", "\"");
			if (jmatch(ampfix(cleanup(id)), ampfix(sym.id)))
				{
				found = TRUE;
				sym.SetStr(ITEM_DESC, cdatajlink(cleanup(desc)));
				}
			}
		if (!found)
			return -1;

		return TRUE;
}


int PanoramioKML(const char *file, CSymList &glist, LLRect *bbox, CSymList *preload = NULL)
{
	int errors = 0;
	//"https://mw1.google.com/mw-earth-vectordb/dynamic/panoramio_clustered_delta/000144/root.kmz"
	//http://www.panoramio.com/kml/?all
	//http://www.panoramio.com/panoramio.kml?all&LANG=en_US.utf8&
	//DownloadKML dk(list, "http://www.panoramio.com/kml/?all", box, 15 );
	//"http://mw1.google.com/mw-earth-vectordb/dynamic/panoramio_clustered_delta/000144/002.kmz"
	CSymList klist;
	if (preload)
		{
		// preload
		klist.Add(*preload);
		}
	else
		{
		// download through kml
		DownloadKML dk;
		errors = dk.Download(klist, "http://www.panoramio.com/kml/?all", bbox, 15 );		
		}

	// update with old file to get descriptions
	CString tagfile(file); 
	tagfile.Replace(PANORAMIODIR[0], PANORAMIODIR[1]);
	if (CFILE::exist(tagfile))
		klist.Load(tagfile);
	if (CFILE::exist(file))
		klist.Load(file);
	for (int i=0; i<klist.GetSize(); ++i)
		{
		CSym &sym = klist[i];
		ampfix(sym.id);
		}
	// update to get descriptions
	UpdateList(klist, CSymList(), NULL, TRUE);

	// filter with tags
	for (int ver=0; panoramios[ver]!=NULL; ++ver)
		{
		vara tags(pictags[ver]);
		for (int i=0; i<klist.GetSize(); ++i)
			{
			CSym &sym = klist[i];
			if (!matchtags(sym.id, tags))
				continue; // skip

			sym.SetStr(ITEM_TAG, vars(panoramios[ver],1));
			glist.Add(sym);
			}
		}


	// fill up the description gaps
	int fixerror =0, unavail = 0, fixed = 0;
	for (int i=0; i<glist.GetSize(); ++i)
		{
		if (!glist[i].GetStr(ITEM_DESC).IsEmpty())
			continue;

		printf("Fixing %s (%d/%d)                  \r", glist[i].id, i, glist.GetSize());
		// empty description
		int ret = PanoramioFix(glist[i]);
		if (ret<0) ++unavail;
		if (ret==0) ++fixerror;
		if (ret>0) ++fixed;
		}

	if (fixed>0 || fixerror>0 || unavail>0)
		Log(LOGINFO, "Fixed %s %s: %d items, %d fixed, %d unavail, %d error", PANORAMIODIR[FALSE], file, glist.GetSize(), fixed, unavail, fixerror);
	return errors+fixerror;
}

int duptagpanoramio(int j, vars &tag, vara &tags)
{
	for (int i=0; i<tags.length(); ++i)
		if (i!=j)
			if (strcmp(tag, tags[i])==0)
				return TRUE;
	return FALSE;
}

#define PANORAMIOSTEP 400

static LLRect *obox;

int PanoramioAPI(int oneshot, CSymList &list, LLRect *bbox, const char *msg, const char *tag = NULL, const char *tgc = NULL)
{
			int errors = 0;
			DownloadFile f;

			CString set = "full";
			if (tag)
				set = CString("tag-")+tag;

			int more = TRUE;
			int step = PANORAMIOSTEP;
			int n = 0;
			while (more)
				{				
				//http://www.panoramio.com/map/get_panoramas?order=popularity&set=tag-waterfall&size=thumbnail&from=0&to=24&minx=-180&miny=-76.87933531085397&maxx=180&maxy=84.28533579318314&mapfilter=false
				CString url = MkString("http://www.panoramio.com/map/get_panoramas?set=%s&from=%d&to=%d&size=thumbnail", set, n, n+step);
				if (bbox) url += MkString("&miny=%s&minx=%s&maxy=%s&maxx=%s", CCoord(bbox->lat1), CCoord(bbox->lng1), CCoord(bbox->lat2), CCoord(bbox->lng2));
				url += "&mapfilter=true";
				Throttle(panoramioticks, 1000);
				if (DownloadUrl(f, url))
					{
					Log(LOGERR, "Could not download %s", url);
					return -1;
					}

				CString hasmore, count;
				GetSearchString(f.memory, "\"has_more\":", hasmore, "", ",");
				more = stricmp(hasmore, "true")==0;
				if (!more)
					more = more;
				GetSearchString(f.memory, "\"count\":", count, "", ",");
				int nnum = (int)CGetNum(count);

				if (!msg) 
					return nnum;

				double percent = 0;
				if (obox)
					{
					ASSERT(obox->lat2+1e-5>=bbox->lat2 && obox->lng2+1e-5>=bbox->lng2 && obox->lat1-1e-5<=bbox->lat1 && obox->lng1-1e-5<=bbox->lng1);
					percent = CBox::Progress(*bbox, *obox);
					}
				printf("Processing %s : %dlist %d%% %d/%d             \r", msg, list.GetSize(), (int)(percent*100), n, nnum);

				vara plist(f.memory, "},");
				for (int i=1; i<plist.length(); ++i, ++n)
					{
					CString id, desc, lat, lng;
					GetSearchString(plist[i], "\"latitude\"", lat, ":", ",");
					GetSearchString(plist[i], "\"longitude\"", lng, ":", ",");
					double vlat = CGetNum(lat);
					double vlng = CGetNum(lng);
					if (vlat==InvalidNUM || vlng==InvalidNUM)
						continue;
					//if (!obox->Contains(vlat, vlng))
					//	continue;
					GetSearchString(plist[i], "\"photo_title\"", id, ":", ",");		
					GetSearchString(plist[i], "\"photo_url\"", desc, ":", ",");
					CSym sym(ampfix(cleanup(id)), cdatajlink(cleanup(desc)));
					sym.SetStr(ITEM_LAT, CCoord(vlat));
					sym.SetStr(ITEM_LNG, CCoord(vlng));
					if (tgc!=NULL)
						sym.SetStr(ITEM_TAG, tgc);
					list.Add(sym);
					}		
				if (oneshot) 
					return n;
				}

	return n;
}


int PanoramioSCAN(double *steps, CSymList &list, LLRect *box, const char *msg, const char *tag = NULL, const char *tgc = NULL, int i = 0)
{
		int oneshot = steps[i] != 0;
		if (i==0)
			{
			int ret = PanoramioAPI(FALSE, list, box, msg, tag, tgc);
			if (ret<0) return TRUE; // err
			if (ret<PANORAMIOSTEP) return FALSE; // enough
			}
		if (oneshot)		   
			{
			// scan high level
			++i;
			CIntArrayList digdown;
			int n, error = 0;
			LLRect *bbox;
			CBox cb(box, steps[i-1]); 
			BOOL oneshot = steps[i]!=0;
			for (bbox = cb.Start(), n=0; bbox!=NULL && list.GetSize()<MAXPANORAMIOQUERY; bbox = cb.Next(), ++n) {
				int ret = PanoramioAPI(FALSE, list, bbox, MkString("%s %d%%Z%d ", msg, (int)(cb.Progress()*100), i), tag, tgc);
				if (ret<0) ++error;
				digdown.AddTail(ret>PANORAMIOSTEP);
				}
			if (oneshot)
			  for (bbox = cb.Start(), n=0; bbox!=NULL && list.GetSize()<MAXPANORAMIOQUERY; bbox = cb.Next(), ++n)
				if (digdown[n])
					error += PanoramioSCAN(steps, list, bbox, MkString("%s %d%%Z%d+ ", msg, (int)(cb.Progress()*100), i), tag, tgc, i);
			return error;
			}
	   return FALSE;
}

int PanoramioURL(const char *file, CSymList &glist, LLRect *box, int usetags)
{
	int error = 0;
	obox = box;

	if (usetags)
		{
		// skip empty cells
		CSymList xlist;
		if (!PanoramioAPI(TRUE, xlist, box, NULL))
			return 0;

		// get tags
		for (int ver=0; pictags[ver]!=NULL; ++ver)
			{
			vara tags(pictags[ver]);
			for (int t=0; t<tags.length(); ++t)
				{
				vars tag = tags[t], tgc = vars(panoramios[ver],1);
				/*
				tag.Replace(" ","");
				if (duptagpanoramio(t, tag, tags)) // avoid duplicate queries
					continue;
				*/
				double steps[] = { 1, 0 };
				error += PanoramioSCAN(steps, glist, box, MkString("%s %s", PANORAMIODIR[usetags], tag), tag, tgc);
				}
			}
		}
	else
		{
		// no tags
		CSymList list;
		if (FLOATVAL<0)
			{
			// KML shit
			error += PanoramioKML(file, list, box);
			}
		else
			{
			// API
			/*
			int i = 0;
			CBox cb(box, step);
			for (LLRect *bbox = cb.Start(); bbox!=NULL; bbox = cb.Next(), ++i);
			int num = i; i = 0;
			for (LLRect *bbox = cb.Start(); bbox!=NULL; bbox = cb.Next(), ++i)
			*/
			double steps[] = { 1, 0.2, 0 };
			error += PanoramioSCAN(steps, list, box, PANORAMIODIR[usetags]);
			}

		for (int ver=0; panoramios[ver]!=NULL; ++ver)
			{
			vara tags(pictags[ver]);
			for (int i=0; i<list.GetSize(); ++i)
				{
				CSym &sym = list[i];
				if (!matchtags(sym.id, tags))
					continue; // skip

				sym.SetStr(ITEM_TAG, vars(panoramios[ver],1));
				glist.Add(sym);
				}
			}
		Log(LOGINFO, "%s: %d total, %d tagmatch, %d err", file, list.GetSize(), glist.GetSize(), error);
		}

	return error;
}



void DownloadPanoramioUrl(int runmode)
{
	DownloadDirUrl(runmode, PANORAMIODIR, PanoramioURL, panoramios);
}



// 
// FlickR
//



static double flickrticks = 0;

int duptagflickr(int j, const char *tag, vara &tags)
{
	for (int i=0; i<tags.length(); ++i)
		if (i!=j)
			if (strstr(tag, tags[i]))
				return TRUE;
	return FALSE;
}


int FlickrAPI(CSymList &list, LLRect *bbox, const char *msg, const char *tag = NULL, const char *tgc = NULL, vara *textsearch = FALSE)
{

		//double lngstep = bbox->lng2-bbox->lng1, latstep = bbox->lat2-bbox->lat1;
			DownloadFile f;
			int errors = 0;
			int more = TRUE;
			int step = 400;
			int n = 0, p=1;

			CString set;
			if (tag) 
				{
				if (textsearch)
					{
					//if (duptagflickr(textsearch->indexOf(tag), tag, *textsearch))
					//	return 0;
					set = MkString("&text=%s&sort=relevance", tag);
					}
				else
					set = MkString("&tags=%s", tag);
				}

			while (more)
				{				
				//tags[t].Replace(" ","");
				
				CString url = MkString("https://api.flickr.com/services/rest/?method=flickr.photos.search&api_key=", FLICKR_API_KEY, "&extras=geo&page=%d&per_page=%d", p, step);

				if (set!="") url += set;
				if (bbox) url += MkString("&bbox=%s,%s,%s,%s", CCoord(bbox->lng1), CCoord(bbox->lat1), CCoord(bbox->lng2), CCoord(bbox->lat2));

				// force 1 download/sec (<100,000 limit per day)
				Throttle(flickrticks, 1000);
				if (DownloadUrl(f, url))
					{
					++errors;
					Log(LOGERR, "Could not download %s", url);
					continue;
					}

				CString info, pages, num;
				GetSearchString(f.memory, "<photos ", info, "", ">");
				GetSearchString(info, "pages=", pages, "\"", "\"");
				GetSearchString(info, "total=", num, "\"", "\"");
				double npages = CGetNum(pages), nnum = CGetNum(num);
				if (npages==InvalidNUM || nnum==InvalidNUM)
					{
					Log(LOGERR, "Invalid Flickr response %s", info);
					++errors;
					break;
					}
				
				// cap "mine" results
				if (textsearch && n>MAXFLICKRQUERY)
				  if (strcmp(tag,"mine")==0)
					{
					Log(LOGWARN, "Truncating Text Search %s at %d (%d > %d)", tag, n, (int)nnum, MAXFLICKRQUERY);
					break;
					}

				printf("Processing %s %d/%d             \r", msg, n, (int)nnum);
				more = ++p <= npages;

				vara plist(f.memory, "<photo ");
				for (int i=1; i<plist.length(); ++i, ++n)
					{
					CString id;
					GetSearchString(plist[i], "title=", id, "\"", "\"");
					if (textsearch && !matchtags(id, *textsearch))
						continue; // skip

					CString pid, pown;
					GetSearchString(plist[i], "id=", pid, "\"", "\"");
					GetSearchString(plist[i], "owner=", pown, "\"", "\"");
					if (pid.IsEmpty() || pown.IsEmpty())
						{
						Log(LOGERR, "Invalid Flickr photo %s", plist[i]);
						continue;
						}
					CString lat, lng;
					GetSearchString(plist[i], "latitude=", lat, "\"", "\"");
					GetSearchString(plist[i], "longitude=", lng, "\"", "\"");
					double vlat = CGetNum(lat);
					double vlng = CGetNum(lng);
					if (vlat==InvalidNUM || vlng==InvalidNUM)
						continue;
					//if (!obox->Contains(vlat, vlng))
					//	continue;

					// description
					CString link = MkString("https://www.flickr.com/photos/%s/%s", pown, pid);
					CString desc = cdatajlink(link);

					/*
					GetSearchString(plist[i], ">", desc, "<description>", "</description>");
					desc.Replace("\n", " ");
					desc.Replace("\t", " ");
					desc.Replace("  ", " ");
					*/
					CSym sym(cleanup(id), desc);
					sym.SetStr(ITEM_LAT, CCoord(vlat));
					sym.SetStr(ITEM_LNG, CCoord(vlng));
					if (tgc!=NULL)
						sym.SetStr(ITEM_TAG, tgc);
					list.Add(sym);
					}		
				}

return errors;
}


int FlickrURL(const char *file, CSymList &glist, LLRect *bbox, int usetags)
{
	int error = 0;

	if (usetags)
		{
 		int i = 0;
		for (int ver=0; pictags[ver]!=NULL; ++ver, ++i);
		int num = i; i = 0;
		for (int ver=0; pictags[ver]!=NULL; ++ver, ++i)
			error += FlickrAPI(glist, bbox, MkString("%s %d: %s %d%% %d/%d", FLICKRDIR[usetags], glist.GetSize(), vars(flickrs[ver],1), (i*100)/num, i, num), pictags[ver], vars(flickrs[ver],1));
		}
	else
		{
		// no tags
		CSymList list;

		if (FLOATVAL<0)
			{
			// Alternate method
			error += FlickrAPI(list, bbox, MkString("%s %d: ", FLICKRDIR[usetags], list.GetSize()));
			}
		else
			{
			int i=0;
			for (int ver=0; pictags[ver]!=NULL; ++ver)
				{
				vara tags(pictags[ver]);
				for (int t=0; t<tags.length(); ++t, ++i);
				}
			int num = i; i=0;
			for (int ver=0; pictags[ver]!=NULL; ++ver)
				{
				vara tags(pictags[ver]);
				for (int t=0; t<tags.length(); ++t, ++i)
					error += FlickrAPI(glist, bbox, MkString("%s %d: %s %d%% %d/%d", FLICKRDIR[usetags], glist.GetSize(), tags[t], (i*100)/num, i, num), tags[t], vars(flickrs[ver],1), &tags);
				}
			}


		for (int ver=0; panoramios[ver]!=NULL; ++ver)
			{
			vara tags(pictags[ver]);
			for (int i=0; i<list.GetSize(); ++i)
				{
				CSym &sym = list[i];
				if (!matchtags(sym.id, tags))
					continue; // skip

				sym.SetStr(ITEM_TAG, vars(panoramios[ver],1));
				glist.Add(sym);
				}
			}
		if (FLOATVAL>0)
			Log(LOGINFO, "%s: %d total, %d tagmatch, %d err", file, list.GetSize(), glist.GetSize(), error);
		}

	return error;
}



void DownloadFlickrUrl(int runmode)
{
	DownloadDirUrl(runmode, FLICKRDIR, FlickrURL, flickrs);
}



//
// Custom POI Download
//
float dmstodeg(double dd, double mm, double ss)
{
	float deg = (float)(abs(dd) + ((abs(mm) + (abs(ss) / 60.0)) / 60.0));
	return (dd<0) ? -deg : deg;
}

float dmstodeg(const char *str, char spc)
{
	int sig = 0;
	while (isspace(*str) || *str==':' || *str==';' || *str==',') ++str;
	if (toupper(*str)=='E') ++str, sig=1;
	if (toupper(*str)=='W') ++str, sig=-1;
	if (toupper(*str)=='N') ++str, sig=1;
	if (toupper(*str)=='S') ++str, sig=-1;
	while (isspace(*str) || *str==':' || *str==';' || *str==',') ++str;

	vars aux(str), auxspc = CString(spc, 1); 
	aux.Replace( "\t", auxspc);
	aux.Replace( "\n", auxspc);
	aux.Replace( "\r", auxspc);
	aux.Replace( CString(spc, 2), auxspc);	
	vara list( aux, auxspc );

	double dd[3] = { 0, 0, 0};
	for (int i=0; i<list.length() && i<3; ++i)
		{
		dd[i] = CGetNum( list[i] );
		if (dd[i]==InvalidNUM)
			dd[i] = 0;
		}
	if (dd[0]<0) sig = -1; 
	double deg = ( abs(dd[0]) + ((abs(dd[1]) + (abs(dd[2]) / 60.0)) / 60.0) );
	if (deg == 0 || deg<-180 || deg>180) 
		return InvalidNUM;

	for (int i=0; str[i]!=0 && !isalpha(str[i]); ++i);
	char last = str[i];
	
	if (!sig && toupper(last)=='W') sig = -1;
	if (!sig && toupper(last)=='S') sig = -1;
	if (!sig && toupper(last)=='N') sig = 1;
	if (!sig && toupper(last)=='E') sig = 1;
	if (!sig)
		{
		sig = 1; // should never happen
		Log(LOGERR, "Improper coordinates dmstodeg(%s)", str);
		return InvalidNUM;
		}
	return (float)(sig * deg);
}

BOOL DownloadOregonWaterfalls(CSymList &ilist)
{
	vara regions;
	DownloadFile f;

	// download base
	{
	CString url = "http://www.oregonwaterfalls.net/alphabet.htm";
	if (DownloadUrl(f, url))
		{
		Log(LOGERR, "Can't download oregonwaterfalls from %s", url);
		return FALSE;
		}
	vara list(f.memory, "HREF=");
	for (int i=1; i<list.length(); ++i)
		{
		CString id;
		GetSearchString(list[i], "", id, "\"", "\"");
		if (id != "" && regions.indexOf(id)<0)
			regions.push(id);
		}
	}

	for (int r=0; r<regions.length(); ++r)
		{
		CString url = "http://www.oregonwaterfalls.net/"+regions[r];
		if (DownloadUrl(f, url))
			{
			Log(LOGERR, "Can't download oregonwaterfalls from %s", url);
			continue;
			}
		vars title;
		GetSearchString(f.memory, "<H1", title, ">", "</H1");

		// multiple GPS in separate lines?
		int gps = 0;
		vara list;

		{
		vara list2(f.memory, ">GPS");
		for (int i2=1; i2<list2.length(); ++i2) 
			{
			vars line;
			GetSearchString(list2[i2], "", line, "", "<");
			list.push("GPS"+line);
			}
		}
		
		{
		vara list2(f.memory, "N4");
		for (int i2=1; i2<list2.length(); ++i2) 
			{
			vars line;
			GetSearchString(list2[i2], "", line, "", "<");
			list.push("GPS N4"+line);
			}
		}

		for (int i=0; i<list.length(); ++i)
		{
			//if (list[i][0]!='.')
			{
			vars coords = GetTokenRest(list[i], 1, ' ');
			vara pcoords( coords );
			if (pcoords.length()<2)
				{
					//Log(LOGERR, "no oregonwaterfalls coords for %s", regions[r]);
					continue;
				}		
			double lat = dmstodeg(pcoords[0]);
			double lng = dmstodeg(pcoords[1]); if (lng>0) lng = -lng;
			if (lat==InvalidNUM || lng==InvalidNUM || lat<40 || lat>=50 || lng>-100 || lng<-130)
				{
					Log(LOGERR, "invalid oregonwaterfalls coords %s", coords);
					continue;
				}		
			CSym sym(htmlcleanup(title), cdatajlink(url));
			sym.SetStr(ITEM_LAT, CCoord(lat));			
			sym.SetStr(ITEM_LNG, CCoord(lng));
			ilist.Add(sym);
			++gps;
			}
		}
		if (strstr(f.memory, ">GPS")!=NULL && !gps)
			Log(LOGERR, "no oregonwaterfalls GPS coords for %s", regions[r]);
		}

	return TRUE;
}

BOOL DownloadGeology(CSymList &ilist)
{
	vara regions;
	DownloadFile f;

	// download base
	{
	CString url = "http://geology.com/waterfalls/";
	if (DownloadUrl(f, url))
		{
		Log(LOGERR, "Can't download Geology from %s", url);
		return FALSE;
		}
	vara list(f.memory, "http://geology.com/waterfalls/");
	for (int i=1; i<list.length(); ++i)
		{
		CString id;
		GetSearchString(list[i], "", id, "", ".");
		if (id != "" && regions.indexOf(id)<0)
			regions.push(id);
		}
	}

	for (int r=0; r<regions.length(); ++r)
		{
		CString url = "http://geology.com/waterfalls/"+regions[r]+".xml";
		if (DownloadUrl(f, url))
			{
			Log(LOGERR, "Can't download Geology from %s", url);
			continue;
			}
		vara list(f.memory, "<marker ");
		for (int i=1; i<list.length(); ++i)
			{
			vars slat, slng, title;
			GetSearchString(list[i], "lat=", slat, "\"", "\"");
			GetSearchString(list[i], "lng=", slng, "\"", "\"");
			GetSearchString(list[i], "title=", title, "\"", "\"");
			double lat = CGetNum(slat);
			double lng = CGetNum(slng);
			if (lat==InvalidNUM || lng==InvalidNUM)
				{
					Log(LOGERR, "invalid Geology coords %s,%s", lat, lng);
					continue;
				}		
			CSym sym(cleanup(title), cdatajlink("http://geology.com/waterfalls/"));
			sym.SetStr(ITEM_LAT, CCoord(lat));			
			sym.SetStr(ITEM_LNG, CCoord(lng));
			ilist.Add(sym);
			}
		}

	return TRUE;
}


BOOL DownloadWaterfallsWest(CSymList &ilist)
{
	vara regions;
	DownloadFile f;

	// download base
	{
	CString url = "http://www.waterfallswest.com/page.php?id=region";
	if (DownloadUrl(f, url))
		{
		Log(LOGERR, "Can't download WaterfallsWest from %s", url);
		return FALSE;
		}
	vara list(f.memory, "?id=region");
	for (int i=1; i<list.length(); ++i)
		{
		CString id;
		GetSearchString(list[i], "", id, "", "\"");
		if (id != "" && regions.indexOf(id)<0)
			regions.push(id);
		}
	}

	for (int r=0; r<regions.length(); ++r)
		{
		CString url = "http://www.waterfallswest.com/page.php?id=region"+regions[r];
		if (DownloadUrl(f, url))
			{
			Log(LOGERR, "Can't download WaterfallsWest from %s", url);
			continue;
			}
		vara list(f.memory, "var pt = new google.maps.LatLng(");
		for (int i=1; i<list.length(); ++i)
			{
			vars coords, info;
			GetSearchString(list[i], "", coords, "", ")");
			GetSearchString(list[i], "createmarker(", info, "", ");");
			vara pinfo = info.split();
			if (pinfo.length()<5)
				{
					Log(LOGERR, "invalid waterfallwest line %s", list[i]);
					continue;
				}

			double lat = CGetNum(GetToken(coords, 0));
			double lng = CGetNum(GetToken(coords, 1));
			if (lat==InvalidNUM || lng==InvalidNUM)
				{
					Log(LOGERR, "invalid waterfallwest coords %s", coords);
					continue;
				}		
			CSym sym(cleanup(pinfo[3]), CDATAS+cleanup(pinfo[4])+"<br>"+cleanup(pinfo[5])+CDATAE);
			sym.SetStr(ITEM_LAT, CCoord(lat));			
			sym.SetStr(ITEM_LNG, CCoord(lng));
			ilist.Add(sym);
			}
		}

	return TRUE;
}

BOOL DownloadOregonCaves4U(CSymList &ilist)
{
	vara regions;
	DownloadFile f;

	// download base
	CString url = "http://www.oregoncaves4u.com/";
	if (DownloadUrl(f, url))
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
		if (DownloadUrl(f, nurl))
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
		CSym sym(cleanhref(vals[0]),CDATAS+vals.join("<br>")+CDATAE);
		sym.SetNum(ITEM_LAT, lat);
		sym.SetNum(ITEM_LNG, lng);
		ilist.Add(sym);
		}

	return TRUE;
}


const struct { const char *file; BOOL (*function)(CSymList &ilist); } download[] = {
	{ "V01-OregonCaves4U.com", DownloadOregonCaves4U },
	{ "W03-WaterfallsWest.com", DownloadWaterfallsWest },
	{ "W06-Geology.com", DownloadGeology },
	{ "W08-OregonWaterfalls.net", DownloadOregonWaterfalls },
	NULL };

/*
void FixSoakOregon(CSymList &list)
{
	const char *http = "http://soakoregon.com/";
	for (int i=0; i<list.GetSize(); ++i)
		{
		CString link;
		GetSearchString(list[i].data, http, link, NULL, ')');
		if (link.IsEmpty())
			continue;

		// replace
		link = http + link;
		CString newlink = jlink(link, link);
		list[i].data.Replace(link, newlink);
		}
}
*/

int GetLink(const char *desc, const char *pattern, CString &link, char a, char b)
{
		link = "";
		const CString http = "http";
		GetSearchString(desc, pattern+http, link, a, b);
		if (link.IsEmpty())
			return FALSE;
		link = http+link;
		return TRUE;
}

int FixJLink(CSymList &list)
{

	int changes = 0;
	for (int j=0; j<list.GetSize(); ++j)
		{
		CSym &sym = list[j];
		CString desc = sym.GetStr(ITEM_DESC);
		CString link;
		// promote to CDATA
		if (strncmp(desc, CDATAS, strlen(CDATAS))!=0)
		  {
		  const char *html[]= {"http", "<", ">", "&", NULL };
		  for (int h=0; html[h]!=NULL; ++h)
		   if (strstr(desc, html[h])!=NULL)
			{
			desc = CDATAS+desc+CDATAE;
			sym.SetStr(ITEM_DESC, desc);
			++changes;
			break;
			}
		  }
		for (int l=1; l<desc.GetLength(); ++l)
			{
			if (strncmp(((const char *)desc)+l, "http", 4)==0)
				if (isalpha(desc[l-1]))
					{
					desc.Insert(l, " ");
					sym.SetStr(ITEM_DESC, desc);
					++changes;
					}
			}

		vars test = "http://hp.com";
		if (jref(test)==test)
			continue;

		// inline url
		const char *prefix[] = { CDATAS, "<br>", "(", " ", ":", "\xC2\xA0", NULL };
		for (int p=0; prefix[p]!=NULL; ++p)
		  while (GetLink(desc, prefix[p], link, 0, 0))
			{
			int len = 0;
			while (!isspace(link[len]) && link[len]!='<' && link[len]!=')' && link[len]!=0 && strcmp(((const char *)link)+len,CDATAE)!=0)
				++len;
			//ASSERT( sym.id!="Oregon Coast Waterfalls");
			link = CString(link, len);
			int cdataelen = strlen(CDATAE);
			if (link.Right(cdataelen)==CDATAE)
				link.Truncate(link.GetLength() - cdataelen);
			desc.Replace(prefix[p]+link, prefix[p]+jlink(link));
			sym.SetStr(ITEM_DESC, desc);
			++changes;
			}
		// a href
		const char *href[] = { "<a href=\"", "<A HREF=\"", NULL };
		for (int p=0; href[p]!=NULL; ++p)
		  while (GetLink(desc, href[p], link, 0, '\"'))
			{
			desc.Replace(href[p]+link+"\"", href[p]+jref(link)+"\"");
			sym.SetStr(ITEM_DESC, desc);
			++changes;
			}

		// integrity check
		desc.Replace("<img src=\"http", "");
		desc.Replace("<IMG SRC=\"http", "");
		desc.Replace("window.open('http", "");
		desc.Replace(");\">http", "");
		if (strstr(desc, "http")!=NULL)
			Log(LOGERR, "Invalid JLink Fix: %s", desc);
		}
	return changes;
}


void DownloadPOI(void)
{
	// Download special
	for (int i=0; download[i].file!=NULL; ++i)
		{
		CSymList ilist;
		if (!download[i].function(ilist))
			{
			Log(LOGERR, "DownloadPOI failed for %s", download[i].file);
			continue;
			}
		FixJLink(ilist);
		UpdateList(filename(download[i].file), ilist);
		}

	// Download KML
	CSymList filelist;
	const char *exts[] = { "KML", NULL };
	GetFileList(filelist, "POI\\KML", exts, FALSE);
	for (int i=0; i<filelist.GetSize(); ++i)
		{
		CSymList newlist;
		Log(LOGINFO, "Processing %s", GetFileNameExt(filelist[i].id));
		DownloadKML dk;
		if (dk.Download(newlist, filelist[i].id)>0)
			Log(LOGERR, "Error downloading %s", GetFileNameExt(filelist[i].id));

		// fix links and descriptions
		//if (strstr(filelist[i].id, "SoakOregon.com"))
		//	FixSoakOregon(newlist);

		CString file = filename(GetFileName(filelist[i].id));
		FixJLink(newlist);
		UpdateList(file, newlist);
		}
}






//
// Rivers
//

/*
http://arcgis-owarcgis-1022610217.us-east-1.elb.amazonaws.com/arcgis/services/OWOther_NP21/WATERS_KMZ_NP21/MapServer/KmlServer?LayerIDs=1,0&transparent=true&bbox=-116.8256474053638,33.73809082140065,-116.7555422994169,33.77733105000227&size=1544,917&

http://arcgis-owarcgis-1022610217.us-east-1.elb.amazonaws.com/arcgis/services/OWOther_NP21/WATERS_KMZ_NP21/MapServer/KmlServer?LayerIDs=1,0&transparent=true&bbox=-117.0,33.0,-116.5,33.5&size=1544,917&

http://arcgis-owarcgis-1022610217.us-east-1.elb.amazonaws.com/arcgis/services/OWOther_NP21/WATERS_KMZ_NP21/MapServer/KmlServer?LayerIDs=1,0&transparent=true&bbox=-117.8256474053638,33.30809082140065,-116.0255422994169,33.93733105000227&size=3044,3000&
http://arcgis-owarcgis-1022610217.us-east-1.elb.amazonaws.com/arcgis/services/OWOther_NP21/WATERS_KMZ_NP21/MapServer/KmlServer?LayerIDs=1,0&transparent=true&bbox=-117.0099,33.3030,-116.0266,33.9388&size=1544,1500&
*/

int SkipNumId(CString &id)
{
	for (int i=0, n=id.GetLength(); i<n && (isdigit(id[i]) || isspace(id[i])); ++i);
	if (i>0)
		{
		id.Delete(0, i);
		return TRUE;
		}
	return FALSE;
}

int kmlusrivers(const char *line, CSym &sym)
{
	vars desc, ul;
	GetSearchString(line, "", desc, "<description>", "</description>");
	GetSearchString(desc, "", ul, "<ul", "</ul>");
	vars out(CDATAS);
	out += cleanup("<ul"+ul+"</ul>");
	out += CDATAE;
	sym.SetStr(ITEM_DESC, out);
	SkipNumId(sym.id);
	return TRUE;
}


int symusrivers(CSym &sym)
{
		if (INVESTIGATE<0 && !sym.GetStr(ITEM_WATERSHED).IsEmpty())
			return 0;

		CString url = WatershedURL(sym);
		if (url.IsEmpty())
			return 0;
		
		url += "&optOutFormat=JSON";

		DownloadFile f;
		if (DownloadUrl(f, url))
			{
			if (INVESTIGATE>0)
				Log(LOGERR, "Could not download river watershed from %s", url);
			return -1;
			}

		vars data(f.memory);
		vara str; str.SetSize(10); //Order, Level, CFS1, CFS2, Catchment, Watershed;
		GetSearchString(data, "\"streamorder\"", str[WSORDER], ":", ",");
		GetSearchString(data, "\"streamlevel\"", str[WSLEVEL], ":", ",");
		GetSearchString(data, "\"Q0001E\"", str[WSCFS1], ":", ",");
		GetSearchString(data, "\"V0001E\"", str[WSCFS2], ":", ",");
		//GetSearchString(data, "velocity (UROM)", FPS1, ":", "\n");
		//GetSearchString(data, "velocity (Vogel)", FPS2, ":", "\n");
		// drainage
		GetSearchString(data, "\"areasqkm\"", str[WSAREA1], ":", ",");
		GetSearchString(data, "\"cumdrainag\"", str[WSAREA2], ":", ",");
		// temperature
		GetSearchString(data, "\"tempv\"", str[WSTEMP1], ":", ",");
		GetSearchString(data, "\"cumtempvc\"", str[WSTEMP2], ":", ",");
		// temperature
		GetSearchString(data, "\"precipv\"", str[WSPREP1], ":", ",");
		GetSearchString(data, "\"cumprecipvc\"", str[WSPREP2], ":", ",");
		
		for (int s=0; s<str.length(); ++s)
			str[s] = vars(CData(CGetNum(str[s])));
		sym.SetStr(ITEM_WATERSHED, str.join(";"));
		return TRUE;
}


	  

int GetSymElev(CSym &sym, tcoord *coord)
{
		int change = 0;
		if (INVESTIGATE>=1)
			{
			LLRect bb;
			CSym tmpsym;
			KMLSym("name", "desc", sym.GetStr(ITEM_COORD), bb, tmpsym);
			CString summary = tmpsym.GetStr(ITEM_SUMMARY);
			sym.SetStr(ITEM_SUMMARY, summary);
			change = TRUE;
			}
		vara summaryp( sym.GetStr(ITEM_SUMMARY), " ");
		if (summaryp.length()<3)
			{
			Log(LOGERR, "Invalid summary %s", summaryp.join(" "));
			return -1;
			}

		for (int i=0; i<2; ++i)
			{
			LLE lle;
			if (!RiverLL(summaryp[i], lle))
				{
				Log(LOGERR, "Invalid summary %s", summaryp.join(" "));
				return -1;
				}
			coord[i].lat = lle.lat, coord[i].lng = lle.lng, coord[i].elev = lle.elev;
			if (coord[i].elev<=0 || coord[i].elev>MAXELEV || INVESTIGATE>=0)
				{
				double elev = coord[i].elev;
				DEMARRAY.height(&coord[i]);
				//CRASHDEMARRAY(&coord[i]);
				if (INVESTIGATE>0)
					if (elev>0 && elev!=coord[i].elev)
						Log(LOGWARN, "New height for %s %f != %f", CCoord2(coord[i].lat, coord[i].lng), elev, coord[i].elev);
				++change;
				}
			}

		if (change==0)
			return 0;
		
		// set new summary
		for (int i=0; i<2; ++i)
			{
			vara scoord(summaryp[i],";");
			scoord.SetAtGrow(2, vars(MkString("%.3f", coord[i].elev)));
			summaryp[i] = scoord.join(";");
			}
		/*
		// swap coordinates if needed
		if (coord[0].elev < coord[1].elev)
			{
			vars x = summaryp[0];
			summaryp[0] = summaryp[1];
			summaryp[1] = x;
			}		
		*/
		sym.SetStr(ITEM_SUMMARY, CRiver::Summary(summaryp[0], summaryp[1], summaryp[2]));
		return 1;
}


int GetRiverElev(CSymList &list, const char *file)
{

	// get river elev
	int changes = 0, notsorted = 0, badelev = 0, errors = 0;
	for (int i=0; i<list.GetSize(); ++i)
		{
		tcoord coord[2];
		int ret = GetSymElev(list[i], coord );
		if (ret<0) ++errors;
		if (ret==0) continue;

		++changes;
		if (coord[0].elev == coord[1].elev)
			{
			if (INVESTIGATE>0)
				{
				double elev = coord[1].elev;
				DEMARRAY.height(&coord[1]);
				DEMARRAY.height(&coord[0]);
				if (coord[0].elev != coord[1].elev || coord[0].elev != elev)
					Log(LOGERR, "Inconsitent height for %s & %s, %f != %f != %f", CCoord2(coord[0].lat, coord[0].lng), CCoord2(coord[1].lat, coord[1].lng), elev, coord[0].elev, coord[1].elev);
				}
			++notsorted;
			if (coord[0].elev<0)
				++badelev;
			}
		}
	if (notsorted) 
		Log(LOGWARN, "%s : could not sort %d rivers %s", file, notsorted, badelev>0 ? MkString("(%d bad elev)", badelev) : "");
	if (errors)
		Log(LOGERR, "%s : %d INVALID river summaries", file, errors);
	return changes;
}




void IntReplace(CIntArrayList &list, int src, int dst)
{
	for (int j=0; j<list.length(); ++j)
		if (list[j]==src)
			list[j]=dst;
}

int SaveKeepTimestamp(const char *file, CSymList &list)
{
		FILETIME time;
		BOOL ok = CFILE::gettime(file, &time);
		list.Save(file);
		if (ok)
			ok = CFILE::settime(file, &time);
		if (!ok)
			Log(LOGERR, "ERROR: Could not preserve date for %s", file);
		return ok;
}



void DownloadRiversUS(int mode, const char *folder)
{
	if (!Savebox)
		{
		Log(LOGERR, "Rivers needs -bbox to capture properly");
		return;
		}
	CBox cb(Savebox);
	for (LLRect *bbox = cb.Start(); bbox!=NULL; bbox = cb.Next())
	  {
	  CString file = MkString("%s\\%s" CSV, folder, cb.file());  //filename(cb.file(), RIVERSUS);
	  if (cb.DaysOld(file)>MAXRIVERSUSDAYS || INVESTIGATE>=0)
		{
		vara urls;
		CSymList list;
		//LLRect safetybox(*bbox);
		//safetybox.Expand(0.5, 0.5);
		CBox cb2(bbox, 0.5);
		for (LLRect *bbox2 = cb2.Start(); bbox2!=NULL; bbox2 = cb2.Next())
			{
			CString url = "http://arcgis-owarcgis-1022610217.us-east-1.elb.amazonaws.com/arcgis/services/OWOther_NP21/WATERS_KMZ_NP21/MapServer/KmlServer?LayerIDs=1,0&transparent=true";
			url += MkString("&bbox=%6.6lf,%6.6lf,%6.6lf,%6.6lf", bbox2->lng1, bbox2->lat1, bbox2->lng2, bbox2->lat2);
			urls.push(url);
			}
		
		if (mode!=1)
			{
			DownloadKML dk;
			int error = dk.Download(list, urls, bbox, 20, kmlusrivers, MkString("%d%% ", (int)(100*cb.Progress(cb2.Progress()))));
			if (error>0) 
				continue;
			// merge duplicates
			UpdateList(list);
			}
		else
			{
			list.Load(file);
			if (list.GetSize()==0)
				continue;
			}

		if (mode!=0)
			{
			// download watershed
			printf("Downloading Watershed for %s                \r", file);
			CSymListThread proc(list, symusrivers, MAXTHREADRIVERS);
			if (proc.errors>0) 
				Log(LOGERR, "Watershed for %s: %d errors", file, proc.errors);
			//error = ;
			}
		
		GetRiverElev(list, file);
		list.Save(file); // new file
		}
	  }
}



void DownloadRiversUSUrl(int mode, const char *folder)
{
	CSymList filelist;
	const char *exts[] = { "CSV", NULL };
	GetFileList(filelist, folder, exts, FALSE);

	// sort by oldest to newer
	for (int i=0; i<filelist.GetSize(); ++i)
		{
		FILETIME time;
		CFILE::gettime(filelist[i].id, &time);
		filelist[i].index = COleDateTime(time);
		if (INVESTIGATE<=0)
			filelist[i].index *= -1;
		}
	filelist.SortIndex();

	for (int i=0; i<filelist.GetSize(); ++i)
	    {
	    CString file = filelist[i].id;
		const char *filename = GetFileNameExt(file);
		CSymList list; list.Load(file);
		printf("Processing %d%% (%d/%d) %s            \r", (i*100)/filelist.GetSize(), i, filelist.GetSize(), filename);

		CSymListThread proc(list, symusrivers, MAXTHREADRIVERS);
		if (proc.changes>0 || proc.errors>0)
			Log(LOGINFO, "%s: fixed %d/%d updated watershed%s", filename, proc.changes, list.GetSize(), proc.errors>0 ? MkString(", %d errors", proc.errors) : "");
		if (proc.changes>0)
			SaveKeepTimestamp(file, list);
		}
#if 0
	if (errors>=0)
		{
		// Update but 
		FILETIME time;
		BOOL ok = CFILE::gettime(*file, &time);
		list.Save(*file);
		if (errors>0)
			SetFileError(*file);
		else
			{
			if (ok)
				ok = CFILE::settime(*file, &time);
			if (!ok)
				Log(LOGERR, "Error getting/setting file time for %s", *file);
			}
		
		Log(LOGINFO, "Watershed updated %s: %d items %s", *file, list.GetSize(), errors>0 ? MkString(", %d errors", errors) : "");
		}
#endif

}



void DownloadRiversKML(const char *poifolder, const char *kmlfolder, fdescription riverskml)
{
	// Download KML
	CSymList filelist;
	const char *exts[] = { "KML", "KMZ", NULL };
	GetFileList(filelist, kmlfolder, exts, TRUE);
	for (int i=0, n=filelist.GetSize(); i<n; ++i)
		{
		CSymList newlist;
		CString file = filelist[i].id;
		Log(LOGINFO, "KML Processing: %d%% (%d/%d) %s", (i*100)/n, i, n, GetFileNameExt(file));
		DownloadKML dk;
		if (dk.Download(newlist, file, 0, 21, riverskml)>0)
			{
			Log(LOGERR, "Error downloading %s", GetFileNameExt(file));
			continue;
			}
		
		// cache bbox		
		typedef struct { CSym *sym; LLRect bbox; } sb;
		CArrayList<sb> sblist(newlist.GetSize());
		for (int i=0; i<newlist.GetSize(); ++i)
			{
			CSym &sym = newlist[i];
			sblist[i].sym = &sym;
			sblist[i].bbox = LLRect(sym.GetNum(ITEM_LAT), sym.GetNum(ITEM_LNG), sym.GetNum(ITEM_LAT2), sym.GetNum(ITEM_LNG2));
			}

		if (dk.GlobalBox()->IsNull())
			continue;

		// update impacted files
		CBox cb(dk.GlobalBox());
		for (LLRect *box =cb.Start(); box!=NULL; box=cb.Next())
			{
				CSymList list;
				for (int i=0; i<sblist.GetSize(); ++i)
					if (box->Intersects(sblist[i].bbox))
						list.Add(*sblist[i].sym);
				CString file = MkString("%s\\%s%s",poifolder, cb.file(), CSV);
				GetRiverElev(list, file);
				UpdateList(file, list);
			}
		}
}


// Load GML
BOOL DownloadGML(CSymList &list, const char *file, gmlread gml, LLRect &gbbox)
{
		CFILE f;
		if (!f.fopen(file))
			{
			Log(LOGERR, "ERROR: can't open file %s", file);
			return FALSE;
			}

		CString item;
		const char *line;
		while (line=f.fgetstr())
			{
			if (strstr(line, "<gml:featureMember>"))
				{
				item.Empty();
				continue;
				}
			if (strstr(line, "</gml:featureMember>"))
				{
				CSym sym;
				LLRect bbox;
				if (gml(item, sym, bbox))
					{
					gbbox.Expand(bbox);
					list.Add(sym);
					}
				item.Empty();
				continue;
				}
			item += line;
			}
		return TRUE;
}

void DownloadRiversGML(const char *poifolder, const char *gmlfolder, gmlread read, gmlfix fix)
{
	// Download KML
	CSymList filelist;
	const char *exts[] = { "GML", NULL };
	GetFileList(filelist, gmlfolder, exts, TRUE);
	for (int i=0, n=filelist.GetSize(); i<n; ++i)
		{
		CSymList newlist;
		CString file = filelist[i].id;
		Log(LOGINFO, "GML Processing: %d%% (%d/%d) %s", (i*100)/n, i, n, GetFileNameExt(file));

		LLRect gbb;
		if (!DownloadGML(newlist, file, read, gbb))
			{
			Log(LOGERR, "Error downloading %s", GetFileNameExt(filelist[i].id));
			continue;
			}

		if (fix!=NULL)
			fix(file, newlist);

		// cache bbox		
		CRiverList rlist(newlist);
		for (int i=0; i<rlist.GetSize(); ++i)
			gbb.Expand( rlist[i].Box() );

		if (gbb.IsNull())
			continue;

		// update impacted files
		CBox cb(&gbb);
		for (LLRect *box =cb.Start(); box!=NULL; box=cb.Next())
			{
				CSymList list;
				for (int i=0; i<rlist.GetSize(); ++i)
					if (box->Intersects(rlist[i].Box()))
						list.Add(*rlist[i].sym);
				CString file = MkString("%s\\%s%s",poifolder, cb.file(), CSV);
				GetRiverElev(list, file);
				UpdateList(file, list);
			}
		}
}

static int cmpse(const char *SE1, const char *SE2)
{
	return strcmp(SE1, SE2)==0;
}


CString TranSRSUMmary(const char *sum, const char *osum)
	{
		vara oSE[2], SE[2];
		
		vara osump(osum, " ");
		oSE[0] = vara(osump[0],";");
		oSE[1] = vara(osump[1],";");

		vara sump(sum, " ");
		SE[0] = vara(sump[0], ";"); 
		SE[1] = vara(sump[1], ";");

		for (int j=0; j<2; ++j)
			if (cmpse(SE[0][0],oSE[j][0]) && cmpse(SE[0][1],oSE[j][1]) && cmpse(SE[1][0],oSE[!j][0]) && cmpse(SE[1][1],oSE[!j][1]))
				{
				if (oSE[j].length()>2)
					SE[0].SetAtGrow(2, oSE[j][2]);
				if (oSE[!j].length()>2)
					SE[1].SetAtGrow(2, oSE[!j][2]);
				sump[0] = SE[0].join(";");
				sump[1] = SE[1].join(";");
				return sump.join(" ");
				}

		Log(LOGERR, "Could not match summary %s --> %s", osum, sum);
		return CString(sum);
	}


void TransRivers(const char *folder, const char *outfolder, int mode)
{
	/*
	   CSymList list;
	   CString file = "RIVERSCA\\GML\\n49w124.csv";
		list.Load(file);
	   gmlcanadafix(file, list);
	*/
	
	CSymList filelist;
	const char *exts[] = { "CSV", NULL };
	GetFileList(filelist, folder, exts, FALSE);

	for (int i=0; i<filelist.GetSize(); ++i)
		{
		CSymList list;
		CString file = filelist[i].id;
		list.Load(file);
		printf("Processing %s      \r", filelist[i].id);
		for (int l=0; l<list.GetSize(); ++l)
			{
			LLRect bb;
			tcoord coord[2];
			CSym sym, &osym = list[l]; 
			osym.SetStr(ITEM_CONN,"");
			switch (mode)
				{
				case 0:
					enum { OLD_COORD = 5, OLD_SUMMARY, OLD_WATERSHED, OLD_TAG, OLD_SUMMARYELEV };
					#define OLD_SUMMARY 
					if (!KMLSym(osym.id, osym.GetStr(ITEM_DESC), osym.GetStr(OLD_COORD), bb, sym))
						{
						Log(LOGERR, "Invalid old sym %s", osym.Line());
						continue;
						}
					sym.SetStr(ITEM_WATERSHED, osym.GetStr(OLD_WATERSHED));
					CString osum = osym.GetStr(OLD_SUMMARYELEV);
					sym.SetStr(ITEM_SUMMARY, TranSRSUMmary(sym.GetStr(ITEM_SUMMARY), osum));
					//GetSymElev(sym, coord);
					osym = sym;
					break;
				}
			}
		list.Save(MkString("%s\\%s", outfolder, GetFileNameExt(file)));
		}
	
}

int gmlmexico(const char *data, CSym &sym, LLRect &bb)
{
		//if (!IsSimilar(data, "<gml2:hd_1470009_1>"))
		//	return FALSE;

		/*
		CString tipo;
		GetSearchString(data, "<ogr:TIPO>", tipo, "", "<");
		if (tipo=="102") // avoid CANAL
			return FALSE;
		*/

		CString line, coords;
		GetSearchString(data, "", line, "<gml:LineString", "</gml:LineString>");
		GetSearchString(line, "<gml:coordinates>", coords, "", "<");
		// line string
		if (line.IsEmpty() || coords.IsEmpty())
			return FALSE;

		CString order, level;
		GetSearchString(data, "<ogr:ORDER_1>", order, "", "<");
		GetSearchString(data, "<ogr:LEVEL_1>", level, "", "<");
		if (level=="-1")
			return FALSE; // avoid meanders
		
		CString desc;
		const char *attr[] = { "ENTIDAD", "CONDICION", NULL };
		for (int i=0; attr[i]!=NULL; ++i)
			{
			CString val;
			GetSearchString(data, MkString("<ogr:%s>", attr[i]), val, "", "<");
			if (!val.IsEmpty())
				desc += MkString("%s: %s<br>", attr[i], val);
			}
		
		CString title, description = CString(CDATAS) + desc + CString(CDATAE);
		//GetSearchString(data, "<gml2:nameen>", title, "", "<");

		int ret = KMLSym(title, description, coords, bb, sym);
		if (ret<0) 
			Log(LOGERR, "ERROR: Invalid GML coords %s", coords);
		if (ret<=0) 
			return FALSE;
		sym.SetStr(ITEM_WATERSHED, order+';'+level);
		return TRUE;
}


int gmlmexicopt(const char *data, CSym &sym, LLRect &bb)
{
		//if (!IsSimilar(data, "<gml2:hd_1470009_1>"))
		//	return FALSE;

		CString line, coords;
		GetSearchString(data, "", line, "<gml:Point>", "</gml:Point>");
		GetSearchString(line, "<gml:coordinates>", coords, "", "<");
			// line string
		if (line.IsEmpty() || coords.IsEmpty())
			return FALSE;

		CString clase;
		const char match[] = "ELEMENTOS HIDROGR";
		GetSearchString(data, "<ogr:CLASE>", clase, "", "<");
		if (strncmp(clase, match, strlen(match))!=0)
			return FALSE;

		CString description;
		CString type, title;
        GetSearchString(data, "<ogr:CLASE>", description, "", "<");
		GetSearchString(data, "<ogr:NOMBRE_COM>", title, "", "<");
		
		int ret = KMLSym(title, description, coords, bb, sym);
		if (ret<0) 
			Log(LOGERR, "ERROR: Invalid GML coords %s", coords);
		if (ret<=0) 
			return FALSE;
		return TRUE;
}

typedef struct { tcoord coord; const char *id; } tpt;

int cmpcoordelev(tpt *A, tpt *B)
{
	if (A->coord.elev > B->coord.elev) return 1;
	if (A->coord.elev < B->coord.elev) return -1;
	return 0;
}

int gmlmexicofix(const char *file, CSymList &list)
{
	CString ptfile = CString(file, strlen(file)-3)+"FIX";

	LLRect gbbox;
	CSymList ptlist;
	if (!DownloadGML(ptlist, ptfile, gmlmexicopt, gbbox))
		{
		Log(LOGERR, "Could not load fix file %s", ptfile);
		return FALSE;
		}

	// process 

	// sort points by height
	CArrayList<tpt> plist(ptlist.GetSize());
	for (int i=0; i<plist.length(); ++i)
		{
		CSym &pt = ptlist[i];
		double lat = pt.GetNum(ITEM_LAT);
		double lng = pt.GetNum(ITEM_LNG);
		plist[i].coord = tcoord(lat, lng);
		DEMARRAY.height(&plist[i].coord);
		plist[i].id = pt.id;
		}

	plist.Sort(cmpcoordelev);

	// find points on rivers
	CRiverList rlist(list);
	CArrayList<CRiver *> fixlist;
	for (int i=0; i<plist.GetSize(); ++i)
		{
		tpt &pt = plist[i];
		CRiver *r = rlist.FindPoint(pt.coord.lat, pt.coord.lng, 100);
		if (!r) 
			continue;

		r->sym->id = pt.id;
		r->fixed = TRUE;
		fixlist.AddTail(r);
		}

	 // now match names
	 for (int i=0; i<fixlist.GetSize(); ++i)
		{
		CRiver *rf = fixlist[i];
		CString &id = rf->sym->id;
		double level = rf->Level();
		for (int n=0; n<2; ++n)
		  {
		  LLE lle;
		  if (n>0) 
			  lle = rf->SE[1];
		  else
			  lle = rf->SE[0]; 
		  for (int i=0; i<rlist.GetSize(); ++i)
			{
			CRiver &r = rlist[i];
			if (r.Level()!=level)
				continue;
			if (r.fixed)
				continue;
			// find min distance
			BOOL s = LLE::Distance(&lle, &r.SE[0])<MINRIVERJOIN;
			BOOL e = LLE::Distance(&lle, &r.SE[1])<MINRIVERJOIN;
			if (!s && !e)
				continue;
			if (r.sym->id == id)
				continue;
			//ASSERT( i!= 448);
			if (!r.sym->id.IsEmpty())
				{
				//Log(LOGWARN, "Conflicting fix name %s != %s  %s - %s", r.sym->id, id, CCoord2(r.slat, r.slng), CCoord2(r.elat, r.elng));
				if (r.sym->GetStr(ITEM_WATERSHED) != rf->sym->GetStr(ITEM_WATERSHED))
					continue;
				//Log(LOGWARN, "Overriding fix name %s -> %s  %s - %s", r.sym->id, id, CCoord2(r.slat, r.slng), CCoord2(r.elat, r.elng));
				}

			// found it!
			r.sym->id = id;
			if (s) lle = r.SE[1];
			if (e) lle = r.SE[0]; 
			i=-1;
			}
		  }
		}

	// load fixes
	list.Empty();
	for (int i=0; i<rlist.GetSize(); ++i)
		list.Add(*rlist[i].sym);

	return TRUE;
}







class SEG 
{
public:
	DWORD id;
	LLRect bbox;
	double length;
	CSym *sym, symdata;
	//CRiver *river;
	short int order, minor;
	LLEC *lle;


/*
		//ASSERT(slist.GetSize()>0);
		//if (slist.GetSize()==0)
		//	return FALSE;
		//if (!lle)
		//	return FALSE;
		//slist.Sort(segcmp);
		CRiverListPtr oldlist(slist);
		//ASSERT(slist.GetSize()<4);
		for (int r=0; r<slist.GetSize(); )
		  {
		  int joined = FALSE;
		  for (int i=0; i<slist.GetSize(); ++i)
		   if (slist[i])
			{
			if (!slist[i]->sym)
				return FALSE;
			CSym &sym = *slist[i]->sym;

			// first 
			if (res.data.IsEmpty())
				{
				res = sym;
				}
			else
				{
				// merge
				CString watershed = res.GetStr(ITEM_WATERSHED);
				if (watershed=="")
					sym.GetStr(ITEM_WATERSHED);
				CString coords = CoordMerge(res.GetStr(ITEM_COORD), sym.GetStr(ITEM_COORD));
				if (coords.IsEmpty())
					continue;
				KMLSym(res.id, res.GetStr(ITEM_DESC), coords, bbox, res);
				res.SetStr(ITEM_WATERSHED, watershed);
				}

			slist[i]=NULL;
			joined = TRUE;
			++r;
			}
		  if (!joined)
				{
				Log(LOGERR, "ERROR: Invalid %d SEG join\n%s", oldlist.GetSize(), res.GetStr(ITEM_COORD));
				for (int i=0; i<oldlist.length(); ++i)
					{
					CSym *sym  = oldlist[i]->sym;
					vara coords(sym->GetStr(ITEM_COORD), " ");
					vara files;
					CBox cb(&bbox);
					for (LLRect *box =cb.Start(); box!=NULL; box=cb.Next())
						files.push(cb.file());
					Log(LOGINFO,"#%d %s %d: %s - %s [%d@%s]", i, slist[i] ? "x" : "Y", oldlist[i]->pri, coords[0], coords[coords.length()-1], oldlist[i]->num, files.join() );
					slist[i] = oldlist[i];
					}
				res.data.Empty();
				// go debug!
				return FALSE;
				}
		  }
*/


	int MergeCoord(CSym *sym, CSym *osym)
	{
			// summary
			vara osump(osym->GetStr(ITEM_SUMMARY), " ");
			//oSE[0] = vara(osump[0],";");
			//oSE[1] = vara(osump[1],";");

			vara sump(sym->GetStr(ITEM_SUMMARY), " ");
			//SE[0] = vara(sump[0], ";"); 
			//SE[1] = vara(sump[1], ";");



			// coords
			vara rc = vars(sym->GetStr(ITEM_COORD)).split(" ");
			vara orc = vars(osym->GetStr(ITEM_COORD)).split(" ");
			int rcl = rc.length()-1, orcl = orc.length()-1;
			if (rcl<=0 || orcl<=0)
				{
				Log(LOGERR, "Malformed Merge Coords of \n%s\n%s", sym->Line(), osym->Line());
				return FALSE;
				}
			vara coords;
			/*
			if ( rmatch == 3729 || i == 3729)
				{
				Log(LOGINFO, "Debug #%d vs #%d in %s:\n%s\n%s", i, rmatch, file, rc.join(" "), orc.join(" "));
				}
			*/
			int m = 0;
			double d = 1e10, mind = 1e10;

			LLEC rc0, rc1, orc0, orc1;
			if (!RiverLL(rc[0], rc0) || !RiverLL(rc[rcl], rc1) || !RiverLL(orc[0], orc0) || !RiverLL(orc[orcl], orc1))
				{
				Log(LOGERR, "Invalid merge coords");
				return FALSE;
				}


			if ((d=LLEC::Distance(&rc0, &orc1))<=mind)
				mind = d, m = 1;
			if ((d=LLEC::Distance(&rc1, &orc0))<=mind)
				mind = d, m = 2;
			if ((d=LLEC::Distance(&rc0, &orc0))<=mind)
				mind = d, m = 3;
			if ((d=LLEC::Distance(&rc1, &orc1))<=mind)
				mind = d, m = 4;

			const char *S, *E;

			switch(m)
			{
			case 1:
				{
				S = osump[0], E = sump[1];
				coords = orc;
				for (int i=1; i<=rcl; ++i)
					coords.push(rc[i]);
				break;
				}
			case 2:
				{
				S = sump[0], E = osump[1];
				coords = rc;
				for (int i=1; i<=orcl; ++i)
					coords.push(orc[i]);
				}
				break;
			case 3:
				{
				S = sump[1], E = osump[1];
				for (int i=rcl; i>=0; --i)
					coords.push(rc[i]);
				for (int i=1; i<=orcl; ++i)
					coords.push(orc[i]);
				}
				break;
			case 4:
				{
				S = sump[0], E = osump[0];
				coords = rc;
				for (int i=orcl-1; i>=0; --i)
					coords.push(orc[i]);
				}
				break;
			default:
				Log(LOGALERT, "Inconsistent merge");
				return FALSE;
			}

		// check
		if (strncmp(S, coords[0], GetTokenPos(S,2,';')-1)!=0 || strncmp(E, coords[coords.length()-1], GetTokenPos(E,2,';')-1)!=0)
			{
			Log(LOGALERT, "Inconsistent merge S:%s!=%s  E:%s!=%s", S, coords[0], E, coords[coords.length()-1]);
			return FALSE;
			}
		if (coords.length()<=1)
			{
			Log(LOGALERT, "Inconsistent merge result Coords: %s", coords.join(" "));
			return FALSE;
			}
		// merge summary
		sym->SetStr(ITEM_SUMMARY, CRiver::Summary(S, E, length));
		sym->SetStr(ITEM_COORD, coords.join(" "));
		return TRUE;
	}


	int Merge(SEG *oriver)
	{
			bbox.Expand( oriver->bbox );
			length += oriver->length;
			// copy sym data
			if (sym != &symdata)
				{
				symdata = *sym; 
				sym = &symdata;
				}
			// preserve watershed
			if (sym->GetStr(ITEM_WATERSHED).GetLength() < oriver->sym->GetStr(ITEM_WATERSHED).GetLength())
				sym->SetStr(ITEM_WATERSHED, oriver->sym->GetStr(ITEM_WATERSHED));
			// merge coords
			return MergeCoord(sym, oriver->sym); 
	}

	int Sym(CSym &res)
	{
		// rebuild sym

		LLRect bbox;
		KMLSym(sym->id, sym->GetStr(ITEM_DESC), sym->GetStr(ITEM_COORD), bbox, res);
		res.SetStr(ITEM_WATERSHED, sym->GetStr(ITEM_WATERSHED));
		res.SetStr(ITEM_SUMMARY, sym->GetStr(ITEM_SUMMARY));

		if (order>=0)
			if (res.GetStr(ITEM_WATERSHED).IsEmpty())
				res.SetNum(ITEM_WATERSHED, order);

		LLEC *lle2 = lle->lle;
		/*
		// connections
		vara conn[2];
		for (int c=0; c<lle->conn.length(); ++c)
				conn[0].push( lle->conn[c]->river->seg->IdStr() );
		for (int c=0; c<lle2->conn.length(); ++c)
				conn[1].push( lle2->conn[c]->river->seg->IdStr() );
		*/

		LLEC clle;
		if (!RiverLL(res.GetStr(ITEM_COORD), clle))
			{
			Log(LOGALERT, "Bad res coordinates %.450s", res.Line());
			return FALSE;
			}
		double d0 = Distance(clle.lat, clle.lng, lle->lat, lle->lng);
		double d1 = Distance(clle.lat, clle.lng, lle2->lat, lle2->lng);
		if (d0>MINRIVERJOIN && d1>MINRIVERJOIN)
			{
			Log(LOGALERT, "Inconsistent SEG LLEC %g,%g vs %g,%g & %g,%g", clle.lat, clle.lng, lle->lat, lle->lng, lle2->lat, lle2->lng); 
			return FALSE;
			}
		int mode = d0>d1;
		
		//res.SetStr(ITEM_CONN, lle->river->seg->IdStr() + " " + conn[mode].join(";")+ " " + conn[!mode].join(";"));
		symdata.Empty();
		sym=NULL;
		return TRUE;
	}

	CString IdStr(void)
	{
		vara sump(sym->GetStr(ITEM_SUMMARY), " ");
		double id = CRiver::Id(sump[0], sump[1], sump[2]);
		return ::IdStr(id);
	}

	SEG *Set( CRiver *cr)
	{	
		Reset();
		cr->Box();
		length = cr->length;
		bbox = cr->Box();
		sym = cr->sym;
		//river = cr;
		id = 0;
		if (cr->sym)
			{
			const char *name = cr->sym->id;
			id = crc32(name, strlen(name));
			}
		return cr->seg = this;
	}

/*
	int Find( int num )
	{
		for (int i=0; i<slist.GetSize(); ++i)
			if (slist[i]->num == num)
				return TRUE;
		return FALSE;
	}
*/
	void Reset(void)
	{
		id = 0;
		order = -1;
		minor = FALSE;
		lle = NULL;
	}

	SEG(void)
	{
		Reset();
	}
};


typedef CArrayList<SEG*> LLESEGLIST;




int cmpprop(LLEC **A, LLEC **B)
{
	int Ae = (*A)->elev == (*A)->lle->elev;
	int Be = (*B)->elev == (*B)->lle->elev;
	if (Ae && !Be) return 1;
	if (!Ae && Be) return -1;
	if ((*A)->river()->length > (*B)->river()->length) return 1;
	if ((*A)->river()->length < (*B)->river()->length) return -1;
	return 0;
}


typedef int rproc(CRiver *r, LLEC *lle, double maxdist, void *data);

int LLEFind(LLECLIST &llelist, double id)
{
	for (int i=0; i<llelist.GetSize(); ++i)
		if (llelist[i]->Id()==id)
			return i;
	return -1;
}


class LLEProcess
{
SEG *newseg;
LLESEGLIST seglist;
LLECLIST llelist;
CArrayList<LLEC> lledata;

int LLENum(LLEC *lle)
{
	for (int i=0; i<llelist.GetSize(); ++i)
		if (llelist[i]==lle)
			return i;
	return -1;
}


void LLEConnPrint(const char *id, LLEC *lle)
{
	LLEPrint(MkString("%s:", id), lle);
	for (int i=0; i<lle->conn.GetSize(); ++i)
		{
		double d = Distance(lle->lat, lle->lng, lle->conn[i]->lat, lle->conn[i]->lng);
		double d2 = 0; if (i>0) d2 = Distance(lle->conn[i]->lat, lle->conn[i]->lng, lle->conn[i-1]->lat, lle->conn[i-1]->lng);
		LLEPrint(MkString("%s -%d->", id, i)+MkString(" %4.2fm ", d)+MkString(" +%4.2fm ", d2), lle->conn[i]); 
		}
}

void LLEConnCheck(LLEC *lle, int join)
{
	int n = lle->conn.GetSize();
	for (int i=0; i<n; ++i)
		{
		for (int j=i+1; j<n; ++j)
			if (lle->conn[i]==lle->conn[j])
				{
				Log(LOGERR, "LLEC Integrity check failed! DUP CONN");
				LLEConnPrint(">>>", lle);
				}

		int nconn = lle->conn[i]->conn.GetSize() != n;
		if (!join) nconn = FALSE;
		if (nconn || lle->conn[i]->conn.Find(lle)<0)
			{
			Log(LOGERR, "LLEC Integrity check failed! WRONG CONN");
			LLEConnPrint(">>>", lle);
			for (int c=0; c<n; ++c)
				{
				LLEConnPrint((i==c ? "ERR" : "OK "), lle->conn[c]);
				}
			}
		}
}


public:

LLECLIST &LLElist(void) { return llelist; }

int LLEFind(double id)
{
	return ::LLEFind(llelist, id);
}

int LLESize(void)
{
	return llelist.GetSize();
}

void LLEPrint(const char *id, LLEC *lle)
{
	Log(LOGINFO, "%s #%d (%p) %dc %.8f,%.8f - %.8f,%.8f L%g D%g M%d", id, LLENum(lle), lle->river(), lle->conn.GetSize(), lle->lat, lle->lng, lle->lle->lat, lle->lle->lng, 
		lle->river() ? lle->river()->length : -1, Distance(lle->lat, lle->lng, lle->lle->lat, lle->lle->lng), 0);
}

void LLECheck(LLEC *lle)
{
	if ( lle->lle->lle!=lle)
		Log(LOGFATAL, "LLEC Integrity check failed! LLEC PTR");
	if (lle->river()!=lle->lle->river())
		Log(LOGFATAL, "LLEC Integrity check failed! SEG PTR");
#if 0
	if (lle->river())
	{
			LLEC SE[2];
			vara sump(lle->river()->seg->sym.GetStr(ITEM_SUMMARY), " ");
			if (!RiverLL(sump[0], SE[0]) || !RiverLL(sump[1], SE[1]))
				{
				Log(LOGERR, "Bad res coordinates %s", sump.join(" "));
				}
			double d0 = LLEC::Distance(&SE[0], lle);
			double d1 = LLEC::Distance(&SE[1], lle->lle);
			double d2 = LLEC::Distance(&SE[0], lle->lle);
			double d3 = LLEC::Distance(&SE[1], lle);
			int ok = (d0<=0 && d1<=0) || (d2<=0 && d3<=0);
			if (!ok)
				{
				Log(LOGALERT, "Inconsistent merged seg %g %g %g %g <> %s", d0, d1, d2, d3, sump.join(" "));
				LLEPrint("Bad", lle);
				}
	}

#endif
}


//int LLEProcess(CCacheRiverList &rlist, , int simplify, int reorder)
~LLEProcess(void)
{
	Reset();
}

LLEProcess(void)
{
	newseg = NULL;
}

void Reset(void)
{
	delete [] newseg;
	newseg = NULL;
	seglist.Reset();
	llelist.Reset();
	lledata.Reset();
}

void Dim(int addnum)
{
	// allocate extra structs
	int size = lledata.GetSize();
	lledata.SetSize(size+addnum);
	lledata.SetSize(size);
}

LLEC *Add(LLE &lle, void *ptr = NULL)
{
	LLEC *data = lledata.push(lle);
	data->ptr = ptr;
	return data;
}

void Add(LLE &A, LLE &B, CRiver *ptr)
{
	lledata.push(A);
	lledata.push(B);
	int size = lledata.GetSize();
	LLEC::Link(&lledata[size-2], &lledata[size-1], ptr);
}

void Sort(void)
{
	// mappings
	llelist.SetSize(lledata.GetSize());
	for (int ii=0; ii<llelist.GetSize(); ii++)
		llelist[ii] = &lledata[ii];

	// Calc
	llelist.Sort(LLEC::cmp);
}

void Load(tpoints &list)
{
		Dim(list.GetSize());

		// load data
		for (int i=0; i<list.GetSize(); i++)
			Add(LLE(list[i].lat, list[i].lng, list[i].elev));

		// Calc
		Sort();
}


void Load(CSymList &list, BOOL useptr = TRUE)
{
		Dim(list.GetSize());

		// load data
		for (int i=0; i<list.GetSize(); i++)
			Add(LLE(list[i].GetNum(ITEM_LAT), list[i].GetNum(ITEM_LNG)), useptr ? &list[i] : NULL);

		 // Calc
		 Sort();
}

void Load(CScanList &scanlist);
void Load(CScanListPtr &scanlist);

void Load(CCacheRiverList &rlist)
{
		int n = 0;
		for (int i=0; i<rlist.GetSize(); ++i)
			n += rlist[i]->GetSize();	

		// allocate extra structs
		newseg = new SEG[n];
		seglist.SetSize(n);

		Dim(n*2);

		// load data
		int j =0;
		for (int r=0; r<rlist.GetSize(); ++r)
		  for (int i=0; i<rlist[r]->GetSize(); i++)
			{
			CRiver *cr = &((*rlist[r])[i]);
			SEG *seg = seglist[j] = &newseg[j]; j++;
			LLEC *s = Add(cr->SE[0]);
			LLEC *e = Add(cr->SE[1]);
			
			LLEC::Link(s, e, cr); // MUST be reestablished for cached CRivers
			seg->Set(cr);

			LLECheck(s);
			LLECheck(e);
			ASSERT( s->river()!=NULL && e->river()!=NULL );	
			}

		 // Calc
		 Sort();
}


void Load(CRiverList &rlist)
{
	CCacheRiverList crlist;
	crlist.AddTail( &rlist );
	Load(crlist);
}

inline void _MatchBox(CRiver &river, double distbox, rproc *proc, void *data, register int &j)
{
	  register int nlle = llelist.GetSize();
	  LLRect box = river.Box();
	  if (distbox>0) 
		  box.Expand(distbox);
	  //skip out of box
	  //ASSERT(i!=2380);
	  while (j<nlle && llelist[j]->lat < box.lat1)
		  ++j;
	  // process inside box
	  for (register int k=j; k<nlle; ++k) 
		{
		register LLEC *lle = llelist[k];
		if (lle->lat > box.lat2)
			break; // finish box
		if (lle->lng < box.lng1 || lle->lng > box.lng2)
			continue; // out of bounds

		// match, single or multiple
		if (proc(&river, lle, distbox, data))
			break;
		}
}

void MatchBox(CRiver &river, double distbox, rproc *proc, void *data)
{	
	int j = 0;
	_MatchBox(river, distbox, proc, data, j);
}

void MatchBox(CRiverListPtr &list, double distbox, rproc *proc, void *data)
{
	int nbox = list.GetSize();
	for (int i=0, j=0; i<nbox; ++i)
	  _MatchBox(*list[i], distbox, proc, data, j);
}


inline void MatchBox(CRiverList *rlist, double distbox, rproc *proc, void *data)
{
	return MatchBox(rlist->SortList(CRiverList::cmpbox), distbox, proc, data);
}


typedef int lproc(LLEC *r, LLEC *lle, double maxdist, void *data);
void MatchLLE(LLECLIST &rlist, double dist, lproc *proc, void *data)
{
	int match = 0;
	rlist.Sort(LLE::cmp);
	int nbox = rlist.GetSize(), nlle = llelist.GetSize();
	if (nbox<=0) return;

	// average distance at this lat, lng x 2 for safety
	LLEC *mlle = rlist[nbox/2];
	LLRect llbox = LLDistance(mlle->lat, mlle->lng, dist);
	double dlat = llbox.lat2-llbox.lat1, dlng = llbox.lng2-llbox.lng1;

	for (int i=0, j=0; i<nbox; ++i)
	  {
	  //skip out of box
	  //ASSERT(i!=2380);
	  LLEC &box = *rlist[i];
	  while (j<nlle && llelist[j]->lat < box.lat - dlat)
		  ++j;
	  // process inside box
	  for (int k=j; k<nlle; ++k) 
		{
		LLEC *lle = llelist[k];
		if (lle->lat > box.lat + dlat)
			break; // finish box
		if (lle->lng < box.lng - dlng || lle->lng > box.lng + dlng)
			continue; // out of bounds

		//ASSERT(LLRect(box.lat, box.lng, dist).Contains(lle->lat, lle->lng));
		// match, single or multiple
		if (proc(rlist[i], lle, dist, data))
			break;
		}
	  }
}


void MatchLLEElev(LLECLIST &rlist, double dist, lproc *proc, void *data)
{
	PROFILE("MatchLLEElev()");

	#define MAXELEVDIFF 15 //15x2 = 30m / 100ft meters

	int match = 0;
	rlist.Sort(LLE::cmpelev);

	int nbox = rlist.GetSize(), nlle = llelist.GetSize();
	if (nbox>0)
		{
		// average distance at this lat, lng x 2 for safety
		LLEC *mlle = rlist[nbox/2];
		LLRect llbox = LLDistance(mlle->lat, mlle->lng, dist);
		float dlat = llbox.lat2-llbox.lat1, dlng = llbox.lng2-llbox.lng1;

		for (register int i=0, j=0; i<nbox; ++i)
		  {
		  //skip out of box
		  //ASSERT(i!=2380);
		  register LLEC *box = rlist[i];
		  register float boxelev1 = box->elev + MAXELEVDIFF;
		  while (j<nlle && llelist[j]->elev > boxelev1)   
			  ++j;
		  // process inside box
		  register float boxelev2 = box->elev - MAXELEVDIFF;
		  register float boxlat1 = box->lat - dlat; 
		  register float boxlat2 = box->lat + dlat;
		  register float boxlng1 = box->lng - dlng; 
		  register float boxlng2 = box->lng + dlng;
		  for (register int k=j; k<nlle; ++k) 
			{
			register LLEC *lle = llelist[k];
			if (lle->elev < boxelev2)
				break; // finish box
			if (lle->lat < boxlat1 || lle->lat > boxlat2)
				continue; // out of bounds
			if (lle->lng < boxlng1 || lle->lng > boxlng2)
				continue; // out of bounds

			// match, single or multiple
			if (proc(box, lle, dist, data))
				break;
			}
		  }
		}
	// recover original sort
	rlist.Sort(LLE::cmp);
}

void Disable(LLEC *lle)
{
	//LLECheck(lle);
	//ASSERT(seglist[34]!=lle->river()->seg);
	lle->river()->seg->symdata.Empty();
	lle->river()->seg->sym = NULL;
	lle->river()->seg = NULL;
	lle->ptr = lle->lle->ptr = NULL;
}

int Connect(int join)
{
	double minjoin = join ? 0 : MINRIVERJOIN;

	int joined = 0;
	int n = llelist.GetSize();
	if (!n) return 0;

	// compute median and ample pre-distance check
	LLEC *mlle = llelist[n/2];
	LLRect mbox = LLDistance(mlle->lat, mlle->lng, MINRIVERJOIN);
	double dlat = mbox.lat2-mbox.lat1, dlng = mbox.lng2-mbox.lng1;

	for (int i=0; i<n; ++i)
	  {
	  LLEC *lle = llelist[i];
	  lle->conn.Reset();
	  if (lle->river())
		{
			/*
			double l = lle->river()->seg->length;
			if (l<1)
				Log(LOGALERT, "Inconsistent seg length %g<1m\n%s", l, lle->river()->sym->Line());
			if (l<minjoin)
				minjoin = l/2;
			*/
	   if (join)
		 if (lle->river()->length<=minjoin) //LLEC::Match(lle, lle->lle))
		    {
			// ignore small pieces
			Disable(lle);
			}

		}
	  LLECheck(lle);
	  }
	for (int i=0; i<n; ++i)
	  if (llelist[i]->ptr)
		{
		LLEC *lle = llelist[i];
		//if ( LLEC::Match(lle, &LLEC(49.07522300,-122.99075800)) || i==2076 )
		//	lle = lle;
		// top elev, find connections
		for (int j=i+1; j<n; ++j)
		  if (llelist[j]->ptr)
			{
			LLEC *llej = llelist[j];
			//ASSERT(!(i>=8694 && j<=8696));
			//if ( LLEC::Match(lle, &LLEC(49.07522300,-122.99075800)) && LLEC::Match(llej, &LLEC(49.07522300,-122.99075800)) )
			//	lle = lle;
			
			if (llej->lat > lle->lat + dlat)
				break; // finish box
			if (llej->lng < lle->lng - dlng || llej->lng > lle->lng + dlng)
				continue; // out of bounds

			if (lle->ptr == llej->ptr)
				continue; // same segment
			if (lle->river()->id==llej->river()->id)
				{
				// avoid duplicates
				Disable(llej);
				continue;
				}
			// distance check
			double d = LLEC::Distance(lle, llej);
			if (d>minjoin)
				continue;
			// avoid loops
			double d2 = LLEC::Distance(lle, llej->lle);
			if (d>d2)
				continue;
			double d3 = LLEC::Distance(lle->lle, llej);
			if (d>d3)
				continue;
			double d4 = LLEC::Distance(lle->lle, llej->lle);
			if (d>d4)
				continue;
			if (d==0 && d4==0 && lle->river()->seg->length==llej->river()->seg->length)
				{
				// avoid duplicates
				Disable(llej);
				continue;
				}

			lle->conn.AddTail(llej);
			llej->conn.AddTail(lle);
			//if ( LLEC::Match(lle, &LLEC(19.53015133059999,-99.74134687199999)) )
			//	lle = lle;
			}

	    LLECLIST &conn = lle->conn;
		for (int s=conn.GetSize()-1; s>=0; --s)
			if (!conn[s]->river())
				conn.Delete(s);
		//if ( LLEC::Match(lle, &LLEC(19.53015133059999,-99.74134687199999)) )
		//		LLEPrint(MkString("%d", i), lle);
		if (join && lle->conn.GetSize()==1)
			{
			LLEC *lle2 = lle->conn[0];
			if (LLEC::Distance(lle->lle, lle2->lle)<=minjoin)
				continue; // do not join circles
#if 1 // Mexico Problem, but better keep it
			// no join: ->.<- <-.->
			if (!LLEC::Directional(lle, lle->lle) && !LLEC::Directional(lle2, lle2->lle))				
				continue;			
			if (!LLEC::Directional(lle->lle, lle->lle->lle) && !LLEC::Directional(lle2->lle, lle2->lle->lle))
				continue;
#else
			if (seg->river()->GetNum(ITEM_WATERSHED)!=seg2->river()->GetNum(ITEM_WATERSHED))
				continue;
#endif
			// yes join: ->.->  -.-> ->.- -.-
			if (INVESTIGATE>=0)
				Log(LOGINFO, "Joining @%.6f,%.6f", lle->lat, lle->lng);
			//ASSERT( llelist[82]!=lle && llelist[82]!=lle2);
			// join lle2 <- lle
			SEG *seg = lle->river()->seg;
			SEG *seg2 = lle2->river()->seg;
			// yes join: ONLY if same name and same order (if available)
			if (seg==seg2)
				continue;
			if (seg->id!=seg2->id)
				continue;

			//ASSERT( seg->slist.Find("@5381")<0);
			//ASSERT( seg2->slist.Find("@5381")<0);
			//ASSERT(!MatchCoord(lle->lat,48.75) || !MatchCoord(lle->lng,-123.8849848));
			//ASSERT(!MatchCoord(lle->lat,48.75) || !MatchCoord(lle->lng,-123.8849848));
			//ASSERT(MatchCoords(lle->lat,lle->lng,48.75,-123.8849848)!=0);
			//seg2->list.AddTail( seg->list );
			//ASSERT(i!=56601 || j!=56604);
			//ASSERT( seg2->river()->GetStr(ITEM_SUMMARY)!="-122.5211075;49.0033987;45.600 -122.5213673;49.0033446;46.700 20");
			if (!seg->Merge(seg2))
				Log(LOGERR, "Invalid SEG merge");
#if 1		// Integrity check

			LLEC SE[2];
			vara sump(seg->sym->GetStr(ITEM_SUMMARY), " ");
			if (!RiverLL(sump[0], SE[0]) || !RiverLL(sump[1], SE[1]))
				{
				Log(LOGERR, "Bad res coordinates %s", sump.join(" "));
				return FALSE;
				}
			LLEC::Link(&SE[0], &SE[1], NULL);
			double d0 = LLEC::Distance(&SE[0], lle->lle);
			double d1 = LLEC::Distance(&SE[1], lle2->lle);
			if (d0>0 || d1>0)
				{
				double d2 = LLEC::Distance(&SE[0], lle2->lle);
				double d3 = LLEC::Distance(&SE[1], lle->lle);
				if (d2>0 || d3>0)
					{
					Log(LOGALERT, "Inconsistent merged seg %g %g %g %g <> %s", d0, d1, d2, d3, sump.join(" "));
					LLEPrint("LLEC", lle);
					LLEPrint("LLE2", lle2);
					LLEPrint("LLEC->LLEC", lle->lle);
					LLEPrint("LLE2->LLEC", lle2->lle);
					LLEPrint("SE[0]", &SE[0]);
					LLEPrint("SE[1]", &SE[1]);
					Log(LOGINFO, "LLEC:%s", lle->river()->GetStr(ITEM_SUMMARY));
					Log(LOGINFO, "LLE2C:%s", lle2->river()->GetStr(ITEM_SUMMARY));
					}
				}

#endif
			LLEC::Link(lle->lle, lle2->lle, lle->river());
			LLECheck(lle->lle);
			LLECheck(lle2->lle);

			seg2->symdata.Empty();
			seg2->sym = NULL;
			LLEC::Link(lle2, lle, NULL);
			LLECheck(lle);
			LLECheck(lle2);

			++joined;
			continue;
			}
		}

	//cleanup  
	for (int i=0; i<n; ++i)
	  if (llelist[i]->river())
		{
	    LLECLIST &conn = llelist[i]->conn;
		for (int s=conn.GetSize()-1; s>=0; --s)
			if (!conn[s]->river())
				conn.Delete(s);
		}
	for (int i=0; i<n; ++i)
		  if (llelist[i]->river())
			{
			LLEC *lle = llelist[i];
			LLEC *lle2 = lle->lle;
			LLEConnCheck(lle, join);
			LLEConnCheck(lle2, join);
			}
  return joined;
}


int Join(void)
{
		// join
		int tjoined = Connect(TRUE);
		Log(LOGINFO, "Joined %d (Original %d)", tjoined, seglist.GetSize());
		return tjoined;
}

int Simplify(void)
{
		// simplify grid
		int tsimplified = 0;
		int notsimplified = 0;
		for (int i=0; i<llelist.GetSize(); ++i)
		 if (llelist[i]->river())
			{
			LLEC *lle = llelist[i];
			//ASSERT( !LLEC::Match(lle, &LLEC(19.53015133059999,-99.74134687199999)) );
			if (!lle->river()->seg->id) // no name
				{
				if ((lle->conn.GetSize()==0 && LLEC::Directional(lle, lle->lle)) || lle->river()->Level()==1) // level 1
					{
					//ASSERT( !seg->Find(4850) );
					Disable(lle);
					++tsimplified;
					}
				}

			  }
		int tjoined = Connect(TRUE);

		Log(LOGINFO, "Simplified %d & Joined %d (Original %d)", tjoined, tsimplified, seglist.GetSize());
		//LLEPurge(minconnections);
		return tsimplified;
}	

static int Propagate(LLEC *l, int order) //LLECLIST &connlist
{
	if (l->river()->seg->minor)
		--order;
	if  (l->river()->seg->order >= order)
		return FALSE;
	l->river()->seg->order = order;

	LLECLIST prop;
	LLEC *ll = l->lle;
	for (int c=0; c<ll->conn.GetSize(); ++c)
	  {
	  LLEC *lle = ll->conn[c];
	  if (lle->river())
	   if (lle->elev > lle->lle->elev) // no uphill propagation 
		   prop.AddTail(lle);
	  }

	//if (MatchCoord(l->lat, 49.5203212) && MatchCoord(l->lng, -123.2325034))
	//	Log(LOGINFO, "id:%d LLEC:%s LLELLE:%s LLEConn:%d LLELLEConn:%d Prop:%d order:%d", l->river()->seg->id, CCoord2(l->lat, l->lng), CCoord2(l->lle->lat, l->lle->lng), l->conn.GetSize(), l->lle->conn.GetSize(), prop.GetSize(), order);

	prop.Sort(cmpprop);
	for (int c=0; c<prop.GetSize(); ++c)
		{
		// main branch has strongest order
		if (c>0)
			prop[c]->river()->seg->minor = TRUE;
		// propagate new order
		Propagate(prop[c], order);
		}
	return TRUE;
}


static int Cluster(LLEC *l)
{
	if (!(l->elev > l->lle->elev)) // top of arrow
		return 0;

	int upco = 0, maxco = -1;
	for (int c=0; c<l->conn.GetSize(); ++c)
	 {
	  LLEC *lle = l->conn[c];
	  ASSERT( lle!=l && lle->lle!=l);
	  if (lle->river())
	   if (lle->elev < lle->lle->elev) // downhill aggregation
		{
		int cco = lle->river()->seg->order;
		if (cco == maxco )
			upco = maxco+1;
		else
			maxco = max(maxco, cco);
		}
	 }
	return maxco >= upco ? maxco : upco;
}

int Order(void)
{
		// recompute order
		int tinitial = 0, torder = 0, trun = 0;
		// set initial order and propagate
		for (int i=0; i<llelist.GetSize(); ++i)
		 if (llelist[i]->river())
			{
			LLEC *lle = llelist[i];
			if (lle->conn.GetSize()==0 && lle->elev>=lle->lle->elev)
				{
				// Level 1
				Propagate(lle, 1);
				++tinitial;
				}
			}

		// propagate order
		while (TRUE)
		 {
		 ++trun;
		 int neworder = 0;
		 for (int i=0; i<llelist.GetSize(); ++i)
		   if (llelist[i]->river())
		   {
		   LLEC *lle = llelist[i];
		   //if (MatchCoord(lle->lat, 49.5203212) && MatchCoord(lle->lng, -123.2325034))
		   //	Log(LOGINFO, "LLEC:%s LLELLE:%s LLEConn:%d LLELLEConn: %d", CCoord2(lle->lat, lle->lng), CCoord2(lle->lle->lat, lle->lle->lng), lle->conn.GetSize(), lle->lle->conn.GetSize());
		   int order = Cluster(lle);
		   if (Propagate(lle, order))
				++neworder;
		   }
	     if (!neworder)
			 break;
		 torder += neworder;
		 }

		Log(LOGINFO, "Ordered %d with %d initials %d runs (Original %d)", torder, tinitial, trun, seglist.GetSize());
		return torder;
}



void SEGCheck(void)
{
	for (int i=0; i<seglist.GetSize(); ++i)
		if (seglist[i]->lle)
			Log(LOGALERT, "Inconsistent seglist");
	for (int i=0; i<llelist.GetSize(); ++i)
		  if (llelist[i]->river())
			llelist[i]->river()->seg->lle = llelist[0];
	for (int i=0; i<seglist.GetSize(); ++i)
		if (!seglist[i]->lle && seglist[i]->lle->river()==NULL)
			Log(LOGALERT, "Inconsistent seglist");
	for (int i=0; i<llelist.GetSize(); ++i)
		  if (llelist[i]->river())
			llelist[i]->river()->seg->lle = NULL;
}


int Output(CSymList &outlist, LLRect &obox)
{
	Connect(FALSE);
	//SEGCheck();

	// Save connections
	for (int i=0; i<llelist.GetSize(); ++i)
		  if (llelist[i]->river())
			llelist[i]->river()->seg->lle = llelist[i];

	// Save segments
	int errors = 0;
	for (int i=0; i<seglist.GetSize(); ++i)
	  if (seglist[i]->lle!=NULL)
		if (obox.Intersects(seglist[i]->bbox))
			{
			CSym sym;
			if (!seglist[i]->Sym(sym) || sym.data.IsEmpty())
				{
				++errors;
				continue;
				}

			// get connections
			outlist.Add(sym);
			}
		//UpdateList(file, list);


	return errors;
}


};


#ifdef USEDIRECT

static int direct(LLEC *r, LLEC *lle, double maxdist, void *data)
	{
		if (r->river()==lle->river())
			return FALSE;

		LLRect *bbox = (LLRect *)data;
		if (!bbox->Contains(*r))
			return TRUE;
		/*
		if ( Distance( r->lat, r->lng, 38.363541, -118.834496)<1 )
			Log(LOGINFO, "Conencted %p %s (%s) <-> %p %s (%s)",  r, r->Summary(TRUE), IdStr(r->river()->id), lle, lle->Summary(TRUE), IdStr(lle->river()->id));
		*/

		r->conn.AddTail(lle);
		return FALSE; // keep checking
	}


int Direction(LLE *SE0, LLE *SE1, register float minelev = MINELEVDIFF*2)
{
	int dir = 0;
	if (LLE::Directional(SE0, SE1, minelev))
		++dir;
	if (LLE::Directional(SE1, SE0, minelev))
		--dir;
	return dir;
}

#define isSE0(lle) (lle->lat==lle->river()->SE[0].lat && lle->lng==lle->river()->SE[0].lng)
/*
int DirectionRecurse2(LLEC *olle, LLEC *lle, LLEC *maxv, int dir) // +1 outflow from olle -1 inflow into olle
{
		int dir = Direction(olle, lle);
		if (dir!=0) return 

		if (dir>0)
			{
			if (lle->elev > maxv->elev)
				maxv = lle;
			}
		else
			{
			if (lle->elev < maxv->elev)
				maxv = lle;
			}

		for (int i=lle->conn.length()-1; i>=0; ++i)
			{
			dir = DirectionRecurse2(lle, maxv, dir);
			}
}
*/

int DirectionRecurse(LLEC *olle, LLEC *lle) // +1 outflow from olle -1 inflow into olle
{
	while (TRUE)
		{
		int islle0 = isSE0(lle);
		int dir = lle->river()->dir * (islle0 ? -1 : 1);
		int dir2 = Direction(olle, lle);
		ASSERT(dir==0 || dir2==0 || dir==dir2);
		return dir;
		}
	//ASSERT( dir1==0 || dir2==0 || dir1 == dir2);
	return 0;

#ifdef DEBUGXXX
	int rdir = Direction(lle, lle->lle);
	int rdir2 = Direction(&lle->river()->SE[0], &lle->river()->SE[1]);
	int rdir2b = lle->river()->dir;
	ASSERT( isSE0(lle) || isSE0(lle->lle));
	ASSERT(LLE::Distance(lle, &lle->river()->SE[0])< 1 || LLE::Distance(lle, &lle->river()->SE[1])< 1);
	ASSERT(LLE::Distance(lle->lle, &lle->river()->SE[0])< 1 || LLE::Distance(lle->lle, &lle->river()->SE[1])< 1);
	ASSERT (LLE::Distance(lle, lle->lle)>1);
	ASSERT (islle0 == (LLE::Distance(lle, &lle->river()->SE[0])<1));
	ASSERT(rdir2==0 || rdir2==rdir2b);
	if (rdir!=0 && dir1!=0)
		if (rdir!=dir)
			rdir=dir;
#endif
	/*
	LLECLIST &conn = lle->conn;
	for (int i=conn.length()-1; i>=0 && dir==0; --i)
		dir = DirectionRecurse(olle, conn[i]->lle, ++recurse);
	*/
}

void SetDirect(LLEC *olle, int dir = 1)
{
	LLEC *lle = olle;
	while (lle->river()->dir==0)
	{
		CRiver *r =  lle->river();
		ASSERT( IdStr(r->id)!="R3bdf9f4766edd541"); // Ree7cdf2a91d5c241
		r->dir = isSE0(lle) ? dir : -dir;
		if (lle->lle->conn.length()==1 && Direction(lle , lle->lle)==0)
			{
			lle = lle->lle->conn[0];
			if (lle->lle->elev < olle->elev)
				continue;
			}
		break;
	}
}

int DirectionPlus(LLEC *olle)
{
	LLEC *lle = olle;
	while (TRUE) // avoid circulars
	{
		int dir = Direction(olle , lle->lle);
		if (dir!=0) return dir;
		if (lle->lle->conn.length()==1)
			{
			lle = lle->lle->conn[0];
			if (lle->river()->id == olle->river()->id)
				break; //ciclic
			if (lle->lle->elev < olle->elev)
				continue;
			}
		break;
	}
	return 0;
}



LLEC *GetDirect(LLEC *SEC[2], int dir[2][3])
{
		for (int d=0; d<2; ++d)
			{
			int ddir = d==0 ? 1 : -1;
			LLECLIST &conn = SEC[d]->conn;
			for (int s=conn.length()-1; s>=0; --s)
				{
				LLEC *&c = conn[s];
				int flow = DirectionRecurse(SEC[d], c->lle);
				// <0 inflow >0 outflow
				if (flow<0) ++dir[d][0];
				if (flow>0) ++dir[d][1];
				if (flow==0) ++dir[d][2];
				}
			if (dir[d][2]==0)
				{
				if (dir[d][1]>0 && dir[d][0]==0) // outflow
					return SEC[!d];
				if (dir[d][0]>0 && dir[d][1]==0) // inflow
					return SEC[d];
				}
			}

		return NULL;
}


void DirectRiver(LLEC *lle, LLRect &bbox)
{
		//ASSERT( Direction(lle, lle->lle) ==0 );

		// compute propagated dir
		LLEC *SEC[2]; 
		CRiver *r = lle->river();
		if (isSE0(lle))
			SEC[0] = lle, SEC[1] = lle->lle;
		else
			SEC[1] = lle, SEC[0] = lle->lle;

		//ASSERT( IdStr(r->id)!="R52b81e6db7f88241");

		int rdir = DirectionPlus(lle);
		if (rdir!=0)
			{
			SetDirect(lle, rdir);
			return;
			}
		//ASSERT( r->dir*-1 == Direction(&r->SE[1], &r->SE[0]));


		int dir[2][3] = { 0, 0, 0, 0, 0, 0};
		LLEC *newlle = GetDirect(SEC, dir);
		if (newlle)	
			SetDirect(newlle);

		BOOL sc = SEC[0]->conn.length();
		BOOL ec = SEC[1]->conn.length();
		if (sc==0 && ec==0)
			{
			//int dir = Direction(SEC[0], SEC[1], 1);
			//if (dir>0) return SEC[0];
			//if (dir<0) return SEC[1];
			return;
			}
		if (sc==0 && bbox.Contains(*SEC[0]))
			SetDirect(SEC[0]);
		if (ec==0 && bbox.Contains(*SEC[1]))
			SetDirect(SEC[1]);
}


void CRiverList::Direct(LLRect &bbox)
	{
	int n = rplist.GetSize();
	LLEProcess proc;
	proc.Dim(n*2);
	for (int i=0; i<n; ++i)
		{
		CRiver *r = rplist[i];
		if (LLE::Distance(&r->SE[0], &r->SE[1])>MINSTEP)
			{
			proc.Add(r->SE[0], r->SE[1], r);
			// set up initial dirs
			}
		  else
		   { 
		   // aberration
		   i = i;
		   }
		}

	// connect rivers
	proc.Sort();
	LLECLIST &list = proc.LLElist();
	proc.MatchLLE(list, MINSTEP, direct, &bbox);

	// propagate from higher elev to lower
	list.Sort(LLE::cmpelev);
	int n2 = list.GetSize();
	for (int r=0; r<3; ++r)
	  for (int i=0; i<n2; ++i)
	    if (list[i]->river())
		  if (list[i]->river()->dir==0)
			DirectRiver(list[i], bbox);

#ifdef DEBUGXXX
	int inconsistent = 0, unknown  = 0;
	CDoubleArrayList idlist;
	for (int i=0; i<n2; ++i)
	   if (list[i]->river())
		  {
			LLEC *SEC[2]; 
			LLEC * lle = list[i];
			CRiver *r = lle->river();
			if (isSE0(lle))
				SEC[0] = lle, SEC[1] = lle->lle;
			else
				SEC[1] = lle, SEC[0] = lle->lle;

			if (idlist.Find(r->id)>=0)
				continue;
			idlist.AddTail(r->id);

			int dir[2][3] = { 0, 0, 0, 0, 0, 0 };
			GetDirect(SEC, dir);

			//if (INVESTIGATE>=0)
			{
			int odir = Direction(SEC[0], SEC[1], MINELEVDIFF);
			if (odir!=0 && r->dir!=0 && r->dir!=odir)
				{
				++inconsistent;
				Log(LOGERR, "Inconsistent direction %s %d != %delev = 0:[%din %dout %d?] 1:[%din %dout %d?] : %p %s %dC-?- %p %s %dC", IdStr(r->id), r->dir, odir, dir[0][0], dir[0][1], dir[0][2], dir[1][0], dir[1][1], dir[1][2], SEC[0], SEC[0]->Summary(TRUE), SEC[0]->conn.GetSize(), SEC[1], SEC[1]->Summary(TRUE), SEC[1]->conn.GetSize());
				}
			}

			if (r->dir == 0)
				if (abs(SEC[0]->elev-SEC[1]->elev)>MINELEVDIFF)
					{
					++unknown;
					Log(LOGERR, "No direction for %s 0:[%din %dout %d?] 1:[%din %dout %d?] : %p %s %dC-?- %p %s %dC", IdStr(r->id), dir[0][0], dir[0][1], dir[0][2], dir[1][0], dir[1][1], dir[1][2], SEC[0], SEC[0]->Summary(TRUE), SEC[0]->conn.GetSize(), SEC[1], SEC[1]->Summary(TRUE), SEC[1]->conn.GetSize());
					}

			}
	Log(LOGINFO, "%d inconsistent and %d unknown directions for %s", inconsistent, unknown, file);
#endif
	}


#else
void CRiverList::Direct(void)
	{
	}
#endif


CRiver *CRiverList::Find(LLE *SE)
	{
		for (int i=0; i<rplist.length(); ++i)
			{
			CRiver &r = *rplist[i];
			if (r.SE[0].lat==SE[0].lat && r.SE[0].lng==SE[0].lng && r.SE[1].lat==SE[1].lat && r.SE[1].lng==SE[1].lng)
				return &r;
			if (r.SE[0].lat==SE[1].lat && r.SE[0].lng==SE[1].lng && r.SE[1].lat==SE[0].lat && r.SE[1].lng==SE[0].lng)
				return &r;
			}
		return NULL;
	}

typedef struct { double mind; CRiver *r; } minddata;
static int checkmind(CRiver *r, LLEC *lle, double maxdist, void *data)
	{
		minddata *md = (minddata *)data;
		double d = r->DistanceToLine(lle->lat, lle->lng);
		if (d<md->mind)
			md->mind = d, md->r = r;
		return FALSE; // keep checking
	}

CRiver *CRiverList::FindPoint(double lat, double lng, double mind)
	{
#if 1
		LLEProcess proc;
		proc.Add(LLE(lat,lng));
		proc.Sort();

		minddata data = { mind, NULL };
		proc.MatchBox(this, mind, checkmind, &data);
		return data.r;
#else
		CRiver *ret = NULL;
		for (int i=0; i<rplist.length(); ++i)
			{
			CRiver &r = *rplist[i];
			if (r.Box().Contains(lat, lng))
				{				
				double d = r.DistanceToLine(lat, lng);
				double ds = Distance(lat, lng, r.SE[0].lat, r.SE[0].lng);
				double de = Distance(lat, lng, r.SE[1].lat, r.SE[1].lng);
				if (d<mind || ds<mind || de<mind)
					mind = d, ret = &r;
				}
			}
		return ret;
#endif
	}





void CalcRivers(const char *folder, const char *poifolder, int join, int simplify, int reorder, const char *startfile)
{
	// limit cache
	rivercache.SetMaxsize(9);

	int terrors = 0;
	// Rename CSV to BAK
	const char *exts[] = { "CSV", NULL };
	Log(LOGINFO, "===== PROCESSING RIVERS: %s --> %s =====", folder, poifolder);

	// Delete old files
	Log(LOGINFO, "%s contents will be deleted in 10 seconds...", poifolder);
	Sleep(10*1000);
	CSymList dellist; 
	Log(LOGINFO, "Deleting %s contents...", poifolder);
	GetFileList(dellist, poifolder, exts);
	for (int i=0; i<dellist.GetSize(); ++i)
		DeleteFile(dellist[i].id);

	// Load files
	CSymList filelist; 
	GetFileList(filelist, folder, exts);
	for (int i=0; i<filelist.GetSize(); ++i)
		{
		CSymList list;
		CString ofile = filelist[i].id;
		printf("Processing %d/%d (%d%%): %s    \r", i, filelist.GetSize(), (i*100)/filelist.GetSize(), GetFileNameExt(ofile));

		// get bounding box, expand 
		LLRect obox;
		CBox::box(ofile, obox);
		if (obox.IsNull())
			{
			Log(LOGERR, "Invalid CBox filename %s", ofile);
			++terrors;
			continue;
			}
		LLRect ebox(obox.lat1-1, obox.lng1-1, obox.lat2+1, obox.lng2+1);

		if (startfile)
			{
			// force to start processing with specific file
			if (strnicmp(startfile, GetFileName(ofile), strlen(startfile))==0)
				startfile = NULL;
			else
				continue;
			}

		// load cached files
		CRiverCache obj(ofile);
		int oldn = obj.rlist->GetSize();
		CRiversCache cache(&ebox, folder);


		// process list
		CSymList newlist;
		CString newfile = MkString("%s\\%s%s",poifolder, GetFileName(ofile), CSV);

		LLEProcess process;
		process.Load(cache.rlists);
		if (join)
			process.Join();
		if (simplify)
			process.Simplify();
		if (reorder)
			process.Order();		
		int errors = process.Output(newlist, obox);

		UpdateList(newlist);
		GetRiverElev(newlist, newfile);
		if (newlist.GetSize()>0)
			{
			newlist.Save(newfile);
			Log(LOGINFO, "Saved %s %d->%d items %s", newfile, oldn, newlist.GetSize(), errors ? MkString("(%d errors)", errors) : "");
			}
		terrors += errors;

		}

	// Summary
	Log(LOGINFO, "Processed %d files %s", filelist.GetSize(), terrors ? MkString("(%d errors)", terrors) : "");
}



int kmlcanada(const char *line, CSym &sym)
{
	if (!strstr(line, "<LineString>"))
		return FALSE;
	if (!strstr(line, "HN_NLFLOW"))
		return FALSE;
	vars desc;
	GetSearchString(line, "", desc, "<description>", "</description>");
	sym.SetStr(ITEM_DESC, desc);
	return TRUE;
}



int gmlcanada(const char *data, CSym &sym, LLRect &bb)
{
		int stream = strstr(data, "<gml2:hd_1470009_1>")!=NULL;
		int lake = strstr(data, "<gml2:hd_1440009_1>")!=NULL;
		if (!stream && !lake)
			return FALSE;

		//ASSERT( !lake );

		CString line, coords;
		GetSearchString(data, "", line, "<gml:LineString", "</gml:LineString>");
		GetSearchString(line, "<gml:coordinates>", coords, "", "<");
		// line string
		if (line.IsEmpty() || coords.IsEmpty())
			return FALSE;

		// water feature
		CString code;
		GetSearchString(data, "<gml2:code>", code, "", "<");
		const char *codes[] = { "1470111", "1470121","1470131","1470171","1470141", "1470181", NULL};
		const char *types[] = {  "None", "Canal", "Conduit", "Watercourse", "Ditch", "Tidal river", NULL };
		int n = IsMatchN(code, codes);
		CString desc = MkString("%s: %s<br>", "type", n<0 ? "Unknown" : types[n]);
		/*
		const char *attr[] = { "code", "definition", "permanency", "accuracy", NULL };
		for (int i=0; attr[i]!=NULL; ++i)
			{
			CString val;
			GetSearchString(data, MkString("<gml2:%s>", attr[i]), val, "", "<");
			if (!val.IsEmpty())
				desc += MkString("<li>%s: %s</li>", attr[i], val);
			}
		*/
		
		CString title, description = CString(CDATAS) + desc + CString(CDATAE);
		GetSearchString(data, "<gml2:nameen>", title, "", "<");

		int ret = KMLSym(title, description, coords, bb, sym);
		if (ret<0) 
			Log(LOGERR, "ERROR: Invalid GML coords %s", coords);
		if (ret<=0) 
			return FALSE;

		if (lake)
			{
			CString type;
			GetSearchString(data, "<gml2:type>", type, "", "<");
			if (type=="1") // no coast line
				return FALSE;
			ASSERT( type =="2" );

			sym.SetStr(ITEM_TAG, "X");
			}

		return TRUE;
}


int AddSplit(int startj, int endj, const vara &coords, CSymList *list)
{
		if (startj==endj)
			return FALSE;
		// break up line
		vara newcoords;
		for (int j=startj; j<=endj; ++j)
			newcoords.push(coords[j]);

		if (INVESTIGATE>0)
			Log(LOGINFO, "Split: %s [%d] - %s [%d]", CCoordS(newcoords[0]), startj, CCoordS( newcoords[newcoords.length()-1]), endj );

		CSym sym;
		LLRect bb;
		int ret = KMLSym("", CDATAS+CString("type: lake edge")+CDATAE, newcoords.join(" "), bb, sym);
		if (ret<=0)
			{
			Log(LOGERR, "Could not break up coords %s", newcoords.join(" "));
			return FALSE;
			}
		list->Add(sym);
		return TRUE;
}
					
int gmlcanadasplit(CRiver *r, LLEC *lle, double maxdist, void *data)
{
		CSymList *list = (CSymList *)data;
		int f = r->FindPoint(lle->lat, lle->lng);
		if (INVESTIGATE>0)
			Log(LOGINFO, "Point %.5f,%.5f => %d", lle->lat, lle->lng, f);
		if (f<0)
			return FALSE;

		((CIntArrayList *)r->seg)->AddAt(TRUE, f);
		return FALSE; // keep checking

/*
		// find join points
		for (int r=0; r<rlist.GetSize(); ++r)
			{
			register int j;
			if ((j=rx.FindPoint(rlist[r].SE[1].lat, rlist[r].SE[1].lng))>=0)
				jcoords[j] = TRUE;
			if ((j=rx.FindPoint(rlist[r].SE[0].lat, rlist[r].SE[0].lng))>=0)
				jcoords[j] = TRUE;
			}
*/
}


int gmlcanadafix(const char *file, CSymList &list)
{
	CSymList xlist;
	for (int i=list.GetSize()-1; i>=0; --i)
		if (list[i].GetStr(ITEM_TAG)!="")
			{
			xlist.Add(list[i]);
			list.Delete(i);
			}

	// xlist vs list
	//xlist.Save("X.csv");
	//list.Save("A.csv");

	CRiverList rlist(list), rxlist(xlist);
	
	// set int array list
	CArrayList<CIntArrayList> ilist(rxlist.GetSize());
	for (int i=0; i<rxlist.GetSize(); ++i)
		rxlist[i].seg = (SEG *)&ilist[i];

	LLEProcess proc;
	proc.Load(rlist);
	proc.MatchBox(&rxlist, 0, gmlcanadasplit, &list);

	for (int i=0; i<rxlist.GetSize(); ++i)
	  {
	  CIntArrayList &jcoords = *((CIntArrayList *)rxlist[i].seg);

	  if (jcoords.GetSize()>0)
		{
		vara coords(rxlist[i].GetStr(ITEM_COORD), " ");
		ASSERT(!IsSimilar(coords[0], "-123.2016334;49.5025"));
		// split at join points
		int lastj = 0, splits = 0;
		for (int j=1; j<jcoords.length(); ++j)
			{
			if (jcoords[j])
				{
				AddSplit(lastj, j, coords, &list);
				lastj = j;
				}
			}
		int n = coords.GetSize()-1;
		if (lastj>0 && lastj<n)
			{
			AddSplit(lastj, n, coords, &list);
			}
		}
	  }

	return TRUE;
}





void GetRiversElev(const char *folder)
{
	for (int r=0; RIVERSALL[r]!=NULL; ++r)
	{
	CSymList filelist;
	const char *exts[] = { "CSV", NULL };
	GetFileList(filelist, folder ? folder : filename(NULL, RIVERSALL[r]), exts);
	for (int i=0; i<filelist.GetSize(); ++i)
		{
		CSymList list;
		CString file = filelist[i].id;
		printf("Processing %d%% (%d/%d) %s  \r", (i*100)/filelist.GetSize(), i, filelist.GetSize(), file);
		list.Load(file);
		if (GetRiverElev(list, file))
			SaveKeepTimestamp(file, list);
		}
	if (folder) break;
	}
}

#define WR "WR"
#define WC "WC"
#define GA "GA"
#define GX "GX"

int wgstamp(CRiver *r, LLEC *lle, void *data)
{
	if (r->DistanceToLine(lle->lat, lle->lng)<MAXWGDIST)
		{
		const char *stamp = (const char *)data;
		CString tag = r->sym->GetStr(ITEM_TAG);
		if (strstr(tag, stamp))
			return TRUE;
		r->sym->SetStr( ITEM_TAG, addmode(tag, stamp) );
		return TRUE;
		}
	return FALSE;
}

#define W 'W'
void LoadWaterfalls(CSymList &wrlist)
{
	CSymList filelist;
	const char *exts[] = { "CSV", NULL };
	GetFileList(filelist, POI, exts);
	for (int i=0; i<filelist.GetSize(); ++i)
		{
		char type = GetFileNameExt(filelist[i].id)[0];
		if (type==W) 
			{
			CSymList list;
			list.Load(filelist[i].id);

			// set the source#, order it's important
			vars source = GetFileNameExt(filelist[i].id);
			for (int i=0; i<list.GetSize(); ++i)
				list[i].SetStr(0, source);

			// Add
			wrlist.Add(list);
			}
		}
}





int FixUl(CString &desc)
{
	const char *ulerr = "<ultype";
	if (!strstr(desc, ulerr))
		return 0;
	desc.Replace(ulerr, "<ul type");
	return 1;
}

void FixRivers(const char *folder)
{
	// Download KML
	CSymList filelist;
	const char *exts[] = { "CSV", NULL };
	GetFileList(filelist, folder, exts);
	for (int i=0; i<filelist.GetSize(); ++i)
		{
		CSymList list;
		LLRect box;
		CString file = filelist[i].id;
		CBox::box(file, box);
		printf("Processing %d%% (%d/%d) %s [%s-%s]           \r", (i*100)/filelist.GetSize(), i, filelist.GetSize(),  GetFileNameExt(file), CCoord2(box.lat1, box.lng1), CCoord2(box.lat2, box.lng2));
		list.Load(file);
		int outbb = 0, numid = 0, numul = 0;
		for (int l=0; l<list.GetSize(); ++l)
			{
			CSym &sym = list[l];
			if (FixUl(sym.data))
				++numul;
			if (SkipNumId(sym.id))
				++numid;
			LLRect rbox(sym.GetNum(ITEM_LAT), sym.GetNum(ITEM_LNG), sym.GetNum(ITEM_LAT2), sym.GetNum(ITEM_LNG2));
			if (!box.Intersects(rbox))
				{
				list.Delete(l); --l;
				++outbb;
				}
			}
		int size = list.GetSize();
		UpdateList(list);

		// check CRiver ID
		CRiverList rlist(list);
		rlist.Sort();

		int deleted = size-list.GetSize();		
		if (list.GetSize()==0)
			{
			Log(LOGINFO, "%s: Deleted empty file               ", GetFileNameExt(file));
			DeleteFile(file);
			}
		else if (outbb>0 || deleted>0 || numid>0 || numul>0) 
			{
			// update file but keep old date
			Log(LOGINFO, "%s: Deleted %d out of bounds & %d duplicates & fixed %d ids & %d uls", GetFileNameExt(file), outbb, deleted, numid, numul);
			SaveKeepTimestamp(file, list);
			}
		}
}

void CheckRiversID(const char *folder)
{
	// Download KML
	CSymList filelist;
	const char *exts[] = { "CSV", NULL };
	GetFileList(filelist, folder, exts);
	for (int i=0; i<filelist.GetSize(); ++i)
		{
		// check CRiver ID
		CString file = filelist[i].id;
		printf("Processing %d%% (%d/%d) %s            \r", (i*100)/filelist.GetSize(), i, filelist.GetSize(),  GetFileNameExt(file));
		CSymList newlist;
		newlist.Load(file);
		int item = (INVESTIGATE==0) ? ITEM_WATERSHED : ITEM_SUMMARY;
		for (int i=0; i<newlist.GetSize(); ++i)
			{
			vara summaryp(newlist[i].GetStr(item), " ");
			if (summaryp.length()==3)
				newlist[i].index = CRiver::Id(summaryp[0], summaryp[1], summaryp[2]);
			else
				Log(LOGERR, "Invalid Summary %s", summaryp.join(" "));
			}
		newlist.SortIndex();
		for (int i=1; i<newlist.GetSize(); ++i)
			if (newlist[i].index == newlist[i-1].index)
				{
				CString id0 = newlist[i].GetStr(item);
				CString id1 = newlist[i-1].GetStr(item);
				if (id0!=id1)
					Log(LOGALERT, "Inconsistent ID %g for\n%s\n%s", newlist[i].index, id0, id1);		
				}
		}
}


void CheckCSV(void)
{
	// Download KML
	CSymList filelist;
	const char *exts[] = { "CSV", NULL };
	GetFileList(filelist, POI, exts, TRUE);
	GetFileList(filelist, RIVERSBASE, exts, TRUE);
	for (int i=0; i<filelist.GetSize(); ++i)
		{
		CSymList list;
		printf("Processing %s      \r", filelist[i].id);
		list.Load(filelist[i].id);
		int unsorted = UpdateListCheck(list);
		if (unsorted>0) 
			Log(LOGERR, "Detected %d out of order for %s", unsorted, filelist[i].id);
		
		int invalid = 0;
		for (int l=0; l<list.GetSize(); ++l)
			{
			CSym &sym = list[l];
			double lat = sym.GetNum(ITEM_LAT);
			double lng = sym.GetNum(ITEM_LNG);
			if (lat==InvalidNUM || lng==InvalidNUM)
				{
				if (INVESTIGATE>0)
					Log(LOGERR, "Detected invalid lat/lng for %s (%s, %s)", sym.id, sym.GetStr(ITEM_LAT), sym.GetStr(ITEM_LNG));
				++invalid;
				}
			}
		if (invalid>0) 
			Log(LOGERR, "Detected %d invalid lat/lng for %s", invalid, filelist[i].id);

		int size = list.GetSize();
		UpdateList(list);
		int duplicates = size-list.GetSize();
		if (duplicates>0) 
			Log(LOGERR, "Detected %d duplicates for %s", duplicates, filelist[i].id);
		}
}

void CheckKMZ(void)
{
	CSymList filelist;
	GetFileList(filelist, "KMZ", NULL, TRUE);
	for (int i=0; i<filelist.GetSize(); ++i)
		{
			FILETIME wt, at;
			if (!CFILE::gettime(filelist[i].id, &wt, &at))
				continue;
			double ad = COleDateTime(at);
			double cd = GetTime();
			double days = GetCacheMaxDays(filelist[i].id);
			if (cd-ad > days)
				{
				extern int MODE;
				const char *action = "Checking";
				if (MODE>0) action = "Deleting";
				printf("%s %s %g > %g", action, filelist[i].id, cd-ad, days);
				if (MODE>0)
					if (!DeleteFile(filelist[i].id))
						Log(LOGERR, "could not delete %s", filelist[i].id);
				}
		}
}

#if 1
enum { P_NULL = 0, P_COORD, P_STAR, P_CLASS, P_SUMMARY, P_IMAGE, P_KML, P_ID, P_RAPS, P_LONGEST, P_MAX };
#define P_STRING "|Has_coordinates|Has_star_rating|Has_location_class|Has_summary|Has_banner_image_file|Has_KML_file|Has pageid|Has info rappels|Has info longest rappel"

int rwfloc(const char *line, CSymList &newlist)
	{
	vara val; 
	val.SetSize(P_MAX);

	vara prop;
	vars lat = ExtractString(line,"lat=") ;
	vars lng = ExtractString(line,"lon=");
	if (lat.IsEmpty() || lng.IsEmpty())
		return FALSE;
	vars nameurl = ExtractString(line,"fullurl=").replace(",", "%2C");
	vars name = ExtractString(line,"fulltext=").replace(",", "%2C").replace("&#039;", "'");
	vara labels(line, "label=\"");
	for (int l=2; l<labels.length() && l<P_MAX; ++l) {
		vars v = ExtractString(labels[l], "", "<value>", "</value>").replace("&amp;", "&").replace("&#58;",":");
		if (v.IsEmpty())
			v = ExtractString(labels[l], "<value ", "\"", "\"");
		v.Trim(" \n\r\t\x13\x0A");
		val[l] = v;
		}
	
	vars desc = "<div>"+val[P_SUMMARY].split("*").join("&#9733;")+"</div>";
	desc += "<div>"+jlink(nameurl, "More information")+"</div>";
	if (!val[P_IMAGE].IsEmpty())
		desc += "<div><img src=\""+val[P_IMAGE]+"\"/></div>";
	if (!val[P_KML].IsEmpty())
		desc += "<div><a href=\""+val[P_KML]+"\">Download KML Route</a></div>";

	CSym sym( val[P_ID], CDATAS+desc+CDATAE );
	sym.SetStr(ITEM_LAT, lat);
	sym.SetStr(ITEM_LNG, lng);
	sym.SetStr(ITEM_TAG, val[P_STAR]+val[P_CLASS]);
	sym.SetStr(ITEM_WATERSHED, cleanup(name));
	sym.SetStr(ITEM_SUMMARY, val[P_RAPS]+" "+val[P_LONGEST]);
	sym.SetStr(ITEM_CONN, val[P_STAR]+"*");
	newlist.Add(sym);
	return TRUE;
	}


void DownloadRWLOC(const char *strdays)
{
				int step = 500;
				vara prop(P_STRING, "|");

				Log(LOGINFO, "Started RWLoc update...");
				// http://ropewiki.com/index.php/KMLList?action=raw&templates=expand&ctype=application/x-zope-edit&query=[[Category:Canyons]]&sort=Modification%20date&order=descending&limit=10&offset=5&gpx=off&more=&ext=.kml
				//CString ourl("http://ropewiki.com/index.php/KMLList?action=raw&templates=expand&ctype=application/x-zope-edit");
				//ourl += "&gpx=off&more=&ext=.kml";

				double days = 0;
				if (strdays!=NULL)
					days = CDATA::GetNum(strdays);
				if (days<=0) days = 7;
				CString date = CDate(GetTime()-days);
				CString query = "[[Category:Canyons]]";
				if (!date.IsEmpty())
					query += MkString("[[Category:Canyons]][[Max_Modification_date::>%s]]OR[[Category:Canyons]][[Modification_date::>%s]]", date, date);
				query += prop.join("|%3F")+"|sort=Modification_date|order=asc";

				CSymList newlist;
				GetASKList(newlist, query, rwfloc);

				CString file = filename(RWLOC);
				CSymList list, olist;
				list.Load(file); olist = list;
				UpdateList(list, newlist, "RWLoc", -1);
				list.Save(file);

				for (int i=0, o=0; i<list.GetSize();)
				{
				int cmp = o>=olist.GetSize() ? -1 : strcmp(list[i].id, olist[o].id);
				if (cmp<0)
					{
					DeleteKMZ( GridName(RWLOC, RWLOCOPT, list[i].GetNum(ITEM_LAT), list[i].GetNum(ITEM_LNG)) );
					if (INVESTIGATE>0) Log(LOGINFO, "Added %s", list[i].id);

					++i;
					}
				else if (cmp==0)
					{
					if (strcmp(list[i].data, olist[o].data)!=0)
					  {
					  DeleteKMZ( GridName(RWLOC, RWLOCOPT, olist[o].GetNum(ITEM_LAT), olist[o].GetNum(ITEM_LNG)) );
					  DeleteKMZ( GridName(RWLOC, RWLOCOPT, list[i].GetNum(ITEM_LAT), list[i].GetNum(ITEM_LNG)) );
					  if (INVESTIGATE>0) Log(LOGINFO, "Updated %s", list[i].id);
					  }
					++i, ++o;
					}
				else if (cmp>0)
					{
					DeleteKMZ( GridName(RWLOC, RWLOCOPT, olist[o].GetNum(ITEM_LAT), olist[o].GetNum(ITEM_LNG)) );
					if (INVESTIGATE>0) Log(LOGINFO, "Deleted %s", olist[o].id);
					++o;
					}
				}

				Log(LOGINFO, "Finished RWLoc update (%d locations)", list.GetSize());
}
#else
void DownloadRWLOC(void)
{
				Log(LOGINFO, "Started RWLoc update...");
				CString ourl("http://ropewiki.com/index.php/KMLList?action=raw&templates=expand&ctype=application/x-zope-edit");
				ourl += "&query=[[Category:Canyons]][[Has latitude::>#MINLAT#]][[Has longitude::>#MINLNG#]][[Has latitude::<#MAXLAT#]][[Has longitude::<#MAXLNG#]]";					
					//"&query=%5B%5BCategory%3ACanyons%5D%5D%5B%5BHas%20latitude%3A%3A%3E#MINLAT#%5D%5D%0A%5B%5BHas%20longitude%3A%3A%3E#MINLNG#%5D%5D%0A%5B%5BHas%20latitude%3A%3A%3C#MAXLAT#%5D%5D%0A%5B%5BHas%20longitude%3A%3A%3C#MAXLNG#%5D%5D";
				//ourl += "&query=[[Modification date::>2015-01-01]]&sort=Modification date&order=descending";
				ourl += "&gpx=off&more=&ext=.kml";

				// dowload all quadrants
				DownloadFile f;
				LLRect box(-90, -180, 90, 180);
				CBox cb(&box, 10); // to match GridList 10x10
				for (LLRect *box =cb.Start(); box!=NULL; box=cb.Next())
					{					
					CString file = filename(cb.file()+KML, RWLOC);
					printf("Processing %d%% %s                \r", (int)(cb.Progress()*100), file);

					CString url = ourl;
					url.Replace("#MINLAT#", CData(box->lat1));
					url.Replace("#MINLNG#", CData(box->lng1));
					url.Replace("#MAXLAT#", CData(box->lat2));
					url.Replace("#MAXLNG#", CData(box->lng2));
					if (DownloadUrl(f, url))
						{
						Log(LOGERR, "Could not download update for %s", file);
						continue;
						}
					
					// compare with existing one
					CString data;
					int size = LoadFile(data, file);
					if (f.size!=size || strcmp(f.memory, data)!=0)
						{
						for (int i=0; i<size && f.memory[i]==data[i]; ++i);


						// update file
						Log(LOGINFO, "Updating %s", file);
						CFILE fp;
						if (fp.fopen(file, CFILE::MWRITE))
							{
							fp.fwrite(f.memory, f.size, 1);
							// delete cache
							DeleteKMZ( GridName(RWLOC, RWLOCOPT, box->lat1, box->lng1) );
							}
						}
					else
						{
						// do nothing
						size = size;
						}
					}

				//data.Replace("<scale>1.0</scale></LabelStyle>", "<scale>0.0</scale></LabelStyle>");

				Log(LOGINFO, "Finished RWLoc update");
}
#endif

void DownloadDEM(void)
{
	if (!Savebox)
		{
		Log(LOGERR, "GDEM needs -bbox to capture properly");
		return;
		}
	CBox cb(Savebox);
	int cnt = 0;
	for (LLRect *bbox = cb.Start(); bbox!=NULL; bbox = cb.Next())
	  {
	  
	  CString str = MkString("%c%g%c%g", bbox->lat1<0 ? 'S' : 'N', abs(bbox->lat1), bbox->lng1<0 ? 'W' : 'E', abs(bbox->lng1));
	  CString file = MkString("ASTGTM2_%s.zip", str);
	  CString url = "http://113.35.103.196/gdServlet/Download?POST?_gd_download_file_name="+file;
	  if (CFILE::exist(file))
		  continue;
	  printf("Downloading %s\r", file);
	  CString url0 = MkString("http://gdem.ersdac.jspacesystems.or.jp/gdServletAsyn/AddDownloadLog?POST?_gd_download_file_name=%s&_gd_download_log_method=immediate", file);
	  DownloadFile f;
	  if (f.Download(url0))
		  Log(LOGERR, "Could not download url0 for %s", file);
	  if (f.Download(url, file))
		  Log(LOGERR, "Could not download %s", file);
	  Sleep(500);
	  }
}







CString WatershedString(CSym &sym)
{
			CString url = WatershedURL(sym);
			CString desc = WatershedSummary(sym).join(";");
			desc.Replace(" ", ""); desc.Replace(";", " ");
			if (url!="")
				{
				CRiver r(sym);
				desc = sym.id +" "+desc+" "+alink(url, "[Full Report]", "target=\"blank\"");
				}
			return desc;
}

CString KMLRiverList(CSymList &ulist, CSymList &dlist, vara &list)
{
		CString out;
		for (int m=0; m<2; ++m)
		{
		CString lout;
		CSymList &olist = m ? ulist : dlist;
		for (int i=m; i<olist.GetSize(); ++i)
			{
			CSym &sym = olist[i];
			list.push(RiverSummary(sym));

			lout += "<Placemark>" + KMLName(sym.id, CDATAS+WatershedString(sym)+CDATAE);
			lout += "<Style><LineStyle>";
			lout += MkString("<color>ff%6.6X</color>", m ? RGB(0x00,0x80,0xFF) : RGB(0,0,0xFF));
			lout += "</LineStyle></Style>";
			lout += "<LineString><tessellate>0</tessellate><coordinates>";
			CString coords = sym.GetStr(ITEM_COORD);
			coords.Replace(";",",");
			lout += coords;
			lout += "</coordinates></LineString></Placemark>\n";
			}
		out += "<Folder>" + MkString("<name>%d %s segments</name>", olist.GetSize(), m ? "upstream" : "downstream");
		out += lout;
		out += "</Folder>";
		}
		return out;
}

vara GetRiversPointSummary(const char *coords)
{
	vara list;
	CSym sym;
	if (!RiverListPoint(url_decode(coords), &sym, NULL, NULL))
		return list;
	list.push( WatershedString(sym) );
	return list;
}

CString GetRiversPoint(const char *coords, vara &list)
{
		// find closest river to point and propagate from there
		CSymList ulist, dlist;
		RiverListPoint(coords, NULL, &ulist, &dlist);
		return KMLRiverList(ulist, dlist, list);
}





CString GetRiverSRSUMmary(const char *summary, vara &list)
{
		CSymList ulist, dlist;
		if (isScanner2(summary))
			{
			RiverListScan(summary, list);
			return "";
			}

		// find river by summary and propagate from there
		RiverListSummary(summary, ulist, dlist);
		// get kml lines & list
		return KMLRiverList(ulist, dlist, list);
}


void friendly(CString &name)
{
	if (name[0]=='(')
		name = "Unnamed"+name;
}

CString NetworkLink(const char *name, const char *ourl)
{
	CString out;
	vars url = ourl; //ScannerMap(ourl);
	int ulen = strlen(url);
		if (url[ulen-1]=='!')
			{
			// Delete Loading link with NetworkLinkControl
			CString link = MkString("http://%s/rwr/?rwl=%s", server, CString(url,ulen-1));
			out += MkString("<NetworkLinkControl id=\"%s\"><Update><targetHref>%s</targetHref>\n", name, link);
			out += "<Delete><Document targetId=\"doc\"></Document></Delete>\n";
			out += "</Update></NetworkLinkControl>";	
			}
		else if (url[ulen-2]=='@')
			{
			// NetworkLinkControl
			out += MkString("<NetworkLinkControl><Update><targetHref>http://%s/rwr/?rwz=RWE.KC</targetHref>\n", server);
			if(url[ulen-3]=='R')
				{
				// river list
				vara list;
				CSymList olist;
				GetRiverSRSUMmary(url, list);		
				for (int i=0; i<list.GetSize(); ++i)
					{
					CString link = list[i]+",$MC";
					CString name = ElevationFilename(link);			//<Document id=\"%s\"><name>%s L</name>
					out += MkString("<Delete><Document targetId=\"%s\"></Document></Delete>\n", name);
					out += MkString("<Create><Folder targetId=\"rwres\">", name, name);		
					//CString name1 = ElevationFilename("North%20Fork%20San%20Jacinto%20River,-116.790977;33.748631;1011.900000,-116.799679;33.739198;739.900000,2459.62,$MC");
					CString nlink = MkString("http://%s/rwr/?rwz=%s", server, link);
					out += MkString("<NetworkLink id=\"%s\"><name>Loading</name><Link><href>%s</href></Link></NetworkLink>", name, nlink);
					out += "</Folder></Create>\n";  //</Document><open>0</open><visibility>1</visibility>
					}
				}
			else
				{
					// mapped point
					CString link = vars(url).TrimRight("@$");
					CString name = ElevationFilename(link);			
					out += MkString("<Delete><Document targetId=\"%s\"></Document></Delete>\n", name);
					out += MkString("<Create><Folder targetId=\"rwres\"><Document id=\"%s\"><name>%s</name>", name, name);		
					out += MkString("<NetworkLink><Link><href>%s</href></Link></NetworkLink>", MkString("http://%s/rwr/?rwz=%s,MC", server, link));
					out += "</Document></Folder></Create>\n"; //<open>0</open><visibility>1</visibility><
				}
			out += "</Update></NetworkLinkControl>";	 					
			}
		else if (url[ulen-1]=='@')
			{
			// NetworkLink to load NetworkLinkControl
			CString link = MkString("http://%s/rwr/?rwl=%s", server, url);
			out += MkString("<Document id=\"doc\"><name>MapperLink %s</name>\r\n", name);
			out += MkString("<description>%s</description>", link);
			out += MkString("<NetworkLink id=\"nlink\"><name>%s</name><Link><href>%s@</href></Link></NetworkLink>", "Network Link", link);
			// GE crashes with this
			//link = MkString("http://%s/rwr/?rwl=%s!", server, url);
			//out += MkString("<NetworkLink><name>%s</name><Link><href>%s</href></Link></NetworkLink>", "Cleaning up", link);
			out += "</Document>";
			}
		else if (url[ulen-1]=='$')   // Load with NetworkLinkControl!
			{
			// NetworkLink to load NetworkLinkControls
			vara urlp(url);
			BOOL rivers = url[ulen-2]=='R';
			CString fname = name; 
			if (rivers)
				friendly(fname);
			else
				{
				if (urlp.length()<3)
					{
					Log(LOGERR, "NetworkLink invalid param '%s'", url);
					return ""; // error
					}
				fname = MkString("@%s,%s", urlp[1], urlp[2]);
				}
			CString kmllink = MkString("http://%s/rwr/?rwzdownload=%s", server, CString(url,ulen-1));
			out += MkString("<Document id=\"doc\"><name>MapperLink %s</name>\r\n", fname);
			CString download = jlink(kmllink, "Download KML File");
			out += "<description>" CDATAS+download+CDATAE "</description><Snippet maxLines=\"0\"></Snippet>";


			if (rivers)
				{
				// list (mapper rivers, cached)
				vara list;
				CSymList olist;
				out += GetRiverSRSUMmary(url, list);		
#if 0 // Crashes Google Earth
				// self delete
				CString link = MkString("http://%s/rwr/?rwl=%s!", server, url);
				out += MkString("<NetworkLink><name>%s</name><Link><href>%s</href></Link></NetworkLink>", "Cleanup", link);
#endif
				for (int i=0; i<list.GetSize(); ++i)
					{
					CString fname = ElevationFilename(list[i]); friendly(fname);
					CString link = MkString("http://%s/rwr/?rwz=%s,$MC", server, list[i]);
					out += MkString("<NetworkLink><name>%s</name><Link><href>%s</href></Link></NetworkLink>", "LoadLink "+fname, link);
					}
				}
			else
				{
				// only one (mapped point, not cached)
				out += "<Placemark>";
				out += MkString("<Style><IconStyle><Icon><href>http://sites.google.com/site/rwicons/%s.png</href></Icon><scale>%g</scale><hotSpot x=\"0.5\" y=\"0.5\" xunits=\"fraction\" yunits=\"fraction\"/></IconStyle></Style>", "clicked", 0.5);
  				out += MkString("<Point><coordinates>%s,%s,0</coordinates></Point>", urlp[2], urlp[1]);
				out += "</Placemark>\n";
				CString link = MkString("http://%s/rwr/?rwz=%s", server, url);
				out += MkString("<NetworkLink><name>%s</name><Link><href>%s</href></Link></NetworkLink>", "LoadLink "+fname, link);
				}
			out += "</Document>";
			}
		return out;
}


CString KMLClickMe(const char *param)
{
	// click me
	vars name = "Start", geo;
	vara lookat = vars(param, strlen(param)-4).split("_");
	if (lookat.length()<3)
		{
		Log(LOGALERT, "Invalid call to @ params: %s", param);
		return "";
		}

	for (int i=1; i<=2; ++i)
		{
		int pos = lookat[i].Find('.');
		if (pos>0) lookat[i].Truncate(pos+6);
		}

	if (param[1]=='@')
		{
		// river me
		vara list;
		while (lookat.length()<4) 
			lookat.push("x");
		vara nparam; 
		nparam.push(lookat[2]);
		nparam.push(lookat[1]);
		nparam.push(lookat[3]);
		return GetRiversPoint(nparam.join(";"), list);
		}

	// see info at https://mrdata.usgs.gov/catalog/api.php?id=25
	// example https://mrdata.usgs.gov/geology/state/point-unit.php?y=34.202117&x=-118.122599&d=MRDATA_CODE
	// to get info on unit use https://mrdata.usgs.gov/geology/state/sgmc-unit.php?unit=CAgrMZ3
	CString urlp = MkString("http://mrdata.usgs.gov/geology/state/point-unit.php?y=%s&x=%s&d=", MRDATA_CODE, lookat[1], lookat[2]);
	//if (param[1]=='!')
		{
		DownloadFile f;
		if (!f.Download(urlp))
			if (f.memory[0]!=0)
				name = geo = f.memory;
		}
	
	double lat = CDATA::GetNum(lookat[1]), lng = CDATA::GetNum(lookat[2]);
	tcoord coord(lat, lng);
	DEMARRAY.height(&coord);

	CString url = MkString("http://%s/rwr/?rwl=@,%s,%s,0,MC$", server, lookat[1], lookat[2]);
	//http://www.bing.com/mapspreview?&=&ty=18&q=33.05111%2c-116.98161&lvl=18&style=b&ftst=0&ftics=False&v=2&sV=1&form=S00027
	//http://www.bing.com/mapspreview?&=&ty=18&q=33.05111%2c-116.98161&lvl=18&style=b&ftst=0&ftics=False&v=2&sV=1&form=S00027
	CString url1 = MkString("http://www.bing.com/mapspreview?&=&ty=18&q=%s%%2c%s&lvl=18&style=b&ftst=0&ftics=False&v=2&sV=1&form=S00027", lookat[1], lookat[2]);
	CString url2 = MkString("http://mrdata.usgs.gov/geology/state/sgmc-unit.php?y=%s&x=%s&d=", MRDATA_CODE, "&format=html", lookat[1], lookat[2]);
	CString url3 = MkString("http://mrdata.usgs.gov/general/near-point.php?y=%s&x=%s&d=", MRDATA_CODE, "&format=html", lookat[1], lookat[2]);
	CString url4 = MkString("http://ropewiki.com/Recent_Pictures?location=%s,%s", lookat[1], lookat[2]);
	//<input type=\"button\" id=\"map\" onclick=\"window.open('%s', '_blank');\" value=\"Map this location\">
	//CString desc = MkString(CDATAS"<a href=\"%s\">Run Canyon Mapper</a>"CDATAE, url);
	CString desc = CDATAS;
	vars elev = MkString("%dft [%dm]",(int)(coord.elev*m2ft), (int)coord.elev);
	desc += MkString("<div>Coordinates: %s,%s</div>", lookat[1], lookat[2]);
	desc += MkString("<div>Elevation: %s</div>", elev); 
	desc += MkString("<div>%s Data Resolution: %s</div>", resCredit(coord.credit), resText(coord.res));
	//desc += MkString("<div>Geology: %s</div>", geo);
	desc += MkString("<b><input type=\"button\" id=\"map\" onclick=\"window.open('%s', '_blank');\" value=\"Run Canyon Mapper at this location\"></b><br>", url);
	const char *urlrdesc[] = { "Flow down", "Flow up", "Simulate Rain", NULL };
	for (int i=0; urlrdesc[i]!=NULL; ++i)
		{
		CString urlr = MkString("http://%s/rwr/?rwl=@,%s,%s,0,%cMC$", server, lookat[1], lookat[2], '5'+i);
		desc += MkString("<b><input type=\"button\" id=\"map\" onclick=\"window.open('%s', '_blank');\" value=\"%s\"></b> ", urlr, urlrdesc[i]);
		}
	desc += "<br>";
	desc += "<br><div>Additional tools:<ul>";
	//\" 
	desc += "<li>"+jlink(url1, "Bing Bird View for this location")+"</li>";
	//desc += "<li><a href=\""+url1+"\" target=\"_blank\">Bing Bird View for this location</a></li>";
	desc += "<li>"+jlink(url4, "Recent Pictures for this location")+"</li>";
	desc += "<li>"+jlink(url2, "Geology report for this location")+"</li>";
	desc += "<li>"+jlink(url3, "Full report for this location")+"</li>";
	desc += "</ul></div>";
	desc += CDATAE;
	CString out;
	out += "<Placemark>";
	out += KMLName(elev+":"+name, desc);
	vars icon("clickme");
	if (coord.res > 0)
		icon += MkString("%d", resnum(coord.res));
	else
		icon += 'n';
	out += MkString("<Style><BalloonStyle><text>" CDATAS "<b><font size=\"+1\">Start Location</font></b><br/><br/><div style='width:22em'>$[description]</div>" CDATAE "</text></BalloonStyle>"
		"<IconStyle><Icon><href>http://sites.google.com/site/rwicons/%s.png</href></Icon><scale>%g</scale><hotSpot x=\"0.5\" y=\"0.5\" xunits=\"fraction\" yunits=\"fraction\"/></IconStyle>"
		"<LabelStyle><scale>%g</scale></LabelStyle></Style>", icon, 0.5, 0.7);
  	out += MkString("<Point><coordinates>%s,%s,0</coordinates></Point>", lookat[2], lookat[1]);
	out += "</Placemark>\n";
	return out;
}


#define SCANMAXQUAD 25

vars deletekey(const char *config)
{
		vara list(config, ";");
		int found = list.indexOfSimilar("KEY=");
		if (found>=0)
			list.RemoveAt(found);
		return list.join(";");
}


int ScanBoxRect(const char *config, LLRect &bbox)
{
	vars coord1 = ExtractString(config, "COORD1=", "", ";");
	vars coord2 = ExtractString(config, "COORD2=", "", ";");
	double lat1 = CDATA::GetNum(coord1);
	double lng1 = CDATA::GetNum(GetToken(coord1,1));
	double lat2 = CDATA::GetNum(coord2);
	double lng2 = CDATA::GetNum(GetToken(coord2,1));
	if (!CheckLL(lat1,lng1) || !CheckLL(lat1,lng1))
		return FALSE;
	bbox = LLRect(lat1, lng1, lat2, lng2);
	return TRUE;
}

CString KMLScanBox(const char *config, const char *cookies, const char *status)
{
	LLRect bbox; 

	CString html;
	if (!LoadFile(html, "POI\\rws.htm"))
		Log(LOGERR, "Could not read rws.htm!!!");

	// admin mode
	const char *user = AuthenticateUser(cookies);
	if (user!=NULL && IsSimilar(user, "luca"))
		html.Replace("class=\"admin\"", "");

	CString out;
	const char *boxdesc = NULL;
	int color = color = RGBA(0xA0, 0xFF, 0, 0); 
	vars name = "Canyon Scanner";
	vars icon = "http://sites.google.com/site/rwicons/squarer.png";
	vars snippet = status ? status : "";
	if (strstr(config, "scanname="))
		{
		// SCANNED AREA

		// take out key
		vars nokeyconfig = deletekey(config);
		html.Replace("#CONFIG#", nokeyconfig);
		html.Replace("Run Canyon Scanner", "Re-Run Canyon Scanner");
		vars url = MkString("http://%s/rwr/?rws=", server)+nokeyconfig;

		int cfgstart = html.Find("<a id=\"cfglink\"");
		int cfgend = html.Find("</a>", cfgstart);
		html.Delete(cfgstart, cfgend-cfgstart);
		html.Insert(cfgstart, "<a id=\"cfglink\" href=\""+url+"\">config link</a>");

		/*
		found = list.indexOfSimilar("URL=");
		//out += MkString("<NetworkLinkControl><Update><targetHref>http://%s/rwr</targetHref>\n", server);
		if (found>=0)
			{
			out += MkString("<NetworkLinkControl><Update><targetHref>%s</targetHref>\n", list[found].Mid(4));
			out += MkString("<Change><Folder targetId=\"%s\"><visibility>0</visibility></Folder></Change>", "activescanner");
			out += MkString("</Update></NetworkLinkControl>");
			}
		*/
		color = RGBA(0x40, 0xFF, 0, 0xFF); 
		icon = "http://sites.google.com/site/rwicons/squarem.png";
		ScanBoxRect(config, bbox);

		//if (strstri(config,"OUTPUT=KMLDATA")!=NULL)
		//	html = snippet;
		//out += "<name>"+ExtractString(config, "scanname=","",";")+"</name>";
		//out += "<styleUrl>#scanner</styleUrl>";
		//out += "<description>"+vars(status)+"</description>";
		}
	else
		{
		//out += "<name>"+name+"</name><description></description><Snippet maxLines=\"0\"></Snippet>";
		//out += "<styleUrl>#scanner</styleUrl><Style><ListStyle><listItemType>checkHideChildren</listItemType></ListStyle><IconStyle><Icon><href>"+icon+"</href></Icon></IconStyle></Style>";
		// click me
		vara box = vars(config).split("_");
		if (box.length()>=4) 
			bbox = LLRect(CDATA::GetNum(box[1]), CDATA::GetNum(box[2]), CDATA::GetNum(box[3]), CDATA::GetNum(box[4]));

		int num = 0;
		CBox cb(&bbox);
		for (LLRect *box =cb.Start(); box!=NULL; box=cb.Next(), ++num);
		if (num>SCANMAXQUAD) 
			{
			color = RGBA(0x70, 0, 0, 0);			
			html = snippet = "Scan area is too big, select a smaller area\n"+bbox.Summary();
			}
		}

	// build kml
	vara box;
	box.push(CCoord(bbox.lat1));
	box.push(CCoord(bbox.lng1));
	box.push(CCoord(bbox.lat2));
	box.push(CCoord(bbox.lng2));

	//int scanned = color & RGB(0,0,0xFF);
	html.Replace("#COORD1#", MkString("%s,%s", box[0],box[1]));
	html.Replace("#COORD2#", MkString("%s,%s", box[2],box[3]));

	//CString url = MkString("http://%s/rwr/?rwl=%s,%s,%s,%s,*C", server, box[0],box[1],box[2],box[3]);

	out += "<Style id=\"scanner\"><IconStyle><Icon><href>"+icon+"</href></Icon></IconStyle><BalloonStyle><text>" CDATAS;
	out += html;
	out += CDATAE "</text></BalloonStyle></Style>";

	out += KMLBox(name, boxdesc, bbox,  color, 5, 
		"<IconStyle><Icon><href>"+icon+"</href></Icon></IconStyle>"+
		MkString("<Snippet maxLines=\"%d\">", vara(snippet,"\n").length())+snippet+"</Snippet>"+
		"<styleUrl>#scanner</styleUrl>", 
		"<ListStyle><ItemIcon><href>"+icon+"</href></ItemIcon></ListStyle>"); // extrastyle);

	return out;
}




CString FixJLink(const char *desc)
{
	vara descp = desclist(desc);
	for (int p=0; p<descp.length(); ++p)
		if (strnicmp(descp[p], "http", 4)==0)
			descp[p] = vars(jlink(descp[p]));
	return desctext(descp);
}

void FixPanoramio(const char *folder, const char *outfolder)
{
	CSymList filelist;
	const char *exts[] = { "CSV", NULL };
	GetFileList(filelist, folder, exts, FALSE);

	// load all data
	CSymList list;
	for (int i=0; i<filelist.GetSize(); ++i)
		{
		printf("Processing %s (%d/%d)               \r", GetFileNameExt(filelist[i].id), i, filelist.GetSize());
		list.Load(filelist[i].id);
		}

	// find bbox
	LLRect bbox;
	tcoords coords(list.GetSize());
	for (int i=0; i<list.GetSize(); ++i)
		{
		float &lat = coords[i].lat = (float)list[i].GetNum(ITEM_LAT);
		float &lng = coords[i].lng = (float)list[i].GetNum(ITEM_LNG);
		if (lat==InvalidNUM || lng==InvalidNUM)
			{
			Log(LOGERR, "Invalid lat/lng: %s", list[i].Line());
			continue;
			}
		bbox.Expand(lat, lng);

		ampfix(list[i].id);

		CString desc = list[i].GetStr(ITEM_DESC);
		if (desc=="Panoramio link not available")
			list[i].SetStr(ITEM_DESC, desc = "");
		if (desc!="")
			list[i].SetStr(ITEM_DESC, FixJLink(desc));
		}

	// restructure
	CBox cb(&bbox, POIGRIDSIZE);
	for (LLRect *box =cb.Start(); box!=NULL; box=cb.Next())
		{
		CSymList blist;
		for (int i=0; i<coords.length(); ++i)
			if (box->Contains( coords[i].lat, coords[i].lng))
				blist.Add( list[i] );

		CSymList outlist;
		CString file = MkString("%s\\%s" CSV, outfolder, cb.file());
		int errors = PanoramioKML( file, outlist, box, &blist);
		ASSERT( !errors );

		printf("Processing %s: %d items                \r", file, outlist.GetSize());
		outlist.Save(file);
		}
}


void FixJLink(const char *folder, const char *match)
{
	CSymList filelist;
	const char *exts[] = { "CSV", NULL };
	GetFileList(filelist, folder, exts, FALSE);

	for (int i=0; i<filelist.GetSize(); ++i)
		{
	    CString &file = filelist[i].id;
		if (match)
		 if (!strstr(GetFileNameExt(file), match))
			continue;

		// fix description
		CSymList list; 
		list.Load(file);
		if (FixJLink(list)>0)
			UpdateList(file, list);
		}
}


void CopyFiles(const char *folder, vara &filelist)
{
	double size= 0;
	int exfiles = 0, newfiles = 0;
	for (int i=0; i<filelist.length(); ++i)
	{
			vars &ffrom = filelist[i];
			CString fto = MkString("%s\\%s", folder, GetFileNameExt(ffrom));
			if (!CFILE::exist(ffrom))
				continue;
			if (CFILE::exist(fto))
				{
				++exfiles;
				ffrom.Empty();
				continue;
				}

			CFILE f;
			if (f.fopen(ffrom))
				{
				size += f.fsize();
				++newfiles;
				}
			else
				{
				Log(LOGERR, "ERROR: Could not access file %s", ffrom);
				}
	}

	extern int MODE;
	const char *action = MODE>0 ? "Moving" : "Copying";
	Log(LOGINFO, "%s to %s: %d existing + %d new [%.2fGb]", action, folder, exfiles, newfiles, size/1024/1024/1024 );

	for (int i=0; i<filelist.length(); ++i)
		if (!filelist[i].IsEmpty())
		{
			CString ffrom = filelist[i];
			CString fto = MkString("%s\\%s", folder, GetFileNameExt(ffrom));
			printf("%s %d%% %d/%d : %s            \r", action, (i*100)/filelist.length(), i, filelist.length(), ffrom);
			BOOL error = TRUE;
			for (int r=0; r<3 && error; ++r)
				{
				error = !CopyFile(ffrom, fto, TRUE);
				if (error) Sleep(5*1000);
				}
			if (error)
				Log(LOGERR, "ERROR: Could not copy %s (GetLastError = %d)", ffrom, GetLastError());
			else
				if (MODE>0)
					if (!DeleteFile(ffrom))
						Log(LOGERR, "ERROR: Could not delete %s (GetLastError = %d)", ffrom, GetLastError());
		}

}

void CompareCSV(const char *dir0, const char *dir1)
{
	CSymList filelist[2];
	const char *exts[] = { "CSV", NULL };
	GetFileList(filelist[0], dir0, exts, FALSE);
	GetFileList(filelist[1], dir1, exts, FALSE);

	vara only[2];
	int tonly0 = 0, tonly1 = 0, tdiff = 0;
	int tfonly0 = 0, tfonly1 = 0, tfdiff = 0;
	int i0=0, i1=0; 
	while (i0<filelist[0].GetSize() && i1<filelist[1].GetSize())
		{
		CString file = GetFileName(filelist[0][i0].id);
		printf("Processing %d%% %d/%d %s           \r", (i0*100)/filelist[0].GetSize(), i0, filelist[0].GetSize(), file);
		int cmp = strcmp(GetFileName(filelist[0][i0].id), GetFileName(filelist[1][i1].id));
		if (cmp<0)
			{
			only[0].push( filelist[0][i0++].id );
			continue;
			}
		if (cmp>0)
			{
			only[1].push( filelist[1][i1++].id );
			continue;
			}
		// same files
		CSymList list[2];
		list[0].Load( filelist[0][i0++].id ); list[0].Sort(cmpmarkers);
		list[1].Load( filelist[1][i1++].id ); list[1].Sort(cmpmarkers);

		int diff = 0;
		vara onlysym[2];
		int i0=0, i1=0; 
		while (i0<list[0].GetSize() && i1<list[1].GetSize())
			{
			int cmp = cmpsymmarkers(&list[0][i0], &list[1][i1]);
			if (cmp<0)
				{
				onlysym[0].push(list[0][i0++].Line());
				continue;
				}
			if (cmp>0)
				{
				onlysym[1].push(list[1][i1++].Line());
				continue;
				}
			
			CString line0 = list[0][i0++].Line(); line0.Replace(" ", ""); line0.Replace("\r", "");
			CString line1 = list[1][i1++].Line();line1.Replace(" ", ""); line1.Replace("\r", "");
			if (strcmp(line0, line1)!=0)
				++diff;
			}
		while (i0<list[0].GetSize())
			onlysym[0].push(list[0][i0++].Line());
		while (i1<list[1].GetSize())
			onlysym[1].push(list[1][i1++].Line());

		int only0 = onlysym[0].length();
		int only1 = onlysym[1].length();
		if (only0>0 || only1>0 || diff>0)
			{
			Log(LOGINFO, "%s: <=%d >=%d !=%d", file, only0, only1, diff);
			if (INVESTIGATE>0)
				{
				for (int i=0; i<only0; ++i)
					Log(LOGINFO, "sym <= %s", onlysym[0][i]);
				for (int i=0; i<only1; ++i)
					Log(LOGINFO, "sym >= %s", onlysym[1][i]);
				}
			}
		tonly0 += only0; if (only0>0) tfonly0++;
		tonly1 += only1; if (only1>0) tfonly1++;
		tdiff += diff; if (diff>0) tfdiff++;
		}
	while (i0<filelist[0].GetSize())
		only[0].push( filelist[0][i0++].id );
	while (i1<filelist[1].GetSize())
		only[1].push( filelist[1][i1++].id );

	if (INVESTIGATE>0)
		for (int i=0; i<only[0].length(); ++i)
			Log(LOGINFO, "file <= %s", only[0][i]);

	//if (INVESTIGATE>0)
		for (int i=0; i<only[1].length(); ++i)
			Log(LOGINFO, "file >= %s", only[1][i]);

	Log(LOGINFO, "files <=%d >=%d filesym <=%d >=%d !=%d sym <=%d >=%d !=%d", only[0].length(), only[1].length(), tfonly0, tfonly1, tfdiff, tonly0, tonly1, tdiff);
}



void CopyCSV(const char *from, const char *to)
{
	vara filelist;
	CBox cb(Savebox);
	for (LLRect *box =cb.Start(); box!=NULL; box=cb.Next())
		{
		CString file = MkString("%s\\%s" CSV, from, cb.file());
		if (CFILE::exist(file))
			filelist.push( file );
		}

	CopyFiles(to, filelist);
}

void CopyDEM(const char *demfile, const char *to)
{
	vara filelist;
	CBox cb(Savebox);
	CSymList dem;
	dem.Load(demfile);
	if (!dem.GetSize())
		{
		Log(LOGERR, "Could not load %s", demfile);
		return;
		}

	for (int i=0; i<dem.GetSize(); ++i)
		{
		CSym &sym = dem[i];
		LLRect bbox(sym.GetNum(1), sym.GetNum(2), sym.GetNum(3), sym.GetNum(4));
		if (Savebox->Intersects(bbox))
			if (sym.GetNum(DEM_RES)>1.5)
				filelist.push( dem[i].id );
		}
	CopyFiles(to, filelist);
}





void ElevationFlush()
{
	//rivercache.Flush();
}












// ================ SCAN =============



extern int MODE;

static 	LLEProcess wrproc, wcproc, gaproc, gxproc;

static void wcprocLoad(LLRect *bbox)
{
		// cache grid files
		static vara wcprocfiles;
		if (!bbox)
			{
			wcprocfiles.RemoveAll();
			wcproc.Reset();
			return;
			}

		CBox cb(bbox, POIGRIDSIZE); cb.Start(); 
		CString gridfile = cb.file();
		if (wcprocfiles.indexOf(gridfile)>=0)
			return;
		wcprocfiles.push(gridfile);

		// Load new data
		CSymList wclist;
		vara paths("PANOR,PANORTAG,FLICKR,FLICKRTAG");
		DIRMarkerList(wclist, paths, bbox, W);
		wcproc.Load(wclist, FALSE);
}

#define SCANSIG "SCAN"
typedef struct {
	char sig[4];
	int size, nsize;
	int progress, progressmax;
	double demdate;
	double reserved1;
	double reserved2;
	double reserved3;
} scanfile;



class CScanList
{
	#define SCANSIGNATURE 0xCACBFA13

	CArrayList<CScan> slist;
	CArrayList<CScan> nlist; // fake list to aid GOOGDATA
	//CArrayList<CScan*> ptrlist;
	CStringArrayList namelist;
	int errors;
	int progress[2];
	vars file;
	double demdate;

public:
	inline CScan& operator[](int i) { return slist[i]; }
	inline int GetSize(void) { return slist.GetSize(); }

	CScanPtr AddScan()
	{
		return slist.AddTail();
	}

	CScanPtr AddScan(double id, double oid, int split, CanyonMapper &cm, const char *name = NULL)
	{ 
		CScan *oldscan = Find(id);
		if (oldscan && oldscan->oid==oid)
			Log(LOGALERT, "Adding duplicate scan %s %s %s", file, IdStr(id), IdStr(oid));

		ASSERT(IdStr(id)!="TESTID");

		CScanPtr ptr = slist.AddTail();
		ptr->Set(ptr, id, oid, split, cm, name!=NULL ? namelist[namelist.AddTail(name)] : "");
		return ptr;
	}

	inline CScanPtr ptr(int n) { return slist.ptr(n); }

	CRiverListPtr SortList(int cmp(CRiver **a1, CRiver **a2))
	{
		// map elist
		CRiverListPtr elist;
		for (int i=0; i<slist.GetSize(); ++i)
			if (slist[i].id!=0 && slist[i].trk)
				elist.push(slist[i].trk);
		elist.Sort(cmp);
		return elist;
	}

	CScanList(void)
	{
		progress[0] = 0, progress[1] = 1;
	}

	void Dim(int preallocate)
	{
		slist.SetSize(preallocate);
		slist.SetSize(0);
	}

	CScan *Find(double id)
	{
		if (id!=0) 
			for (int i=0; i<slist.GetSize(); ++i)
				if (slist[i].id==id)
					return &slist[i];
		return NULL;
	}

	void Reset(void)
	{
		progress[0] = 0, progress[1] = 1;
		slist.Reset();
		namelist.Reset();
	}


	static int cmpid(CScan **s1, CScan **s2)
	{
		if ((*s1)->id > (*s2)->id) return -1;
		if ((*s1)->id < (*s2)->id) return 1;
		return 0;
	}


	int Total(void) {
		return progress[1];
	}

	int Scanned(void) {
		return progress[0];
	}


	int Invalids(int mode = 0) 
	{
		// check errors
		int errors = 0;
		for (int i=0; i<slist.GetSize(); ++i)
			{
			if (slist[i].id!=0)
				if (!slist[i].IsValid(mode))
					++errors;
			}
		return errors;
	}

	BOOL IsValid(int mode = 0)
	{
		if (MODE==2) // reset
			{
			Reset();
			return FALSE;
			}

		// last date of DEM
		LLRect bbox;
		CBox::box(file, bbox);
		double newdemdate = DEMARRAY.DEMDate(bbox);
		if (newdemdate>demdate)
			{
			Log(LOGINFO, "%s: invalid DEM date detected [%s > %s] (%d invalid scans)", file, CDate(newdemdate), CDate(demdate), GetSize());
			Reset();
			return FALSE;
			}
		// recompute errors (if needed)
		int invv = Invalids(INVVERSION), invg = Invalids(INVGOOG);
		if (progress[0]<progress[1])
			{
			Log(LOGINFO, "%s: incomplete scans detected [%d/%d] (InvV:%d InvG:%d)", file, progress[0], progress[1], invv, invg);
			return FALSE; // incomplete			
			}

		int inv = Invalids(mode);
		if (inv>0)
			{
			Log(LOGINFO, "%s: invalid scans detected [%d/%d] (InvV:%d InvG:%d)", file, GetSize()-inv, GetSize(), invv, invg);
			return FALSE;
			}

		if (MODE!=-1) // recompute
			return FALSE; 

		// ok
		return TRUE;
	}


	void Optimize(CScanListPtr &ptrlist, BOOL warn = TRUE)
	{
		CScanListPtr newlist; 
		newlist.Dim(slist.GetSize());
		for (int i=0; i<slist.GetSize(); ++i)
			if (slist[i].id!=0)
				newlist.AddTail(&slist[i]);
	
		newlist.Sort(cmpid);	
		ptrlist.Sort(cmpid);

		int max = newlist.GetSize();
		for (int i=0, j=0; i<max && j<ptrlist.GetSize(); )
			{
			switch (cmpid(&newlist[i], &ptrlist[j]))
				{
					case 0:
						// duplicate id -> merge
						{
						CScan *pscan = ptrlist[j], *scan = newlist[i];
						vara diff = pscan->Merge(scan);
						if (warn || diff.length()>0)
							{
							Log(diff.length()>0 ? LOGERR : LOGWARN, "Duplicate Scan id #%d (%s) %s", i, IdStr(scan->id), diff.length()>0 ? diff.join(", ") : "");
							Log(pscan->Summary());
							}
						}
						i++;
						break;

					case -1:
						// new
						ptrlist.AddTail(newlist[i++]);
						break;

					case 1:
						// maybe new?
						++j;
						break;
				}

			}
	// add rest
	while (i<max)
		ptrlist.AddTail(newlist[i++]);

#ifdef DEBUGXXX
	ptrlist.Sort(cmpid);
	int size = ptrlist.GetSize();
	for (int i=1; i<size; ++i)
		ASSERT(ptrlist[i]->id<ptrlist[i-1]->id);
			
#endif
	}

	double Progress(void)
	{
		if (progress[1]>0)
			if (progress[0]>=progress[1])
				return 1;
			else
				return progress[0]/(double)progress[1];
		return 0;
	}



	int Load(const char *file, BOOL tmp = FALSE, BOOL trk = FALSE)
	{
		this->file = file;
		progress[0] = 0, progress[1] = 1;

		// load from SCAN, otherwise from SCANTMP
		CString filepath = file;
		if (!strstr(file, "\\"))
			{
			// auto load
			if (tmp) // TMP is only used when scanning
				if (Load(scanname(file, SCANTMP), tmp, trk))
					{
					if (trk)
						Load(scanname(file, SCANFINAL), tmp, -1);
					return TRUE;
					}
			return Load(scanname(file, SCANFINAL), tmp, trk);
			}

		// check if file exists
		if (!CFILE::exist(filepath))
			return FALSE;

		CFILE f, tf;
		if (!f.fopen(filepath))
			{
			Log(LOGALERT, "Cannot open SCAN file %s", f.name());
			return FALSE;
			}
		// load counter
		// save header
		scanfile stub;
		if (!f.fread(&stub, sizeof(stub), 1))
			return FALSE;
		if (strncmp(stub.sig, SCANSIG, 4)!=0 || stub.size<0 || stub.size>5000000 || stub.nsize<0 || stub.nsize>5000000)
			{
			Log(LOGALERT, "Corrupted SCAN file %s: bad stub data", f.name());
			return FALSE;
			}
		demdate = stub.demdate;

		int psize = stub.size, nsize = stub.nsize;

		if (trk && psize>0)
			{
			if (!tf.fopen(TrkFile(file, filepath)))
				{
				Log(LOGALERT, "Cannot open TRK file %s", f.name());
				return FALSE;
				}
			}


		// name list
		char name[256];
		namelist.SetSize(nsize);
		for (int i=0; i<nsize; ++i)		
			{
			unsigned char nlenc = 0;
			if (!f.fread(&nlenc, sizeof(nlenc), 1))
				{
				Log(LOGALERT, "Corrupted SCAN file %s: cannot read name data #%d", f.name(), i);
				return FALSE;
				}
			if (nlenc>0)
				if (!f.fread(name, nlenc, 1))
					{
					Log(LOGALERT, "Corrupted SCAN file %s: bad name data #%d", f.name(), i);
					return FALSE;
					}
			name[nlenc] = 0;
			namelist[i] = htmltrans(name);
			}

		// fake load for MODE 1
		CArrayList<CScan> &list = trk<0 ? nlist : slist;

		// load CScans
		list.SetSize(psize);
		for (int n = 0; n<psize; ++n)
		{
			// load CScans
			CScan *scan = &list[n];
			if (!scan->Load(f, tf, namelist))
				{
				Reset();
				Log(LOGALERT, "Corrupted SCAN file %s: bad scan data #%d", f.name(), n);
				return FALSE;
				}
			scan->file = this->file;			
			//ASSERT(!IsSimilar(scan->name, "Cripple"));
		} 

		// validation check
		int signature = 0;
		if (!f.fread(&signature, sizeof(signature), 1) || signature!=SCANSIGNATURE)
			{
			Reset();
			Log(LOGALERT, "Corrupted SCAN file %s: mismatched end signature", f.name());
			return FALSE;
			}

		if (trk)
			{
			int errors = 0;
			for (int i=0; i<list.GetSize() && !errors; ++i)
				if (!GOOGDATA.Add(&list[i]))
					++errors;
			GOOGDATA.Sort();
			// if errors detected, reset GOOGDATA
			if (errors>0)
				{
				GOOGDATA.Reset();
				Log(LOGWARN, "Invalid GOOGDATA for ", f.name());
				}
			}

	
		if (!scangkey->Load(f))
			{
			Reset();
			Log(LOGALERT, "Corrupted SCAN file %s: bad scankey data", f.name());
			return FALSE;
			}

		signature = 0;
		if (!f.fread(&signature, sizeof(signature), 1) || signature!=SCANSIGNATURE)
			{
			Reset();
			Log(LOGALERT, "Corrupted SCAN file %s: mismatched end signature2", f.name());
			return FALSE;
			}

		progress[0] = stub.progress, progress[1] = stub.progressmax;

#if 0
		// special validations / conversions
		int raps = 0, sideg = 0, sidegerr = 0;
		for (int i=0; i<slist.GetSize(); ++i)
		{
		CScan &scan = slist[i];
		for (int r=0; r<scan.rappels.GetSize(); ++r)
				{
				++raps;
				unsigned char sg = scan.rappels[r]->rflags[RSIDESG];
				if (sg>0)
					++sideg;
				if (sg>4)
					++sidegerr;
				if (sg<0)
					++sidegerr;
				}
		}

		if (raps>10 && (sideg==0 || sidegerr>0))
			{
			Log(LOGERR, "Invalid file %s - sideg %d, sidegerr %d", file, sideg, sidegerr);
			CFILE f;
			if (f.fopen("redo.csv", CFILE::MAPPEND))
				f.fputstr(file);
			return FALSE;
			}
#endif

		return TRUE;
	}


	int Save(const char *file, int count, int countmax)
	{
		CScanListPtr ptrlist;
		Optimize(ptrlist);

		CFILE f, tf;
		int done = count==countmax;
		vars filepath = scanname(file, done ? SCANFINAL : SCANTMP);
		if (!f.fopen(filepath, CFILE::MWRITE))
			return FALSE;
		if (!tf.fopen(scanname(GetFileName(file)+".trk", done ? SCANTRK : SCANTMP ), CFILE::MWRITE))
				return FALSE;

		// save name list
		namelist.Sort();
		// find used names
		int size = 0;
		CStringArrayList namesavelist;
		for (int i=0; i<slist.GetSize(); ++i)
			if (slist[i].id!=0)
				{
				++size;
				const char *name = slist[i].name;
				if (name!=NULL && namesavelist.Find(name)<0)
					namesavelist.AddTail(name);
				}	
		// save name list
		int nsize = namesavelist.GetSize();

		// save header
		scanfile stub;
		strncpy(stub.sig, SCANSIG, 4);
		stub.progress = count;
		stub.progressmax = countmax;
		stub.size = size;
		stub.nsize = nsize;
		// last date of DEM
		LLRect bbox;
		CBox::box(file, bbox);
		stub.demdate = DEMARRAY.DEMDate(bbox);
		if (!f.fwrite(&stub, sizeof(stub), 1))
			return FALSE;

		for (int i=0; i<nsize; ++i)		
			{
			int nlen = namesavelist[i].GetLength();
			if (nlen>0xFF) nlen = 0xFF;
			unsigned char nlenc = (unsigned char)nlen;
			if (!f.fwrite(&nlenc, sizeof(nlenc), 1))
				return FALSE;
			if (nlen>0)
				if (!f.fwrite(namesavelist[i], nlen, 1))
					return FALSE;
			}

		// save CScans
		CDoubleArrayList idlist;
		for (int i=0; i<slist.GetSize(); ++i)
		 if (slist[i].id!=0)
		  {
		  int iname = -1;
		  CScan *scan = &slist[i];
		  ASSERT(IdStr(scan->id)!="TESTID");
		  //ASSERT(IdStr(scan->id)!="ec3a99515290f5c1");
		  const char *name = slist[i].name;
		  if (name)
			 {
		     iname = namesavelist.Find(name);
			 ASSERT(iname>=0);
			 }
		  if (!scan->Save(f, tf, iname))
			return FALSE;
		  }

		int signature = SCANSIGNATURE;
		if (!f.fwrite(&signature, sizeof(signature), 1))
			return FALSE;

		if (!scangkey->Save(f))
			return FALSE;

		if (!f.fwrite(&signature, sizeof(signature), 1))
			return FALSE;

		Log(LOGINFO, "Saved SCAN %s %s [%d+%d-%d GOOG calls]", file, done? "DONE!" : MkString("%d%% %d/%d", (count*100)/(countmax+1), count, countmax), scangONkey.calls, GOOGDATA.calls, GOOGDATA.missed);

		if (done)
		  {
		  DeleteFile(scanname(file, SCANTMP));
		  DeleteFile(scanname(GetFileName(file)+".trk", SCANTMP));
		  }

		return TRUE;
	}

	int SaveCSV(const char *file)
	{
		/*
		CFILE f;
		//CString filepath = filename(CString(file)+"csv", count<0 ? SCAN : SCANTMP);
		if (!f.fopen(file, CFILE::MWRITE))
			return FALSE;
		// save counter
		*/

		CSymList list;
		// save CScans
		for (int i=0; i<slist.GetSize(); ++i)
		  {
		  CScan *scan = &slist[i];
		  if (scan->id==0)
			continue;
		  // save performed scans
		  //f.fputstr(scan->Summary());
		  list.Add(CSym(IdStr(scan->id)+"."+IdStr(scan->oid), scan->Summary()));
		  }

		if (TMODE==0)
			list.Sort();

		list.Save(file);

		return TRUE;
	}

};





void LLEProcess::Load(CScanList &scanlist)
{		
		// compute num of 
		int n = 0;
		for (int i=0, in=scanlist.GetSize(); i<in; ++i)
		 if (scanlist[i].id!=0)
			n += scanlist[i].rappels.GetSize();

		Dim(n);

		// load data
		for (int i=0, in=scanlist.GetSize(); i<in; i++)
		 if (scanlist[i].id!=0)
		  for (int j=0, jn=scanlist[i].rappels.GetSize(); j<jn; j++)
			{
			Add(scanlist[i].rappels[j]->lle, scanlist[i].rappels[j]);
			scanlist[i].rappels[j]->scan = scanlist.ptr(i);
			}
		  
		 // Calc
		 Sort();
}

void LLEProcess::Load(CScanListPtr &scanlist)
{		
		// compute num of 
		int n = 0;
		for (int i=0, in=scanlist.GetSize(); i<in; ++i)
		 if (scanlist[i]->id!=0)
			n += scanlist[i]->rappels.GetSize();

		Dim(n);

		// load data
		for (int i=0, in=scanlist.GetSize(); i<in; i++)
		 if (scanlist[i]->id!=0)
		  for (int j=0, jn=scanlist[i]->rappels.GetSize(); j<jn; j++)
			{
			Add(scanlist[i]->rappels[j]->lle, scanlist[i]->rappels[j]);
			scanlist[i]->rappels[j]->scan = CScanPtr(&scanlist[i], 0); // scanlist.ptr(i);
			}
		  
		 // Calc
		 Sort();
}


CString CScan::Summary(void)
{		  
		  // save performed scans
		  vara raps, down;
		  for (int j=0; j<downconn.GetSize(); ++j)
			  down.push(IdStr(downconn[j]));
		  for (int j=0; j<rappels.GetSize(); ++j)
			  raps.push(MkString("R%d(%s):[",j,rappels[j]->Id())+vars(rappels[j]->Summary()).replace(","," ")+"]");
			  
		  vara out;
		  out.push( MkString("%s,#%d,%s,%s,%s,%skm2,%scfs,%sC,v%d,%s,THISF%s,UPF%s,DNF%s,ERRF%s", name ? name : "(NULL)", getsplit(), IdStr(id), IdStr(oid), mappoint.Summary(TRUE), CData(km2), CData(cfs), CData(temp), version, CDate(date), FlagStr(flags[ETHIS]), FlagStr(flags[EUP]), FlagStr(flags[EDN]), FlagStr(geterror())));
		  out.push(MkString("%dr",rappels.GetSize()));
		  out.push( raps.join(";") );
		  out.push(MkString("%dL",down.GetSize()));
		  out.push( down.join(";") );
		  return out.join(",");
}


#define DRIPMULN 4				// multiplier
#define DRIPANGLEN 50			// 360/50
#define DRIPSPLASH 100		// 100m
#define DRIPDIST (MINSTEP+MINSTEP/2.0)	// 10m
#define DRIPOVERLAP 10			// min 5 overlap
//#define DRIPLOOP 5				// refine 5 times
#define DRIPMAXDIST 50			// 50m
#define DRIPSTEP (DRIPDIST*2)	// 15m




void WaterSimulation(double lat, double lng, CKMLOut *out, LLEProcess *proc)
{
			tcoord coord(lat, lng);
			// 100m splash zone 
			CanyonMapper cm(NULL, "Splash", "NOGOOGLE");
			cm.Map(coord.lat, coord.lng, 0);
			// account only for higher elevations (drip zone)
			if (cm.trk.GetSize()>1) 
				if (proc)
					proc->Load(cm.trk);
			//out += KMLMarkerNC("pin", newcoord.lat, newcoord.lng, "pin");
			if (out && cm.trk.GetSize()>1)
				{
				vara plist;
				for (int j=0; j<cm.trk.GetSize(); ++j)
					plist.push(CCoord3(cm.trk[j].lat, cm.trk[j].lng));
				*out += KMLLine("Splash", NULL, plist, RGBA(0x80, 0x00, 0, 0xFF), 5);
				}
}

//#define OLDLOCATE // use old algorithm

void RainSimulation(double lat, double lng, CKMLOut *out, LLEProcess *proc)
{
		tcoord coord(lat, lng);
		for (int m=0; m<DRIPMULN; ++m)
		  for (int i=0; i<DRIPANGLEN; ++i)
			{
			// calct splashpoint
			tcoord newcoord;
			double distmul = 2;
			double dist = (m+1)*4*DRIPSPLASH/(DRIPMULN+1);
#ifdef OLDLOCATE
			if (proc)
				{
				distmul = 2/3.0;
				dist = (double)DRIPSPLASH/(m+1);
				}
#endif
			CoordDirection(coord, (i*2*M_PI)/DRIPANGLEN, dist, newcoord);

			// 100m splash zone 
			CanyonMapper cm(NULL, "Splash", "NOGOOGLE");
			cm.Map(newcoord.lat, newcoord.lng, dist * distmul);
			// account only for higher elevations (drip zone)
			if (cm.trk.GetSize()>1) 
				if (proc)
					proc->Load(cm.trk);
			//out += KMLMarkerNC("pin", newcoord.lat, newcoord.lng, "pin");
			if (out && cm.trk.GetSize()>1)
				{
				vara plist;
				for (int j=0; j<cm.trk.GetSize(); ++j)
					plist.push(CCoord3(cm.trk[j].lat, cm.trk[j].lng));
				*out += KMLLine(MkString("Splash#%d %dp",i,cm.trk.GetSize()), NULL, plist, RGBA(0x40, 0x00, 0, 0xFF), 5);
				}
			}
}

static int cmprivercnt(LLEC **lle1, LLEC **lle2)
{
		if ((*lle1)->ptr > (*lle2)->ptr) return -1;
		if ((*lle1)->ptr < (*lle2)->ptr) return 1;
		return 0;
}

static int findmaxhpoint(LLEC *r, LLEC *lle, double maxdist, void *data)
{
	LLE *maxhpoint = (LLE *)data;
	if (r!=lle)
	  if (r->elev > maxhpoint->elev)
	    if (LLE::Distance(r, lle)<maxdist)
			r->ptr = &r->river()[1]; 
	return FALSE;
}

static int findmaxhconn(LLEC *r, LLEC *lle, double maxdist, void *data)
{
	if (r!=lle)
	    if (LLE::Distance(r, lle)<maxdist)
			r->conn.AddTail(lle); 
	return FALSE;
}

static int findmaxhsplash(LLEC *r, LLEC *lle, double maxdist, void *data)
{
	LLEC *maxhpoint = (LLEC *)data;
	if (lle->elev > r->elev)
		if (lle->conn.GetSize() > maxhpoint->conn.GetSize())
			if (LLE::Distance(r, lle)<maxdist)
				if (LLE::Distance(r, lle)>maxdist/2)
					{
					*maxhpoint = *lle; 
					maxhpoint->conn = lle->conn; 
					}
	return FALSE;
}


int LocateRogue(LLE &lle, CKMLOut *out)
{
	if (out)
		{
		*out+=KMLMarkerStyle("pin", "http://maps.google.com/mapfiles/kml/paddle/red-circle-lv.png");
		*out+=KMLMarkerStyle("new", "http://maps.google.com/mapfiles/kml/paddle/red-circle.png");
		*out+=KMLMarkerStyle("old", "http://maps.google.com/mapfiles/kml/shapes/placemark_circle.png", 0.5);
		}

	// allow to drop for first step

#ifdef OLDLOCATE
	lle.elev = 0;
	for (int n=0; n<DRIPLOOP; ++n)
	{
		// compute splash zone
		LLEProcess proc;
		RainSimulation(lle.lat, lle.lng, out, &proc);

		// find highest elevation common point (drip zone)
		LLECLIST &llelist = proc.LLElist();
		proc.MatchLLE(llelist, DRIPDIST, findmaxhpoint, (void *)&lle);

		llelist.Sort(cmprivercnt);
		if (llelist.GetSize()>0)
			if (llelist.Head()->ptr!=NULL)
				lle = *llelist.Head();
	if (out)
		if (n==DRIPLOOP-1)
				*out += KMLMarkerNC("new", lle.lat, lle.lng, "new");
			else
				*out += KMLMarkerNC("old", lle.lat, lle.lng, "old");
	}
#else
	// find starting point to flow up
	const char *name = "";
	if (out) name = out->name;


	LLE olle, start;
	CanyonMapper cm(NULL, "Splash", "NOGOOGLE");
	cm.Map(lle.lat, lle.lng, DRIPMAXDIST);
	if (cm.trk.GetSize()<=1) 
		return FALSE;
	olle = cm.trk.Head();
	start = cm.trk.Tail();
	if (olle.elev<0)
		return FALSE;

	// compute splash zone
	LLEProcess proc;
	RainSimulation(olle.lat, olle.lng, out, &proc);

	LLECLIST &llelist = proc.LLElist();
	proc.MatchLLE(llelist, DRIPDIST, findmaxhconn, NULL);

	LLEC newllec;
	LLEC llec = lle; //start;
	LLECLIST one; one.AddTail(&llec);

	for (int i=1; i*DRIPSTEP<DRIPMAXDIST; ++i)
	{
	llec.elev = 0;
	newllec = llec; newllec.conn.Reset();
	proc.MatchLLE(one, i*DRIPSTEP, findmaxhsplash, (void *)&newllec);
	if (newllec.conn.GetSize()>=DRIPOVERLAP)
		break;
	}

	if (newllec.conn.GetSize()<DRIPOVERLAP)
		{
		Log(LOGERR, "Bad LocateRogue start? %s -x-> %s [%s]", olle.Summary(TRUE), newllec.Summary(TRUE), name);
		return FALSE; // use original lle
		//proc.MatchLLE(one, DRIPSPLASH, findmaxhsplash, (void *)&newllec);
		}

	llec = newllec;

	// loop up river
	for (int n=0; n<100; ++n)
	{
	// find first point
	newllec = llec; newllec.conn.Reset();
	proc.MatchLLE(one, DRIPSTEP, findmaxhsplash, (void *)&newllec);

	if (newllec.conn.GetSize()<DRIPOVERLAP)
		{
		if (newllec.elev < olle.elev )
			Log(LOGERR, "Bad LocateRogue end? %s --> %s [%s]", olle.Summary(TRUE), llec.Summary(TRUE), name);
		break;
		}

	// good results
	llec = newllec;
	if (newllec.elev > olle.elev && LLE::Distance(&newllec, &olle)>DRIPMAXDIST*2)
		break;

		/*
		llelist.Sort(cmprivercnt);
		if (llelist.GetSize()>0)
			if (llelist.Head()->ptr!=NULL)
				lle = *llelist.Head();
		*/
	if (out)
			*out += KMLMarkerNC("old", llec.lat, llec.lng, "old");
	}
	if (out)
		*out += KMLMarkerNC("new", llec.lat, llec.lng, "new");
	lle = llec; // use new lle

	// double check
	cm.Reset("SplashCheck");
	cm.Map(lle.lat, lle.lng, DRIPMAXDIST*4);
	if (DistanceToLine(olle.lat, olle.lng, cm.trk)>DRIPMAXDIST)
		{
		Log(LOGERR, "Mispositioned LocateRogue? %s -x-> %s [%s]", olle.Summary(TRUE), llec.Summary(TRUE), name);
		lle = olle;
		}

#endif
	return TRUE;
}

void TestRogues(void)
{
#ifdef DEBUG
	 
		LocateRogue(LLE(45.573334,-122.133057), &CKMLOut("test3_123_35_Mist"));
		LocateRogue(LLE(46.932259, -121.766541), &CKMLOut("test1Top")); // strange stuff
		LocateRogue(LLE(46.859631,-121.684990), &CKMLOut("test1Glacier")); // strange stuff
		LocateRogue(LLE(45.634232,-122.106598), &CKMLOut("test1Cynthia")); // strange stuff
		LocateRogue(LLE(45.629900,-122.110140), &CKMLOut("test2Archer")); // archer
		LocateRogue(LLE(45.911282,-122.178703), &CKMLOut("test4_123_14"));
		LocateRogue(LLE(45.782219,-122.021942), &CKMLOut("test5_123_15")); // wall behind falls
		// Test2
		LocateRogue(LLE(45.915722,-122.183952), &CKMLOut("test6_unk"));		
		LocateRogue(LLE(45.711590,-121.663582), &CKMLOut("test7_cook"));
		LocateRogue(LLE(45.367210,-121.645088), &CKMLOut("test8_122_4"));
		//ASSERT(!"FINISHED");
#endif
}



class CScanProcess
{
	// all objects here for clean crash recovery
	vars file;
	LLRect bbox;
	CScanList scanlist; 
	CScanList loadlist;
	CRiverCache *rcache;
	CRiverList trklist;
	LLEProcess rapproc;
	CRiverListPtr elist;
	int rprogressmax, rprogress;
	int scanchanges;

public:

	static int cmpscan(CScan *A, CScan *B)
	{
		if ((A)->id > (B)->id) return 1;
		if ((A)->id < (B)->id) return -1;
		return 0;
	}




static int scanrstamp(CRiver *r, LLEC *lle, double maxdist, void *data)
{
	if (r->scan==NULL)
		return TRUE;
	if (r->DistanceToLine(lle->lat, lle->lng)<maxdist)
		{
		// set rappel flag
		ASSERT(IdStr(r->id)!="R54e375f8d78ed341");
		setflag(r->scan->flags[ETHIS], (int)data);
		return TRUE;
		}
	return FALSE;
}

static int scanlstamp(LLEC *r, LLEC *lle, double maxdist, void *data)
{
	if (LLE::Distance(r, lle)<maxdist)
		{
		// set rappel flag
		trappel *rap = r->rappel();
		setflag(rap->rflags[RTHIS], (int)data);
		return TRUE;
		}
	return FALSE;
}


static int connectpoi(LLEC *r, LLEC *lle, double maxdist, void *data)
{
	// connect waterfalls withing MAXWGDIST
	double dist = LLE::Distance(r, lle);
	if (dist<maxdist)
		r->conn.AddTail(lle); // CONNECTED
#if 0 // problem with MIST FALLS
	else
		if (dist<MINRIVERPOINT)
			{
			CString &rid = r->sym()->id;
			CString &lleid = lle->sym()->id;
			if (!rid.IsEmpty() && !lleid.IsEmpty() && !IsSimilar(rid, "Unnamed") && !IsSimilar(lleid, "Unnamed"))
				if (IsSimilar(rid, lleid) || IsSimilar(lleid, rid))
				{
				if (INVESTIGATE>=0)
					Log(LOGINFO, "Extended match for %s === %s", r->sym()->Line(), lle->sym()->Line());
				r->conn.AddTail(lle); // CONNECTED
				}
			}
#endif
	return FALSE;
}

static void LoadPOI(void)
{
		// global
		if (wrproc.LLESize()==0)
			{
			static CSymList wrlist; // keep access to symlist for name
			LoadWaterfalls(wrlist);
			wrproc.Load(wrlist);
			LLECLIST &list = wrproc.LLElist();
			// set up poi connections (there might be duplicates)
			wrproc.MatchLLE(list, MAXWGDIST, connectpoi, NULL);
			}
		if (gaproc.LLESize()==0)
			{
			CSymList galist;
			LoadWaterflow(galist, TRUE);
			gaproc.Load(galist);
			}
		if (gxproc.LLESize()==0)
			{
			CSymList gxlist;
			LoadWaterflow(gxlist, FALSE);
			gxproc.Load(gxlist);
			}

}



static int ScanRiver(CRiver &river, rproc *proc)
	{
		LoadPOI();

		// dynamic
		wcprocLoad(&LLRect(river.SE[0].lat, river.SE[0].lng, river.SE[1].lat, river.SE[1].lng)); 

		// stamp river segments
		wcproc.MatchBox(river, MAXONRAPDIST, proc, (void*)EWC);
		wrproc.MatchBox(river, MAXWGDIST, proc, (void*)EWR);
		gaproc.MatchBox(river, MAXWGDIST, proc, (void*)EGA);
		gxproc.MatchBox(river, MAXWGDIST, proc, (void*)EGX);
		return TRUE;
	}

int ScanRivers(CRiverList &riverlist, CScanList &scanlist, LLRect *bbox, rproc *proc)
	{
		LoadPOI();

		// dynamic
		wcprocLoad(bbox); 

		// stamp river segments
		wcproc.MatchBox(&riverlist, MAXWGDIST, proc, (void*)EWC);
		wrproc.MatchBox(&riverlist, MAXWGDIST, proc, (void*)EWR);
		gaproc.MatchBox(&riverlist, MAXWGDIST, proc, (void*)EGA);
		gxproc.MatchBox(&riverlist, MAXWGDIST, proc, (void*)EGX);

		// stamp tracks
		CRiverListPtr list = scanlist.SortList(CRiverList::cmpbox);
		wcproc.MatchBox(list, MAXWGDIST, proc, (void*)EWC);
		wrproc.MatchBox(list, MAXWGDIST, proc, (void*)EWR);
		gaproc.MatchBox(list, MAXWGDIST, proc, (void*)EGA);
		gxproc.MatchBox(list, MAXWGDIST, proc, (void*)EGX);
		return TRUE;
	}


void ResetRogues(void)
{
		LLECLIST &llelist = wrproc.LLElist();
		for (int i=0; i<llelist.GetSize(); ++i)
			llelist[i]->lle = NULL;
}


BOOL NeedAbortScan(void)
{
	return TMODE==1 && scangONkey.IsDisabled();
}



// Rogue POI 
static int scanroguerstamp(CRiver *r, LLEC *lle, double maxdist, void *data)
{
	// only map scanned rivers
	if (r->scan==NULL || r->scan->id==0)
		return TRUE;

	// tag waterfalls located 100m from a stream
	double dist = r->DistanceToLine(lle->lat, lle->lng);
	if (dist<maxdist)
		{
		// do not account endcaps (Big waterfall next to Cynthia error)
		if (abs(LLE::Distance(&r->SE[0], lle)-dist)<1 || abs(LLE::Distance(&r->SE[1], lle)-dist)<1)
			{
			//Log(LOGINFO, "Promoting to rogue %s", lle->sym()->Line());
			return FALSE;
			}

		// do not account self waterfall extra
		if (r->id == lle->Id())
			return FALSE;

		//ASSERT(!r->sym || !IsSimilar(r->sym->id, "Mist"));
		lle->lle = lle; // TAGGED

		// try to find name for unnamed scans
		CScan *scan = r->scan;
		if (scan)
			if (scan->name==NULL || *scan->name==0)
				if (lle->sym()!=NULL)
					{
					const char *id = lle->sym()->id;
					if (id!=NULL && *id!=0)
						if (!IsSimilar(id, "unnamed") && !IsSimilar(id, "waterfall"))
							{
							LLECLIST &wrlist = *scan->wrlist;
							// add to possible name list 
#ifdef DEBUGXXX
							Log(LOGINFO, "%s <= %s", r->sym->Line(), lle->sym()->Line());
#endif
							if (wrlist.Find(lle)<0)
								wrlist.AddTail(lle);
							}
					}
		}

	return FALSE;
}

/*
static int scanroguelstamp(LLEC *r, LLEC *lle, void *data)
{
	if (r->lle)
		return TRUE;

	// propagate tags between waterfalls
	if (lle->lle && LLE::Distance(r, lle)<MAXWGDIST)
		{
		//ASSERT(!IsSimilar(r->sym()->id, "Little Jack Falls"));
		r->lle = r; // TAGGED
		return TRUE;
		}
	return FALSE;
}
*/



void StampRogues(LLEProcess &proc)
{
		// stamp used waterfallse lle->lle==lle
		CRiverListPtr slist = scanlist.SortList(CRiverList::cmpbox);
		proc.MatchBox(slist, MAXWGDIST, scanroguerstamp, NULL);

		//proc.MatchLLE(proc.LLElist(), MAXWGDIST, scanroguelstamp, NULL);
		LLECLIST wlist = proc.LLElist();
		for (int i=0; i<wlist.GetSize(); ++i)
		  if (wlist[i]->lle) // tagged?
			{
			LLEC *lle = wlist[i];
			for (int j=0; j<lle->conn.length(); ++j)
				{
				//ASSERT(!strstr(lle->conn[j]->sym()->id, "Mist "));
				lle->conn[j]->lle = lle;				
				}
			}
}


int ScanRogues(void)
{
		const char *WATERFALL = "WATERFALL";
		memorycheck();

		// scan unused (and stamp on the fly)
		int rogue = 0;
		LLRect bbox;
		CBox::box(file, bbox);
		//bbox.Expand(MAXWGDIST);

		// find elev of rogues
		LLECLIST wlist;
		CDoubleArrayList widlist;
		LLECLIST &alllist = wrproc.LLElist();
		for (int i=0; i<alllist.GetSize(); ++i)
			{
			LLEC *lle = alllist[i];
			CSym *sym = lle->sym();
			//ASSERT(!strstr(lle->sym()->id, "Cooper "));
			if (bbox.Contains(lle->lat, lle->lng))
				{
				// substitute for prioretized poi
				lle->conn.Sort(cmpsymdesc);
				lle = lle->conn.Head();			
#if 0
				if (alllist[i]!=lle)
					{
					Log(LOGINFO, "SUBST FRM: %s", wlist[i]->sym()->Line());
					Log(LOGINFO, "SUBST TO:  %s", lle->sym()->Line());
					}
#endif
				// add only if not already added
				double id = lle->Id();
				if (widlist.Find(id)<0)
					{
					lle->GetElev();
					wlist.AddTail(lle);
					widlist.AddTail(id);
					}
				}
			}

		// validate waterfall scans
		for (int i=0; i<scanlist.GetSize(); ++i)
			if (scanlist[i].id!=0 && scanlist[i].oid==0)
				{
				// waterfalls
				if (!scanlist[i].IsValid())
					Invalidate(&scanlist[i]);
				else if (widlist.Find(scanlist[i].id)<0)
					Invalidate(&scanlist[i]);
				else if (MODE==1)
					Invalidate(&scanlist[i]);
				}

		// stamp valid scans
		StampRogues(wrproc);

		// process rogues IN ORDER
		wlist.Sort(LLE::cmpelev);
		for (int i=0; i<wlist.GetSize(); ++i)
			{
			LLEC *lle = wlist[i];
#ifdef DEBUG
			// check ids are ok
			vars id1= IdStr(lle->Id());
			LLE lle2; lle2.Scan(lle->Summary()); 
			vars id2= IdStr(lle2.Id());
			ASSERT(id1==id2);

			CSym *sym = lle->sym();
			//ASSERT(!strstr(lle->sym()->id, "Mist "));
			//ASSERT( IdStr(lle->Id())!="P0ca2fe253522f7c1");
#endif

			if (lle->lle!=NULL)
				continue; // tagged skip

			// invalidate all scans but the main poi
			for (int l=0; l<lle->conn.GetSize(); ++l)
				lle->conn[l]->lle = lle; // TAGGED

			CScan *scan = scanlist.Find(lle->Id());
			if (scan && scan->IsValid())
				continue; // valid skip

			// process
			lle->lle = lle;
			Invalidate(scan);
			CString id = MkString("%s,%s,%s,%s",lle->sym()->id,IdStr(lle->Id()),CCoord(lle->lat),CCoord(lle->lng));

			// compute start point
			LLE newlle(*lle);
#if 0
#define DEBUGOUT &CKMLOut(MkString("splash_%s_%d_%s", file, rogue, ElevationFilename(htmltrans(lle->sym()->id))))
#else
#define DEBUGOUT NULL
#endif			
			LocateRogue(newlle, DEBUGOUT);

			// check if the new point is on a stream (Lancaster Falls error)
			// skip if located on a stream or close to a waterfall on a stream
			LLEProcess newproc;
			newproc.Add(newlle);
			newproc.Sort();
			StampRogues(newproc);
			if (newproc.LLElist()[0]->lle!=NULL)
				continue; // skip

			++rogue;
			++scanchanges;
			printf("Scanning Rogues %s %d%% %d/%d '%s' [%d+%d-%d GOOG calls]     \r", file, (100*i)/wlist.GetSize(), i, wlist.GetSize(), lle->sym()->id, scangONkey.calls, GOOGDATA.calls, GOOGDATA.missed);
			if (INVESTIGATE>=0)
				Log(LOGINFO, "Scanning Rogues #%s_%d [%d/%d] '%s' -> %s [%d+%d-%d GOOG calls]", file, rogue, i, wlist.GetSize(), id, newlle.Summary(), scangONkey.calls, GOOGDATA.calls, GOOGDATA.missed);

			CanyonMapper cm(NULL, "Scan Rogue Point");
			cm.Map(newlle.lat, newlle.lng, 0);
			if (cm.trk.GetSize()<2) 
				{					
				// try original location
				cm.Map(lle->lat, lle->lng, 0);
				if (cm.trk.GetSize()<2) // just add fake coordinates
					{	
					cm.trk.push(tcoord(0.1,0.1));
					cm.trk.push(tcoord(0.2,0.2));
					}
				}

			CScanPtr scanptr = scanlist.AddScan(lle->Id(), 0, 0, cm, lle->sym()->id);
			// stamp new scan
			wrproc.MatchBox(CRiver(cm.trk, scanptr), MAXWGDIST, scanroguerstamp, NULL);
			setflag(scanptr->flags[ETHIS], EWR);

			if (NeedAbortScan())
				break;
			}
		Log(LOGINFO, "Scanned new %d rogue waterfalls", rogue);

		// now try to find suitable names for unnamed creeks
		int count = 0;
		for (int i=0; i<scanlist.GetSize(); ++i)
		 if (scanlist[i].id!=0)
			{
			CScan *scan = &scanlist[i];
			if (scan->name==NULL || *scan->name==0)
				if (scan->GetName())
					++count;
			}
		Log(LOGINFO, "Named %d unnamed segments to waterfalls", count);

		// cleanup
		return rogue;
	}


int SaveProgress(int count, int countmax, int force = FALSE)
{
	static DWORD lasttick = 0;
	if (!lasttick)
		lasttick = GetTickCount();
	if (GetTickCount()-lasttick > SCANFLUSHTIME*1000 || force)
				{
				scanlist.Save(file, count, countmax);

				/*
				for (int i=0; i<extralist.GetSize(); ++i)
					if (extralist[i].scan==NULL || extralist[i].scan->id==0)
							extralist[i].sym->data = "";
				extralist.Save(extrafile);
				*/

#ifdef DEBUG
				// check Save/Load
				CScanList checklist;
				checklist.Load(scanname(file, SCANTMP), TRUE, FALSE);
				//ASSERT(checklist.GetSize()==scanlist.GetSize());
				for (int j=0, i=0; j<checklist.GetSize(); ++j, ++i)
					{
					while (scanlist[i].id==0) ++i;
					
					ASSERT(checklist[j].id==scanlist[i].id);
					ASSERT(checklist[j].Summary()==scanlist[i].Summary());
					if (checklist[j].Summary()!=scanlist[i].Summary())
						{
						Log(LOGALERT, "Save/Load inconsistency SAVE#%d<>LOAD#%d", i, j);
						if (i>0) 
							Log("SAVE :"+scanlist[i-1].Summary());
						Log("SAVE>:"+scanlist[i].Summary());
						if (i<scanlist.GetSize()) 
							Log("SAVE :"+scanlist[i+1].Summary());
						if (j>0) 
							Log("LOAD:"+checklist[j-1].Summary());
						Log("LOAD>:"+checklist[j].Summary());
						if (j<checklist.GetSize()) 
							Log("LOAD :"+checklist[j+1].Summary());
						}
					//ASSERT(checklist[j].trk!=NULL);
					if (checklist[j].trk && scanlist[i].trk)
						{
						tcoords checktrk = checklist[j].trk->Coords(), scantrk = scanlist[i].trk->Coords();
						if (checktrk.GetSize()!=checktrk.GetSize())
							Log(LOGALERT, "Trk size mismatch!");
						for (int k=0; k<checktrk.GetSize(); ++k)
							if (checktrk[k].lat!=scantrk[k].lat || checktrk[k].lng!=scantrk[k].lng)
								Log(LOGALERT, "Trk point mismatch! %s != %s", checktrk[k].Summary(), scantrk[k].Summary() );
						}
					}
#endif
				lasttick = GetTickCount();
				CTEST("S3");					
				}
		return FALSE;
}



int Map(BOOL delnull = FALSE)
	{
		//rlist.Sort();
		int errors = 0;

		// map scanlist to rlist
		CRiverList &rlist = *rcache->rlist;

		CDoubleArrayList idlist;
		for (int i=0; i<rlist.GetSize(); ++i)
			{
			rlist[i].scan = NULL;

			// check id
			if (idlist.Find(rlist[i].id)<0)
			  {
			  idlist.AddTail(rlist[i].id);
			  }
			else
			  {
			  Log(LOGALERT, "Duplicate River id #%d (%s), should never happen!", i, IdStr(rlist[i].id));
			  rlist[i].id = 0;
			  }
			}

		// validate scans
		CDoubleArrayList widlist;
		LLECLIST &awlist = wrproc.LLElist();
		for (int i=0; i<awlist.GetSize(); ++i)
			widlist.push(awlist[i]->Id());		
		widlist.Sort();

		int invalids = 0;
		for (int i=0; i<scanlist.GetSize(); ++i)
			{
			CScan &scan = scanlist[i];
			if (scan.id==0)
				continue;

			//ASSERT(IdStr(scan.id)!="R105831845273d841");

			// extras are preferent over rivers (for splits)
			CRiver *river = rlist.Find(scan.id);
			CRiver *oriver = rlist.Find(scan.oid);

			// link to itself
			CScanPtr ptr = scanlist.ptr(i);
			if (scan.trk)
				scan.trk->scan = ptr;
			if (river)
				river->scan = ptr;
			if (oriver)
				oriver->splits.push(ptr);

			// rivers
			if (oriver)
				continue;

			// waterfalls
			if (widlist.Find(scan.id)>=0)
				continue;

			// unmatched id! (delete?)
			if (delnull) 
				{
				++invalids;
				if (invalids<5)
					Log(LOGINFO, "Deleted unmapped scan id=%s oid=%s mappoint=%s name='%s'", IdStr(scan.id), IdStr(scan.oid), scan.mappoint.Summary(), scan.name);
				Invalidate(&scan);
				}
			}

		if (invalids>5)
			Log(LOGALERT, "Deleted %d unmapped scans!", invalids);

		// check scans 
		invalids = 0;
		idlist.Reset();
		for (int i=0; i<scanlist.GetSize(); ++i)
		   if (scanlist[i].id!=0)
			{
			// check id
			BOOL invalid = FALSE;
			CScan *scan = &scanlist[i];
			if (idlist.Find(scan->id)<0)
			  {
			  idlist.AddTail(scan->id);
			  }
			else
			  {
			  for (int dup=0; dup<i && scanlist[dup].id!=scan->id; ++dup);
			  if (scanlist[dup].oid == scan->oid)
				{
				++invalids;
				invalid = TRUE;
				if (invalids<5)
					  {
					  Log(LOGALERT, "Cleaning up duplicate Scan id (%s %s) #%d vs #%d (%s %s), should never happen!", IdStr(scanlist[dup].id), IdStr(scanlist[dup].oid), dup, i, IdStr(scan->id), IdStr(scan->oid));
					  Log(scanlist[dup].Summary());
					  Log(scan->Summary());
					  }
				}
			  }
			// check track
		    if (!scan->trk || !scan->trk->IsValid())
				{
				++invalids;
				invalid = TRUE;
				if (invalids<5)
					Log(LOGALERT, "Cleaning up invalid trk scan %s %s at %s", scan->name, IdStr(scan->id), scan->mappoint.Summary());
				}

			// invalidate
			if (invalid)
				{
				++errors;
				if (scan->oid!=0)
					{
					// invalidate original
					CRiver *r = rlist.Find(scan->oid);
					if (r) Invalidate(r->splits);
					Invalidate(scanlist.Find(scan->oid));
					}
				Invalidate(scan);
				}
			}
		if (invalids>5)
			Log(LOGALERT, "Cleaned up %d invalid trk scans", invalids);
		return errors;
	}




static void scanflags(CRiver *r, CRiverListPtr &rlist, int dir, void *data)
	{
		CScanList *scanlist = (CScanList *)data;
		if (r->scan==NULL)
			return;

		// add down connections, within 1 order of magnitude only
		int maxorder = r->Order()+1;
		CDoubleArrayList &dconn = r->scan->downconn;
		if (dir>0) dconn.Reset();

		// first one is segment itself
		for (int j=1; j<rlist.GetSize(); ++j)
			{
			CScan stub;
			CScan *stubptr = &stub;
			CRiver *or = rlist[j];
			BOOL newscan = or->scan==NULL;
			if (newscan)
				{
				// compute for propagation but DO NOT STORE
				//CanyonMapper cm("Scan River:"+or->GetStr(ITEM_SUMMARY));
				//cm.Map(or, or->scan = scanlist->AddTail());
				//or->scan = scanlist->AddTail(NULL);
				or->scan = CScanPtr(&stubptr, 0);
				CScanProcess::ScanRiver(*or, CScanProcess::scanrstamp);
				}
			if (dir>0)
				{
				r->scan->flags[EDN] = r->scan->flags[EDN] | or->scan->flags[ETHIS];
				or->scan->flags[EUP] = or->scan->flags[EUP] | r->scan->flags[ETHIS];
				if (or->Order()>maxorder)
					break;
				dconn.AddTail(or->id);
				}
			if (dir<0)
				{
				r->scan->flags[EUP] = r->scan->flags[EUP] | or->scan->flags[ETHIS];
				or->scan->flags[EDN] = or->scan->flags[EDN] | r->scan->flags[ETHIS];
				}
			if (newscan)
				or->scan = NULL;
			}
	}



int Scan(const char *file)
{
	// foreground scan
	//Sleep(30*1000);
	int ret = 0;	
#ifdef CRASHPROTECTION
#ifndef DEBUG
	__try { 
#endif
#endif
	ret = _Scan(file);
#ifdef CRASHPROTECTION
#ifndef DEBUG
} __except(EXCEPTION_EXECUTE_HANDLER) 
	{ 
	Log(LOGALERT, "SCAN CRASHED!!! %s", file);
	return FALSE;
	}
#endif
#endif
	return ret; 
}



CScanProcess(void)
{
	// avoid google 2nd pass (save on calls)
	rcache = NULL;
}

~CScanProcess(void)
{
	// cleaup
	wcprocLoad(NULL);
	if (rcache)
		{
		CRiverList &rlist = *rcache->rlist;
		for (int i=0; i<rlist.GetSize(); ++i)
			rlist[i].scan = NULL;
		delete rcache;
		}
}




BOOL IsValid(CScanPtrList &extras)
{
	for (int i=0; i<extras.GetSize(); ++i)
		{
		if (!extras[i]->IsValid())
			return FALSE;
		}
	return TRUE;
}

inline void Invalidate(CScan *scan)
{
	if (scan) 
		{
		ASSERT(IdStr(scan->id)!="TESTID");
		scan->id = 0;
		++scanchanges;
		}
}

void Invalidate(CScanPtrList &extras)
{
	for (int i=0; i<extras.GetSize(); ++i)
		Invalidate(extras[i]);
	extras.Reset();
}




void ScanRivers(void)
{
		CRiverList &rlist = *rcache->rlist;

		// Scan rivers (if not scanned already) and keep record of extras
		elist = rlist.SortList(CRiverList::cmpelev);
		for (int i=rprogress=0; i<rlist.GetSize(); rprogress = ++i)
		  {
		  CRiver *river = elist[i];
		  //if (!bbox->Intersects(river->Box()))
		  // continue; // out of bounds
		  
		  // check if scan already exist and is valid
		  if (river->scan->IsValid() && IsValid(river->splits))
				continue;
/*
#ifdef DEBUGXXX
		  vars test = "e3a50f96c802e741";
		  if (test.IsEmpty() || IdStr(river->id)!=test)
#endif
		  //ASSERT(IdStr(river->id)!="7f6a0c45285fcb41");
*/


		  ASSERT(IdStr(river->id)!="R105831845273d841");

		  // compute
		  ++scanchanges;
		  Invalidate(river->splits);
		  Invalidate(river->scan);

		  //ASSERT(IdStr(river->id)!="R643bfb9fd4d3e041");

	      CSym split;		  
		  CRiver stub;
		  CRiver *oriver = river;
		  vars name = oriver->sym->id;
		  CArrayList<tpoints> trks;
		  for (int retry=0; river!=NULL && retry<=MAXSPLITS; ++retry)
		  {
			  printf("Scanning Rivers %s %s%s %d%% %d/%d   [%d+%d-%d GOOG calls]\r", file, IdStr(river->id), retry>0 ? MkString("#%d",retry) : "", (i*100)/rlist.GetSize(), i, rlist.GetSize(), scangONkey.calls, GOOGDATA.calls, GOOGDATA.missed);
			  CanyonMapper cm(NULL, "Scan River:"+river->GetStr(ITEM_SUMMARY));
			  BOOL splitted = cm.MapRiver(river, &rlist, -1, &split, retry);
			  trks.push(cm.trk);

			  if (!retry)
				 {
				 CScanPtr *nscan = oriver->splits.push( river->scan = scanlist.AddScan(oriver->id, oriver->id, retry, cm, name) );
				 ASSERT( (*nscan)->IsValid() );
				 }

			  if (retry>0)
				{
				CSym sym;
				if (cm.trk.length()>=2)
					{
					LLE lle(cm.trk[0]);
					LLE lleid; lleid.Scan(lle.Summary()); 
					vars desc = MkString("SPLIT%d:%s=%s@%s", retry, IdStr(oriver->id), IdStr(lleid.Id()), lle.Summary());
					if (retry>MAXSPLITS/2 && INVESTIGATE>=0)
						Log(LOGWARN, "High number of splits for %s = %s", name, desc);
					if (retry==MAXSPLITS)
						{
						// check if there was any real rappel in all the splits
						int realrap = 0;
						for (int i=0; i<trks.GetSize(); ++i)
							{
							tpoints &trk = trks[i];
							for (int j=0; j<trk.GetSize(); ++j)
								if (Rap(trk[j].grade)>1)
									++realrap;
							}
						if (realrap>0 || INVESTIGATE>=0)
							Log(LOGWARN, "MAXED OUT number of splits for %s = %s", name, desc);
						}
					CScanPtr *nscan = oriver->splits.push( scanlist.AddScan(lle.Id(), oriver->id, retry, cm, name) );					
					ASSERT( (*nscan)->IsValid() );
					}
				}

			  // no splits
			  if (!splitted)
				  break;

			  // compute split too
			  stub.Set(&split, 0);
			  river = &stub;
		  }

		  SaveProgress(rprogress, rprogressmax);

		  if (NeedAbortScan())
			  break;
		  }		

}


void ScanWaterfalls(void)
{
		// Waterfalls
		CRiverList &rlist = *rcache->rlist;

		// reset stamps
		CTEST("S6");					
		for (int i=0; i<scanlist.GetSize(); ++i)
		  if (scanlist[i].id!=0)
			{
			CScan *scan = &scanlist[i];
			scan->flags[ETHIS] = scan->flags[EUP] = scan->flags[EDN] = 0;
			for (int j=0, jn = scan->rappels.GetSize(); j<jn; ++j)
				scan->rappels[j]->rflags[RTHIS] = 0;
			}

		// process rogue waterfalls
		ScanRogues();

		// stamp waterfalls/gauges
		ScanRivers(rlist, scanlist, &bbox, scanrstamp);

		// stamp rappels
		rapproc.Reset();
		rapproc.Load(scanlist);
		LLECLIST &raplist = rapproc.LLElist();
		wrproc.MatchLLE(raplist, MAXONRAPDIST, scanlstamp, (void*)EWR);
		wcproc.MatchLLE(raplist, MAXONRAPDIST, scanlstamp, (void*)EWC);
		// post process segments
		for (int i=0; i<scanlist.GetSize(); ++i)
		  if (scanlist[i].id!=0)
			{
			CScan *scan = &scanlist[i];
			// flags propagate to segments (just in case)
			for (int r=0; r<scan->rappels.GetSize(); ++r)
				scan->flags[RTHIS] |= scan->rappels[r]->rflags[RTHIS];
			}
}

void ScanConnection(void)
{
		// set up connections and propagate stamps up to MAXCONNDIST
	    int size = elist.GetSize();
		for (int i=0; i<size; ++i)
			{
			if (i%100==0)
				printf("Scanning %s DCONN %d%% %d/%d   \r", file, (i*100)/size, i, size);
			CRiver *r = elist[i];
			//ASSERT(IdStr(r->id)!="R643bfb9fd4d3e041");
			if (r->scan==NULL || r->scan->id==0)
				continue;
			LLRect bbox = LLDistance(r->SE[0].lat, r->SE[0].lng, MAXCONNDIST);
			bbox.Expand( LLDistance(r->SE[1].lat, r->SE[1].lng, MAXCONNDIST) );
			// propagate flags
			CRiverTree dtree(NULL);
			dtree.GetTree(r, &bbox, 1, scanflags, &scanlist);

			// propagate the connections to the splits
			CDoubleArrayList downconn = r->scan->downconn;
			for (int s=0; s<r->splits.length(); ++s)
				{
				// other stuff
				CScan *scan = r->splits[s];
				scan->SetRiver(r);

				// downconn
				CDoubleArrayList &sdownconn = r->splits[s]->downconn;
				sdownconn.Reset();
				CRiver *trk = r->splits[s]->trk;
				if (trk && trk->Coords().GetSize()>0)
					{
					// reset the connection if the track ends far away from connection point
					tcoord &last = trk->Coords().Tail();
					if (Distance(last.lat, last.lng, r->SE[0].lat, r->SE[0].lng)<MAXSPLITDIST || Distance(last.lat, last.lng, r->SE[1].lat, r->SE[1].lng)<MAXSPLITDIST)
						sdownconn = downconn;
					}

				}

			// propagate flags (no need for upconn)
			CRiverTree utree(NULL);
			utree.GetTree(r, &bbox, -1, scanflags, &scanlist);
			//r->scan->upconn = utree.idlist;
			//r->scan->upconn.Delete(0);
			}
}

void ScanGoogle(void)
{
#if 0
	// check res of all scans
	int n = 0, tot = 0;
	for (int i=n=0; i<scanlist.GetSize(); ++i)
		if (scanlist[i].id!=0)
			{
			if (rescanerrors)
				scanlist[i].geterror()=EERRGOOG;
			if (scanlist[i].geterror()!=EERROK)
				n += 2 * scanlist[i].rappels.GetSize();
			tot += scanlist[i].rappels.GetSize();
			}
	
	// get elevations
	scangkey = &scangONkey;
	tcoords coords(n);
	for (int i=n=0; i<scanlist.GetSize(); ++i)
		if (scanlist[i].id!=0)
			if (scanlist[i].geterror()!=EERROK)
				{
				CRapList &raplist = scanlist[i].rappels;
				for (int j=0; j<raplist.GetSize(); ++j, n+=2)
					{
					trappel &r = *raplist[j];
					tcoord &c = coords[n];
					tcoord &c1 = coords[n+1];
					c.lat = r.lle.lat;
					c.lng = r.lle.lng;
					c.res = r.res;
					c.resg = r.resg;
					c1 = c;
					}
				}

	coords[0].lat = 34.180622;
	coords[0].lng = -118.060860;
	CoordDirection(coords[0], 0, 5, coords[1]);
	CoordDirection(coords[1], 0, 5, coords[2]);
	scangkey->getelevationGOOG(n, coords.Data(), FALSE);

	// check scans that need recomputing
	int rescan = 0;
	for (int i=n=0; i<scanlist.GetSize(); ++i)
		if (scanlist[i].id!=0)
			if (scanlist[i].geterror()!=EERROK)
				{
				int better = 0;
				CRapList &raplist = scanlist[i].rappels;
				for (int j=0; j<raplist.GetSize(); ++j)
					{
					trappel &r = *raplist[j];
					tcoord &c = coords[n++];
					if (isBetterResG(c.res, c.resg))
						++better;
					}
				if (better==0) // there's nothing better!
					scanlist[i].geterror() = EERROK;
				else
					++rescan;
				}
	
	//  RE-Scan invalid with Google Elevation
	Log(LOGINFO, "Scanning Google high res %d/%d scans [Checked %d/%d raps]", rescan, scanlist.GetSize(), n, tot);
	if (rescan>0)
		{
		scanerrors = TRUE;
		ScanRivers();
		ScanWaterfalls();
		}
	scangkey = &scangOFFkey;
#endif
}


int _Scan(const char *file)
	{
#ifdef DEBUG
	   double invfd = (float)InvalidNUM;
	   double invd = InvalidNUM;
	   float invf = InvalidNUM;
	   ASSERT(invf==invfd);
	   ASSERT(invd==invfd);
	   ASSERT(invd==invf);
	   ASSERT(invf==InvalidNUM);
	   ASSERT(invd==InvalidNUM);
#endif

		if (CScanProcess::NeedAbortScan())
			return FALSE;

		DEBUGSTR = file;
		if (MODE<-1) TMODE = 0;
		scangkey = TMODE==0 ? &scangOFFkey : &scangONkey;
		if (loadlist.Load(file, TRUE))
			if (loadlist.IsValid())
				return TRUE;

		// start scan
		LoadPOI();
		ResetRogues();
		GOOGDATA.Reset();

		// load rivers
		rcache = new CRiverCache(this->file = file);
		if (!rcache->rlist)
			{
			Log(LOGERR, "SCAN ERROR: Could not get rlist %s", file);
			return FALSE;
			}


#ifdef DEBUGXXX
		// truncate list
		rcache.rlist->SetSize(100);
#endif

		this->file = file;
		if (!CBox::box(file, bbox))
			{
			Log(LOGERR, "BAD SCAN FILE %s", file);
			return FALSE;
			}

		CRiverList &rlist = *rcache->rlist;

		// pre-allocate proper memory size
		scanlist.Dim(rlist.GetSize()*2);
		scanlist.Load(file, TRUE, loadlist.GetSize()==0 ? -1 : 1);
		
		// track changes
		scanchanges = 0;

		// map scans, extras and delete unmapped ones
		Map(TRUE);

		if (MODE==-3)
			return TRUE;

		//scanlist.Check();
		Log(LOGINFO, "Scanning File %s %dRiv %dScans %dVInv %dGInv [%s]", file, rlist.GetSize(), scanlist.GetSize(), scanlist.Invalids(INVVERSION), scanlist.Invalids(INVGOOG), bbox.Summary());
		rprogressmax = rlist.GetSize()+1;

		//scanerrors = FALSE;
		ScanRivers();
		ScanWaterfalls();
		if (MODE==0 || scanchanges)
			{
			ScanConnection();
			++scanchanges;
			}

		// disable google but still count as errors when needed!
		//scangkey.disabled = GetTickCount()+15*24*60*60*1000; // 15 days
		//if (TMODE<0)
		//	ScanGoogle();

		// finished scan
		if (scanchanges)
			SaveProgress(rprogress+1, rprogressmax, TRUE);

		if (NeedAbortScan())
			{
			Log(LOGERR, "Out of google quota, scan aborted");
			return FALSE;
			}

		if (scanlist.Invalids(INVGOOG)>0)
			return -1; // needs second pass

		return TRUE; // finished
	}

};



void CScan::SetRiver(CRiver *r)
	{				
			level = r->Level();
			order = r->Order();
			res = 0;

			cfs = temp = km2 = InvalidNUM;
			vars watershed = r->sym->GetStr(ITEM_WATERSHED);
			vara stats = GetWatershed(watershed);
			double val = CDATA::GetNum(stats[WSCFS1]);
			if (val!=InvalidNUM) cfs = (float)val;
			val = CDATA::GetNum(stats[WSTEMP1]);
			if (val!=InvalidNUM) temp = (float)val/100;
			val = CDATA::GetNum(stats[WSAREA2]);
			if (val==InvalidNUM)
				val = CDATA::GetNum(stats[WSAREA1]);
			if (val!=InvalidNUM) km2 = (float)val;
	}

void CScan::Set(CScanPtr ptr, double id, double oid, int split, CanyonMapper &cm, const char *name)
{
		this->id = id;
		this->oid = oid;
		ASSERT(id>=0 || oid>=0);

		this->name = name;
		this->setsplit(split);
		this->date = (float)CurrentDate;
		this->version = SCANVERSION;

		traplist &raplist = cm.rappels;
		tpoints &points = cm.trk;

		this->mappoint = cm.mappoint;

		// set rappels
		int nrappels = raplist.GetSize();
		rappels.SetSize(nrappels);
		rapdata.SetSize(nrappels);
		for (int i=0; i<nrappels; ++i)
			{
			rappels[i] = &rapdata[i];
			rappels[i]->lle = points[raplist[i].start];
			rappels[i]->end = points[raplist[i].end];
			rappels[i]->res = (float)points[raplist[i].start].res;
			rappels[i]->resg = (float)points[raplist[i].start].resg;
			rappels[i]->avgg = (float)raplist[i].avggrade;
			//rappels[i]->ming = (float)raplist[i].mingrade;
			//rappels[i]->dist = (float)raplist[i].dist;
			ZeroMemory(rappels[i]->rflags, sizeof(rappels[i]->rflags));
			rappels[i]->rflags[RSIDEG] = (unsigned char)max(0, Rap(raplist[i].sidegrade));
			}

		ASSERT(!trk && !wrlist);

		// prepare for wrlist
		wrlist = new LLECLIST;

		// set track
		ASSERT(points.GetSize()>1);
		trk = new CRiver(points, ptr);

		// set error
		seterror( cm.googerror>0 ? EERRGOOG : EERROK );
}


#define CFGMAX 5
enum { CEWR, CEGA, CDIST, CMGR, CSGR, CMAX };
const char *CfgName[] = { "EWR", "EGA", "DIST", "MGR", "SGR", NULL };

enum { CRHT, CRKM2, CRCFS, CRTEMP, CRMAX };
const char *CfgRName[] = { "HT", "KM2", "CFS", "TEMP", NULL };
enum { CFGRCMIN, CFGRMIN, CFGRMAX, CFGRCMAX, CFGRNA, CFGMAXR };
const char *CfgRMM[] = { "CMIN", "MIN", "MAX", "CMAX", "NA" };

const char DefaultKey[] = "KEY=b8f5f54789b5f5c1;";
const char DefaultCfg[] =
"http://localhost/rwr/?rws=scanname=GorgeTest2;OUTPUT=KMLDATA;COORD1=45.564339,-122.127205;COORD2=45.583206,-122.097458;RESMAX=300;RWCMP=on;RWPR=18;HTCMIN=26ft;HTMIN=49ft;HTMAX=200ft;HTCMAX=;metric=off;DIST4=9;DIST3=7;DIST2=5;DIST1=3;DIST0=0;MGR4=7;MGR3=6;MGR2=5;MGR1=0;MGR0=0;SGR4=10;SGR3=7;SGR2=5;SGR1=3;SGR0=0;EWR=on;EWR4=8;EWR3=8;EWR2=7;EWR1=6;EWR0=5;WGR=on;EWC=off;EWCBOOST=8;KM2MM=off;KM2CMIN=0;KM2MIN=0;KM2MAX=250;KM2CMAX=400;CFSMM=off;CFSCMIN=5;CFSMIN=10;CFSMAX=50;CFSCMAX=100;TEMPMM=off;TEMPCMIN=5;TEMPMIN=10;TEMPMAX=20;TEMPCMAX=20;";
//const char DefaultCfg[] = ";metric=off;HTMIN=20;HTMAX=200ft;DIST4=8;DIST3=6;DIST2=4;DIST1=2;DIST0=0;MGR4=7;MGR3=6;MGR2=5;MGR1=0;MGR0=0;EWR=on;EWR4=8;EWR3=8;EWR2=7;EWR1=6;EWR0=5;WGR=on;EWC=on;EWCBOOST=5;CFS=on;CFS4=5;CFS3=5;CFS2=5;CFS1=5;CFS0=5;scanname=BendArea2;OUTPUT=KML;COORD1=43.2977,-122.6832;COORD2=44.7922,-120.5256;RESMAX=100;KEY=f0c579a07ebbf5c1";
// ";RWCMP=on;metric=off;HTMIN=20;HTMAX=200ft;DIST4=8;DIST3=6;DIST2=4;DIST1=0;DIST0=0;MGR4=6;MGR3=6;MGR2=5;MGR1=0;MGR0=0;EWR=on;EWR4=8;EWR3=8;EWR2=6;EWR1=4;EWR0=2;EWC=off;EWCBOOST=5;CFS=on;CFS4=3;CFS3=7;CFS2=7;CFS1=3;CFS0=1;scanname=BendAreaNew5;OUTPUT=KML;COORD1=43.2977,-122.6832;COORD2=44.7922,-120.5256;RESMAX=100;";
//"http://localhost/rwr/?rws=metric=off;HTMIN=20;HTMAX=200ft;DIST4=10;DIST3=5;DIST2=2;DIST1=0;DIST0=0;MGR4=5;MGR3=5;MGR2=5;MGR1=0;MGR0=0;EWR=on;EWR4=10;EWR3=8;EWR2=7;EWR1=6;EWR0=5;EWC=on;EWCBOOST=8;CFS=on;CFS4=5;CFS3=6;CFS2=7;CFS1=6;CFS0=8;scanname=Columbia%20Gorge%20Test2;OUTPUT=KML;COORD1=45.393707,-122.425514;COORD2=45.852448,-121.334290;RESMAX=100;RWCMP=off";
//";metric=off;HTMIN=20;HTMAX=200ft;DIST4=8;DIST3=6;DIST2=4;DIST1=2;DIST0=0;MGR4=7;MGR3=6;MGR2=5;MGR1=0;MGR0=0;EWR=on;EWR4=8;EWR3=8;EWR2=7;EWR1=6;EWR0=5;EWC=off;EWCBOOST=8;CFS=on;CFS4=4;CFS3=8;CFS2=7;CFS1=4;CFS0=1;scanname=BendArea3;OUTPUT=KML;COORD1=43.2977,-122.6832;COORD2=44.7922,-120.5256;RESMAX=100;KEY=f0c579a07ebbf5c1";
//"http://lucac.no-ip.org/rwr/?rws=scanname=ColumbiaGorgeJul4;OUTPUT=KML;COORD1=45.426594,-122.831238;COORD2=45.866154,-121.398216;RESMAX=100;RWCMP=off;metric=off;HTMIN=30;HTMAX=150ft;DIST4=8;DIST3=6;DIST2=4;DIST1=1;DIST0=0;MGR4=6;MGR3=6;MGR2=5;MGR1=0;MGR0=0;SGR4=10;SGR3=6;SGR2=5;SGR1=4;SGR0=0;EWR=on;EWR4=9;EWR3=8;EWR2=7;EWR1=6;EWR0=0;WGR=on;EWC=on;EWCBOOST=5;KM2MM=off;KM2MIN=0;KM2MAX=250;CFSMM=on;CFSMIN=10;CFSMAX=80;TEMPMM=off;TEMPMIN=5;TEMPMAX=15";
//bigger"http://localhost/rwr/?rws=scanname=BendArea8;OUTPUT=KML;COORD1=45.269634,-122.987274;COORD2=46.029312,-121.173584;RESMAX=100;RWCMP=off;metric=off;HTMIN=20;HTMAX=200ft;DIST4=8;DIST3=6;DIST2=4;DIST1=0;DIST0=0;MGR4=7;MGR3=6;MGR2=5;MGR1=0;MGR0=0;SGR4=10;SGR3=7;SGR2=5;SGR1=3;SGR0=1;EWR=on;EWR4=8;EWR3=8;EWR2=7;EWR1=6;EWR0=5;WGR=on;EWC=off;EWCBOOST=8;KM2MM=off;KM2MIN=0;KM2MAX=250;CFSMM=on;CFSMIN=10;CFSMAX=30;TEMPMM=off;TEMPMIN=10;TEMPMAX=20";
//                        ";metric=off;HTMIN=20;HTMAX=200ft;DIST4=10;DIST3=5;DIST2=2;DIST1=0;DIST0=0;MGR4=5;MGR3=5;MGR2=5;MGR1=0;MGR0=0;EWR=on;EWR4=10;EWR3=8;EWR2=7;EWR1=6;EWR0=5;WGR=on;EWC=on;EWCBOOST=5;CFS=on;CFS4=10;CFS3=8;CFS2=7;CFS1=5;CFS0=1;scanname=Columbia%20Gorge8;OUTPUT=KML;COORD1=46.0124,-122.1634;COORD2=46.2073,-121.6692;RESMAX=100
/*
"KEY=b8f5f54789b5f5c1;RESMAX=200;HTMIN=10m;HTMAX=60m;"
//"HT0:<10m=0;HT1:10m-25m=90;HT2:25m-50m=100;HT3:50m-100m=110;HT4:>100m=120;"
"MGR0:Green=0;MGR1:Yellow=0;MGR2:Orange=9;MGR3:Red=10;MGR4:Black=10;"
"EWR0:No=8;EWR1:Below=9;EWR2:Local=10;EWR3:Above=10;EWR4:OnRappel=10;"
//EWC=on
"DIST0:2km-3km=0;DIST1:1km-2km=0;DIST2:500m-1km=2;DIST3:500m-250m=5;DIST4:>250m=9;"
//"EGX0:No=100;EGX1:Below=100;EGX2:Local=110;EGX3:Above=110;EGX4:OnRappel=110;"
//"EGA0:No=100;EGA1:Below=100;EGA2:Local=110;EGA3:Above=110;EGA4:OnRappel=110;"
//Avg Flow? Check BEST canyons region
//Rock Type? Check BEST canyons region
//River has name
*/
;



class CScoreProcess {

	int EWG(trappel &rap)
	{
		if (getewcflag(rap.rflags[RTHIS], ewc))
			return 4;
		if (getewcflag(rap.scan->flags[ETHIS], ewc))
			return 3;
		if (getewcflag(rap.scan->flags[EUP], ewc))
			return 2;
		if (getewcflag(rap.scan->flags[EDN], ewc))
			return 1;
		return 0;
	}

	int CFS(trappel &rap)
	{
		double cfs = rap.scan->cfs;

		if (cfs>=100)
			return 4;
		if (cfs>=50)
			return 3;
		if (cfs>=10)
			return 2;
		if (cfs>=1)
			return 1;
		return 0;
	}

	int MGR(trappel &rap)
	{
		int g = max(0, Rap(rap.avgg));
		// patch for presence of waterfalls
		// res { "High", "Med", "Low" }
		if (wgr && resnum(rap.res)>0)
			if (getewcflag(rap.rflags[RTHIS], ewc))
				g = min(4,g+1);
		return g;
	}

	int SGR(trappel &rap)
	{
		int g = max(0, min(3, rap.rflags[RSIDEG]));
		// patch for presence of waterfalls
		// res { "High", "Med", "Low" }
		return g;
	}

	int  HT(trappel &rap)
	{
		float h = rap.m();
		if (h>=100) // 300'
			return 4;
		if (h>=50) // 150'
			return 3;
		if (h>=25) // 75'
			return 2;
		if (h>=10)  // 30'
			return 1;
		return 0;
	}

	static int DIST(double d)
	{
		if (d<=125)
			return 4;
		if (d<=250)
			return 3;
		if (d<=500)
			return 2;
		if (d<=1000)
			return 1;
		return 0;
	}

	/*
	static int validrapstamp(CRiver *r, LLEC *lle, void *data)
	{
	trappel *rap = lle->rappel();
	ASSERT( r->Box().Contains(lle->lat, lle->lng));
	if (rap->scan!=NULL)
		rap->scan->setvalid();
	return FALSE;
	}
	*/

	static int scorerapstamp(CRiver *r, LLEC *lle, double maxdist, void *data)
	{
	trappel *rap = lle->rappel();
	ASSERT( r->Box().Contains(lle->lat, lle->lng));

	register CScoreProcess *obj = (CScoreProcess*)data;
	rap->score =  obj->Score(*rap);
	rap->scoreg = 0; //rap->score*2; // self counts for 10/10 scale
	return FALSE;
	}

	static int scoregroupstamp(LLEC *r, LLEC *lle, double maxdist, void *data)
	{
	trappel *A = (trappel *)r->rappel();
	trappel *B = (trappel *)lle->rappel();
	if (A->score<=0) // ignore
		return TRUE;
	if (B->score<=0) // irrelevant
		return FALSE;
	double dist = LLE::Distance(r, lle);
	if (dist<maxdist)
		{
		register short int AisB = A->scan->id==B->scan->id;
		register short int BtoA = B->scan->downconn.Find(A->scan->id)>=0;
		register short int AtoB = A->scan->downconn.Find(B->scan->id)>=0;
		if (AisB || AtoB || BtoA)
			{
			//ASSERT(IdStr(r->Id())!="462f1ff7945fc1c1");
			ASSERT(!(IdStr(A->lle.Id())=="R4e6218d49960d041" && IdStr(B->lle.Id())=="f200800f0db5f5c1"));
			//ASSERT(!(IdStr(A->lle.Id())=="f200800f0db5f5c1" && IdStr(B->lle.Id())=="a61cc5d687b5f5c1"));
			//ASSERT(IdStr(A->lle.Id())!="e7634f6cde9bf5c1");
			//ASSERT(IdStr(A->lle.Id())!="d1080ad4d39bf5c1");

			if (BtoA && !AisB && !AtoB)
				{
				// only allow scoreg from 1 upstream segment
				if (A->upscoreid == 0) 
					A->upscoreid = B->scan->id;
				else
					if (A->upscoreid != B->scan->id)
						return FALSE;
				}

			CScoreProcess *obj = (CScoreProcess*)data;
			float mul = obj->Cfg[CDIST][DIST(dist)];
			if (A->Id()==B->Id())
				mul = 2; // self
			else
				{
				// avoid overlapping rappels
				if ((A->lle.elev > B->lle.elev))
					{
					if (!(A->end.elev >= B->lle.elev))
						return FALSE;
					}
				else
					{
					if (!(B->end.elev >= A->lle.elev))
						return FALSE;
					}
				}

#ifdef DEBUGXXXX
			if (IdStr(A->lle.Id())=="90dcd22c7ab4f5c1")
				Log(LOGINFO, "%s:%.0f += %s:%.0f * %.5f  [%.fm]", IdStr(A->lle.Id()), A->scoreg, IdStr(B->lle.Id()), B->score, mul, dist);
#endif
			if (mul>0)
				{
				//ASSERT(IdStr(A->lle.Id())!="Pc8fd8b77f76df5c1");
				A->scoreg += B->score*mul;
				A->scorelist.push(B);
#ifdef DEBUG
				A->logscoreg.push(MkString("%s:%s*%s", B->Id(), CData(B->score), CData(mul)));
#endif
				}
			return FALSE;
			}
		}
	return FALSE;
	}

#ifdef DEBUG
#define logscore(name, mul)  { float m = mul; ASSERT(m>=0); score *= m; if (m!=1) rap.logscore.push(name+MkString("%.1f",m)); }
#else
#define logscore(name, mul) score *= mul
#endif

	inline float ScoreRange(float val, int c, float mul = 2)
	{
			if (CfgR[c][CFGRMIN]==InvalidNUM)
				return 1;

			if (val==InvalidNUM && CfgR[c][CFGRNA]!=InvalidNUM)
				return CfgR[c][CFGRNA];

			if (val<CfgR[c][CFGRMIN])
				if (CfgR[c][CFGRCMIN]==InvalidNUM)
					mul = 1; // no cutoff 20% => 200% 10x
				else
					if (val<CfgR[c][CFGRCMIN])
						mul = 0; // cutoff
					else
						mul = mul*(val-CfgR[c][CFGRCMIN])/(CfgR[c][CFGRMIN]-CfgR[c][CFGRCMIN]); // interpolation

			if (val>CfgR[c][CFGRMAX])
				if (CfgR[c][CFGRCMAX]==InvalidNUM)
					mul = 1; // no cutoff 20% => 200% 10x
				else
					if (val>CfgR[c][CFGRCMAX])
						mul = 0; // cutoff
					else
						mul = mul*(CfgR[c][CFGRCMAX]-val)/(CfgR[c][CFGRCMAX]-CfgR[c][CFGRMAX]); // interpolation
			return mul;
	}

	float Score(trappel &rap)
	{
			if (!rap.scan)
				return 0;

			
			float score = rap.m();;

			//ASSERT(IdStr(rap.scan->id)!="Rf0a7f678dcfecb41"); // Dry

			// cap height
			float maxh = CfgR[CRHT][CFGRMAX];
			if (maxh!=InvalidNUM)
				if (score>maxh)
					score = maxh;

			// range adjustments
			logscore("HT", ScoreRange(score, CRHT));
			logscore("KM2", ScoreRange(rap.scan->km2, CRKM2));
			logscore("CFS", ScoreRange(rap.scan->cfs, CRCFS));
			logscore("TEMP", ScoreRange(rap.scan->temp, CRTEMP));

			//ASSERT(IdStr(rap.lle.Id())!="P30c807cf5086f5c1"); // umpqua
			//ASSERT(IdStr(rap.lle.Id())!="Pc8fd8b77f76df5c1");
			//score *= Cfg[CHT][HT(rap)];
			logscore("EWR", Cfg[CEWR][EWG(rap)]);
			logscore("SGR", Cfg[CSGR][SGR(rap)]);
			logscore("MGR", Cfg[CMGR][MGR(rap)]);
			// Panoramio/Flickr booster for OnRappel only
			if (getflag(rap.rflags[RTHIS], EWC))
				logscore("EWC", ewcboost);
			return score;
	}

	static CString getcfgstr(vara &list, const char *id)
	{
				int found = list.indexOfSimilar(id);
				if (found<0)
					return "";
				return GetToken(list[found],1,'=');
	}

	float getcfg(vara &list, const char *id, float def, vara *undef = NULL)
	{
				vars str = getcfgstr(list, id);
				if (str.IsEmpty())
					{
					if (undef)
						undef->push(id);
					return def;
					}
				// found
				double v = CDATA::GetNum(str);
				if (v<0)
					{
					Log(LOGERR, "CScoreProcess Invalid CFG %s=%s", id, str);
					return def;
					}
				return (float)v;
	}
 


	static int cmpscanptr(CScan **A, CScan **B)
	{
		if ((*A)->id > (*B)->id) return 1;
		if ((*A)->id < (*B)->id) return -1;
		return 0;
	}

	static int cmpscoreg(LLEC **rA, LLEC **rB)
	{
		trappel *A = (*rA)->rappel();
		trappel *B = (*rB)->rappel();
		if (A->scoreg > B->scoreg) return -1;
		if (A->scoreg < B->scoreg) return 1;
		if (A->score > B->score) return -1;
		if (A->score < B->score) return 1;
		return 0;
	}




#define OPTIMIZERDIST (MINSTEP*2) // 10m


// scan is main, lle flows into scan. lle is truncated, scan might get splitted in 2 scans if needed
static int optimizerap(CScoreProcess *proc, CScanPtr &scan, trappel *rap, CScanPtr &llescan, trappel *llerap, BOOL nooverlap)
{
	PROFILE("Load().optimizerap()");
	if (scan->id!=rap->scan->id || llescan->id!=llerap->scan->id)
		{
		Log(LOGALERT, "optimizeraps Inconsistent lle llescan %s!=%s scan %s!=%s", IdStr(llescan->id), IdStr(llerap->scan->id), IdStr(scan->id), IdStr(rap->scan->id));
		return FALSE;
		}

	// maximize joining rap
	//if (llerap->m()>rap->m())
	//	rap->m = llerap->m();
	//if (llerap->avgg>rap->avgg)
	//	rap->avgg = llerap->avgg;

	// check joining rap relative position
	int nrap = scan->RapNum(rap);
	int nllerap = llescan->RapNum(llerap);

	if (nllerap<0 || nrap<0)
		{
		vara raps;
		for (int i=0; i<llescan->rappels.GetSize(); ++i)
			raps.push(llescan->rappels[i]->Summary());
		CScanPtr scan2 = rap->scan, llescan2 = llerap->scan;
		Log(LOGALERT, "optimizeraps Inconsistent lle nllerap %s CScan %s:%s", llerap->Summary(), IdStr(llescan->id), raps.join(", "));
		return FALSE;
		}
	if (nrap<0)
		{
		vara raps;
		for (int i=0; i<scan->rappels.GetSize(); ++i)
			raps.push(scan->rappels[i]->Summary());
		Log(LOGALERT, "optimizeraps Inconsistent lle nrap %s CScan %s:%s", rap->Summary(), IdStr(scan->id), raps.join(", "));
		return FALSE;
		}

	if (nooverlap)
		++nllerap;

	// delete overlapping raps
	for (int i=nllerap, j=nrap; i<llescan->rappels.GetSize(); ++i, ++j)
		{
#ifdef DEBUGXXX 
		if (j<scan->rappels.GetSize())
			if (LLE::Distance(&llescan->rappels[i]->lle, &scan->rappels[j]->lle)>=OPTIMIZERDIST) // not really consistent
				Log(LOGALERT, "NOT Inconsistent overlapping raps scan#%d=%s vs llescan#%d=%s (join at %s)", j, scan->rappels[j]->lle.Summary(), i, llescan->rappels[i]->lle.Summary(), rap->lle.Summary());
#endif
		llescan->rappels[i]->scan = NULL;
		}
	llescan->rappels.SetSize(nllerap);


	// if needed, break up original segment
	if (nrap>0 && scan->rappels.GetSize()-nrap>0)
		{
		int nraps = scan->rappels.GetSize();
		// setup new scan
		CScanPtr newscan = proc->optimized.AddScan();
		*newscan = *scan;
		newscan->id = rap->lle.Id();
		newscan->rappels.Reset();
		// split rappels
		for (int i=nrap; i<scan->rappels.GetSize(); ++i)
			{
			scan->rappels[i]->scan = newscan;
			ASSERT(scan->rappels[i]->scan->id == newscan->id);
			newscan->rappels.AddTail(scan->rappels[i]);
			}
#ifdef DEBUG
		for (int i=0; i<newscan->rappels.GetSize(); ++i)
			{
			ASSERT(newscan->rappels[i]->scan->id == newscan->id);
			//ASSERT(newscan->rappels[i].lle.river == newscan->rappels[i]);
			}
#endif
		scan->rappels.SetSize(nrap);
		ASSERT(scan->rappels.GetSize()>0);
		ASSERT(newscan->rappels.GetSize()>0);
		// connect
		connect(scan, newscan);
		connect(llescan, newscan);

		if (INVESTIGATE>=0)
			{
			Log(MkString("Split CScan %s R%d/%d into: %s %draps + %s %draps", IdStr(scan->id), nrap, nraps, IdStr(scan->id), scan->rappels.GetSize(), IdStr(newscan->id), scan->rappels.GetSize()));
			vara scanraps, llescanraps, newscanraps;
			for (int i=0; i<scan->rappels.GetSize(); ++i)
				scanraps.push(scan->rappels[i]->Summary(scan));
			for (int i=0; i<llescan->rappels.GetSize(); ++i)
				llescanraps.push(llescan->rappels[i]->Summary(llescan));
			for (int i=0; i<newscan->rappels.GetSize(); ++i)
				newscanraps.push(newscan->rappels[i]->Summary(newscan));
			Log("A)" + scanraps.join(", "));
			Log("B)" + llescanraps.join(", "));
			Log("N)" + newscanraps.join(", "));
			}
		}
	else
		{
		// connect to scan (if not connected already)
		connect(llescan, scan);
		}

	return FALSE;
}

static void connect(CScan *r, CScan *down)
{
		// are they same order? if not skip (Tish and Eagle Creek problem)
		if (abs( r->order - down->order )>1)
			return;

		// connect to scan (if not connected already)
		if (r->downconn.Find(down->id)<0)
			r->downconn.AddTail(down->id);
}

static int optimizeraps(LLEC *r, LLEC *lle, double maxdist, void *data)
{
	PROFILE("Load().optimizeraps()");
	if (r==lle)
		return FALSE;
	trappel *rap = r->rappel(), *llerap = lle->rappel();
	CScanPtr scan = rap->scan, llescan = llerap->scan;
	if (scan==NULL || llescan==NULL)
		return FALSE;
	if (scan==llescan || scan->id==llescan->id)
		return FALSE;

	//ASSERT(IdStr(scan->id)!="P0ca25ef1409bf5c1");
	//ASSERT(IdStr(scan->id)!="P3c5b7d07639bf5c1");

	//vars ids = IdStr(scan->id)+IdStr(llescan->id);
	//ASSERT( !strstr(ids, "R2506c97c4e51db41") || !strstr(ids, "P58fecd9303a9f5c1"));


	// end between start-end => shorten end, if then completely in merge
	// R   a b c d  NO
	//     |   |
	// |   |   | 
	// |   | | | 
	// |   | | . |
	// |   | . . |
	// .   | .   |  |
	//              |

	if (rap->end.elev > llerap->lle.elev) // NO, disjoint rappels
		return FALSE; 
	if (llerap->end.elev > rap->lle.elev) // NO, disjoint rappels
		return FALSE; 

	//double lst;
	//if (DistanceToSegment(*lle, *r, r->rappel()->end, lst)>OPTIMIZERDIST)
	if (LLE::Distance(r, lle)>OPTIMIZERDIST)
		return FALSE;

	// overlap in elev and pos detected

#ifdef DEBUG
	if (LLE::Distance(&rap->lle, &llerap->lle)>OPTIMIZERDIST)
		{
		Log(LOGALERT, "Inconsistent overlapping raps %s vs %s", rap->lle.Summary(), llerap->lle.Summary());
		Log("Rscan:"+scan->Summary());
		Log("LLEscan:"+llescan->Summary());
		}
#endif

	//ASSERT(IdStr(r->Id())!="Pf47a566086abf5c1");
	//ASSERT(IdStr(scan->id)!="Pf47a566086abf5c1");
	
	if (INVESTIGATE>=0)
		{
		//setflag(rap->flags[ETHIS], id);
		
		Log(LOGINFO, "Overlapping Raps R:%s=%s  LLE:%s=%s (%.0fm dist)", IdStr(scan->id), r->Summary(), IdStr(llescan->id), lle->Summary(), LLE::Distance(r,lle));
		Log("Rscan:"+scan->Summary());
		Log("LLEscan:"+llescan->Summary());
		CRiverCache cache(*r);
		CRiver *r;
		r = cache.rlist->Find(scan->id);
		if (r) Log("Rriver:"+r->sym->GetStr(ITEM_SUMMARY));
		r = cache.rlist->Find(llescan->id);
		if (r) Log("LLEriver:"+r->sym->GetStr(ITEM_SUMMARY));
		}
	
	// are they already connected?
	BOOL scantolle = FALSE, lletoscan = FALSE;
	BOOL samescan = scan->oid!=0 && scan->oid==llescan->oid;
	if (!samescan)
		{
		scantolle = scan->downconn.Find(llescan->id)>=0;
		lletoscan = llescan->downconn.Find(scan->id)>=0;
		}

	// optimize smaller rappel
	// dominant the one with more downcomm (optimize the other)
	int cmp = LLE::Distance(&rap->lle, &scan->rappels.Tail()->lle) > LLE::Distance(&llerap->lle, &llescan->rappels.Tail()->lle) ? 1 : -1;		
		//rap->lle.elev >= llerap->lle.elev ? 1 : -1;
	if (scan->downconn.GetSize()>llescan->downconn.GetSize())
		cmp = 1;
	if (scan->downconn.GetSize()<llescan->downconn.GetSize())
		cmp = -1;
	if (lletoscan && !scantolle)
		cmp = 1;
	if (scantolle && !lletoscan)
		cmp = -1;
	if (samescan) // manage splits
		{
		cmp = llescan->getsplit() - scan->getsplit();
		ASSERT(cmp!=0); // same scan and same split
		}

	BOOL nooverlap = FALSE;
	if (rap->end.elev - llerap->end.elev > 1 ) 
		if (rap->lle.elev - llerap->lle.elev > 1 && cmp<0)
			nooverlap = TRUE; // d)  //rap->end = llerap->lle, 
		//else
		//	rap->end = llerap->end;	//a)
	
	if (llerap->end.elev - rap->end.elev > 1)
		if (llerap->lle.elev - rap->lle.elev > 1 && cmp>0)
			nooverlap = TRUE; // c) //llerap->end = rap->lle, 
		//else
		//	llerap->end = rap->end;	// b)

	// cmp>0 scan is main, lle flows into scan. lle is truncated, scan might get splitted in 2 scans if needed
	CScoreProcess *proc = (CScoreProcess *)data;
	if (cmp>0)
		return optimizerap(proc, scan, rap, llescan, llerap, nooverlap);
	else
		return optimizerap(proc, llescan, llerap, scan, rap, nooverlap);

	ASSERT(r->Id()!=lle->Id());
	return FALSE;
}







LLRect bbox;
vars user;
vars name;
vars config, cookies;
int priority;

CArrayList<CScanList> loaded;
CScanList optimized; // for brone up chunks

LLEProcess rapproc;
CScanListPtr scanlist;
CArrayList<CRapGroup> grouplist, rwgrouplist;

CSymList rwlist;
LLEProcess rwproc;

float resmax;
float Cfg[CMAX][CFGMAX];
float CfgR[CRMAX][CFGMAXR];
double numdone, numtotal, numrappels;
short int metric, ewc, wgr, rwcmp;
float ewcboost;
vars error;

public:

	CScoreProcess(const char *_config, const char *_cookies, CKMLOut *out, inetdata *data)
	{
	numdone = 0;
	numtotal = 0;
	numrappels = 0;

	PROFILEReset();
	if (Setup(_config, _cookies))
		{
		Load();
		Score();
		Group();
		}
	else
		{
		Log(LOGERR, "CScoreProcess: %s", error);
		}
	Output(out, data);
	if (INVESTIGATE>=0)
		PROFILESummary("ScoreProcess");
	}


	
	BOOL Setup(const char *_config, const char *_cookies)
	{
		// get config
		config = !_config ? DefaultCfg : _config;
		cookies = !_config ? DefaultKey : _cookies;

		config.Replace("&",";");
		config.Replace("\n",";");
		config.Replace("\r",";");
		vara list(config, ";");

		// get bbox
		if (Savebox)
			{
			bbox = *Savebox;
			vara configa(config, ";");
			int found;
			const char *field;
			field = "COORD1=";
			found = configa.indexOfSimilar(field);
			if (found>=0) 
				configa[found] = field+CCoord2(bbox.lat1,bbox.lng1);
			field = "COORD2=";
			found = configa.indexOfSimilar(field);
			if (found>=0) 
				configa[found] = field+CCoord2(bbox.lat2,bbox.lng2);
			config = configa.join(";");
			}

		if (!ScanBoxRect(config, bbox))
			{
			error = "Invalid box coordinates";
			return FALSE;
			}
	
		// get scan settings

		// scale 1 to 10
		for (int c=0; c<CMAX; ++c)
			{
			vara undef;
			vars name = CfgName[c];
			BOOL off = getcfgstr(list, name)=="off";
			for (int p=0; p<CFGMAX; ++p)
				{
				Cfg[c][p] = getcfg(list, MkString("%s%d",name,p), 5, &undef)/5;
				if (off) 
					Cfg[c][p] = 1;
				}
			if (undef.length()>0)
				if (undef.length()!=CFGMAX)
					Log(LOGERR, "CScoreProcess Undefined some CFG %s", undef.join(", "));
			}

		metric = getcfgstr(list, "metric")=="on";

		// relevant ranges
		for (int c=0; c<CRMAX; ++c)
			{
			vars name = CfgRName[c];
			BOOL off = getcfgstr(list, name+"MM")=="off";

			for (int p=0; p<CFGMAXR; ++p)
				{			
				vars idstr = getcfgstr(list, name+CfgRMM[p]);
				float v = (float)CGetNum(idstr, InvalidNUM);
				if (off) 
					v = InvalidNUM;
				if (p==CFGRNA && v!=InvalidNUM)
					{
					CfgR[c][p] = v/5;
					continue;
					}
				if (c==CRHT && v!=InvalidNUM)
					{
					float mul = 1;
					if (!metric)
						mul = 1/(float)m2ft;
					if (strstri(idstr, "ft"))
						mul = 1/(float)m2ft;
					if (strstri(idstr, "m"))
						mul = 1;
					v *= mul;
					}
				ASSERT(v>=0 || v==InvalidNUM);
				CfgR[c][p] = v + (float)(p<CFGRMAX ? -0.3 : 0.3);
				}

			// validation 
			if (CfgR[c][CFGRMIN]>CfgR[c][CFGRMAX] || CfgR[c][CFGRMIN]==InvalidNUM)
				CfgR[c][CFGRMIN] = CfgR[c][CFGRMAX] = CfgR[c][CFGRCMIN] = CfgR[c][CFGRCMAX] = InvalidNUM;
			}

		// get other stuff
		// get name
		name = getcfgstr(list, "scanname");
		resmax = getcfg(list, "RESMAX", 100);
		ewc = FLAG(EWR); ewcboost = 1;
		if (getcfgstr(list, "EWC")=="on")
			{
			ewc = FLAG(EWC)|FLAG(EWR);
			ewcboost = getcfg(list, "EWCBOOST", 5)/5;
			}

		wgr = getcfgstr(list, "WGR")=="on"; // force low grade raps to become high grade if waterfall in the area
		rwcmp = getcfgstr(list, "RWCMP")=="on";
		SPRIMARY = getcfgstr(list, "RANK")=="PIVOT" ? SPIVOT : SGSUM;

		// check bbox
		numtotal = 0;
		CBox cb(&bbox);
		for (LLRect *box =cb.Start(); box!=NULL; box=cb.Next(), ++numtotal);
		if (_config && numtotal>SCANMAXQUAD)
			{
			error = MkString("Area too big (%g>%d of 1 arc quadrangles)", numtotal, SCANMAXQUAD);
			return FALSE;
			}

		//  check key, get user and priority
		const char *userdata = AuthenticateUser(cookies);
		if (!userdata)
			userdata = AuthenticateUser(config);
		if (!userdata)
			{
			error = "Invalid key";
			return FALSE;
			}
		CSym sym("KEY", userdata);			
		user = sym.GetStr(0);
		priority = (int)sym.GetNum(1);
		double pr = CGetNum(getcfgstr(list, "RWPR"));
		if (pr>0) priority = (int)pr;

		// log calls
		Log(LOGINFO, "CanyonScanner %s: %s [%s]", user, name, bbox.Summary());
		Log(config);
		return TRUE;
	}


	int Load()
	{
		PROFILE("Load()");

		numdone = 0;

		// preset set total size
		loaded.SetSize((int)numtotal); 

		int n = 0;
		CBox cb(&bbox);
		for (LLRect *box =cb.Start(); box!=NULL; box=cb.Next(), ++n)
			{
			PROFILE("Load().Load");
			if (!loaded[n].Load(cb.file()) || !loaded[n].IsValid(INVVERSION))
				ScanAdd(cb.file(), user, priority);
			else if (!loaded[n].IsValid(INVGOOG))
				ScanAdd(cb.file(), user, -priority);

			loaded[n].Optimize(scanlist, FALSE);
			//rapproc.Load(loaded[n]);

			// total progress
			numdone += loaded[n].Progress();

#ifdef DEBUGXXX
			// save CSVO
			loaded[n].SaveCSV(cb.file()+"O");
#endif
			}

		// optimize raps
		rapproc.Load(scanlist);
		numrappels = rapproc.LLElist().GetSize();

		rapproc.MatchLLEElev(rapproc.LLElist(), OPTIMIZERDIST, optimizeraps, (void *)this);

		// sort connections
		for (int i=0; i<scanlist.GetSize(); ++i)
			scanlist[i]->downconn.Sort();

		return n;
	}

	void Score(void)
	{
		PROFILE("Score()");

		// rap scoring
		LLRect scorebbox = bbox;
		scorebbox.Expand(MAXSCOREDIST);

		CRiver rsbox(&scorebbox);
		rapproc.MatchBox(rsbox, 0, scorerapstamp, (void*)this);

		//CRiver rbox(&bbox);
		//rapproc.MatchBox(rbox, 0, validrapstamp, (void*)this);

		// group scoring
		LLECLIST &llelist = rapproc.LLElist();
		rapproc.MatchLLE(llelist, MAXSCOREDIST, scoregroupstamp, (void*)this);
	}


	static int cmpscore(CRapGroup *A, CRapGroup *B)
	{
		return (B)->score[SPRIMARY] - (A)->score[SPRIMARY];
	}

	void Group(void)
	{
		PROFILE("Group()");

		// sort
		LLECLIST &llelist = rapproc.LLElist();
		llelist.Sort(cmpscoreg);

#ifdef DEBUG
		{
		// output csv
		CSymList symlist;
		symlist.header = trappel::Header();
		for (int i=0; i<llelist.GetSize(); ++i)
			{
			trappel *rap = llelist[i]->rappel();
			if (!rap->scan)
				continue;
			symlist.Add(CSym(IdStr(rap->lle.Id()), rap->Summary(rap->scan, TRUE)));
			}
		symlist.Save("SCORE-RAPS" CSV);
		}

		// integrity check
		llelist.Sort(LLE::cmpelev);
		for (int i=0; i<llelist.GetSize(); ++i)
		  if (llelist[i]->rappel()->IsValid())
			{
			int dup = 0;
			trappel *rapi = llelist[i]->rappel();
			CScan *scani = rapi->scan;
			double id = rapi->lle.Id();
			//ASSERT(IdStr(id)!="P50c6d1c7f19af5c1");
			for (int j=++i; j<llelist.GetSize() && llelist[j]->rappel()->lle.Id()==id; ++j, ++i)
				if (llelist[j]->rappel()->IsValid())
					{
					trappel *rapj = llelist[j]->rappel();
					CScan *scanj = rapj->scan;
					Log(LOGALERT, "Duplicate valid rappel %s from scans #%s & #%s %s", IdStr(id), rapi->scan.Summary(), rapj->scan.Summary(), llelist[i]->rappel()->Summary());
					Log(rapi->scan->Summary());
					Log(rapj->scan->Summary());
					}
			--i;
			}
#endif

		// group

		// sort rappels by elevation, and set up id index 
		llelist.Sort(LLE::cmpelev);

		// set up id index for quick connections
		//CDoubleArrayList idlist(llelist.GetSize());
		//for (int i=0; i<llelist.GetSize(); ++i)
		//	idlist[i] = llelist[i]->lle->Id();

		//follow connections to form groups
		//float imax = resmax+resmax;
		for (int i=0; i<llelist.GetSize(); ++i)
			{
			//ASSERT(IdStr(llelist[i]->Id())!="Pfc89ff2326a5f5c1");

			trappel *rap = llelist[i]->rappel();
			if (!rap->IsValid())
				continue;

			// new group
			CRapGroup *group = grouplist.AddTail();
			// add chain of connected scorelist rappels
			group->Set(rap, ewc, &bbox); 
			}

		// delete groups completely out of bounds
		for (int i=0; i<grouplist.GetSize(); ++i)
			if (!grouplist[i].valid)
				grouplist.Delete(i--);

		// rank main score
		int n = grouplist.GetSize();
		CArrayList<CRapGroup*> sortlist(n);
		grouplist.Sort(cmpscore);
		for (int i=0; i<n; ++i)
			{
			grouplist[i].rank = i+1;
			sortlist[i] = &grouplist[i];
			}

		// rank other scores
		ASSERT( (sizeof(SCMP)/sizeof(*SCMP))==SMAX );
		for (int s=0; s<SMAX; ++s)
			{
			sortlist.Sort(SCMP[s]);
			for (int i=0; i<n; ++i)
				sortlist[i]->score[s] = i+1;
			}


#ifdef DEBUG
		{
		// output csv
		CSymList symlist;
		symlist.header = "SCANID,"+CRapGroup::Header(metric);
		for (int i=0; i<grouplist.GetSize(); ++i)
			{
			CRapGroup &group = grouplist[i];
			symlist.Add(CSym(IdStr(group.id), group.Summary(TRUE, FALSE)));
			}
		symlist.Save("SCORE-GRPS" CSV);
		}

		// integrity check
		for (int i=0; i<grouplist.GetSize(); ++i)
			{
			CRapGroup &group = grouplist[i];
			for (int j=i+1; j<grouplist.GetSize(); ++j)
				if (grouplist[i].raplist[0]->lle.Id()==grouplist[j].raplist[0]->lle.Id())
					Log(LOGALERT, "Duplicate group #%d vs #%d %s", i, j, grouplist[j].Summary(FALSE, FALSE));
			}
#endif

		if (rwcmp)
			RWGroup();
	}


	static int cmprstars(LLEC **lle1, LLEC **lle2)
	{
		double d1= (*lle1)->sym()->GetNum(ITEM_CONN);
		double d2 = (*lle2)->sym()->GetNum(ITEM_CONN);
		if (d1 > d2) return -1;
		if (d1 < d2) return 1;
		return 0;
	}

	static int cmprscore(CRiver **a1, CRiver **a2)
	{
		double score1 = (*a1)->length;
		double score2 = (*a2)->length;
		if (score1>score2) return -1;
		if (score1<score2) return 1;
		return 0;
	}


	static int cmpname(const char *n1, const char *n2)
	{
		vars name1 = n1, name2 = n2;
		name1 = " "+name1+" ";
		name2 = " "+name2+" ";
		name1.MakeLower(); name2.MakeLower();
		while(name1.Replace("  ", " ")); name1.Trim();
		while(name2.Replace("  ", " ")); name2.Trim();

		//ASSERT(!strstr(name1, "eagle") || !strstr(name2, "eagle"));

		if (name1.IsEmpty() || name2.IsEmpty())
			return FALSE;

		if (strstr(name1, name2) || strstr(name1, name2))
			return TRUE;


		const char *list[] = { " canyon ", " creek ", " upper ", " lower ", " north ", " south ", " fork ", " falls ", NULL };
		for (int i=0; list[i]!=NULL; ++i)
			{
			name1.Replace(list[i], " ");
			name2.Replace(list[i], " ");
			}

		while(name1.Replace("  ", " ")); name1.Trim();
		while(name2.Replace("  ", " ")); name2.Trim();
		
		if (name1.IsEmpty() || name2.IsEmpty())
			return FALSE;

		if (strstr(name1, name2) || strstr(name1, name2))
			return TRUE;

		return FALSE;
	}

	#define MINRWFIND 3000  // 3km

	static int rwfind(LLEC *r, LLEC *lle, double maxdist, void *data)
	{
		if (!r->rappel()->scan)
			return FALSE;

		double d = Distance(r->lat, r->lng, lle->lat, lle->lng);
		if (d<maxdist)
			{
			vars symname = GetToken(lle->sym()->GetStr(ITEM_WATERSHED), 0, '(');
			vars scanname = r->rappel()->scan->name;
			if (r->rappel()->scan->group())
				scanname = r->rappel()->scan->group()->name;
#ifdef DEBUG
			CScan *scan = r->rappel()->scan;
			CRapGroup *group = scan->group();
#endif

			//|| !strstr(scanname, "Henline"));
			//ASSERT(!strstr(symname, "Opal") || !strstr(scanname, "Opal"));
			if (cmpname(symname, scanname))
				d = d/1000;
			if (lle->elev<0 || d<lle->elev) // using elev as mindistance
				{
				//if (strstr(symname, "Eagle"))
				//	symname = symname;
				lle->lle = r;
				lle->elev = (float)d;
				}
			}
		return FALSE;
	}


	void RWGroup()
	{
		// associate scans to group (using trk)
		for (int i=0; i<grouplist.GetSize(); ++i)
			{
			CRapList raplist = grouplist[i].raplist;
			for (int r=0; r<raplist.GetSize(); ++r)
				if (raplist[r]->scan!=NULL)
					if (raplist[r]->scan->trk==NULL)
						raplist[r]->scan->trk = (CRiver *)&grouplist[i];
			}

		// load proc with rw locations
		rwlist.Load(filename(RWLOC));
		rwproc.Load(rwlist);

		// match rw locations to closest rappel
		rwproc.MatchLLE(rapproc.LLElist(), MINRWFIND, rwfind, NULL);


		// process results
		LLECLIST &list = rwproc.LLElist();
		list.Sort(cmprstars);


		int count = 0;
		for (int i=0; i<list.GetSize() && count<resmax; ++i)
			{
			CSym *sym = list[i]->sym();
			double lat = sym->GetNum(ITEM_LAT), lng = sym->GetNum(ITEM_LNG);
			int type = (int)sym->GetNum(ITEM_TAG) % 10;
			if (type!=1) // match only technical canyons
				continue;
			if (!bbox.Contains(lat, lng)) // only inside box
				continue;

			++count;

			//ASSERT(!strstr(sym->data, "Cave"));

			// arrange some of the 
			vars raps, longest;
			vara summary(sym->GetStr(ITEM_SUMMARY), " ");
			if (summary.length()>0)
				raps = summary[0];
			if (summary.length()>1)
				{
				double len = CGetNum(summary[1]);
				longest = numtoi(len, metric ? 1/m2ft : 1) + (metric ? "m" : "ft");
				}
			sym->SetStr(ITEM_NUM, raps);
			sym->SetStr(ITEM_NUM+1, longest);

			CRapGroup *g = rwgrouplist.AddTail();
			g->rwsym = sym;

			if (!list[i]->lle)
				{
				// no match
				continue; 
				}

			// Matched
			//ASSERT(!strstri(sym->data, "Eagle"));
			CScan *scan = list[i]->lle->rappel()->scan;
			CRapGroup *group = (CRapGroup *)scan->trk;
			if (group)
				{
				// matched with group
				*g = *group;
				g->rwsym = sym;
				continue;
				}
			
			// matched with an ungrouped scan (that had 0 score)
			g->Set(list[i]->lle->rappel(), ewc, &bbox); 
			}

		// clear trk association
		for (int i=0; i<grouplist.GetSize(); ++i)
			{
			CRapList raplist = grouplist[i].raplist;
			for (int r=0; r<raplist.GetSize(); ++r)
				if (raplist[r]->scan!=NULL)
					raplist[r]->scan->trk = NULL;
			}
	}



	void Output(CKMLOut *outp, inetdata *data)
	{
	PROFILE("Output()");
	// truncate to max res
	if (grouplist.GetSize()>resmax)
		grouplist.SetSize((int)resmax);

	CArrayList<CRapGroup> &grouplist = rwcmp ? this->rwgrouplist : this->grouplist;

	if (data)
		{
		BOOL rapinfo = strstr(config,"OUTPUT=CSV+")!=NULL;
#ifdef DEBUG
		rapinfo = TRUE;
#endif

		data->write( (rwcmp ? "STARS,NAME,COORDS,RAPS,LONGEST,=," : "") + CRapGroup::Header(metric) +"\n" );
		for (int i=0; i<grouplist.GetSize() && i<resmax; ++i)
			{
			CString rwdata;
			data->write( grouplist[i].Summary(rapinfo, metric)+"\n" );
			}
		if (!error.IsEmpty())
			data->write( "ERROR!:"+error+"\n" );
		}

	if (!outp)
		return;

		int n=min(grouplist.GetSize(), (int)resmax);

		// output kml
		CKMLOut &out = *outp;
		vars status = MkString("Scan %.0f%% complete: %d/%d quads, %.0f rappels.", numdone/numtotal*100, (int)numdone, (int)numtotal, numrappels);
		if (numdone<numtotal)
			status += MkString("\nRefresh scan later for more complete results.", ceil(numtotal-numdone));

		if (!error.IsEmpty())
			status = "ERROR!:"+error;

		// set up styles
		out += KMLName(name,NULL,-1);
		out += "<open>1</open>";
		
		
		//CDATAS+status+CDATAE
		out += KMLScanBox(config, cookies, status);

		// outpoints
		vars outpoints, outlines;

		//out += KMLFolderStart("Lat", "Lat", TRUE);
		for (int i=0; i<n; ++i)
			{
			CRapGroup &group = grouplist[i];
			if (group.raplist.GetSize()<1)
				continue;

			vars raps;
			vara list;
			float dist = 0;
			for (int r=0; r<group.raplist.GetSize(); r++)
				{
				trappel *rap = group.raplist[r];
				if (r>0)
					dist += (float)LLE::Distance(&rap->lle, &group.raplist[r-1]->lle);
				list.push(CCoord3(rap->lle.lat, rap->lle.lng));
				float grade = rap->avgg;
				int g = Rap(grade);
				vars icon = MkString("rgi%d", g);
				raps += "<div>";
				if (group.pivot==rap)
					raps += "<b>";
				//if (rap->wpresent)
				//	raps += "<img class=\"icon\" src=\"https://sites.google.com/site/rwicons/PW.png\"/>";
				//ASSERT(!strstr(rap->scan->name, "Foxgl"));
				float m = rap->m();
				vars raptext;
				if (metric)
					raptext = MkString("%.0fm", m);
				else
					raptext = MkString("%.0fft", m*m2ft);
				raps += MkString("<span class=\"sg%d %s\"><img class=\"icon\" src=\"http://sites.google.com/site/rwicons/%s.png\"/><span class=\"trg%d\">%s</span></span>", 
					min(3, rap->rflags[RSIDEG]), getewcflag(rap->rflags[RTHIS], ewc) ? "ewc": "", icon, g, raptext);
				vars wspans, wspane;
				if (getewcflag(rap->scan->flags[ETHIS], ewc))
					wspans = "<span class=\"ewc\">", wspane ="</span>";
				if (metric)
					raps += MkString(" %s@%.2fkm%s", wspans, dist/1000, wspane);
				else
					raps += MkString(" %s@%.2fmi%s", wspans, dist/1000*km2mi, wspane);
				raps += MkString(" Score:%.0f [%.0fpt]", rap->scoreg, rap->score);
				//raps += MkString(" Angle:%.0f&#186; ", g2deg(grade));
				//raps += MkString(" Grade:%.0f%%", grade*100);
				raps += MkString(" Res:%s", resText(rap->res));
				if (isBetterResG(rap->res, rap->resg))
					raps += MkString(" 2nd:%s", resText(rap->resg));
				if (group.pivot==rap)
					raps += "</b>";
				raps += "</div>";
#ifdef DEBUG
				raps += "<div>SCORE "+rap->Id()+":"+rap->logscore.join(",")+"</div>";
				raps += "<div>SCOREG:"+rap->logscoreg.join(",")+"</div>";
#endif
#ifdef SCANRAPS
				outpoints += KMLMarker( MkString("rgo%d", g), rap->lle.lat, rap->lle.lng, "  "+raptext, NULL);
#endif
				}
			list.push(CCoord3(group.raplist[r-1]->end.lat, group.raplist[r-1]->end.lng));

			vars name;
			if (group.rwsym)
				name += MkString("%s %s %s %s = ", group.rwsym->GetStr(ITEM_CONN), group.rwsym->GetStr(ITEM_WATERSHED), group.rwsym->GetStr(ITEM_NUM), group.rwsym->GetStr(ITEM_NUM+1));
			name += MkString("#%d %s %dr %s", group.rank, group.name,
				group.raplist.GetSize(), metric ? MkString("%.0fm",group.maxlen) : MkString("%.0fft",group.maxlen*m2ft));
			vars desc = "(edit Properties to add your notes here)\n\n\n<style>"
".icon { margin-bottom:-5px;height:20px;} "
".ewc { background-color:cyan; } "
".sg0 { border: 5px solid rgba(225,225,255,0.0); border-bottom:0; border-top:0; } "
".sg1 { border: 5px solid rgba(225,225,0,0.9); border-bottom:0; border-top:0; } "
".sg2 { border: 5px solid rgba(255,127,0,0.9); border-bottom:0; border-top:0; } "
".sg3 { border: 5px solid rgba(255,0,0,0.9); border-bottom:0; border-top:0; } "
".trg1 { font-size:larger; color:rgb(225,225,0); text-shadow: 0px 1px 0px #000000; }"
".trg2 { font-size:larger; color:rgb(255,127,0); text-shadow: 0px 1px 0px #000000; }"
".trg3 { font-size:larger; color:rgb(255,0,0); text-shadow: 0px 1px 0px #000000; }"
".trg4 { font-size:larger; color:rgb(120,120,120); text-shadow: 0px 1px 0px #000000; }"
"</style><div style='width:auto'>";
/*
color:rgb(255,127,0);
    text-shadow: 0px 1px 1px #000000, 0px -1px 1px #000000, -1px 0px 1px #000000, 1px 0px 1px #000000;
    
}
.tst2 {
color:rgb(255,127,0);
    text-shadow: 0px 1px #000000, 0px -1px #000000, -1px 0px #000000, 1px 0px  #000000;
    
}
.tst3 {
color:rgb(255,127,0);

}

.tst5 {
  color:rgb(255,127,0);
  -webkit-text-stroke: 0.5px black; 
}
*/

			desc += MkString("%d rappels ",  group.raplist.GetSize());
			if (metric)
				desc += MkString("(max %.0fm)", group.maxlen);
			else
				desc += MkString("(max %.0fft)", group.maxlen*m2ft);
#if 1
			if (metric)
				desc += MkString(" over %.2fkm (%.0fm elevation change)", group.length/1000, group.depth);
			else
				desc += MkString(" over %.2fmi (%.0fft elevation change)", group.length*km2mi/1000, group.depth*m2ft);
			desc += raps;
#else
			desc += raps;
			if (metric)
				desc += MkString("<div>Total: %.2fkm (%.0fm)</div>", group.length/1000, group.depth);
			else
				desc += MkString("<div>Total: %.2fmi (%.0fft)</div>", group.length*km2mi/1000, group.depth*m2ft);
#endif
			desc += "</div>";

			// useful stats
			desc += MkString("<div>Ranking: Pivot#%d Group#%d GroupAvg#%d</div>", group.score[SPIVOT], group.score[SGSUM], group.score[SGAVG]);

			vara watershed, watershed2;
			if (group.order>0)
				watershed.push(MkString("O%d L%d", group.order, group.level));
			if (group.km2!=InvalidNUM)
				{
				vars km2 = MkString("Drainage:%.0f", group.km2); 
				//if (group.mincfs!=group.maxcfs)
				//	cfs += MkString("~%.0f", group.maxcfs);
				km2 += "km2";
				watershed.push(km2);
				}
			if (group.cfs!=InvalidNUM)
				{
				vars cfs = MkString("AvgFlow:%.0f", group.cfs); 
				//if (group.mincfs!=group.maxcfs)
				//	cfs += MkString("~%.0f", group.maxcfs);
				cfs += "cfs";
				watershed2.push(cfs);
				}
			if (group.temp!=InvalidNUM)
				{
				vars temp = MkString("AvgTemp:%.0f", group.temp); 
				//if (group.mintemp!=group.maxtemp)
				//	temp += MkString("~%.0f", group.maxtemp);
				temp += "C";
				watershed2.push(temp);
				}

			desc += MkString("<div>%s %s</div>", watershed.join(" "), watershed2.join(" "));
			desc += MkString("<div>ScanID:%s %s</div>", IdStr(group.id), (group.oid!=group.id && group.oid!=0) ? IdStr(group.oid) : "");

			CScan *scan = group.raplist.Head()->scan;
			CScan *scanII = scan;
			for (int s=1; s<group.raplist.GetSize() && scanII==scan; ++s)
				scanII = group.raplist[s]->scan;

			// add button to run canyon mapper
			trappel *lastrap = group.raplist.Tail();

			// WARNING! not using maplinks for this shit! because it does not go with RWE!!!
			vars linkname = "@"; //url_encode(name.replace(",",";"))
			//CString url = MkString("http://%s/rwr/?rwl=%s,%s,%s,%s,SMC$", server, linkname, CCoord(scan->mappoint.lat), CCoord(scan->mappoint.lng), CData(lastrap->end.elev) + ";" + IdStr(scan->oid));

			// -112.193447;31.933110;562.500 -112.101144;32.000084;613.200 13224
			vara sum;
			BOOL waterfall = scan->oid==0;
			CScan *scan1, *scan2;
			float rapelev1, rapelev2;
			scan1 = scan2 = scan;
			rapelev1 = rapelev2 = 0;
			if (waterfall)
				{
				// waterfall
				rapelev1 = scan->rappels[0]->lle.elev;
				if (scanII!=scan)
					{
					// river join
					scan2 = scanII;
					rapelev2 = scan2->rappels[0]->lle.elev;
					}
				}
			else
				{
				// river only
				rapelev2 = scan->rappels[0]->lle.elev;
				}
				
			sum.push(vars(scan->name).replace(","," ") + " " + IdStr(scan->id));
			sum.push(vars(CCoord4(scan1->mappoint.lat, scan1->mappoint.lng, rapelev1, 0)).replace(",",";"));  // waterfall (elev=0 if disabled)
#ifdef SHOWSCAN
			vara clist;
			CScan *oscan = NULL;
			for (int s=0; s<group.raplist.GetSize(); ++s)
				{
				trappel *rap = group.raplist[s];
				if (oscan==NULL || rap->scan->oid!=oscan->oid || (oscan->oid==0 && rap->scan->id!=oscan->id))
					{
					oscan = rap->scan;
					vars scaninfo = MkString("%.6lf@%s:%d,%.6lfA%.6lf,%.2lf", rap->lle.lng, GetFileName(scan->file), scan->trktell, rap->lle.lat, rap->lle.ang, rap->lle.elev);
					clist.push(scaninfo.replace(",",";")); 
					}
				}
			sum.push(clist.join(";"));
#else
			sum.push(vars(CCoord4(scan2->mappoint.lat, scan2->mappoint.lng, rapelev2, 0)).replace(",",";"));  // river / joint (elev=0 if disabled)
#endif
			sum.push(vars(CCoord4(lastrap->end.lat, lastrap->end.lng, lastrap->end.elev, 0)).replace(",",";")); // last rappel (always enabled)
			//sum.push(MkString("%.0f", LLE::Distance(&scan2->mappoint, &lastrap->end)));
			CString url = MkString("http://%s/rwr/?rwl=%sS%d,R$", server, sum.join(","), scan2->version);
			desc += MkString("<br><b><input type=\"button\" id=\"map\" onclick=\"window.open('%s', '_blank');\" value=\"Run Canyon Mapper at this location\"></b><br>", url);

			CString url2 = MkString("http://ropewiki.com/index.php/Waterflow?location=%g,%g&watershed=on", group.pivot->lle.lat, group.pivot->lle.lng );
			desc += MkString("<b><input type=\"button\" id=\"map\" onclick=\"window.open('%s', '_blank');\" value=\"Run Waterfow Analysis on this stream\"></b><br>", url2);

			
			outlines += KMLLine(name, CDATAS+desc+CDATAE, list.flip(), grouplist[i].valid ? RGBA(0x80, 0xFF, 0, 0xFF) : RGBA(0x80, 0xFF, 0, 0), 5+(10*(n-i))/n, "<Snippet maxLines=\"0\"></Snippet>");
			}

#ifdef SCANRAPS
		// style for points
		out += KMLFolderStart("Rappel Data", "rapdata", TRUE, FALSE);
		for (int g=1; g<=4; ++g)
			{
			vars gid = MkString("rgo%d", g);
			DWORD c = RapColor(g);
			vars rgb = MkString("%6.6X", c==RAPINVERT ? RGB(0xFF,0xFF,0xFF) : c);
			out += 
			"<Style id=\""+gid+"0\">"
			"<IconStyle><scale>0.4</scale><Icon><href>http://sites.google.com/site/rwicons/"+gid+".png</href></Icon></IconStyle>"
			"<LabelStyle><color>ff"+rgb+"</color><scale>0.0</scale></LabelStyle>"
			"</Style>\n"
			"<Style id=\""+gid+"1\">"
			"<IconStyle><scale>0.4</scale><Icon><href>http://sites.google.com/site/rwicons/"+gid+".png</href></Icon></IconStyle>"
			"<LabelStyle><color>ff"+rgb+"</color><scale>1.0</scale></LabelStyle>"
			"</Style>\n"
			"<StyleMap id=\""+gid+"\"><Pair><key>normal</key><styleUrl>#"+gid+"0</styleUrl></Pair><Pair><key>highlight</key><styleUrl>#"+gid+"1</styleUrl></Pair></StyleMap>\n";
			}
		out += outpoints;
		out += KMLFolderEnd();
#endif
		out += outlines;
		//out += KMLRect(, NULL, *bbox, RGB(0xFF,0x00,0xFF), 8, 0);
		 //out += KMLFolderEnd();

	}

};

CString CRapGroup::Header(BOOL metric)
	{
		vars m = metric ? "m" : "ft";
		vars km = metric ? "km" : "mi";
		return "RANK,NAME,RAPS,LONGEST("+m+"),LENGTH("+km+"),DEPTH("+m+"),DRAINAGE(km2),FLOW(cfs),TEMP(C),RANKING:," SDESC ",DETAILS:,ID,OID,RAPINFO...";
	}

CString CRapGroup::Summary(int full, BOOL metric)
	{		  
		  // save performed scans
		  vara raps;
		  if (full)
			{
			for (int j=0; j<raplist.GetSize(); ++j)
			  raps.push(MkString("R%d:%s %s",j+1,IdStr(raplist[j]->lle.Id()),vars(raplist[j]->Summary()).replace(","," ")));
			raps.push(" ");
			}

		  if (!name)
			  return "";

		  CString rw;
		  if (rwsym) 
				rw = MkString("%s,%s,%s %s,%s,%s,=,", rwsym->GetStr(ITEM_CONN), rwsym->GetStr(ITEM_WATERSHED), rwsym->GetStr(ITEM_LAT), rwsym->GetStr(ITEM_LNG), rwsym->GetStr(ITEM_NUM), rwsym->GetStr(ITEM_NUM+1));

		  rw += MkString("#%d,%s,%d,%s,%.2f,%s,%s,%s,%s,", rank, name, raplist.GetSize(),  
			  numtoi(maxlen, metric ? 1 : m2ft), 
			  (metric ? length/1000 : length/1000*km2mi), 
			  numtoi(depth, metric ? 1 : m2ft), 
			  numtoi(km2), numtoi(cfs), numtoi(temp));
		  rw += ",";
		  for (int i=0; i<SMAX; ++i)
			  rw += MkString("%d,", score[i]);
		  rw += ",";
		  rw += MkString("%s,%s,", IdStr(id), IdStr(oid));
		  rw += raps.join(",");
		  return rw;
	}


CString trappel::Header()
{
		return "RAPID,GR,GS,HT,LAT,LNG,SCOREG,SCORE,RAPNUM,RFLAG,SFLAG,UFLAG,DFLAG";
}

CString trappel::Summary(CScan *scan, BOOL extra)
	{
		vara out;
		if (scan)
			out.push(MkString("%s:R%d@%s", IdStr(lle.Id()), scan->RapNum(this)+1, IdStr(scan->id)));
		else
			out.push(MkString("%s:", IdStr(lle.Id())));
		out.push( MkString("G%d,GS%d,%.0fm", Rap(avgg), rflags[RSIDEG], m()) );
		out.push( MkString("%.6f,%.6f %.3fa", lle.lat, lle.lng, lle.ang) );
		out.push( MkString("%.0fg", scoreg) );
		out.push( MkString("%.0fr", score) );
		if (scan)
			{
			out.push( MkString("RF%s,SF%s,UF%s,DF%s", FlagStr(rflags[RTHIS]), FlagStr(scan->flags[ETHIS]), FlagStr(scan->flags[EUP]), FlagStr(scan->flags[EDN])) );
			out.push(MkString("SCORELIST:%d", scorelist.GetSize()));
			for (int i=0; i<scorelist.GetSize(); ++i)
				out.push(scorelist[i]->Summary(FALSE));
			}

		return out.join(",");		
	}


enum { SCPR = 0, SCTIME, SCUSERS, SCORDER };

class ScanList {	
CSection CC;

public:

static int cmpscan( const void *arg1, const void *arg2 )
		{
			CSym *s1 = *((CSym**)arg1);
			CSym *s2 = *((CSym**)arg2);
			double p1 = s1->GetNum(SCPR);
			double p2 = s2->GetNum(SCPR);
			double t1 = s1->GetNum(SCTIME);
			double t2 = s2->GetNum(SCTIME);
			if (p1>p2) return -1;
			if (p1<p2) return 1;

			int val = p1>0 ? 1 : -1;
			if (t1>t2) return val;
			if (t1<t2) return -val;

			return strcmp(s1->id, s2->id);
		}

void ScanSave(CSymList &list)
{
	list.Sort(cmpscan);

	// compute order
	for (int mid=0; mid<list.GetSize() && list[mid].GetNum(SCPR)>=0; ++mid);
	for (int i=0; i<list.GetSize(); ++i)
		list[i].SetNum(SCORDER, i<mid ? i+1 : i-list.GetSize());
	list.Save(SCANFILE);
}

void ScanAdd(CSym &sym)
{
	CC.Lock();
	CSymList list;
	if (CFILE::exist(SCANFILE)) 
		list.Load(SCANFILE);
	list.Sort();

		int f = list.Find(sym.id);
		if (f<0)
			list.Add(sym);
		else
			{
			// update priority and user list
			double p = list[f].GetNum(SCPR);
			double newp = sym.GetNum(SCPR);
			if (abs(newp)>abs(p) || newp<0)
				{
				list[f].SetNum( SCPR, newp);
				if (abs(newp)>abs(p)) // reset time
					list[f].SetNum( SCTIME, sym.GetNum(SCPR));
				}
			//else
				//if (newp<0)
				//	list[f].SetNum( SCTIME, sym.GetTi);

			vars user = GetToken(sym.GetStr(SCUSERS), 0, ';');
			vara users( list[f].GetStr(SCUSERS), ";");
			if (users.indexOf(user)<0)
				{
				users.push(user);
				list[f].SetStr( SCUSERS, users.join(";") );
				}
			}

	ScanSave(list);

	CC.Unlock();
	// resume background calc (if any going on)
	extern CWinThread *scanth;
	if (scanth)
		scanth->ResumeThread();
}


BOOL ToDo(CSym &sym)
{
	CSymList list;
	CC.Lock();
	if (CFILE::exist(SCANFILE))
		list.Load(SCANFILE);
	CC.Unlock();
	if (!list.GetSize())
		return FALSE;
	// get last if neg priority, otherwise first
	sym = list[list.GetSize()-1];
	if (TMODE==1) // only Google
		{
		if (sym.GetNum(0)>0)
			return FALSE;		
		}
	else
		{
		BOOL disabled = scangONkey.IsDisabled() || TMODE==0; // no google
		if (sym.GetNum(0)>0 || disabled)
			sym = list[0];
		// exit if all is left is neg priority and scang disabled
		if (sym.GetNum(0)<0 && disabled)
			return FALSE;
		}
	Log(LOGINFO, "Background Scanning [%d quads pending]: %s", list.GetSize(), sym.Line());  
	return TRUE;
}

void ToDone(CSym &sym)
{
	CSymList list;
	CC.Lock();
	if (CFILE::exist(SCANFILE))
		list.Load(SCANFILE);
	int f = list.Find(sym.id);
	if (f>=0)
		{
		list.Delete(f);
		ScanSave(list);
		}
	CC.Unlock();
}



int RunScan(CSym &sym)
{
	// foreground scan
	CScanProcess scan;
	switch(scan.Scan(GetFileName(sym.id)))
		{
		case FALSE:
			Log(LOGALERT, "Scan failed for %s", sym.id);
			return TRUE;
		case -1:
			Log(LOGINFO, "Scan rescheduled for %s", sym.id);
			return FALSE;
		default:
			return TRUE;
		}
	return TRUE;
}

void RunScan(void)
{
	// background scan
	CSym sym;
	while (ToDo(sym))
		{
		// only activate googleelevation for high priority users
		double priority = sym.GetNum(0);
		// check if second pass is needed
		if (!RunScan(sym))
			{
			if (priority>0)
				{
				sym.SetNum(0, -priority);
				ScanAdd(sym);
				}
			continue;
			}

		ToDone(sym);
		}
}


BOOL ScoreScan(const char *config, const char *cookies, CKMLOut *out, inetdata *data)
{
	CScoreProcess process(config, cookies, out, data);
	return TRUE;
}


}  scanlist;



void  ScanCanyons(void)
{
	scanlist.RunScan();
}


void ScanAdd(const char *file, const char *user, int priority)
{
	scanlist.ScanAdd(CSym(GetFileName(file), MkString("%d,%f,%s", priority, GetTime(), user)));
}



void ScanCanyons(const char *file, LLRect *bbox)
{
	// command line scan
	CSymList list;
	if (file)
		{
			if (IsSimilar(GetFileExt(file),"CSV")) // csv file
				list.Load(file);
			else
			if (tolower(file[0])=='n') // grid file
				list.Add(CSym(file));
			else
				{
				// get scan
				Log(LOGERR, "use -getscan <ScanID>");
				return;
				}

		}
	else
	if (bbox)
		{
		CBox cb(bbox);
		for (LLRect *box =cb.Start(); box!=NULL; box=cb.Next())
			list.Add(CSym(cb.file()));
		}
	else
		{
		const char *exts[] = { "CSV", NULL };
		GetFileList(list, SCANFINAL, exts);
		}

	if (list.GetSize()==0)
		return;


	Log(LOGINFO, "Scanning %d Quads [%s...]", list.GetSize(), list[0].id);
	for (int i=0; i<list.GetSize(); ++i)
		{
		if (MODE==-20)
			{
			// add to scan.csv
			ScanAdd(list[i].id, "SERVER", 5);
			continue;
			}
		Log(LOGINFO,"Scanning Quad %d/%d %s", i, list.GetSize(), list[i].id);
		scanlist.RunScan(list[i]);
		}

}


void ScoreCanyons(const char *config, const char *cookies, CKMLOut *out, inetdata *data)
{
#ifndef DEBUG
	__try { 
#endif
  	scanlist.ScoreScan(config, cookies, out, data);
  
#ifndef DEBUG
} __except(EXCEPTION_EXECUTE_HANDLER) 
	{ 
	Log(LOGALERT, "REPORTSCAN CRASHED!!! %.200s", config); 	
	}
#endif
}


void ElevationSaveScan(CKMLOut &out)
{
		CSymList list;
		const char *exts[] = { "CSV", NULL };
		GetFileList(list, SCANFINAL, exts);
		for (int i = 0; i<list.GetSize(); ++i)
			{
			list[i].id = GetFileName(list[i].id);
			list[i].data = "";
			}
		list.Sort();

		CSymList scanfile;
		scanfile.Load(SCANFILE);
		for (int i=0; i<scanfile.GetSize(); ++i)
			{
			int f = list.Find(scanfile[i].id);			
			if (f<0)
				list.Add(scanfile[i]);
			else
				list[f].data = scanfile[i].data;
			}

		int colorscnt[3] = { 0, 0, 0};
		COLORREF colors[] = { RGB(0xFF, 0, 0), RGB(0xFF, 0xFF,0), RGB(0, 0xFF,0) };
		out += KMLMarkerStyle("order", "http://maps.google.com/mapfiles/kml/shapes/placemark_circle.png", 0.0, 0.6);
		out += KMLName("Scan Status", "This map shows the coverage and resolution of the Scan data.<ul><li>Red : Scan pending</li><li>Yellow : 2nd pass scan pending</li><li>Green : Fully scanned </li></ul>");
		for (int i=0; i<list.GetSize(); ++i)
			{
			// output CSV
			CScanList scanlist;
			CString scanfile = list[i].id;
			int load = scanlist.Load(scanfile) && scanlist.IsValid(INVVERSION);

			LLRect bbox;
			if (!CBox::box(scanfile, bbox))
				{
				Log(LOGERR, "BAD SCAN FILE %s", scanfile);
				continue;
				}
			int color = load ? 2 : 0;
			int invv = scanlist.Invalids(INVVERSION), invg = scanlist.Invalids(INVGOOG);
			if (invg>0)				
				color = min(color, 1);
			if (invv>0)
				color = min(color, 0);
			out += KMLRect(scanfile, MkString("Status:%d/%d\nInvV:%d InvG:%d", scanlist.Scanned(), scanlist.Total(), invv, invg), bbox, colors[color]); //jet(res));

			if (!list[i].data.IsEmpty())
				{
				double pr = list[i].GetNum(SCORDER);
				vars name = pr>0 ? MkString("#%s", CData(pr)) : MkString("#G%s", CData(abs(pr)));
				out += KMLMarker("order", (bbox.lat1+bbox.lat2)/2, (bbox.lng1+bbox.lng2)/2, name, list[i].GetStr(SCUSERS));
				}

			++colorscnt[color];
			}
		//Log(LOGINFO, "%s: %dRed + %dYellow + %dGreen = %d", "SCAN.KML", colorscnt[0], colorscnt[1], colorscnt[2], list.GetSize());
}


void ElevationSaveScan(const char *file)
{
	   if (!file)
			{
			ElevationSaveScan(CKMLOut(POI"\\SCAN"));
			return;
			}

		if (tolower(*file)=='n')
			{
			// output CSVn46w123.csv file
			CScanList scanlist;
			scanlist.Load(file);
			vars csvfile = MkString("CSV%s.csv",file);
			Log(LOGINFO, "Generating %s", csvfile);
			scanlist.SaveCSV(csvfile);
			return;
			}

		CSymList list;
		const char *exts[] = { "CSV", NULL };
		GetFileList(list, SCANFINAL, exts);
		// find riverid and output CSVfile
		for (int i=0; i<list.GetSize(); ++i)
			{
			CScanList slist;
			printf("%s    \r", list[i].id);
			slist.Load(list[i].id);
			for (int j=0; j<slist.GetSize(); ++j)
				if (IsSimilar(IdStr(slist[j].id),file))
					{
					vars datafile = GetFileName(list[i].id);
					Log(LOGINFO, "%s: %s", datafile, slist[j].Summary());
					if (IsSimilar(file, "R"))
					{
					CRiverCache obj(datafile);
					if (obj.rlist)
						{
						CRiver *r = obj.rlist->Find(StrId(file));
						if (r)
							{
							vars sum = r->sym->GetStr(ITEM_SUMMARY);
							Log(LOGINFO, "RIVER TEST:%s,%s", IdStr(r->id), sum.replace(" ",","));
							}
						}
					}
					vars csvfile = MkString("CSV%s.csv", datafile);
					Log(LOGINFO, "%s saved as %s", datafile, csvfile);
					slist.SaveCSV(csvfile);
					}
			}
		return;
}

void IdCanyons(LLRect *bbox, const char *id)
{
	double idv = 0;
	FromHex(id, &idv);

	CBox cb(bbox);
	for (LLRect *box =cb.Start(); box!=NULL; box=cb.Next())
		{
		CRiverCache cache(cb.file());
		CRiver *r = cache.rlist->Find(idv);
		if (r)
			{
			float lat, lng;
			if (RiverLL(r->GetStr(ITEM_SUMMARY), lat, lng))
				Log(LOGINFO, "%s=[%.6f,%.6f]=%s\n%s", IdStr(r->id), lat, lng, r->sym->id, r->sym->data);
			}
		}
}


int ComputeCoordinates(const char *kml, double &length, double &depth, LL SE[2])
{
	LLRect bb;
	CSym sym;
	if (KMLSym("", "", ExtractString(kml, "<coordinates>", "", "</coordinates>"), bb, sym)<1)
		return FALSE;
	vara summaryp(sym.GetStr(ITEM_SUMMARY), " ");
	if (summaryp.length()!=3)
		return FALSE;
	if (!RiverLL(summaryp[0], SE[0].lat, SE[0].lng) || !RiverLL(summaryp[1], SE[1].lat, SE[1].lng))
		return FALSE;
	length = CGetNum(summaryp[2]);
	if (length<50) // 50m min
		length = InvalidNUM;

	tcoords coordsp(2);
	coordsp[0] = tcoord(SE[0].lat, SE[0].lng);
	coordsp[1] = tcoord(SE[1].lat, SE[1].lng);
	maingkey.getelevationGOOG(coordsp, TRUE);
	
	if (coordsp[0].elev>=0 && coordsp[1].elev>=0)
		{
		depth = coordsp[1].elev - coordsp[0].elev;
		if (abs(depth)<10) // 10m min
			depth = InvalidNUM;
		}

	return TRUE;
}

/*
void GetRiversWG(const char *folder)
{
	//CScanProcess scan;
	for (int r=0; RIVERSALL[r]!=NULL; ++r)
		{
		CSymList filelist;
		const char *exts[] = { "CSV", NULL };
		GetFileList(filelist, folder ? folder : filename(NULL, RIVERSALL[r]), exts);
		for (int i=0; i<filelist.GetSize(); ++i)
			{
			CString file = filelist[i].id;
			printf("Processing %d%% (%d/%d) %s  \r", (i*100)/filelist.GetSize(), i, filelist.GetSize(), file);
			CRiverList riverlist(file);
			LLRect bbox;
			CBox::box(file, bbox);
			CScanProcess::ScanRivers(riverlist, &bbox, wgstamp);
			// update file
			SaveKeepTimestamp(file, riverlist.SymList());
			}
		if (folder) break;
		}
}
*/

// Mt hood 45.854175,-121.898934, 45.155298, -121.201284
// Portland area  45.792067, -122.407607°, 45.155298, -121.201284 




const char *AuthenticateUser(const char *cookies)
{
	if (!cookies) return NULL;

	const char *keystr = strstr(cookies, "KEY=");
	if (!keystr) return NULL;

	CString key = GetToken(keystr+4, 0, " ;\n");
	if (key.IsEmpty()) return NULL;

	// check key
	static CSymList keylist;
	if (keylist.GetSize()==0)
		keylist.Load(filename("rwskeys_keepconfidential"));
	int usernum = keylist.Find(key);
	if (usernum<0) return NULL;

	// success
	return keylist[usernum].data;
}


void Test(const char *file)
{
	TestRogues();
}


static int countlle(CRiver *r, LLEC *lle, double maxdist, void *data)
{
	int &counter = *((int *)data);
	++counter;
	return TRUE;
}


static int getwatershed(CRiver *r, LLEC *lle, double maxdist, void *data)
{
	double d  = r->DistanceToLine(lle->lat, lle->lng);
	if (d<maxdist)
		{
		double od = lle->sym()->GetNum(SITE_WATERSHEDDIST);
		char *sep = " ";
		vars oname = GetToken(lle->sym()->GetStr(SITE_WATERSHED), 0, sep);
		vars llename = GetToken(lle->sym()->GetStr(SITE_DESC), 0, sep);
		vars rname = GetToken(r->sym->id, 0, sep);
		BOOL closer = od-d<=1;
		BOOL rmatch = IsSimilar(llename, rname) && !rname.IsEmpty();
		BOOL omatch = IsSimilar(llename, oname) && !oname.IsEmpty();

		/*
		// Test
		if( strcmp(lle->sym()->id, "USGS:14128600")==0 || strcmp(lle->sym()->id, "USGS:14128870")==0)
			{
			CSym *sym = lle->sym();
			CSym *rsym = r->sym;
			sep = sep;
			}
		*/

		// skip some cases
		if ((rmatch && !omatch) || (closer && omatch==rmatch))
			{
			if (od==InvalidNUM)
				{
				int &counter = *((int *)data);
				++counter;
				}
			lle->sym()->SetStr(SITE_WATERSHEDDIST, MkString("%s;%s", CData(d), IdStr(r->id)));
			lle->sym()->SetStr(SITE_WATERSHED, r->sym->id + " " + WatershedSummary(*r->sym).join(" "));
			}
		}
	return FALSE;
}


void WatershedSaveSites(const char *name)
{
	// get lists
	CSymList filelist;
	const char *exts[] = { "CSV", NULL };
	GetFileList(filelist, "WF", exts);
	
	// load lists
	LLEProcess proc;
	CSymList *lists = new CSymList[filelist.GetSize()];
	for (int i=0; i<filelist.GetSize(); ++i)
		if (name==NULL || IsSimilar(name, GetFileName(filelist[i].id)))
			{
			lists[i].Load(filelist[i].id);
			proc.Load(lists[i]);
			}

	// compute bbox
	LLRect bbox;
	LLECLIST &list = proc.LLElist();
	for (int i=0; i<list.GetSize(); ++i)
		bbox.Expand(list[i]->lat, list[i]->lng);

	// process all river
	CBox cb(&bbox);
	int matchcounter = 0;
	for (LLRect *box =cb.Start(); box!=NULL; box=cb.Next())
		{
		vars file = cb.file();
		LLRect filebox;
		if (!CBox::box(file, filebox))
			{
			Log(LOGERR, "BAD SCAN FILE %s", file);
			continue;
			}
		
		// check if there are gauges there		
		int counter  = 0;
		proc.MatchBox(CRiver(&filebox), MINRIVERPOINT, countlle, &counter);
		if (counter==0)
			continue;

		printf("Finding watershed %s [%d]...  \r", file, matchcounter);
		CRiverCache cache(file);
		if (cache.rlist)
			proc.MatchBox(cache.rlist, MINRIVERPOINT*2, getwatershed, &matchcounter);
		}

	// save lists
	for (int i=0; i<filelist.GetSize(); ++i)
		lists[i].Save(filelist[i].id);

	delete [] lists;
}


