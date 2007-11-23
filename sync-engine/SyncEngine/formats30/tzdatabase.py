# -*- coding: utf-8 -*-
############################################################################
# TZDatabase.py
#
# Timezone factories and database for conversions
#
# Dr J A Gow 9/11/2007
#
############################################################################

import libxml2
import libxslt
import string
import struct
import datetime
import dateutil
import tzutils
import SyncEngine.xml2util as xml2util
import base64
import SyncEngine.util
import calendar


class TZDatabase:
	
	def __init__(self):
		
		# The basic database is a dictionary that we can interrogate
		# for timezones during the conversions. The dictionary is 
		# keyed on the timezone ID string.

		self.db = {}
		self.initialkey = None
		
	#
	# Flush
	#
	# Flush the timezone database
	
	def Flush(self):
		self.db = {}
		self.initialkey = None
		
	#
	# GetTimezoneList
	#
	# Return a list of the timezone IDs stored in the database
	
	def GetTimezoneList(self):
		return self.db.keys()
		
	#
	# GetTimezone
	#
	# Get a timezone from the database by ID
	
	def GetTimezone(self,tzname):
		if self.db.has_key(tzname):
			return self.db[tzname]
		else:
			return None
		
	#
	# PutTimezone
	#
	# Put a timezone in the database
	
	def PutTimezone(self,vctz):
		if vctz != None:
			self.db[vctz.name] = vctz
			if self.initialkey == None:
				self.initialkey = vctz.name
			
	#
	# GetCurrentTimezone
	#
	# At the moment, the 'current' TZ (the one we convert)
	# is the first one in the list
	#
	
	def GetCurrentTimezone(self):
		if self.initialkey != None:
			return self.db[self.initialkey]
		else:
			return None
		

#
# Our global tzdb database
# We are not multithreading the conversion process, only one item will be converted
# at a time, so we can do this easily.

tzdb = TZDatabase()

###############################################################################
# Timezone factories
#
# This group of functions allows us to create and convert timezones from any
# of the formats encountered in the sync process. 

#
# NewTZFromXML
#
# We can only generate new timezones as part of the database from XML - the
# reason for this is simply that the timezones are not specified as a single
# level XML block. This may change in later versions of OpenSync but for
# now we have to do it here. We can specify the database we use.

def NewTZFromXML(node,tzdb):
	tzid=xml2util.GetNodeAttr(node,"TimezoneID")
	if tzid:
		tz = tzutils.tzone.VcalTZ(tzid)
		child=node.children
		while child != None:
			content = str(child.content).strip()
			if child.name == "X-Location":
				tz.loc = content
			child = child.next
		tzdb.PutTimezone(tz)
		return True
	else:
		return False
	
#
# NewTZComponentFromXML
#
# Again, we rely on sequence here. NewTZFromXML should have been called
# first to establish the timezone in the database
	
def NewTZComponentFromXML(node,tzdb):
	tzid=xml2util.GetNodeAttr(node,"TimezoneID")
	comp=xml2util.GetNodeAttr(node,"TZComponent")
	if tzid and comp:
		tz=tzdb.GetTimezone(tzid)
		if tz != None:
				
			component = tzutils.tzone.TZComponent()
				
			child=node.children
			while child != None:
				content = str(child.content).strip()
				if child.name == "TZName":
					component.name = child.content
				if child.name == "DateTimeStart":
					component.startdate = dateutil.TextToDate(content)
				if child.name == "TZOffsetFrom":
					component.offsetfrom = dateutil.OffsetToDelta(content)
				if child.name == "TZOffsetTo":
					component.offset = dateutil.OffsetToDelta(content)
				if child.name == "RecurrenceDateTime":
					component.rdates.append(dateutil.XMLDateTimeToDate(child,tzdb))
						
				# we don't care about 'Comment'
					
				child=child.next
					
			if comp=="Standard":
				tz.stdcomponents.append(component)
			elif comp=="Daylight":
				tz.dstcomponents.append(component)
			else:
				print "Unknown TZ component type %s" % comp.content

			return True
		else:
			print "No timezone available for component %s" % tzid
			return False
	else:
		return False

#
# NewTZRRuleFromXML
#
# Again, we rely on sequence here. NewTZFromXML and NewTZComponentFromXML should 
# have been called first to establish the timezone and component in the database
#
# The OpenSync schema breaks RFC2445 in that it is impossible to have multiple
# timezone components (STD or DST) keyed by name - the schema keys the rule by
# ComponentID which also is used to determine the difference between DST and STD

