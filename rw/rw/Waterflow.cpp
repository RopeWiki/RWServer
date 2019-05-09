#include <afxinet.h>
#include "stdafx.h"
#include "trademaster.h"
#include "math.h"
#include "fcgiapp.h"
#include "afxmt.h"
#include "rw.h"

#include "passwords.h"

#define MIN100MDEG 0.02 // 0.02 degrees 2km   1/60 = 0.0167 = 1.8km    1km = 0.0093 ~ 0.001
#define MIN100M 150.0  // 150m
#define MIN5KM 5000  // 5Km
#define MIN1KM 1000  // 1Km
#define USGS "USGS"
#define INVERSE(x) (CString(x)+"-")

#define MAXTODAY 2  // max difference between ropewiki date and latest flow

// SENSORS  0:cfs 1:ft 2:TF 3:TC 4:Rcfs 5:valid


/*
#include <iostream> 
#include <string> 
#import "C:\Program files\Common Files\System\Ado\msado15.dll" 
rename("EOF", "ADOEOF") 
*/

static int testing = 0, logging = 0;

// misc
#define null NULL

StdDev::StdDev(double _n, double _acc, double _acc2)
	{
	n = _n;
	acc = _acc;
	acc2 = _acc2;
	}	

StdDev& StdDev::operator+=(StdDev &std)
	{
	n += std.n;
	acc += std.acc;
	acc2 += std.acc2; 
	return *this;
	}

int StdDev::Cmp(StdDev &std)
	{
	double d;
	d = n - std.n;
	if (d>0) return 1;
	if (d<0) return -1;

	d = acc - std.acc;
	if (d>0) return 1;
	if (d<0) return -1;

	d = acc2 - std.acc2;
	if (d>0) return 1;
	if (d<0) return -1;

	// perfect match!
	return 0;
	}


double StdDev::Avg(double realn)
	{
	if (realn<0) realn = n;
	if (realn<=0) return 0;
	return acc / realn;
	}

double StdDev::Std(double realn)
	{
	if (realn<0) realn = n;
	if (realn<=0) return 0;

	double var = 0;
	if (realn>1) 
		var = (acc2 - acc * Avg()) / (realn-1); 
	if (var<EPSILON) var = 0; // rounding error

	return sqrt(var);
	}

double StdDev::Shp(double realn)
	{
	if (realn<0) realn = n;
	if (realn<=0) return 0;
	double avg = Avg();
	double std = Std();
	if (std!=0) return avg/std;
	return avg/fabs(avg)*100;
	}

/*

#define NCSECTION 5
class CSection {
static int violations;
volatile int csection[NCSECTION];
public:

CSection(void)
{
	for (int i=0; i<NCSECTION; ++i)
		csection[i] = 0;
}


inline int isLocked(register int n)
{
	for (register int i=n, locked=0; i<NCSECTION; ++i, locked += csection[i]);
	return locked;
}

void Lock(void) 
{
	// simulate csection
	for (register int n=0; n<NCSECTION; ++n)
		for (register int i=n, locked=0; i<NCSECTION; ++i, locked += csection[i]);			
				{
				while (csection[i]>0)
					Sleep(100);
				++csection[i];
				}
	// keep track of violations
	if (csection[NCSECTION-1]>1)
		{
		++violations;
		Log(LOGERR, "CSection Violantions %d", violations);
		}
} 

void Unlock(void) 
{
	for (int i=NCSECTION-1; i>=0; --i)
		--csection[i];
}

};
*/





//CSection::violations = 0;



//===========================================================================
// Waterflow structs and aux
//===========================================================================

class tavg
{
	//int cnt;
	//double avg;
	//double last;

	CDoubleArrayList list;

public:
	tavg(void)
	{
		//cnt = 0; avg = 0; last = -1;
	}

	void set(double nv)
	{
		  //avg = last = nv, cnt = 1;
	   if (nv!=InvalidNUM)
			{
			list.SetSize(1);
			list[0] = nv;
			}
	}

	inline int cnt(void)
	{
		return list.GetSize();
	}


	void add(double nv)
	{
	// if (last<0 || (nv>=last/5 && nv<=last*5))
	//    avg += last = nv, ++cnt;
	   if (nv!=InvalidNUM)
		   list.push(nv);
	}

	double get(double &res)
	{
		#define MAG 4  // max magnitude change
		int n = list.GetSize();
		if (n<=0)
			return InvalidNUM;
		//return avg/cnt;
		if (n==1)
			return res = list[0];
		if (n==2)
			return res = (list[0]+list[1])/2;
		// average and clean up
		int vcnt = 0;
		double vavg = 0;
		for (int i=0; i<n; ++i)
			{
			double v = list[i];
			double v1 = i-1>=0 ? list[i-1] : list[i+2];
			double v2 = i+1<n ? list[i+1] : list[i-2];
			double mag = v>0 ? MAG : 1/MAG;
			if ((v/mag>v1 && v/mag>v2) || (v*mag<v1 && v*mag<v2))
				continue; // BAD DATA!
			vavg += v, ++vcnt;
			}
		if (vcnt>0)
			return res = vavg/vcnt;
		return InvalidNUM;
		//return res = list[n/2];
	}

};

#if 0
static int DownloadRetry(DownloadFile &f, const char *url, int retry = 2, int delay = 3)
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
#else
#define DownloadRetry(f,url) f.Download(url)
#endif



CSymList sites;
CString basedir("WF");

vars addmode(const char *mode, const char *newmode)
{
	vara modep = vars(mode).split(";");
	vara nmp = vars(newmode).split(";");

	for (int i=0; i<nmp.length(); ++i)
		if (modep.indexOf(nmp[i])<0)
			modep.push(nmp[i]);
	return modep.join(";");
}

const char *siteshdr = "id,desc,lat,lng,modes,extra,equiv";

#define isNaN(v) (v==InvalidNUM)



vars cleanup(vars str)
{
const char *stdsubst[] = { 
"\r", " ",
"\n", " ",
"\t", " ",
","," ", 
";"," ",
"  ", " ",
" -", "-",
"- ", "-",
"\"", "",
NULL
};

int i=0;
while (stdsubst[i]!=NULL)
	{
	str.Replace(stdsubst[i], stdsubst[i+1]);
	i+=2;
	}
return str.trim();
}

vara states = vars("AL,AK,AQ,AZ,AR,CA,CO,CT,DE,DC,FL,GA,HI,ID,IL,IN,IA,KS,KY,LA,ME,MD,MA,MI,MN,MS,MO,MT,NE,NV,NH,NJ,NM,NY,NC,ND,MP,OH,OK,OR,PA,PR,RI,SC,SD,TN,TX,UT,VT,VI,VA,WA,WV,WI,WY").split(); //GU

vars simplify(const char *str)
{
const char *subst[] = { 
"BLACK", "BLK",
"GRN", "GRN",
"RIVER", "R",
"CREEK", "C",
"CR", "C",
"CK", "C",
"CANAL", "CN",
"NEAR", "NR",
"AT", "NR",
"@", "NR",
"ABV", "AB",
"ABOVE", "AB",
"BLW", "BL",
"BELOW", "BL",
"LAT", "LAT",
"LATERAL", "LAT",
"LAKE", "LK",
"RESERVOIR", "RES",
"TRIBUTARY", "TRIB",
"ROAD", "RD",
"LANE", "LN",
"STREET", "ST",
"FORK", "F",
"FK", "F",
"WE", "W",
"EA", "E",
"NO", "N",
"SO", "S",
"NORTH", "N",
"NORTHEAST", "N E",
"NORTHWEST", "N W",
"SOUTH", "S",
"SOUTHEAST", "S E",
"SOUTHWEST", "S W",
"WEST", "W",
"EAST", "E",
"SAN", "S",
"SANTA", "S",
"SAINT", "S",
"ST", "S",
"MOUNT", "MT",
"LOWER", "L",
"UPPER", "U",
"RIGHT", "R",
"MIDDLE", "M",
"LEFT", "L",
"BRANCH", "BR",
"NEBR", "NE",
NULL
};
	vars s(cleanup(str)); 
	s.MakeUpper();
	s.Replace(".","");
	s.Replace("'","");
	vara text = s.split(" ");
	for (int i=0; i<text.length(); ++i)
		{
		for (int j=0; subst[j]!=NULL; j+=2)
			{
			if (strcmp(text[i], subst[j])==0)
				{
					text[i] = subst[j+1];
					break;
				}
			}
		}
	// cleanup
	if (text.length()>0)
		{
		BOOL del = FALSE;
		vars last = text[text.length()-1];
		for (int i=0; !del && i<states.length(); ++i)
			if (last==states[i])
				del = TRUE;
		if (del)
			text.splice(text.length()-1, 1);
		}
	for (int i=text.length()-1; i>=0; --i)
		if (text[i].indexOf("(")>=0)
			text.splice(i, 1);

return text.join(" ");
}


int findsite(const char *name)
{
	return sites.Find(name);
}

const char *msep = "~";

int addsite(CSymList &sites, const char *name, const char *description, const char *lat, const char *lng, const char *mode = "", const char *extra = "", const char *equiv = "", BOOL auxsites = FALSE)
{
	double vlat = CDATA::GetNum(lat), vlng = CDATA::GetNum(lng);
	if (!auxsites)
		{
		vara ids = vars(name).split(":");
		if (ids.length()!=2 || ids[0].length()==0 || ids[1].length()==0)
			{
			Log(LOGERR, "invalid site name %s", name);
			return -1;
			}
		if (vlat==InvalidNUM || vlng==InvalidNUM || vlat==0 || vlng==0)
			{
			Log(LOGERR, "invalid lat/lng name %s %s,%s", name, lat, lng);
			return -1;
			}
		if (strncmp(name, USGS, 4)!=0)
		  if (vlat<=0 || vlng>=0)
			{
			Log(LOGERR, "neg/pos lat/lng mismatch name %s %s,%s", name, lat, lng);
			return -1;
			}
		}

	vara data;
	vars desc(description);
	data.push(cleanup(desc));	
	data.push(cleanup(lat));	
	data.push(cleanup(lng));	
	data.push(mode);
	data.push(extra);
	data.push(equiv);
	int af=sites.Find(name);
	if (af<0)
		{
		CSym sym(name, data.join());
		sym.index = -1; // new
		sites.Add(sym);
		return sites.GetSize()-1;
		}
	sites[af].index = 1; // updated

	// merge new and old
	vara odata = vars(sites[af].data).split();
	for (int j=odata.length(); j<SITE_NUM; ++j)
		odata.push("");
	for (int j=data.length(); j<SITE_NUM; ++j)
		data.push("");

	//add mode and extra (if needed)
	data[SITE_MODE] = addmode(odata[SITE_MODE], mode);
	data[SITE_EQUIV] = addmode(odata[SITE_EQUIV], equiv);
	if (auxsites && extra!="" && strncmp(extra, "http", 4)!=0)
		data[SITE_EXTRA] = addmode(odata[SITE_EXTRA], extra);

	// preserve any old data no longer available or fixed data (*) 
	for (int j=0; j<SITE_NUM; ++j)
		if (data[j]=="" || odata[j].Right(1)=="*")
			data[j] = odata[j];

	sites[af].data = data.join();
	return af;
}

CString filename(const char *name)
{
	vara id = vars(name).split(":");
	if (id.length()<2) 
		return basedir+"\\"+name+".csv";
	return basedir+"\\"+id[0]+"\\"+id[1]+".csv";
}

BOOL istoday(const char *str)
{
	return str[10] == 'T';
}



#define InvalidURL -1

class CSite;
typedef int fgetsites(const char *name, CSymList &sitelist, int active);
typedef void fgetdate(CSite *id, vars &date, double *res);
typedef int fgethistory(CSite *id);

typedef struct {
	const char *name, *cfs;
	fgetsites *getsites;
	fgetdate *getdate;
	fgethistory *gethistory;
	const char *testsite;
	BOOL calcgroup;
	const char *urls[4];
	int cnt;
} tprovider;

extern tprovider providers[];
tprovider *getprovider(const char *str);

inline int IsValid(double *val)
{
		return val[0]>=0 || val[1]>=0;
}


inline double GetVal(const char *line)
{
	// <0 is invalid >0 is time left
	if (!line || *line==0)
		return -1;

	double curd = GetTime();
	double vald = CDATA::GetNum( GetToken(line, GetTokenCount(line)-1) );
	return vald-curd;
}





vars SetDateVal(const char *date, double *val, const char *oldline = NULL)
	{
		// validity: 30d old valid 30d, 7d valid 7d, 1d valid 1d etc.
		double curd = GetTime();
		double dated = CDATA::GetDate( date );
		double x = curd-dated; 
		// retry today or tomorrow in 5min
		if (istoday(date) || x<=0)
			x = 5.0/60.0/24.0; 
		// retry failed URLs immediately
		if (val[0]==InvalidURL) // || val[1]==InvalidURL)
			x = 0;

		vara line;
		line.push( vars(date, 10) ); // date
		for (int i=0; i<5; ++i) //data		  
		  {
		  CString v;
		  if (val[i]>=0) 
			 v = CData(val[i]);
		  else
			  if (oldline && *oldline!=0)
				  if (GetTokenCount(oldline)-1>1+i)
					v = GetToken(oldline, 1+i);
		  line.push(v);
		  }
		// retry invalids immediately
		if (line[1].IsEmpty() && line[2].IsEmpty() && line[5].IsEmpty())
			x = 0; 
		line.push( CData(curd+x) ); //valid
		return line.join();
	}


typedef struct {
	double G, Q;
} tGQ;
	
#define VRESNULL double vres[6] = { InvalidNUM, InvalidNUM, InvalidNUM, InvalidNUM, InvalidNUM, InvalidNUM };

typedef CArrayList<CSite*> CSiteList;

class CSite {
	double loadsave;
	CLineList data;
	CArrayList<tGQ> GQ;
	CSection CCDATA;
	CSection CCLOCK;
	BOOL gothistory;
public:
	CSym *sym;
	CString id, fid, gid;
	tprovider *prv;
	CSiteList group;

	inline int GetSize(void) { return data.GetSize(); }
	inline double Modified(void) { return loadsave; }

	void Lock()
	{
		CCLOCK.Lock();
	}

	void Unlock()
	{
		CCLOCK.Unlock();
	}

	inline CString Info(int i = -1)
	{
		if (i<0)
			return sym->Line();
		return sym->GetStr(i);
	}

	CSite(CSym *s)
	{
	sym = s;
	fid =  sym->id;

	id = vars(fid).split(":")[1];
	prv = getprovider(fid);	
	if (prv && prv->calcgroup)
		{
		vara mode = vars(sym->GetStr(SITE_MODE)).split("=");
		if (mode.length()>0)
			gid = mode[0];
		}
	if (!prv)
		Log(LOGERR, "Provider not found for %s", fid);
	loadsave = -1;
	gothistory = FALSE;
	}

	void GetDate(vars &date, double *res)
	{
		// get value
		prv->getdate(this, date, res);
	   // temp C-> F
	   if (res[2]<0 && res[3]>=0)
		   res[2]= res[3] * 9 / 5 + 32;

#if 0	// using DownloadRetry with autoretry
		// retry bad urls after 5s
		if (res[0]==InvalidURL)  //res[1]==InvalidURL
			{
			if (logging)
				Log(LOGINFO, "SLEEPING TO RETRY %s %s", fid, date);
			Sleep(3000); // wait 3s and retry
			prv->getdate(this, date, res);
			}
#endif

		if (logging) 
			Log(LOGINFO, "GETDATE() -> %s %s = %s, %s, %s, %s", fid, date, CData(res[0]), CData(res[1]), CData(res[2]), CData(res[3]));
	}



	inline int Load(void)
	{
		CCDATA.Lock();
		int ret = FALSE;
		if (loadsave<0)
			{
			// load historicals
			loadsave = 0;
			data.Header() = "DATE,FLOW,STAGE,TEMP,VDATE";
			data.Load(filename(fid));
			int cnt = data.Purge(0);
			if (cnt>0)
				{
				Log(LOGWARN, "PURGED %d lines for %s", cnt, fid);
				loadsave = GetTime();
				}
			ret = TRUE;
			}
		CCDATA.Unlock();
		return ret;
	}

	inline int Save(void)
	{
		// update with new 
		//CLineList file;
		//file.Load(fname);
		//file.Merge(data,0);
		CCDATA.Lock();
		int ret = FALSE;
		if (loadsave>0)
			{
			if (logging)
				Log(LOGINFO, "Saving %s", filename(fid));
			data.Save(filename(fid));
			loadsave = 0;
			ret = TRUE;
			}
		CCDATA.Unlock();
		return ret;
	}

	void SetDate(double datev, double v, int sensor = 0)
	{
		VRESNULL;
		vres[sensor] = v;
		vars date = CDate(datev);
		SetDate(date, vres);
	}

	void SetDate(vars &date, double *val)
	{
		Load();
		CCDATA.Lock();
		int f = data.Find(CString(date,10));
		if (f<0)
			{
			// add
			date = SetDateVal(date, val);
			data.Add(date);
			}
		else
			{
			// merge
			date = SetDateVal(date, val, data[f]);
			data.Set(f, date);
			}
		loadsave = GetTime();
		CCDATA.Unlock();
	}
	
static int GQsort(const tGQ *arg1, const tGQ *arg2)
	{
		if (arg1->G < arg2->G)
			return -1;
		if (arg1->G > arg2->G)
			return 1;
		// same G
		if (arg1->Q < arg2->Q)
			return -1;
		if (arg1->Q > arg2->Q)
			return 1;
		// same G and same Q
		return 0;
	}


#define MAXGQ 365 // max number of data points
#define MINGQ 5 // minimumm number of data points

#define rnd(x) (x) //((int)((x+0.5)*1000))

inline int GQcmp(register tGQ *GQi, register tGQ *GQj)
{
	register double Gd = (GQi->G - GQj->G);
	register double Qd = (GQi->Q - GQj->Q);
	return (Gd*Qd<0 || Gd==0 || Qd==0);
}


void GQSave(const char *file, int n, tGQ *GQ, int *err)
{
	CFILE f;
	f.fopen(file, CFILE::MWRITE);
	for (int i=0; i<n; ++i)
		f.fputstr(MkString("%s,%s,%d",CData(GQ[i].Q),CData(GQ[i].G),err[i]));
}

void ComputeFlow(void)
	{
		// load substantial set
		GQ.SetSize(0);
		for (int i=data.GetSize()-1; i>=0 && GQ.GetSize()<MAXGQ; --i)
			{
			if (data[i][11]=='C')
				continue;
			vara linep = vars(data[i]).split();
			// skip computed lines
			tGQ gq; 
			gq.Q = CDATA::GetNum(linep[1]);
			if (gq.Q==InvalidNUM && linep.length()>5+1)
				gq.Q = CDATA::GetNum(linep[5]);
			gq.G = CDATA::GetNum(linep[2]);
			if (gq.G>0 && gq.Q>0)
			  GQ.AddTail(gq);
			}

		  // sort and polish
		  GQ.Sort(GQsort);
		  //if (GQ.GetSize()>MINGQ)
		  //	ASSERT(id!="TSP");

		  // compute error array
		  CArrayList<int> err; err.SetSize(GQ.GetSize());
		  for (register int i=err.GetSize()-1; i>=0; --i)
			  err[i] = 0;

		  register int maxerr = 0, maxj = -1;
		  for (register int i=GQ.GetSize()-1; i>=0; --i)
			for (register int j=GQ.GetSize()-1; j>=0; --j)
			  if (i!=j)
				if ((err[j] += GQcmp(&GQ[i], &GQ[j]))>maxerr)
					maxerr = err[maxj = j];

		
		BOOL oldmaxerr = maxerr;
		if (maxerr>0)
			if (logging)
				GQSave(vars(fid).split(":").join(".")+".GQ0.csv", GQ.GetSize(), &GQ[0], &err[0]);

		while (maxerr>0)
				{
				// delete offender
				int i = maxj;
				maxerr = err[i] = 0;
				for (register int j=GQ.GetSize()-1; j>=0; --j)
					if (i!=j)
					  if ((err[j] -= GQcmp(&GQ[i], &GQ[j]))>maxerr)
					   maxerr = err[maxj=j];
				err.Delete(i);
				GQ.Delete(i);
				if (maxj>i)
					--maxj;
				}

		// check for errors
		int error = 0;
		for (int i=1; i<GQ.GetSize()-1; ++i)
			{
			if (GQ[i-1].G>=GQ[i].G || GQ[i].G>=GQ[i+1].G)
				++error;
			if (GQ[i-1].Q>=GQ[i].Q || GQ[i].Q>=GQ[i+1].Q)
				++error;
			}
		if (error)
			Log(LOGERR, "ERROR: GQ not properly sorted for %s", fid);

		if (oldmaxerr || error)
			if (logging)
				GQSave(vars(fid).split(":").join(".")+".GQ1.csv", GQ.GetSize(), &GQ[0], &err[0]);
	}

	void GetFlow(vars &date)
	{
		vara line = date.split();
		if (line.length()<2 || !line[1].IsEmpty())
			return; // ok

		if (line.length()>5 && !line[5].IsEmpty())
			{
			// get reference
			line[1] = line[5];
			line[0] += "E";
			date = line.join();
			return;
			}

		if (line[2].IsEmpty())
			return; // can't compute

		// try to download 1y of data
		Lock();
		if (GQ.GetSize()<MINGQ)
			ComputeFlow();
		if (GQ.GetSize()<MINGQ)
			if (prv->gethistory && !gothistory)
				{
				if (logging) 
					Log(LOGINFO, "GETHISTORY %s %s", fid, date);
				prv->gethistory(this);
				ComputeFlow();
				gothistory = TRUE;
				}
		Unlock();
		if (GQ.GetSize()>=MINGQ)
			{
			// find nearest match, avoid i out of bounds
			double G = CDATA::GetNum(line[2]);
			for (int i=1; i<GQ.GetSize()-1 && GQ[i].G<G; ++i);
			double minG = GQ[i-1].G;
			double minQ = GQ[i-1].Q;
			double maxG = GQ[i].G;
			double maxQ = GQ[i].Q;
			//double Q = minQ + (maxQ - minQ) * (G-minG) / (maxG-minG); // lineal interp
			double Q = exp(log(minQ) + (log(maxQ) - log(minQ)) * (log(G)-log(minG)) / (log(maxG)-log(minG))); // logarithm interp

			/*
			// compute Ge & Gc as per https://pubs.usgs.gov/twri/twri3-a10/pdf/TWRI_3-A10.pdf
			int m = GQ.GetSize()-1;
			double Gmin = GQ[0].G;
			double Gmax = GQ[m].G;
			double Qmin = GQ[0].Q;
			double Qmax = GQ[m].Q;
			double G1 = Gmin, G2 = Gmax, G3 = G;
			double Q1 = Qmin, Q2 = Qmax;
			double Geb = (G1*G2-G3*G3)/(G1+G2-2*G3);
			double Ge = (10*Gmin-Gmax)/9;
			double Gx = (G1-Ge)/(G2-Ge);
			double b = log(Q1/Q2)/log(Gx);
			double P = Q1 / pow(G1-Ge, b);
			double Pb = Q2 / pow(G2-Ge, b);
			double Qb = P*pow(G-Ge, b);
		#ifdef DEBUG
			Log(LOGINFO, "Q COMPUTATION %s %s = Gmin=%s Gmax=%s Qmin=%s Qmax=%s P=%s Pb=%s b=%s Ge=%s Geb=%s", fid, date, 
				CData(Gmin), CData(Gmax), CData(Qmin), CData(Qmax), CData(P), CData(Pb), CData(b), CData(Ge), CData(Geb) );
		#endif
			*/

			if (logging)
				Log(LOGINFO, "FLOW COMPUTATION %s %s = #%d %s [%s - %s] => %s [%s - %s]", fid, date, 
					i, CData(G), CData(minG), CData(maxG), CData(Q), CData(minQ), CData(maxQ) );

			// keep interpolated value within realistic terms
			if (Q<maxQ*1000)
				{
				line[1] = vars(CData(Q));
				line[0] += "C";
				date = line.join();
				}
			}
			else
			{
			if (logging)
				Log(LOGINFO, "INSUFFICIENT FLOW %s %s = GQ Size %d", fid, date, GQ.GetSize());
			}
	}


	CString FindDate(const char *date, double *vres = NULL)
	{
		Load();

		CString line;
		CCDATA.Lock();
		int f = data.Find(CString(date,10));
		if (f>=0)
			line = data[f];
		CCDATA.Unlock();
		if (vres)
			{
			if (GetVal(line)<0)
				return ""; // expired
			vara val = vars(line).split();
			for (int i=1; i<val.length()-1; ++i)
				vres[i-1] = CDATA::GetNum( val[i] );
			}
		return line;
	}


	~CSite(void)
	{
		Save();
	}
};



//===========================================================================
// USGS
//===========================================================================



typedef struct {
	double Q, D;
} tQD;

static int QDsort(const tQD *arg1, const tQD *arg2)
	{
		if (arg1->Q < arg2->Q)
			return -1;
		if (arg1->Q > arg2->Q)
			return 1;
		if (arg1->D < arg2->D)
			return -1;
		if (arg1->D > arg2->D)
			return 1;
		return 0;
	}



int USGS_getsites(CSymList &list, int today, double lat1, double lng1, double lat2, double lng2)
{
  double end = GetTime();
  double start = end - (today ? 1 : 5*365);

  CString select = "bBox="+CData(lng1)+","+CData(lat1)+","+CData(lng2)+","+CData(lat2);
  CString url = "https://waterservices.usgs.gov/nwis/"+vars(today ? "iv" : "dv")+"/?format=json&parameterCd=00060,00065&"+select+"&startDT="+CDate(start)+"&endDT="+CDate(end);
  DownloadFile f;
  if ( DownloadRetry(f,  url ) )
	 {
	 Log(LOGERR, "USGS_getsites (box): failed url %s", url);
	 return FALSE;
	 }

   vars data(f.memory);
   vara lines = data.split("\"sourceInfo\"");

	// find closest match
	//console.log("addval "+siteid+" "+date+" :"+data.substr(0,40));
	for (int l=1; l<lines.length(); ++l)
		 {
		 vars sid, sdesc, slat, slng;
		 GetSearchString(lines[l], "\"siteCode\"", sid, ":\"", "\"");
		 GetSearchString(lines[l], "\"siteName\"", sdesc, ":\"", "\"");
		 GetSearchString(lines[l], "\"latitude\"", slat, ":", ",");
		 GetSearchString(lines[l], "\"longitude\"", slng, ":", ",");
		 list.Add(CSym(USGS+vars(":")+sid, sdesc+","+slat+","+slng));
		 }
  return TRUE;
}

void SetEquivalence(CSite *site, const char *fid)
{
	 CSymList fsites;
	 CString file = filename( site->prv->name );
	 fsites.Load(file);
	 int f = fsites.Find(site->fid);
	 if (f>=0)
		{
		fsites[f].SetStr(SITE_EQUIV, fid);
		fsites.Save(file);
		}
	 site->sym->SetStr(SITE_EQUIV, fid);
}

BOOL USGS_getdates(CSite *site, double start, double end, BOOL getref = TRUE, double *today = NULL)
{
  vars fid = vars(site->Info(SITE_EQUIV));
  double lat = site->sym->GetNum(SITE_LAT);
  double lng = site->sym->GetNum(SITE_LNG);

  CString select;
  if (fid.IsEmpty())
	{
	double d= MIN100MDEG;
	select = "bBox="+CData(lng-d)+","+CData(lat-d)+","+CData(lng+d)+","+CData(lat+d);
	}
  else
	{
	fid = fid.split(";")[0];
	select = "sites=" + fid.split(":")[1];
	}

  CString url = "https://waterservices.usgs.gov/nwis/"+vars(today ? "iv" : "dv")+"/?format=json&parameterCd=00060&"+select+"&startDT="+CDate(start)+"&endDT="+CDate(end);
  DownloadFile f;
  if ( DownloadRetry(f,  url ) )
	 {
	 Log(LOGERR, "%s getusgssites: failed url %s", site->prv->name, url);
	 return FALSE;
	 }

   vars data(f.memory);
   vara lines = data.split("\"sourceInfo\"");

   CSym sym;
   int minl = -1;
   // check within 100m distance
   double mind = MIN100M;

	// find closest match
	//console.log("addval "+siteid+" "+date+" :"+data.substr(0,40));
	for (int l=1; l<lines.length(); ++l)
		 {
		 vars sid, sdesc, slat, slng;
		 GetSearchString(lines[l], "\"siteCode\"", sid, ":\"", "\"");
		 GetSearchString(lines[l], "\"siteName\"", sdesc, ":\"", "\"");
		 GetSearchString(lines[l], "\"latitude\"", slat, ":", ",");
		 GetSearchString(lines[l], "\"longitude\"", slng, ":", ",");

		 vars lfid = USGS+vars(":")+sid;
		 double llat = CDATA::GetNum(slat);
		 double llng = CDATA::GetNum(slng);
		 if (llat==InvalidNUM || llng==InvalidNUM)
			 continue;
		 double d = Distance(lat, lng, llat, llng);
		 if (lfid==fid || (fid.IsEmpty() && d<mind))
			{
			 mind=d, minl=l;
			 sym = CSym(lfid, sdesc+","+slat+","+slng);
			}
		 }

   if (minl<0)
	 return FALSE;

   if (logging) 
	   Log(LOGINFO, "GETUSGS%s %s = %s\n%s\n%s", (start==end || today ? "DATE" : "HISTORY" ), site->fid, sym.id, site->Info(), sym.Line());

#if 1
	// record equivalence for future use
   if (fid.IsEmpty())
	 SetEquivalence(site, sym.id);
#endif

   // add cfs values to data
   int cnt = 0;
   double mindate = 1e10, maxdate = 0;
   vara values = lines[minl].split("\"value\"");
   CArrayList<tQD> qdate;
   for (int l=values.length()-1; l>=1; --l)
	{
		vars sv, sdate;
		GetSearchString(values[l], ":", sv, "\"", "\"");
		GetSearchString(values[l], "\"dateTime\":", sdate, "\"", "\"");
		if (sv=="" || sdate=="")
			continue;
		
		double v = CDATA::GetNum(sv);
		double date = CDATA::GetDate(sdate.substr(0,10));
		if (v<0 || date<=0)
			continue;
		if (today)
			{
			// get only one value (latest)
			*today = v;
			return 1;
			}

		++cnt;
		site->SetDate(date, v, 4);

		// to sort later G=v Q=date
		tQD qd; 
		qd.Q = v;
		qd.D = date;
		qdate.AddTail(qd);

		if (date>maxdate)
			maxdate = date;
		if (date<mindate)
			mindate = date;
	}

   if (start!=end && getref)
   {
   //start = mindate;
   //end = maxdate;

   qdate.Sort(QDsort);

   // get at least 12 reference points (one per month)
   double step = (qdate.GetSize()-1)/12;
   for (double i =0; i<qdate.GetSize(); i+=step)
		{
		VRESNULL;
		tQD &qd = qdate[(int)(i+0.5)];
		vars vdate = CDate(qd.D);
		site->FindDate(vdate, vres);
		if (vres[1]>0)
			continue;

		site->prv->getdate(site, vdate, vres);	
		if (vres[1]<=0)
			continue;
		
		site->SetDate(vdate, vres);
		}
	}

	return cnt;
}

