<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="index.php">main page</a>.</p>

<h1>SynCE - Connect</h1>

<p>Remember that you must <a href="setup.php">setup</a> SynCE first! :-)</p>

<ol>

<li class=SPACED>Make sure you are running as your own user.</li>

<li>Start dccm:

<p>If your device is password-protected, for example with password "1234":</p>

<pre>    dccm -p 1234</pre>

<p>If your device is not password-protected:</p>

<pre>    dccm</pre>

</li>

<p>You can have dccm running all the time!</p>

<p><i>Note users of old SynCE versions:</i> You have to disable the obsolete
<i>asyncd</i> program before you can run dccm.</p>

<li class=SPACED>Make sure you are running as the root user.</li>

<li class=SPACED>If you are using USB or serial cable, connect the cable to the
PC.</li>

<li>Run synce-serial-start:<br>
<pre>    synce-serial-start</pre></li>

<li class=SPACED>If your device has not automatically connected, initiate the
connection manually on your PDA.</li>

<li>The following message (or similar) should appear in your logs:

<pre>pppd[2695]: pppd 2.4.1 started by root, uid 0
pppd[2695]: Serial connection established.
pppd[2695]: Using interface ppp0
pppd[2695]: Connect: ppp0 <--> /dev/ttyUSB0
pppd[2695]: local  IP address 192.168.131.102
pppd[2695]: remote IP address 192.168.131.201</pre>

</li>


<li class=SPACED>Try the <a href="tools.php">tools</a> as your own user.</li>

<li class=SPACED>Follow the instructions to <a
href="disconnect.php">disconnect</a> properly.</li>

</ol>

<p>Return to <a href="index.php">main page</a>.</p>

</div>
<?php include 'footer.php'; ?>
