
#pragma once

#include "stdafx.h"
#include "utils.h"


int utils::WriteFile(const char *ofile, inetdata *data) //, int skip = 0)
{
	CFILE f;
	if (!f.fopen(ofile))
		{
		// check file exist
		Log(LOGERR, "ERROR: can't read file %.128s", ofile);
		return FALSE;
		}

	DWORD size;

	size = WriteFile(f.f, data);

	f.fclose();
	return size;
}


int utils::WriteFile(HANDLE hFile, inetdata *data)
{
	DWORD read, size = 0;
	char buff[1024*8];
	while (ReadFile(hFile, buff, sizeof(buff), &read, NULL)>0 && read>0)
	{
		if (data)
			data->write(buff, read);
		size += read;
	}
	
	return size;
}
