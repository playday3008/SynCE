<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="index.php">main page</a>.</p>

<h1>SynCE - USB connection</h1>

<p>If you are not running Linux, please request help on the mailing list: <a
href="mailto:synce-devel@lists.sourceforge.net">synce-devel@lists.sourceforge.net</a>.

<p>To be able to use SynCE with a USB connection, you need to be running Linux
kernel version 2.4.18 or later.</p>

<p>Are you running one of these devices on a Linux kernel prior to 2.4.20?</p>

<ul>
<li>Compaq/HP iPAQ (except the 1910)</li>
<li>Casio (EM500 and probably others)</li>
</ul>

<p>If not, you should first <a href="usbpatch.php">patch your kernel driver</a>.</p>

<p>Also, if you going to use SynCE on an SMP system you should first <a
href="usbpatch.php">patch your kernel driver</a>.</p>

<p>When you have fulfilled the conditons above you can follow these steps:</p>

<ol>

<li>Connect your USB device. The following message (or similar) should appear in your logs:

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

</li>

<li>Run synce-serial-config with your serial-over-USB port as parameter. For most systems, this is ttyUSB0:

<pre>synce-serial-config ttyUSB0</pre></li>

<li class=SPACED>Time to <a href="start.php">start your connection</a>!</li>

</ol>

<p>If your device was not in the above list, you will unfortunately need to
modify the kernel module. Please request help on the mailing list: <a
href="mailto:synce-devel@lists.sourceforge.net">synce-devel@lists.sourceforge.net</a>.</p>

<p>Return to <a href="index.php">main page</a>.</p>

</div>
<?php include 'footer.php'; ?>
