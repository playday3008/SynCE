<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="index.php">main page</a>.</p>

<h1>SynCE - Setup</h1>

<p>Remember that you must <a href="download.php">download and install</a>
SynCE first! :-)</p>

<h2>Open firewall</h2>

<p>You need to allow your handheld device to access ports 5678 and 5679 on your
PC. The PC will need to access port 990 on your handheld device.</p>

<p>If you are running Linux, you can list your firewall rules with <tt>ipchains
-L</tt> for Linux kernel 2.2 and <tt>iptables -L</tt> for Linux kernel 2.4.</p>

<p>If you are running RedHat Linux, you may have RedHat's built-in firewall
function enabled. Run either the <tt>lokkit</tt> or the
<tt>redhat-config-securitylevel</tt> program as root to open the ports listed
above.</p>


<h2>Connection</h2>

<p>Please select the type of connection you will use:</p>

<ul>

<li><a href="serial.php">Serial cable</a></li>
<li><a href="infrared.php">Infrared</a></li>
<li><a href="usb.php">USB cable</a></li>
<li><a href="bluetooth.php">Bluetooth</a></li>

</ul>

<p>Return to <a href="index.php">main page</a>.</p>

</div>
<?php include 'footer.php'; ?>
