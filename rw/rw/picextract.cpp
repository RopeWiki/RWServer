#include <afxinet.h>
#include <atlimage.h>
#include "stdafx.h"
#include "trademaster.h"
#include "rw.h"
#include "math.h"

#include "passwords.h"

#define ITEM_PIC ITEM_LAT2
#define ITEM_DATE ITEM_LNG2

int jsonarray(const char *data, vara &list)
{
	int count = 0, n = 0;
	const char *start = NULL;
	int bracket = -1, skip = -1;
	for (;*data!=0; ++data)
		{
			if (*data=='[')
				++bracket;
			if (*data==']')
				if(--bracket<0)
					break;
			if (*data=='{') 
				if (++skip==0)
					start = data+1, count = 0;
			if (*data=='}') 
				if (--skip<0)
					list.push(CString(start,count)), n++;			
			if (skip>=0)
				++count;
		}
	return n;
}

int jsonarray(vara &data, vara &list)
{
	int n = 0;
	for (int i=1; i<data.length(); ++i)
		n += jsonarray(data[i], list);
	return n;
}


static int ishex(char c)
{
	return (c>='A' && c<='F') || (c>='a' && c<='f') || (c>='0' && c<='9');
}

static vars cleanup(const char *_str)
{
static const char *trim = " '\"";
static const char *stdsubst[] = { 
"\r", " ",
"\n", " ",
"\t", " ",
"&amp;", "&",
"&amp;", "&",
"&apos;", "'",
"&quot;", "\"",
"&nbsp;", " ",
"&#26", "&",
"&#3e", "&gt;",
"&#3c", "&lt;",
"&#39;", "\'",
"&#039;", "\'",
"\\u0026", "&",
"\\u003e", "&gt;",
"\\u003c", "&lt;",
"\\u0039", "\'",
"\\\"", "\"",
"\\\'", "\'",
"\\/", "/",
"\"", " ",
",", " ",
"  ", " ",
NULL
};

vars str(_str);
int i=0;
while (stdsubst[i]!=NULL)
	{
	str.Replace(stdsubst[i], stdsubst[i+1]);
	i+=2;
	}

// \\u => &#x
for (int i=0; str[i]!=0; ++i)
 if (str[i]=='\\' && tolower(str[i+1])=='u')
	{
	const char *s = str;
	for (int j=i+2; ishex(str[j]); ++j);
	CString rep(s+i, j-i);
	str.Replace(rep, "&#x"+rep.Mid(2)+";");
	i = j;
	}

 str.Replace("\\", "");

return vars(str.Trim(trim));
}



void AddPic(CSymList &list, const char *tag, const char *id, const char *desc, double lat, double lng, const char *pic, const char *sdate, const char *fmt, const char *conn = NULL)
{
	CSym sym(cleanup(id));

	vars text = cleanup(desc);
	if (text.GetLength()>100)
		text=text.Mid(0,97)+"...";
	sym.SetStr(ITEM_DESC, text);

	sym.SetStr(ITEM_TAG, cleanup(tag));
	sym.SetStr(ITEM_PIC, cleanup(pic));

	if (CheckLL(lat,lng,NULL))
		{
		sym.SetNum(ITEM_LAT, lat); 
		sym.SetNum(ITEM_LNG, lng);
		}
	else
		Log(LOGERR, "AddPic Invalid coordinates for %s:%s:%s", tag, id, desc);
		
	double date = fmt!=NULL ? CDATA::GetDate(sdate, fmt) : CDATA::GetDate(sdate);
	if (date<=0) {
		//Log(LOGERR, "AddPic Invalid date %s for %s:%s:%s", sdate, tag, id, desc);
		}
	else
		{
		sym.SetStr(ITEM_DATE, MkString("%s:%s", CData(date), cleanup(sdate)));
		sym.SetStr(ITEM_SUMMARY, OleDateTime(date).Format("%d %b %Y"));
		}

	if (conn!=NULL)
		sym.SetStr(ITEM_CONN, cleanup(conn)+": ");

	int f;
	if ((f=list.Find(sym.id))>=0)
		{
		CSym &osym = list[f];
		for (int i=0; i<ITEM_TAG; ++i)
			if (!sym.GetStr(i).IsEmpty())
				osym.SetStr(i, sym.GetStr(i));
		}
	else
		list.Add(sym);
}