BOOL USGS_gethistory(CSite *site)
{
  double end = GetTime();
  double start = end-5*365; 
  return USGS_getdates(site, start, end);
}

BOOL USGS_getdate(CSite *site, vars &date, double *vres)
{

  BOOL today = NULL;
  double datev = CDATA::GetDate(date);
  double start = datev, end = datev;
#if 1
  // try usgs
  if (istoday(date))
	  {
	  //VRESNULL;
	  if (USGS_getdates(site, end-2, end, FALSE, vres)>0)
		return TRUE;
	  }
  else
	  {
	  if (USGS_getdates(site, start, end, FALSE, NULL)>0)
		{
		// save historical (but don't TODAY)
		site->FindDate(date, vres);
		//site->SetDate(datev, *vres, 4);
		return TRUE;
		}
	  }

  return FALSE;
#else
  // try usgs
  if (USGS_getdates(site, start, end, FALSE, NULL)<=0)
	return FALSE;
	  
  site->FindDate(date, vres);
  return TRUE;
#endif
}


int USGS_getsites(DownloadFile &f, CSymList &list, const char *params)
{
		  const char *name = USGS;
		  CString url = "https://waterservices.usgs.gov/nwis/site/?format=rdb&parameterCd=00060,00065";
		  url += params;

		  
		  if ( DownloadRetry(f,  url ) )
			 {
			 Log(LOGERR, "%s getusgssites: failed url %s", USGS, url);
			 return FALSE;
			 }
		
		  vara table(f.memory, "\n");

		  // skip header
		  for (int i=0; i<table.length() && table[i][0]=='#'; ++i);

		  for (i=i+2; i<table.length(); ++i)
			{
			vara line = table[i].split("\t");
			if (line.length()<5)
				{
				Log(LOGERR, "%s getusgssites: BAD LINE %s", name, table[i]);
				continue;
				}
			
			double lat = CDATA::GetNum(line[4]);
			double lng = CDATA::GetNum(line[5]);
			if (line[4].trim().indexOf(" ")>=0 || line[5].trim().indexOf(" ")>=0 )
				{
				Log(LOGERR, "%s Spaced lat/lng %s url: %s", name, table[i], url);
				}
			 if (lat==InvalidNUM || lng==InvalidNUM)
				{
				if (line[4]!="" || line[5]!="")
					Log(LOGERR, "%s Invalid lat/lng %s url: %s", name, table[i], url);
				continue;
				}
			 //if (lng>0) lng = -lng;
			 addsite(list, USGS+vars(":")+line[1].trim(), line[2].trim(), CData(lat), CData(lng), line[0].trim());
			}
	return TRUE;
}



int USGS_getsites(const char *name, CSymList &list, int active)
{
	DownloadFile f;
	for (int s=0; s<states.length(); ++s)
		{
		printf("%s State:%s (%d%%)\r", USGS, states[s], (s*100)/states.length());
		CString param = MkString("&stateCd=%s", states[s]);
		if (active) 
			param += "&siteStatus=active&hasDataTypeCd=iv";
		else
			param += MkString("&period=P%dD&outputDataTypeCd=dv", 50*365);
		USGS_getsites(f, list, param);
		}
	return TRUE;
}


BOOL CompareDescription(const char *desc1, const char *desc2)
{
	// match description
	if (strcmp(desc1,desc2)==0)
		return TRUE;

	vara riverp = vars(desc1).split(" ");
	vara descp = vars(desc2).split(" ");
	for (int p=0; p<riverp.length() && riverp[p].length()<=2; ++p);
	for (int q=0; q<descp.length() && descp[q].length()<=2; ++q);

	// match river name
	if (p<riverp.length() && q<descp.length() && riverp[p]==descp[q])
		return FALSE;

	// no match
	return -1;
}


struct {
	CString name;
	CString base;
	CSymList list;
} USGS_ref[4] = { 
	{ USGS , USGS },
	{ INVERSE(USGS), USGS},
	{ "NOOA", "NOOA" },
	{ "BRPNAUX", "" }
  };
int USGS_getlatlng(const char *id, const char *desc, double &lat, double &lng, int mincmp = 0)
{
	int num = sizeof(USGS_ref)/sizeof(USGS_ref[0]);

	// load if not loaded yet
	if (USGS_ref[0].list.GetSize()==0)
		{
		for (int n=0; n<num; ++n)
			{
			// load, simpify, sort 
			USGS_ref[n].list.Load(filename(USGS_ref[n].name));
			for (int i=USGS_ref[n].list.GetSize()-1; i>=0; --i)
				USGS_ref[n].list[i].SetStr(SITE_DESC, simplify(USGS_ref[n].list[i].GetStr(SITE_DESC)));
			USGS_ref[n].list.Sort();
			}
		}

	CString str = simplify(desc);
	for (int n=0; n<num; ++n)
		{
		int dmatch = -2;
		// try to find ID and partial match
		CSymList &list = USGS_ref[n].list;
		vars searchid = USGS_ref[n].base != "" ? USGS_ref[n].base+":"+id : id;
		int f = list.Find(searchid);
		if (f>=0)
			{
			dmatch = CompareDescription(str, list[f].GetStr(SITE_DESC));
			if (dmatch<0)
				f = -1;
			}

		// try to find desc match
		for (int i=0; f<0 && i<list.GetSize(); ++i)
			if (CompareDescription(str, list[i].GetStr(SITE_DESC))>0)
				f = i;

		if (f>=0)
			{
			Log(LOGINFO, "NEW DMATCH %d:\n%s,%s\n%s\n",dmatch, id,desc,list[f].Line());
			lat = list[f].GetNum(SITE_LAT);
			lng = list[f].GetNum(SITE_LNG);
			return TRUE;
			}
		}
	return FALSE;
}




//===========================================================================
// CDEC
//===========================================================================

// urlbox = 'loc_chk=on&lon1='+boxrect[0]+'&lon2='+boxrect[2]+'&lat1='+boxrect[1]+'&lat2='+boxrect[3];

// sensor=20 RIVER DISCHARGE sensor=1 RIVER STAGE sensor=25<option value=25>TEMPERATURE, WATER <option value=146>TEMPERATURE, WATER C
vara CDEC_modes = vars("E,H").split();
vara CDEC_sensors = vars("20,1,25,146").split();

int CDEC_getsites(const char *name, CSymList &sitelist, int active)
{
	int i, l;
	vara sensors = CDEC_sensors; 
	vara modes = CDEC_modes;
	vars url = "http://cdec.water.ca.gov/cgi-progs/staSearch?staid=&sensor_chk=on&sensor=%s&dur_chk=on&dur=%s"; 
	if (active) url += "&active_chk=on&active=Y";

	DownloadFile f;
	for (int s=0; s<CDEC_sensors.length(); ++s)
	 for (int m=0; m<modes.length(); ++m)
	  {
	  if ( DownloadRetry(f,  MkString(url, sensors[s], modes[m]) ) )
		 {
		 Log(LOGERR, "%s getsites: failed url %s", name, url);
		 return FALSE;
		 }
	  vars data(f.memory);

	  vara list = data.split("/cgi-progs/staMeta?station_id=");

	  for (i=1; i<list.length(); ++i)
			 {
			 vara line = list[i].split("<td");     
			 vars id = line[0].split("'")[0];
			 vars siteid = name;
			 siteid += ":";
			 siteid += id;
			   
			 for (l=1; l<=5; ++l) 
			  {
			  vars val;
			  GetSearchString(line[l], ">", val, NULL, "</td>");
			  line[l] = val;
			  }
			  
			 //var sitename = line[1]+", "+line[2]+", "+line[3];
			 if (line[4].Trim().GetAt(0)!='-') // fix when >0
				 line[4].Insert(0, '-');
			 addsite(sitelist, siteid, line[1], line[5], line[4], sensors[s]+"="+modes[m]);
			 }
	   }

	return TRUE;
}

//http://localhost/rwr?waterflow=&wfid=CDEC:HIB&wfdates=2014-10-28
//&stamp=1414566289509
void CDEC_getdate(CSite *site, vars &date, double *vres)
{
	   int today = istoday(date);
	   double start, end, datev;
	   datev = CDATA::GetDate(date);
	   start = datev-1;
	   end = datev+1;

	   vara mode = vars(site->Info(SITE_MODE)).split(";");

	   DownloadFile f;
	   for (int m=0; m<mode.length(); ++m)
	   {
	   vara mod = mode[m].split("=");

	   // get sensor data ONLY if not already available
	   int sensor = CDEC_sensors.indexOf(mod[0]);
	   if (sensor<0 || vres[sensor]>=0)
		  continue;

	   // add all modes even if only 1 is supported bc in the past more could have been
	   if (sensor<2) //main sensor
		for (int u=0;u<CDEC_modes.length(); ++u) 
			{
			vars mod = CDEC_sensors[sensor]+"="+CDEC_modes[u];
			if (mode.indexOf(mod)<0)
				mode.push(mod);
			}

		vars url = vars("http://cdec.water.ca.gov/cgi-progs/queryCSV?station_id=")+site->id+"&sensor_num="+mod[0]+"&dur_code="+mod[1]+"&start_date="+CDate(start)+"+08:00"+"&end_date="+CDate(end)+"+14:00"+"&data_wish=View+CSV+Data";
	   //Log(LOGINFO, "getdate M%s %s", mode[m], url);
		if (logging) Log(LOGINFO, "url: %s %s\n%s", site->fid, date, url);
	   if ( DownloadRetry(f, url) )
		   {
		   vres[sensor] = InvalidURL;
		   Log(LOGERR, "%s getdate %s: failed url: %s", site->fid, date, url);
		   continue;
		   }

	   vars data(f.memory);
	   if (data.indexOf("<body")>=0)
		   {
		   vres[sensor] = InvalidNUM;
		   //Log(LOGERR, "%s getdate %s: no html url: %s", site->fid, date, url);
		   continue;
		   }

	   // process csv
	   // instead of computing average, get level at 1pm
	   vara lines = data.split("\r").join("").split("\n");
		//console.log("addval "+siteid+" "+date+" :"+data.substr(0,40));
	   tavg avg;
	   for (int l=lines.length()-1; l>1; --l)
		 {
		 vara line = lines[l].split(",");
		 if (line.length()!=3)
			{
			Log(LOGERR, "%s getdate %s: no good line %d!=3 %s", site->fid, date, line.length(), lines[l]);
			continue;
			}
		 double nv = CDATA::GetNum(line[2]);
		 //console.log("addval "+siteid+" "+date+" val:"+nv);
		 if (nv==InvalidNUM)
			 continue;

		  // got value       	      
		  vars ldate = line[0];
		  double ldatev = CDATA::GetDate(ldate, "YYYYMMDD" ); //ldate.substr(0,4)+"-"+ldate.substr(4,2)+"-"+ldate.substr(6,2) );
		  if (ldatev==InvalidDATE)
				{
				Log(LOGERR, "%s getdate %s: no good date %s", site->fid, date, ldate);
				continue;
				}

		  if (!today)
			{
			// DAILY AVERAGE
			if (ldatev==datev)
				avg.add(nv);
			continue;
			}

		  if (datev-ldatev>MAXTODAY)
				{
				Log(LOGERR, "%s getdate %s: date too different %s", site->fid, date, ldate);
				continue;
				}
		  avg.set(nv);
		  break;
		  }

	   // got result!
	   avg.get(vres[sensor]);
	   }
}       	    


//===========================================================================
// CDWR
//===========================================================================

vara CDWR_sensors = vars("DISCHRG=0,DISCHRG1=0,DISCHRG2=0,DISCHRG3=0,DISCHRG4=0,DISCHARG=0,DISCHRGA=0,GAGE_HT=1,GAGE_HT1=1,GAGE_HT2=1,GAGE_HT3=1,GAGE_HT4=1,WATTEMP=3,WATTEMP2=3").split();
vara CDWR_nosensors = vars("PRECIP,AIRTEMP,STORAGE,ELEV,ELEV2,COND,HUMID,PH,WND_DIR,WND_SPD,Q1,D1,D2,SURF_AC,AIR_C,BAR_P,SOLAR,GH_ADVM,VEL_ADVM,Ha,Hb,EVAP,DO,TDS,TURBID,DO_SAT,TOTAL,WLEVEL,WLELVEL").split();

#if 1

void convertUTMToLatLong(const char *szone, const char *utmx, const char *utmy, double &latitude, double &longitude)
	{
	// Lat Lon to UTM variables	
	latitude = 0.0;
	  longitude = 0.0;
	  int hemisphere = CString(szone).Right(1)>='S';
	  double zone = CDATA::GetNum(szone);
	  double easting = CDATA::GetNum(utmx);
	  double northing = CDATA::GetNum(utmy);

	double MPI = M_PI;

	// equatorial radius
	double equatorialRadius = 6378137;

	// polar radius
	double polarRadius = 6356752.314;

	// flattening
	double flattening = 0.00335281066474748;// (equatorialRadius-polarRadius)/equatorialRadius;

	// inverse flattening 1/flattening
	double inverseFlattening = 298.257223563;// 1/flattening;

	// Mean radius
	double rm = pow(equatorialRadius * polarRadius, 1 / 2.0);

	// scale factor
	double k0 = 0.9996;
	
	double b = 6356752.314;

	double a = 6378137;

	double e = 0.081819191;

	double e1sq = 0.006739497;


	// eccentricity
	//double e = sqrt(1 - pow(polarRadius / equatorialRadius, 2));

	//double e1sq = e * e / (1 - e * e);

	double n = (equatorialRadius - polarRadius)
		/ (equatorialRadius + polarRadius);

	// r curv 1
	double rho = 6368573.744;

	// r curv 2
	double nu = 6389236.914;

	// Calculate Meridional Arc Length
	// Meridional Arc
	double S = 5103266.421;

	double A0 = 6367449.146;

	double B0 = 16038.42955;

	double C0 = 16.83261333;

	double D0 = 0.021984404;

	double E0 = 0.000312705;

	// Calculation Constants
	// Delta Long
	double p = -0.483084;

	double sin1 = 4.84814E-06;

	// Coefficients for UTM Coordinates
	double K1 = 5101225.115;

	double K2 = 3750.291596;

	double K3 = 1.397608151;

	double K4 = 214839.3105;

	double K5 = -2.995382942;

	double A6 = -1.00541E-07;


	if (hemisphere)
	  {
		northing = 10000000 - northing;
	  }
	  double arc = northing / k0;
	  double mu = arc
		  / (a * (1 - pow(e, 2) / 4.0 - 3 * pow(e, 4) / 64.0 - 5 * pow(e, 6) / 256.0));

	  double ei = (1 - pow((1 - e * e), (1 / 2.0)))
		  / (1 + pow((1 - e * e), (1 / 2.0)));

	  double ca = 3 * ei / 2 - 27 * pow(ei, 3) / 32.0;

	  double cb = 21 * pow(ei, 2) / 16 - 55 * pow(ei, 4) / 32;
	  double cc = 151 * pow(ei, 3) / 96;
	  double cd = 1097 * pow(ei, 4) / 512;
	  double phi1 = mu + ca * sin(2 * mu) + cb * sin(4 * mu) + cc * sin(6 * mu) + cd
		  * sin(8 * mu);

	  double n0 = a / pow((1 - pow((e * sin(phi1)), 2)), (1 / 2.0));

	  double r0 = a * (1 - e * e) / pow((1 - pow((e * sin(phi1)), 2)), (3 / 2.0));
	  double fact1 = n0 * tan(phi1) / r0;

	  double _a1 = 500000 - easting;
	  double dd0 = _a1 / (n0 * k0);
	  double fact2 = dd0 * dd0 / 2;

	  double t0 = pow(tan(phi1), 2);
	  double Q0 = e1sq * pow(cos(phi1), 2);
	  double fact3 = (5 + 3 * t0 + 10 * Q0 - 4 * Q0 * Q0 - 9 * e1sq) * pow(dd0, 4)
		  / 24;

	  double fact4 = (61 + 90 * t0 + 298 * Q0 + 45 * t0 * t0 - 252 * e1sq - 3 * Q0
		  * Q0)
		  * pow(dd0, 6) / 720;

	  //
	  double lof1 = _a1 / (n0 * k0);
	  double lof2 = (1 + 2 * t0 + Q0) * pow(dd0, 3) / 6.0;
	  double lof3 = (5 - 2 * Q0 + 28 * t0 - 3 * pow(Q0, 2) + 8 * e1sq + 24 * pow(t0, 2))
		  * pow(dd0, 5) / 120;
	  double _a2 = (lof1 - lof2 + lof3) / cos(phi1);
	  double _a3 = _a2 * 180 / MPI;

	  latitude = 180 * (phi1 - fact1 * (fact2 + fact3 + fact4)) / MPI;

	  double zoneCM = 0;
	  if (zone > 0)
	  {
		zoneCM = 6 * zone - 183.0;
	  }
	  else
	  {
		zoneCM = 3.0;

	  }

	  longitude = zoneCM - _a3;
	  if (hemisphere)
	  {
		latitude = -latitude;
	  }
	}

// LatLong-UTM.c++
// Conversions:  LatLong to UTM;  and UTM to LatLong;
// by Eugene Reimer, ereimer@shaw.ca, 2002-December;
// with LLtoUTM & UTMtoLL routines based on those by Chuck Gantz chuck.gantz@globalstar.com;
// with ellipsoid & datum constants from Peter H Dana website (http://www.colorado.edu/geography/gcraft/notes/datum/edlist.html);
//
// Usage:  see the Usage() routine below;
//
// Copyright © 1995,2002,2010 Eugene Reimer, Peter Dana, Chuck Gantz.  Released under the GPL;  see http://www.gnu.org/licenses/gpl.html
// (Peter Dana's non-commercial clause precludes using the LGPL)


#include <cmath>			//2010-08-11: was <math.h>	  
#include <cstdio>			//2010-08-11: was <stdio.h>	  
#include <cstdlib>			//2010-08-11: was <stdlib.h>  
#include <cstring>			//2010-08-11: was <string.h>  
#include <cctype>			//2010-08-11: was <ctype.h>  
#include <iostream>			//2010-08-11: was <iostream.h>
#include <iomanip>			//2010-08-11: was <iomanip.h> 
using namespace std;			//2010-08-11: added
const double PI       =	M_PI;	//Gantz used: PI=3.14159265;
const double deg2rad  =	PI/180;
const double rad2deg  =	180/PI;
const double k0       =	0.9996;

class Ellipsoid{
public:
	Ellipsoid(){};
	Ellipsoid(int id, char* name, double radius, double fr){
		Name=name;  EquatorialRadius=radius;  eccSquared=2/fr-1/(fr*fr);
	}
	char* Name;
	double EquatorialRadius; 
	double eccSquared;
};
static Ellipsoid ellip[] = {		//converted from PeterDana website, by Eugene Reimer 2002dec
//		 eId,  Name,		   EquatorialRadius,    1/flattening;
	Ellipsoid( 0, "Airy1830",		6377563.396,	299.3249646),
	Ellipsoid( 1, "AiryModified",		6377340.189,	299.3249646),
	Ellipsoid( 2, "AustralianNational",	6378160,	298.25),
	Ellipsoid( 3, "Bessel1841Namibia",	6377483.865,	299.1528128),
	Ellipsoid( 4, "Bessel1841",		6377397.155,	299.1528128),
	Ellipsoid( 5, "Clarke1866",		6378206.4,	294.9786982),
	Ellipsoid( 6, "Clarke1880",		6378249.145,	293.465),
	Ellipsoid( 7, "EverestIndia1830",	6377276.345,	300.8017),
	Ellipsoid( 8, "EverestSabahSarawak",	6377298.556,	300.8017),
	Ellipsoid( 9, "EverestIndia1956",	6377301.243,	300.8017),
	Ellipsoid(10, "EverestMalaysia1969",	6377295.664,	300.8017),	//Dana has no datum that uses this ellipsoid!
	Ellipsoid(11, "EverestMalay_Sing",	6377304.063,	300.8017),
	Ellipsoid(12, "EverestPakistan",	6377309.613,	300.8017),
	Ellipsoid(13, "Fischer1960Modified",	6378155,	298.3),
	Ellipsoid(14, "Helmert1906",		6378200,	298.3),
	Ellipsoid(15, "Hough1960",		6378270,	297),
	Ellipsoid(16, "Indonesian1974",		6378160,	298.247),
	Ellipsoid(17, "International1924",	6378388,	297),
	Ellipsoid(18, "Krassovsky1940",		6378245,	298.3),
	Ellipsoid(19, "GRS80",			6378137,	298.257222101),
	Ellipsoid(20, "SouthAmerican1969",	6378160,	298.25),
	Ellipsoid(21, "WGS72",			6378135,	298.26),
	Ellipsoid(22, "WGS84",			6378137,	298.257223563)
};
#define	eClarke1866	5		//names for ellipsoidId's
#define	eGRS80		19
#define	eWGS72		21
#define	eWGS84		22

class Datum{
public:
	Datum(){};
	Datum(int id, char* name, int eid, double dx, double dy, double dz){
		Name=name;  eId=eid;  dX=dx;  dY=dy;  dZ=dz;
	}
	char* Name;
	int   eId;
	double dX;
	double dY;
	double dZ;
};
static Datum datum[] = {		//converted from PeterDana website, by Eugene Reimer 2002dec
//	      Id,  Name,			eId,		dX,	dY,	dZ;	//when & where this datum is applicable
	Datum( 0, "NAD27_AK",			eClarke1866,	-5,	135,	172),	//NAD27 for Alaska Excluding Aleutians
	Datum( 1, "NAD27_AK_AleutiansE",	eClarke1866,	-2,	152,	149),	//NAD27 for Aleutians East of 180W
	Datum( 2, "NAD27_AK_AleutiansW",	eClarke1866,	2,	204,	105),	//NAD27 for Aleutians West of 180W
	Datum( 3, "NAD27_Bahamas",		eClarke1866,	-4,	154,	178),	//NAD27 for Bahamas Except SanSalvadorIsland
	Datum( 4, "NAD27_Bahamas_SanSalv",	eClarke1866,	1,	140,	165),	//NAD27 for Bahamas SanSalvadorIsland
	Datum( 5, "NAD27_AB_BC",		eClarke1866,	-7,	162,	188),	//NAD27 for Canada Alberta BritishColumbia
	Datum( 6, "NAD27_MB_ON",		eClarke1866,	-9,	157,	184),	//NAD27 for Canada Manitoba Ontario
	Datum( 7, "NAD27_NB_NL_NS_QC",		eClarke1866,	-22,	160,	190),	//NAD27 for Canada NewBrunswick Newfoundland NovaScotia Quebec
	Datum( 8, "NAD27_NT_SK",		eClarke1866,	4,	159,	188),	//NAD27 for Canada NorthwestTerritories Saskatchewan
	Datum( 9, "NAD27_YT",			eClarke1866,	-7,	139,	181),	//NAD27 for Canada Yukon
	Datum(10, "NAD27_CanalZone",		eClarke1866,	0,	125,	201),	//NAD27 for CanalZone (ER: is that Panama??)
	Datum(11, "NAD27_Cuba",			eClarke1866,	-9,	152,	178),	//NAD27 for Cuba
	Datum(12, "NAD27_Greenland",		eClarke1866,	11,	114,	195),	//NAD27 for Greenland (HayesPeninsula)
	Datum(13, "NAD27_Carribean",		eClarke1866,	-3,	142,	183),	//NAD27 for Antigua Barbados Barbuda Caicos Cuba DominicanRep GrandCayman Jamaica Turks
	Datum(14, "NAD27_CtrlAmerica",		eClarke1866,	0,	125,	194),	//NAD27 for Belize CostaRica ElSalvador Guatemala Honduras Nicaragua
	Datum(15, "NAD27_Canada",		eClarke1866,	-10,	158,	187),	//NAD27 for Canada
	Datum(16, "NAD27_ConUS",		eClarke1866,	-8,	160,	176),	//NAD27 for CONUS
	Datum(17, "NAD27_ConUS_East",		eClarke1866,	-9,	161,	179),	//NAD27 for CONUS East of Mississippi Including Louisiana Missouri Minnesota
	Datum(18, "NAD27_ConUS_West",		eClarke1866,	-8,	159,	175),	//NAD27 for CONUS West of Mississippi Excluding Louisiana Missouri Minnesota
	Datum(19, "NAD27_Mexico",		eClarke1866,	-12,	130,	190),	//NAD27 for Mexico
	Datum(20, "NAD83_AK",			eGRS80,		0,	0,	0),	//NAD83 for Alaska Excluding Aleutians
	Datum(21, "NAD83_AK_Aleutians",		eGRS80,		-2,	0,	4),	//NAD83 for Aleutians
	Datum(22, "NAD83_Canada",		eGRS80,		0,	0,	0),	//NAD83 for Canada
	Datum(23, "NAD83_ConUS",		eGRS80,		0,	0,	0),	//NAD83 for CONUS
	Datum(24, "NAD83_Hawaii",		eGRS80,		1,	1,	-1),	//NAD83 for Hawaii
	Datum(25, "NAD83_Mexico_CtrlAmerica",	eGRS80,		0,	0,	0),	//NAD83 for Mexico CentralAmerica
	Datum(26, "WGS72",			eWGS72,		0,	0,	0),	//WGS72 for world
	Datum(27, "WGS84",			eWGS84,		0,	0,	0)	//WGS84 for world
};
#define	dNAD27_MB_ON	6		//names for datumId's
#define	dNAD27_Canada	15
#define	dNAD83_Canada	22
#define	dNAD83_ConUS	23
#define	dWGS84		27


void DatumConvert(int dIn, double LatIn, double LongIn, double HtIn, int dTo,  double& LatTo, double& LongTo, double& HtTo){
   // converts LatLongHt in datum dIn, to LatLongHt in datum dTo;  2002dec: by Eugene Reimer, from PeterDana equations.
   // Lat and Long params are in degrees;  North latitudes and East longitudes are positive;  Height is in meters;
   // ==This approach to Datum-conversion is a waste of time;  to get acceptable accuracy a large table is needed -- see NADCON, NTv2...
   double a,ee,N,X,Y,Z,EE,p,b,t;
   
   //--transform to XYZ, using the "In" ellipsoid
   //LongIn += 180;
   a = ellip[datum[dIn].eId].EquatorialRadius;
   ee= ellip[datum[dIn].eId].eccSquared;
   N = a / sqrt(1 - ee*sin(LatIn*deg2rad)*sin(LatIn*deg2rad));
   X = (N + HtIn) * cos(LatIn*deg2rad) * cos(LongIn*deg2rad);
   Y = (N + HtIn) * cos(LatIn*deg2rad) * sin(LongIn*deg2rad);
   Z = (N*(1-ee) + HtIn) * sin(LatIn*deg2rad);

   //--apply delta-terms dX dY dZ
   //cout<<"\tX:" <<X <<" Y:" <<Y <<" Z:" <<Z;		//==DEBUG
   X+= datum[dIn].dX - datum[dTo].dX;
   Y+= datum[dIn].dY - datum[dTo].dY;
   Z+= datum[dIn].dZ - datum[dTo].dZ;
   //cout<<"\tX:" <<X <<" Y:" <<Y <<" Z:" <<Z;		//==DEBUG
   
   //--transform back to LatLongHeight, using the "To" ellipsoid
   a = ellip[datum[dTo].eId].EquatorialRadius;
   ee= ellip[datum[dTo].eId].eccSquared;
   EE= ee/(1-ee);
   p = sqrt(X*X + Y*Y);
   b = a*sqrt(1-ee);
   t = atan(Z*a/(p*b));
   LatTo = atan((Z+EE*b*sin(t)*sin(t)*sin(t)) / (p-ee*a*cos(t)*cos(t)*cos(t)));
   LongTo= atan(Y/X);
   HtTo  = p/cos(LatTo) - a/sqrt(1-ee*sin(LatTo)*sin(LatTo));
   LatTo  *= rad2deg;
   LongTo *= rad2deg;  LongTo -= 180;
}


