# -*- coding: utf-8 -*-
############################################################################
#    Copyright (C) 2006  Ole André Vadla Ravnås <oleavr@gmail.com>       #
#                                                                          #
#    This program is free software; you can redistribute it and#or modify  #
#    it under the terms of the GNU General Public License as published by  #
#    the Free Software Foundation; either version 2 of the License, or     #
#    (at your option) any later version.                                   #
#                                                                          #
#    This program is distributed in the hope that it will be useful,       #
#    but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#    GNU General Public License for more details.                          #
#                                                                          #
#    You should have received a copy of the GNU General Public License     #
#    along with this program; if not, write to the                         #
#    Free Software Foundation, Inc.,                                       #
#    59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             #
############################################################################

import random
import os

def generate_id():
    w1 = random.randint(0x0, 0x0FFF)
    w2 = random.randint(0x0, 0xFFFF)
    return (w1 << 16) | w2

def generate_guid():
    d1 = random.randint(0, 0xFFFFFFFF)
    d2 = random.randint(0, 0xFFFF)
    d3 = random.randint(0, 0xFFFF)
    d4 = []
    for i in range(8):
        d4.append(random.randint(0, 0xFF))

    guid = u"{%08X-%04X-%04X-" % (d1, d2, d3)
    for i in xrange(len(d4)):
        guid += u"%02X" % d4[i]
        if i == 1:
            guid += u"-"
    guid += u"}"

    return guid

def generate_opensync_guid():
    return "pas-id-%08X%08X" % (random.randint(0, 0xFFFFFFFF),
                                random.randint(0, 0xFFFFFFFF))

def hexdump(raw):
    buf = ""
    line = ""

    start = 0
    done = False
    while not done:
        buf += "%04x: " % start

        end = start + 16
        max = len(raw)
        if end > max:
            end = max
            done = True

        chunk = raw[start:end]
        for i in xrange(len(chunk)):
            if i > 0:
                spacing = " "
            else:
                spacing = ""
            buf += "%s%02x" % (spacing, ord(chunk[i]))

        if done:
            for i in xrange(16 - (end % 16)):
                buf += "   "

        buf += "  "

        for c in chunk:
            val = ord(c)
            if val >= 33 and val <= 126:
                buf += c
            else:
                buf += "."

        buf += "\n"

        start += 16

    return buf

def escape_str(s):
    ret = u""
    for c in s:
        if c == u"\r":
            continue
        elif c == u"\n":
            ret += u"\\n"
            continue
        elif c == u"\\":
            ret += u"\\\\"
            continue
        elif c in (u";", u","):
            ret += u"\\"
        ret += c
    return ret

def encode_wstr(s):
    return s.encode("utf_16_le") + "\x00\x00"

def decode_wstr(s):
    return s.decode("utf_16_le").rstrip("\0")

#
# 'deltree' - recursively delete a non-empty directory
# Why, oh why , is this never provided as an API by the OS?
#

def deltree(folder):

    # belts and braces..
    # We really don't want to risk nuking our root or home dirs.

    if folder == '/' or folder == "~" or folder==os.path.expanduser("~"):
        return

    # walk the path and trash the contents

    if os.path.isdir(folder):
        for path,dirs,files in os.walk(folder,topdown=False):
            for p in files:

                # Errors will most likely be permissions-related
                # so we just ignore them. Deal with files first

                try:
                    os.remove(os.path.join(path,p))
                except:
                    pass

            for p in dirs:

                # Same applies here for errors -  just ignore them,
                # the result will leave the dir in place.

                try:
                    os.rmdir(os.path.join(path,p))
                except:
                    pass

    # Dump the actual folder.

    try:
        os.rmdir(folder)
    except:
        pass
    
