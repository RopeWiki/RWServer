## Introduction

This repository houses code originally developed by Luca Chiarabini run by an external server to provide additional functionality to the RopeWiki site.

## Changes

Before beginning the repository, these changes were made to the original code recovered from Luca's computer:
* Moved passwords and other sensitive information in the code to passwords.h, which is not included in this repository
* Identified the minimum number of files for the build to succeed, out of the thousands of files in the original folder

## Development environment setup

The RopeWiki codebase is upgraded to use Visual Studio 2017. (The original work by Luca was in VS2008).

## HTTP server

'rw.exe' listens for FastCGI commands on port 9001. Nginx is a commonly paired HTTP server that will forward incoming requests using this protocol, and is what Luca chose for the RW server to use. Instructions to configure nginx are in the 'Installation steps' below.

## Notes

This repo does not include data files that are necessary for RopeWiki Explorer.  Documentation will have to be added for how to acquire this data.

It's likely that there will be more bugs due to the differences between compilers. Specifically, the change from 'char' types defaulting to 'signed' now instead of 'unsigned' is likely to be an issue.

## Installation steps

* download nginx:  http://nginx.org/en/download.html
* unzip and place in c:\ folder
* edit conf\nginx.conf file
* add following lines under 'location /' tag:
```
            fastcgi_pass 127.0.0.1:9001;
            include fastcgi_params; # need this line, or query string will be empty
```

* copy nginx.bat file to the AWS server from the RopeWiki development folder
* edit nginx.bat file and modify the path of the nginx.exe to where it was placed above
* run the nginx.bat file and make sure the nginx.exe process starts. You may have to enable permissions

* install [Microsoft Visual C++ 2010 Redistributable Package (x86) package](https://www.microsoft.com/en-us/download/details.aspx?id=5555)
* edit environment variable PATH (for some reason the files above are installed in folder not in the PATH):
  * add new: C:\Windows\SysWOW64 (note: this step may not be necessary)
* open windows firewall: create 'new rule...', open Port 80, give some name (e.g. 'Web Access')
* create new folder c:\rw
* copy following files inside:
  * rwr.exe  (rwr.exe is release version; rw.exe is debug version)
  * GPSBabel.exe
  * gdal111.dll
  * geos.dll
  * geos_c.dll
  * iconv.dll
  * libcurl.dll
  * libeay32.dll
  * libexpat.dll
  * libmysql.dll
  * libpq.dll
  * openjp2.dll
  * phantomjs.exe
  * phantomjs.js
  * proj.dll
  * spatialite.dll
  * ssleay32.dll
  * xerces-c_2_8.dll
  * zlib1.dll

* run rwr.exe and make sure it starts up ok. You should see the line 'Actively Listening'
* test from a KML/GPX download link from RopeWiki.com.
* if that does not succeed, you can test from a local web browser on the server, with something like this URL: "http://localhost/rwr?gpx=on&filename=Behunin_Canyon&url=http://ropewiki.com/images/c/cb/Behunin_Canyon.kml"
* You should see in the rw.exe output that it received the command, and the web browser should give you a "Behunin_Canyon.gpx" file to download.

## Development TODO:
Working:
- KML->GPX conversion commands are working. 
- ZIP/PDF download:
  - PDF: Page (http://localhost/rwr?filename=Behunin_Canyon.pdf&pdfx=Behunin_Canyon&summary=off&docwidth=1903&ext=.rw)
  - PDF: Map (http://localhost/rwr?filename=Behunin_Canyon_MAP.pdf&pdfx=Map?pagename=Behunin_Canyon&summary=off&docwidth=1903&ext=.rw)
  - ZIP: Page+Maps (http://localhost/rwr?filename=Behunin_Canyon.zip&zipx=Behunin_Canyon&bslinks=off&trlinks=off&summary=off&docwidth=1903&ext=.rw)

Not working / TODO:
  - Wikiloc waypoint downloading not working (example: map here: http://ropewiki.com/Matacanes_(upper) )
  - ZIP: P+M+Links (http://localhost/rwr?filename=Behunin_Canyon+.zip&zipx=Behunin_Canyon&bslinks=on&summary=off&docwidth=1903&ext=.rw) - took long time, produced truncated zip
  - we have not tried to run the 'httprobot' executable. 
  - Waterflow analysis broke when loading and is turned off in code.

## RopeWiki references

To have RopeWiki point to a new RWServer, the server URL must be updated in the ropewiki.com MediaWiki content at the following locations:
  - For Templates, a global variable is set as its own template, named 'RWServerUrl', which resolves to "http://luca.ropewiki.com".
  - For the .js code, a similar global variable is named at the top of the Common.js file.
  - Also, similar variable at the top of ropewiki.com/PDFScript (this page is called by ropewiki.com/PDFList)

## Contributors

This original codebase was created by Luca Chiarabini. Michelle Nilles brought the code back online and installed to an AWS server provided by Dav, and also brought the code to a state that could be distributed publicly and published here to Github.  Some of this document attributed to Ben on GitHub was written by Fredrik.
