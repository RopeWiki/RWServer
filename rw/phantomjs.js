

//SCRIPT START
var	debug =	false;
var smallscreen = false;
var	pscale = 1;	// 1.5
var zscale = 1;
var	bigwidth = 800, medwidth = 600, smallwidth = 320, dwidth = 0;
if (typeof docwidth	 !=	"undefined") 
   {
   dwidth =	parseInt(docwidth, 10);
   if (isNaN(dwidth))
   		smallscreen = true; // default size
   	else
   		{
   		if (dwidth<medwidth)
   			{
   			smallscreen = true;
   			if (dwidth>smallwidth)
   				smallwidth = dwidth;
   			
   			}
   		else
   			{
   			smallscreen = false;
   			if (dwidth<bigwidth)
   				bigwidth = dwidth;
   			}
   		}
   
   }

// Capture Console log
var	consolelog = "\n\n=== CONSOLE LOG ===\n";
if (typeof console	!= "undefined")	
	if (typeof console.log != 'undefined')
		console.olog = console.log;
	else
		console.olog = function() {};

console.log	= function(message)	{
	// ignore translate.google.com frame errors
	if (message.indexOf('window.top.gtcomm')>0)
	  return;
	//if (debug)
	//   message = Date()+":"+message;
	console.olog(message);
	consolelog += message+'\n';
};

console.error =	console.debug =	console.info =	console.log


// Render Multiple URLs	to file

var	fs = require('fs');
var	system = require("system");

// cookies
if (gtrans!="")	phantom.addCookie({'name': 'googtrans',	'value': '/en/'+gtrans,	'domain': '.ropewiki.com' });
if (metric)	phantom.addCookie({'name': 'metric', 'value': 'on',	'domain': '.ropewiki.com' });
if (french)	phantom.addCookie({'name': 'french', 'value': 'on',	'domain': '.ropewiki.com' });

var	notranslist	= notrans.split(' ').join(',').split(',');

/*
system.webSecurity = false;
system.webSecurityEnabled =	false;
system.localToRemoteUrlAccess =true;
system.localToRemoteUrlAccessEnabled =true;
webSecurity=false;
webSecurityEnabled=false;
localToRemoteUrlAccess =true;
localToRemoteUrlAccessEnabled =true;
*/
var	errors = "", summary = "";
var	OK = "success";
var	tproc=0, nproc=0, nrend=0, ndown=0,	nskip=0, nerr=0;
var	lines =	[],	urls = [];

function isRopeWiki(url)
{
	return url.indexOf('ropewiki.com')>0;
}


function isTranslated(url)
{
	return url.indexOf('translate.google.com')>0;
}

function ext(url)
{
	return url.substring(url.length-4).toLowerCase();
}

function isDownloadFile(url)
{
	var kmlidx = url.indexOf('?kmlidx=');
	if (kmlidx<0)
	  kmlidx = url.indexOf('&kmlidx=');
	if (kmlidx>0)
	   {
	   var kmlidxend = url.indexOf("&", kmlidx);
	   var url1 = url.substring(0, kmlidx);
	   var url2 = kmlidxend<0 ? "" : url.substring(kmlidxend);
	   console.log(url+" != "+url1+" + "+url2);
	   url = url1 + url2;
	   }
	
  var exts = [ ".pdf", ".kmz",	".kml",	".gpx",	".jpg",	".png",	".tif",	".gif",	".zip",	".rar" ];
  var match	= exts.indexOf(ext(url));
  if (match>0) 
	return true;
  if (match==0 && !isTranslated(url)) 
	return true;
  return false;
}
  
function addline(file, url,	top)
{
  url =	url.split("%3D").join("=");

	// gpx
	if (gpxbs && ext(file)==".kml")
		{
		if (url.indexOf('http://lucac.no-ip.org/rwr')>=0)
			{
			var	startstr = '&kmlx=', endstr	= "&ext=.kml";
			var	start =	url.indexOf(startstr), end = url.indexOf(endstr);		
			url	= url.substring(start+startstr.length, end);
			console.log("gpx url = "+url);
			}		
		file = file.substring(0, file.length-4)+".gpx";
		url	= 'http://lucac.no-ip.org/rwr?gpx=on&filename=X&kmlx='+url+'&ext=.gpx';
		//console.log("gpx "+url);
		}

	// avoid ../ in	url			
	var	urla = url.split('/');
	while (urla.indexOf('..')>0)
		urla.splice(urla.indexOf('..')-1,2);
	url	= urla.join('/');

	var	line  =	file + ' ' + url;
	//console.log('Added: '+line);
	if (!!top)
		lines.unshift(line);
	else
		lines.push(line);
	urls.push(url);
}

