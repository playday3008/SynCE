#!/usr/bin/python

import sys,time
import getopt
import pyrapi

opts, args = getopt.getopt(sys.argv[1:], 'r')
print opts,args

recurse = 0

for o, a in opts:
    if o in ("-r", "--recurse"):
        recurse = 1

path = args[0]


def rListDir(path):
    file_list = []
    for fileobj in  pyrapi.CeFindAllFiles(path+r"\*.*"):
        file_list.append( "%s\\%s ->\t %s %s %s %s %s" % (path,
                                                          str(fileobj[0]),
                                                          str(fileobj[1]),
                                                          time.ctime(fileobj[2]),
                                                          time.ctime(fileobj[3]),
                                                          time.ctime(fileobj[4]),
                                                          str(fileobj[5])))
        if fileobj[1] == 16 and recurse == 1:
            new_list = rListDir(path+'\\'+fileobj[0])
            if (new_list != None):
                file_list = file_list + new_list
    return file_list



for f in rListDir(path):
    print f

