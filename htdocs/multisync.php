<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="index.php">main page</a>.</p>

<h1>SynCE - Using MultiSync for synchronization</h1>

<h2>Overview</h2>

<p>SynCE is capable of synchronizing calendar, task and address book data with
for example <a href="http://www.ximian.com/products/evolution/">Ximian
Evolution</a> using a plugin for <a
href="http://multisync.sourceforge.net/">MultiSync</a>.</p>

<p><b>Warning:</b> This is experimental software. Anything can happen. Use it
at your own risk.</p>

<h2>About Evolution and MultiSync versions</h2>

<p>This table tries to help you find the correct versions of MultiSync and SynCE's <tt>multisync_plugin</tt> module.</p>

<table cellspacing=5 width="100%">

<tr><th>Evolution<br>version</th><th>MultiSync<br>version</th><th>MultiSync<br>CVS branch</th><th>multisync_plugin<br>version</th><th>multisync_plugin<br>CVS branch</th></tr>

<tr><td>1.2.x or earlier</td><td>0.72</td><td>gnome-1</td><td>multisync_plugin-gnome1<br>0.8.3 or later</td><td>BRANCH_GNOME_1</td></tr>

<tr><td>1.4.0 or later</td><td>0.80</td><td>HEAD</td><td>multisync_plugin<br>0.8.2 or later</td><td>HEAD</td></tr>

</table>

<h2>Getting started</h2>

<p>These instructions will hopefully get you started:</p>

<ol>

<li class=SPACED>Get the correct version of <a
href="http://multisync.sourceforge.net/">MultiSync</a> up and running. Verify
that MultiSync works with your version of Evolution by creating and testing a
synchronization pair with the Evolution and Backup plugins.</li>

<li class=SPACED><a href="download.php">Download</a> and <a
href="setup.php">setup</a> SynCE including the <tt>rra</tt> and
<tt>multisync_plugin</tt> modules.</li>

<li class=SPACED>If you have previously used ActiveSync to synchronize your
Pocket PC handheld, follow the instructions at <i>Using both the MultiSync
plugin and ActiveSync</i> below to change the current partnership.</li>

<li class=SPACED>Create a MultiSync synchronization pair with Evolution and the
SynCE plugin. You should only have the SynCE plugin in <i>one</i> connection
pair.</li>

<li class=SPACED>Synchronize!</li>

</ol>

<h2>Questions and Answers</h2>

<p><b>Q:</b> Synchronization fails with the message <tt>Connection reset by
peer</tt> and the connection between the PDA and the PC is lost!</p>

<p><b>A:</b> This is a known problem and it is under investigation. If you have
any information that may be of use for solving this problem, please send write
to one of the mailing lists!</p>

<hr size=1 width="50%">

<p><b>Q:</b> Tasks that were completed on my PDA were copied to evolution as
uncompleted. Why?</p>

<p><b>A:</b> It appears that Evolution requires a completion date to show the
appointment as completed. This will be fixed in the next SynCE version.</p>

<hr size=1 width="50%">

<p><b>Q:</b> All of my calendar appointments are one hour earlier in Evolution
than they are on my PDA. It looks like a time zone problem but I have the same
timezone on both PC and PDA.</p>

<p><b>A:</b> Time zones are a problem because we've not yet found a way to
deduce what timezone the PDA is currently using! Any help on this would be most
welcome. The problem may have something to do with daylight saving too.</p>

<hr size=1 width="50%">

<!--
<p><b>Q:</b> When I use the MultiSync plugin with SmartPhone 2003 or Pocket PC
2003, I get a message saying I should upgrade ActiveSync. What shall I do?</p>

<p><b>A:</b> Use the <a href="cvs.php">CVS version</a> of SynCE or wait for SynCE version 0.8.</p>

<hr size=1 width="50%">
-->

<p>

<b>Q:</b> All contacts from Evolution were synchronized to the Pocket PC, but
not all contacts from Pocket PC were synchronized to Evolution, why is
that? </p>

<p>

<b>A:</b> You are probably using the same partnership as you have used with
ActiveSync before. See <i>Using both the MultiSync plugin and ActiveSync</i>
below for more information.

</p>

<hr size=1 width="50%">

<!--
<p><b>Q:</b> Phone numbers and adresses are not synchronized properly from Pocket
PC to Evolution.</p>

