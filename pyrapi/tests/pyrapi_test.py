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
        
    def CeDeleteFile(self):
        """CeDeleteFile"""
        
    def CeFindAllFiles(self):
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
        
    def estCeGetFileAttributes(self):
        """CeGetFileAttributes"""

        attr = pyrapi.CeGetFileAttributes(RapiFileTests._test_directory)

        print "attr = ", attr, " attr & pyrapi.FILE_ATTRIBUTE_DIRECTORY = ", ( attr & pyrapi.FILE_ATTRIBUTE_DIRECTORY )

        self.failUnless(attr & pyrapi.FILE_ATTRIBUTE_DIRECTORY, "attribute should be a directory")
        
    def CeGetSpecialFolderPath(self):
        """CeGetSpecialFolderPath"""
        
    def CeMoveFile(self):
        """CeMoveFile"""
        
    def CeRemoveDirectory(self):
        """CeRemoveDirectory"""
        
    def CeSetFileAttributes(self):
        """CeSetFileAttributes"""
    


class RapiDatabaseTests(unittest.TestCase):

    def setUp(self):
        #Clean up any previous test run.
        try:
            new_db = [db for db in pyrapi.CeFindAllDatabases() if db.DbInfo.szDbaseName == "pyrapi_test_db"]
            pyrapi.CeDeleteDatabase(new_db[0].OidDb)
        except: pass
        

    def testCeCreateDatabase(self):
        """CeCreateDatabase"""

        dboid = pyrapi.CeCreateDatabase("pyrapi_test_db")

        new_db = [db for db in pyrapi.CeFindAllDatabases() if db.DbInfo.szDbaseName == "pyrapi_test_db"]

        self.failIfEqual(len(new_db),0,"new db not in database list")

        pyrapi.CeDeleteDatabase(new_db[0].OidDb)
        
                                
    def CeDeleteDatabase(self):
        """CeDeleteDatabase"""
        pass # same as testCeCreateDatabase
        
    def testCeFindAllDatabases(self):
        """CeFindAllDatabases"""

        dblist = pyrapi.CeFindAllDatabases()

        self.failUnless(len(dblist) > 0,
                        'found on databases')

    def CeFindFirstDatabase(self):
        """CeFindFirstDatabase"""
        
    def CeFindNextDatabase(self):
        """CeFindNextDatabase"""
        
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

    def CeSeekDatabase(self):
        """CeSeekDatabase"""
        
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
        dboid = pyrapi.CeCreateDatabase("pyrapi_test_db")
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
