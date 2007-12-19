import sys

from pyrapi2 import *
from rapiutil import *
import libxml2
import xml2util
import characteristics
import logging


class PhoneCommunicator(object):
    def __init__(self):
        self.phoneConnected = False
        self.rapi_session = None
        self.checkConnection()

    def checkConnection(self):
        try:
            self.rapi_session = RAPISession(0)
            self.phoneConnected = True
        except:
			self.phoneConnected = False


    def getPowerStatus(self):
        self.checkConnection() 

        if self.phoneConnected:
            powerStatus = self.rapi_session.getSystemPowerStatus(True)
            return ( powerStatus["BatteryLifePercent"], powerStatus["BatteryFlag"] , powerStatus["ACLineStatus"] )

        return (0,0,0)
        



    def getListInstalledPrograms(self):
        result = []
        #if no phone is connected, try to build up connection
        self.checkConnection()

		#if we have connection, use it :)
        if self.phoneConnected:
            try:
                for program in config_query_get(self.rapi_session, None ,   "UnInstall").children.values():
                    result.append( program.type )
                    #print "",program.type
            except:
                self.phoneConnected = False
        
        return result	
