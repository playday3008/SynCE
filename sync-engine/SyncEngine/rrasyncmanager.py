# -*- coding: utf-8 -*-
############################################################################
# RRASyncMgr.py
#
# RRA synchronization manager. Currently supports file types, but extensible
#
#
# Copyright (C) 2007 Dr J A Gow 
#
############################################################################

import pyrra
import threading
import logging
import gobject
import time
import select
import os
import os.path
import struct
import pshipmgr
import util
from errors import *
import cPickle as pickle
from constants import *
import config

EVENT_TYPES = {
	pyrra.SYNCMGR_TYPE_EVENT_UNCHANGED : "Unchanged",
	pyrra.SYNCMGR_TYPE_EVENT_CHANGED   : "Changed",
	pyrra.SYNCMGR_TYPE_EVENT_DELETED   : "Deleted"
}

LOCAL_STATE_NEW=0
LOCAL_STATE_UNCHANGED=1
LOCAL_STATE_MODIFIED=2
LOCAL_STATE_DELETED=3

LOCAL_STATE_TYPES = {
	LOCAL_STATE_NEW : "New",
	LOCAL_STATE_UNCHANGED : "Unchanged",
	LOCAL_STATE_MODIFIED : "Modified",
	LOCAL_STATE_DELETED : "Deleted"
}

###############################################################################
# FileDBEntry
# 
# Local file processing database entry
#
###############################################################################

class FileDBEntry:
	def __init__(self,name,mtime,localstate,isDir):
		self.name = name
		self.mtime = mtime
		self.localstate = localstate
		self.isDir = isDir
		self.scanFlag = 0
		self.Reported = False
		self.DeleteCount = 0
		
	#
	# Dump a database entry
	#
		
	def dump(self):
		print "scanflag %d, name %s, mtime %d, localstate %s" % (self.scanFlag,\
		                self.name,self.mtime,LOCAL_STATE_TYPES[self.localstate])
		
###############################################################################
# FileDB
# 
# The local file change database
#
###############################################################################

