<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="index.php">main page</a>.</p>

<h1>Using SynCE</h1>

<h2>Contents</h2>

<ul>
<li><a href="#download">Download and installation</a></li>
<li><a href="#gettingconnected">Getting connected</a></li>
</ul>

<a name="download"></a>
<h2>Download and installation</h2>


<a name="openfirewall"></a>
<h2>Open your firewall</h2>

<p>You need to allow your handheld device to access ports 5678 and 5679 on your
PC. The PC will need to access port 990 on your handheld device.</p>

<p>If you are running Linux, you can list your firewall rules with <tt>ipchains
-L</tt> for Linux kernel 2.2 and <tt>iptables -L</tt> for Linux kernel 2.4.</p>

<p>If you are running RedHat Linux, you may have RedHat's built-in firewall
function enabled. Run either the <tt>lokkit</tt> or the
<tt>redhat-config-securitylevel</tt> program as root to open the ports listed
above.</p>

<a name="gettingconnected"></a>
<h2>Getting connected</h2>

<p>Please select the type of connection you will use:</p>

<ul>

<li><a href="serial.php">Serial cable</a></li>
<li><a href="infrared.php">Infrared</a></li>
<li><a href="usb.php">USB cable</a></li>
<li><a href="bluetooth.php">Bluetooth</a></li>

</ul>

<p>When you have configured your connection you are ready to <a href="start.php">start it</a>.</p>

<p>When you have started your connection you can try the <a href="tools.php">tools</a>.</p>

<a name="support"></a>
<h2>Support</h2>

<p>We are very happy to answer your questions about the SynCE project. However,
in order to limit the amount of support, we would like you to first read <a
href="http://www.catb.org/~esr/faqs/smart-questions.html#intro">this
document</a> and search for the answer to your question in the <a
href="http://sourceforge.net/mail/?group_id=30550">mailing list archives</a>.
If your question remains unanswered, send mail to <a
href="mailto:synce-users@lists.sourceforge.net">synce-users@lists.sourceforge.net</a>.</p>

<p>Return to <a href="index.php">main page</a>.</p>

</div>
<?php include 'footer.php'; ?>
