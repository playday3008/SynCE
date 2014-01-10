# -*- coding: utf-8 -*-
############################################################################
# PSHIPMGR.py
#
# Partnership manager for sync-engine.
#
# This is a rewritten version of the old Partnerships module from the
# original sync-engine. It provides considerably more flexibility in the
# assignment of partnerships over the original version and introduces
# per-partnership configurations
#
# Original partnerships.py (C) 2006  Ole André Vadla Ravnås <oleavr@gmail.com>         
#
# Dr J A Gow 29/11/07
#
############################################################################

from constants import *
import cPickle as pickle
import os
import os.path
import urlparse
import pyrapi2
import libxml2
import rapicontext
import xml2util
import syncdb
import logging
import util
import characteristics
import socket
import errors

class PartnershipManager:

	def __init__(self, device):
	    
		self.logger = logging.getLogger("engine.pshipmgr.PartnershipManager")
		self.device = device

		self.current = None
		self.DevicePartnerships = [ None, None, None ]

	###############################
	# INTERNAL FUNCTIONS
	###############################

	#
	# _ReadDevicePartnerships
	#
	# This function retrieves the two device partnerships and inserts them into the DevicePartnerships
	# fields.
	#

	def _ReadDevicePartnerships(self):

		reg_entries = {}
		dangling_entries = []

		hklm = self.device.rapi_session.HKEY_LOCAL_MACHINE

		# Inspect registry entries
		
		try:
		
			self.logger.debug("ReadDevicePartnerships: reading partnerships from device registry")
			partners_key = hklm.create_sub_key(r"Software\Microsoft\Windows CE Services\Partners")
		
			for pos in xrange(1, 3):
				key = partners_key.create_sub_key("P%d" % pos)
				if key.disposition == pyrapi2.REG_OPENED_EXISTING_KEY:
					try:
						id = key.query_value("PId")
						hostname = key.query_value("PName")

						self.logger.debug("ReadDevicePartnerships: read partnership ID = %s, Hostname = %s" % (id, hostname))

						if id != 0 and len(hostname) > 0:
							self.logger.debug("_read_device: Adding entry")
							reg_entries[hostname] = (pos, id)
						
					except pyrapi2.RAPIError, e:
					
						self.logger.warn("ReadDevicePartnerships: Error getting partnership key %d from device registry: %s" % (pos, e))
				key.close()
			partners_key.close()
			
		except pyrapi2.RAPIError, e:
	
			self.logger.warn("ReadDevicePartnerships: Error opening partnership keys in remote registry (%d, %s)" % (e.err_code, e))

		# Look up the synchronization data on each
		
		self.logger.debug("ReadDevicePartnerships: querying synchronization source information from device")
		
		for ctic in self.device.rapi_session.GetConfig("Sync", "Sources").children.values():
			
			sub_ctic = self.device.rapi_session.GetConfig("Sync.Sources", ctic.type, recursive=True)

			guid        = sub_ctic.type
			storetype   = int(sub_ctic["StoreType"])
			hostname    = sub_ctic["Server"]
			description = sub_ctic["Name"]

			self.logger.debug("ReadDevicePartnerships: read source GUID = %s, Hostname = %s, Description = %s", guid, hostname, description)
			
			if storetype == PSHMGR_STORETYPE_AS:
				
				if hostname in reg_entries:

					pos, id = reg_entries[hostname]
					del reg_entries[hostname]

					# 'pos' for AS partnerships can only exist as 1 and 2. We reserve the third slot for
					# server partnerships

               				self.logger.debug("ReadDevicePartnerships: source matches partnerhip from registry.  Initializing partnership")
					
					pship = Partnership(self.device.config, pos, id, guid, hostname, description, storetype, self.device.deviceName,self.device.rapi_session)

					self.DevicePartnerships[pos-1] = pship

					self.logger.debug("ReadDevicePartnerships: querying partnerhip synchronization items (providers)")

					engine = sub_ctic.children["Engines"].children[GUID_WM5_ACTIVESYNC_ENGINE]
					for provider in engine.children["Providers"].children.values():

						self.logger.debug("ReadDevicePartnerships: found provider %s", provider["Name"])

						if int(provider["Enabled"]) != 0:
							id = None

							self.logger.debug("ReadDevicePartnerships: provider is enabled")

							if provider.type in SYNC_ITEM_ID_FROM_GUID:
								id = SYNC_ITEM_ID_FROM_GUID[provider.type]
							elif provider["Name"] == "Media":
								id = SYNC_ITEM_MEDIA
							elif provider["Name"] == "WorldMate":
								id = SYNC_ITEM_WORLDMATE

							if id == None:
								# TODO: a non-standard provider, for now we will just ignore these
								# but we should probably flag the partnership or have an "unknown" type
								self.logger.info("Unknown GUID \"%s\" for provider with name \"%s\"" % (provider.type, provider["Name"]))
								continue

							self.logger.debug("ReadDevicePartnerships: provider ID is %d", id)

							pship.devicesyncitems.append(id)
				else:

					self.logger.debug("ReadDevicePartnerships: Found dangling sync source: GUID = %s, Hostname = %s, Description = %s",
					                  guid, hostname, description)
						  
					dangling_entries.append((guid, hostname, description))
					
					
			elif storetype == PSHMGR_STORETYPE_EXCH:
				
				pos=3;
				id=0

				self.logger.debug("ReadDevicePartnerships: Found server sync source: GUID = %s, Hostname = %s, Description = %s",
						  guid, hostname, description)

				# we expect server sources to not have matching registry entries, but their seems to be
				# the normal MS confusion, so if we find a match we accept it
				if hostname in reg_entries:
					discard_pos, id = reg_entries[hostname]
					del reg_entries[hostname]

               				self.logger.debug("ReadDevicePartnerships: source matches partnerhip from registry.")

				self.DevicePartnerships[pos-1] = Partnership(self.device.config, pos, id, guid, hostname, description, storetype, self.device.deviceName,self.device.rapi_session)

			else:
				
				self.logger.debug("ReadDevicePartnerships: Found partnership with unknown storetype: GUID = %s, Hostname = %s, Description = %s",
					                  guid, hostname, description)
				self.logger.debug("ReadDevicePartnerships: - this will be ignored");

		# get rid of registry entries without a corresponding characteristic
	
		for entry in reg_entries.values():
			self.logger.info("ReadDevicePartnerships: Deleting dangling registry entry: %s", entry)
			hklm.delete_sub_key(r"Software\Microsoft\Windows CE Services\Partners\P%d" % entry[0])
		
		# Delete dangling entries that claim to be AS but do not have a registry key
		
		for entry in dangling_entries:
			self.logger.info("ReadDevicePartnerships: Deleting dangling sync source: %s", entry)
			self.device.rapi_session.RemoveConfig("Sync.Sources", entry[0])


	######################################
	# EXPORTED FUNCTIONS
	######################################

	#
	# ClearDevicePartnerships
	#
	# Remove all recorded device partnerships.If we have a current
	# bound partnership, save it before deletion
	#
	
	def ClearDevicePartnerships(self):
		
		self.logger.info("ClearDevicePartnerships: clearing all device partnership info")

		if self.current != None:
			self.current.SaveItemDB()
		self.current = None
		del self.DevicePartnerships
		self.DevicePartnerships = [ None, None, None]

	#
	# GetDevicePartnerships (formerly get_list)
	#
	# Return a list of current device partnerships
	#

	def GetList(self):
		
		pships = []
		for pship in self.DevicePartnerships:
			if pship != None:
				pships.append(pship)
		return pships
	
	#
	# GetHostBindings
	#
	# Returns an array of PSInfo structures associated with each
	# host binding.
	
	def GetHostBindings(self):
		
		self.logger.info("GetHostBindings: reading existing host bindings")
		
		psdir = os.path.join(self.device.config.config_user_dir,"partnerships")
		bindings = []
		
		# get all items in the dir.
		
		entries = os.listdir(psdir)
		
		for d in entries:
			
			# is it a file? - shouldn't be here so
			# ignore it.
			
			fullPath = os.path.join(psdir,d)
			if not os.path.isdir(fullPath):
				continue
			
			# it is a dir. Look for a psinfo.dat file
			
			infofp = os.path.join(fullPath,"psinfo.dat")
			
			# no psinfo, just ignore it
			
			if not os.path.isfile(infofp):
				continue
			
			# otherwise attempt to load the psinfo file
		
			try:
				f = open(infofp,"r")
				info = pickle.load(f)
				f.close()
			except Exception, e:
				self.logger.info("GetHostBindings: corrupted binding found at %s (%s), ignoring" % (infofp,e))
				continue
			
			bindings.append(info)
			
		return bindings
	
	#
	# QueryBindingConfiguration
	#
	# Returns an XML string containing the configuration block of a host binding
	
	def QueryBindingConfiguration(self,id,guid):
		
		# we need to load the config entries for a partnership from its
		# bindings
		
		psdir = os.path.join(self.device.config.config_user_dir,"partnerships")
		pspath = os.path.join(psdir,"PS" + "-" + str(id) + "-" + str(guid))
		
		s = ""
		if os.path.isdir(pspath):
			conffile = os.path.join(pspath,"psconfig.xml")
			try:
				f=open(conffile,"r")
				s=f.read()
				f.close()
			except Exception,e:
				self.logger.info("QueryBindingConfiguration: unable to read configuration for binding %s" % pspath)
				raise Exception("corrupt binding")
		else:
			raise Exception("Binding has no configuration")
		
		return s

	#
	# SetBindingConfiguration
	#
	# Returns an XML string containing the configuration block of a host binding
	
	def SetBindingConfiguration(self,id,guid,config):
		
		# first check that we can parse the config!
		
		try:
			node = libxml2.parseDoc(config)
		except:
			self.logger.error("SetBindingConfiguration: bad XML in configuration string")
			raise Exception("bad XML in configuration string")
		
		# we need to load the config entries for a partnership from its
		# bindings
		
		psdir = os.path.join(self.device.config.config_user_dir,"partnerships")
		pspath = os.path.join(psdir,"PS" + "-" + str(id) + "-" + str(guid))

		if os.path.isdir(pspath):
			conffile = os.path.join(pspath,"psconfig.xml")
			if os.path.isfile(conffile):
				try:
					f=open(conffile,"wb")
					f.write(config)
					f.close()
				except Exception,e:
					self.logger.info("SetBindingConfiguration: unable to write configuration for binding %s" % pspath)
					raise
			else:
				raise Exception("corrupted binding")
		else:
			raise Exception("Binding has no configuration")


	#
	# GetDevicePartnershipByID (formerly get)
	#
	# Return a device partnership by ID, if it exists
	# in the slots

	def GetDevicePartnershipByID(self, id):
				
		for pship in self.DevicePartnerships:
			if pship != None and pship.info.id == id:
				return pship
		return None

	#
	# GetCurrentPartnership (formerly get_current)
	#
	# If a current bound partnership is present, return it
	#

	def GetCurrentPartnership(self):
		return self.current

	#
	# AttemptToBind (formerly part of set_current)
	#
	# This function will load all device partnerships and then attempt to find a binding with a 
	# host partnership. If a binding is found, set the current binding and return True. If binding
	# fails, then leave the current binding open and return false. Remove any pre-existing partnership
	# info before continuing
	
	def AttemptToBind(self):
		
		self.logger.info("AttemptToBind: Reading partnerships and looking for host binding")
		
		# clear up any mess left behind
		
		self.ClearDevicePartnerships()

		# Now read the device
		
		self._ReadDevicePartnerships()
				
		partners = self.device.rapi_session.HKEY_LOCAL_MACHINE.create_sub_key(r"Software\Microsoft\Windows CE Services\Partners")
		
		# Iterate through our partnerships and try and find a binding
		
		for i in self.DevicePartnerships[0:2]:
			if i != None:
				if i.AttemptToBind():
					self.current = i
					break
				
		if self.current == None:
			self.logger.debug("AttemptToBind: No valid host bindings found for any device partnership")
			self.logger.debug("AttemptToBind: setting current partnership to None")
			partners.set_value("PCur", 0, pyrapi2.REG_DWORD)
			return False
		else:
			idx = self.DevicePartnerships.index(self.current)
			self.logger.debug("AttemptToBind: Valid host binding found for partnership in slot %d", idx + 1)
			partners.set_value("PCur", idx + 1, pyrapi2.REG_DWORD)
			
			return True

	#
	# CreateNewPartnership (formerly add)
	#
	# Allows us to create a brand new device-host binding if there is a free slot.
	# This call will also make the new binding current.
	#
	# Add a new partnership, make it current if we do not have one
	#

	def CreateNewPartnership(self, name, sync_items):

		# Basic argument checking

		if len(name) > 20:
			raise errors.InvalidArgument("name too long (20 chars max)")

		for item in sync_items:
			if not item in SYNC_ITEMS:
				raise errors.InvalidArgument("sync item identifier %d is invalid" % item)

		if not (None in self.DevicePartnerships[0:2]):
			raise errors.NoFreeSlots("all slots are currently full")

		slot = self.DevicePartnerships[0:2].index(None) + 1

		self.logger.debug("CreateNewPartnership: attempting to create new partnership in slot %d", slot)
		pship = Partnership(self.device.config, 
		                    slot, 
				    util.generate_id(), util.generate_guid(), 
				    socket.gethostname(), name,
				    PSHMGR_STORETYPE_AS,
				    self.device.deviceName,
				    self.device.rapi_session)

		for item in sync_items:
			self.logger.debug("CreateNewPartnership: adding synchronization item %s", item)
			pship.devicesyncitems.append(item)

		# Create the synchronization config data source
	
		source = characteristics.Characteristic(pship.info.guid)
		source["Name"] = pship.info.name
		source["Server"] = pship.info.hostname

		# StoreType
	
		source["StoreType"] = str(PSHMGR_STORETYPE_AS)

		ASEngines = characteristics.Characteristic("Engines")
		source.add_child(ASEngines)

		ASEngine = characteristics.Characteristic(GUID_WM5_ACTIVESYNC_ENGINE)
		ASEngines.add_child(ASEngine)

		settings = characteristics.Characteristic("Settings")
		settings["User"] = "DEFAULT"
		settings["Domain"] = "DEFAULT"
		settings["Password"] = "DEFAULT"
		settings["SavePassword"] = "1"
		settings["UseSSL"] = "0"
		settings["ConflictResolution"] = "1"
		settings["URI"] = "Microsoft-Server-ActiveSync"
		ASEngine.add_child(settings)

		providers = characteristics.Characteristic("Providers")
		ASEngine.add_child(providers)

		for item_id, item_rec in SYNC_ITEMS.items():
			item_str, item_readonly = item_rec
			item_guid = SYNC_ITEM_ID_TO_GUID[item_id]
			item_enabled = (item_id in sync_items)

			provider = characteristics.Characteristic(item_guid)
			provider["Enabled"] = str(int(item_enabled))
			provider["ReadOnly"] = str(int(item_readonly))
			provider["Name"] = item_str
			providers.add_child(provider)

		self.logger.debug("CreateNewPartnership: setting synchronization data source \n%s", source)

		self.device.rapi_session.SetConfig("Sync.Sources", source)

		# Update the registry

		self.logger.debug("add: updating device registry")
		hklm = self.device.rapi_session.HKEY_LOCAL_MACHINE

		partners_key = hklm.create_sub_key(r"Software\Microsoft\Windows CE Services\Partners")
		partners_key.set_value("PCur", pship.slot)

		key = partners_key.create_sub_key("P%d" % slot)
		key.set_value("PId", pship.info.id)
		key.set_value("DataSourceID", pship.info.guid)
		key.set_value("PName", pship.info.hostname)
		key.close()

		partners_key.close()

		# Store it
	
		self.logger.debug("add: storing partnership in slot %d", pship.slot)
		self.DevicePartnerships[pship.slot-1] = pship

		if pship.CreateNewBinding():
			self.logger.info("CreateNewPartnership: successfully saved host binding")
			
			# now, if we already are bound, do not try to bind. Otherwise bind
			# this partnership
			
			if self.current == None:
				if pship.AttemptToBind():
					self.logger.info("CreateNewPartnership: bind complete")
					self.current = pship
				else:
					self.logger.debug("CreateNewPartnership: failed to bind to new partnership")
					raise Exception("unable to bind to new partnership")
			else:
				self.logger.info("CreateNewPartnership: not rebinding as binding exists")
		else:
			self.logger.debug("CreateNewPartnership: Unable to create binding for new partnership")
			raise Exception("unable to create binding - check config dir")
		
		return pship
	
	#
	# DeleteDevicePartnership (formerly delete)
	#
	# Delete a partnership on the device and on the host, taking care to remove the binding as well
	#

	def DeleteDevicePartnership(self,id,guid):


		match = False
		for pship in self.DevicePartnerships:
			if pship != None:
				if pship.info.guid==guid and pship.info.id == id:

					hklm = self.device.rapi_session.HKEY_LOCAL_MACHINE

					if self.current == pship:
						self.logger.info("DeletePartnership: setting current partnership to 0")
						partners = hklm.create_sub_key(r"Software\Microsoft\Windows CE Services\Partners")
						partners.set_value("PCur", 0, pyrapi2.REG_DWORD)
						self.current = None
		
					if pship.storetype == PSHMGR_STORETYPE_AS:
						self.logger.info("DeletePartnership: Deleting registry entry: %s", pship.slot)
						hklm.delete_sub_key(r"Software\Microsoft\Windows CE Services\Partners\P%d" % pship.slot)

					self.logger.debug("DeletePartnership: removing partnership %s from device", pship)
					self.device.rapi_session.RemoveConfig("Sync.Sources", pship.info.guid)

					self.logger.debug("DeletePartnership: cleaning up partnership")
					pship.DeleteBinding()
					self.DevicePartnerships[pship.slot-1] = None
					match=True
					break;

		if not match:
			raise errors.InvalidArgument("Invalid partnership %s" % pship)