class FileDB:
	
	def __init__(self, confpath, syncdir, pship, extra_deletedelay):
		
		#
		# The db contains existing objects that have been assigned
		# an oid. The list 'dbnew' is for files/dirs on the local side
		# that have not yet been processed. 'dbdel' contains objects
		# scheduled for deletion.
		
		self.db = {}
		self.dbdel = {}
		self.dbnew = []
	        self.logger = logging.getLogger("engine.rrasyncmanager.FileDB")
		self.config_dir = pship.pshippath
        	self.config_path = pship.idbpath + "-FILE"
		self.syncdir = syncdir
		self.ExtraDeleteDelay = extra_deletedelay

	# Load the database file

	def LoadDB(self):
		f = None
        	try:
            		f = open(self.config_path, "rb")
        	except:
            		return

        	try:
            		self.db = pickle.load(f)
			self.dbdel = pickle.load(f)
			f.close()
			for fe in self.db.keys():
				self.db[fe].scanFlag = False
				self.db[fe].Reported = False
				
        	except Exception, e:
            		self.logger.warning("LoadDB: Failed to load local file DB: %s", e)
            	return
	
	# Save the database file
	
	def SaveDB(self):
		
        	try:
			f = open(self.config_path, "wb")
			pickle.dump(self.db, f, pickle.HIGHEST_PROTOCOL)
			pickle.dump(self.dbdel, f, pickle.HIGHEST_PROTOCOL)
			f.close()
		except Exception, e:
			self.logger.warning("SaveDB: Failed to save local file DB: %s", e)
	
		
	# Update the database file from the local sync directory
	
	def UpdateDB(self):
		
		self.logger.info("rescanning local files")
		
		for syncpath,dirs,files in os.walk(self.syncdir):
			
			dirtime = os.path.getmtime(syncpath);
			self._ScanItemDB(self.syncdir,syncpath,dirs,True)
			self._ScanItemDB(self.syncdir,syncpath,files,False)
			
		for fe in self.db.keys():
			if self.db[fe].scanFlag == 0:
				self.logger.debug("marking oid %d for remote deletion" % fe)
				self.db[fe].localstate = LOCAL_STATE_DELETED
			self.db[fe].scanFlag = 0
			
		# save our new snapshot of the database
		
		self.SaveDB()
				
	#
	# Internal: check if a named file is in the db and return the key
	#	
	
	def _NameInDB(self,name):
		
		for fe in self.db.keys():
			if self.db[fe].name == name:
				return fe
		return None
		
	#
	# Internal: scan a path and item and update the DB entry
	#
	
	def _ScanItemDB(self,prefix,syncpath,items,isDir):
		
		for item in items:
			tfpath = os.path.join(syncpath,item)
			mtime = os.path.getmtime(tfpath)
			tfpath = tfpath[len(prefix)+1:]
			oid = self._NameInDB(tfpath)
			if oid == None:
				self.dbnew.append(FileDBEntry(tfpath,mtime,LOCAL_STATE_NEW,isDir))
			else:
				self.db[oid].scanFlag = 1
				
				# skip the mtime checks on dirs - otherwise we will get all sorts
				# of grief as the dir mtime is updated when the contents are.
				
				if mtime != self.db[oid].mtime and not isDir:
					self.logger.debug("mtime changed on %s" % self.db[oid].name)
					self.db[oid].mtime = mtime
					self.db[oid].localstate = LOCAL_STATE_MODIFIED
				else:
					self.db[oid].localstate = LOCAL_STATE_UNCHANGED

	#
	# Schedule an item for deletion. All items are delayed
	#
	
	def ScheduleForDeletion(self,oid):
		if self.db.has_key(oid):
			self.dbdel[oid] = self.db[oid]
			del self.db[oid]
			self.dbdel[oid].DeleteCount = 0

	#
	# Delete all local items in delete queue 
	#
	
	def DeleteScheduledItems(self):
		for oid in self.dbdel.keys():
			if self.dbdel[oid].DeleteCount >= 3+self.ExtraDeleteDelay:
				self.logger.debug("deleting oid %d file %s" % (oid,self.dbdel[oid].name))
				self.DeleteLocalItem(oid)
			else:
				self.dbdel[oid].DeleteCount += 1

	#
	# Scan the delete queue for objects with the same name that have been recently
	# fetched under a different OID

	def ClearDeleteByPath(self,path):
		for oid in self.dbdel.keys():
			if self.dbdel[oid].name == path:
				del self.dbdel[oid]
				break

	# 
	# RequeueOrCreateDir handles the brain-damaged delete-rename sequence of RRAC.
	# If we have the dir, we rename it. If it has been marked for deletion, we 
	# unmark it and rename. If we do not have it, we create it.
	
	def RequeueOrCreateDir(self,objID,path):
		
		fullpath = os.path.join(self.syncdir,path)
		
		if self.db.has_key(objID):
			
			# check primary list
			
			oldpath = os.path.join(self.syncdir,self.db[objID].name)
			try:
				os.rename(oldpath,fullpath)
			except:
				self.logger.debug("WARNING: unable to rename directory")
					
			self.db[objID].name = path
				
		elif self.dbdel.has_key(objID):
			
			# check deleted list
			
			oldpath = os.path.join(self.syncdir,self.dbdel[objID].name)
			try:
				os.rename(oldpath,fullpath)
			except:
				self.logger.debug("WARNING: unable to rename directory")
					
			self.dbdel[objID].name = path
			self.db[objID] = self.dbdel[objID]
			del self.dbdel[objID]

		else:
			# it's a new one
			
			if not os.path.isdir(fullpath):
				try:
					os.makedirs(fullpath)
				except:
					self.logger.debug("WARNING: Unable to create directory")

	#
	# Delete file by OID and remove from database
	#

	def DeleteLocalItem(self,oid):
		if self.dbdel.has_key(oid):
			if self.dbdel[oid].localstate != LOCAL_STATE_DELETED:
				pth = os.path.join(self.syncdir,self.dbdel[oid].name)
				
				if self.dbdel[oid].isDir:
					self.logger.debug("running deltree(%s)" % pth)
					util.deltree(pth)
				else:
					try:
						os.remove(pth)
					except:
						self.logger.debug("unable to delete %s, possibly already removed" % pth)
			del self.dbdel[oid]
			return True
		return False

	#
	# Requeue - take an item out of the DB and requeue it as a new item
	#
	
	def Requeue(self, oid):
		if self.db.has_key(oid):
			entry = self.db[oid]
			entry.localstate = LOCAL_STATE_NEW
			entry.DeleteCount = 0
			self.dbnew.append(entry)
			del self.db[oid]

	# Dump the database for debugging
	
	def DumpDB(self):
		
		for fe in self.db.keys():
			print "File %s :" % fe
			self.db[fe].dump()

