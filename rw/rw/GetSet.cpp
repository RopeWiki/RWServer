#include "stdafx.h"
#include "atlstr.h"
#include "msxml.h"
#include "math.h"
#include "TradeMaster.h"

//#define DEBUGDATE	// DEBUGLOAD debug proper date conversion Ole vs Julian
#define ALERTLARGENUM // alert of large numbers being Get or Set

#if 0 // 1=DEBUG try and check for consistency different implementations
#define SetNumTest CDATA::SetNum
#else
#define SetNumNew CDATA::SetNum
#endif

#if 0	// 1=DEBUG try and check for consistency different implementations
#define GetNumTest CGetNum
#else
#define GetNumNew CGetNum 
#endif


#define FRACPREC 1e4	// max precision of OUTPUT floating point numbers 1e3 = 3






BOOL CDATA::CompareNUM(double nd, double od, double erreps)
	{
	if (nd==0 || od==0)
		return nd==od;
	if (nd==InvalidNUM || od==InvalidNUM)
		return nd==od;
	if (fabs(nd-od)<erreps)
		return TRUE;
	return fabs(nd-od)/min(fabs(nd),fabs(od))<erreps;
	}

////////////////////////////////////////////
// GET
////////////////////////////////////////////

#define valid_digit(c) ((c) >= '0' && (c) <= '9')


double GetNumNew(char const *p, double def)
{
	const char *op = p;
    if (*p == 0) return def;
	PROFILE("CDATA::GetNumNew()");

	int digit = 0;
	double r = 0.0;
    bool neg = false;

	while (isspace(*p) || *p=='$') 
		++p;

	// neg
    if (*p == '-') 
		{
        neg = true;
        ++p;
		}

	if (*p!='.')
	{
	if (!valid_digit(*p))
		return def;

	// numbers
    while (valid_digit(*p)) 
		{
        r = (r*10.0) + (*p - '0');
        ++p, ++digit;
		// ,
		//if (*p==',') 
		//	++p;
		}
	}

	// dot
    if (*p == '.') 
		{
        double f = 0.0;
        int n = 0;
        ++p;
        while (valid_digit(*p)) 
			{
            f = (f*10.0) + (*p - '0');
            ++p, ++digit;
            ++n;
	        }
        r += f / pow(10.0, n);
		}

	// dot
    if (*p == 'e') 
		{
		++p;
		bool eneg = false;
		if (*p == '+') 
			++p;
		if (*p == '-') 
			{
			eneg = true;
			++p;
			}
        double e = 0.0;
		while (valid_digit(*p)) 
			{
			e = (e*10.0) + (*p - '0');
			++p, ++digit;
			}
		if (eneg) 
			e = -e;
		r *= pow(10.0, e);
		}

	if (!digit)
		return def;

    if (neg) 
		r = -r;

	if (*p=='M')
		r = r*1000000;
	if (*p=='B')
		r = r*1000000*1000;	
	if (*p=='%')
		r = r/100.0;

#ifdef ALERTLARGENUM
	if (IsIllegalNUM(r)) // warn of suspiciously large numbers
		Log(LOGALERT, "LARGENUM: %.100s = SetNum(%g)", op, r);
#endif

    return r;
}

double GetNumNewB(char const *p, double def)
{
    if (*p == 0) return def;
	PROFILE("CDATA::GetNumNewB()");
	return atof(p);
}

double GetNumNewC(char const *p, double def)
{
    if (*p == 0) return def;
	PROFILE("CDATA::GetNumNewC()");
	char *stopstring = NULL;
	return strtod(p, &stopstring);
}


#define white_space(c) ((c) == ' ' || (c) == '\t')

