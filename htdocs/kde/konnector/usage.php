<?php include 'header.php'; ?>

<a href="./index.php">back to main page</a>
<br>
<h3><a name="Contents" id="Contents"></a>Contents</h3>
<ul>
    <li><a href="#Usage">Usage of the test application</a></li>
    <li><a href="#Basics">Basic functionality of this software</a></li>
    <li><a href="#GetTypes">Get Types</a></li>
    <li><a href="#GetPush">Getting and Pushing Data</a></li>
    <li><a href="#SyncData">Syncing Data</a></li>    
    <li><a href="#Backup">Backup</a></li>    
    <li><a href="#Preferences">Preferences</a></li>    
</ul>
<br>
<h4><a name="Usage" id="Usage">Usage of the test application</a></h4>
This part describes the functionality of <code>pocketpccommunication</code>.
This program enables you to get/put data from/to the device and it has a
usefull backup/restore mechanism.
<h5><a name="Basics" id="Basics">Basic functionality of this software</a></h5>
<p>
This package is a testing software. It has been developed to learn about
the synCE-libraries. Nevertheless it has some handy functions.
Maybe the most important one is the possibility to backup and restore data
from/to the device. The data is stored in ~/.kde/share/apps/pocketpccommunication.
Every dataset has the name of the partnership in the file name. So using
multiple devices should be no problem (but as I only have one device this has
not been tested!) 
<br>
PLEASE NOTE:<br>
As this is testing software it is possible that the connection to the device
breaks! If this happens please restart the connection to the device and the
application. I also encountered this with the rra-get-types utility in
conjunction with a Fujitsu Siemens LOOX with USB connection. 
</p>
<h5><a name="GetTypes" id="GetTypes">Get Types</a></h5>
<p>
This returns all the types which are available on this device. The returned
listview is clickable to get the id's and state of all the entries which are
stored within this type. 
</p>
<h5><a name="GetPush" id="GetPush">Getting and Pushing Data</a></h5>
<p>
You can separately get and push Addressbook- and Calendar-data. Additionally
it is possible to deal with events and todos. The data is stored in the
files which can be specified within the preferences.
</p>
<h5><a name="SyncData" id="SyncData">Syncing Data</a></h5>
<p>
As you will see the menu entries for syncing data are disabled: 
These were just tests with a rather old kitchensync version. Just ignore them.
Also the konnector is disabled, because testing within kitchensync is 
much better implemented than here.
</p>
<h5><a name="Backup" id="Backup">Backup and Restore</a></h5>
<p>
Maybe the most useful functions in the application. You can make a full backup
of your data to files in ~/.kde/share/apps/pocketpccommunication. The files have
unique names based on the partnership-id. So although it is not tested, because I only have one
device, multiple devices should work. 
<br>
<code>DELETE EVERYTHING</code> does what the name tells you. The complete AddressBook and
Calendar will be cleared on the device. <em>Please use with care as bugs may still reside!</em> 
Before calling this function you should backup the data and check the backup files! This function is usefull
when you want to restore the backuped data. If you do a restore all entries will have
new id's on the device! So it is necessary to reset the stored meta data when syncing.
</p>
<h5><a name="Preferences" id="Preferences">Preferences</a></h5>
<p>
Please let me know if some of these entries are not clear. 
Currently only the first tab seems to be useful :)
<br>
<strong>Attention:</strong> Think twice before using the standard Addressbook!!
If you use it do not forget to make a backup first!! It is called <code>std.vcf</code>.
On my system it can be found here: <code>&#126;/.kde/share/apps/kabc/std.vcf</code>
</p>

<?php include 'footer.php'; ?>