###############################################################################
# Processor
#
# Generic processor object. Inherit from this to generate a specific RRA
# item processor.
#
###############################################################################

class Processor:
	
	def __init__(self, typeID, pship, thread):
		self.enabled  = True
		self.type_id  = typeID
		self.pship    = pship
		self.thread   = thread
		
		# Lists of event OIDs passed out from handheld. These
		# are updated by the 
		
		self.new = []
		self.unchanged = []
		self.modified = []
		self.deleted = []

		# Lists of OIDs for subsequent handheld processing
		# These are used internally and should not be tweaked

		self.OldOIDs = []
		self.ToSend = []
		self.ToFetch = []
		self.ToDelete = []

	# Processor main function

	def Process(self):
		return

	# object writer callback

	def CB_Writer(self, objID, data):
		return FALSE
		
	# object reader callback
		
	def CB_Reader(self, index, maxlen):
		return []


###############################################################################
# FileProcessor
#
# File processor object
#
###############################################################################

class FileProcessor(Processor):
	
	def __init__(self,type_id,pship,thread):
		Processor.__init__(self,type_id,pship,thread)

		# now read the config
				
		self.CfgItem = thread.config.config_FileSync
		
		path = os.path.join(os.path.expanduser(self.CfgItem.cfg["BaseFilePath"]),
		                    os.path.expanduser(pship.QueryConfig("/syncpartner-config/General/LocalFilePath[position() = 1]","0")))
		
		self.fdb = FileDB(thread.config.config_user_dir,\
		                  path,\
				  pship,\
				  self.CfgItem.cfg["ExtraDeleteDelay"])
		self.UpdateDB = True
		self.SaveDB = False
		self.readeroids = []

		self.logger = logging.getLogger("engine.rrasyncmanager.FileProcessor")

		# Load our initial database

		self.fdb.LoadDB()

		self.rdrdata = None
		
		self.dbUpdateCount = 0
		self.DBTouched = False
		self.initial = True
		self.initrepdelay = 0
		self.reporttimeout = self.CfgItem.cfg["ObjectReportTimeout"]
		
	# Signal a database update for the next pass.
	
	def SignalDBUpdate(self):
		UpdateDB = True

	#
	# Process the items. We should have all of them, including folder renames
	# (delete followed by change)
	#
	# Returns if interrupted 
	#
		
	def Process(self):
		
		self.DBTouched = False
		
		if self.CfgItem.cfg["Disable"] or self.thread.stopping:
			return
		
		#
		# Update the local file DB if we need to, at appropriate intervals
		
		self.dbUpdateCount += 1
		if self.dbUpdateCount >= 10 + self.CfgItem.cfg["LocalUpdateFreq"]:
			self.UpdateDB = True
			self.dbUpdateCount = 0
			
		if self.UpdateDB:
			self.fdb.UpdateDB()
			self.UpdateDB = False
			
		if self.thread.stopping:
			return
			
		#
		# First, handle remote deletions.
		
		# these are OIDs that have been deleted from the handheld. We must check
		# to see if they are present on the desktop
		#
		# If desktop = UNCHANGED delete item
		# If desktop = DELETED just delete database entry
		# If desktop = MODIFIED, then aim to preserve: - send file back to handheld with new id
		# if desktop = nonexistent then just dump it
		
		if len(self.deleted):
			self.logger.info("handling %d remote deletions" % len(self.deleted))
		
		for obj in self.deleted:
						
			if self.fdb.db.has_key(obj):
				
				self.fdb.db[obj].Reported = True
				
				if self.fdb.db[obj].localstate == LOCAL_STATE_UNCHANGED:
					
					# mark the file/folder for deletion after all processing
					# is complete
					
					self.DBTouched = True
					self.fdb.ScheduleForDeletion(obj)
					
					# remove from database and delete file
					
					self.logger.debug("oid %d marked for deletion" % obj)
					
				elif self.fdb.db[obj].localstate == LOCAL_STATE_DELETED:
					
					# the file has been deleted. Just dump the DB entry
					
					self.logger.debug("oid %d already deleted, removing DB entry" % obj)
					self.DBTouched = True
					del self.fdb.db[oid]

				else: 
					# must be MODIFIED.
					#
					# send the file back to the handheld as a new item - take it
					# out of the database and place it on the new queue
					
					self.logger.debug("requeuing OID %d" % obj)
					self.fdb.Requeue(obj)
			else:
					# the desktop has no knowledge of this OID - just mark the
					# OID for destruction
					
					self.logger.debug("orphaned oid %d - deleting" % obj)
					self.OldOIDs.append(obj)
					
		# clear the object ID queue for deleted events
		
		self.deleted = []
				
		#
		# give ourselves a breathing space
			
		if self.thread.stopping:
			return
				
		# Now we need to deal with OIDs that have been modified on the handheld.These
		# will generally be sent back to the desktop. However, if the desktop
		# is also showing 'modified' then desktop takes precedence. We delete the
		# item, and send the desktop item back to the PDA as a new item
		#
		# If desktop = UNCHANGED -> send modified item to desktop
		# If desktop = DELETED -> send modified item to desktop
		# If desktop = MODIFIED -> delete item on PDA and resend desktop item
		# If desktop = nonexistent -> send to desktop
			
		if len(self.modified):
			self.logger.info("handling %d remote changes" % len(self.modified))
			
		for obj in self.modified:

			if self.fdb.db.has_key(obj):

				self.fdb.db[obj].Reported = True

				if self.fdb.db[obj].localstate == LOCAL_STATE_UNCHANGED or \
				   self.fdb.db[obj].localstate == LOCAL_STATE_DELETED:
					
					# reset the local state to unchanged so we don't send
					# a delete to the handheld.
					
					self.DBTouched = True
					self.fdb.db[obj].localstate = LOCAL_STATE_UNCHANGED
					self.ToFetch.append(obj)
					
				elif self.fdb.db[obj].localstate == LOCAL_STATE_MODIFIED:
					self.ToSend.append(obj)
				else:
					self.ToFetch.append(obj)
			else:
				self.ToFetch.append(obj)
					
		#self.modified = [] --- not yet. We need this on initial sync
		
		#
		# quit while still ahead
			
		if self.thread.stopping:
			return
			
		# OIDs unchanged on the handheld are checked against the database:
		#
		# If desktop = UNCHANGED -> do nothing
		# If desktop = DELETED -> delete on handheld
		# If desktop = MODIFIED -> send to handheld
		# If desktop = nonexistent send to the desktop
		#
		# Note that we only get this call on the initial connection. However,
		# the blasted handheld does not report items deleted while disconnected.
		# So, we have to scan the database and flag for deletion  any item not
		# reported.
			
		if len(self.unchanged):
			self.logger.info("handling %d remote unchanged items" % len(self.unchanged))
			
			for obj in self.unchanged:
				
				self.logger.debug("unchanged object %d" % obj)
			
				if self.fdb.db.has_key(obj):
				
					self.fdb.db[obj].Reported = True
			
					self.logger.debug("obj %d is key" % obj)
				
					if self.fdb.db[obj].localstate == LOCAL_STATE_DELETED:
						self.ToDelete.append(obj)
					elif self.fdb.db[obj].localstate == LOCAL_STATE_MODIFIED:
						self.ToSend.append(obj)
				else:
					self.ToFetch.append(obj)	
				
		self.unchanged = []
		
		if self.thread.stopping:
			return
		
		#
		# initial report delay
		# This mechanism marks local files for deletion that are unreported after a sensible delay from start
		
		if self.initial:
			if self.initrepdelay > self.reporttimeout:
				for oid in self.fdb.db.keys():
					if not self.fdb.db[oid].Reported:
						self.logger.debug("object %d unreported - scheduling for deletion",oid)
						self.fdb.ScheduleForDeletion(oid)
				self.initial = False
			else:
				self.initrepdelay += 1
			
		#
		# time still to quit
		
		if self.thread.stopping:
			return
			
		# OIDs deleted on the desktop, and not reset as part of the recovery scheme are
		# now sent for deletion here.
		
		for obj in self.fdb.db.keys():
			if self.fdb.db[obj].localstate == LOCAL_STATE_DELETED:
			
				self.DBTouched = True
			
				# doesn't matter if this fails, just means it was already deleted
			
				self.logger.debug("deleting object %d from handheld" % obj)
				self.thread.DeleteObject(self.type_id,obj)
		
				del self.fdb.db[obj]
				
		if self.thread.stopping:
			return
	
		############################################
		# Now to do the actual handheld processing.
		#
		# First let's deal with the OIDs to dump.
		#
		############################################
	
		# just don't bother with them for now. Not sure if handheld will reuse?
		
		self.OldOIDs = []
					
		#
		# Object retrieval - file from handheld 
		# 
		
		if len(self.ToFetch) > 0:
			
			self.logger.info("fetching %d items from handheld" % len(self.ToFetch))
		
			try:
				self.thread.GetMultipleObjects(self.type_id, self.ToFetch)
			except pyrra.RRAError, e:
				self.logger.debug("Failed to retrieve all files from handheld: %s" % e)
		
			self.ToFetch = []
						
		if self.thread.stopping:
			return
	
		#
		# Object send: file to handheld. Do these seperately from the new items so
		# we can collision-check the new items on a slow sync later.
		#
		
		if self.thread.stopping:
			return
		
		if len(self.ToSend) > 0:
			
			self.logger.info("sending %d items to handheld" % len(self.ToSend))
			
			newoids = []
			self.readeroids = self.ToSend

			try:
				self.thread.PutMultipleObjects(self.type_id, self.ToSend, newoids, pyrra.RRA_SYNCMGR_UPDATE_OBJECT)
					
				# update the oids in our database
				
				self.DBTouched = True
		
				for i in range(len(newoids)):
					if self.ToSend[i] != newoids[i]:
						f = self.fdb.db[i]
						del self.fdb.db[i]
						db[newoids[i]] = f
						
				self.ToSend = []

			except pyrra.RRAError, e:
				self.logger.debug("Failed to send all files to handheld: %s" % e)
			
		if self.thread.stopping:
			return
	
		# 
		# Now we must scan all remaining new objects, and send them to the handheld
		#
		
		if len(self.fdb.dbnew) > 0:
			
			oids = [0 for i in range(len(self.fdb.dbnew))]
			newoids = []
			
			self.readeroids = oids

			try:
				self.thread.PutMultipleObjects(self.type_id, oids, newoids, pyrra.RRA_SYNCMGR_NEW_OBJECT)
				
				# we now must add the new oids for the new items back into
				# the database
				
				self.DBTouched = True
				
				i=0
				for m in newoids:
					self.fdb.db[m] = self.fdb.dbnew[i]
					self.fdb.db[m].localstate = LOCAL_STATE_UNCHANGED
					i = i + 1
				
				# All new items sent and recorded.
				
				self.fdb.dbnew = []

			except pyrra.RRAError, e:
				self.logger.debug("Failed to send all files to handheld: %s" % e)

		if self.thread.stopping:
			return
				
		# 
		# Object deletion - file on handheld. Do this last.
		#
		
		for i in self.ToDelete:
			
			self.logger.info("deleting item %d from handheld" % i)
			
			self.thread.DeleteObject(self.type_id,i)
			
		self.ToDelete = []

		# Mark modified items as unchanged and clear the modified list.
		
		for oid in self.modified:
			self.thread.MarkObjectUnchanged(self.type_id,oid)
		self.modified = []

		# 
		# Now go through the database and delete scheduled items
		
		self.fdb.DeleteScheduledItems()
				
		# Now, save the database if we updated it
		
		if self.DBTouched:
			self.fdb.SaveDB()
			self.DBTouched = False

		# We now should all be done. 
		
	#
	# File writer callback
	#	
	
	def CB_Writer(self, objID, data):
		
		self.logger.debug("CB_Writer OID: %d" % objID)
	
		# Writer's easier, we get the whole file as one big chunk (This may change in
		# future as we are memory-bound with the current RRA implementation. However.
		# don't bother doing anything if we are about to quit
		
		if len(data) < 4:
			self.logger.debug("data of insufficient length (%d) to retrieve" % len(data))
			return True
		
		if self.thread.stopping:
			return False
		
		ft,path,df = self._unpack_file(data)
		
		# get full path to file (or dir) to write
		
		fullpath = os.path.join(self.fdb.syncdir,path)
		isDir = False
	
		# is it a file?
			
		if (ft & 0x10) == 0:
			dirs,file = os.path.split(fullpath)
			try:
				if not os.path.isdir(dirs):
					self.logger.debug("making dirs %s" % dirs)
					os.makedirs(dirs)
				f=open(fullpath,"wb")
				f.write(df)
				f.close()
			except:
				self.logger.debug("WARNING: Unable to create file %s" % fullpath)
		
		# is it a dir? Handle the brain-damaged RRA sequence of sending a 
		# delete before a name change. We would have handled the delete, so look for the
		# delete in the deleted DB
			
		else:
			isDir = True
			self.fdb.RequeueOrCreateDir(objID,path)
		
		# Now, check our database. If we already have the OID, mark it as 
		# updated. Also, be sure to cancel deletion
		# if we have scheduled it for deletion. Otherwise add it as it is a new item. 
		
		if self.fdb.db.has_key(objID):
			self.fdb.db[objID].localstate = LOCAL_STATE_UNCHANGED
		else:
			# if in the delete queue, refresh the item.
			
			if self.fdb.dbdel.has_key(objID):
				self.fdb.db[objID] = self.fdb.dbdel[objID]
				del self.fdb.dbdel[objID]
				self.fdb.db[objID].localstate = LOCAL_STATE_UNCHANGED
			else:
				# it's a new item.
				# We need to collision-check against new items here. If we get a hit,
				# we have a NEW item on the desktop, and a MODIFIED or UNCHANGED item on the
				# handheld. Resolve in favour of the PDA
			
				for m in range(len(self.fdb.dbnew)):
					if self.fdb.dbnew[m].name == path:
						
						# collision. We already have the file from the handheld,]
						# so just delete from the new list,
						
						del self.fdb.dbnew[m]
			
				# now create and update the fdb entry
			
				mtime = os.path.getmtime(fullpath)
				self.fdb.db[objID] = FileDBEntry(path,mtime,LOCAL_STATE_UNCHANGED,isDir)
				self.fdb.db[objID].Reported = True
				
				# Problem is, we also need to guard against a fast delete/recreate cycle.
				# of a file with the same name - if we don't clear the delete entry the file
				# will get pulled from the handheld, then deleted. So we do this here.
				
				self.fdb.ClearDeleteByPath(path)
		
		self.DBTouched = True
		
		# we're done	
		
		return True
	#
	# File reader callback
	#

	def CB_Reader(self, index, maxlen):
		
		first = False
		d = ""
		rlen = maxlen
		
		# clean up after us if the previous read
		# went south
		
		if self.rdrdata != None:
			idx,fd,ftype = self.rdrdata
			
			if index != idx:
				if fd != 0:
					fd.close()
				self.rdrdata = None
		
		# handle new entry
		
		if self.rdrdata == None:
			
			if self.readeroids[index] == 0:
				path = self.fdb.dbnew[index].name
				if self.fdb.dbnew[index].isDir:
					ftype = 0x10
				else:
					ftype = 0x20
			else:
				path = self.fdb.db[self.readeroids[index]].name
				if self.fdb.db[self.readeroids[index]].isDir:
					ftype = 0x10
				else:
					ftype = 0x20
				
			fullpath = os.path.join(self.fdb.syncdir, path)
			fd = 0
			if (ftype & 0x10) == 0:
				fd = open(fullpath,"rb")
				
			self.rdrdata = (index,fd,ftype)
			
			d += self._fhdr(ftype,path)
			rlen -= len(d)
		
		if (ftype & 0x10) == 0:
			d += fd.read(rlen)
			if len(d) == 0:
				fd.close()
	
		if len(d) == 0:
			self.rdrdata = None
			
		self.logger.debug("reader returning file of len %d" % len(d))
		return d

	#
	# _unpack_file
	# 
	# Unpack a file from the RRAC data block - returns type, path and data
	#
	
	def _unpack_file(self,s):
		
		ftype = struct.unpack("<I",s[0:4])
		# need to find the end of the name
		pos=4
		while pos < len(s):
			v=struct.unpack("<H",s[pos:pos+2])
			if v[0] == 0:
				break
			pos += 2
		winpathname = util.decode_wstr(s[4:pos])
		pth=winpathname.replace("\\","/")
		data = s[pos+2:]
		self.logger.debug("unpacking data - name %s" % pth)
		return ftype[0],pth,data

	#
	# _fhdr
	# 
	# Pack a file into a string usable as an RRAC data block
	#

	def _fhdr(self,ftype, pth):
		return struct.pack("<I",ftype) + util.encode_wstr(pth.replace("/","\\"))

