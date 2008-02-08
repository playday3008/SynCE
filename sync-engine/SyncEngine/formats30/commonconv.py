# -*- coding: utf-8 -*-
############################################################################
# COMMONCONV.py
#
# Dr J A Gow 10/11/2007
#
# XSLT extensions common to more than one sync item
#
############################################################################

import libxml2
import libxslt
import string
import pyrtfcomp
import base64
from time import gmtime, strftime
import tzutils
import tzdatabase
import dateutil
import SyncEngine.xml2util as xml2util

# exactly what it says on the tin.

MINUTES_PER_HOUR    = 60
MINUTES_PER_DAY     = MINUTES_PER_HOUR * 24

# Date formats

DATE_FORMAT_EVENT         = '%Y%m%dT%H%M%SZ'
DATE_FORMAT_SHORT         = '%Y%m%d'

# Compressed RTF header

RTFHDR = "\\ansi \\deff0{\\fonttbl{\\f0\\fnil\\fcharset0\\fprq0 Tahoma;}{\\f1\\froman\\fcharset2\\fprq2 "
RTFHDR += "Symbol;}{\\f2\\fswiss\\fcharset204\\fprq2  ;}}{\\colortbl;\\red0\\green0\\blue0;\\red128\\green128"
RTFHDR += "\\blue128;\\red192\\green192\\blue192;\\red255\\green255\\blue255;\\red255\\green0\\blue0;\\red0"
RTFHDR += "\\green255\\blue0;\\red0\\green0\\blue255;\\red0\\green255\\blue255;\\red255\\green0\\blue255;"
RTFHDR += "\\red255\\green255\\blue0;\\red128\\green0\\blue0;\\red0\\green128\\blue0;\\red0\\green0"
RTFHDR += "\\blue128;\\red0\\green128\\blue128;\\red128\\green0\\blue128;\\red128\\green128\\blue0;}\x0a\x0d"
RTFHDR += "\\f0 \\fs16 "

VCALDAYSTOASDAYS         = { "SU" : 1,
                                  "MO" : 2,
                                  "TU" : 4,
                                  "WE" : 8,
                                  "TH" : 16,
                                  "FR" : 32,
                                  "SA" : 64 }
 
ASDAYSTOVCALDAYS =              {  1   : "SU",
                                   2   : "MO",
                                   4   : "TU",
                                   8   : "WE",
                                   16  : "TH",
                                   32  : "FR",
                                   64  : "SA" }


###############################################################################
# VcalDaysToAirsyncDays
#
# Utility function not called directly by XSLT transforms

def VcalDaysToAirsyncDays(vcal_days):
	airsync_days = 0
	for vcal_day in vcal_days.split(","):
		airsync_days = airsync_days | VCALDAYSTOASDAYS[vcal_day.upper()]
	return airsync_days

###############################################################################
# AirsyncDaysToVcalDays
#
# Utility function not called directly by XSLT transforms

def AirsyncDaysToVcalDays(airsync_days):
	vcal_days = []
	for i in xrange(0,8):
		mask = (1 << i)
		if mask & int(airsync_days):
			vcal_days.append(ASDAYSTOVCALDAYS[mask])
	return ",".join(vcal_days)

###############################################################################
# GenerateVcalByDay
#
# Utility function not called directly by XSLT transforms

def GenerateVcalByDay(airsync_week, airsync_day):
	week = int(airsync_week)
	if week == 5:
		# Special case: Airsync uses '5' to denote the last week of a month
		week = -1
	return "%d%s" % (week, airsync_days_to_vcal_days_map[int(airsync_day)])


###############################################################################
# HandleOSTZ
#
# Handle an Opensync <Timezone> element.

def HandleOSTZ(ctx):
	
	parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
	tznode = transform_ctx.current()

	tzdatabase.NewTZFromXML(tznode,tzdatabase.tzdb)

	return ""

###############################################################################
# HandleOSTZComponent
#
# Handle an Opensync <TimezoneComponent> element.

def HandleOSTZComponent(ctx):
	
	parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
	tznode = transform_ctx.current()
	
	tzdatabase.NewTZComponentFromXML(tznode,tzdatabase.tzdb)
	
	return ""

###############################################################################
# HandleOSTZRRyle
#
# Handle an Opensync <TimezoneRule> element.

def HandleOSTZRRule(ctx):
	
	parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
	tznode = transform_ctx.current()
	
	tzdatabase.NewTZRRuleFromXML(tznode,tzdatabase.tzdb)
	
	return ""

###############################################################################
# CurrentTZToAirsync
#
# If we have a current TZ, export it to AirSync.

