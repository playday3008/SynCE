<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="usb.php">USB page</a>.</p>

<h1>SynCE - USB connection - Linux</h1>

<p>If you compile the Linux kernel yourself, make sure that this driver is
included in your kernel configuration:</p>

<blockquote><tt>USB Support -&gt;<br>
USB Serial Converter support -&gt;<br>
USB PocketPC PDA Driver</tt></blockquote>

<p>(Earlier this was called the <tt>USB Compaq iPAQ / HP Jornada / Casio EM500
Driver</tt>)</p>

<p>Pre-compiled Linux kernels usually includes the <tt>ipaq</tt> Linux kernel
driver that is required to use SynCE with USB.</p>

<p><b>Important</b> Do not use an external USB hub when you are trying to get
SynCE working for the first time! If you want to use a USB hub later you should
first read the information regarding the <tt>ipaq</tt> kernel driver and use
of an <a href="usb_linux_hub.php">external USB hub</a>.</p>

<!--
<p>Useful reading: How to <a href="usbids.php">find the USB Vendor and Product
IDs</a> for your PDA.</p>

<p>See <a href="#success">Successful USB connection</a> for a log message from
a successful USB connection.</p>
-->

<p>Select your kernel version to see how to get your device working:</p>

<ul>

<li><a href="#many">2.4.21 or later kernels, including 2.6 series</a></li>

<li><a href="#early">2.4.18 to 2.4.20 kernel</a></li>

<li><a href="#none">Kernel prior to 2.4.18</a></li>

</ul>

<a name="many"></a>
<h2>2.4.21 or later kernels, including 2.6 series</h2>

<p>(This also includes RedHat's 2.4.20 kernels.)</p>

<p>Please note that no 2.5 kernels are supported!</p>

<a name="werestuffed"></a>
<h3>The "we're stuffed" bug</h3>

<blockquote>

<p><b>Symptoms</b> The <tt>ipaq</tt> kernel driver recognizes the USB device, but it
is not possible to get a PPP connection.</p>

<p><b>Showing the bug</b> Only if you really care! :-) if you follow the <a
href="usb_linux_debug.php">debug instructions</a> you will get something like
this in your log:</p>

<pre>synce-serial-start: Executing '/usr/sbin/pppd call synce-device'
pppd[6162]: pppd 2.4.2 started by root, uid 0
kernel: drivers/usb/serial/ipaq.c: ipaq_open - port 0
kernel: drivers/usb/serial/ipaq.c: ipaq_read_bulk_callback - port 0
kernel: drivers/usb/serial/ipaq.c: ipaq_read_bulk_callback - length = 1, data = 00
kernel: drivers/usb/serial/ipaq.c: ipaq_chars_in_buffer - queuelen 0
kernel: drivers/usb/serial/ipaq.c: ipaq_read_bulk_callback - port 0
kernel: drivers/usb/serial/ipaq.c: ipaq_read_bulk_callback - length = 6, data = 43 4c 49 45 4e 54
kernel: drivers/usb/serial/ipaq.c: ipaq_write - port 0
<span class="RED">kernel: drivers/usb/serial/ipaq.c: ipaq_write_bulk - we're stuffed</span>
kernel: drivers/usb/serial/ipaq.c: ipaq_read_bulk_callback - port 0
kernel: drivers/usb/serial/ipaq.c: ipaq_read_bulk_callback - length = 6, data = 43 4c 49 45 4e 54
kernel: drivers/usb/serial/ipaq.c: ipaq_read_bulk_callback - port 0
kernel: drivers/usb/serial/ipaq.c: ipaq_read_bulk_callback - length = 6, data = 43 4c 49 45 4e 54
kernel: drivers/usb/serial/ipaq.c: ipaq_read_bulk_callback - port 0
kernel: drivers/usb/serial/ipaq.c: ipaq_read_bulk_callback - length = 6, data = 43 4c 49 45 4e 54
kernel: drivers/usb/serial/ipaq.c: ipaq_read_bulk_callback - port 0
kernel: drivers/usb/serial/ipaq.c: ipaq_read_bulk_callback - nonzero read bulk status received: -84
kernel: usb 1-2: USB disconnect, address 9
kernel: usbserial 1-2:1.0: device disconnected</pre>

<p><b>Affected systems</b> At least:</p>

<ul>

<li>Suse 9.1</li>
<li>Gentoo</li>
<li>Debian Unstable</li>

</ul>

<p><b>Solution</b> Download <a
href="http://synce.sourceforge.net/tmp/kernel-2.6-driver.tar.gz">kernel-2.6-driver.tar.gz</a>
and follow the instructions in the README file.</p>

</blockquote>

<h3>Special information for Mandrake 9.2 users with kernel 2.4</h3>
 

<blockquote>

<p>If you are using the Mandrake 9.2 kernels, for example 2.4.22-18mdk or
2.4.22-21mdk, the <tt>ipaq</tt> kernel module needed by SynCE will not work.
The problem is in the <tt>usbserial</tt> kernel module.  Apply <a
href="patches/usbserial.patch-mandrake-9.2">usbserial.patch-mandrake-9.2</a>
(courtesy of Olivier Dugeon) to make the ipaq driver work. See <a
href="usbpatch.php#compile">Compile the kernel module</a> for help with
compiling the module.</p>

</blockquote>


<h3>Special information for HP iPAQ 5550 owners</h3>

<p>Also applies to the <b>MiTAC Mio 168</b>, also known as <b>Bluemedia PDA
255</b> and <b>Yakumo Delta GPS 300</b>!</p>

<blockquote><p>The HP iPAQ 5550 and some other newer devices, probably
including the HP iPAQ 4150, do not work with any currently existing  version of
the <tt>ipaq</tt> module.</p>

