#!/usr/bin/env python

import sys
sys.path.insert(0, "..")
import cPickle as pickle
from xml.dom import minidom
from xml import xpath
from engine.util import *
from engine.constants import *
import dbus

f = open(sys.argv[1], "rb")
entries = {}
while True:
    try:
        guid = pickle.load(f)
        xml = pickle.load(f)

        entries[guid] = xml
    except Exception, e:
        f.close()
        break

# find the dupes based on subject
subjects = {}
for guid, xml in entries.items():
    doc = minidom.parseString(xml.encode("utf-8"))
    subj = node_get_value(xpath.Evaluate("/ApplicationData/Subject", doc)[0])
    start = node_get_value(xpath.Evaluate("/ApplicationData/StartTime", doc)[0])
    if not subj in subjects:
        instances = []
        subjects[subj] = instances
    else:
        instances = subjects[subj]

    instances.append((guid, start, xml))

# then build a list of the ones to keep based on the highest date
kept = {}
for subj, instances in subjects.items():
    #print "\"%s\": %d instances" % (subj, len(instances))
    date = ""
    guid = ""
    for guid, start, xml in instances:
        d = start[:8]
        c = start[9:11]
        if d > date and c == "22":
            date = d
            guid = guid

    kept[guid] = date

print "Keeping %d entries" % len(kept)

# now build a list of GUIDs to delete, excluding the ones to keep
changes = { SYNC_ITEM_CALENDAR : [] }
for subj, instances in subjects.items():
    for guid, start, xml in instances:
        if guid in kept:
            continue
        changes[SYNC_ITEM_CALENDAR].append((guid, CHANGE_DELETED, ""))

print "Deleting %d items" % len(changes[SYNC_ITEM_CALENDAR])

bus = dbus.SessionBus()
proxy_obj = bus.get_object("org.synce.SyncEngine", "/org/synce/SyncEngine")
engine = dbus.Interface(proxy_obj, "org.synce.SyncEngine")

print "Submitting to SyncEngine...",
engine.AddLocalChanges(changes)
print "done"

