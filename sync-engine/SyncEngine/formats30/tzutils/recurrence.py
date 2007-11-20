# -*- coding: utf-8 -*-
############################################################################
# RECURRENCE.py
#
# Recurrence rule processors
#
# Dr J A Gow 29/3/2007
#
############################################################################

import string
import datetime
import timespan
import calendar
import libxml2

WEEKDAYS = {"MO" : 0, "TU" : 1, "WE" : 2, "TH" : 3, "FR" : 4, "SA" : 5, "SU" : 6 }
WEEKDAYLIST = ["MO","TU","WE","TH","FR","SA","SU"]
FREQS    = { "SECONDLY":6, "MINUTELY":5, "HOURLY":4, "DAILY":3, "WEEKLY":2, "MONTHLY":1, "YEARLY":0 }
FREQLIST = ["YEARLY","MONTHLY","WEEKLY","DAILY","HOURLY","MINUTELY","SECONDLY"]

(YEARLY,MONTHLY,WEEKLY,DAILY,HOURLY,MINUTELY,SECONDLY) = range(7)

###############################################################################
# _TextToDate
#
# INTERNAL
#
# Utility, timezone agnostic

def _TextToDate(text):
	
	text = text.upper()
	
	if len(text) < 8:
		raise ValueError("Corrupted date string")
			
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
	
	return datetime.datetime(year,month,day,hour,minute,second,0,tzinfo=None)

###############################################################################
# RecurrentEvent
#
# Class defining a recurrent event