function taddline(file,	url)
{	
	// translation
	var	ins	= file.indexOf('.',	file.indexOf('/'));
	var	tfile =	file.substring(0, ins)+'T'+file.substring(ins);		
	
	if (isDownloadFile(file) &&	ext(file)!='.pdf') 
		{
		addline(file, url);
		return;
		}

	var	sl = "en";
	var	tl = gtrans!=""	? gtrans : "en";
	notranslist.push(tl);
	if (isTranslated(url))
		{
		sl = url.substring(url.indexOf('&sl=')).substring(4,6);
		// add original
		addline(file, url.substring(url.indexOf('http',	5)).split('%26').join('&'));
		// add translation
		//console.log("sl="+sl+" tl="+tl);
		if (tlinks && notranslist.indexOf(sl)<0	&& !isRopeWiki(url))
			addline(tfile, url.split('&tl=en&').join('&tl='+tl+'&'));
		}
	else
		{
		// add original
		addline(file, url);
		// add translated
		if (tlinks && notranslist.indexOf(sl)<0	&& !isRopeWiki(url))
			addline(tfile, 'http://translate.google.com/translate?hl='+tl+'&sl=en&tl='+tl+'&u='+url);
		}		
}


function skipa(str)
{  
  var i;
  for (i=0;	i<str.length &&	str[i]=='*'; ++i);
  return str.substring(i);
}
 
function fixfilename(filepath) 
{
			filepath =	 skipa(filepath);
	  var chars	= [	' ', '\n', '?',	'*', '|', '/', ':',	'<', '>', '"', ';',	',', '\\' ];
	  for (var i=0;	i<chars.length;	++i)
		filepath = filepath.split(chars[i]).join('');
	  return filepath;			
}
		

function getfilename(folder, file, url)	{
		folder = fixfilename(folder);
		file = fixfilename(file);
		
		if (!isRopeWiki(url))
			{
			var	http = url;
			var	httpa =	http.split('=http');
			if (httpa.length>0)
				http = httpa[httpa.length-1];
			var	urla = http.split('/');
			if (urla.length>=2)
					{
					var	urla2 =	urla[2].split('.');
					if (urla2.length>=2)
						{
						var	domain = urla2[urla2.length-2];
						//console.log("Domain "+domain+" from "+url);
						var	filepatha =	file.split('.');
						filepatha.splice(filepatha.length-1, 0,	domain);
						file = filepatha.join('.');
						}
					}
			}
		if (folder.length>0)
		file = folder+'/'+file;
		//console.log("file: "+file);
	return file;
}
	
function DownloadFile(url, file, callback)
{
  
  // avoid CORS	problems
  var lucac	= 'http://lucac.no-ip.org/';
  if (url.substring(0,lucac.length)!=lucac)
	 url = 'http://lucac.no-ip.org/rwr?url='+url;

  //console.log('XMLHTTP '+url);
  var xhr =	new	XMLHttpRequest();
  /*
	xhr.webSecutiry	=false;
	xhr.webSecurityEnabled=false;
	xhr.localToRemoteUrlAccess =true;
	xhr.localToRemoteUrlAccessEnabled =true;
	xhr.addEventListener('load', function()	{
		console.log('DownloadFile OK status: ' + xhr.status	+ '	(' + xhr.statusText	+ ') size:'	+ xhr.response.length);
		//callback.call(xhr, null);
	});
	xhr.addEventListener('error', function(err)	{
		console.log('DownloadFile ERR status: '	+ xhr.status + ' ('	+ xhr.statusText + ') size:' + xhr.response.length);
		//callback.call(xhr, err ||	new	Error('HTTP	error!'));
	});
  */
	
  xhr.open("GET", url, true);
  //xhr.responseType = "arraybuffer";
  //console.log('0 readystate: '+xhr.readyState	+ '	status:	' +	' (' + xhr.statusText +	')');
  //xhr.overrideMimeType("text/plain; charset=x-user-defined");
  xhr.overrideMimeType("application/octet-stream");
  xhr.responseType = "arraybuffer";	 
  xhr.onreadystatechange = function	() {
  //console.log('readystate: '+xhr.readyState +	' status: '	+ '	(' + xhr.statusText	+ ')');
	if (xhr.readyState == 4) {
	  if (xhr.status !=	400	&& xhr.response) 
		{
		  var res =	new	Uint8Array(xhr.response);
		  if (res.length>0)
			{
			var	resb = [].map.call(res,	function(v){ return	String.fromCharCode(v);	}).join("");
			fs.write(file, resb, 'b');
			//console.log('DownloadFile	OK status: ' + xhr.status +	' (' + xhr.statusText +	') size:' +	xhr.response.length);
			callback(OK);
			return;
			}
		  }
	callback('ERROR	' +	xhr.status + ' (' +	xhr.statusText + ')');
	}								 
  };
  xhr.send(null);
  //console.log('ERROR status: ' + xhr.status +	' (' + xhr.statusText +	')');
  //console.log('bodysize:'	+ xhr.response.length +	'\n');

  return true;
}

