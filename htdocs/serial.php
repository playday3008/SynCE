<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="index.php">main page</a>.</p>

<h1>SynCE - Serial connection</h1>

<ol>

<li class=SPACED>Make sure you are running as the root user.</li>

<li class=SPACED>Find out the device name for your serial port:<br/>

<table>

<hr size=1 />

<tr><td><i>Operating system</i>&nbsp;&nbsp;</td><td><i>Serial ports (first, second, ...)</i></td></tr>
<tr><td>Linux</td><td>ttyS0, ttyS1, ...</td></tr>
</table>

<hr size=1 />

Please drop a mail to <a
href="mailto:synce-devel@lists.sourceforge.net">synce-devel@lists.sourceforge.net</a>
to add the serial ports on your favorite operating system to the above list!

</li>

<li>Run synce-serial-config with the serial port as parameter, for example ttyS0:

<pre>synce-serial-config ttyS0</pre></li>

<li class=SPACED>Time to <a href="start.php">start your connection</a>!</li>

</ol>

<p>If you, out of curiosity, wants to know how this works "under the hood" you
can read <a href="http://www.cewindows.net/wce/linux-serial.htm">Connecting
Linux and Windows CE via Serial &amp; IrDA</a>.</p>

<p>Return to <a href="index.php">main page</a>.</p>

</div>
<?php include 'footer.php'; ?>
