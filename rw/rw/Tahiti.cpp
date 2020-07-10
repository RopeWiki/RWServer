
#include "BetaC.h"
#include "BETAExtract.h"
#include "trademaster.h"

class TAHITI : public BETAC
{
protected:
	CSymList trans;

public:

	TAHITI(const char *base) : BETAC(base)
	{
		kfunc = SELL_ExtractKML;
	}

	virtual int DownloadPage(const char *url, CSym &sym)
	{
		Throttle(tickscounter, ticks);
		if (f.Download(url))
		{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			vars name = GetFileName(vars(sym.id).replace("/", "\\").replace("%20", " "));
			sym.SetStr(ITEM_DESC, name);
			return TRUE;
		}

		vars name = stripHTML(ExtractString(f.memory, "class=\"ContentBox\"", ">", "</span"));
		if (name.IsEmpty())
		{
			Log(LOGWARN, "WARNING: using title for name of url %.128s", url);
			name = stripHTML(ExtractString(f.memory, "<title", ">", "</title"));
		}
		sym.SetStr(ITEM_DESC, name);
		return TRUE;
	}

	virtual int DownloadInfo(const char *data, CSym &sym)
	{
		//name.Replace("riv ", "Riviere ");
		if (!strstr(sym.id, "/Topo/"))
			return FALSE;
		return TRUE;
	}

	virtual void DownloadPatch(vars &memory)
	{
		memory.Replace("href=\"#\"", "");
	}


	virtual int DownloadBeta(CSymList &symlist)
	{

		// get page list
		CSymList clist;
		BETAX xmain("<div", "</div", ">Liste canyons par commune<", ">Les Lavatubes<");
		DownloadUrl(umain = "http://canyon-a-tahiti.shost.ca/", clist, xmain, 0);
		vars memory = f.memory;
		vars base;
		x = BETAX("<a");
		for (int i = 0; i < clist.GetSize(); ++i)
		{
			vara urla(clist[i].id, "/");
			urla.SetSize(urla.length() - 1);
			ubase = base = urla.join("/");
			DownloadUrl(clist[i].id, symlist, x);
		}

		// get table coords
		trans.Load(filename("trans-tahiti"));
		trans.Sort();

		DownloadPatch(memory);
		//vars pdffile = ExtractHREF(ExtractString(memory,">Inventaire<", "", ">Liste canyons par commune<"));
		vars pdffile = ::ExtractLink(memory, "ContentBox\">Inventaire");
		//Michelle: I commented this out because this class originally derived from Madinina class, and this method is in there, but the Tahiti site is now defunct
		//if (pdffile.IsEmpty() || !DownloadIndex(symlist, makeurl(umain, pdffile), "6K", 3, 4, 19))
		//	Log(LOGERR, "ERROR: Could not find/download INDEX PDF for TAHITI");

		return symlist.GetSize() > 0;
	}
};

