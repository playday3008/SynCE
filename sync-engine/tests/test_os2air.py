#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import logging
import os
import unittest
sys.path.insert(0, "..")

#from xml.dom import minidom
import libxml2

from SyncEngine import formats
from SyncEngine import constants
from SyncEngine import formatapi


logging.basicConfig(level=logging.DEBUG,
                    stream=sys.stdout,
                    format='%(asctime)s %(levelname)s %(name)s : %(message)s')


class OS2Air(unittest.TestCase):

    def setUp(self):
        self.testdatadir = os.path.join(os.path.dirname(__file__),"testdata","os2air")
        libxml2.keepBlanksDefault(False)

    def _compare_files(self, suffix):
        """Compare reference to conversion output"""
        src_file = os.path.join(self.testdatadir,
                                "opensync_%s.xml" % suffix)
        if os.path.basename(src_file)[9:].startswith("contact_"):
            itemtype = constants.SYNC_ITEM_CONTACTS
        elif os.path.basename(src_file)[9:].startswith("event_"):
            itemtype = constants.SYNC_ITEM_CALENDAR
        elif os.path.basename(src_file)[9:].startswith("task_"):
            itemtype = constants.SYNC_ITEM_TASKS
        src = libxml2.parseFile(src_file)
        dst = formatapi.ConvertFormat(constants.DIR_TO_AIRSYNC,
                                      itemtype, src, "OS20")
        # Load the reference file
        reference_file = os.path.join(self.testdatadir,
                                      "airsync_%s.xml" % suffix)
        reference = libxml2.parseFile(reference_file)
        #print src_file
        #print "dest:",dst.serialize("utf-8")
        #print "refe:",reference.serialize("utf-8")
        assert dst.serialize("utf-8") == reference.serialize("utf-8")

    def test_contact_ex2(self):
        return self._compare_files("contact_ex2")

    def test_event_ex4(self):
        return self._compare_files("event_ex4")

    def test_event_ex5(self):
        return self._compare_files("event_ex5")

    def test_event_ex6(self):
        return self._compare_files("event_ex6")

    def test_event_ex7(self):
        return self._compare_files("event_ex7")

    def test_event_transp1(self):
        return self._compare_files("event_transp1")


if __name__ == '__main__':
    unittest.main()
