<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="index.php">main page</a>.</p>

<h1>SynCE - Using the SynCE plugin for MultiSync</h1>

<p><b>Warning!</b> This is experimental software. Anything can happen. Use it
at your own risk.</p>

<p>Follow these instructions to use the SynCE plugin for MultiSync</p>

<ol>

<li class=SPACED>Get the latest 0.8x version of <a
href="http://multisync.sourceforge.net/">MultiSync</a> up and running. Verify
that MultiSync works with your version of Evolution by creating and testing a
synchronization pair with the Evolution and Backup plugins.</li>

<li><p>Use the <tt>synce-matchmaker</tt> tool to create a partnership between
SynCE and your Windows CE device. If you don't have this tool you must install
the <b>synce-rra</b> module. <a
href="http://sourceforge.net/project/showfiles.php?group_id=30550&package_id=22569">Download</a>.</p></li>

<li><p>Install the SynCE plugin for MultiSync. <a
href="http://sourceforge.net/project/showfiles.php?group_id=30550&package_id=92164">Download</a>.</p></li>

<li><p>Make sure that you have a working connection to your PDA, for example by
running the pstatus <a href="tools.php">tool</a>.</p></li>

<li><p>Create a new synchronization pair in MultiSync, where SynCE is one of
the plugins.</p></li>

<li><p>The first time you synchronize, the SynCE plugin for MultiSync will
attach to the current partnership and subsequently only synchronize with a
device that has this partnership.</p></li>

</ol>

<p>Only have the SynCE plugin for MultiSync in one synchronization pair for
each of your Windows CE devices. If you want to synchronize a Windows CE device
with more than one other plugin, setup your synchronization pairs something
like this:</p>

<blockquote>SynCE - Evolution<br/>
Evolution - IrMC</blockquote>


<p>Se also the <a href="multisync.php">MultiSync Questions and Answers</a> page.</p>

<p>Return to <a href="index.php">main page</a>.</p>

</div>
<?php include 'footer.php'; ?>
