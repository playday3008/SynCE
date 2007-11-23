# -*- coding: utf-8 -*-
############################################################################
# TZCONV.py
#
# Timezone conversion utilities
#
# Dr J A Gow 29/3/2007
#
############################################################################

import libxml2
import libxslt
import string
import struct
import datetime
import tzutils
import SyncEngine.xml2util as xml2util
import base64
import SyncEngine.util
import calendar

# The global timezone database. This exists for the runtime life of a 
# sync-engine session and is updated when necessary/

TZDB = {}
CUR_TZ = {"current":None}


class ASTimezoneData:
	
	def __init__(self):
		self._zero()
		self.tzcurrent = None
		
	def _zero(self):
		self.bias = 0
		self.std_name = ""
		self.std_year = 0
		self.std_month = 0
		self.std_dow = 0
		self.std_occurrence = 0
		self.std_start_hour = 0
		self.std_minute = 0
		self.std_second = 0
		self.std_millisecs = 0
		self.std_bias = 0
		self.dst_name = ""
		self.dst_year = 0
		self.dst_month = 0
		self.dst_dow = 0
		self.dst_occurrence = 0
		self.dst_start_hour = 0
		self.dst_minute = 0
		self.dst_second = 0
		self.dst_millisecs = 0
		self.dst_bias = 0
		
	def dump(self):
	
		print "bias             %d" % self.bias
		print "std_name         %s" % self.std_name
		print "std_year         %d" % self.std_year
		print "std_month        %d" % self.std_month
		print "std_dow          %d" % self.std_dow
		print "std_occurrence   %d" % self.std_occurrence
		print "std_start_hour   %d" % self.std_start_hour
		print "std_minute       %d" % self.std_minute
		print "std_second       %d" % self.std_second
		print "std_millisecs    %d" % self.std_millisecs
		print "std_bias         %d" % self.std_bias
		print "dst_name		%s" % self.dst_name
		print "dst_month        %d" % self.dst_month
		print "dst_dow          %d" % self.dst_dow
		print "dst_occurrence   %d" % self.dst_occurrence
		print "dst_start_hour   %d" % self.dst_start_hour
		print "dst_minute       %d" % self.dst_minute
		print "dst_second       %d" % self.dst_second
		print "dst_millisecs    %d" % self.dst_millisecs
		print "dst_bias         %d" % self.dst_bias

	def UnpackFromAirsync(self, stream):
		
		(self.bias, 
		 stdname,
		 self.std_year, self.std_month, stddow,
		 self.std_occurrence, self.std_start_hour, self.std_minute, self.std_second,
		 self.std_millisecs, self.std_bias, 
		 dstname, 
		 self.dst_year, self.dst_month, dstdow,
		 self.dst_occurrence, self.dst_start_hour, self.dst_minute, self.dst_second,
		 self.dst_millisecs, self.dst_bias) =  struct.unpack("<i64shhhhhhhhi64shhhhhhhhi",stream)

		self.std_name = SyncEngine.util.decode_wstr(stdname)
		self.std_dow = (stddow + 6) % 7
		self.dst_name = SyncEngine.util.decode_wstr(dstname)
		self.dst_dow = (dstdow + 6) % 7
		
		if self.std_name == "":
			self.std_name = "STD"
		if self.dst_name == "":
			self.dst_name = "DST"
		
	def PackToAirsync(self):
		
		stdname = SyncEngine.util.encode_wstr(self.std_name)
		dstname = SyncEngine.util.encode_wstr(self.dst_name)
		stddow = (self.std_dow - 6) % 7
		dstdow = (self.dst_dow - 6) % 7
		
		return struct.pack("<i64shhhhhhhhi64shhhhhhhhi",
				    self.bias,
	                            stdname,
				    self.std_year, self.std_month, stddow,
				    self.std_occurrence, self.std_start_hour, self.std_minute, self.std_second,
				    self.std_millisecs, self.std_bias, 
				    dstname,
				    self.dst_year, self.dst_month, dstdow,
				    self.dst_occurrence, self.dst_start_hour, self.dst_minute, self.dst_second,
				    self.dst_millisecs, self.dst_bias)

	def FromVcalTZ(self,tz):
		
		self._zero()
		
		if tz.stdoffset < datetime.timedelta(0):
			stdos = -tz.stdoffset
		else:
			stdos = tz.stdoffset
		
		if len(tz.stdRecurrence.byDay) > 0:
			self.bias = stdos.days * 24 * 60 + stdos.seconds/60
			if len(tz.stdRecurrence.byMonth) > 0:
				self.std_month = tz.stdRecurrence.byMonth[0]
			else:
				self.std_month = tz.stdRecurrence.startdate.month
			day,pos = tz.stdRecurrence.byDay[0]
			d = pos
			if d < 0:
				d = 5
			self.std_dow = day
			self.std_occurrence = d
			self.std_hour = tz.stdRecurrence.startdate.hour
			self.std_minute = tz.stdRecurrence.startdate.minute
		
			if len(tz.dstRecurrence.byDay) > 0:
				
				os_dst = -tz.dstoffset
				self.dst_bias = os_dst.days * 24 * 60 + os_dst.seconds/60
				if len(tz.dstRecurrence.byMonth) > 0:
					self.dst_month = tz.dstRecurrence.byMonth[0]
				else:
					self.dst_month = tz.dstRecurrence.startdate.month
				day,pos = tz.dstRecurrence.byDay[0]
				d=pos
				if d < 0:
					d = 5
				self.dst_dow = day
				self.dst_occurrence = d
				self.dst_hour=tz.dstRecurrence.startdate.hour
				self.dst_minute = tz.dstRecurrence.startdate.minute
	
	def ToVcalTZ(self,node,year):
		
		#
		# Now, if we do not have a DST definition, then the timezone
		# field is of no use to us as we can not just specify offsets. So
		# do not generate anything if the self.dst_month or self.std_month 
		# is zero. 
	
		if self.dst_month!=0 and self.std_month!=0:
	
			# Calculate year and day values for DST
			
			dst_sd = self.dst_occurrence
			dst_sy = self.dst_year
	
			if self.dst_year == 0:
			
				# we need to calculate the date from the month and year.
			
				d = datetime.datetime(year,self.dst_month,1,0,0,0)
				wd,ndays = calendar.monthrange(year,self.dst_month)
				dd = datetime.timedelta(days=1)
				while wd!=self.dst_dow:
					d+=dd
					wd = d.weekday()
				oc=1
				dd = datetime.timedelta(days=7)
				while oc < self.dst_occurrence:
					d  += dd
					oc += 1
					if (ndays - d.day) < 7:
						break
				dst_sd = d.day
				dst_sy = year
				
			# Calculate year and day values for STD

			std_sd = self.std_occurrence
			std_sy = self.std_year
			
			if self.std_year == 0:

				# we need to calculate the date from the month and year.
			
				d = datetime.datetime(year,self.std_month,1,0,0,0)
				wd,ndays = calendar.monthrange(year,self.std_month)
				dd = datetime.timedelta(days=1)
				while wd!=self.std_dow:
					d+=dd
					wd = d.weekday()
				oc=1
				dd = datetime.timedelta(days=7)
				while oc < self.std_occurrence:
					d  += dd
					oc += 1
					if (ndays - d.day) < 7:
						break
				std_sd = d.day
				std_sy = year

			#
			# The timezone ID (for vcal)

			tzid = "/synce.org/DST-%02d%02d%02dT%02d%02d%02d-STD-%02d%02d%02dT%02d%02d%02d" \
				   % (self.dst_month,
				      self.dst_dow,self.dst_occurrence,
				      self.dst_start_hour,self.dst_minute,self.dst_second,
				      self.std_month,
				      self.std_dow,self.std_occurrence,
				      self.std_start_hour,self.std_minute,self.std_second)

			# Build the document.

			tznode=node.newChild(None,"Timezone",None)
				      
			tznode.newChild(None,"TimezoneID",tzid)
			tznode.newChild(None,"Location","Unknown/Unknown")
		
			dstnode = tznode.newChild(None,"DaylightSavings",None)
			osf = "%+03d%02d" % (-self.bias/60, self.dst_minute)
			dstnode.newChild(None,"TZOffsetFrom", osf)
			ost = "%+03d%02d" % (-(self.bias/60 + self.dst_bias/60),self.dst_minute)
			dstnode.newChild(None,"TZOffsetTo",ost)
			dstnode.newChild(None,"TimezoneName",self.dst_name)
		
		
			ds = "%4d%02d%02dT%02d%02d%02d" % (dst_sy,self.dst_month,dst_sd,
			                                   self.dst_start_hour,self.dst_minute,
							   self.dst_second)
			dstnode.newChild(None,"DateStarted",ds)
			dst_rr = dstnode.newChild(None,"RecurrenceRule",None)
			dst_rr.newChild(None,"Rule","FREQ=YEARLY")
			dst_rr.newChild(None,"Rule","INTERVAL=1")
			dst_rr.newChild(None,"Rule","BYMONTH=%d" % self.dst_month)
			
			d=self.dst_occurrence
			if d==5:
				d=-1
			
			dst_rr.newChild(None,"Rule","BYDAY=%d%s" % (d, tzutils.dowtxt[self.dst_dow]))
		
			stdnode = tznode.newChild(None,"Standard",None)
			osf = "%+03d%02d" % (-(self.bias/60 + self.dst_bias/60), self.dst_minute)
			stdnode.newChild(None,"TZOffsetFrom", osf)
			ost = "%+03d%02d" % (-self.bias/60,self.dst_minute)
			stdnode.newChild(None,"TZOffsetTo",ost)
			stdnode.newChild(None,"TimezoneName",self.std_name)
		
			
			ds = "%4d%02d%02dT%02d%02d%02d" % (std_sy,self.std_month,std_sd,
			                                   self.std_start_hour,self.std_minute,
							   self.std_second)		
		
			stdnode.newChild(None,"DateStarted",ds)
			std_rr = stdnode.newChild(None,"RecurrenceRule",None)
			std_rr.newChild(None,"Rule","FREQ=YEARLY")
			std_rr.newChild(None,"Rule","INTERVAL=1")
			std_rr.newChild(None,"Rule","BYMONTH=%d" % self.std_month)
			
			d=self.std_occurrence
			
			if d==5:
				d=-1
			
			std_rr.newChild(None,"Rule","BYDAY=%d%s" % (d, tzutils.dowtxt[self.std_dow]))

			# Now, we can generate a new actual timezone based on this.
			
			self.tzcurrent = tzutils.tzone.TZInfoFromVcal(tznode)

			print "CONFIG - DST OFFSET ", self.tzcurrent.dstoffset


