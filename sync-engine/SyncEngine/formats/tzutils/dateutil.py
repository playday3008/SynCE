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
import tzone

#
# Day of week constant

dowtxt = { 0:"MO", 1:"TU", 2:"WE", 3:"TH", 4:"FR", 5:"SA", 6:"SU" }


############################################################################
# TextToDate
#
# Convert a standard date or date-time string to a datetime structure
#
############################################################################

def TextToDate(text):
	
	if len(text) < 8:
		raise ValueError("Corrupted date string")
	
	text = text.upper()
	
	tz = None
        if text[len(text)-1] == 'Z':
		tz = tzone.utc
		
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
# TaskTextToDate
#
# Converts a text date in the form YYYY-MM-DDTHH-MM-SS.hhh[Z] to a datetime
# structure

def TaskTextToDate(text):
	
	print "DATESTRING - ", text
	
	if len(text)<10:
		raise ValueError("Corrupted date string")
	
	text = text.upper()
	
	tz=None
	if text[len(text)-1] == 'Z':
		tz = tzone.utc
		
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
	