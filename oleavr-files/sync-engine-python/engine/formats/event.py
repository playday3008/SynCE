# -*- coding: utf-8 -*-
#
# Copyright (C) 2006  Ole André Vadla Ravnås <oleavr@gmail.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#

from engine.util import *
from xml import xpath
from base64 import standard_b64decode, standard_b64encode
import struct
import datetime
import calendar
import parser

"""
    Unhandled fields:
        Body (subnodes: BodyTruncated), BusyStatus, Rtf
        Exceptions (subnodes: Deleted, Exception, ExceptionStartTime)
        MeetingStatus, OrganizerEmail, OrganizerName,
        Recurrence (subnodes: DayOfMonth, DayOfWeek, MonthOfYear, Type, WeekOfMonth)
        Until, Occurrences, Interval, Sensitivity
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

    name = ""
    for c in std_name:
        if c.isalnum():
            name += c
        else:
            name += "_"

    node_append_child(tz_node, "TimezoneID", "/synce.sourceforge.net/SynCE/%s" % name)

    std_offset = format_tz_offset_string(bias, std_bias)
    print "std_offset: def=%d, extra=%d => %s" % (bias, std_bias, std_offset)

    dst_offset = format_tz_offset_string(bias, daylight_bias)
    print "dst_offset: def=%d, extra=%d => %s" % (bias, daylight_bias, dst_offset)

    print

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

date_from_airsync = lambda d, sd, sn, v: v.rstrip("Z")
date_to_airsync = lambda d, sd, sn, v: v + "Z"

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

FROM_AIRSYNC_SPEC = \
(
 ("Timezone", timezone_from_airsync),
 ("DtStamp", ("Event/LastModified", date_from_airsync)), # possibly "DateCreated"
 ("StartTime", ("Event/DateStarted", startend_from_airsync)),
 ("EndTime", ("Event/DateEnd", startend_from_airsync)),
 ("AllDayEvent", lambda *args: ()),
 ("Subject", "Event/Summary"),
 ("Location", "Event/Location"),
 ("Categories", ("Event", None)),
 ("UID", "Event/Uid"), # not entirely sure about this one
 ("Attendees", ("Event", attendees_from_airsync)),
 ("Reminder", ("Event", alarm_from_airsync)),
)

TO_AIRSYNC_SPEC = \
(
 ("Timezone", (timezone_to_airsync,)),
 ("Event/LastModified", ({ "Content" : date_to_airsync },)),
)

def from_airsync(guid, app_node):
    doc = parser.from_airsync(guid, app_node, "vcal", FROM_AIRSYNC_SPEC, None)

    node = doc.createElement("Method")
    node_append_child(node, "Content", "PUBLISH")
    doc.documentElement.insertBefore(node, doc.documentElement.firstChild)
    return doc

def to_airsync(os_doc):
    return parser.to_airsync(os_doc, "vcal", TO_AIRSYNC_SPEC)
