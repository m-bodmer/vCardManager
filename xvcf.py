# -*- coding: utf-8 -*-
#!/usr/bin/python

"""
XVCF is a GUI application which uses the utilities in Vcutil through VCFTOOL and is fully capable of being
interactive with the user. Can also connect to an SQL database to store and retrieve information.

Marc Bodmer
0657005
mbodmer@uoguelph.ca
"""

#All Credit for MultiListBox goes to Source:  http://code.activestate.com/recipes/52266-multilistbox-tkinter-widget/
#Checkbar class taken from http://www.java2s.com/Code/Python/GUI-Tk/Checkboxbargetselectedcheckbox.htm

from Tkinter import *
import tkMessageBox
import subprocess
import os
from tkFileDialog import *
import Vcf
import shutil
from GMapData import *
import sys
import re
import psycopg2
import psycopg2.extras

root = Tk()
card = []
backupcard = []
mappointlist = []
textfr=Frame(root)
text=Text(textfr,height=10,width=140,background='white')
filestring = ""
username = ""
conn = ""
existingnameID = ""
cancelflag = 0
hostname = "db.socs.uoguelph.ca"
undoflag = 0
addcardflag = 0
mapstarted = 0
propertiesedited = 0
saveDisabled = 0
queryOpen = 0

def selectionCheck():
    
    fileviewselection = mlb.curselection()  
    if (len(fileviewselection) == 0):
	return
    else:
        mlb2.delete(0, mlb2.size())
	fileviewselection = int(fileviewselection[0])
	for element in range(0, len(card[fileviewselection])):
	    #1 is Name. 2 is Parval, 3 is ParType, 4 is Value
	    if (element % 4 == 0):
		mlb2.insert(END, (card[fileviewselection][element], card[fileviewselection][element + 1], card[fileviewselection][element + 2], card[fileviewselection][element + 3]))	
    
    #if (len(fileviewselection) == 0):
	#pass
	##top = Toplevel()
	##top.title("Error")
	##top.bind('<Escape>', lambda x: destroyWindow(top)) 
	##top.geometry('250x150+550+300');
	##msg = Message(top, text="Please Select a Card!", width = 250)
	##msg.pack()
	
	##button = Button(top, text="Cancel", command=top.destroy)
	##button.pack()
    #else:
	#fileviewselection = int(fileviewselection[0])
    
#def propertyCheck():
    #cardviewselection = mlb2.curselection()
    
    #if (len(cardviewselection) == 0):
	#pass
    #else:
	#cardviewselection = int(cardviewselection[0])

class MultiListbox2(Frame):
    def __init__(self, master, lists):
	Frame.__init__(self, master)
	self.lists = []
	for l,w in lists:
	    frame = Frame(self); frame.pack(side=LEFT, expand=YES, fill=BOTH)
	    Label(frame, text=l, borderwidth=1, relief=RAISED).pack(fill=X)
	    lb = Listbox(frame, width=w, borderwidth=0, selectborderwidth=0,
			 relief=FLAT, exportselection=FALSE, selectmode=EXTENDED, bg="white")
	    lb.pack(expand=YES, fill=BOTH)
	    self.lists.append(lb)
	    lb.bind('<B1-Motion>', lambda e, s=self: s._select(e.y))
	    lb.bind('<Button-1>', lambda e, s=self: s._select(e.y))
	    lb.bind('<Leave>', lambda e: 'break')
	    lb.bind('<B2-Motion>', lambda e, s=self: s._b2motion(e.x, e.y))
	    lb.bind('<Button-2>', lambda e, s=self: s._button2(e.x, e.y))
	    lb.bind('<Button-4>', lambda e, s=self: s._scroll(SCROLL, -1, UNITS))
	    lb.bind('<Button-5>', lambda e, s=self: s._scroll(SCROLL, 1, UNITS))

	    
	frame = Frame(self); frame.pack(side=LEFT, fill=Y)
	Label(frame, borderwidth=1, relief=RAISED).pack(fill=X)
	sb = Scrollbar(frame, orient=VERTICAL, command=self._scroll)
	sb.pack(expand=YES, fill=Y)
	sbx = Scrollbar(frame, orient=HORIZONTAL,command=self.horizontalscroll)
	sbx.pack(expand=YES,fill=X)
	self.lists[0]['yscrollcommand']=sb.set

    def _select(self, y):
	row = self.lists[0].nearest(y)
	self.selection_clear(0, END)
	self.selection_set(row)
	#propertyCheck()
	return 'break'

    def _button2(self, x, y):
	for l in self.lists: l.scan_mark(x, y)
	return 'break'

    def _b2motion(self, x, y):
	for l in self.lists: l.scan_dragto(x, y)
	return 'break'

    def _scroll(self, *args):
	for l in self.lists:
	    apply(l.yview, args)
	return 'break'
	
    def horizontalscroll(self, *args):
	for l in self.lists:
	    apply(l.xview, args)
	return 'break'

    def curselection(self):
	return self.lists[0].curselection()

    def delete(self, first, last=None):
	for l in self.lists:
	    l.delete(first, last)

    def get(self, first, last=None):
	result = []
	for l in self.lists:
	    result.append(l.get(first,last))
	if last: return apply(map, [None] + result)
	return result
	    
    def index(self, index):
	self.lists[0].index(index)

    def insert(self, index, *elements):
	for e in elements:
	    i = 0
	    for l in self.lists:
		l.insert(index, e[i])
		i = i + 1

    def size(self):
	return self.lists[0].size()

    def see(self, index):
	for l in self.lists:
	    l.see(index)

    def selection_anchor(self, index):
	for l in self.lists:
	    l.selection_anchor(index)

    def selection_clear(self, first, last=None):
	for l in self.lists:
	    l.selection_clear(first, last)

    def selection_includes(self, index):
	return self.lists[0].selection_includes(index)

    def selection_set(self, first, last=None):
	for l in self.lists:
	    l.selection_set(first, last)

class MultiListbox(Frame):
    def __init__(self, master, lists):
	Frame.__init__(self, master)
	self.lists = []
	for l,w in lists:
	    frame = Frame(self); frame.pack(side=LEFT, expand=YES, fill=BOTH)
	    Label(frame, text=l, borderwidth=1, relief=RAISED).pack(fill=X)
	    lb = Listbox(frame, width=w, borderwidth=0, selectborderwidth=0,
			 relief=FLAT, exportselection=FALSE, selectmode=EXTENDED, bg="white")
	    lb.pack(expand=YES, fill=BOTH)
	    self.lists.append(lb)
	    lb.bind('<B1-Motion>', lambda e, s=self: s._select(e.y))
	    lb.bind('<Button-1>', lambda e, s=self: s._select(e.y))
	    lb.bind('<Leave>', lambda e: 'break')
	    lb.bind('<B2-Motion>', lambda e, s=self: s._b2motion(e.x, e.y))
	    lb.bind('<Button-2>', lambda e, s=self: s._button2(e.x, e.y))
	    lb.bind('<Button-4>', lambda e, s=self: s._scroll(SCROLL, -1, UNITS))
	    lb.bind('<Button-5>', lambda e, s=self: s._scroll(SCROLL, 1, UNITS))

	    
	frame = Frame(self); frame.pack(side=LEFT, fill=Y)
	Label(frame, borderwidth=1, relief=RAISED).pack(fill=X)
	sb = Scrollbar(frame, orient=VERTICAL, command=self._scroll)
	sb.pack(expand=YES, fill=Y)
	sbx = Scrollbar(frame, orient=HORIZONTAL,command=self.horizontalscroll)
	sbx.pack(expand=YES,fill=X)
	self.lists[0]['yscrollcommand']=sb.set
  
    def _select(self, y):
	row = self.lists[0].nearest(y)
	self.selection_clear(0, END)
	self.selection_set(row)
	selectionCheck()
	return 'break'

    def _button2(self, x, y):
	for l in self.lists: l.scan_mark(x, y)
	return 'break'

    def _b2motion(self, x, y):
	for l in self.lists: l.scan_dragto(x, y)
	return 'break'

    def _scroll(self, *args):
	for l in self.lists:
	    apply(l.yview, args)
	return 'break'
  
    def horizontalscroll(self, *args):
	for l in self.lists:
	    apply(l.xview, args)
	return 'break'

    def curselection(self):
	return self.lists[0].curselection()

    def delete(self, first, last=None):
	for l in self.lists:
	    l.delete(first, last)

    def get(self, first, last=None):
	result = []
	for l in self.lists:
	    result.append(l.get(first,last))
	if last: return apply(map, [None] + result)
	return result
	    
    def index(self, index):
	self.lists[0].index(index)

    def insert(self, index, *elements):
	for e in elements:
	    i = 0
	    for l in self.lists:
		l.insert(index, e[i])
		i = i + 1

    def size(self):
	return self.lists[0].size()

    def see(self, index):
	for l in self.lists:
	    l.see(index)

    def selection_anchor(self, index):
	for l in self.lists:
	    l.selection_anchor(index)

    def selection_clear(self, first, last=None):
	for l in self.lists:
	    l.selection_clear(first, last)

    def selection_includes(self, index):
	return self.lists[0].selection_includes(index)

    def selection_set(self, first, last=None):
	for l in self.lists:
	    l.selection_set(first, last)

# Local Global Variables

mlb = MultiListbox(root, (('Card', 10), ('Name', 50), ('Region', 10), ('Country', 20), ('Addresses', 20), ('Telephones', 20), ('Flags', 20)))
mlb2 = MultiListbox2(root, (('Name', 20), ('ParType', 20), ('ParValue', 20), ('Value', 50)))
	
