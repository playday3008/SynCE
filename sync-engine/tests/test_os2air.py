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
        if dst.serialize("utf-8") != reference.serialize("utf-8"):
            print src_file
            print "result:",dst.serialize("utf-8")
            print "reference:",reference.serialize("utf-8")
        assert dst.serialize("utf-8") == reference.serialize("utf-8")

    def test_contact_ex2(self):
        return self._compare_files("contact_ex2")

    def test_contact_fileas1(self):
        return self._compare_files("contact_fileas1")

    def test_contact_fileas2(self):
        return self._compare_files("contact_fileas2")

    def test_contact_fileas3(self):
        return self._compare_files("contact_fileas3")

    def test_contact_nick1(self):
        return self._compare_files("contact_nick1")

    def test_contact_photo1(self):
        return self._compare_files("contact_photo1")

    def test_contact_addr1(self):
        return self._compare_files("contact_addr1")

    def test_contact_cat1(self):
        return self._compare_files("contact_cat1")

    def test_contact_assist1(self):
        return self._compare_files("contact_assist1")

    def test_contact_email1(self):
        return self._compare_files("contact_email1")

    def test_contact_im1(self):
        return self._compare_files("contact_im1")

    def test_contact_manager1(self):
        return self._compare_files("contact_manager1")

    def test_contact_org1(self):
        return self._compare_files("contact_org1")

    def test_contact_spouse1(self):
        return self._compare_files("contact_spouse1")

    def test_contact_tel1(self):
        return self._compare_files("contact_tel1")

    def test_contact_title1(self):
        return self._compare_files("contact_title1")

    def test_contact_url1(self):
        return self._compare_files("contact_url1")

    def test_contact_ann1(self):
        return self._compare_files("contact_ann1")

    def test_contact_bday1(self):
        return self._compare_files("contact_bday1")

    def test_contact_note1(self):
        return self._compare_files("contact_note1")

    def test_contact_case1(self):
        return self._compare_files("contact_case1")

    def test_contact_notype(self):
        return self._compare_files("contact_notype")

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

    def test_task_ex1(self):
        return self._compare_files("task_ex1")

    def test_task_ex2(self):
        return self._compare_files("task_ex2")

    def test_task_ex3(self):
        return self._compare_files("task_ex3")

    def test_task_ex4(self):
        return self._compare_files("task_ex4")

    def test_task_desc1(self):
        return self._compare_files("task_desc1")


if __name__ == '__main__':
    unittest.main()