double GetNumNewD(const char *p, double def)
{
	if (*p==0) return def;
	PROFILE("CDATA::GetNumNewD()");


    // Skip leading white space, if any.

	while (white_space(*p) ) {
        p += 1;
    }

    // Get sign, if any.

    double sign = 1.0;
    if (*p == '-') {
        sign = -1.0;
        p += 1;

    } else if (*p == '+') {
        p += 1;
    }

	if (!valid_digit(*p))
		return def;

	// Get digits before decimal point or exponent, if any.

    double value = 0.0;
    while (valid_digit(*p)) {
        value = value * 10.0 + (*p - '0');
        p += 1;

		//if (*p==',') 
		//	p += 1;
    }

    // Get digits after decimal point, if any.

    if (*p == '.') {
        double pow10 = 10.0;
        p += 1;

        while (valid_digit(*p)) {
            value += (*p - '0') / pow10;
            pow10 *= 10.0;
            p += 1;
        }
    }

    // Handle exponent, if any.

	int frac = 0;
    double scale = 1.0;
    if ((*p == 'e') || (*p == 'E')) {
        unsigned int expon;
        p += 1;

        // Get sign of exponent, if any.

        if (*p == '-') {
            frac = 1;
            p += 1;

        } else if (*p == '+') {
            p += 1;
        }

        // Get digits of exponent, if any.

        expon = 0;
        while (valid_digit(*p)) {
            expon = expon * 10 + (*p - '0');
            p += 1;
        }
        if (expon > 308) expon = 308;

        // Calculate scaling factor.

        while (expon >= 50) { scale *= 1E50; expon -= 50; }
        while (expon >=  8) { scale *= 1E8;  expon -=  8; }
        while (expon >   0) { scale *= 10.0; expon -=  1; }
    }

	if (*p=='%')
		value /= 100.0;
	if (*p=='B')
		value *= 1000000*1000;	
	if (*p=='M')
		value *= 1000000;
    // Return signed and scaled floating point result.

    return sign * (frac ? (value / scale) : (value * scale));
}


double GetNumOld(char const *buffer, double def)
{
	if (*buffer==0)
		return def;

	PROFILE("CDATA::GetNumOld()");
	double mul = 1;
	char value[400], *v=value;
	while (*buffer!=0 && *buffer!=',')
		{
		if (*buffer!='$')
			*v++ = *buffer;
		if (*buffer=='M')
			mul = 1000000;
		if (*buffer=='B')
			mul = 1000000*1000;
		if (*buffer=='%')
			mul = 1/100.0;
		buffer++;
		}
	*v++ = 0;

	double f;
	if (sscanf(value, "%lf", &f)==1)
		return f*mul;
#ifdef DEBUG
	//Log(LOGALERT, "unexpected illegal num GetNum(%.30s) for %s", value, "GetNum");
#endif
	return def;
}


double GetNumTest(char const *buffer, double def)
{	
	double r1 = GetNumNew(buffer, def);
	double r1b = GetNumNewB(buffer, def);
	double r1c = GetNumNewC(buffer, def);
	double r1d = GetNumNewD(buffer, def); //very fast but bit unreliable

	double r2 = GetNumOld(buffer, def);
	/*
	double diff = (*((long*)&r1)-*((long*)&r2)); 
	if (diff<0) diff = -diff;
	if (diff>1)
	*/
	CString s1 = MkString("%g", r1);
	CString s2 = MkString("%g", r2);
	if (s1.Compare(s2)!=0)
	//if (!CDATA::CompareNUM(r2,r1))
		Log(LOGALERT, "ERROR: CDATA::GetNumTest() inconsistent %g OLD<>NEW %g for \"%.16s\"", r2, r1, buffer);
	return r1;
}

/*

double GetNum(const char *)
{
	if (!buffer|| *buffer==0)
		return InvalidNUM;

	PROFILE("CDATA::GetNumOld()");
	double mul = 1;
	char value[50], *v=value;
	for (int i=0; i<sizeof(value)-1; ++i,++v)
		{
		if (*buffer!=',' && *buffer!='$' && buffer!=' ')
			v[i] = *buffer;
		if (*buffer=='M')
			mul = 1000000;
		if (*buffer=='B')
			mul = 1000000*1000;
		if (*buffer=='%')
			mul = 1/100.0;
		buffer++;
		}
	*v++ = 0;

	return CGetNum();
}

*/



