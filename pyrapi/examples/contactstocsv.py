#!/usr/bin/python

import sys,time,string
import getopt

from pyrapi import database
from pyrapi import pyrapi


def writeCSVHead(file,db):
    file.write(string.join(db.getFieldMapping().keys(),','))
    file.write('\n')

def writeCSVTail(file):
    pass

def writeContact(file,db,record):
    writeCSVRecord(db,record)


def removeBadChars(s):
    s = string.join(string.split(s,'\n'),' ')
    s = string.join(string.split(s,','), ' ')
    return s

def writeCSVRecord(db,record):

    buf = ''
    record_fields = record.keys()
    for field_name in db.getFieldMapping():
        if field_name in record_fields:
            buf += "%s," % (removeBadChars(str(record[field_name])),)
        else:
            buf += ","

    buf = buf[:-1]
    buf += '\n'

    file.write(buf)



opts, args = getopt.getopt(sys.argv[1:], '')

if len(args) != 1:
    print "Usage: %s  filename" % (sys.argv[0],)
    sys.exit(-1)
    
outfile = args[0]


file = open(outfile,'w')

db = database.ContactsDatabase()

writeCSVHead(file,db)

for record in db:
    writeContact(file,db,record)

file.close()
