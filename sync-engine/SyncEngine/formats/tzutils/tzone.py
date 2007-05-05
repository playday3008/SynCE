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
import dateutil
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
# VcalTZ class
#
# Derived timezone class that reflects a vcal timezone

class VcalTZ(datetime.tzinfo):
	
	#
	# __init__
	#
	# required by the definition for a tzinfo
	
	def __init__(self):
		
		self.name = None
		self.loc  = None
		
		self.dstname = ""
		self.dstoffset = datetime.timedelta(seconds=0)
		self.dstRecurrence = recurrence.RecurrentEvent()
		
		self.stdname = ""
		self.stdoffset = datetime.timedelta(seconds=0)
		self.stdRecurrence = recurrence.RecurrentEvent()
		
	#
	# utcoffset
	#
	# return the utc offset including dst
		
		
	def utcoffset(self,dt):
		return self.stdoffset + self.dst(dt)
		
	#
	# dst
	#
	# return dst
		
	def dst(self,dt):
		dlt = dt.replace(tzinfo=None)
		
		# make sure we have sufficient DST and STD recurrences
		# to take us to the current date
		
		self.dstRecurrence.ProcessIteration(dlt)
		self.stdRecurrence.ProcessIteration(dlt)
			
		# Now get the deltas between us and the DST events
		
		deltaDST = datetime.timedelta.max
		deltaSTD = datetime.timedelta.max
		
		# If no valid dstRecurrence, delta is zero
		
		for m in reversed(self.dstRecurrence.ilist):
			if m > dlt:
				continue
			deltaDST = dlt - m
			break
		
		# if no valid stdRecurrence, delta is zero 
		 
		for m in reversed(self.stdRecurrence.ilist):
			if m > dlt:
				continue
			deltaSTD = dlt - m
			break
					
		if deltaDST < deltaSTD:
			# we are in DST
			return self.dstoffset
		else:
			# we are in STD
			return datetime.timedelta(0)

	#
	# tzname
	#
	
	def tzname(self,dt):
		return self.name

#
# TZInfoFromVcal
#
# Function providing vcal support 
	
def TZInfoFromVcal(tznode):
	
	tz = VcalTZ()
	
	child = tznode.children
	while child != None:
		
		if child.name == "TimezoneID":
			tzid = str(child.content).strip()
			tz.name = tzid
		if child.name == "Location":
			loc  = str(child.content).strip()
			tz.loc = loc
		if child.name == "Standard":
			cstd = child.children
			while cstd != None:
				if cstd.name == "TimezoneName":
					tz.stdname = str(cstd.content).strip()
				if cstd.name == "DateStarted":
					tz.stdRecurrence.SetStartDateFromText(str(cstd.content).strip())
				if cstd.name == "RecurrenceRule":
					rules = cstd.children
					while rules != None:
						if rules.name == "Rule":
							tz.stdRecurrence.AppendStringRule(str(rules.content).strip())
						rules=rules.next
				if cstd.name == "TZOffsetTo":
					tz.stdoffset = dateutil.OffsetToDelta(str(cstd.content).strip())
				
				cstd = cstd.next
		
		if child.name == "DaylightSavings":
			dso1 = datetime.timedelta(0)
			dso2 = datetime.timedelta(0)
			cstd = child.children			
			while cstd != None:
				
				if cstd.name == "TimezoneName":
					tz.dstname = str(cstd.content).strip()
				if cstd.name == "DateStarted":
					tz.dstRecurrence.SetStartDateFromText(str(cstd.content).strip())
				if cstd.name == "RecurrenceRule":
					rules = cstd.children
					while rules != None:
						if rules.name == "Rule":
							tz.dstRecurrence.AppendStringRule(str(rules.content).strip())
						rules=rules.next
				if cstd.name == "TZOffsetTo":
					dso1 = dateutil.OffsetToDelta(str(cstd.content).strip())
				if cstd.name == "TZOffsetFrom":
					dso2 = dateutil.OffsetToDelta(str(cstd.content).strip())
				
				cstd = cstd.next
			tz.dstoffset = dso1-dso2
	
		child=child.next

	return tz
