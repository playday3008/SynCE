#!/usr/bin/env python
# -*- coding: iso-8859-1 -*-
#	Created:	Fre Mär 26 18:28:40 CET 2004	by M. Biermaier	on linuxorange
__FileVersion__ = """\
#	Version:	Mit Apr 21 18:18:36 CEST 2004	on linuxorange
"""
#	$Id$

"""
GUI for files on PocketPC.

Show files from PocketPC in a TreeView.


Based on:
pyrapi - A python wrapper class around the librapi2 library
	by Richard Taylor <r.taylor@bcs.org.uk>

SynCE - The purpose of the SynCE project is to provide a means of
	communication with a Windows CE device from a computer running
	Linux, *BSD or other unices.
	by David Eriksson <twogood@users.sourceforge.net>

PyQt - Python Bindings for the Qt Toolkit
	by Phil Thompson <phil@riverbankcomputing.co.uk>

QT - A multiplatform C++ GUI application framework
	by Trolltech AS http://www.trolltech.com

dirview.py
	QT-example reworked by Phil Thompson (?)
"""

__author__		= "M. Biermaier"
__version__		= "$Revision$"
__date__		= "$Date$"


# Imports [
#--------------------------------------------------
import sys
import os

try:
	# M.B. 2003-01-15
	# We set Qtopia before the "import", because "import"
	# may raise other errors (e.g. missing libraries).
	Qtopia			= 1
	from qtpe import QPEApplication
except ImportError:
	Qtopia			= 0

# We have a problem on MacOS X...
if sys.platform == "darwin":
	import qtui
from qt import *

theHOME			= os.getenv ("HOME")
sys.path.append (theHOME + "/Python")

import DebugPrint	# DebugPrinting

from pyrapi import pyrapi
import time
import getopt
#--------------------------------------------------
# Imports ]


# Constants [
#--------------------------------------------------
SETTINGS_NAME	= ".CE_DirView.cfg"

# Debug switches
DEBUG			= False
DEBUG_PRINT		= False
DEBUG_OBJECTS	= False
DEBUG_POSITIONS	= False
DEBUG_COMPARE	= False
DEBUG_RENAME	= False
DEBUG_COPY		= False

# Settings
prefStatusWant	= True

AppName			= "PocketPC_DirectoryBrowser"
theCaption		= "PocketPC - Directory Browser - " + __version__.replace ("$", "")

PATH_SEPEARATOR	= "\\"

# Column Numbers
COL_NAME		= 0
COL_TYPE		= 1
COL_SIZE		= 2
COL_LASTWRITETIME= 3

# Drag 'n' Drop actions
ActionNone		= 0
ActionCopy		= 1
ActionMove		= 2
ActionLink		= 3

folder_closed_xpm = [
	"16 16 9 1",
	"g c #808080",
	"b c #c0c000",
	"e c #c0c0c0",
	"# c #000000",
	"c c #ffff00",
	". c None",
	"a c #585858",
	"f c #a0a0a4",
	"d c #ffffff",
	"..###...........",
	".#abc##.........",
	".#daabc#####....",
	".#ddeaabbccc#...",
	".#dedeeabbbba...",
	".#edeeeeaaaab#..",
	".#deeeeeeefe#ba.",
	".#eeeeeeefef#ba.",
	".#eeeeeefeff#ba.",
	".#eeeeefefff#ba.",
	".##geefeffff#ba.",
	"...##gefffff#ba.",
	".....##fffff#ba.",
	".......##fff#b##",
	".........##f#b##",
	"...........####."
]

folder_open_xpm = [
	"16 16 11 1",
	"# c #000000",
	"g c #c0c0c0",
	"e c #303030",
	"a c #ffa858",
	"b c #808080",
	"d c #a0a0a4",
	"f c #585858",
	"c c #ffdca8",
	"h c #dcdcdc",
	"i c #ffffff",
	". c None",
	"....###.........",
	"....#ab##.......",
	"....#acab####...",
	"###.#acccccca#..",
	"#ddefaaaccccca#.",
	"#bdddbaaaacccab#",
	".eddddbbaaaacab#",
	".#bddggdbbaaaab#",
	"..edgdggggbbaab#",
	"..#bgggghghdaab#",
	"...ebhggghicfab#",
	"....#edhhiiidab#",
	"......#egiiicfb#",
	"........#egiibb#",
	"..........#egib#",
	"............#ee#"
]

folder_locked = [
	"16 16 10 1",
	"h c #808080",
	"b c #ffa858",
	"f c #c0c0c0",
	"e c #c05800",
	"# c #000000",
	"c c #ffdca8",
	". c None",
	"a c #585858",
	"g c #a0a0a4",
	"d c #ffffff",
	"..#a#...........",
	".#abc####.......",
	".#daa#eee#......",
	".#ddf#e##b#.....",
	".#dfd#e#bcb##...",
	".#fdccc#daaab#..",
	".#dfbbbccgfg#ba.",
	".#ffb#ebbfgg#ba.",
	".#ffbbe#bggg#ba.",
	".#fffbbebggg#ba.",
	".##hf#ebbggg#ba.",
	"...###e#gggg#ba.",
	".....#e#gggg#ba.",
	"......###ggg#b##",
	".........##g#b##",
	"...........####."
]

pix_file = [
	"16 16 7 1",
	"# c #000000",
	"b c #ffffff",
	"e c #000000",
	"d c #404000",
	"c c #c0c000",
	"a c #ffffc0",
	". c None",
	"................",
	".........#......",
	"......#.#a##....",
	".....#b#bbba##..",
	"....#b#bbbabbb#.",
	"...#b#bba##bb#..",
	"..#b#abb#bb##...",
	".#a#aab#bbbab##.",
	"#a#aaa#bcbbbbbb#",
	"#ccdc#bcbbcbbb#.",
	".##c#bcbbcabb#..",
	"...#acbacbbbe...",
	"..#aaaacaba#....",
	"...##aaaaa#.....",
	".....##aa#......",
	".......##......."
]
#--------------------------------------------------
# Constants ]


# Global VARs [
#--------------------------------------------------
folderLocked	= None
folderClosed	= None
folderOpened	= None
fileNormal		= None
#--------------------------------------------------
# Global VARs ]


# Trace2 [
#--------------------------------------------------
def TraceBeg ():
	if DEBUG > 1: print ">>>>>", sys._getframe (1).f_code.co_name

def TraceEnd ():
	if DEBUG > 1: print "<<<<<", sys._getframe (1).f_code.co_name
#--------------------------------------------------
# Trace2 ]

# dPrint [
#--------------------------------------------------
def dPrint (aLabel, aValue):
	"""Allow printing of unicode-strings"""

	if isinstance (aValue, QString):
		dPrint1 (aLabel, aValue.latin1 ())
	else:
		dPrint1 (aLabel, aValue)
#--------------------------------------------------
# dPrint ]


# CE_FileInfo [
#--------------------------------------------------
class CE_FileInfo (QFileInfo):
	def __init__ (self):
		QFileInfo.__init__ (self)
		ce_file			= ""
		ce_filePath		= ""

	# filePath [
	#--------------------------------------------------
	def filePath (self):
		TraceBeg ()

		TraceEnd ()
		return self.ce_filePath
	#--------------------------------------------------
	# filePath ]

	# fileName [
	#--------------------------------------------------
	def fileName (self):
		TraceBeg ()

		TraceEnd ()
		return self.ce_file.cFileName
	#--------------------------------------------------
	# fileName ]

	# isFile [
	#--------------------------------------------------
	def isFile (self):
		TraceBeg ()

		TraceEnd ()
		return self.ce_file.dwFileAttributes != 16
	#--------------------------------------------------
	# isFile ]

	# isDir [
	#--------------------------------------------------
	def isDir (self):
		TraceBeg ()

		TraceEnd ()
		return self.ce_file.dwFileAttributes == 16
	#--------------------------------------------------
	# isDir ]

	# fileSize [
	#--------------------------------------------------
	def fileSize (self):
		TraceBeg ()

		TraceEnd ()
		return self.ce_file.nFileSizeLow
	#--------------------------------------------------
	# fileSize ]

	# ftLastWriteTime [
	#--------------------------------------------------
	def ftLastWriteTime (self):
		TraceBeg ()

		TraceEnd ()
		return time.ctime (self.ce_file.ftLastWriteTime)
	#--------------------------------------------------
	# ftLastWriteTime ]

	# ftLastWriteTimeRaw [
	#--------------------------------------------------
	def ftLastWriteTimeRaw (self):
		TraceBeg ()

		TraceEnd ()
		return self.ce_file.ftLastWriteTime
	#--------------------------------------------------
	# ftLastWriteTimeRaw ]
