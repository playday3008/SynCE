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


class Air2OS(unittest.TestCase):

    def setUp(self):
        self.testdatadir = os.path.join(os.path.dirname(__file__),"testdata","air2os")
        libxml2.keepBlanksDefault(False)

    def _compare_files(self, suffix):
        """Compare reference to conversion output"""
        src_file = os.path.join(self.testdatadir,
                                "airsync_%s.xml" % suffix)
        if os.path.basename(src_file)[8:].startswith("contact_"):
            itemtype = constants.SYNC_ITEM_CONTACTS
        elif os.path.basename(src_file)[8:].startswith("event_"):
            itemtype = constants.SYNC_ITEM_CALENDAR
        elif os.path.basename(src_file)[8:].startswith("task_"):
            itemtype = constants.SYNC_ITEM_TASKS
        src = libxml2.parseFile(src_file)
        dst = formatapi.ConvertFormat(constants.DIR_FROM_AIRSYNC,
                                      itemtype, src, "OS20")
        # Load the reference file
        reference_file = os.path.join(self.testdatadir,
                                      "opensync_%s.xml" % suffix)
        reference = libxml2.parseFile(reference_file)
        assert dst.serialize("utf-8") == reference.serialize("utf-8")

    def test_contact_ex1(self):
        return self._compare_files("contact_ex1")

    def test_event_birthday(self):
        return self._compare_files("event_birthday")

    def test_event_ex1(self):
        return self._compare_files("event_ex1")

    def test_event_ex2(self):
        return self._compare_files("event_ex2")

    def test_event_ex3(self):
        return self._compare_files("event_ex3")

    def test_event_exclusion_ex1(self):
        return self._compare_files("event_exclusion_ex1")

    def test_event_recurrence_ex1(self):
        return self._compare_files("event_recurrence_ex1")

    def test_event_recurrence_ex2(self):
        return self._compare_files("event_recurrence_ex2")

    def test_event_recurrence_ex3(self):
        return self._compare_files("event_recurrence_ex3")

    def test_event_recurrence_ex4(self):
        return self._compare_files("event_recurrence_ex4")

    def test_event_recurrence_ex5(self):
        return self._compare_files("event_recurrence_ex5")

    def test_event_recurrence_ex6(self):
        return self._compare_files("event_recurrence_ex6")

    def test_event_recurrence_ex7(self):
        return self._compare_files("event_recurrence_ex7")

    def test_task_ex1(self):
        return self._compare_files("task_ex1")


if __name__ == '__main__':
    unittest.main()
