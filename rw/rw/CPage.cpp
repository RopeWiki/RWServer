
#include "CPage.h"
#include "BETAExtract.h"
#include "trademaster.h"


int CPage::Open(DownloadFile &f, const char *url, int retries, const char *action)
{
	for (int retry = 0; retry < retries; ++retry)
	{
		// download page 
#ifdef DEBUG
		if (f.Download(url, "testpage.html")) {
#else
		if (f.Download(url)) {
#endif
			//Log(LOGERR, "ERROR: can't EDIT %.128s, retrying", pageurl);
			if (retries > 1)
				Login(f);
			continue;
		}

		// modify page
		vars memory(htmltrans(f.memory));
		textname = ExtractString(memory, "<textarea", " name=\"", "\"");
		lines = vara(oldlines = ExtractString(memory, "<textarea", ">", "</textarea"), "\n");

		// group multilines
		BOOL properties = FALSE;
		for (int i = 0; i < lines.length(); ++i)
		{
			if (IsSimilar(lines[i], "{{"))
			{
				properties = TRUE;
				continue;
			}
			if (IsSimilar(lines[i], "}}"))
			{
				properties = FALSE;
				continue;
			}
			if (properties)
				if (lines[i - 1][0] == '|' && lines[i][0] != '|')
				{
					lines[i - 1] += "\n" + lines[i];
					lines.RemoveAt(i--);
				}
		}

		// get values
		if (!f.GetForm(action)) {
			//Log(LOGERR, "ERROR: could not edit/submit, retrying %s", title);
			Login(f);
			continue;
		}
		return TRUE;
	}

	pageurl = "";
	return FALSE;
}

CPage::CPage(DownloadFile &f, const char *_id, const char *_title, const char *_oldid)
{
	fp = &f;

	if (!_id) _id = "";
	if (!_title) _title = "";
	id = _id;
	title = _title;

	if (!id.IsEmpty() && !IsID(id))
	{
		// id = title
		title = id;
		id = "";
	}

	open = -1;
	if (_oldid)
	{
		// Edit
		pageurl = CString("http://ropewiki.com/index.php?");
		if (id.IsEmpty())
			pageurl += CString("title=") + url_encode(title);
		else
			pageurl += CString("curid=") + id;
		if (_oldid == "new")
			open = Open(f, pageurl + "&action=edit");
		else
			open = Open(f, pageurl + "&action=edit" + (*_oldid != 0 ? CString("&oldid=") + _oldid : ""));
	}
}

int CPage::NeedsUpdate()
{
	int ret = !pageurl.IsEmpty() && comment.length() != 0 && oldlines.trim() != lines.join("\n").trim();
#ifdef DEBUGXXX
	if (ret)
	{
		CFILE of, nf;
		if (of.fopen("testo.log", CFILE::MWRITE))
			of.fputstr(oldlines);
		if (nf.fopen("testn.log", CFILE::MWRITE))
			nf.fputstr(lines.join("\n"));
	}
#endif
	return ret;
}

vars CPage::Get(const char *name)
{
	vars prop = "|" + vars(name) + "=";
	int len = prop.GetLength();
	for (int i = 0; i < lines.length(); ++i)
		if (strncmp(prop, lines[i], len) == 0)
		{
			const char *line = lines[i];
			return line + len;
		}
	return "";
}

void CPage::Set(const char *name[])
{
	lines = vara(name);
}

void CPage::Set(const char *name, const char *value, int force)
{
	UpdateProp(name, value, lines, force);
}

int CPage::Override(const char *name, const char *value)
{
	if (value != NULL && *value != 0)
		if (Get(name) != value)
		{
			Set(name, value, TRUE);
			return TRUE;
		}
	return FALSE;
}

int CPage::Section(const char *section, int *iappend)
{
	return FindSection(lines, section, iappend);
}

void CPage::Purge(void)
{
	DownloadFile &f = *fp;
	PurgePage(f, id, title);
}
	
int CPage::Update(int forceupdate)
{
	if (open <= 0)
	{
		Log(LOGINFO, "COULD NOT OPEN! SKIPPING UPDATING %s %s : %s", id, title, comment.join(";"));
		return FALSE;
	}

	if (!forceupdate && !NeedsUpdate())
		return FALSE;

	//UpdateProp(pname, pval, lines, TRUE);

	DownloadFile &f = *fp;
	Log(LOGINFO, "UPDATING %s %s : %s", id, title, comment.join(";"));
	f.SetFormValue(textname, lines.join("\n"));
	f.SetFormValue("wpSummary", comment.join(";"));
	f.SetFormValue("wpWatchthis", "0");
	f.SetFormValue("wpPreview", NULL);
	f.SetFormValue("wpDiff", NULL);
#ifdef DEBUGXXX
	for (int i = 0; i < lines.GetSize(); ++i) Log(LOGINFO, "'%s'", lines[i]);
#endif
	/*
	CString url = pageurl;
	f.SetPostFile(PostPage);
	url += "&action=submit?POSTFILE?";
	url += f.SetFormValue(textname, lines.join("\n"));
	url += f.SetFormValues(lists);
	//printf("SUBMITTING %s ...\r", title);
	for (int retry=0; retry<3; ++retry)
		{
		if (f.Download(url)) {
			Login(f);
			continue;
			}
		Purge();
		return TRUE;
		}
	*/
	for (int retry = 0; retry < 3; ++retry)
	{
		if (f.SubmitForm()) {
			Login(f);
			continue;
		}
		Purge();
		return TRUE;
	}

	Log(LOGERR, "ERROR: can't submit form for %s %.128s", id, title);
	return FALSE;
}

void CPage::Delete(void)
{
	if (comment.length() <= 0)
		return;

	if (open <= 0)
	{
		Log(LOGINFO, "ALREADY DELETED %s %s : %s", id, title, comment.join(";"));
		return;
	}


	Log(LOGINFO, "DELETING %s %s : %s", id, title, comment.join(";"));
	for (int i = 0; i < lines.length(); ++i)
		Log(LOGINFO, "%s", lines[i]);

	// Delete
	DownloadFile &f = *fp;
	vars url = "http://ropewiki.com/index.php?action=delete&title=" + url_encode(title.replace(" ", "_"));
	if (!Open(f, url, 1, "action=delete"))
	{
		Log(LOGERR, "ERROR: could not open submit page %s (3 retries)", url);
		return;
	}

	//wpOldTitle		
	//wpNewTitleNs
	//f.SetFormValue(lists, "wpOldTitle", url_encode(title));

	//f.SetFormValue(lists, "wpDeleteReasonList", "other");
	f.SetFormValue("wpReason", comment.join(";"));
	f.SetFormValue("wpWatch", "0");

#ifdef DEBUGXXX
	for (int i = 0; i < lists[0].GetSize(); ++i) Log(LOGINFO, "'%s' = '%s'", lists[0][i], lists[1][i]);
#endif
	/*
	url += "?POST?" + f.SetFormValues(lists, FALSE);
	*/
	//printf("SUBMITTING %s ...\r", title);
	for (int retry = 0; retry < 3; ++retry)
	{
		if (f.SubmitForm()) {
			Login(f);
			continue;
		}
		DownloadFile f;
		if (Open(f, pageurl, 1)) {
			Log(LOGERR, "FAILED DELETE %s", title);
			return;
		}

		Purge();
		return;
	}
	Log(LOGERR, "ERROR: can't delete %.128s", url);
	// manual chek
	//if (MODE>=0) 
	//	system("start \"check\" \"+url+"&action=history\"");
}

void CPage::Move(const char *newtitle)
{
	if (comment.length() <= 0)
		return;

	vars pageurl = this->pageurl;
	Log(LOGINFO, "MOVING %s %s -> %s", id, title, newtitle);

	// Move
	DownloadFile &f = *fp;
	vars url = "http://ropewiki.com/Special:MovePage/" + url_encode(title.replace(" ", "_"));
	if (!Open(f, url)) {
		Log(LOGERR, "ERROR: could not open submit page %s (3 retries)", url);
		return;
	}

	//wpOldTitle		
	//wpNewTitleNs
	//f.SetFormValue(lists, "wpOldTitle", url_encode(title));

	vars nstitle = newtitle;
	const char *ns = f.GetFormValue2("wpNewTitleNs");
	if (ns && strcmp(ns, "0") != 0)
		nstitle = GetTokenRest(newtitle, 1, ':');

	f.SetFormValue("wpNewTitleMain", nstitle);
	f.SetFormValue("wpReason", comment.join(";"));
	f.SetFormValue("wpWatch", "0");
	f.SetFormValue("wpLeaveRedirect", 0);

#ifdef DEBUGXXX
	for (int i = 0; i < lists[0].GetSize(); ++i) Log(LOGINFO, "'%s' = '%s'", lists[0][i], lists[1][i]);
#endif
	/*
	url = "http://ropewiki.com/index.php?title=Special:MovePage&action=submit?POST?";
	url += f.SetFormValues(lists, FALSE);
	*/
	//printf("SUBMITTING %s ...\r", title);
	for (int retry = 0; retry < 3; ++retry)
	{
		if (f.SubmitForm()) {
			Login(f);
			continue;
		}
		DownloadFile f;
		if (Open(f, pageurl, 1)) {
			Log(LOGERR, "FAILED MOVE %s -> %s", title, newtitle);
			return;
		}

		id = "";
		title = newtitle;
		Purge();
		return;
	}
	Log(LOGERR, "ERROR: can't move %.128s", url);
	// manual chek
	//if (MODE>=0) 
	//	system("start \"check\" \"+url+"&action=history\"");
}

int CPage::FileExists(const char *rfile)
{
	DownloadFile &f = *fp;
	vars ufile = url_encode(vars(rfile).replace(" ", "_"));
	vars url = "http://ropewiki.com/File:" + ufile;
	if (!f.Download(url))
		if (!strstr(f.memory, "No file by this name exists"))
			return TRUE;

	return FALSE;
}

int CPage::UploadFile(const char *lfile, const char *rfile, const char *desc)
{
	DownloadFile &f = *fp;
	Log(LOGINFO, "UPLOADING FILE %s -> File:%s", lfile, rfile);

	// Upload
	vars ufile = url_encode(vars(rfile).replace(" ", "_"));
	vars url = "http://ropewiki.com/index.php?title=Special:Upload&wpDestFile=" + ufile + "&wpForReUpload=1";
	if (!Open(f, url, 3, "action=\"/Special:Upload\"")) {
		Log(LOGERR, "ERROR: could not open upload page %s (3 retries)", url);
		return FALSE;
	}

	/*
	f.SetPostFile(UploadFileHandler);
	url = "http://ropewiki.com/index.php/Special:Upload?POSTFILE?";
	url += lfile +vars(">")+rfile;
	*/
	//printf("SUBMITTING %s ...\r", title);
	vars ext = GetFileExt(lfile);
	varas mime("kml=application/vnd.google-earth.kml+xml,pdf=application/pdf,jpg=image/jpeg,gif=image/gif,png=image/png,tif=image/tiff");
	int found = mime[0].indexOf(ext);
	if (found < 0)
	{
		Log(LOGERR, "Invalid mime for file %s", lfile);
		return FALSE;
	}
	f.SetFormFile("wpUploadFile", rfile, mime[1][found], lfile);
	f.SetFormValue("wpDestFile", rfile);
	if (desc && *desc != 0)
		f.SetFormValue("wpUploadDescription", desc); // comment
	f.SetFormValue("wpWatchthis", "0");
	f.SetFormValue("wpIgnoreWarning", "1"); // ignore warns
	//f.SetFormValue("wpEditToken", token);
	//f.SetFormValue("title", "Special:Upload");
	//f.SetFormValue("wpDestFileWarningAck", "");
	//f.SetFormValue("wpForReUpload", "1");
	//f.SetFormValue("wpUpload","Upload file");
	for (int retry = 0; retry < 3; ++retry)
	{
		if (f.SubmitForm()) {
			Login(f);
			continue;
		}
		if (FileExists(rfile))
			return TRUE;
	}


	Log(LOGERR, "UPLOAD ERROR for file '%s', skipping File:%s", lfile, rfile);
	return FALSE;
}


BOOL CPage::IsID(const char *id)
{
	// all digits
	if (!id) return FALSE;
	for (int i = 0; id[i] != 0; ++i)
		if (!isdigit(id[i]))
			return FALSE;
	return TRUE;
}