#--------------------------------------------------
# CE_FileInfo ]


# CE_Dir [
#--------------------------------------------------
class CE_Dir (QDir):
	def __init__ (self, filename=None):
		QDir.__init__ (self, filename)
		self.pathName	= filename

	# drives [
	#--------------------------------------------------
	def drives (self):
		TraceBeg ()

		files	= []

		theFileInfo				= CE_FileInfo ()

		theFileInfo.ce_file		= file
		theFileInfo.ce_filePath	= ""
			
		files.append (theFileInfo)

		TraceEnd ()
		return files
	#--------------------------------------------------
	# drives ]

	# entryInfoList [
	#--------------------------------------------------
	def entryInfoList (self):
		TraceBeg ()

		entries	= []

		path	= str (self.pathName.latin1 ())
		if path == "\\":
			path = ""
		if DEBUG > 3:
			dPrint ("path", path)
			dPrint ("type (path)", type (path))

		for file in pyrapi.CeFindAllFiles (
								path + r"\*.*",
								pyrapi.FAF_NAME | \
								pyrapi.FAF_ATTRIBUTES | \
								pyrapi.FAF_CREATION_TIME | \
								pyrapi.FAF_LASTACCESS_TIME | \
								pyrapi.FAF_LASTWRITE_TIME | \
								pyrapi.FAF_SIZE_LOW \
								):
			theFileInfo				= CE_FileInfo ()
			theFileInfo.ce_file		= file
			theFileInfo.ce_filePath	= path
			entries.append (theFileInfo)

		TraceEnd ()

		if DEBUG > 3:
			dPrint ("entries", entries)
		return entries
	#--------------------------------------------------
	# entryInfoList ]

	# isReadable [
	#--------------------------------------------------
	def isReadable (self):
		TraceBeg ()

		TraceEnd ()
		return True
	#--------------------------------------------------
	# isReadable ]
#--------------------------------------------------
# CE_Dir ]


# CE_File [
#--------------------------------------------------
class CE_File (QFile):
	def __init__ (self, filename=None):
		QFile.__init__ (self, filename)
		self.ce_fileName	= filename

	# name [
	#--------------------------------------------------
	def name (self):
		TraceBeg ()

		TraceEnd ()
		return QString (self.ce_fileName)
	#--------------------------------------------------
	# name ]
#--------------------------------------------------
# CE_File ]


# CE_compare [
#--------------------------------------------------
def CE_compare (aListViewItem, i, col, ascending):
	TraceBeg ()

	theResult	= 0

	if DEBUG_COMPARE:
		dPrint ("CE_compare", "----------")
		dPrint ("CE_compare: aListViewItem", aListViewItem)
		dPrint ("CE_compare: i", i)
		dPrint ("CE_compare: col", col)
		dPrint ("CE_compare: text", aListViewItem.text (col))

	try:
		if col == COL_SIZE:
			if DEBUG_COMPARE:
				dPrint ("CE_compare: aListViewItem.theFileSize", str (aListViewItem.theFileSize))
				dPrint ("CE_compare: i.theFileSize", str (i.theFileSize))
			if aListViewItem.theFileSize > i.theFileSize:
				theResult	= 1
			elif aListViewItem.theFileSize < i.theFileSize:
				theResult	= -1
			else:
				pass

		elif col == COL_LASTWRITETIME:
			if aListViewItem.theFtLastWriteTimeRaw > i.theFtLastWriteTimeRaw:
				theResult	= 1
			elif aListViewItem.theFtLastWriteTimeRaw < i.theFtLastWriteTimeRaw:
				theResult	= -1
			else:
				pass

		else:
			theResult	= QListViewItem.compare (aListViewItem, i, col, ascending)

	except AttributeError:
		# M.B. Thu Apr  1 16:09:17 CEST 2004
		# I think this problem is MacOS X (or my MacOS X-installation-) specific.
		print "Err"
		print "aListViewItem:", aListViewItem
		print "text:         ", aListViewItem.text (0)
		print "col:          ", col
		print "i:            ", i

	TraceEnd ()

	if sys.platform == "darwin":
											# M.B. Don Apr  1 10:38:24 CEST 2004
		return theResult * (-1)				# Don't know why, but at least on MacOS X the sort indicator always
											# shows the wrong direction...
	else:
		return theResult
#--------------------------------------------------
# CE_compare ]

# PathToFolder [
#--------------------------------------------------
def PathToFolder (thePath):
	"""Build the first part of a pathName"""

	TraceBeg ()

	NameList		= QStringList ()
	names			= NameList.split (PATH_SEPEARATOR, thePath, True)
	folderPath		= QString ()
	for name in names [:-1]:
		folderPath.append (name)
		folderPath.append (PATH_SEPEARATOR)

	TraceEnd ()

	return folderPath
#--------------------------------------------------
# PathToFolder ]


# FileItem [
#--------------------------------------------------
class FileItem (QListViewItem):
	def __init__ (self, parent=None, s1=None, s2=None):
		QListViewItem.__init__ (self, parent, s1, s2)
		self.pix					= None
		self.MB_ObjName				= s1
		self.p						= parent
		self.NameLink				= parent
		self.theFileSize			= 0
		self.theFtLastWriteTimeRaw	= 0
		self.thePathName			= ""	# Used for rename
	
	# dump [
	#--------------------------------------------------
	def dump (self):
		TraceBeg ()

		dPrint ("[ Begin of dump for", self)
		dPrint ("MB_ObjName", self.MB_ObjName)
		dPrint ("p", self.p)
		dPrint ("NameLink", self.NameLink)
		dPrint ("] End   of dump for", self)

		TraceEnd ()
	#--------------------------------------------------
	# dump ]

	def setPixmap (self, p):
		#TraceBeg ()
		self.pix	= p
		self.setup ()
		self.widthChanged (0)
		self.invalidateHeight ()
		self.repaint ()
		#TraceEnd ()
	
	def pixmap (self, i):
		#TraceBeg ()
		if i: return None
		else: return self.pix;
		#TraceEnd ()

	# text [
	#--------------------------------------------------
	def text (self, column):
		if column == COL_NAME:
			tempString	= QListViewItem.text (self, column).latin1 ()
			if len (tempString) > 0:
				try:
					return tempString.decode ('utf-8')
				except UnicodeDecodeError:
					return tempString.decode ('latin1').encode ('utf-8').decode ('utf-8')
			else:
				return ""
		if column == COL_SIZE:
			return str (self.theFileSize)
		elif column == COL_LASTWRITETIME:
			return str (self.theFtLastWriteTime)
		else:
			return QListViewItem.text (self, column)
	#--------------------------------------------------
	# text ]

	# compare [
	#--------------------------------------------------
	def compare (self, i, col, ascending):
		return CE_compare (self, i, col, ascending)
	#--------------------------------------------------
	# compare ]

	# okRename [
	#--------------------------------------------------
	def okRename (self, col):
		TraceBeg ()

		if DEBUG_RENAME:
			print "okRename: self:             ", self
			print "type (self.thePathName):    ", type (self.thePathName)
			print "self.thePathName:           ", self.thePathName.latin1 ()
			print "OLD: type (self.text (col)):", type (self.text (col))
			print "OLD: self.text (col):       ", self.text (col)

		OldName		= QString (self.thePathName)
		OldName.append (PATH_SEPEARATOR)
		OldName.append (self.text (col))

		QListViewItem.okRename (self, col)

		NewName		= QString (self.thePathName)
		NewName.append (PATH_SEPEARATOR)
		NewName.append (self.text (col))

		if DEBUG_RENAME:
			print "NEW: type (self.text (col)):", type (self.text (col))
			print "NEW: self.text (col):       ", self.text (col)
			print "type (OldName):             ", type (OldName)
			print "Rename: [%s] -> [%s]" % (OldName.latin1 (), NewName.latin1 ())

		pyrapi.CeMoveFile (str (OldName.utf8 ()), str (NewName.utf8 ()))

		TraceEnd ()
	#--------------------------------------------------
	# okRename ]
#--------------------------------------------------
# FileItem ]


