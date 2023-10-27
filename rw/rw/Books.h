#pragma once

#include "BetaC.h"

int BOOK_DownloadBeta(const char *ubase, CSymList &symlist);

class BOOK : public BETAC
{
public:

	BOOK(const char *base) : BETAC(base, BOOK_DownloadBeta)
	{
	}
};
