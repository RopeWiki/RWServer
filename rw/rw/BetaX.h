#pragma once

#include <afx.h>
#include "ArrayList.h"

class BETAX
{
public:
	const char *start, *end, *bstart, *bend;

	BETAX(const char *start = NULL, const char *end = NULL, const char *bstart = NULL, const char *bend = NULL)
	{
		this->start = start;
		this->end = end;
		this->bstart = bstart;
		this->bend = bend;
	}
};

typedef CArrayList<BETAX> BETAXL;