void LLtoUTM(int eId, double Lat, double Long,  double& Northing, double& Easting, int& Zone){
   // converts LatLong to UTM coords;  3/22/95: by ChuckGantz chuck.gantz@globalstar.com, from USGS Bulletin 1532.
   // Lat and Long are in degrees;  North latitudes and East Longitudes are positive.
   double a = ellip[eId].EquatorialRadius;
   double ee= ellip[eId].eccSquared;
   Long -= int((Long+180)/360)*360;			//ensure longitude within -180.00..179.9
   double N, T, C, A, M;
   double LatRad = Lat*deg2rad;
   double LongRad = Long*deg2rad;

   Zone = int((Long + 186)/6);
   if( Lat >= 56.0 && Lat < 64.0 && Long >= 3.0 && Long < 12.0 )  Zone = 32;
   if( Lat >= 72.0 && Lat < 84.0 ){			//Special zones for Svalbard
	  if(      Long >= 0.0  && Long <  9.0 )  Zone = 31;
	  else if( Long >= 9.0  && Long < 21.0 )  Zone = 33;
	  else if( Long >= 21.0 && Long < 33.0 )  Zone = 35;
	  else if( Long >= 33.0 && Long < 42.0 )  Zone = 37;
   }
   double LongOrigin = Zone*6 - 183;			//origin in middle of zone
   double LongOriginRad = LongOrigin * deg2rad;

   double EE = ee/(1-ee);

   N = a/sqrt(1-ee*sin(LatRad)*sin(LatRad));
   T = tan(LatRad)*tan(LatRad);
   C = EE*cos(LatRad)*cos(LatRad);
   A = cos(LatRad)*(LongRad-LongOriginRad);

   M= a*((1 - ee/4    - 3*ee*ee/64 - 5*ee*ee*ee/256  ) *LatRad 
		- (3*ee/8 + 3*ee*ee/32 + 45*ee*ee*ee/1024) *sin(2*LatRad)
		+ (15*ee*ee/256 + 45*ee*ee*ee/1024	  ) *sin(4*LatRad)
		- (35*ee*ee*ee/3072			  ) *sin(6*LatRad));
   
   Easting = k0*N*(A+(1-T+C)*A*A*A/6+(5-18*T+T*T+72*C-58*EE)*A*A*A*A*A/120) + 500000.0;

   Northing = k0*(M+N*tan(LatRad)*(A*A/2+(5-T+9*C+4*C*C)*A*A*A*A/24
				+ (61-58*T+T*T+600*C-330*EE)*A*A*A*A*A*A/720));
}


void UTMtoLL(int eId, double Northing, double Easting, int Zone,  double& Lat, double& Long){
   // converts UTM coords to LatLong;  3/22/95: by ChuckGantz chuck.gantz@globalstar.com, from USGS Bulletin 1532.
   // Lat and Long are in degrees;  North latitudes and East Longitudes are positive.
   double a = ellip[eId].EquatorialRadius;
   double ee = ellip[eId].eccSquared;
   double EE = ee/(1-ee);
   double e1 = (1-sqrt(1-ee))/(1+sqrt(1-ee));
   double N1, T1, C1, R1, D, M, mu, phi1Rad;
   double x = Easting - 500000.0;			//remove 500,000 meter offset for longitude
   double y = Northing;
   double LongOrigin = Zone*6 - 183;			//origin in middle of zone

   M = y / k0;
   mu = M/(a*(1-ee/4-3*ee*ee/64-5*ee*ee*ee/256));

   phi1Rad = mu + (3*e1/2-27*e1*e1*e1/32) *sin(2*mu) 
		+ (21*e1*e1/16-55*e1*e1*e1*e1/32) *sin(4*mu)
		+ (151*e1*e1*e1/96) *sin(6*mu);
   N1 = a/sqrt(1-ee*sin(phi1Rad)*sin(phi1Rad));
   T1 = tan(phi1Rad)*tan(phi1Rad);
   C1 = EE*cos(phi1Rad)*cos(phi1Rad);
   R1 = a*(1-ee)/pow(1-ee*sin(phi1Rad)*sin(phi1Rad), 1.5);
   D = x/(N1*k0);

   Lat = phi1Rad - (N1*tan(phi1Rad)/R1)*(D*D/2-(5+3*T1+10*C1-4*C1*C1-9*EE)*D*D*D*D/24
		   +(61+90*T1+298*C1+45*T1*T1-252*EE-3*C1*C1)*D*D*D*D*D*D/720);
   Lat *= rad2deg;
   Long = (D-(1+2*T1+C1)*D*D*D/6+(5-2*C1+28*T1-3*C1*C1+8*EE+24*T1*T1)*D*D*D*D*D/120) / cos(phi1Rad);
   Long = LongOrigin + Long*rad2deg;
}
#endif

/*
//==procedures to use and test the LatLong-to-UTM and UTM-to-LatLong conversion routines

int Z,Fmt,K; double E,N,Lat,Long;		//global vars for Prt routine
#define fUT4	1				//for Fmt: UTM-4wd-style
#define fLLD	2				//for Fmt: LL-degrees-N|S-E|W
#define fLLDM	4				//for Fmt: LL-degrees:minutes-N|S-E|W
#define fLLDMS	8				//for Fmt: LL-degrees:minutes:seconds-N|S-E|W
#define fUTs	16				//for Fmt: UTM-signed
#define fLLs	32				//for Fmt: LL-degrees-signed
#define fUT3	64				//for Fmt: UTM-3wd-style
#define nbr	atof														//atof: ascii-to-number
#define Nbr	strtod
double  nbN(char*a){return strchr("Ss",*a) ?atof(a+1)-10000000 :strchr("Nn",*a) ?atof(a+1) :atof(a);}				//Northing
double  nbL(char*a){														//lat|long: [N|E|S|W]d[:m[:s]][N|E|S|W]
   char*p; double R=Nbr(a+!!isalpha(*a),&p); if(*p==':')R+=Nbr(p+1,&p)/60; if(*p==':')R+=Nbr(p+1,&p)/3600;
   return strchr("SsWw",*a)||(*p&&strchr("SsWw",*p)) ?-R :R;
}
void prtDM (double D){double M=(D-int(D))*60; cout<<setprecision(3)<<int(D)<<":"<<M;}
void prtDMS(double D){double M=(D-int(D))*60; cout<<setprecision(1)<<int(D)<<":"<<int(M)<<":"<<(M-int(M))*60;}
void ps() {if(K++)cout<<" == ";}
void Prt(int Fmt){														//Prt: print according to Fmt bits
   K=0;
   if(Fmt&fLLs)  {ps(); cout<<setprecision(9) <<Lat <<" " <<Long;								}	//LL degrees signed  
   if(Fmt&fLLD)  {ps(); char NS=(Lat<0?'S':'N'), EW=(Long<0?'W':'E'); cout<<setprecision(5)<<abs(Lat)<<NS<<" "<<abs(Long)<<EW;	}	//LL degrees N|S E|W 
   if(Fmt&fLLDM) {ps(); char NS=(Lat<0?'S':'N'), EW=(Long<0?'W':'E'); prtDM (abs(Lat));cout<<NS<<" ";prtDM (abs(Long));cout<<EW;}	//LL  d:m    N|S E|W 
   if(Fmt&fLLDMS){ps(); char NS=(Lat<0?'S':'N'), EW=(Long<0?'W':'E'); prtDMS(abs(Lat));cout<<NS<<" ";prtDMS(abs(Long));cout<<EW;}	//LL  d:m:s  N|S E|W 
   if(Fmt&fUTs)  {ps(); cout<<setprecision(3)<<Z<<" "<<E<<" "<<N;								}	//UTM signed          
   if(Fmt&fUT3)  {ps(); cout<<setprecision(0)<<Z<<" "<<E<<" ";                  if(N<0)cout<<'S'<<N+10000000;else cout<<N;	}	//UTM std 3-word S-style
   if(Fmt&fUT4)  {ps(); cout<<setprecision(0)<<Z<<" "<<(N<0?"S ":"N ")<<E<<" "; if(N<0)cout     <<N+10000000;else cout<<N;	}	//UTM std 4-word N|S-style
}
void CvtLine(int C, char**V, char*p){
   if     (C==2){Lat=nbL(V[0]); Long=nbL(V[1]);                                               LLtoUTM(eWGS84,Lat,Long,N,E,Z);}	//2 words, convert from LatLong
   else if(C==3){Z=atoi(V[0]); E=nbr(V[1]); N=nbN(V[2]);                                      UTMtoLL(eWGS84,N,E,Z,Lat,Long);}	//3 words, convert from UTM
   else         {Z=atoi(V[0]); E=nbr(V[2]); N=nbN(V[3]); N=strchr("Ss",V[1][0])?N-10000000:N; UTMtoLL(eWGS84,N,E,Z,Lat,Long);}	//4 words, convert from UTM 2nd wd is N|S
   Prt(Fmt); if(*p)cout<<"\t"<<p; cout<<"\n";											//print as per Fmt, add comment if any
   //==Prt(Fmt) replaced by  Prt(fXXX);cout<<" == ";Prt(Fmt&~fXXX)  to show in approximately input-format first, then in the other formats?
}

void test(int eId, double pLat, double pLong){
   Lat=pLat; Long=pLong;
   cout<<"\nEllipsoid:" <<ellip[eId].Name;	cout<<"\nStarting Lat,Long position:              "; Prt(fLLs);
   LLtoUTM(eId, Lat, Long, N, E, Z);		cout<<"\nConverted to UTM-Zone,Easting,Northing:  "; Prt(fUTs);
   UTMtoLL(eId, N, E, Z, Lat, Long);		cout<<"\nConverted back to Lat,Long:              "; Prt(fLLs);
   cout<<"\n";
}
void testD(int dIn, double LatIn, double LongIn, double HtIn, int dTo){
   cout<<"\nDatum:" <<datum[dIn].Name <<" Ellipsoid:" <<ellip[datum[dIn].eId].Name  <<" To-Datum:" <<datum[dTo].Name <<" To-Ellipsoid:" <<ellip[datum[dTo].eId].Name;
   cout<<setprecision(9);							//note: Ht (in metres) needs 5-digits less precision than Lat,Long do
   double LatTo, LongTo, HtTo;					cout<<"\nIN: datum:" <<datum[dIn].Name <<"\tLat:" <<LatIn <<" Long:" <<LongIn <<" Ht:" <<HtIn;
   DatumConvert(dIn,LatIn,LongIn,HtIn, dTo,LatTo,LongTo,HtTo);	cout<<"\nTO: datum:" <<datum[dTo].Name <<"\tLat:" <<LatTo <<" Long:" <<LongTo <<" Ht:" <<HtTo;
   DatumConvert(dTo,LatTo,LongTo,HtTo, dIn,LatIn,LongIn,HtIn);	cout<<"\nTO: datum:" <<datum[dIn].Name <<"\tLat:" <<LatIn <<" Long:" <<LongIn <<" Ht:" <<HtIn;
   cout<<"\n";
}
void Testcases(){
   test(eWGS84,	  47+22.690/60, 8+13.950/60);					//WGS-84,     47:22.690N   8:13.950E    == ChuckGantz-eg  (Swiss Grid: 659879/247637)
   test(eWGS84,	  30+16./60+28.82/3600, -(97+44./60+25.19/3600));		//WGS-84,     30:16:28.82N 97:44:25.19W == PeterDana-eg1
   test(eClarke1866, 30+16./60+28.03/3600, -(97+44./60+24.09/3600));		//Clarke1866, 30:16:28.03N 97:44:24.09W == PeterDana-eg2;  recall NAD27 uses Clarke1866
   test(eClarke1866, 51.5, -101.5);						//Clarke1866, 51:30N 101:30W == SE-corner of topo-map:62N12
   testD(dWGS84, 30+16./60+28.82/3600, -(97+44./60+25.19/3600), 0, dWGS84);	//test DatumConvert on Dana-eg1 to itself (to see X,Y,Z)
   testD(dNAD27_MB_ON,  51.5, -101.5, 0, dNAD83_Canada);			//test DatumConvert on SE-corner of topo-map:62N12;  FROM: dNAD27_Canada | dNAD27_MB_ON
}
void Usage(){
   cout<<"Usage:";
   cout<<"\n	LatLong-UTM  [--Outputformat]  Latitude[N|S]  Longitude[E|W]";
   cout<<"\n	LatLong-UTM  [--Outputformat]  Zone  Easting  Northing";
   cout<<"\n	LatLong-UTM  [--Outputformat]  Zone  N|S  Easting  Northing";
   cout<<"\n	LatLong-UTM  [--Outputformat]";
   cout<<"\nreads from standard-input when no coordinates given on cmdline, where each line is one of:";
   cout<<"\n	Latitude[N|S]  Longitude[E|W]";
   cout<<"\n	Zone  Easting  Northing";
   cout<<"\n	Zone  N|S  Easting  Northing";
   cout<<"\nLatitude and Longitude are in degrees or degrees:minutes or degrees:minutes:seconds with north and east positive, S and W are alternate minus-signs;";
   cout<<"\nZone is an integer 1..60;  Easting in metres with origin for zone#N at 500,000m west of longitude N*6-183°;  northing is metres from equator with north positive;";
   cout<<"\nin the 3-word style, Northing is northing or the letter S followed by northing-plus-ten-million;";
   cout<<"\nin the 4-word style when 2nd word is S, Northing is northing-plus-ten-million;";
   cout<<"\nin the signed style, Northing is northing (and sanity prevails, unfortunately that style is not widely used);";
   cout<<"\nOutputformat is an integer, the sum of one or more of: 1 for UTM-4-word, 2 for LL-degrees, 4 for LL-degrees:minutes, 8 for LL-degrees:minutes:seconds,";
   cout<<"\n	16 for UTM-signed, 32 for LL-signed, 64 for UTM-3-word;  the default for Outputformat is 7 (which is UTM + LL-degrees + LL-degrees:minutes);";
   cout<<"\nan input-file can have comments, from '#' to end-of-line, and such comments as well as empty-lines will be echoed to the output;";
   cout<<"\nno datum conversion is provided, the WGS84 datum is used throughout (WGS84 is identical to GRS80 aka NAD83 to 8 significant digits);";
   cout<<"\nEXAMPLES:";
   cout<<"\n	LatLong-UTM  50.26     -96		-- convert Latitude:50.26 Longitude:-96 to UTM";
   cout<<"\n	LatLong-UTM  50:15.6   -96		-- same as preceding";
   cout<<"\n	LatLong-UTM  50:15:36  -96		-- same as preceding";
   cout<<"\n	LatLong-UTM  50:15:36N  96W		-- same as preceding";
   cout<<"\n	LatLong-UTM  N50:15:36  W96		-- same as preceding";
   cout<<"\n	LatLong-UTM  -25.0  18.13		-- convert Latitude:-25.0 Longitude:18.13 to UTM";
   cout<<"\n	LatLong-UTM  14 501000 5678901		-- convert Zone:14 Easting:501000 Northing:5678901 to LatLong";
   cout<<"\n	LatLong-UTM  14 501000 -5666777		-- convert Zone:14 Easting:501000 Northing:-5666777 to LatLong";
   cout<<"\n	LatLong-UTM  14 501000 S4333223		-- same as preceding;  Northing as S followed by Northing-plus-ten-million";
   cout<<"\n	LatLong-UTM  14 S 501000 4333223	-- same as preceding, in the 4-word style";
   cout<<"\n	LatLong-UTM  --48  <trailpoints-utm.txt	-- convert each line of trailpoints-utm.txt, with Outputformat: UTM-signed + LL-signed";
   cout<<"\n";
}
int main(int argc, char**argv){							//2010-08-11: was void main(...
   char buf[256]; char*p; int L,ac; char*av[129];				//vars for reading stdin
   cout<<setiosflags(ios::fixed);						//decided against including ios::showpoint
   Fmt=fUT4|fLLD|fLLDM;								//default for Fmt, if not specified by input
   --argc; ++argv;								//remove spurious first element of argv array
   while(argc && memcmp(argv[0],"--",2)==0){					//handle leading options: --Outputformat, --test, --help
	  if     (isdigit(argv[0][2]))		Fmt=atoi(argv[0]+2);		//for --<DIGIT>, parse Outputformat into Fmt
	  else if(strcmp(argv[0],"--test")==0)	{Testcases(); return 0;}	//for --test, run testcases & exit
	  else					{Usage(); return 0;}		//for --help, show Usage & exit
	  --argc; ++argv;
   }
   if(argc==0)  while(cin.getline(buf,256), cin.good()){			//0 args, read stdin converting each line
	  p=buf; ac=0;
	  while(1){ while(*p&&strchr(" \t",*p))++p; if(*p==0||*p=='#')break; av[ac++]=p; while(*p&&!strchr(" \t#",*p))++p;}	//break line into whitespace-separated words
	  if(ac>=2&&ac<=4) CvtLine(ac,av,p);					//line with 2|3|4 words, convert and print
	  else if(ac==0)   cout<<buf<<"\n";						//line with no words, echo the line (comments)
	  else             cout<<"==invalid number-of-words: "<<buf<<"\n";		//anything else is invalid, produce errmsg
   }
   else if(argc>=2&&argc<=4)	CvtLine(argc,argv,"");				//2|3|4 args, convert and print
   else				Usage();					//argc other than 0|2|3 is invalid, show Usage
   return 0;									//2010-08-11: added when void became illegal
}

// 2002-December:  by Eugene Reimer, ereimer@shaw.ca
// started with 2 routines (LLtoUTM, UTMtoLL) by Chuck Gantz chuck.gantz@globalstar.com;
//	Gantz cites his source as:  USGS Bulletin#1532;
// dropped the "LatZone" letters (aka Latitude-Bands);
// reworked handling "origin 10million" for northings in Southern-hemisphere -- his method needed "LatZone" letters which are even sillier than ten-million convention;
// better value for PI:
//	made 6mm difference in calculated UTM coords (in Gantz example);
// ellipsoid constants:  replaced Gantz's constants, which he said were from PeterDana website, with data from PeterDana website (to my surprise they differ):
//	http://www.colorado.edu/geography/gcraft/notes/datum/edlist.html
//	Dana cites his sources as:  NIMA-8350.2-1977july4  and  MADTRAN-1996october1
//	-- better Clarke1866 ellipsoid made 360mm difference in calculated UTM coords in MB;  NAD-27 example in Manitoba now within 1mm of NR-Canada online-converter!!
//	-- computed UTM-coords for AustinDomeStar differ from Dana's by roughly 100mm; but agree to the mm with NR-Canada online-converter (nrcan.gc.ca)!!
// datum constants & datum conversion routine:  using formulae from PeterDana website;  the dX,dY,dZ constants are for the conversion formulae;
//	have defined only those datums that apply to NorthAmerica (28 out of several hundred);
//	Dana cites:  Bowring,B. 1976. Transformations from spatial to geographical coordinates. Survey Review XXIII; pg323-327.
// added support for input from a file;  am tempted to scrap cmdline-args in which case the Usage-Examples would become:  echo "..." | LatLong-UTM
// considered scrapping Testcases routine and using a bash-script of testcases instead;  ==keeping it for non-WGS84 testcases since my UI has no datum-conversion...

// 2010-08-11:  in 2002 this compiled just fine,  but the current g++ insists main must return int;  and warns about deprecated header-files;
//	fixed by revising void main() --> int main();  adding return 0;  revising #include's as shown above eg: <math.h> --> <cmath>;  adding: using namespace std;

// 2010-08-12:  debugging DatumConvert:
//	my 1st testD eg, Dana-eg1, going TO-XYZ;
//		according to Dana it is:	X:-742507.1    Y:-5462738.5    Z:3196706.5		(Dana calls it ECEF XYZ)
//		I was getting:			X:-743130.2424 Y:-5467322.9359 Z:3199389.2544
//		I'm now getting (sin-sqred):	X:-742507.1145 Y:-5462738.4892 Z:3196706.5101		<==AGREES after sin->sin-sqred fix!!
//		GEOTRANS (to Geocentric) gets:	X:-742507      Y:-5462738      Z:3196707		(Geotrans calls it Geocentric)
//		ergo: GEOTRANS agrees with Dana, my code based on Dana-equations does too after fixing;
//	my 2nd testD eg, converting Lat:51.5 Long:101.5 from NAD27 to NAD83:
//		NTv2 (online):		Lat:51:29:59.96845  Long:101:30:1.65354W			<==using http://www.geod.nrcan.gc.ca/apps/ntv2/ntv2_geo_e.php
//					Lat:51.49999124     Long:101.5004593W				<--converted to degrees on my calculator (10-digit precision)
//		NTv2 (exe under Wine):  Lat:51.499991236    Long:101.500459317W				<==AUTHORITATIVE ANSWER
//		NADCON gets:		Lat:==NO LUCK, NORTH-AMERICA==USA==!!==				<==http://www.ngs.noaa.gov/cgi-bin/nadcon.prl
//		GEOTRANS gets:		Lat:51.50004        Long:-101.50059      Ht:-25			<--inaccurate;  its UI truncates Datum-name => unsure which??
//		I'm getting:		Lat:51.500036202    Long:-101.500594748  Ht:-24.919516822	<--From:NAD27_Canada;  same as GEOTRANS but not good enough
//		I'm getting:		Lat:51.500013926    Long:-101.500577765  Ht:-26.781441874    	<--From:NAD27_MB_ON;   slightly closer but still not good enough
//		Paper http://www.h-e.com/pdfs/de_sbe94.pdf:  shows MB has approx no change in Lat,  from -1.0 in E-MB to -1.6seconds in W-MB change in Long;
//			same info in degrees:  Longitude from -0.00027 in Eastern-MB to -0.00044degrees in Western-MB;  <--my eg in W-MB changes by -0.00046degrees;
//			Error on that map-corner-eg to 5-place-fractional-degrees:  Lat-error: 0.00001  Long-error: 00012;
//	==Conclusions after some reading:  (1) NADCON & NTv2(NRCAN) are the reasonably accurate methods;  (2) my (and GEOTRANS's) formula approach is a waste of time;
//	PROJ (http://trac.osgeo.org/proj/) -- it needs grid shift file, eg: NTV2_0.GSB from http://www.geod.nrcan.gc.ca/online_data_e.php
//	NTv2, DOS-program that comes with the "grid shift file" NTV2_0.GSB from NRCAN, runs under Wine;  http://www.geod.nrcan.gc.ca/online_data_e.php
//	note: NTv2 is Canadian;  Australia, NewZealand, Germany, Switzerland, Spain also use its file-format;  and unofficially GreatBritain, France, Portugal;
//	NADCON is yank;  they call it the NORTH-AMERICAN converter but it only supports locations within-USA:-)
//	TRANSDAT: Windows program installed via Wine, into c:\Program Files\transdat;  	needs MFC42.DLL, installed into /home/ereimer/.wine/drive_c/windows/system32
// NTv2:  have found OpenSource programs with NTv2 support;  also found freely downloadable NTv2 Data-files;  and the NTv2 Developer's Guide;
// Notes on NTv2 doc & tools are now in my Datum-Conversion rant:  http://ereimer.net/rants/datum-conversion.htm
//	[For me: copies of NTv2 doc & tools are in:  /pix/er-GIS-DatumConversion-NAD83-WGS84-NTv2-etc-data;  see 00-NOTES... in that dir]
// those tools offer UTM<->LatLong as well as Datum-shift conversions;  may abandon this program as hardly needed, except for more flexible outputformat==??==
// one thing that is needed:  a NAD27->NAD83 conversion-table (in NTv2 format) that handles all of North America==??==

// 2010-08-30:
// put up a web-interface:  http://ereimer.net/cgi-bin/coordinatecvt;
// nbL:  accept N|S E|W at either end of a  Lat/Long coordinate;  (letter-up-front style is not offered for output);
// CvtLine:  accept UTM-4-word input eg: 14 N 501000 5678901 (same as 14 501000 N5678901);  Prt: fUT4 is Fmt bit for UTM-4-word output, replaces fUT3 as on-by-default;
// Usage: added Examples and reworded Instructions for the 4-word UTM style now supported on input & output;
// Consider:
//	doing space->colon fixups to get Lat Long pair might be possible, since valid UTM-Easting is never less than 60...
//	adding some checking for better errmsgs;  eg: each "word" must start with digit|minus-sign|N|S|E|W|n|s|e|w;
*/

int isvariable(const char *variable)
{
	for (int s=0; s<CDWR_sensors.length(); ++s)
		if (variable==CDWR_sensors[s].split("=")[0])
			return atoi(CDWR_sensors[s].split("=")[1]);
	return -1;
}

int CDWR_getsites(const char *name, CSymList &sitelist, int active)
{
#if 1

	{
	vars url = "http://www.dwr.state.co.us/SMS_WebService/ColoradoWaterSMS.asmx?SOAP?"
	"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
	"<soap12:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap12=\"http://www.w3.org/2003/05/soap-envelope\">"
	"  <soap12:Body>"
	"    <GetSMSTransmittingStations xmlns=\"http://www.dwr.state.co.us/\">"
	"    </GetSMSTransmittingStations>"
	"  </soap12:Body>"
	"</soap12:Envelope>";

	DownloadFile f;
	if ( DownloadRetry(f, url) )
		 {
		 Log(LOGERR, "%s getsites: failed url %s", name, url);
		 return FALSE;
		 }

	   vars data(f.memory);
	   vara lines = data.split("<Station>");
		//console.log("addval "+siteid+" "+date+" :"+data.substr(0,40));
		for (int l=1; l<lines.length(); ++l)
		 {
		 vars id, desc, agency, utmx, utmy;
		 GetSearchString(lines[l], "<abbrev>", id, "", "</abbrev>");
		 GetSearchString(lines[l], "<stationName>", desc, "", "</stationName>");
		 GetSearchString(lines[l], "<DataProviderAbbrev>", agency, "", "</DataProviderAbbrev>");
		 GetSearchString(lines[l], "<UTM_x>", utmx, "", "</UTM_x>");
		 GetSearchString(lines[l], "<UTM_y>", utmy, "", "</UTM_y>");

		if (id=="")
			{
			 Log(LOGERR, "%s getsites: invalid id %s", name, lines[l]);
			 continue;
			}
		if (utmx=="" || utmy=="")
			{
			if (id.indexOf("TEST")<0)
				Log(LOGERR, "%s getsites: invalid position %s", name, lines[l]);
			 continue;
			}


		 float lat, lng;
		 UTM2LL("13N", utmx, utmy, lat, lng);
		 addsite(sitelist, name+vars(":")+id, desc, CData(lat), CData(lng), "", agency);
		 }
	}

	{
	vars url = "http://www.dwr.state.co.us/SMS_WebService/ColoradoWaterSMS.asmx?SOAP?"
	"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
	"<soap12:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap12=\"http://www.w3.org/2003/05/soap-envelope\">"
	"  <soap12:Body>"
	"    <GetSMSTransmittingStationVariables xmlns=\"http://www.dwr.state.co.us/\">"
	"    </GetSMSTransmittingStationVariables>"
	"  </soap12:Body>"
	"</soap12:Envelope>";

	DownloadFile f;
	if ( DownloadRetry(f, url) )
		 {
		 Log(LOGERR, "%s getsites: failed url %s", name, url);
		 return FALSE;
		 }
	vars data(f.memory);
	vara list = data.split("<StationVariables>");
	for (int i=1; i<list.length(); ++i)
		{
		vars id, variable;
		GetSearchString(list[i], "<abbrev>", id, "", "</abbrev>");
		GetSearchString(list[i], "<variable>", variable, "", "</variable>");
		int sensor = isvariable(variable);
		if (sensor<0) 
			{
			if (CDWR_nosensors.indexOf(variable)>=0)
				continue;
			Log(LOGERR, "%s getsites: unknown mode %s", name, variable);
			continue;
			}
		addsite(sitelist, name+vars(":")+id,"","","",MkString("%s=%d", variable, sensor), "", "", TRUE );
		}
	}

	 for (int l=sitelist.GetSize()-1; l>=0; --l)
		if (sitelist[l].GetStr(SITE_MODE)=="" || sitelist[l].GetStr(SITE_LAT)=="")
			sitelist.Delete(l);

#else
	vars url = "https://data.colorado.gov/api/views/ceb5-u3hr/rows.json?accessType=DOWNLOAD";
	DownloadFile f;
	if ( DownloadRetry(f,  url ) )
		 {
		 Log(LOGERR, "%s getsites: failed url %s", name, url);
		 return FALSE;
		 }
	  vars data(f.memory);
	  int ts = data.indexOf(" [ [");
	  if (ts<0)
		{
		Log(LOGERR, "%s getsites: failed url %s", name, url);
		return FALSE;
		}

	  data = data.substr(ts+3);
	  data.Replace("null", "\"null\"");

	  vara list = data.split("] ]");

	  for (int i=0; i<list.length()-1; ++i)
			 {
			 vara line = list[i].split("[");
			 if (line.length()<3)
				{
				Log(LOGERR, "%s getsites: bad line %s", name, line.join(";"));
				continue;
				}
			 vara info = line[1].split("\",");
			 if (info.length()<=16)
				{
				Log(LOGERR, "%s getsites: bad info %s", name, info.join(";"));
				continue;
				}

			 int d = 8;
			 vars desc = cleanup(info[d]);
			 vars id1 = cleanup(info[d+1]);
			 vars id2 = cleanup(info[d+2]);
			 vars agency = cleanup(info[d+3]);
			 vars status = cleanup(info[d+4]);
			 if (status=="Historic")
				 continue;

			 if (status!="Active")
				{
				Log(LOGERR, "%s getsites: bad status %s", name, status);
				continue;
				}
						 
			 vara loc = line[3].split("\",");
			 if (info.length()<=2)
				{
				Log(LOGERR, "%s getsites: bad coords %s", name, loc.join(";"));
				continue;
				}
			 vars lat = cleanup(loc[1]);
			 vars lng = cleanup(loc[2]);

			 vars id;
			 GetSearchString(line[2], "&sValue=", id, "", "\"");

			 if (id1=="")
				{
				Log(LOGERR, "%s getsites: bad id %s", name, line);
				continue;
				}
			  
			 //var sitename = line[1]+", "+line[2]+", "+line[3];
			 addsite(sitelist, name+vars(":")+id1, desc, lat, lng, "", agency);
			 }
#endif

	return TRUE;
}


