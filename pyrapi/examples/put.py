#!/usr/bin/python

import sys
from pyrapi import file

infile = open(sys.argv[1],'r')
outfile = file.openCeFile(sys.argv[2],'w')
outfile.write(infile.read())
outfile.close()


