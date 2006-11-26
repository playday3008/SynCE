#!/bin/sh

HAL_PREFIX=/usr

set -x
install -o root -g root -m 644 data/hal/20-usb-pda.fdi ${HAL_PREFIX}/share/hal/fdi/information/20thirdparty/
install -o root -g root -m 755 data/hal/hal-usb-rndis-ng.sh ${HAL_PREFIX}/lib/hal/

