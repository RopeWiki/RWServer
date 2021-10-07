// Wikiloc.cpp : source file
//

#include "Wikiloc.h"
#include "BETAExtract.h"
#include "passwords.h"

// ===============================================================================================

int WIKILOC_ExtractKML(const char *ubase, const char *url, inetdata *out, int fx)
{	
	DownloadFile f;

	static double wikilocticks;
	Throttle(wikilocticks, WIKILOCTICKS);
	if (DownloadRetry(f, url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}

	const vars user = stripHTML(ExtractString(f.memory, "\"author\":", R"("name": ")", "\","));
	CString credit = " (Data by " + user + " at " + CString(ubase) + ")";
	const vars desc = stripHTML(ExtractString(f.memory, "\"mainEntity\":", R"("name": ")", "\",")) + " " + credit;

	vara styles, waypoints, tracks;
	styles.push(  "dot=http://maps.google.com/mapfiles/kml/shapes/open-diamond.png");
	styles.push("begin=http://maps.google.com/mapfiles/kml/paddle/go.png");
	styles.push(  "end=http://maps.google.com/mapfiles/kml/paddle/red-square.png");
	
	const vars mapData = stripHTML(ExtractString(f.memory, "\"mapData\":", "[", "]"));
		
	vara wpt(mapData, "{");
	for (int i = 1; i < wpt.length(); ++i)
	{
		vars isWaypoint = ExtractString(wpt[i], "\"waypoint\"", ":", nullptr); //empty endchar will default to terminate with & " < }

		if (isWaypoint == "true")
		{
			WIKILOC_ProcessWaypoint(&wpt[i], &waypoints, &credit, url);
		}
		else
		{
			WIKILOC_ProcessTrack(&wpt[i], &tracks, &waypoints, &credit, url);
		}		
	}

	if (waypoints.length() == 0 && tracks.length() == 0)
		return FALSE;

	// generate kml
	SaveKML("WikiLoc", desc, url, styles, waypoints, tracks, out);
	return TRUE;
}

void WIKILOC_ProcessWaypoint(vars* entry, vara* waypoints, CString* credit, const char* url)
{
	vars name = ExtractString(*entry, "\"nom\":", "\"", "\"");
	if (name.IsEmpty()) return;

	CString type = "b";	
	WIKILOC_ExtractWaypointCoords(entry, &type, "dot", &name, waypoints, credit, url);
}

void WIKILOC_ProcessTrack(vars* entry, vara* tracks, vara* waypoints, CString* credit, const char *url)
{
	CString type;  vars name;

	//begin waypoint
	type = "b"; name = "Start";
	WIKILOC_ExtractWaypointCoords(entry, &type, "begin", &name, waypoints, credit, url);

	//end waypoint
	type = "e"; name = "End";
	const auto end = WIKILOC_ExtractWaypointCoords(entry, &type, "end", &name, waypoints, credit, url);

	name = ExtractString(*entry, "\"nom\":", "\"", "\"");

	const auto encodedPolyline = ExtractString(*entry, "\"geom\":", "\"", "\"");

	//the following code to decode the polyline string was converted from the embedded javascript on wikiloc:
	vara linelist;
	//linelist.push("geom:" + encodedPolyline.c_str()); //<- this was to pass it off to our javascript, before the decoding code was converted to C++
	
	auto coords = WikilocCPPDecode::decodeWikilocEncodedPath(encodedPolyline);

	for (int i = 0; i < coords.size(); i++) {
		linelist.push(CCoord3(coords[i][1], coords[i][0]));
	}
	//the encoded polyline stops short of including the final 'end' waypoint. Add this to the end of the track
	linelist.push(end);


	tracks->push(KMLLine("Track", *credit, linelist, OTHER, 3));

	//luca's original code to add coordinates:
		
	/*vara linelist;
	vara lats(ExtractString(f.memory, "lat:", "[", "]"));
	vara lngs(ExtractString(f.memory, "lng:", "[", "]"));
	for (int i = 0; i < lats.length() && i < lngs.length(); ++i)
	{
		double lat = CDATA::GetNum(lats[i]);
		double lng = CDATA::GetNum(lngs[i]);
		linelist.push(CCoord3(lat, lng));
	}	
	tracks->push(KMLLine("Track", name, linelist, OTHER, 3));*/
}

