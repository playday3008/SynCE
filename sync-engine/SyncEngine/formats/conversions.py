# -*- coding: utf-8 -*-
# vim: set tabstop=4 shiftwidth=4 expandtab:
############################################################################
#    Copyright (C) 2006  Ole André Vadla Ravnås <oleavr@gmail.com>         #
#    MODIFIED: 17/2/2007 Dr J A Gow: Task support added                    #
#                                                                          #
#    This program is free software; you can redistribute it and#or modify  #
#    it under the terms of the GNU General Public License as published by  #
#    the Free Software Foundation; either version 2 of the License, or     #
#    (at your option) any later version.                                   #
#                                                                          #
#    This program is distributed in the hope that it will be useful,       #
#    but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#    GNU General Public License for more details.                          #
#                                                                          #
#    You should have received a copy of the GNU General Public License     #
#    along with this program; if not, write to the                         #
#    Free Software Foundation, Inc.,                                       #
#    59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             #
############################################################################

# Useful documentation:
# http://www.xmldatabases.org/movabletype/archives/000291.html

import libxml2
import libxslt
import string
import pyrtfcomp
import base64
import tzconv
from time import gmtime, strftime
import tzutils
import SyncEngine.xml2util as xml2util

### conversion constants ###

RTFHDR = "\\ansi \\deff0{\\fonttbl{\\f0\\fnil\\fcharset0\\fprq0 Tahoma;}{\\f1\\froman\\fcharset2\\fprq2 "
RTFHDR += "Symbol;}{\\f2\\fswiss\\fcharset204\\fprq2  ;}}{\\colortbl;\\red0\\green0\\blue0;\\red128\\green128"
RTFHDR += "\\blue128;\\red192\\green192\\blue192;\\red255\\green255\\blue255;\\red255\\green0\\blue0;\\red0"
RTFHDR += "\\green255\\blue0;\\red0\\green0\\blue255;\\red0\\green255\\blue255;\\red255\\green0\\blue255;"
RTFHDR += "\\red255\\green255\\blue0;\\red128\\green0\\blue0;\\red0\\green128\\blue0;\\red0\\green0"
RTFHDR += "\\blue128;\\red0\\green128\\blue128;\\red128\\green0\\blue128;\\red128\\green128\\blue0;}\x0a\x0d"
RTFHDR += "\\f0 \\fs16 "

DATE_FORMAT_NORMAL        = '%Y%m%dT%H%M%SZ'
DATE_FORMAT_EVENT         = '%Y%m%dT%H%M%SZ'
DATE_FORMAT_EVLOCAL       = '%Y%m%dT%H%M%S'
DATE_FORMAT_TASK          = '%Y-%m-%dT%H:%M:%S.000Z'
DATE_FORMAT_TASKLOCAL     = '%Y-%m-%dT%H:%M:%S.000'
DATE_FORMAT_VCALTASK      = '%Y%m%dT%H%M%SZ'
DATE_FORMAT_VCALTASKLOCAL = '%Y%m%dT%H%M%S'
DATE_FORMAT_SHORT         = '%Y%m%d'

MINUTES_PER_HOUR    = 60
MINUTES_PER_DAY     = MINUTES_PER_HOUR * 24

vcal_days_to_airsync_days_map = { "SU" : 1,
                                  "MO" : 2,
                                  "TU" : 4,
                                  "WE" : 8,
                                  "TH" : 16,
                                  "FR" : 32,
                                  "SA" : 64 }

airsync_days_to_vcal_days_map = {  1   : "SU",
                                   2   : "MO",
                                   4   : "TU",
                                   8   : "WE",
                                   16  : "TH",
                                   32  : "FR",
                                   64  : "SA" }


### Conversion Utility functions ###

def vcal_days_to_airsync_days(vcal_days):
    airsync_days = 0
    for vcal_day in vcal_days.split(","):
        airsync_days = airsync_days | vcal_days_to_airsync_days_map[vcal_day.upper()]
    return airsync_days

