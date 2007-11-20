# -*- coding: utf-8 -*-
############################################################################
# CONTACTCONV.py
#
# Dr J A Gow 10/11/2007
#
# XSLT extensions used by events only. Contains code split off from the old
# 'conversions' module to keep things a little clearer
#
############################################################################

import libxml2
import libxslt
import xml2util

###############################################################################
# AnniversaryToAirsync
#
# This is a straightforward date conversion with no timezone

def AnniversaryToAirsync(ctx):
	parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
	return xml2util.GetNodeValue(transform_ctx.current()) + "T00:00:00.000Z"

###############################################################################
# AnniversaryFromAirsync
#
# This is a straightforward date conversion with no timezone

def AnniversaryFromAirsync(ctx):
	parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
	return xml2util.GetNodeValue(transform_ctx.current()).split("T")[0]

###############################################################################
# BirthdayToAirsync
#
# This is a straightforward date conversion with no timezone

def BirthdayToAirsync(ctx):
	parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
	s = xml2util.GetNodeValue(transform_ctx.current())
	return "%s-%s-%sT00:00:00.000Z" % (s[0:4], s[4:6], s[6:8])

###############################################################################
# BirthdayFromAirsync
#
# This is a straightforward date conversion with no timezone

def BirthdayFromAirsync(ctx):
	parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
	return xml2util.GetNodeValue(transform_ctx.current()).split("T")[0].replace("-", "")



###############################################################################
# RegisterXSLTExtensionFunctions
#
# Register common converter functions for the parser
	
def RegisterXSLTExtensionFunctions():

	libxslt.registerExtModuleFunction("AnniversaryToAirsync",     "http://synce.org/contact", AnniversaryToAirsync)
	libxslt.registerExtModuleFunction("AnniversaryFromAirsync",   "http://synce.org/contact", AnniversaryFromAirsync)
	libxslt.registerExtModuleFunction("BirthdayToAirsync",        "http://synce.org/contact", BirthdayToAirsync)
	libxslt.registerExtModuleFunction("BirthdayFromAirsync",      "http://synce.org/contact", BirthdayFromAirsync)