CString WIKILOC_ExtractWaypointCoords(vars* entry, CString* type, const char* style, vars* name, vara* waypoints, CString* credit, const char *url)
{
	const vars latStr = ExtractString(*entry, "\"" + *type + "lat\"", ":", ";");
	const vars lngStr = ExtractString(*entry, "\"" + *type + "lng\"", ":", ";");

	if (latStr.IsEmpty() || lngStr.IsEmpty()) return nullptr;

	const double lat = CDATA::GetNum(latStr);
	const double lng = CDATA::GetNum(lngStr);

	if (!CheckLL(lat, lng, url)) return nullptr;

	waypoints->push(KMLMarker(style, lat, lng, *name, *credit));

	return CCoord3(lat, lng);
}

int WIKILOC_DownloadPage(DownloadFile &f, const char *url, CSym &sym)
{
	if (strstr(url, "imgServer"))
	{
		Log(LOGERR, "ERROR wikiloc.com ImgServer reference: %s", url);
		return FALSE;
	}
	if (f.Download(url))
	{
		Log(LOGERR, "ERROR: can't download url %.128s", url);
		return FALSE;
	}

	vara coords(ExtractString(f.memory, "maps.LatLng(", "", ")"));
	if (coords.length() > 1)
	{
		sym.SetStr(ITEM_LAT, coords[0]);
		sym.SetStr(ITEM_LNG, coords[1]);
	}
	/*
		vara rating(ExtractString(f.memory, "title=\"Rated:","","\""), "(");
		double stars = 0, num = 0;
		if (rating.length()>1)
			{
			stars = CDATA::GetNum(rating[0]);
			num = CDATA::GetNum(rating[1]);
			sym.SetStr(ITEM_STARS, starstr(stars, num));
			}
	*/
	sym.SetStr(ITEM_KML, "X");

	vara wpt(f.memory, "maps.LatLng(");
	vars description = stripHTML(ExtractString(f.memory, "class=\"description\"", ">", "</div"));
	description.Replace(sym.GetStr(ITEM_DESC), "");
	const int n = wpt.length() - 2;
	const int len = description.length();
	const int score = n * 100 + len;
	const vars desc = MkString("%05d:%db+%dwpt", score, len, n);
	if (score < 300) sym.SetStr(ITEM_CLASS, "0:" + desc);
	else sym.SetStr(ITEM_CLASS, "1:" + desc);

	return TRUE;
}

