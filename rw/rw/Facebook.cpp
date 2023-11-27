//Michelle: this class doesn't seem to be used anywhere

#include "CPage.h"
#include "Code.h"
#include "BETAExtract.h"
#include "trademaster.h"


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

extern GeoCache _GeoCache;
extern GeoRegion _GeoRegion;
extern Code *codelist[];

extern int INVESTIGATE;

int iReplace(vars &line, const char *ostr, const char *nstr)
{
	int n = 0;
	vars nline;
	register int olen = strlen(ostr), nlen = strlen(nstr);
	for (register int i = 0; line[i] != 0; ++i)
	{
		if (strnicmp(((const char*)line) + i, ostr, olen) == 0)
		{
			line.Delete(i, olen);
			if (*nstr != 0)
				line.Insert(i, nstr);  // 3  4
			i += nlen - 1;
			++n;
		}
	}

	return n;
}


int BestMatchSimple(const char *prname, const char *pllname, int *perfects = NULL)
{
	int n = 0, m = 0, nskip = 0, mskip = 0;
	int nperfect = 0, npartial = 0;
	int mperfect = 0, mpartial = 0;
	while (prname[n] != 0 && pllname[m] != 0)
	{
		if (tolower(prname[n]) != tolower(pllname[m]))
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
			if (!isanum(prname[n + 1]) && !isanum(pllname[m + 1]))
				nperfect = n + 1, mperfect = m + 1;
			npartial = n + 1;
		}
		++n; ++m;
	}

	if (perfects)
	{
		int pr0 = prname[n] == 0, pl0 = pllname[m] == 0;
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

void LoadRWList();

const char *skipalphanum(const char *str)
{
	while (isanum(*str))
		++str;
	return str;
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

const char *skipnotalphanum(const char *str)
{
	while (*str != 0 && !isanum(*str))
		++str;
	return str;
}

int countnotalphanum(const char *str)
{
	int cnt = 0;
	while (*str != 0 && !isanum(*str))
		++str, ++cnt;
	return cnt;
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
	if (!link || *link == 0)
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
		if (strstr(url, "/profile.php?"))
		{
			vars id = ExtractString(url + "&", "id=", "", "&");
			if (id.IsEmpty())
			{
				Log(LOGERR, "ERROR: Cannot convert profile.php link %s", url);
				return "";
			}
			url = "/" + id;
		}
		if (strstr(url, "/story.php"))
		{
			//https://m.facebook.com/story.php?story_fbid=10208789220476568&id=1494703586&_rdr
			//https://www.facebook.com/1494703586/posts/10208789220476568
			vars url2 = url + "&";
			vars story = ExtractString(url2, "?story_fbid=", "", "&");
			vars user = ExtractString(url2, "&id=", "", "&");
			if (story.IsEmpty() || user.IsEmpty())
			{
				Log(LOGERR, "ERROR: Cannot convert story_fbid link %s", url);
				return "";
			}
			url = "/" + user + "/posts/" + story;
		}
		if (noparam < 0)
			url = vars(url).split("&refid=").first().split("?refid=").first();
		else
			url = GetToken(url, 0, '?');
	}

	url = burl("m.facebook.com", url);
	url.Replace("//www.facebook.com", "//m.facebook.com");
	url.Replace("//facebook.com", "//m.facebook.com");
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
		if (username == name)
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
	while (*str == ' ') ++str;
	return str;
}


static int cmplength(const void *arg1, const void *arg2)
{
	return ((CSym**)arg2)[0]->id.GetLength() - ((CSym**)arg1)[0]->id.GetLength();
	//	return CSymID::Compare(((CSym**)arg1)[0]->id, ((CSym**)arg2)[0]->id);
}



void PrepMatchList(CSymList &list)
{
	// prepare to use IsMatchList
	for (int i = 0; i < list.GetSize(); ++i)
		list[i].id = stripAccents2(list[i].id).Trim();
	list.Sort(cmplength);
}


int IsMatchList(const char *str, CSymList &list, BOOL noalpha = TRUE, int *matchi = NULL, int minmatch = -1)
// noalpha = TRUE common string must terminate in a non alpha char, FALSE can end in any char
// minmatch = minimum chars to match, if more are present they have to match too
{
	str = skipnotalphanum(str);
	for (int i = 0; i < list.GetSize(); ++i)
	{
		const char *strid = list[i].id;
		ASSERT(*strid != ' ');
		int l;
		for (l = 0; str[l] != 0 && tolower(str[l]) == tolower(strid[l]); ++l);
		int  match = strid[l] == 0;
		if (minmatch > 0 && l >= minmatch)
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
	if (y < 100) y += 2000;
	if (d < 0 || d>31 || m < 0 || m>12 || y < 0 || y <= 2012)
		return -1;
	return Date((int)d, (int)m, (int)y);
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
		double date = CheckDate(d, m, y);
		if (date <= 0)
			return FALSE;
		double diff = fabs(date - pdate);
		if (diff < mind)
		{
			mind = diff;
			mindate = date;
			return TRUE;
		}
		return FALSE;
	}



#define MAXNUMS 4


	int FACEBOOK_AdjustDate(int num, tnums nums[], double vdate, double &date, int &start, int &end)
	{
		double vdatey = CDATA::GetNum(CDate(vdate)), vdatey1 = vdatey - 1, vdatey2 = vdatey + 1;
		double mindate = 0, mind = 1000;
		for (int n = num; n < MAXNUMS; ++n)
			nums[n].val = -1;
		for (int n = 0; n < 2; ++n)
		{
			double v0 = nums[n].val, v1 = nums[n + 1].val, v2 = nums[n + 2].val;
			if (v2 >= 0)
			{
				if (FACEBOOK_CheckDate(v0, v1, v2, mind, mindate, vdate))
					start = nums[n].start, end = nums[n + 2].end;
				if (FACEBOOK_CheckDate(v1, v0, v2, mind, mindate, vdate))
					start = nums[n].start, end = nums[n + 2].end;
				if (FACEBOOK_CheckDate(v2, v1, v0, mind, mindate, vdate))
					start = nums[n].start, end = nums[n + 2].end;
				if (FACEBOOK_CheckDate(v2, v0, v1, mind, mindate, vdate))
					start = nums[n].start, end = nums[n + 2].end;
			}
			else
			{
				if (FACEBOOK_CheckDate(v0, v1, vdatey, mind, mindate, vdate))
					start = nums[n].start, end = nums[n + 1].end;
				if (FACEBOOK_CheckDate(v1, v0, vdatey, mind, mindate, vdate))
					start = nums[n].start, end = nums[n + 1].end;
				if (FACEBOOK_CheckDate(v0, v1, vdatey1, mind, mindate, vdate))
					start = nums[n].start, end = nums[n + 1].end;
				if (FACEBOOK_CheckDate(v1, v0, vdatey1, mind, mindate, vdate))
					start = nums[n].start, end = nums[n + 1].end;
				if (FACEBOOK_CheckDate(v0, v1, vdatey2, mind, mindate, vdate))
					start = nums[n].start, end = nums[n + 1].end;
				if (FACEBOOK_CheckDate(v1, v0, vdatey2, mind, mindate, vdate))
					start = nums[n].start, end = nums[n + 1].end;
			}
			if (nums[3].val <= 0)
				break;
		}
		if (mindate <= 0)
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
			for (int i = 0; i < tmp.GetSize(); ++i)
				if (months.Find(tmp[i].id) < 0)
				{
					double m = tmp[i].GetNum(0);
					if (m < 1 || m>12)
						Log(LOGERR, "Invalid month %s=%s", tmp[i].id, CData(m));
					else
						months.Add(CSym(stripAccents2(tmp[i].id), CData(m)));
				}
		}

		// date & week
		{
			CSymList tmp;
			tmp.Load(filename(TRANSBASIC"DATE"));
			for (int i = 0; i < tmp.GetSize(); ++i)
			{
				if (tmp[i].id.GetLength() > 1)
				{
					// weekdays
					double n = tmp[i].GetNum(0);
					if (n < 0 || n>7)
						Log(LOGERR, "Invalid weekday %s=%s", tmp[i].id, tmp[i].data);
					else
						weekdays.Add(CSym(stripAccents2(tmp[i].id), CData(n)));
				}
				else
					if (tmp[i].id.GetLength() == 1)
					{
						// match
						vara list(tmp[i].data);
						for (int l = 0; l < list.length(); ++l)
							reldate.AddUnique(CSym(stripAccents2(list[l]), tmp[i].id));
					}
			}
		}

	}


	double FACEBOOK_GetMonth(const char *m)
	{
		for (int i = 0; i < months.GetSize(); ++i)
			if (IsSimilar(m, months[i].id))
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


		for (int f = 0; f < line.GetLength(); ++f)
		{
			int n = -1;
			f += countnotalphanum(line);
			if (IsMatchList((const char*)line + f, reldate, TRUE, &n))
			{
				vars m = reldate[n].id;
				int flen = strlen(m);
				if (f > 0 && isa(line[f - 1]))
					continue; // only full words

				int iw;
				double daynum = -1, dayago = -1;
				char mc = reldate[n].data[0];
				switch (mc)
				{
				case '0':
				case '1':
					daynum = mc - '0';
					break;

				case 'I': // ignore
					dayago = daynum = -1;
					break;

				case 'A':
				{
					static const char *numbers[] = { "today", "one", "two", "three", "four", "five", "six", "seven", NULL };
					int l;
					for (l = f - 1; l >= 0 && !isanum(line[l]); --l);
					for (; l >= 0 && isanum(line[l]); --l); ++l;
					if (l >= 0)
						if ((daynum = IsMatchNC(((const char *)line) + l, numbers)) > 0 || (daynum = CDATA::GetNum(((const char *)line) + l)) > 0)
							flen = f + flen - l, f = l;
				}
				break;

				case 'L':
				{
					// last sunday
					flen += countnotalphanum((const char *)line + f + flen);
					if (IsMatchList(((const char *)line) + f + flen, weekdays, TRUE, &iw, 3))
					{
						dayago = weekdays[iw].GetNum(0);
						// make full word
						while (isa(line[f + flen])) ++flen;
					}
				}
				break;

				case 'P':
				{
					// pasado domingo OR domingo pasado
					flen += countnotalphanum((const char *)line + f + flen);
					//if ((dayago = IsMatchNC(((const char *)line)+f+flen, weekdays))>=0)
					if (IsMatchList(((const char *)line) + f + flen, weekdays, TRUE, &iw, 3))
					{
						dayago = weekdays[iw].GetNum(0);
						// make full word
						while (isa(line[f + flen])) ++flen;
					}
					else
					{
						int l;
						for (l = f - 1; l >= 0 && !isa(line[l]); --l);
						for (; l >= 0 && isa(line[l]); --l); ++l;
						if (l >= 0 && IsMatchList(((const char *)line) + l, weekdays, TRUE, &iw, 3))
						{
							dayago = weekdays[iw].GetNum(0);
							flen = f + flen - l, f = l;
						}
					}
				}
				break;
				}

				if (dayago >= 0)
				{
					// adjust for weekend
					if (dayago >= 7)
						dayago = 5;
					// adjust for last 'day'
					int day = ((int)odate) % 7;  // 3
					daynum = day - dayago;  // 3 - 5 = -2
					if (daynum < 0) daynum += 7; // 5
				}

				if (daynum >= 0)
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
		if (f < 0) return FALSE;
		return (!isdigit(line[f + strlen(year)]) && (f == 0 || !isdigit(line[f - 1])));
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
		int i = 0;
		while (TRUE)
		{
			// ignore sep
			//while (strchr(sep, line[i]) && line[i]!=0)
			//	++i;

			if (isdigit(str[i]))
			{
				// found number
				nums[num].start = i;
				nums[num].val = CDATA::GetNum(str + i);
				while (isdigit(str[i]))
					++i;
				if (IsMatch(str + i, numignore))
				{
					FBLog(LOGINFO, FBDEBUG"Ignoring #' number in %.500s", str);
					continue;
				}
				if (IsMatch(str + i, numbering))
					while (isa(str[i]))
						++i;
				if (IsMatch(str + i, numof))
					while (isa(str[++i]))
						++i;
				while (strchr(sep, str[i]) && str[i] != 0)
					++i;

				nums[num].end = i;
				++num;
				if (num == MAXNUMS)
					goto datecheck;
				continue;
			}

			// ie: Dec 
			double m = FACEBOOK_GetMonth(str + i);
			if (m > 0)
			{
				// found month
				if (++nummonth > 1)
					goto datecheck; // too many months
				nums[num].start = i;
				nums[num].val = m;
				// August2016 ok
				while (!strchr(sep, str[i]) && !isdigit(str[i]) && str[i] != 0)
					++i;
				if (IsMatch(str + i, numof))
					while (isa(str[++i]))
						++i;
				while (strchr(sep, str[i]) && str[i] != 0)
					++i;
				nums[num].end = i;
				++num;
				if (num == MAXNUMS)
					goto datecheck;
				continue;
			}

			// ie: Jueves 30
			int iw;
			if (IsMatchList(str + i, weekdays, TRUE, &iw, 3))
			{
				int start = i;
				while (isa(str[i])) ++i;
				while (!isanum(str[i])) ++i;
				double dayn = CDATA::GetNum(str + i);
				if (dayn <= 0)
					continue;

				// skip num
				int end = i;
				while (isdigit(str[end])) ++end;

				// adjust for weekend
				double dayago = weekdays[iw].GetNum(0);
				if (dayago >= 7)
					dayago = 5;
				// adjust for last 'day'
				int day = ((int)odate) % 7;  // 3
				double daynum = day - dayago;  // 3 - 5 = -2
				if (daynum < 0) daynum += 7; // 5
				double ddate = odate - daynum;

				double ddayn = CDATA::GetNum(GetToken(CDate(ddate), 2, '-'));
				if (ddayn != dayn)
					continue;

				date = ddate;
				vars datestr = oline.Mid(start, end - start); oline.Delete(start, end - start); oline.Insert(start, DELDATE);
				decipher = MkString("'%s' = %s", datestr, CDate(date));
				return TRUE;
			}




		datecheck:
			// bla bla
			if (num >= 2)
			{
				// try to decipher
				int start = -1, end = -1;
				if (FACEBOOK_AdjustDate(num, nums, odate, date, start, end))
				{
					vars datestr = oline.Mid(start, end - start); oline.Delete(start, end - start); oline.Insert(start, DELDATE);
					decipher = MkString("'%s' = %s", datestr, CDate(date));
					return num > 2 ? 1 : -1;
				}
			}

			// skip words
			//if (++retry>10)
			//	break;
			num = nummonth = 0;
			while (!strchr(sep, str[i]) && !isdigit(str[i]) && str[i] != 0)
				++i;
			while (strchr(sep, str[i]) && str[i] != 0)
				++i;
			if (str[i] == 0)
				break;
		}


		// find 
		if (!IsSimilar(str, "http"))
			if (CheckYear(oline, "17") || CheckYear(oline, "16") || CheckYear(oline, "2017") || CheckYear(oline, "2016"))
				FBLog(LOGWARN, "? Missed date? '16/17'? '%.500s'", oline);

		// try relative
		return FACEBOOK_MatchRelativeDate(odate, oline, date, decipher);
	}

} FBDATE;




int FACEBOOK_MatchLink(const char *line, CSymList &matchlist)
{
	int match = 0;
	vara links(line, "href=");
	for (int i = 1; i < links.length(); ++i)
	{
		vars link = GetToken(links[i], 0, " >");
		vars url = url_decode(ExtractString(link.Trim(" '\""), "u=http", "", "&"));
		if (url.IsEmpty())
			continue;
		url = urlstr("http" + url);
		if (strstr(url, "ropewiki.com"))
			url.Replace(" ", "_");
		int f = titlebetalist.Find(url);

		if (f >= 0)
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
		if (adesc != desc)
			return FALSE;
		if (aaka != aka)
			return FALSE;
		return TRUE;
	}


	void AKA_Save(const char *file)
	{
		CSymList savelist;
		for (int i = 0; i < akalist.GetSize(); ++i)
		{
			CSym sym(akalist[i].sym->id, akalist[i].sym->GetStr(ITEM_DESC));
			sym.SetStr(ITEM_AKA, akalist[i].sym->GetStr(ITEM_AKA));
			sym.SetStr(ITEM_MATCH, akalist[i].aka.join(";"));
			savelist.Add(sym);
		}
		savelist.Save(file);
	}

	void AKA_Push(vara &aka, const char *str)
	{
		str = skipnotalphanum(str);
		if (strlen(str) < 3)
			return;
		if (aka.indexOf(str) >= 0)
			return;
		if (akacommonlist.Find(str) >= 0)
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
		vars st = stripAccents2(_str); st.Replace("%2C", ",");

		// substitution lists
		vars ostr = st;
		CSymList *sublists[] = { &mainsublist, &midsublist, &uplowsublist, &postsublist, &presublist, &shortsublist, &typosublist, &typo2sublist, NULL };
		for (int i = 0; sublists[i]; ++i)
		{
			vars str = FACEBOOK_SubstList(ostr, *(sublists[i]));
			if (i == 0 || str != ostr)
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
			AKA_Add(aka, parenthesis + vars(" ") + noparenthesis);
			AKA_Add(aka, noparenthesis + vars(" ") + parenthesis);
		}
		else
		{
			// AND / OR
			int subst = 0;

			//Michelle: this is duplicated from BETAExtract.cpp
			const char *andstring[] = { " and ", " y ", " et ", " e ", " und ", "&", "+", "/", ":", " -", "- ", NULL };

			for (int s = 0; andstring[s] != NULL; ++s)
				if (st.Replace(andstring[s], ";"))
					++subst;
			if (subst)
			{
				vara stlist(st, ";");
				for (int i = 0; i < stlist.length(); ++i)
					AKA_Add(aka, stlist[i]);
			}
		}
	}

	void AKA_Load(vara &aka, CSym &sym)
	{
		AKA_Add(aka, sym.GetStr(ITEM_DESC));
		vara akalist(sym.GetStr(ITEM_AKA), ";");
		for (int i = 0; i < akalist.length(); ++i)
			AKA_Add(aka, akalist[i]);
#ifdef DEBUGXXX
		if (strstr(sym.data, "Lytle"))
		{
			Log(LOGINFO, "%s", sym.GetStr(ITEM_DESC));
			for (int i = 0; i < aka.aka.length(); ++i)
				Log(LOGINFO, "#%d %s", i, aka.aka[i]);
			i = i + 1;
		}
#endif
		// set full region	
	}



	vars FACEBOOK_SubstList(const char *str, CSymList &list)
	{
		vars line = " " + vars(str) + " ";
		for (int i = 0; i < list.GetSize(); ++i)
			iReplace(line, list[i].id, list[i].data);

		while (line.Replace("  ", " "));
		return line.Trim(" .,;");
	}



#define PUNCTCHARS ".:,;/!?@#$%^&*=+//[]{}<>"

	int FACEBOOK_MatchName(int &l, vars &lmatch, const char *matchline, const char *matchname, const char *strflags)
	{
		matchline = skipnotalphanum(matchline);
		if (*matchline == 0)
			return 0;
		ASSERT(isanum(matchline[0]));
		ASSERT(isanum(matchname[0]));

		//ASSERT(!(IsSimilar(matchline, "barrancomascun") && IsSimilar(matchname, "barranco mascun")));

		// best match on simple name
		int perfects[2] = { 0, 0 };
		//const char *matchsep = " -()"PUNCTCHARS;
		int lold = l;
		int lnew = BestMatchSimple(matchname, matchline, perfects);
		if (lnew < MINCHARMATCH || lnew <= lold)
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
		if (perfects[0] < MINCHARMATCH || perfects[0] <= lold)
			return 0;
		// accept full match only
		if (perfects[0] < (int)strlen(matchname))
			return 0;

		int lpnew = perfects[0];

		// avoid unwanted matches (ALL common words, empty meaning)
		//if (FACEBOOK_CommonNames(matchname, mainname)>=perfects[0])
		//	return 0;
		if (IsMatchList(matchname, commonlist) == perfects[0])
			return 0;
		if (lpnew == MINCHARMATCH && !IsSimilar(matchline, matchname))
			return 0;

		//const char *matchnamell = skipblanks(matchname+ll);
		const char *postmatch = skipblanks(matchline + perfects[1]);

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
		if (nn < 2)
		{
			// if not MATCH MORE, skip single words unless 
			if (strflags && !FCHK(strflags, F_MATCHMORE))
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
		if (line1 != line2 && pline->indexOf(line2) < 0)
			pline->push(line2);

		// barranco X inferior vs barranco X
		vars line3 = FACEBOOK_SubstList(line2, uplowsublist);
		if (line2 != line3 && pline->indexOf(line3) < 0)
			pline->push(line3);

		vars line4 = FACEBOOK_SubstList(line3, uplowsublist2);
		if (line3 != line4 && pline->indexOf(line4) < 0)
			pline->push(line4);

		// process hashtags: barrancodelabolulla vs delabolulla vs bolulla
		int len = 0;
		while ((len = IsMatchList(line, prelist, FALSE)) > 0 && isanum(line[len]))
			pline->push(line += len);
		while ((len = IsMatchList(line, midlist, FALSE)) > 0 && isanum(line[len]))
			pline->push(line += len);
	}


	int FACEBOOK_MatchLine(const char *linestr, CSymList &matchlist, vara &matchregions, const char *strflags, int retry)
	{
		linestr = skipnotalphanum(linestr);
		if (!linestr || *linestr == 0)
			return FALSE;

		int maxl = MINCHARMATCH;
		// try taking out the prefix

		vara pline;
		FACEBOOK_AddLine(&pline, linestr);	//FACEBOOK_ScanLine(linestr, &pline, hash);

		// scan name database
		CSymList matched;
		for (int i = 0; i < akalist.GetSize(); ++i)
		{
			int l = 1;
			vars lmatch;
			CSym &sym = *akalist[i].sym;
			vara &aka = akalist[i].aka;

			// region block
			//if (FCHKstrflags,F_REGSTRICT) || FCHK(strflags,F_GROUP))
			if (matchregions.length() > 0)
			{
				vars fullregion = sym.GetStr(ITEM_REGION);
				// force to belong to a specific region set
				BOOL matched = matchregions[0].IsEmpty();
				for (int r = 0; r < matchregions.length() && !matched; ++r)
					if (strstri(fullregion, matchregions[r]))
						matched = TRUE;
				if (!matched)
					continue;
			}

			// must match partially
			for (int a = 0; a < aka.length(); ++a) // aka
				for (int p = 0; p < pline.length(); ++p) // plines
					FACEBOOK_MatchName(l, lmatch, pline[p], aka[a], strflags);

			if (l >= maxl)
			{
				if (l > maxl)
				{
					// reset list for better matches
					maxl = l;
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

		if (matched.GetSize() == 0)
			return FALSE;

		for (int i = 0; i < matched.GetSize(); ++i)
		{
			int found = matchlist.Find(matched[i].id);
			if (found < 0 || matched[i].GetStr(M_MATCH) != matchlist[found].GetStr(M_MATCH))
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

		if (*line != 0)
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
		for (int r = 0; r < 3; ++r)
		{
			extra = skipblanks(skipalphanum(extra));
			if (*extra == '(')
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
		int maxretry = FCHK(strflags, F_MATCHMORE) ? 5 : 2;

		// try N first Capitalized words: me and erl gave a shot at Eaton Canyon
		for (const char *linescan = skipnotalphanum(linestr), *nextscan = NULL; *linescan != 0 && retry < maxretry; linescan = nextscan, ++words)
		{
			int skipretry = FALSE;
			int scan = scannext || words < 3; scannext = FALSE;

			// find next word (and special symbols coming up)
			const char *linescanend;
			for (linescanend = linescan; isanum(*linescanend); ++linescanend); // skip alpha
			for (nextscan = linescanend; *nextscan != 0 && !isanum(*nextscan); ++nextscan) // skip not alpha
				if (strchr(scanchr, *nextscan))
					++scannext;

			// end scan at ignorelist words
			if (IsMatchList(linescan, ignorelistpre))
				return;


			// try ALL pre or mid words and do not count retries (more restrictive with mid bc shorter)
			if (IsMatchList(linescan, prelist, FALSE) || IsMatchList(linescan, midlist, TRUE))
				scan++, scannext++, skipretry = TRUE;

			// try all the words in the line
			if (!scan && FCHK(strflags, F_MATCHANY))
				scan++;

			// try uppercase words
			if (!scan && isupper(*linescan))
				scan++;

			// try numbers (7 Teacups 12 cascadas)
			if (!scan && isdigit(*linescan) && *linescanend == ' ')
				scan++;

			// try X Creek or X Canyon
			if (!scan && IsMatchList(nextscan, canyonlist))
				scan++;

			// try startlist word // SHOULD SKIP, BUT JUST TO BE SURE WE DONT SKIP ANYTHING IMPORTANT! ie:"Running springs"
			if (IsMatchList(linescan, startlist))
				scan++, scannext++, skipretry = TRUE;

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
		if (*main == 0 || *match == 0)
			return FALSE;

		vars smain = vars(main).replace(" ", "  ");
		vars smatch = vars(match).replace(" ", "  ");
		switch (cmp)
		{
		case C_EQUAL:
			return stricmp(main, match) == 0 || stricmp(smain, smatch);
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
		if (rwlist.GetSize() == 0)
			LoadRWList();

		if (akalist.GetSize() > 0)
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
		for (int i = 0; i < akalist.GetSize(); ++i)
		{
			CSym &sym = rwlist[i];
			akalist[i].sym = &sym;
			printf("%d/%d building AKA list...        \r", i, akalist.GetSize());

			int f = loadlist.Find(sym.id);
			if (f < 0 || !AKA_IsSaved(sym, loadlist[f]))
				f = -1;
			if (f < 0)
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
		for (const char *pr = skipnotalphanum(prdesc); *pr != 0; pr = skipnotalphanum(skipalphanum(pr)))
			pline.push(pr);

		for (int a = 0; a < aka.length(); ++a) // aka
			for (int p = 0; p < pline.length(); ++p) // plines			
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
		if (f >= 0)
			aka = akalist[f].aka;
		else
			AKA_Load(aka, sym);

		// allow no common words
		//vars name = sym.GetStr();
		for (int i = 0; i < aka.length(); ++i)
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
	vara symaka(sym.GetStr(ITEM_AKA), ";");
	for (int a = 0; a < symaka.length(); ++a)
		namelist.Add(CSym(symaka[a].Trim()));
	namelist.Sort(cmplength);

#endif



	int FACEBOOK_CommonNames(const char *matchname, CSymList *mainname = NULL, BOOL noalpha = TRUE)
	{
		int len;
		const char *ptr = matchname;
		while ((len = IsMatchList(ptr, commonlist, noalpha)) > 0)
		{
			if (ptr > matchname)
				if (mainname && IsMatchList(ptr, *mainname, noalpha))
					break;
			ptr = skipnotalphanum(ptr + len);
		}
		return ptr - matchname;
	}

	double FACEBOOK_Score(CSym &sym, int retry, const char *line, const char *match, vara &prefregions)
	{
		int len = strlen(match);
		//int nwords = vara(match, " ").length();
		vars name = vars(sym.GetStr(ITEM_DESC));

		// initial score
		int commlen = FACEBOOK_CommonNames(match);
		double score = (len - commlen / 2) / (retry + 1);

		// advantage for region mentioned in line
		vara matchregions(_GeoRegion.GetFullRegion(sym.GetStr(ITEM_REGION)), ";");
		for (int r = 0; r < matchregions.length(); ++r)
			if (prefregions.indexOfi(matchregions[r]) >= 0)
				score += score / 2; // 50% more

		// advantage for perfect title match
		//if (strlen(match)<=len) 
		//	score += 1; 

		// advantage for simplified title match
		vars omatch = match, oname = name, oline = line;
		CSymList *sublists[] = { &mainsublist, &midsublist, &postsublist, &presublist, &shortsublist, &typosublist, &typo2sublist, NULL };
		for (int i = 0; sublists[i]; ++i)
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
			score = (score + scoreadd / (i + 1)) / scorediv;
		}

		return score * 100;
	}



	int FACEBOOK_Match(const char *_line, CSymList &reslist, const char *region, const char *strflags)
	{
		_line = skipnotalphanum(_line);
		if (*_line == 0)
			return FALSE;

		FACEBOOK_InitAKA();

		vars line = FACEBOOK_SubstList(_line, mainsublist);

		// try to find possible matches
		CSymList matchlist;
		vara prefregions(region, ";");
		FACEBOOK_MatchLists(line, matchlist, prefregions, strflags);
		if (matchlist.GetSize() == 0) // no match
			return FALSE;

		if (!FCHK(strflags, F_MATCHMANY))
		{
			// maximum match
			vara equiv;
			double maxe = -1;
			matchlist.SortNum(M_SCORE, -1);
#ifdef DEBUG
			if (matchlist.GetSize() > 0)
				for (int i = 0; i < matchlist.GetSize(); ++i)
					Log(LOGINFO, "     BM#%d %s", i, GetMatch(matchlist[i]));
#endif
			for (int i = 0; i < matchlist.GetSize(); ++i)
			{
				CSym &sym = matchlist[i];
				double e = sym.GetNum(M_SCORE);
				if (e < maxe)
				{
					matchlist.Delete(i--);
					continue;
				}
				maxe = e;
				vars match = " " + sym.GetStr(ITEM_DESC) + " ";
				for (int j = 0; equivlist[j] != NULL; ++j)
					iReplace(match, equivlist[j], " ");
				while (match.Replace("  ", " "));
				if (equiv.indexOfi(match) < 0)
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

			if (equiv.length() > 1)
			{
				vars wlist;
				int MAXOVERMATCH = 10;
				// report overmatch
				Log(LOGWARN, "FBOVERMATCHED '%.256s' [%s,%s... #%d]", line, equiv[0], equiv[1], equiv.length());
				matchlist.SortNum(ITEM_STARS, -1);
				for (int i = 0; i < MAXOVERMATCH && i < matchlist.GetSize(); ++i)
					Log(LOGWARN, "OM#%d %s", i, GetMatch(matchlist[i]));
				//ASSERT(!strstr(linestr, "Barranco del chorro"));
				//ASSERT(!strstr(linestr, "Parker Creek Canyon"));
				if (PICMODE)
				{
					// report overmatches
					for (int i = 0; i < MAXOVERMATCH && i < matchlist.GetSize(); ++i)
						reslist.Add(matchlist[i]);
					return -1;
				}
				// pick best canyon if 3 overmatched
				if (matchlist.GetSize() <= 3)
				{
					reslist.Add(matchlist[0]);
					return -1;
				}
				return FALSE;
			}
		}

		reslist.Add(matchlist);
		return matchlist[0].GetNum(M_RETRY) > 0 ? -1 : 1;
	}


	vara FACEBOOK_CheckUsed(vara &usedlist, CSymList &matchlist)
	{
		vara dellist;
		if (usedlist.length() > 0)
		{
			// already used date, delete any canyon that does not match
			for (int m = 0; m < matchlist.GetSize(); ++m)
			{
				vars title = matchlist[m].GetStr(ITEM_DESC);
				if (usedlist.indexOf(title) < 0)
				{
					dellist.push(title);
					matchlist.Delete(m--);
				}
			}
		}
		else
		{
			// first time using it
			for (int m = 0; m < matchlist.GetSize(); ++m)
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

		while (*(text = skipnotalphanum(text)) != 0)
		{
			int m;
			// region must start with Upper case and be in the database
			if (IsMatchList(text, reglist, TRUE, &m))
				if (list.indexOfi(reglist[m].id) < 0)
				{
					vars reg = reglist[m].id;
					if (!reglist[m].data.IsEmpty())
						reg = reglist[m].data;
					if (reg == "X")
						continue; // skip some regions 'Como'

					list.push(reg);
					FBLog(LOGINFO, FBDEBUG"Detected region '%s' <= '%.30s...'", reg, text);
				}
			text = skipalphanum(text);
		}
		vara more(preregions, ";");
		for (int i = 0; i < more.length(); ++i)
			if (list.indexOfi(more[i]) < 0)
				list.push(more[i]);

		return list.join(";");
	}


private:



	void PrepSublist(CSymList &list, const char *def = NULL)
	{
		CSymList newlist;
		for (int i = 0; i < list.GetSize(); ++i)
		{
			vars id = " " + stripAccents2(list[i].id).Trim() + " ";
			while (id.Replace("  ", " "));
			vars data = " " + stripAccents2(list[i].data).Trim() + " ";
			while (data.Replace("  ", " "));
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
		if (mainsublist.GetSize() == 0)
		{
			CSymList basic, list, sublist, sublistp;

			// main synonims = standardized comparisons
			list.Load(filename(TRANSBASIC"PRE"));
			for (int i = 0; i < list.GetSize(); ++i)
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
			for (int i = 0; blanks[i] != 0; ++i)
				sublistp.AddUnique(CSym(CString(blanks[i]), " "));
			const char *punct = PUNCTCHARS;
			for (int i = 0; punct[i] != 0; ++i)
				sublistp.AddUnique(CSym(CString(punct[i]), " " + CString(punct[i]) + " "));
			sublistp.AddUnique(CSym("&#8217;", "'"));
			sublistp.AddUnique(CSym("&#8217", "'"));
			sublistp.Sort(cmplength);

			//ignorelistpre.Load(filename("facebookignorepre"));
			//PrepSublist(ignorelistpre, "XXXX");

			// inferiore, superiore, etc.
			mainsublist.Add(sublistp);
			mainsublist.Add(sublist);
			mainsublist.Add(ignorelistpre);

			// uplow list (Upper) (Lower)
			mainsublist.AddUnique(CSym(")", " "));
			mainsublist.AddUnique(CSym("(", " "));

			// uplow list for hash
			for (int i = 0; i < basic.GetSize(); ++i)
			{
				vars id = basic[i].id; id.Trim();
				vars data = basic[i].data; data.Trim(" ()");
				if (id.GetLength() >= 5)
					uplowsublist2.AddUnique(CSym(id, " " + data));
			}

			PrepSublist(sublist, " ");
			uplowsublist.AddUnique(sublist);
		}
		if (midsublist.GetSize() == 0)
		{
			midsublist.Load(filename(TRANSBASIC"MID"));
			// extend with Devil's or "The Force"
			midsublist.AddUnique(CSym("'"));
			midsublist.AddUnique(CSym("\""));
			for (int i = 0; i < midsublist.GetSize(); ++i)
				midsublist[i].data = " ";
			//PrepSublist(midsublist, " ");
		}
		if (postsublist.GetSize() == 0)
		{
			postsublist.Load(filename(TRANSBASIC"POST"));
			PrepSublist(postsublist, " ");
		}
		if (presublist.GetSize() == 0)
		{
			presublist.Load(filename(TRANSBASIC"PRE"));
			PrepSublist(presublist, " ");
		}
		if (shortsublist.GetSize() == 0)
		{
			shortsublist.Load(filename(TRANSBASIC"SHORT"));
			PrepSublist(shortsublist);
		}
		if (typosublist.GetSize() == 0)
		{
			typosublist.Load(filename(TRANSBASIC"TYPO"));
			//typosublist.Sort(cmplength);
		}
		if (typo2sublist.GetSize() == 0)
		{
			typo2sublist.Load(filename(TRANSBASIC"TYPO2"));
			//typo2sublist.Sort(cmplength);
		}
		// IsMatchList lists
		if (prelist.GetSize() == 0)
		{
			prelist.AddUnique(presublist);
			PrepMatchList(prelist);
		}
		if (midlist.GetSize() == 0)
		{
			midlist.AddUnique(midsublist);
			PrepMatchList(midlist);
		}
		if (canyonlist.GetSize() == 0)
		{
			const char *list[] = { "Canyon", "Creek", "Falls", "caudal", NULL };
			for (int i = 0; list[i] != NULL; ++i)
				canyonlist.AddUnique(CSym(list[i]));
			PrepMatchList(canyonlist);
		}
		if (startlist.GetSize() == 0)
		{
			CSymList list;
			list.Load(filename("facebookstart"));
			for (int i = 0; i < list.GetSize(); ++i)
				startlist.AddUnique(CSym(list[i].id));
			startlist.Add(CSym(HASHTAG));
			PrepMatchList(startlist);
		}
		if (endlist.GetSize() == 0)
		{
			CSymList list;
			list.Load(filename("facebookend"));
			for (int i = 0; i < list.GetSize(); ++i)
				endlist.AddUnique(CSym(list[i].id));
			endlist.AddUnique(postsublist);
			for (int i = 0; equivlist[i] != NULL; ++i)
				endlist.AddUnique(CSym(equivlist[i]));
			PrepMatchList(endlist);
		}
		if (ignorelistpre.GetSize() == 0)
		{
			ignorelistpre.Load(filename("facebookignorepre"));
			PrepMatchList(ignorelistpre);
		}
		if (ignorelistpost.GetSize() == 0)
		{
			ignorelistpost.Load(filename("facebookignorepost"));
			PrepMatchList(ignorelistpost);
		}
		if (reglist.GetSize() == 0)
		{
			// set up proper region detections
			CSymList redir;
			redir.Load(filename(RWREDIR));
			redir.Load(filename("facebookredir"));

			// main regions
			CSymList &regionlist = _GeoRegion.Regions();
			for (int i = 0; i < regionlist.GetSize(); ++i)
				//if (!IsMatchList(regionlist[i].id, commonlist))
			{
				vars oreg = regionlist[i].id;
				reglist.Add(CSym(oreg));
				// synonims automatic list
				vars sreg = stripAccents2(oreg).Trim();
				if (sreg != oreg)
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
			for (int i = 0; i < redir.GetSize(); ++i)
			{
				vars name = redir[i].GetStr(ITEM_DESC);
				int f = regionlist.Find(name);
				if (f < 0) continue;

				// set up synonims
				vara aka(redir[i].GetStr(ITEM_AKA), ";");
				for (int a = 0; a < aka.length(); ++a)
					//if (!IsMatchList(aka[a], commonlist))
					reglist.AddUnique(CSym(aka[a], name));
			}

			PrepMatchList(reglist);
		}

		if (commonlist.GetSize() == 0)
		{
			CSymList list;
			list.Load(filename("facebookcommon"));
			for (int i = 0; i < list.GetSize(); ++i)
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
	int ret = FBMATCH.FACEBOOK_Match(stripAccents2(linestr), reslist, region, F_MATCHMORE F_MATCHMANY) > 0;

	for (int i = 1; i < reslist.GetSize(); ++i)
		if (reslist[i].GetStr(ITEM_DESC) == reslist[i - 1].GetStr(ITEM_DESC))
			reslist.Delete(i--);


	return reslist.GetSize() > 0;
}



int FACEBOOK_MatchWater(const char *line, vars &water, const char *label = NULL)
{
	//enum { W_DRY=0, W_WADING=1, W_SWIMMING=2, W_VERYLOW=3, W_LOW=4, W_MODLOW=5, W_MODERATE=6, W_MODHIGH=7, W_HIGH=8, W_VERYHIGH=9, W_EXTREME=10 };


	int n = -1;
	static CSymList waterlev;
	if (waterlev.GetSize() == 0)
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
			while (*tomatch != 0 && isa(*tomatch))
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
	if (!(v = GetToken(summary[R_STARS], 0, '-')).IsEmpty())
		out.push("*:" + v.Trim());
	if (!(v = GetToken(summary[R_T], 0, '-')).IsEmpty())
		out.push("D:" + v.Trim());
	if (!(v = GetToken(summary[R_W], 0, '-')).IsEmpty())
		out.push("W:" + v.Trim());
	if (!(v = GetToken(summary[R_TEMP], 0, '-')).IsEmpty())
		out.push("MM:" + v.Trim());
	if (!(v = GetToken(summary[R_TIME], 0, '-')).IsEmpty())
		out.push("HR:" + v.Trim());
	if (!(v = GetToken(summary[R_PEOPLE], 0, '-')).IsEmpty())
		out.push("PP:" + v.Trim());
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
	if (matchnum >= 0)
		orating[R_T] = cond_diff[matchnum], ++matched;

	//static vara cond_temp("0 - None,1 - Rain jacket,2 - Thin wetsuit,3 - Full wetsuit,4 - Thick wetsuit,5 - Drysuit");
	static varas mtempstrstr("15MM=4,14MM=4,13MM=4,12MM=4,11MM=4,10MM=4,0MM=0,1MM=1,2MM=1,3MM=2,4MM=2,5MM=3,6MM=3,7MM=4,8MM=4,9MM=4,XMM=5");
	if (match = mtempstrstr.strstr(rating[R_TEMP] + "MM"))
		orating[R_TEMP] = cond_temp[atoi(match)], ++matched;

	//static vara cond_stars("0 - Unknown,1 - Poor,2 - Ok,3 - Good,4 - Great,5 - Amazing");
	double num = CDATA::GetNum(rating[R_STARS]);
	if (num >= 0 && num < 6)
		orating[R_STARS] = cond_stars[(int)min(5, ceil(num))], ++matched;

	if ((num = CDATA::GetNum(rating[R_TIME])) > 0)
		orating[R_TIME] = rating[R_TIME], ++matched;

	if ((num = CDATA::GetNum(rating[R_PEOPLE])) > 0)
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
	if (pdate < 0 && datea.length() >= 3)
		pdate = CheckDate(CDATA::GetNum(datea[1]), CDATA::GetMonth(datea[0]), CDATA::GetNum(datea[2], year));
	if (pdate < 0 && datea.length() >= 1 && IsSimilar(datea[0], "Yesterday"))
		pdate = RealCurrentDate - 1;
	if (pdate < 0 && datea.length() >= 2 && CDATA::GetMonth(datea[0])>0)
		pdate = CheckDate(CDATA::GetNum(datea[1]), CDATA::GetMonth(datea[0]), CDATA::GetNum(CDate(RealCurrentDate)));
	if (pdate < 0 && datea.length() >= 2 && CDATA::GetNum(datea[0]) >= 0)
		if (strchr(hrmin, datea[1][0]))
			pdate = RealCurrentDate;
	if (pdate < 0 && datea.length() >= 2 && datea[0] == "Just")
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
	if (link && *link != 0)
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
	if ((a = atext.Find(url)) >= 0)
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

	vars titletext = "<FBTITLE:" + title + ">";
	titletext += "<FBURL:" + url + ">";
	titletext += "<FBLIKE:" + likelink + ">";
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
		err = "(nodate)";
		return FALSE;
	}
	if (cdate > pdate + 1)
	{
		err = MkString("FUTURE DATE %s>%s", CDate(cdate), CDate(pdate));
		return FALSE;
	}
	if (cdate < pdate - MAXCDATE)
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
	if (fgu < 0)
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
		if (glinks.indexOf(fblink) < 0)
		{
			glinks.push(fblink);
			user.SetStr(FBL_GLINKS, glinks.join(";"));
		}
		vara gnames(user.GetStr(FBL_GNAMES), ";");
		if (gnames.indexOf(fbname) < 0)
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
	if (u >= 0)
	{
		vara list(fbgroupuserlist[u].GetStr(FBL_GLINKS), ";");
		for (int l = 0; l < list.length(); ++l)
		{
			int g = fbgrouplist.Find(fburlstr(list[l]));
			if (g < 0)
			{
				// complain if link is not in group list
				FBLog(LOGWARN, "Mismatched groupuser for groupuser '%s' grouplink '%s'", user, list[l]);
				continue;
			}
			vara grlist(fbgrouplist[g].GetStr(FBL_REGION), ";");
			for (int l = 0; l < grlist.length(); ++l)
				if (rlist.indexOf(grlist[l]) < 0)
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
	if (year == 0)
		year = CDATA::GetNum(CDate(RealCurrentDate));

	static CSymList translist;
	if (translist.GetSize() == 0)
		translist.Load(filename("facebooktrans"));

	const char* truncate[] = { ">More</", ">See Translation</", ">See More Photos</", ">Play Video</", ">Suggested Groups</", ">Friend Requests</", ">People You May Know</", NULL };

	double maxpdate = -1;
	vara list(memory, "<div><div><h3"); // main stories
	for (int i = 1; i < list.length(); ++i)
	{
		// cleanup story
		const char *memory = strchr(list[i], '>') + 1;

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
		if (pdate <= 0)
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
			if (*PICMODE == 0 && !strstr(memory, "ropewiki.com/"))
			{
				// skip if user did not authorize pics and not mentioning ropewiki
				int f = fbuserlist.Find(GetToken(fburlstr(userlink), 0, '?'));
				if (f < 0 || !strchr(fbuserlist[f].GetStr(FBL_FLAGS), 'P'))
					continue;
			}
		}

		double cdate = pdate;
		vara list2(list[i], "<div><h3"); // secondary stories

		for (int i2 = 0; i2 < list2.length(); ++i2)
		{
			vars h3 = ExtractString(list2[i2], ">", "", "</h3");
			vars text = ExtractString(list2[i2], "</h3", ">", "<abbr");

			// cut it short
			for (int t = 0; truncate[t]; ++t)
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
			text.Replace("<br", "<p");
			text.Replace("</br", "<p");
			text.Replace("</p", "<p");
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
			for (int i = 0; i < htmllines.length(); ++i)
			{
				vars line = stripHTML(htmllines[i].replace(">#<", ">" HASHTAG "<")).Trim();
				if (line.IsEmpty() || IsSimilar(line, "http"))
					continue;
				// add '. ' separator
				if (!posttext.IsEmpty())
					if (posttext[strlen(posttext) - 1] != '.')
						posttext += ".";
				posttext += " " BR + line;
			}

			// add albums titles
			const char *albumsep[] = { " album: <a", "<a", NULL };
			for (int a = 0; albumsep[a] != NULL; ++a)
			{
				vara albums(h3, albumsep[a]);
				for (int l = 1; l < albums.length(); ++l)
				{
					vars albumlink = ExtractHREF(albums[l]);
					vars album = stripHTML(ExtractString(albums[l], ">", "", "</a"));
					if (album.IsEmpty() || albumlink.IsEmpty())
					{
						Log(LOGWARN, "empty album title from url %s", urllink);
						continue;
					}

					if (albumsep[a][0] == '<')
						if (!strstr(albumlink, "/albums/"))
							continue; // skip nonalbums for general case

					// insert album title
					//if (htmllines.indexOf(album)<0)
					//	htmllines.InsertAt(1, album);

					// insert album title : description
					vars txt = MFACEBOOK_GetAlbumText(f, albumlink);
					if (htmllines.indexOf(txt) < 0)
						htmllines.InsertAt(0, txt);
				}
			}

			// process text
			vara olines;
			vara lines = htmllines;
			for (int l = 0; l < lines.length(); ++l)
			{
				// transform text
				vars &line = lines[l];

				// eliminate profiles
				vars aref;
				while (!ExtractStringDel(line, "href=\"/hashtag", "", ">", FALSE, -1).IsEmpty());
				while (!ExtractStringDel(line, "href=\"/", "", "</a>", FALSE, -1).IsEmpty());

				line = stripAccents2(stripHTML(line));
				if (IsSimilar(line, "http"))
					line = "";
				if (line.IsEmpty())
					continue;

				// original lines
				if (l > 0)
					olines.push(line);

				// translate line
				for (int t = 0; t < translist.GetSize(); ++t)
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
			FBMatch llist[MAXFBMATCH + 1];

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
			for (int nl = 0; nl < lines.length(); ++nl)
				if (!lines[nl].IsEmpty())
				{
					vars line = " " + lines[nl] + " ";
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
					if (nlist < MAXFBMATCH)
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
				for (int n = 0; n < nlist; ++n)
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
					if (used > 0)
					{
						if (mm->matchname < 0)
						{
							FBLog(LOGWARN, FBDEBUG"L#? %s : '%s' %s <= '%.500s'=>", "SHAKY#2NAME", user, mm->decipher, stripHTML(mm->htmlline));
							continue;
						}
						if (md->matchdate < g.matchdate)
						{
							FBLog(LOGWARN, FBDEBUG"L#? %s : '%s' %s <= '%.500s'=>", "SHAKY#2DATE", user, md->decipher, stripHTML(md->htmlline));
							continue;
						}
					}

					// OUTPUT FBSYMs
					++used;
					for (int m = 0; m < mm->matchlist.GetSize(); ++m)
					{
						// fb mapping
						vars date = CDate(md->cdate);
						CSym &msym = mm->matchlist[m];
						vars title = msym.GetStr(ITEM_DESC);


						CSym sym("Conditions:" + title + "-" + user.replace(" ", "") + "@Facebook-" + date, title);
						sym.SetStr(FB_DATE, "\'" + date);
						sym.SetStr(FB_NAME, title);
						sym.SetStr(FB_AKA, msym.GetStr(ITEM_AKA));
						sym.SetStr(FB_RWID, msym.id);
						sym.SetStr(FB_REGION, invertregion(msym.GetStr(ITEM_REGION)).replace(";", "."));
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
							fbtitle = strstr(fburl, "/posts/") ? user + "'s timeline" : fbname;
						vars fbtext = posttext;
						if (fbtext.IsEmpty())
							fbtext = stripHTML(mm->htmlline);
						fbtext = fbtext.replace("\"", "'");
						if (fbtext.GetLength() > FBMAXTEXT)
							fbtext = fbtext.Mid(0, FBMAXTEXT) + "...";

						vars fbuser = user.replace(";", " ");
						fbtitle = fbtitle.replace(";", " ");

						sym.SetStr(FB_TEXT, fbtext.replace(";", FBCOMMA));
						sym.SetStr(FB_SIGN, "[" + fburl + " " + fbtitle + "]");

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

						if (md->matchdate > 0)
							sym.SetStr(FB_SUREDATE, "X");
						if (mm->matchlink > 0 || mm->matchname > 0)
							sym.SetStr(FB_SURELOC, "X");
						if (ms->matchsummary && !mm->summary[R_W].IsEmpty())
							sym.SetStr(FB_SURECOND, "X");

						vars sum = GetFBSummary(ms->summary);
						FBLog(LOGINFO, FBDEBUG"FBC DATE [%s] --- SUM [%s] <='%s'", md->decipher, sum, ms->summary[R_SUMMARY]);
						Log(LOGINFO, ">>>" + (used > 1 ? MkString("#%d", used) : "") + " SYM(%d):\"%s\" <='%.500s'=> DATE(%d):%s SUM(%d):[%s] REG:[%s] by '%s@%s' url:%s", mm->matchname, title, fbtext, md->matchdate, date, ms->matchsummary, sum, matchregions, user, fbtitle, fburl);


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
				text.Replace("\"", "'");
				if (text.GetLength() > FBMAXTEXT)
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
				for (int i = 0; warn[i] != NULL; ++i)
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

	if (INVESTIGATE > 0)
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
	for (u = 0; u < umax && retry < 3 && !url.IsEmpty(); ++u)
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
		for (int mode = 0; seemores[mode] != NULL && more.IsEmpty(); ++mode)
			more = fburlstr(ExtractLink(f.memory, seemore = seemores[mode]));
		if (more.IsEmpty())
		{
			if (morelist.length() == 0)
			{
				vara list(f.memory, "<div class=\"i\"");
				for (int i = 1; i < list.length(); ++i)
					morelist.push(fburlstr(ExtractHREF(list[i])));
			}
			if (morelistn < morelist.length())
				more = morelist[morelistn++];
		}

		vars memory = ExtractString(f.memory, "<body", "", seemore);
		postdate = MFACEBOOK_ProcessPage(f, fbsymlist, url, memory, ubase, title, strregions, strflags);
		if (postdate > 0 && postdate < minpostdate)
			break;

		url = more;
	}

	if (retry >= 3)
		Log(LOGERR, "====================== DOWNLOAD ERROR FOR %s PG#%d ====================", name, u);

	return TRUE;
}



int FACEBOOK_DownloadList(DownloadFile &f, CSymList &symlist, const char *ourl, const char *match, const char *start, const char *end, const char *region = NULL, const char *flags = NULL)
{
	// check proper urls
	for (int i = 0; i < symlist.GetSize(); ++i)
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
		for (int i = 1; i < friends.length(); ++i)
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
			if (found < 0)
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

static int cmpgeoregion(const void *arg1, const void *arg2)
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
	if (a != 0)
		return a;
	/*
	int s = -strcmp(s1, s2);
	if (s!=0)
		return s;
	*/
	return 0;
}

int invcmpconddate(const void *arg1, const void *arg2)
{
	return ((CSym**)arg1)[0]->GetStr(ITEM_CONDDATE).Compare(((CSym**)arg2)[0]->GetStr(ITEM_CONDDATE));
}


int FACEBOOK_TestLine(int MODE, const char *_line, const char *result = NULL)
{
	const char *strflags = MODE == 0 ? "+G" : "";
	const char *strregions = MODE == 0 ? "Europe" : "";

	static CSymList translist;
	if (translist.GetSize() == 0)
		translist.Load(filename("facebooktrans"));

	vars line = stripAccents2(_line);
	line.Replace(BR, ".");
	// translate line
	for (int t = 0; t < translist.GetSize(); ++t)
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
		if (matchlist.GetSize() > 0)
		{
			if (matchlist[0].GetStr(ITEM_DESC) != result)
				alert = MkString("%s", result);
		}
		else
		{
			if (*result != 0)
				alert = MkString("(empty)", result);
		}
	}
	Log(alert.IsEmpty() ? LOGINFO : LOGERR, ">>> SYM(%d):'%s' DATE(%d):%s SUM(%d):%s REG:[%s]", matchname, matchname ? matchlist[0].GetStr(ITEM_DESC) : "", matchdate, matchdate ? decipher : "", matchsum, matchsum ? GetFBSummary(summary) : "", matchregions);
	matchname = matchname + 1;
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
		for (int i = 0; i < tmp.GetSize(); ++i)
		{
			CSym &sym = tmp[i];
			vars name = sym.GetStr(FBL_NAME);
			vara gllist(sym.GetStr(FBL_GLINKS), ";");
			vara gnlist(sym.GetStr(FBL_GNAMES), ";");
			for (int g = 0; g < gllist.length(); ++g)
				FACEBOOK_LogUserGroup(sym.id, gllist[g], name, gnlist[g]);
		}
		fbgroupuserlist.Save(filename(FBGROUPUSERS));
	}
#endif

#ifdef DEBUGXXXX
	for (int u = 0; u < fbgroupuserlist.GetSize(); ++u)
	{
		CSym &usym = fbgroupuserlist[u];
		vara list(usym.GetStr(FBL_GLIST), ";");
		usym.SetStr(FBL_GLIST + 1, list.join(";"));
		if (list.GetSize() == 0)
			continue;
		for (int i = 0; i < fbgrouplist.GetSize(); ++i)
		{
			CSym &sym = fbgrouplist[i];
			if (sym.data.Trim(", ").IsEmpty())
				continue;
			int f = list.indexOf(sym.GetStr(FBL_NAME));
			if (f >= 0)
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
	int ng = FACEBOOK_DownloadList(f, fbgrouplist, burl(ubase, "groups/?seemore"), "<li", ">Groups You", "</ul", "", "X");
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
	for (int i = 0; i < fblist.GetSize(); ++i)
		if (fboldfriendslist.Find(fblist[i].id) < 0)
			fbnewfriendslist.AddUnique(fblist[i]);
	fbnewfriendslist.Save(filename(FBNEWFRIENDS));

	if (MODE == 3)
	{
		// new friends only
		fbuserlist = fbnewfriendslist;
	}

	if (MODE == 2)
	{
		// posting users only
		CSymList tmp;
		tmp.Load(filename(FBPOWERUSERS));
		tmp.Sort();

		for (int i = 0; i < fbuserlist.GetSize(); ++i)
			if (tmp.Find(fbuserlist[i].id) < 0)
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

		for (int i = 0; i < list.GetSize(); ++i)
			if (IsSimilar(list[i], "*["))
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
	if (MODE < 0 || MODE == 0)
		for (int i = 0; i < fbgrouplist.GetSize(); ++i)
		{
			CSym &sym = fbgrouplist[i];
			if (IsSimilar(sym.id, "http"))
				if (FBNAME == NULL || strstr(sym.GetStr(FBL_NAME), FBNAME))
					if (FBNAME != NULL || !strstr(sym.GetStr(FBL_FLAGS), F_NOMATCH))
						MFACEBOOK_DownloadConditionsDate(f, fbsymlist, sym.id, sym.GetStr(FBL_NAME), sym.GetStr(FBL_REGION), sym.GetStr(FBL_FLAGS) + F_GROUP);
			fbgroupuserlist.Save(filename(FBGROUPUSERS));
		}

	// USERS
	if (MODE < 0 || MODE >= 1)
		for (int i = 0; i < fbuserlist.GetSize(); ++i)
		{
			CSym &sym = fbuserlist[i];
			if (IsSimilar(sym.id, "http"))
				if (FBNAME == NULL || strstr(sym.GetStr(FBL_NAME), FBNAME))
					if (FBNAME != NULL || !strstr(sym.GetStr(FBL_FLAGS), F_NOMATCH))
					{
						vars regions = sym.GetStr(FBL_REGION);
						if (regions.IsEmpty())
							regions = FACEBOOK_PreferredRegions(sym.id);
						MFACEBOOK_DownloadConditionsDate(f, fbsymlist, sym.id + "?v=timeline", sym.GetStr(FBL_NAME), regions, sym.GetStr(FBL_FLAGS));
					}
		}

	if (MODE == 3)
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

	if (fbname && IsSimilar(fbname, "TEST"))
	{
		// TEST
		CSymList list;
		list.Load(file);

		Log(LOGINFO, "---------------------TEST START---------------------------");
		// Test MATCHNAME

		for (int i = 0; i < list.GetSize(); ++i)
		{
			CSym &sym = list[i];
			vars id = sym.GetStr(FB_RWID);
			if (IsSimilar(id, RWID) || id.IsEmpty())
				continue;
			vars line(sym.GetStr(FB_TEXT));
			line.replace(FBCOMMA, ",");
			FACEBOOK_TestLine(strstr(sym.GetStr(FB_SOURCE), "timeline") ? 1 : 0, line, sym.GetStr(FB_NAME));
			sym.id = "";
		}
		Log(LOGINFO, "---------------------TEST END---------------------------");
		return TRUE;
	}

	if (fbname && strstr(fbname, ".csv"))
	{
		vars file2 = GetFileName(file) + "." + GetFileName(fbname) + ".csv";
		Log(LOGINFO, "COMPARING %s <<>> %s => %s ", file, fbname, file2);

		CSymList list, list2;
		list.Load(file);
		list2.Load(fbname);

		// set =
#define EQ " "
		for (int i = 0; i < list.GetSize(); ++i)
		{
			int eq = -1;
			for (int i2 = 0; i2 < list2.GetSize() && eq < 0; ++i2)
			{
				if (FBSymCmp(list[i], list2[i2]))
					if (list2[i2].GetStr(FB_RWID) != EQ)
						eq = i2;
			}
			if (eq < 0)
				list[i].SetStr(FB_RWID, "<<<<");
			else
			{
				list[i].SetStr(FB_RWID, EQ);
				list2[eq].SetStr(FB_RWID, EQ);
			}
		}
		for (int i2 = 0; i2 < list2.GetSize(); ++i2)
			if (list2[i2].GetStr(FB_RWID) != EQ)
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
	for (int i = 0; i < fbsymlist.GetSize(); ++i)
		for (int j = i + 1; j < fbsymlist.GetSize(); ++j)
			if (fbsymlist[i].id == fbsymlist[j].id)
				if (fbsymlist[i].GetStr(FB_LINK) == fbsymlist[j].GetStr(FB_LINK))
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
	for (int i = 0; i < fbsymlist.GetSize(); ++i)
		if (IsSimilar(fbsymlist[i].id, CONDITIONS))
			++count;

	CSymList fbuserposting;
	fbuserposting.Load(filename(FBPOWERUSERS));

	Log(LOGINFO, "%s %d Facebook condition reports...", MODE >= 0 ? "Testing " : "Updating ", count);
	Sleep(3000);

	// UPLOAD (Consolidates while uploading)
	for (int i = 0; i < fbsymlist.GetSize(); ++i)
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
		cond[COND_TEXT] = "\"" + posttext + "\"" + " '''" + fbsym.GetStr(FB_SIGN) + "'''";
		cond[COND_TEXT].Replace(";", FBCOMMA);
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

		CSym sym(fbsym.id, fbsym.GetStr(FB_NAME));
		sym.SetStr(ITEM_CONDDATE, cond.join(";"));

		vars credit = fbsym.GetStr(FB_USER) + "@Facebook";
		printf("%d/%d %s            \r", i, fbsymlist.GetSize(), fbsym.id);
		if (!UploadCondition(sym, sym.GetStr(ITEM_DESC), credit.replace(" ", ""), TRUE))
			fbsym.id = "=";

		if (MODE < 0)
		{
			// add likes
			vara likelist(fbsym.GetStr(FB_LIKE), " ");
			for (int l = 0; l < likelist.length(); ++l)
				MFACEBOOK_Like(f, likelist[l]);
		}

		fbuserposting.AddUnique(CSym(fburlstr(fbsym.GetStr(FB_ULINK)), fbsym.GetStr(FB_USER)));
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
	fbsymlist.header.Replace("ITEM_", "");
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
		for (int i = 1; i < imgs.length() && imgurl.IsEmpty(); ++i)
			if (strstr(imgs[i], "class=\"main-photo\""))
				imgurl = ExtractString(imgs[i], "src=", "\"", "\"");
		if (!imgurl.IsEmpty() && !IsSimilar(imgurl, "http"))
			imgurl = "http:" + imgurl;
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
			imgurl = "http://ropewiki.com" + imgurl;
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
	RWLogin(f);
	CPage up(f, NULL, NULL, NULL);
	DownloadFile fb(TRUE);

	CSymList fbsymlist;
	fbsymlist.Load(file);


	for (int i = 0; i < fbsymlist.GetSize(); ++i)
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

		vars comment = "Picture by " + author;//fburlstr2(pic);
		if (!authorlink.IsEmpty())
			comment += " " + fburlstr2(authorlink);
		vars rfile = name.replace(" ", "_") + "_Banner.jpg";
		printf("%d/%d %s         \r", i, fbsymlist.GetSize(), rfile);
		if (MODE > -2 && MODE < 2)
			if (up.FileExists(rfile))
				continue;
		if (MODE > 1)
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

		ShellExecute(NULL, "open", "http://ropewiki.com/File:" + ACP8(rfile), NULL, NULL, SW_SHOWNORMAL);
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

const char *wconv[] = { "DRY", "WADING", "SWIMMING", "LOW", "MODLOW", "3 - Significant flow (class C)", "MODHIGH","4 - High flow features (class C2)]","VERYHIGH", "5 - Extreme (class C3+)", NULL };

int FixConditions(int MODE, const char *query)
{
	CSymList idlist;
	if (!query || *query == 0)
		query = "[[Category:Conditions]][[Has condition water::~3 -*]]";
	GetASKList(idlist, CString(query), rwftitle);

	DownloadFile f;
	for (int i = 0; i < idlist.GetSize(); ++i)
	{
		int changes = 0;
		const char *title = idlist[i].id;
		CPage p(f, NULL, title);
		vars prop = "Wetsuit condition";
		vars w, ow = w = p.Get(prop);
		int m = IsMatchNC(w, wconv);
		if (m >= 0)
			w = cond_temp[m];
		if (w[0] == 'C')
			if (w[1]<'0' || w[1]>'4')
				w = w; //ASSERT(!w);

		if (w != ow)
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
	vara hdr(vars(headers).replace(" ", ""), ",");
	int col = hdr.indexOf(value) - 1;
	if (col < 0)
	{
		Log(LOGERR, "Could not find %s in %s", value, headers);
		return FALSE;
	}

	for (int i = 0; codelist[i]->code != NULL; ++i)
	{
		//if (value && *value!=0 && strcmp(codelist[i].code, value)!=0)
		//	continue;

		CSymList list;
		vars file = filename(codelist[i]->code);
		list.Load(file);
		for (int l = 0; l < list.GetSize(); ++l)
		{
			if (MODE == 1)
			{
				// insert
				for (int j = ITEM_BETAMAX; j >= col; --j)
					list[l].SetStr(j + 1, list[l].GetStr(j));
				list[l].SetStr(col, "");
			}
			if (MODE == 2)
			{
				// move columns
				vars region = list[l].GetStr(ITEM_REGION);
				for (int j = ITEM_REGION - 1; j >= ITEM_ACA; --j)
					list[l].SetStr(j + 1, list[l].GetStr(j));
				list[l].SetStr(ITEM_ACA, region);
			}
			if (MODE == -1)
				list[l].SetStr(col, "");
		}
		list.header = GetToken(list.header, 0) + "," + GetTokenRest(headers, 1);
		list.header.Replace("ITEM_", "");
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
	l = l + 1;

	l = l + 1;

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