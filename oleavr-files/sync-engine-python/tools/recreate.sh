#!/bin/bash

./test_sync clean
#./create_partnership "Linux desktop" Contacts,Calendar
#./create_partnership "Linux desktop" Calendar
./create_partnership "Linux desktop" Contacts
msynctool --delgroup evo2-sync
msynctool --addgroup evo2-sync
msynctool --addmember evo2-sync evo2-sync
msynctool --addmember evo2-sync synce-plugin
msynctool --configure evo2-sync 1