/*
int WIKI_DownloadBeta(const char *ubase, CSymList &symlist, int (*DownloadPage)(DownloadFile &f, const char *url, CSym &sym))
{
	int n = 0, step = 20;
	DownloadFile f;
	int from=0;
	vars url = "http://www.wikiloc.com/trails/canyoneering";
	while (!url.IsEmpty())
	{
		if (f.Download(url))
			{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			return FALSE;
			}
		url = ExtractLink(f.memory, ">Next&nbsp;&rarr;");
		double count = ExtractNum(f.memory, "&nbsp;of&nbsp;", "", "&nbsp;");
		if (count<=0 || count>10000)
			{
			Log(LOGERR, "Bad count %s!", CData(count));
			return FALSE;
			}

		vara list(f.memory, "wikiloc/view.do?id=");
		for (int i=1; i<list.length(); ++i)
			{
			vars id = ExtractString(list[i], "", "", "\"").Trim(" \"'");
			if (id.IsEmpty())
				continue;

			vars name = stripHTML(ExtractString(list[i], ">", "", "<"));
			vars loc = stripHTML(ExtractString(list[i], "\"near\"", "\"", "\""));
			double lat = ExtractNum(list[i], "\"lat\"", ":", ",");
			double lng = ExtractNum(list[i], "\"lon\"", ":", ",");
			if (!CheckLL(lat,lng))
				{
				Log(LOGERR, "Invalid WIKILOC coords for %s", list[i]);
				continue;
				}

			vars link = "http://wikiloc.com/wikiloc/view.do?id="+id;

			++n;
			CSym sym(urlstr(link), name);
			vars country = stripHTML(ExtractString(loc, "(", "", ")"));
			vars region = invertregion( GetToken(loc, 0, '('), country + " @ ");
			sym.SetNum(ITEM_LAT, lat);
			sym.SetNum(ITEM_LNG, lng);
			sym.SetStr(ITEM_REGION, region);
			sym.SetStr(ITEM_KML, "X");
			vara rated(name,"*");
			double stars = rated[0][strlen(rated[0])-1]-(double)'0';
			if (stars>=1 && stars<=5)
				sym.SetStr(ITEM_STARS, starstr(stars, 1));

			printf("%d %d/%d %d/%d/%d    \r", symlist.GetSize(), i, list.length(), from, n, (int)count);
			if (!UpdateCheck(symlist, sym) && MODE>-2)
				continue;

			if (DownloadPage(f, sym.id, sym))
				Update(symlist, sym, NULL, FALSE);
			}

		// next
		from += step;
		if (from>count)
			break;
	}

	return TRUE;
}

*/

int WIKI_DownloadBeta(const char *ubase, CSymList &symlist, int(*DownloadPage)(DownloadFile &f, const char *url, CSym &sym))
{
	int n = 0;
	const int step = 20;
	DownloadFile f;
	if (f.Download("http://www.wikiloc.com/wikiloc/start.do"))
	{
		Log(LOGERR, "ERROR: can't login in wikiloc");
		return FALSE;
	}
	f.GetForm();
	f.SetFormValue("email", WIKILOC_EMAIL);
	f.SetFormValue("password", WIKILOC_PASSWORD);
	if (f.SubmitForm())
	{
		Log(LOGERR, "ERROR: can't login in wikiloc");
		return FALSE;
	}

	int from = 0;
	while (true)
	{
		//vars url = "https://www.wikiloc.com/wikiloc/find.do?act=46%2C&uom=mi&s=id"+MkString("&from=%d&to=%d", from, from+step); 
		const vars url = "https://www.wikiloc.com/wikiloc/find.do?a=46&z=2&sw=-90%2C-180&ne=90%2C180" + MkString("&from=%d&to=%d", from, from + step);
		//vars url = https://www.wikiloc.com/wikiloc/find.do?a=46&z=2&from=0&to=15&sw=-90%2C-180&ne=90%2C180
		if (f.Download(url)) // sw=-90%%2C-180&ne=90%%2C180&tr=0 https://www.wikiloc.com/wikiloc/find.do?act=46%2C&uom=mi&from=10&to=20
		{
			Log(LOGERR, "ERROR: can't download url %.128s", url);
			return FALSE;
		}

		//double count = ExtractNum(f.memory, "&nbsp;of&nbsp;", "", "</a");
		const double count = ExtractNum(f.memory, "count", ":", ",");
		if (count <= 0 || count > 10000)
		{
			Log(LOGERR, "Count too big %s>10K", CData(count));
			return FALSE;
		}

		vara mem(f.memory, "\"tiles\"");
		vara list(mem.length() > 0 ? mem[0] : f.memory, "{\"id\"");
		for (int i = 1; i < list.length(); ++i)
		{
			vars id = ExtractString(list[i], "", ":", ",").Trim(" \"'");
			if (id.IsEmpty())
				continue;

			vars name = stripHTML(ExtractString(list[i], "\"name\"", "\"", "\""));
			vars loc = stripHTML(ExtractString(list[i], "\"near\"", "\"", "\""));
			const double lat = ExtractNum(list[i], "\"lat\"", ":", ",");
			const double lng = ExtractNum(list[i], "\"lon\"", ":", ",");
			if (!CheckLL(lat, lng))
			{
				Log(LOGERR, "Invalid WIKILOC coords for %s", list[i]);
				continue;
			}

			vars link = "http://wikiloc.com/wikiloc/view.do?id=" + id;

			++n;
			CSym sym(urlstr(link), name);
			vars country = stripHTML(ExtractString(loc, "(", "", ")"));
			vars region = invertregion(GetToken(loc, 0, '('), country + " @ ");
			sym.SetNum(ITEM_LAT, lat);
			sym.SetNum(ITEM_LNG, lng);
			sym.SetStr(ITEM_REGION, region);
			sym.SetStr(ITEM_KML, "X");
			vara rated(name, "*");
			const double stars = rated[0][strlen(rated[0]) - 1] - static_cast<double>('0');
			if (stars >= 1 && stars <= 5)
				sym.SetStr(ITEM_STARS, starstr(stars, 1));

			printf("%d %d/%d %d/%d/%d    \r", symlist.GetSize(), i, list.length(), from, n, static_cast<int>(count));
			if (!UpdateCheck(symlist, sym) && MODE > -2)
				continue;

			if (DownloadPage(f, sym.id, sym))
				Update(symlist, sym, nullptr, FALSE);
		}

		// next
		from += step;
		if (from > count)
			break;
	}

	return TRUE;
}