//http://localhost/rwr?waterflow=&wfid=CDEC:HIB&wfdates=2014-10-28
//&stamp=1414566289509
void CDWR_getdate(CSite *site, vars &date, double *vres)
{
	int asensors[] = { TRUE, TRUE, FALSE, TRUE };

	DownloadFile f;

int today = istoday(date);
double start, end, datev;
start = end = datev = CDATA::GetDate(date);
start = start-1;
end = end + 1;

vara mode = vars(site->Info(SITE_MODE)).split(";");
for (int m=0; m<mode.length(); ++m)
{
	vara sensorp = mode[m].split("=");
	int sensor = atoi(sensorp[1]);
	if (vres[sensor]>=0)
		continue;

	vars url = "http://www.dwr.state.co.us/SMS_WebService/ColoradoWaterSMS.asmx?SOAP?"
	"<?xml version=\"1.0\" encoding=\"utf-8\"?>"
	"<soap12:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap12=\"http://www.w3.org/2003/05/soap-envelope\">"
	"<soap12:Body>"
	"<GetSMSProvisionalData xmlns=\"http://www.dwr.state.co.us/\">"
	"<Abbrev>"+site->id+"</Abbrev>"
	"<Variable>"+sensorp[0]+"</Variable>"
	"<StartDate>"+CDate(start)+"</StartDate>"
	"<EndDate>"+CDate(end)+"</EndDate>"
	//"      <Aggregation>string</Aggregation>"
	"</GetSMSProvisionalData>"
	"</soap12:Body>"
	"</soap12:Envelope>";

	   if (logging) Log(LOGINFO, "url: %s %s\n%s", site->fid, date, url);
	   if ( DownloadRetry(f, url /*, MkString("%s-%s-%s.xml",site->id,date,sensorp[1])*/ ) )
		   {
		   vres[sensor] = InvalidURL;
		   Log(LOGERR, "%s getdate %s: failed url: %s", site->fid, date, url);
		   continue;
		   }

	   vars data(f.memory);
	   if (data.indexOf("<StreamflowTransmission>")<0)
		   {
			   vres[sensor] = InvalidNUM;
		   //Log(LOGERR, "%s getdate %s: no html url: %s", site->fid, date, url);
		   continue;
		   }

	   // process csv
	   // instead of computing average, get level at 1pm
	   tavg avg;
	   vara lines = data.split("<StreamflowTransmission>");
		//console.log("addval "+siteid+" "+date+" :"+data.substr(0,40));
	   for (int l=lines.length()-1; l>=1; --l)
		 {
		 vars lval, ldate;
		 GetSearchString(lines[l], "<transDateTime>", ldate, "", "</transDateTime>");
		 GetSearchString(lines[l], "<amount>", lval, "", "</amount>");
		 if (ldate=="" || lval=="")
			{
			Log(LOGERR, "%s getdate %s: no good line %s", site->fid, date, lines[l]);
			continue;
			}
		 double nv = CDATA::GetNum(lval);
		 //console.log("addval "+siteid+" "+date+" val:"+nv);
		 if (nv==InvalidNUM)
			 continue;

		  // got value       	      
		  double ldatev = CDATA::GetDate( ldate.split(" ")[0], "M/D/Y"); //ldatep[2]+"-"+ldatep[0]+"-"+ldatep[1] );
		  if (ldatev==InvalidDATE)
				{
				Log(LOGERR, "%s getdate %s: no good date %s", site->fid, date, ldate);
				continue;
				}

		  if (!today)
			{
			// DAILY AVERAGE
			if (ldatev==datev)
				avg.add(nv);
			continue;
			}

		  if (datev-ldatev>MAXTODAY)
				{
				Log(LOGERR, "%s getdate %s: date too different %s", site->fid, date, ldate);
				continue;
				}
		  avg.set(nv);
		  break;
		  }

	   // got result!
	   avg.get(vres[sensor]);
}

}       	    


//===========================================================================
// Canada Wateroffice
//===========================================================================


int CNDWO_getsites(const char *name, CSymList &sitelist, int active)
{
	var i, l;
	vars url = "http://wateroffice.ec.gc.ca/station_metadata/stationResult_e.html?search_type=province&province=all&region=CAN&station_status=all";
		// old "http://wateroffice.ec.gc.ca/station_metadata/stationResult_e.html?type=province&selProvince=ALL&wscRegion=ALL";
	if (active) url += "&status=A";

	DownloadFile f;
	if ( DownloadRetry(f, url) )
		 {
		 Log(LOGERR, "%s getsites: failed url %s", name, url);
		 return FALSE;
		 }
	vars data(f.memory);

	vara list = data.split("\n");

	for (i=1; i<list.length(); ++i)
		 {
		 vara line = list[i].split();     
		 vars id = line[0];
		 vars siteid = name;
		 siteid += ":";
		 siteid += id;
		 
		 // join desc
		 l = 1;
		 vars desc = line[l++];
		 while (desc[0]=='\"' && desc[desc.length()-1]!='\"' && l<line.length())
		   desc += line[l++];	   
		 if (desc[0]=='\"')
			 desc = desc.substr(1);
		 if (desc[desc.length()-1]=='\"')
			 desc = desc.substr(0, desc.length()-1);

		 if (toupper(line[l+9].trim()[0])=='Y')
			addsite(sitelist, siteid, desc, line[l+2], line[l+3]);
		 }

	return TRUE;
}

	
//CNDWOFLOW "&prm1=46" 
//CNDWOSTAGE "&prm1=47"
#define CNDWOMODE "&prm1=46" 

//CLineList &yeardata, 
int CNDWO_gethistoricretry(CSite *site, vars &date, int sensor = 0)
{
	   DownloadFile f;
	   vars url = vars("http://wateroffice.ec.gc.ca/report/real_time_e.html?mode=Table&type=h2oArc&stn=")+site->id+"&dataType=Daily";
	   if (sensor)
		   url += "&parameterType=Level";
	   else
		   url += "&parameterType=Flow";

	  vars yearstr = date.substr(0,4);
	  if (yearstr!="")
		   url += "&year="+yearstr;

	   InternetSetCookie( url, "disclaimer", "agree");
	   //Log(LOGINFO, "getdate %s", url);
		if (logging) Log(LOGINFO, "url: %s %s\n%s", site->fid, date, url);
	   if ( DownloadRetry(f, url) )
		   {
		   Log(LOGERR, "%s gethistoric %s: failed url: %s", site->fid, date, url);
		   return -1;
		   }

	   vars data(f.memory);
	   
	   var ts= data.indexOf("<tbody");
	   var te= data.indexOf("</tbody");
	   vara table = data.substr(ts, te-ts).split("<tr>");
	   if (table.length()<2)
		   return -1;
	
	   // get current year
	   GetSearchString(data, "<input type=\"hidden\" name=\"year\" value=", yearstr, '"', '"');

	   var month = atoi(date.substr(5,2));
	   var day = atoi(date.substr(8,2));
	   var year = atoi(yearstr);
	   if (yearstr!="")
		if (data.indexOf(yearstr+" Daily ")<0)
		   return -1;

	   for (int s=0; s<2; ++s)
		for (int i=0; i<table.length(); ++i)
		{
		const char * pos = strstr(table[i], "<th>");
		if (!pos) 
			continue;
		for (pos+=4; isspace(*pos); ++pos);
		double d = CDATA::GetNum(pos);
		if (d!=InvalidNUM && (int)d-d==0 && d>=1 && d<=31)
			{
			vara line  = table[i].split("<td>");
			if (line.length()!=13)
				{
				Log(LOGERR, "%s gethistoric %s: no 12 months year %d (%d)", site->fid, date, year, line.length());
				continue;
				}			
			for (int m=1; m<=12; ++m)					
				{
				double v = CDATA::GetNum(line[m]);
				if (v!=InvalidNUM)
					{
					double newdate = Date((int)d,(int)m,(int)year);
					if (newdate==InvalidDATE)
						{
						Log(LOGERR, "%s gethistoric %s: no valid date %dD %dM %dY (%s)", site->fid, date, d, m, year, line[m]);
						continue;
						}
					site->SetDate(newdate, v, sensor);
					}
				}
			}
		}
	
	return 0;
}



void CNDWO_gethistoric2(CSite *site, vars &date, int sensor = 0)
{
	if (CNDWO_gethistoricretry(site, date, sensor)<0)
		{
		// retry after 5 sec
		Sleep(5000);
		CNDWO_gethistoricretry(site, date, sensor);
		}
}




void CNDWO_getrealtime(CSite *site, vars &date, double *vres)
{
	   double startv, endv;
	   int today = istoday(date);
	   startv = endv = CDATA::GetDate(date);
	   startv = startv-1;
	   if (today)
		  endv = endv + 1;

	   vars start = CDate(startv);
	   vars end = CDate(endv);

	   DownloadFile f;
		
	   for (int s=0; s<2 && vres[0]<0; ++s)
	   {
	   // if flow invalid but level is valid, interpolate
	   CString mode = s==0 ? "47" : "46";
	   vars url = vars("http://wateroffice.ec.gc.ca/report/real_time_e.html?mode=Table&type=realTime&stn=")+site->id+"&dataType=Real-Time&startDate="+start+"&endDate="+end+"&prm1="+mode;
	   InternetSetCookie( url, "disclaimer", "agree");
	   //Log(LOGINFO, "getdate %s", url);
		if (logging) Log(LOGINFO, "url: %s %s\n%s", site->fid, date, url);
	   if ( DownloadRetry(f, url) )
		   {
		   Log(LOGERR, "%s getrealtime %s: failed url: %s", site->fid, date, url);
		   vres[s] = InvalidURL;
		   continue;
		   }

	   vars data(f.memory);
	   var ts= data.indexOf("<tbody");
	   var te= data.indexOf("</tbody");
	   vara table = data.substr(ts, te-ts).split("<tr>");
	   if (table.length()<2)
		   {
		   //Log(LOGERR, "%s getrealtime %s: failed table from url: %s", site->fid, date, url);
		   vres[s] = InvalidNUM;
		   continue;
		   }

		 tavg avg;
		 for (var i=table.length()-1; i>=1; --i)
			{
			vara line = table[i].split("<td>");
			CString sline = line.join(",");
			vars tdate = line[1].trim().substr(0, 10);
			double val = CDATA::GetNum(line[2].trim());
			if (val==InvalidNUM)
				continue;
			if (today)
				{
				// don't accept if too different dates
				if (CDATA::GetDate(date)-CDATA::GetDate(tdate)<=MAXTODAY)
					avg.set(val);
				break;
				}
			if (strcmp(tdate, end)==0)
				avg.add(val);
			}

		 avg.get(vres[s]);
	   }
}

int CNDWO_gethistory(CSite *site)
{
	vars latest;
	CNDWO_gethistoric2(site, latest, 0);
	CNDWO_gethistoric2(site, latest, 1);
	return TRUE;
}

void CNDWO_getdate(CSite *site, vars &date, double *vres)
{
	   // less than 180 days realtime
	   if (GetTime()-CDATA::GetDate(date)<18*365.0/12)
			{
			CNDWO_getrealtime( site, date, vres);
			}
		else
			{
			CNDWO_gethistoric2(site, date);
			site->FindDate(date, vres);
			}
}       	    


//===========================================================================
// USBR PN
//===========================================================================

vara BR_sensors = vars("Q=QD=0,QC=QJ=0,QH=QJ=0,GH=GJ=1,CH=CJ=1,WF=WF=2,WC=WC=3").split();

int BRsensor(const char *mode)
{
	vara list = vars(mode).split(";");
	for (int i=0; i<BR_sensors.length(); ++i)
		if (list.indexOf(BR_sensors[i].split("=")[0])>=0)
			return atoi(BR_sensors[i].split("=")[2]);
	return -1;
}

CString BRmode(const char *mode)
{
	// check if any supported mode is listed
	vara mlist;
	BOOL m = FALSE;
	vara list = vars(mode).split(";");
	for (int i=0; i<BR_sensors.length(); ++i)
		if (list.indexOf(BR_sensors[i].split("=")[0])>=0)
			{	
			mlist.push(BR_sensors[i]);
			// only for Q or G
			CString v = BR_sensors[i].split("=")[2];
			if (v=="0" || v=="1")
				m = TRUE;
			}
	if (!m) return "";
	return mlist.join(";");
}


CString BRmodes(CString prep = "")
{
		vara modes;
		for (int i=0; i<BR_sensors.length(); ++i)
			modes.push(prep+BR_sensors[i].split("=")[0]);
		return modes.join();
}


vars BRPN_geturl = "http://www.usbr.gov/pn-bin/webdaycsv.pl?parameter=";

vars getmode(DownloadFile &f, const char *id)
{

		CString url = BRPN_geturl+BRmodes(CString(id)+"%20")+"&format=2&back=24";
		if ( DownloadRetry(f, url) )
			return "";
		const char *str = strstr(f.memory, "12:00,");
		if (!str || *str==0) 
			return "";

		// get line for noon
		vara mode;
		vara tok = vars(str).split("\n")[0].split();
		for (int i=0; i<BR_sensors.length(); ++i)
			if (tok[i+1].trim()!="")
				mode.push(BR_sensors[i].split("=")[0]);
		return mode.join(";");
}



double getdeg(vars line, const char *label, const char *sep = "-")
{
	if (label)
		{
		vara a = line.split(label);
		if (a.length()<2)
			return InvalidNUM;
		line = a[1];
		}
	vara dms = line.split(sep);
	if (dms.length()<3)
		return InvalidNUM;
	
	double val[3];
	for (int i=0; i<3; ++i)
		{
		val[i] = CDATA::GetNum(dms[i]);
		if (val[i]==InvalidNUM)
			return InvalidNUM;
		}

	return dmstodeg(val[0], val[1], val[2]);
}

vara spacesplit(const char *line)
{
	const char *lines;
	vara res;
	while (*line!=0)
		{
		while (isspace(*line) && *line!=0)
			++line;
		lines = line;
		while (!isspace(*line) && *line!=0)
			++line;
		res.push(vars(lines, line-lines).trim());
		}
	return res;
}

vara sepsplit(vars &line, vara &sep)
{
	vara res;
	int ptr = 0, len = 0;
	for (int s=0; s<sep.length(); ++s)
		{
		res.push(line.substr(ptr, len = sep[s].length()).trim());
		ptr += len + 1;
		}	
	return res;
}

vars cleanhref(const char *str, const char *tag)
{
	vars res;
	int taglen = strlen(tag);
	while (*str!=0)
		{
		if ((str[0]=='<' && strnicmp(str+1, tag, taglen)==0) || (str[0]=='<' && str[1]=='/' && strnicmp(str+2, tag, taglen)==0))
			{
			while (*str!=0 && *str!='>')
				++str;
			if (*str!=0)
				++str;
			}
		else
			{
			res.AppendChar(*str++);
			}
		}
	res.Replace("&amp;", "&");
	res.Replace("&AMP;", "&");
	return res.trim();
}



int addauxsite(CSymList &auxlist, const char *_id, const char *_desc, const char *lat, const char *lng, const char *mode, const char *owner)
{
		vars id = cleanhref(_id, "A");
		vars desc = cleanhref(_desc, "A");

		id.Replace("*", "");
		id.MakeUpper();

		CSym sym(id);

		double latv = CDATA::GetNum(lat);
		double lngv = CDATA::GetNum(lng);
		if (latv!=InvalidNUM && latv<0)
			latv = -latv;
		if (lngv!=InvalidNUM && lngv>0)
			lngv = -lngv;

		vars extra(owner); extra.Replace(",", " ");
		return addsite(auxlist, id, desc, CData(latv), CData(lngv), mode, extra, "", TRUE);
}

void getnooakmlsites(CSymList &sites, CSymList *nooa = NULL);

void getauxsites(CSymList &auxlist)
{
	DownloadFile f;
	vars url = "http://www.nwd-wc.usace.army.mil/ftppub/cafe/station_info";
	if ( DownloadRetry(f, url) )
		{
		Log(LOGERR, "getauxsites: failed url %s", url);
		return;
		}

	vara sep;
	vara list(f.memory, "\n");
	for (int i=0; i<list.length(); ++i)
		{		
		if (strncmp(list[i],"---",3)==0)
			{
			sep = list[i].split(" ");
			continue;
			}
		if (sep.length()==0)
			continue;

		// process lines
		vara linep = sepsplit(list[i], sep);
		if (linep.length()<3 || linep[0]=="")
			continue;

		vara owner;
		if (linep[4]!="")
			owner.push("NOOA:"+linep[4]);
		if (linep[5]!="")
			owner.push("USGS:"+linep[5]);
		if (linep[6]!="")
			owner.push("SNOTEL:"+linep[6]);

		addauxsite(auxlist, linep[0], linep[7], linep[1], linep[2], "", owner.join(";"));
		}

	CSymList nooalist;
	getnooakmlsites(nooalist);
	for (int i=0; i<nooalist.GetSize(); ++i)
		{
		vara linep = vars(nooalist[i].data).split();
		addauxsite(auxlist, nooalist[i].id, linep[SITE_DESC], linep[SITE_LAT], linep[SITE_LNG], "", linep[SITE_MODE]);
		}
			

	url = "http://apps.wrd.state.or.us/apps/sw/hydro_near_real_time/near_real_time_gage_station_kml.aspx";
	if ( DownloadRetry(f, url) )
		{
		Log(LOGERR, "getauxsites: failed url %s", url);
		return;
		}

	list = vars(f.memory).split("<Placemark");
	for (int i=1; i<list.length(); ++i)
		{
			CString id, desc, lat, lng;
			GetSearchString(list[i], "Station Number:", id, " ", "&");
			GetSearchString(list[i], "<name", desc, ">", "<");
			GetSearchString(list[i], "\"station_longname\"", desc, "\"", "\"");
			GetSearchString(list[i], "<coordinates", lat, ",", "<");
			GetSearchString(list[i], "<coordinates", lng,  ">", ",");
			addauxsite(auxlist, id, desc, lat, lng, "", "OWDR");
		}
}


vars getquoted(vars &str, const char *quote1=NULL, const char *quote2=NULL)
{
	vars res;
	if (!quote1)
		quote1 = "\"";
	if (!quote2)
		quote2 = quote1;

	vara tmp = str.split(quote1);
	if (tmp.length()<2)
		return "";
	return tmp[1].split(quote2)[0];
	
}




int BRPN_getsites(const char *name, CSymList &sitelist, int active)
{
	DownloadFile f;
	CString auxfile = filename("BRPNAUX");
	CString manfile = filename("BRPNMAN");

	CSymList qlist, auxlist, manlist;
	auxlist.Load(auxfile);
	/* defunct getauxsites(auxlist); */
	
	manlist.Load(manfile);
	for (int i=0; i<manlist.GetSize(); ++i)
		{
		CSym s = manlist[i];
		CString e = s.GetStr(SITE_EXTRA);
		if (e!="")
			{
			int n = auxlist.Find(e);
			if (n>=0) 
				{
				s.SetStr(SITE_LAT, auxlist[n].GetStr(SITE_LAT));
				s.SetStr(SITE_LNG, auxlist[n].GetStr(SITE_LNG));
				}
			else
				Log(LOGERR, "%s getsites : failed man map %s -> %s", name, s.id, e);
			}
		addauxsite(auxlist, s.id, s.GetStr(SITE_DESC), s.GetStr(SITE_LAT), s.GetStr(SITE_LNG), "", "MAN");
		}

	const char **urls;

	// dialog lists
	const char *url_dlg[] ={
		"http://www.usbr.gov/pn/hydromet/station.js",
		"http://www.usbr.gov/pn/hydromet/yakima/yakwebdayread.html",
		"http://www.usbr.gov/pn/hydromet/yakima/yakwebarcread.html",
		"http://www.usbr.gov/pn/hydromet/umatilla/umawebhydreadarc.html",
		"http://www.usbr.gov/pn/hydromet/elwha/elwha_station.js",
		NULL
		};
	urls = url_dlg;
	for (int u=0; urls[u]!=NULL; ++u)
	{
		if ( DownloadRetry(f, urls[u]) )
			 {
			 Log(LOGERR, "%s getsites 2nd: failed url %s", name, urls[u]);
			 }
		else
			{
			vara list(f.memory, "<option value=");
			for (int i=1; i<list.length(); ++i)
				{
				int start = list[i].indexOf(">")+1;
				int end = list[i].indexOf("\\", start+1);
				int end2 = list[i].indexOf("<", start+1);
				if (end<0) end = end2;
				vars line = list[i].substr(start, end-start);
				int join = line.indexOf("-");
				vars id = line.substr(0, join).trim();
				vars desc = line.substr(join+1).trim();
				if (id=="" || desc=="")
					{
					Log(LOGERR, "%s invalid siteid 2nd %s", name, line);
					continue;
					}
				addauxsite(qlist, id, desc, "", "", "", ">DIALOG");
				}
			}
	}

	typedef struct { const char *sep, *end; } sepmode;
	// /graphrt.pl?emi_q
	// /rtgraph.pl?site=bjbo&amp;pcode=q
	// //graphwy.pl?kerm_q_daily" 
	sepmode mode[] = {
		{ "graphrt.pl?", "_" },
		{ "graphwy.pl?", "_" },
		{ "rtgraph.pl?site=", "pcode=" },
		{ "rtgraph.pl?sta=", "parm=" }
		};
	sepmode descmode[] = {
		{ "alt=\"", "\"" },
		{ "ALT=\"", "\"" },
		{ "==", "" },
		{ "window.status='", "'"}
		};
	// from http://www.usbr.gov/pn/hydromet/select.html
	const char *url_q[] ={
	"http://www.usbr.gov/pn/hydromet/rogue_stations.html",
	"http://www.usbr.gov/pn/hydromet/roguetea.html",
	"http://www.usbr.gov/pn/hydromet/tuatea.html",
	"http://www.usbr.gov/pn/hydromet/umatilla/umatea.html",
	"http://www.usbr.gov/pn/hydromet/destea.html",
	"http://www.usbr.gov/pn/hydromet/owytea.html",
	"http://www.usbr.gov/pn/hydromet/se_oregon_stations.html",
	"http://www.usbr.gov/pn/hydromet/boipaytea.html",
	"http://www.usbr.gov/pn/hydromet/boipaytea_stations.html",
	"http://www.usbr.gov/pn/hydromet/burtea.html",
	"http://www.usbr.gov/pn/hydromet/burtea_stations.html",
	"http://www.usbr.gov/pn/hydromet/loidtea.html",
	"http://www.usbr.gov/pn/hydromet/yakima/yaktea.html",
	"http://www.usbr.gov/pn/hydromet/yakima/yak_mcf_list.html",
	"http://www.usbr.gov/pn/hydromet/elwha/elwhatea.html",
	"http://www.usbr.gov/pn/hydromet/esatea.html",

	"http://www.usbr.gov/pn/hydromet/rtindex/boise.html",
	"http://www.usbr.gov/pn/hydromet/rtindex/crab.html",
	"http://www.usbr.gov/pn/hydromet/rtindex/crooked.html",
	"http://www.usbr.gov/pn/hydromet/rtindex/deschutes.html",
	"http://www.usbr.gov/pn/hydromet/rtindex/flathead.html",
	"http://www.usbr.gov/pn/hydromet/rtindex/loid.html",
	"http://www.usbr.gov/pn/hydromet/rtindex/malheur.html",
	"http://www.usbr.gov/pn/hydromet/rtindex/owyhee.html",
	"http://www.usbr.gov/pn/hydromet/rtindex/payette.html",
	"http://www.usbr.gov/pn/hydromet/rtindex/powder.html",
	"http://www.usbr.gov/pn/hydromet/rtindex/rogue.html",
	"http://www.usbr.gov/pn/hydromet/rtindex/abvidf.html",
	"http://www.usbr.gov/pn/hydromet/rtindex/abvmil.html",
	"http://www.usbr.gov/pn/hydromet/rtindex/tual.html",
	"http://www.usbr.gov/pn/hydromet/rtindex/umatilla.html",

	"http://www.usbr.gov/pn/hydromet/rtindex/flow.html",
 
	NULL};
	urls = url_q;
	for (int u=0; urls[u]!=NULL; ++u)
	{
	if ( DownloadRetry(f, urls[u]) )
		 {
		 Log(LOGERR, "%s getsites: failed url %s", name, urls[u]);
		 continue;
		 }
	vars data(f.memory);

	// modes
	for (int m=0; m<sizeof(mode)/sizeof(mode[0]); ++m)
		{
		vara list = data.split(mode[m].sep);
		for (int i=1; i<list.length(); ++i)
			 {
			 vara line = list[i].split(mode[m].end);
			 if (line.length()<2)
				 continue;
			 vars id = line[0].split("&")[0];
			 vars mode = line[1].split("\"")[0].split("'")[0];
			 vars desc;
			 vars descline = line[1].split("\n")[0];
			 for (int md=0; md<sizeof(mode)/sizeof(mode[0]); ++md)
				{
				int start = descline.indexOf(descmode[md].sep);
				int end = descline.indexOf(descmode[md].end);
				if (start>=0)
					{
					desc = descline.substr(start+strlen(descmode[md].sep), end-start);
					break;
					}
				}
			addauxsite(qlist, id, desc, "", "", mode, ">HTML");
			}
		}

	// <PRE> ID == Desc \n </PRE>
	vara prelist = data.split("<PRE>");
	if (prelist.length()<2)
		prelist = data.split("<pre>");
	if (prelist.length()>=2)
		{
		vara list = prelist[1].split("</pre>")[0].split("</PRE>")[0].split("\n");
		for (int i=0; i<list.length(); ++i)
			{
			vara line = list[i].split("==");
			if (line.length()<2)
				continue;
			vars id = line[0].trim();
			vars desc = line[1].trim();
			addauxsite(qlist, id, desc, "", "", "", ">PRE");
			}
		}

	{
		vara list = data.split("{ cbtt:");
		for (int i=1; i<list.length(); ++i)
				{
				vars id, desc, mode;
				id = getquoted(list[i]);
				vara line = list[i].split("description:");
				if (line.length()>1)
					desc = getquoted(line[1]);
				line = list[i].split("pcodes:");
				if (line.length()>1)
					{
					vara modes = getquoted(line[1], "[", "]").split();
					for (int i=0; i<modes.length(); ++i)
						modes[i] = getquoted(modes[i]);
					mode = modes.join(";");
					}
				if (id=="" || desc=="")
					continue;
				addauxsite(qlist, id, desc, "", "", mode, ">JSCRIPT");
				}
		}
	}


	// get lists with LAT= LONG=
	const char *url_ll[] = {
		"http://www.usbr.gov/pn/hydromet/decod_params.html",
		"http://www.usbr.gov/pn/hydromet/umatilla/umatsites.html",
		"http://www.usbr.gov/pn/hydromet/idwdsites.html",
		NULL };
	urls = url_ll;
	for (int u=0; urls[u]!=NULL; ++u)
	{
	if ( DownloadRetry(f, urls[u]) )
		 {
		 Log(LOGERR, "%s getsites: failed url %s", name, urls[u]);
		 continue;
		 }
	vars data(f.memory);
 
	vara list = data.split("<A NAME=\"");
	for (int i=1; i<list.length(); ++i)
		 {
		 vara line = list[i].split("\n");
		 vars descline = line[1].trim();
		 int sp = descline.indexOf(" ");
		 vars id = descline.substr(0, sp).trim();
		 vars desc = descline.substr(sp+1).trim();
		 if (id=="" || desc=="")
			{
			Log(LOGERR, "%s invalid siteid %s url: %s", name, descline, urls[u]);
			continue;
			}

		 double lat = getdeg(list[i], "LAT=");
		 double lng = getdeg(list[i], "LONG=");
		 if (lat==InvalidNUM || lng==InvalidNUM)
			{
			Log(LOGERR, "%s Invalid lat/lng %s url: %s", name, list[i], urls[u]);
			continue;
			}
		 if (lng>0) lng = -lng;

		 CString mode;
		 for (int l=4; l<line.length(); ++l)
			 mode = addmode(mode, line[l].trim().substr(0,2).trim().upper());
		 addauxsite(qlist, id, desc, CData(lat), CData(lng), mode, ">LATLONG");
		 }
	}
	

	// process qlist with auxlist
	for (int i=0; i<qlist.GetSize(); ++i)
		{
		CSym *aux = NULL;
		CString id = qlist[i].id;
		CString desc = qlist[i].GetStr(SITE_DESC);
		CString owner = qlist[i].GetStr(SITE_EXTRA);

		int af = auxlist.Find(id);
		if (af>=0) aux = &auxlist[af];

		// lat lng
		double lat = qlist[i].GetNum(SITE_LAT);
		double lng = qlist[i].GetNum(SITE_LNG);
		if (lat==InvalidNUM || lng==InvalidNUM)
			{
			if (aux!=NULL)
				{
				lat = aux->GetNum(SITE_LAT);
				lng = aux->GetNum(SITE_LNG);
				if (desc=="")
					desc = aux->GetStr(SITE_DESC);			
				}
			// if not aux found, find a similar
			vars cmp = simplify(qlist[i].GetStr(SITE_DESC));
			if (cmp!="")
			  for (int a=0; lat==InvalidNUM && lng==InvalidNUM && a<auxlist.GetSize(); ++a)
				{
				vars acmp = simplify(auxlist[a].GetStr(SITE_DESC));
				if (cmp==acmp)
					{
					lat = auxlist[a].GetNum(SITE_LAT);
					lng = auxlist[a].GetNum(SITE_LNG);
					if (aux!=NULL && lat!=InvalidNUM && lng!=InvalidNUM)
						{
						aux->SetNum(SITE_LAT, lat);
						aux->SetNum(SITE_LNG, lng);
						}
					}
				}
			}

		// mode
		CString mode = qlist[i].GetStr(SITE_MODE);
		if (aux!=NULL) // get mode from aux
			mode = addmode(mode, aux->GetStr(SITE_MODE));
		if (BRmode(mode)=="") // test mode if unknown
			{
#if 1		// explicitly test for modes
			mode = addmode(mode, getmode(f, id).upper());
#endif
			if (BRmode(mode)=="") // if no mode, ignore site
				{
				addauxsite(auxlist, id, desc, CData(lat), CData(lng), mode, owner);
				continue;
				}
			Log(LOGINFO, "%s Qmode forced for %s : %s", name, id, mode);
			}


		if (lat==InvalidNUM || lng==InvalidNUM) // if no lat/lng skip
			{
			Log(LOGERR, "%s known but invalid lat/lng %s,%s", name, id, desc);
			addauxsite(auxlist, id, desc, CData(lat), CData(lng), mode, owner);
			continue;
			}

		// add!
		addauxsite(auxlist, id, desc, CData(lat), CData(lng), mode, owner);
		addsite(sitelist, vars(name)+":"+id, desc, CData(lat), CData(lng), BRmode(mode));
		}
	
	auxlist.Sort();
	auxlist.Save(auxfile);
	return qlist.GetSize()>0;
}