###############################################################################
# PSInfo
#
# Class containing stored information about a partnership. This is designed 
# for ease of storage
#
###############################################################################

class PSInfo:
	
	def __init__(self):

		folder_items = ( (0, "Inbox", 2),
				 (0, "Drafts", 3),
				 (0, "Deleted Items", 4),
				 (0, "Sent Items", 5),
				 (0, "Outbox", 6),
				 (0, "Tasks", 7),
				 (0, "Calendar", 8),
				 (0, "Contacts", 9),
				 (0, "Notes", 10),
				 (0, "Journal", 11),
				 (0, "Junk E-mail", 12),
				)
		
		self.id   = None
		self.guid = None
		self.name = ""
		self.hostname = ""
		self.devicename = ""
		
		self.lastSyncItems = []
		
		self.folders = {}		# used by Airsync

		for item in folder_items:
			id = util.generate_guid()
			self.folders[id] = item
	
	#
	# isEqual
	#
	# Allows us to compare the core items of a partnership to determine if it is equal

	def isEqual(self, b):
		
		if self.id == b.id and self.guid == b.guid and self.name == b.name:
			return True
		else:
			return False
 	
	#
	# Debug dumps...
	#
	
	def __str__(self):
		return "id=%#x, guid=\"%s\", hostname=\"%s\", name=\"%s\"" % (self.id, self.guid, self.hostname,self.name)

	
