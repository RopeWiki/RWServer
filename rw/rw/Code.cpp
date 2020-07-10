
#include "Code.h"
#include "BETAExtract.h"
#include "trademaster.h"

	vars Code::Filename(int sec)
	{
		static const char *transfiles[T_MAX] = { "N", "R", "S", "T" };
		return filename(MkString("trans-%s%s", code, transfiles[sec]));
	}

	Code::Code(int order, int goodtitle,
		const char *code,
		const char *translink,
		const char *name,
		const char *staruser,
		BETAC *betac,
		const char *xregion,
		const char *xstar,
		const char *xmap,
		const char *xcond
		//int flags = 0
	)
	{
		this->order = order;
		this->goodtitle = goodtitle;
		this->code = code;
		this->name = name;
		this->staruser = staruser;
		this->betac = betac;

		/*
		this->downloadbeta = downloadbeta;
		this->extractkml = extractkml;
		this->downloadcond = downloadcond;
		*/


		this->translink = translink;

		this->xregion = xregion; //region;
		this->xstar = xstar;
		this->xmap = xmap;
		this->xcond = xcond;

		this->flags = 0; //flags;

		for (int i = 0; i < T_MAX; ++i)
			transfile[i] = 0;
	}	

	CString Code::BetaLink(CSym &sym, vars &pre, vars &post)
	{
		CString url = sym.id;
		if (!IsSimilar(url, "http")) // BQN: KML: etc.
			return "";

		CString kmlidx = sym.GetStr(ITEM_KML);
		if (IsSimilar(kmlidx, "http"))
			kmlidx = "X"; // skip complex ids
		if (betac->kfunc)
			url = KMLIDXLink(url, kmlidx);
		vars pattern = "[" + url + " " + name + "]";
		vars language = translink ? translink : "en";
		//if (translink)
		{
			vars flag = MkString("[[File:%s.png|alt=|link=]]", IsBook() ? "book" : vars("rwl_") + language);
			//pattern = flag+"[http://translate.google.com/translate?hl=en&sl="+vars(translink)+"&tl=en&u="+vars(url).replace("&","%26")+" "+name+" "TTRANSLATED"] ["+url+" "TORIGINAL"]";
			vars trans = "[http://translate.google.com/translate?hl=en&sl=" + language + "&tl=en&u=" + vars(url).replace("&", "%26") + " " TTRANSLATED "] ";
			pattern = flag + "[" + url + " " + name + "]";
			//if (!IsBook())
			//	pattern += MkString("<span class='translink' %s>%s</span> ", !translink ? "style='display:none'" : "", trans);

		}
		//if (IsWIKILOC())
		post = " : " + sym.GetStr(ITEM_DESC);
		CString ret;
		if (pre.IsEmpty())
			pre = "*";
		if (!pre.IsEmpty())
			ret += pre + " ";
		ret += pattern;
		if (!post.IsEmpty())
			ret += " " + post;
		return ret.Trim();
	}

	BOOL Code::FindLink(const char *url)
	{
		if (!(url = strstr(url, "http")))
			return -1;
		vars id = urlstr("http:" + GetToken(GetTokenRest(url, 1, ':'), 0, " ]"));
		if (list.GetSize() == 0) {
			list.Load(filename(code));
			list.Sort();
		}
		return list.Find(id);
	}

	int Code::IsBulletLine(const char *oline, vars &pre, vars &post)
	{
		vars line(oline);
		line.Replace("\t", " ");
		line.Replace("\r", " ");
		line.Replace("  ", " ");
		int http = line.Find("http");
		if (http < 0) return -1;

		pre = line.Mid(0, http).Trim("* [");
		const char *match[] = { "File:rwl_", "File:book.png", "File:es.png", "File:en.png", "File:fr.png", "File:pl.png", "File:fr.png", "File:it.png", "File:pt.png", "File:pl.png", "File:auto.png", "File:.png", NULL };
		if (IsMatch(pre, match))
			pre = "";
		int bracket = line.Find("]", http);
		if (bracket < 0) bracket = line.Find(" ", http);
		post = line.Mid(bracket + 1).Trim(" .");
		if (bracket < 0 || (strstr(line, "translate.google.com") && strstr(post, " " TORIGINAL "]")))
			post = "";
		if (bracket < 0 || (strstr(line, "translate.google.com") && strstr(post, " " TTRANSLATED "]")))
			post = "";
		//if (GetTokenCount(post, ' ')>5 || GetTokenCount(pre, ' ')>5)
		if (!pre.IsEmpty())
			return 0;
		return 1;
	}
	   
	BOOL Code::NeedTranslation(int sec) {

		if (!transfile[sec])
		{
			transfile[sec] = -1;
			vars file = Filename(sec);
			if (CFILE::exist(file))
			{
				transfile[sec] = 1;
				translate[sec].Load(file);
				translate[sec].Sort();
			}
		}

		return transfile[sec] > 0;
	}

	vars Code::Region(const char *region, BOOL addnew)
	{
		if (!NeedTranslation(T_REGION) || strstr(region, RWID))
			return region;

		CSymList &list = translate[T_REGION];
		vara ret(regionmatch(region), ";");
		for (int i = 0; i < ret.length(); ++i) {
			vars &str = ret[i];
			// eliminate (#)
			CString num;
			GetSearchString(str, "", num, "(", ")");
			if (!num.IsEmpty())
			{
				int n;
				for (n = 0; num[n] != 0 && isdigit(num[n]); ++n);
				if (num[n] == 0)
					str.Replace("(" + num + ")", "");
			}
			str.Trim();
			int f = list.Find(str);
			if (f < 0) {
				// NEW!
				if (addnew)
				{
					++transfile[T_REGION];
					list.Add(CSym(str));
				}
				continue;
			}
			// TRANS
			if (list[f].data[0] != 0 && list[f].data[0] != '=')
				str = list[f].data;
		}

		vara newreg;
		for (int i = 0; i < ret.length(); ++i)
			if (newreg.indexOf(ret[i]) < 0)
				newreg.push(ret[i]);

		vars newregs = newreg.join(";");
		if (strstr(region, "@"))
			newregs = regionmatch(newregs, GetTokenRest(region, 1, '@'));
		return newregs;
	}

	vars Code::SuffixTranslate(const char *name)
	{
		if (!name || name[0] == 0)
			return name;

		return Description(stripSuffixes(name));
	}

	vars Code::Description(const char *desc)
	{

		static CSymList list;
		if (list.GetSize() == 0)
		{
			list.Load(filename(TRANSBASIC));
			list.iSort();
		}

		// suffix
		vara words(stripAccents(desc), " ");
		int n = words.length() - 1;
		if (n >= 0)
		{
			int find = list.Find(words[n]);
			if (find >= 0 && !list[find].data.IsEmpty())
				words[n] = list[find].data;
		}

		vars ret = words.join(" ").Trim();
		return 	ret;
	}

	vars Code::Rock(const char *orock, BOOL addnew)
	{
		if (!NeedTranslation(T_TROCK))
			return vars(orock).trim();

		CSymList &list = translate[T_TROCK];
		vara ret(vars(orock).replace("-", ";").replace("/", ";").replace(".", ";"), ";");
		for (int i = 0; i < ret.length(); ++i) {
			vars &str = ret[i];
			str.Trim();
			int f = list.Find(str);
			if (f < 0) {
				// NEW!
				if (addnew)
				{
					++transfile[T_TROCK];
					list.Add(CSym(str));
				}
				continue;
			}
			// TRANS
			if (list[f].data[0] != 0 && list[f].data[0] != '=')
			{
				str = list[f].data;
				if (str.trim().IsEmpty())
					ret.RemoveAt(i--);
			}
		}

		return ret.join(";");
	}

	vars Code::Season(const char *_oseason)
	{
		//ASSERT( !strstr(oseason, "Evit"));
		if (!_oseason || _oseason[0] == 0)
			return "";
		vars oseason = stripAccentsL(_oseason);
		vars tseason = oseason;

		tseason.Replace("...", "-");
		tseason.Replace("..", "-");
		tseason.Replace("", "'");
		tseason.trim();

		//ASSERT( !strstr(_oseason, "ideal"));

		if (NeedTranslation(T_SEASON))
			tseason = Translate(tseason, translate[T_SEASON], ";").replace(";;", ";").Trim(";-");

		if (tseason.IsEmpty())
			return "";

		// validate season
		vars res;
		vars sep = " ! ";
		if (!IsSeasonValid(tseason, &res) || strstr(tseason, "?"))
		{
#ifdef DEBUG
			//Log(LOGWARN, "WARNING BAD SEASON?=%s\n%s\n%s", res, tseason, oseason);
#endif
			sep = " !!! " + tseason + " ??? ";
		}
		if (!res.IsEmpty())
			return res + sep + oseason;
		else
			return "";
	}

	vars Code::Name(const char *name, BOOL removesuffix)
	{
		vars ret(name);

		// basic translation
		if (removesuffix)
			ret = SuffixTranslate(name);

		if (!NeedTranslation(T_NAME))
			return ret.trim();

		CSymList &list = translate[T_NAME];
		ret.Trim();
		ret.Replace("[", "(");
		ret.Replace("]", ")");
		ret.Replace("(", "{");
		ret.Replace(")", "}");
		while (TRUE)
		{
			CString str, ostr;
			GetSearchString(ret, "", str, "{", "}");
			if (str.IsEmpty())
				break;
			ostr = str;
			str.Trim();
			if (!strstr(ret, "}"))
				ret += "}";
			int f = list.Find(str);
			if (f < 0) {
				// NEW!
				++transfile[T_NAME];
				list.Add(CSym(str));
			}
			else
			{
				// TRANS
				if (list[f].data[0] != 0 && list[f].data[0] != '=')
					str = list[f].data;
			}
			ret.Replace("{" + ostr + "}", "(" + str + ")");
		}
		//ASSERT(strstr(name, "Neste")==NULL);
		// without parenthesis
		int updated = 0;
		vara reta(ret, " ");
		const char *replace[] = { "s", "i", "a", "m", NULL };
		for (int i = 1; i < reta.length(); ++i)
			for (int j = 0; replace[j] != NULL; ++j)
				if (strnicmp(reta[i], replace[j], 1) == 0) {
					int f = list.Find(reta[i]);
					if (f >= 0) {
						++updated;
						reta[i] = "(" + list[f].data + ")";
					}
				}
		vars ret2 = reta.join(" ");
#ifdef DEBUGX
		if (updated)
			Log(LOGINFO, "translated %s = %s", ret, ret2);
#endif

		// patches
		ret2.Replace("\xc3 ", "\xc3\xa0 ");
		ret2.Replace("- Partie (", "(");
		ret2.Replace("\"", "");
		ret2.Replace("(=)", "");
		ret2.Replace("+ Quirino (Lower)", "");
		ret2.Replace("1\xc3\xa8re partie", "1st part");
		ret2.Replace("2\xc3\xa8me partie", "2nd part");
		ret2.Replace("(Upper) III", "(Upper III)");
		ret2.Replace("Lower section}", "");
		return ret2.Trim();
	}