def destroyWindow(window):
    window.destroy() 	
	
def clearTextArea():
    text.delete(0.0, END)
    text.pack()
    
def insertTextArea(line):
    text.insert(END, line)

def makeTextArea():
    global textfr, text
    # put a scroll bar in the frame
    scroll=Scrollbar(textfr)
    text.configure(yscrollcommand=scroll.set)
    
    #pack everything
    text.pack(side=LEFT)
    scroll.pack(side=RIGHT,fill=Y)
    textfr.pack(side=TOP)

def destroyProgram():
    if (os.path.exists("tempoutput.txt") == True):
	os.remove("tempoutput.txt")
    if (os.path.exists("tempoutput2.txt") == True):
	os.remove("tempoutput2.txt")
    if (os.path.exists("infooutput.txt") == True):
	os.remove("infooutput.txt")
    killServers()
    global conn
    #cur = conn.cursor()
    #cur.execute("DROP TABLE NAME CASCADE;")
    #cur.execute("DROP TABLE PROPERTY CASCADE;")
    #conn.commit()
    conn.close()
    root.destroy()

def exitProgram():
    # Remove temp files
    top = Toplevel()
    top.title("Confirm")
    top.geometry('220x100+580+360');
    msg = Message(top, text="Are you sure you want to quit?!!!", width = 220)
    msg.pack()
    button = Button(top, text="Yes", command=destroyProgram)
    button.pack()
    button2 = Button(top, text="Cancel", command=top.destroy)
    button2.pack()
  
    
def aboutFlagsColours():
    top = Toplevel()
    top.title("About Flags and Colours...")
    top.bind('<Escape>', lambda x: destroyWindow(top)) 
    top.geometry('600x250+550+300');
    msg = Message(top, text="Flag Meanings:\n\tC = card as a whole is in canonical form\n\tM = card has multiple same mandatory properties: FN or N.\n\tU, P, G = card has at least one URL, PHOTO, or GEO property,\n\trespectively.", width = 500)
    msg.pack()
    msg2 = Message(top, text="Colour Meanings:\n\tGreen  = card is in canonical form (the C flag is on).\n\tRed = card needs fixing because (a) it has multiple mandatory\n\tproperties (the M flag is on) or (b) its FN property has the same\n\tvalue as the preceding card's FN (signalling a duplication in the file).\n\tRed overrides green.\n\tYellow  = default state if neither green nor red applies. ", width = 500)
    msg2.pack()
    button = Button(top, text="Cancel", command=top.destroy)
    button.pack()   
    
def aboutProgram():
    top = Toplevel()
    top.title("About this application...")
    top.geometry('400x180+550+300');
    top.bind('<Escape>', lambda x: destroyWindow(top)) 
    msg = Message(top, text="This is VCard Analyzer\n\nWritten By: Marc Bodmer\n\nOnly compatible with VCard Versions: 3.0\n")
    msg.pack()
    
    button = Button(top, text="Cancel", command=top.destroy)
    button.pack()
  
def checkquit(event):
    top = Toplevel()
    top.title("Confirm")
    top.geometry('200x100+580+360');
    msg = Message(top, text="Are you sure you want to quit?!!!", width = 200)
    msg.pack()
    button = Button(top, text="Yes", command=root.destroy)
    button.pack()
    button2 = Button(top, text="Cancel", command=top.destroy)
    button2.pack()
    root.destroy()

def rewriteCard():
      FILE = open("tempoutput.txt","w")
      fullstring = ""
      for i in range(0, len(card)):
	  FILE.write("BEGIN:VCARD")
	  FILE.write("\r\n")
	  FILE.write("VERSION:3.0")
	  FILE.write("\r\n")
	  for element in range(0, len(card[i])):
	      #1 is Name. 2 is Parval, 3 is ParType, 4 is Value
	      if (element % 4 == 0):
		  fullstring = fullstring + card[i][element]
		  if (card[i][element + 1] != ""):
		      #Partype is not null
		      fullstring = fullstring + ";TYPE="
		      fullstring = fullstring + card[i][element + 1] 
		      fullstring = fullstring + ":"
		  else:
		      fullstring = fullstring + ":"
		      fullstring = fullstring + card[i][element + 1] 
		  if (card[i][element + 2] != ""):
		      #ParVal is not null
		      fullstring = fullstring + ";VALUE="
		      fullstring = fullstring + card[i][element + 2] 
		      fullstring = fullstring + ":"
		  else:
		      fullstring = fullstring + card[i][element + 2] 
		  fullstring = fullstring + card[i][element + 3]
		  for j in range(0, len(fullstring)):
		      FILE.write(fullstring[j])
		      if j % 75 == 0 and j != 0:
			  FILE.write("\r\n")
		  FILE.write("\r\n")
		  fullstring = ""
	  FILE.write("END:VCARD")
	  FILE.write("\r\n")

def makeBackupCard():
    backupcard = card

#File View Buttons
def deleteCard():
    global card
    global filestring
        
    top2 = Toplevel()   
        
    def deleteOK():
	global card
	if len(card) == 1:
	    top2.destroy()
	    top = Toplevel()
	    top.bind('<Escape>', lambda x: destroyWindow(top)) 
	    top.title("Error...")
	    top.geometry('300x80+550+300');
	    msg = Message(top, text="Cannot Delete Last Card!!!", width = 500)
	    msg.pack()	    
	    button = Button(top, text="Cancel", command=top.destroy)
	    button.pack()
	else:
	    top2.destroy()
	    card.remove(card[cardtodelete])
	    rewriteCard()
	    card = []
	    statuscode, card = Vcf.getFile("tempoutput.txt", card)
	    refreshCards()         
        
    if card == []:
	return
    else:  
	item = mlb.curselection()
	cardtodelete = int(item[0])
	    
	top2.title("Confirm")
	top2.geometry('400x90+550+300');
	top2.bind('<Escape>', lambda x: destroyWindow(top)) 
	msg = Message(top2, text="Are you sure you want to delete the 1 card(s)?", width = 500)     
	msg.pack()	
	button = Button(top2, text="OK", command=deleteOK)
	button.pack()
	button2 = Button(top2, text="Cancel", command=top2.destroy)
	button2.pack()

def addCards():
      global card
      global filestring
      global addcardflag
      if card == []:
	  return
      else:
	  tempcard = []
	  tempcard.append("N")
	  tempcard.append ("")
	  tempcard.append ("")
	  tempcard.append ("")
	  tempcard.append("FN")
	  tempcard.append ("")
	  tempcard.append ("")
	  tempcard.append ("")
	  card.insert(len(card), tempcard)
	  addcardflag = 1
	  refreshCards()

def mapSelected():
  	global card
  	global mapstarted
  	global mappointlist
  	
        if card == []:
	    return
	else:
	    if (mapstarted == 0):
		startWebServer(47005)
		mapstarted = 1
	    item = mlb.curselection()
	    cardtobrowse = int(item[0])
	    firstgeofound = 0
	    firstphotofound = 0
	    firstadrfound = 0
	    geotofind = ""
	    phototofind = ""
	    adrtofind = ""
	    for element in range(0, len(card[cardtobrowse])):
		if card[cardtobrowse][element] == "GEO":
		    if firstgeofound == 0:
			geotofind = card[cardtobrowse][element + 3]
			firstgeofound = 1
		if card[cardtobrowse][element] == "PHOTO":
		    if firstphotofound == 0:
			phototofind = card[cardtobrowse][element + 3]
			firstphotofound = 1 
		if card[cardtobrowse][element] == "ADR":
		    if firstadrfound == 0:
			adrtofind = card[cardtobrowse][element + 3]
			firstadrfound = 1 
	
	    if firstgeofound == 0:
		insertTextArea("No GEO Property Found!!!")
		return
	    elif ((geotofind == None or geotofind == "") and (phototofind != None or phototofind != "")):
		    insertTextArea("Displayable PHOTO but Invalid GEO!!!")
		    launchBrowser(phototofind)
		    return
	    elif geotofind == None or geotofind == "":
		insertTextArea("Invalid GEO Property!!!")
		return	    
	    if (phototofind.endswith(".jpg") != True and phototofind.endswith(".jpeg") != True):
		insertTextArea("Invalid PHOTO Property (No Inline JPEG)!!!")
		return
	    else:
		parts = geotofind.split(';')
		parts = geotofind.split(',')
		parts = geotofind.split(',')
		lat = float(parts[0])
		lon = float(parts[1])		
		mappointlist.append(phototofind)
		mappointlist.append(adrtofind)
		mappointlist.append(lat)
		mappointlist.append(lon)
		count = 0
		for element in range(0, len(mappointlist)):
		    if count == 0:
			gmd = GMapData( "Google Map", "Mapped Places", [lat, lon], 14 )	# use default values
		    if ((element + 1) % 4 == 0 and element != 0):
			gmd.addPoint( [mappointlist[element - 1], mappointlist[element]], mappointlist[element - 3], mappointlist[element - 2])	# s.b. center of map
			gmd.addOverlay(count, 1, 3 )
			count = count + 1
		###3. generate HTML to serve
		gmd.serve( "public_html/index.html" );
		###4. launch browser
		launchBrowser( "http://localhost:47005/" )
		#killServers()
		
def resetMaps():
    if card == []:
	return
    else:
	del mappointlist[:]
		

