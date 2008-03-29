# -*- coding: utf-8 -*-
############################################################################
#    New file: Dr J A Gow 18/2/2007                                        #
#              Bundle all the config stuff in one place.                   #
#                                                                          #
#    This program is free software; you can redistribute it and#or modify  #
#    it under the terms of the GNU General Public License as published by  #
#    the Free Software Foundation; either version 2 of the License, or     #
#    (at your option) any later version.                                   #
#                                                                          #
#    This program is distributed in the hope that it will be useful,       #
#    but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#    GNU General Public License for more details.                          #
#                                                                          #
#    You should have received a copy of the GNU General Public License     #
#    along with this program; if not, write to the                         #
#    Free Software Foundation, Inc.,                                       #
#    59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             #
############################################################################

import cPickle as pickle
import os
import os.path
import getopt
import string
import logging
import libxml2
import xml2util
import shutil
from formatapi import SupportedFormats,DefaultFormat

#
# Debug log levels

DBLOGLEVELS = { "INFO"    : logging.INFO,
		"DEBUG"   : logging.DEBUG,
		"WARNING" : logging.WARNING,
		"ERROR"   : logging.ERROR
	      }

#
#
# ValidateCommandOptions
#
# Validate command line options.

def ValidateCommandOptions(olist):
	
	try:
		opts,args = getopt.getopt(olist,
					  "hc:ov:l:d",
					  ["help","config=","once","verbosity=","logfile=","detached"])
		if len(args) > 1:
			raise Exception("cmderr")
		for opt,arg in opts:
			if opt in ("-h","--help"):
				raise Exception("usage")
	except:
		print "Usage: sync-engine [opts]"
		print
		print "Options"
		print "-------"
		print " -h, --help          Display usage info"
		print
		print " -c<confdir>,"
		print " --config=<confdir>  Use <confdir> as the sync-engine config location"
		print
		print " -o, --once          Run for one connection only"
		print 
		print " -v<arg>,"
		print " --verbosity=<arg>   Set the log level to one of:"
		print "                     INFO,DEBUG,ERROR"
		print " -l<file>,"
		print " --logfile=<file>    Set the log file to <file>. If not specified"
		print "                     will log to stdout"
		print
		print " -d, --detached      Daemonize the sync-engine"
		raise
	
	return opts,args


############################################################################
# ConfigObject
#
# Base configuration object. Each configuration block is handled by one
# instance of a derived class of this object
#
############################################################################

class ConfigObject:
	def __init__(self):
		self.handlers = {}
		self.cfg = {}

############################################################################
# FileSyncConfig
#
# Configuration object for the file synchronization subsystem
#
############################################################################

class FileSyncConfig(ConfigObject):
	
	def __init__(self):
		ConfigObject.__init__(self)
		self.logger = logging.getLogger("FileSyncConfig")
		self.LocalFilePath=""
		self.Disabled = 1
		self.handlers = { "BaseFilePath" : self.validate_BaseFilePath,
				  "Disable" : self.validate_Disable,
				  "LocalUpdateFreq" : self.validate_UpdateFreq, 
				  "ExtraDeleteDelay" : self.validate_DeleteDelay,
				  "ObjectReportTimeout" : self.validate_ObjectReportTimeout
				}
				
		self.cfg = { "BaseFilePath" : "~/PDAFiles",
		             "Disable" : 1,
			     "LocalUpdateFreq" : 10,
			     "ExtraDeleteDelay" : 0,
			     "ObjectReportTimeout" : 8
			   }
	
	def validate_BaseFilePath(self,arg):
		self.cfg["BaseFilePath"] = os.path.expanduser(arg)
		
	def validate_Disable(self,arg):
		try:
			self.cfg["Disable"] = int(arg)
		except:
			self.logger.debug("'Disable': invalid argument %s in config file" % arg)
			
	def validate_UpdateFreq(self,arg):
		try:
			self.cfg["LocalUpdateFreq"] = int(arg)
		except:
			self.logger.debug("'LocalUpdateFreq': invalid argument %s in config file" % arg)

	def validate_DeleteDelay(self,arg):
		try:
			self.cfg["ExtraDeleteDelay"] = int(arg)
		except:
			self.logger.debug("'ExtraDeleteDelay': invalid argument %s in config file" % arg)
	
	def validate_ObjectReportTimeout(self,arg):
		try:
			self.cfg["ObjectReportTimeout"] = int(arg)
		except:
			self.logger.debug("'ObjectReportTimeout': invalid argument %s in config file" % arg)
	
	def dump(self):
		print "File-sync config: "
		print "   local path           %s " % self.cfg["BaseFilePath"]
		print "   disable?             %d " % self.cfg["Disable"]
		print "   LocalUpdateFreq      %d " % self.cfg["LocalUpdateFreq"]
		print "   ExtraDeleteDelay     %d " % self.cfg["ExtraDeleteDelay"]