function waitFor(testFx, onReady, timeOutMillis) {
			var	maxtimeOutMillis = timeOutMillis ? timeOutMillis : 30*1000,	//<	Default	Max	Timout is 10s
		start =	new	Date().getTime(),
		interval = setInterval(function() {
			var	condition =	(typeof(testFx)	===	"string" ? eval(testFx)	: testFx()); //< defensive code
			if ( (new Date().getTime() - start > maxtimeOutMillis) || condition	) 
					{
				// Condition fulfilled (timeout	and/or condition is	'true')
				//console.log("'waitFor()' finished	in " + (new	Date().getTime() - start) +	"ms.");
				typeof(onReady)	===	"string" ? eval(onReady) : onReady(); //< Do what it's supposed	to do once the condition is	fulfilled
				clearInterval(interval); //< Stop this interval
					}
		}, 1000); //< repeat check every 1s
};

/*
Render/Download	given urls
@param array of	URLs to	render
next Function called after finishing each URL, including the last URL
finish Function	called after finishing everything
*/
var	ProcessUrlsToFile =	function() {
	var	facebooklogin =	false;
	var	finish,	next, process, create, render;
	var	webpage	= require("webpage");

	// func	declaration
	
	var	create = function(url)	{
		var	page = webpage.create();
		page.settings.resourceTimeout =	30*1000;
		//page.settings.userAgent =	'Chrome/13.0.782.41';
		//page.paperSize = { format: 'Letter', orientation:	'portrait',	margin:	'1cm' };
		//page.viewportSize	= {	width: 400,	height:	800};
		//page.paperSize = { width:	'12in',	height:	'15.5in', margin: '0.5cm' };
				//page.onConsoleMessage	= function(msg)	{ console.log('jsconsole:' + msg); };
						//var wh = { w:8, h:11 };
				//page.viewportSize	= {	width: page.vwidth, height: Math.round((page.vwidth*wh.h)/wh.w)};
				//page.paperSize = { width:	wh.w+'in', height: wh.h+'in', margin: '0.1in' };
		page.vwidth = smallscreen ? ( /*isRopeWiki(url)*/ true ? smallwidth : medwidth ) : bigwidth;
		if (url.indexOf("/Map?pagename=")>0 || url.indexOf("/Summary?query=")>0)
		  page.vwidth = bigwidth;		
		// first set
		var px = page.vwidth;
		page.viewportSize =	{ width: px, height: px};
		//page.paperSize = { width: px, height: px, margin: 5};
		return page;
	}
	
	
	var	next = function(status,	url, file, mode) {
		if (debug) console.log("next()");
		if (status !== OK || !fs.exists(file)) {
			++nerr;	
			var	msg	= "ERROR: "+status+" - Unable to " + (!mode	? "Download" : "Render") + " file "	+ file + " [" +	url	+ "]\n"; 
			errors += msg;
			console.log(msg); 
		} else {
			if (mode<0)	++nskip;
			if (!mode) ++ndown;
			if (mode>0)	++nrend;
			if (mode>=0)
			  console.log((!mode ? "Downloaded " : "Rendered ")	+ file ); 
		}
		process();
	};

	var	finish = function()	{
		if (debug) console.log("finished()");
		console.log(summary);
		phantom.exit(0);
		};

  var numpages = -1;
	var setpagenum = function(pageNum, numPages) {
	  numpages = numPages;
    return "   ";
  	}
		
	var	render = function(status, page,	url, file) {	
		if (status===OK)
		  {

		  if (lnkbs>0 && !isRopeWiki(url) && !isTranslated(url))
			{			
			var	uslist = page.evaluate(function() {
					var	exts = [ ".pdf", ".kmz",  ".kml", ".gpx", ".jpg", ".png", ".tif", ".gif", ".zip", ".rar" ];
					var	last = [ "mapa.html", "topo.html" ];
					var	ulist =	[];
					// links
					var	alist =	document.querySelectorAll("a");
					if (alist && alist.length>0)
					  for (var i=0;	i<alist.length;	++i)
						{
						var	url	= alist[i].href;
						if (!!url)
							{
							var	ext	= url.substring(url.length-4).toLowerCase();
							if (exts.indexOf(ext)>=0)
							  ulist.push(url);
							var	urla = url.split('/'); 
							if (urla.length>0 && urla[urla.length-1])
							  if (last.indexOf(urla[urla.length-1])>=0)
								ulist.push(url);
							}
						}
					// area	links
					var	alist =	document.querySelectorAll("area");
					if (alist && alist.length>0)
					  for (var i=0;	i<alist.length;	++i)
						{
						var	url	= alist[i].href;
						if (!!url)
							ulist.push(url);
						}
					return ulist.join(',');
				});
			if (uslist)
			  {
			  var MAXLINKS = 30;
			  var ulist	= uslist.split(',');
			  var maxi = ulist.length, maximsg ="";
			  if (maxi>MAXLINKS) maxi =	MAXLINKS, maximsg =	" (out of "+ulist.length+")";
			  console.log("Downloading "+maxi+maximsg+"	extra files	referenced by "+file+" ["+url+"]");
			  for (var i=0;	i<maxi;	++i)
				if (urls.indexOf(ulist[i])<0)
				  {
				  var urlnamea = ulist[i].split('/');
				  var addname =	urlnamea[urlnamea.length-1];
				  if (!isDownloadFile(addname))
					addname	+= ".pdf";
				  
				  var filename = file.substring(0, file.length-4);
				  var filenamea	= filename.split('.'); filenamea[0]	+= 'X';
				  addline(filenamea.join('.')+'.'+fixfilename(addname),ulist[i],true);
				  ++tproc;
				  }
			  }
			}					  
			
		  // make sure kml/tiles are finished loading  
					waitFor(function() {
				// Check in	the	map	has	finished loading
				return page.evaluate(function()	{ return typeof(isloadingmap)!='function' || !isloadingmap(); });
			}, function() {





				   //page.evaluate(function	() { document.body.style.width = '800px	!important'; return	{  h: $(document).height(),	w: $(document).width() }; });
				   //page.includeJs("http://code.jquery.com/jquery-1.10.1.min.js", function() {	}
				 if	(debug)
					 	{
		   			var whlog = page.evaluate(function () {
						//$(document).width(800);
						//document.body.style.width	= '800px';
					  var elem = document;
					  var doc =	elem.documentElement;
					  var h	= "	H:"+elem.body["scrollHeight"] +","+	doc["scrollHeight"]	+","+ elem.body["offsetHeight"]	+","+ doc["offsetHeight"] +","+	doc["clientHeight"] + "," + document.scrollHeight;
					  var w	= "	W:"+elem.body["scrollWidth"]+","+ doc["scrollWidth"]+","+ elem.body["offsetWidth"]+","+	doc["offsetWidth"] +","+ doc["clientWidth"] ;
					  return w+h;
				 		 });
						 console.log("whlog:"+whlog);
						 }
					var	wh = page.evaluate(function	() {
						var	elem = document;
						var	doc	= elem.documentElement;
						var	h =	Math.max(elem.body["scrollHeight"],	doc["scrollHeight"], elem.body["offsetHeight"],	doc["offsetHeight"], doc["clientHeight"]);
						var	w =	Math.max(elem.body["scrollWidth"], doc["scrollWidth"], elem.body["offsetWidth"], doc["offsetWidth"], doc["clientWidth"]);

						// override body
						var sheet = document.createElement('style')
						sheet.id = 'nocontent';
					  sheet.innerHTML = 'html { min-width:'+w+'px; }'; //header, footer, address { display:none; }
					  document.body.insertBefore(sheet, document.body.firstChild);
	
						var	h2 =	Math.max(elem.body["scrollHeight"],	doc["scrollHeight"], elem.body["offsetHeight"],	doc["offsetHeight"], doc["clientHeight"]);
						var	w2 =	Math.max(elem.body["scrollWidth"], doc["scrollWidth"], elem.body["offsetWidth"], doc["offsetWidth"], doc["clientWidth"]);

						return { h:	h2, w: w2, h1: h, w1: w};
						});
					
				
				if (debug) console.log("wh:"+wh.w+"x"+wh.h+" <= "+wh.w1+"x"+wh.h1);
				
				wh.h = wh.h * 1.05; // 5% extra
				if (!singlepage)
						wh.h = (wh.w*297)/210;

				// 2nd set
				var scale = page.vwidth == bigwidth ? pscale : 1;
				page.paperSize = { 
					width: wh.w*scale, 
					height: wh.h*scale, 
					margin: 5*scale, 
					header: { height: "5px", contents: phantom.callback(setpagenum) }
				};
				page.viewportSize =	{ width: wh.w*scale, height: wh.h*scale};
				if (debug) console.log(wh.w+","+wh.h+" "+page.paperSize.width+","+page.paperSize.height+" "+page.viewportSize.width+","+page.viewportSize.height+" z="+page.zoomFactor);
				//page.zoomFactor = zscale;
				numpages = -1;
			  var ret = page.render(file);
				if (debug) console.log("rendered file "+file+" ret="+ret+" numpages="+numpages);
				if (singlepage && numpages>1)
				   	{
				   	// multiple page error
				   	fs.remove(file);
						page.paperSize = { 
							width: wh.w*scale, 
							height: 2*wh.h*scale, 
							margin: 5*scale, 
							header: { height: "5px", contents: phantom.callback(setpagenum) }
						};
				   page.viewportSize =	{ width: wh.w*scale, height: 2*wh.h*scale};
				   ret = page.render(file);
					 if (debug) console.log("rendered#2 file "+file+" ret="+ret+" numpages="+numpages);
				   }
				if	(debug)
					 {
		   			var whlog = page.evaluate(function () {
						//$(document).width(800);
						//document.body.style.width	= '800px';
					  var elem = document;
					  var doc =	elem.documentElement;
					  var h	= "	H:"+elem.body["scrollHeight"] +","+	doc["scrollHeight"]	+","+ elem.body["offsetHeight"]	+","+ doc["offsetHeight"] +","+	doc["clientHeight"] + "," + document.scrollHeight;
					  var w	= "	W:"+elem.body["scrollWidth"]+","+ doc["scrollWidth"]+","+ elem.body["offsetWidth"]+","+	doc["offsetWidth"] +","+ doc["clientWidth"] ;
					  return w+h;
				 		 });
						 console.log("whlog:"+whlog);
						 }				page.close();																						
				next(status, url, file,	1);
			});
			return;
		  }
		page.close();																						
		next(status, url, file,	1);
		}


	var	process	= function() {
		if (debug) console.log('process()');
		var	url, file, filename, line ="";
		summary	= "=== SUMMARY:	Processed:"+nproc+"	of "+tproc+", Rendered:"+nrend+", Downloaded:"+ndown+",	Previously Processed:"+nskip+",	Errors:" + nerr	+ "	===\nAll produced files	are	available at: "+fs.workingDirectory+"\n";
		fs.write(system.args[0]+".log",	summary+errors+consolelog, 'w');	

		while (lines.length>=0 && line.length==0)
		  {
		  if (lines.length==0)
			 {
			 finish();
			 return;
			 }
		  line = lines.shift().trim();
		  }
							
		var	n =	line.indexOf(' ');
		if (n<0) {
			console.log("INTERNAL ERROR: No	'file url' line	in "+line);
			next("INTERNAL ERROR", "", "", 0);
			return;
		  }
		  
		// process url
		if (debug) console.log('process	url	'+line);
		
		url	= line.substring(n+1);
		file = line.substring(0,n);
					   
		++nproc; 
				
		if (fs.exists(file))
		  {
		  ++nskip;
		  if (debug) console.log('fs.exist '+file);
		  // skip files	that were already processed
		  window.setTimeout(function() {
			//next(OK, url,	file, -1);
			process();
		  }, 10);
		  return;
		  }

		console.log("--- "+Math.round((nproc*100)/tproc)+"%	("+nproc+"/"+tproc+") Processing "+file+" ---");  
		
		var	folder = file.split('/');
		if (folder.length>1)
					if (!fs.exists(folder[0]))
						fs.makeDirectory(folder[0]);
		
		if (isDownloadFile(url))
		  {
		  // download file
		  DownloadFile(url,	file, function(status) {
			next(status, url, file,	0);
			});
		  return;
		  }
					
		/*			  
		  if (!facebooklogin &&	url.indexOf("facebook.com")>0)
			  {
				  facebooklogin	= true;
				  // dummy facebook	activation to properly display fb links		  
				  if (debug) console.log("facebook activation()	");
				  var fbpage = create();
				  fbpage.open('https://www.facebook.com/', function(status){
					//var title	= "Welcome to Facebook";
					//if(fbpage.title.substring(0,title.length)	== title)
					  {
					  if (debug) console.log("facebook activating");
					  fbpage.evaluate(function(){
						  // fill login	info
						  var frm =	document.getElementById("login_form");
						  frm.elements["email"].value =	'rwphantomjs@gmail.com';
						  frm.elements["pass"].value = 'ropewiki';
						  frm.submit();
						  });
					  if (debug) console.log("facebook activated");
					  }
					
			window.setTimeout(function() { open(url, file);	}, 1000);
					});
				  return;
				  }			  
				*/

		/*
		var	injtrans = false;
		if (isTranslated(url))
				{
				var	fr = "en", to =	"en";
				var	pos	= url.indexOf("&sl=");
				if (pos>=0)
					fr = url.substring(pos+4, url.indexOf("&", pos+4));
				var	pos	= url.indexOf("&tl=");
				if (pos>=0)
					to = url.substring(pos+4, url.indexOf("&", pos+4));				
				var	trans =	'/'+fr+'/'+to;

				var	pos	= url.indexOf("&u=");				
				url	= url.substring(pos+3);				
				var	domain = "", domaina = url.split('/');
				if (domaina.length>1)
					domain = domaina[2];

				console.log("GTRANS="+trans+" URL="+domain);
						phantom.addCookie({'name': 'googtrans',	'value': trans,	'domain': '.'+domain });				
				injtrans = true;
				}
		*/

			 open(url, file);
	};	

	var	open = function(url, file) {

				
		// open	page
		if (debug) console.log("page.open()");
		var	page = create(url)
	  if (debug)
	    page.onLoadFinished = function() {
		    var height = page.evaluate(function() { return document.body.offsetHeight }),
	        width = page.evaluate(function() { return document.body.offsetWidth });
    		console.log("loaded hxw = "+height+","+width);
				}

		page.open(url, function(status)	{
		  if (debug) console.log("page.opened()	=" + status);
		  window.setTimeout((function()	{

		  if (debug) console.log("page.timer() " + status);
		  /*
		  if (injtrans)
			{
			console.log("injecting");
						page.evaluate(function() {	
						  var div =	document.createElement('DIV')
						  div.id = 'google_translate_element';
						  document.body.appendChild(div);
							var	script = document.createElement("script");
							script.type	= "text/javascript";
							script.innerHTML = "function googleTranslateElementInit() {	new	google.translate.TranslateElement({pageLanguage: 'fr', includedLanguages: 'en',	layout:	google.translate.TranslateElement.FloatPosition.TOP_LEFT, autoDisplay: false, multilanguagePage: true},	'google_translate_element'); }";
							document.body.appendChild(script);
						  var script = document.createElement("script");
							script.type	= "text/javascript";
							script.src = "//translate.google.com/translate_a/element.js?cb=googleTranslateElementInit";	
							document.body.appendChild(script);
						});
			}
			*/
			/*
			if (page.vwidth==smallwidth && url.indexOf("ropewiki.com")>0)
						{			
						//console.log("injecting css ropewiki w"+page.vwidth);
						page.evaluate(function() {	
						  var sheet	= document.createElement('style')
						  sheet.id = 'nocontent';
						  var list = [];
							//list.push('html { width:360px; background-color:red; }');
							//list.push('#fb-root { display:none !important; }');						
						  sheet.innerHTML =	list.join('\n');
						  document.body.appendChild(sheet);
						});
						}
			*/
			if (url.indexOf("wikiloc.com")>0)
					{			
						//console.log("injecting css wikiloc");
						page.evaluate(function() {	
						  var sheet	= document.createElement('style')
						  sheet.id = 'nocontent';
						  sheet.innerHTML =	'a[href]:after{content:"" !important}';
						  document.body.appendChild(sheet);
						});
					}

			if (isTranslated(url))
					{
				  // --	google translate patch
				  var iframe_src = page.evaluate(function()	{
					  var frame	= null;
					  if (document)
						 frame = document.querySelector('#contentframe');
					  if (frame)
						 frame = frame.querySelector('iframe');
					  if (frame	&& frame.src) 
						 return	frame.src;
					  return null;
				  });
				  //console.log('Found iframe: ' + iframe_src);
				  if (iframe_src) 
					{
					page.close();					
					// render the iframe
					console.log('Translating: '	+ file); //	+ "	["+iframe_src+"]");
							page = create();
					page.open(iframe_src, function(status) 
					  {
						// wait	a bit for javascript to	translate
						window.setTimeout(function() 
						  {
						  // print the page	into PDF  
						  render(status, page, url,	file);				 
						  }, 10000); //	give 10s for translation
					  });
					return;
					}
				  // --	google translate patch				  
				  }
		  if (debug) console.log("page.render()	" +	url);
		  render(status, page, url,	file);
		  }), 5000);  // give 5s per page
		});
	};	
  
  return process();
}