int WIKILOC_DownloadBeta(const char *ubase, CSymList &symlist)
{
	/*
	if (MODE==-2)
		{
		//TRENCANOUS_DownloadBeta(ubase, symlist, WIKILOC_DownloadPage);
		BARRANQUISMO_DownloadBeta(ubase, symlist, WIKILOC_DownloadPage);
		}
	*/
	WIKI_DownloadBeta(ubase, symlist, WIKILOC_DownloadPage);
	return TRUE;
}

//class to decode Wikiloc encoded polyline

const std::string WikilocCPPDecode::base64_chars =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvwxyz"
	"0123456789+/";

std::vector<std::vector<double>> WikilocCPPDecode::decodeWikilocEncodedPath(const CString encodedPath)
{	
	auto byteArray = base64_decode(encodedPath);

	auto coords = toGeoJSON(byteArray);

	return coords;
}

std::vector<BYTE> WikilocCPPDecode::base64_decode(const CString encodedPath) {
	int in_len = encodedPath.GetLength();
	int i = 0;
	int j;
	int in_ = 0;
	BYTE char_array_4[4], char_array_3[3];
	std::vector<BYTE> ret;

	while (in_len-- && (encodedPath[in_] != '=') && is_base64(encodedPath[in_])) {
		char_array_4[i++] = encodedPath[in_]; in_++;
		if (i == 4) {
			for (i = 0; i < 4; i++)
				char_array_4[i] = base64_chars.find(char_array_4[i]);

			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
			char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

			for (i = 0; (i < 3); i++)
				ret.push_back(char_array_3[i]);
			i = 0;
		}
	}

	if (i) {
		for (j = i; j < 4; j++)
			char_array_4[j] = 0;

		for (j = 0; j < 4; j++)
			char_array_4[j] = base64_chars.find(char_array_4[j]);

		char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
		char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
		char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

		for (j = 0; (j < i - 1); j++) ret.push_back(char_array_3[j]);
	}

	return ret;
}

std::vector<std::vector<double>> WikilocCPPDecode::toGeoJSON(std::vector<unsigned char> &buffer)
{
	auto wlData = new wikilocData();
	wlData->buffer = buffer;
	wlData->cursor = 0;
	wlData->refpoint = std::vector<int>(4);

	auto res = readBuffer(wlData);

	std::vector<std::vector<double>> result = toCoords(res, wlData->ndims);

	delete wlData;

	return result;
}