def airsync_days_to_vcal_days(airsync_days):
    vcal_days = []
    for i in xrange(0,8):
        mask = (1 << i)
        if mask & int(airsync_days):
            vcal_days.append(airsync_days_to_vcal_days_map[mask])
    return ",".join(vcal_days)

def vcal_split_byday(vcal_byday):
    return (vcal_byday[:-2], vcal_byday[-2:])

def generate_vcal_byday(airsync_week, airsync_day):
    week = int(airsync_week)
    if week == 5:
        # Special case: Airsync uses '5' to denote the last week of a month
        week = -1
    return "%d%s" % (week, airsync_days_to_vcal_days_map[int(airsync_day)])

### conversion functions ###

def contact_has_type(ctx, type_string):
    parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
    curnode = transform_ctx.current()
    s=xml2util.GetNodeValue(curnode)
    for child in curnode.children:
        if child.name != "Type":
            continue
        if xml2util.GetNodeValue(child).upper() == type_string:
            return True
    return False
 
def contact_position(ctx):
    parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
    curnode = transform_ctx.current()
    s=xml2util.GetNodeValue(curnode)
    # Extract the types
    types = []
    for child in curnode.children:
        if child.name == "Type":
            types.append(xml2util.GetNodeValue(child).upper())
    types.sort()
    position = 1
    prevnode = curnode.prev
    while prevnode != None:
        if prevnode.name == curnode.name:
            #print "found another", curnode.name
            # Now compare types
            prev_types = []
            for child in prevnode.children:
                if child.name == "Type":
                    prev_types.append(xml2util.GetNodeValue(child).upper())
            prev_types.sort()
            #print "prev types:",prev_types
            if prev_types == types:
                position += 1
        prevnode = prevnode.prev
    return position
 
def event_reminder_to_airsync(ctx):
    parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
    src_node = transform_ctx.current()
    content_node = xml2util.FindChildNode(src_node, "Content")
    value_node = xml2util.FindChildNode(src_node, "Value")
    related_node = xml2util.FindChildNode(src_node, "Related")
    if value_node == None or xml2util.GetNodeValue(value_node).lower() != "duration":
        return ""
    if related_node != None and xml2util.GetNodeValue(related_node).lower() != "start":
        return ""
    s = xml2util.GetNodeValue(content_node)
    s = s.lstrip("-PT")
    s = s.upper()
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

def event_reminder_from_airsync(ctx):
    parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
    s = int(xml2util.GetNodeValue(transform_ctx.current()))
    if s % MINUTES_PER_DAY == 0:
        return "-P%iD" % (s / MINUTES_PER_DAY)
    elif s % MINUTES_PER_HOUR == 0:
        return "-PT%iH" % (s / MINUTES_PER_HOUR)
    else:
        return "-PT%iM" % s

def event_time_to_airsync(ctx):
    parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
    return tzconv.ConvertDateNodeToUTC(transform_ctx.current())[1].strftime(DATE_FORMAT_EVENT)

def event_datetime_from_airsync(ctx, node=None):
    parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
    if node:
        node = libxml2.xmlNode(_obj=node[0])
    else:
        node = transform_ctx.current()
    return tzutils.TextToDate(xml2util.GetNodeValue(node)).strftime(DATE_FORMAT_EVENT)

def event_datetime_short_from_airsync(ctx):
    parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
    return tzutils.TextToDate(xml2util.GetNodeValue(transform_ctx.current())).strftime(DATE_FORMAT_SHORT)

def event_dtstamp_from_now(ctx):
    parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
    return strftime("%Y%m%dT%H%M%SZ", gmtime())

def event_alldayevent_to_airsync(ctx):
    parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
    if xml2util.GetNodeValue(transform_ctx.current()).find("T") >= 0:
        return "0"
    else:
        return "1"

