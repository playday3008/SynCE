<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="usb.php">USB page</a>.</p>

<h1>SynCE - USB connection - Linux</h1>

<p>To use a USB connection on Linux, the following kernel module must be compiled:</p>

<p><tt>USB Support -&gt;<br>
USB Serial Converter support -&gt;<br>
USB Compaq iPAQ / HP Jornada / Casio EM500 Driver</tt></p>

<p>Useful reading: How to <a href="usbids.php">find the USB Vendor and Product
IDs</a> for your PDA.</p>

<p>See <a href="#success">Successful USB connection</a> for a log message from
a successful USB connection.</p>

<p>Select your kernel version to see if your device works with your kernel:</p>

<ul>

<li><a href="#many">2.4.21 or later kernels, including 2.6 series</a></li>

<li><a href="#early">2.4.18 to 2.4.20 kernel</a></li>

<li><a href="#none">Kernel prior to 2.4.18</a></li>

</ul>

<a name="many"></a>
<h2>2.4.21 or later kernels, including 2.6 series</h2>

<p>Please note that no 2.5 kernels are supported!</p>

<p>(This also includes RedHat's 2.4.20 kernels.)</p>

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

<blockquote><p>The HP iPAQ 5550 and some other newer devices, probably including the HP iPAQ 4150, do not work 
with any currently existing  version of
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

<p>You could also put a line in your <tt>/etc/modules.conf</tt> file for
this:</p>

<pre>option ipaq ttyUSB=1</pre>

<p>Then configure SynCE to use ttyUSB1 instead of ttyUSB0.</p>

</blockquote>

<h3>Supported vendor and product IDs</h3>

<p>This kernel supports many common and uncommon devices, but certain newer
devices are missing.</p>

<table cellpadding=3>
<tr><th>Vendor</th><th>Vendor ID</th><th>Product</th><th>Product ID</th></tr>

<tr><td>Askey</td><td>1690</td><td>Unknown</td><td>0601</td></tr>

<tr><td>Bcom</td><td>0960</td><td></td><td>0065<br>0066<br>0067</td></tr>

<tr><td>Casio</td><td>07cf</td><td>Unknown<br>EM500 etc</td><td>2001<br>2002</td></tr>

<tr><td>Compaq</td><td>049f</td><td>iPAQ<br>Unknown</td><td>0003<br>0032</td></tr>

<tr><td>Dell</td><td>413c</td><td>Axim</td><td>3001</td></tr>

<tr><td>Fujitsu-Siemens</td><td>0bf8</td><td>Loox</td><td>1001</td></tr>

<tr><td>HP</td><td>03f0</td><td>
Jornada&nbsp;540/548,&nbsp;iPAQ&nbsp;2215/5550/etc<br>
Jornada 568<br>
Unknown
</td><td>
1016<br>
1116<br>
2016<br>
2116<br>
2216<br>
3016<br>
3116<br>
3216<br>
4016<br>
4116<br>
4216<br>
5016<br>
5116<br>
5216<br>
</td></tr>

<tr><td>Linkup</td><td>094b</td><td>Unknown</td><td>0001</td></tr>

<tr><td>Microsoft</td><td>045e</td><td>Unknown</td><td>00ce</td><td>(Used by Motorola MPX200 SmartPhone)</td></tr>

<tr><td>Portatec</td><td>0961</td><td>Unknown</td><td>0010</td></tr>

<tr><td>Sagem</td><td>5e04</td><td>Unknown</td><td>ce00</td></tr>

<tr><td>Socket</td><td>0104</td><td>Unknown</td><td>00be</td></tr>

<tr><td>Toshiba</td><td>0930</td><td>
Unknown<br>
E740
</td><td>
0700<br>
0706
</td></tr>

<tr><td>HTC</td><td>0bb4</td><td>Unknown</td><td>00ce</td></tr>

<tr><td>NEC</td><td>0409</td><td>Unknown</td><td>00d5</td></tr>

<tr><td>Asus</td><td>0b05</td><td>A600</td><td>4201</td></tr>

</table>

<p>If your device is in the list above, go ahead and <a
href="usb_linux_setup.php">configure SynCE</a>.</p>

<h3>Enabling devices not directly supported by the driver</h3>

<p>If your device is not in the list above, load the <tt>ipaq</tt> kernel
module manual like this, but with the vendor and product ID for your
device:</p>

<pre>insmod usbserial
insmod ipaq vendor=0x049f product=0x0032</pre>

<p>Or add a line similar to this to your <tt>/etc/modules.conf</tt> file before
connecting your device:</p>

<pre>options ipaq vendor=0x3340 product=0x3326</pre>

<p>If you can provide additional details above the devices listed above, please
<a href="help.php">contact the SynCE developers</a>.</p>

<a name="early"></a>
<h2>2.4.18 to 2.4.20 kernels</h2>

<p>(If you have a RedHat 2.4.20 kernel, see <a href="#many">2.4.21 or later
kernels, including 2.6 series</a>.)</p>

<p>If you going to use SynCE on an SMP (multi-processor) system with this
kernel you must <a href="usbpatch.php">patch your kernel driver</a> or upgrade
to <a href="#many">2.4.21 or a later kernel</a>.</p>

<p>These kernel version only support these devices:</p>

<table cellpadding=3>
<tr><th>Vendor</th><th>Vendor ID</th><th>Product</th><th>Product ID</th></tr>
<tr><td>Casio</td><td>07cf</td><td>EM500 and probably others</td><td>2002</td></tr>
<tr><td>Compaq</td><td>049f</td><td>iPAQ (any model?)</td><td>0003</td></tr>
</table>

<p>If your device is in the list above, go ahead and <a
href="usb_linux_setup.php">configure SynCE</a>, otherwise you need to <a
href="usbpatch.php">patch your kernel driver</a> or upgrade to <a
href="#many">2.4.21 or newer 2.4.x kernel</a>.</p>

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
