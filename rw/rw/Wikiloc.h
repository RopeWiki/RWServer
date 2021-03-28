// Wikiloc.h : header file
//

#pragma once

#include "inetdata.h"

#define WIKILOC "wikiloc.com"

#define WIKILOCTICKS 3000

#define MAXWIKILOCLINKS 5

int WIKILOC_ExtractKML(const char *ubase, const char *url, inetdata *out, int fx);
int WIKILOC_DownloadPage(DownloadFile &f, const char *url, CSym &sym);
int WIKI_DownloadBeta(const char *ubase, CSymList &symlist, int(*DownloadPage)(DownloadFile &f, const char *url, CSym &sym));
int WIKILOC_DownloadBeta(const char *ubase, CSymList &symlist);
void WIKILOC_ProcessWaypoint(vars* entry, vara* waypoints, CString* credit, const char* url);
void WIKILOC_ProcessTrack(vars* entry, vara* tracks, vara* waypoints, CString* credit, const char *url);
CString WIKILOC_ExtractWaypointCoords(vars* entry, CString* type, const char* style, vars* name, vara* waypoints, CString* credit, const char *url);


//class to decode Wikiloc encoded polyline
#include <string>
#include <vector>

typedef unsigned char BYTE;

class WikilocCPPDecode
{
public:
	class wikilocData
	{
	public:
		std::vector<unsigned char> buffer;
		int cursor = 0;
		std::vector<int> refpoint;
		int ndims = 0;
		std::vector<double> factors;
	};

	static std::vector<std::vector<double>> decodeWikilocEncodedPath(CString encodedPath);

	static std::vector<BYTE> base64_decode(CString encoded_string);

	static std::vector<std::vector<double>> toGeoJSON(std::vector<unsigned char> &buffer);

	static std::vector<double> readBuffer(wikilocData *wlData);

	static std::vector<double> readObjects(wikilocData *wlData);

	static std::vector<double> parse_line(wikilocData *wlData);

	static std::vector<double> read_pa(wikilocData *wlData, int npoints);

	static int ReadVarSInt64(wikilocData *wlData);

	static int ReadVarInt64(wikilocData *wlData);

	static int unzigzag(int nVal);

	static std::vector<std::vector<double>> toCoords(std::vector<double> &coordinates, int ndims);

	static bool is_base64(const unsigned char c) {
		return isalnum(c) || c == '+' || c == '/';
	}

	static const std::string base64_chars;
};
