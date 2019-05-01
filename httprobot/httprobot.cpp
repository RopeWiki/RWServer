// httprobot.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

int logging = 0;

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc<=1)
		{
		printf("usage: %s [http://url] [urlfile.txt] [...]\n", argv[0]);
		return 0;
		}

	CStringArrayList list;
	for (int i=1; i<argc; ++i)
		{
		if (stricmp(argv[i],"-l")==0)
			{
			extern BOOL DEBUGLOAD;
			DEBUGLOAD = TRUE;
			continue;
			}
		if (strnicmp(argv[i],"http:",5)==0)
			{
			list.AddTail(argv[i]);
			continue;
			}
		// load file
	      {
		  CFILE f;
		  if (!f.fopen(argv[i],CFILE::MREAD))
			  {
			  printf("ERROR: %s file not exist\n", argv[i]);
			  continue;
		      }
		  const char *line;
		  while ( (line=f.fgetstr())!=NULL )
			  if (*line!=';' && *line!='/')
				  list.AddTail(CString(line).Trim("\"\' "));
		  }
		}


	DownloadFile f;
	CString url;
	for (int i=0; i<list.GetSize(); ++i)
		DownloadFile::AddUrl(url, list[i]);
	if (f.Download(url))
		{
		Log(LOGERR, "ERROR URL#%d: %.128s", i, list[i]);
		return FALSE;
		}


	return 0;
}