def CurrentTZToAirsync(ctx):
	
	parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
	tznode = transform_ctx.current()
	
	tz = tzdatabase.tzdb.GetCurrentTimezone()
	if tz!=None:
		dst_node = transform_ctx.insertNode()
		tznode=dst_node.newChild(None,"Timezone",base64.b64encode(tzdatabase.TZToAirsync(tz)))
		ns=tznode.newNs("http://synce.org/formats/airsync_wm5/calendar",None)
		tznode.setNs(ns)
		
	return ""

###############################################################################
# TZFromAirsync
#
# Load an AirSync timezone into the database

def TZFromAirsync(ctx):
	
	parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
	
	tznode = transform_ctx.current()
	tzdata = base64.b64decode(xml2util.GetNodeValue(tznode))	
	
	# Some AS timezones fail to include a year. See if we can
	# graft this from the AS body
	
	datenode = xml2util.FindChildNode(tznode.parent,"StartTime")
	if datenode == None:
		datenode = xml2util.FindChildNode(tznode.parent,"DtStamp")
	if datenode == None:
		year = datetime.datetime.now().year
	else:
		txtyear = xml2util.GetNodeValue(datenode)
		year = int(txtyear[0:4])
		
	tz = tzdatabase.TZFromAirsync(tzdata,year)
	
	# only add it if we have a valid timezone
	
	if tz!=None:
		tzdatabase.tzdb.PutTimezone(tz)
		
	return ""

###############################################################################
# AllTZToOpensync
#
# Insert the entire timezone database to Opensync XML

def AllTZToOpensync(ctx):
	
	parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
	dst_node = transform_ctx.insertNode()
	
	tzs = tzdatabase.tzdb.GetTimezoneList()
	for tz in tzs:
		tzdatabase.TZToXML(dst_node,tzdatabase.tzdb.GetTimezone(tz))
	return ""
	

###############################################################################
# OSDateToAirsync
#
# Convert an OpenSync date to an AirSync date. This involves capturing the
# timezone ID and converting the date to UTC if necessary.
#

def OSDateToAirsync(ctx):
	
	parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
	tznode = transform_ctx.current()
		
	# handle input (could be DATE or DATE-TIME)
	
	date = dateutil.XMLDateTimeToDate(tznode,tzdatabase.tzdb)
	
	# convert date to UTC

	utcdate = date.astimezone(tzutils.tzone.utc)
	return utcdate.strftime(DATE_FORMAT_EVENT)

###############################################################################
# OSDateFromAirsync
#
# Convert an Airsync date to an OpenSync date. This is somewhat dependent
# upon the presence of an AirSync timezone being present in the database. 
#
# TODO: Preseed the database with the current system timezone if one not
# specified

def OSDateFromAirsync(ctx):

	parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
	tznode = transform_ctx.current()
		
	date = dateutil.TextToDate(xml2util.GetNodeValue(transform_ctx.current()))
	date.replace(tzinfo=tzutils.tzone.utc)
	
	# We are going from AirSync, so 'date' will be in UTC
	# If we have a current timezone, set it.
	
	dst_node = transform_ctx.insertNode()
	
	dst_node.setProp("Value","DATE-TIME")
	curtz = tzdatabase.tzdb.GetCurrentTimezone()
	if curtz!=None:
		date = date.astimezone(curtz)
		dst_node.setProp("TimezoneID",curtz.name)
		
	tznode=dst_node.newChild(None,"Content",date.strftime(DATE_FORMAT_EVENT))
	
	return ""

###############################################################################
# AirsyncDateFromNow
#
# Convert the current system timestamp to an Airsync date
# TODO: Insert the current system timezone. We may need to export this
# as a separate ID.

def AirsyncDateFromNow(ctx):
	
	parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
	return strftime("%Y%m%dT%H%M%SZ", gmtime())

###############################################################################
# OSTextToAirsyncRTF
#
# Encodes standard text to the compressed-RTF used by AirSync

def OSTextToAirsyncRTF(ctx):
	
	parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
	src_node = transform_ctx.current()

	s=xml2util.GetNodeValue(transform_ctx.current())
	
	ec = ""
	if len(s) > 0:
		try:
			asnote = pyrtfcomp.RTFConvertFromUTF8(s,RTFHDR,1)
			ec=base64.b64encode(asnote)
		except pyrtfcomp.RTFException, ConvErr:
			pass
	return ec

###############################################################################
# OSTextFromAirsyncRTF
#
# Converts Airsync RTF content to standard text

