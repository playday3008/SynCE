<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="usb.php">USB page</a>.</p>

<h1>SynCE - USB connection - Linux - Configuration</h1>

<p>Run synce-serial-config with your serial-over-USB port as parameter. For
most Linux systems, this is ttyUSB0:</p>

<pre>synce-serial-config ttyUSB0</pre>

<p>You only have to do run this command again if you change what device you use
for connecting your PDA.</p>

<p>Time to <a href="start.php">start your connection</a>!</p>

<p>Tip of the day: When you are sure that your connection is working properly,
  you may want to install a <a href="hotplug.php">hotplug script</a>.</p>

</div>
<?php include 'footer.php'; ?>
