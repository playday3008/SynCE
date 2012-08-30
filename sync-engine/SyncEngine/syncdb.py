# -*- coding: utf-8 -*-
############################################################################
# SYNCDB.py
#
# This module contains objects relating to sync items and databases used
# in the maintenance of state information by partnerships. It is based
# on code from the original partnerships.py but cleaned up and merged
# considerably.
#
# Original partnerships.py (C) 2006  Ole André Vadla Ravnås <oleavr@gmail.com> 
#
# Dr J A Gow 29/11/07
#
############################################################################

from constants import *
import cPickle as pickle
import logging
import random

###############################################################################
#
# ItemDB (formerly SyncItem)
#
# Objects of this class represent a database of sync item entries. It is 
# written for easy pickling allowing us to conveniently save the item 
# database as necessary. When not syncing, the local and remote change DB
# should (nominally) be empty and will not create an overhead. However, if
# phone triggers a sync and there is no sync on the Opensync side, the remote
# change database will contain entries. These will be saved in case 
# the sync-engine is shut down in the meantime.
#
# The mapping tables are also saved with the item database.
#
###############################################################################


logger = logging.getLogger("engine.syncdb.ItemDB")

class ItemDB:
	
	def __init__(self, type):
		
		self.type = type
		
		self.itemdb = {}		# slow sync DB
		self.itemdbIsLoaded = False
		
		self.localchanges = {}
		self.remotechanges = {}
		self.remIDtoitemID = {}
		self.itemIDtoremID = {}
		
		# it is a lot less cumbersome to use a dirty flag than it is
		# to track itemDB accesses through the sync-engine. This way
		# we only ever save what we want
		
		self.isDirty = False

	###################################
	# INTERNAL FUNCTIONS
	###################################
	
	#
	# _GenerateNewItemID
	#
	# Generate a new itemID for newly sent items
	#


	def _GenerateNewItemID(self):
		return "pas-id-%08X%08X" % (random.randint(0, 0xFFFFFFFF),
                                            random.randint(0, 0xFFFFFFFF))

	#
	# __str__
	#
	# Debug dumps
	#

	def __str__(self):
		
		s = "Local Changes:\n"
		i = 0
		for key, value in self.local_changes.items():
			change_type, data = value
			s += "  %d: GUID = %s, Type = %s, Data = %s\n" % (i, key, change_type, data)
			i += 1

		s += "Remote Changes:\n"
		i = 0
		for key, value in self.remote_changes.items():
			change_type, data = value
			s += "  %d: GUID = %s, Type = %s, Data = %s\n" % (i, key, change_type, data)
			i += 1

		s += "LUID -> GUID Mappings:\n"
		for key, value in self.luid_to_guid.items():
			s += "  LUID = %s, GUID = %s\n" % (key, value)

		s += "GUID -> LUID Mappings:\n"
		for key, value in self.guid_to_luid.items():
			s += "  GUID = %s, LUID = %s\n" % (key, value)

		return s


	####################################
	# EXTERNAL FUNCTIONS
	####################################
	
	#
	# Save
	#
	# Save ourselves, but only if the itemDB is dirty. This way we prevent unnecessary
	# disk-thrashing.
	
	def Save(self, idbpath):
		
		if self.isDirty == True:
			
			logger.info("itemDB(%d) is dirty, saving to file" % self.type)
			
			itemfile = idbpath + str(self.type)
			try:
				f=open(itemfile,"wb")
				
				# chicken and egg. We need to set the dirty flag to false before
				# we save (so when restored, it is restored as false). However, 
				# if we cock up, we need to retain it. So reset it to True in
				# the exception handler
				
				self.isDirty = False
				pickle.dump(self, f, pickle.HIGHEST_PROTOCOL)
				f.close()
			except:
				self.isDirty = True
				logger.info("SaveItemDB: FAILED TO SAVE ITEMDB for item %d" % self.type)

		else:
			logger.info("itemDB(%d) is clean, no save required" % self.type)

	#
	# PrefillRemoteChangeDB
	#
	# This is called in response to a slow sync. For this sync only, it prefills the
	# local change db with all the nonmatching items, while matching items marked as MODIFIED
	# are recast as ADDED. This has the effect of squirting EVERYTHING back to OpenSync.
	# There will be no DELETED items in our db list as these will already have been removed -
	# we leave deleted items in place. However, on a slow sync we must remove DELETED
	# items from the sync list, otherwise they just get added back again by OpenSync
	# Note that we save on the AirSync side - this way we mirror what is stored in the phone,
	# not what is stored locally. 

	def PrefillRemoteChangeDB(self,config):

		self.isDirty = True
		
		for itemID in self.itemdb.keys():

			d = self.itemdb[itemID]
			ct = CHANGE_ADDED

			if self.remotechanges.has_key(itemID):
				ct,data = self.remotechanges[itemID]
				
			self.remotechanges[itemID] = ct, d
	
		# Now scan for items marked as DELETED and remove them - in a slow sync
		# we only want to know what's actually in the db AND what's actually
		# in the phone, not OR what's in the phone

		for itemID in self.remotechanges.keys():
			ct,data = self.remotechanges[itemID]
			if ct == CHANGE_DELETED:
				del self.remotechanges[itemID]

	#
	# QueryRemoteIDsInItemDB
	#
	# Function returning a list of remote IDs known about in the mapping. Sync
	# backends that do not explicitly report deletions when disconnected (RRA)
	# may be able to make use of this
	#
	
	def QueryRemoteIDsInItemDB(self):
		return remIDtoitemID.keys()

	#
	# AddLocalChange (formerly add_local_change)
	#
	# Adds a changed item to the local changes DB
	#

	def AddLocalChanges(self,changelist):
		
		self.isDirty = True
		for change in changelist:
			itemid,chtype,itemdata = change
			
			# Get the remote ID. We will not have this if the type
			# is ADDED - so send in None and let the backend set the
			# appropriate OID
			
			remid = None
			if self.itemIDtoremID.has_key(itemid):
				remid = self.itemIDtoremID[itemid] 
				
			self.localchanges[itemid] = (remid,chtype,itemdata)
			logger.info("ItemDB (%d) AddLocalChange: item %s added %d" % (self.type, itemid, len(self.localchanges)))

	#
	# GetLocalChangeCount (formerly get_local_change_count)
	#
	# Return the number of changes in the local change database
	#

	def GetLocalChangeCount(self):
		return len(self.localchanges)

	#
	# QueryLocalChanges (formerly extract_local_changes)
	#
	# This returns a set of local changes up to a maximum count
	# supplied and removes these from the database. Any items not returned
	# are left in the database
	# Returns list of [(itemID, (remid,chtype,itemdata))]
	#

	def QueryLocalChanges(self, max):
		
		logger.info("AcquireLocalChanges: item type %d , %d changes " % (self.type,len(self.localchanges)))
		
		if len(self.localchanges) <= max:
			changeset = self.localchanges.items()
		else:
			changeset = self.localchanges.items()[:max]
			
        	return changeset

	#
	# AcknowledgeLocalChanges
	#
	# Acknowledges safe syncing of the item and ensures that the remote OID (and the data,
	# if necessary,is updated in the database.
	#
	# 'changes' is a list of [(itemID, remID)] The backend will update the remID - we
	# are agnostic here of its actual content being only concerned with the mapping
	
	def AcknowledgeLocalChanges(self,changes):
		
		self.isDirty = True
		
		for change in changes:
			
			itemID,remID = change
			
			if self.localchanges.has_key(itemID):
				
				rid,chtype,itemdata = self.localchanges[itemID]
				
				if chtype == CHANGE_DELETED:
					
					# If deleted, dump the itemDB entry if there
					# is one
					
					if self.itemdb.has_key(itemID):
						del self.itemdb[itemID]
					else:
						logger.info("AcknowledgeLocalChanges: Attempt to delete non-existent entry (%s) in itemDB" % itemID)
						
					# delete the reverse mapping.
					
					if self.remIDtoitemID.has_key(remID):
						del self.remIDtoitemID[remID]
						
					# delete the forward mapping
					
					if self.itemIDtoremID.has_key(itemID):
						del self.itemIDtoremID[itemID]

				else:
					# ADDED or MODIFIED
					# just add (or update) the itemDB and the reverse mapping
					
					self.itemdb[itemID] = itemdata
					self.remIDtoitemID[remID] = itemID
					self.itemIDtoremID[itemID] = remID

					
				# remove the local change.
					
				del self.localchanges[itemID]
			else:
				logger.info("AcknowledgeLocalChanges: Ack for non-existent local change (%s)" % itemID)


	#
	# AddRemoteChanges (formerly add_remote_change)
	#
	# Add remote changes
	#
	# Argument is a list of [(remID,chgtype,data)]
	#

	def AddRemoteChanges(self, changes):
	
		self.isDirty = True
		
		for change in changes:
			
			remID,chgtype,data = change
			
			# Check if we have a mapping. If we do not, we have not seen this change OR
			# this item before so we can process as normal. If we do have a mapping,
			# we have seen the item before, so we need to check the change.
			
			if self.remIDtoitemID.has_key(remID):
				
				itemID = self.remIDtoitemID[remID]
	
				# Now update our changesets for the current sync.
	
				if self.remotechanges.has_key(itemID):
		
					# There was already a remote change for this item, so we need to
					# reconcile it here.  This is an exhaustive list of each of the
					# possible state transitions, along with the actions we take:
					#   ADDED    -> DELETED  : delete change record
					#   ADDED    -> ADDED    : can happen if item is turned off/on on remote
					#   ADDED    -> MODIFIED : overwrite change record
					#   DELETED  -> DELETED  : impossible
					#   DELETED  -> ADDED    : impossible
					#   DELETED  -> MODIFIED : impossible
					#   MODIFIED -> DELETED  : delete change record
					#   MODIFIED -> ADDED    : impossible
					#   MODIFIED -> MODIFIED : overwrite change record

					# JAG: check - is this right? - can we miss a delete this way? OK for modified though
					# .... seems OK so far, no spurious reports of missed deletes.

					cur_change_type, cur_data = self.remotechanges[itemID]
					
					if cur_change_type in (CHANGE_ADDED, CHANGE_MODIFIED) and chgtype == CHANGE_DELETED:
						
						# we have since got rid of it between OpenSync syncs, so just kill it off

						del self.remotechanges[itemID]
					
					elif cur_change_type in (CHANGE_ADDED, CHANGE_MODIFIED) and chgtype == CHANGE_MODIFIED:
						
						# update the data in the known change

						self.remotechanges[itemID] = (chgtype, data)
						
					elif cur_change_type in (CHANGE_ADDED, CHANGE_MODIFIED) and chgtype == CHANGE_ADDED:
						
						# this will happen if we turn off the sync item on the phone, disconnect,
						# reconnect, disconnect, re-enable the item, then reconnect. Form of 
						# phone-side slow-sync. If we do this without running an OpenSync run
						# in the meantime we end up here.
						# just update the data in the known change

						self.remotechanges[itemID] = (chgtype,data)
	
					else:
						logger.error("Unhandled change state transition: %d -> %d" % (cur_change_type, chgtype))
						logger.error("ignoring transaction")

				else:

					# The change didn't exist already, so we add it. However, check the itemDB if the
					# change type is ADDED as we may be running a sync after toggling an item's availability
					
					if chgtype == CHANGE_ADDED:
						if self.itemDB.has_key(itemID):
							
							# we have it in the itemDB, so we are trying to add an object with the
							# same ID. Check the data. If it is the same, then ignore the change.
							# Otherwise recast as MODIFIED.
	
							olddata = self.itemDB[itemID]
							
							if olddata != data:
								self.remotechanges[itemID]=(CHANGE_MODIFIED, data)
								
							# otherwise we just toss it.
						else:
							# we know nothing about the item, so can add the ransaction as normal.
	
							self.remotechanges[itemID] = (chgtype, data)
					else:
						self.remotechanges[itemID] = (chgtype,data)
			else:
				
				# we have no pre-existing mapping for this item
				
				itemID = self._GenerateNewItemID()
				self.remIDtoitemID[remID] = itemID
				self.itemIDtoremID[itemID] = remID
				self.remotechanges[itemID] = (chgtype,data)
				

	#
	# GetRemoteChangeCount (formerly get_remote_change_count)
	#
	# Return the number of changes in the remote change database
	#

	def GetRemoteChangeCount(self):
		return len(self.remotechanges)

	#
	# GetRemoteChanges (formerly get_remote_changes)
	#
	# Return the contents of the remote change database, but do not empty it.

	def GetRemoteChanges(self):
		return self.remotechanges

	#
	# AcknowlegeRemoteChange (formerly ack_remote_change)
	#
	# Remove a specified remote chnage from the remote change database
	#

	def AcknowledgeRemoteChange(self, itemID):
		
		self.isDirty = True
		
		if self.remotechanges.has_key(itemID):
			chgtype,data = self.remotechanges[itemID]
			
			if chgtype in (CHANGE_ADDED, CHANGE_MODIFIED):
				self.itemdb[itemID] = data
			else:
				if self.itemdb.has_key(itemID):
					del self.itemdb[itemID]
				if self.itemIDtoremID.has_key(itemID):
					remID = self.itemIDtoremID[itemID]
					del self.itemIDtoremID[itemID]
					del self.remIDtoitemID[remID]
				
				
			del self.remotechanges[itemID]