def FindASDateElementContent(doc,element):
	
	up = xml2util.FindChildNode(doc,element)
	if up != None:
		return xml2util.GetNodeValue(up)
	else:
		return None


def curtz():
	return CUR_TZ["current"]

###############################################################################
# ConvertASTimezoneToVcal
#
# Take an AS timezone, if fully specified, and convert it to a valid
# (but unknown) VCAL timezone.
#

def ConvertASTimezoneToVcal(ctx):
	
    parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
    src_node = transform_ctx.current()
    dst_node = transform_ctx.insertNode()
    s=xml2util.GetNodeValue(transform_ctx.current())
    if len(s) > 0:
        dcas = base64.b64decode(s)
	base_node = transform_ctx.insertNode()
	astd = ASTimezoneData()
	astd.UnpackFromAirsync(dcas)
	astd.dump()

	sdate = FindASDateElementContent(src_node.parent,"StartTime")
	
	if sdate ==None:
		sdate=datetime.datetime.now()
		year = sdate.year
	else:
		year = int(sdate[0:4])
	
	astd.ToVcalTZ(dst_node,year)
	CUR_TZ["current"] = astd.tzcurrent
		
	# We now have a (hopefully valid) timezone entry as the current 
	# timezone. We need to back-convert the timezoneable dates as 
	# they appear.
	
    return ""