def event_starttime_from_airsync(ctx):
    parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
    src_node = transform_ctx.current()
    dst_node = transform_ctx.insertNode()

    allday_node = xml2util.FindChildNode(src_node.parent, "AllDayEvent")
    asdate = tzutils.TextToDate(xml2util.GetNodeValue(transform_ctx.current()))

    if tzconv.curtz() != None:

        # if we have a tz, we must insert the ID and convert to it

        dst_node.newChild(None,"TimezoneID",tzconv.curtz().name)
        asdate = tzconv.ConvertToLocal(asdate,tzconv.curtz())
        result = asdate.strftime(DATE_FORMAT_EVLOCAL)
    else:
        result = asdate.strftime(DATE_FORMAT_EVENT)

    if allday_node != None and xml2util.GetNodeValue(allday_node) != "0":
        result=result[0:8]

    dst_node.newChild(None,"Content",result)
    return ""

def event_endtime_from_airsync(ctx):
    parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
    src_node = transform_ctx.current()
    dst_node = transform_ctx.insertNode()

    allday_node = xml2util.FindChildNode(src_node.parent, "AllDayEvent")
    asdate = tzutils.TextToDate(xml2util.GetNodeValue(transform_ctx.current()))

    if tzconv.curtz() != None:

        # if we have a tz, we must insert the ID and convert to it

        dst_node.newChild(None,"TimezoneID",tzconv.curtz().name)
        asdate = tzconv.ConvertToLocal(asdate,tzconv.curtz())

        result = asdate.strftime(DATE_FORMAT_EVLOCAL)
    else:
        result = asdate.strftime(DATE_FORMAT_EVENT)

    if allday_node != None and xml2util.GetNodeValue(allday_node) != "0":
        result=result[0:8]

    dst_node.newChild(None,"Content",result)
    return ""

def event_recurrence_to_airsync(ctx):

    parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
    src_node = transform_ctx.current()
    dst_node = transform_ctx.insertNode()

    # Extract the rules

    src_rules = {}
    child = src_node.children

    while child != None:
        if child.name == "Rule":
            rrule_val = xml2util.GetNodeValue(child)
            sep = rrule_val.index("=")
            key = rrule_val[:sep]
            val = rrule_val[sep+1:]
            src_rules[key.lower()] = val.lower()
        child = child.next


    # Interval, Count, and Until rules have straightforward conversions

    if src_rules.has_key("interval"):
        dst_node.newChild(None, "Interval", src_rules["interval"])
    if src_rules.has_key("until"):
        dst_node.newChild(None, "Until", tzutils.TextToDate(src_rules["until"]).strftime(DATE_FORMAT_EVENT))
    if src_rules.has_key("count"):
        dst_node.newChild(None, "Occurrences", src_rules["count"])

    # Handle different types of recurrences on a case-by-case basis

    if src_rules["freq"].lower() == "daily":

        # There really isn't much to convert in this case..
        dst_node.newChild(None, "Type", "0")

    elif src_rules["freq"].lower() == "weekly":
        dst_node.newChild(None, "Type", "1")
        dst_node.newChild(None, "DayOfWeek", str(vcal_days_to_airsync_days(src_rules["byday"])))

    elif src_rules["freq"].lower() == "monthly":
        if src_rules.has_key("bymonthday"):
            dst_node.newChild(None, "Type", "2")
            dst_node.newChild(None, "DayOfMonth", src_rules["bymonthday"])

        elif src_rules.has_key("byday"):

            week, day = vcal_split_byday(src_rules["byday"])
            dst_node.newChild(None, "Type", "3")
            dst_node.newChild(None, "DayOfWeek", str(vcal_days_to_airsync_days(day)))
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

    elif src_rules["freq"].lower() == "yearly":

        if src_rules.has_key("bymonth"):

            if src_rules.has_key("bymonthday"):

                dst_node.newChild(None, "Type", "5")
                dst_node.newChild(None, "MonthOfYear", src_rules["bymonth"])
                dst_node.newChild(None, "DayOfMonth", src_rules["bymonthday"])

            elif src_rules.has_key("byday"):

                week, day = vcal_split_byday(src_rules["byday"])
                dst_node.newChild(None, "Type", "6")
                dst_node.newChild(None, "MonthOfYear", src_rules["bymonth"])
                dst_node.newChild(None, "DayOfWeek", str(vcal_days_to_airsync_days(day)))

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
            utcdate=tzconv.ConvertDateNodeToUTC(start)[1]

            # We need the month and the day

            dst_node.newChild(None,"Type", "5")
            dst_node.newChild(None,"MonthOfYear",str(utcdate.month))
            dst_node.newChild(None,"DayOfMonth",str(utcdate.day))


    return ""

