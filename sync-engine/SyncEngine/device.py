
import dbus
import logging
import rrasyncmanager
import auth
import rapicontext
import pyrapi2
import dtptserver
import pshipmgr
import errors
import re
import gobject

from mutex import mutex
from constants import *
from airsync import AirsyncThread
from synchandler import SyncHandler
from config import Config

#
#
#
# Device
#
# This object represents a connected device
#

class Device(gobject.GObject):

	__gsignals__ = {
		"connected": (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, () ),
		"disconnected": (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, () ),
		"prefill-complete"  : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, () ),
		"synchronized"  : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, () ),
		"partnerships-changed"  : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, () ),

		"status-sync-start": (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, () ),
		"status-sync-end"  : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, () ),
		"status-sync-start-partner": (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, (gobject.TYPE_STRING,) ),
		"status-sync-end-partner"  : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, (gobject.TYPE_STRING,) ),
		"status-sync-start-datatype": (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, (gobject.TYPE_STRING, gobject.TYPE_STRING,) ),
		"status-sync-end-datatype"  : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, (gobject.TYPE_STRING, gobject.TYPE_STRING,) ),
		"status-set-progress-value"  : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, (gobject.TYPE_UINT,) ),
		"status-set-max-progress-value"  : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, (gobject.TYPE_UINT,) ),
		"status-set-status-string"  : (gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, (gobject.TYPE_STRING,) ),
		}


	#
	#
	# Initialization

	def __init__(self,config,objpath):

		self.__gobject_init__()

		# reference to the app's Config object
		self.config = config
		# odccm or udev object path to the device
		self.objpath = objpath

		self.logger = logging.getLogger("engine.syncengine.device")

		# if we are ready to communicate
		self.isConnected = False

		# device partnership information
		self.PshipManager = pshipmgr.PartnershipManager(self)

		self.synchandler = None

		self.syncing = mutex()

		# RAPI connection to the device
		self.rapi_session = None	

#		self.rra = None
		# RRA connection to the device
		self.RRASession = rrasyncmanager.RRASyncManager(self)

		# DTPT (desktop passthrough) for the device
		self.dtptsession = None

		self.airsync = None
		self.sync_begin_handler_id = None
		self.autosync_triggered = False