def browseSelected():	
	global card
        if card == []:
	    return
	else:  
	    firsturlfound = 0
	    urltobrowse = ""
	    item = mlb.curselection()
	    cardtobrowse = int(item[0])
	    for element in range(0, len(card[cardtobrowse])):
		if card[cardtobrowse][element] == "URL":
		    if firsturlfound == 0:
			urltobrowse = card[cardtobrowse][element + 3]
			firsturlfound = 1
	    
	    if firsturlfound == 0:
		#Insert no URL found into text Area
		insertTextArea("No URL Property Found!!!")
	    else:
		#Open web page to browse to that url
		if urltobrowse == None or urltobrowse == "":
		    insertTextArea("Invalid URL Property!!!")
		    return
		else:
		    launchBrowser(urltobrowse)

#Card View Buttons
def addProperty():
    global card
    global propertiesedited   
    fileviewselection = mlb.curselection()
    if card == [] or (len(fileviewselection) == 0):
	return
    else:
	propertiesedited = 1
	top = Toplevel()
	e1 = Entry(top)
	e2 = Entry(top)
	e3 = Entry(top)
	variable = StringVar(top)
	variable.set("NICKNAME")
	w = OptionMenu(top, variable, "NICKNAME", "PHOTO", "BDAY", "ADR", "TEL", "EMAIL", "GEO", "TITLE", "ORG", "NOTE", "UID", "URL", "OTHER")
         
	def addOK():
	    global card
	    name = variable.get()
	    if (name != "OTHER"):
		partype = e1.get()
		parval = e2.get()
	    value = e3.get()
	    tempcard = []
	    tempcard.append(name)
	    tempcard.append(partype)
	    tempcard.append(parval)
	    tempcard.append(value)

	    fileviewselection = mlb.curselection()
	    fileviewselection = int(fileviewselection[0])
	    card[fileviewselection].extend (tempcard)
	    refreshCards()
	    top.destroy()
	    #Add property to end of card list	    	    
      
	top.title("Select Card to Add...")
	top.bind('<Escape>', lambda x: destroyWindow(top)) 
	top.geometry('260x175+580+360');

	w.grid(row=0, column=1)
	Label(top, text="PARTYPE:").grid(row=1)
	e1.grid(row=1,column=1)
	Label(top, text="PARVAL:").grid(row=2)
	e2.grid(row=2,column=1)
	Label(top, text="VALUE:").grid(row=3)
	e3.grid(row=3,column=1)
	button = Button(top, text="OK", command=addOK)
	button.grid(row=4,column=1)
	button2 = Button(top, text="Cancel", command=top.destroy)
	button2.grid(row=5,column=1)
	
def deleteProperty():
    global card
    global propertiesedited  
    fileviewselection = mlb.curselection()
    if card == [] or (len(fileviewselection) == 0):
	return
    else:
	cardviewselection = mlb2.curselection() 
	if (len(cardviewselection) == 0):
	    return
	else:
	    propertiesedited = 1
	    cardviewselection = int(cardviewselection[0])
	    fileviewselection = int(fileviewselection[0])
	    cardviewselection = cardviewselection * 4
	    cardON = card[fileviewselection]
	    fncount = 0
	    ncount = 0
	    lastcheck = 0;
	    if (cardON[cardviewselection] == "FN"):
	      #Search list for any other FN
		for element in cardON:
		    if element == "FN":
			fncount = fncount + 1		
		if (fncount == 1):
		    lastcheck = 1
		    top = Toplevel()
		    top.bind('<Escape>', lambda x: destroyWindow(top)) 
		    top.title("Error...")
		    top.geometry('300x80+550+300');
		    msg = Message(top, text="Cannot Delete Last FN!!!", width = 500)
		    msg.pack()	    
		    button = Button(top, text="Cancel", command=top.destroy)
		    button.pack()
	    elif (cardON[cardviewselection] == "N"):
		for element in cardON:
		      if element == "N":
			  ncount = ncount + 1
		if (ncount == 1):
			  lastcheck = 1
			  top = Toplevel()
			  top.bind('<Escape>', lambda x: destroyWindow(top)) 
			  top.title("Error...")
			  top.geometry('300x80+550+300');
			  msg = Message(top, text="Cannot Delete Last N!!!", width = 500)
			  msg.pack()	    
			  button = Button(top, text="Cancel", command=top.destroy)
			  button.pack()
	    if (lastcheck != 1):	  
		del cardON[cardviewselection]
		del cardON[cardviewselection]
		del cardON[cardviewselection]
		del cardON[cardviewselection]
		refreshCards()    	    
		
def editProperty():
    global card
    fileviewselection = mlb.curselection()
    cardviewselection = mlb2.curselection() 
    global propertiesedited  
    if card == [] or (len(fileviewselection) == 0):
	return
    else:
	propertiesedited = 1
	if (len(cardviewselection) == 0):
	    return
	else:
	    top = Toplevel()
	    e = Entry(top)
	    e1 = Entry(top)
	    e2 = Entry(top)
	    e3 = Entry(top)
	
    def editOK():
        propname = e.get()
        partype = e1.get()
        parval = e2.get()
        value = e3.get()
        top.destroy()
	fileviewselection = mlb.curselection()
	cardviewselection = mlb2.curselection() 
        cardviewselection = int(cardviewselection[0])
	fileviewselection = int(fileviewselection[0])
	cardviewselection = cardviewselection * 4
	cardON = card[fileviewselection]
	cardON[cardON.index(cardON[cardviewselection])] = propname
	cardON[cardON.index(cardON[cardviewselection]) + 1] = partype
	cardON[cardON.index(cardON[cardviewselection]) + 2] = parval
	cardON[cardON.index(cardON[cardviewselection]) + 3] = value
	refreshCards()
    
    if card == [] or (len(fileviewselection) == 0):
	return
    else:
	if (len(cardviewselection) == 0):
	    return
	else:
	    top.title("Enter Property Values...")
	    top.bind('<Escape>', lambda x: destroyWindow(top)) 
	    top.geometry('270x150+580+360');
	    Label(top, text="NAME:").grid(row=0)
	    e.grid(row=0,column=1)
	    Label(top, text="PARTYPE:").grid(row=1)
	    e1.grid(row=1,column=1)
	    Label(top, text="PARVAL:").grid(row=2)
	    e2.grid(row=2,column=1)
	    Label(top, text="VALUE:").grid(row=3)
	    e3.grid(row=3,column=1)
	    button = Button(top, text="OK", command=editOK)
	    button.grid(row=4,column=1)
	    button2 = Button(top, text="Cancel", command=top.destroy)
	    button2.grid(row=5,column=1)	    
	    
def upProperty():
    global card
    fileviewselection = mlb.curselection()
    cardviewselection = mlb2.curselection() 
    global propertiesedited  
    if card == [] or (len(fileviewselection) == 0):
	return
    else:
	if (len(cardviewselection) == 0):
	    return
	else:
	    cardviewselection = int(cardviewselection[0])
	    fileviewselection = int(fileviewselection[0])
	    cardviewselection = cardviewselection * 4
	    if cardviewselection == 0:
		return
	    else:
		propertiesedited = 1
		cardON = card[fileviewselection]
		element1 = cardON[cardON.index(cardON[cardviewselection]) - 4]
		element2 = cardON[cardON.index(cardON[cardviewselection]) - 3]
		element3 = cardON[cardON.index(cardON[cardviewselection]) - 2] 
		element4 = cardON[cardON.index(cardON[cardviewselection]) - 1]
		element5 = cardON[cardON.index(cardON[cardviewselection])]
		element6 = cardON[cardON.index(cardON[cardviewselection]) + 1]
		element7 = cardON[cardON.index(cardON[cardviewselection]) + 2]
		element8 = cardON[cardON.index(cardON[cardviewselection]) + 3]
		e1 = cardON.index(element1)
		e2 = cardON.index(element5)
		cardON[e1], cardON[e2] = cardON[e2], cardON[e1]
		#Cant swap spaces
		e3 = cardON.index(element2)
		e4 = cardON.index(element6)
		cardON[e3], cardON[e4] = cardON[e4], cardON[e3]
		#Cant swap spaces
		e5 = cardON.index(element3)
		e6 = cardON.index(element7)
		cardON[e5], cardON[e6] = cardON[e6], cardON[e5]
		#Values are fine to swap
		e7 = cardON.index(element4)
		e8 = cardON.index(element8)
		cardON[e7], cardON[e8] = cardON[e8], cardON[e7]
		refreshCards()

def downProperty():
    global card
    fileviewselection = mlb.curselection()
    cardviewselection = mlb2.curselection() 
    global propertiesedited  
    if card == [] or (len(fileviewselection) == 0):
	return
    else:
	if (len(cardviewselection) == 0):
	    return
	else:
	    cardviewselection = int(cardviewselection[0])
	    fileviewselection = int(fileviewselection[0])
	    cardviewselection = cardviewselection * 4
	    if cardviewselection == 0:
		return
	    else:
		propertiesedited = 1
		cardON = card[fileviewselection]
		element1 = cardON[cardON.index(cardON[cardviewselection])]
		element2 = cardON[cardON.index(cardON[cardviewselection]) + 1]
		element3 = cardON[cardON.index(cardON[cardviewselection]) + 2] 
		element4 = cardON[cardON.index(cardON[cardviewselection]) + 3]
		element5 = cardON[cardON.index(cardON[cardviewselection]) + 4]
		element6 = cardON[cardON.index(cardON[cardviewselection]) + 5]
		element7 = cardON[cardON.index(cardON[cardviewselection]) + 6]
		element8 = cardON[cardON.index(cardON[cardviewselection]) + 7]
		e1 = cardON.index(element1)
		e2 = cardON.index(element5)
		cardON[e1], cardON[e2] = cardON[e2], cardON[e1]
		e3 = cardON.index(element2)
		e4 = cardON.index(element6)
		cardON[e3], cardON[e4] = cardON[e4], cardON[e3]
		e5 = cardON.index(element3)
		e6 = cardON.index(element7)
		cardON[e5], cardON[e6] = cardON[e6], cardON[e5]
		e7 = cardON.index(element4)
		e8 = cardON.index(element8)
		cardON[e7], cardON[e8] = cardON[e8], cardON[e7]
		refreshCards()
		