class RecurrentEvent:

	def __init__(self,start=None):
		self.startdate = start
		self.ruleset = []
		self.ClearRuleSet()
		self.currentEvent = None
		
	def ClearRuleSet(self):
		self.freq       = None
		self.interval   = 1
		self.count      = 0
		self.until      = None
		self.stop	= None
		
		# Rules in order of processing
		
		self.byMonth    = []
		self.byWeekNo   = []
		self.byYearDay  = []
		self.byMonthDay = []
		self.byDay      = []
		self.byHour     = []
		self.byMinute   = []
		self.bySecond   = []
		
		# internal state
		
		self.evCount = 0
		self.ilist = []
	
	#
	# AppendRule
	#
	# Mechanism to allow the recurrence ruleset to be built up
	# through multiple calls to this function specifying the
	# command and the rule. Multiple calls to the same
	# command will overwrite the previous rule. Unrecognized 
	# commands are just ignored; the function will return false 
	# in this case.
	
	def AppendRule(self,cmd,rule):
				
		if cmd == "Frequency":
			self.freq = YEARLY
			if rule in FREQS.keys():
				self.freq = FREQS[rule]
				return True
			return False
		
		if cmd == "Interval":
			self.interval = int(rule)
			return True
		
		if cmd == "Count":
			self.count = int(rule)
			return True
		
		if cmd == "Until":
			self.until = _TextToDate(rule)
			return True
		
		if cmd == "ByMonth":
			self.byMonth += list(int(x) for x in rule.split(','))
			self.byMonth.sort()
			return True
		
		if cmd == "ByWeekNo":
			self.byWeekNo += list(int(x) for x in rule.split(','))
			self.byWeekNo.sort()
			return True
		
		if cmd == "ByDay":
			self.byDay = []
			l=list(rule.split(','))
			for x in l:
				if len(x) >=2:
					day = x.lstrip("-+1234567890")
					if WEEKDAYS.has_key(day):
						d = WEEKDAYS[day]
						pos = 0
						if len(x) > 2:
							pos = (int(x[0:len(x)-2]))
						self.byDay.append((d,pos))
					else:
						return False
			self.byDay.sort()
			return True
						
		if cmd == "ByMonthDay":
			self.byMonthDay += list(int(x) for x in rule.split(','))
			self.byMonthDay.sort()
			return True
		
		if cmd == "ByYearDay":
			self.byYearDay += list(int(x) for x in rule.split(','))
			self.byYearDay.sort()
			return True
		
 		if cmd == "ByHour":
			self.byHour += list(int(x) for x in rule.split(','))
			self.byHour.sort()
			return True
		
		if cmd == "ByMinute":
			self.byMinute += list(int(x) for x in rule.split(','))
			self.byMinute.sort()
			return True
		
		if cmd == "BySecond":
			self.bySecond += list(int(x) for x in rule.split(','))
			self.bySecond.sort()
			return True
		
		return False
		
	def AppendStringRule(self,rule):
				
		cmd,val = rule.split("=")
		
		if cmd == "FREQ":
			self.freq = YEARLY
			if val in FREQS.keys():
				self.freq = FREQS[val]
				return True
			return False
		
		if cmd == "INTERVAL":
			self.interval = int(val)
			return True
		
		if cmd == "COUNT":
			self.count = int(val)
			return True
		
		if cmd == "UNTIL":
			self.until = _TextToDate(val)
			return True
		
		if cmd == "BYMONTH":
			self.byMonth += list(int(x) for x in val.split(','))
			self.byMonth.sort()
			return True
		
		if cmd == "BYWEEKNO":
			self.byWeekNo += list(int(x) for x in val.split(','))
			self.byWeekNo.sort()
			return True
		
		if cmd == "BYDAY":
			self.byDay = []
			l=list(val.split(','))
			for x in l:
				if len(x) >=2:
					day = x.lstrip("-+1234567890")
					if WEEKDAYS.has_key(day):
						d = WEEKDAYS[day]
						pos = 0
						if len(x) > 2:
							pos = (int(x[0:len(x)-2]))
						self.byDay.append((d,pos))
					else:
						return False
			self.byDay.sort()
			return True
						
		if cmd == "BYMONTHDAY":
			self.byMonthDay += list(int(x) for x in val.split(','))
			self.byMonthDay.sort()
			return True
		
		if cmd == "BYYEARDAY":
			self.byYearDay += list(int(x) for x in val.split(','))
			self.byYearDay.sort()
			return True
		
 		if cmd == "BYHOUR":
			self.byHour += list(int(x) for x in val.split(','))
			self.byHour.sort()
			return True
		
		if cmd == "BYMINUTE":
			self.byMinute += list(int(x) for x in val.split(','))
			self.byMinute.sort()
			return True
		
		if cmd == "BYSECOND":
			self.bySecond += list(int(x) for x in val.split(','))
			self.bySecond.sort()
			return True
		
		return False
		
	#####################################################################
	# PROCESSOR FUNCTIONS
	#
	# This group of functions distil the ranges according to the rules
	
	#
	# _ProcessSecondRules
	#
	# Handle the second rules, using the timespans provided
	# If we get here, the maximum range in the list of spans 
	# will be one minute. It may be less. We never have ranges of
	# less than one second - and the start time is used once these have
	# been resolved
	
	def _ProcessSecondRules(self,tslist):
				
		rules = self.bySecond
				
		if self.freq < SECONDLY:
			if len(rules) == 0:
				rules.append(self.startdate.second)

		if len(rules) == 0:
			
			# pass through
			
			for l in tslist:
				if l.starttime >= self.stop:
					return False
				if self.count:
					self.evCount += 1
					if self.evCount > self.count:
						return False

				self.ilist.append(l.starttime)
		else:
			for l in tslist:
				rlist = l.SecondIntersect(rules)
				for r in rlist:				
					if r.starttime >= self.stop:
						return False
					
					if self.count:
						self.evCount += 1
						if self.evCount > self.count:
							return False
					
					self.ilist.append(r.starttime)
		return True
					
			
	#
	# _ProcessMinuteRules
	#
	# Handle the minute rules, using the timespans provided
	
	def _ProcessMinuteRules(self,tslist):
		
		rules = self.byMinute
		
		if self.freq < MINUTELY:
			if len(rules) == 0:
				rules.append(self.startdate.minute)
		
		if len(rules) == 0:
			if not self._ProcessSecondRules(tslist):
				return False
		else:
			for l in tslist:
				if not self._ProcessSecondRules(l.MinuteIntersect(rules)):
					return False
				
		return True
	
	#
	# _ProcessHourRules
	#
	# Handle the hour rules, using the timespans provided
	
	def _ProcessHourRules(self,tslist):
			
		rules = self.byHour
		
		if self.freq < HOURLY:
			if len(rules) == 0:
				rules.append(self.startdate.hour)
				
		if len(rules) == 0:
			if not self._ProcessMinuteRules(tslist):
				return False
		else:
			for l in tslist:
				if not self._ProcessMinuteRules(l.HourIntersect(rules)):
					return False
				
		return True

	#
	# _ProcessByDayRule
	#
	# Handle the BYDAY rules
	
	def _ProcessByDayRules(self,tslist):
		
		rules = self.byDay
		
		if self.freq <= WEEKLY :
			if len(rules) == 0 and len(self.byMonthDay) == 0 and len(self.byYearDay) == 0:
				wd = calendar.weekday(self.startdate.year,self.startdate.month,self.startdate.day)
				rules.append((wd,0))
								
		if len(rules) == 0:
			if not self._ProcessHourRules(tslist):
				return False
		else:
			for l in tslist:
				if not self._ProcessHourRules(l.WeekdayIntersect(rules)):
					return False

		return True

	# 
	# _ProcessByMonthDayRules
	#
	# Handle the BYMONTHDAY rules
	
	def _ProcessByMonthDayRules(self,tslist):
		
		rules = self.byMonthDay
		
		if self.freq < WEEKLY:
			if len(rules)==0 and len(self.byDay)==0 and len(self.byYearDay)==0:
				rules.append(self.startdate.day)
				
		if len(rules) == 0:
			if not self._ProcessByDayRules(tslist):
				return False
		else:
			for l in tslist:
				if not self._ProcessByDayRules(l.MonthDayIntersect(rules)):
					return False

		return True
		
	#
	# _ProcessByYearDayRules
	#
	# Handle the ByYearDay rules
	
	def _ProcessByYearDayRules(self,tslist):
		
		rules = self.byYearDay
		
