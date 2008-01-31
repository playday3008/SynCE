#!/usr/bin/env python

import sys
sys.path.insert(0, "..")
from SyncEngine import wbxml
import libxml2


if len(sys.argv) != 3:
    print "Invalid argument count"
    sys.exit(1)

out =open(sys.argv[2],"w")

doc = libxml2.parseFile(sys.argv[1])
wbxml=wbxml.XMLToWBXML(doc)
out.write(wbxml)
out.close()