def commitProps():
      global card
      if card == []:
	  return
      else:
	  rewriteCard()
	  refreshCards()
	  
def revertProps():
      global card
      global backupcard
      if card == []:
	  return
      else:
	  card = backupcard[:]
	  #rewriteCard()
	  refreshCards()
	
#Menu Commands
def openFile():
    global propertiesedited  
    global saveDisabled
    saveDisabled = 0
    
    def loseChanges(top):
	global filestring
	global propertiesedited
	#Save card
	destroyWindow(top)
	rewriteCard()
	shutil.copyfile("tempoutput.txt", filestring)
	clearTextArea()
	filename = askopenfilename(filetypes=[("All Files","*"),("Vcard Files","*.vcf")])
	if filename == "":
	    return
	else:
	    global card
	    global backupcard
	    if card != []:
		card = []
	    filestring = filename
	    root.title(filename)
	    filecommand = "./vcftool -info < "
	    filecommand = filecommand + filename
	    filecommand = filecommand + "> infooutput.txt"
	    shutil.copyfile(filestring, "tempoutput.txt")
	    os.system (filecommand)
	    statuscode, card = Vcf.getFile(filename, card)
	    backupcard = card[:]
	    if card == []:
		top = Toplevel()
		top.title("Error")
		top.bind('<Escape>', lambda x: destroyWindow(top)) 
		top.geometry('250x60+550+300');
		msg = Message(top, text="No Cards Found in File!!!", width = 220)
		msg.pack()
		
		button = Button(top, text="Cancel", command=top.destroy)
		button.pack()
	    else:
		inFile = open("infooutput.txt", "r")
		line = inFile.readline()
		while line:
		    insertTextArea(line)
		    line = inFile.readline()
		propertiesedited = 0
		refreshCards()
    
    if propertiesedited == 1:
	top = Toplevel()
	top.title("Confirm")
	top.bind('<Escape>', lambda x: destroyWindow(top)) 
	top.geometry('350x100+550+300');
	msg = Message(top, text="Properties have been edited with current card. \nWould you like to: ", width = 350)
	msg.pack()
	
	button = Button(top, text="CANCEL, Keep Changes", command=top.destroy)
	button.pack(side=LEFT)
	button = Button(top, text="OK, Lose Changes", command=lambda:loseChanges(top))
	button.pack(side=LEFT)
    else:
	clearTextArea()
	filename = askopenfilename(filetypes=[("All Files","*"),("Vcard Files","*.vcf")])
	if filename == "":
	    return
	else:
	    global filestring
	    global card
	    global backupcard
	    if card != []:
		card = []
	    filestring = filename
	    root.title(filename)
	    filecommand = "./vcftool -info < "
	    filecommand = filecommand + filename
	    filecommand = filecommand + "> infooutput.txt"
	    shutil.copyfile(filestring, "tempoutput.txt")
	    os.system (filecommand)
	    statuscode, card = Vcf.getFile(filename, card)
	    backupcard = card[:]
	    if card == []:
		top = Toplevel()
		top.title("Error")
		top.bind('<Escape>', lambda x: destroyWindow(top)) 
		top.geometry('250x60+550+300');
		msg = Message(top, text="No Cards Found in File!!!", width = 220)
		msg.pack()
		
		button = Button(top, text="Cancel", command=top.destroy)
		button.pack()
	    else:
		inFile = open("infooutput.txt", "r")
		line = inFile.readline()
		while line:
		    insertTextArea(line)
		    line = inFile.readline()
		
		refreshCards()
  
def sortCards():
      global card
      global filestring
      if card == []:
	  return
      else:
	  filecommand = "./vcftool -sort < "
	  filecommand = filecommand + "tempoutput.txt"
	  filecommand = filecommand + " > tempoutput2.txt"   
	  os.system (filecommand) 
	  shutil.copyfile("tempoutput2.txt", "tempoutput.txt")
	  #Re read in Sorted File
	  card = []
	  statuscode, card = Vcf.getFile("tempoutput.txt", card)
	  refreshCards()
	  
def canonCards():
      global card
      global filestring
      if card == []:
	  return
      else:
	  filecommand = "./vcftool -canon < "
	  filecommand = filecommand + "tempoutput.txt"
	  filecommand = filecommand + " > tempoutput2.txt"
	  os.system (filecommand) 
	  shutil.copyfile("tempoutput2.txt", "tempoutput.txt")

	  #Re read in Sorted File
	  card = []
	  statuscode, card = Vcf.getFile("tempoutput.txt", card)
	  if card == []:
	      insertTextArea("No Cards Selected")
	  else:
	      refreshCards()

#OptionMenu Class
class Checkbar(Frame):
    def __init__(self, parent=None, picks=[], side=LEFT, anchor=W):
        Frame.__init__(self, parent)
        self.vars = []
        for pick in picks:
            var = IntVar()
            chk = Checkbutton(self, text=pick, variable=var)
            chk.pack(side=side, anchor=anchor, expand=YES)
            self.vars.append(var)
    def state(self):
        return map((lambda var: var.get()), self.vars)
          
                     
def selectCards():
    global card
    global filestring
    if card == []:
        return
    else:
	top = Toplevel()
	top.title("Select cards...")
	top.geometry('300x100+580+360');
	bar1 = Checkbar(top, ['Photos', 'URLs', 'Locations'])
	bar1.pack(side=TOP,  fill=X)
	bar1.config(relief=GROOVE, bd=2)    

	#Get states from the check boxes
	def runINFO():
	    global card
	    listoptions = bar1.state()
	    selectstring = ""
	    if (listoptions[0] == 1):
		selectstring = selectstring + "p"
	    if (listoptions[1] == 1):
		selectstring = selectstring + "u"
	    if (listoptions[2] == 1):
		selectstring = selectstring + "g"
	    
	    os.system("./vcftool -select '%s' < tempoutput.txt > tempoutput2.txt" % (selectstring))
	    shutil.copyfile("tempoutput2.txt", "tempoutput.txt")
	    #Re read in Sorted File
	    card = []
	    statuscode, card = Vcf.getFile("tempoutput.txt", card)
	    top.destroy()
	    if card == []:
		top2 = Toplevel()
		top2.bind('<Escape>', lambda x: destroyWindow(top)) 
		top2.title("Error...")
		top2.geometry('300x80+550+300');
		msg = Message(top2, text="Cannot Delete All Cards!!!", width = 500)
		msg.pack()	    
		button = Button(top2, text="Cancel", command=top.destroy)
		button.pack()
	    else:
		refreshCards()
	    
	button = Button(top, text="OK", command=runINFO)
	button.pack()
	button2 = Button(top, text="Cancel", command=top.destroy)
	button2.pack()
    
def undoAction():
      global card
      #Tempoutput is equal to filestring output again
      if card == []:
	  return
      else:
	  shutil.copyfile(filestring, "tempoutput.txt")
	  card = []
	  statuscode, card = Vcf.getFile(filestring, card)
	  refreshCards()
      
	  
def refreshCards():
 
      mlb.delete(0, mlb.size())
      mlb2.delete(0, mlb2.size())
      global card 
      #Redraw all the Cards 
      cardADRcount = 0
      cardTELcount = 0
      firstADRflag = 0
      nfncount = 0
      urlcount = 0
      geocount = 0
      photocount = 0
      region = ""
      country = ""
      flagstring = "12345"
      for i in range(len(card)):
	flagstring = flagstring.replace('1', '?')
	for element in range(0, len(card[i])):
	    if firstADRflag == 0:
		if card[i][element] == "ADR":
		    #get region prop
		    firstADR = card[i][element + 3]
		    parts = firstADR.split(';');
		    if len(parts) != 7:
			return
		    else:
			region = parts[4]
			country = parts [6]
			cardADRcount = cardADRcount + 1
			firstADRflag = 1
	    if card[i][element] == "ADR" and firstADRflag == 0:
		cardADRcount = cardADRcount + 1	    
	    if card[i][element] == "TEL":
		cardTELcount = cardTELcount + 1
	    if (card[i][element] == "N" or card[i][element] == "FN"):
		nfncount = nfncount + 1
	    if card[i][element] == "GEO":
		geocount = 1
	    if card[i][element] == "URL":
		urlcount = 1
	    if card[i][element] == "PHOTO":
		photocount = 1       
	    if card[i][element] == "UID":
		uidvalue = card[i][element + 3]
		for character in uidvalue:
		    if character == "*":
			flagstring = flagstring.replace('?', '-')
			break
		    else:
			flagstring = flagstring.replace('?', 'C')
	
	if (nfncount > 2):
	    flagstring = flagstring.replace('2', 'M') 
	else:
	    flagstring = flagstring.replace('2', '-') 
	if (urlcount != 0):
	    flagstring = flagstring.replace('3', 'U') 
	else:
	    flagstring = flagstring.replace('3', '-') 
	if (photocount != 0):
	    flagstring = flagstring.replace('4', 'P') 
	else:
	    flagstring = flagstring.replace('4', '-') 
	if (geocount != 0):
	    flagstring = flagstring.replace('5', 'G') 
	else:
	    flagstring = flagstring.replace('5', '-')     
	
	mlb.insert(END, ('%d' % (i + 1), card[i][3], region, country, cardADRcount, cardTELcount, flagstring))
	region = ""
	country = ""
	cardADRcount = 0
	cardTELcount = 0
	firstADRflag = 0
	nfncount = 0
	urlcount = 0
	geocount = 0
	flagstring = "12345"
	
      showfirstcard = 0 
      for i in range(0, len(card)):
	  for element in range(0, len(card[i])):
	      #1 is Name. 2 is Parval, 3 is ParType, 4 is Value
	      if (i == (len(card) - 1) and addcardflag == 1 and element % 4 == 0):
		  mlb2.insert(END, (card[len(card) - 1][element], card[len(card) - 1][element + 1], card[len(card) - 1][element + 2], card[len(card) - 1][element + 3]))
	      if (element % 4 == 0 and showfirstcard == 0 and addcardflag != 1):
		  mlb2.insert(END, (card[i][element], card[i][element + 1], card[i][element + 2], card[i][element + 3]))	       
	  showfirstcard = 1
	  