#		self.device = None
		self.deviceName = ""
		self.devicePath = ""
		self.iface_addr = ""
		self.partnerships = None

		if "org.synce.odccm" in objpath:
			device_obj = dbus.SystemBus().get_object("org.synce.odccm",objpath)
			self.dev_iface = dbus.Interface(device_obj,"org.synce.odccm.Device")
			self.iface_addr = "0.0.0.0"
		else:
			device_obj = dbus.SystemBus().get_object("org.synce.dccm", objpath)
			self.dev_iface = dbus.Interface(device_obj,"org.synce.dccm.Device")
			self.iface_addr = self.dev_iface.GetIfaceAddress()

		self.dev_iface.connect_to_signal("PasswordFlagsChanged", self._CBDeviceAuthStateChanged)
		self.name = self.dev_iface.GetName()
		self.logger.info(" device %s connected" % self.name)
		self.devicePath = objpath

	#
	# _CBDeviceAuthStateChanged
	#
	# INTERNAL
	#
	# Callback triggered when a device authorization state is changed
	#

	def _CBDeviceAuthStateChanged(self,added,removed):
			
		self.logger.info("_CBDeviceAuthStateChanged: device authorization state changed: reauthorizing")
		if not self.isConnected:
			if self._ProcessAuth():
				self.OnConnect()


	#
	# _ProcessAuth
	#
	# INTERNAL
	#
	# Process authorization on either callback or initial connection

	def _ProcessAuth(self):

		self.logger.info("ProcessAuth : processing authorization for device '%s'" % self.name) 
		rc=True
		if auth.IsAuthRequired(self.dev_iface):
		
			# if we suddenly need auth, first shut down all threads if they
			# are running

			if self.PshipManager.GetCurrentPartnership() != None:
				self.OnDisconnect()

			result = auth.Authorize(self.devicePath,self.dev_iface,self.config.config_Global)
			if result == 0:
				self.logger.info("Authorization pending")
				rc = False
			elif result == 1:
				self.logger.info("Authorization successful - reconnecting to device")
				self.isConnected = True
			elif result == 2:
				self.logger.info("Authorization pending - waiting for password on device")
				rc = False
			else:
				self.logger.info("Failed to authorize - disconnect and reconnect device to try again")
				rc = False
		else:
			self.logger.info("ProcessAuth: authorization not required for device '%s'" % self.name)
			self.isConnected = True

		return rc

	# 
	# OnConnect
	#
	# Called when device is firmly established. Sets up the RAPI connection
	# and then starts the sync handler sessions
	#

	def OnConnect(self):
	
		# ensure current state is set to defaults

		self._ResetCurrentState()

		# and start the sessions

		self.logger.debug("OnConnect: setting up RAPI session")
		self.rapi_session = rapicontext.RapiContext(self.name, pyrapi2.SYNCE_LOG_LEVEL_DEFAULT)

		self.logger.debug("OnConnect: Attempting to bind partnerships")
		self.PshipManager.AttemptToBind()

		# don't start any sessions if we don't have a valid partnership.

		try:
			self._CheckAndGetValidPartnership()
			self.StartSessions()

		except Exception,e:
			self.logger.debug("OnConnect: No valid partnership bindings are available, please create one (%s)" % str(e))
			pass

		self.isConnected = True


	#
	# OnDisconnect
	#
	# Called when the device disconnects from the bus. Ensures all sessions
	# are cleanly shut down.

	def OnDisconnect(self):

		self.StopSessions()
		self.WaitForStoppingSessions()

		self.logger.debug("OnDisconnect: closing RAPI session")
		self.rapi_session = None

		self.logger.debug("OnDisconnect: clearing partnerships")
		self.PshipManager.ClearDevicePartnerships()
	
		self.isConnected = False
		
	#
	# StartSessions
	#
	# Performs the mechanics of actually starting the sync handler sessions
	#
	
	def StartSessions(self):

		# We know we have a valid partnership if we get here, so run the config
		# without looking for exceptions
	
		pship = self.PshipManager.GetCurrentPartnership()
	
		# check if DTPT is enabled for this partnership - if so, start it
	
		mh = pship.QueryConfig("/syncpartner-config/DTPT/Enabled[position()=1]","0")
		gh = self.config.config_Global.cfg["EnableDTPT"]
	
		#### we MUST change this to bind to the device address only!
	
		if mh == "1" and gh == 1:
			self.logger.debug("StartSessions: DTPT starting")
			self.dtptsession = dtptserver.DTPTServer(self.iface_addr)
			self.dtptsession.start()
		else:
			self.dtptsession = None

		self.logger.debug("StartSessions: starting AirSync handler")
		self.airsync = AirsyncThread(self)
        	self.sync_begin_handler_id = self.airsync.connect("sync-begin", self._CBStartDeviceTriggeredSync)
        	self.airsync.start()

		self.logger.debug("StartSessions: calling RAPI start_replication")
		self.rapi_session.start_replication()

		# The device will never trigger an autosync, or attempt to sync, until
		# sync_resume is called.

		self.logger.debug("StartSessions: calling RAPI sync_resume")
		self.rapi_session.sync_resume()

		self.logger.debug("StartSessions: starting RRA session")
		self.RRASession.StartRRAEventHandler()

	#
	# StopSessions
	#
	# Triggers all sync session threads and servers to stop. It does not
	# wait for a stop.

	def StopSessions(self):

		self.logger.debug("StopSessions: stopping RRA server")
		self.RRASession.StopRRAEventHandler()
		
		if self.dtptsession != None:
			self.logger.debug("StopSessions: stopping DTPT server")
			self.dtptsession.shutdown()
	
		if self.synchandler != None:
			self.logger.debug("StopSessions: stopping sync handler thread")
			self.synchandler.stop()

		if self.airsync != None:
			self.logger.debug("StopSessions: stopping Airsync server")
			self.airsync.stop()

	#
	# WaitForStoppingSessions
	#
	# Once StopSessions has been called, this function can be called to wait
	# until all threads and servers have actually stopped
	#
		
	def WaitForStoppingSessions(self):

		if self.dtptsession != None:
        		self.logger.debug("WaitForStoppingSessions: waiting for DTPT server thread")
			self.dtptsession.join()
			self.dtptsession = None

		if self.synchandler != None:
			self.logger.debug("WaitForStoppingSessions: waiting for sync handler thread")
			self.synchandler.join()
			self.synchandler = None

		if self.airsync != None:
			self.logger.debug("WaitForStoppingSessions: waiting for Airsync server thread")
			self.airsync.join()
			self.sync_begin_handler_id = None
			self.airsync = None

		self.logger.debug("sessions_wait_for_stop: shutting down RRA server")
		self.RRASession.StopRRAEventHandler()

	# _CBStartDeviceTriggeredSync
	#
	# Called to trigger a device-triggered sync autosync, either a manual sync from the device,
	# or from the timer
	#

	def _CBStartDeviceTriggeredSync(self, res):

		pship=self._CheckAndGetValidPartnership()
	
		if not self.syncing.testandset():
			raise errors.SyncRunning
		
		self.logger.info("_CBStartDeviceTriggeredSync: monitoring auto sync with partnership %s", pship)
	
		if not pship.itemDBLoaded:
			pship.LoadItemDB()
		
		self.logger.info("_CBStartDeviceTriggeredSync: itemDB loaded")

		self.synchandler = SyncHandler(self, True)
		self.synchandler.start()
	
		if self.config.config_AutoSync.cfg["Disable"] == 0:
		
			cmd_list = self.config.config_AutoSync.cfg["AutoSyncCommand"]
				
			if len(cmd_list) > 0:
				self.logger.info("_CBStartDeviceTriggeredSync: command %s" % cmd_list[0])
				try:
					self.autosync_triggered = True	
					pid = os.spawnvp(os.P_NOWAIT,cmd_list[0],cmd_list)
					self.logger.info("_CBStartDeviceTriggeredSync: process spawned with PID %d" % pid)
				except:
					self.autosync_triggered = False
					self.logger.debug("_CBStartDeviceTriggeredSync : failed to spawn process : cmd=%s" % cmd_list[0])
		else:
			self.logger.debug("_CBStartDeviceTriggeredSync : device triggered sync disabled in config")

	#
	# _CheckAndGetValidPartnership
	#
	# INTERNAL
	#
	# Utility function to retrieve the current partnership. Will throw if 
	# the system is currently unbound.
	#

	def _CheckAndGetValidPartnership(self):

		pship = self.PshipManager.GetCurrentPartnership()
		if pship is None:
			raise errors.NoBoundPartnership
		return pship

	#
	# _ResetCurrentState
	#
	# INTERNAL
	#
	# Called to reset any internal state objects to defaults, usually
	# called on disconnect. Just so we can put them all in one place

	def _ResetCurrentState(self):
		
		self.autosync_triggered=False
		self.syncing.unlock()

	# emit signals
	#
	def PrefillComplete(self):
		self.emit("prefill-complete")

	def Synchronized(self):
		self.emit("synchronized")

	def PartnershipsChanged(self):
		self.emit("partnerships-changed")

	def StatusSyncStart(self):
		self.emit("status-sync-start")

	def StatusSyncEnd(self):
		self.emit("status-sync-end")

	def StatusSyncStartPartner(self, partner):
		self.emit("status-sync-start-partner", partner)

	def StatusSyncEndPartner(self, partner):
		self.emit("status-sync-end-partner", partner)

	def StatusSyncStartDatatype(self, partner, datatype):
		self.emit("status-sync-start-datatype", partner, datatype)

	def StatusSyncEndDatatype(self, partner, datatype):
		self.emit("status-sync-end-datatype", partner, datatype)

	def StatusSetProgressValue(self, progress_current):
		self.emit("status-set-progress-value", progress_current)

	def StatusSetMaxProgressValue(self, progress_max):
		self.emit("status-set-max-progress-value", progress_max)

	def StatusSetStatusString(self, status):
		self.emit("status-set-status-string", status)

gobject.type_register(Device)
