#!/bin/bash

./clean_partnerships.py
#./create_partnership.py "Linux desktop" Contacts,Calendar
#./create_partnership.py "Linux desktop" Calendar
./create_partnership.py "LinuxDesktop" "Contacts,Calendar,Tasks"
msynctool --delgroup treo-kde
msynctool --addgroup treo-kde
msynctool --addmember treo-kde kdepim-sync
msynctool --addmember treo-kde synce-plugin
msynctool --configure treo-kde 2