if (system.args.length > 1)	{
	var	line, linedata;
	var	file_h = fs.open(system.args[1], 'r');
	while(!file_h.atEnd()) {
	  line = file_h.readLine();	   
	  if (line.length>0) 
		  linedata+=line+',';
	}
	file_h.close();
 } else	{
	// assuming	line data defined
	//console.log("Usage: phantomjs	render_multi_url.js	[domain.name1, domain.name2, ...]");
}

var	folder = "";
var	linesdata =	linedata.split("\n");

// remap translation links
var	minj = 0;
var	pattern	= "&tl=en&u=";
for	(var i=0; i<linesdata.length; ++i)
  {
  var u	= linesdata[i].indexOf(pattern);
  if (u<0) 
	{
	if (linesdata[i].substr(0,2)!='**')	
		minj = i;
	continue;
	}
  var url =	linesdata[i].substr(u+pattern.length);
  for (var j=i-1; j>minj; --j)
	 if	(linesdata[j].substr(linesdata[j].length-url.length)==url)
		{
		linesdata[j] = linesdata[j].split("	")[0] +" "+linesdata[i].split("	")[1];
		linesdata.splice(i--, 1);
		break;
		}
	}
//fs.write("list.txt", linesdata.join("\n"), 'w');


// process links
for	(var i=0; i<linesdata.length; ++i)
  {
  linesdata[i]=linesdata[i].substring(1).trim();
  if (linesdata[i].length == 0)
	  continue;
	  
  var linea	= linesdata[i].split(' ');
  if (linea.length<2)
	{
		console.log("Skipping line '"+linea.join(' ')+"'");
		continue;
		}
	
	if (linea[0][0]!='*' &&	linea[0][0]!='@')
		{
		// folder		
		folder = skipa(linea[0]);
		continue;
		}		 
		
  var url =	linea[1];
  var filename = getfilename(folder, linea[0], linea[1]);
	
	if (url.indexOf('descente-canyon.com')>0 &&	ext(filename)=='.pdf')
			{
			var	pos	= url.indexOf('descente-canyon.com');
			var	urla = url.substring(pos).split('/');
			if (urla.length>1)
				{
				var	id = urla[urla.length-1];
				url	= url.substring(0, pos)+'descente-canyon.com/canyoning.php/219-'+id+'-canyons.html';
				taddline(filename, url);
				url	= url.substring(0, pos)+'descente-canyon.com/canyoning/canyon-debit/'+id+'/observations.html';
				taddline(filename.substring(0, filename.length-4)+".observations.pdf", url);
				}
			}

	if (url.indexOf('ropewiki.com/User:')>0)
		continue;
					
	taddline(filename, url);
  }

console.log("Downloading "+(tproc=lines.length)+" web pages	to folder: "+fs.workingDirectory +" (dwidth:"+dwidth+")");
ProcessUrlsToFile();

//SCRIPT END
