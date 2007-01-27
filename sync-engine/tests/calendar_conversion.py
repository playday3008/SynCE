#!/usr/bin/env python

import cPickle as pickle
import sys
sys.path.insert(0, "..")
import codecs
from engine import formats
from xml.dom import minidom
from xml import xpath

def extract_biggest():
    f = open("/tmp/se_debug", "rb")
    entries = []
    biggest_count = 0
    biggest_index = 0
    while True:
        try:
            print len(entries)
            node = pickle.load(f)
            if len(node.childNodes) > biggest_count:
                biggest_count = len(node.childNodes)
                biggest_index = len(entries)
                print "i=%d c=%d" % (biggest_index, biggest_count)
            entries.append(node)
        except Exception, e:
            f.close()
            print e
            break

    print "%d items" % len(entries)

    f = codecs.open("calendar_airsync.xml", "w", "utf-8")
    f.write(entries[biggest_index].toxml())
    f.close()

ex1_doc = minidom.parseString(open("calendar_opensync_ex1.xml", "r").read())
ex2_doc = minidom.parseString(open("calendar_opensync_ex2.xml", "r").read())
ex3_doc = minidom.parseString(open("calendar_opensync_ex3.xml", "r").read())

print "Example 1:"
print ex1_doc.toprettyxml()
print

print "Example 2:"
print ex2_doc.toprettyxml()
print

print "Example 3:"
print ex3_doc.toprettyxml()

print
print "----------------------------------------------------------------------"
print

for name in ("calendar_airsync.xml", "calendar_airsync_bday.xml"):
    as_doc = minidom.parseString(open(name, "r").read())

    print "Document to convert:"
    print as_doc.toprettyxml()
    print

    print "Converted:"
    os_doc = formats.event.from_airsync("pas-id-1234567800000000", as_doc.documentElement)
    print os_doc.toprettyxml()

    print "Converted back:"
    doc = formats.event.to_airsync(os_doc)
    print doc.toprettyxml()

