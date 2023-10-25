#pragma once

#include "BetaX.h"
#include "inetdata.h"

typedef int DownloadBetaFunc(const char *ubase, CSymList &symlist);
typedef int DownloadKMLFunc(const char *ubase, const char *url, inetdata *out, int fx);
typedef int DownloadConditionsFunc(const char *ubase, CSymList &symlist);

class BETAC
{
protected:
	BETAX x;

public:
	virtual ~BETAC() = default;
	const char *ubase;
	vars obase;
	vars umain;
	vars curl;
	vara urls;
	DownloadBetaFunc *dfunc;
	DownloadKMLFunc *kfunc;
	DownloadConditionsFunc *cfunc;

	DownloadFile f;
	vars region;
	int pages;
	int utf;
	int ticks;
	double tickscounter;

	enum { D_DATA = -1, D_LINK, D_SYM = 1 };

	static vars SELL_Make(double slat, double slng, double elat, double elng);
	static int SELL_MakeKML(const char *title, const char *credit, const char *url, const char *idx, inetdata *out);
	static int SELL_ExtractKML(const char *ubase, const char *url, inetdata *out, int fx);
	static vars MAP_GetID(const char *memory, const char *id);
	static vars MAP_GetID(const char *memory);
	static int MAP_ExtractKML(const char *ubase, const char *url, inetdata *out, int fx);
	BETAC(const char *base, DownloadBetaFunc *_dfunc = NULL, DownloadKMLFunc *_kfunc = NULL, DownloadConditionsFunc *_cfunc = NULL);
	static int ExtractLink(const char *data, const char *ubase, CSym &sym, int utf = FALSE);

	// Customizable functions
	virtual int DownloadLink(const char *data, CSym &sym);

	virtual void DownloadPatch(vars &memory)
	{
	}

	virtual int DownloadInfo(const char *data, CSym &sym);

	virtual int DownloadPage(const char *url, CSym &sym)
	{
		return TRUE;
	}

	virtual vars DownloadMore(const char *memory)
	{
		return "";
	}

	virtual int DownloadBeta(CSymList &symlist);

	int DownloadSym(CSym &sym, CSymList &symlist);

	vars DownloadUrl(const char *url, CSymList &symlist, BETAX &x, int mode = D_SYM);

	int DownloadHTML(const char *path, CSymList &symlist, BETAX &x, int mode = D_SYM);

	int DownloadMemory(const char *memory, CSymList &symlist, BETAX &x, int mode = D_SYM);

	typedef vars ExtractKMLIDXF(const char *memory);

	static vars GetKMLIDX(DownloadFile &f, const char *url, ExtractKMLIDXF fid);
};

