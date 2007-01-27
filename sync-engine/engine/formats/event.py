# -*- coding: utf-8 -*-
############################################################################
#    Copyright (C) 2006  Ole André Vadla Ravnås <oleavr@gmail.com>       #
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


from engine.xmlutil import *
from engine.util import *
from xml import xpath
from base64 import standard_b64decode, standard_b64encode
import struct
import datetime
import calendar
import parser

"""
    Unhandled fields:
        Body (subnodes: BodyTruncated), Rtf
"""

def attendees_from_airsync(doc, src_doc, src_node, value):
    nodes = []
    for att_node in xpath.Evaluate("Attendee", src_node):
        node = doc.createElement("Attendee")

        name = node_get_value_of_child(att_node, "Name")
        email = node_get_value_of_child(att_node, "Email")

        if email != None:
            content = email
        else:
            content = name

        node_append_child(node, "Content", "MAILTO:%s" % content)
        node_append_child(node, "Role", "REQ-PARTICIPANT")
        node_append_child(node, "PartStat", "ACCEPTED")
        node_append_child(node, "RSVP", "TRUE")

        if name != None:
            node_append_child(node, "CommonName", name)

        nodes.append(node)

    return nodes

def attendee_to_airsync(doc, src_doc, src_node, value):
    app_node = xpath.Evaluate("/ApplicationData", doc)[0]
    # We need to find an existing 'Attendees' node in the target
    # and add the converted data to that instead of simply returning
    # the converted data

    nodes = xpath.Evaluate("Attendees", app_node)
    if nodes:
        parent = nodes[0]
    else:
        parent = node_append_child(app_node, "Attendees")

    name = None
    node = node_find_child(src_node, "CommonName")
    if node != None:
        name = node_get_value(node)
    email = node_get_value(node_get_child(src_node, "Content"))[7:]

    node = node_append_child(parent, "Attendee")
    if name != None:
        node_append_child(node, "Name", name)
    node_append_child(node, "Email", email)

    return ()

def categories_to_airsync(doc, src_doc, src_node, value):
    app_node = xpath.Evaluate("/ApplicationData", doc)[0]
    # We need to find an existing 'Categories' node in the target
    # and add the converted data to that instead of simply returning
    # the converted data

    nodes = xpath.Evaluate("Categories", app_node)
    if nodes:
        parent = nodes[0]
    else:
        parent = node_append_child(app_node, "Categories")

    for cat_node in xpath.Evaluate("Category", src_node):
        node_append_child(parent, "Category", node_get_value(cat_node))

    return ()

MINUTES_PER_HOUR = 60
MINUTES_PER_DAY = MINUTES_PER_HOUR * 24

def alarm_from_airsync(doc, src_doc, src_node, value):
    node = doc.createElement("Alarm")

    trigger_node = node_append_child(node, "AlarmTrigger")

    value = int(value)
    if value % MINUTES_PER_DAY == 0:
        s = "-P%iD" % (value / MINUTES_PER_DAY,)
    elif value % MINUTES_PER_HOUR == 0:
        s = "-PT%iH" % (value / MINUTES_PER_HOUR,)
    else:
        s = "-PT%iM" % (value,)

    node_append_child(trigger_node, "Content", s)

    node_append_child(trigger_node, "Value", "DURATION")
    node_append_child(trigger_node, "Related", "START")

    node_append_child(node, "AlarmAction", "DISPLAY")

    nodes = xpath.Evaluate("/ApplicationData/Subject", src_doc)
    if nodes:
        node_append_child(node, "AlarmDescription", node_get_value(nodes[0]))

    return ( node, )

def alarm_to_airsync(doc, src_doc, src_node, value):
    value = node_get_value(xpath.Evaluate("AlarmTrigger/Content", src_node)[0])
    value = value.lstrip("-PT")
    count = int(value[:-1])
    specifier = value[-1:].upper()

    if specifier == "M":
        minutes = count
    elif specifier == "H":
        minutes = count * MINUTES_PER_HOUR
    elif specifier == "D":
        minutes = count * MINUTES_PER_DAY

    node = doc.createElement("Reminder")
    node.appendChild(doc.createTextNode(unicode(minutes)))

    return ( node, )