###############################################################################
#
# Partnership
#
# This is an internal representation of a partnership. It may or may not
# be bound (a link set up between host and device). A partnership is latent
# if not bound, and can not be used in a sync.

class Partnership:
	
	def __init__(self, config, slot, id, guid, hostname, name, storetype, devicename, rapisession):
	
		self.logger = logging.getLogger("engine.pshipmgr.Partnership")

		# Partnership identification

		self.info            = PSInfo()
		self.info.id         = id
		self.info.guid       = guid
		self.info.name       = name
		self.info.hostname   = hostname
		self.info.devicename = devicename

		# is the host-device relationship established? 
		# is the itemDB loaded?

		self.isBound = False
		self.itemDBLoaded = False
		
		# store type
		
		self.storetype = storetype
		
		# configuration (established during bind)
		
		self.config = None
		self.xpc = None
		self.rapisession = rapisession

		# sync data
		
		self.slot = slot		# slot in which partnership is held?
		self.devicesyncitems = []	# formerly sync_items
		self.deviceitemdbs = {}		# item databases

		self.psdir       = os.path.join(config.config_user_dir,"partnerships")
		self.pshippath   = os.path.join(self.psdir, "PS" + "-" + str(self.info.id) + "-" + str(self.info.guid))
		self.configpath  = os.path.join(self.pshippath, "psconfig.xml")
		self.infopath    = os.path.join(self.pshippath, "psinfo.dat")
		self.idbpath     = os.path.join(self.pshippath, "IDB")
		
		# make sure we have a partnerships dir!
		
		if not os.path.isdir(self.psdir):
			try:
				os.makedirs(self.psdir)
			except:
				self.logger.error("Unable to create partnership repository in config")

	#
	# _RunPartnershipConfig
	#
	# Run the per-partnership configuration
	#
	
	def _RunPartnershipConfig(self):
		
		# 1. Do we need to sync the time to the PC?
		
		doSyncTime = self.QueryConfig("/syncpartner-config/General/SyncTimeToPc[position() = 1]","0")
		if doSyncTime == "1":
			self.logger.info("synchronization of time to host is enabled")
			self.rapisession.SyncTimeToPc()
		else:
			self.logger.info("synchronization of time to host is disabled")
			
		# 2. Set up DTPT
		
		allowDTPTMH = self.QueryConfig("/syncpartner-config/DTPT/EnableMultihoming[position() = 1]","0")
		if allowDTPTMH == "1":
			value = 1
			self.logger.info("DTPT multihoming is enabled")
		else:
			value = 0
			self.logger.info("DTPT multihoming is disabled")
			
		cticset = characteristics.Characteristic("Settings",None)
		cticset.params["AllowDTPTMultihoming"]=str(value)
		self.rapisession.SetConfig("Sync",cticset)
		
		# 3. Set up the connected network
		# Mimic ActiveSync: http://blogs.msdn.com/windowsmobile/archive/2006/11/20/wmdc-activesync-pass-through-feature.aspx
		
		network = self.QueryConfig("/syncpartner-config/DTPT/Network[position() = 1]","0")
		if network and network in NETWORK_GUID:
			self.logger.info("DTPT network is set to %s" % network)
			self._SetDTPTnet(network)
		else:
			self.logger.info("DTPT network is set to automatic, unset or unknown -> automatic config")
			proxy = self._GetLocalProxy()
			if proxy["http"]:
				self._SetDTPTnet("work")
				self._SetProxy(proxy)
			else:
				self._SetDTPTnet("internet")
				self._SetProxy(None)

		# config done

	def _GetLocalProxy(self):
		"""TODO: read proxy settings from elsewhere too (GConf, KConfig, Firefox, ...)"""
		proxy = { "http": os.getenv("http_proxy", None),
		          "no_proxy": os.getenv("no_proxy", None),
		        }
		return proxy

	def _SetDTPTnet(self, network):
		"""
		This setting is persistant between syncs. Must be called at least once
		in the device's lifetime to get DTPT working.
		"""
		cticset = characteristics.Characteristic("CurrentDTPTNetwork", None)
		cticset.params["DestId"] = NETWORK_GUID[network]
		self.rapisession.SetConfig("CM_NetEntries", cticset)

	def _SetProxy(self, proxy):
		"""
		Sends the proxy configuration to the device.  Assumes the connection
		source is "work", it seems to be ActiveSync's behavior.
		"""
		if proxy is None:
			current_proxy = self.rapisession.GetConfig("CM_ProxyEntries",
			                                           "HTTP-%s" % NETWORK_GUID["work"])
			if "Enable" in current_proxy.params and \
					current_proxy.params["Enable"] == "1":
				self.rapisession.RemoveConfig("CM_ProxyEntries",
				                              "HTTP-%s" % NETWORK_GUID["work"])
		else:
			self._SetHTTPproxy(proxy["http"])
			self._SetNoProxy(proxy["no_proxy"])

	def _SetHTTPproxy(self, proxy):
		cticset = characteristics.Characteristic("HTTP-%s" % NETWORK_GUID["work"],
		                                         None)
		cticset.params["SrcId"] = NETWORK_GUID["work"]
		cticset.params["DestId"] = NETWORK_GUID["internet"]
		cticset.params["Type"] = "1"
		cticset.params["Enable"] = "1"
		proxy_obj = urlparse.urlparse(proxy)
		cticset.params["Proxy"] = proxy_obj.netloc
		if proxy_obj.username:
			cticset.params["UserName"] = proxy_obj.username
		if proxy_obj.password:
			cticset.params["Password"] = proxy_obj.password
		self.rapisession.SetConfig("CM_ProxyEntries", cticset)
	
	def _SetNoProxy(self, no_proxy):
		order = 10000 # must be > 500 (http://msdn.microsoft.com/en-us/library/aa455850.aspx)
		for no_proxy_host in no_proxy.split(","):
			no_proxy_host = no_proxy_host.strip() # may have spaces after commas
			cticset = characteristics.Characteristic(str(order), None)
			pattern = ["*://"] # use lists because adding to strings is slow (PEP-8)
			if no_proxy_host.startswith("."):
				pattern.append("*")
			pattern.append(no_proxy_host)
			pattern.append("/*")
			cticset.params["Pattern"] = "".join(pattern)
			cticset.params["Network"] = NETWORK_GUID["work"]
			self.rapisession.SetConfig("CM_Mappings", cticset)
			order += 1


	#
	# __str__
	#
	# Debug dumping
	#

	def __str__(self):
		
		str = ""
		for id in self.devicesyncitems:
			if str:
				str += ", "
			else:
				str = "[ "
			str += SYNC_ITEMS[id][0]
		str += " ]"

		return "P%d: id=%#x, guid=\"%s\", hostname=\"%s\", name=\"%s\", sync_items=%s" % \
			(self.slot, self.info.id, self.info.guid, self.info.hostname, self.info.name, str)


	####################################
	## INTERNAL FUNCTIONS
	####################################
	
	#
	# _CreateNewConfig
	#
	# Create a new config structure for the partnership
	#
	
	def _CreateNewConfig(self):
		
		self.config = libxml2.newDoc("1.0")
		confnode = self.config.newChild(None,"syncpartner-config",None)
		generalnode = confnode.newChild(None,"General",None)
		synctime = generalnode.newChild(None,"SyncTimeToPc", "0")
		filepath = generalnode.newChild(None,"LocalFilePath", self.info.name)
		dtptnode = confnode.newChild(None,"DTPT",None)
		dtptnode.newChild(None,"Enabled","1")
		dtptnode.newChild(None,"EnableMultihoming","0")
		comment = self.config.newDocComment(' "auto", "internet" or "work". ')
		dtptnode.addChild(comment)
		dtptnode.newChild(None,"Network","auto")
		
		if not self._SaveConfigFile():
			self.logger.warning("unable to save config info for partnership %s", str(self.id))
	
	#
	# _LoadConfigFile
	#
	# Load the configuration for a partnership when binding. If no file exists, create a 
	# default one.
	#

	def _LoadConfigFile(self):
	
		try:
			self.config = libxml2.parseFile(self.configpath)
		except:
			self.logger.info("load_config: No previous config file available, creating default")
			self._CreateNewConfig()
		
		self.xpc = self.config.xpathNewContext()

	#
	# _SaveConfigFile
	#
	# Attempts to save the config data as an XML file
	#

	def _SaveConfigFile(self):
		
		try:
			f = open(self.configpath,"wb")
			f.write(self.config.serialize("utf-8",0))
			f.close()
		except Exception, e:
			self.logger.warning("failed to save file for partnership configuration")
			return False
	
		return True

	#####################################
	## EXPORTED FUNCTIONS
	#####################################

	#
	# AttemptToBind
	#
	# This function takes an unbound partnership and looks to see if we have a previously
	# established host-device relationship. If one is found, the bind information is 
	# loaded from the config and the partnership initialized. At this point it is said
	# to be 'bound' and can be used to sync.
	#
	# If no host-device relationship can be found, the function will return
	#
	# If a corrupted binding is found on the host side, then the host side binding
	# is deleted and the function continues to search.
	
	
	def AttemptToBind(self):
		
		# Partnerships with a store-type of PSHMGR_STORETYPE_EXCH can not be
		# bound by sync-engine
		
		if self.storetype == PSHMGR_STORETYPE_EXCH:
			self.logger.info("unable to bind Exchange Server partnership %s" % hostname);
			return False
		
		# The path will have already been constructed by the initializer. Check
		# if it exists. If not, we have no binding.
		
		if not os.path.isdir(self.pshippath):
			return False
		
		# See if we can open the info file.
		
		try:
			f=open(self.infopath,"r")
			bindinfo = pickle.load(f)
			f.close()
		except:
			self.logger.error("corrupt/nonexistent partnership info file at %s - deleting directory" % self.infopath)
			util.deltree(self.pshippath)
			return False

		
		# we have the info file. Check it!
		
		if self.info.isEqual(bindinfo):
			
			# the binding looks good!. Load the config file
			
			self._LoadConfigFile()
			
			# Bind the partnership
			
			self.info = bindinfo
			
			# ItemDBs are loaded when needed. The tendency for Airsync to send 
			# everything down the wire when an item is deselected then subsequently
			# selected is handled in the itemDB

			self.info.lastSyncItems = self.devicesyncitems

			# Binding is now complete
			
			self.isBound = True
			
			# Now save out all our info
			
			try:
				f=open(self.infopath,"wb")
				pickle.dump(self.info, f, pickle.HIGHEST_PROTOCOL)
				f.close()
			except Exception, e:
				self.logger.error("AttemptToBind: Failed to save sync info: ", e)

			self.logger.info("AttemptToBind : bound partnership %s - running config" % str(self))

			# Now the partnership is bound, run the per-partnership config
			
			self._RunPartnershipConfig()

			return True

		else:
			# if we get here, the partnership was not ours, but the id/guid combination
			# has placed us in this path. There is something very wrong here, but we can
			# fix it by removing the broken host binding info and reporting a failed bind.
		
			util.deltree(self.pshippath)
			self.logger.warning("bind: broken partnership (%d) configuration info found on host - removing traces", self.info.id)
			
			return False

	#
	# CreateNewBinding
	#
	# Used when we create a new partnership. This creates and updates a new binding
	#
	
	def CreateNewBinding(self):
		
		if self.storetype != PSHMGR_STORETYPE_AS:
			self.logger.error("CreateNewBinding: Exchange Server partnerships can not be bound")
			return False
		
		# The path will have already been constructed by the initializer. Check
		# if it exists. If so, we must clean it up ready for the new binding. If not, 
		# create it.
		
		if os.path.isdir(self.pshippath):
			deltree(self.pshippath)
			
		try:
			os.makedirs(self.pshippath)
		except Exception,e:
			self.logger.error("CreateNewBinding: unable to create bind info directory (%s)",e)
			return False
					
		# Generate a config.
		
		self._CreateNewConfig()
		
		# save out our info
					
		try:
			f=open(self.infopath,"wb")
			pickle.dump(self.info, f, pickle.HIGHEST_PROTOCOL)
			f.close()
		except Exception, e:
			self.logger.error("CreateNewBinding: Failed to save sync info: ", e)
			return False
		
		# mark us as successfully bound.
		
		self.isBound = True
		
		return True

	#
	# DeleteBinding
	#
	# Remove a host binding for this partnership
	
	def DeleteBinding(self):
		
		if self.storetype != PSHMGR_STORETYPE_AS:
			self.logger.info("DeleteBinding: Exchange Server partnerships have no binding - doing nothing")
			return False
		
		self.logger.info("DeleteBinding: deleting host binding for partnership %s",self.info.name)
		
		if self.isBound:
			self.isBound=False
		
		util.deltree(self.pshippath)


	#
	# QueryIsBound
	#
	# Returns true or false depending on whether the partnership is currently bound
	
	def QueryIsBound(self):
		return self.isBound
	

	#
	# QueryItemDB
	#
	# If an itemDB is available for a specific sync item, then  return it
	
	def QueryItemDB(self, itemid):
		
		if self.deviceitemdbs.has_key[itemid]:
			return self.deviceitemdbs[itemid]
		else:
			return None

	#
	# QueryConfig
	#
	# Used to query the content of a config file entry
	#

	def QueryConfig(self,path,default):

		nodelist = self.xpc.xpathEval(path)
		for n in nodelist:
			if n.type == "element":
				if n.content != None:
					return n.content
				else:
					return default
		return default
	
	#
	# Sync state information is now retained in the individual itemDBs. These are demand-loaded
	#

	#
	# LoadItemDB
	#
	# Load the item database for the items we have

	def LoadItemDB(self):
		
		if self.isBound:
			for item in self.devicesyncitems:
				self.logger.info("loading itemDB for item %d" % item)
				itemfile = self.idbpath + str(item)
				try:
					f = open(itemfile,"r")
					self.deviceitemdbs[item] = pickle.load(f)
					f.close()
				except:
					# if we can't open an existing one, create a new one.
					self.logger.info("LoadItemDB: no DB available for item %d",item)
					self.deviceitemdbs[item] = syncdb.ItemDB(item)
					
			self.itemDBLoaded = True
			return True
					
		else:
			self.logger.error("LoadItemDB: Partnership is unbound")
			return False

	#
	# SaveItemDB
	#
	# Save the item database for the items we have

	def SaveItemDB(self):
		if self.isBound:
			for itemid in self.deviceitemdbs.keys():
				self.deviceitemdbs[itemid].Save(self.idbpath)
			return True
		else:
			
			self.logger.error("SaveItemDB: Partnership is unbound")
			return False

	#
	# FlushItemDB
	#
	# Clears the item database from memory between syncs

	def FlushItemDB(self):
		
		self.logger.info("FlushItemDB: Flushing all itemDBs")
		self.itemDBLoaded = False
		del self.deviceitemdbs
		self.deviceitemdbs = {}


