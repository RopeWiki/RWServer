## Introduction

This repository houses code originally developed by Luca Chiarabini run by an external server to provide additional functionality to the RopeWiki site.

## Changes

Before beginning the repository, these changes were made to the original code recovered from Luca's computer:
* Moved sensitive information in code (passwords, etc) into sensitive.h which is not included in this repository
* TODO: document other pre-repo changes

## Development environment setup

The RopeWiki executables can be built with Visual Studio 2010, but not the free Express edition.

## HTTP server

'rw.exe' listens for FastCGI commands on port 9001. In order to use it from a web browser, a HTTP server is needed. For local testing purposes, on Windows this can be set up through this:
* Download nginx from here: https://nginx.org/download/nginx-1.12.1.zip
* Unzip it somewhere.
* Run nginx.exe.
* Make sure you can connect through a web browser - simply type in "localhost" as the URL and make sure you get a response indicating that nginx is running.
* Kill the process.
* Copy ropewiki/nginx/conf/nginx.conf over the installed configuration file with the same name.
* Run rw.exe and make sure it says that it's listening.
* Run nginx.exe.
* From your web browser, use something like this as the URL: "http://localhost/rwr?gpx=on&filename=Behunin_Canyon&url=http://ropewiki.com/images/c/cb/Behunin_Canyon.kml"
  (compare to "http://lucac.no-ip.org/rwr?gpx=on&filename=Behunin_Canyon&url=http://ropewiki.com/images/c/cb/Behunin_Canyon.kml" for the GPX download on "http://ropewiki.com/Behunin_Canyon")
* You should see in the rw.exe output that it received the command, and the web browser should give you a "Behunin_Canyon.gpx" file to download.
* If something seems to not work with this, you may need to unblock some applications or ports in the Windows firewall.

## Notes

This repo does not include data files that will surely be necessary for RopeWiki Explorer.  Documentation will have to be added for how to acquire this data.

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
  * libcurl.dll
  * libexpat.dll
  * libmysql.dll
  * libpq.dll
  * openjp2.dll
  * spatialite.dll
  * xerces-c_2_8.dll
  * zlib1.dll
  * libeay32.dll
  * ssleay32.dll

* run rwr.exe and make sure it starts up ok

## Tests

[**TODO**: confirm these tests are still valid; originally reported by Fredrik]

Launching 'rw.exe' produces "Actively Listening" output, and the server is ready for incoming connections.

- KML->GPX conversion commands seem to be working - output seems to be identical to files received from the original service.
- ZIP/PDF download:
  - PDF: Page (http://localhost/rwr?filename=Behunin_Canyon.pdf&pdfx=Behunin_Canyon&summary=off&docwidth=1903&ext=.rw) - seems to work
  - PDF: Map (http://localhost/rwr?filename=Behunin_Canyon_MAP.pdf&pdfx=Map?pagename=Behunin_Canyon&summary=off&docwidth=1903&ext=.rw)
  - ZIP: Page+Maps (http://localhost/rwr?filename=Behunin_Canyon.zip&zipx=Behunin_Canyon&bslinks=off&trlinks=off&summary=off&docwidth=1903&ext=.rw) - seems to work
  - ZIP: P+M+Links (http://localhost/rwr?filename=Behunin_Canyon+.zip&zipx=Behunin_Canyon&bslinks=on&summary=off&docwidth=1903&ext=.rw) - probably not working - took long time, produced truncated zip

Other commands have not been attempted yet. Fredrik has not tried to run the 'httprobot' or 'GPSBabel' executables.

## RopeWiki references

To have RopeWiki call a new RWServer, the server location must be updated in the ropewiki.com MediaWiki content.  References to luca.ropewiki.com (or old server) must be changed in the following [Templates](http://ropewiki.com/index.php?title=Special:Templates)

* KMLDisplay
* KMLDisplay2
* KMLLink
* KMLExtract

## Contributors

This original codebase was created by Luca Chiarabini.  Fredrik Farnstrom and Michelle did a lot of pre-repo work to bring the code to a state that could be distributed publicly.  Some of this document attributed to Ben on GitHub was written by Fredrik