def format_tz_offset_string(default_bias, extra_bias):
    bias = default_bias + extra_bias
    return "%+03i%02i" % (-bias / 60, abs(bias) % 60)

def day_in_1970_from_month_and_week(month, week):
    if month < 1 or month > 12 or week < 1 or week > 5:
        raise ValueError("invalid time: month=%d, week=%d" % (month, week))

    oneday = datetime.timedelta(days=1)
    oneweek = datetime.timedelta(days=7)

    first_sunday = datetime.date(1970, month, 1)
    while first_sunday.weekday() != calendar.SUNDAY:
        first_sunday += oneday

    while True:
        day = first_sunday + (oneweek * (week - 1))
        if day.month != month:
            week -= 1
        else:
            break

    return day.day

def format_tz_time_string(month, week, hour):
    day = day_in_1970_from_month_and_week(month, week)
    return "1970%02i%02iT%02i0000" % (month, day, hour)

def node_add_tz_rrule(node, month, week):
    rr_node = node_append_child(node, "RecurrenceRule")
    node_append_child(rr_node, "Rule", "FREQ=YEARLY")
    node_append_child(rr_node, "Rule", "INTERVAL=1")

    if week == 5:
        s = -1
    else:
        s = week

    node_append_child(rr_node, "Rule", "BYDAY=%iSU" % s)
    node_append_child(rr_node, "Rule", "BYMONTH=%i" % month)

def timezone_from_airsync(doc, src_doc, src_node, value):
    tz_node = doc.createElement("Timezone")

    bytes = standard_b64decode(value)

    bias = struct.unpack("<i", bytes[0:4])[0]
    std_name = decode_wstr(bytes[4:68])
    std_month_of_year = struct.unpack("<H", bytes[70:72])[0]
    std_instance = struct.unpack("<H", bytes[74:76])[0]
    std_start_hour = struct.unpack("<H", bytes[76:78])[0]
    std_bias = struct.unpack("<i", bytes[84:88])[0]
    dst_month_of_year = struct.unpack("<H", bytes[154:156])[0]
    dst_instance = struct.unpack("<H", bytes[158:160])[0]
    dst_start_hour = struct.unpack("<H", bytes[160:162])[0]
    daylight_bias = struct.unpack("<i", bytes[168:172])[0]

    # It is possible that the event we get from Airsync doesn't
    # actually contain anything valid.  In this case, we just don't
    # insert any Timezone information into the converted data.
    if std_month_of_year == 0 and   \
        std_instance == 0 and       \
        std_start_hour == 0 and     \
        std_bias == 0 and           \
        dst_month_of_year == 0 and  \
        dst_instance == 0 and       \
        dst_start_hour == 0 and     \
        daylight_bias == 0:
        return ()

    #print "bias: %s" % bias
    #print "std_name: %s" % std_name
    #print "std_month_of_year: %s" % std_month_of_year
    #print "std_instance: %s" % std_instance
    #print "std_start_hour: %s" % std_start_hour
    #print "std_bias: %s" % std_bias
    #print "dst_month_of_year: %s" % dst_month_of_year
    #print "dst_instance: %s" % dst_instance
    #print "dst_start_hour: %s" % dst_start_hour
    #print "daylight_bias: %s" % daylight_bias

    name = ""
    for c in std_name:
        if c.isalnum():
            name += c
        else:
            name += "_"

    node_append_child(tz_node, "TimezoneID", "/synce.sourceforge.net/SynCE/%s" % name)

    std_offset = format_tz_offset_string(bias, std_bias)
    dst_offset = format_tz_offset_string(bias, daylight_bias)

    #parse_tz_offset_strings(std_offset, dst_offset)

    #print

    dst_node = node_append_child(tz_node, "DaylightSavings")
    node_append_child(dst_node, "TZOffsetFrom", std_offset)
    node_append_child(dst_node, "TZOffsetTo", dst_offset)
    node_append_child(dst_node, "DateStarted",
        format_tz_time_string(dst_month_of_year, dst_instance, dst_start_hour))
    node_add_tz_rrule(dst_node, dst_month_of_year, dst_instance)

    std_node = node_append_child(tz_node, "Standard")
    node_append_child(std_node, "TZOffsetFrom", dst_offset)
    node_append_child(std_node, "TZOffsetTo", std_offset)
    node_append_child(std_node, "DateStarted",
        format_tz_time_string(std_month_of_year, std_instance, std_start_hour))
    node_add_tz_rrule(std_node, std_month_of_year, std_instance)

    return ( tz_node, )

