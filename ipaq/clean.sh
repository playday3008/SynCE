#!/bin/sh

find /lib/modules/$(uname -r) -name "ipaq.ko" -exec rm -v {} \;
depmod -ae
./reload.sh --unload-only 2> /dev/null

echo "Done! :)"
