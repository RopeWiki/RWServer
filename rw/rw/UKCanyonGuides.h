#pragma once

#include "stdafx.h"
#include "TradeMaster.h"

#define UKCGKML "http://www.google.com/maps/d/u/0/kml?mid=1NiNQaWJLW6amt87I06KU9CM49DA&forcekml=1";

int UKCG_DownloadPage(const char *memory, CSym &sym);
int UKCG_DownloadBeta(const char *ubase, CSymList &symlist);
