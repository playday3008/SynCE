""" rapi.py - A python module to put a friendly face on the pyrapi.so wrapper. """

import sys
import string
import os.path
import pyrapi

class RapiException(Exception):
    pass

def openCeFile(file_name,mode):
    """Open a file on the PocketPC. file_name must be passed in the form
    \\directory\\filename.ext so be careful to escape the slashes. The best
    approach is to use python raw strings e.g. (r'\my documents\test.txt')
    mode must be 'w' for writing to a file and 'r' for reading from a file.
    Returns a CeFile object. Raises an exception if the file could not be
    opened."""
    
    file_handle = pyrapi.CeCreateFile(file_name,mode)
    return CeFile(file_handle,mode)
    

class CeFile:
    """Simple class to wrap a PocketPC file object returned from the librapi2
    library. Use the openCeFile function to create a instance of this class."""

    READ_BUFFER_SIZE = 10000
    
    def __init__(self,file_handle,mode):
        self._file_handle = file_handle
        self._mode = mode
        self._read_buffer_size = CeFile.READ_BUFFER_SIZE

    def __del__(self):
        self.close()

    def _set_defaults(self):
        self._file_handle = -1
        self._mode = None
        
    def read(self):
        """Return the contents of the file as a string."""

        if self._mode != 'r':
            raise RapiException,"Attempt to read from a not in read mode"
    
        buf = ''
        while 1:
            res = pyrapi.CeReadFile(self._file_handle,self._read_buffer_size)
            if res == None: break 
            
            buf = buf + res[0]

        return buf
    
    def write(self,buffer):
        """Write the contents of buffer to the file."""
        if self._mode != 'w':
            raise RapiException,"Attempt to write to file not in write mode"
        
        return pyrapi.CeWriteFile(self._file_handle,buffer)
        
    def close(self):
        """Close the open file handle."""
        pyrapi.CeCloseHandle(self._file_handle)
        self._set_defaults()

    