std::vector<double> WikilocCPPDecode::readBuffer(wikilocData *wlData)
{
	int flag = wlData->buffer[wlData->cursor];

	wlData->cursor++;

	const auto precisionXy = unzigzag((240 & flag) >> 4);

	wlData->factors = std::vector<double>(4);
	wlData->factors[1] = std::pow(10, precisionXy);
	wlData->factors[0] = wlData->factors[1];
	flag = wlData->buffer[wlData->cursor];
	wlData->cursor++;

	int hasZ = 0, hasM = 0;

	const auto extendedDims = (8 & flag) >> 3;

	if (extendedDims > 0)
	{
		const auto extendedDimsFlag = wlData->buffer[wlData->cursor];
		wlData->cursor++;
		hasZ = 1 & extendedDimsFlag;
		hasM = (2 & extendedDimsFlag) >> 1;
		const auto precisionZ = (28 & extendedDimsFlag) >> 2;
		const auto precisionM = (224 & extendedDimsFlag) >> 5;
		if (hasZ > 0)
		{
			wlData->factors[2] = std::pow(10, precisionZ);
		}
		if (hasM > 0)
		{
			wlData->factors[2 + hasZ] = std::pow(10, precisionM);
		}
	}

	const auto ndims = 2 + hasZ + hasM;
	wlData->ndims = ndims;

	const auto has_size = (2 & flag) >> 1;
	if (has_size > 0)
	{
		ReadVarInt64(wlData); //originally had 'size =', need this here to advance the cursor
	}

	return readObjects(wlData);
}

std::vector<double> WikilocCPPDecode::readObjects(wikilocData *wlData)
{
	for (int i = 0; i < wlData->ndims; i++)
	{
		wlData->refpoint[i] = 0;
	}

	return parse_line(wlData);
}

std::vector<double> WikilocCPPDecode::parse_line(wikilocData *wlData)
{
	const auto npoints = ReadVarInt64(wlData);

	return read_pa(wlData, npoints);
}

std::vector<double> WikilocCPPDecode::read_pa(wikilocData *wlData, int npoints)
{
	const auto ndims = wlData->ndims;
	auto factors = wlData->factors;
	auto coords = std::vector<double>(npoints * ndims);

	for (int i = 0; i < npoints; i++)
	{
		for (int j = 0; j < ndims; j++)
		{
			wlData->refpoint[j] += ReadVarSInt64(wlData);
			coords[ndims * i + j] = wlData->refpoint[j] / factors[j];
		}
	}

	return coords;
}

int WikilocCPPDecode::ReadVarSInt64(wikilocData *wlData)
{
	const auto nVal = ReadVarInt64(wlData);
	return unzigzag(nVal);
}

int WikilocCPPDecode::ReadVarInt64(wikilocData *wlData)
{
	auto cursor = wlData->cursor;
	auto nVal = 0;
	auto nShift = 0;

	while (true)
	{
		const int nByte = wlData->buffer[cursor];

		if ((128 & nByte) == 0)
		{
			cursor++;
			wlData->cursor = cursor;
			return nVal | nByte << nShift;
		}

		nVal |= (127 & nByte) << nShift;
		cursor++;
		nShift += 7;
	}
}

int WikilocCPPDecode::unzigzag(int nVal)
{
	return (1 & nVal) == 0 ? nVal >> 1 : -(nVal >> 1) - 1;
}

std::vector<std::vector<double>> WikilocCPPDecode::toCoords(std::vector<double> &coordinates, int ndims)
{
	auto coords = std::vector<std::vector<double>>(coordinates.size() / ndims);

	auto index = 0;

	const int size = coordinates.size();

	for (int i = 0; i < size; i += ndims)
	{
		auto pos = std::vector<double>(4);

		for (int c = 0; c < ndims; ++c)
		{
			pos[c] = coordinates[i + c];
		}

		coords[index++] = pos;
	}

	return coords;
}
