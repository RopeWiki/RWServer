#pragma once

#include "BetaC.h"
#include "BETAExtract.h"

class BETAWPC : public BETAC
{
	vars ctitle, cmore, cmore2;

public:
	BETAWPC(const char *base, const char *ctitle = "class='post-title",
		const char *cmore = "class='blog-pager-older-link",
		const char *cmore2 = "</a") : BETAC(base)
	{
		ticks = 500;
		x = BETAX(this->ctitle = ctitle); //, "</h3");
		this->cmore = cmore;
		this->cmore2 = cmore2;
	}
	
	vars DownloadMore(const char *memory)
	{
		return ExtractHREF(ExtractString(f.memory, cmore, "", cmore2));
	}
};