def appendCards():
	#Contents of selected filename
	global card
	tempcard = []
	#Tempoutput is equal to filestring output again
	if card == []:
	    return
	else:	    
	    filename = askopenfilename(filetypes=[("All Files","*"),("Vcard Files","*.vcf")])
	    if filename == "":
		return
	    else:
		statuscode, tempcard = Vcf.getFile(filename, tempcard)
		card = card + tempcard
		rewriteCard()
		refreshCards()
	
def saveFile():
	global card
	global saveDisabled
	if card == [] or saveDisabled == 1:
	    return
	else:
	  rewriteCard()
	  shutil.copyfile("tempoutput.txt", filestring)
	    
	
def saveAsFile():
	global card
	if card == []:
	    return
	else:
	  savefilename = asksaveasfilename(filetypes=[("All Files","*"),("Vcard Files","*.vcf")])
	  if savefilename == "":
	      return
	  else:
	      rewriteCard()
	      shutil.copyfile("tempoutput.txt", savefilename)
	      root.title(savefilename)
	  

def makeFileButtons(frame):
    filebuttons = Frame(height=2, bd=1)
    button1 = Button(filebuttons, text="Map Selected", command=mapSelected)
    button1.pack(side=LEFT)
    button2 = Button(filebuttons, text="Reset Map", command=resetMaps)
    button2.pack(side=LEFT)
    button3 = Button(filebuttons, text="Browse Selected", command=browseSelected)
    button3.pack(side=LEFT)
    button4 = Button(filebuttons, text="Delete Selected", command=deleteCard)
    button4.pack(side=LEFT)
    button5 = Button(filebuttons, text="Add card", command=addCards)
    button5.pack(side=LEFT)
    filebuttons.pack()
	
def makeCardButtons(frame):
    cardbuttons = Frame(height=2, bd=1)
    button = Button(cardbuttons, text="Up", command=upProperty)
    button.pack(side=LEFT)
    button1 = Button(cardbuttons, text="Down", command=downProperty)
    button1.pack(side=LEFT)
    button2 = Button(cardbuttons, text="Add Property", command=addProperty)
    button2.pack(side=LEFT)
    button3 = Button(cardbuttons, text="Delete Property", command=deleteProperty)
    button3.pack(side=LEFT)
    button4 = Button(cardbuttons, text="Edit Property", command=editProperty)
    button4.pack(side=LEFT)
    button5 = Button(cardbuttons, text="Commit", command=commitProps)
    button5.pack(side=LEFT)
    button6 = Button(cardbuttons, text="Revert", command=revertProps)
    button6.pack(side=LEFT)
    cardbuttons.pack()
	
def makeBottomBar(self,frame):
	bottomframe = Frame(frame,relief=RAISED,borderwidth=1)
	bottomframe.pack( side = BOTTOM )
	
	blackbutton = Button(bottomframe, text="Card View Panel", fg="black")
	blackbutton.pack( side = BOTTOM)
      
def storeAllDatabase():
    global conn
    global card   
    global existingnameID

    if card == []:
      print "Command Disabled!"
      return
    else:
      
      def handler():
	  pass
      
      def checkExisting(currentname, cur):
	 cur.execute("SELECT * FROM NAME;")
	 namelist = cur.fetchall()
	 for element in namelist:
	    temp = element[1].strip(";")
	    currentname = currentname.strip(";")
	    if (currentname == temp):
		global existingnameID
		existingnameID = element[0]
		return 1
	 return 0
	
      def replaceCard(currentname, newcard, top):
	cur.execute("SELECT * FROM NAME;")
	namelist = cur.fetchall()
	pinst = 1
	fn = 0
	nickname = 0
	photo = 0
	bday = 0
	adr = 0
	label = 0
	tel = 0
	email = 0
	geo = 0
	title = 0
	org = 0
	note = 0
	uid = 0
	url = 0
	other = 0
	
	for i in namelist:
	    temp = i[1].strip(";")
	    currentname = currentname.strip(";")
	    if (currentname == temp):
		#delete properties then at nameID
		replaceID = i[0]
		i = i[0]
		cur.execute("DELETE FROM PROPERTY where name_id = %s" % (replaceID,))
		conn.commit()
		for element in range(0, len(newcard)):
		  if (element % 4 == 0):
		    if (card[i][element] == "FN"):
		      fn = fn + 1
		      pinst = fn
		    if (card[i][element] == "NICKNAME"):
		      nickname = nickname + 1
		      pinst = nickname
		    if (card[i][element] == "PHOTO"):
		      photo = photo + 1
		      pinst = photo
		    if (card[i][element] == "BDAY"):
		      bday = bday + 1
		      pinst = bday
		    if (card[i][element] == "ADR"):
		      adr = adr + 1
		      pinst = adr
		    if (card[i][element] == "LABEL"):
		      label = label + 1
		      pinst = label
		    if (card[i][element] == "TEL"):
		      tel = tel + 1
		      pinst = tel
		    if (card[i][element] == "EMAIL"):
		      email = email + 1
		      pinst = email
		    if (card[i][element] == "GEO"):
		      geo = geo + 1
		      pinst = geo
		    if (card[i][element] == "TITLE"):
		      title = title + 1
		      pinst = title
		    if (card[i][element] == "ORG"):
		      org = org + 1
		      pinst = org
		    if (card[i][element] == "NOTE"):
		      note = note + 1
		      pinst = note
		    if (card[i][element] == "UID"):
		      uid = uid + 1
		      pinst = uid
		    if (card[i][element] == "URL"):
		      url = url + 1
		      pinst = url
		    if (card[i][element] == "OTHER"):
		      other = other + 1
		      pinst = other    
		    cur.execute("INSERT INTO PROPERTY (name_id,pname,pinst,partype,parval,value) VALUES ('%s','%s', '%s', '%s', '%s', '%s')" % (replaceID, card[i][element], pinst, card[i][element + 1], card[i][element + 2], card[i][element + 3],))
		    conn.commit()
        top.destroy()	    
	  
      def cancelStore(top):
	  global cancelflag
	  cancelflag = 1
	  top.destroy()
      
      def mergeCards(mergingcard, newcard, top):
	  for element in range(0, len(newcard)):
	    if (element % 4 == 0 and newcard[element] != "N"):
	      matchfound = 0
	      cur.execute("SELECT * FROM PROPERTY where name_id = '%s' and pname = '%s'" % (mergingcard, newcard[element],))
	      newlist = cur.fetchall()
	      instancenumber = len(newlist)

	      for i in range(0,len(newlist)):
		storedpname = newlist[i][1].strip(" ")
		if (newcard[element] == storedpname and newcard[element + 1] == newlist[i][3] and newcard[element + 2] == newlist[i][4] and newcard[element + 3] == newlist[i][5]):
		  matchfound = 1
	
	      if (matchfound != 1):	      
		#Insert property into list instancenumber + 1
		cur.execute("INSERT INTO PROPERTY (name_id,pname,pinst,partype,parval,value) VALUES ('%s','%s', '%s', '%s', '%s', '%s')" % (newlist[0][0], newcard[element], instancenumber + 1, newcard[element + 1], newcard[element + 2], newcard[element + 3],))
		conn.commit()
	  top.destroy()
	      	                  
      cur = conn.cursor()
      currentname = ""
      exists = 0     
      
      for i in range(0, len(card)):
	  pinst = 1
	  fn = 0
	  nameinst = 0
	  nickname = 0
	  photo = 0
	  bday = 0
	  adr = 0
	  label = 0
	  tel = 0
	  email = 0
	  geo = 0
	  title = 0
	  org = 0
	  note = 0
	  uid = 0
	  url = 0
	  other = 0
	  for element in range(0, len(card[i])):
	    
	    global cancelflag
	    
	    if (cancelflag == 1):
	      return
	    
	    if (element % 4 == 0 and card[i][element] == "N"):
	      currentname = card[i][element + 3]
	      exists = checkExisting(currentname, cur) #Check if the name currently exists in any cards
	    if (element % 4 == 0 and card[i][element] == "N"):
	      nameinst = nameinst + 1
	      if (exists == 1):
		  top = Toplevel()
		  top.title("Name Already Found!!!")
		  top.protocol("WM_DELETE_WINDOW", handler)
		  top.bind('<Escape>', lambda x: destroyWindow(top)) 
		  top.geometry('940x320+400+360');
		  mlb3 = MultiListbox2(top, (('Name', 20), ('ParType', 20), ('ParValue', 20), ('Value', 50)))
		  mlb3.pack(expand=YES,fill=BOTH)	    
		  #Fill mlb3 with existing card properties
		  global existingnameID
		  cur.execute("SELECT * FROM NAME where name_id = '%s'" % (existingnameID,))
		  existingcard = cur.fetchone()
		  mergingcard = existingcard[0]
		  selected = existingcard[0]
		  #Get the existing card from database
		  cur.execute("SELECT * FROM PROPERTY where name_id = '%s'" % (selected))
		  propertylist = cur.fetchall()
		  displaycard = []
		  for element in propertylist:
		    displaycard.append(element[1].strip(" "))
		    displaycard.append(element[3])
		    displaycard.append(element[4])
		    displaycard.append(element[5])
		    
		  for element in range(0, len(displaycard)):
		    if (element % 4 == 0):
			mlb3.insert(END, (displaycard[element], displaycard[element + 1], displaycard[element + 2], displaycard[element + 3]))
		  button1 = Button(top, text="Don't Store This Card", command=top.destroy)
		  button1.pack()
		  button2 = Button(top, text="Replace With This Card", command=lambda:replaceCard(currentname,card[i], top))
		  button2.pack()
		  button3 = Button(top, text="Merge With This Card", command=lambda:mergeCards(mergingcard, card[i], top))
		  button3.pack()
		  button4 = Button(top, text="Cancel Storing", command=lambda:cancelStore(top))
		  button4.pack()
		  top.wait_window()
	      else:
		  cur.execute("INSERT INTO NAME (name) VALUES (%s)", (currentname,))
		  #cur.execute("SELECT * FROM NAME where name = '%s'" % (currentname,))
		  #nameID = cur.fetchone()[0]
		  #cur.execute("INSERT INTO PROPERTY (name_id,pname,pinst,partype,parval,value) VALUES ('%s','%s', '%s', '%s', '%s', '%s')" % (nameID, card[i][element], nameinst, card[i][element + 1], card[i][element + 2], card[i][element + 3],))
		  conn.commit()

	    elif (element % 4 == 0 and card[i][element] != "N" and exists != 1):
	      
	      #Keep track of PINST
	      if (card[i][element] == "FN"):
		fn = fn + 1
		pinst = fn
	      if (card[i][element] == "NICKNAME"):
		nickname = nickname + 1
		pinst = nickname
	      if (card[i][element] == "PHOTO"):
		photo = photo + 1
		pinst = photo
	      if (card[i][element] == "BDAY"):
		bday = bday + 1
		pinst = bday
	      if (card[i][element] == "ADR"):
		adr = adr + 1
		pinst = adr
	      if (card[i][element] == "LABEL"):
		label = label + 1
		pinst = label
	      if (card[i][element] == "TEL"):
		tel = tel + 1
		pinst = tel
	      if (card[i][element] == "EMAIL"):
		email = email + 1
		pinst = email
	      if (card[i][element] == "GEO"):
		geo = geo + 1
		pinst = geo
	      if (card[i][element] == "TITLE"):
		title = title + 1
		pinst = title
	      if (card[i][element] == "ORG"):
		org = org + 1
		pinst = org
	      if (card[i][element] == "NOTE"):
		note = note + 1
		pinst = note
	      if (card[i][element] == "UID"):
		uid = uid + 1
		pinst = uid
	      if (card[i][element] == "URL"):
		url = url + 1
		pinst = url
	      if (card[i][element] == "OTHER"):
		other = other + 1
		pinst = other    
	      
	      cur.execute("SELECT * FROM NAME where name = '%s'" % (currentname,))
	      nameID = cur.fetchone()[0]
	      cur.execute("INSERT INTO PROPERTY (name_id,pname,pinst,partype,parval,value) VALUES ('%s','%s', '%s', '%s', '%s', '%s')" % (nameID, card[i][element], pinst, card[i][element + 1], card[i][element + 2], card[i][element + 3],))
	      conn.commit()	  