def event_recurrence_from_airsync(ctx):
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
    if interval_node != None:
        dst_node.newChild(None, "Rule", "INTERVAL=%s" % xml2util.GetNodeValue(interval_node))
    if until_node != None:
        dst_node.newChild(None, "Rule", "UNTIL=%s" % xml2util.GetNodeValue(until_node))
    if occurrences_node != None:
        dst_node.newChild(None, "Rule", "COUNT=%s" % xml2util.GetNodeValue(occurrences_node))

    if type_node != None:
        type = int(xml2util.GetNodeValue(type_node))

        # Special case: we can treat this as simple weekly event
        if type == 0 and dayofweek_node != None:
            type = 1

        if type == 0:
            dst_node.newChild(None, "Rule", "FREQ=DAILY")
        elif type == 1:
            dst_node.newChild(None, "Rule", "FREQ=WEEKLY")
            dst_node.newChild(None, "Rule", "BYDAY=%s" % airsync_days_to_vcal_days(xml2util.GetNodeValue(dayofweek_node)))
        elif type == 2:
            dst_node.newChild(None, "Rule", "FREQ=MONTHLY")
            dst_node.newChild(None, "Rule", "BYMONTHDAY=%s" % xml2util.GetNodeValue(dayofmonth_node))
        elif type == 3:
            dst_node.newChild(None, "Rule", "FREQ=MONTHLY")
            dst_node.newChild(None, "Rule", "BYDAY=%s" % generate_vcal_byday(xml2util.GetNodeValue(weekofmonth_node), xml2util.GetNodeValue(dayofweek_node)))
        elif type == 5:
            dst_node.newChild(None, "Rule", "FREQ=YEARLY")
            dst_node.newChild(None, "Rule", "BYMONTH=%s" % xml2util.GetNodeValue(monthofyear_node))
            dst_node.newChild(None, "Rule", "BYMONTHDAY=%s" % xml2util.GetNodeValue(dayofmonth_node))
        elif type == 6:
            dst_node.newChild(None, "Rule", "FREQ=YEARLY")
            dst_node.newChild(None, "Rule", "BYMONTH=%s" % xml2util.GetNodeValue(monthofyear_node))
            dst_node.newChild(None, "Rule", "BYDAY=%s" % generate_vcal_byday(xml2util.GetNodeValue(weekofmonth_node), xml2util.GetNodeValue(dayofweek_node)))
        else:
            # Unsupported type
            raise ValueError("Unknown recurrence type %d from Airsync" % type)

    else:
        # If we don't know what type of recurrence it is, we
        # can't construct its vcal rules
        raise ValueError("No recurrence type specified from Airsync")
    return ""

def task_start_date_to_airsync(ctx):
    parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
    # StartDate without DueDate is not allowed
    duedate_node = xml2util.FindChildNode(transform_ctx.current().parent, "DueDate")
    if not duedate_node:
        return ""
    localDate,utcDate = tzconv.ConvertDateNodeToUTC(transform_ctx.current())
    dst_node = transform_ctx.insertNode()
    tasks_ns = dst_node.searchNsByHref(transform_ctx.outputDoc(), "http://synce.org/formats/airsync_wm5/tasks")
    n=dst_node.newChild(tasks_ns,"StartDate",localDate.strftime(DATE_FORMAT_TASKLOCAL))
    n=dst_node.newChild(tasks_ns,"UtcStartDate",utcDate.strftime(DATE_FORMAT_TASK))
    return ""

