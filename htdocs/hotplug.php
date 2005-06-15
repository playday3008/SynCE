<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="index.php">main page</a>.</p>

<h1>SynCE - Linux hotplug script for USB connections</h1>

<p><b>2005-06-14</b> Yaron Tausky provides <a
href="http://sourceforge.net/mailarchive/forum.php?thread_id=7513803&forum_id=1226">A
better way to handle iPAQ auto-connection</a>.</p>

<p><b>2005-05-19</b> Ed Catmur suggests some <a
href="http://sourceforge.net/tracker/?func=detail&atid=399601&aid=1204938&group_id=30550">improvements
to the hotplug script below</a>.</p>

<p><b>2004-10-19</b> If you have the <b>devfs</b> daemon (for example on
Gentoo), you can follow Stanislaw Pusep's <a
href="https://sourceforge.net/forum/message.php?msg_id=2813010">tips for using
"devfsd" instead of "hotplug"</a>.</p>

<p>Instructions originally provided by Fredrik Nilsson.</p>

<ol>

<li class=SPACED>First make sure that your connection works with manual operation using
<tt>synce-serial-start</tt>.</li>

<li>Make sure that <a href="http://linux-hotplug.sourceforge.net/">Linux
Hotplug</a> is installed on your system. (For RedHat Linux, this is an RPM
called <tt>hotplug</tt>.)</li>

<li><p>Create the directory <tt>/var/run/usb</tt> if it doesn't already exist.</p></li>

<li>

<p>If you are using a Linux Hotplug release prior to the August 8, 2002 release,
add a line like below to your <tt>/etc/hotplug/usb.usermap</tt> file. (This
applies to for example RedHat 9 and earlier.)</p>

<p>If you are using Linux Hotplug released August 8, 2002 or later, create the
file <tt>/etc/hotplug/synce.usermap</tt> with the contents below. (Thanks to
Andrew Radke and Mark "Delirium" for this info.)</p>

<pre>synce   0x0003  0x<span class="RED">VVVV</span>  0x<span class="RED">PPPP</span>  0x00    0x00    0x00    0x00    0x00    0x00    0x00    0x00    0x00</pre>

<p>Replace VVVV with the USB vendor ID for your device and PPPP with the USB
product ID for your device. IDs for common devices:</p>

<table width="400">

<tr><th>Device</th><th>Vendor ID</th><th>Product ID</th></tr>

<tr><td>Casio EM500</td>   <td>07cf</td><td>2002</td></tr>
<tr><td>Compaq iPAQ</td>   <td>049f</td><td>0003</td></tr>
<tr><td>Dell Axim X5</td>  <td>413c</td><td>4001</td></tr>
<tr><td>HP Jornada 548</td><td>03f0</td><td>1016</td></tr>
<tr><td>HP Jornada 568</td><td>03f0</td><td>1116</td></tr>

</table>

<p>For devices missing in the list above, <a href="usbids.php">find the Vendor
and Product IDs</a> for your device.</p>

</li>

<li><p>Create the file <tt>/etc/hotplug/usb/synce</tt> with the following
contents (<a href="hotplug.txt">download</a>) and make it executable by root: </p>

<pre>#!/bin/bash
 
export time=`date +"%b %d %X"`
export uname=`uname -n`
 
echo "$time $uname $0: iPAQ added" &gt;&gt; /var/log/synce
synce-serial-abort &gt;&gt; /dev/null
synce-serial-start &gt;&gt; /var/log/synce
 
:&gt; $REMOVER
echo "export time=\`date +\"%b %d %X\"\`" &gt;&gt; $REMOVER
echo "export uname=\`uname -n\`" &gt;&gt; $REMOVER
echo "echo \"\$time \$uname $0: iPAQ removed\" &gt;&gt; /var/log/synce" &gt;&gt; $REMOVER
echo "sleep 15 &gt;&gt; /var/log/synce" &gt;&gt; $REMOVER
echo "synce-serial-abort &gt;&gt; /var/log/synce" &gt;&gt; $REMOVER
chmod +x $REMOVER</pre>

</li>

<li>When you have the hotplug script installed you should <b>not</b> run
<tt>synce-serial-start</tt> yourself.</li>

</ol>

<p>Return to <a href="index.php">main page</a>.</p>

</div>
<?php include 'footer.php'; ?>
