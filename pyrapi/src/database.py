"""
    Classes to provide access to PocketPC databases.

"""

from __future__ import generators

import pyrapi

class CeRecord(object):
    """
    Base class for CeDatabase records.
    """

    def __init__(self, record,field_mapping=None):
        """
        record - the return from the CeReadRecordProps call.
        """

        self._oid    = record[0]
        self._record = record[1]
        self._field_mapping = field_mapping
        
        self._reverse_mapping = {}
        for mapping in self._field_mapping:
            if self._field_mapping[mapping] in self._record.keys():
                self._reverse_mapping[self._field_mapping[mapping]] = mapping

        print "Reverse mapping -> ",repr(self._reverse_mapping)
        print "Record -> ", repr(self._record)

    def getoid(self):
        return self._oid

    def keys(self):
        if self._field_mapping == None:
            return self._record.keys()

        return self._reverse_mapping.values()
    
    def __getitem__(self,field_oid):
        """
        Accessor function so that record can be treated as a dictionary.
        """
        if self._field_mapping.has_key(field_oid):
            # field_oid is a string name
            return self._record[self._field_mapping[field_oid]][1]
        
        self._record[field_oid][1]

    def next(self):
        """
        Return the next field in the record.
        """
        for field in self._record.values():
            yield field[1]

        raise StopIteration
    
    def __iter__(self):
        return self

    def __repr__(self):
        s = "Record oid = %s\n" % (str(self._oid),)
        s += "oid\t name\t type\t value\n\n"
        for oid,field in self._record.items():
            if self._field_mapping == None:
                s += "%s\t None\t %s\t %s\n" % (oid,field[0],field[1])
            else:
                field_name = "Unknown"
                for mapping in self._field_mapping:
                    if self._field_mapping[mapping] == oid:
                        field_name = mapping
                s += "%s\t %s\t %s\t %s\n" % (str(oid),field_name,field[0],str(field[1]))

        return s
        
    
class CeDatabase(object):
    """
    Container class for PocketPC database.
    """

    # list of databases, this is filled in by the first CeDatabase object that
    # is created and then reused.
    _database_list = None

    def __init__(self,database_name,field_mapping=None,record_class=CeRecord):
        self._record_class=CeRecord
        self._database_name=database_name
        self._field_mapping=field_mapping

        self._database_oid=None

        if CeDatabase._database_list == None:
            CeDatabase._database_list=pyrapi.CeFindAllDatabases()

        for db in CeDatabase._database_list:
            if db[0] == self._database_name:
                self._database_oid = pyrapi.CeOpenDatabase(db)

    def getFieldMapping(self):
        return self._field_mapping

    def __iter__(self):
        return self
    
    def next(self):
        """
        Return the next record in the database.
        The class of the object returned is governed by the record class passed into the __init__.
        """

        record = pyrapi.CeReadRecordProps(self._database_oid)
        if record == None:
            raise StopIteration

        return self._record_class(record,self._field_mapping)
    

# list of task fields taken from
# http://msdn.microsoft.com/library/default.asp?url=/library/en-us/wcesdkr/htm/_wcesdk_ITask_Unknown.asp
#
# As you can see many are still to be discovered :-)

_app_and_task_field_mappings = {"Subject"            : 55,
                                "Categories"         : 22,
                                "StartDate"          : 16644,
                                "DueDate"            : 16645,
                                "ReminderSoundFile"  : 17673,
                                "XNotes"	            : 23,    # Blob contains note in known format
                                "Location"           : 16904,
                                "Start"              : 16909,
                                "Duration"           : 16915,
                                "XDurationUnit"      : 16917,
                                "Sensitivity"        : 4,   # Private 1 Public 0
                                "BusyStatus"         : 15, # Free 0, Tentative 1, Busy 2, OutOfOffice 3
                                "XSecret"            : 103,
                                "ReminderMinutesBeforeStart" : 17665,
                                "ReminderEnabled"    : 17667,
                                "Importance"         : 38, # High 1 Normal 2 Low 3
                                "IsRecurring"        : 16678, # Noreoccurance 0, reoccuring 1
                                "RecurranceDetail"   : 16662, # Blob, unknown content. only present when IsRecurring = 1
                                "Complete"           : 16655}

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



class AppointmentsDatabase(CeDatabase):
    """
    Class to access appointments.
    """

    def __init__(self):
        CeDatabase.__init__(self,'Appointments Database',_app_and_task_field_mappings,CeRecord)

class TasksDatabase(CeDatabase):
    """
    Class to access tasks.
    """

    def __init__(self):
        CeDatabase.__init__(self,'Tasks Database',_app_and_task_field_mappings,CeRecord)



_contact_field_mappings = {"Birthday"                : 16385, #(date)
                           "BusinessFaxNumber"       : 14884,
                           "HomeFaxNumber"           : 14885,
                           "Email2Address"           : 16531,
                           "Email1Address"           : 16515,
                           "Email3Address"           : 16547,
                           "MobileTelephoneNumber"   : 14876,
                           "BusinessTelephoneNumber" : 14856,
                           "Business2TelephoneNumber" : 16391,
                           "HomeTelephoneNumber"     : 14857,
                           "Home2TelephoneNumber"    : 14895,
                           "CarTelephoneNumber"      : 14878,
                           "Email2Address"           : 16531,
                           "FileAs"                  : 16403,
                           "Title"                   : 16419,
                           "FirstName"               : 14854,
                           "MiddleName"              : 16420,
                           "LastName"                : 14865,
                           "HomeAddressStreet"       : 16453,
                           "HomeAddressCity"         : 16454,
                           "HomeAddressState"        : 16455,
                           "HomeAddressPostalCode"   : 16456,
                           "HomeAddressCountry"      : 16452,
                           "CompanyName"             : 14870,
                           "OfficeLocation"          : 14873,
                           "Department"              : 14872,
                           "JobTitle"                : 14871,
                           "BusinessAddressCity"     : 16454,
                           "BusinessAddressState"    : 16455,
                           "BusinessAddressCountry"  : 16457,
                           "WebPage"                 : 16392,
                           "PagerNumber"             : 16393}

##Anniversary (date)
##Spouse
##AssistantName
##AssistantTelephoneNumber
##Children
##Categories
##RadioTelephoneNumber
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

class ContactsDatabase(CeDatabase):
    """
    Class to contacts.
    """

    def __init__(self):
        CeDatabase.__init__(self,'Contacts Database',_contact_field_mappings,CeRecord)