// double Date functions

static COleDateTime ref_monday(1900, 1, 1, 0, 0, 0);
double CurrentDate = Date( COleDateTime::GetCurrentTime() );
double RealCurrentDate = Date( COleDateTime::GetCurrentTime(), TRUE );

BOOL NOWEEKEND = TRUE;

double NoWeekend( double date, BOOL advance )
{
	if (!NOWEEKEND)
		return date;

	int day = ((int)date)%7;
	if (day==6) date += advance ? 1 : -2;
	if (day==5) date += advance ? 2 : -1;
	return date;
}

double SetCurrentDate( double date)
{
	if (date == 0 )
		date = Date( COleDateTime::GetCurrentTime() );
	return CurrentDate = date;
}

double Date( COleDateTime &date, BOOL real)
{
	int n = (date - ref_monday).GetDays();	
	return real ? n : NoWeekend(n); 
}

double DateFloat( COleDateTime &date, BOOL real)
{
	double n = (date - ref_monday).GetTotalDays();	
	return real ? n : NoWeekend(n); 
}


COleDateTime OleDateTime( double date)
{
	return ref_monday + COleDateTimeSpan( (int)date, 0, 0, 0 );
}

double GetTime(void) 
{
	return COleDateTime::GetCurrentTime();
}



#define JREFDATE (2415021) // 1900-1-1

double Date( int d, int m, int y)
{
	if (d<=0 || m<=0 || y<=0)
		return InvalidDATE;

	if (m==2 && d==29) d=28; // patch Feb to void errors

	int month = m;
	int year = y%100;
	int century = y/100;
	if (!century)
		century = 20;
    if (month > 2)
        month -= 3;
    else
      {
        month += 9;
        if (year)
            year--;
        else
          {
            year = 99;
            century--;
          }
      }
	long idate = 0;
	idate += (146097L * century)    / 4L;
	idate += (1461L   * year)       / 4L;
	idate += (153L    * month + 2L) / 5L;

	double date = NoWeekend( idate + d + 1721119L - JREFDATE );

	if (DEBUGLOAD)
	{
	//int odate = (COleDateTime(y, m, d, 0, 0, 0)- ref_monday).GetDays();
	double odate = Date( COleDateTime(y, m, d, 0, 0, 0) );
	if (odate!=date)
		Log(LOGALERT, "Inconsitency with Date2Days(%d,%d,%d) %.0f!=%.0f", y,m,d, date, odate);
	}
	return date;
}




static char *Date( char *data, double d)
{
	int days = (int)d+JREFDATE;
	int  century, year, month, day;

    days   -= 1721119L;
    century = (4L * days - 1L) / 146097L;
    days    =  4L * days - 1L  - 146097L * century;
    day     =  days / 4L;

    year    = (4L * day + 3L) / 1461L;
    day     =  4L * day + 3L  - 1461L * year;
    day     = (day + 4L) / 4L;

    month   = (5L * day - 3L) / 153L;
    day     =  5L * day - 3   - 153L * month;
    day     = (day + 5L) / 5L;

    if (month < 10)
        month += 3;
    else
      {
        month -= 9;
        if (year++ == 99)
          {
            year = 0;
            century++;
          }
      }
	year += century*100;

	if (!IsValidDay(day) || !IsValidMonth(month) || !IsValidYear(year) || d<0 || d>365*200) //1900-2100
		{
		ASSERT( !"WTF??" );
		Log(LOGALERT, "ERROR: CDATA::SetDate() unexpected out of range date %g (%4d-%2d-%d)", d, year, month, day);
		*data = 0;
		return data;
		}

	char *datastr;
	itoa(year, datastr = data, 10);
	datastr+=4; 
	*datastr++ = '-';
	*datastr++ = month/10+'0';
	*datastr++ = month%10+'0';
	*datastr++ = '-';
	*datastr++ = day/10+'0';
	*datastr++ = day%10+'0';
	*datastr++ = 0;

	if (DEBUGLOAD)
	{
	CString odate = OleDateTime(d).Format(DATEFORMAT);
	// consistency check
	ASSERT( CGetDate(data)!=InvalidDATE );
	if (*data==0 || !IsDate(data,DATELEN))
		Log(LOGALERT, "ERROR: CDATA::SetDate() unexpected illegal date #%g (\"%.30s\")", d, data);
	if (odate.Compare(data)!=0)
		Log(LOGALERT, "Inconsitency with Date2Days %f %s!=%s", d, data, odate);
	}
	return data;
}







