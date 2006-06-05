#!/usr/bin/env python

from pyrapi2 import *
from xml.dom import minidom

def get_node_on_level(parent, level):
    while level > 0:
        for el in parent.childNodes:
            if el.nodeType == el.ELEMENT_NODE:
                parent = el
                level -= 1
                if level > 0:
                    break
                else:
                    return el

class Characteristic:
    def __init__(self, type, parent=None):
        self.type = type
        self.params = {}
        self.parent = None
        self.children = {}

    def __getitem__(self, key):
        return self.params[key]

    def __setitem__(self, key, value):
        self.params[key] = value

    def __str__(self):
        return self.to_string()

    def to_string(self, level=0):
        indent = "%*s" % (2 * level, "")
        str = "%s%s\n" % (indent, self.type)
        if self.params:
            for name, value in self.params.items():
                str += "%s  %s = %s\n" % (indent, name, value)
        else:
            str += "%s  [no params]\n" % indent

        for child in self.children.values():
            str += "\n"
            str += child.to_string(level + 1)

        return str

def characteristic_tree_from_xml(xml_node):
    return parse_characteristic_node(xml_node)

def parse_characteristic_node(xml_node, parent=None):
    ctic = Characteristic(xml_node.getAttribute("type"), parent)
    for node in xml_node.childNodes:
        if node.nodeType == node.ELEMENT_NODE:
            if node.localName == "characteristic":
                ctic.children[node.getAttribute("type")] = parse_characteristic_node(node, ctic)
            elif node.localName == "parm":
                ctic[node.getAttribute("name")] = node.getAttribute("value")
            else:
                print "unhandled node '%s'" % node.localName

    return ctic

def do_query(session, path, leaf, recursive=False):
    doc = minidom.Document()
    doc_node = doc.createElement("wap-provisioningdoc")
    doc.appendChild(doc_node)

    tokens = path.split(".")

    parent = doc_node
    for token in tokens:
        node = doc.createElement("characteristic")
        node.setAttribute("type", token)
        parent.appendChild(node)
        parent = node

    node = doc.createElement("characteristic-query")
    node.setAttribute("type", leaf)
    if not recursive:
        node.setAttribute("recursive", "false")
    parent.appendChild(node)

    reply = s.process_config(doc_node.toxml(), 1)
    reply_doc = minidom.parseString(reply)
    reply_node = get_node_on_level(reply_doc, 2 + len(tokens))
    return characteristic_tree_from_xml(reply_node)

def get_partnerships(session):
    partnerships = [ None, None ]

    # Enumerate partnerships
    hklm = session.HKEY_LOCAL_MACHINE
    partners_key = hklm.create_sub_key(
            r"Software\Microsoft\Windows CE Services\Partners")
    for i in xrange(1, 3):
        key = partners_key.create_sub_key("P%d" % i)
        if key.disposition == REG_OPENED_EXISTING_KEY:
            try:
                id = key.query_value("PId")
                name = key.query_value("PName")

                if id != 0:
                    partnerships[i - 1] = { "id" : id, "name" : name }
            except RAPIError:
                pass
        key.close()
    partners_key.close()

    # Look up the synchronization data on each
    for ctic in do_query(s, "Sync", "Sources").children.values():
        sub_ctic = do_query(s, "Sync.Sources", ctic.type, recursive=False)

        sync_guid = sub_ctic.type
        sync_server = sub_ctic["Server"]
        sync_name = sub_ctic["Name"]

        p = None
        for p in partnerships:
            if p is not None:
                if p["name"] == sync_server:
                    break

        if p is None:
            print "dangling sync relationship: [\"%s\", \"%s\", \"%s\"]" % \
                (sync_guid, sync_server, sync_name)
        else:
            p["srcid"] = sync_guid
            p["description"] = sync_name

    return partnerships


s = RAPISession(SYNCE_LOG_LEVEL_DEFAULT)

pships = get_partnerships(s)

for i in xrange(len(pships)):
    pship = pships[i]
    print "P%d:" % i
    if pship is not None:
        keys = pship.keys()
        keys.sort()
        for key in keys:
            print "  %s: %s" % (key, pship[key])
    else:
        print "  [empty]"

