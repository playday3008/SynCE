<?php include 'header.php'; ?>

<div style="text-align:center"><h3>Syncing WinCE 3.0 Devices with kitchensync</h3></div>
<p>
This page presents you information about the software necessary to sync AddressBook- and Calendar-data
from/to a Windows CE 3.0 driven device. 
</p>
<h3><a name="Contents" id="Contents"></a>Contents</h3>
<ul>
    <li><a href="#Introduction">Introduction</a></li>
    <li><a href="#Requirements">Requirements</a></li>
    <li><a href="#Download">Download</a></li>
    <li><a href="#Installation">Installation</a></li>
    <li><a href="#Developer">Developer Information</a></li>
    <li><a href="./usage.php">Usage of the test application</a></li>
    <li><a href="./kitchensyncusage.php">Usage of the konnector within kitchensync</a></li>
    <li><a href="#Contact">Contact and Help</a></li>
    <li><a href="./news.php">News</a></li>
    <li><a href="./todos.php">Todos</a></li>
    <li><a href="./faq.php">Frequently Asked Questions</a></li>
    <li><a href="../index.php">back to SynCE-KDE</a></li>
</ul>
<h4><a name="Introduction" id="Introduction">Introduction</a></h4>
<p>
This text should give you the needed information to get synchronization running with kitchensync and a
Windows CE 3.0 device. A pda in this context means a Windows CE 3.0 device.<br>
<strong>Attention:</strong> make backups of your data as the software may still have bugs!! Backup and restore possibilities
are described <a href="./usage.php#Backup">here</a>.
</p>
<h4><a name="Requirements" id="Requirements">Requirements</a></h4>
This software is in development on Linux and till now it does not know that other systems exist.
<ul>
    <li><a href="http://www.kde.org">KDE</a> 3.3.0 (3.3.1 not tested!)</li>
    <li><a href="http://pim.kde.org">KDE-PIM</a> 3.3</li>
</ul>
Additionally you need several external libraries to get this software compiling and linking.
<br><br>
<em>SynCE libraries</em>
<br>
The most important ones can be found at <a href="http://synce.sourceforge.net">synce.sourceforge.net</a>.
These libraries are crucial for communication with the pda. If the test programs there do not work, trying to
sync will fail too. Please install these libraries and test them (eg with the test programs in synce-rra).
As this is software in development you also need the synce-kde version of <code>dccm</code> 
called <code>vdccm</code>. This is available <a href="http://synce.sourceforge.net/synce/kde/">here</a>.
<br><br>
<em>kitchensync</em>
<br>
For synchronization you need kitchensync. As the version in KDE-PIM 3.3.0 is not very useful please get
<a href="http://handhelds.org/~zecke/">kitchensync-0.0.8</a>. As there are some problems with the installed
include-files please download and apply these <a href="./downloads/include_patch.tgz">patches</a>.
<br>
applying the patch:<br><br>
<div style="margin-left:1cm">
    <code>
    %cd kitchensync/libkonnector2<br>
    %patch &lt; konnector_patch.txt --strip=1<br>
    %cd ../libksync<br>
    %patch &lt; ksync_patch --strip=1<br>
    </code>
</div>
<br>
<strong>Attention:</strong> For synchronization I always used the <code>LocalKonnector</code> in conjunction with 
<code>PocketPCKonnector</code>! I did not try the AddressBookKonnector and CalendarKonnector!
<br>
For applying just unpack the archive and copy konnector_patch.txt to kitchensync/libkonnector2 
and ksync_patch.txt to kitchensync/libksync2.
<br>
For installation of kitchensync please refer to <a href="http://handhelds.org/~zecke/">the official documentation</a>.
<br><br>
<strong>Hint:</strong> use <code>./configure --prefix=/path/to/kde3</code> to ensure correct compiling, 
linking and installation.
<h4><a name="Download" id="Download">Download</a></h4>
<p>
You need the following archives to start working.
<br><br>
<em>pocketpccommunication</em>
<br>
This is the base software. It gives you a C++ library for getting/pushing AddressBook- and Calendar-data. Along
with it comes a KDE-application which can be used for testing and backup/restore purposes!
<br>
Get the <a href="./downloads/pocketpccommunication-0.1.tar.gz">archive</a>.
<br><br>
<em>libpocketpckonnector</em>
<br>
This is the konnector which can be used within kitchensync. After successfull installation start kitchensync
and you will see a new entry when adding new konnectors!
<br>
Get the <a href="./downloads/libpocketpckonnector-0.1.tar.gz">archive</a>.
<br><br>
<em>kitchensync include-file patches</em>
<br>
You need a working development installation of <code>kitchensync</code>. Please apply these patches:
<br>
Get the <a href="./downloads/include_patch.tgz">patches</a>.
</p>
<h4><a name="Installation" id="Installation">Installation</a></h4>
After downloading the complete software the installation should be straight forward. 
At this point all the external libraries and applications have to be installed. Otherwise compilation, linking
and/or running will fail. As <code>libpocketpckonnector</code> depends on a library within 
<code>pocketpcommunication</code> it is necessary to install <code>pocketpccommunication</code> first.
<br>
As these are just snapshots of my local cvs please do the following:
<br><br>
<div style="margin-left:1cm">
    <code>
    %cd install-dir<br>
    %make -f Makefile.cvs<br>
    %./configure --prefix=/path/to/kde<br>
    %make<br>
    %sudo make install
    </code>
</div>
<h4><a name="Developer" id="Developer">Developer Information</a></h4>
Many of the distributed classes have doxygen comments in the header file. The may not be complete but a start.
<h4><a name="Contact" id="Contact">Contact and Help</a></h4>
You have problems? The software does everything but not what it was designed to? 
<br>
Do not hesitate to ask for assistance at the following mailing lists:
<ul>
    <li><a href="mailto:synce-users@lists.sourceforge.net">synce-users@lists.sourceforge.net</a></li>
    <li><a href="mailto:synce-devel@lists.sourceforge.net">synce-devel@lists.sourceforge.net</a></li>
</ul>
or send a mail directly to <a href="mailto:cfremgen@users.sourceforge.net">me</a>.

<?php include 'footer.php'; ?>