###############################################################################
# ObjTypeHandler
#
# Class linking type_ids to processors
#
###############################################################################

class ObjTypeHandler:
	def __init__(self,name,processor):
		self.name = name
		self.processor = processor
		self.subscribed = False

###############################################################################
# RRAThread
#
# Brains of the beast. The process loops live here
# 
#
###############################################################################

class RRAThread(pyrra.RRASession,threading.Thread):
	
	def __init__(self):
		threading.Thread.__init__(self)
		pyrra.RRASession.__init__(self)
		self.logger = logging.getLogger("engine.rrasyncmanager.RRAThread")
		self.stopping=0
        	self.syncmgr = None
		self.started = 0
#		self.db_config_dir = os.path.join(os.path.expanduser("~"), "PDAFiles")
		self.tidhandlers = {}
		self.config = None
		self.logger.info("new RRA handler thread created")

	def stop(self):
		self.logger.debug("stop: stopping RRA server")
		self.stopping = 1
		self.join()
		self.started = 0
		self.tidhandlers = {}

	def Subscribe(self,pship):
		
		rc = False
	
		# get us a list of object IDs - we need this to know what we
		# can support.

		oj = self.GetObjectTypes()
		
		self.logger.debug("have %d object types" % len(oj))
		
		# If we have other ID processor classes available, make sure we 
		# assign them here, in this loop. We only have 'File' types here
		# at the moment. The other four seem to be 'Ink' (eh?), 'Media' 
		# (probably synced with files?) and 'Favourites' (bookmarks, I think).
		# It should be possible to add derivatives from the Processor class
		# to handle these items without having to rewrite the threading code.
		
		self.tidhandlers = {} 
		
		# scan for object types
		
		for ot in oj:
			if ot.name1 == "File":
				self.tidhandlers[ot.id] = ObjTypeHandler(ot.name1,FileProcessor(ot.id,pship,self))
			
		# search for and subscribe to events specified by the partnership
			
		for item_id in pship.devicesyncitems:
			if SYNC_ITEM_PSHIPID_TO_RRANAME.has_key(item_id):
				rra_item_name = SYNC_ITEM_PSHIPID_TO_RRANAME[item_id]
				for tid in self.tidhandlers.keys():
					if self.tidhandlers[tid].name == rra_item_name:
						self.logger.info("Subscribing to %s objects" % rra_item_name)
						try:
							self.SubscribeObjectEvents(tid)
							self.tidhandlers[tid].subscribed=True
							rc=True
						except pyrra.RRAError, e:
							self.logger.debug("Subscribe failed: type %s: %s" % (self.tidhandlers[oid].name, e))
		return rc


	def run(self):
		
		self.started = 1
 
		self.logger.debug("run: starting RRA event loop")
	
		# get the file descriptor so we can use a select call to wait on.

		self.fd = self.GetEventDescriptor()

		# start listening for scheduled events

		self.StartEventListener()

		# the event processor loop

		while True:
			select.select([self.fd],[],[],1.0)
			
			# select will trigger on the first event - but
			# grab 'em all at once. This handles the brain-damaged
			# way in which folders are renamed (delete followed by change)
			
			try:
				self.HandleAllPendingEvents()
			except pyrra.RRAError, e:
				self.logger.debug("Failed in HandleAllPendingEvents(): %s" % e)
							
			# go process all supported event types
			
			for th in self.tidhandlers.keys():
				
				# quit if necessary, some of these processes
				# may be slow
				
				if self.stopping:
					break	
				if self.tidhandlers[th].subscribed:
					self.tidhandlers[th].processor.Process()
					
			if self.stopping:
				break
			
		self.tidhandlers = []
			
	
	def CB_TypeCallback(self,event,type,idarray):
		
		# Called only in the context of the RRA thread
		# Handle objects of subscribed type here only
		
		if  self.tidhandlers.has_key(type):
			
			self.logger.debug("Object callback for event %s type %d" % (self.tidhandlers[type].name, event))
			
			# we are subscribed to this type
						
			if event == pyrra.SYNCMGR_TYPE_EVENT_UNCHANGED:
				self.tidhandlers[type].processor.unchanged += idarray
			elif event == pyrra.SYNCMGR_TYPE_EVENT_CHANGED:
				self.tidhandlers[type].processor.modified += idarray
			elif event == pyrra.SYNCMGR_TYPE_EVENT_DELETED:
				self.tidhandlers[type].processor.deleted += idarray
			else:
				return False
		return True

	def CB_ObjectWriterCallback(self,type_id, obj_id, data):
		
		if  self.tidhandlers.has_key(type_id):			
			self.logger.debug("Object writer for item %d" % obj_id)
			return self.tidhandlers[type_id].processor.CB_Writer(obj_id, data)
		else:
			return False


	def CB_ObjectReaderCallback(self,type_id, index, maxlen):
		if  self.tidhandlers.has_key(type_id):			
			self.logger.debug("Object reader for item index %d" % index)
			return self.tidhandlers[type_id].processor.CB_Reader(index, maxlen)
		else:
			return 0
		