def NewTZRRuleFromXML(node,tzdb):
	tzid = xml2util.GetNodeAttr(node,"TimezoneID")
	comp = xml2util.GetNodeAttr(node,"TZComponent")
	if tzid and comp:
		tz=tzdb.GetTimezone(tzid)
		if tz!=None:

			rule = tzutils.recurrence.RecurrentEvent()

			child = node.children
			while child != None:
				cmd = str(child.name).strip()
				val = str(child.content).strip()
				rule.AppendRule(cmd,val)
				child=child.next

			if comp=="Standard" and len(tz.stdcomponents)>0:
				if rule.startdate == None:
					rule.startdate = tz.stdcomponents[0].startdate
				tz.stdcomponents[0].rrules.append(rule)
			elif comp=="Daylight" and len(tz.dstcomponents)>0:
				if rule.startdate == None:
					rule.startdate = tz.dstcomponents[0].startdate
				tz.dstcomponents[0].rrules.append(rule)
			else:
				print "No matching component for rule"

			return True
		else:
			print "No timezone available for rule"
			return False
	else:
		return False


#
# TZToXML
#
# Dump a timezone to an XML node using the OpenSync > 0.30 schemas
#

def TZToXML(node,tz):
	
	# First create the <Timezone> element.
	
	el_tz = node.newChild(None,"Timezone",None)
	el_tz.setProp("TimezoneID",tz.name)
	el_tz.newChild(None,"X-Location",tz.loc)
	
	# Now create the components elements
	#
	# for standard
	
	for i in tz.stdcomponents:
		el_comp = node.newChild(None,"TimezoneComponent",None)
		el_comp.setProp("TimezoneID",tz.name)
		el_comp.setProp("TZComponent","Standard")
		for date in i.rdates:
			dateutil.DateToXMLDateTime(el_comp,"RecurrenceDateTime",date)
		print "STARTDATE"
		print i.startdate
		print "END STARTDATE"
		el_comp.newChild(None,"DateTimeStart",dateutil.DateToText(i.startdate))
		el_comp.newChild(None,"TZName",i.name)
		el_comp.newChild(None,"TZOffsetFrom",dateutil.TimedeltaToText(i.offsetfrom))
		el_comp.newChild(None,"TZOffsetTo",dateutil.TimedeltaToText(i.offset))
		
	# and for dst
		
	for i in tz.dstcomponents:
		el_comp = node.newChild(None,"TimezoneComponent",None)
		el_comp.setProp("TimezoneID",tz.name)
		el_comp.setProp("TZComponent","Daylight")
		for date in i.rdates:
			dateutil.DateToXMLDateTime(el_comp,"RecurrenceDateTime",date)
		el_comp.newChild(None,"DateTimeStart",dateutil.DateToText(i.startdate))
		el_comp.newChild(None,"TZName",i.name)
		el_comp.newChild(None,"TZOffsetFrom",dateutil.TimedeltaToText(i.offsetfrom))
		el_comp.newChild(None,"TZOffsetTo",dateutil.TimedeltaToText(i.offset))
	
	# Now dump the recurrence rules
	#
	# for standard
	
	for i in tz.stdcomponents:
		for rrule in i.rrules:
			el_rr = node.newChild(None,"TimezoneRule",None)
			el_rr.setProp("TimezoneID",tz.name)
			el_rr.setProp("TZComponent","Standard")
			dateutil.StandardRRuleToXML(el_rr,rrule)
	
	# and for dst
	
	for i in tz.dstcomponents:
		for rrule in i.rrules:
			el_rr = node.newChild(None,"TimezoneRule",None)
			el_rr.setProp("TimezoneID",tz.name)
			el_rr.setProp("TZComponent","Daylight")
			dateutil.StandardRRuleToXML(el_rr,rrule)

#
# TZFromAirsync
#
# Construct a VcalTZ class from an AirSync block. This function will not
# error out, but can generate a null timezone. Some AS timezones do not
# specify the year and this could be a problem.
#
	