###############################################################################
# ExtractTZData
#
# Extract the timezone data from a vcal node block and populate our internal
# timezone database.

def ExtractTZData(ctx):
	
	parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
	tznode = transform_ctx.current()
	
	timezone = tzutils.tzone.TZInfoFromVcal(tznode)
	TZDB[timezone.name]=timezone
	astd = ASTimezoneData()
	astd.FromVcalTZ(timezone)
	astd.dump()
	return base64.b64encode(astd.PackToAirsync())
	
###############################################################################
# ConvertToUTC
#
# Convert a provided local date to UTC taking into account daylight-savings	

def ConvertToUTC(date,tzid):
	
	if TZDB.has_key(tzid):
	
		tz = TZDB[tzid]
		time = date.replace(tzinfo=tz)
		utc_date = time.astimezone(tzutils.tzone.utc)
	else:
		utc_date = date
		
	return utc_date
	
###############################################################################
# ConvertToLocal
#
# Convert a provided UTC date to local time, taking into account daylight
# savings. The arg list is different here as we take an actual timezone
# as an argument, not an index (we maintain no list in the reverse direction
# as every timezone on the AS side could, in fact, be different. If the 
# supplied timezone is None - the returned time is unchanged

def ConvertToLocal(date,tz):
	
	if tz!=None:
		time = date.replace(tzinfo=tzutils.tzone.utc)
		local_date = time.astimezone(tz)
	else:
		local_date = date
		
	return local_date

	
###############################################################################
# ConvertDateNodeToUTC
#
# Takes a node containing a <Content> and a <Timezone>. Assumes the <Content>
# is a date. If <TimezoneID> is specified, then will return a UTC representation
# of the string providing the TimezoneID is in the timezone database. If no
# TimezoneID is provided, will assume UTC. Also, if date ends with 'Z', will 
# assume UTC from read. Can return None if datetime not present

def ConvertDateNodeToUTC(node):
	date = None
	tdate = None
	udate = None
	ctnode = xml2util.FindChildNode(node,"Content")
	if ctnode != None:
		tdate = xml2util.GetNodeValue(ctnode).upper()
		if tdate != "":
			date = tzutils.TextToDate(tdate)
			udate = date
			if date.tzname() != "UTC":
				tzn=xml2util.FindChildNode(node,"TimezoneID")
				if tzn!=None:
					udate = ConvertToUTC(date,xml2util.GetNodeValue(tzn))
	else:
		raise ValueError("Bad date content")
	return date,udate
	
###############################################################################
# RegisterXSLTExtensionFunctions
#
# Register timezone-related functions for the parser
	
def RegisterXSLTExtensionFunctions():

	libxslt.registerExtModuleFunction("ExtractTZData"          ,"http://synce.org/tz", ExtractTZData)
	libxslt.registerExtModuleFunction("ConvertASTimezoneToVcal","http://synce.org/tz", ConvertASTimezoneToVcal)

  
  