#define MAXPICS 50

int PanoramioPic(double lat, double lng, double dist, const char *locid, const char *locname, CSymList &list)
{
			int errors = 0;
			DownloadFile f;

			CString set = "full";
			LLRect bbox(lat, lng, dist);
			static double panoramioticks = 0;

			int oneshot = FALSE;
			int more = TRUE;
			int step = 400;
			int n = 0;
			// run max 3
			for (int dn=0; more && dn<3 && list.GetSize()<MAXPICS; ++dn)
				{				
				//http://www.panoramio.com/map/get_panoramas?order=popularity&set=tag-waterfall&size=thumbnail&from=0&to=24&minx=-180&miny=-76.87933531085397&maxx=180&maxy=84.28533579318314&mapfilter=false
				CString url = MkString("http://www.panoramio.com/map/get_panoramas?set=%s&from=%d&to=%d&size=thumbnail&mapfilter=false&order=date_desc", set, n, n+step);
				url += MkString("&miny=%s&minx=%s&maxy=%s&maxx=%s", CCoord(bbox.lat1), CCoord(bbox.lng1), CCoord(bbox.lat2), CCoord(bbox.lng2));
				Throttle(panoramioticks, 1000);
				if (f.Download(url))
					{
					Log(LOGERR, "Could not download %s", url);
					return -1;
					}

				CString hasmore, count;
				GetSearchString(f.memory, "\"has_more\":", hasmore, "", ",");
				more = stricmp(hasmore, "true")==0;
				if (more)
					more = more;
				GetSearchString(f.memory, "\"count\":", count, "", ",");
				int nnum = (int)CGetNum(count);

				vara plist(f.memory, "},");
				for (int i=1; i<plist.length(); ++i, ++n)
					{
					CString id, desc, lat, lng;
					GetSearchString(plist[i], "\"latitude\"", lat, ":", ",");
					GetSearchString(plist[i], "\"longitude\"", lng, ":", ",");
					double vlat = CGetNum(lat);
					double vlng = CGetNum(lng);
					if (vlat==InvalidNUM || vlng==InvalidNUM)
						continue;
					//if (!obox->Contains(vlat, vlng))
					//	continue;
					GetSearchString(plist[i], "\"photo_url\"", id, ":", ",");
					GetSearchString(plist[i], "\"photo_title\"", desc, ":", ",");		
					AddPic(list, "Panoramio.com", id, desc, vlat, vlng, 
						cleanup(ExtractString(plist[i], "\"photo_file_url\"", ":", ",")),
						cleanup(ExtractString(plist[i], "\"upload_date\"", ":", ",")), "DD MMM YY");
					}		
				if (oneshot) 
					return n;

				}

	return n;
}