def storeSelectedDatabase():
    fileviewselection = mlb.curselection()
    if card == [] or (len(fileviewselection) == 0):
      print "Command Disabled!"
      return
    else:   
      fileviewselection = int(fileviewselection[0])
      
      def handler():
	  pass
      
      def checkExisting(currentname, cur):
	  cur.execute("SELECT * FROM NAME;")
	  namelist = cur.fetchall()
	  for element in namelist:
	    temp = element[1].strip(";")
	    currentname = currentname.strip(";")
	    if (currentname == temp):
		global existingnameID
		existingnameID = element[0]
		return 1
	  return 0
	
      def replaceCard(currentname, newcard, top):
	cur.execute("SELECT * FROM NAME;")
	namelist = cur.fetchall()
	pinst = 1
	fn = 0
	nickname = 0
	photo = 0
	bday = 0
	adr = 0
	label = 0
	tel = 0
	email = 0
	geo = 0
	title = 0
	org = 0
	note = 0
	uid = 0
	url = 0
	other = 0
	for i in namelist:
	    temp = i[1].strip(";")
	    currentname = currentname.strip(";")
	    if (currentname == temp):
		#delete properties then at nameID
		replaceID = i[0]
		i = i[0]
		cur.execute("DELETE FROM PROPERTY where name_id = %s" % (replaceID,))
		conn.commit()
		for element in range(0, len(newcard)):
		  #print i
		  #print element
		  if (element % 4 == 0):
		    if (card[i][element] == "FN"):
		      fn = fn + 1
		      pinst = fn
		    if (card[i][element] == "NICKNAME"):
		      nickname = nickname + 1
		      pinst = nickname
		    if (card[i][element] == "PHOTO"):
		      photo = photo + 1
		      pinst = photo
		    if (card[i][element] == "BDAY"):
		      bday = bday + 1
		      pinst = bday
		    if (card[i][element] == "ADR"):
		      adr = adr + 1
		      pinst = adr
		    if (card[i][element] == "LABEL"):
		      label = label + 1
		      pinst = label
		    if (card[i][element] == "TEL"):
		      tel = tel + 1
		      pinst = tel
		    if (card[i][element] == "EMAIL"):
		      email = email + 1
		      pinst = email
		    if (card[i][element] == "GEO"):
		      geo = geo + 1
		      pinst = geo
		    if (card[i][element] == "TITLE"):
		      title = title + 1
		      pinst = title
		    if (card[i][element] == "ORG"):
		      org = org + 1
		      pinst = org
		    if (card[i][element] == "NOTE"):
		      note = note + 1
		      pinst = note
		    if (card[i][element] == "UID"):
		      uid = uid + 1
		      pinst = uid
		    if (card[i][element] == "URL"):
		      url = url + 1
		      pinst = url
		    if (card[i][element] == "OTHER"):
		      other = other + 1
		      pinst = other    
		    cur.execute("INSERT INTO PROPERTY (name_id,pname,pinst,partype,parval,value) VALUES ('%s','%s', '%s', '%s', '%s', '%s')" % (replaceID, card[i][element], pinst, card[i][element + 1], card[i][element + 2], card[i][element + 3],))
		    conn.commit()
	top.destroy()	    
	  
      def cancelStore(top):
	  global cancelflag
	  cancelflag = 1
	  top.destroy()
      
      def mergeCards(mergingcard, newcard, top):
	  for element in range(0, len(newcard)):
	    if (element % 4 == 0 and newcard[element] != "N"):
	      matchfound = 0
	      cur.execute("SELECT * FROM PROPERTY where name_id = '%s' and pname = '%s'" % (mergingcard, newcard[element],))
	      newlist = cur.fetchall()
	      instancenumber = len(newlist)

	      for i in range(0,len(newlist)):
		storedpname = newlist[i][1].strip(" ")
		if (newcard[element] == storedpname and newcard[element + 1] == newlist[i][3] and newcard[element + 2] == newlist[i][4] and newcard[element + 3] == newlist[i][5]):
		  matchfound = 1
	
	      if (matchfound != 1):	      
		#Insert property into list instancenumber + 1
		cur.execute("INSERT INTO PROPERTY (name_id,pname,pinst,partype,parval,value) VALUES ('%s','%s', '%s', '%s', '%s', '%s')" % (newlist[0][0], newcard[element], instancenumber + 1, newcard[element + 1], newcard[element + 2], newcard[element + 3],))
		conn.commit()
	  top.destroy()
				  
      cur = conn.cursor()
      
      currentname = ""
      exists = 0     
      
      pinst = 1
      nameinst = 0
      fn = 0
      nickname = 0
      photo = 0
      bday = 0
      adr = 0
      label = 0
      tel = 0
      email = 0
      geo = 0
      title = 0
      org = 0
      note = 0
      uid = 0
      url = 0
      other = 0
      for element in range(0, len(card[fileviewselection])):
	
	global cancelflag
	
	if (cancelflag == 1):
	  return
	
	if (element % 4 == 0 and card[fileviewselection][element] == "N"):
	  currentname = card[fileviewselection][element + 3]
	  exists = checkExisting(currentname, cur) #Check if the name currently exists in any cards
	if (element % 4 == 0 and card[fileviewselection][element] == "N"):
	  nameinst = nameinst + 1
	  if (exists == 1):
	      top = Toplevel()
	      top.title("Name Already Found!!!")
	      top.protocol("WM_DELETE_WINDOW", handler)
	      top.bind('<Escape>', lambda x: destroyWindow(top)) 
	      top.geometry('940x320+400+360');
	      mlb3 = MultiListbox2(top, (('Name', 20), ('ParType', 20), ('ParValue', 20), ('Value', 50)))
	      mlb3.pack(expand=YES,fill=BOTH)	    

	      global existingnameID
	      cur.execute("SELECT * FROM NAME where name_id = '%s'" % (existingnameID,))
	      existingcard = cur.fetchone()
	      mergingcard = existingcard[0]
	      selected = existingcard[0]
	      #Get the existing card from database
	      cur.execute("SELECT * FROM PROPERTY where name_id = '%s'" % (selected))
	      propertylist = cur.fetchall()
	      displaycard = []
	      for element in propertylist:
		displaycard.append(element[1].strip(" "))
		displaycard.append(element[3])
		displaycard.append(element[4])
		displaycard.append(element[5])
		
	      for element in range(0, len(displaycard)):
		if (element % 4 == 0):
		    mlb3.insert(END, (displaycard[element], displaycard[element + 1], displaycard[element + 2], displaycard[element + 3]))
	      button1 = Button(top, text="Don't Store This Card", command=top.destroy)
	      button1.pack()
	      button2 = Button(top, text="Replace With This Card", command=lambda:replaceCard(currentname,card[fileviewselection], top))
	      button2.pack()
	      button3 = Button(top, text="Merge With This Card", command=lambda:mergeCards(mergingcard, card[fileviewselection], top))
	      button3.pack()
	      button4 = Button(top, text="Cancel Storing", command=lambda:cancelStore(top))
	      button4.pack()
	      top.wait_window()
	  else:
	      cur.execute("INSERT INTO NAME (name) VALUES (%s)", (currentname,))
	      #cur.execute("SELECT * FROM NAME where name = '%s'" % (currentname,))
	      #nameID = cur.fetchone()[0]
	      conn.commit()

	elif (element % 4 == 0 and card[fileviewselection][element] != "N" and exists != 1):
	  if (card[fileviewselection][element] == "FN"):
	    fn = fn + 1
	    pinst = fn
	  if (card[fileviewselection][element] == "NICKNAME"):
	    nickname = nickname + 1
	    pinst = nickname
	  if (card[fileviewselection][element] == "PHOTO"):
	    photo = photo + 1
	    pinst = photo
	  if (card[fileviewselection][element] == "BDAY"):
	    bday = bday + 1
	    pinst = bday
	  if (card[fileviewselection][element] == "ADR"):
	    adr = adr + 1
	    pinst = adr
	  if (card[fileviewselection][element] == "LABEL"):
	    label = label + 1
	    pinst = label
	  if (card[fileviewselection][element] == "TEL"):
	    tel = tel + 1
	    pinst = tel
	  if (card[fileviewselection][element] == "EMAIL"):
	    email = email + 1
	    pinst = email
	  if (card[fileviewselection][element] == "GEO"):
	    geo = geo + 1
	    pinst = geo
	  if (card[fileviewselection][element] == "TITLE"):
	    title = title + 1
	    pinst = title
	  if (card[fileviewselection][element] == "ORG"):
	    org = org + 1
	    pinst = org
	  if (card[fileviewselection][element] == "NOTE"):
	    note = note + 1
	    pinst = note
	  if (card[fileviewselection][element] == "UID"):
	    uid = uid + 1
	    pinst = uid
	  if (card[fileviewselection][element] == "URL"):
	    url = url + 1
	    pinst = url
	  if (card[fileviewselection][element] == "OTHER"):
	    other = other + 1
	    pinst = other    
	  
	  cur.execute("SELECT * FROM NAME where name = '%s'" % (currentname,))
	  nameID = cur.fetchone()[0]
	  cur.execute("INSERT INTO PROPERTY (name_id,pname,pinst,partype,parval,value) VALUES ('%s','%s', '%s', '%s', '%s', '%s')" % (nameID, card[fileviewselection][element], pinst, card[fileviewselection][element + 1], card[fileviewselection][element + 2], card[fileviewselection][element + 3],))
	  conn.commit()

      
	  

