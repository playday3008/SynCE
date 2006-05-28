<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="index.php">main page</a>.</p>

<h1>SynCE - Bluetooth</h1>

<p>Please note that this information is quite Linux-specific.</p>

<ol>

<li><p>Install <a href="http://bluez.sourceforge.net/">BlueZ</a>. The needed
modules are at least:</p>

<ul>
<li>bluez-libs</li>
<li>bluez-utils</li>
<li>bluez-sdp</li>
<li>bluez-pan</li>
</ul>

<p>Hint: You may have to create your /dev/rfcomm* devices manually like this:</p>

<blockquote><pre>
for i in `seq 0 255`; do
  mknod -m 666 /dev/rfcomm$i c 216 $i
done</pre></blockquote>

</li>

<li><p>Modify the device class in <tt>/etc/bluetooth/hcid.conf</tt>. Suitable
values:</p>
<ul>
<li>0xbe0104 - Workstation</li>
<li>0xbe0108 - Server</li>
<li>0xbe010c - Laptop</li>
</ul>

<p>See <a
href="https://www.bluetooth.org/foundry/assignnumb/document/baseband">https://www.bluetooth.org/foundry/assignnumb/document/baseband</a>
for more.

</li>

<li><p>Create a special PPP peer file called <tt>/etc/ppp/peers/dun</tt></p>

<pre>nodefaultroute
noauth
local
192.168.131.102:192.168.131.201
ms-dns 192.168.131.102
linkname synce-device</pre></li>

<li><p>Try the <tt>/usr/bin/bluepin</tt> program to see that it works. If not,
  replace it with this hack. Adjust PIN as needed.</p>

<pre>#!/bin/sh
echo "PIN:1234"</pre></li>


<li><p>Start <tt>dund</tt> so that it advertises an Activesync service :</p>

<p>If you have a device that is running Windows CE 2 or 3 then the following should work :</p>

<ol type="A">
  <li><pre>dund --listen --msdun call dun</pre></li>
  <li><pre>sdptool add SP</pre></li>
</ol>

<p>If you have a device that is running Windows Mobile then the following will work</p>

<ol type="A">
  <li><pre>dund --listen --activesync --msdun call dun</pre></li>
</ol>

<p><b>NOTE : </b> the <tt>--activesync</tt> flag is only available in bluez with version > 2.25</p>

<li><p>In the Bluetooth Manager on the PDA, go to the "Device Information" page
for the PC, check the "ActiveSync partner" checkbox. (If this checkbox is
disabled you forgot to modify <tt>/etc/bluetooth/hcid.conf</tt> or you need to use the other dund command above.)</li>

<li><p>Make sure you have started vdccm as your normal user (not root).</p></li>

<li>You should now be able to use the "Start ActiveSync" menu entry found when
you tap the Bluetooth Manager icon on the Today screen.</p></li>

<li><p>You can have something like this in <tt>/etc/rc.local</tt> or similar
file to prepare your computer for Bluetooth connection from boot. This is for
RedHat 9 and Fedora Core 1/2:</p>

<pre>/sbin/service bluetooth restart
/usr/bin/dund --listen --msdun call dun
/usr/bin/sdptool add SP
/usr/bin/sudo -u david /usr/bin/dccm</pre>

<p>(The Bluetooth service is restarted because that made everything work much
    better for me on my Thinkpad T30... don't know why.)</p></li>

<li>Please note that the <tt>synce-serial-config</tt> and
<tt>synce-serial-start</tt> scripts are <b>never</b> used with Bluetooth
connections!</li>

</ol>

<p>If this information is not enough, please see if <a
href="http://www.grinta.net/howto/bluez-ipaq.html">Daniele Nicolodi's HOWTO</a>
is helpful.</p>

<p>It is also possible to connect over Bluetooth Personal Area Network (PAN) by
following the instructions by Joe Ammond in the mail titled <a
href="http://sourceforge.net/mailarchive/message.php?msg_id=6157699">Bluetooth
serial vs. Bluetooth network ActiveSync</a>.</p>

<p>Return to <a href="index.php">main page</a>.</p>

</div>
<?php include 'footer.php'; ?>