def timezone_to_airsync(doc, src_doc, src_node, value):
    tz_node = doc.createElement("Timezone")

    s = ""
    for i in xrange(172):
        s += "\x00"

    value = standard_b64encode(s)
    tz_node.appendChild(doc.createTextNode(value))

    return ( tz_node, )

def date_to_complete_date(value):
    if value.find("T") < 0:
        value += "T000000"
    if value.find("Z") < 0:
        value += "Z"
    return value

def complete_date_to_short_date(value):
    timepos = value.find("T")
    if timepos >= 0:
        value = value[:timepos]
    return value

def is_date_opensync_allday(value):
    return value.find("T") < 0

def startend_from_airsync(doc, src_doc, src_node, value):
    nodes = []

    nodes.append(doc.createElement("Content"))

    value = value.rstrip("Z")

    ad_nodes = xpath.Evaluate("/ApplicationData/AllDayEvent", src_doc)
    if ad_nodes:
        if int(node_get_value(ad_nodes[0])) != 0:
            value = value[0:8]

    nodes[0].appendChild(doc.createTextNode(value))
    if len(value) == 8:
        nodes.append(doc.createElement("Value"))
        nodes[1].appendChild(doc.createTextNode("DATE"))

    return nodes

def lastmodified_to_airsync(doc, src_doc, src_node, value):
    value = node_get_value(node_get_child(src_node, "Content"))
    node = doc.createElement("DtStamp")
    node.appendChild(doc.createTextNode(date_to_complete_date(value)))
    return (node,)

def dateend_to_airsync(doc, src_doc, src_node, value):
    value = node_get_value(node_get_child(src_node, "Content"))
    node = doc.createElement("EndTime")
    node.appendChild(doc.createTextNode(date_to_complete_date(value)))
    return (node,)

def datestarted_to_airsync(doc, src_doc, src_node, value):
    value = node_get_value(node_get_child(src_node, "Content"))

    allday_node = doc.createElement("AllDayEvent")
    allday_node.appendChild(doc.createTextNode(str(int(is_date_opensync_allday(value)))))

    start_node = doc.createElement("StartTime")
    start_node.appendChild(doc.createTextNode(date_to_complete_date(value)))

    return (start_node,)

vcal_days_to_airsync_days_map = {
    "SU" : 1,
    "MO" : 2,
    "TU" : 4,
    "WE" : 8,
    "TH" : 16,
    "FR" : 32,
    "SA" : 64,
    }

airsync_days_to_vcal_days_map = {
    1   : "SU",
    2   : "MO",
    4   : "TU",
    8   : "WE",
    16  : "TH",
    32  : "FR",
    64  : "SA",
    }

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
    sep = ","
    return sep.join(vcal_days)

def vcal_split_byday(vcal_byday):
    return (vcal_byday[:-2], vcal_byday[-2:])

def generate_vcal_byday(airsync_week, airsync_day):
    week = int(airsync_week)
    if week == 5:
        # Special case: Airsync uses '5' to denote the last week of a month
        week = -1
    return "%d%s" % (week, airsync_days_to_vcal_days_map[int(airsync_day)])

