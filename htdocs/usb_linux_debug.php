<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="usb_linux.php">USB on Linux page</a>.</p>

<h1>SynCE - USB connection - Linux - Debug</h1>

<p>Follow these instructions to debug the <tt>ipaq</tt> kernel driver:</p>

<ol>

<li><p>Edit <tt>/etc/syslog.conf</tt> and add this line:</p>
<blockquote><tt>*.* /var/log/all.log</tt></blockquote></li>

<li><p>Make sure that <tt>/var/log/all.log</tt> exists with this command:</p>
<blockquote><tt>touch /var/log/all.log</tt></blockquote></li>

<li><p>Restart syslog with one of these commands:</p>
<blockquote><tt>service syslog restart</tt> (e.g. Fedora Core)</blockquote>
<blockquote><tt>/etc/init.d/sysklogd restart</tt> (e.g. Debian)</blockquote>
</li>

<li><p>Verify that <tt>/var/log/all.log</tt> contains something like this near
the end:</p> <blockquote><tt>syslog: syslogd startup
succeeded</tt></blockquote></li>

<li><p>Unload the <tt>usbserial</tt> and <tt>ipaq</tt> kernel modules with
these commands:</p><blockquote><tt>rmmod ipaq<br>rmmod
usbserial</tt></blockquote></li>

<li><p>Load the kernel modules with debug enabled using these
commands:</p><blockquote><tt>modprobe usbserial debug=1<br>modprobe ipaq
debug=1</tt></blockquote><p>(If needed by your device, don't forget to add
<tt>vendor</tt> and <tt>product</tt> options when loading the <tt>ipaq</tt>
kernel module.)</p></li>

<li><p>Only if you want PPP debugging: Edit <tt>/etc/ppp/peers/synce-device</tt> and add these lines:</p>
<blockquote><tt>debug<br>kdebug 255</tt></blockquote></li>

<li><p>Connect your PDA</p></li>

<li><p>Run <tt>synce-serial-start</tt></p></li>

<li><p>Look in <tt>/var/log/all.log</tt> for debug output from the kernel
modules and pppd.</p></li>

</ol>

<p>Return to <a href="usb_linux.php">USB on Linux page</a>.</p>
</div>
<?php include 'footer.php'; ?>
