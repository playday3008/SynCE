from pyrapi import pyrapi
import unittest

def get_field_value(field):
    if field.type ==  pyrapi.CEVT_I2:
        return field.val.iVal
    if field.type ==  pyrapi.CEVT_I4:
        return field.val.lVal
    if field.type ==  pyrapi.CEVT_R8:
        return field.val.dblVal
    if field.type ==  pyrapi.CEVT_BOOL:
        return field.val.boolVal
    if field.type ==  pyrapi.CEVT_UI2:
        return field.val.uiVal
    if field.type ==  pyrapi.CEVT_UI4:
        return field.val.ulVal
    if field.type ==  pyrapi.CEVT_LPWSTR:
        return field.val.lpwstr
    if field.type ==  pyrapi.CEVT_FILETIME:
        return field.val.filetime
    if field.type ==  pyrapi.CEVT_BLOB:
        # Not yet implemented
        return "BLOB"
        #return repr(field.blob)

def set_field_value(field,value):
    if field.type ==  pyrapi.CEVT_I2:
        field.val.iVal = value
    if field.type ==  pyrapi.CEVT_I4:
       field.val.lVal = value
    if field.type ==  pyrapi.CEVT_R8:
        field.val.dblVal = value
    if field.type ==  pyrapi.CEVT_BOOL:
        field.val.boolVal = value
    if field.type ==  pyrapi.CEVT_UI2:
        field.val.uiVal = value
    if field.type ==  pyrapi.CEVT_UI4:
        field.val.ulVal = value
    if field.type ==  pyrapi.CEVT_LPWSTR:
        field.val.lpwstr = value
    if field.type ==  pyrapi.CEVT_FILETIME:
        field.val.filetime = value
    if field.type ==  pyrapi.CEVT_BLOB:
        field.blob = value
    

class RapiFileTests(unittest.TestCase):

    _test_directory = "\\My Documents\\pyrapi_tests"
    _test_file_name = _test_directory + "\\test_file.txt"
    
    def setUp(self):
        """Setup a directory to work in."""

        try:
            pyrapi.CeCreateDirectory(RapiFileTests._test_directory)
        except RuntimeError:
            pass
        
    def tearDown(self):
        """Delete the test directory"""

        try: pyrapi.CeDeleteFile(RapiFileTests._test_file_name)
        except: pass
        try: pyrapi.CeRemoveDirectory(RapiFileTests._test_directory)
        except: pass
    

class FileAccessFunctions(RapiFileTests):

    def testCeCloseHandle(self):
        """CeCloseHandle should close an open handle."""

        f = pyrapi.CeCreateFile(RapiFileTests._test_file_name, pyrapi.GENERIC_WRITE, 0, pyrapi.CREATE_ALWAYS)
        
        pyrapi.CeCloseHandle(f)
        
    def testCeCreateFile(self):
        """CeCreateFile should create files for create,read,write"""

        # Create for write
        f = pyrapi.CeCreateFile(RapiFileTests._test_file_name, pyrapi.GENERIC_WRITE, 0, pyrapi.CREATE_ALWAYS)
        pyrapi.CeCloseHandle(f)

        # Create for read
        f = pyrapi.CeCreateFile(RapiFileTests._test_file_name, pyrapi.GENERIC_READ)
        pyrapi.CeCloseHandle(f)

    
    def testCeReadFile(self):
        """CeReadFile should read a file."""

        import time
        testline = time.ctime(time.time())
        
        f = pyrapi.CeCreateFile(RapiFileTests._test_file_name, pyrapi.GENERIC_WRITE, 0, pyrapi.CREATE_ALWAYS)
        pyrapi.CeWriteFile(f,testline)
        pyrapi.CeCloseHandle(f)

        f = pyrapi.CeCreateFile(RapiFileTests._test_file_name, pyrapi.GENERIC_READ)
        res = pyrapi.CeReadFile(f,100)
        pyrapi.CeCloseHandle(f)

        self.assertEqual(testline,res)

    
    def testCeWriteFile(self):
        """CeWriteFile should write a file."""
        pass # same as testCeReadFile




