#!/usr/bin/python

import sys
import rapi

infile = open(sys.argv[1],'r')
outfile = rapi.openCeFile(sys.argv[2],'w')
outfile.write(infile.read())
outfile.close()


