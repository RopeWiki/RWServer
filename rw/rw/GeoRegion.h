#pragma once

#include "Books.h"
#include "BETAExtract.h"
#include "trademaster.h"

// ===============================================================================================
#define GEOCODETICKS 200
static double geocodeticks;

#define GEOCODE "geocode"
#define NOGEOCODE "geocodeerr"

class GeoCache {
	BOOL init;
	CSymList list, nolist;

public:
	GeoCache(void)
	{
		init = FALSE;
	}

	~GeoCache(void)
	{
		/*
		if (init<0)
			{
			list.Save(filename(GEOCODE));
			nolist.Save(filename(NOGEOCODE));
			}
		*/
	}


	vars idstr(const char *addr)
	{
		return vars(addr).replace(",", ";");
	}

	int GetNoCache(const char *address, double &lat, double &lng);

	int Get(const char *address, double &lat, double &lng);

};


#define GEOREGION "georegion"
#define NOGEOREGION "georegionerr"
#define GEOREGIONFIX "georegionfix"
#define GEOREGIONTRANS "georegiontrans"
#define GEOREGIONITRANS "georegionredir"
#define GEOREGIONNOMAJOR "georegionnomaj"
#define GEOREGIONLEV "georegionlev"
#define RWREGIONS "rwreg"

class GeoRegion {
	BOOL init;
	vars key;
	CSymList list, nolist, translist, levlist;

public:
	int overlimit;

	GeoRegion(void)
	{
		init = FALSE;
		overlimit = 0;
		key = "&key=AIzaSyA5GHhRPXaJTiD-lbTOE3I3sn2dQ5lUlq0";

		/*
		// Patch georegion
		CSymList list;
		vars replace = "TOBEREPLACED";
		list.Load(filename(GEOREGION));
		for (int i=0; i<list.GetSize(); ++i)
			list[i].data = GeoRegion::bestgeoregion(list[i].data);
		list.Save(filename(GEOREGION));
		*/
	}
	
	~GeoRegion(void)
	{
		/*
		if (init)
			{
			list.Save(filename(GEOREGION));
			nolist.Save(filename(NOGEOREGION));
			}
		*/
	}

	int Translate(vara &regions);

	static vars bestgeoregion(const char *data);

	static int okgeoregion(const char *reg);

	static int cmpgeoregion(const void *arg1, const void *arg2);

	int Get(CSym &sym, vara &regions);

	int GetRegion(CSym &sym, vara &georegions, int *forced = NULL);

	vars GetSubRegion(const char *region, const char *region2, int exact = FALSE);

	CSymList oregionlist, oredirectlist;

	CSymList &Redirects(void)
	{
		return oredirectlist;
	}

	CSymList &Regions(void);

	vars GetRegion(const char *oregion, BOOL check = FALSE);

	void AddRegion(const char *region, const char *parent);

	vars GetFullRegion(const char *oregion);

};