# Directory [
#--------------------------------------------------
class Directory (QListViewItem):
	def __init__ (self, parent=None, filename=None, col2=None):
		QListViewItem.__init__ (self, parent, filename, col2)
		self.pix					= None
		self.theFileSize			= 0
		self.theFtLastWriteTimeRaw	= 0
		global folderLocked, folderClosed, folderOpened, fileNormal
		self.showDirsOnly	= False
		dPrint ("Directory.__init__", "------------------------------")
		dPrint ("Directory.__init__ filename", filename)
		dPrint ("Directory.__init__ parent", parent)
		self.NameLink		= parent
		if filename == None:
			self.MB_ObjName		= "Directory ---None---"
		elif filename == "":
			self.MB_ObjName		= "Directory ---Empty String---"
		else:
			self.MB_ObjName		= filename

		self.thePathName	= QString ()

		if isinstance (parent, QListView):
			self.p	= None
			self.showDirsOnly	= DirectoryView ().showDirsOnly ()
			self.f	= QFile (QString (PATH_SEPEARATOR))
			#self.f	= CE_File (QString (PATH_SEPEARATOR))

		else:
			dPrint ("Directory.__init__ parent.f.name ()", parent.f.name ())
			self.p	= parent
			dPrint ("Directory.__init__ self", self)
			dPrint ("Directory.__init__ self.p", self.p)
			self.showDirsOnly	= parent.showDirsOnly
			self.f	= QFile (QString (filename))
			
			### print filename.decode ('utf-8')

		self.readable	= CE_Dir (self.fullName ()).isReadable ()
		if not self.readable: self.setPixmap (folderLocked)
		else: self.setPixmap (folderClosed)

	# dump [
	#--------------------------------------------------
	def dump (self):
		TraceBeg ()

		dPrint ("[ Begin of dump for", self)
		dPrint ("parent", self.parent)
		dPrint ("theFileSize", self.theFileSize)
		dPrint ("showDirsOnly", self.showDirsOnly)
		dPrint ("NameLink", self.NameLink)
		dPrint ("p", self.p)
		dPrint ("f", self.f)
		dPrint ("readable", self.readable)
		dPrint ("] End   of dump for", self)

		TraceEnd ()
	#--------------------------------------------------
	# dump ]

	def setPixmap (self, px):
		#TraceBeg ()
		self.pix	= px
		self.setup ()
		self.widthChanged (0)
		self.invalidateHeight ()
		self.repaint ()
		#TraceEnd ()
	
	def pixmap (self, i):
		#TraceBeg ()
		#TraceEnd ()
		if i: return None
		else: return self.pix
	
	def setOpen (self, o):
		TraceBeg ()
		if o: self.setPixmap (folderOpened)
		else: self.setPixmap (folderClosed)

		if o and not self.childCount ():
			s	= QString (self.fullName ())
			thisDir	= CE_Dir (s)
			if not thisDir.isReadable ():
				self.readable	= False
				self.setExpandable (False)
				return
			
			# Get FileInfo [
			#--------------------------------------------------
			self.listView ().setUpdatesEnabled (False)

			# Reset ProgressBar [
			#--------------------------------------------------
			theProgressBar.reset ()
			qApp.processEvents ()
			#--------------------------------------------------
			# Reset ProgressBar ]

			files	= thisDir.entryInfoList ()

			if files:
				# Init ProgressBar [
				#--------------------------------------------------
				theProgressBar.setTotalSteps (len (files))
				i	= 0
				theProgressBar.setProgress (0)
				#--------------------------------------------------
				# Init ProgressBar ]
				fi	= CE_FileInfo ()
				for it in files:
					fi	= it
					if DEBUG > 3:
						dPrint ("it", it)
						dPrint ("fi.fileName ()", fi.fileName ())
					if str (fi.fileName ()) == "." or str (fi.fileName ()) == "..":
						continue # nothing
					elif fi.isSymLink () and not self.showDirsOnly:
						item	= FileItem (self, fi.fileName (), "Symbolic Link")
						item.setPixmap (fileNormal)
					elif fi.isDir ():
						Directory (self, fi.fileName ())
					elif not self.showDirsOnly:
						if fi.isFile ():
							item	= FileItem (self, fi.fileName (), "File")
							item.theFileSize			= fi.fileSize ()
							item.theFtLastWriteTime		= fi.ftLastWriteTime ()
							item.theFtLastWriteTimeRaw	= fi.ftLastWriteTimeRaw ()
						else:
							item	= FileItem (self, fi.fileName (), "Special")
						item.setPixmap (fileNormal)

					# Update ProgressBar [
					#--------------------------------------------------
					i	+= 1
					theProgressBar.setProgress (i)
					if sys.platform == 'darwin':
						# Or no progress update is shown...
						qApp.processEvents (1)
					#--------------------------------------------------
					# Update ProgressBar ]

			else:
				# If we have an empty directory, we want to show 100% complete...
				theProgressBar.setProgress (100, 100)

			# Uncomment the following line if you don't want to see always the "100%" indicator.
			#theProgressBar.reset ()

			self.listView ().setUpdatesEnabled (True)
			#--------------------------------------------------
			# Get FileInfo ]

		QListViewItem.setOpen (self, o)
		TraceEnd ()

	def setup (self):
		TraceBeg ()
		self.setExpandable (True)
		QListViewItem.setup (self)
		TraceEnd ()

	def fullName (self):
		TraceBeg ()
		s	= QString ()
		if self.p:
			s	= self.p.fullName ()
			s.append (self.f.name ())
			s.append (PATH_SEPEARATOR)
		else:
			s	= self.f.name ()
		return s
		TraceEnd ()

	def text (self, column):
		TraceBeg ()
		if column == COL_NAME:
# M.B. 2004-04-13
			#return self.f.name ()	# Hope, this is not needed!
			#tempString	= QListViewItem.text (self, column)
			tempString	= QListViewItem.text (self, column).latin1 ()
			if len (tempString) > 0:
				try:
					return tempString.decode ('utf-8')
				except UnicodeDecodeError:
					return tempString.decode ('latin1').encode ('utf-8').decode ('utf-8')
			else:
				return ""
		elif column == COL_TYPE:
			if self.readable:
				return "Directory"
			else:
				return "Unreadable Directory"
		elif column == COL_SIZE:
			return str (self.theFileSize)
		elif column == COL_LASTWRITETIME:
			return "---"
		else:
			return "???"
		TraceEnd ()

	# compare [
	#--------------------------------------------------
	def compare (self, i, col, ascending):
		return CE_compare (self, i, col, ascending)
	#--------------------------------------------------
	# compare ]

	# okRename [
	#--------------------------------------------------
	def okRename (self, col):
		TraceBeg ()

		if DEBUG_RENAME:
			print "okRename: self:         ", self
			print "type (self.thePathName):", type (self.thePathName)
			print "self.thePathName:       ", self.thePathName.latin1 ()
			print "OLD: self.text (col):   ", self.text (col)

		folderPath		= PathToFolder (self.thePathName)

		QListViewItem.okRename (self, col)

		NewName		= folderPath.append (self.text (col))

		if DEBUG_RENAME:
			print "folderPath:             ", folderPath.latin1 ()
			print "NEW: self.text (col):   ", self.text (col)
			print "type (NewName):         ", type (NewName)
			print "NewName:                ", NewName.latin1 ()
			print "Rename: [%s] -> [%s]" % (self.thePathName.latin1 (), NewName.latin1 ())

		pyrapi.CeMoveFile (str (self.thePathName.utf8 ()), str (NewName.utf8 ()))

		TraceEnd ()
	#--------------------------------------------------
	# okRename ]
#--------------------------------------------------
# Directory ]
	

# FileDrag [
#--------------------------------------------------
class FileDrag (QIconDrag):
	def __init__ (self, dragSource = None, name = None):
		QIconDrag.__init__ (self, dragSource, name)
		self.urls	= QStringList ()

	# format [
	#--------------------------------------------------
	def format (self, i):
		TraceBeg ()
		TraceEnd ()
		if i == 0:
			return "text/uri-list"
		else:
			return None
	#--------------------------------------------------
	# format ]

	# encodedData [
	#--------------------------------------------------
	def encodedData (self, mime):
		TraceBeg ()
		a	= ""
		if mime == "text/uri-list":
			a	= self.urls.join ("\r\n")
		#print "mime:", mime
		#print "a:   ", a
		#mime: text/uri-list
		#a:    /tmp/ImageBrowser-Cache/Cache_00000008
		dPrint ("FileDrag.encodedData a", a)
		TraceEnd ()
		return QByteArray (str (a.utf8 ()))
	#--------------------------------------------------
	# encodedData ]

	# canDecode [
	#--------------------------------------------------
	def canDecode (self, e):
		TraceBeg ()
		TraceEnd ()
		return e.provides ("text/uri-list")
	#--------------------------------------------------
	# canDecode ]

	# append [
	#--------------------------------------------------
	def append (self, item, pr, tr, url):
		TraceBeg ()
		TraceEnd ()
		QIconDrag.append (self, item, pr, tr)
		self.urls.append (url)
	#--------------------------------------------------
	# append ]
#--------------------------------------------------
# FileDrag ]


