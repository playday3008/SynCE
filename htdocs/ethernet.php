<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="index.php">main page</a>.</p>

<h1>SynCE - Ethernet/WLAN connection</h1>

In this documentation it is assumed that you have already successfull 
installed your Ethernet/WLAN card in your device. We could not provide
support for this step as the installation procedure differs between different
network cards.

<ol>

<li class=SPACED>Before you could start using your Ethernet/WLAN connection with
SynCE you have to connect your PDA via one of the serial-based connection methodes:
<ul>
<li><a href="serial.php">Serial cable</a></li>
<li><a href="infrared.php">Infrared</a></li>
<li><a href="usb.php">USB cable</a> or</li>
<li><a href="bluetooth.php">Bluetooth</a></li>
</ul>
Before doing this make sure that the hostname of your desktop is known by a name server.
This is neccessary because the PDA does hostname resolution to initiate a Ethernet/WLAN 
connection. This hostname is stored in the registry of your PDA during the initial 
connection sequence.</li>

<li class=SPACED>Connect your device to the Ethernet/WLAN and make sure there is a name server
specified in the network settings of it.</li>

<li class=SPACED>Open ActiveSync on your device and click on <tt>Extras</tt> in the lower left
corner.</li>

<li class=SPACED>Choose the <tt>Options...</tt> menu entry and make sure, the checkbox
before "PC bei Remotesynchronisierung einschliessen und verbinden mit:" (Sorry, I only
have devices localized for german).</li>

<li class=SPACED>Choose your PC in the combobox below the checkbox and acknowledge with
<tt>ok</tt>.</li>

<li class=SPACED>Make sure dccm is running on your desktop.</li>

<li class=SPACED>On the main screen of ActiveSync on your PDA click <tt>Synchronisieren</tt></li>

</ol>

You should be connected now. Thats it! No need for <tt>synce-serial-start</tt> anymore. 

<br/>

<p>Return to <a href="index.php">main page</a>.</p>

</div>
<?php include 'footer.php'; ?>
