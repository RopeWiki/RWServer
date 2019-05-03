#pragma once

#include "stdafx.h"

class utils {
public:
	static int WriteFile(const char *ofile, inetdata *data);

	static int WriteFile(HANDLE hFile, inetdata *data);
};