void BRPN_getdate(CSite *site, vars &date, double *vres)
{
	   vara mode = vars(site->Info(SITE_MODE)).split("=");

	   int today = istoday(date);
	   double datev = CDATA::GetDate(date);


	   CString matchdate;
	   vars url = BRPN_geturl+BRmodes(CString(site->id)+"%20")+"&format=2";
	   //vars url = vars("http://www.usbr.gov/pn-bin/dfcgi.pl?sta=")+site->id+"&pcode="+mode[0]+"&back=24";
	   if (today)
		   {
		   url += "&back=24";
		   }
	   else
		   {
		   vara datep = date.split("-");
		   url += "&syer="+datep[0]+"&smnth="+datep[1]+"&sdy="+datep[2];
		   url += "&eyer="+datep[0]+"&emnth="+datep[1]+"&edy="+datep[2];
		   /*
		   vars end( CDate(datev+1) );
		   vara endp = end.split("-");
		   url += "&last="+endp[0]+monthname[atoi(endp[1])]+endp[2];
		   vara datep = date.split("-");
		   matchdate = datep[0]+monthname[atoi(datep[1])]+datep[2];
		   */
		   matchdate = datep[1]+"/"+datep[2]+"/"+datep[0];
		   }
	   //Log(LOGINFO, "getdate M%s %s", mode[m], url);
	   DownloadFile f;
		if (logging) Log(LOGINFO, "url: %s %s\n%s", site->fid, date, url);
	   if ( DownloadRetry(f, url) )
		   {
		   Log(LOGERR, "%s getdate %s: failed url: %s", site->fid, date, url);
		   vres[0] = InvalidURL;
		   return;
		   }

	   vars data(f.memory);
	   var ts= data.indexOf("BEGIN DATA");
	   var te= data.indexOf("END DATA");
	   vara table = data.substr(ts, te-ts).split("\n");
	   if (table.length()<2)
		   return;

	   #define MAXPARAM 20
	   tavg avg[MAXPARAM];

	   // process table
	   for (int t=2; t<table.length(); ++t)
		{
		vara line = table[t].split();
		if (line.length()<2)
			{
			Log(LOGERR, "%s getdate %s: no good line %d!=3 %s", site->fid, date, line.length(), table[t]);
			continue;
			}
		vars ldate = line[0].trim(); //substr(line[1].indexOf(">")+1);

		int maxs = min(-1, BR_sensors.length());
		for (int s=1; s<line.length(); ++s)
		 {
		 vars lval = line[s].trim(); //substr(line[2].indexOf(">")+1);
		 double val = CDATA::GetNum(lval);
		 if (val==998877 || val<0)
			val=InvalidNUM;
		 if (val==InvalidNUM)
			continue;

		// YYYYMMMDD
		double ldatev = CDATA::GetDate( ldate.split(" ")[0], "M/D/Y");// ldatep[2]+"-"+ldatep[0]+"-"+ldatep[1]);
		if (ldatev==InvalidDATE)
			{
			Log(LOGERR, "%s getdate %s: no good date %s", site->fid, date, ldate);
			continue;
			}

		if (today)
			{
			// TODAY
			if (datev-ldatev>MAXTODAY)
				continue; // too different dates

			avg[s].set(val);
			}
		 else
			{
			// DAILY VALUE
			if (ldatev==datev)
				avg[s].add(val);
			}
		 }
		}

	// return average
   vara header = table[1].split();
	for (int s=1; s<header.length(); ++s)
	  if (avg[s].cnt()>0)
		{
		vars hs = header[s].trim();
		int i=0; 
		while (i<hs.length() && !isspace(hs[i])) ++i;
		while (i<hs.length() && isspace(hs[i])) ++i;
		CString hdr = hs.substr(i);
		int r = BRsensor(hdr);
		if (r>=0 && vres[r]<0)
			avg[s].get(vres[r]);
		}
}

/*
double BRPN_getdate_historic(const char *id, vars &date, const char *mode)
{
	   double datev = CDATA::GetDate(date);

	   vars start = CDate(CDATA::GetDate(date)-1);
	   vara startp = start.split("-");
	   vara endp = date.split("-");
		   
	   double v = InvalidNUM;
	   DownloadFile f;
	   vars url = vars("http://www.usbr.gov/pn-bin/webarccsv.pl?station=")+id+"&format=3"+
		   "&year="+startp[0]+"&month="+startp[1]+"&day="+startp[2]+
		   "&year="+endp[0]+"&month="+endp[1]+"&day="+endp[2]+
		   "&pcode="+mode;
	   //Log(LOGINFO, "getdate M%s %s", mode[m], url);
	   if ( DownloadRetry(f, url) )
		   {
		   v = InvalidURL;
		   Log(LOGERR, "BRPN failed getdate %s %s url: %s", id, date, url);
		   return InvalidNUM;
		   }

	   vars data(f.memory);
	   //var ts= data.indexOf("<TABLE");
	   //var te= data.indexOf("</TABLE");
	   vara table = data.split("<TR>");
	   if (table.length()<2)
		   return InvalidNUM;

	   for (int t=table.length()-1; t>1; --t)
		{
		vara line = table[t].split("<TD");
		if (line.length()!=3)
			{
			Log(LOGERR, "BRPN no good %s %s url: %s", id, date, url);
			continue;
			}
		vars ldate = line[1].substr(line[1].indexOf(">")+1);
		vars lval = line[2].substr(line[2].indexOf(">")+1);
		double val = CDATA::GetNum(lval);
		if (val==InvalidNUM)
			continue;

		// valid data
		vara ldatep = ldate.substr(0,10).split("/");
		double ldatev = CDATA::GetDate(ldatep[2]+"-"+ldatep[0]+"-"+ldatep[1]);
		if (ldatev==InvalidDATE)
			{
			Log(LOGERR, "BRPN no good date %s %s url: %s [%s]", id, date, url, ldate);
			continue;
			}
		if (datev-ldatev<=1)
			return val;
		return InvalidNUM; // too different dates
		}


	   return InvalidNUM;
}

double BRPN_getdate(const char *id, vars &date)
{
	vara ids = vars(id).split(":");
	int s = findsite(id);
	vara mode = vars(GetTokenRest(sites[s].data, SITE_MODE)).split("=");

	if (istoday(date))
		return BRPN_getdate_realtime(ids[1], date, mode[0]);
	else
		return BRPN_getdate_historic(ids[1], date, mode[1]);
}
*/

//===========================================================================
// USBR GP
//===========================================================================

//http://www.usbr.gov/gp-bin/hydromet_dayfiles.pl?station=arkcanco&date=2008JUN10
//http://www.usbr.gov/gp-bin/hydromet_dayfiles.pl?station=alfvalmt&date=2008JUN10
//http://www.usbr.gov/gp-bin/hydromet_dayfiles.pl?station=arkgrnco&pcode=Q,GH&date=2008JUN10
vars BRGP_geturl = "http://www.usbr.gov/gp-bin/hydromet_dayfiles.pl?station=";

int BRGP_getsites(const char *name, CSymList &sitelist, int active)
{
	const char *urls[] = {
		"http://www.usbr.gov/gp/hydromet/sites_all.htm",
		"http://www.usbr.gov/gp/hydromet/sites_co.htm",
		"http://www.usbr.gov/gp/hydromet/sites_ks.htm",
		"http://www.usbr.gov/gp/hydromet/sites_mt.htm",
		"http://www.usbr.gov/gp/hydromet/sites_ne.htm",
		"http://www.usbr.gov/gp/hydromet/sites_nd.htm",
		"http://www.usbr.gov/gp/hydromet/sites_ok.htm",
		"http://www.usbr.gov/gp/hydromet/sites_sd.htm",
		"http://www.usbr.gov/gp/hydromet/sites_tx.htm",
		"http://www.usbr.gov/gp/hydromet/sites_wy.htm",
		NULL };

	DownloadFile f;
	vara newids;
	for (int u=0; urls[u]!=NULL; ++u)
	{
	if ( DownloadRetry(f, urls[u]) )
		 {
		 Log(LOGERR, "%s getsites: failed url %s", name, urls[u]);
		 return FALSE;
		 }
	vars data(f.memory);
 
	vara list = data.split("newLatLng = new google.maps.LatLng(");
	for (int i=1; i<list.length(); ++i)
		 {
		 vara line = list[i].split("title:");
		 if (line.length()!=2)
			{
			Log(LOGERR, "%s invalid site 'title:' %s", name, list[i]);
			continue;
			}
		 vara title = line[1].split("\"");
		 if (title.length()<3)
			{
			Log(LOGERR, "%s invalid site '\"title\"' %s", name, line[1]);
			continue;
			}
		 int split = title[1].indexOf("-");
		 vars id = title[1].substr(0,split).trim();
		 vars desc = title[1].substr(split+1).trim();
		 if (id.length()==0 || desc.length()==0)
			{
			Log(LOGERR, "%s site 'id - desc' %s", name, line[1]);
			continue;
			}

		 vara latlng = line[0].split();
		 double lat = CDATA::GetNum(latlng[0]);
		 double lng = CDATA::GetNum(latlng[1]);
		 if (lat==InvalidNUM || lng==InvalidNUM)
			{
			Log(LOGERR, "%s Invalid lat/lng %s", name, line[0]);
			continue;
			}

		 id.MakeUpper();
		 if (newids.indexOf(id)>=0)
			 continue;

		 newids.push(id);
		 vars siteid = vars(name)+":"+id;
		 if (sitelist.Find(siteid)>=0)
			 continue;

		 vars url = BRGP_geturl + id.MakeLower() + "&date=";
		 if ( DownloadRetry(f, url) )
			 {
			 Log(LOGERR, "%s getsites: site %s failed url %s", name, id, url);
			 continue;
			 }

		 id.MakeUpper();
		 vars data(f.memory);
		 var ts= data.indexOf(id);
		 if (ts>=0) 
			 data = data.substr(ts);

		 vara modes;
		 vara line1 = data.split("\n")[0].split("|");
		 for (int s=2; s<line1.length(); ++s)
			modes.push( line1[s].trim() );

		 vars mode = BRmode(modes.join(";"));
		 if (mode!="")
			addsite(sitelist, siteid, desc, CData(lat), CData(lng), mode);
		}
	}

	return TRUE;
}


//&date=2008JUN10
void BRGP_getdate(CSite *site, vars &date, double *vres)
{
	   vara mode = vars(site->Info(SITE_MODE)).split("=");

	   int today = istoday(date);
	   double datev = CDATA::GetDate(date);
   
	   vars url = BRGP_geturl + CString(site->id).MakeLower() + "&date=";
	   if (!today)
		   {
		   vara datep = date.split("-");
		   url += datep[0]+CDATA::GetMonth(atoi(datep[1]))+datep[2];
		   }
	   //Log(LOGINFO, "getdate M%s %s", mode[m], url);
	   DownloadFile f;
	   if (logging) Log(LOGINFO, "url: %s %s\n%s", site->fid, date, url);
	   if ( DownloadRetry(f, url) )
		   {
		   Log(LOGERR, "%s getdate %s: failed url: %s", site->fid, date, url);
		   vres[0] = InvalidURL;
		   return;
		   }

	   vars data(f.memory);
	   var ts= data.indexOf("<PRE>");
	   var te= data.indexOf("</PRE>");
	   vara table = data.substr(ts, te-ts).split("\n");
	   if (table.length()<2)
		   return;

	   vara header = table[1].split("|");
	   if (header.length()<2)
		   return;

		vars ldate = header[1];
		double ldatev = CDATA::GetDate(ldate, "YYMMMDD");
		if (ldatev==InvalidDATE)
			{
			Log(LOGERR, "%s getdate %s: no good date %s", site->fid, date, ldate);
			return;
			}

		if (today && datev-ldatev>MAXTODAY)
			return; // too different dates

		if (!today && datev-ldatev!=0)
			return; // too different dates   

	   // process table
	   #define MAXPARAM 20
	   tavg avg[MAXPARAM];

	   int col[MAXPARAM];
	   vara colid = vars(BRmodes()).split();

	   for (int t=1; t<table.length(); ++t)
		{
		vara line = table[t].split("|");
		if (line.length()<2)
			{
			Log(LOGERR, "%s getdate %s: no good line %d!=3 %s", site->fid, date, line.length(), table[t]);
			continue;
			}

		if (line[0].trim()!="")
			{
			// HEADERS
			for (int s=0; s<MAXPARAM; ++s)
				col[s] = -1;
			for (int s=2; s<line.length(); ++s)
				col[s] = colid.indexOf(line[s].trim());
			}
		else
			// DATA
			for (int s=2; s<line.length(); ++s)
			 {
			 if (col[s]<0)
				 continue;

			 // get value
			 vars lval = line[s].trim(); 
			 double val = CDATA::GetNum(lval);
			 if (val==998877 || val<0)
				val=InvalidNUM;
			 if (val==InvalidNUM)
				continue;

			 if (today)
				{
				// TODAY
				avg[col[s]].set(val);
				}
			 else
				{
				// DAILY VALUE
				avg[col[s]].add(val);
				}
			 }
		 }

	// return average
	for (int s=0; s<BR_sensors.length(); ++s)
	  if (avg[s].cnt()>0)
		{
		int r = BRsensor(colid[s]);
		if (r>=0 && vres[r]<0)
			avg[s].get(vres[r]);
		}
}

//===========================================================================
// SCE
//===========================================================================


int SCE_getsites(const char *name, CSymList &sitelist, int active)
{
		CString url = MkString("http://kna.kisters.net/scepublic/data/stationdata.json?v=%g", GetTime());

		DownloadFile f;
		if ( DownloadRetry(f, url) )
			 {
			 Log(LOGERR, "%s getsites: failed url %s", name, url);
			 return FALSE;
			 }

		vara list(f.memory, "{\"attributes\"");
		for (int i=1; i<list.length(); ++i)
			{
				vars rid, id, desc, ldesc, lat, lng;
				GetSearchString(list[i], "\"river_id\"", rid, "\"", "\"");
				if (rid=="") continue;
				GetSearchString(list[i], "\"station_no\"", id, "\"", "\"");
				GetSearchString(list[i], "\"station_name\"", desc, "\"", "\"");
				GetSearchString(list[i], "\"station_longname\"", desc, "\"", "\"");
				GetSearchString(list[i], "\"station_latitude\"", lat, "\"", "\"");
				GetSearchString(list[i], "\"station_longitude\"", lng,  "\"", "\"");
				if (lat=="" || lng=="")
					{
					if (id=="131")
						lat = "37.1443", lng = "-119.3035";
					else
						Log(LOGERR, "Invalid lat/lng for %s %s", id, desc);
					}
				desc.MakeUpper();
				ldesc.MakeUpper();
				addsite(sitelist, name+CString(":")+id, ldesc!="" ? ldesc : desc, lat, lng);
				//station_name
				//station_id
				//"site_name" all SCE
				//"site_no" all SCE 
				//"site_id" strange number 
			}
		return TRUE;
}



#include "BasicExcelVC6.hpp"

using namespace YExcel;
CString cell(BasicExcelWorksheet* sheet, int r, int c)
{
	CString str;
	BasicExcelCell* cell = sheet->Cell(r,c);
	switch (cell->Type())
	{
		case BasicExcelCell::UNDEFINED:
			break;

		case BasicExcelCell::INT:
			str = CData(cell->GetInteger());
			break;

		case BasicExcelCell::DOUBLE:
			str = CData(cell->GetDouble());
			break;

		case BasicExcelCell::STRING:
			str = cell->GetString();
			break;

		case BasicExcelCell::WSTRING:
			str = cell->GetWString();
			break;
	}
	return str.Trim();
}

int SCE_Excel(CSite *site, const char *data, int size)
{
	BasicExcel e;
	CString tmpxls = "TMP.XLS";

	// Load a workbook with one sheet, display its contents and save into another file.
	CFILE f;
	if (!f.fopen(tmpxls, CFILE::MWRITE))
		return FALSE;
	f.fwrite(data, size, 1);
	f.fclose();

	e.Load(tmpxls);	
	//e.Load(data, size);

	int def = 0;
	BasicExcelWorksheet* sheet1 = e.GetWorksheet(def);
	if (sheet1)
	{
		size_t maxRows = sheet1->GetTotalRows();
		size_t maxCols = sheet1->GetTotalCols();

		if (maxCols<2)
			return FALSE;
		//cout << "Dimension of " << sheet1->GetAnsiSheetName() << " (" << maxRows << ", " << maxCols << ")" << endl;

		size_t r=0;
		for (; r<maxRows && cell(sheet1, r, 0)!="Date"; ++r);
		if (r>=maxRows)
			return FALSE;

		// process data
		for ( ; r<maxRows; ++r)
			{
			vars date = cell(sheet1, r, 0);
			vars val = cell(sheet1, r, 1);
			double vdate = CDATA::GetNum(date);			
			double v = CDATA::GetNum(val);
			if (vdate!=InvalidNUM && vdate!=InvalidDATE && v!=InvalidNUM)
				site->SetDate(vdate-2, v); // -2 to adjust for excel
			}
	}
  return TRUE;
}


void SCE_gethistory(CSite *site, vars &date, double *vres)
{
	   double curd = GetTime();
	   double datev = CDATA::GetDate(date);
	   if (curd-datev>360)
		   {
		   // too late, try usgs
		   USGS_getdate(site, date, vres);
		   return; 
		   }

	   // Download HISTORIC
	   vars url = "http://kna.kisters.net/scepublic/stations/"+site->id+"/Parameter/Q/DayMean%20Discharge%20data,%20year.xls";
	   DownloadFile f(TRUE);
	   if (logging) Log(LOGINFO, "url: %s %s\n%s", site->fid, date, url);
	   if ( DownloadRetry(f, url) )
		   {
		   Log(LOGERR, "%s getdate %s: failed url: %s", site->fid, date, url);
		   vres[0] = InvalidURL;
		   //try usgs anyway
		   USGS_getdate(site, date, vres);
		   return;
		   }
		
		if (!SCE_Excel(site, f.memory, f.size))
			{
			Log(LOGERR, "%s getdate %s: failed excel file url: %s", site->fid, date, url);
			return;
			}
		// try to get value again
		site->FindDate(date, vres);
}


void SCE_getdate(CSite *site, vars &date, double *vres)
{
	if (istoday(date))
		{
		// TODAY
		vars url = MkString("http://kna.kisters.net/scepublic/data/1.json?v=%g",GetTime());
	   DownloadFile f;
	   if (logging) Log(LOGINFO, "url: %s %s\n%s", site->fid, date, url);
	   if ( DownloadRetry(f, url) )
		   {
		   Log(LOGERR, "%s getdate %s: failed url: %s", site->fid, date, url);
		   vres[0] = InvalidURL;
		   return;
		   }

		vara list(f.memory, "{\"attributes\"");
		for (int i=1; i<list.length(); ++i)
			{
				vars id, val;
				GetSearchString(list[i], "\"1\":", id, "\"", "\"");
				if (id!=site->id) continue;

				GetSearchString(list[i], "\"value\"", val, "\"", "\"");
				vres[0] = CDATA::GetNum(val.replace(",",""));
				return;
			}
		}
	else
		{
		// download bulk historic
		site->Lock();
		// check if existing
		site->FindDate(date, vres);
		if (vres[0]<0 && vres[1]<0 && vres[4]<0)
			SCE_gethistory(site, date, vres);
		site->Unlock();
		}
}

//===========================================================================
// NID
//===========================================================================

/*
int NID_getsites(const char *name, CSymList &sitelist, int active)
{
	// manual entry from <COMMENT REMOVED FROM PUBLIC>
}



void NID_getdate(CSite *site, vars &date, double *vres)
{
	if (istoday(date))
		{
		// TODAY
	   vars url = MkString("http://nidwater.com/river-lake-hourly-data/%s.csv",site->id);
	   DownloadFile f;
	   if (logging) Log(LOGINFO, "url: %s %s\n%s", site->fid, date, url);
	   if ( DownloadRetry(f, url) )
		   {
		   Log(LOGERR, "%s getdate %s: failed url: %s", site->fid, date, url);
		   vres[0] = InvalidURL;
		   return;
		   }

		vara list(f.memory, "{\"attributes\"");
		for (int i=1; i<list.length(); ++i)
			{
				vars id, val;
				GetSearchString(list[i], "\"1\":", id, "\"", "\"");
				if (id!=site->id) continue;

				GetSearchString(list[i], "\"value\"", val, "\"", "\"");
				vres[0] = CDATA::GetNum(val);
				return;
			}
		}
	else
		{
		site->Lock();
		// too late, try usgs
		USGS_getdate(site, datev);
		// try to get value again
		site->FindDate(date, vres);
		site->Unlock();
		return; 
		}
}
*/

//===========================================================================
// DF
//===========================================================================

CSite *GetSite(const char *id);
CSite *MatchSite(double lat, double lng, CSite *osite);

void RW_getdate(CSite *site, vars &date, double *vres)
{
  // if set, get data from equivalence  
  double datev = CDATA::GetDate(date);
  vars fid = vars(site->Info(SITE_EQUIV));
  if (fid.length())
	{
	vara fidp = fid.split(":");
	if (fidp[0]==site->prv->name)
		return;
	if (fidp[0]==USGS)
		{
		// try usgs
		USGS_getdate(site, date, vres);
		return;
		}
	CSite *match = GetSite(fid);
	if (match)
		match->prv->getdate(match, date, vres);
	return;
	}

  // find equivalence by trying usgs first
  if (USGS_getdate(site, date, vres)>0)
	return;

  // try providers database
  CSite *match = MatchSite(site->sym->GetNum(SITE_LAT), site->sym->GetNum(SITE_LNG), site);
  if (match)
	{
	// save equivalence and return data
	SetEquivalence(site, match->fid);
	match->prv->getdate(match, date, vres);
	}
}


int DF_getsites(const char *name, CSymList &sitelist, int active)
{
		vara dist;
		DownloadFile f;
		vars url = "http://www.dreamflows.com/flowApi/sites.php?latMin=0&latMax=90&lonMin=-0&lonMax=-180&user=lucach&pass=DREAMFLOWS_PASSWORD";
		if ( DownloadRetry(f, url) )
			 {
			 Log(LOGERR, "%s getsites: failed url %s", name, url);
			 return FALSE;
			 }

		vara list(f.memory, "<site ");
		for (int i=1; i<list.length(); ++i)
			{
			vars &line = list[i];
			CString id, river, place, lat, lng;
			GetSearchString(line, "rid=", id, "\"", "\"");
			GetSearchString(line, "river=", river, "\"", "\"");
			GetSearchString(line, "place=", place, "\"", "\"");
			GetSearchString(line, "lat=", lat, "\"", "\"");
			GetSearchString(line, "long=", lng, "\"", "\"");
			if (id=="" || river=="" || place=="")
				{
				Log(LOGERR, "%s getsites: failed id=%s river=%s place=%s", name, id, river, place);
				continue;
				}
			CString mode  = id;
			while (mode.GetLength()<3)
				mode = "0"+mode;
			addsite(sitelist, name+vars(":")+id, river+" "+place, lat, lng, mode);
			}
	return TRUE;
}


void DF_getdate(CSite *site, vars &date, double *vres)
{
	   int today = istoday(date);
	   double datev = CDATA::GetDate(date);
	   /*
	   double curd = GetTime();
	   if (curd-datev>360*2)
		   {
		   // too late, try usgs
		   USGS_getdate(site, datev);
		   // try to get value again
		   site->FindDate(date, vres);
		   return; 
		   }
	   */

	   vara mode = vars("hour;davg").split(";");
	   CString end = CDate(datev); 
	   CString start = end;
	   if (today) 
			{
			start = CDate(datev-1); 
			end = "today";
			}
	   start.Replace("-", ""); 
	   end.Replace("-", "");

	   DownloadFile f;
	   for (int m=0; m<mode.length() && vres[0]<0; ++m)
	   {
		int day = today ? m : !m;
		vars url = "http://www.dreamflows.com/flowApi/flows.php?rid="+site->id+"&span="+mode[day]+"&from="+start+"&thru="+end+"&user=DREAMFLOWS_USERNAME&pass=DREAMFLOWS_PASSWORD";

	   //Log(LOGINFO, "getdate M%s %s", mode[m], url);
	   if (logging) Log(LOGINFO, "url: %s %s\n%s", site->fid, date, url);
	   if ( DownloadRetry(f, url) )
		   {
		   vres[0] = InvalidURL;
		   Log(LOGERR, "%s getdate %s: failed url: %s", site->fid, date, url);
		   continue;
		   }

	   vars data(f.memory);
	   vara lines = data.split("<flowData ");
	   if (lines.length()<2)
		   {
		   // no data
		   vres[0] = InvalidNUM;
		   continue;
		   }
	   
	   // temp units
	   CString T;
	   int celsius = FALSE;
	   GetSearchString(data, "tempUnit=", T, "\"", "\"");
	   if (T.Right(1)=="C")
		   celsius = TRUE;

	   // get last or compute average
	   vara sensor = vars("flow,gage,temp").split();
	   for (int s=0; s<sensor.length(); ++s)
	   {
		tavg avg;
		for (int l=lines.length()-1; l>=1; --l)
		 {
		 CString D, V;
		 vars &line = lines[l];
		 GetSearchString(line, "time=", D, "\"", "\"");
		 if (!today)
			if (CDATA::GetDate(D, "YYYY-MM-DD" )!=datev)
			 continue;
		 GetSearchString(line, sensor[s]+"=", V, "\"", "\"");
		 double nv = CDATA::GetNum(V);
		 if (nv==InvalidNUM)
			 continue;

		 // got value       	      
		 if (today)
		   {
		   avg.set(nv);
		   break;
		   }

		 // DAILY AVERAGE
		 avg.add(nv);
		 }

	   // got result!
	   avg.get(vres[s + (s==2 && celsius)]); 
	   }
	   }

   // if no data, find matching source
   if (!today && vres[0]==InvalidNUM && vres[1]==InvalidNUM)
	   RW_getdate(site, date, vres);
}       	    

//===========================================================================
// COERG
//===========================================================================
vara COERG_sensors = vars("QR=0,QT=0,QC=0,QW=0,HG=1,HT=1,TW=2,TU=2,TC=3").split();
vara COERG_badsensors = vars("PC,PA,UD,US,TA,HP,SW,SD,WT,VB,HL,YR,VT,WO,WC,TN,TX,HW,WP,HF,HN,HX,PN,PP,WB,XR,LF,GD,IT,HA,HE,QO,QI,QJ,UP,WV,WX,WS,PE").split();

