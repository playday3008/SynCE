<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="index.php">main page</a>.</p>

<h1>SynCE - Starting connection</h1>

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

<li>Run synce-serial-start:<br>
<pre>    synce-serial-start</pre></li>

<li class=SPACED>If your device has not automatically connected, initiate the
connection manually.</li>

<li class=SPACED>Try the <a href="tools.php">tools</a>.</li>

</ol>

<p>Return to <a href="index.php">main page</a>.</p>

</div>
<?php include 'footer.php'; ?>
