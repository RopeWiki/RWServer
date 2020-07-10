
#include "Books.h"
#include "BETAExtract.h"
#include "trademaster.h"

int BOOK_DownloadBeta(const char *ubase, CSymList &symlist)
{
	CSymList list = symlist;
	//list.Load(filename(ubase));
	for (int i = 0; i < list.GetSize(); ++i)
		UpdateCheck(symlist, list[i]);
	return TRUE;
}
