<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="index.php">main page</a>.</p>

<h1>SynCE - Getting help</h1>

<p>How to get help from the SynCE community:</p>

<ol>

<li class=SPACED>Learn how to <a
href="http://www.catb.org/~esr/faqs/smart-questions.html#intro">ask smart
questions</a></li>

<li class=SPACED>Read the <a href="qa.php">Questions &amp; answers</a> page</li>

<li>Search the <a href="http://sourceforge.net/mail/?group_id=30550">mailing
list archives</a> and <a
href="http://sourceforge.net/forum/?group_id=30550">forums</a> to see if your
problem has been discussed before</li>

<li><p>If you still can't solve the problem send mail to a suitable <a
href="http://sourceforge.net/mail/?group_id=30550">mailing list</a> or write a
new entry in one of the <a
href="http://sourceforge.net/forum/?group_id=30550">forums</a>. These are some things
that might be useful to include when you ask for help:</p>

<ul>


<li class=SPACED><p><b>General</b></p>

<ul>
<li>Device name and Windows CE version</li>
<li>Connection method (serial cable, USB cable, IrDA, Bluetooth)</li>
<li>Operating system name, distribution and kernel version</li>
<li>SynCE version</li>
<li>If you compiled SynCE yourself or used pre-compiled packages</li>
</ul></li>


<li class=SPACED><p><b>For connection problems</b></p>

<ul>
<li>Information from the <tt>usbview</tt> tool for the device</li>
<li>Relevant log extracts from kernel driver and pppd</li>
<li>Your <tt>/etc/ppp/peers/synce-device</tt> file (created by synce-serial-config)</li>
<li>Kill dccm, and restart it like this: <tt>dccm -f -d 3</tt>. Now connect
your PDA and provide the output from dccm.</li>

</ul></li>

<li class=SPACED><p><b>For synchronization problems</b></p>

<ul>
<li>MultiSync and Evolution versions</li>
<li>Messages in the terminal window when running MultiSync</li>

</ul></li>

</ol>

<p>Return to <a href="index.php">main page</a>.</p>

</div>
<?php include 'footer.php'; ?>
