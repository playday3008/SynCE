#!/usr/bin/python

import sys
import string
import os.path
from pyrapi import file

# Fetch the file from the PocketPC
infile = file.openCeFile(sys.argv[1],'r')
buf = infile.read()
infile.close()

# Make the file name local
outfilename = sys.argv[1]
outfilename = os.path.basename(string.translate(outfilename,string.maketrans('\\','/')))

outfile = open(outfilename,'w')

# Attempt to make a sensible unicode to ascii conversion and write the file
try:
    a = unicode(buf, 'unicodelittleunmarked')
    ascii =  a.encode('ascii', 'ignore')
    if len(ascii) > 0:
        outfile.write(ascii)
    else:
        outfile.write(buf)
except:
    print "Failed unicode conversion, writing raw output"
    outfile.write(buf)