int dcomp(const char *str, int n)
{
	for (int i=0; i<n && str!=NULL && *str!=0; ++i, ++str)
		{
		str = strchr(str, '-');
		if (!str) return 0;
		}
	return atoi(str);
}

double CDATA::GetDate(const char *value)
{
	PROFILE("CGetDate()");
	// YYYY-MM-DD
	int Y = dcomp(value, 0);
	int M = dcomp(value, 1);
	int D = dcomp(value, 2);
	if (!IsValidDay(D) || !IsValidMonth(M) || !IsValidYear(Y))
		{
		Log(LOGALERT, "ERROR: CGetDate() unexpected illegal date \"%.30s\"", value);
		return InvalidDATE;
		}
	return Date( D, M, Y);
}


inline int dvar(char c)
{
	if (c=='Y') return 2;
	if (c=='M') return 1; 
	if (c=='D') return 0;
	return -1;
}

static const char *monthname[] = {"X-0-X", "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                      "JUL", "AUG", "SEP", "OCT", "NOV", "DEC", "JAN", NULL};

const char *CDATA::GetMonth(int m)
{
	return monthname[m];
}

int CDATA::GetMonth(const char *m)
{
	for (int i=1; i<=12; ++i)
		if (IsSimilar(m,monthname[i]))
			return i;

	return 0;
}

// fmt = M/D/Y  Y-M-D YYYYMMMDD DDMMMYY
double CDATA::GetDate(const char *date, const char *fmt)
{
		// get 3 values
		char str[3][10];

		{
		int f=0, d=0; 
		while (isspace(date[d]))
			++d;

		if (date[d]==0)
			return InvalidDATE;

		int lastv = -1, lastc = -1;
		while (fmt[f]!=0 && date[d]!=0)
			{
			// set variable
			int dv = dvar(fmt[f++]);
			if (dv!=lastv)
				{
				lastv = dv;
				lastc = 1;
				while (dvar(fmt[f])==lastv)
					++f, ++lastc;
				}
			if (lastv<0)
				return InvalidDATE;

			// get value
			int n = 0;
			if (dvar(fmt[f])<0)
				{
				// get chars up to separator
				while (date[d]!=0 && date[d]!=fmt[f] && !isspace(date[d]))
					str[lastv][n++] = date[d++];
				}
			else
				{
				// get fixed length
				for (int i=0; i<lastc && date[d]!=0; ++i)
					str[lastv][n++] = date[d++];
				}
			str[lastv][n] = 0;

			// skip separators
			while (fmt[f]!=0 && date[d]!=0 && dvar(fmt[f])<0 && date[d]==fmt[f])
				++d, ++f;
			}
		}

		double d = CDATA::GetNum( str[0] );
		double m = CDATA::GetNum( str[1] );
		if (m==InvalidNUM)
			m = GetMonth( str[1] );
		double y = CDATA::GetNum( str[2] );
		if (y>=0 && y<100)
			{
			if (y<50)
				y += 2000;
			else
				y +=1900;
			}
		if (y<=0 || d<=0 || m<=0)
			{
			Log(LOGALERT, "ERROR: CGetDate() unexpected illegal date %s \"%.30s\"", fmt, date);
			return InvalidDATE;
			}
		return Date((int)d, (int)m, (int)y);
	}



double CDATA::GetDateDbg(const char *value, const char *file, int line)
{
	PROFILE("CGetDate()");
	// YYYY-MM-DD
	int Y = dcomp(value, 0);
	int M = dcomp(value, 1);
	int D = dcomp(value, 2);
	if (!IsValidDay(D) || !IsValidMonth(M) || !IsValidYear(Y))
		{
		Log(LOGALERT, "ERROR: CGetDate() unexpected illegal date \"%.30s\" at %s#%d", value, file, line);
		return InvalidDATE;
		}
	return Date( D, M, Y);
}





















////////////////////////////////////////////
// SET
////////////////////////////////////////////



const char *CDATA::SetString(CString &str, int *count)
{
	if (str.IsEmpty())
		return InvalidSTR;

	str.Replace(",",";"); // avoid problems

	if (count) 
		++count[0];
	return str;
}

const char *CDATA::SetDate(char *data, double d, int *count)
{
	if (d==InvalidDATE || d<0)
		return InvalidSTR;


	if (count) 
		++count[0];

	PROFILE("CDATA::SetDate()");
	return Date(data, d);
}


// SetNum
const char *SetNumOld(char *data, double d, int *count)
{
	if (d==InvalidNUM)
		return strcpy(data,InvalidSTR);

	if (count) 
		++count[0];

	PROFILE("CDATA::SetNumOld()");

	// set max 3 decimals
	double frac = d-floor(d);
	double nfrac = floor(frac*FRACPREC+0.5)/FRACPREC;
	double df = d - frac + nfrac;
/*
	double e = d;
	*((DWORD *)&e) &= 1;
	*((DWORD *)&e) |= ~1;
*/
	//str.Format();
	//ASSERT( str.Find("#IND")<0 );
	sprintf(data, "%lg", df);
	// ALSO possible:
	// _fcvt _ecvt
	return data;
}


// other interesting references on num to string convesion
// http://www.jb.man.ac.uk/~slowe/cpp/itoa.html
// and http://www.ddj.com/dept/cpp/184401596?pgno=6

// Version 19-Nov-2007
// Fixed round-to-even rules to match printf
//   thanks to Johannes Otepka

/**
* Powers of 10
* 10^0 to 10^9
*/
static const double pow10[] = {1, 10, 100, 1000, 10000, 100000, 1000000,
10000000, 100000000, 1000000000};

static void strreverse(char* begin, char* end)
{
char aux;
while (end > begin)
aux = *end, *end-- = *begin, *begin++ = aux;
}


const char *SetNumNew(char *str, double d, int *count)
{
	if (d==InvalidNUM)
		return strcpy(str,InvalidSTR);

	if (count) 
		++count[0];

	PROFILE("CDATA::SetNumNew()");

#ifdef ALERTLARGENUM
	if (IsIllegalNUM(d)) // warn of suspiciously large numbers
		Log(LOGALERT, "LARGENUM: SetNum(%g)", d);
#endif

	// set max 4 decimals
	#define prec 4
	register double value = d;	

	{
	double frac = value-floor(value);
	double nfrac = floor(frac*FRACPREC+0.5)/FRACPREC;
	value = value - frac + nfrac;
	}
	
	typedef unsigned int uint32_t;

	/* if input is larger than thres_max, revert to exponential */
	const double thres_max = (double)(0x7FFFFFFF);

	double diff = 0.0;
	register char* wstr = str;

#ifndef prec
	if (prec < 0) {
		prec = 0;
	} else if (prec > 9) {
	/* precision of >= 10 can lead to overflow errors */
		prec = 9;
	}
#endif

	/* we'll work in positive values and deal with the
	negative sign issue later */
	int neg = 0;
	if (value < 0) {
		neg = 1;
		value = -value;
	}
	int whole = (int) value;

	/* for very large numbers switch back to native sprintf for exponentials.
	anyone want to write code to replace this? */
	/*
	normal printf behavior is to print EVERY whole number digit
	which can be 100s of characters overflowing your buffers == bad
	*/
	if (value >= thres_max) 
		{
		sprintf(str, "%e", neg ? -value : value);		
		return str;
		}
	if (whole<0) // impossible!
		return strcpy(str,InvalidSTR);

	double tmp = (value - whole) * pow10[prec];
	uint32_t frac = (uint32_t)(tmp);
	diff = tmp - frac;

	if (diff > 0.5) {
		++frac;
		/* handle rollover, e.g.  case 0.99 with prec 1 is 1.0  */
		if (frac >= pow10[prec]) 
			{
			frac = 0;
			++whole;
			}
	} else if (diff == 0.5 && ((frac == 0) || (frac & 1))) {
		/* if halfway, round up if odd, OR
		if last digit is 0.  That last part is strange */
		++frac;
	}


	if (prec == 0) 
	{
		diff = value - whole;
		if (diff > 0.5)	
				{
				/* greater than 0.5, round up, e.g. 1.6 -> 2 */
				++whole;
				} 
		else 
		if (diff == 0.5 && (whole & 1)) 
				{
				/* exactly 0.5 and ODD, then round up */
				/* 1.5 -> 2, but 2.5 -> 2 */
				++whole;
				}
	} 
	else 
	{
		if (frac>0)
			{
			register int count = prec;
			register char do0 = FALSE;
			// now do fractional part, as an unsigned number
			do {
				--count;
				register char c = (char)(frac % 10);
				do0 |= c; 
				if (do0) *wstr++ = 48+c;
			} while (frac /= 10);
			// add extra 0s
			while (count-- > 0) *wstr++ = '0';
			// add decimal
			*wstr++ = '.';
			}
	}

	// do whole part
	// Take care of sign
	// Conversion. Number is reversed.
	do *wstr++ = (char)(48 + (whole % 10)); while (whole /= 10);
	if (neg) {
	*wstr++ = '-';
	}
	*wstr=0;

	strreverse(str, wstr-1);
	return str;
	}


double TestNum(const char *value)
{
	double f;
	if (sscanf(value, "%lf", &f)==1)
		return f;
	return InvalidNUM;
}

const char *SetNumTest(char *data, double d, int *count)
{
	if (d==InvalidNUM)
		return InvalidSTR;

	char odata[50];
	SetNumOld(odata, d,count);
	double od = TestNum(odata);

	SetNumNew(data, d,count); // super fast but different (minor errors)
	double nd = TestNum(data);

	/*
	double value = d;
	double frac = value-floor(value);
	double nfrac = floor(frac*FRACPREC+0.5)/FRACPREC;
	value = value - frac + nfrac;
	*/
	

#if 0 // exact
	if (strcmp(data,odata)!=0)
		Log(LOGALERT, "ERROR: SetNumTest  discrepancy >.1%% \"%.16s\" OLD<>NEW \"%.16s\" for %g", odata, data, d);
#else
	// aprox 
	char ndata2[50];
	SetNumNew(ndata2, nd, NULL);
	double nd2 = TestNum(data);
	if (nd!=nd2)
		Log(LOGALERT, "ERROR: CDATA::SetNumTest() inconsistency \"%.16s\" OLD<>NEW \"%.16s\"", odata, data);
	if (strcmp(ndata2,data)!=0)
		Log(LOGALERT, "ERROR: CDATA::SetNumTest() unreliable \"%.16s\" NEW2<>NEW \"%.16s\"", ndata2, data);

	if (!CDATA::CompareNUM(nd,od))
		Log(LOGALERT, "ERROR: CDATA::SetNumTest() discrepancy \"%.16s\" OLD<>NEW \"%.16s\" for %g", odata, data, d);
#endif

	return SetNumNew(data, d,count);
}