def openFromDatabase():
    global propertiesedited  
    global saveDisabled
    saveDisabled = 1
    
    def loseChanges(top):
	global filestring
	global propertiesedited
	#Save card
	destroyWindow(top)
	rewriteCard()
	shutil.copyfile("tempoutput.txt", filestring)
	clearTextArea()
	filename = askopenfilename(filetypes=[("All Files","*"),("Vcard Files","*.vcf")])
	if filename == "":
	    return
	else:
	    global card
	    global backupcard
	    if card != []:
		card = []
	    filestring = filename
	    root.title(filename)
	    filecommand = "./vcftool -info < "
	    filecommand = filecommand + filename
	    filecommand = filecommand + "> infooutput.txt"
	    shutil.copyfile(filestring, "tempoutput.txt")
	    os.system (filecommand)
	    statuscode, card = Vcf.getFile(filename, card)
	    backupcard = card[:]
	    if card == []:
		top = Toplevel()
		top.title("Error")
		top.bind('<Escape>', lambda x: destroyWindow(top)) 
		top.geometry('250x60+550+300');
		msg = Message(top, text="No Cards Found in File!!!", width = 220)
		msg.pack()
		
		button = Button(top, text="Cancel", command=top.destroy)
		button.pack()
	    else:
		inFile = open("infooutput.txt", "r")
		line = inFile.readline()
		while line:
		    insertTextArea(line)
		    line = inFile.readline()
		propertiesedited = 0
		refreshCards()
    
    if propertiesedited == 1:
	top = Toplevel()
	top.title("Confirm")
	top.bind('<Escape>', lambda x: destroyWindow(top)) 
	top.geometry('350x100+550+300');
	msg = Message(top, text="Properties have been edited with current card. \nWould you like to: ", width = 350)
	msg.pack()
	
	button = Button(top, text="CANCEL, Keep Changes", command=top.destroy)
	button.pack(side=LEFT)
	button = Button(top, text="OK, Lose Changes", command=lambda:loseChanges(top))
	button.pack(side=LEFT)
    else:
	clearTextArea()
	cur = conn.cursor()
	cur.execute("SELECT * FROM NAME;")
	databaselist = cur.fetchall()
	
	if len(databaselist) == 0:
	    print("Nothing Found In Database!")
	    return
	else:
	    global card
	    global backupcard
	    if card != []:
		card = []
	    for name in databaselist:
	      newcard = []
	      root.title("Database")
	      cur.execute("SELECT * FROM PROPERTY where name_id = '%s'" % (name[0],))
	      propertylist = cur.fetchall()
	      tempname = name[1]
	      newcard.append("N")
	      newcard.append("")
	      newcard.append("")
	      newcard.append(tempname)
	      for element in propertylist:
		#Build newcard to add to card
		newcard.append(element[1].strip(" "))
		newcard.append(element[3])
		newcard.append(element[4])
		newcard.append(element[5])
	      card.append(newcard)

	    backupcard = card[:]
	    refreshCards()

def appendFromDatabase():
  global card
  
  if card == []:
    print "Command Disabled!"
    return
  else:
    global saveDisabled
    saveDisabled = 0
    clearTextArea()
    cur = conn.cursor()
    cur.execute("SELECT * FROM NAME;")
    databaselist = cur.fetchall()
    
    if len(databaselist) == 0:
	print("Nothing Found In Database!")
	return
    else:
	global backupcard

	for name in databaselist:
	  newcard = []
	  root.title("Database")
	  cur.execute("SELECT * FROM PROPERTY where name_id = '%s'" % (name[0],))
	  propertylist = cur.fetchall()
	  tempname = name[1]
	  newcard.append("N")
	  newcard.append("")
	  newcard.append("")
	  newcard.append(tempname)
	  for element in propertylist:
	    #Build newcard to add to card
	    newcard.append(element[1].strip(" "))
	    newcard.append(element[3])
	    newcard.append(element[4])
	    newcard.append(element[5])
	  card.append(newcard)
	  
	#rewriteCard()         
	refreshCards()
	