def recurrence_from_airsync(doc, src_doc, src_node, value):
    node = doc.createElement("RecurrenceRule")

    interval_node = node_find_child(src_node, "Interval")
    until_node = node_find_child(src_node, "Until")
    occurrences_node = node_find_child(src_node, "Occurrences")
    type_node = node_find_child(src_node, "Type")
    dayofweek_node = node_find_child(src_node, "DayOfWeek")
    dayofmonth_node = node_find_child(src_node, "DayOfMonth")
    weekofmonth_node = node_find_child(src_node, "WeekOfMonth")
    monthofyear_node = node_find_child(src_node, "MonthOfYear")

    # Add the common nodes that don't really require conversion
    if interval_node != None:
        node_append_child(node, "Rule", "INTERVAL=%s" % node_get_value(interval_node))
    if until_node != None:
        node_append_child(node, "Rule", "UNTIL=%s" % node_get_value(until_node))
    if occurrences_node != None:
        node_append_child(node, "Rule", "COUNT=%s" % node_get_value(occurrences_node))

    if type_node != None:
        type = int(node_get_value(type_node))

        # Special case: we can treat this as simple weekly event
        if type == 0 and dayofweek_node != None:
            type = 1

        if type == 0:
            node_append_child(node, "Rule", "FREQ=DAILY")
        elif type == 1:
            node_append_child(node, "Rule", "FREQ=WEEKLY")
            node_append_child(node, "Rule", "BYDAY=%s" % airsync_days_to_vcal_days(node_get_value(dayofweek_node)))
        elif type == 2:
            node_append_child(node, "Rule", "FREQ=MONTHLY")
            node_append_child(node, "Rule", "BYMONTHDAY=%s" % node_get_value(dayofmonth_node))
        elif type == 3:
            node_append_child(node, "Rule", "FREQ=MONTHLY")
            node_append_child(node, "Rule", "BYDAY=%s" % generate_vcal_byday(node_get_value(weekofmonth_node), node_get_value(dayofweek_node)))
        elif type == 5:
            node_append_child(node, "Rule", "FREQ=YEARLY")
            node_append_child(node, "Rule", "BYMONTH=%s" % node_get_value(monthofyear_node))
            node_append_child(node, "Rule", "BYMONTHDAY=%s" % node_get_value(dayofmonth_node))
        elif type == 6:
            node_append_child(node, "Rule", "FREQ=YEARLY")
            node_append_child(node, "Rule", "BYMONTH=%s" % node_get_value(monthofyear_node))
            node_append_child(node, "Rule", "BYDAY=%s" % generate_vcal_byday(node_get_value(weekofmonth_node), node_get_value(dayofweek_node)))
        else:
            # Unsupported type
            # TODO: throw an exception??
            pass

    else:
        # If we don't know what type of recurrence it is, we
        # can't construct its vcal rules
        # TODO: throw an exception??
        pass

    return (node, )

def recurrence_to_airsync(doc, src_doc, src_node, value):
    node = doc.createElement("Recurrence")

    # Extract the rules
    src_rules = {}
    for rrule in xpath.Evaluate("Rule", src_node):
        rrule_val = node_get_value(rrule)
        seppos = rrule_val.index("=")
        key = rrule_val[:seppos]
        val = rrule_val[seppos+1:]

        src_rules[key.lower()] = val

    # Interval, Count, and Until rules have straightforward conversions
    if src_rules.has_key("interval"):
        node_append_child(node, "Interval", src_rules["interval"])
    if src_rules.has_key("until"):
        node_append_child(node, "Until", date_to_complete_date(src_rules["until"]))
    if src_rules.has_key("count"):
        node_append_child(node, "Occurrences", src_rules["count"])

    # Handle different types of recurrences on a case-by-case basis
    if src_rules["freq"].lower() == "daily":
        # There really isn't much to convert in this case..
        node_append_child(node, "Type", "0")
    elif src_rules["freq"].lower() == "weekly":
        node_append_child(node, "Type", "1")
        node_append_child(node, "DayOfWeek", vcal_days_to_airsync_days(src_rules["byday"]))
    elif src_rules["freq"].lower() == "monthly":
        if src_rules.has_key("bymonthday"):
            node_append_child(node, "Type", "2")
            node_append_child(node, "DayOfMonth", src_rules["bymonthday"])
        elif src_rules.has_key("byday"):
            week, day = vcal_split_byday(src_rules["byday"])
            node_append_child(node, "Type", "3")
            node_append_child(node, "DayOfWeek", vcal_days_to_airsync_days(day))
            if week >= 0:
                node_append_child(node, "WeekOfMonth", week)
            elif week == -1:
                # Airsync deals with this as a special case
                node_append_child(node, "WeekOfMonth", "5")
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
                node_append_child(node, "Type", "5")
                node_append_child(node, "MonthOfYear", src_rules["bymonth"])
                node_append_child(node, "DayOfMonth", src_rules["bymonthday"])
            elif src_rules.has_key("byday"):
                week, day = vcal_split_byday(src_rules["byday"])
                node_append_child(node, "Type", "6")
                node_append_child(node, "MonthOfYear", src_rules["bymonth"])
                node_append_child(node, "DayOfWeek", vcal_days_to_airsync_days(day))
                if week >= 0:
                    node_append_child(node, "WeekOfMonth", week)
                elif week == -1:
                    # Airsync deals with this as a special case
                    node_append_child(node, "WeekOfMonth", "5")
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
            # It seems like this should be against the rules, and filling in
            # a default might not make sense because either of the above interpretations
            # is equally valid.
            raise ValueError("Yearly events must either specify BYMONTH or BYYEARDAY rules")

    return ( node, )

