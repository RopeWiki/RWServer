#pragma once

#include "BetaC.h"
#include "BETAExtract.h"
#include "trademaster.h"

class Code {
public:
	int order;
	int goodtitle;
	const char *code;
	const char *name;
	const char *staruser;
	BETAC *betac;
	const char *xregion;
	const char *xstar, *xmap, *xcond;
	/*
	tbeta *downloadbeta;
	textract *extractkml;
	tcond *downloadcond;
	*/

	// translation
	const char *translink;
	int transfile[T_MAX]{};
	CSymList translate[T_MAX];
	int flags;

	CSymList list;

	vars Filename(int sec);

	Code(int order, int goodtitle,
		const char *code,
		const char *translink,
		const char *name,
		const char *staruser,
		BETAC *betac,
		const char *xregion,
		const char *xstar = "",
		const char *xmap = "",
		const char *xcond = ""
		//int flags = 0
	);

	BOOL IsRW(void)
	{
		return IsEqual(code, "rw");
	}

	BOOL IsRedirects(void)
	{
		return IsEqual(code, RWREDIR);
	}

	BOOL IsWIKILOC(void)
	{
		return IsEqual(code, "wik");
	}

	BOOL IsCandition(void)
	{
		return IsEqual(code, "cond_can");
	}

	BOOL IsTripReport(void)
	{
		return IsSimilar(code, "cond_") != NULL || IsWIKILOC();
	}

	BOOL IsBook(void)
	{
		return IsSimilar(code, "book_") != NULL;
	}

	CString BetaLink(CSym &sym, vars &pre, vars &post);
	BOOL FindLink(const char *url);
	static int IsBulletLine(const char *oline, vars &pre, vars &post);
	BOOL NeedTranslation(int sec);
	vars Region(const char *region, BOOL addnew = TRUE);
	vars SuffixTranslate(const char *name);
	vars Description(const char *desc);
	vars Rock(const char *orock, BOOL addnew = TRUE);
	vars Season(const char *_oseason);
	vars Name(const char *name, BOOL removesuffix = TRUE);

	~Code(void)
	{
		for (int i = 0; i < T_MAX; ++i)
			if (transfile[i] > 1)
			{
				translate[i].Sort();
				translate[i].Save(Filename(i));
			}

		delete betac;
	}

};