int FlickrPic(double lat, double lng, double dist, const char *locid, const char *locname, CSymList &list)
{
			static double flickrticks = 0;
			LLRect bbox(lat, lng, dist);

		//double lngstep = bbox->lng2-bbox->lng1, latstep = bbox->lat2-bbox->lat1;
			DownloadFile f;
			int errors = 0;
			int more = TRUE;
			int step = 100;
			int n = 0, p=1;

			CString set; // "&sort=date-taken-desc";
			while (more && list.GetSize()<MAXPICS)
				{				
				//tags[t].Replace(" ","");
				
				CString url = MkString("https://api.flickr.com/services/rest/?method=flickr.photos.search&api_key=", FLICKR_API_KEY, "&extras=geo,date_upload,date_taken,url_t&page=%d&per_page=%d", p, step);

				if (set!="") url += set;
				url += MkString("&bbox=%s,%s,%s,%s", CCoord(bbox.lng1), CCoord(bbox.lat1), CCoord(bbox.lng2), CCoord(bbox.lat2));

				// force 1 download/sec (<100,000 limit per day)
				Throttle(flickrticks, 1000);
				if (f.Download(url))
					{
					++errors;
					Log(LOGERR, "Could not download %s", url);
					continue;
					}

				CString info, pages, num;
				GetSearchString(f.memory, "<photos ", info, "", ">");
				GetSearchString(info, "pages=", pages, "\"", "\"");
				GetSearchString(info, "total=", num, "\"", "\"");
				double npages = CGetNum(pages), nnum = CGetNum(num);
				if (npages==InvalidNUM || nnum==InvalidNUM)
					{
					Log(LOGERR, "Invalid Flickr response %s", info);
					++errors;
					break;
					}
				

				more = ++p <= npages;

				vara plist(f.memory, "<photo ");
				for (int i=1; i<plist.length(); ++i, ++n)
					{
					CString desc;
					GetSearchString(plist[i], "title=", desc, "\"", "\"");

					CString pid, pown;
					GetSearchString(plist[i], "id=", pid, "\"", "\"");
					GetSearchString(plist[i], "owner=", pown, "\"", "\"");
					if (pid.IsEmpty() || pown.IsEmpty())
						{
						Log(LOGERR, "Invalid Flickr photo %s", plist[i]);
						continue;
						}
					CString lat, lng;
					GetSearchString(plist[i], "latitude=", lat, "\"", "\"");
					GetSearchString(plist[i], "longitude=", lng, "\"", "\"");
					double vlat = CGetNum(lat);
					double vlng = CGetNum(lng);
					if (vlat==InvalidNUM || vlng==InvalidNUM)
						continue;
					//if (!obox->Contains(vlat, vlng))
					//	continue;

					// description
					CString link = MkString("https://www.flickr.com/photos/%s/%s", pown, pid);

					/*
					GetSearchString(plist[i], ">", desc, "<description>", "</description>");
					desc.Replace("\n", " ");
					desc.Replace("\t", " ");
					desc.Replace("  ", " ");
					*/
					AddPic(list, "Flickr.com", link, desc, vlat, vlng, 
						cleanup(ExtractString(plist[i], "url_t", "\"", "\"")), 
						cleanup(ExtractString(plist[i], "datetaken=", "\"", "\"")), "YYYY-MM-DD");
					}		

				// just run one page
				break;
				}

return n;
}

CString InstagramAuth()
{
	DownloadFile f;
	vars url = "https://instagram.com/oauth/authorize/?client_id=INSTAGRAM_CLIENT_ID&redirect_uri=http://localhost&response_type=token&scope=public_content";
	if (f.Download(url, "test.htm"))
		{
		Log(LOGERR, "Can't authorize Instagram");
		return "";
		}

	url = url;
	return "";
}


/*
https://api.instagram.com/v1/locations/search?lat=32.971054&lng=-116.688326&access_token=XXX get locations
https://api.instagram.com/v1/locations/3433774/media/recent?access_token=XXX get recent media
https://www.instagram.com/explore/locations/3433774/ page
https://www.instagram.com/p/BHqHEexj74J/
*/

#define MAXLOCATIONS 20

#define INSTAGRAMTICKS 1000
static double instaticks;

