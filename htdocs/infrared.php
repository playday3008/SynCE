<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="index.php">main page</a>.</p>

<h1>SynCE - Infrared connection</h1>

<ol>

<li class=SPACED>Make sure you are running as the root user.</li>

<li class=SPACED>Make sure you have IrDA setup properly. On Linux, you can
easily verify this by typing <code>ifconfig</code> and see if the
<code>irda0</code> interface is present. If it's not, you may want to consult
some of these links:

<hr size=1 />
<ul>
<li><a href="http://mobilix.org/Infrared-HOWTO/Infrared-HOWTO.html">Linux Infrared HOWTO</a></li>
<li><a href="http://irda.sourceforge.net/">Linux-IrDA project</a></li>
<li><a href="http://www.hpl.hp.com/personal/Jean_Tourrilhes/IrDA/index.html">Linux-IrDA quick tutorial</a></li>
</ul>
<hr size=1 />

</li>

<li>Run synce-serial-config with the infrared port as parameter, this is probably ircomm0:

<pre>synce-serial-config ircomm0</pre></li>

<li class=SPACED>Time to <a href="start.php">connect</a>!</li>

</ol>

<p>If you, out of curiosity, wants to know how this works "under the hood" you
can read <a href="http://www.cewindows.net/wce/linux-serial.htm">Connecting
Linux and Windows CE via Serial &amp; IrDA</a>.</p>

<p>Return to <a href="index.php">main page</a>.</p>

</div>
<?php include 'footer.php'; ?>
