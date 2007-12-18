###############################################################################
# BACKEND.py
#
# Simple backend at present handling mail folders and providing a platform
# for simple mail sync to folders
#
# (C) Dr J A Gow 2007
#
###############################################################################

import cPickle as pickle
import os
import os.path
import email
import email.generator
import util
import base64
from constants import *


AM_FOLDER_TYPES = { "Junk"    : 1,
		    "Inbox"   : 2,
		    "Drafts"  : 3,
		    "Deleted" : 4,
		    "Sent"    : 5,
		    "Outbox"  : 6 }



class Backend:
	
	def __init__(self):
		
		self.amconfig = os.path.join(os.path.expanduser("~"), ".airmail")
		self.amfolders = os.path.join(self.amconfig,"folders")
		self.sync_key = "{062E8DA7-CA3D-5972-AC10-5164ABD0FF24}"

		#
		# folderlist is keyed by folder guid
		#
		# guid: (parent,displayname,type,change,{itemsbyID},{itemIDbyName})
		#
		# nametoguid is a direct mapping for folders
		#
		
		self.folderlist = {}
		self.nametoguid = {}
		
		pass
	
	#
	# _GetPathToFolder
	#
	# Quick way of reconstructing the path
	#
	
	def _GetPathToFolder(self,folderID):
					
		# get path to folder. Backwards.

		curParentID = folderID
		path=None
		while curParentID != "0":
			(curParentID,parentname,p_type,p_uptodate,p_itemsbyID,p_itemIDByName) = self.folderlist[curParentID]
			if path==None:
				path=parentname
			else:
				path=os.path.join(parentname,path)
				
		path = os.path.join(self.amfolders,path)
		return path
	
	#
	# _ScanFolders
	#
	# This internal function scans our folder tree for changes, and updates the internal database
	# appropriately
	#
	
	def _ScanFolders(self):

		if os.path.isdir(self.amfolders):
			
			parent = "0"
			isFirstLevel=True
			
			#
			# whip round all folders and set them to OBJECT_DELETE
			
			for itemID in self.folderlist.keys():
				parent,displayname,typeval,change,itemsbyID,itemIDByName = self.folderlist[itemID]
				self.folderlist[itemID] = parent,displayname,typeval,OBJECT_TODEL,itemsbyID,itemIDByName
			
			for path,dirs,files in os.walk(self.amfolders,topdown=True):

				if isFirstLevel:
					# this is the first level. Create the parent list
					parent="0"
					isFirstLevel = False
					pname = ""
				else:
					root,pname = os.path.split(path)
					parent = self.nametoguid[pname]

				for fldrname in dirs:
					
					# if we already have it in the database, mark its
					# change type as OBJECT_NOCHANGE. Otherwise
					# mark it as OBJECT_NEW

					if fldrname in self.nametoguid.keys():
						
						db_fldrID = self.nametoguid[fldrname]
						parent,displayname,typeval,change,itemsbyID,itemIDByName = self.folderlist[db_fldrID]
						self.folderlist[db_fldrID] = (parent,displayname,typeval,OBJECT_NOCHANGE,itemsbyID,itemIDByName)
						
					else:
						
						# Same applies here for errors -  just ignore them,
						# the result will leave the dir in place.

						try:
							
							fldrid = util.generate_guid()
					
							if fldrname in AM_FOLDER_TYPES.keys():
								fldrtype = AM_FOLDER_TYPES[fldrname]
							else:
								fldrtype = 1
							
							self.folderlist[fldrid] = (parent,fldrname,fldrtype,OBJECT_NEW,{},{})
							self.nametoguid[fldrname] = fldrid

						except:
							pass


	#
	# InitializeFolderList
	#
	# Load us from permanent storage. If our pickle does not
	# exist, then create it.
	
	def InitializeFolderList(self):
		
		self.LoadFolderList()
		self._ScanFolders()
		self.SaveFolderList()


	#
	# SaveFolderList
	#
	# Save an updated folder list (a copy of ourselves)
	
	def SaveFolderList(self):
		
		ff = os.path.join(self.amconfig,"FL-1")
		try:
			f=open(ff,"wb")
			pickle.dump(self.folderlist,f,pickle.HIGHEST_PROTOCOL)
			pickle.dump(self.nametoguid,f,pickle.HIGHEST_PROTOCOL)
			f.close()
			return True
		except Exception, e:
			print "failed to save %s" % e
			return False
	
	#
	# LoadFolderList
	#
	# Load our folder list from file, create if we can't load.
	
	def LoadFolderList(self):
		
		ff = os.path.join(self.amconfig,"FL-1")
		try:
			f=open(ff,"r")
			self.folderlist = pickle.load(f)
			self.nametoguid = pickle.load(f)
			f.close()
			return True
		except Exception,e:
			print "failed to load %s" % e
			return False
	
	#
	# DumpFolderList
	#
	# Display the folder list
	
	def DumpFolderList(self):
		
		for folderID in self.folderlist.keys():
			parent,displayname,typeval,uptodate,itemsbyID,itemIDByName = self.folderlist[folderID]
			print "Folder %s" % folderID
			print "   Parent %s" % parent
			print "   Name %s" % displayname
			print "   Type %d" % typeval
			print "   Sync %s" % uptodate
	
	#
	# QueryFolderList
	#
	# returns a complete list of folders on the server, including
	# all associated data - then marks them up to date
	#
	
	def QueryFolderList(self):
		
		flist = []
		
		for guid in self.folderlist.keys():
			parentID,displayname,type, uptodate,itemsbyID, itemIDByName = self.folderlist[guid]
			flist.append((guid,parentID,displayname,type))
			uptodate = True
			self.folderlist[guid]= parentID,displayname,type,uptodate,itemsbyID, itemIDByName

		self.SaveFolderList()
		return flist
	
	#
	# QueryFolderChanges
	#
	# Return any new folders
	#
	
	def QueryFolderChanges(self,initialsync):
		
		self._ScanFolders()
		
		flist = []
		for folderID in self.folderlist.keys():
			parentID,displayname,type,change,itemsbyID, itemIDByName = self.folderlist[folderID]
			if initialsync:
				self.folderlist[folderID]= parentID,displayname,type,OBJECT_NEW,itemsbyID, itemIDByName
				flist.append((folderID,parentID,OBJECT_NEW,displayname,type))
				
			else:
				if change != OBJECT_NOCHANGE:
					flist.append((folderID,parentID,change,displayname,type))
					self.folderlist[folderID]= parentID,displayname,type,change,itemsbyID, itemIDByName
				
		self.SaveFolderList()
		return flist
		
	#
	# AcknowledgeFolderChanges
	#
	# Called back from the sync process when a change has been actioned
	
	def AcknowledgeFolderChanges(self,changes):
		
		for folderID,parentID,change,displayname,type in changes:
			parentID,displayname,type,change,itemsbyID, itemIDByName = self.folderlist[folderID]
			if change == OBJECT_TODEL:
				path = self._GetPathToFolder(folderID)
				
				del self.folderlist[folderID]
				del self.nametoguid[displayname]
				
				util.deltree(path)
			else:
				self.folderlist[folderID] = parentID,displayname,type,OBJECT_NOCHANGE,itemsbyID,itemIDByName
		self.SaveFolderList()
	
	#
	# AddFolder
	#
	# Called to add a folder to the list. Returns the guid of the new
	# folder, or an empty string if failed
	#
	# 
	
	def AddFolder(self,name,parent):
		
		if not self.nametoguid.has_key(name):
		
			folderID = util.generate_guid()
			type = 1
			
			# Now try and create it. We need to build the path. Backwards.
			
			path = name
			curParentID = parent
			
			print " New folder %s " % name
			print " Parent ID  %s " % parent
			
			try:
				while curParentID != "0":
					(curParentID,parentname,type,uptodate,itemsbyID,itemIDByName) = self.folderlist[curParentID]
					path=os.path.join(parentname,path)
		
				path = os.path.join(self.amfolders,path)
			except:
				print "can not find parent"
				return ""
			
			# make the physical dir.
			
			try:
				os.makedirs(path)
				self.folderlist[folderID] = (parent,name,type,True,{},{})
				self.nametoguid[name]=folderID
				self.SaveFolderList()
			except:
				print "could not make folder"
				return ""
		else:
			folderID = self.nametoguid[name]
			
		return folderID
	
	#
	# DeleteFolder
	#
	# Called to delete a folder
	#
	
	def DeleteFolder(self,folderID):
		
		if self.folderlist.has_key(folderID):
			parentID,name,type,uptodate,itemsbyID, itemIDByName = self.folderlist[folderID]
			
			curParentID = parentID
			path = name
			while curParentID != "0":
				(curParentID,parentname,type,uptodate,p_itemsbyID,p_itemIDByName) = self.folderlist[curParentID]
				path=os.path.join(parentname,path)
		
			path = os.path.join(self.amfolders,path)
	
			util.deltree(path)
			
			del self.folderlist[folderID]
			del self.nametoguid[name]
			
			self.SaveFolderList()
			
			return 1
		else:
			return 0
		
	#
	# IncomingMail
	#
	# When mail is sent on the remote, it ends up here in Python's 'email' format
	
	def IncomingMail(self,message):
		
		print "Incoming message - saving to the sent file"
		
		pathSent = os.path.join(self.amconfig,"mboxout")
		
		try:
			fp = open(pathSent,"ab")
			generator = email.generator.Generator(fp)
			generator.flatten(message)
			fp.close()
			print "Successfully written mail message to send stack"
			
		except Exception,e:
			print "Failed to update send stack (%s)" % e
			raise
		
	#
	# QueryChangeCount
	#
	# Return the number of changed items in a folder. The folderID is passed in
	# from the server, and the "firstrequest" will be set True if we need
	# to send everything down the wire.
	#
	
	def QueryChangeCount(self,folderID,firstrequest):
		
		print "QueryChangeCount for folder %s" % folderID
		
		if self.folderlist.has_key(folderID):
			parentID,name,type,uptodate,itemsbyID,itemIDByName=self.folderlist[folderID]
			
			# Construct the folder path.
			
			curParentID = parentID
			path = name
			while curParentID != "0":
				(curParentID,parentname,p_type,p_uptodate,p_itemsbyID,p_itemIDByName) = self.folderlist[curParentID]
				path=os.path.join(parentname,path)
		
			path = os.path.join(self.amfolders,path)
			
			checked = {}
			for i in itemsbyID.keys():
				checked[i]=False
			
			print "Path %s" % path
			
			# Now scan the folder for files of email
			
			items = os.listdir(path)
			
			print "Items %s" % str(items)
			
			for f in items:
				ipth = os.path.join(path,f)
				print "Item path %s" % str(ipth)
				
				if os.path.isfile(ipth):
					
					print "file %s" % f
					
					# we only bother with files. Do we already have it?
					
					if itemIDByName.has_key(f):
						
						itemID = itemIDByName[f]
						name,change = itemsbyID[itemID]
						
						# TODO: is it changed? Check here
						
						# assume not for now
						
						checked[i]=True

					else:
						
						# it is new: - generate a new OID
						
						itemID = util.GenerateMessageID()
						change = OBJECT_NEW
						print "New item %s(%s)" % (f,itemID)
												
						itemsbyID[itemID] = f,change
						itemIDByName[f]=itemID
						
			#
			# Now go for deleted objects
			
			for itemid in checked.keys():
				if checked[itemid] ==  False:
					# we have a deleted object
					name,change = itemsbyID[itemid]
					change = OBJECT_TODEL
					itemsbyID[itemID] = name,change
				
			# preserve us
			
			self.SaveFolderList()
			
			#
			# OK, we have checked them all. Now we must query the changes
			
			changecount=0
			
			for itemid in itemsbyID.keys():
				
				name,change = itemsbyID[itemid]
				if change != OBJECT_NOCHANGE:
					changecount += 1
					
			return changecount
		else:
			print "No such folder for id %s" % folderID
			return 0
	
	#
	# QueryChanges
	#
	# Get the changes for a given folder, up to a maximum window size
	
	def QueryChanges(self,folderID,window):
		
		changes = []
		if self.folderlist.has_key(folderID):
			parentID,name,type,uptodate,itemsbyID,itemIDByName=self.folderlist[folderID]
			
			# get path to folder

			curParentID = parentID
			path = name
			while curParentID != "0":
				(curParentID,parentname,p_type,p_uptodate,p_itemsbyID,p_itemIDByName) = self.folderlist[curParentID]
				path=os.path.join(parentname,path)
		
			path = os.path.join(self.amfolders,path)

			# Now, for each changed item, insert in the list up to a maximum number
			# of 'window'
			
			for i in itemsbyID.keys():
				
				name,change = itemsbyID[i]
				
				if change != OBJECT_NOCHANGE:
					
					message=None
					
					if change in (OBJECT_NEW,OBJECT_CHANGED):
						mpath = os.path.join(path,name)
						try:
							fp = open(mpath,"r")
							message = email.message_from_file(fp)
							fp.close()
						except:
							print "bad message in folder"
							message = None
					changes.append((i,change,message))
					
					window -= 1
					if window==0:
						break
					
		else:
			print "No such folder ID"
			
		return changes
		
		
	#
	# AcknowledgeChanges
	#
	# Called when the sync module has handled the change and it needs to be actioned
	
	def AcknowledgeChanges(self,folderID,changes):
		
		for itemID, change, message in changes:
		
			if self.folderlist.has_key(folderID):
			
				parentID,name,type,uptodate,itemsbyID,itemIDByName=self.folderlist[folderID]
			
				# get path to folder

				curParentID = parentID
				path = name
				while curParentID != "0":
					(curParentID,parentname,p_type,p_uptodate,p_itemsbyID,p_itemIDByName) = self.folderlist[curParentID]
					path=os.path.join(parentname,path)
		
				path = os.path.join(self.amfolders,path)

				if itemsbyID.has_key(itemID):
					name,change = itemsbyID[itemID]
					if change == OBJECT_TODEL:
						mpath = os.path.join(path,name)
						print "Item %s removed" % itemID
						del itemsbyID[itemID]
						del itemIDByName[name]
						try:
							os.remove(mpath)
						except:
							print "Unable to remove file %s" % mpath
	
					else:
						print "Item %s acknowledged" % itemID
						change = OBJECT_NOCHANGE
						itemsbyID[itemID] = name,change
				else:
					print "No such item %s found in folder" % itemID
					
		self.SaveFolderList()
		
	#
	#
	# FetchItem
	#
	# Fetch a specific mail item, whether changed or not.
	
	def FetchItem(self,folderID, itemID):
		
		if self.folderlist.has_key(folderID):
			
			parentID,name,type,uptodate,itemsbyID,itemIDByName=self.folderlist[folderID]
			
			# get path to folder

			curParentID = parentID
			path = name
			while curParentID != "0":
				(curParentID,parentname,p_type,p_uptodate,p_itemsbyID,p_itemIDByName) = self.folderlist[curParentID]
				path=os.path.join(parentname,path)
		
			path = os.path.join(self.amfolders,path)

			if itemsbyID.has_key(itemID):
				
				name,change = itemsbyID[itemID]
				mpath = os.path.join(path,name)

				try:
					fp = open(mpath,"r")
					message = email.message_from_file(fp)
					fp.close()
				except:
					print "bad message in folder"
					message = None
					
				return (itemID,change,message)
			else:
				return None
		else:
			return None
		
	#
	#
	# DeleteItem
	#
	# Delete a specific mail item
	
	def DeleteItem(self,folderID, itemID):
		
		if self.folderlist.has_key(folderID):
			
			parentID,name,type,uptodate,itemsbyID,itemIDByName=self.folderlist[folderID]
			
			# get path to folder

			curParentID = parentID
			path = name
			while curParentID != "0":
				(curParentID,parentname,p_type,p_uptodate,p_itemsbyID,p_itemIDByName) = self.folderlist[curParentID]
				path=os.path.join(parentname,path)
		
			path = os.path.join(self.amfolders,path)

			if itemsbyID.has_key(itemID):
				name,change = itemsbyID[itemID]
				mpath = os.path.join(path,name)
				print "Item %s(%s) removed" % (itemID,name)
				
				del itemsbyID[itemID]
				del itemIDByName[name]
				
				os.remove(mpath)
				
				self.SaveFolderList()
				
				return True
			else:
				return False
		else:
			return False
	
	#
	# MoveItem
	#
	# Move an item between folders
	
	def MoveItem(self,src_fldrID,dst_fldrID,itemID):
		
		if self.folderlist.has_key(src_fldrID) and self.folderlist.has_key(dst_fldrID):
			
			src_parentID, src_name, src_type, src_uptodate, src_itemsbyID, src_itemIDByName = self.folderlist[src_fldrID]
			dst_parentID, dst_name, dst_type, dst_uptodate, dst_itemsbyID, dst_itemIDByName = self.folderlist[dst_fldrID]
			
			src_path = self._GetPathToFolder(src_fldrID)
			dst_path = self._GetPathToFolder(dst_fldrID)
			
			# Now get the item.
			
			if src_itemsbyID.has_key(itemID):
				name,change = src_itemsbyID[itemID]
				spath = os.path.join(src_path,name)
				
				try:
					fp = open(spath,"rb")
					data = fp.read()
					fp.close()
				except:
					print "MoveItem: can not load file data %s" % spath
					return False
				
				dpath = os.path.join(dst_path,name)
				
				try:
					fp=open(dpath,"wb")
					fp.write(data)
					fp.close()
				except:
					print "MoveItem: can not write file %s" % dpath
					return False
				
				# Now we can transfer the file and update the IDs
				
				os.remove(spath)
				
				dst_itemsbyID[itemID] = name,change
				dst_itemIDByName[name] = itemID
				del src_itemsbyID[itemID]
				del src_itemIDByName[name]
				
				self.SaveFolderList()
				
				return True
			else:
				return False
		else:
			print "MoveItem: No such source or destination folder"
			return False
				
				
	#
	# GetAttachment
	#
	# Given a specific folder ID, message ID, filename and index, return
	# the (contenttype,data).
	
	def GetAttachment(self,folderID,messageID, filename, index):
		
		item = self.FetchItem(folderID,messageID)
		if item != None:
			itemID,change,message=item
			
			# now need to find attachment in message
			
			idx = 0
			for msgpart in message.walk():
				fn=msgpart.get_filename()
				print "Part: index %d filename %s" % (idx,fn)
				if fn==filename and index==idx:
					print "Content-Type %s" % msgpart.get_content_type()
					print "index        %d" % idx
					print "data length  %d" % len(msgpart.get_payload())
					return (msgpart.get_content_type(),base64.b64decode(msgpart.get_payload()))
				idx += 1
			print "not found"
		else:
			print "fetch failed"
		return None	