############################################################################
# AutoSyncConfig
#
# Configuration object for autosync
#
############################################################################

class AutoSyncConfig(ConfigObject):
	def __init__(self):
		ConfigObject.__init__(self)
		self.logger = logging.getLogger("AutoSyncConfig")
		
		self.handlers = { "AutoSyncCommand" : self.validate_AutoSyncCommand,
				  "Disable"         : self.validate_Disable
				}
				
		self.cfg = { "AutoSyncCommand" : [],
		             "Disable" : 1
			   }
	
	def validate_AutoSyncCommand(self,arg):
		self.cfg["AutoSyncCommand"] = string.split(arg)
		
	def validate_Disable(self,arg):
		try:
			self.cfg["Disable"] = int(arg)
		except:
			self.logger.debug("'Disable': invalid argument %s in config file" % arg)
			
	def dump(self):
		self.logger.info("Autosync config: ")
		if len(self.cfg["AutoSyncCommand"]) > 0: 
			self.logger.info("   Autosync command    L%s " % self.cfg["AutoSyncCommand"])
		self.logger.info("   disable?             %d " % self.cfg["Disable"])

############################################################################
# GlobalConfig
#
# Global configuration object for autosync
#
############################################################################

class GlobalConfig(ConfigObject):
	def __init__(self):
		ConfigObject.__init__(self)
		self.logger = logging.getLogger("GlobalConfig")
		
		self.handlers = { "SlowSyncDisable" : self.validate_SlowSyncEnable,
		                  "AuthMethod"      : self.validate_AuthMethod,
				  "AppendDefaultTimezone" : self.validate_AppendDefaultTimezone,
				  "OpensyncXMLFormat" : self.validate_OpensyncXMLFormat,
				  "FlushIDB" : self.validate_FlushIDB,
				  "EnableDTPT" : self.validate_EnableDTPT }
				  
		self.cfg = { "SlowSyncDisable" : 0,
		             "AuthMethod"      : "INTERNAL_CLI",
			     "AppendDefaultTimezone" : 0,
			     "OpensyncXMLFormat" : DefaultFormat,
			     "FlushIDB" : 1,
			     "EnableDTPT" : 1
			   }
			
	def validate_SlowSyncEnable(self,arg):
		try:
			self.cfg["SlowSyncDisable"] = int(arg)
		except:
			self.logger.debug("'SlowSyncDisable': invalid argument %s in config file" % arg)
			
	def validate_AuthMethod(self,arg):
		self.cfg["AuthMethod"] = arg
			
	def validate_AppendDefaultTimezone(self,arg):
		try:
			self.cfg["AppendDefaultTimezone"] = int(arg)
		except:
			self.logger.debug("'Disable': invalid argument %s in config file" % arg)
		
	def validate_OpensyncXMLFormat(self,arg):
		if arg in SupportedFormats:
			self.cfg["OpensyncXMLFormat"] = arg
		else:
			self.logger.debug("'OpensyncXMLFormat': no such format type '%s', substituting default" % arg)

	def validate_FlushIDB(self,arg):
		try:
			self.cfg["FlushIDB"] = int(arg)
		except:
			self.logger.debug("'Disable': invalid argument %s in config file" % arg)

	def validate_EnableDTPT(self,arg):
		try:
			self.cfg["EnableDTPT"] = int(arg)
		except:
			self.logger.debug("'Disable': invalid argument %s in config file" % arg)

	def dump(self):
		self.logger.info("Global config: ")
		self.logger.info("   SlowSyncDisable: %d " % self.cfg["SlowSyncEnable"])

