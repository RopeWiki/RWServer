#pragma once

#include "BETAExtract.h"
#include "trademaster.h"

class CPage
{
	BOOL open;
	vars id, title;
	vars pageurl, textname, oldlines;
	DownloadFile *fp;

	int Open(DownloadFile &f, const char *url, int retries = 3, const char *action = "action=submit");

public:
	vara lines;
	vara comment;

	const char *Title()
	{
		return title;
	}

	const char *Id()
	{
		return id;
	}

	BOOL Exists()
	{
		return open > 0 && lines.length() > 0;
	}

	CPage(DownloadFile &f, const char *_id, const char *_title, const char *_oldid = "");

	int NeedsUpdate();

	vars Get(const char *name);

	void Set(const char *name[]);

	void Set(const char *name, const char *value, int force = FALSE);

	int Override(const char *name, const char *value);

	int Section(const char *section, int *iappend);

	void Purge(void) const;

	int Update(int forceupdate = FALSE);

	void Delete(void);

	void Move(const char *newtitle);

#if 0
	static int UploadFileHandler(DownloadFile *df, const char *lrfile, inetdata &data)
	{
		/*
		// get token from previous page
		CString token = df->GetFormValue("wpEditToken", "id");
		*/

		CString lfile = GetToken(lrfile, 0, '>');
		CString rfile = GetToken(lrfile, 1, '>');




		data.write(df->SetFormFile("wpUploadFile", rfile, mime[1][found]));
		CFILE f;
		if (!f.fopen(lfile, CFILE::MREAD))
		{
			Log(LOGERR, "ERROR: %s file not exist\n", lfile);
			return FALSE;
		}
		int bread = 0;
		char buff[8 * 1024];
		while (bread = f.fread(buff, 1, sizeof(buff)))
			data.write(buff, bread);
		data.write("\n");

		form += df->SetFormValue("wpDestFile", rfile);
		form += df->SetFormValue("wpUploadDescription", ""); // comment
		form += df->SetFormValue("wpWatchthis", "0");
		form += df->SetFormValue("wpIgnoreWarning", "1"); // ignore warns
		form += df->SetFormValue("wpEditToken", token);
		form += df->SetFormValue("title", "Special:Upload");
		form += df->SetFormValue("wpDestFileWarningAck", "");
		form += df->SetFormValue("wpForReUpload", "1");
		form += df->SetFormValue("wpUpload", "Upload file");
		//url += f.SetFormValues(lists);

		data.write(form);
		return TRUE;
	}
#endif


	int FileExists(const char *rfile) const;

	int UploadFile(const char *lfile, const char *rfile, const char *desc = NULL);

	static BOOL IsID(const char *id);
};

