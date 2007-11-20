# -*- coding: utf-8 -*-

import calendar
import datetime
import operator

MMASK = [31,29,31,30,31,30,31,31,30,31,30,31]
	
class TimeSpan:
	
	def __init__(self,start,end):
		self.starttime = start
		self.endtime   = end
		
	def SetDelta(self, delta):
		if self.starttime+delta >= self.starttime:
			self.endtime = self.starttime+delta
		else:
			raise ValueError("Invalid time delta")
	
	def SetFromStartAndEndTime(self,start,end):
		
		if start < end:
			self.starttime = start
			self.endtime   = end
		else:
			raise ValueError("Invalid start and end time")

	def GetDelta(self):
		return endtime-starttime

	def Intersect(self, span):

		if span.starttime < self.starttime:
			if span.endtime <= self.starttime:
				return None
			else:
				if span.endtime < self.endtime:
					return TimeSpan(self.starttime,span.endtime)
				else:
					return TimeSpan(self.starttime,self.endtime)
		else:
			if span.starttime >= self.endtime:
				return None
			else:
				if span.endtime < self.endtime:
					return TimeSpan(span.starttime,span.endtime)
				else:
					return TimeSpan(span.starttime,self.endtime)

	#
	# MonthIntersect
	#
	# From a set of month rules, compute a list of intersects with the current range
	
	def MonthIntersect(self,mrulelist):
		
		rlist = []
		
		# we make an assumption here: that the range never spans years.
		# (we can never have a freq > YEARLY
		
		for mrule in mrulelist:
			
			start = datetime.datetime(self.starttime.year,mrule,1,0,0,0)
			if mrule ==  12:
				end = datetime.datetime(self.starttime.year+1,1,1,0,0,0)
			else:
				end = datetime.datetime(self.starttime.year,mrule+1,1,0,0,0)
			
			isect = self.Intersect(TimeSpan(start,end))
			
			if isect!=None:
				rlist.append(isect)
			
		# no need to sort here if rules were pre-sorted
			
		return rlist
	
	#
	# YearWeekIntersect
	#
	# From a set of year week rules, compute a list of intersects with the 
	# current range
		
		
	def YearWeekIntersect(self,ywrulelist):
		
		rlist = []
		
		for weekno in ywrulelist:
			
			# Week 1 is the first week with four days or more:
	
			if weekno==0:
				weekno=1
	
			wd,mdays = calendar.monthrange(self.starttime.year,1)
			daydelta = datetime.timedelta(days=(weekno-1)*7)
	
			initdate = datetime.datetime(self.starttime.year,1,1,0,0,0)
			if wd > 3:
				initdate += datetime.timedelta(days=(7-wd))
				wd = 0
		
			initdate += daydelta
			enddate  = initdate + datetime.timedelta(days=7)
	
			if enddate.year > self.starttime.year:
				if initdate.year == self.starttime.year:
					enddate = datetime.datetime(year+1,1,1,0,0,0)
				else:
					# the year week is invalid, so ignore.
					
					continue
		
			isect=self.Intersect(TimeSpan(initdate,enddate))
			
			if isect != None:
				rlist.append(isect)
		
		return rlist
		
	#
	# YearDayIntersect
	#
	# From a set of year day rules, compute a list of intesects with the current
	# range
	
	def YearDayIntersect(self,ydrulelist):
		
		rlist = []
		
		if calendar.isleap(self.starttime.year):
			MMASK[1] = 29
		else:
			MMASK[1] = 28
		
		for yearday in ydrulelist:
			
			if (MMASK[1] == 29 and yearday > 366) or \
			   (MMASK[1] == 28 and yearday > 365):
				continue
			
			m = 0
			ydc = yearday
			while True:
				yc = ydc - MMASK[m]
				if yc <= 0:
					break
				ydc = yc
				m+=1
	
			isect=self.Intersect(TSFromStartAndDelta(datetime.datetime(self.starttime.year,m+1,ydc,0,0,0),datetime.timedelta(days=1)))
			
			if isect != None:
				rlist.append(isect)
				
		return rlist

	# 
	# MonthDayIntersect
	#
	# From a set of month day rules, compute a list of intersects with the 
	# current range

	def MonthDayIntersect(self,mdrule):
		
		rlist = []
			
		daydelta = datetime.timedelta(days=1)
		etc = datetime.datetime(self.starttime.year,self.starttime.month,1,0,0,0)
		
		while True:
			
			wd,ndays = calendar.monthrange(etc.year,etc.month)
						
			for monthday in mdrule:
				
				if monthday > ndays:
					continue	# ignore what we don't have
				
				# get the intersect region
				
				etc=etc.replace(day=monthday)
				
				print etc
				
				isect = self.Intersect(TSFromStartAndDelta(etc,daydelta))
				
				if isect != None:
					rlist.append(isect)
					
			delta = datetime.timedelta(days=ndays)
			etc += delta	
			if etc > self.endtime:
				break
			
		return rlist
		
	#
	# WeekdayIntersect
	#
	# Returns a list of TimeSpan objects where the weekday rules intersect the 
	# current range.

	def WeekdayIntersect(self,wdrulelist):
		
		rlist =  []
		
		for weekday,posrule in wdrulelist:
		
			tslist = []

			# get the first instance. Remember that end times are 
			# noninclusive
		
			daydelta = datetime.timedelta(days=1)
		
			start = self.starttime
			if start.weekday() != weekday:
				start=start.replace(hour=0,minute=0,second=0)
				while start.weekday() != weekday and start < self.endtime:
					start+=daydelta
		
			if start <= self.endtime:
				end   = start + daydelta
				end = end.replace(hour=0,minute=0,second=0)
				if end > self.endtime:
					end = self.endtime
				inst1 = TimeSpan(start,end)
			else:
				return []	# no instances exist
		
			# get the last instance (could be the same as the first!)
			# We must deal with the instance where the end time is the first second
			# in a new day
				
			start = self.endtime - datetime.timedelta(seconds=1)
			start = start.replace(hour=0,minute=0,second=0)
		
			if start.weekday() != weekday:
				while start.weekday() != weekday and start > self.starttime:
					start -= daydelta
			
			if start < self.starttime:
				start = self.starttime
						
			end = start + daydelta
			end.replace(hour=0,minute=0,second=0)
			if end > self.endtime:
				end = self.endtime
			instL = TimeSpan(start,end)
		
			# Now how many instances do we have (in days)
			
			weekdelta = datetime.timedelta(days=7)
		
			if posrule == 0:
			
				# we need 'em all
	
				tslist.append(inst1)
						
				inststart = inst1.starttime.replace(hour=0,minute=0,second=0)
			
				while True:
					inststart+=weekdelta
					if inststart < instL.starttime:
						tslist.append(TimeSpan(inststart,inststart+daydelta))
					else:
						break
	
				if inst1.starttime != instL.starttime:
					tslist.append(instL)
				
			else:
				if posrule > 0:
				
					# need only the nth one in the set.
				
					if posrule == 1:
						tslist.append(inst1)
					else:
						posrule -= 1
						inststart = inst1.starttime.replace(hour=0,minute=0,second=0)
			
						while True:
							inststart+=weekdelta
							posrule -= 1;
							if inststart < instL.starttime:
								if posrule == 0:
									tslist.append(TimeSpan(inststart,inststart+daydelta))
									break
							else:
								if posrule == 0:
									tslist.append(instL)
								break

				else:
	
					# need the nth one from the end
				
					if posrule == -1:
						tslist.append(instL)
					else:
						
						posrule += 1
						lastinst=instL.starttime.replace(hour=0,minute=0,second=0)
					
						while True:
							lastinst -= weekdelta
							posrule += 1
							if lastinst > inst1.starttime:
								if posrule == 0:
									tslist.append(TimeSpan(lastinst,lastinst+daydelta))
									break
							else:
								if posrule == 0:
									tslist.append(inst1)
								break
							
			rlist+=tslist
		
		# Sort it. We could do this on a per-rule basis, but it's a ball-ache and is probably not that
		# much more efficient.
		
		rlist.sort(key=operator.attrgetter('starttime'))
		return rlist


	#
	# HourIntersect
	#
	# Process a list of hour rules, returning a list of intersections with
	# the current range.
	#
	
	def HourIntersect(self,hourrulelist):
		
		rlist = []
		
		# Get the first hour interval
		
		hrdelta = datetime.timedelta(hours=1)
		ddelta  = datetime.timedelta(days=1)
		
		dst = datetime.datetime(self.starttime.year,self.starttime.month,self.starttime.day,0,0,0)
		
		while True:
			
			for hourrule in hourrulelist:
				if hourrule > 23:
					continue
				
				dst=dst.replace(hour=hourrule)
				
				tsval = TSFromStartAndDelta(dst,hrdelta)
				
				isect=self.Intersect(tsval)
				if isect != None:
					rlist.append(isect)
			dst += ddelta;
			if dst >= self.endtime:
				break
		
		return rlist

	#
	# MinuteIntersect
	#
	# Process a list of minute rules, returning a list of intersections with
	# the current range.
	#
	
	def MinuteIntersect(self,minuterulelist):
				
		rlist = []
		
		# Get the first minute interval
		
		mindelta = datetime.timedelta(minutes=1)
		hrdelta = datetime.timedelta(hours=1)
		
		dst = datetime.datetime(self.starttime.year,self.starttime.month,self.starttime.day,self.starttime.hour,0,0)
		
		while True:
			
			for minuterule in minuterulelist:
				
				if minuterule > 59:
					continue
				
				dst=dst.replace(minute=minuterule)
				
				tsval = TSFromStartAndDelta(dst,mindelta)
				
				isect=self.Intersect(tsval)
				
				if isect != None:
					rlist.append(isect)
					
			dst += hrdelta;
					
			if dst >= self.endtime:
				break
		
		return rlist

	#
	# SecondIntersect
	#
	# Process a list of second rules, returning a list of intersections with
	# the current range.
	#
	
	def SecondIntersect(self,secondrulelist):
				
		rlist = []
		
		# Get the first minute interval
		
		mindelta = datetime.timedelta(minutes=1)
		secdelta = datetime.timedelta(seconds=1)
		
		dst = datetime.datetime(self.starttime.year,self.starttime.month,self.starttime.day,self.starttime.hour,self.starttime.minute,0)
				
		while True:
			
			for secondrule in secondrulelist:
				if secondrule > 59:
					continue
				
				dst=dst.replace(second=secondrule)
				
				tsval = TSFromStartAndDelta(dst,secdelta)
				
				isect=self.Intersect(tsval)
				if isect != None:
					rlist.append(isect)
					
			dst += mindelta;
					
			if dst >= self.endtime:
				break
		
		return rlist

#
# TSFromStartAndDelta
#
# Factory for a TimeSpan specified by start and elapsed time
#

def TSFromStartAndDelta(start,delta):
	return TimeSpan(start,start+delta)

#
# TSYearDayfromDate
#
# Calculate the year day from a date

def TSYearDayFromDate(date):
	
	if calendar.isleap(date.year):
		MMASK[1] = 29
	else:
		MMASK[1] = 28
	yd=0
	for i in range(date.month-1):
		yd+=MMASK[i]
	yd+=date.day
	
	return yd
	