###############################################################################
# RRASyncManager
#
# RRA Sync manager object
#
###############################################################################

class RRASyncManager:
	
	def __init__(self,engine):
		self.engine = engine
		self.thread = None
		self.logger = logging.getLogger("engine.rrasyncmanager.RRASyncManager")

	
	def StartRRAEventHandler(self):
		
		# get current partnership
		
		pship = self.engine.PshipManager.GetCurrentPartnership()
		self.thread = RRAThread()
		self.thread.syncmgr = self
		self.thread.config = self.engine.config
		if pship:
			self.logger.info("connecting to RRA")
			try:
				self.thread.Connect(self.engine.rapi_session)

				self.logger.info("connected")
				if not self.thread.Subscribe(pship):
					self.logger.debug("No RRA type subscriptions - ignoring RRA sync items")
				self.thread.start()
			except pyrra.RRAError, e:
				self.logger.debug("Unable to start RRA event handler: %s" % e)
				return
		else:
			self.logger.debug("No current partnerships for RRA connection")
			
	def StopRRAEventHandler(self):
		if self.thread != None:
			if self.thread.started:
				self.thread.stop()
				self.thread.join()
		
			if self.thread.isConnected():
				self.thread.Disconnect()
				if self.thread.isConnected():
					self.logger.debug("Failed to disconnect from RRA")
			
			del self.thread
			self.thread = None
					
