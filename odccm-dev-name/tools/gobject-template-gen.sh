#!/usr/bin/env python

import sys
import tempfile
import os

def gen_names(name):
    camel=name
    dashed=""
    lower=""
    upper_pfx=""
    upper_sfx=""

    count=0
    for c in name:
        if c.isupper():
            count += 1

            if count > 1:
                dashed += "-"
                lower += "_"

            if count > 2:
                upper_sfx += "_"

        dashed += c.lower()
        lower += c.lower()
        if count <= 1:
            upper_pfx += c.upper()
        else:
            upper_sfx += c.upper()

    return (camel, dashed, lower, upper_pfx, upper_sfx)


if len(sys.argv) != 2:
    print "usage: %s <typename>"
    sys.exit(1)

f, fname = tempfile.mkstemp()
f = os.fdopen(f, "w+b")

camel, dashed, lower, upper_pfx, upper_sfx = gen_names(sys.argv[1])

out_c = "%s.c" % dashed
out_h = "%s.h" % dashed

print >> f, "set -x"

print >> f, "cp %s %s" % ("gobject-template.c", out_c)
print >> f, "cp %s %s" % ("gobject-template.h", out_h)

templ = "sed -i \"s/%s/%s/g\" %s"

substs = (
    ("projname-objname", dashed),
    ("ProjnameObjname", camel),
    ("PROJNAME_TYPE_OBJNAME", "%s_TYPE_%s" % (upper_pfx, upper_sfx)),
    ("PROJNAME_IS_OBJNAME", "%s_IS_%s" % (upper_pfx, upper_sfx)),
    ("PROJNAME_OBJNAME", "%s_%s" % (upper_pfx, upper_sfx)),
    ("projname_objname", lower),
)

for subst in substs:
    print >> f, templ % (subst + (out_c,))
    print >> f, templ % (subst + (out_h,))

f.close()
os.chmod(fname, 0755)
os.system(fname)
os.unlink(fname)