def TZFromAirsync(asdata,year):
	
	(ASbias, 
	 ASstd_name,
	 ASstd_year, ASstd_month, ASstd_dow,
	 ASstd_occurrence, ASstd_start_hour, ASstd_minute, ASstd_second,
	 ASstd_millisecs, ASstd_bias, 
	 ASdst_name, 
	 ASdst_year, ASdst_month, ASdst_dow,
	 ASdst_occurrence, ASdst_start_hour, ASdst_minute, ASdst_second,
	 ASdst_millisecs, ASdst_bias) =  struct.unpack("<i64shhhhhhhhi64shhhhhhhhi",asdata)
			
	ASstd_name = ASstd_name.decode("utf_16_le").rstrip("\0")
	ASstd_dow = (ASstd_dow + 6) % 7
	ASdst_name = ASdst_name.decode("utf_16_le").rstrip("\0")
	ASdst_dow = (ASdst_dow + 6) % 7
		
	if ASstd_name == "":
		ASstd_name = "STD"
	if ASdst_name == "":
		ASdst_name = "DST"

		
	if ASdst_month!=0 and ASstd_month!=0:
		
		dst_sd = ASdst_occurrence
		dst_sy = ASdst_year

		# Calculate year and day values for DST
			
		if ASdst_year == 0:
			
			# we need to calculate the date from the month and year.
			
			d = datetime.datetime(year,ASdst_month,1,0,0,0)
			wd,ndays = calendar.monthrange(year,ASdst_month)
			dd = datetime.timedelta(days=1)
			while wd!=ASdst_dow:
				d+=dd
				wd = d.weekday()
			oc=1
			dd = datetime.timedelta(days=7)
			while oc < ASdst_occurrence:
				d  += dd
				oc += 1
				if (ndays - d.day) < 7:
					break
			dst_sd = d.day
			dst_sy = year
		
		std_sd = ASstd_occurrence
		std_sy = ASstd_year

		# Calculate year and day values for STD

		if ASstd_year == 0:

			# we need to calculate the date from the month and year.
			
			d = datetime.datetime(year,ASstd_month,1,0,0,0)
			wd,ndays = calendar.monthrange(year,ASstd_month)
			dd = datetime.timedelta(days=1)
			while wd!=ASstd_dow:
				d+=dd
				wd = d.weekday()
			oc=1
			dd = datetime.timedelta(days=7)
			while oc < ASstd_occurrence:
				d  += dd
				oc += 1
				if (ndays - d.day) < 7:
					break
			std_sd = d.day
			std_sy = year
	
		#
		# The timezone name
		#

		name = "/synce.org/DST-%02d%02d%02dT%02d%02d%02d-STD-%02d%02d%02dT%02d%02d%02d" \
		       % (ASdst_month,
        		  ASdst_dow,ASdst_occurrence,
		          ASdst_start_hour,ASdst_minute,ASdst_second,
		          ASstd_month,
			  ASstd_dow,ASstd_occurrence,
		          ASstd_start_hour,ASstd_minute,ASstd_second)
	
		# Build the document.
		# In OpenSync 0.3x, we break up the timezone structures. The first
		# element is the definition
		
		vctz = tzutils.tzone.VcalTZ(name)
		vctz.loc="Unknown/Unknown"
		
		# and each component has its rules attached. 
		# Start with the DST rule
		
		ds = datetime.datetime(dst_sy,ASdst_month,dst_sd,ASdst_start_hour,ASdst_minute,ASdst_second)
	
		dstComponent = tzutils.tzone.TZComponent()
		dstComponent.name = ASdst_name
		dstComponent.startdate = ds
		dstComponent.offsetfrom = datetime.timedelta(hours=-ASbias/60, minutes=ASstd_minute)
		dstComponent.offset = datetime.timedelta(hours=-(ASbias/60 + ASdst_bias/60), minutes=ASdst_minute)

		rule=tzutils.recurrence.RecurrentEvent(ds)
		rule.AppendRule("Frequency","YEARLY")
		rule.AppendRule("Count","0")
		rule.AppendRule("Interval","1")
		d=ASdst_occurrence
		if d==5:
			d=-1
		rule.AppendRule("ByDay","%d%s" % (d, dateutil.dowtxt[ASdst_dow]))
		rule.AppendRule("ByMonth","%d" % ASdst_month)
		
		dstComponent.rrules.append(rule)

		# add component to timezone

		vctz.dstcomponents.append(dstComponent)

		# and do the STD rule

		ds = datetime.datetime(std_sy,ASstd_month,std_sd,ASstd_start_hour,ASstd_minute,ASstd_second)

		stdComponent = tzutils.tzone.TZComponent()
		stdComponent.name = ASstd_name
		stdComponent.startdate = ds
		stdComponent.offsetfrom = datetime.timedelta(hours=-(ASbias/60 + ASdst_bias/60), minutes=ASstd_minute)
		stdComponent.offset = datetime.timedelta(hours=-ASbias/60,minutes=ASstd_minute)

		rule=tzutils.recurrence.RecurrentEvent(ds)
		rule.AppendRule("Frequency","YEARLY")
		rule.AppendRule("Count","0")
		rule.AppendRule("Interval","1")

		d=ASstd_occurrence
		if d==5:
			d=-1
		rule.AppendRule("ByDay","%d%s" % (d, dateutil.dowtxt[ASstd_dow]))
		rule.AppendRule("ByMonth","%d" % ASstd_month)
	
		stdComponent.rrules.append(rule)
		
		# add component to timezone
		
		vctz.stdcomponents.append(stdComponent)

		# return timezone
	
		return vctz
	else:
		return None


