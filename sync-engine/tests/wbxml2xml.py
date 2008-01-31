#!/usr/bin/env python

import sys
sys.path.insert(0, "..")
from SyncEngine import wbxml
import libxml2


if len(sys.argv) != 3:
    print "Invalid argument count"
    sys.exit(1)

infile = open(sys.argv[1],"r")
wb = infile.read()
out =open(sys.argv[2],"wb")
doc=wbxml.WBXMLToXML(wb)
out.write(doc.serialize("UTF-8",0))
out.close()