def OSTextFromAirsyncRTF(ctx):
	
	parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
	src_node = transform_ctx.current()
	
	s=xml2util.GetNodeValue(transform_ctx.current())
	
	asnote = ""
	if len(s) > 0:
		dc = base64.b64decode(s)
		try:
			asnote = pyrtfcomp.RTFConvertToUTF8(dc,1)
		except pyrtfcomp.RTFException, ConvErr:
			pass
	return asnote

###############################################################################
# RecurrenceRuleToAirsync
#
# Recurrence rules are complex structures in both OpenSync 0.3x and Airsync

def RecurrenceRuleToAirsync(ctx):
	
	parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
	src_node = transform_ctx.current()
	dst_node = transform_ctx.insertNode()

	# Extract the rules
	
	src_rules = {}
	
	child = src_node.children
	while child != None:
		src_rules[child.name.lower()] = xml2util.GetNodeValue(child)
		child = child.next

	# Interval, Count, and Until rules have straightforward conversions

	if src_rules.has_key("interval"):
        	dst_node.newChild(None, "Interval", src_rules["interval"])
	if src_rules.has_key("until"):
		dst_node.newChild(None, "Until", dateutil.TextToDate(src_rules["until"]).strftime(DATE_FORMAT_EVENT))
	if src_rules.has_key("count"):
		dst_node.newChild(None, "Occurrences", src_rules["count"])

	# Handle different types of recurrences on a case-by-case basis
    
	if src_rules["frequency"].lower() == "daily":
		
		# There really isn't much to convert in this case..
		dst_node.newChild(None, "Type", "0")
		
	elif src_rules["frequency"].lower() == "weekly":

		dst_node.newChild(None, "Type", "1")
		dst_node.newChild(None, "DayOfWeek", str(VcalDaysToAirsyncDays(src_rules["byday"])))
		
	elif src_rules["frequency"].lower() == "monthly":
		
		if src_rules.has_key("bymonthday"):
			dst_node.newChild(None, "Type", "2")
			dst_node.newChild(None, "DayOfMonth", src_rules["bymonthday"])

		elif src_rules.has_key("byday"):

			week, day = VcalSplitByDay(src_rules["byday"])
			dst_node.newChild(None, "Type", "3")
			dst_node.newChild(None, "DayOfWeek", str(VcalDaysToAirsyncDays(day)))

			if week >= 0:
		
				dst_node.newChild(None, "WeekOfMonth", week)

			elif week == -1:

				# Airsync deals with this as a special case
				dst_node.newChild(None, "WeekOfMonth", "5")
		
			else:
				# Not supported (as far as I can tell...)
				# Airsync cannot count backwards from the end of the
				# month in general
		
                		raise ValueError("Airsync does not support counting from end of month")
        	else:

			# It seems like this should be against the rules, and filling in
			# a default might not make sense because either of the above interpretations
			# is equally valid.
			
			raise ValueError("Monthly events must either specify BYMONTHDAY or BYDAY rules")
		
	elif src_rules["frequency"].lower() == "yearly":

		if src_rules.has_key("bymonth"):
			
			if src_rules.has_key("bymonthday"):
				dst_node.newChild(None, "Type", "5")
				dst_node.newChild(None, "MonthOfYear", src_rules["bymonth"])
				dst_node.newChild(None, "DayOfMonth", src_rules["bymonthday"])
			elif src_rules.has_key("byday"):
				week, day = VcalSplitByDay(src_rules["byday"])
				dst_node.newChild(None, "Type", "6")
				dst_node.newChild(None, "MonthOfYear", src_rules["bymonth"])
				dst_node.newChild(None, "DayOfWeek", str(VcalDaysToAirsyncDays(day)))
        			
				if week >= 0:
					dst_node.newChild(None, "WeekOfMonth", week)
				elif week == -1:

					# Airsync deals with this as a special case

					dst_node.newChild(None, "WeekOfMonth", "5")

				else:
					# Not supported (as far as I can tell...)
					# Airsync cannot count backwards from the end of the
					# month in general

					raise ValueError("Airsync does not support counting from end of month")
			else:

				# It seems like this should be against the rules, and filling in
				# a default might not make sense because either of the above interpretations
				# is equally valid.
				
				raise ValueError("Yearly events which are by month must either specify BYMONTHDAY or BYDAY rules")
			
		elif src_rules.has_key("byyearday"):

			# Not supported (as far as I can tell...)
			# Airsync does not have a 'DayOfYear' field
			raise ValueError("Airsync does not support day-of-year yearly events")

		else:
			# we need to get the start/recurrence date from the start date
			# Get the start date - we need this to expand yearly rules
			# We should always have a start date in a legal event!
	
			start = xml2util.FindChildNode(src_node.parent,"DateStarted")
			date = dateutil.XMLDateTimeToDate(start,tzdatabase.tzdb)
			utcdate = date.astimezone(tzutils.tzone.utc)

			# We need the month and the day
			
			dst_node.newChild(None,"Type", "5")
			dst_node.newChild(None,"MonthOfYear",str(utcdate.month))
			dst_node.newChild(None,"DayOfMonth",str(utcdate.day))
			
			# 'Count' is already handled
			
	return ""