# DirectoryView [
#--------------------------------------------------
class DirectoryView (QListView):
	def __init__ (self, parent=None, name=None, sdo=False):
		QListView.__init__ (self, parent, name)
		self.dirsOnly		= sdo
		self.oldCurrent		= 0
		self.dropItem		= 0
		self.presspos		= QPoint (0,0)
		self.mousePressed	= False
		global folderLocked, folderClosed, folderOpened, fileNormal #= QPixmap ()
		self.autoopenTime	= 750

		dPrint ("----- DirectoryView.__init__ parent", parent)
		dPrint ("----- DirectoryView.__init__ name", name)
		if name == None:
			self.MB_ObjName		= "DirectoryView ---None---"
		elif name == "":
			self.MB_ObjName		= "DirectoryView ---Empty String---"
		else:
			self.MB_ObjName		= name
		self.parent			= parent
		self.NameLink		= parent
		self.theSource		= ""
		self.theSourceItem	= None
		self.theLeafName	= QString ()
	  
		self.autoopen_timer	= QTimer (self)
		if not folderLocked:
			folderLocked	= QPixmap (folder_locked)
			folderClosed	= QPixmap (folder_closed_xpm)
			folderOpened	= QPixmap (folder_open_xpm)
			fileNormal  	= QPixmap (pix_file)
		
		#self.connect (self.button, PYSIGNAL ("sigClicked"),
		#				self.doAppSpecificFunction)
		self.connect (self, SIGNAL ("doubleClicked (QListViewItem *)"),
					  self.slotFolderSelected)
		self.connect (self, SIGNAL ("returnPressed (QListViewItem *)"),
					  self.slotFolderSelected)

		self.connect (self, SIGNAL ("contextMenuRequested (QListViewItem *, const QPoint &, int)"),
					  self.slotRightPressed)

		self.setAcceptDrops (True)
		self.viewport ().setAcceptDrops (True)

		self.connect (self.autoopen_timer, SIGNAL ("timeout ()"), self.openFolder)

		self.setShowSortIndicator (True)

	# dump [
	#--------------------------------------------------
	def dump (self):
		TraceBeg ()

		dPrint ("[ Begin of dump for", self)
		dPrint ("parent", self.parent)
		dPrint ("name", self.name)
		dPrint ("NameLink", self.NameLink)
		dPrint ("] End   of dump for", self)

		TraceEnd ()
	#--------------------------------------------------
	# dump ]

	def showDirsOnly (self):
		TraceBeg ()
		TraceEnd ()
		return self.dirsOnly
	
	def slotFolderSelected (self, i):
		TraceBeg ()
		if not i or not self.showDirsOnly ():
			TraceEnd ()
			return
		dir	= i
		self.emit (PYSIGNAL ("folderSelected"), (1, "dir.fullName ()"))
		TraceEnd ()
			
	def openFolder (self):
		TraceBeg ()
		self.autoopen_timer.stop ()
		if self.dropItem and not self.dropItem.isOpen ():
			self.dropItem.setOpen (True)
			self.dropItem.repaint ()
		TraceEnd ()
		
	def contentsDragEnterEvent (self, e):
		TraceBeg ()
		if not QUriDrag.canDecode (e):
			e.ignore ()
			return
		self.oldCurrent	= self.currentItem ()
		i	= self.itemAt (self.contentsToViewport (e.pos ()))
		if i:
			self.dropItem	= i
			self.autoopen_timer.start (self.autoopenTime)
		TraceEnd ()

	def contentsDragMoveEvent (self, e):
		TraceBeg ()
		if not QUriDrag.canDecode (e):
			e.ignore ()
			return
		vp	= self.contentsToViewport (e.pos ())
		i	= self.itemAt (vp)
# M.B. 2004-04-15
		# We allow drops only on directories
		#if i:
		if i and isinstance (i, Directory):
			self.setSelected (i, True)
			e.accept ()
			if i != self.dropItem:
				self.autoopen_timer.stop ()
				self.dropItem	= i
				self.autoopen_timer.start (self.autoopenTime)

			if e.action () == QDropEvent.Copy:
				pass
			elif e.action () == QDropEvent.Move:
				e.acceptAction ()
			elif e.action () == QDropEvent.Link:
				e.acceptAction ()
			else:
				pass
		else:
			e.ignore ()
			self.autoopen_timer.stop ()
			self.dropItem	= 0
		TraceEnd ()

	def contentsDragLeaveEvent (self, QDragLeaveEvent):
		TraceBeg ()
		self.autoopen_timer.stop ()
		self.dropItem	= 0

# M.B. 2004-03-31 Help by phil
#		setCurrentItem (self.oldCurrent)
#		setSelected (self.oldCurrent, True)
		self.setCurrentItem (self.oldCurrent) 
		self.setSelected (self.oldCurrent, True)
		TraceEnd ()

	# CopyFile [
	#--------------------------------------------------
	def CopyFile (self, source, target, pathSeperator):

		from pyrapi import file

		USE_BUFFER_OBJ	= False

		TraceBeg ()

		dPrint ("CopyFile: type (source)", type (source))
		dPrint ("CopyFile: type (target)", type (target))
		dPrint ("CopyFile: source", source)
		dPrint ("CopyFile: target", target)
		# Local Move:
		#CopyFile: type (source):   <type 'instance'>
		#CopyFile: type (target):   <type 'str'>

		# Split [
		#--------------------------------------------------
		NameList		= QStringList ()
		names			= NameList.split (pathSeperator, source, True)
		# M.B. Die Apr 20 09:17:07 CEST 2004
		# Don't understand the following:
		# If we use the following line we get a "Segmentation fault" after a short while...
		#leafName		= names.last ()
		leafName		= QString (names.last ())
		#--------------------------------------------------
		# Split ]

		dPrint ("CopyFile: type (leafName)", type (leafName))
		dPrint ("CopyFile: leafName", leafName)

		if pathSeperator == "/":
			altPathSeperator	= "\\"
		else:
			altPathSeperator	= "/"

		targetFileName	= QString (target)
		targetFileName.append (altPathSeperator)
		targetFileName.append (leafName)

		dPrint ("CopyFile [%s] [%s] (%s)" % (source.latin1 (), target.latin1 (), leafName.latin1 ()), "")
		dPrint ("type (targetFileName)", type (targetFileName))
		dPrint ("targetFileName", targetFileName)

		if pathSeperator == "/":
			# PC -> PDA [
			#--------------------------------------------------
			theInfile		= open (source.latin1 (), "r")
			theOutFile		= file.openCeFile (str (targetFileName.utf8 ()), "w")
			#--------------------------------------------------
			# PC -> PDA ]

		else:

			# PDA -> PC [
			#--------------------------------------------------
			dPrint ("type (source)", type (source))
			dPrint ("source", source)
			#type (source):             <type 'instance'>
		   
			theInfile		= file.openCeFile (str (source.utf8 ()), "r")
			theOutFile		= open (str (targetFileName.utf8 ()), "w")
			#--------------------------------------------------
			# PDA -> PC ]

		# M.B. Mit Apr  7 19:59:25 CEST 2004
		# Since we have a problem with NULL-Bytes in binary-data if the
		# file is written a string, we use a Python Buffer Object...
		#theOutFile.write (theInfile.read ())

		buf			= theInfile.read ()
		if USE_BUFFER_OBJ:
			theBuf		= buffer (buf)
			result		= theOutFile.write (theBuf)
		else:
			result		= theOutFile.write (buf)

		dPrint ("result", result)
		
		theInfile.close ()
		theOutFile.close ()

		dPrint ("CopyFile (END): type (targetFileName)", type (targetFileName))
		dPrint ("CopyFile (END): targetFileName", targetFileName)
		dPrint ("CopyFile (END): type (leafName)", type (leafName))
		dPrint ("CopyFile (END): leafName", leafName)

		TraceEnd ()

		return (targetFileName, leafName)
	#--------------------------------------------------
	# CopyFile ]
	
	# insertFileInView [
	#--------------------------------------------------
	def insertFileInView (self, anItem, target, targetFileName, leafName, ignoreChildCount):
		TraceBeg ()

		dPrint ("insertFileInView", ">>>>>")
		dPrint ("anItem", anItem)
		dPrint ("anItem.childCount ()", anItem.childCount ())

		dPrint ("target", target)
		dPrint ("type (targetFileName)", type (targetFileName))
		dPrint ("targetFileName", targetFileName)
		dPrint ("type (leafName)", type (leafName))
		dPrint ("leafName", leafName.latin1 ())
		#target:                    \Temp
		#targetFileName:            \Temp\Ducky.jpg
		#leafName:                 Ducky.jpg
		dPrint ("insertFileInView", "<<<<<")

		if (anItem.childCount () > 0) or ignoreChildCount:
			fi				= CE_FileInfo ()

			Handle, Data    = pyrapi.CeFindFirstFile (str (targetFileName.utf8 ()))

			fi.ce_file		= Data
			fi.ce_filePath	= target

			item			= FileItem (anItem, leafName, "File")
			item.theFileSize			= fi.fileSize ()
			item.theFtLastWriteTime		= fi.ftLastWriteTime ()
			item.theFtLastWriteTimeRaw	= fi.ftLastWriteTimeRaw ()
			item.setPixmap (fileNormal)

		TraceEnd ()
	#--------------------------------------------------
	# insertFileInView ]

	def contentsDropEvent (self, e):

		VARIANT			= 2
		
		# MoveFile [
		#--------------------------------------------------
		def MoveFile (source, target, anItem):

			targetFileName, \
			leafName		= self.CopyFile (source, target, "/")

			# Insert new file in view [
			#--------------------------------------------------
			self.insertFileInView (anItem, target, targetFileName, leafName, False)
			#--------------------------------------------------
			# Insert new file in view ]

			#self.triggerUpdate ()

			TraceEnd ()
		#--------------------------------------------------
		# MoveFile ]

		TraceBeg ()
		self.autoopen_timer.stop ()
		if not QUriDrag.canDecode (e):
			e.ignore ()
			return
		item	= self.itemAt (self.contentsToViewport (e.pos ()))
		if item:
