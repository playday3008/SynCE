<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="index.php">main page</a>.</p>

<h1>SynCE - Using MultiSync for synchronization</h1>

<h2>Overview</h2>

<p>SynCE is capable of synchronizing address book data with for example <a
href="http://www.ximian.com/products/evolution/">Ximian Evolution</a> using a
plugin for <a href="http://multisync.sourceforge.net/">MultiSync</a>.</p>

<p><b>Warning:</b> This is experimental software. Anything can happen. Use it
at your own risk.</p>

<h2>Getting started</h2>

<p>These instructions will hopefully get you started:</p>

<ol>

<li class=SPACED>Get <a href="http://multisync.sourceforge.net/">MultiSync</a>
up and running. Verify that MultiSync works with your version of Evolution by
creating and testing a synchronization pair with the Evolution and Backup
plugins.</li>

<li class=SPACED><a href="using.php">Install SynCE</a> including the
<tt>rra</tt> and <tt>multisync_plugin</tt> modules.</li>

<li class=SPACED>If you have previously used ActiveSync to synchronize your
Pocket PC handheld, follow the instructions at <i>Using both SynCE and
ActiveSync</i> below to change the current partnership.</li>

<li class=SPACED>Create a MultiSync synchronization pair with Evolution and the
SynCE plugin.</li>

<li class=SPACED>Synchronize!</li>

</ol>

<h2>Questions and Answers</h2>

<p>

<b>Q:</b> All contacts from Evolution were synchronized to the Pocket PC, but
not all contacts from Pocket PC were synchronized to Evolution, why is
that? </p>

<p>

<b>A:</b> You are probably using the same partnership as you have used with
ActiveSync before. See <i>Using both SynCE and ActiveSync</i> below for more
information.

</p>

<hr size=1 width="50%">

<p>

<b>Q:</b> Synchronization from Pocket PC to Evolution fails with these
messages:

<pre>[rrac_recv_reply_6f_c1:402] Unexpected command
[rra_get_object_types:118] Failed to receive reply
[rra_get_object_ids:174] Failed to get object types
[synce_get_changed_objects:44] Failed to get object ids</pre>

or with this message:

<pre>[synce_get_changed_objects:44] Failed to get object ids</pre>

<b>A:</b> These are bugs that are not yet fixed. You may make it easier to fix
them by telling how you got them :

<ol>

<li>Change the <tt>#define</tt> named <tt>DUMP_PACKETS</tt> from
<tt>0</tt> to <tt>1</tt> in <tt>rra/lib/rrac.c</tt></li>

<li>Recompiling and installing <tt>rra</tt></li>

<li>Recompiling and installing <tt>multisync_plugin</tt></li>

<li>Reproducing the problem</li>

<li>Sending relevant parts of the output from MultiSync to the <a
href="http://lists.sourceforge.net/mailman/listinfo/synce-devel">synce-devel</a>
mailing list.</li>

</ol>

</p>


<h2>Using both SynCE and ActiveSync</h2>

<p>A Pocket PC device can only have two simultaneous partnerships with desktop
computers. When using both SynCE and ActiveSync, one of these partnerships will
be used by SynCE and the other by ActiveSync. When you want to synchronize with
SynCE and you last synchronized with ActiveSync, you have to change what
parntership you use. This is described below.</p>

<p>Note: This manual operation will not be neccessary in future versions of
SynCE.</p>

<ol> 

<li class=SPACED>Get a <a href="http://www.phm.lu/Products/PocketPC/">registry editor
for Pocket PC</a>. Tip: You can install a .cab file on your Pocket PC with the
<tt>synce-install-cab</tt> tool.</li>

<li class=SPACED>Go to
<b>HKEY_LOCAL_MACHINE\Software\Microsoft\Windows&nbsp;CE&nbsp;Services\Partners</b></li>

<li class=SPACED>Change the value of <b>PCur</b> from 1 to 2, or from 2 to
1.</li>

</ol>

<p>Return to <a href="index.php">main page</a>.</p>

</div>
<?php include 'footer.php'; ?>