CString InstagramAuthenticate(void)
{
	#define AUTHORIZATION_REDIRECT_URI "http://lucac.no-ip.org/rwr"

	vars url;
	DownloadFile f;
	f.usereferer = TRUE;
	f.hostname = "https://www.instagram.com";

	//vars url = "https://instagram.com/oauth/authorize/?client_id=INSTAGRAM_CLIENT_ID&redirect_uri=http://localhost&response_type=token&scope=public_content";
	Throttle(instaticks, INSTAGRAMTICKS);
	url = "https://api.instagram.com/oauth/authorize/?client_id=" CLIENTID "&redirect_uri=" AUTHORIZATION_REDIRECT_URI "&response_type=code&scope=public_content";
	if (f.Download(url, "itest1.html"))
		{
		Log(LOGERR, "Could not get instagram auth #1");
		return "";
		}

	// if requests to log in
	if (f.GetForm("login-form"))
		{
		f.SetFormValue("username", "ropewikiconditions");
		f.SetFormValue("password", "conditions4all");	
		Throttle(instaticks, INSTAGRAMTICKS);
		if (f.SubmitForm())
			{
			Log(LOGERR, "Could not login on instagram #2");
			return "";
			}
		}

	if (strstr(f.memory, "value=\"Authorize\""))
	  if (f.GetForm())
		{
		Throttle(instaticks, INSTAGRAMTICKS);
		if (f.SubmitForm())
			{
			Log(LOGERR, "Could not login on instagram #3");
			return "";
			}
		}

	vars code = ExtractString(f.memory, "<CODE>", "", "</CODE>");
	if (code.IsEmpty())
		{
		Log(LOGERR, "Could not get instagram code #4");
		return "";
		}
	url = "https://api.instagram.com/oauth/access_token?POST?client_id=" CLIENTID "&client_secret=" CLIENT_SECRET "&grant_type=authorization_code&redirect_uri=" AUTHORIZATION_REDIRECT_URI "&code=" + code;
	if (f.Download(url, "itest2.html"))
		{
		Log(LOGERR, "Could not get instagram token_access #5");
		return "";
		}

	vars token = ExtractString(f.memory, "\"access_token\"", "\"", "\""); 
	if (token.IsEmpty())
		{
		Log(LOGERR, "Could not get good token_access #6");
		return "";
		}
	return token;
}

static DWORD accesstokenticks;
static vars accesstoken;

int InstagramPicList(double lat, double lng, double dist, CSymList &list)
{

			DownloadFile f;

			//INSTAGRAM_Login(f);
	
			if (accesstoken.IsEmpty())
				{
				if (GetTickCount()<accesstokenticks)
					return FALSE;
				accesstoken = InstagramAuthenticate();
				if (accesstoken.IsEmpty())
					{
					Log(LOGERR, "Invalid Instagram Authentication!");
					accesstokenticks = GetTickCount() + 1000*60*30;  // do not retry in30 min
					return FALSE;
					}
				}

			CString url = MkString("https://api.instagram.com/v1/locations/search?distance=%d&lat=%s&lng=%s", (int)dist, CCoord(lat), CCoord(lng));
#if 1
			for (int r=0; r<3; ++r)
				{
				Throttle(instaticks, INSTAGRAMTICKS);
				if (f.Download(url+"&access_token="+accesstoken))
					{
					Log(LOGWARN, "Can't download Instagram Pics %s, resetting access token", url);
					accesstoken = InstagramAuthenticate();
					}
				}
#else
			// force 1 download/sec (<100,000 limit per day)
			const char *auth[] = { "&access_token=", INSTAGRAM_ACCESS_TOKEN, "&client_id=b59fbe4563944b6c88cced13495c0f49", NULL};

			for (int a=0; auth[a]!=NULL; ++a)
				{
				Throttle(instaticks, 1000);
				if (!f.Download(url+auth[a]))
					break;
				}

			if (auth[a]==NULL)
				{
				Log(LOGERR, "Could not download %s", url);
				return 0;
				}
#endif

			vara locs(f.memory, "}, {");

			int nloc = 0;
			for (int i=0; i<locs.length(); ++i)
				{
				vars &loc = locs[i]; //ExtractString(locs[i], "\"location\":{", "", "}");
				double llat = ExtractNum(loc, "\"latitude\"", ":", ",");
				double llng = ExtractNum(loc, "\"longitude\"", ":", ",");
				vars locname = cleanup(ExtractString(loc, "\"name\"", ":", ","));
				vars locid = cleanup(ExtractString(loc, "\"id\"", ":", ","));
				if (locid=="0")
					continue;
				if (Distance(lat, lng, llat, llng)>dist)
					continue;

				if (nloc++>MAXLOCATIONS)
					break;

				CSym sym(MkString("http://%s/rwr?pictures=%s,%s,%s&mode=I,%s,%s", server, CData(llat), CData(llng), CData(dist), url_encode(locid), url_encode(locname)));
				sym.SetNum(0, Distance(lat, lng, llat, llng));
				list.Add(sym);
				}

			return list.GetSize();
}


