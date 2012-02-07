# -*- coding: utf-8 -*-
# GMapData module

# Andrew Berry, CIS 2750 A3 Prototype
#  February 14, 2007
# W Gardner, repackaged as module with OO
# "EZ" edition, doesn't use RPC, but outputs HTML/Javascript directly
#  February 20, 2007
# W Gardner, allow browser to be closed w/o "no running window found" and
# "no such process" errors.
#  February 20, 2010
# N Presta (W10 course), fix one-off error in point colours
#  August 6, 2010
# N Girard (W11 course), added infoWindow and title bar
#  February 10, 2011
# N Girard, fix Google Maps pop-ups attaching to only last point
#  Mar. 18, 2011

import os, signal, time
import CGIHTTPServer, BaseHTTPServer

### Global variables

serverPids = [0,0]     # list of server process IDs [web server,browser]


### Public functions

# Start CGI/HTML web server (do once).  Pages to be served must be in
# public_html.
#
# portNum: port number that HTTP server should listen on. Recommend 4xxxx,
#   where "xxxx" are the last 4 digits of your student number. 
#
def startWebServer( portNum=8080 ):
	pid = os.fork()
	serverPids[0] = pid
	if (pid == 0):			# in child process
		# The website is served from public_html, and CGI programs
		# of any type (perl, python, etc) can be placed in cgi-bin
		class Handler(CGIHTTPServer.CGIHTTPRequestHandler):
			cgi_directories = ["/cgi-bin"]

		os.chdir('public_html');
		httpd = BaseHTTPServer.HTTPServer(("", portNum), Handler)
		print "Web server on port:", portNum
		httpd.serve_forever()
		

# Kill all servers
#
def killServers():
	print "Killing all servers...",serverPids
	for pid in serverPids:
		try:
			print " Sending SIGHUP to ",pid
			os.kill(pid, signal.SIGHUP)
		except OSError:
			print "OS Error: PID " + str(pid) + " has already died."
		
		
# Launch Firefox browser (add tab to browser)
#
# url: URL to be opened in Firefox browser, "http://localhost:8080/" (replace
#   8080 with the port number of the web server).
# options: as list (not string); see "man firefox"
#
def launchBrowser( url, options=[] ):
	FIREFOX = "/usr/bin/firefox"
	if (serverPids[1] == 0 ):		# open first browser instance
		pid = os.fork()
		serverPids[1] = pid
		if (pid == 0):			# in child process
			os.execv(FIREFOX, [FIREFOX]+options+[url]);
	else:							# open new tab in existing browser
		pid = os.fork()
		if (pid == 0):			# in child process
			os.execl(FIREFOX, FIREFOX, url);
		else:
			os.wait()


### GMapData class
#
class GMapData:
	# Constructor
	#   title: title to display on title bar of browser
	#   header: string to display above map with <H1> tag
	#   center: list of [lat, long] for center of map
	#   zoom: integer with initial zoom setting for map
	def __init__( self, t="Google Map", h="50 Stone Road E, Guelph, ON", c=[43.530318,-80.223241], z=15 ):
		self.title = t
		self.header = h
		self.center = c
		self.zoom = z
		self.lineColors = ["#ff0000","#00ff00","#0000ff"]
		self.points = []
		self.overlays = []
		self.windows = []

	# setColors
	#   palette: list of color values for lines, in Google Maps RGB format,
	#       e.g., "#33a570" (string with 3 pairs of hex digits)
	def setColors( self, palette ):
		self.lineColors = palette
		
	# addPoint
	#   point: list of [lat, long] to be added to points array for waypoint,
	#       trackpoint, or connecting line
	def addPoint( self, point, photo, address ):
		self.points.append( (point[0], point[1]) )	# make into tuple
		content='<div id="content"><table><tr><td><img width="70" src="%s" /></td><td width="200">%s</td></tr></table></div>' % (photo,address) # HTML content of pop up window
		self.windows.append( (content) )
		
	# addOverlay
	#   start: index in array of points where this overlay starts
	#   np: number of points in line, or 1 for single point
	#   style: for a single point, icon color (0=default Google Maps icon,
	#       1=red, 2=green, 3=blue); for a line, index in color palette
	def addOverlay( self, start, np, style):
		self.overlays.append( (start, np, style) );
		
	# serve
	#	path: where to generate HTML
	def serve( self, path ):
		htm = open( path, "w" )		# open for writing (truncate existing)

  		# use specified map center coord and zoom level
		print >> htm, """<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
 <head>
  <meta http-equiv="content-type" content="text/html; charset=utf-8"/>
  <title>%s</title>
  <script src="http://maps.google.com/maps?file=api&amp;v=2&amp;key=ABQIAAAASveZAKDchy8aE-6K6klrVRTwM0brOpm-All5BF6PoaKBxRWWERS4kZrUi1p4QOZXGVxTLHvCXsg94A"
          type="text/javascript"></script>
  <script type="text/javascript">
  //<![CDATA[
  function load() {
    if (GBrowserIsCompatible()) {
      var baseIcon = new GIcon();
      baseIcon.shadow = "http://labs.google.com/ridefinder/images/mm_20_shadow.png"
      baseIcon.iconSize = new GSize(12, 20);
      baseIcon.shadowSize = new GSize(22, 20);
      baseIcon.iconAnchor = new GPoint(6, 20);
      baseIcon.infoWindowAnchor = new GPoint(5, 1);
      var icons = [new GIcon(baseIcon),new GIcon(baseIcon),new GIcon(baseIcon)];
      icons[0].image = "http://labs.google.com/ridefinder/images/mm_20_red.png";
      icons[1].image = "http://labs.google.com/ridefinder/images/mm_20_green.png";
      icons[2].image = "http://labs.google.com/ridefinder/images/mm_20_blue.png";
      var map = new GMap2(document.getElementById("map"));
      map.addControl(new GSmallMapControl());
      map.addControl(new GMapTypeControl());
      map.setCenter(new GLatLng(%f, %f), %d);""" % (self.title, self.center[0], self.center[1], self.zoom)
		count = 0
		
		# add overlays to map
		for draw in self.overlays:
			print self.windows[draw[0]]
			if ( draw[1]==1 ):		# single point
				print >> htm, '      var p = new GLatLng(%f, %f);' % self.points[draw[0]]
				if ( draw[2]==0 ):	# style = default Google Maps marker
					print >> htm, '      var marker%d = new GMarker(p));' % (count)
					print >> htm, '      map.addOverlay(marker%d);' % (count)
				else:				# style = colored icon
					print >> htm, '      var marker%d = new GMarker(p,icons[%d]);' % (count, int(draw[2]) - 1)
					print >> htm, '      map.addOverlay(marker%d);' % (count)
				print >> htm, "      GEvent.addListener(marker%d, 'click', function(){marker%d.openInfoWindowHtml('%s')});" % (count, count, self.windows[draw[0]])
			else:					# polyline
				print >> htm, '      var points = [];'
				for pt in self.points[draw[0]:draw[0]+draw[1]]:
					print >> htm, '      points.push(new GLatLng(%f, %f));' % pt
				print >> htm, '      map.addOverlay(new GPolyline(points,"%s"));' % self.lineColors[draw[2]]
			count = count + 1
			
		# display header above map
		print >> htm, """    }
  }
  //]]>
  </script>
 </head>
 <body onload="load()" onunload="GUnload()">
  <h1>%s</h1>
  <div id="map" style="width: 500px; height: 300px"></div>
 </body>
</html>""" % self.header

		htm.close()
