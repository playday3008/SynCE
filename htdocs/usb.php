<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="index.php">main page</a>.</p>

<h1>SynCE - USB connection</h1>

<p>These instructions apply to systems with Linux kernel version 2.4.18 or
later. On other systems, please request help on the mailing list: <a
href="mailto:synce-devel@lists.sourceforge.net">synce-devel@lists.sourceforge.net</a>.</p>

<p>Are you running one of these devices?</p>

<ul>
<li>Compaq/HP iPAQ (any model?)</li>
<li>HP Jornada (548, 568, and probably others)</li>
<li>Casio (EM500 and probably others)
</ul>

<p>If your device was listed above, go ahead with these instructions:</p>

<ol>

<li class=SPACED>Make sure you are running as the root user.</li>

<li>Try to load the "ipaq" kernel module:

<pre>modprobe ipaq</pre>

<p>This should give the following message (or similar) in your logs:</p>

<pre>kernel: usbserial.c: USB Serial support registered for Compaq iPAQ
kernel: ipaq.c: USB Compaq iPAQ, HP Jornada, Casio EM500 driver v0.2</pre>

</li>

<li>Now connect your USB device. The following message (or similar) should appear in your logs:

<pre>kernel: usbserial.c: Compaq iPAQ converter now attached to ttyUSB0 (or usb/tts/0 for devfs)</pre>
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
