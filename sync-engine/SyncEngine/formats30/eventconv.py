# -*- coding: utf-8 -*-
############################################################################
# EVENTCONV.py
#
# Dr J A Gow 10/11/2007
#
# XSLT extensions used by events only. Contains code split off from the old
# 'conversions' module to keep things a little clearer
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
import xml2util
import dateutil
import commonconv

DATE_FORMAT_SHORT         = '%Y%m%d'
DATE_FORMAT_EVENT         = '%Y%m%dT%H%M%SZ'

###############################################################################
# VcalSplitByDay
#
# Utility function

def VcalSplitByDay(vcal_byday):
    return (vcal_byday[:-2], vcal_byday[-2:])



###############################################################################
# TimeTransparencyToAirsync
#
# Straightforward conversion of the <TimeTransparency> field

def TimeTransparencyToAirsync(ctx):
	
	parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
	
	if xml2util.GetNodeValue(transform_ctx.current()) == "TRANSPARENT":
		return "0"
	else:
		return "2" # 'Busy' is our default value

###############################################################################
# TimeTransparencyFromAirsync
#
# Straightforward conversion of the <TimeTransparency> field

def TimeTransparencyFromAirsync(ctx):
    
	parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)

	if xml2util.GetNodeValue(transform_ctx.current()) == "0":
		return "TRANSPARENT"
	else:
		return "OPAQUE" # 'Busy' is our default value

###############################################################################
# AllDayEventToAirsync
#
# We look for an all-day event by examining the <DateStarted> element. This
# will have a value of DATE, rather than DATE-TIME if an all day event

def AllDayEventToAirsync(ctx):

	parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
	src_node = transform_ctx.current()
 	datetext = xml2util.GetNodeValue(xml2util.FindChildNode(src_node, "Content"))
	if datetext.find("T") >= 0:
		return "0"
	else:
		return "1"

###############################################################################
# AttendeeToAirsync
#
# We only use the <Content> child (email) and the <CommonName> child, for now

def AttendeeToAirsync(ctx):
	
	parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
	src_node = transform_ctx.current()
	dst_node = transform_ctx.insertNode()

	email = xml2util.GetNodeValue(xml2util.FindChildNode(src_node, "Content"))[7:]
	name = xml2util.GetNodeValue(xml2util.FindChildNode(src_node, "CommonName"))
	if name != "":
		dst_node.newChild(None, "Name", name)
	dst_node.newChild(None, "Email", email)
	return ""

###############################################################################
# AttendeeFromAirsync
#
# Again, we only fill the <Content> and <CommonName> fields for now

def AttendeeFromAirsync(ctx):
	parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
	src_node = transform_ctx.current()
	dst_node = transform_ctx.insertNode()
    
	email = xml2util.GetNodeValue(xml2util.FindChildNode(src_node, "Email"))
	name = xml2util.GetNodeValue(xml2util.FindChildNode(src_node, "Name"))
	if email != "":
		dst_node.newChild(None, "Content", "MAILTO:%s" % email)
	dst_node.newChild(None, "CommonName", name)
	return ""


###############################################################################
# ExceptionDateTimeToAirsync
#
# ExclusionDateTime is a standard OS date, but the Airsync representation
# is different

def ExceptionDateTimeToAirsync(ctx):
	
	parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
	src_node = transform_ctx.current()
	dst_node = transform_ctx.insertNode()
	
	exclusion_date = dateutil.XMLDateTimeToDate(src_node,tzdatabase.tzdb)
	exclusion_value = xml2util.GetNodeAttr(src_node, "Value")

	if exclusion_value != None:
		if exclusion_value.lower() != "date":
			raise ValueError("Exclusions with values other than 'DATE' are not supported")

	# the date can be in a timezone, so convert to UTC
	
	exclusion_date = exclusion_date.astimezone(tzutils.tzone.utc)

	dst_node.newChild(None, "Deleted", "1")
	dst_node.newChild(None, "ExceptionStartTime", exclusion_date.strftime(DATE_FORMAT_EVENT))
	return ""

###############################################################################
# ExceptionDateTimeFromAirsync
#
# ExclusionDateTime is a standard OS date, but the Airsync representation
# is different

def ExceptionDateTimeFromAirsync(ctx):
	
	parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
	src_node = transform_ctx.current()
	dst_node = transform_ctx.insertNode()
    
	exception_deleted = xml2util.GetNodeValue(xml2util.FindChildNode(src_node, "Deleted"))
	exception_date = xml2util.GetNodeValue(xml2util.FindChildNode(src_node, "ExceptionStartTime"))
    
	if exception_deleted != "1":
		raise ValueError("Opensync does not support exceptions for modified occurrences")
	
	# We need to convert to current timezone if one is provided, else
	# we can assume UTC
	
	curtz = tzdatabase.tzdb.GetCurrentTimezone()
	if curtz!=None:
		date = date.astimezone(curtz)
		dst_node.setProp("TimezoneID",curtz.name)
		
	dst_node = transform_ctx.insertNode()
	dst_node.setProp("Value", "DATE")
	tznode=dst_node.newChild(None,"Content",date.strftime(DATE_FORMAT_SHORT))

	return ""


###############################################################################
# RegisterXSLTExtensionFunctions
#
# Register common converter functions for the parser
	
def RegisterXSLTExtensionFunctions():

	libxslt.registerExtModuleFunction("TimeTransparencyToAirsync",   "http://synce.org/event", TimeTransparencyToAirsync)
	libxslt.registerExtModuleFunction("TimeTransparencyFromAirsync", "http://synce.org/event", TimeTransparencyFromAirsync)
	libxslt.registerExtModuleFunction("AllDayEventToAirsync",        "http://synce.org/event", AllDayEventToAirsync)
	libxslt.registerExtModuleFunction("AttendeeToAirsync",           "http://synce.org/event", AttendeeToAirsync)
	libxslt.registerExtModuleFunction("AttendeeFromAirsync",         "http://synce.org/event", AttendeeFromAirsync)
	libxslt.registerExtModuleFunction("ExceptionDateTimeToAirsync",  "http://synce.org/event", ExceptionDateTimeToAirsync)
	libxslt.registerExtModuleFunction("ExceptionDateTimeFromAirsync","http://synce.org/event", ExceptionDateTimeFromAirsync)