int InstagramPic(double lat, double lng, double dist, const char *locid, const char *locname, CSymList &list)
{
			int n = 0;
			vars locidurl = vars("https://www.instagram.com/explore/locations/")+locid;
			vars url = vars("https://www.instagram.com/explore/locations/")+locid;
			Throttle(instaticks, INSTAGRAMTICKS);
			DownloadFile f;
			if (f.DownloadNoRetry(url))
				{
				//Log(LOGERR, "Could not download %s", url);
				AddPic(list, "Instagram.com", locidurl, "See Instagram page for latest pictures", lat, lng, "", "http://ropewiki.com/images/c/c0/InstaIcon.png", NULL, "<a href='"+locidurl+"/' target='_blank'><b>"+vars(locname)+"</b></a>");
				++n;
				}
			else
				{					
				vara loclist;
				vara nodes(ExtractString(f.memory, "\"LocationsPage\"", NULL, "</script"), "\"nodes\"");
				int n = jsonarray(nodes, loclist);
				for (int j=0; j<loclist.length(); ++j)
					{
					vars &loc = loclist[j];
					vars picid = ExtractString(loc, "\"code\"", "\"", "\"");
					vars link = "https://www.instagram.com/p/"+picid; //ExtractString(locs[i], "\"link\"", "\"", "\"");
					vars pic = ExtractString(loc, "\"thumbnail_src\"", "\"", "\"");
					vars desc = ExtractString(loc, "\"caption\"", "\"", "\"");
					//vars desc = ExtractString(caption, "\"text\":", "\"", "\"");

					double d = ExtractNum(loc, "\"date\"", ":", ",");				
					vars date = d<=0 ? "" : CDate(Date(COleDateTime((__time64_t)d), TRUE));

					if (link.IsEmpty())
						continue;

					AddPic(list, "Instagram.com", link, desc, lat, lng, pic, date, NULL, "<a href='"+locidurl+"/' target='_blank'><b>"+locname+"</b></a>");
					++n;
					}
				}

			return n;
}


//Google Locations
//https://maps.googleapis.com/maps/api/place/nearbysearch/json?location=32.9703,-116.6801&radius=5000&key=GOOGLE_MAPS_KEY
//types = point of interest
//https://maps.googleapis.com/maps/api/place/photo?maxwidth=100&maxheight=100&key=GOOGLE_MAPS_KEY&photoreference=CmRdAAAAiUfMGmK_R1eNkly4jU34zvnQ0z1-p6q8JC2QBxjhfVWDFFFqmdGDR4QxuVdDD9YoHC0-khDuUQiA5DxatJJRkWeWPpymrlJAma4P9iV-pLKmQLr2rZtbhvkTrWIgEkYFEhAeA2XCsMrLc-3w_Ch2LlFZGhSu6bFVUNArgURyITH_a8j3iO-71Q