# M.B. 2004-03-30
			#lst	= QStrList ()		# Is not implemented in PyQt
										# We have to use a list of python-strings instead...
			dPrint ("e", e)
			theFormat	= e.format ()
			dPrint ("e.format ()", e.format ())
			dPrint ("e.encodedData (theFormat)", e.encodedData (theFormat))

			if VARIANT == 1:
				lst	= []
				Result	= QUriDrag.decode (e, lst)
				dPrint ("Result", Result)
				dPrint ("lst", lst)
				for xString in lst:
					dPrint ("xString", xString)
					#xString: /\Programme\GPRSMonitor
					xString	= xString [1:]	# Drop first char.
			elif VARIANT == 2:
				lst	= QStringList ()
				Result	= QUriDrag.decodeLocalFiles (e, lst)
				dPrint ("Result", Result)
				dPrint ("lst", lst)
				for xString in lst:
					dPrint ("xString", xString)

# M.B. 2004-03-30
			#str	= QString ()		# Python-Bug: "str" isn't really a good python identifier ;-)
			theAction	= ActionNone
			if e.action () == QDropEvent.Copy:
				theStr	= "Copy"
				theAction	= ActionCopy
			elif e.action () ==  QDropEvent.Move:
				theStr	= "Move"
				e.acceptAction ()
				theAction	= ActionMove
			elif e.action () == QDropEvent.Link:
				theStr	= "Link"
				e.acceptAction ()
				theAction	= ActionLink
			else:
				theStr	= "Unknown";
			theStr += "\n\n"

			theActionStr	= QString (theStr)

			e.accept ()
			dPrint ("lst", lst)
# M.B. 2004-03-30
			#for i in range (0, lst.count (), 1):
			filename		= ""
			for i in range (0, len (lst), 1):
# M.B. 2004-03-30
				#filename	= QString (lst.at (i))
				filename	= QString (lst [i])
				#if VARIANT == 2:
				#	filename	= filename [1:]	# Drop first char.
				dPrint ("filename", filename)
				theActionStr.append (filename)
				theActionStr.append ("\n")

			#theStr 		+= str (QString ("\nTo\n\n%1").arg (self.fullPath (item)))
			targetDir	= self.fullPath (item)
			#theStr      += "\nTo\n\n" + targetDir.latin1 ()
			theActionStr.append ("\nTo\n\n")
			theActionStr.append (targetDir)

			if theAction == ActionMove and isinstance (item, Directory): 
				dPrint ("ActionMove: self.theSource", self.theSource)
				dPrint ("ActionMove: item", item)
				dPrint ("ActionMove: type (targetDir)", type (targetDir))
				dPrint ("ActionMove: targetDir", targetDir)
				if filename == "":
					# Local move [
					#--------------------------------------------------
					dPrint ("ActionMove: type (self.theLeafName)", type (self.theLeafName))
					dPrint ("ActionMove: self.theLeafName", self.theLeafName)

					destFile	= targetDir.append (PATH_SEPEARATOR)
					destFile.append (self.theLeafName)

					dPrint ("ActionMove: destFile", destFile)

					#if 1:
					try:
						dPrint ("ActionMove: type (self.theSource)", type (self.theSource))
						pyrapi.CeMoveFile (str (self.theSource.utf8 ()), str (destFile.utf8 ()))

						# Update the view
						dPrint ("ActionMove", ">>>>>")
						dPrint ("self.theSourceItem", self.theSourceItem)
						dPrint ("item", item)
						dPrint ("ActionMove", "<<<<<")
						self.theSourceItem.parent().takeItem (self.theSourceItem)

						# M.B. Mit Apr 14 08:36:21 CEST 2004
						# FIXME We have a problem:
						# - if we use "insertItem" the folder-information is not correctly updated.
						# - if we use "insertFileInView" the items re-appear if something is
						#   changed in the view.
						# - if we use both functions the items are shown twice.
						# I think, every "takeItem" (used above) should have a corresponding "insertItem".
						# So the bottom line is: I've to come back and fix "insertFileInView".

						#self.insertFileInView (item, targetDir, destFile, self.theLeafName, True)
						# M.B. Mit Apr 14 09:39:38 CEST 2004
						# I think "...NameLink = item" is the solution for the above problem.
						item.insertItem (self.theSourceItem)
						self.theSourceItem.NameLink	= item

					#if 0:
					except:
						QMessageBox.warning	(
											self, AppName + " Move File",
											QString	("Could not move\nfile \"%1\"\nto \"%2\".\n"
													"Perhaps a file with this name allready exists?"
													). arg (self.theSource, destFile)
											)
					#--------------------------------------------------
					# Local move ]
				else:
					# Drop from extern [
					#--------------------------------------------------
					MoveFile (filename, targetDir, item)
					#--------------------------------------------------
					# Drop from extern ]
			else:
				if theAction == ActionCopy and isinstance (item, Directory): 
					# Copy a File [
					#--------------------------------------------------
					dPrint ("ActionCopy: item", item)
					dPrint ("ActionCopy: type (self.theSource)", type (self.theSource))
					dPrint ("ActionCopy: self.theSource", self.theSource)
					dPrint ("ActionCopy: type (self.theLeafName)", type (self.theLeafName))
					dPrint ("ActionCopy: self.theLeafName", self.theLeafName)
					dPrint ("ActionCopy: type (targetDir)", type (targetDir))
					dPrint ("ActionCopy: targetDir", targetDir)

					ExistingFile	= self.theSource
					dPrint ("ActionCopy: type (ExistingFile)", type (ExistingFile))
					dPrint ("ActionCopy: ExistingFile", ExistingFile)

					NewFile			= targetDir.append (PATH_SEPEARATOR)
					NewFile.append (self.theLeafName)

					if DEBUG_COPY:
						print "ActionCopy: [%s] -> [%s]" % (ExistingFile.latin1 (), NewFile.latin1 ())

					#if 1:
					try:
						pyrapi.CeCopyFile	(str (ExistingFile.utf8 ()), str (NewFile.utf8 ()), True)

						# Update the view
						self.insertFileInView (item, targetDir, NewFile, self.theLeafName, True)

					#if 0:
					except:
						QMessageBox.warning	(
											self, AppName + " Copy File",
											QString	("Could not copy\nfile \"%1\"\nto \"%2\".\n"
													"Perhaps a file with this name allready exists?"
													).arg (ExistingFile, NewFile)
											)
					#--------------------------------------------------
					# Copy a File ]
				else:
					#QMessageBox.information (self, AppName + " Drop target", theStr, "Not implemented")
					QMessageBox.information	(
											self, AppName + " Drop target",
											QString ("%1 not implemented").arg (theActionStr)
											)
		else:
			e.ignore ()
		TraceEnd ()
   
	# fullPath [
	#--------------------------------------------------
	def fullPath (self, item):

		TraceBeg ()

		fullpath	= QString (item.text (0))
