<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="index.php">main page</a>.</p>

<h1>SynCE - Disconnecting your device</h1>

<p>Before you unplug your PDA from your PC, you should close the network
connection between Windows CE and SynCE. There are a couple of ways to do this.
They are listed here with the most preferred ways first.</p>

<ol>

<li class=SPACED>Disconnect with the appropriate action on your PDA</li>

<li class=SPACED>Disconnect with the GNOME Tray Icon or SynCE-KDE</li>

<li class=SPACED>Run <tt>killall -HUP dccm</tt> from the command line</li>

<li class=SPACED>Run <tt>synce-serial-abort</tt>. Please note that this command is only to
be used when everything else fails. It also seems like it only works for USB
connections while the USB cable is connected.</li>

</ol>

<p>Future versions of SynCE will include a special command line tool to handle
this.</p>


<br>
<p>Return to <a href="index.php">main page</a>.</p>

</div>
<?php include 'footer.php'; ?>