int COERG_getsites(const char *name, CSymList &sitelist, int active)
{
		vara dist;
		DownloadFile f;
		vars base = "http://rivergages.mvr.usace.army.mil";
		vars baseurl = base+"/WaterControl/new/layout.cfm";

		// get districts
		{
		vars url = baseurl;
		if ( DownloadRetry(f, url) )
			 {
			   Log(LOGERR, "%s getsites: failed url %s", name, url);
			   return FALSE;
			 }
		vars districts;
		vars data(f.memory);
		GetSearchString(data, ">District<", districts, "", "<select");
		vara districtsp = districts.split("<option");
		for (int i=1; i<districtsp.length(); ++i)
			{
			vars d;
			GetSearchString(districtsp[i], "value", d, "\"", "\"");
			double val = CDATA::GetNum(d);
			if (val!=InvalidNUM)
				dist.push(CData(val));
			}
		}


		// process districts
		vars sep = "/WaterControl/stationinfo";

		vara ulist;
		for (int u=0; u<dist.length(); ++u)
		{
		CString url = MkString("%s?POST?fld_district=%s&rb_limit=RT&fld_readingsin=S&fld_basin=All", baseurl, dist[u]);
		printf("downloading %d/%d     \r", u, dist.length());
		if ( DownloadRetry(f, url) )
			 {
			 Log(LOGERR, "%s getsites: failed url %s", name, url);
			 continue;
			 }
		vara list(f.memory, sep);
		for (int i=1; i<list.length(); ++i)
			{
				vars url;
				GetSearchString(list[i], "", url, "", "\"");
				if (url.length()>0)
					ulist.push(base+sep+url);
			}
		}

		// process urls
		vara gmodes;
		for (int i=0; i<COERG_sensors.length(); ++i)
			gmodes.push(COERG_sensors[i].split("=")[0]);
		for (int i=0; i<ulist.length(); ++i)
			{
				int fi;
				vars id;
				GetSearchString(ulist[i], "?sid=", id, "", "&");
				if ((fi=sitelist.Find(name+vars(":")+id))>=0)
					{
					CString mode = sitelist[fi].GetStr(SITE_MODE);
					double v = sitelist[fi].GetNum(SITE_LAT);
					if (v!=0 && v!=InvalidNUM && !mode.IsEmpty())
						{
						// skip site
						sitelist[fi].index=1;
						continue;
						}
					}

				printf("downloading %d/%d     \r", i, ulist.length());
				if ( DownloadRetry(f, ulist[i]) )
					{
					Log(LOGERR, "%s getsites: failed ulist %s", name, ulist[i]);
					continue;
					}
				vars url, desc, mode, lat, lng, agency;
				GetSearchString(f.memory, "Latitude:", lat, "", "<");
				GetSearchString(f.memory, "Longitude:", lng, "", "<");
				GetSearchString(f.memory, "", desc, "<b>", "</b>");
				if (id=="")
					{
					Log(LOGERR, "%s getsites: failed id %s", name, ulist[i]);
					continue;
					}

				vara mp;
				GetSearchString(f.memory, "Historic Data", mode, "<select", "</select");
				vara modes = mode.split("<option");
				for (int o=1; o<modes.length(); ++o)
					{
					vars m, mdesc;
					GetSearchString(modes[o], "value=", m, "\"", "\"");
					GetSearchString(modes[o], "value=", mdesc, ">", "<");
					int mi = gmodes.indexOf(m);
					if (mi<0)
						{
						if (!m.IsEmpty() && COERG_badsensors.indexOf(m)<0)
							Log(LOGERR, "%s getsites: %s unknown mode %s = %s", name, id, m, mdesc);
						continue;
						}
					mp.push(COERG_sensors[mi]);
					}
				if (mp.length()==0)
					{
					//Log(LOGERR, "%s getsites: no good modes %s = %s", name, id, desc);
					continue;
					}

				desc = cleanhref(desc);
				double latv = CDATA::GetNum(lat);
				double lngv = CDATA::GetNum(lng);
				if (latv<0) latv = -latv;
				if (lngv>0) lngv = -lngv;
				if (latv==0 && lngv==0)
					{
					if (id=="NSP" || id=="CI05E6E0")
						continue;
					if (!USGS_getlatlng(id, desc, latv, lngv))
						Log(LOGERR, "%s getsites: NEED FIX! lat/lng for \"%s\" = \"%s\"", name, id, desc);
					}

				addsite(sitelist, name+CString(":")+id, desc, CData(latv), CData(lngv), mp.join(";"), "", "", TRUE);
				//addsite(sitelist, name+CString(":")+id+"I", cleanhref(river.trim()) +" inflow " + cleanhref(desc.trim().split("(")[0]), "", "", gid+"=FLOW-RES IN ", cleanhref(agency.trim()), TRUE);
			}


		return TRUE;
}






void COERG_getdate(CSite *site, vars &date, double *vres)
{
#if 0
	if (today)
		 http://rivergages.mvr.usace.army.mil/WaterControl/shefdata2.cfm?sid=TRR&d=1&dt=S
#endif

	   int today = istoday(date);
	   double datev = CDATA::GetDate(date);
	   vara datep = vars(CDate(datev)).split("-");
	   vars yyyy = datep[0], mm = datep[1], dd = datep[2];

	   vara mode = vars(site->Info(SITE_MODE)).split(";");

	   DownloadFile f;
	   for (int m=0; m<mode.length(); ++m)
	   {
	   vara mod = mode[m].split("=");

	   // get sensor data ONLY if not already available
	   int sensor = atoi(mod[1]);
	   if (sensor<0 || vres[sensor]>=0)
		  continue;

		vars url = "http://rivergages.mvr.usace.army.mil/WaterControl/shefgraph-historic.cfm?sid="+site->id+"?POST?";
		url += "fld_param="+mod[0]+"&fld_mon1="+mm+"&fld_day1="+dd+"&fld_year1="+yyyy+"&fld_mon2="+mm+"&fld_day2="+dd+"&fld_year2="+yyyy;
		url += "&fld_displaytype=S&fld_from1="+mm+"/"+dd+"/"+yyyy+"&fld_to1="+mm+"/"+dd+"/"+yyyy;
		url += "&fld_frompor="+mm+dd+"&fld_topor="+mm+dd+"&hdn_floodyear=&hdn_floodyear2=&fld_type1=Table";
	   //Log(LOGINFO, "getdate M%s %s", mode[m], url);
	   if (logging) Log(LOGINFO, "url: %s %s\n%s", site->fid, date, url);
	   //if ( DownloadRetry(f, url) )
	   if ( f.Download(url))
		   {
		   vres[sensor] = InvalidURL;
		   Log(LOGERR, "%s getdate %s: failed url: %s", site->fid, date, url);
		   continue;
		   }

	   // instead of computing average, get level at 1pm
	   vara lines(f.memory, "class=\"style3\"");
	   tavg avg;
	   for (int l=lines.length()-1; l>=1; --l)
		 {
		 vara line = lines[l].split("<td>");
		 if (line.length()!=3)
			{
			Log(LOGERR, "%s getdate %s: no good line %d!=3 %s", site->fid, date, line.length(), lines[l]);
			continue;
			}
		 vars v = cleanhref(line[2]);
		 double nv = CDATA::GetNum(v);
		 //console.log("addval "+siteid+" "+date+" val:"+nv);
		 if (nv==InvalidNUM)
			 continue;

		  // got value       	      
		  vars ldate = cleanhref(line[1]);
		  double ldatev = CDATA::GetDate(ldate, "MM/DD/YYYY" ); //ldate.substr(0,4)+"-"+ldate.substr(4,2)+"-"+ldate.substr(6,2) );
		  if (ldatev==InvalidDATE)
				{
				Log(LOGERR, "%s getdate %s: no good date %s", site->fid, date, ldate);
				continue;
				}

		  if (!today)
			{
			// DAILY AVERAGE
			if (ldatev==datev)
				avg.add(nv);
			continue;
			}

		  if (datev-ldatev>MAXTODAY)
				{
				Log(LOGERR, "%s getdate %s: date too different %s", site->fid, date, ldate);
				continue;
				}
		  avg.set(nv);
		  break;
		  }

	   // got result!
	   avg.get(vres[sensor]);
	   }
}       	    


//===========================================================================
// COESPK
//===========================================================================


int COESPK_getsites(const char *name, CSymList &sitelist, int active)
{
		//"http://www.spk-wc.usace.army.mil/reports/archives.html"
		
		const char *urls[] = {
			"http://www.spk-wc.usace.army.mil/plots/california.html",
			"http://www.spk-wc.usace.army.mil/plots/greatbasin.html",
			NULL
		};

		DownloadFile f;
		for (int u=0; urls[u]!=NULL; ++u)
		{
		CString url = urls[u];
		if ( DownloadRetry(f, url) )
			 {
			 Log(LOGERR, "%s getsites: failed url %s", name, url);
			 continue;
			 }

		vara list(f.memory, "<tr");
		for (int i=1; i<list.length(); ++i)
			{
				vars id, desc, river, agency;
				GetSearchString(list[i], "fcgi-bin/getplot.py?plot", id, "=", "&");
				if (id=="")
					continue;

				vara line = list[i].split("<td");
				GetSearchString(line[1], ">", desc, "", "</td>");
				GetSearchString(line[2], ">", river, "", "</td>");
				GetSearchString(line[3], ">", agency, "", "</td>");
				vars gid = id.trim(); 
				id = id.upper().trim().substr(0,3); 
				addsite(sitelist, name+CString(":")+id+"O", cleanhref(river.trim()) +" outflow " + cleanhref(desc.trim().split("(")[0]), "", "", gid+"=FLOW-RES OUT", cleanhref(agency.trim()), "", TRUE);
				//addsite(sitelist, name+CString(":")+id+"I", cleanhref(river.trim()) +" inflow " + cleanhref(desc.trim().split("(")[0]), "", "", gid+"=FLOW-RES IN ", cleanhref(agency.trim()), TRUE);
			}
		}

		// delete incoming flow, too inaccurate
		for (int i=0; i<sitelist.GetSize(); ++i)
			if (sitelist[i].id.Right(1)=="I")
				sitelist.Delete(i--);

		return TRUE;
}





void COESPK_gethistory(CSite *ssite, vars &date, double *vres)
{

	// compute date
	int today = istoday(date);
	double datev = CDATA::GetDate(date);

/*
// download in h and d mode for today
// download 2 water years for completeness
2009->30SEP2008
2010->30SEP2009
2011->30SEP2010-29SEP2012
2012->30SEP2011-29SEP2013
2013->30SEP2014
2014->01OCT2013
*/
for (int y=0; y<2; ++y)
{
	vars url = "http://www.spk-wc.usace.army.mil/fcgi-bin/getplottext.py?plot="+vars(ssite->gid).lower(); 
	if (today)
		url += "&length=5day&interval=" + vars(y==0 ? "d" : "h"); 
	else
		url += MkString("&length=wy&interval=d&wy=%d", OleDateTime( datev ).GetYear()+y);

	DownloadFile f(TRUE);
	   if (logging) Log(LOGINFO, "url: %s %s\n%s", ssite->fid, date, url);
	   if ( DownloadRetry(f, url) )
		   {
		   Log(LOGERR, "%s getdate %s: failed url: %s", ssite->fid, date, url);
		   vres[0] = InvalidURL;
		   return;
		   }

	   vars data(f.memory);
	   var ts= data.indexOf("<PRE");
	   if (ts<0)
		   ts = data.indexOf("<pre");
	   var te= data.indexOf("</PRE");
	   if (te<0)
			te= data.indexOf("</pre");
	   vara table = data.substr(ts, te-ts).split("\n");
	   if (ts<0 || te<0 || table.length()<2)
			return;

	 // process siblings toghether
	 for (int s=0; s<ssite->group.GetSize(); ++s)
	 {
		CSite *site = ssite->group[s];
		vara titlep = vars(site->Info(SITE_MODE)).split("=");
		if (titlep.length()<2)
			continue;

		vars title = titlep[1];
		int c = -1, len = title.length()+1;
		for (int i=0; i<table.length(); ++i)
			{
			vars &line = table[i];
			int nc = line.indexOf(title);
			if (nc>=0) c = nc;
			if (c<0) 
				continue;
			
			//  we got header, now loof for DIGIT_DIGIT
			int dd=0, dn=0;
			for (; dd<line.length() && !isdigit(line[dd]); ++dd);
			for (dn=0; dd<line.length() && isdigit(line[dd]); ++dd, ++dn);
			if (dn!=4) // time
				continue;
			for (; dd<line.length() && !isdigit(line[dd]); ++dd);
			for (dn=0; dd<line.length() && !isspace(line[dd]); ++dd, ++dn);
			if (dn!=9) //date
				continue;

			vars ldate = line.substr(dd-dn, dn);	
			double ldatev = CDATA::GetDate(ldate, "DDMMMYYYY");
			if (ldatev<=0)
				continue;

			const char *valine = line;
			vars val = line.substr(c, len);
			double nv = CDATA::GetNum(val);
			if (nv==InvalidNUM)
				continue;

			if (today)
				{
				// get latest good value valid for range
				if (datev-ldatev>1)
				   continue;
				// we got value!
				site->SetDate(datev, nv);
				}
			else
				{
				// set periferic historics
				site->SetDate(ldatev, nv);
				}
			}
	 }
}

	// set return value
	vres[0] = InvalidNUM;
	ssite->FindDate(date, vres);
}


void COESPK_getdate(CSite *site, vars &date, double *vres)
{
	site->group.Head()->Lock();
	if (site->FindDate(date, vres)=="")		
		COESPK_gethistory(site, date, vres);
	site->group.Head()->Unlock();
}


//===========================================================================
// WECY
//===========================================================================

vara WECY_sensors = vars("Discharge:DSG=0,Stage:STG=1,Water Temp.:WTM=3").split();

int WECY_getsites(const char *name, CSymList &sitelist, int active)
{
		
	const char *urls[] = {
		"https://fortress.wa.gov/ecy/wrx/wrx/flows/regions/state.asp",
		"https://fortress.wa.gov/ecy/wrx/wrx/flows/stationMessages.js",
		NULL
	};

	DownloadFile f;
	for (int u=0; urls[u]!=NULL; ++u)
	{
		CString url = urls[u];
		if ( DownloadRetry(f, url) )
			 {
			 Log(LOGERR, "%s getsites: failed url %s", name, url);
			 continue;
			 }

		{
		vara list(f.memory, "showstationnote('");
		for (int i=1; i<list.length(); ++i)
			{
				vara line = list[i].split("', '");
				vars id = line[0].upper().trim(); 
				if (id=="")
					{
					Log(LOGERR, "%s getsites: empty showstationnote id line %d", name, i);
					continue;
					}
				// line[2] = 'telem*'?
				addsite(sitelist, name+CString(":")+id, line[1].trim(), "", "", "", "", "", TRUE);
			}
		}

		{
		vara list(f.memory, "\nstation.s");
		for (int i=1; i<list.length(); ++i)
			{
				vars id = GetToken(list[i], 0, '=');
				id = id.upper().trim(); 
				if (id=="")
					{
					Log(LOGERR, "%s getsites: empty station.s id line %d", name, i);
					continue;
					}

				vara mode;
				vara mtext = WECY_sensors;
				for (int m = 0; m<mtext.length(); ++m)
					{
					vars v;
					vara mp = mtext[m].split(":");
					GetSearchString(list[i], mp[0]+" = ", v, "", " ");
					if (CDATA::GetNum(v)!=InvalidNUM)
						mode.push(mp[1]);
					}

				addsite(sitelist, name+CString(":")+id, "", "", "", mode.join(";"), "", "", TRUE);
			}
		}		
	}

	// get detailed information when needed
	for (int i=0; i<sitelist.GetSize(); ++i)
		if (sitelist[i].GetStr(SITE_LAT)=="")
		{
		vars url = "https://fortress.wa.gov/ecy/wrx/wrx/flows/station.asp?sta="+vars(sitelist[i].id).split(":")[1];
		if ( DownloadRetry(f, url) )
			 {
			 Log(LOGERR, "%s getsites: failed sta=%s url =%s", name, sitelist[i].id, url);
			 continue;
			 }
		vars data(f.memory);
		vars desc, lat, lng;
		GetSearchString(data, "Latitude</td>", lat, "", "</tr>");
		GetSearchString(data, "Longitude</td>", lng, "", "</tr>");
		GetSearchString(data, "", desc, "<h1>", "</h1");
		double vlat = getdeg(cleanhref(lat), NULL, " ");
		double vlng = getdeg(cleanhref(lng), NULL, " ");
		if (vlat==InvalidNUM || vlng==InvalidNUM)
			{
			Log(LOGERR, "Invalid coordinates for %s (%s, %s)", sitelist[i].id, lat, lng);
			continue;
			}
		if (vlng>0) vlng = -vlng;
		addsite(sitelist, sitelist[i].id, cleanhref(desc), CData(vlat), CData(vlng));
		}

	// delete the stations with no information
	for (int i=0; i<sitelist.GetSize(); ++i)
		if (sitelist[i].GetStr(SITE_MODE)=="")
			sitelist.Delete(i--);

	return TRUE;
}






void WECY_gettoday(CSite *site, vars &date, double *vres)
{
			double datev = CDATA::GetDate(date);
			DownloadFile f;
			CString url = "https://fortress.wa.gov/ecy/wrx/wrx/flows/stationMessages.js";
			if (logging) Log(LOGINFO, "url: %s %s\n%s", site->fid, date, url);
			if ( DownloadRetry(f, url) )
				 {
				 Log(LOGERR, "%s gettoday: failed url %s", site->fid, url);
				 vres[0] = InvalidURL;
				 return;
				 }
			vara list(f.memory, "\nstation.s");
			for (int i=1; i<list.length(); ++i)
				{
					vars id = GetToken(list[i], 0, '=');
					id = id.upper().trim(); 
					if (id=="")
						{
						Log(LOGERR, "%s gettoday: empty station.s id line %d", site->fid, i);
						continue;
						}

					// find site
					CSite *gsite = NULL;
					for (int g=0; g<site->group.GetSize() && !gsite; ++g)
						if (id==site->group[g]->id)
							gsite = site->group[g];
					if (!gsite)
						{
						//Log(LOGERR, "%s gettoday: station.s %s not in group", site->fid, id);
						continue;
						}

					// set data
					VRESNULL
					vara mtext = WECY_sensors;
					for (int m = 0; m<mtext.length(); ++m)
						{
						vars val;
						vara mp = mtext[m].split(":");
						GetSearchString(list[i], mp[0]+" = ", val, "", " ");
						double v = CDATA::GetNum(val);
						if (v!=InvalidNUM)
							vres[atoi(mp[1].split("=")[1])] = v;
						}
					
					vars ldate;
					GetSearchString(list[i], "Latest values", ldate, "of", "<");
					double ldatev = CDATA::GetDate(ldate, "M/D/Y");
					if (datev-ldatev>1)
						continue;
					
					gsite->SetDate(date, vres);
				}

	site->FindDate(date, vres);
}


int GetWaterYear(double date)
{
		COleDateTime d = OleDateTime(date);
		int m = d.GetMonth();
		int y = d.GetYear();
		return m>=10 ? y+1 : y;	
}

void WECY_gethistory(CSite *site, vars &date, double *vres)
{
		int cwy = GetWaterYear(GetTime());
		int wy = GetWaterYear(CDATA::GetDate(date));
		DownloadFile f;
		vara modes = vars(site->Info(SITE_MODE)).split(";");
		for (int m=0; m<modes.length(); ++m)
			{
			vara mode = modes[m].split("=");
			vars wymode = (wy!=cwy) ? MkString("_%d", wy) : "";
			vars url = MkString("https://fortress.wa.gov/ecy/wrx/wrx/flows/stafiles/%s/%s%s_%s_DV.txt",site->id, site->id, wymode, mode[0]);

			int sensor = atoi(mode[1]);
			if (vres[sensor]>=0)
				continue;
				
			if (logging) Log(LOGINFO, "url: %s %s\n%s", site->fid, date, url);
			if ( DownloadRetry(f, url) )
			   {
			   if (logging) 
				  Log(LOGERR, "%s getdate %s: failed url: %s", site->fid, date, url);
			   continue;
			   }

			vara lines(f.memory, "\n");
			for (int i=0; i<lines.length(); ++i)
				{
				vara line = lines[i].split(" ");
				for (int l=line.length()-1; l>=0; --l)
					if (line[l].length()==0)
						line.splice(l, 1);
				if (line.length()<3 || line[0].length()!=10 || line[0][2]!='/')
					continue;
					
				double datev = CDATA::GetDate(line[0], "M/D/Y");
				double v = CDATA::GetNum(line[2]);
				site->SetDate(datev, v, sensor);
				}
			}

	site->FindDate(date, vres);
}



void WECY_getdate(CSite *site, vars &date, double *vres)
{
	if (istoday(date))
		{
		// current status
		site->group.Head()->Lock();
		if (site->FindDate(date, vres)=="")
			WECY_gettoday(site, date, vres);
		site->group.Head()->Unlock();
		}		
	else
		{
		// historical
		WECY_gethistory(site, date, vres);
		}
}

//===========================================================================
// OWRD
//===========================================================================


int OWRD_getsites(const char *name, CSymList &sitelist, int active)
{
		CString url = "http://apps.wrd.state.or.us/apps/sw/hydro_near_real_time/near_real_time_gage_station_kml.aspx";

		DownloadFile f;
		if ( DownloadRetry(f, url) )
			 {
			 Log(LOGERR, "%s getsites: failed url %s", name, url);
			 return FALSE;
			 }

		vara list(f.memory, "<Placemark");
		for (int i=1; i<list.length(); ++i)
			{
				CString id, desc, lat, lng, usgs;
				// reject usgs duplicates
				//if (list[i].indexOf("usgs.gov/nwis/")>=0)
				//	continue;
				// accept only if near real time data available
				if (active && list[i].indexOf("Instantaneous")<0)
					continue;
				GetSearchString(list[i], "Station Number:", id, " ", "&");
				GetSearchString(list[i], "<name", desc, ">", "<");
				GetSearchString(list[i], "\"station_longname\"", desc, "\"", "\"");
				GetSearchString(list[i], "<coordinates", lat, ",", "<");
				GetSearchString(list[i], "<coordinates", lng,  ">", ",");
				GetSearchString(list[i], "/nwis/uv?site_no", usgs, '=', '\'');
				if (usgs!="") 
					usgs = "USGS:"+usgs;
				addsite(sitelist, name+CString(":")+id, desc, lat, lng, NULL, NULL, usgs);
			}
		return TRUE;
}




void OWRD_getdate(CSite *site, vars &date, double *vres)
{
	   int today = istoday(date);
	   double start, end, datev;
	   start = end = datev = CDATA::GetDate(date);
	   start = start-1;
	   end = end + 1;

	   //ASSERT(site->id!="14140000");
	   vara mode = vars(today ? "Instantaneous_Flow,Instantaneous_Stage" : "MDF,Mean_Daily_Stage" ).split();
	
	   DownloadFile f;
	   for (int sensor=0; sensor<mode.length(); ++sensor)
	   {

		vars url = vars("http://apps.wrd.state.or.us/apps/sw/hydro_near_real_time/hydro_download.aspx?station_nbr=")+site->id+"&start_date="+CDate(start)+"+08:00"+"&end_date="+CDate(end)+"+14:00"+"&dataset="+mode[sensor]+"&format=tsv";
	   //Log(LOGINFO, "getdate M%s %s", mode[m], url);
		if (logging) Log(LOGINFO, "url: %s %s\n%s", site->fid, date, url);
	   if ( DownloadRetry(f, url) )
		   {
		   vres[sensor] = InvalidURL;
		   Log(LOGERR, "%s getdate %s: failed url: %s", site->fid, date, url);
		   continue;
		   }

	   vars data(f.memory);
	   if (data.indexOf("<body")>=0)
		   {
		   vres[sensor] = InvalidNUM;
		   //Log(LOGERR, "%s getdate %s: no html url: %s", site->fid, date, url);
		   continue;
		   }

	   // process csv
	   tavg avg;
	   vara lines = data.split("\n");
		//console.log("addval "+siteid+" "+date+" :"+data.substr(0,40));
	   for (int l=lines.length()-1; l>0; --l)
		 {
		 vara line = lines[l].split("\t");
		 if (line.length()<3)
			{
			Log(LOGERR, "%s getdate %s: no good line %d!=3 %s", site->fid, date, line.length(), lines[l]);
			continue;
			}
		 double nv = CDATA::GetNum(line[2]);
		 if (nv==InvalidNUM)
			 continue;

		  // got value       	      
		  vars ldate =  line[1].trim().substr(0,10);
		  double ldatev = CDATA::GetDate( ldate, "M-D-Y" );
		  if (ldatev==InvalidDATE)
			{
			Log(LOGERR, "%s getdate %s: no good date %s", site->fid, date, ldate);
			continue;
			}

		  if (!today)
			{
			// DAILY AVERAGE
			if (ldatev == datev)
				avg.add(nv);
			continue;
			}

		  // TODAY
		  if (datev-ldatev>MAXTODAY)
			{
			Log(LOGERR, "%s getdate %s: date too different %s", site->fid, date, ldate);
			continue;
			}
		  avg.set(nv);
		  break;
		  }


	   // got result!
		avg.get(vres[sensor]);
	   }
   // if no data, find matching source
   if (today && vres[0]==InvalidNUM)
	   RW_getdate(site, date, vres);
}       	    


//===========================================================================
// Waterflow
//===========================================================================
class tsite {
public:
	union {
		CSite *csite;
		CSym *csym;
	};
	double lat, lng;
	int cnt;
	tsite(void)
	{
		csite = NULL;
		csym = NULL;
	}


	tsite(CSym *sym)
	{
		csym = sym;
		lat = sym->GetNum(SITE_LAT);
		lng = sym->GetNum(SITE_LNG);
	}

	tsite(CSym *sym, double _lat, double _lng)
	{
		csym = sym;
		lat = _lat;
		lng = _lng;
	}

	tsite(CSite *site, double _lat, double _lng)
	{
		csite = site;
		lat = _lat;
		lng = _lng;
	}

	tsite(CSite *site)
	{
		csite = site;
		lat = site->sym->GetNum(SITE_LAT);
		lng = site->sym->GetNum(SITE_LNG);
	}

};


class tsiteval {
public:
	double val;
	tsite *site;

	tsiteval()
	{
		val = 0;
		site = NULL;
	}

	tsiteval(double v, tsite *s)
	{
		val = v; 
		site =s;
	}

	double operator-(const tsiteval &b) 
	{
	double d = b.val-val;
	if (d>0) return 1;
	if (d<0) return -1;
	return 0;
	}
};





typedef CArrayList <tsiteval> tsitevalarray;
typedef CArrayList <tsite> tsitelist;
tsitelist sortlist;
//tsitevalarray lat, lng;

static int cmplatlng(tsite *lle1, tsite *lle2)
	{
		if ((lle1)->lat>(lle2)->lat) return 1;
		if ((lle1)->lat<(lle2)->lat) return -1;
		if ((lle1)->lng>(lle2)->lng) return 1;
		if ((lle1)->lng<(lle2)->lng) return -1;
		return 0;
	}

void RectSites(tsitelist &sortlist, tsitelist &list, register double boxlat1, register double boxlng1, register double boxlat2, register double boxlng2)
{
	int listsize = list.GetSize();
	sortlist.Sort(cmplatlng);

	int aprox = 0;
	tsite mock; mock.lat = boxlat1; mock.lng = boxlng1;
	sortlist.Find(cmplatlng, mock, &aprox);

	//for (register int i = FindAprox(lat, maxlat); i<imax && lat[i].val>=minlat; ++i)
	register int nlle = sortlist.GetSize();
	register tsite *data = &sortlist.Head();

	register int j = aprox;
	  while (j<nlle && data[j].lat < boxlat1)
		  ++j;
	  // process inside box
	  for (register int k=j; k<nlle; ++k) 
		{
		if (data[k].lat > boxlat2)
			break; // finish box
		if (data[k].lng < boxlng1 || data[k].lng > boxlng2)
			continue; // out of bounds
		list.AddTail(data[k]);
		}

#ifdef DEBUG
	// check for accuracy
	int cnt = 0;
	for (register int i = 0; i<nlle; ++i)
		{
		register tsite &site = data[i];
		if (site.lat>=boxlat1 && site.lat<=boxlat2)
		   if (site.lng>=boxlng1 && site.lng<=boxlng2)
			   ++cnt;
		}
	ASSERT( cnt == list.GetSize()-listsize );
#endif
}



CSymList nooamap[2];
tsitelist nooamaplist[2];

CSymList usgs[2];
tsitelist usgslist[2];



#define NOOA "NOOA"
#define NOOAKML "NOOAKML"
#define NOOAMAP "NOOAMAP"


void LoadSite(const char *name, CSymList &list)
{
	list.Load(filename(name));
	list.Sort();

	// delete any disabled locations
	for (int i=list.GetSize()-1; i>=0; --i)
		if (list[i].GetStr(SITE_LAT)=="x")
			list.Delete(i);
}

void SaveSite(const char *name, CSymList &list, int check = TRUE)
{
	CString extra;
	CString file = filename(name);

	// integrity check
	if (check)
	{
		int oldcnt = 0, newcnt = 0, upcnt = 0;
		for (int i=0; i<list.GetSize(); ++i)
			{
			double index = list[i].index;
			if (index==0) ++oldcnt;
			if (index==-1) ++newcnt;
			if (index==1) ++upcnt;
			}
		extra = MkString("(%d new, %d updated, %d old)", newcnt, upcnt, oldcnt);
		if (newcnt+upcnt==0)
			Log(LOGERR, "%s getsites is MALFUNCTIONING!", name);
		if (newcnt+upcnt+oldcnt!=list.GetSize())
			Log(LOGERR, "%s getsites is INTEGRITY ERROR (new+old+up!=tot)!", name);
	}

	list.Sort();
	if (list.Save(file))
		Log(LOGINFO, "%s %d ids %s", file, list.GetSize(), extra);
}


void WaterflowSaveUSGSSites(void)
{
/*
	double lat1, lng1, lat2, lng2;
	convertUTMToLatLong("11N", "0", "0", lat1, lng1);
	convertUTMToLatLong("11N", "100", "100", lat2, lng2);
	double dlat = lat2-lat1;
	double dlng = lng2-lng1;

	double d = Distance(0,0, 0.02, 0.0);
*/

	WaterflowLoadSites();

	CSymList usgsp;
	LoadSite(USGS, usgsp);
#if 1
	USGS_getsites(USGS, usgsp, TRUE);
	SaveSite(USGS, usgsp);
#endif

	CSymList usgsm;
#if 1
	USGS_getsites(USGS, usgsm, FALSE);
#else
	LoadSite(INVERSE(USGS), usgsm);
#endif

	// take out current USGS
	for (int i=usgsm.GetSize()-1; i>=0; --i)
		if (usgsp.Find(usgsm[i].id)>=0)
			usgsm.Delete(i);

	// save active withing 5Y
	SaveSite(vars(USGS)+"-OLD", usgsm);

	// take out other sources <100m away
	for (int i=usgsm.GetSize()-1; i>=0; --i)
		if (MatchSite(usgsm[i].GetNum(SITE_LAT), usgsm[i].GetNum(SITE_LNG), NULL))
			usgsm.Delete(i);

	SaveSite(INVERSE(USGS), usgsm);
	WaterflowUnloadSites();
}


int identifyforecastsite(CSym &sym, CSymList &nooa);