############################################################################
# Config
#
# Configuration subsystem object. This object houses the configuration
# system, and it is from here that all configuration objects are referenced
# by other components of the sync-engine
#
############################################################################

class Config:
	
	def __init__(self,progopts):
		
		self.logger = logging.getLogger("engine.config.Config")
		self.config_FileSync = FileSyncConfig()
		self.config_AutoSync = AutoSyncConfig()
		self.config_Global   = GlobalConfig()

		# Now, get our current path (this may be useful)
		# Avoids messing about with having to pull a PREFIX from
		# an installer
		
		self.sepath = str(xml2util).split()[3][1:-2]
		self.sepath = os.path.dirname(self.sepath)	# lose module name
		self.sepath = os.path.dirname(self.sepath)	# lose SyncEngine
		
		# check if we have a config

		self.config_dir   = os.path.join(os.path.expanduser("~"), ".synce")
		self.config_path  = os.path.join(self.config_dir, "config.xml")
		
		if not os.path.exists(self.config_path):
			if not os.path.isdir(self.config_dir):
				os.mkdir(self.config_dir)
			oldconf = os.path.join(self.sepath,"config/config.xml")
			shutil.copy(oldconf, self.config_path)

		
		self.logfile = None
	
		self.runonce = False
		self.fork = False
		self.loglevel = DBLOGLEVELS["DEBUG"]
		self._ProcessCommandOptions(progopts)
		
	#
	# _ProcessCommandOptions
	#
	# Process the command line options
	
	def _ProcessCommandOptions(self,olist):
		opts,args = olist
		for opt,arg in opts:
			if opt in ("-o","--once"):
				self.runonce = True
			if opt in ("-c","--config"):
				self.config_dir = os.path.join(arg,".synce")
			if opt in ("-d","--detached"):
				self.fork = True
			if opt in ("-l","--logfile"):
				self.logfile = os.path.expanduser(arg)
			if opt in ("-v","--verbosity"):
				if DBLOGLEVELS.has_key(arg):
					self.loglevel = DBLOGLEVELS[arg]
				else:
					print "error: unknown log level %s, defaulting to DEBUG" % arg
					
	
	#
	# UpdateConfig updates information that may be changed between connections. It is re-read
	# at the start of each connectiomn

	def UpdateConfig(self):
	
		try:
			f = open(self.config_path, "rb")
		except:
			self.logger.info("UpdateConfig - unable to open config file - using defaults")
			return False
		try:
			cf = f.read()
			f.close()
		except:
			self.logger.info("UpdateConfig - could not read config file - using defaults")
			return False
		
		confdoc = libxml2.parseDoc(cf)
		
		config = xml2util.FindChildNode(confdoc,"syncengine-config")
		if config is not None:
			self._ReadConfElements(config,"FileSync",self.config_FileSync)
			self._ReadConfElements(config,"Autosync",self.config_AutoSync)	
			self._ReadConfElements(config,"Global",self.config_Global)
		
		return True
		
	#
	# Read configuration elements from the XML config file (private)
	#
		
	def _ReadConfElements(self,doc, element, conf_obj):
		
		conf_element = xml2util.FindChildNode(doc,element)
		if conf_element is not None:
			if conf_element.children != None:
				for item in conf_element.children:
					if item.type == "element" and conf_obj.cfg.has_key(item.name):
						f=xml2util.GetNodeValue(item)
						if f is not None:
							conf_obj.handlers[item.name](f)

		