def task_due_date_to_airsync(ctx):
    parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
    localDate,utcDate = tzconv.ConvertDateNodeToUTC(transform_ctx.current())
    dst_node = transform_ctx.insertNode()
    tasks_ns = dst_node.searchNsByHref(transform_ctx.outputDoc(), "http://synce.org/formats/airsync_wm5/tasks")
    n=dst_node.newChild(tasks_ns,"DueDate",localDate.strftime(DATE_FORMAT_TASKLOCAL))
    n=dst_node.newChild(tasks_ns,"UtcDueDate",utcDate.strftime(DATE_FORMAT_TASK))
    return ""

def task_start_date_from_airsync(ctx):
    
    parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
    src_node = transform_ctx.current()
    
    asdate = tzutils.TaskTextToDate(xml2util.GetNodeValue(transform_ctx.current()))

    if tzconv.curtz() != None:

        # if we have a tz, we must insert the ID and convert to it

        dst_node.newChild(None,"TimezoneID",tzconv.curtz().name)
        asdate = tzconv.ConvertToLocal(asdate,tzconv.curtz())

        result = asdate.strftime(DATE_FORMAT_VCALTASKLOCAL)
    else:
        # if not, does the source have a UtcStartDate element?
        nd = xml2util.FindChildNode(src_node.parent,"UtcStartDate")
        if nd != None:
            result = tzutils.TaskTextToDate(xml2util.GetNodeValue(nd)).strftime(DATE_FORMAT_VCALTASK)

        else:
            # we don't have this either. Better hope that the StartDate value
            # is correct.

            result = asdate.strftime(DATE_FORMAT_VCALTASKLOCAL)

    dst_node = transform_ctx.insertNode()
    dst_node.newChild(None,"Content",result)
    return ""

def task_due_date_from_airsync(ctx):
    
    parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
    src_node = transform_ctx.current()
    
    asdate = tzutils.TaskTextToDate(xml2util.GetNodeValue(transform_ctx.current()))

    if tzconv.curtz() != None:

        # if we have a tz, we must insert the ID and convert to it

        dst_node.newChild(None,"TimezoneID",tzconv.curtz().name)
        asdate = tzconv.ConvertToLocal(asdate,tzconv.curtz())

        result = asdate.strftime(DATE_FORMAT_VCALTASKLOCAL)
    else:
        # if not, does the source have a UtcStartDate element?
        nd = xml2util.FindChildNode(src_node.parent,"UtcDueDate")
        if nd != None:
            result = tzutils.TaskTextToDate(xml2util.GetNodeValue(nd)).strftime(DATE_FORMAT_VCALTASK)

        else:
            # we don't have this either. Better hope that the DueDate value
            # is correct.

            result = asdate.strftime(DATE_FORMAT_VCALTASKLOCAL)

    dst_node = transform_ctx.insertNode()
    dst_node.newChild(None,"Content",result)
    return ""

def task_classification_to_airsync(ctx):
    parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
    s = xml2util.GetNodeValue(transform_ctx.current())
    if s == "PRIVATE":
        return "2"
    elif s == "CONFIDENTIAL":
        return "3"
    else:
        return "0" # 'PUBLIC' is our default value

def task_classification_from_airsync(ctx):
    parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
    s = xml2util.GetNodeValue(transform_ctx.current())
    if s == "2":
        return "PRIVATE"
    elif s == "3":
        return "CONFIDENTIAL"
    else:
        return "0" # 'PUBLIC' is our default value
    
    # We only sync the 'COMPLETED' state here. Evo2 maintains a number of 
    # different status values for various states of a job except COMPLETED
    # and we don't want to clobber these. AirStink seems only to maintain
    # the two states: Not Completed and Completed. However, we force 
    # the PercentComplete field to 100 if the task is marked as completed
    
def task_status_from_airsync(ctx):
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

def task_status_to_airsync(ctx):
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
 
# Here. let us not destroy the 'unspecified' priority when going
# _to_ airsync. We can't really help reassigning this as 'low' 
# in the other direction.
#

def task_prio_to_airsync(ctx):
    parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
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

def task_prio_from_airsync(ctx):
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

def all_description_from_airsync(ctx):
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