void getnooasites(CSymList &nooa)
{
	DownloadFile f;
	// get NOOA site info for all states
	nooa.header = "ID,DESC,LAT,LNG,EQUIV";
#if 1
	// HAD sites
	for (int s=0; s<states.length(); ++s)
		{
		  CString url = MkString("http://www.nws.noaa.gov/oh/hads/USGS/%s_USGS-HADS_SITES.txt", states[s]);
		  if ( DownloadRetry(f, url ) )
			 {
			 Log(LOGERR, "%s getnooasites: failed url %s", NOOA, url);
			 continue;
			 }
		
		  vara table(f.memory, "\n");

		  // skip header
		  for (int i=0; i<table.length() && table[i][0]!='-'; ++i);

		  for (i=i+1; i<table.length(); ++i)
			{
			vara line = table[i].split("|");
			if (line.length()<7)
				{
				Log(LOGERR, "%s getnooasites: BAD LINE %s", NOOA, table[i]);
				continue;
				}
			
			double lat = getdeg(line[4],NULL," ");
			double lng = getdeg(line[5],NULL," ");
			 if (lat==InvalidNUM || lng==InvalidNUM)
				{
				Log(LOGERR, "%s Invalid lat/lng %s url: %s", NOOA, table[i], url);
				continue;
				}
			 if (lng>0) lng = -lng;
			 int n = addsite(nooa, NOOA+vars(":")+line[0].trim(), line[6].trim(), CData(lat), CData(lng), "HADSLIST", "", "USGS"+vars(":")+line[1].trim());
			 if (n>=0) identifyforecastsite(nooa[n], nooa);
			}
		}
#endif
	nooa.Sort();
		
	//CString url = MkString("https://hads.ncep.noaa.gov/charts/%s.shtml", states[s]);
#if 1
	// HAD maps (no longer exists)
	for (int s=0; s<states.length(); ++s)
		{
		  //CString url = MkString("http://amazon.nws.noaa.gov/hads/maps/%s_map.html", states[s]);
		  CString url = MkString("http://hads.ncep.noaa.gov/maps/%s_map.html", states[s]);
		  if ( DownloadRetry(f, url ) )
			 {
			 Log(LOGERR, "%s getnooasites: failed url %s", NOOA, url);
			 continue;
			 }
		
		  vara table(f.memory, "=createMarker(");

		  // skip header
		  for (int i=1; i<table.length(); ++i)
			{
			vars id, desc, text;
			GetSearchString(table[i], ",", text, "\"", "\"");
			//GetSearchString(table[i], "href=", id, ">", " ");
			vara textp = text.split("-");
			if (textp.length()!=2)
				{
				//Log(LOGERR, "%s invalid text %s url: %s", NOOA, text, url);
				}
			int last = textp.length()-1;
			id = cleanhref(textp[last]).trim(); textp.splice(last, 1);
			desc = cleanhref(textp.join("-")).trim();
			double lat = CDATA::GetNum(GetToken(table[i], 1));
			double lng = CDATA::GetNum(GetToken(table[i], 0));
			if (lat==InvalidNUM || lng==InvalidNUM)
				{
				Log(LOGERR, "%s Invalid lat/lng %s url: %s", NOOA, table[i], url);
				continue;
				}
#if 0	// check for consistenct between map and list (both are used for matching)
			 int af=nooa.Find(NOOA+vars(":")+id);
			 if (af>=0)
				{
				CSym &sym = nooa[af];
				double d = Distance(lat, lng, sym.GetNum(SITE_LAT), sym.GetNum(SITE_LNG));
				if (d>MIN100M)
					// location mismatch
					Log(LOGWARN, "%s id %s maps!=list lat/lng %s,%s != %s,%s url: %s", NOOA, id, CData(lat), CData(lng), sym.GetStr(SITE_LAT), sym.GetStr(SITE_LNG), url);
				}
			 else
				{
				// unexpected new ID
				Log(LOGWARN, "%s new id %s from maps \"%s\" url: %s", NOOA, id, text, url);
				}
#endif
			 int n = addsite(nooa, NOOA+vars(":")+id, desc, CData(lat), CData(lng), "HADSMAP");
			 if (n>=0) identifyforecastsite(nooa[n], nooa);
			}
		}
#endif
	nooa.Sort();
}


void getnooakmlsites(CSymList &sites, CSymList *nooa)
{
	CString url, urlp;
	DownloadFile f;
	CString stamp = MkString("%g", GetTime());

	// generic NOOA forecast
	  urlp = "http://water.weather.gov/ahps2/hydrograph.php?gage=";
	  url = "http://www.srh.noaa.gov/rfcexp/readSRAhpsDbf.php?datadefine=both|major|moderate|minor|action|normal|old|no_flooding&left=-180&right=180&top=90&bottom=-90&rand="+stamp;
	  //url = "http://www.srh.noaa.gov/rfcexp/readSRAhpsDbf.php?datadefine=both|major|moderate|minor|action|normal|old|no_flooding&left=-122.20910400390619&right=-114.82629150390619&top=38.07925226444034&bottom=31.54673130511674&rand="+stamp;
	  if ( DownloadRetry(f,  url ) )
		 Log(LOGERR, "%s SaveForecastSites: failed url %s", NOOA, url);
	  else
		{
		vara table(f.memory, "\"Point\"");
		for (int i=1; i<table.length(); ++i)
			{
			CString forecast, id, desc1, desc2, coords;
			GetSearchString(table[i], "\"forecast\"", forecast, "\"", "\"");
			if (forecast=="")
				continue;

			// find matching nooa site
			GetSearchString(table[i], "\"id\"", id, "\"", "\"");
			GetSearchString(table[i], "\"waterbody\"", desc1, "\"", "\"");
			GetSearchString(table[i], "\"location\"", desc2, "\"", "\"");
			GetSearchString(table[i], "\"coordinates\"", coords, "[", "]");
			vara coordsp = vars(coords).split();
			int n = addauxsite(sites, id, desc1+" AT "+desc2, coordsp[1], coordsp[0], "NOOA", urlp+id);
			if (n>=0 && nooa) identifyforecastsite(sites[n], *nooa);
			}
		}

	  // california forecast
	  urlp = "http://www.cnrfc.noaa.gov/graphicalRVF.php?id=";
	  url = "http://www.cnrfc.noaa.gov/data/kml/riverFcst.xml?random="+stamp;
	  if ( DownloadRetry(f,  url ) )
		 Log(LOGERR, "%s SaveForecastSites: failed url %s", "NOOACA", url);
	  else
		{
		vara table(f.memory, "<riverFcst ");
		for (int i=1; i<table.length(); ++i)
			{
			CString forecast, id, desc1, desc2, lat, lng;
			// find matching nooa site
			GetSearchString(table[i], "id=", id, "\"", "\"");
			GetSearchString(table[i], "riverName=", desc1, "\"", "\"");
			GetSearchString(table[i], "stationName=", desc2, "\"", "\"");
			GetSearchString(table[i], "latitude=", lat, "\"", "\"");
			GetSearchString(table[i], "longitude=", lng, "\"", "\"");
			int n = addauxsite(sites, id, desc1+" AT "+desc2, lat, lng, "NOOACA", urlp+id);
			if (n>=0 && nooa) identifyforecastsite(sites[n], *nooa);
			}
		}

	  // pacific northwest forecast
	  urlp = "http://www.nwrfc.noaa.gov/river/station/flowplot/flowplot.cgi?";
	  url = "http://www.nwrfc.noaa.gov/rfc/qdb.php?sw_lat=-90&sw_lng=-180&ne_lat=90&ne_lng=180&rand="+stamp;
	  if ( DownloadRetry(f,  url ) )
		 Log(LOGERR, "%s SaveForecastSites: failed url %s", "NOOANW", url);
	  else
		{
		vara table(f.memory, "{\"lid\"");
		for (int i=1; i<table.length(); ++i)
			{
			CString forecast, id, lat, lng;
			// find matching nooa site
			GetSearchString(table[i], ":", id, "\"", "\"");
			GetSearchString(table[i], "\"lat\"", lat, "\"", "\"");
			GetSearchString(table[i], "\"lng\"", lng, "\"", "\"");
			int n = addauxsite(sites, id, "", lat, lng, "NOOANW", urlp+id);
			if (n>=0 && nooa) identifyforecastsite(sites[n], *nooa);
			}
		}

	// colorado forecast
	  urlp = "http://www.cbrfc.noaa.gov/station/flowplot/flowplot.cgi?";
	  url = "http://www.cbrfc.noaa.gov/gmap/csv/data_river.js";
	  if ( DownloadRetry(f,  url ) )
		 Log(LOGERR, "%s SaveForecastSites: failed url %s", "NOOACO", url);
	  else
		{
		vars ids, desc1s, desc2s, rfss, lats, lngs;
		vars data(f.memory); 
		data.Replace("\n", ""); data.Replace("\r", "");
		GetSearchString(data, "var ids=", ids, "('", "')");
		GetSearchString(data, "var rivers=", desc1s, "('", "')");
		GetSearchString(data, "var names=", desc2s, "('", "')");
		GetSearchString(data, "var rfss=", rfss, "('", "')"); // or segnum
		GetSearchString(data, "var lats=", lats, "('", "')");
		GetSearchString(data, "var lngs=", lngs, "('", "')");
		vara idp = ids.split("','"), desc1p = desc1s.split("','"), desc2p = desc2s.split("','"), rfsp = rfss.split("','");
		vara latp = lats.split("','"), lngp = lngs.split("','");
		while (desc2p.length()<desc1p.length()) desc2p.push("");

		if (idp.length()!=desc1p.length() || idp.length()!=desc2p.length() || idp.length()!=rfsp.length() || idp.length()!=latp.length() || idp.length()!=lngp.length())
			{
				Log(LOGERR, "%s SaveForecastSites: inconsistent kml data id:%d desc1:%d desc2:%d lat:%d lng:%d %s", "NOOACO", 
					idp.length(), desc1p.length(), desc2p.length(), latp.length(), lngp.length(), url);
			for (int i=0; i<idp.length(); ++i)
				Log(LOGINFO, "DUMP:,%s,%s,%s,%s,%s", idp[i], desc1p[i], desc2p[i], latp[i], lngp[i]);
			}
		else
			{
			for (int i=0; i<idp.length(); ++i)
				if (rfsp[i]!='0')
					{
					int n = addauxsite(sites, idp[i], desc1p[i]+ " AT " + desc2p[i], latp[i], lngp[i], "NOOACO", urlp+idp[i]);
					if (n>=0 && nooa) identifyforecastsite(sites[n], *nooa);
					}
			}
		}

	  /*
	  urlp = "http://www.cbrfc.noaa.gov/station/flowplot/flowplot.cgi?";
	  url = "http://www.cbrfc.noaa.gov/gmap/list/list.php?search=&point=forecast&psv=on&rand="+stamp;
	  if ( DownloadRetry(f,  url ) )
		 Log(LOGERR, "%s SaveForecastSites: failed url %s", "NOOACO", url);
	  else
		{
		int count = 0, err = 0;
		vara table(f.memory, "\n");
		for (int i=2; i<table.length(); ++i)
			{
			CString forecast, id;
			// find matching nooa site
			id = vars(GetToken(table[i],0,'|')).trim();
			id.MakeUpper();
			int n=identifyforecastsite(urlp, id, "", "", "", nooa, TRUE);
			if (n<0)
				{
				++err;
				errf.fputstr( urlp+id);
				continue;
				}
			++count;
			}
		Log(LOGINFO, "NOOACO forecast sites %d OK %d ERR", count, err);
		}
	*/
}


int checkid(const char *id, CSym &nooasym)
{
	vara namep = vars(id).split(":");
/* 
	// dynamic check for USGS may report obsolete sites
	if (strcmp(namep[0], USGS)==0)
		{
		// special case for USGS
		DownloadFile f;
		CString url = CString("https://waterservices.usgs.gov/nwis/site/?format=rdb&sites=")+namep[1];
		if ( DownloadRetry(f,  url ) )
			{
			Log(LOGERR, "%s addforecastsites: failed url %s", NOOA, url);
			return "";
			}
		  vara table(f.memory, "\n");

		  // skip header
		  for (int i=0; i<table.length() && table[i][0]=='#'; ++i);
		  for (i=i+1; i<table.length(); ++i)
			{
			vara line = table[i].split("\t");
			if (line[1].trim()==namep[1])
				return line[2].trim()+","+line[4].trim()+","+line[5].trim();
			}
		
		Log(LOGERR, "%s addforecastsite: unknown USGS linked id %s", NOOA, id);
		return "";
		}
*/

	//CSymList list;
	//list.Load(filename(namep[0]));
	//int n = list.Find(id);

	int n = sites.Find(id);
	if (n<0)
		{
		Log(LOGERR, "%s checkid: invalid linked id %s", NOOA, id);
		return -1;
		}

	CString &data = sites[n].data;

	// check distance is reasonable
	double lat1 = nooasym.GetNum(SITE_LAT);
	double lng1 = nooasym.GetNum(SITE_LNG);
	double lat2 = CDATA::GetNum(GetToken(data, SITE_LAT));
	double lng2 = CDATA::GetNum(GetToken(data, SITE_LNG));
	if (lat1==InvalidNUM || lat2==InvalidNUM || lng1==InvalidNUM  || lng2==InvalidNUM)
		{
		Log(LOGERR, "%s checkid: invalid lat/lng for \n%s\n%s,%s\n", NOOA, nooasym.Line(), id, data);
		return -1;
		}

	double dist = Distance(lat1, lng1, lat2, lng2);
	if (dist<1) dist = 0;
	if (dist>MIN5KM) // 5km
		{
		Log(LOGERR, "%s checkid: rejected for too far (@%skm)\n%s\n%s,%s\n", NOOA, CData(dist), nooasym.Line(), id, data);
		return -1;
		}

	Log(LOGINFO, "%s NEW LINK: %s=%s (@%skm)\n%s\n%s,%s\n", NOOA, nooasym.id, id, CData(dist), nooasym.Line(), id, data);
	return 1;
}



class counters {
	//CFILE errf;
	int mn;
	CString mid[10];
	int mcnt[10], merr[10];

	public:
	counters(void)
	{
		mn = 0;
	}

	void add(const char *mode, int res, const char *url)
	{
	for (int mi=0; mi<mn && strcmp(mid[mi],mode)!=0; ++mi);
	if (mi==mn) 
		{
		// setup new mode
		//if (mi==0) 	
		//	errf.fopen("NOOAERR.log", CFILE::MWRITE);
		mid[mi] = mode;
		mcnt[mi] = merr[mi] = 0;
		++mn;
		}
	++mcnt[mi];
	if (res<0)
		{
		++merr[mi];
		//errf.fputstr( url );
		}
	}

	void report(void)
	{
	for (int mi=0; mi<mn; ++mi)
		Log(LOGINFO, "%s %d ids (%d unlinked)", mid[mi], mcnt[mi], merr[mi]);
	//Log(LOGINFO, "TOTAL %d ids (%d unliked ids in NOOAERR.htm)", tcnt, terr);
	}

} cnt;



int INVESTIGATE = -1;


int identifyforecastsite(CSym &sym, CSymList &nooa)
{
	// try to find links on NOOA web page for id
	// -1 to try always, 1 to try only when no match

	CString id = GetToken(sym.id, 1, ':');
	if (id=="") id = sym.id;

	vara data = vars(sym.data).split();
	CString desc = data[SITE_DESC];
	CString lat = data[SITE_LAT];
	CString lng = data[SITE_LNG];
	CString url = GetToken(sym.data, SITE_EXTRA);

	vara matchlist;
	CString nooaid = NOOA+vars(":")+id;
	int n = nooa.Find(nooaid);
	if (n>=0) matchlist = vars(nooa[n].GetStr(SITE_EQUIV)).split(";");

	// find based on proximity and id similarity
	double vlat = CDATA::GetNum(lat);
	double vlng = CDATA::GetNum(lng);
	if (vlat!=InvalidNUM && vlng!=InvalidNUM && vlat>0 && vlng<0)
		{
		CIntArrayList sites; 
		CDoubleArrayList dists;
		// get list of matches
		int mini = -1;
		double mindist = 1e10;
		double mind = 2.0; // expand grid 2 degree
		register int imax = sortlist.GetSize();
		register double minlat = vlat-mind, minlng = vlng-mind, maxlat = vlat+mind, maxlng = vlng+mind;
		for (register int i = 0; i<imax; ++i)
			{
			register tsite *site = &sortlist[i];
			if (site->lat>=minlat && site->lat<=maxlat)
			   if (site->lng>=minlng && site->lng<=maxlng)
				{
				sites.AddTail(i);
				double dist = Distance(site->lat, site->lng, vlat, vlng);
				dists.AddTail(dist);
				if (dist<mindist)
					mindist = dist, mini = i;
				}
			}

		// process list
		for (int i=0; i<sites.GetSize(); ++i)
			{		    
				const char *match = NULL;
				tsite *site = &sortlist[sites[i]];
				const char *sid = site->csite->id;
				double dist = dists[i];

				// matches
				if (strncmp(sid, id, 3)==0 && dist<MIN5KM) // within 5km
					match = "ID";				
				else if (dist<MIN100M) // within 100m
					match = "VCLOSE";
				else if (dist<MIN5KM) // within 5km 
					{
					int dmatch = CompareDescription(simplify(desc), simplify(site->csite->Info(SITE_DESC)));
					if (dmatch>0 && dist<MIN5KM) // match description within 5km
						match = "DESC";				
					else 
					if (dmatch==0 && dist<MIN1KM) // river match within 1km
						match = "RNAME";
					}
				if (!match)
					continue;

				// found match!
				CSite *&csite = site->csite;
				if (matchlist.indexOf(csite->fid)>=0)
					continue;

				// new match
				matchlist.push(csite->fid);
				//ASSERT(strcmp(nooaid,"NOOA:LWCN2")!=0);
				CString siteurl(csite->prv->urls[0]); siteurl.Replace("%id", sid);
				Log(LOGINFO, "%s NEW %s MATCH: %s=%s (@%skm)\n%s\n%s\n", NOOA, match, nooaid, csite->fid, CData(dist),
					MkString("%s, %s, %s, %s, %s", nooaid, desc, lat, lng, url),
					MkString("%s, %s", csite->Info(), siteurl));
			}
		}
	else
		{
		Log(LOGERR, "%s identifyforecastsite: invalid Lat/Lng %s, %s, %s, %s, %s", NOOA, nooaid, desc, lat, lng, url);
		}

	//vara modes = data[SITE_EQUIV].split(";");
	if (INVESTIGATE>0 || (matchlist.length()==0 && INVESTIGATE==0))
	  //if (modes.indexOf("NOOACA")>=0 || modes.indexOf("NOOACO")>=0 || modes.indexOf("NOOA")>=0)
	  if (url!="")
		{
		// search further
		DownloadFile f;
		if ( DownloadRetry(f,  url ) )
			{
			Log(LOGERR, "%s identifyforecastsite: failed url %s", NOOA, url);
			return -1;
			}

		vars data(f.memory);

		// try to identify based on links
		struct { const char *url, *s, *e, *name; } idlist[] = { 
			{"http://cdec.water.ca.gov/cgi-progs/queryF?s", "=", "\"", "CDEC" },
			{"http://apps.wrd.state.or.us/apps/sw/hydro_near_real_time/display_hydro_graph.aspx?station_nbr", "=", "\"", "OWRD" },
			{"/nwis/uv/?site_no", "=", "\"", "USGS" },
			NULL
		};
		for (int i=0; idlist[i].url!=NULL; ++i)
			{
			CString fid;
			GetSearchString(data, idlist[i].url, fid, idlist[i].s, idlist[i].e);
			if (fid!="")
				{
				fid.MakeUpper();
				fid = idlist[i].name+vars(":")+fid;
				if (matchlist.indexOf(fid)<0)
					{
					// check id is known
					if (checkid(fid, sym)<0)
						continue;
					matchlist.push(fid);
					}
				}
			}
		}


	// add matching lists
	addsite(nooa, nooaid, sym.GetStr(SITE_DESC), sym.GetStr(SITE_LAT), sym.GetStr(SITE_LNG), sym.GetStr(SITE_MODE), url, matchlist.join(";"));
	int res = matchlist.length()==0 ? -1 : 1;


	// keep counters and link errors
	vara cntmodes = vars(sym.GetStr(SITE_MODE)).split(";");
	cnt.add(cntmodes[ cntmodes.length()-1 ], res, url);

	return res;
}




void WaterflowSaveForecastSites(void)
{
	// for rectangle search
	WaterflowLoadSites(TRUE);

	CSymList nooa;
	LoadSite(NOOA, nooa);
#if 1
	getnooasites(nooa);
#endif
#if 1
	// find forecast url for nooa sites
	CSymList nooakml;
	getnooakmlsites(nooakml, &nooa);
	//nooakml.Save(filename(NOOAKML));
	cnt.report();
#endif
	SaveSite(NOOA, nooa);

	CSymList nooamap, nooanomap;
	// re-build NOOA map every time
	//LoadSite(NOOAMAP, nooamap);
	for (int i=0; i<nooa.GetSize(); ++i)
	  if (!nooa[i].GetStr( SITE_EXTRA ).IsEmpty())
		{
		vars mode = nooa[i].GetStr( SITE_EQUIV );
		if (mode.IsEmpty())
			{
			nooanomap.Add( nooa[i] );
			continue;
			}
		if (mode[0]=='-')
			continue; // disabled
		vara list = mode.split(";");
		for (int l=0; l<list.length(); ++l)
			{			
			int n = nooamap.Find(list[l]);
			if (n<0)
				{
				// new
				n = nooamap.Add( CSym(list[l], nooa[i].data) );
				continue;
				}
			// existing
			if (nooamap[n].data != nooa[i].data)
				{
				int f = sites.Find(list[l]);
				if (f<0)
					{
					Log(LOGERR, "%s duplicated for %s, but can't find it", NOOAMAP, list[l]);
					continue;
					}
				CSym &sym = sites[f];
				double nooamapd = Distance(sym.GetNum(SITE_LAT), sym.GetNum(SITE_LNG), nooamap[n].GetNum(SITE_LAT), nooamap[n].GetNum(SITE_LNG));
				double nooad = Distance(sym.GetNum(SITE_LAT), sym.GetNum(SITE_LNG), nooa[i].GetNum(SITE_LAT), nooa[i].GetNum(SITE_LNG));
				if (logging)
					Log(LOGINFO, "%s duplicated for %s  %g<%g?:\n%s\n%s\n", NOOAMAP, list[l], nooamapd, nooad, 
					(nooad<nooamapd ? " " : ">") + nooamap[n].data, 
					(nooad<nooamapd ? ">" : " ") + nooa[i].data);
				if (nooad<nooamapd)
					nooamap[n].data = nooa[i].data;
				}			
			}
		}

	// save results
	SaveSite(NOOAMAP, nooamap, FALSE);
	SaveSite(INVERSE(NOOAMAP), nooanomap, FALSE);

	WaterflowUnloadSites();
}


// Only Active, real time data, flow

tprovider *getprovider(const char *str)
{
	const char *strdiv = strchr(str, ':');
	if (!strdiv) 
		return NULL;
	int strlen = strdiv-str;
	for (int i=0; providers[i].name!=NULL; ++i)
		if (strnicmp(str, providers[i].name, strlen)==0)
			return &providers[i];
	return NULL;
}

void LoadWaterflow(CSymList &list, int active)
{
	if (active)
		{
		for (int i=0; providers[i].name!=NULL; ++i)
			list.Load(filename( providers[i].name ));
		}	
	else
		{
		CSymList filelist;
		const char *exts[] = { "CSV", NULL };
		GetFileList(filelist, basedir, exts);
		for (int i=0; i<filelist.GetSize(); ++i)
			list.Load(filelist[i].id);
		}
}


void WaterflowSaveSites(const char *name)
{
	CreateDirectory(basedir, NULL);
	// get sites
	for (int i=0; providers[i].name!=NULL; ++i)
	  if (providers[i].getsites!=NULL)
		{
		if (name && *name && strcmp(name,providers[i].name)!=0)
			continue;
		CSymList sitelist;
		LoadSite( providers[i].name, sitelist);
		
		CreateDirectory(basedir+"\\"+providers[i].name, NULL);
		if (!providers[i].getsites( providers[i].name, sitelist, TRUE ) || sitelist.GetSize()==0)
			{
			Log(LOGERR, "ERROR: %s getsites MALFUNCTIONING (failed)", providers[i].name);
			continue;
			}

		sitelist.header = siteshdr;
		SaveSite(providers[i].name, sitelist);
		}
}

void WaterflowTestSites(const char *name)
{
	ShellExecute(NULL, "open", "http://ropewiki.com/Waterflow?location=Parker+Canyon&debug=test", NULL, NULL, SW_SHOWNORMAL);
}


int FindAprox(tsitevalarray &data, double val)
	{
		const int nline = data.GetSize();
		if (!nline) 
			return -1;

		//int &nline = cfg->nline;

		// out of range
		if (val>data[0].val)
			return 0;
		if (data[nline-1].val>val)
			return nline-1; 

		// in range
		register int high, i, low;
		for ( low=(-1), high=nline;  high-low > 1;  )
			  {
			  i = (high+low) / 2;
			  if (val == data[i].val)
				  return i;
			  if ( val >= data[i].val )  high = i;
				   else             low  = i;
			  }
		 return low;
	}



typedef CSite *CSitePtr;
CSitePtr *csites;

#define MAXLASTFIND 4
int lastfindi = MAXLASTFIND;
struct {
	vars id;
	CSite *site;
} lastfind[4];

CSite *GetSite(const char *id)
{
	// find in cache
	for (register int i=lastfindi-1, ilast=lastfindi-MAXLASTFIND; i>=ilast; --i)
		{
		register int ai = i % MAXLASTFIND;
		if (id == lastfind[ai].id)
			return lastfind[ai].site;
		}

	// find for real
	int f = findsite(id);
	if (f<0)
		{
		Log(LOGERR, "invalid site id %s", id);
		return NULL;
		}

	int ai = (lastfindi++) % MAXLASTFIND;
	lastfind[ai].id = id;
	return lastfind[ai].site = csites[f];
}

CSite *MatchSite(double lat, double lng, CSite *osite)
{
	CSite *site = NULL;
	tprovider *prv = NULL;
	tsitelist csitelist;
	double d = MIN100MDEG;
	RectSites(sortlist, csitelist, lat-d, lng-d, lat+d, lng+d);
	for (int c=0; c<csitelist.GetSize(); ++c)
		if (Distance(lat, lng, csitelist[c].lat, csitelist[c].lng)<MIN100M)
			if (csitelist[c].csite!=osite)
			  if (!prv || csitelist[c].csite->prv<prv) // prioritize providers in order
				site = csitelist[c].csite;
	return site;
}


void LoadList( const char *file, CSymList &nooamap, tsitelist &nooamaplist)
{
	if (file!=NULL)
		nooamap.Load( filename(file) );
	nooamaplist.SetSize(nooamap.GetSize());
	for (int i=0; i<nooamap.GetSize(); ++i)
		{
		tsite &s = nooamaplist[i] = tsite(&nooamap[i]);
		if (s.lat==InvalidNUM || s.lng==InvalidNUM)
			{
			Log(LOGINFO, "invalid lat/lng for %s", s.csym->Line());
			continue;
			}
		}
}


void WaterflowLoadSites(int loadusgs)
{
	//void test(void);
	//test();

	// load providers
	for (int i=0; providers[i].name!=NULL; ++i)
		if (loadusgs || providers[i].getdate!=NULL)
			LoadSite(providers[i].name, sites);

	// set up sites
	sites.Sort();
	csites = new CSitePtr[sites.GetSize()] ;
	for (int i=0; i<sites.GetSize(); ++i)
		csites[i] = new CSite(&sites[i]);

	// find group ids
	for (int i=0; i<sites.GetSize(); ++i)
		if (csites[i]->prv->calcgroup)
			for (int j=0; j<sites.GetSize(); ++j)
			  if (csites[i]->prv==csites[j]->prv)
				if (csites[i]->gid==csites[j]->gid)
					csites[i]->group.AddTail(csites[j]);

	// load sort
	sortlist.SetSize(sites.GetSize());
	for (i=0; i<sites.GetSize(); ++i)
		{
		tsite &s = sortlist[i] = tsite( csites[i] );
		if (s.lat==InvalidNUM || s.lng==InvalidNUM)
			{
			Log(LOGINFO, "Invalid lat/lng for %s", s.csite->Info());
			continue;
			}
		//lat.AddTail(tsiteval(s.lat, &s) );
		//lng.AddTail(tsiteval(s.lng, &s) );
		}

	//lat.Sort();
	//lng.Sort();

	// load aux lists
	LoadList(NOOAMAP, nooamap[0], nooamaplist[0]);
	LoadList(INVERSE(NOOAMAP), nooamap[1], nooamaplist[1]);
	LoadList(USGS, usgs[0], usgslist[0]);
	LoadList(INVERSE(USGS), usgs[1], usgslist[1]);
}

void WaterflowFlush(void)
{
	double minutes = 3;
	double time = GetTime()-minutes/24/60;
	for (int i=0; i<sites.GetSize(); ++i)
		if (csites[i]->Modified()<time)
			if (csites[i]->Save())
				Sleep(1*1000);
}

void WaterflowUnloadSites(void)
{
	// destroy sites
	for (int i=0; i<sites.GetSize(); ++i)
		delete csites[i];
	delete[] csites;
}


void TestSites(tsitelist &list)
{
		for (register int i = 0; providers[i].name!=NULL; ++i)
			{
			vara ids = vars(providers[i].testsite).split();
			for (register int j=0; j<ids.length(); ++j)
				{
				vars id = providers[i].name+vars(":")+ids[j];
				CSite *csite = GetSite(id);
				if (!csite) 
					{
					Log(LOGERR, "invalid site id %s", id);
					continue;
					}
				list.AddTail(tsite(csite, 0, 0));
				}
			}
}