# M.B. 2004-03-30
		# while item == item.parent ():				# BUG: C++ source reads: while ( (item=item->parent()) ) ...
													# Typical '==' / '=' !!!
		#item	= item.parent
		item	= item.NameLink
		while not item == None:
			if DEBUG_OBJECTS:
				dPrint ("----------", "----------")
				dPrint ("item", item)
				dPrint ("item.MB_ObjName", item.MB_ObjName)
				dPrint ("item.parent", item.parent)
			if item.parent:
				if DEBUG_OBJECTS:
					dPrint ("item.parent ()", item.parent ())
				# FIXME Is this good to do via an exception?
				#if 1:
				try:
					if item.parent ():
						fullpath.insert (0, PATH_SEPEARATOR)
						fullpath.insert (0, item.text (0))
					else:
						fullpath.insert (0, item.text (0))
				#if 0:
				except:
					break
			#item	= item.parent
			item	= item.NameLink

		dPrint ("type (fullpath)", type (fullpath))
		dPrint ("... fullpath", fullpath)

		TraceEnd ()

		return fullpath
	#--------------------------------------------------
	# fullPath ]
	
	def contentsMousePressEvent (self, e):
		TraceBeg ()
		QListView.contentsMousePressEvent (self, e)
		p	= QPoint (self.contentsToViewport (e.pos ()))
		i	= self.itemAt (p)
		if i:
			# if the user clicked into the root decoration of the item, don't try to start a drag!
			if self.rootIsDecorated (): isdecorated	= 1
			else: isdecorated	= 0
			if p.x () > self.header ().sectionPos (self.header ().mapToIndex (0)) + self.treeStepSize () *  (i.depth () + isdecorated + self.itemMargin () or
				p.x () < self.header ().sectionPos (self.header ().mapToIndex (0))):
