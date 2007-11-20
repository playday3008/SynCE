# -*- coding: utf-8 -*-

############################################################################
# DATEUTIL.py
#
# Utilities for date processing
#
# Dr J A Gow 10/4/2007
#
############################################################################

import datetime
import recurrence
import libxml2

#
# UTC timezone class
#
# Really does nothing but allows us to convert between local and UTC

class UtcTZ(datetime.tzinfo):

	def utcoffset(self, dt):
		return datetime.timedelta(0)

	def tzname(self, dt):
		return "UTC"

	def dst(self, dt):
		return datetime.timedelta(0)

# handy instance

utc = UtcTZ()

#
# TZComponent class
#
# Class provides a container for each 'component' of the timezone. This
# 'component' consists of a name and a number of recurrence rules
#

class TZComponent:
	
	#
	# __init__
	#
	# New components must have a name, but possess an empty list
	#
	# Note that RFC2445 indicates that ical timezones MUST have 
	# both offset and offsetfrom attributes - so we can rely on
	# this. Each component may have multiple RRules and RDates
	
	def __init__(self):
		self.name   = ""
		self.offsetfrom = datetime.timedelta(seconds=0)
		self.offset = datetime.timedelta(seconds=0)
		self.rrules = []	# list of recurrence rules
		self.rdates = []	# list of rdates
		self.startdate = None	# startdate
		
	# 
	# GetMinDelta
	#
	# From a specified date, get the difference between us and the last
	# transition of the last rule. This takes into account both RDate list
	# and RRule lists
		
	def GetMinDelta(self,date):
		delta = datetime.timedelta.max
				
		if self.startdate != None:
			if date >= self.startdate:

				for rr in self.rrules:
					print rr.startdate
					rr.ProcessIteration(date)

				# we are only interested in the delta if the
				# rdate is earlier than the supplied date
		
				for rd in self.rdates:
					if rd < date:
						delta = date - rd
		
				# now process the recurrence rule lists. Again
				# we look for minimum distance to the next earliest
				# rrule recurrence.
		
				for rr in self.rrules:
			
					idelta = datetime.timedelta.max
		
					for m in reversed(rr.ilist):
						if m > date:
							continue
						idelta = date - m
						break
					if idelta < delta:
						delta = idelta
		
		return delta


#
# VcalTZ class
#
# Derived timezone class that reflects a vcal timezone. This has now been
# updated to handle multiple components

class VcalTZ(datetime.tzinfo):
	
	#
	# __init__
	#
	# required by the definition for a tzinfo
	# We do not check for dupes when adding new components as they
	# won't kill us (but may slow things down a bit)
	#
	# An empty VcalTZ class, without any components, will describe UTC
	# but we still maintain the UtcTZ class for clarity.
	
	def __init__(self,name):
	
		self.name = name
		self.loc  = None
	
		self.stdcomponents = []
		self.dstcomponents = []

	#
	# utcoffset
	#
	# return the utc offset including dst.
	#
			
	def utcoffset(self,dt):
		
		# get a date we can play with (without the timezone)
		
		date = dt.replace(tzinfo=None)
		
		os=datetime.timedelta(seconds=0)
				
		# here we get the closest standard offset time
				
		delta = datetime.timedelta.max
		index = None
				
		for i in range(len(self.stdcomponents)):
			d = self.stdcomponents[i].GetMinDelta(date) 
			if d < delta:
				delta = d
				index = i

		if index != None:
			os = self.stdcomponents[index].offset
	
		return os + self.dst(dt)
		
	#
	# dst
	#
	# return dst
		
	def dst(self,dt):
		
		dlt = dt.replace(tzinfo=None)
				
		os=datetime.timedelta(seconds=0)
				
		# here we get the closest dst offset time
				
		delta = datetime.timedelta.max
		isDST = False
		index = 0

		for i in range(len(self.stdcomponents)):
			d = self.stdcomponents[i].GetMinDelta(dlt) 
			if d < delta:
				delta = d
				isDST = False

		for i in range(len(self.dstcomponents)):
			d = self.dstcomponents[i].GetMinDelta(dlt) 
			if d < delta:
				delta = d
				isDST = True
				index = i

		if isDST == True:
			os = self.dstcomponents[index].offset - self.dstcomponents[index].offsetfrom
		else:
			os = datetime.timedelta(seconds=0)
	
		return os
		
	#
	# tzname
	#
	
	def tzname(self,dt):
		return self.name