def exceptions_from_airsync(doc, src_doc, src_node, value):
    nodes = []
    for ex_node in xpath.Evaluate("Exception", src_node):
        node = doc.createElement("ExclusionDate")

        ex_deleted = node_get_value(node_find_child(ex_node, "Deleted"))
        if not int(ex_deleted):
            raise ValueError("Opensync does not support exceptions for modified occurrences")

        ex_date = node_get_value(node_find_child(ex_node, "ExceptionStartTime"))

        node_append_child(node, "Content", complete_date_to_short_date(ex_date))
        node_append_child(node, "Value", "DATE")

        nodes.append(node)

    return nodes


def exclusiondate_to_airsync(doc, src_doc, src_node, value):
    app_node = xpath.Evaluate("/ApplicationData", doc)[0]

    # We need to find an existing 'Exclusions' node in the target
    # and add the converted data to that instead of simply returning
    # the converted data

    nodes = xpath.Evaluate("Exceptions", app_node)
    if nodes:
        parent = nodes[0]
    else:
        parent = node_append_child(app_node, "Exceptions")

    exclusion_date = node_get_value(node_find_child(src_node, "Content"))
    exclusion_value = node_get_value(node_find_child(src_node, "Value"))

    if exclusion_value.lower() != "date":
        raise ValueError("Exclusions with values other than 'DATE' are not supported")

    exception_node = node_append_child(parent, "Exception")
    node_append_child(exception_node, "Deleted", "1")
    node_append_child(exception_node, "ExceptionStartTime", date_to_complete_date(exclusion_date))

    return ()

def busystatus_from_airsync(doc, src_doc, src_node, value):
    node = doc.createElement("Transparency")

    busystatus = int(node_get_value(src_node))
    if busystatus == 0 or busystatus == 1: # Free or Tentative
        busystatus = "TRANSPARENT"
    elif busystatus == 2 or busystatus == 3: # Busy or Out Of Office
        busystatus = "OPAQUE"
    else:
        raise ValueError("Opensync does not support this BusyStatus value")

    node_append_child(node, "Content", busystatus)

    return (node,)

def transparency_to_airsync(doc, src_doc, src_node, value):
    value = node_get_value(node_get_child(src_node, "Content"))
    node = doc.createElement("BusyStatus")
    if value == "TRANSPARENT":
        value = "0"
    elif value == "OPAQUE":
        value = "2"
    else:
        raise ValueError("Unknown Transparency value from OpenSync")
    node.appendChild(doc.createTextNode(value))
    return (node,)

def sensitivity_from_airsync(doc, src_doc, src_node, value):
    node = doc.createElement("Class")

    sensitivity = int(node_get_value(src_node))
    if sensitivity == 0:
        sensitivity = "PUBLIC"
    elif sensitivity == 1 or sensitivity == 2:
        sensitivity = "PRIVATE"
    elif sensitivity == 3:
        sensitivity = "CONFIDENTIAL"
    else:
        raise ValueError("Invalid Sensitivity value from Airsync")

    node_append_child(node, "Content", sensitivity)

    return (node,)

