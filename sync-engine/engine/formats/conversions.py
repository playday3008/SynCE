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

import libxml2
import libxslt
import string

### conversion constants ###

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


### libxml2 utility functions ###

def node_find_child(node, name):
    child = node.children
    while child != None:
        if child.name == name:
            return child
        child = child.next
    return None

def node_value(node):
    if node is None:
        return ""
    else:
        return str(node.content).strip()

### libxslt utility functions ###

def _extract_contexts(ctx):
    parser_ctx = libxslt.xpathParserContext(_obj=ctx).context()
    transform_ctx = parser_ctx.transformContext()
    return parser_ctx, transform_ctx


### Conversion Utility functions ###

def short_date_to_complete_date(value):
    value = value.upper()
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

def contact_anniversary_to_airsync(ctx):
    parser_ctx, transform_ctx = _extract_contexts(ctx)
    return node_value(transform_ctx.current()) + "T00:00:00.000Z"

def contact_birthday_to_airsync(ctx):
    parser_ctx, transform_ctx = _extract_contexts(ctx)
    s = node_value(transform_ctx.current())
    return "%s-%s-%sT00:00:00.000Z" % (s[0:4], s[4:6], s[6:8])

def contact_anniversary_from_airsync(ctx):
    parser_ctx, transform_ctx = _extract_contexts(ctx)
    return node_value(transform_ctx.current()).split("T")[0]

def contact_birthday_from_airsync(ctx):
    parser_ctx, transform_ctx = _extract_contexts(ctx)
    return node_value(transform_ctx.current()).split("T")[0].replace("-", "")

def event_reminder_to_airsync(ctx):
    parser_ctx, transform_ctx = _extract_contexts(ctx)
    src_node = transform_ctx.current()
    content_node = node_find_child(src_node, "Content")
    value_node = node_find_child(src_node, "Value")
    related_node = node_find_child(src_node, "Related")
    if value_node == None or node_value(value_node).lower() != "duration":
        return ""
    if related_node != None and node_value(related_node).lower() != "start":
        return ""
    s = node_value(content_node)
    s = s.lstrip("-PT")
    minutes = int(s[:-1])
    units = s[-1:].upper()
    if units == "H":
        minutes *= MINUTES_PER_HOUR
    elif units == "D":
        minutes *= MINUTES_PER_DAY
    return str(minutes)

def event_reminder_from_airsync(ctx):
    parser_ctx, transform_ctx = _extract_contexts(ctx)
    s = int(node_value(transform_ctx.current()))
    if s % MINUTES_PER_DAY == 0:
        return "-P%iD" % (s / MINUTES_PER_DAY)
    elif s % MINUTES_PER_HOUR == 0:
        return "-PT%iH" % (s / MINUTES_PER_HOUR)
    else:
        return "-PT%iM" % s

def event_busystatus_to_airsync(ctx):
    parser_ctx, transform_ctx = _extract_contexts(ctx)
    if node_value(transform_ctx.current()) == "TRANSPARENT":
        return "0"
    else:
        return "2" # 'Busy' is our default value

def event_busystatus_from_airsync(ctx):
    parser_ctx, transform_ctx = _extract_contexts(ctx)
    if node_value(transform_ctx.current()) == "0":
        return "TRANSPARENT"
    else:
        return "OPAQUE" # 'Busy' is our default value

def event_dtstamp_to_airsync(ctx):
    parser_ctx, transform_ctx = _extract_contexts(ctx)
    return short_date_to_complete_date(node_value(transform_ctx.current()))

def event_dtstamp_from_airsync(ctx):
    parser_ctx, transform_ctx = _extract_contexts(ctx)
    return short_date_to_complete_date(node_value(transform_ctx.current()))

def event_alldayevent_to_airsync(ctx):
    parser_ctx, transform_ctx = _extract_contexts(ctx)
    if node_value(transform_ctx.current()).find("T") >= 0:
        return "0"
    else:
        return "1"

def event_starttime_to_airsync(ctx):
    parser_ctx, transform_ctx = _extract_contexts(ctx)
    return short_date_to_complete_date(node_value(transform_ctx.current()))

def event_starttime_from_airsync(ctx):
    parser_ctx, transform_ctx = _extract_contexts(ctx)
    src_node = transform_ctx.current()
    allday_node = node_find_child(src_node.parent, "AllDayEvent")
    result = short_date_to_complete_date(node_value(transform_ctx.current()))
    if allday_node == None or node_value(allday_node) == "0":
        return result
    else:
        return result[0:8]

def event_endtime_to_airsync(ctx):
    parser_ctx, transform_ctx = _extract_contexts(ctx)
    return short_date_to_complete_date(node_value(transform_ctx.current()))

def event_endtime_from_airsync(ctx):
    parser_ctx, transform_ctx = _extract_contexts(ctx)
    src_node = transform_ctx.current()
    allday_node = node_find_child(src_node.parent, "AllDayEvent")
    result = short_date_to_complete_date(node_value(transform_ctx.current()))
    if allday_node == None or node_value(allday_node) == "0":
        return result
    else:
        return result[0:8]