/*
// Facebook
// https://graph.facebook.com/search?q=&type=place&center=32.9703,-116.6801&distance=5000&access_token=FACEBOOK_ACCESS_TOKEN
// -> locations:	365417156884439, 175596392475250, 932213996814361
// https://www.facebook.com/175596392475250
// https://www.facebook.com/search/365417156884439/photos-in  -> list of pictures fbid=X&
// https://www.facebook.com/photo.php?fbid=10200800162961935 -> date of first pic

https://graph.facebook.com/175596392475250/photos&access_token=FACEBOOK_ACCESS_TOKEN
https://graph.facebook.com/search/175596392475250/photos-in?access_token=FACEBOOK_ACCESS_TOKEN

https://graph.facebook.com/v2.5/{photo-id}  or {page-id}
https://graph.facebook.com/v2.5/10200800162961935
https://graph.facebook.com/v2.5/365417156884439?access_token=FACEBOOK_ACCESS_TOKEN
https://graph.facebook.com/v2.5/365417156884439/photos/tagged?access_token=FACEBOOK_ACCESS_TOKEN
https://graph.facebook.com/v2.5/365417156884439?fields=albums.limit(5){name,photos.limit(10){name,picture}}&access_token=FACEBOOK_ACCESS_TOKEN

https://graph.facebook.com/v2.5/365417156884439?fields=place_type,name&access_token=FACEBOOK_ACCESS_TOKEN
https://graph.facebook.com/v2.5/365417156884439?fields=place_type,name,albums,photos,picture&access_token=FACEBOOK_ACCESS_TOKEN

//https://graph.facebook.com/endpoint?key=value&amp;access_token=app_id|app_secret
//https://graph.facebook.com/175596392475250?fields=albums.limit(5){name,photos.limit(10){name,picture}}&access_token=FACEBOOK_ACCESS_TOKEN

https://www.facebook.com/photo.php?fbid=10200800162961935 <--- link to pic
https://www.facebook.com/photo.php?fbid=365417156884439
*/


	

static double facebookticks;
int FacebookPicList(double lat, double lng, double dist, CSymList &list)
{
			DownloadFile f;
			CString url = MkString("https://graph.facebook.com/search?q=&type=place&center=%s,%s&distance=%d&access_token=FACEBOOK_ACCESS_TOKEN", CCoord(lat), CCoord(lng), (int)dist);
				// force 1 download/sec (<100,000 limit per day)
			Throttle(facebookticks, 1000);
			if (f.Download(url))
					{
					Log(LOGERR, "Could not download %s", url);
					return 0;
					}

			vara ids, names;
			CDoubleArrayList lats, lngs;
			int skip = -1;
			const char *val;
			for (int i=0; f.memory[i]!=0; ++i)
				{
				if (f.memory[i]=='[') 
					++skip;
				if (f.memory[i]==']') 
					--skip;
				if (skip==0) {
					if (IsSimilar(f.memory+i, val="\"id\":"))
						ids.push(ExtractString(f.memory+i, val, "\"", "\""));
					if (IsSimilar(f.memory+i, val="\"name\":"))
						names.push(ExtractString(f.memory+i, val, "\"", "\""));
					if (IsSimilar(f.memory+i, val="\"latitude\":"))
						lats.push(ExtractNum(f.memory+i, val, "", "\n"));
					if (IsSimilar(f.memory+i, val="\"longitude\":"))
						lngs.push(ExtractNum(f.memory+i, val, "", "\n"));
					}
				}
			
			vara doneids;
			int nloc = 0;
			for (int i=0; i<ids.length(); ++i)
				{
				if (doneids.indexOf(ids[i])>=0)
					continue;
				if (nloc++>MAXLOCATIONS)
					break;
				doneids.push(ids[i]);

				if (Distance(lat, lng, lats[i], lngs[i])>dist)
					continue;

				CSym sym(MkString("http://%s/rwr?pictures=%s,%s,%s&mode=F,%s,%s", server, CData(lats[i]), CData(lngs[i]), CData(dist), url_encode(ids[i]), url_encode(names[i])));
				sym.SetNum(0, Distance(lat, lng, lats[i], lngs[i]));
				list.Add(sym);				
				}

			return ids.length();
}