def class_to_airsync(doc, src_doc, src_node, value):
    value = node_get_value(node_get_child(src_node, "Content"))
    node = doc.createElement("Sensitivity")
    if value == "PUBLIC":
        value = "0"
    elif value == "PRIVATE":
        value = "2"
    elif value == "CONFIDENTIAL":
        value = "3"
    else:
        raise ValueError("Unknown Class value from OpenSync")
    node.appendChild(doc.createTextNode(value))

    app_node = xpath.Evaluate("/ApplicationData", doc)[0]

    meetingstatus_node = doc.createElement("MeetingStatus")
    meetingstatus_node.appendChild(doc.createTextNode("0"))

    organizername_node = doc.createElement("OrganizerName")
    organizername_node.appendChild(doc.createTextNode("Unknown"))

    organizeremail_node = doc.createElement("OrganizerEmail")
    organizeremail_node.appendChild(doc.createTextNode("Unknown"))

    return (meetingstatus_node, organizername_node, organizeremail_node, node,)

FROM_AIRSYNC_SPEC = (
 ("Timezone", timezone_from_airsync),
 ("DtStamp", "Event/LastModified"), # possibly "DateCreated"
 ("StartTime", ("Event/DateStarted", startend_from_airsync)),
 ("EndTime", ("Event/DateEnd", startend_from_airsync)),
 ("AllDayEvent", lambda *args: ()),
 ("Subject", "Event/Summary"),
 ("Location", "Event/Location"),
 ("Categories", ("Event", None)),
 ("Attendees", ("Event", attendees_from_airsync)),
 ("Reminder", ("Event", alarm_from_airsync)),
 ("Recurrence", ("Event", recurrence_from_airsync)),
 ("Exceptions", ("Event", exceptions_from_airsync)),
 ("BusyStatus", ("Event", busystatus_from_airsync)),
 ("Sensitivity", ("Event", sensitivity_from_airsync)),
)

#("UID", "Event/Uid"), # not entirely sure about this one


# Note that order matters here!!
TO_AIRSYNC_SPEC = (
 ("Method", (lambda *args: (),)),
 ("UID", (lambda *args: (),)),
 ("Event/Alarm", (alarm_to_airsync,)),
 ("Event/Transparency", (transparency_to_airsync,)),
 ("Event/LastModified", (lastmodified_to_airsync,)),
 ("Event/DateStarted", (datestarted_to_airsync,)),
 ("Event/DateEnd", (dateend_to_airsync,)),
 ("Event/Location", ("Location",)),
 ("Event/Class", (class_to_airsync,)),
 ("Event/Summary", ("Subject",)),
 ("Timezone", (timezone_to_airsync,)),
 ("Event/Categories", categories_to_airsync,),
 ("Event/Attendee", attendee_to_airsync,),
 ("Event/RecurrenceRule", (recurrence_to_airsync,)),
 ("Event/ExclusionDate", exclusiondate_to_airsync),
)

#("Event/Uid", ("UID",)),

def from_airsync(guid, app_node):
    doc = parser.from_airsync(guid, app_node, "vcal", FROM_AIRSYNC_SPEC, None)

    node = doc.createElement("Method")
    node_append_child(node, "Content", "PUBLISH")
    doc.documentElement.insertBefore(node, doc.documentElement.firstChild)
    return doc

def to_airsync(os_doc):
    #doc = parser.to_airsync(os_doc, "vcal", TO_AIRSYNC_SPEC, "POOMCAL:")

    #app_node = xpath.Evaluate("ApplicationData", doc)[0]

    #busystatus_node = xpath.Evaluate("BusyStatus", app_node)[0]
    #alldayevent_node = doc.createElement("AllDayEvent")
    #alldayevent_node.appendChild(doc.createTextNode("0"))
    #app_node.insertBefore(alldayevent_node, busystatus_node)

    #uid_node = doc.createElement("UID")
    #uid_node.appendChild(doc.createTextNode("040000008200E00074C5B7101A82E0080000000010541DEE613DC7010000000000000000100000006DC332D6DA964A44820F2CE03F544FE5"))
    #app_node.appendChild(uid_node)

    #return doc
    return parser.to_airsync(os_doc, "vcal", TO_AIRSYNC_SPEC, "POOMCAL:")
