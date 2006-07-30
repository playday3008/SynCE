#!/usr/bin/env python

import sys
sys.path.insert(0, "..")
import codecs
from engine import formats
from xml.dom import minidom
from xml import xpath

print "Opening and parsing AirSync XML"
as_doc = minidom.parseString(open("contact_airsync.xml", "r").read())
#print as_doc.toprettyxml(encoding="utf-8")

print "Number of AirSync nodes:", len(xpath.Evaluate("/ApplicationData/*", as_doc))

print "Converting to OpenSync format...",
os_doc = formats.contact.from_airsync("pas-id-1234567800000000", as_doc.documentElement)
print "done"

print os_doc.toprettyxml(encoding="utf-8")

print "Converting back to AirSync format...",
doc = formats.contact.to_airsync(os_doc)
print "done"

print doc.toprettyxml()

print "Number of AirSync nodes now:", len(xpath.Evaluate("/ApplicationData/*", doc))