int FacebookPic(double lat, double lng, double dist, const char *locid, const char *locname, CSymList &list)
{

			int n = 0;
			DownloadFile f;
			/*
			CString url = "https://www.facebook.com/login.php?login_attempt=1&lwv=110?POST?email=sdgrottofb@gmail.com&pass=caving2000";
			if (f.Download(url))
				{
				Log(LOGERR, "Could not login at %s", url);
				return 0;
				}
			*/
			FACEBOOK_Login(f);
			
			//vars link = "https://www.facebook.com/search/"+ids[i]+"/photos-in";
			vars rlink = "https://www.facebook.com/"+vars(locid);
			// get latest update
			vars rurl = rlink, orurl;
			while (!rurl.IsEmpty() )
			{
			Throttle(facebookticks, 1000);
			if (f.Download(orurl = rurl)) //, "fbtest.htm"))
				{
				Log(LOGERR, "Could not download %s", rurl);
				break;
				}
			//ASSERT( !strstr(names[i],"Yosemite National Park") );
			
			vara dates(f.memory, "<abbr title=");
			for (int d=1; d<dates.length(); ++d)
				{
				static const char *weekdays[] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun", NULL };
				//vars date = GetTokenRest(ExtractString(dates[d], "\"", "", "\""), 1);
				vars date = ExtractString(dates[d], "\"", "", "\"").replace(",", " ").replace("  ", " ");
				if (IsMatch(date, weekdays))
					date = GetTokenRest(date, 1, ' ');

				// collect list of thumbnails (max 3)
				//pic = cleanup(ExtractString(f.memory, "uiScaledImageContainer", "src=\"", "\""));
				//vara list(f.memory, "uiScaledImageContainer");
				vars link, pic, glink;
				const char *photos[] = { "/photo.php?fbid=", "/photos/", NULL};
				for (int p=0; (link.IsEmpty() || pic.IsEmpty()) && photos[p]!=NULL; ++p) {
					link = cleanup(ExtractLink(dates[d], photos[p]));
					pic = cleanup(ExtractString(strstr(dates[d], photos[p]), "<img ", "src=\"", "\""));
					if (!link.IsEmpty())
						glink = link;
					}
				if (glink.IsEmpty())
					continue;
				if (strstr(pic, "/hprofile-xt"))
					pic = "";
				// get latest date
				if (!IsSimilar(glink,"http"))
					glink = "http://www.facebook.com"+glink;
				++n;
				vars desc = stripHTML(ExtractString(dates[d], "userContent\"", ">", "</div>"));
				AddPic(list, "Facebook.com", glink, desc, lat, lng, pic, date, "MMM D YYYY", "<a href='"+rlink+"' target='_blank'><b>"+vars(locname)+"</b></a>");
				}
#ifdef DEBUG
			Log(LOGINFO, "FACEBOOK %d pics <- %s", n, orurl);
#endif
			// detect redirect
			/*
			const char *jump[] = { "window.location.replace(", "window.location.href=", NULL };
			rurl = "";
			for (int i=0; jump[i]!=NULL && rurl.IsEmpty(); ++i)
				rurl = cleanup(ExtractString(f.memory, jump[i], "\"", "\")"));
			if (rurl.IsEmpty() || rurl==orurl)
				break;
			*/
			rurl = ExtractString(ExtractString(f.memory, "<meta http-equiv=\"refresh\"", "", "/>"), "url=", "", "\""); 
			if (rurl.IsEmpty())
				{
				rurl = ExtractString(f.memory, "<link rel=\"canonical\"", "href=\"", "\"");
				if (orurl == rurl)
					rurl = "";
				}
			}

			return n;
}


int GetPicturesList(double lat, double lng, double dist, CSymList &list)
{
	if (lat==InvalidNUM || lng==InvalidNUM || dist==InvalidNUM)
		return 0;

	list.Add( CSym(MkString("http://%s/rwr?pictures=%s,%s,%s&mode=R", server, CData(lat), CData(lng), CData(dist)), "0") );
	//list.Add( CSym(MkString("http://%s/rwr?pictures=%s,%s,%s&mode=P", server, CData(lat), CData(lng), CData(dist)), "0") );
	FacebookPicList(lat, lng, dist, list);
	InstagramPicList(lat, lng, dist, list);
	list.SortNum(0);
	return list.GetSize();
}

