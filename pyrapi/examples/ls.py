#!/usr/bin/python

import sys,time
import getopt
from pyrapi import pyrapi

opts, args = getopt.getopt(sys.argv[1:], 'r')
print opts,args

recurse = 0

for o, a in opts:
    if o in ("-r", "--recurse"):
        recurse = 1

path = args[0]


def rListDir(path):
    file_list = []
    for fileobj in  pyrapi.CeFindAllFiles(path+r"\*.*",pyrapi.FAF_NAME|pyrapi.FAF_ATTRIBUTES|pyrapi.FAF_CREATION_TIME|pyrapi.FAF_LASTACCESS_TIME|pyrapi.FAF_LASTWRITE_TIME|pyrapi.FAF_SIZE_LOW):
        file_list.append( "%s\\%s ->\t %d %s %s %s %d" % (path,
                                                          fileobj.cFileName,
                                                          fileobj.nFileSizeLow,
                                                          time.ctime(fileobj.ftCreationTime),
                                                          time.ctime(fileobj.ftLastAccessTime),
                                                          time.ctime(fileobj.ftLastWriteTime),
                                                          fileobj.dwFileAttributes))
        if fileobj.dwFileAttributes == 16 and recurse == 1:
            new_list = rListDir(path+'\\'+fileobj.cFileName)
            if (new_list != None):
                file_list = file_list + new_list
    return file_list



for f in rListDir(path):
    print f