###############################################################################
# RecurrenceRuleFromAirsync
#
# Recurrence rules are complex structures in both OpenSync 0.3x and Airsync

def RecurrenceRuleFromAirsync(ctx):

	parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
	src_node = transform_ctx.current()
	dst_node = transform_ctx.insertNode()

	interval_node    = xml2util.FindChildNode(src_node, "Interval")
	until_node       = xml2util.FindChildNode(src_node, "Until")
	occurrences_node = xml2util.FindChildNode(src_node, "Occurrences")
	type_node        = xml2util.FindChildNode(src_node, "Type")
	dayofweek_node   = xml2util.FindChildNode(src_node, "DayOfWeek")
	dayofmonth_node  = xml2util.FindChildNode(src_node, "DayOfMonth")
	weekofmonth_node = xml2util.FindChildNode(src_node, "WeekOfMonth")
	monthofyear_node = xml2util.FindChildNode(src_node, "MonthOfYear")

	# Add the common nodes that don't really require conversion

	if until_node != None:
		dst_node.newChild(None, "Until", "%s" % xml2util.GetNodeValue(until_node))
	if occurrences_node != None:
		dst_node.newChild(None, "Count", "%s" % xml2util.GetNodeValue(occurrences_node))
	if interval_node != None:
		dst_node.newChild(None, "Interval", "%s" % xml2util.GetNodeValue(interval_node))

	if type_node != None:
		type = int(xml2util.GetNodeValue(type_node))
		
		# Special case: we can treat this as simple weekly event

		if type == 0 and dayofweek_node != None:
			
			type = 1

        	if type == 0:
			
			dst_node.newChild(None, "Frequency", "DAILY")
			
		elif type == 1:
			
			dst_node.newChild(None, "Frequency", "WEEKLY")
			dst_node.newChild(None, "ByDay", "%s" % AirsyncDaysToVcalDays(xml2util.GetNodeValue(dayofweek_node)))
		
		elif type == 2:

			dst_node.newChild(None, "Frequency", "MONTHLY")
			dst_node.newChild(None, "ByMonthDay", "%s" % xml2util.GetNodeValue(dayofmonth_node))
			
		elif type == 3:
			
			dst_node.newChild(None, "Frequency", "MONTHLY")
			dst_node.newChild(None, "ByDay", "%s" % GenerateVcalByDay(xml2util.GetNodeValue(weekofmonth_node), xml2util.GetNodeValue(dayofweek_node)))

		elif type == 5:

			dst_node.newChild(None, "Frequency", "YEARLY")
			dst_node.newChild(None, "ByMonth", "%s" % xml2util.GetNodeValue(monthofyear_node))
			dst_node.newChild(None, "ByMonthDay", "%s" % xml2util.GetNodeValue(dayofmonth_node))

		elif type == 6:

			dst_node.newChild(None, "Frequency", "YEARLY")
			dst_node.newChild(None, "ByMonth", "%s" % xml2util.GetNodeValue(monthofyear_node))
			dst_node.newChild(None, "ByDay", "%s" % GenerateVcalByDay(xml2util.GetNodeValue(weekofmonth_node), xml2util.GetNodeValue(dayofweek_node)))

		else:

			# Unsupported type
			raise ValueError("Unknown recurrence type %d from Airsync" % type)

	else:

		# If we don't know what type of recurrence it is, we
		# can't construct its vcal rules
		
		raise ValueError("No recurrence type specified from Airsync")
	
	return ""

###############################################################################
# ClassToAirsync
#
# Converts the <Class> element of OpenSync into the numeric <Sensitivity>
# value used by AirSync

def ClassToAirsync(ctx):
	
	parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
	s = xml2util.GetNodeValue(transform_ctx.current())
    
	if s == "PRIVATE":
		return "2"
	elif s == "CONFIDENTIAL":
		return "3"
	else:
		return "0" # 'PUBLIC' is our default value

###############################################################################
# ClassFromAirsync