# -rule checked elsewhere-
#		if self.freq < DAILY:
#			if len(rules)==0 and len(self.byMonthDay)==0 and len(self.byDay)==0:
#				
#				# calculate year day of startdate.
#				rules.append(timespan.TSYearDayFromDate(self.startdate))
#-------------------------
				
		if len(rules) == 0:
			if not self._ProcessByMonthDayRules(tslist):
				return False
		else:
			for l in tslist:
				if not self._ProcessByMonthDayRules(l.YearDayIntersect(rules)):
					return False

			
		return True
		
	#
	# _ProcessByWeekNoRules
	#
	# Handle the ByWeekNo rules. These are only valid for a YEARLY freq.
	
	def _ProcessByWeekNoRules(self,tslist):
				
		if self.freq != YEARLY or len(self.byWeekNo)==0:
			if not self._ProcessByYearDayRules(tslist):
				return False
		else:
			for l in tslist:
				if not self._ProcessByYearDayRules(l.YearWeekIntersect(self.byWeekNo)):
					return False
		
		return True

	# 
	# _ProcessByMonthRules
	#
	# Handle the BYMONTH rules
	
	def _ProcessByMonthRules(self,tslist):
		
		rules = self.byMonth
		
		if self.freq < MONTHLY:
			if len(rules) == 0 and (len(self.ByYearDay)==0 and self.freq == YEARLY):
				rules.append(self.startdate.month)
				
		if len(rules) == 0:
			if not self._ProcessByWeekNoRules(tslist):
				return False
		else:
			for l in tslist:
				if not self._ProcessByWeekNoRules(l.MonthIntersect(rules)):
					return False

		return True

	#
	# ProcessIteration
	#
	# Main iterator function to generate the event set.

	def ProcessIteration(self,enddate,datefrom=None):
		
		if self.freq == None:
			return
		
		# end time
		
		if self.until == None:
			self.stop = enddate
		else:
			if enddate < self.until:
				self.stop = enddate
			else:
				self.stop = self.until
		
		# calculate the exact start date
		
		if datefrom != None:
			if datefrom < self.startdate:
				datestart = self.startdate
			else:
				datestart = datefrom
		else:
			if len(self.ilist) > 0:
				datestart = self.ilist[len(self.ilist)-1]
			else:
				datestart = self.startdate
				
		# Now, process according to timespan, by frequency and interval
		
		st = datestart.replace(hour=0,second=0,minute=0)
		et = st
		
		if self.freq == YEARLY:
			st = st.replace(month=1,day=1)
			et = et.replace(year=datestart.year+1,month=1,day=1)
		
		elif self.freq == MONTHLY:
			
			m = datestart.month
			y = datestart.year
			if m == 12:
				y+=1
				m=1
			else:
				m+=1
			st = st.replace(day=1)
			et = et.replace(year=y,month=m,day=1)
			
		elif self.freq == WEEKLY:
			
			wd = datestart.weekday()
			ddelta = datetime.timedelta(days=wd)
			st -= ddelta
			ddelta = datetime.timedelta(days=7)
			et = st+ddelta
			
		elif self.freq == DAILY:
			
			ddelta = datetime.timedelta(days=1)
			et = st + ddelta
			
		elif self.freq == HOURLY:

			ddelta = datetime.timedelta(hours=1)
			et = st + ddelta
			
		elif self.freq == MINUTELY:
			
			st=st.replace(hour=datestart.hour)
			ddelta = datetime.timedelta(minutes=1)
			et = st + ddelta
			
		else:
			# treat all unknowns as SECONDLY
			
			self.freq=SECONDLY
			
			st = st.replace(hour=datestart.hour,minute=datestart.minute)
			ddelta = datetime.timedelta(seconds=1)
			et = st + ddelta

		if st < datestart:
			st = datestart

		ts = timespan.TimeSpan(st,et)

		# Now we can iterate.
		
		while ts.starttime < self.stop:
						
			if not self._ProcessByMonthRules([ts]):
				break	# we have reached the endpoint
		
			# now handle the iteration.
			
			ts.starttime = ts.endtime
			
			if self.freq == YEARLY:
				ts.starttime = ts.starttime.replace(year=ts.starttime.year+(self.interval-1))
				ts.endtime   = ts.endtime.replace(year=ts.starttime.year+1)
			elif self.freq == MONTHLY:
				for i in range(self.interval-1):
					wd,nmth = calendar.monthrange(ts.starttime.year,ts.starttime.month)
					ddelta = datetime.timedelta(days=nmth)
					ts.starttime+=ddelta
				
				wd,nmth = calendar.monthrange(ts.starttime.year,ts.starttime.month)
				ddelta = datetime.timedelta(days=nmth)
				ts.endtime+=ddelta
			else:
				ts.starttime+=ddelta*(self.interval-1)
				ts.endtime = ts.starttime + ddelta
						
		return	
