#!/usr/bin/python

import sys,time
import getopt
from pyrapi import database


def buildSimple(value):
    return value

def buildDateTime(value):
    return time.strftime(time_format,time.gmtime(value))

def buildDuration(value):
    return "-P%dDT%s" % ( value/(60*60*24),
                         time.strftime("%HH%MM%SS", time.gmtime(value)))

task_db_mapping = {"Subject":   ('SUMMARY',buildSimple),
                   "DueDate":   ('DUE',    buildDateTime),
                   "StartDate": ('DTSTART',buildDateTime),
                   "Start":     ('DTSTART',buildDateTime),
                   "Duration":  ('DURATION',buildDuration)} 

time_format = r'%Y%m%dT%H%M%S'


def foldLine(name,line):
    max_line_length = 74

    ret_buf = "%s\n" % (name,)

    ret_buf += " :%s\n" % (line[0:max_line_length],)
    
    for i in range(1,len(line)/max_line_length):
        ret_buf += " %s\n" % (line[i*max_line_length:((i+1)*max_line_length)-1],)

    if ((len(line) / max_line_length) and (len(line) % max_line_length)):
        ret_buf += " %s\n" % (line[-(len(line) % max_line_length):],)

    return ret_buf

def writeIcalHead(file):
    file.write(foldLine("BEGIN","VCALENDAR"))
    file.write(foldLine("PRODID","-//K Desktop Environment//NONSGML KOrganizer 3.0.3//EN"))
    file.write(foldLine("VERSION","2.0"))

def writeIcalTail(file):
    file.write(foldLine("END","VCALENDAR"))
    
def writeTaskICAL(file,record):
    file.write(foldLine("BEGIN","VTODO"))
    writeICALRecord(record)
    file.write(foldLine("END","VTODO"))

def writeEventICAL(file,record):
    file.write(foldLine("BEGIN","VEVENT"))
    writeICALRecord(record)
    file.write(foldLine("END","VEVENT"))
    
def writeICALRecord(record):
    file.write(foldLine("UID","rapi_sync-%i" % (record.getoid(),)))

    for field_oid in record.keys():
        print "Field_oid ->", repr(field_oid)
        print "task_db_mapping ->", task_db_mapping
        if task_db_mapping.has_key(field_oid):
            file.write(foldLine(task_db_mapping[field_oid][0],
                                task_db_mapping[field_oid][1](record[field_oid])))
            



opts, args = getopt.getopt(sys.argv[1:], 'at')
#print opts,args


appointments = 0
tasks        = 0

for o, a in opts:
    if o in ('-a',):
        appointments = 1
    if o in ('-t',):
        tasks = 1
     

if len(args) != 1:
    print "Usage: %s [-a -t] filename" % (sys.argv[0],)
    sys.exit(-1)
    
outfile = args[0]


file = open(outfile,'w')
writeIcalHead(file)


if tasks:
    db = database.TasksDatabase()
    for record in db:
        writeTaskICAL(file,record)

if appointments:
    db = database.AppointmentsDatabase()

    for record in db:
        writeEventICAL(file,record)

writeIcalTail(file)

file.close()