def event_sensitivity_to_airsync(ctx):
    parser_ctx, transform_ctx = _extract_contexts(ctx)
    s = node_value(transform_ctx.current())
    if s == "PRIVATE":
        return "2"
    elif s == "CONFIDENTIAL":
        return "3"
    else:
        return "0" # 'PUBLIC' is our default value

def event_sensitivity_from_airsync(ctx):
    parser_ctx, transform_ctx = _extract_contexts(ctx)
    s = node_value(transform_ctx.current())
    if s == "2":
        return "PRIVATE"
    elif s == "3":
        return "CONFIDENTIAL"
    else:
        return "0" # 'PUBLIC' is our default value

def event_attendee_to_airsync(ctx):
    parser_ctx, transform_ctx = _extract_contexts(ctx)
    src_node = transform_ctx.current()
    dst_node = transform_ctx.insertNode()
    email = node_value(node_find_child(src_node, "Content"))[7:]
    name = node_value(node_find_child(src_node, "CommonName"))
    if name != "":
        dst_node.newChild(None, "Name", name)
    dst_node.newChild(None, "Email", email)
    return ""

def event_attendee_from_airsync(ctx):
    parser_ctx, transform_ctx = _extract_contexts(ctx)
    src_node = transform_ctx.current()
    dst_node = transform_ctx.insertNode()
    email = node_value(node_find_child(src_node, "Email"))
    name = node_value(node_find_child(src_node, "Name"))
    if email != "":
        dst_node.newChild(None, "Content", "MAILTO:%s" % email)
    dst_node.newChild(None, "CommonName", name)
    return ""

def event_recurrence_to_airsync(ctx):
    parser_ctx, transform_ctx = _extract_contexts(ctx)
    src_node = transform_ctx.current()
    dst_node = transform_ctx.insertNode()

    # Extract the rules
    src_rules = {}
    child = src_node.children
    while child != None:
        if child.name == "Rule":
            rrule_val = node_value(child)
            sep = rrule_val.index("=")
            key = rrule_val[:sep]
            val = rrule_val[sep+1:]
            src_rules[key.lower()] = val.lower()
        child = child.next

    # Interval, Count, and Until rules have straightforward conversions
    if src_rules.has_key("interval"):
        dst_node.newChild(None, "Interval", src_rules["interval"])
    if src_rules.has_key("until"):
        dst_node.newChild(None, "Until", short_date_to_complete_date(src_rules["until"]))
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
            # It seems like this should be against the rules, and filling in
            # a default might not make sense because either of the above interpretations
            # is equally valid.
            raise ValueError("Yearly events must either specify BYMONTH or BYYEARDAY rules")
    return ""

def event_recurrence_from_airsync(ctx):
    parser_ctx, transform_ctx = _extract_contexts(ctx)
    src_node = transform_ctx.current()
    dst_node = transform_ctx.insertNode()

    interval_node    = node_find_child(src_node, "Interval")
    until_node       = node_find_child(src_node, "Until")
    occurrences_node = node_find_child(src_node, "Occurrences")
    type_node        = node_find_child(src_node, "Type")
    dayofweek_node   = node_find_child(src_node, "DayOfWeek")
    dayofmonth_node  = node_find_child(src_node, "DayOfMonth")
    weekofmonth_node = node_find_child(src_node, "WeekOfMonth")
    monthofyear_node = node_find_child(src_node, "MonthOfYear")

    # Add the common nodes that don't really require conversion
    if interval_node != None:
        dst_node.newChild(None, "Rule", "INTERVAL=%s" % node_value(interval_node))
    if until_node != None:
        dst_node.newChild(None, "Rule", "UNTIL=%s" % node_value(until_node))
    if occurrences_node != None:
        dst_node.newChild(None, "Rule", "COUNT=%s" % node_value(occurrences_node))

    if type_node != None:
        type = int(node_value(type_node))

        # Special case: we can treat this as simple weekly event
        if type == 0 and dayofweek_node != None:
            type = 1

        if type == 0:
            dst_node.newChild(None, "Rule", "FREQ=DAILY")
        elif type == 1:
            dst_node.newChild(None, "Rule", "FREQ=WEEKLY")
            dst_node.newChild(None, "Rule", "BYDAY=%s" % airsync_days_to_vcal_days(node_value(dayofweek_node)))
        elif type == 2:
            dst_node.newChild(None, "Rule", "FREQ=MONTHLY")
            dst_node.newChild(None, "Rule", "BYMONTHDAY=%s" % node_value(dayofmonth_node))
        elif type == 3:
            dst_node.newChild(None, "Rule", "FREQ=MONTHLY")
            dst_node.newChild(None, "Rule", "BYDAY=%s" % generate_vcal_byday(node_value(weekofmonth_node), node_value(dayofweek_node)))
        elif type == 5:
            dst_node.newChild(None, "Rule", "FREQ=YEARLY")
            dst_node.newChild(None, "Rule", "BYMONTH=%s" % node_value(monthofyear_node))
            dst_node.newChild(None, "Rule", "BYMONTHDAY=%s" % node_value(dayofmonth_node))
        elif type == 6:
            dst_node.newChild(None, "Rule", "FREQ=YEARLY")
            dst_node.newChild(None, "Rule", "BYMONTH=%s" % node_value(monthofyear_node))
            dst_node.newChild(None, "Rule", "BYDAY=%s" % generate_vcal_byday(node_value(weekofmonth_node), node_value(dayofweek_node)))
        else:
            # Unsupported type
            raise ValueError("Unknown recurrence type %d from Airsync" % type)

    else:
        # If we don't know what type of recurrence it is, we
        # can't construct its vcal rules
        raise ValueError("No recurrence type specified from Airsync")
    return ""