# M.B. 2004-03-30
				#self.presspos	= e.pos ()				# BUG! This makes "presspos" a synomym for "e.pos ()"
				self.presspos.setX (e.pos ().x ())
				self.presspos.setY (e.pos ().y ())
				self.mousePressed	= True
			
		TraceEnd ()

	def contentsMouseMoveEvent (self, e):
		#TraceBeg ()
		offset	= QPoint (self.presspos.x () - e.pos ().x (),
						 self.presspos.y () - e.pos ().y ())

		if DEBUG_POSITIONS:
			dPrint ("self.presspos", "(" + str (self.presspos.x ()) +  ", " + str (self.presspos.y ()) + ")")
			dPrint ("e.pos ()", "(" + str (e.pos ().x ()) +  ", " + str (e.pos ().y ()) + ")")
			dPrint ("offset", "(" + str (offset.x ()) +  ", " + str (offset.y ()) + ")")
			dPrint ("QApplication.startDragDistance ()", QApplication.startDragDistance ())
			dPrint ("(offset).manhattanLength ()", (offset).manhattanLength ())
			dPrint ("self.mousePressed", self.mousePressed)

		if self.mousePressed and (offset).manhattanLength () > QApplication.startDragDistance ():
			self.mousePressed	= False
			item	= self.itemAt (self.contentsToViewport (self.presspos))
			if DEBUG_POSITIONS:
				dPrint ("item", item)
			if item:
				dPrint ("+++ self", self)
				dPrint ("+++ self.MB_ObjName", "[%s]" % self.MB_ObjName)
				self.theSource	= self.fullPath (item)
				dPrint ("+++ self.theSource", self.theSource)
				self.theSourceItem	= item
				if True:
				#if QFile.exists (self.theSource):
				#if CE_File.exists (self.theSource):

					# Copy PDA-File to SynCE-temporary-Directory [
					#--------------------------------------------------
					target			= QString (TempDirectory)
					dPrint ("+++ target", target)
					targetFileName, \
					self.theLeafName	= self.CopyFile (self.theSource, target, PATH_SEPEARATOR)

					dPrint ("PDA-2-tmp: type (self.theLeafName)", type (self.theLeafName))
					dPrint ("PDA-2-tmp: self.theLeafName", self.theLeafName)
					#--------------------------------------------------
					# Copy PDA-File to SynCE-temporary-Directory ]

					if sys.platform != "darwin":
						ud	= QUriDrag (self.viewport ())
						x	= QStringList ()
						x.append (targetFileName)
						
						#theUri	=  ud.localFileToUri (targetFileName)
						#dPrint ("theUri", theUri)
						#x.append (str (theUri))

						ud.setUnicodeUris (x)
					else:
						# WorkAround for our MacOS X FileDrag-problem. [
						#--------------------------------------------------
						ud	= FileDrag (self.viewport())
						id	= QIconDragItem ()
						dPrint ("+++ type (targetFileName)", type (targetFileName))
						dPrint ("+++ targetFileName", targetFileName)
						#+++ type (targetFileName):  <class '__main__.qt.QString'>
						#print dir (targetFileName)
						#id.setData (targetFileName)
						id.setData (targetFileName.utf8 ())
						ud.append   (
									id,
									QRect (0, 0, 10, 20),
									QRect (0, 0, 10, 20),
									targetFileName
									)
						#--------------------------------------------------
						# WorkAround for our MacOS X FileDrag-problem. ]

					if ud.drag ():
						QMessageBox.information (
												self, AppName + " Drag source",
												QString	("Delete \"%1\" not implemented").arg (self.theSource)
												)

					#if sys.platform != "darwin":
					#	os.remove (targetFileName)	# Remove PDA-File from SynCE-working-Directory

		#TraceEnd ()

	def contentsMouseReleaseEvent (self, QMouseEvent):
		TraceBeg ()
		self.mousePressed	= False
		TraceEnd ()

	def setDir (self, s):
		TraceBeg ()
		it	= QListViewItem (self)
		it.setOpen (False)
		thisDir	= CE_Dir (s)
		it	= QListViewItem (self)
		it.setOpen (False)
		lst	= QStringList (QStringList.split (PATH_SEPEARATOR, s))
		listview2	= []
		listview2.extend (lst)
		
		item	= self.firstChild ()
		for it2 in listview2:
			while item:
				if item.text (0) == it2:
					item.setOpen (True)
					break
				item	= item.itemBelow ()
		if item:
			self.setCurrentItem (item)
		TraceEnd ()

	# ShowTextFile [
	#--------------------------------------------------
	def ShowTextFile (self, aFileName):

		from pyrapi import file

		TraceBeg ()

		dPrint ("aFileName", aFileName)
		theFile			= file.openCeFile (str (aFileName.utf8 ()), "r")
		buf				= theFile.read ()
		theFile.close ()

		#a				= unicode (buf, "unicodelittleunmarked")
		#ascii			= a.encode ("ascii", "ignore")
		ascii			= ""

		if len (ascii) > 0:
			theText			= ascii
		else:
			theText			= buf

		TraceEnd ()

		return theText
	#--------------------------------------------------
	# ShowTextFile ]

	# ShowTextWindow [
	#--------------------------------------------------
	def ShowTextWindow (self, aTitle, aText):
		TraceBeg ()

		aDialog				= QDialog (None)
		aGrid				= QGridLayout (aDialog, 1, 1)
		aTextView			= QTextEdit (aDialog)
		aDialog.setCaption (aTitle)
		aTextView.setTextFormat (Qt.PlainText)
		aTextView.setReadOnly (True)

		# Make font fixed [
		#--------------------------------------------------
		theFont				= aTextView.font ()
		if sys.platform == 'darwin':
			theFont.setFamily("Monaco")
			theFont.setPointSize (12)
		else:
			theFont.setFamily("fixed")
			theFont.setPointSize (13)
		aTextView.setFont (theFont)
		#--------------------------------------------------
		# Make font fixed ]

		aTextView.setText (aText)

		aGrid.addWidget (aTextView, 1, 1)
		aDialog.resize (600, 200)
		aDialog.show ()
		aDialog.exec_loop ()

		TraceEnd ()
	#--------------------------------------------------
	# ShowTextWindow ]

	# ShowPictFile [
	#--------------------------------------------------
	def ShowPictFile (self, aFileName):

		from pyrapi import file

		TraceBeg ()

		theFile			= file.openCeFile (str (aFileName.utf8 ()), "r")
		thePict			= theFile.read ()
		theFile.close ()

		return thePict
	#--------------------------------------------------
	# ShowPictFile ]

	# ShowPictWindow [
	#--------------------------------------------------
	def ShowPictWindow (self, aTitle, aPict):
		TraceBeg ()

		aPixmap				= QPixmap ()
		if aPixmap.loadFromData (aPict):
			aDialog				= QDialog (None)
			aGrid				= QGridLayout (aDialog, 1, 1)
			aPictView			= QLabel (aDialog)
			aDialog.setCaption (aTitle)

			aPictView.setPixmap (aPixmap)

			aGrid.addWidget (aPictView, 1, 1)
			aDialog.show ()
			aDialog.exec_loop ()

		TraceEnd ()
	#--------------------------------------------------
	# ShowPictWindow ]

	# DeleteFile [
	#--------------------------------------------------
	def DeleteFile (self):
		TraceBeg ()

		#print "in DeleteFile"

		TraceEnd ()
	#--------------------------------------------------
	# DeleteFile ]

	# LargeView [
	#--------------------------------------------------
	def LargeView (self):
		TraceBeg ()

		print "in LargeView"

		TraceEnd ()
	#--------------------------------------------------
	# LargeView ]

	# SmallView [
	#--------------------------------------------------
	def SmallView (self):
		TraceBeg ()

		print "in SmallView"

		TraceEnd ()
	#--------------------------------------------------
	# SmallView ]

	# slotRightPressed [
	#--------------------------------------------------
	def slotRightPressed (self, anItem, aPos, aCol):
		TraceBeg ()

		dPrint ("slotRightPressed", "")
		dPrint ("anItem", anItem)
		dPrint ("aPos", aPos)
		dPrint ("aCol", aCol)

		if anItem:			# On item
			aMenu				= QPopupMenu (self)

			ShowTextFile_ITEM	= -1
			ShowPictFile_ITEM	= -1
			DeleteFile_ITEM		= -1
			CreateNewDir_ITEM	= -1
			DeleteDir_ITEM		= -1

			ShowInfo_ITEM		= aMenu.insertItem (qApp.translate ("qApp", "Show Object Info"))
			Rename_ITEM			= aMenu.insertItem (qApp.translate ("qApp", "Rename Item"))
			if isinstance (anItem, FileItem): 
				ShowTextFile_ITEM	= aMenu.insertItem (qApp.translate ("qApp", "Show File as Text"))
				ShowPictFile_ITEM	= aMenu.insertItem (qApp.translate ("qApp", "Show File as Picture"))
				DeleteFile_ITEM		= aMenu.insertItem (qApp.translate ("qApp", "Delete File"), self.DeleteFile)
			elif isinstance (anItem, Directory): 
				CreateNewDir_ITEM	= aMenu.insertItem (qApp.translate ("qApp", "Create new Folder"))
				DeleteDir_ITEM		= aMenu.insertItem (qApp.translate ("qApp", "Delete Folder"))

			dPrint ("ShowInfo_ITEM", ShowInfo_ITEM)

			aMenu.setMouseTracking (True)
			id					= aMenu.exec_loop (aPos)
			self.mousePressed	= False	# Don't fall to dragging!

			theFullPath			= self.fullPath (anItem)	# Because it's used so often here...

			if id == ShowInfo_ITEM:
				# itemInfo [
				#--------------------------------------------------
				theInfo				= QString ()
				theInfo.append (str (anItem) + "\n")
				theInfo.append ("Name:                 ")
				theInfo.append (anItem.text (0))
				theInfo.append ("\n")
				theInfo.append ("fullPath:             ")
				theInfo.append (theFullPath)
				theInfo.append ("\n")
				theInfo.append ("anItem.thePathName:   ")
				theInfo.append (anItem.thePathName)
				theInfo.append ("\n")

				if isinstance (anItem, FileItem): 
					theInfo.append ("File" + "\n")
					theInfo.append ("anItem.p.thePathName: ")
					theInfo.append (anItem.p.thePathName)

				if isinstance (anItem, Directory): 
					theInfo.append ("Directory" + "\n")
					theInfo.append ("childCount (): " + str (anItem.childCount ()) + "\n")

				self.ShowTextWindow (anItem.text (0), theInfo)
				#--------------------------------------------------
				# itemInfo ]

			elif id == Rename_ITEM:
				anItem.setRenameEnabled (COL_NAME, True)
				if isinstance (anItem, FileItem):
					folderPath			= PathToFolder (theFullPath)
					dPrint ("folderPath", folderPath)
					# Move last char to front [
					#--------------------------------------------------
					#folderPath:                Temp\
					#anItem.thePathName:        \Temp
					folderPath.insert (0, folderPath.right (1))
					folderPath.remove (len (folderPath) - 1, 1)
					#--------------------------------------------------
					# Move last char to front ]
				else:
					folderPath			= QString (theFullPath)
				anItem.thePathName	= folderPath
				anItem.startRename (COL_NAME)

			elif id == ShowTextFile_ITEM:
				theText		= self.ShowTextFile (theFullPath)
				self.ShowTextWindow (anItem.text (0), theText)

			elif id == ShowPictFile_ITEM:
				thePict		= self.ShowPictFile (theFullPath)
				self.ShowPictWindow (anItem.text (0), thePict)

			elif id == DeleteFile_ITEM:
				answer		= QMessageBox.question	(
													self, AppName + " Delete File",
													#"Do you really want to delete the file \"" + theFullPath + "\"?\n",
													QString ("Do you really want to delete the file \"%1\"?\n").arg (theFullPath),
													QMessageBox.No,
													QMessageBox.Yes,
													)
				if answer == QMessageBox.Yes:
					try:
						pyrapi.CeDeleteFile (str (theFullPath.utf8 ()))
						anItem.parent().takeItem (anItem)
					except:
						QMessageBox.warning	(
											self, AppName + " Delete File",
											QString	("Could not delete the file \"%1\".\n"
													"Perhaps bad characters in fileName?"
													).arg (theFullPath)
											)

			elif id == CreateNewDir_ITEM:
				NewFolderName	= "New Folder"
				try:
					aFolderName		= QString (theFullPath)
					aFolderName.append (PATH_SEPEARATOR + NewFolderName)
					result			= pyrapi.CeCreateDirectory (str (aFolderName.utf8 ()))
					Directory (anItem, NewFolderName)
				except:
					QMessageBox.warning	(
										self, AppName + " Create new Folder",
										#"Could not create new folder \"" + NewFolderName + "\".\n"
										QString	("Could not create new folder \"%1\".\n"
												"Perhaps a folder with this name allready exists?"
												).arg (NewFolderName)
										)

			elif id == DeleteDir_ITEM:
				answer		= QMessageBox.question	(
													self, AppName + " Delete Folder",
													#"Do you really want to delete the folder \"" + theFullPath + "\"?\n",
													QString ("Do you really want to delete the folder \"%1\"?\n").arg (theFullPath),
													QMessageBox.No,
													QMessageBox.Yes,
													)
				if answer == QMessageBox.Yes:
					try:
						result	= pyrapi.CeRemoveDirectory (str (theFullPath.utf8 ()))
						anItem.parent().takeItem (anItem)
					except:
						QMessageBox.warning	(
											self, AppName + " Delete Folder",
											#"Could not delete folder \"" + theFullPath + "\".\n"
											QString	("Could not delete folder \"%1\".\n"
													"Perhaps the folder is not empty?"
													).arg (theFullPath)
											)

		else:				# On view
			aMenu				= QPopupMenu (self)
			aMenu.insertItem (qApp.translate ("qApp", "Large View"), self.LargeView)
			aMenu.insertItem (qApp.translate ("qApp", "Small View"), self.SmallView)
			aMenu.setMouseTracking (True)
			id					= aMenu.exec_loop (aPos)

		dPrint ("id", id)

		TraceEnd ()
	#--------------------------------------------------
	# slotRightPressed ]
#--------------------------------------------------
# DirectoryView ]


