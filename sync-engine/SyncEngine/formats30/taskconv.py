# -*- coding: utf-8 -*-
############################################################################
# TASKCONV.py
#
# Dr J A Gow 10/11/2007
#
# XSLT extensions used by tasks only. Contains code split off from the old
# 'conversions' module to keep things a little clearer
#
############################################################################

import libxml2
import libxslt
import SyncEngine.xml2util as xml2util
import dateutil
import tzdatabase
import tzutils

DATE_FORMAT_TASK     = '%Y-%m-%dT%H:%M:%S.000'
DATE_FORMAT_VCALTASK = '%Y%m%dT%H%M%S'

###############################################################################
# DateToAirsyncUTC
#
#

def DateToAirsyncUTC(ctx):
	
	parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
	dst_node = transform_ctx.insertNode()
	tznode = transform_ctx.current()
		
	# handle input (could be DATE or DATE-TIME)
	
	date = dateutil.XMLDateTimeToDate(tznode,tzdatabase.tzdb)
	
	# convert date to UTC

	utcdate = date.astimezone(tzutils.tzone.utc)

	return utcdate.strftime(DATE_FORMAT_TASK)+"Z"

###############################################################################
# DateToAirsyncLocal
#
#

def DateToAirsyncLocal(ctx):
	
	parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
	dst_node = transform_ctx.insertNode()
	tznode = transform_ctx.current()
		
	# handle input (could be DATE or DATE-TIME)
	
	date = dateutil.XMLDateTimeToDate(tznode,tzdatabase.tzdb)
	
	return date.strftime(DATE_FORMAT_TASK)+"Z"

###############################################################################
# DateFromAirsync
#
#

def DateFromAirsync(ctx):
	
	parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
	src_node = transform_ctx.current()

	asdate = dateutil.TaskTextToDate(xml2util.GetNodeValue(src_node))
	datetext = asdate.strftime(DATE_FORMAT_VCALTASK)

	dst_node = transform_ctx.insertNode()
	
	curtz = tzdatabase.tzdb.GetCurrentTimezone()
	if curtz != None:

		# if we have a tz, we must insert the ID and convert to it
	
		dst_node.setProp("TimezoneID",curtz.name)

	else:
		# if not, does the source have a UtcStartDate or UtcDueDate element?
		
		if src_node.name == "StartDate":
			utcdatenode = xml2util.FindChildNode(src_node.parent,"UtcStartDate")
		elif src_node.name == "DueDate":
			utcdatenode = xml2util.FindChildNode(src_node.parent,"UtcDueDate")
		else:
			utcdatenode = None
		
		if utcdatenode != None:
			
			asdate = dateutil.TaskTextToDate(xml2util.GetNodeValue(utcdatenode))
			datetext = asdate.strftime(DATE_FORMAT_VCALTASK)+"Z"

	dst_node = transform_ctx.insertNode()
	dst_node.newChild(None,"Content",datetext)
	return ""

###############################################################################
# StatusToAirsync
#
# 

def StatusToAirsync(ctx):
	parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
	curnode = transform_ctx.current()
	s=xml2util.GetNodeValue(curnode)
	
	if s == "COMPLETED":
		return "1"
	else:
		# check that PercentComplete == 100% - mark it completed if
		# this is the case.
        	up = xml2util.FindChildNode(curnode.parent.parent,"PercentComplete")
        	if up != None:
            		ct = xml2util.FindChildNode(up,"Content")
	    		if ct != None:
	        		if xml2util.GetNodeValue(ct) == "100":
	            			return "1"
	return "0"

###############################################################################
# StatusFromAirsync
#
# We only sync the 'COMPLETED' state here. Evo2 maintains a number of 
# different status values for various states of a job except COMPLETED
# and we don't want to clobber these. AirStink seems only to maintain
# the two states: Not Completed and Completed. However, we force 
# the PercentComplete field to 100 if the task is marked as completed

def StatusFromAirsync(ctx):
	
	parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
	src_node = transform_ctx.current()
	s=xml2util.GetNodeValue(transform_ctx.current())
	if s == "1":
		base_node = transform_ctx.insertNode()
		stat_node = base_node.newChild(None, "Status", None)
		stat_node.newChild(None, "Content", "COMPLETED")
		pcnt_node = base_node.newChild(None, "PercentComplete", None)
		pcnt_node.newChild(None, "Content", "100")
    	return ""
 

###############################################################################
# PriorityToAirsync
#

def PriorityToAirsync(ctx):
	
	parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
	
	# Here. let us not destroy the 'unspecified' priority when going
	# _to_ airsync. We can't really help reassigning this as 'low' 
	# in the other direction.

	d = "0"
	s=xml2util.GetNodeValue(transform_ctx.current())
    	if s > "0":
		if s == "7":
	    		d = "0"
		elif s == "5":
	    		d = "1"
		elif s == "3":
            		d = "2"
		else:
	    		d = "0"
    	return d

###############################################################################
# PriorityFromAirsync
#
#

def PriorityFromAirsync(ctx):

	parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
	s=xml2util.GetNodeValue(transform_ctx.current())

	if s == "0":
		return "7"
	elif s == "1":
		return "5"
	elif s == "2":
		return "3"
	else:
        	return "0" # We can use the unspecced one here if we get such an one from Airsync
	
	
###############################################################################
# RegisterXSLTExtensionFunctions
#
# Register common converter functions for the parser
	
def RegisterXSLTExtensionFunctions():

	libxslt.registerExtModuleFunction("DateToAirsyncUTC",   "http://synce.org/task", DateToAirsyncUTC)
	libxslt.registerExtModuleFunction("DateToAirsyncLocal", "http://synce.org/task", DateToAirsyncLocal)
	libxslt.registerExtModuleFunction("DateFromAirsync",    "http://synce.org/task", DateFromAirsync)
	libxslt.registerExtModuleFunction("StatusToAirsync",    "http://synce.org/task", StatusToAirsync)
	libxslt.registerExtModuleFunction("StatusFromAirsync",  "http://synce.org/task", StatusFromAirsync)
	libxslt.registerExtModuleFunction("PriorityToAirsync",  "http://synce.org/task", PriorityToAirsync)
	libxslt.registerExtModuleFunction("PriorityFromAirsync","http://synce.org/task", PriorityFromAirsync)