class FileManagementFunctions(RapiFileTests):


    def testCeCopyFile(self):
        """CeCopyFile should copy file."""

        import time
        testline = time.ctime(time.time())

        f = pyrapi.CeCreateFile(RapiFileTests._test_file_name, pyrapi.GENERIC_WRITE, 0, pyrapi.CREATE_ALWAYS)
        pyrapi.CeWriteFile(f,testline)
        pyrapi.CeCloseHandle(f)

        copy_file_name = RapiFileTests._test_directory+"\\copy_file.txt"
        pyrapi.CeCopyFile(RapiFileTests._test_file_name, copy_file_name, 0)
        
        f = pyrapi.CeCreateFile(copy_file_name, pyrapi.GENERIC_READ)
        res = pyrapi.CeReadFile(f,100)
        pyrapi.CeCloseHandle(f)

        self.assertEqual(testline,res)

        pyrapi.CeDeleteFile(copy_file_name)

        
    def CeCreateDirectory(self):
        """CeCreateDirectory """
        pass # done by SetUp
        
    def CeDeleteFile(self):
        """CeDeleteFile"""
        pass # done by just about everything
        
    def testCeFindAllFiles(self):
        """CeFindAllFiles"""

        import time
        testline = time.ctime(time.time())

        for filename in range(1,11):
            f = pyrapi.CeCreateFile(RapiFileTests._test_directory+"\\XX"+str(filename),
                                    pyrapi.GENERIC_WRITE, 0, pyrapi.CREATE_ALWAYS)
            pyrapi.CeWriteFile(f,testline)
            pyrapi.CeCloseHandle(f)

        found_files = pyrapi.CeFindAllFiles(RapiFileTests._test_directory+"\\XX*",
                                            pyrapi.FAF_NAME|\
                                            pyrapi.FAF_ATTRIBUTES|\
                                            pyrapi.FAF_CREATION_TIME|\
                                            pyrapi.FAF_LASTACCESS_TIME|\
                                            pyrapi.FAF_LASTWRITE_TIME|\
                                            pyrapi.FAF_SIZE_LOW)
                      
        for filename in range(1,11):        
            pyrapi.CeDeleteFile(RapiFileTests._test_directory+"\\XX"+str(filename))

        self.assertEqual(len(found_files),10)


    def testCeFindFirstFile(self):
        """CeFindFirstFile"""

        import time
        testline = time.ctime(time.time())

        for filename in range(1,4):
            f = pyrapi.CeCreateFile(RapiFileTests._test_directory+"\\XX"+str(filename),
                                    pyrapi.GENERIC_WRITE, 0, pyrapi.CREATE_ALWAYS)
            pyrapi.CeWriteFile(f,testline)
            pyrapi.CeCloseHandle(f)

        ret = pyrapi.CeFindFirstFile(RapiFileTests._test_directory+"\\XX*")

        self.assertEqual(ret[1].cFileName, "XX3")

        file = pyrapi.CeFindNextFile(ret[0])
        self.assertEqual(file.cFileName, "XX2")

        pyrapi.CeFindClose(ret[0])
        
        
    def CeFindNextFile(self):
        """CeFindNextFile"""
        pass # done by testCeFindFirstFile
        
    def CeFindClose(self):
        """CeFindClose"""
        pass # done by testCeFindFirstFile
        
    def testCeGetFileAttributes(self):
        """CeGetFileAttributes"""

        attr = pyrapi.CeGetFileAttributes(RapiFileTests._test_directory)


        self.failUnless(attr & pyrapi.FILE_ATTRIBUTE_DIRECTORY, "attribute should be a directory")
        
    def testCeGetSpecialFolderPath(self):
        """CeGetSpecialFolderPath should return all special folders"""

        for i in  [pyrapi.CSIDL_PROGRAMS,
                   pyrapi.CSIDL_PERSONAL,
                   pyrapi.CSIDL_FAVORITES_GRYPHON,
                   pyrapi.CSIDL_STARTUP,
                   pyrapi.CSIDL_RECENT,
                   pyrapi.CSIDL_STARTMENU,
                   #pyrapi.CSIDL_DESKTOPDIRECTORY, # Currently broken
                   pyrapi.CSIDL_FONTS,
                   pyrapi.CSIDL_FAVORITES]:
            
            f = pyrapi.CeGetSpecialFolderPath(i)
            self.failUnless(len(f) > 0, "folder %d should have a non zero length name" % (i,))

    def testCeMoveFile(self):
        """CeMoveFile"""

        import time
        testline = time.ctime(time.time())

        orig_file_name  = RapiFileTests._test_directory + "\\to_move"
        moved_file_name = RapiFileTests._test_directory + "\\moved"

        try: pyrapi.CeDeleteFile(moved_file_name) # clean up from any previous run
        except: pass

        f = pyrapi.CeCreateFile(orig_file_name, pyrapi.GENERIC_WRITE, 0, pyrapi.CREATE_ALWAYS)
        pyrapi.CeWriteFile(f,testline)
        pyrapi.CeCloseHandle(f)

        pyrapi.CeMoveFile(orig_file_name, moved_file_name)
        
        f = pyrapi.CeCreateFile(moved_file_name, pyrapi.GENERIC_READ)
        res = pyrapi.CeReadFile(f,100)
        pyrapi.CeCloseHandle(f)

        self.assertEqual(testline,res)

        pyrapi.CeDeleteFile(moved_file_name)

    def CeRemoveDirectory(self):
        """CeRemoveDirectory"""
        pass # done by TearDown
        
    def testCeSetFileAttributes(self):
        """CeSetFileAttributes"""

        # Create a file to work with
        
        import time
        testline = time.ctime(time.time())
        filename = RapiFileTests._test_directory+"\\attr_test"

        f = pyrapi.CeCreateFile(filename,
                                pyrapi.GENERIC_WRITE, 0, pyrapi.CREATE_ALWAYS)
        pyrapi.CeWriteFile(f,testline)
        pyrapi.CeCloseHandle(f)

        # Set its attr to normal and check that it worked

        pyrapi.CeSetFileAttributes(filename, pyrapi.FILE_ATTRIBUTE_NORMAL)
        attr = pyrapi.CeGetFileAttributes(filename)
        self.failUnless(attr & pyrapi.FILE_ATTRIBUTE_NORMAL, "attr should have been set to FILE_ATTRIBUTE_NORMAL")

        # check each of the other attributes

        for new_attr in [pyrapi.FILE_ATTRIBUTE_ARCHIVE,
                         pyrapi.FILE_ATTRIBUTE_HIDDEN,
                         pyrapi.FILE_ATTRIBUTE_READONLY,
                         pyrapi.FILE_ATTRIBUTE_SYSTEM,
                         #pyrapi.FILE_ATTRIBUTE_TEMPORARY # Does not work
                         ]:

            pyrapi.CeSetFileAttributes(filename, new_attr)
            attr = pyrapi.CeGetFileAttributes(filename)
            self.failUnless(attr & new_attr, "attr should have been set to %0x" % (new_attr,))

            # Set back to FILE_ATTRIBUTE_NORMAL before check next attr.
            pyrapi.CeSetFileAttributes(filename, pyrapi.FILE_ATTRIBUTE_NORMAL)
            attr = pyrapi.CeGetFileAttributes(filename)
            self.failUnless(attr & pyrapi.FILE_ATTRIBUTE_NORMAL, "attr should have been set to FILE_ATTRIBUTE_NORMAL")            
        # remove the test file
        pyrapi.CeDeleteFile(filename)



