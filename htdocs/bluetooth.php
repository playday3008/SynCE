<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="index.php">main page</a>.</p>

<h1>SynCE - Bluetooth</h1>

<ol>

<li><p>Install <a href="http://bluez.sourceforge.net/">BlueZ</a>. The needed
modules are:</p>

<ul>
<li>bluez-libs</li>
<li>bluez-utils</li>
<li>bluez-pan</li>
</ul>
</li>

<li><p>Modify the device class in <tt>/etc/bluetooth/hcid.conf</tt>. Suitable
values:</p>
<ul>
<li>0x104 - Workstation</li>
<li>0x108 - Server</li>
<li>0x10c - Laptop</li>
</ul>

<p>See <a
href="http://www.bluetooth.org/assigned-numbers/baseband.htm">http://www.bluetooth.org/assigned-numbers/baseband.htm</a>
for more.

</li>

<li><p>Create a special PPP peer file called <tt>/etc/ppp/peers/dun</tt></p>

<pre>nodefaultroute
noauthlocal
192.168.131.102:192.168.131.201
ms-dns 192.168.0.1
linkname synce-device</pre></li>

<li><p>Try the <tt>/usr/bin/bluepin</tt> program to see that it works. If not,
  replace it with this hack. Adjust PIN as needed.</p>

<pre>#!/bin/sh
echo "PIN:1234"</pre></li>


<li><p>Start <tt>dund</tt>:</p>

<pre>dund --listen --msdun call dun</pre></li>

<li><p>Advertise the Serial Profile too:</p>

<pre>sdptool add SP</pre></li>

<li><p>In the Bluetooth Manager on the PDA, go to the "Device Information" page
for the PC, check the "ActiveSync partner" checkbox. (If this checkbox is
disabled you forgot to modify <tt>/etc/bluetooth/hcid.conf</tt>.)</li>

<li>You should now be able to use the "Start ActiveSync" menu entry found when
you tap the Bluetooth Manager icon on the Today screen.</p></li>


</ol>


<p>Return to <a href="index.php">main page</a>.</p>

</div>
<?php include 'footer.php'; ?>
