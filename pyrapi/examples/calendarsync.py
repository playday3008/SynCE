#!/usr/bin/python

import sys,time
import getopt
from  pyrapi import pyrapi


##task_db_mapping = {16678: 'unknown',
##                   17673: 'sound', 
##                   17674: 'unknown',
##                   55:    'SUMMARY',
##                   16655: 'unkown',
##                   17667: 'unknown',
##                   22:    'unknown',
##                   23:    'unknown', 
##                   16644: 'DUE',
##                   38:    'unknown',
##                   16645: 'DTSTART', 
##                   17665: 'unknown'}

# list of task fields taken from
# http://msdn.microsoft.com/library/default.asp?url=/library/en-us/wcesdkr/htm/_wcesdk_ITask_Unknown.asp
#
# As you can see many are still to be discovered :-)
#
Subject            = 55
Categories         = 22
StartDate          = 16644
DueDate            = 16645
ReminderSoundFile  = 17673
XNotes	           = 23    # Blob contains note in known format
Location           = 16904
Start              = 16909
Duration           = 16915
XDurationUnit      = 16917
Sensitivity        = 4   # Private 1 Public 0
BusyStatus         = 15 # Free 0, Tentative 1, Busy 2, OutOfOffice 3
XSecret            = 103
ReminderMinutesBeforeStart = 17665
ReminderEnabled    = 17667
Importance         = 38 # High 1 Normal 2 Low 3
IsRecurring        = 16678 # Noreoccurance 0, reoccuring 1
RecurranceDetail   = 16662 # Blob, unknown content. only present when IsRecurring = 1
Complete           = 16655 # 0 uncomplete, 1 complete

# currently unknown 
##DateCompleted      = None
##Sensitivity        = None
##TeamTask           = None
##ReminderSet        = None
##ReminderOptions    = None
##ReminderTime       = None
##Body               = None
##BodyInk            = None
##Application        = None
##End
##AllDayEvent
##MeetingStatus
##ReminderSet
##ReminderSoundFile
##ReminderOptions
##Recipients
##Body
##BodyInk
##Application

def buildSimple(value):
    return value

def buildDateTime(value):
    return time.strftime(time_format,time.gmtime(value))

def buildDuration(value):
    return "-P%dDT%s" % ( value/(60*60*24),
                         time.strftime("%HH%MM%SS", time.gmtime(value)))

task_db_mapping = {Subject:   ('SUMMARY',buildSimple),
                   DueDate:   ('DUE',    buildDateTime),
                   StartDate: ('DTSTART',buildDateTime),
                   Start:     ('DTSTART',buildDateTime),
                   Duration:  ('DURATION',buildDuration)} 

time_format = r'%Y%m%dT%H%M%S'

def getDBList():
    return pyrapi.CeFindAllDatabases()

def getDB(dblist,db_name):
    for db in dblist:
        print db
        if db[0] == db_name:
            db_hand =  pyrapi.CeOpenDatabase(db)

    db = []
    #for i in range(0,100):
    while (1):
        res = pyrapi.CeReadRecordProps(db_hand)
        if res == None:
            break
        db.append(res)

    return db
    
def getTaskDB(dblist):
    return getDB(dblist,'Tasks Database')

def getAppointmentsDB(dblist):
    return getDB(dblist,'Appointments Database')

def printDB(db):
    for record in db:
        print "\n=+++++++++++++++++++++  New record (%i) +++++++++++++++++++++++++++++++++=\n" % (record[0],)
        rec = record[1]
        for field in rec.keys():
            if rec[field][0] == "FILETIME":
                val = time.ctime(rec[field][1])
            else:
                val = str(rec[field][1])

            print "OID: %s \t TYPE: %s \t VALUE: %s " % (str(field),str(rec[field][0]),val)


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
    
def writeTaskICAL(file,oid,record):
    file.write(foldLine("BEGIN","VTODO"))
    writeICALRecord(oid,record)
    file.write(foldLine("END","VTODO"))

def writeEventICAL(file,oid,record):
    file.write(foldLine("BEGIN","VEVENT"))
    writeICALRecord(oid,record)
    file.write(foldLine("END","VEVENT"))
    
def writeICALRecord(oid,record):
    file.write(foldLine("UID","rapi_sync-%i" % (oid,)))

    for field_oid in record.keys():
        if task_db_mapping.has_key(field_oid):
            file.write(foldLine(task_db_mapping[field_oid][0],
                                task_db_mapping[field_oid][1](record[field_oid][1])))
            



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

dblist = getDBList()
if tasks:
    db = getTaskDB(dblist)
    for record in db:
        writeTaskICAL(file,record[0],record[1])

if appointments:
    db = getAppointmentsDB(dblist)

    for record in db:
        writeEventICAL(file,record[0],record[1])

writeIcalTail(file)

file.close()

printDB(db)

#printDB(getAppointmentsDB(getDBList()))

#print foldLine("123456789012345678901234567890123456789012345678901234567890123456789012345next")
#print foldLine("123456789012345678901234567890123456789012345678901234567890123456789012345123456789012345678901234567890123456789012345678901234567890123456789012345123456789012345678901234567890123456789012345678901234567890123456789012345last")