<p><b>A:</b> This bug should be fixed in the <a
href="http://sourceforge.net/cvs/?group_id=30550">CVS version</a> of the
<tt>rra</tt> module and will be included in the next SynCE release.</p>

<hr size=1 width="50%">

<p><b>Q:</b> The MultiSync plugin doesn't work on Power PC, SPARC, or another big
endian platform.</p>

<p><b>A:</b> Multiple bugs related to endianness have been fixed in the <a
href="http://sourceforge.net/cvs/?group_id=30550">CVS version</a> of the
<tt>rra</tt> module. These fixes will be included in the next release of
SynCE.</p>

<hr size=1 width="50%">

<p><b>Q:</b> Synchronization from Pocket PC to Evolution fails with these
messages:</p>

<pre>[rrac_recv_reply_6f_c1:402] Unexpected command
[rra_get_object_types:118] Failed to receive reply
[rra_get_object_ids:174] Failed to get object types
[synce_get_changed_objects:44] Failed to get object ids</pre>

<p><b>A:</b> This seems to be a bug that is not yet fixed. You may make it easier
to fix them by telling how you got them:</p>
-->

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

<hr size=1 width="50%">

<p><b>Q:</b> How do partnerships work?</p>

<p><b>A:</b> Answer provided by <a href="voc@users.sourceforge.net">Volker
Christian</a> and slightly edited:</p>

<p>Partnerships in Windows CE are relevant for synchronizing only. The
synchronizing-algorithm in PocketPC traces by use of the partnerships which
contact, task, ... are already synchronized with which partner. Thats why
activesync forbids synchronizing when only a guest-partnership is active.</p>

<p>The 1 and the 2 actualy numbers the partnerships in the registry.  What you
have in the registry is:</p>

<p>\HKLM\SOFTWARE\Microsoft\Windows CE Services\Partners\PCur ... 1 or 2<br>
\HKLM\SOFTWARE\Microsoft\Windows CE Services\Partners\P1\PId<br>
\HKLM\SOFTWARE\Microsoft\Windows CE Services\Partners\P1\PName<br>
\HKLM\SOFTWARE\Microsoft\Windows CE Services\Partners\P2\PId<br>
\HKLM\SOFTWARE\Microsoft\Windows CE Services\Partners\P2\PName</p>

<p>So 1 and 2 select either P1 or P2.  PId are random numbers generated on the
desktop-machine during partnership setup and are stored in both, the desktop
and the pda. PName are names of the desktop machines.</p>

<p>On connect the desktop looks into P1 and P2 and compares the stored PName
with its own name and if it finds a match it also compares the random number
PId with its own stored number. If also the PId matches the desktop stores a 1
or a 2 in PCur dependent on the matched partnership.</p>

<p>Now Windows CE could manage the synchronization process in that way, that
both partners and also the PDA have consistent states.</p>


<h2>Using both the MultiSync plugin and ActiveSync</h2>

<p>A Pocket PC device can only have two simultaneous partnerships with desktop
computers. When using both SynCE and ActiveSync, one of these partnerships will
be used by SynCE and the other by ActiveSync. When you want to synchronize with
SynCE and you last synchronized with ActiveSync, you have to change what
parntership you use. This is described below.</p>

<p>For SynCE 0.8 or later:</p>

<p>Use the <tt>synce-partnership</tt> tool like this:</p>

<p><tt>synce-partnership create</tt></p>

<p>If you get the message "Partnership creation succeeded" everything is ok.</p>

<p>If you get the message "Partnership creation failed" you need to replace an
existing partnership on your device. To do this you first run
<tt>synce-partnership status</tt> to list the current partnerships on your
device. Decide which partnership (1 or 2) you want to replace. Second you run
<tt>synce-partnership replace X</tt>, where X is the partnership number.</p>


<p>For SynCE 0.7:</p>

<ol> 

<li class=SPACED>Get a <a
href="http://www.phm.lu/Products/PocketPC/RegEdit/">registry editor for Pocket
PC</a>. Tip: You can install a .cab file on your Pocket PC with the
<tt>synce-install-cab</tt> tool.</li>

<li class=SPACED>Go to
<b>HKEY_LOCAL_MACHINE\Software\Microsoft\Windows&nbsp;CE&nbsp;Services\Partners</b></li>

<li class=SPACED>Change the value of <b>PCur</b> from 1 to 2, or from 2 to
1.</li>

</ol>

<p><br>Return to <a href="index.php">main page</a>.</p>

</div>
<?php include 'footer.php'; ?>