def queryCommand():
    

    # ASK ABOUT PRIMARY KEYS IN PROPERTY TABLE
    
    def submitQuery(v, e1, e2, e3, e4, selectstring, text):
	v = v.get()
	global conn
	cur = conn.cursor()
	conn.commit()
	
	if (v == 1):
	    string = e1.get()
	    #Get a list of all FN names, find ones that match and print the property and the NAME using name_id
	    cur.execute("SELECT * FROM PROPERTY where pname = 'FN' and value LIKE '%s'" % (string,))
	    newlist = cur.fetchall()
	    if (len(newlist) == 0):
	      text.insert(END,"No Results Found\n")
	      text.insert(END, "--------------------------------------------------------------------------------------------------------------\n")
	    else:
	      for element in newlist:
		cur.execute("SELECT * FROM NAME where name_id = '%s' ORDER BY name DESC" % (element[0],))	
		namelist = cur.fetchall()
		text.insert(END, namelist)
		text.insert(END, "\n")
		cur.execute("SELECT * FROM PROPERTY where name_id = '%s' ORDER BY pname ASC" % (element[0],))
		propertylist = cur.fetchall()
		for prop in propertylist:
		  text.insert(END, prop)
		  text.insert(END, "\n")
	      text.insert(END, "--------------------------------------------------------------------------------------------------------------\n")
	    
	elif (v == 2):
	    string = e2.get()
	    cur.execute("SELECT DISTINCT name_id FROM PROPERTY where pname = 'ADR' and value LIKE '%%;%%;%%;%%;%%;%s'" % (string,))
	    newlist = cur.fetchall()
	    if (len(newlist) == 0):
	      text.insert(END,"No Results Found\n")
	      text.insert(END, "--------------------------------------------------------------------------------------------------------------\n")
	    else:
	      count = len(newlist)
	      text.insert(END, "There are: " + str(count) + " cards in " + string + "\n")
	      text.insert(END, "--------------------------------------------------------------------------------------------------------------\n")
	elif (v == 3):
	    string = e3.get()
	    cur.execute("SELECT * FROM PROPERTY where pname = 'TEL' and value LIKE '%s'" % (string,))
	    newlist = cur.fetchone()
	    if (newlist == None):
	      text.insert(END,"No Results Found\n")
	      text.insert(END, "--------------------------------------------------------------------------------------------------------------\n")
	    else:
	      cur.execute("SELECT * FROM NAME where name_id = '%s'" % (newlist[0],))
	      namelist = cur.fetchone()
	      namefound = namelist[1]
	      text.insert(END, "" + namefound + " has the telephone number " + str(string) + "\n")
	      text.insert(END, "--------------------------------------------------------------------------------------------------------------\n")	    
	elif (v == 4):
	    string = e4.get()
	    cur.execute("SELECT * FROM PROPERTY where pname = 'ORG'")
	    newlist = cur.fetchall()
	    foundflag = 0;
	    for element in newlist:
		place = element[5]
		newplace = re.sub(r'\W+', '', place)
		newplace = newplace.lower()
		newstring = re.sub(r'\W+', '', string)
		newstring = newstring.lower()
		if (newplace == newstring):
		  foundflag = 1
		  cur.execute("SELECT * FROM NAME where name_id = '%s'" % (element[0],))
		  namefound = cur.fetchone()
		  thename = namefound[1]
		  text.insert(END, thename + " works at " + string + "\n")
	    if (foundflag == 0):
	      text.insert(END,"No Results Found\n")
	    text.insert(END, "--------------------------------------------------------------------------------------------------------------\n")
		
	elif (v == 5):
	    string = selectstring.get()
	    try:
	      cur.execute(string)
	      newlist = cur.fetchall()
	      if (len(newlist) == 0):
		text.insert(END,"No Results Found\n")
		text.insert(END, "--------------------------------------------------------------------------------------------------------------\n")
	      else:
		for element in newlist:
		  text.insert(END, element)
		  text.insert(END, "\n")
	      text.insert(END, "--------------------------------------------------------------------------------------------------------------\n")
	    except Exception, e:
	      text.insert(END,"Error Running Select Statement!!!\n")
	      text.insert(END, "--------------------------------------------------------------------------------------------------------------\n")
	    
    
    def clearText(text):
	text.delete(0.0, END)
	text.pack()
	
    def helpWindow():
      top = Toplevel()
      top.geometry('400x100+530+360');
      top.title("Help...")
      L1 = Label(top, text="Table NAME: name_id name")
      L2 = Label(top, text="Table PROPERTY: name_id pname pinst partype parval value")
      L1.pack()
      L2.pack()
      button = Button(top, text="Cancel", command=top.destroy)
      button.pack()

    global queryOpen
    if queryOpen == 1:
      return
    else:
      queryOpen = 1
      top = Toplevel()
      top.geometry('800x340+500+360');
      top.title("Query Database")
      queryFrame = Frame(top)
      buttonFrame = Frame(top)
      buttonFrame2 = Frame(top)
      
      v = IntVar()
      Radiobutton(queryFrame, variable=v, value=1).grid(row=0, sticky=W)
      Radiobutton(queryFrame, variable=v, value=2).grid(row=1, sticky=W)
      Radiobutton(queryFrame, variable=v, value=3).grid(row=2, sticky=W)
      Radiobutton(queryFrame, variable=v, value=4).grid(row=3, sticky=W)
      Radiobutton(queryFrame, variable=v, value=5).grid(row=4, sticky=W)
      
      e1 = Entry(queryFrame)
      e1.grid(row=0, column=2, sticky=E+W+N+S, columnspan=2)
      e2 = Entry(queryFrame)
      e2.grid(row=1, column=2, sticky=E, columnspan=2)
      e3 = Entry(queryFrame)
      e3.grid(row=2, column=2, sticky=E, columnspan=2)
      e4 = Entry(queryFrame)
      e4.grid(row=3, column=2, sticky=E, columnspan=2)
      selectString = StringVar()
      selectString.set("SELECT")
      e5 = Entry(queryFrame, textvariable=selectString)
      e5.grid(row=4,column=1, sticky=N+E+W+S, columnspan=4)
      Label(queryFrame, text="1. Find all cards with the name _____ (SQL wild card % is permitted):").grid(row=0,column=1, sticky=W)
      Label(queryFrame, text="2. How many cards are in _____ (country)?").grid(row=1,column=1, sticky=W)
      Label(queryFrame, text="3. Find the name of the card with the TEL _____ (Format: 1-XXX-XXX-XXXX):").grid(row=2,column=1, sticky=W)
      Label(queryFrame, text="4. Find the names of people who belong to the organization (ORG) ____ (Any Format):").grid(row=3,column=1, sticky=W)
      textfr=Frame(top)
      text=Text(textfr,height=10,width=110,background='white')
      scroll=Scrollbar(textfr)
      text.configure(yscrollcommand=scroll.set)
      scroll.config(command=text.yview)

      #pack everything
      text.pack(side=LEFT)
      scroll.pack(side=RIGHT,fill=Y)
      submitButton = Button(buttonFrame, text="Submit", fg="black", command = lambda:submitQuery(v, e1, e2, e3, e4, selectString, text))
      submitButton.pack( side = LEFT)
      helpButton = Button(buttonFrame, text="Help", fg="black", command = helpWindow)
      helpButton.pack( side = LEFT)
      clearButton = Button(buttonFrame2, text="Clear", fg="black", command = lambda:clearText(text))
      clearButton.pack( side = LEFT)
      queryFrame.pack(side=TOP)
      buttonFrame2.pack(side=BOTTOM)
      textfr.pack(side=BOTTOM)
      buttonFrame.pack(side=BOTTOM) 
  
def main():
	global root
	global mlb
	global mlb2
	global username
	global conn
	
	if (len(sys.argv) > 1):
	    username = sys.argv[1]
	else:
	  print("Please Specify Username For Database")
	  print ("Usage: python xvcf.py [username]")
	  sys.exit()
	try:
	   conn = psycopg2.connect (host = "db.socs.uoguelph.ca", user = username, database = username)
	except Exception, e:
	   print ("Could Not Connect Database!")
	   print ("Usage: python xvcf.py [username]")
	   sys.exit()
	
	try:
	  cur = conn.cursor()
	  cur.execute("CREATE TABLE NAME (name_id SERIAL PRIMARY KEY, name VARCHAR(60) NOT NULL);")
	  cur.execute("CREATE TABLE PROPERTY (PRIMARY KEY (name_id, pname, pinst), name_id INT NOT NULL REFERENCES NAME ON DELETE CASCADE, pname char(8) NOT NULL, pinst smallint NOT NULL, partype text, parval text, value text);")
	except Exception, e:
	  print("Existing Database Found")
	  conn.commit()
	  
	root.title('XVCF')
	#Create menu bar
	menubar = Menu(root)
	filemenu = Menu(menubar, tearoff=0)
	filemenu.add_command(label="Open...", command=openFile)
	filemenu.add_command(label="Append...", command=appendCards)
	filemenu.add_command(label="Save", command=saveFile )
	filemenu.add_command(label="Save as...", command=saveAsFile )
	filemenu.add_separator()
	filemenu.add_command(label="Exit", command=exitProgram)

	menubar.add_cascade(label="File", menu=filemenu)
	editmenu = Menu(menubar, tearoff=0)

	editmenu.add_command(label='Sort', command=sortCards)
	editmenu.add_command(label='Canonicalize', command=canonCards)
	editmenu.add_command(label='Select...', command=selectCards)
	editmenu.add_command(label="Undo", command=undoAction )

	menubar.add_cascade(label="Organize", menu=editmenu)
	
	databasemenu = Menu(menubar, tearoff=0)
	databasemenu.add_command(label="Store All", command=storeAllDatabase)
	databasemenu.add_command(label="Store Selected", command=storeSelectedDatabase)
	databasemenu.add_command(label="Open From Database", command=openFromDatabase)
	databasemenu.add_command(label="Append From Database", command=appendFromDatabase)
	databasemenu.add_command(label="Query...", command=queryCommand)
	menubar.add_cascade(label="Database", menu=databasemenu)
	
	helpmenu = Menu(menubar, tearoff=0)
	helpmenu.add_command(label='Card flags and colours...', command=aboutFlagsColours)
	helpmenu.add_command(label="About xvcf...", command=aboutProgram )
	menubar.add_cascade(label="Help", menu=helpmenu)
	root.config(menu=menubar)
	#Create file view panel
	mlb.pack(expand=YES,fill=BOTH)
	
	#Create file view buttons
	makeFileButtons(root)
	
	#Create card view panel
	mlb2.pack(expand=YES,fill=BOTH)

	#Create card view buttons
	makeCardButtons(root)

	#Create the scrolling text area
	makeTextArea()
	
	#Create ClearText Button		    
	bottomframe = Frame(root,borderwidth=3)
	bottomframe.pack( side = BOTTOM )
	blackbutton = Button(bottomframe, text="Clear TextArea", fg="black", command = clearTextArea)
	blackbutton.pack( side = BOTTOM)
	
	#root.protocol("WM_DELETE_WINDOW", ask_quit(root))
	root.option_add('*tearOff', FALSE)
	root.config(menu=menubar)
	#k=mywidgets(root)
	root.geometry('1300x800+210+200');
	root.bind('<Escape>', lambda x: exitProgram()) 
	root.mainloop()
main()