def ClassFromAirsync(ctx):
	
	parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
	s = xml2util.GetNodeValue(transform_ctx.current())
    
	if s == "2":
		return "PRIVATE"
	elif s == "3":
		return "CONFIDENTIAL"
	else:
		return "PUBLIC" # 'PUBLIC' is our default value

###############################################################################
# ReminderToAirsync
#
# Converts an <Alarm> element, which 

def AlarmToAirsync(ctx):

	parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
	src_node = transform_ctx.current()
    
	desc_node = xml2util.FindChildNode(src_node,"Description")
	content_node = xml2util.FindChildNode(src_node, "AlarmTrigger")

	# value attribute is mandatory. Convert to blank string if not 
	# present. We only handle 'DURATION' values here, DATE-TIME is a 
	# TODO (we can convert this into a duration for AS)
    
	value_node = xml2util.GetNodeAttr(src_node,"Value")
	
	if value_node == None:
        	return ""

	if value_node.lower() != "duration":
		return ""
	
	s = xml2util.GetNodeValue(content_node).lstrip("-PT").upper()

	days=0
	hours=0
	minutes=0
	
	if s.rfind("D") != -1:
		days = int(s[:s.rfind("D")])
		s = s[s.rfind("D")+1:]
	if s.rfind("H") != -1:
		hours = int(s[:s.rfind("H")])
		s = s[s.rfind("H")+1:]
	if s.rfind("M") != -1:
		minutes = int(s[:s.rfind("M")])
		s = s[s.rfind("M")+1:]

	return str(days * MINUTES_PER_DAY + hours * MINUTES_PER_HOUR + minutes)

###############################################################################
# AlarmFromAirsync
#
# Converts AirSync <Reminder> to Opensync <Alarm> block, including attrs
#

def AlarmFromAirsync(ctx):
	
	parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
	src_node = transform_ctx.current()
	
	s = int(xml2util.GetNodeValue(src_node))
	
	if s % MINUTES_PER_DAY == 0:
		return "-P%iD" % (s / MINUTES_PER_DAY)
	elif s % MINUTES_PER_HOUR == 0:
		return "-PT%iH" % (s / MINUTES_PER_HOUR)
	else:
		return "-PT%iM" % s


###############################################################################
# RegisterXSLTExtensionFunctions
#
# Register common converter functions for the parser
	
def RegisterXSLTExtensionFunctions():

	libxslt.registerExtModuleFunction("HandleOSTZ"           ,     "http://synce.org/common", HandleOSTZ)
	libxslt.registerExtModuleFunction("HandleOSTZComponent"  ,     "http://synce.org/common", HandleOSTZComponent)
	libxslt.registerExtModuleFunction("HandleOSTZRule"       ,     "http://synce.org/common", HandleOSTZRRule)
	libxslt.registerExtModuleFunction("CurrentTZToAirsync"   ,     "http://synce.org/common", CurrentTZToAirsync)
	libxslt.registerExtModuleFunction("TZFromAirsync"   ,          "http://synce.org/common", TZFromAirsync)
	libxslt.registerExtModuleFunction("AllTZToOpensync",           "http://synce.org/common", AllTZToOpensync)
	libxslt.registerExtModuleFunction("OSDateToAirsync"      ,     "http://synce.org/common", OSDateToAirsync)
	libxslt.registerExtModuleFunction("OSDateFromAirsync"    ,     "http://synce.org/common", OSDateFromAirsync)
	libxslt.registerExtModuleFunction("AirsyncDateFromNow"   ,     "http://synce.org/common", AirsyncDateFromNow)
	libxslt.registerExtModuleFunction("OSTextToAirsyncRTF",        "http://synce.org/common", OSTextToAirsyncRTF)
	libxslt.registerExtModuleFunction("OSTextFromAirsyncRTF",      "http://synce.org/common", OSTextFromAirsyncRTF)
	libxslt.registerExtModuleFunction("RecurrenceRuleToAirsync",   "http://synce.org/common", RecurrenceRuleToAirsync)
	libxslt.registerExtModuleFunction("RecurrenceRuleFromAirsync", "http://synce.org/common", RecurrenceRuleFromAirsync)
	libxslt.registerExtModuleFunction("ClassToAirsync",            "http://synce.org/common", ClassToAirsync)
	libxslt.registerExtModuleFunction("ClassFromAirsync",          "http://synce.org/common", ClassFromAirsync)
	libxslt.registerExtModuleFunction("AlarmToAirsync",            "http://synce.org/common", AlarmToAirsync)
	libxslt.registerExtModuleFunction("AlarmFromAirsync",          "http://synce.org/common", AlarmFromAirsync)