# main [
#--------------------------------------------------
def main (theArgs):

	global DEBUG
	global DEBUG_PRINT
	global DEBUG_RENAME			# Preferences
	global DEBUG_COPY			# Preferences
	global dPrint1
	global TempDirectory
	global theProgressBar

	#--------------------------------------------------
	# MenuCmds [
	#--------------------------------------------------

	# about [
	#--------------------------------------------------
	def about ():
		TraceBeg ()

		QMessageBox.about (theMainWindow, AppName,
							#self.tr ("Show thumbnails of images.\n"
							unicode (qApp.translate ("AboutDialog", "Add-on for SynCE.\n"
							"Show PocketPC FileSystem.\n"
							"\nAuthor: "))
							+ __author__  + "\n"
							+ __version__.replace ("$", "") + "\n"
							+ __date__.replace ("$", "") + "\n"
							+ "QT-Version: " + QT_VERSION_STR
							# "__FileVersion__" is only needed for developing...
							+ "\n" + __FileVersion__
							)

		TraceEnd ()
	#--------------------------------------------------
	# about ]

	# DoPreferencesDialog [
	#--------------------------------------------------
	def DoPreferencesDialog ():

		import PreferencesDialog

		global DEBUG_RENAME
		global DEBUG_COPY

		TraceBeg ()

		PreferencesDialog	= PreferencesDialog.Preferences (modal=True)
		if Qtopia:
			PreferencesDialog.showMaximized ()

		# Fill Values [
		#--------------------------------------------------
		PreferencesDialog.checkBoxDebugRename.setChecked (DEBUG_RENAME)
		PreferencesDialog.checkBoxDebugCopy.setChecked (DEBUG_COPY)
		#--------------------------------------------------
		# Fill Values ]

		# Show the Dialog
		#--------------------------------------------------
		ReturnValue	= PreferencesDialog.exec_loop ()
		#--------------------------------------------------
		# Show the Dialog

		if ReturnValue:
			DEBUG_RENAME		= PreferencesDialog.checkBoxDebugRename.isChecked ()
			DEBUG_COPY			= PreferencesDialog.checkBoxDebugCopy.isChecked ()

		TraceEnd ()
	#--------------------------------------------------
	# DoPreferencesDialog ]

	# exit [
	#--------------------------------------------------
	def exit (self):
		theMainWindow.close ()
		qApp.quit ()
	#--------------------------------------------------
	# exit ]
	#--------------------------------------------------
	# MenuCmds ]

	TraceBeg ()

	# Initialize DebugPrint [
	#--------------------------------------------------
	debugPrint		= DebugPrint.DebugPrint ()
	debugPrint.doPrint= DEBUG_PRINT
	dPrint1			= debugPrint.LabelValue	# Name-change!
	#--------------------------------------------------
	# Initialize DebugPrint ]

	# Test
	dPrint ("sys.argv", sys.argv)
	debugPrint.String ("PATH_SEPEARATOR: [%s]" % PATH_SEPEARATOR)

	# Read Settings [
	#--------------------------------------------------
	try:
		SettingsFile    = open (SETTINGS_NAME, "r")
		Settings		= SettingsFile.readlines ()
		SettingsFile.close ()
		# ['host=linuxorange\n', 'port=4711\n']
		dPrint ("Settings", Settings)

		for Line in Settings:
			 Line	= Line [:-1]	# Kill trailing "\n"
			 Params	= Line.split ("=")
			 if Params [0] == "DEBUG_RENAME":
			 	DEBUG_RENAME	= Params [1] == "True"
				dPrint ("DEBUG_RENAME", DEBUG_RENAME)
			 elif Params [0] == "DEBUG_COPY":
			 	DEBUG_COPY	= Params [1] == "True"
				dPrint ("DEBUG_COPY", DEBUG_COPY)
	except:
		print "%s Warning: Could not read settings from [%s]." % (sys.argv [0], SETTINGS_NAME)
	#--------------------------------------------------
	# Read Settings ]

	# Process Options [
	#--------------------------------------------------
	Usage		= \
    """usage: %s [-d n] [-D] [-V]
    -d n ...... debug-level 
                0 = off
                1 = general
                2 = trace
                3 = Debug-File
    -D ........ DebugPrint
    -V ........ print version and exit"""

	try:
		opts, args	= getopt.getopt (theArgs [1:], 'd:DV')
	except getopt.GetoptError:
		print Usage % sys.argv [0]
		sys.exit (1)

	if DEBUG:
		print opts, args

	DEBUG_PRINT		= False

	for o, a in opts:
		if o in ("-d", "--debugLevel"):
			try:
				DEBUG	= int (a)
			except ValueError:
				print "debugLevel needs and integer"
				print Usage % sys.argv [0]
				sys.exit (1)
		if o in ("-D", "--debugPrint"):
			DEBUG_PRINT	= True
		if o in ("-V", "--version"):
			print sys.argv [0] \
				+ " " + __version__.replace ("$", "") \
				+ __date__.replace ("$", "")
			sys.exit (0)
	#--------------------------------------------------
	# Process Options ]

	debugPrint.doPrint= DEBUG_PRINT

	App	= QApplication (theArgs)
	
	# Create MenuBar [
	#--------------------------------------------------
	theMainWindow		= QMainWindow ()

	fileMenu			= QPopupMenu (theMainWindow)
	fileMenu.insertItem (qApp.translate ("qApp", "&Exit"), exit, Qt.CTRL+Qt.Key_Q)

	optionsMenu    		= QPopupMenu (theMainWindow)
	optionsMenu.insertItem (qApp.translate ("qApp", "Preferences..."), DoPreferencesDialog)

	helpMenu       		= QPopupMenu (theMainWindow)
	helpMenu.insertItem (qApp.translate ("qApp", "&About"), about, Qt.CTRL+Qt.Key_H)

	if Qtopia:
		theMenuBar			= QPEMenuBar (theMainWindow)
	else:
		theMenuBar			= QMenuBar (theMainWindow)

	theMenuBar.insertItem (qApp.translate ("qApp", "&File"),    fileMenu)
	theMenuBar.insertItem (qApp.translate ("qApp", "&Options"), optionsMenu)
	theMenuBar.insertItem (qApp.translate ("qApp", "&About"),   helpMenu)

	qApp.connect(qApp, SIGNAL('lastWindowClosed()'), qApp, SLOT('quit()'))

	# StatusBar [
	#--------------------------------------------------
	if prefStatusWant:
		theStatusBar		= QStatusBar (theMainWindow)
		theStatusBar.message ((__version__ + " " + __date__).replace ("$", ""))
		if Qtopia:
			# IMPORTANT!!!  Mit Okt  8 17:15:34 CEST 2003
			theStatusBar.setMinimumHeight (STATUS_BAR_H)
	#--------------------------------------------------
	# StatusBar ]

	#--------------------------------------------------
	# Create MenuBar ]

	# Create DirectoryView [
	#--------------------------------------------------
	theMainView	= DirectoryView (theMainWindow)
	theMainView.addColumn ("Name")
	theMainView.addColumn ("Type")
	theMainView.addColumn ("Size")
	theMainView.addColumn ("LastWriteTime")
	theMainView.setTreeStepSize (20)
	#--------------------------------------------------
	# Create DirectoryView ]
	
	# Initialize ProgressBar [
	#--------------------------------------------------
	theProgressBar  = QProgressBar (0, None, "Reading Directory")
	theStatusBar.addWidget (theProgressBar, 0, True)
	#--------------------------------------------------
	# Initialize ProgressBar ]
	
	try:
		pyrapi.synce_log_set_level (pyrapi.SYNCE_LOG_LEVEL_LOWEST)
	except:
		print "%s Problem in contacting SynCE." % sys.argv [0]
		print "%s Perhaps server not found." % sys.argv [0]

	#roots	= CE_Dir.drives ()
	theRoot	= CE_Dir ()
	roots	= theRoot.drives ()
	if roots:
		for it in roots:
			fi = it
			dPrint ("it", it)
			dPrint ("fi.filePath ()", fi.filePath ())
			root = Directory (theMainView, fi.filePath ())
			if roots.count (it) <= 1:
				try:
					root.setOpen (True)  # be interesting
				except:
					print "%s Could not connect to PDA." % sys.argv [0]
	
	theMainView.setAllColumnsShowFocus (True)

	theMainWindow.resize (400, 400)
	theMainWindow.setCaption (theCaption)

	#App.setMainWidget (theMainWindow)
	#theMainWindow.show ()

	theMainWindow.setCentralWidget (theMainView)
	if Qtopia:
		theMainWindow.showMaximized ()
	else:
		theMainWindow.show ()
	
	# Initialize Temp Directory [
	#--------------------------------------------------
	TempDirectoryName	= AppName + "-Temp"
	TempDirectory		= "/tmp/" + TempDirectoryName + "-" + os.getenv ("USER")
	#print "TempDirectory", TempDirectory
	TempDir				= QDir (TempDirectory)
	if not TempDir.exists ():
		print "mkdir TempDirectory:", TempDirectory
		DirOK				= TempDir.mkdir (TempDirectory)
		if not DirOK:
			print "Couldn't create TempDirectory [%s]." % TempDirectory
	#--------------------------------------------------
	# Initialize Temp Directory ]

	App.exec_loop ()

	# Write Settings [
	#--------------------------------------------------
	try:
		SettingsFile    = open (SETTINGS_NAME, "w")
		Settings		= [
						"DEBUG_RENAME=", str (DEBUG_RENAME), "\n", 
						"DEBUG_COPY=", str (DEBUG_COPY), "\n", 
						]
		SettingsFile.writelines (Settings)
		SettingsFile.close ()
	except:
		print "%s Warning: Could not save settings in [%s]." % (sys.argv [0], SETTINGS_NAME)
	#--------------------------------------------------
	# Write Settings ]
	TraceEnd ()
#--------------------------------------------------
# main ]

if __name__ == "__main__":
	main (sys.argv)

# vim: ts=4 sw=4
