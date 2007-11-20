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
import tzutils
import tzdatabase
import xml2util

#
# Day of week constant

dowtxt = { 0:"MO", 1:"TU", 2:"WE", 3:"TH", 4:"FR", 5:"SA", 6:"SU" }
FREQS    = { "SECONDLY":6, "MINUTELY":5, "HOURLY":4, "DAILY":3, "WEEKLY":2, "MONTHLY":1, "YEARLY":0 }
FREQLIST = ["YEARLY","MONTHLY","WEEKLY","DAILY","HOURLY","MINUTELY","SECONDLY"]
WEEKDAYS = {"MO" : 0, "TU" : 1, "WE" : 2, "TH" : 3, "FR" : 4, "SA" : 5, "SU" : 6 }
WEEKDAYLIST = ["MO","TU","WE","TH","FR","SA","SU"]


############################################################################
# TextToDate
#
# Convert a standard date or date-time string to a datetime structure
#
#

def TextToDate(text):
	
	if len(text) < 8:
		raise ValueError("Corrupted date string")
	
	text = text.upper()
	
	tz = None
        if text[len(text)-1] == 'Z':
		tz = tzutils.tzone.utc
		
	text=text.lstrip('Z')
	
	if text.find("T") < 0:
		text += "T000000"

	try:
		year   = int(text[0:4])
		month  = int(text[4:6])
		day    = int(text[6:8])
		hour   = int(text[9:11])
		minute = int(text[11:13])
		second = int(text[13:15])
	except:
		raise ValueError("Corrupted date string")
	
	return datetime.datetime(year,month,day,hour,minute,second,0,tzinfo=tz)

############################################################################
# DateToText
#
# Take a standard datetime object and return a date string
#
#

def DateToText(date):
	
	s="%4d%02d%02dT%02d%02d%02d" % (date.year,date.month,date.day,
					date.hour,date.minute,date.second)
	return s

############################################################################
# TimedeltaToText
#
# Convert a timedelta object to a string (hours+minutes, +/-HHMM
#

def TimedeltaToText(delta):
	secs = 86400 * delta.days + delta.seconds
	hrs = secs/3600
	mins = (secs - (hrs*3600))/60
	if mins < 0:
		mins = -mins
	return "%+03d%02d" % (hrs,mins)

############################################################################
# TaskTextToDate
#
# Converts a text date in the form YYYY-MM-DDTHH-MM-SS.hhh[Z] to a datetime
# structure

def TaskTextToDate(text):
	
	if len(text)<10:
		raise ValueError("Corrupted date string")
	
	text = text.upper()
	
	tz=None
	if text[len(text)-1] == 'Z':
		tz = tzutils.tzone.utc
		
	text = text.lstrip('Z')
	
	if text.find("T") < 0:
		text += "T00:00.000"
		
	try:
		year  = int(text[0:4])
		month = int(text[5:7])
		day   = int(text[8:10])
		hour  = int(text[11:13])
		minute = int(text[14:16])
		second = int(text[17:19])
		
		# ignore microseconds - we don't use them
	except:
		raise ValueError("Corrupted date string")

	return datetime.datetime(year,month,day,hour,minute,second,tzinfo=tz)

############################################################################
# OffsetToDelta
#
# Convert a time offset to a delta
#
############################################################################

def OffsetToDelta(text):
	
	t = text.lstrip('+-')
	
	second = 0
	minute = 0
	hour   = 0
	
	if len(t) > 4:
		second = int(t[4:])
	minute = int(t[2:4])
	hour   = int(t[0:2])
	
	delta = datetime.timedelta(minutes = hour*60+minute, seconds = second)
	if text[0] == '-':
		delta = -delta
	return delta
	
############################################################################
# XMLDateTimeToDate
#
# OS 0.3x
#
# Take a Date block (which may contain DateTime or Date information, and
# return a date
#
############################################################################
	
def XMLDateTimeToDate(node,tzdb):

	hour = 0
	minute = 0
	second = 0
	
	t = "date-time"
	n=xml2util.GetNodeAttr(node,"Value")
	if n:
		t = n.lower()
		
	text = ""
	child = node.children
	while child != None:
		if child.name == "Content":
			text = str(child.content).strip()
		child=child.next
	
	# handle part common to date and date-time
		
	if len(text) < 8:
		raise ValueError("Corrupted date string")
	
	text = text.upper()
	
	# if specified as utc, set timezone accordingly performing
	# the appropriate lookup in the database
	
	tzid = xml2util.GetNodeAttr(node,"TimezoneID")
	tz=None
	if tzid != None and tzdb != None:
		tz=tzdatabase.tzdb.GetTimezone(tzid)
		
		# CHECK THIS: As of 19/11/07 OS can export a timezoneID that is not
		# described in the event! (seen in 'LastModified') So we fudge this
		# here by treating the time as UTC. 
		
		if tz == None:
			tz = tzutils.tzone.utc
		
	else:
		tz = tzutils.tzone.utc
	
	#
	# Handle the portion common to DATE and DATE-TIME
	
	try:
		year   = int(text[0:4])
		month  = int(text[4:6])
		day    = int(text[6:8])
	except:
		raise ValueError("Corrupted date string")
	
	#
	# Handle the DATE-TIME only component
	
	if t == "date-time":
		
		if len(text) < 15:
			
			# We check here, because sometimes OpenSync does
			# not set the Value attribute (it's own schema suggests
			# it should)!
			
			if len(text)==8:
				# a DATE either has been represented as
				# DATE-TIME, or there is no value attr.
				text+="T000000"
			else:
				#it's definitely corrupt
				raise ValueError("Corrupted date-time string")
		
		try:	
			hour   = int(text[9:11])
			minute = int(text[11:13])
			second = int(text[13:15])
	
		except:
			raise ValueError("Corrupted date-time string")
		
	
	return datetime.datetime(year,month,day,hour,minute,second,0,tzinfo=tz)

###############################################################################
# RRuleToXMLStandard
# 
# Populate a recurrence rule node with the contents of the recurrence rule
# (not extended)
#
###############################################################################

def StandardRRuleToXML(node,rrule):
	
	if rrule.freq != None:
		node.newChild(None,"Frequency",FREQLIST[rrule.freq])
		
		if rrule.until != None:
			node.newChild(None,"Until",DateToText(rrule.until))
		
		node.newChild(None,"Count","%d" % rrule.count)
		node.newChild(None,"Interval","%d" % rrule.interval)
		
		if len(rrule.byDay) > 0:
			v=""
			for (d,pos) in rrule.byDay:
				if pos !=0 :
					v+=str(pos)
				v += WEEKDAYLIST[d]
			node.newChild(None,"ByDay",v)

		if len(rrule.byMonthDay) > 0:
			v=""
			for x in rrule.byMonthDay:
				v += str(x)
			node.newChild(None,"ByMonthDay",v)
		
		if len(rrule.byYearDay) > 0:
			v=""
			for x in rrule.byYearDay:
				v += str(x)
			node.newChild(None,"ByYearDay",v)
		
		if len(rrule.byMonth) > 0:
			v=""
			for x in rrule.byMonth:
				v += str(x)
			node.newChild(None,"ByMonth",v)