#
# TZToAirsync
#
# Construct an AirSync block from a VcalTZ class. This function will not
# error out, but can generate a null timezone.
#


def TZToAirsync(vctz):
	
	# initialize all parts of the AS timezone
	
	ASbias = 0
	ASstd_name = ""
	ASstd_year = 0
	ASstd_month = 0
	ASstd_dow = 0
	ASstd_occurrence = 0
	ASstd_hour = 0
	ASstd_minute = 0
	ASstd_second = 0
	ASstd_millisecs = 0
	ASstd_bias = 0
	ASdst_name = ""
	ASdst_year = 0
	ASdst_month = 0
	ASdst_dow = 0
	ASdst_occurrence = 0
	ASdst_hour = 0
	ASdst_minute = 0
	ASdst_second = 0
	ASdst_millisecs = 0
	ASdst_bias = 0
	
	# Here we only handle the first component of each type in the timezone
	# Currently not sure what to do with RDate lists, so we ignore them
	# for the moment until we can see how AirSunk handles them.
	
	if len(vctz.stdcomponents) > 0:
		
		stdComponent = vctz.stdcomponents[0]
		
		# we have a 'standard' timezone component
		
		ASstd_name = stdComponent.name.encode("utf_16_le") + "\x00\x00"
					
		if stdComponent.offset < datetime.timedelta(0):
			stdos = -stdComponent.offset
		else:
			stdos = stdComponent.offset
	
		# we use only the first rule we find.
		
		if len(stdComponent.rrules) > 0:
			
			rule = stdComponent.rrules[0]
	
			if len(rule.byDay) > 0:
				
				ASbias = stdos.days * 24 * 60 + stdos.seconds/60
				ASstd_year = rule.startdate.year
				
				if len(rule.byMonth) > 0:
					ASstd_month = rule.byMonth[0]
				else:
					ASstd_month = rule.startdate.month
					
				day,pos = rule.byDay[0]
				d = pos
				if d < 0:
					d = 5
				ASstd_dow = (day - 6) % 7
				ASstd_occurrence = d
				ASstd_hour = rule.startdate.hour
				ASstd_minute = rule.startdate.minute
	
	# Now do the DST stuff
	
	if len(vctz.dstcomponents) > 0:
		
		dstComponent = vctz.dstcomponents[0]
	
		# we have a daylight saving component
					
		ASdst_name = dstComponent.name.encode("utf_16_le") + "\x00\x00"
					
		os_dst = -dstComponent.offset
	
		if len(dstComponent.rrules) > 0:
			rule=dstComponent.rrules[0]
	
			if len(rule.byDay) > 0:
				
				ASdst_bias = os_dst.days * 24 * 60 + os_dst.seconds/60
				ASdst_year = rule.startdate.year
							
				if len(rule.byMonth) > 0:
					ASdst_month = rule.byMonth[0]
				else:
					ASdst_month = rule.startdate.month
					
				day,pos = rule.byDay[0]
				d=pos	
				if d < 0:
					d = 5
				ASdst_dow = (day - 6) % 7
				ASdst_occurrence = d
				ASdst_hour = rule.startdate.hour
				ASdst_minute = rule.startdate.minute

	print "bias             %d" % ASbias
	print "std_name         %s" % ASstd_name
	print "std_year         %d" % ASstd_year
	print "std_month        %d" % ASstd_month
	print "std_dow          %d" % ASstd_dow
	print "std_occurrence   %d" % ASstd_occurrence
	print "std_start_hour   %d" % ASstd_hour
	print "std_minute       %d" % ASstd_minute
	print "std_second       %d" % ASstd_second
	print "std_millisecs    %d" % ASstd_millisecs
	print "std_bias         %d" % ASstd_bias
	print "dst_name         %s" % ASdst_name
	print "dst_month        %d" % ASdst_month
	print "dst_dow          %d" % ASdst_dow
	print "dst_occurrence   %d" % ASdst_occurrence
	print "dst_start_hour   %d" % ASdst_hour
	print "dst_minute       %d" % ASdst_minute
	print "dst_second       %d" % ASdst_second
	print "dst_millisecs    %d" % ASdst_millisecs
	print "dst_bias         %d" % ASdst_bias

	return struct.pack("<i64shhhhhhhhi64shhhhhhhhi",
		            ASbias,
	                    ASstd_name,
			    ASstd_year, ASstd_month, ASstd_dow,
			    ASstd_occurrence, ASstd_hour, ASstd_minute, ASstd_second,
			    ASstd_millisecs, ASstd_bias, 
			    ASdst_name,
			    ASdst_year, ASdst_month, ASdst_dow,
			    ASdst_occurrence, ASdst_hour, ASdst_minute, ASdst_second,
			    ASdst_millisecs, ASdst_bias)
	
	