int GetPictures(double lat, double lng, double dist, const char *mode, const char *id, const char *name, CSymList &symlist)
{
	if (lat==InvalidNUM || lng==InvalidNUM || dist==InvalidNUM)
		return 0;

	CSymList list;
	switch (*mode)
		{
			case 'R':
				FlickrPic(lat, lng, dist, id, name, list);
				break;
			case 'P':
				PanoramioPic(lat, lng, dist, id, name, list);
				break;
			case 'I':
				InstagramPic(lat, lng, dist, id, name, list);
				break;
			case 'F':
				FacebookPic(lat, lng, dist, id, name, list);
				break;
			default:
				Log(LOGERR, "Illegal picture mode %s", mode);
				return 0;
				break;
		}

	list.SortNum(ITEM_DATE, -1);
	for (int n=0; n<list.GetSize() && n<MAXPICS; ++n)
		{
		CSym &sym = list[n];
		if (sym.GetNum(ITEM_DATE)==InvalidNUM)
			{
			sym.SetStr(ITEM_DATE, CData(GetCurrentTime()));
			sym.SetStr(ITEM_SUMMARY, "Date Unknown");
			}
		if (sym.GetStr(ITEM_PIC).IsEmpty())
			sym.SetStr(ITEM_PIC, "http://ropewiki.com/images/8/80/Camera.png");
		symlist.Add(sym);
		}

	return list.GetSize();
}


int PictureInfo(const char *q, inetdata &data)
	{
	vara sum;
	vara coords(q);
	if (coords.length()<3)
		return 0;

	CString out;
	CSymList list;
	const char *query2 = ucheck(q, "mode=");
	if (query2)
		{
		vars id = url_decode(GetToken(query2, 1));
		vars name = url_decode(GetTokenRest(query2, 2));
		GetPictures(CDATA::GetNum(coords[0]), CDATA::GetNum(coords[1]), CDATA::GetNum(coords[2]), query2, id, name, list);

		out += "{\"list\":[";
		for (int i=0; i<list.GetSize(); ++i)
			{
			vars line;
			if (i>0) line += ",";
			line += "\"" + list[i].Line() + "\"";
			out += line;
			}
		out += "]}\r\n";
		}
	else
		{
		GetPicturesList(CDATA::GetNum(coords[0]), CDATA::GetNum(coords[1]), CDATA::GetNum(coords[2]), list);
		out += "{\"list\":[";
		//Log(LOGINFO, "Recent pictures %s => %d pics", q, list.GetSize());
		for (int i=0; i<list.GetSize(); ++i)
			{
			vars line;
			if (i>0) line += ",";
			line += "\"" + list[i].id + "\"";
			out += line;
			}
		out += "]}\r\n";
		}

	data.write(out);
	return list.GetSize();
	}


int PictureQuery(const char *query, inetdata &data)
{
#ifndef DEBUG
	__try { 
#endif
	return PictureInfo(query, data);
#ifndef DEBUG
} __except(EXCEPTION_EXECUTE_HANDLER) 
	{ 
	Log(LOGALERT, "PICEXTRACT CRASHED!!! %.200s", query); 	
	return FALSE; 
	}
#endif
}


void TestPics(void)
{
	//CSymList list;
	//list.Load("A.csv");
	//list.Load("X.csv");
	//gmlcanadafix("test.tst", list);
#if 0
	CBox cb(&LLRect(10, -120, 50, -50), 5);
	for (LLRect *box =cb.Start(); box!=NULL; box=cb.Next())
		{
		CBox cb2(box, 0.5);
		for (LLRect *box =cb2.Start(); box!=NULL; box=cb2.Next())
			{
			printf("%.2f %% %g,%g     \r", cb.Progress(cb2.Progress()), box->lat1, box->lng1);
			Sleep(5);
			}
		}	
#endif
	CSymList list;
	//GetPictures(45.5894,-122.0764,1609, list);
	//GetPicturesList(32.971054, -116.688326, 2609, list);
	//list.Save("picextract.csv");

	DownloadFile f;
	if (!f.Download("https://www.instagram.com/explore/locations/844998633/", "itestx.html"))
		{
		vara list;
		vara nodes(ExtractString(f.memory, "\"LocationsPage\"", NULL, "</script"), "\"nodes\"");
		int n = jsonarray(nodes, list);
		for (int i=0; i<list.GetSize(); ++i)
			Log(list[i]);
		}
}
