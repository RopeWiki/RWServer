
#include "GeoRegion.h"
#include "BETAExtract.h"
#include "trademaster.h"


	int GeoCache::GetNoCache(const char *address, double &lat, double &lng)
	{
		// get new geocode
		init = -1;
		DownloadFile f;
		Throttle(geocodeticks, GEOCODETICKS);
		vars url = vars("https://maps.googleapis.com/maps/api/geocode/xml?address=") + url_encode(stripAccents(address));
		if (f.Download(url))
			return FALSE;
		lat = ExtractNum(f.memory, "", "<lat>", "</lat>");
		lng = ExtractNum(f.memory, "", "<lng>", "</lng>");
		return (lat != InvalidNUM && lng != InvalidNUM);
	}

	int GeoCache::Get(const char *address, double &lat, double &lng)
	{
		if (!init)
		{
			init = TRUE;
			list.Load(filename(GEOCODE));
			list.Sort();
			if (CFILE::exist(filename(NOGEOCODE)))
				nolist.Load(filename(NOGEOCODE));
			nolist.Sort();
		}

		if (!address || *address == 0)
			return FALSE;

		// lat;lng
		lat = CDATA::GetNum(address);
		lng = CDATA::GetNum(GetToken(address, 1, ",;"));
		if (CheckLL(lat, lng))
			return TRUE;

		int found = list.Find(idstr(address));
		if (found >= 0)
		{
			lat = list[found].GetNum(0);
			lng = list[found].GetNum(1);
			return TRUE;
		}
		if (nolist.Find(idstr(address)) >= 0)
			return FALSE;

		GetNoCache(address, lat, lng);

		/*
				if (!CheckLL(lat,lng) && address.Find("/")>=0)
					{
					vars naddress = address;
					ExtractStringDel(naddress, "/", "", ".");
					if (vara(naddress, ".")>2)
						GetNoCache(naddress, lat, lng);
					}
		*/
		if (lat != InvalidNUM && lng != InvalidNUM)
		{
			CSym sym(idstr(address), CData(lat) + "," + CData(lng));
			list.Add(sym);
			list.Sort();
			list.Save(filename(GEOCODE));
			return TRUE;
		}
		nolist.Add(CSym(idstr(address)));
		nolist.Sort();
		nolist.Save(filename(NOGEOCODE));
		return FALSE;
	}


	int GeoRegion::Translate(vara &regions)
	{
		// apply translations
		vars region = regions.join(";");
		for (int d = 0; d < translist.GetSize(); ++d)
			region.Replace(translist[d].id, translist[d].data);
		while (region.Replace(" ;", ";"));
		while (region.Replace("; ", ";"));

		regions = vara(region, ";");
		return regions.length() > 0;
	}

	vars GeoRegion::bestgeoregion(const char *data)
	{
		vara list(data);
		int maxbest = 10;
		vara best; best.SetSize(maxbest);

		for (int i = 1; i < list.length(); ++i)
		{
			if (list[i] == "TOBEREPLACED")
				continue;
			vara glist(list[i], ";");
			for (int g = 0; g < glist.length(); ++g)
			{
				double n = CDATA::GetNum(GetToken(glist[g], 1, ':'));
				if (n < 0 || n >= maxbest)
					continue;

				vars &bestn = best[(int)n];
				if (bestn.IsEmpty())
					bestn = glist[g];
			}
		}

		// truncate incomplete lists
		for (int i = 0; i < best.length(); ++i)
			if (best[i].IsEmpty())
				best.SetSize(i);


		list[0] = best.join(";");
		return list.join().Trim(" ;");
	}


	int GeoRegion::okgeoregion(const char *reg)
	{
		vara r(reg, ";");

		char last = 0;
		for (int i = 0; i < r.length(); ++i)
		{
			vars n = GetToken(r[i], 1, ':');
			if (last != 0 && n[0] != last + 1)
				return FALSE;
			last = n[0];
		}
		return TRUE;
	}

	int GeoRegion::cmpgeoregion(const void *arg1, const void *arg2)
	{
		const char *s1 = *((const char **)arg1);
		const char *s2 = *((const char **)arg2);
		const char *a1 = strrchr(s1, ':');
		const char *a2 = strrchr(s2, ':');
		/*
		int ok = okgeoregion(s1)-okgeoregion(s2);
		if (ok!=0)
			return ok;
		*/
		int a = -strcmp(a1, a2);
		if (a != 0)
			return a;
		/*
		int s = -strcmp(s1, s2);
		if (s!=0)
			return s;
		*/
		return 0;
	}

	int GeoRegion::Get(CSym &sym, vara &regions)
	{
		if (!init)
		{
			init = TRUE;
			list.Load(filename(GEOREGION));
			list.Sort();
			/*
			for (int i=0; i<list.GetSize(); ++i)
				{
				vars data = list[i].data;
				data = invertregion(data, "");
				list[i].data = data;
				}
			*/

			if (CFILE::exist(filename(NOGEOREGION)))
				nolist.Load(filename(NOGEOREGION));
			nolist.Sort();

			// translate regions
			translist.Load(filename(GEOREGIONTRANS));

			// max level
			levlist.Load(filename(GEOREGIONLEV));
		}

		double olat = sym.GetNum(ITEM_LAT);
		double olng = sym.GetNum(ITEM_LNG);
		if (!CheckLL(olat, olng))
		{
			// check geocode
			vars geocode = GetToken(sym.GetStr(ITEM_LAT), 1, '@');
			GeoCache GeoCode;

			if (geocode.IsEmpty() || !GeoCode.Get(geocode, olat, olng) || !CheckLL(olat, olng))
				return FALSE;
		}

		double eps = 1e-4;
		double lat = round(olat / eps)*eps, lng = round(olng / eps)*eps;
		vars address = CData(lat) + "x" + CData(lng);
		address.Replace(" ", "");
		int found = list.Find(address);
		if (found >= 0)
		{
			regions = vara(list[found].GetStr(0), ";");
			return Translate(regions);
		}
		if (nolist.Find(address) >= 0)
			return FALSE;

		// get new geocode
		if (overlimit > 5)
		{
			++overlimit;
			return FALSE;
		}

		init = -1;
		DownloadFile f;

		// retry 3 times untill it can get maxlev>=2
		double maxlev = -1;
		const double eval[] = { 0, -eps, eps };
		vara georegions, georegionsxtra; georegions.push("");
		for (int e = 0; e < 3 && maxlev < 2; ++e)
		{
			Throttle(geocodeticks, GEOCODETICKS);
			vars address = CData(lat + eval[e]) + "," + CData(lng + eval[e]);
			vars url = vars("https://maps.googleapis.com/maps/api/geocode/xml?latlng=") + address.replace(" ", "") + "&language=en" + key; //&key=AIzaSyBCOHje4lTAB2-P1UZjtNvEmP7HsntdUh8"; //&key=AIzaSyA5GHhRPXaJTiD-lbTOE3I3sn2dQ5lUlq0"; //"&key=";

			if (f.Download(url))
				return FALSE;
			if (strstr(f.memory, "OVER_QUERY_LIMIT"))
			{
				if (!key.IsEmpty())
				{
					key = "";
					return Get(sym, regions);
				}
				++overlimit;
				return FALSE;
			}

			regions.RemoveAll();
			vara rlist(f.memory, "<result>");

			for (int r = 1; r < rlist.length(); ++r)
			{
				vara alist(rlist[r], "<address_component>");
				if (alist.length() <= 0)
					continue;

				// result type has to be 'political' (different from address
				int rpolitical = strstr(alist[0], "<type>political</type") != NULL;

				vara current;
				for (int i = 1; i < alist.length(); ++i)
				{
					double lev = ExtractNum(alist[i], "<type>administrative_area_level_", "", "</type>");
					if (strstr(alist[i], "<type>country")) lev = 0;
					if (lev < 0)
						continue;
					if (lev > maxlev)
						maxlev = lev;
					vars name = ExtractString(alist[i], "<long_name>", "", "</long_name>").trim();
					int political = strstr(alist[i], "<type>political</type>") != NULL;
					current.push(MkString("%s:%s%s", stripAccents(name), CData(lev), political ? "P" : ""));
				}

				// store result
				if (current.length() > 0)
				{
					vars georegion = invertregion(current.join(), "");
					if (georegions.indexOf(georegion) < 0 && georegionsxtra.indexOf(georegion) < 0)
						if (rpolitical)
							georegions.push(georegion);
						else
							georegionsxtra.push(georegion);
				}
			}
		}
		// unify in one list
		georegions.Append(georegionsxtra);
		if (georegions.GetSize() > 1)
		{
			CSym sym(address, GeoRegion::bestgeoregion(georegions.join()));

			list.Add(sym);
			list.Sort();
			list.Save(filename(GEOREGION));
			regions = vara(sym.GetStr(0), ";");
			return Translate(regions);
		}
		// error
		nolist.Add(CSym(address));
		nolist.Sort();
		nolist.Save(filename(NOGEOREGION));
		return FALSE;
	}
	   
	int GeoRegion::GetRegion(CSym &sym, vara &georegions, int *forced)
	{
		if (!Get(sym, georegions))
			return -1;

		int maxr = -1;
		// auto detect max level
		vars country = GetToken(georegions[0], 0, ':');
		for (int r = georegions.length() - 1; r >= 0; --r)
		{
			int f;
			vars region = GetToken(georegions[r], 0, ':');
			if ((f = levlist.Find(georegions[r])) >= 0)
			{
				int maxlev = (int)levlist[f].GetNum(0);
				if (forced) *forced = maxlev;
				return maxlev; // level is specified in levlist
			}
			if (!GetRegion(region, TRUE).IsEmpty())
				if (strstri(GetFullRegion(region), country))
					if (r > maxr)
						maxr = r;
		}

		return maxr;
	}

	vars GeoRegion::GetSubRegion(const char *region, const char *region2, int exact)
	{
		if (exact)
		{
			vara list(GetFullRegion(region), ";");
			int n = list.indexOfi(region2);
			return n < 0 ? "" : region2;
		}
		vars fullreg = GetFullRegion(region);
		const char *subreg = strstri(fullreg, region2);
		return !subreg ? "" : subreg;
	}
	
	CSymList& GeoRegion::Regions(void)
	{
		if (oregionlist.GetSize() == 0)
		{
			oregionlist.Load(filename(RWREGIONS));
			oregionlist.iSort();

			CSymList tmp;
			tmp.Load(filename(RWREDIR));
			for (int i = 0; i < tmp.GetSize(); ++i)
			{
				CSym &sym = tmp[i];
				vars name = sym.GetStr(ITEM_DESC);
				if (oregionlist.Find(name) < 0)
					continue;
				// process only region redirects
				vara aka(sym.GetStr(ITEM_AKA), ";");
				for (int a = 0; a < aka.length(); ++a)
					oredirectlist.AddUnique(CSym(aka[a], name));
			}

			oredirectlist.iSort();
			oregionlist.iSort();
		}
		return oregionlist;
	}

	vars GeoRegion::GetRegion(const char *oregion, BOOL check)
	{
		CSymList &oregionlist = Regions();

		if (oregion == NULL || *oregion == 0)
			return "";

		vars region = vara(oregion, ";").last().Trim();

		while (!region.IsEmpty())
		{
			int found;
			if ((found = oregionlist.Find(region)) >= 0)
				return oregionlist[found].id;
			if ((found = oredirectlist.Find(region)) >= 0)
			{
				region = oredirectlist[found].data;
				continue; // there may be more redirects
			}
			break;
		}

		if (!check)
			Log(LOGERR, "Unknown region '%s'", oregion);
		return "";
	}

	void GeoRegion::AddRegion(const char *region, const char *parent)
	{
		if (!GetRegion(region).IsEmpty())
			return;
		oregionlist.AddUnique(CSym(region, parent));
	}

	vars GeoRegion::GetFullRegion(const char *oregion)
	{
		if (!oregion || *oregion == 0)
			return "";

		vars region = GetRegion(GetToken(vara(oregion, ";").last(), 0, ':'));
		if (region.IsEmpty())
			return "";

		// precomputed
		CSymList &regions = Regions();
		int or = regions.Find(region);
		if (or < 0)
			return "";
		vars fullregion = regions[or ].GetStr(ITEM_MATCH);
		if (!fullregion.IsEmpty())
			return fullregion;

		// compute now
		vara regionlist;
		vara parent;
		parent.push(region);
		while (parent.length() > 0 && !IsSimilar(parent.last(), "world"))
		{
			regionlist.Append(parent);

			vara newparent;
			for (int i = 0; i < parent.length(); ++i)
			{

				int r = regions.Find(parent[i]);
				if (r >= 0)
				{
					vara newparents(regions[r].GetStr(0), ";");
					newparent.Append(newparents);
				}
				else
					r = r;
			}
			parent = newparent;
		}
		fullregion = invertregion(regionlist.join(), "");
		regions[or ].SetStr(ITEM_MATCH, fullregion);
		return fullregion;
	}