def all_description_to_airsync(ctx):
    parser_ctx, transform_ctx = xml2util.ExtractContexts(ctx)
    src_node = transform_ctx.current()
    content_node = xml2util.FindChildNode(src_node, "Content")
    s = xml2util.GetNodeValue(content_node)
    ec = ""
    if len(s) > 0:
        try:
            asnote = pyrtfcomp.RTFConvertFromUTF8(s,RTFHDR,1)
            ec=base64.b64encode(asnote)
        except pyrtfcomp.RTFException, ConvErr:
            pass
    return ec

def all_upper_case(ctx, string):
    return string.upper()

def register_xslt_extension_functions():
    libxslt.registerExtModuleFunction("contact_has_type",                   "http://synce.org/convert", contact_has_type)
    libxslt.registerExtModuleFunction("contact_position",                   "http://synce.org/convert", contact_position)
    libxslt.registerExtModuleFunction("event_reminder_to_airsync",          "http://synce.org/convert", event_reminder_to_airsync)
    libxslt.registerExtModuleFunction("event_reminder_from_airsync",        "http://synce.org/convert", event_reminder_from_airsync)
    libxslt.registerExtModuleFunction("event_time_to_airsync",           "http://synce.org/convert", event_time_to_airsync)
    libxslt.registerExtModuleFunction("event_datetime_from_airsync",         "http://synce.org/convert", event_datetime_from_airsync)
    libxslt.registerExtModuleFunction("event_datetime_short_from_airsync",   "http://synce.org/convert", event_datetime_short_from_airsync)
    libxslt.registerExtModuleFunction("event_dtstamp_from_now",             "http://synce.org/convert", event_dtstamp_from_now)
    libxslt.registerExtModuleFunction("event_alldayevent_to_airsync",       "http://synce.org/convert", event_alldayevent_to_airsync)
    libxslt.registerExtModuleFunction("event_starttime_from_airsync",       "http://synce.org/convert", event_starttime_from_airsync)
    libxslt.registerExtModuleFunction("event_endtime_from_airsync",         "http://synce.org/convert", event_endtime_from_airsync)
    libxslt.registerExtModuleFunction("event_recurrence_to_airsync",        "http://synce.org/convert", event_recurrence_to_airsync)
    libxslt.registerExtModuleFunction("event_recurrence_from_airsync",      "http://synce.org/convert", event_recurrence_from_airsync)
    libxslt.registerExtModuleFunction("task_start_date_to_airsync",         "http://synce.org/convert", task_start_date_to_airsync)
    libxslt.registerExtModuleFunction("task_due_date_to_airsync",           "http://synce.org/convert", task_due_date_to_airsync)
    libxslt.registerExtModuleFunction("task_start_date_from_airsync",       "http://synce.org/convert", task_start_date_from_airsync)
    libxslt.registerExtModuleFunction("task_due_date_from_airsync",         "http://synce.org/convert", task_due_date_from_airsync)
    libxslt.registerExtModuleFunction("task_classification_from_airsync",   "http://synce.org/convert", task_classification_from_airsync)
    libxslt.registerExtModuleFunction("task_classification_to_airsync",     "http://synce.org/convert", task_classification_to_airsync)
    libxslt.registerExtModuleFunction("task_status_from_airsync",           "http://synce.org/convert", task_status_from_airsync)
    libxslt.registerExtModuleFunction("task_status_to_airsync",             "http://synce.org/convert", task_status_to_airsync)
    libxslt.registerExtModuleFunction("task_prio_to_airsync",               "http://synce.org/convert", task_prio_to_airsync)
    libxslt.registerExtModuleFunction("task_prio_from_airsync",             "http://synce.org/convert", task_prio_from_airsync)
    libxslt.registerExtModuleFunction("all_description_to_airsync",         "http://synce.org/convert", all_description_to_airsync)
    libxslt.registerExtModuleFunction("all_description_from_airsync",       "http://synce.org/convert", all_description_from_airsync)
    libxslt.registerExtModuleFunction("all_upper_case",                     "http://synce.org/convert", all_upper_case)
    