class RapiDatabaseTests(unittest.TestCase):

    _test_db_name = "pyrapi_test_db"

    def setUp(self):
        #Clean up any previous test run.
        try:
            new_db = [db for db in pyrapi.CeFindAllDatabases() if db.DbInfo.szDbaseName == RapiDatabaseTests._test_db_name]
            pyrapi.CeDeleteDatabase(new_db[0].OidDb)
        except: pass
        

    def testCeCreateDatabase(self):
        """CeCreateDatabase"""

        dboid = pyrapi.CeCreateDatabase(RapiDatabaseTests._test_db_name, 0, ())

        new_db = [db for db in pyrapi.CeFindAllDatabases() if db.DbInfo.szDbaseName == RapiDatabaseTests._test_db_name]

        self.failIfEqual(len(new_db),0,"new db not in database list")

        pyrapi.CeDeleteDatabase(new_db[0].OidDb)
        
                                
    def testCeDeleteDatabase(self):
        """CeDeleteDatabase"""

        dboid = pyrapi.CeCreateDatabase(RapiDatabaseTests._test_db_name, 0, ())

        new_db = [db for db in pyrapi.CeFindAllDatabases() if db.DbInfo.szDbaseName == RapiDatabaseTests._test_db_name]

        self.failIfEqual(len(new_db),0,"new db not in database list")

        pyrapi.CeDeleteDatabase(new_db[0].OidDb)

        new_db = [db for db in pyrapi.CeFindAllDatabases() if db.DbInfo.szDbaseName == RapiDatabaseTests._test_db_name]

        self.assertEqual(len(new_db),0,"db not removed from database list")

        
    def testCeFindAllDatabases(self):
        """CeFindAllDatabases"""

        dblist = pyrapi.CeFindAllDatabases()

        self.failUnless(len(dblist) > 0,
                        'found on databases')

    def testCeFindFirstDatabase(self):
        """CeFindFirstDatabase"""

        # Get list of all databases
        full_db_list = [ db.OidDb for db in pyrapi.CeFindAllDatabases() ]

        test_db_list = []
        handle = pyrapi.CeFindFirstDatabase()
        count = 0
        while 1:
            dboid = pyrapi.CeFindNextDatabase(handle)
            if dboid == 0:
                break

            self.failUnless(len(full_db_list) > count,
                            "CeFindNextDatabase should not find more than CeFindAllDatabases")    
            test_db_list.append(dboid)
            count = count + 1
                        

        self.assertEqual(len(full_db_list), count,
                         "CeFindNextDatabase should find the same number of database as CeFindAllDatabases")

        for test_db in test_db_list:
            matches = len( [db for db in full_db_list if db == test_db] )
            self.failIf(matches < 1, "CeFindNextDatabase should not find db that CeFindAllDatabases does not.")
            self.failIf(matches > 1, "CeFindNextDatabase should only find each db once.")
            

                
                
    def CeFindNextDatabase(self):
        """CeFindNextDatabase"""
        pass # done by testCeFindFirstDatabase
    
    def testCeOpenDatabase(self):
        """CeOpenDatabase"""

        dbh = 0
        
        dblist = pyrapi.CeFindAllDatabases()
        for db in dblist:
            if db.DbInfo.szDbaseName == "Contacts Database":
                dbh = pyrapi.CeOpenDatabase(db.OidDb,"")

        self.failIfEqual(dbh,0,"database handle should not be 0")

    def testCeReadRecordProps(self):
        """CeReadRecordProps"""

        dbh = 0
        
        dblist = pyrapi.CeFindAllDatabases()
        for db in dblist:
            if db.DbInfo.szDbaseName == "Contacts Database":
                dbh = pyrapi.CeOpenDatabase(db.OidDb,"")

        self.failIfEqual(dbh,0,"database handle should not be 0")

        record = pyrapi.CeReadRecordProps(dbh)    

    def testCeSeekDatabase(self):
        """CeSeekDatabase"""

        # Clean up from any failed run
        try:
            new_db = [db for db in pyrapi.CeFindAllDatabases() if db.DbInfo.szDbaseName == RapiDatabaseTests._test_db_name]
            pyrapi.CeDeleteDatabase(new_db[0].OidDb)
        except: pass

        # Create dummy database
        #sort = pyrapi.SORTORDERSPEC()                  # Sortspec not implemented yet
        #sort.propid = 1
        #sort.dwFlags = pyrapi.CEDB_SORT_DESCENDING
        #dboid = pyrapi.CeCreateDatabase(RapiDatabaseTests._test_db_name, 0, (sort,))
        dboid = pyrapi.CeCreateDatabase(RapiDatabaseTests._test_db_name, 0, ())
        dbh = pyrapi.CeOpenDatabase(dboid,"")

        # Insert some records
        # Write a new record to db
        field = pyrapi.CEPROPVAL()
        field.type     = pyrapi.CEVT_I2
        field.propid   = 1
        field.val.iVal = 100

        first_oid  = pyrapi.CeWriteRecordProps(dbh,0,(field,))
        field.val.iVal = 101
        second_oid = pyrapi.CeWriteRecordProps(dbh,0,(field,))
        field.val.iVal = 102
        third_oid  = pyrapi.CeWriteRecordProps(dbh,0,(field,))
        field.val.iVal = 103
        last_oid   = pyrapi.CeWriteRecordProps(dbh,0,(field,))

        # Seek to the records
        (seek_oid, index) = pyrapi.CeSeekDatabase(dbh, (pyrapi.CEDB_SEEK_CEOID, first_oid))
        self.assertEqual(seek_oid,first_oid, "CEDB_SEEK_CEOID should have found first_oid")

        # It should be possible to check the order of the returns but sortspec has not been
        # implemented now so we can only check that a valid record is returned
        (seek_oid, index) = pyrapi.CeSeekDatabase(dbh, (pyrapi.CEDB_SEEK_BEGINNING, 0))
        self.failIfEqual(seek_oid, 0, "CEDB_SEEK_BEGINNING 0 should have found first_oid")
        (seek_oid, index) = pyrapi.CeSeekDatabase(dbh, (pyrapi.CEDB_SEEK_BEGINNING, 1))
        self.failIfEqual(seek_oid, 0, "CEDB_SEEK_BEGINNING 1 should have found second")

        (seek_oid, index) = pyrapi.CeSeekDatabase(dbh, (pyrapi.CEDB_SEEK_END, 0))
        self.failIfEqual(seek_oid, 0, "CEDB_SEEK_END 0 should have found last")
        (seek_oid, index) = pyrapi.CeSeekDatabase(dbh, (pyrapi.CEDB_SEEK_END, -1))
        self.failIfEqual(seek_oid, 0, "CEDB_SEEK_END should have found third")

        # Not yet supported
        #(seek_oid, index) = pyrapi.CeSeekDatabase(dbh, (pyrapi.CEDB_SEEK_VALUEFIRSTEQUAL, field))
        #self.failUnless(seek_oid == rec_oid, "should have found equal record")

        # Delete dummy database
        pyrapi.CeCloseHandle(dbh)
        pyrapi.CeDeleteDatabase(dboid)

        
    def testCeWriteRecordPropsNoChanges(self):
        """CeWriteRecordProps"""

        dboid = 0
        
        dblist = pyrapi.CeFindAllDatabases()
        for db in dblist:
            if db.DbInfo.szDbaseName == "Contacts Database":
                dboid = db.OidDb
                
        dbh = pyrapi.CeOpenDatabase(dboid,"")
        self.failIfEqual(dbh,0,"database handle should not be 0")

        orig_record = pyrapi.CeReadRecordProps(dbh)

        ret = pyrapi.CeWriteRecordProps(dbh,orig_record[0],orig_record[1])

        self.assertEqual(ret,orig_record[0],"CeWriteRecordProps did not return the same record oid")

        pyrapi.CeCloseHandle(dbh)

        dbh = pyrapi.CeOpenDatabase(dboid,"")
        self.failIfEqual(dbh,0,"database handle should not be 0")

        new_record = pyrapi.CeReadRecordProps(dbh)

        for field in new_record[1]:
            orig_field = [old_field for old_field in orig_record[1] if old_field.propid == field.propid ][0]

            self.assertEqual(get_field_value(orig_field),get_field_value(field))
            
        pyrapi.CeCloseHandle(dbh)



    def testCeWriteRecordPropsWithChanges(self):
        """CeWriteRecordPropsWithChanges should be able to create new records and fields"""

        # Create new database for test
        dboid = pyrapi.CeCreateDatabase(RapiDatabaseTests._test_db_name, 0, ())
        dbh = pyrapi.CeOpenDatabase(dboid,"")
        
        # Write a new record to db
        field = pyrapi.CEPROPVAL()
        field.type     = pyrapi.CEVT_I2
        field.propid   = 1
        field.val.iVal = 100

        rec_oid = pyrapi.CeWriteRecordProps(dbh,0,(field,))
        
        # Check it worked
        pyrapi.CeCloseHandle(dbh)
        dbh = pyrapi.CeOpenDatabase(dboid,"")

        new_rec = pyrapi.CeReadRecordProps(dbh)
        self.assertEqual(rec_oid,new_rec[0],"wrong record oid returned")
        self.assertEqual(len(new_rec[1]),1,"record is of the wrong length")
        self.assertEqual(new_rec[1][0].val.iVal, 100, "record value is wrong")
        self.assertEqual(new_rec[1][0].type, pyrapi.CEVT_I2,"records type is wrong")
        self.assertEqual(new_rec[1][0].propid, 1,"records propid is wrong")

        # Delete db
        pyrapi.CeCloseHandle(dbh)
        pyrapi.CeDeleteDatabase(dboid)

        
if __name__ == "__main__":
    unittest.main()   
