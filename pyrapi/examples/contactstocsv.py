#!/usr/bin/python

import sys,time
import getopt
from pyrapi import pyrapi

# list of task fields taken from
# http://msdn.microsoft.com/library/default.asp?url=/library/en-us/wcesdkr/htm/_wcesdk_ITask_Unknown.asp
#
# As you can see many are still to be discovered :-)
#

Birthday                = 16385 #(date)
BusinessFaxNumber       = 14884
Email1Address           = 16531
MobileTelephoneNumber   = 14876
BusinessTelephoneNumber = 14856
HomeTelephoneNumber     = 14857
Email2Address           = 16531
FileAs                  = 16403
Title                   = 16419
FirstName               = 14854
LastName                = 14865
HomeAddressStreet       = 16453
HomeAddressCity         = 16454
HomeAddressState        = 16455
HomeAddressPostalCode   = 16456
HomeAddressCountry      = 16452
CompanyName             = 14870
OfficeLocation          = 14873
Department              = 14872
JobTitle                = 14871
BusinessAddressCity     = 16454
BusinessAddressState    = 16455
BusinessAddressCountry  = 16457

field_list = (Birthday, 
              BusinessFaxNumber,
              Email1Address,  
              MobileTelephoneNumber,   
              BusinessTelephoneNumber, 
              HomeTelephoneNumber,    
              Email2Address,         
              FileAs,                  
              Title,                  
              FirstName,              
              LastName,               
              HomeAddressStreet,      
              HomeAddressCity,        
              HomeAddressState,       
              HomeAddressPostalCode,  
              HomeAddressCountry,     
              CompanyName,            
              OfficeLocation,         
              Department,             
              JobTitle,                
              BusinessAddressCity,    
              BusinessAddressState,   
              BusinessAddressCountry)

##Anniversary (date)
##PagerNumber
##Spouse
##Email3Address
##Home2TelephoneNumber
##HomeFaxNumber
##CarTelephoneNumber
##AssistantName
##AssistantTelephoneNumber
##Children
##Categories
##WebPage
##Business2TelephoneNumber
##RadioTelephoneNumber
##MiddleName
##Suffix
##OtherAddressStreet
##OtherAddressCity
##OtherAddressState
##OtherAddressPostalCode
##OtherAddressCountry
##BusinessAddressStreet
##BusinessAddressPostalCode
##Body
##BodyInk
##Application

##def buildSimple(value):
##    return value

##def buildDateTime(value):
##    return time.strftime(time_format,time.gmtime(value))

##def buildDuration(value):
##    return "-P%dDT%s" % ( value/(60*60*24),
##                         time.strftime("%HH%MM%SS", time.gmtime(value)))

##task_db_mapping = {Subject:   ('SUMMARY',buildSimple),
##                   DueDate:   ('DUE',    buildDateTime),
##                   StartDate: ('DTSTART',buildDateTime),
##                   Start:     ('DTSTART',buildDateTime),
##                   Duration:  ('DURATION',buildDuration)} 

##time_format = r'%Y%m%dT%H%M%S'

def getDBList():
    return pyrapi.CeFindAllDatabases()

def getDB(dblist,db_name):
    for db in dblist:
        print db
        if db[0] == db_name:
            db_hand =  pyrapi.CeOpenDatabase(db)

    db = []
    #for i in range(0,10):
    while (1):
        res = pyrapi.CeReadRecordProps(db_hand)
        if res == None:
            break
        db.append(res)

    return db

def getContactsDB(dblist):
    return getDB(dblist,'Contacts Database')

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



def writeCSVHead(file):
    pass

def writeCSVTail(file):
    pass

def writeContact(file,oid,record):
    #file.write(foldLine("BEGIN","VTODO"))
    writeCSVRecord(oid,record)
    #file.write(foldLine("END","VTODO"))

def writeEventICAL(file,oid,record):
    file.write(foldLine("BEGIN","VEVENT"))
    writeICALRecord(oid,record)
    file.write(foldLine("END","VEVENT"))
    
def writeCSVRecord(oid,record):
    #file.write(foldLine("UID","rapi_sync-%i" % (oid,)))

    buf = ''
    for field in field_list:
        print "field: %s record: %s" % (str(field),str(record.keys()))
        if record.has_key(field):
            print "got one!"
            buf += "%s," % (str(record[field][1]),)
        else:
            buf += ","

    buf = buf[:-1]
    buf += '\n'

    file.write(buf)



opts, args = getopt.getopt(sys.argv[1:], '')
#print opts,args


##appointments = 0
##tasks        = 0

##for o, a in opts:
##    if o in ('-a',):
##        appointments = 1
##    if o in ('-t',):
##        tasks = 1
     

if len(args) != 1:
    print "Usage: %s  filename" % (sys.argv[0],)
    sys.exit(-1)
    
outfile = args[0]


file = open(outfile,'w')
#writeIcalHead(file)

dblist = getDBList()
db = getContactsDB(dblist)
printDB(db)
for record in db:
    writeContact(file,record[0],record[1])

##if appointments:
##    db = getAppointmentsDB(dblist)

##    for record in db:
##        writeEventICAL(file,record[0],record[1])

##writeIcalTail(file)

file.close()