def event_exception_to_airsync(ctx):
    parser_ctx, transform_ctx = _extract_contexts(ctx)
    src_node = transform_ctx.current()
    dst_node = transform_ctx.insertNode()
    exclusion_date = node_value(node_find_child(src_node, "Content"))
    exclusion_value = node_value(node_find_child(src_node, "Value"))
    if exclusion_value.lower() != "date":
        raise ValueError("Exclusions with values other than 'DATE' are not supported")
    dst_node.newChild(None, "Deleted", "1")
    dst_node.newChild(None, "ExceptionStartTime", short_date_to_complete_date(exclusion_date))
    return ""

def event_exception_from_airsync(ctx):
    parser_ctx, transform_ctx = _extract_contexts(ctx)
    src_node = transform_ctx.current()
    dst_node = transform_ctx.insertNode()
    exception_deleted = node_value(node_find_child(src_node, "Deleted"))
    exception_date = node_value(node_find_child(src_node, "ExceptionStartTime"))
    if exception_deleted != "1":
        raise ValueError("Opensync does not support exceptions for modified occurrences")
    dst_node.newChild(None, "Content", complete_date_to_short_date(exception_date))
    dst_node.newChild(None, "Value", "DATE")
    return ""

def register_xslt_extension_functions():
    libxslt.registerExtModuleFunction("contact_anniversary_to_airsync",     "http://synce.org/convert", contact_anniversary_to_airsync)
    libxslt.registerExtModuleFunction("contact_anniversary_from_airsync",   "http://synce.org/convert", contact_anniversary_from_airsync)
    libxslt.registerExtModuleFunction("contact_birthday_to_airsync",        "http://synce.org/convert", contact_birthday_to_airsync)
    libxslt.registerExtModuleFunction("contact_birthday_from_airsync",      "http://synce.org/convert", contact_birthday_from_airsync)
    libxslt.registerExtModuleFunction("event_reminder_to_airsync",          "http://synce.org/convert", event_reminder_to_airsync)
    libxslt.registerExtModuleFunction("event_reminder_from_airsync",        "http://synce.org/convert", event_reminder_from_airsync)
    libxslt.registerExtModuleFunction("event_busystatus_to_airsync",        "http://synce.org/convert", event_busystatus_to_airsync)
    libxslt.registerExtModuleFunction("event_busystatus_from_airsync",      "http://synce.org/convert", event_busystatus_from_airsync)
    libxslt.registerExtModuleFunction("event_dtstamp_to_airsync",           "http://synce.org/convert", event_dtstamp_to_airsync)
    libxslt.registerExtModuleFunction("event_dtstamp_from_airsync",         "http://synce.org/convert", event_dtstamp_from_airsync)
    libxslt.registerExtModuleFunction("event_alldayevent_to_airsync",       "http://synce.org/convert", event_alldayevent_to_airsync)
    libxslt.registerExtModuleFunction("event_starttime_to_airsync",         "http://synce.org/convert", event_starttime_to_airsync)
    libxslt.registerExtModuleFunction("event_starttime_from_airsync",       "http://synce.org/convert", event_starttime_from_airsync)
    libxslt.registerExtModuleFunction("event_endtime_to_airsync",           "http://synce.org/convert", event_endtime_to_airsync)
    libxslt.registerExtModuleFunction("event_endtime_from_airsync",         "http://synce.org/convert", event_endtime_from_airsync)
    libxslt.registerExtModuleFunction("event_sensitivity_to_airsync",       "http://synce.org/convert", event_sensitivity_to_airsync)
    libxslt.registerExtModuleFunction("event_sensitivity_from_airsync",     "http://synce.org/convert", event_sensitivity_from_airsync)
    libxslt.registerExtModuleFunction("event_attendee_to_airsync",          "http://synce.org/convert", event_attendee_to_airsync)
    libxslt.registerExtModuleFunction("event_attendee_from_airsync",        "http://synce.org/convert", event_attendee_from_airsync)
    libxslt.registerExtModuleFunction("event_recurrence_to_airsync",        "http://synce.org/convert", event_recurrence_to_airsync)
    libxslt.registerExtModuleFunction("event_recurrence_from_airsync",      "http://synce.org/convert", event_recurrence_from_airsync)
    libxslt.registerExtModuleFunction("event_exception_to_airsync",         "http://synce.org/convert", event_exception_to_airsync)
    libxslt.registerExtModuleFunction("event_exception_from_airsync",       "http://synce.org/convert", event_exception_from_airsync)