<p>The problem is that these devices
have more than one pair of USB endpoints. (Compare the output from the
<tt>usbview</tt> program for <a
href="usbview/Hewlett-Packard_iPAQ_3970.txt">iPAQ 3970</a> and <a
href="usbview/Hewlett-Packard_iPAQ_5550.txt">iPAQ 5550</a>.)</p>

<p> To fix this, apply <a
href="patches/ipaq-0.6-olivier.patch">ipaq-0.6-olivier.patch</a> (courtesy of
Olivier Dugeon), <a href="usbpatch.php#compile">compile the kernel module</a>,
and insert it like this:</p>

<pre>modprobe ipaq ttyUSB=1</pre>

<p>You could also put a line in your <tt>/etc/modules.conf</tt> or
<tt>/etc/modprobe.conf</tt> file for this:</p>

<pre>option ipaq ttyUSB=1</pre>

<p>Then configure SynCE to use ttyUSB1 instead of ttyUSB0.</p>

</blockquote>

<h3>Supported vendor and product IDs</h3>

<p>See the list of <a href="usb_linux_ids.php">vendor/product IDs supported by
the ipaq Linux kernel driver</a> to know if your device is
supported.</p>

<p>If you are unsure of whether your device is included in the list above,
connect it via the USB cable and compare your logs with the log extract shown
in the <a href="#success">Successful USB connection</a> section.</p>

<p>If your device is in the list above, go ahead and <a
href="usb_linux_setup.php">configure SynCE</a>.</p>

<h3>Enabling devices not directly supported by the driver</h3>

<p>If your device is not in the list above, load the <tt>ipaq</tt> kernel
module manual like this, but with the vendor and product ID for your
device:</p>

<blockquote><pre>modprobe ipaq vendor=0x049f product=0x0032</pre></blockquote>

<p>Or add a line similar to this to the file <tt>/etc/modules.conf</tt> or
<tt>/etc/modprobe.conf</tt> before connecting your device:</p>

<blockquote><pre>options ipaq vendor=0x3340 product=0x3326</pre></blockquote>

<p>Useful reading: How to <a href="usbids.php">find the USB Vendor and Product
IDs</a> for your PDA.</p>

<p>See <a href="#success">Successful USB connection</a> for a log message from
a successful USB connection.</p>

<a name="early"></a>
<h2>2.4.18 to 2.4.20 kernels</h2>

<p>(If you have a RedHat 2.4.20 kernel, see <a href="#many">2.4.21 or later
kernels, including 2.6 series</a>.)</p>

<p>If you going to use SynCE on an SMP (multi-processor) system with this
kernel you must <a href="usbpatch.php">patch your kernel driver</a> or upgrade
to <a href="#many">2.4.21 or a later kernel</a> (recommended).</p>

<p>These kernel version only support these devices:</p>

<table cellpadding=3>
<tr><th>Vendor</th><th>Vendor ID</th><th>Product</th><th>Product ID</th></tr>
<tr><td>Casio</td><td>07cf</td><td>EM500 and probably others</td><td>2002</td></tr>
<tr><td>Compaq</td><td>049f</td><td>iPAQ (any model?)</td><td>0003</td></tr>
</table>

<p>If your device is in the list above, go ahead and <a
href="usb_linux_setup.php">configure SynCE</a>, otherwise you need to <a
href="usbpatch.php">patch your kernel driver</a> or upgrade to <a
href="#many">2.4.21 or newer 2.4.x kernel</a> (recommended).</p>

<p>See <a href="#success">Successful USB connection</a> for a log message from
a successful USB connection.</p>

<a name="none"></a>
<h2>Kernel prior to 2.4.18</h2>

<p>These kernels are not supported. Either upgrade or use the <a
href="usb_linux_userspace.php">user-space USB driver</a>.</p>


<a name="success"></a>
<h2>Successful USB connection</h2>

<p>When you connect your USB cable or insert your device into its cradle the
following message (or similar) should appear in your logs.</p>

<h3>Driver version 0.2</h3>

<pre>kernel: hub.c: USB new device connect on bus1/1, assigned device number 2
kernel: usb.c: USB device 2 (vend/prod 0x49f/0x3) is not claimed by any active driver.
/etc/hotplug/usb.agent: Setup ipaq for USB product 49f/3/0
kernel: usb.c: registered new driver serial
kernel: usbserial.c: USB Serial support registered for Generic
kernel: usbserial.c: USB Serial Driver core v1.4
kernel: usbserial.c: USB Serial support registered for Compaq iPAQ
kernel: usbserial.c: Compaq iPAQ converter detected
kernel: usbserial.c: Compaq iPAQ converter now attached to ttyUSB0 (or usb/tts/0 for devfs)
kernel: ipaq.c: USB Compaq iPAQ, HP Jornada, Casio EM500 driver v0.2</pre>

<h3>Driver version 0.5</h3>

<pre>kernel: hub.c: new USB device 00:14.2-2, assigned address 3
kernel: usb.c: USB device 3 (vend/prod 0x49f/0x3) is not claimed by any active driver.
kernel: usb.c: registered new driver serial
kernel: usbserial.c: USB Serial support registered for Generic
kernel: usbserial.c: USB Serial Driver core v1.4
kernel: ipaq.c: USB PocketPC PDA driver v0.5
kernel: usbserial.c: USB Serial support registered for PocketPC PDA
kernel: usbserial.c: PocketPC PDA converter detected
kernel: usbserial.c: PocketPC PDA converter now attached to ttyUSB0 (or usb/tts/0 for devfs)</pre>


</div>
<?php include 'footer.php'; ?>