#define isodate(x) CDate(x)
vars monthname(const char  *num)
{
	const vars name[] = {"NULL", "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC" };
	return name[atoi(num)];
}

CString KMLGraphs(CSym *sym)
{
  CString out;
  tprovider *prv = getprovider(sym->id);
  if (!prv) 
	  return out;

  //ASSERT( sym->id != "COERG:ISBQ");

  vars url = "";
  struct {
	  vara urls;
  } site;

  site.urls.push("0");
  for (int i=1; i<=3 && prv->urls[i]!=NULL; ++i)
	  site.urls.push(prv->urls[i]);
  vars siteid = sym->id, mode = sym->GetStr(SITE_MODE), stamp;
  int today  = TRUE;

  vara links;
  const char *nlabels[] = { "7d", "30d", "1y" };
  const int ndays[] = { 7, 30, 356 };
  for (int d=0; d<3; ++d)
  {
	  vars url;
	  int days = ndays[d];
  // jscript ----
  
	  vars id = siteid.split(":")[1];

	  var u = 1;
	  if (days>7 && site.urls.length()>2) u = 2;
	  if (days>30 && site.urls.length()>3) u = 3;
	  double d2 = GetTime(), d1 = d2 -days;
	  url = site.urls[u].split("%id").join(id).split("%lid").join(id.lower());
	  vara modes = mode.split(";");
	  if (modes.length()>0)
		 modes = modes[0].split("=");
	  if (modes.length()>1)
		url = url.split("%modeb").join(modes[1]);
	  if (modes.length()>0)
	   url = url.split("%mode").join(modes[0]);      	
	  vars date = isodate(d1);
		if (url.indexOf("%YYYY")<0 && !today)
			{
			// disable link
			url = "";
			}
			else
			{
			// enable popup link
			url = url.replace("%YYYY1", date.substr(0,4)).replace("%MM1", date.substr(5,2)).replace("%DD1", date.substr(8,2)).replace("%MT1", monthname(date.substr(5,2))).replace("%stamp", stamp);
			date = vars(isodate(d2));
			url = url.replace("%YYYY2", date.substr(0,4)).replace("%MM2", date.substr(5,2)).replace("%DD2", date.substr(8,2)).replace("%MT2", monthname(date.substr(5,2))).replace("%stamp", stamp);
			//url = "javascript:window.open('"+url+"', '_blank');"; //popupwin(url, site.pwidth, site.pheight);
			} 	     	

		if (url!="")
			links.push( alink(url, nlabels[d]) );
	}
	out += "<div>Charts: " + links.join("-") + "</div>";
	return out;
}




CString KMLFolder(tsitelist &sitelist, const char *name, const char *rwicon, const char *linkref, int load = FALSE, const char *link = NULL)
{
		CString out;
		if (link)
		{
			out += KMLFolderStart(name, rwicon, TRUE, FALSE);		
			out += KMLLink(name, NULL, MkString("http://%s/rwr/?rwz=%s;%s;%s,0,0,0,%s", server, name, rwicon, url_encode(linkref), link));
			out += KMLFolderEnd();
		}
		else
		{
			CSymList symlist;
			if (load>0)
				LoadList(name, symlist, sitelist);

			// Forecast sites
			out += KMLMarkerStyle( rwicon, NULL, 0.8);
			for (int l=0; l<sitelist.GetSize(); ++l)
				{
				CSym *csym = sitelist[l].csym;
				vars link = vars(linkref ? linkref : csym->GetStr(SITE_EXTRA)).split(";")[0]; 
				link.Replace("%id", GetToken(csym->id, 1, ':'));
				vars desc = csym->GetStr(SITE_DESC);

				desc += KMLGraphs(csym);
				desc += MkString("<br><a href=\"%s\">More information</a>", link);
				out += KMLMarker( rwicon, csym->GetNum(SITE_LAT),  csym->GetNum(SITE_LNG), csym->id, CDATAS+desc+CDATAE);
				}		
		}

		return out;
}

void WaterflowMarkerList(CKMLOut &out, const char *name, const char *rwicon, const char *linkref, LLRect *bbox)
{
	CSymList symlist;
	tsitelist sitelist, csitelist;
	LoadList(name, symlist, sitelist);
	RectSites(sitelist, csitelist, bbox->lat1, bbox->lng1, bbox->lat2, bbox->lng2);
	out += KMLFolder(csitelist, name, rwicon, linkref, -1, NULL);
}


int WaterflowRect(const char *q, FCGX_Stream *stream)
{
	vara rect = vars(q).split();
	for (int i=0; providers[i].name!=NULL; ++i)
		providers[i].cnt = 0;

	// get list
	tsitelist csitelist, usitelist;
	if (testing)
		TestSites(csitelist);
	else
		{
		LLRect bbox(CDATA::GetNum(rect[0]), CDATA::GetNum(rect[1]), CDATA::GetNum(rect[2]), CDATA::GetNum(rect[3]));
		RectSites(sortlist, csitelist, bbox.lat1, bbox.lng1, bbox.lat2, bbox.lng2);
		RectSites(usgslist[0], usitelist, bbox.lat1, bbox.lng1, bbox.lat2, bbox.lng2);
		}

	// JSON output
	FCGX_PutS("{\"list\":[", stream);
	
	int usgscnt = 0, cnt = 0;
	for (register int i=0; i<usitelist.GetSize(); ++i)
		{
		if (cnt>0) FCGX_PutS(",", stream);
		FCGX_PutS("\"", stream);
		FCGX_PutS(usitelist[i].csym->Line(), stream);
		FCGX_PutS("\"", stream);
		++usgscnt, ++cnt;
		}

	for (register int i=0; i<csitelist.GetSize(); ++i)
		{
		if (cnt>0) FCGX_PutS(",", stream);
		FCGX_PutS("\"", stream);
		FCGX_PutS(csitelist[i].csite->Info(), stream);
		FCGX_PutS("\"", stream);
		++csitelist[i].csite->prv->cnt, ++cnt;
		}

	// json output url list
	FCGX_PutS("],\r\n\"urllist\":[\r\n", stream);

	vara prov;
	cnt = 0;
	for (int i=0; providers[i].name!=NULL; ++i)
		if (providers[i].cnt>0)
		{
		if (cnt>0) FCGX_PutS(",\r\n", stream);
		FCGX_PutS(MkString("[\"%s\",\"%s\"", providers[i].name, providers[i].cfs), stream);
		for (int u=0; u<sizeof(providers[i].urls)/sizeof(providers[i].urls[0]); ++u)
			if (providers[i].urls[u]!=NULL)
				FCGX_PutS(MkString(",\"%s\"", providers[i].urls[u]), stream);
		FCGX_PutS(MkString("]", providers[i].name), stream);
		cnt += providers[i].cnt;
		prov.push(MkString("%s:%d",providers[i].name,providers[i].cnt));
		}

	FCGX_PutS("]}\r\n", stream);

	Log(LOGINFO, "Found %d USGS + %d [%s] sites for %s", usgscnt, csitelist.GetSize(), prov.join(" "), rect.join());
	return TRUE;
}



int WaterflowForecastRect(const char *q, const char *namestr, FCGX_Stream *stream)
{
	vara rect = vars(q).split();
	register int namelen = 0;
	register const char *name = namestr;
	if (name!=NULL && *name == 0)
		name = NULL;
	if (name!=NULL)
		namelen = strlen(name);


	tsitelist csitelist;
	RectSites(nooamaplist[0], csitelist, CDATA::GetNum(rect[0]), CDATA::GetNum(rect[1]), CDATA::GetNum(rect[2]), CDATA::GetNum(rect[3]));

	// JSON output
	register int cnt = 0;
	FCGX_PutS("{\"list\":[", stream);
	for (register int i=0; i<csitelist.GetSize(); ++i)
		{
		if (name!=NULL)
			if (strncmp(name, csitelist[i].csym->id, namelen)!=0)
				continue;
		if (cnt>0) FCGX_PutS(",", stream);
		FCGX_PutS("\"", stream);
		FCGX_PutS(csitelist[i].csym->Line(), stream);
		FCGX_PutS("\"", stream);
		++cnt;
		}

	FCGX_PutS("]}\r\n", stream);
	return cnt;
}




#define USGSSITEID "https://waterdata.usgs.gov/nwis/uv?site_no=%id"
#define USGSICON "mu"
#define INVUSGSICON "mu-"
#define NOOASITEID "http://water.weather.gov/ahps2/hydrograph.php?gage=%id"
#define INVNOOAMAPICON "mn-"
#define NOOAMAPICON "mn"
#define PRVICON "m%d"  //"http://maps.google.com/mapfiles/kml/paddle/%d.png"


int WaterflowKMLRect(const char *q, const char *allusgs, FCGX_Stream *stream)
{
	vara rect = vars(q).split();

	tsitelist csitelist, nooamapsitelist[2], usgssitelist[2];
	RectSites(sortlist, csitelist, CDATA::GetNum(rect[0]), CDATA::GetNum(rect[1]), CDATA::GetNum(rect[2]), CDATA::GetNum(rect[3]));
	RectSites(nooamaplist[0], nooamapsitelist[0], CDATA::GetNum(rect[0]), CDATA::GetNum(rect[1]), CDATA::GetNum(rect[2]), CDATA::GetNum(rect[3]));
	RectSites(nooamaplist[1], nooamapsitelist[1], CDATA::GetNum(rect[0]), CDATA::GetNum(rect[1]), CDATA::GetNum(rect[2]), CDATA::GetNum(rect[3]));
	
#if 1	
	RectSites(usgslist[0], usgssitelist[0], CDATA::GetNum(rect[0]), CDATA::GetNum(rect[1]), CDATA::GetNum(rect[2]), CDATA::GetNum(rect[3]));
	RectSites(usgslist[1], usgssitelist[1], CDATA::GetNum(rect[0]), CDATA::GetNum(rect[1]), CDATA::GetNum(rect[2]), CDATA::GetNum(rect[3]));
#else
	// does not work for big areas
	CSymList rtusgslist, oldusgslist;
	CString select = "&bBox="+CData(CDATA::GetNum(rect[1]))+","+CData(CDATA::GetNum(rect[0]))+","+CData(CDATA::GetNum(rect[3]))+","+CData(CDATA::GetNum(rect[2]));
	USGS_getsites(f, rtusgslist, select + "&siteStatus=active&hasDataTypeCd=iv");
	USGS_getsites(f, oldusgslist, select + MkString("&period=P%dD&outputDataTypeCd=dv", 5*365));
	rtusgslist.Sort();
	for (int i=oldusgslist.GetSize()-1; i>=0; --i)
		if (rtusgslist.Find(oldusgslist[i].id)<0)
			oldusgssitelist.AddTail( tsite(&oldusgslist[i], 0, 0) );
	LoadList( NULL, rtusgslist, usgssitelist);
#endif

	FCGX_PutS(KMLStart(NULL), stream);

	//vars icon = "<Style><IconStyle><Icon><href>http://maps.google.com/mapfiles/kml/paddle/1.png</href></Icon></IconStyle></Style>";
	//vars icon = "<Style><IconStyle><hotSpot x=\"0.5\" y=\"0.5\" xunits=\"fraction\" yunits=\"fraction\"/><Icon><href>http://sites.google.com/site/rwicons/bg%s_%s.png</href></Icon></IconStyle></Style>";
	
	FCGX_PutS( KMLFolder(usgssitelist[1], INVERSE(USGS), INVUSGSICON, USGSSITEID), stream);
	FCGX_PutS( KMLFolder(nooamapsitelist[1], INVERSE(NOOAMAP), INVNOOAMAPICON, NOOASITEID), stream);
	//FCGX_PutS( KMLFolder(nooamapsitelist[0], NOOAMAP, NOOAMAPICON, NULL), stream);
	FCGX_PutS( KMLFolder(usgssitelist[0], USGS, USGSICON, USGSSITEID), stream);

	// RW sites
	for (int i=0; providers[i].name!=NULL; ++i)
	  if (providers[i].getdate)
		{
		tsitelist list;
		for (int l=0; l<csitelist.GetSize(); ++l)
		 if (csitelist[l].csite->prv->name == providers[i].name)
			list.AddTail(csitelist[l].csite->sym);
		//CString si = MkString("%d", i); if (si.GetLength()<2) si="0"+si;
		FCGX_PutS( KMLFolder(list, providers[i].name, MkString(PRVICON, i), providers[i].urls[0]), stream);
		}


	FCGX_PutS( KMLEnd(), stream);
	return 0;
}


void WaterflowSaveKMLSites(	CKMLOut &out, const char *link)
{
	tsitelist list;
	//KMLFolder(list, vars(USGS)+"-OLD", INVUSGSICON, USGSSITEID, TRUE);
	out += KMLFolder(list, INVERSE(USGS), INVUSGSICON, USGSSITEID, TRUE, link); 
	out += KMLFolder(list, INVERSE(NOOAMAP), INVNOOAMAPICON, NOOASITEID, TRUE, link); 
	//KMLFolder(list, NOOAMAP, NOOAMAPICON, NULL, TRUE); link.push(NOOAMAP);
	out += KMLFolder(list, USGS, USGSICON, USGSSITEID, TRUE, link); 

	// RW sites
	for (int i=0; providers[i].name!=NULL; ++i)
	  if (providers[i].getdate)
		{
		//CString si = MkString("%d", i); if (si.GetLength()<2) si="0"+si;
		out += KMLFolder(list, providers[i].name, MkString(PRVICON, i), providers[i].urls[0], TRUE, link);
		}
}




#if 1

int WaterflowDates(const char *q, const char *o, FCGX_Stream *stream)
	{
	vars id = vars(q).split("&")[0];
	vara dates = vars(o).split("&")[0].split();
	CSite *site = GetSite(id);
	if (!site) 
		{
		Log(LOGERR, "invalid site id %s", id);
		return FALSE;
		}

	for (int d=0; d<dates.length(); ++d)
		{
		//date = date.substr(0, 10);
		//Sleep(5000);
		vars &date = dates[d];

		// try to find
		CString line = site->FindDate(date);	
		if (!line.IsEmpty() && !testing)
			{
			// refresh every 30 days 
			double v = GetVal( line );
			if (v>0)
				{
				/*
				// cached value is good
				if (GetToken(line,1)=="" && GetToken(line,2)=="")
					{
					// invalid!!!
					Log(LOGERR, "INCONSISTENCY: FORCED GetDate for %s %s %gT", id, date, v);
					}
				else
				*/
					{
					// catched line
					date = vars(line);
					site->GetFlow(date);
					continue;
					}
				}
			else
				{
				/*
				if (!istoday(date) && GetToken(line,1)!="")
					Log(LOGERR, "INCONSISTENCY: refresh of not today %s %s %gT: %s", id, date, v, line);
				*/
				}
			}

		// prefetch all but only get 1 date at a time
		if (d>0) 			
			{
			dates.splice(d--, 1);
			continue;
			}

		// get new value (downloaD)
		VRESNULL;
		site->GetDate(date, vres);
		site->SetDate(date, vres);
		site->GetFlow(date);
		}

	FCGX_PutS("{\"list\":[", stream);
	for (int d=0; d<dates.length(); ++d)
		{
		if (d>0) FCGX_PutS(",", stream);
		FCGX_PutS("\"", stream);
		FCGX_PutS(dates[d], stream);
		FCGX_PutS("\"", stream);
		}
	FCGX_PutS("]}\r\n", stream);

	return TRUE;
	}


#else

/*
#define MAXDOWNTHREADS 5

class CWaterflowDates;
typedef struct {
	CWaterflowDates *that;
	vars *date;
	double val;
	int f;
} threaddata;



typedef CArrayList <threaddata> CThreadArrayList;

class CWaterflowDates {

	int i, loadsave, numthreads;
	CLineList data;
	vars id;
public:
	CWaterflowDates(vars &_id, vara &dates, FCGX_Stream *stream)
	{
	id = _id;
	CString fname = filename(id);
	data.Header() = "DATE,VAL,VDATE";

	// load historicals
	CCFILE.Lock();
	data.Load(fname);
	CCFILE.Unlock();

	loadsave = 0;
	numthreads = 0;
	i = getprovider(id);	
	if (i<0)
		{
		Log(LOGERR, "Provider not found for %s", id);
		return;
		}


	int dlen = dates.length();
	int threadlistn = 0;
	threaddata *threadlist = new threaddata[dlen];
	for (int d=dlen-1; d>=0; --d)
		{
		//date = date.substr(0, 10);
		//Sleep(5000);
		vars &date = dates[d];

		// try to find
		int f = data.Find(date.substr(0,10));
		if (f>=0)
			{
			// refresh every 30 days 
			// if invalid refresh every 7 day
			double vald = CDATA::GetNum( GetToken(data[f], 2) );
			if (GetTime()<vald)
				{
				// cached value is good
				date = data[f];
				continue;
				}
			}

		// run multiple dates as threads
		threaddata &th = threadlist[threadlistn++];
		th.that = this; th.date = &date; th.f = f; th.val = InvalidURL;
		//threadlist.AddTail(th);
		++numthreads;
		if (d==0)
			// last one (or only one) if fg
			threaddate(&th);
		else
			{
			AfxBeginThread( threaddate, &th );
			Sleep(500); // give some space between calls
			while (numthreads>MAXDOWNTHREADS) 
				WaitThread();
			}
		}

	// wait for threads
	while (numthreads>0)
		WaitThread();

	// process results
	for (int i=0; i<threadlistn; ++i)
		{
		// double check process finished
		if (threadlist[i].that!=NULL)
			{
			Log(LOGERR, "Threads still running for %s!", id);
			while (threadlist[i].that!=NULL)
				WaitThread();
			Log(LOGERR, "Waiting for thread for %s completed!", id);
			}

		int f = threadlist[i].f;
		double val = threadlist[i].val;
		vars &date = *threadlist[i].date;
		// if invalid, reuse old data
		if (val<0 && f>=0)
			val = CDATA::GetNum(GetToken(data[f], 1));

		date = SetDateVal(date, val);

		if (f<0)
			data.Add(date);
		else
			data.Set(f, date);
		++loadsave;
		}
	delete [] threadlist;
	
	// save updated data
	if (loadsave>0)
		{
		CCFILE.Lock();
		// update with new 
		CLineList file;
		file.Load(fname);
		file.Merge(data,0);
		file.Save(fname);
		CCFILE.Unlock();
		}

	FCGX_PutS("{\"list\":[", stream);
	for (int d=0; d<dlen; ++d)
		{
		if (d>0) FCGX_PutS(",", stream);
		FCGX_PutS("\"", stream);
		FCGX_PutS(dates[d], stream);
		FCGX_PutS("\"", stream);
		}
	FCGX_PutS("]}\r\n", stream);
	}


	double GetDate(vars &date)
	{
		// get value, retry bad urls after 5s
		double val = providers[i].getdate(id, date);
		if (val==InvalidURL)
			{
			Sleep(3000); // wait 3s and retry
			val = providers[i].getdate(id, date);
			}

	#ifdef DEBUG
		Log(LOGINFO, "DOWNLOAD %s %s = %s", id, date, CData(val));
	#endif
		return val;
	}

	static UINT threaddate(LPVOID pParam)
	{
		threaddata *td = (threaddata *)pParam;
		td->val = td->that->GetDate(*td->date);
		td->that->numthreads--;
		td->that = NULL;
		return 0;
	}

};


int WaterflowDates(vars &id, vara &dates, FCGX_Stream *stream)
{
	CWaterflowDates(id, dates, stream);
	return TRUE;
}

*/
#endif

int WaterflowInfo(const char *q, FCGX_Stream *stream)
	{
	vara sum;
	vara coords(q);
	if (coords.length()>=2)
		sum = GetRiversPointSummary(coords[1]+";"+coords[0]);

	if (sum.length()==0)
		Log(LOGWARN, "no river info for winfo=%s", coords.join(","));

	FCGX_PutS("{\"list\":[", stream);
	for (int d=0; d<sum.length(); ++d)
		{
		if (d>0) FCGX_PutS(",", stream);
		FCGX_PutS("\"", stream);		
		FCGX_PutS(sum[d].replace("\"", "\'"), stream);
		FCGX_PutS("\"", stream);
		}
	FCGX_PutS("]}\r\n", stream);

	return TRUE;
	}





int WaterflowQuery(VOID *ptr)
{
	FCGX_Request *request = (FCGX_Request *)ptr;
	const char *query = FCGX_GetParam("QUERY_STRING", request->envp);
#ifndef DEBUG
	__try { 
#endif
	testing = ucheck(query, "wftest=")!=NULL;
	logging = ucheck(query, "wflog=")!=NULL;

	const char * q, *o;
	if (q=ucheck(query, "wfrect="))
		WaterflowRect(q, request->out);		

	if (q=ucheck(query, "wffrect="))
		WaterflowForecastRect(q, ucheck(query, "wfname="), request->out);		

	if (q=ucheck(query, "wfkmlrect="))
		WaterflowKMLRect(q, ucheck(query, "=allusgs"), request->out);		

	if (q=ucheck(query, "wfid="))
		{
		if (o=ucheck(query, "wfdates="))
		   WaterflowDates(q, o, request->out);
		}

	if (q=ucheck(query, "winfo="))
		{
		WaterflowInfo(q, request->out);
		}

	return FALSE;
#ifndef DEBUG
} __except(EXCEPTION_EXECUTE_HANDLER) 
	{ 
	Log(LOGALERT, "WATERFLOW CRASHED!!! %.200s", query); 	
	return FALSE; 
	}
#endif
}


tprovider providers[] = {
	{ USGS, "cfs/ft/F,690,600", NULL, NULL, NULL, "", FALSE, {
	  //"https://waterdata.usgs.gov/nwis/uv?site_no=%id",
	  "https://waterdata.usgs.gov/nwis/nwisman/?site_no=%id", 
	  "https://nwis.waterdata.usgs.gov/nwis/uv/?format=img_stats&site_no=%id&begin_date=%YYYY1%MM1%DD1&end_date=%YYYY2%MM2%DD2", 
	  "https://waterdata.usgs.gov/nwis/dv/?format=img_stats&site_no=%id&begin_date=%YYYY1%MM1%DD1&end_date=%YYYY2%MM2%DD2", 
	  //"https://nwis.waterdata.usgs.gov/nwis/uv?cb_00060=on&format=gif_mult_parms&site_no="+id+"&referred_module=sw&period=7";
	  //https://waterdata.usgs.gov/nwis/dv?cb_00060=on&format=gif_stats&site_no="+id+"&referred_module=sw&period=365";
	}},

#if 1
	{ "DF", "cfs/ft/F,690,600", DF_getsites, DF_getdate, NULL, "75,424", FALSE, {
		"http://www.dreamflows.com/flows.php?page=prod&zone=canv&form=norm",
		"http://www.dreamflows.com/graphs/day.%mode.php",
	  "http://www.dreamflows.com/graphs/mon.%mode.php",
	  "http://www.dreamflows.com/graphs/yir.%mode.php",
	}},

	{ "CDEC", "cfs/ft/F,690,600", CDEC_getsites, CDEC_getdate, USGS_gethistory, "NKD", FALSE, {
	  "http://cdec.water.ca.gov/cgi-progs/staMeta?station_id=%id",
	  //http://cdec.water.ca.gov/jspplot/jspPlotServlet.jsp?sensor_no=20000&end=&geom=small&interval=2&cookies=cdec01
	  "http://cdec.water.ca.gov/histPlot/DataPlotter.jsp?staid=%id&sensor_no=%mode&duration=%modeb&start=%MM1%2F%DD1%2F%YYYY1+00%3A00&end=%MM2%2F%DD2%2F%YYYY2+23%3A59&geom=small&interval=2&cookies=cdec01&stamp=%stamp",
	}},


	{ "SCE", "cfs/ft/F,690,600", SCE_getsites, SCE_getdate, NULL, "11230215,11231500", FALSE, {
	  "http://kna.kisters.net/scepublic/stations/%id/station.html?v=%stamp",
	  "http://kna.kisters.net/scepublic/stations/%id/station.html?v=%stamp"
	}},

	{ "COERG", "cfs/ft/F,690,600", COERG_getsites, COERG_getdate, USGS_gethistory, "ISBQ", FALSE, {
		"http://rivergages.mvr.usace.army.mil/WaterControl/stationinfo2.cfm?sid=%id",
		"http://rivergages.mvr.usace.army.mil/WaterControl/shefgraph-wotem2.cfm?sid=%id&d=7&dt=S",
	  "http://rivergages.mvr.usace.army.mil/WaterControl/shefgraph-wotem2.cfm?sid=%id&d=31&dt=S",
	  "http://rivergages.mvr.usace.army.mil/WaterControl/shefgraph-wotem2.cfm?sid=%id&d=120&dt=S&cy=true",
	}},

	{ "COESPK", "cfs/ft/F,690,600", COESPK_getsites, COESPK_getdate, NULL, "ISB1", TRUE, {
	  "http://www.spk-wc.usace.army.mil/plots/california.html",
	  "http://www.spk-wc.usace.army.mil/fcgi-bin/getplot.py?plot=%mode&length=10day&interval=h",
	  "http://www.spk-wc.usace.army.mil/fcgi-bin/getplot.py?plot=%mode&length=1month&interval=d",
	  "http://www.spk-wc.usace.army.mil/fcgi-bin/getplot.py?plot=%mode&interval=d&length=wy&wy=%YYYY2",
	}},

	{ "CDWR", "cfs/ft/F,690,600", CDWR_getsites, CDWR_getdate, USGS_gethistory, "BIJOUSCO", FALSE, {
	  "http://www.dwr.state.co.us/SurfaceWater/data/detail_graph.aspx?ID=%id",
	  "http://www.dwr.state.co.us/SurfaceWater/data/detail_graph.aspx?ID=%id",
	}},

	{ "OWRD", "cfs/ft/F,690,600", OWRD_getsites, OWRD_getdate, USGS_gethistory, "14073520,14206200", FALSE, {
	  "http://apps.wrd.state.or.us/apps/sw/hydro_near_real_time/display_hydro_graph.aspx?station_nbr=%id",
	  "http://apps.wrd.state.or.us/apps/sw/hydro_near_real_time/display_hydro_graph.aspx?station_nbr=%id&start_date=%MM1/%DD1/%YYYY1&end_date=%MM2/%DD2/%YYYY2", //&dataset=%mode",
	}},

	{ "WECY", "cfs/ft/F,690,600", WECY_getsites, WECY_getdate, USGS_gethistory, "23J070,39S050", TRUE, {
	  "https://fortress.wa.gov/ecy/wrx/wrx/flows/station.asp?sta=%id",
	  "https://fortress.wa.gov/ecy/wrx/wrx/flows/stafiles/%id/%id_%mode_SD.png",
	  "https://fortress.wa.gov/ecy/wrx/wrx/flows/stafiles/%id/%id_%mode_SD.png",
	  "https://fortress.wa.gov/ecy/wrx/wrx/flows/stafiles/%id/%id_%mode_WY.png",
	}},

	{ "CNDWO", "m3s/m,1000,800", CNDWO_getsites, CNDWO_getdate, CNDWO_gethistory, "08GB013", FALSE, {
	  "http://wateroffice.ec.gc.ca/report/real_time_e.html?type=h2oArc&stn=%id" CNDWOMODE,
	  "http://wateroffice.ec.gc.ca/report/real_time_e.html?mode=Graph&type=realTime&stn=%id&dataType=Real-Time&startDate=%YYYY1-%MM1-%DD1&endDate=%YYYY2-%MM2-%DD2&y1Max=&y1Min=&mean1=1&prm2=-1&y2Max=&y2Min=#rt-graph-tbl-data" CNDWOMODE,
	}},

	{ "BRPN", "cfs/ft/F,820,700", BRPN_getsites, BRPN_getdate, USGS_gethistory, "YGVW,ELWW", FALSE, {
	  "http://www.usbr.gov/pn/hydromet/decod_params.html", 
	  "http://www.usbr.gov/pn/hydromet/instant_graph.html?cbtt=%id&pcode=%mode&t1=%YYYY1%MM1%DD1&t2=%YYYY2%MM2%DD2#graphdiv", 
	  //"http://www.usbr.gov/gp-bin/hydromet_arcplt.pl?%id&%modeb",
	  //"http://www.usbr.gov/pn-bin/graphrt.pl?%id_%mode",
	  //"http://www.usbr.gov/pn-bin/graphwy.pl?%id_%mode", 
	}},

	{ "BRGP", "cfs/ft/F,650,600", BRGP_getsites, BRGP_getdate, USGS_gethistory, "ARKCANCO,ARKPUECO", FALSE, {
	  "http://www.usbr.gov/gp/hydromet/%id.html",
	  "http://www.usbr.gov/gp-bin/hydromet_dayplt.pl?em=%MT2&ed=%DD2&nd=7&s1=%id&pa1=%mode&year=%YYYY2&tr=ON",
	  "http://www.usbr.gov/gp-bin/hydromet_dayplt.pl?em=%MT2&ed=%DD2&nd=30&s1=%id&pa1=%mode&year=%YYYY2&tr=ON",
	  "http://www.usbr.gov/gp-bin/hydromet_arcplt.pl?em=%MT2&ed=%DD2&s1=%id&p1=%modeb&y1=%YYYY2&tr=ON"
	  //"http://www.usbr.gov/gp-bin/hydromet_arcplt.pl?%id&%modeb",
	}},

#endif

/*
	{ "NID", "cfs/ft/F,690,600", NID_getsites, NID_getdate, USGS_gethistory, "CCBB", FALSE, {
	  "http://nidwater.com/recreation/river-lake-hourly-data/",
	  "http://nidwater.com/river-lake-hourly-data/%id.png",
	}},
*/
	NULL,
	};

























	
