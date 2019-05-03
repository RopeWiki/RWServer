#include "stdafx.h"
#include "inetdata.h"
#include "RWServer.h"


int MODE = -1, SIMPLIFY = 0;

extern LLRect *Savebox;

extern double FLOATVAL;
extern int INVESTIGATE;
int Command(TCHAR *argv[]) // command line parameters
{
	if (!argv || !*argv)
		return FALSE;

	ccmdlist cmd;

	if (cmd.chk(*argv,"-bbox", "bounding box of region, def in regions.csv, use -bbox TEST to get region test.kml"))
	{
		const char *region = argv[1];
		if ( region == NULL || region[0] =='-')
		{
			Log(LOGERR, "Invalid region %s", region);
			exit(0);
		}

		CArrayList<LLRect> boxes; 
		vara regions = vars(region).split();
		{
			CKMLOut out("BBox"); 
			for (int i=0; i<regions.length(); ++i)
				boxes.push(GetBox(out, regions[i]));
		}
		Log(LOGINFO, "BBOX: %s", regions.join());
		for (int i=0; i<regions.length(); ++i)
			if (!boxes[i].IsNull())
			{
				Log(LOGINFO, "PROCESSING BBOX: %s", regions[i]);
				Savebox = &boxes[i];
				Command(argv+2);
			}
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-investigate", "-1 disabled (def), 0 only if not known, 1 for every link"))		
	{
		INVESTIGATE = atoi(argv[1]);
		return Command(argv+2);
	}

	if (cmd.chk(*argv,"-val", "floating point value"))		
	{
		FLOATVAL = atof(argv[1]);
		return Command(argv+2);
	}

	if (cmd.chk(*argv,"-mode", "-1 0 1"))		
	{
		MODE = atoi(argv[1]);
		return Command(argv+2);
	}

	if (cmd.chk(*argv,"-tmode", "-1:smartmerge 0:forcemerge -2:overwrite"))		
	{
		extern int TMODE;
		TMODE = atoi(argv[1]);
		return Command(argv+2);
	}

	if (cmd.chk(*argv,"-simplify", "to simplify"))		
	{
		SIMPLIFY = 1;
		return Command(argv+1);
	}

	if (cmd.chk(*argv,"-getcraiglist", "keyfile.csv [htmfile.htm] (MODE:1 ignore done list) (1 in keyword list too)") && argv[1])
	{
		GetCraigList(argv[1], argv[2]);
		return TRUE;
	}

	if (cmd.chk(*argv,"-gettop40", "[year] [D:\\Music\\00+Top\\] (MODE: -1:latest 0:download csv 1:download mp3)"))
	{
		GetTop40(argv[1], argv[1] ? argv[2] : "40");
		return TRUE;
	}

	if (cmd.chk(*argv,"-getsites", "[name] update waterflow sites"))
	{
		//CStringArrayList list;
		//WaterflowRect("rect=10,10,10,20", list);
		WaterflowSaveSites(argv[1]);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-getwatershed", "[name] update waterflow sites with watershed information"))
	{
		//CStringArrayList list;
		//WaterflowRect("rect=10,10,10,20", list);
		WatershedSaveSites(argv[1]);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-testsites", "[name] update waterflow sites"))
	{
		//CStringArrayList list;
		//WaterflowRect("rect=10,10,10,20", list);
		WaterflowTestSites(argv[1]);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-getfsites", "update waterflow forecast maps (NOOA)"))
	{
		//CStringArrayList list;
		//WaterflowRect("rect=10,10,10,20", list);
		WaterflowSaveForecastSites();
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-getusgssites", "update usgs site, old and current"))
	{
		//CStringArrayList list;
		//WaterflowRect("rect=10,10,10,20", list);
		WaterflowSaveUSGSSites();
		return TRUE;
	}

	if (cmd.chk(*argv,"-getkml", "get KML for all the CSV data files, use -bbox to specify region"))
	{
		//CStringArrayList list;
		//WaterflowRect("rect=10,10,10,20", list);
		CString name = "RW";
		if (argv[1]!=NULL)
			name = argv[1];
		SaveKML(name);
		return TRUE;
	}

	if (cmd.chk(*argv,"-getcsvkml", "<file.csv> get KML of CSV data") && argv[1])
	{
		//CStringArrayList list;
		//WaterflowRect("rect=10,10,10,20", list);
		SaveCSVKML(argv[1]);
		return TRUE;
	}

	if (cmd.chk(*argv,"-getdem", "[demfile.csv] update DEM.kml and DEM.csv (MODE=0 move irrelevant files out MODE=1 to create thermal png/kml)"))
	{
		ElevationSaveDEM(argv[1]);
		return TRUE;
	}

	if (cmd.chk(*argv,"-getscan", "[quad] [scanID] generates quad CSV file, info on ScanID OR generates SCAN.kml (no parameters)"))
	{  
		// output SCAN.kml
		ElevationSaveScan(argv[1]);
		return TRUE;
	}

	if (cmd.chk(*argv,"-testcanyons", "run canyon mapping from canyons.txt or specific lat lng dist"))
	{
		ElevationProfile(argv+1);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-getlegend", "produce legend.png"))
	{
		ElevationLegend(legendfile);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-getpanoramio", "update panoramio with BBOX (-mode -1:all 0:tags 1:title, -investigate >=0 to force refresh)"))
	{
		DownloadPanoramioUrl(MODE);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-getflickr", "update flickr with BBOX (-mode -1:all 0:tags 1:title, -investigate >=0 to force refresh)"))
	{
		DownloadFlickrUrl(MODE);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-getpoi", "update POI database from web, static and dynamic kmls"))
	{
		DownloadPOI();
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-getrwloc", "<num days> update RWLOC database from web (default 7, 1e6 to reset database)"))
	{
		DownloadRWLOC(argv[1]);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-getbeta1", "[code] BETA -> CHG.CSV (CHANGES)"))
	{
		DownloadBeta(MODE=1, (argv[1] && strcmp(argv[1], "rw")==0) ? NULL : argv[1]);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-getbetakmlx", "<file.kml> <url> run kml extraction on url") && argv[1] && argv[2] )
	{
		inetfile file(argv[1]);
		KMLExtract(argv[2], &file, TRUE);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-getbetakml", "-mode -2:from web FULL -1:from web update [default], 0:from web verbose, 1:from current files)"))
	{
		DownloadBetaKML(MODE, argv[1]);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-fixbetakml","[color_section | file.csv] (-mode -1:transform 1:select-only) "))
	{
		FixBetaKML(MODE, argv[1]);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-uploadbetakml", ""))
	{
		UploadBetaKML(MODE, argv[1]);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-getbeta", "[code] BETA -> CHG.CSV (DOWNLOAD) -mode -2:from web FULL -1:from web update [default], 0:from web verbose, 1:from current files)"))
	{
		DownloadBeta(MODE, argv[1]);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-getbetalinks", "[code] BETA -> CHG.CSV (investigate: -1 auto filter bad links, 0 no filter, 1 recheck all) use -setbeta to upload"))
	{
		extern int ITEMLINKS;
		ITEMLINKS = TRUE;
		DownloadBeta(MODE=1, argv[1]);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-checkbeta", "[code] check for duplicate links and other incongruencies [MODE 1 for dead links]"))
	{
		CheckBeta(MODE, argv[1]);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-setbeta", "[file.csv] (beta\\chg.csv by default) -> ropewiki (-mode -2:force set -1:set, 0:set verbose, 1:simulate )"))
	{
		UploadBeta(MODE, argv[1]);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-setbetaregion", "file.csv create regions if needed"))
	{
		UploadRegions(MODE, argv[1]);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-setstars", "[code] BETA -> ropewiki (-mode -1:set, 0:set verbose, 1:simulate)"))
	{
		UploadStars(MODE, argv[1]);
		return TRUE;
	}

	if (cmd.chk(*argv,"-setconditions", "[code] CONDITIONS -> ropewiki (-mode -3:force all -2:force latest -1:update latest 0:update all 1:test/simulate )"))
	{
		UploadConditions(MODE, argv[1]);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-getfbconditions", "fbc.csv [TEST/fcb2.csv/fbname/file.log/http:] -> FB Conditions (-mode -1:groups+users 0:groups 1:users 2:posting users 3:new friends, -investigate: numpages / -days)") && argv[1])
	{
		DownloadFacebookConditions(MODE, argv[1], argv[2]);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-setfbconditions", "fbc.csv CONDITIONS -> ropewiki (-mode -2:force -1:update 0:no likes 1:filter new)") && argv[1])
	{
		UploadFacebookConditions(MODE, argv[1]);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-getfbpics", "[name/url filter] -> fbpics.csv (-mode -1:latest 1:last10 5:last50)") && argv[1])
	{
		DownloadFacebookPics(MODE, argv[1]);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-setpics", "pic.csv -> ropewiki (-mode -2:overwrite_pic 1:preview_new 2:preview_all)") && argv[1])
	{
		UploadPics(MODE, argv[1]);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-resetbetacol", "ITEM_X will reset column") && argv[1])
	{
		ResetBetaColumn(MODE, argv[1]);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-setbetaprop", "\"string=newstring\" <file.csv/file.log/Region>\"") && argv[1] && argv[2])
	{
		ReplaceBetaProp(MODE, argv[1], argv[2]);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-getquery", "\"[[Category:Page ratings]][[Has page rating user::QualityBot]]\" file.csv") && argv[1] && argv[2])
	{
		QueryBeta(MODE, argv[1], argv[2]);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-getbetakml", "\"url\" [out.kml] (Mode:1 to force unprotected extraction)") && argv[1])
	{
		ExtractBetaKML(MODE, argv[1], argv[2]);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-getbetalist", "gets list of extracted beta sites (Mode:1 test rwlang=?)"))
	{
		ListBeta();
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-uploadbetabqn", "ynfile.csv -> ropewiki (-mode -4:rebuildStarsYes -3:overwrite -2:merge -1:set, 0:set verbose, 1:simulate )") && argv[1])
	{
		UpdateBetaBQN(MODE, argv[1]);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-uploadbetaccs", "chgfile.csv -> ropewiki (-mode -4:rebuildStarsYes -3:overwrite -2:merge -1:set, 0:set verbose, 1:simulate )") && argv[1])
	{
		UpdateBetaCCS(MODE, argv[1]);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-undobeta", "<file.csv/file.log/Region/Canyon/Query> undo last posted updates ") && argv[1])
	{
		UndoBeta(MODE, argv[1]);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-purgebeta", "<file.csv/file.log/Region/Canyon/Query>, ALL if empty (-mode 0:disambiguation 1:regions -2:Autorefresh)"))
	{
		PurgeBeta(MODE, argv[1]);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-refreshbeta", "<file.csv/file.log/Region/Canyon/Query> ( force Beta Site link refresh )") && argv[1] )
	{
		RefreshBeta(MODE, argv[1]);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-fixbeta", "<file.csv/file.log/Region/Canyon/Query>, ALL if empty (including disambiguation) if empty (MODE 1 to fake update)"))
	{
		FixBeta(MODE, argv[1]);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-fixconditions", "[[Query]] or ALL "))
	{
		FixConditions(MODE, argv[1]);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-fixbetaregion", "Region (mode:0,1,2,3 (minlevel) -> georegionfix.csv, nomode:->chg.csv)"))
	{
		FixBetaRegion(MODE, argv[1]);
		//DisambiguateBeta();
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-disbeta", "[rename.csv] (-mode 1 to add new disambiguations only)"))
	{
		DisambiguateBeta(MODE, argv[1]);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-movebeta", "rename.csv (pairs of old name, new name) [MODE 1 to force move of subpages]") && argv[1])
	{
		MoveBeta(MODE, argv[1]);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-deletebeta", "file.csv (name or id) comment") && argv[1] && argv[2])
	{
		DeleteBeta(MODE, argv[1], argv[2]);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-simplynamebeta", "rwfile.csv movefile.csv (mode:0 NO REGIONS, mode:1 capitalize, 2 extra simple)") && argv[1] && argv[2])
	{
		SimplynameBeta(MODE, argv[1], argv[2]);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-testbeta", "column [code] (MODE 1 to force extra checks)") && argv[1])
	{
		TestBeta(MODE, argv[1], argv[2]);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-getriversuskml", "update <outfolder> with RIVERS + WATERSHED data with BBOX (-mode -1:both 0:rivers 1:watershed)")&& argv[1]!=NULL)
	{
		DownloadRiversUS(MODE, argv[1]);
		if (MODE<0) CalcRivers(argv[1], "POI\\RIVERSUS", 0, 0, 0);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-getriversusurl", "update <outfolder> with WATERSHED (-investigate 0 to check, 1 to process oldest to newest)") && argv[1]!=NULL)
	{
		DownloadRiversUSUrl(MODE, argv[1]);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-getriversus", "update RIVERSUS with <infolder> data (-mode 1 to join)") && argv[1]!=NULL)
	{
		CalcRivers(argv[1], "POI\\RIVERSUS", MODE>0, 0, 0);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-getriversca", "update RIVERSCA with <infolder> data (-mode 0 to NOT reorder)") && argv[1]!=NULL)
	{
		extern gmlread gmlcanada;
		extern gmlfix gmlcanadafix;
		DownloadRiversGML(argv[1], argv[1], gmlcanada, gmlcanadafix);
		CalcRivers(argv[1], "POI\\RIVERSCA", 1, 1, MODE!=0);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-getriversmx", "update RIVERSMX with <infolder> data (-mode 1 to reorder)") && argv[1]!=NULL)
	{
		extern gmlread gmlmexico;
		extern gmlfix gmlmexicofix;
		DownloadRiversGML(argv[1], argv[1], gmlmexico, gmlmexicofix);
		CalcRivers(argv[1], "POI\\RIVERSMX", 1, 1, MODE>0);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-calcrivers", "<infolder> to <outfolder>  (-simplify to simplify, -mode 1 to reorder, -mode 0 not join)") && argv[1]!=NULL && argv[2]!=NULL)
	{
		CalcRivers(argv[1], argv[2], MODE!=0, SIMPLIFY, MODE==1, argv[3]);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-transrivers", "<infolder> to <outfolder> (-mode 0 old format, -mode 1 reset summary)") && argv[1]!=NULL && argv[2]!=NULL)
	{
		TransRivers(argv[1], argv[2], MODE);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-checkriversid", "check <folder> RIVERS") && argv[1]!=NULL)
	{
		CheckRiversID(argv[1]);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-fixrivers", "check & fix with timestamp <folder> RIVERS bounding box match and duplicates") && argv[1]!=NULL)
	{
		FixRivers(argv[1]);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-getriverselev", "update with timestamp <folder> [def ALL] with elevation data (-investigate 0 to force recalc, 1 to check values/changes)"))
	{
		GetRiversElev(argv[1]);
		return TRUE;
	}

	/*
	if (cmd.chk(*argv,"-getriverswg", "update with timestamp <folder> with waterfall/gage data (-investigate 0 to force recalc, 1 to check values/changes)"))
	{
		GetRiversWG(argv[1]);
		return TRUE;
	}
	*/
	
	if (cmd.chk(*argv,"-fixjlink", "check & fix <folder> <*name*> CSV files and change naked urls to jlink") && argv[1]!=NULL && argv[2]!=NULL)
	{
		FixJLink(argv[1], argv[2]);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-fixpanoramio", "check & fix from <folderin> to <folderout> CSV files and change naked urls to jlink") && argv[1]!=NULL && argv[2]!=NULL)
	{
		FixPanoramio(argv[1], argv[2]);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-scan", "[file] for direct, [file.csv] for list, -bbox for region, blank for all files in SCAN (-MODE -3:check -2:check&fix -1:add missing 0:redoDCONN 1:redoWF 2:full redo -TMODE GoogleElev -1:auto 0:disabled 1:redo errora -investigate 0 for extra messages)"))
	{
 		ScanCanyons(argv[1], Savebox);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-scanbk", "scans in background, [file] for direct, [file.csv] for list, -bbox for region"))
	{
		if (argv[1]!=NULL || Savebox)
		{
			MODE=-20;
			ScanCanyons(argv[1], Savebox);
			return TRUE;
		}
		
		// infinite loop
		while (TRUE)
		{
			// wake up every minute and scan remaining
			ScanCanyons();
			Sleep(60*1000);
		}
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-score", "report use -bbox BBOX to specify area otherwise from default config"))
	{
		inetfile data("score.csv");
		CKMLOut out("score");
		ScoreCanyons(NULL, NULL, &out, &data);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-id", "id SEGMENT from <id> & BBOX")  && argv[1]!=NULL && Savebox)
	{
		IdCanyons(Savebox, argv[1]);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-cmpcsv", "compare <folderin> vs <folderout> CSV files (-investigate 1 to get details)") && argv[1]!=NULL && argv[2]!=NULL)
	{
		CompareCSV(argv[1], argv[2]);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-copycsv", "copy from <folderin> to <folderout> CSV files to match a BBOX (-mode 1 to move files)") && argv[1]!=NULL && argv[2]!=NULL && Savebox)
	{
		CopyCSV(argv[1], argv[2]);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-copydem", "copy from <demfile> to <folderout> DEM files to match a BBOX (-mode 1 to move files)") && argv[1]!=NULL && argv[2]!=NULL && Savebox)
	{
		CopyDEM(argv[1], argv[2]);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-checkcsv", "check all CVS (POI & RIVERS) for duplicates or bad data"))
	{
		CheckCSV();
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-checkkmz", "check expired files in KMZ (-mode 1 to delete)"))
	{
		CheckKMZ();
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-getdem", "download ASTER GDEM tiles (BBOX required)"))
	{
		DownloadDEM();
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-test", "<file>/folder"))
	{
		void Test(const char *);
		Test(argv[1]);
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-testpics", "<file.csv>"))
	{
		void TestPics(void);
		TestPics();
		return TRUE;
	}
	
	if (cmd.chk(*argv,"-debug", "enable debug mode (inet)"))
	{
		MDEBUG = TRUE;
		return FALSE;
	}
	
	if (cmd.chk(*argv,"-download", "download <file> <url>"))
	{
		if (!argv[1] || !argv[2])
		{
			Log(LOGERR, "-download <file> <url>");
			return TRUE;
		}
		CString url = argv[2], file = argv[1];
		DownloadFile f;
		if (f.Download(url, file))
			Log(LOGERR, "Could not download %s", url);
		return TRUE;
	}

	printf("Commands:\n");
	for (int i=0; i<cmd.list.length(); ++i)
		printf("  %s : %s\n", cmd.list[i].id, cmd.list[i].desc);
	
	exit(0);
}
