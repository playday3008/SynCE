<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="index.php">main page</a>.</p>

<h1>SynCE - Getting help</h1>

<p>Instructions on how to get help from the SynCE community.</p>

<h2>Before asking for help</h2>

<p>If others have had the same problem before, the solution may already be
available!</p>

<ol>

<li class=SPACED>Read the <a href="qa.php">Questions &amp; answers</a>
page.</li>

<li class=SPACED>Search the <a
href="http://sourceforge.net/forum/forum.php?forum_id=96106">SynCE Help
Forum</a>. (Search box on the top left.)</li>

<li class=SPACED>Search the <a
href="http://sourceforge.net/mailarchive/forum.php?forum_id=15200">SynCE-Users
mailing list archive</a>. (Search box on the top left.)</li>

<li class=SPACED>Search the <a
href="http://sourceforge.net/mailarchive/forum.php?forum_id=1226">SynCE-Devel
mailing list archive</a>. (Search box on the top left.)</li>

</ol>

<p>If you can't find a previous solution to your problem, continue reading this
document!</p>

<h2>Information to collect before asking for help</h2>

<p>Provide the information requested in this section and you will get better
help faster when you ask your question!</p>

<ul>

<li class=SPACED><p><b>Always provide these data!</b></p>

<ul>
<li>Device name and Windows CE version</li>
<li>Connection method(s): serial cable, USB cable, IrDA, Bluetooth, Ethernet or WLAN</li>
<li>Operating system name, distribution and kernel version</li>
<li>SynCE version</li>
<li>If you compiled SynCE yourself or used pre-compiled packages</li>
</ul></li>


<li class=SPACED><p><b>For connection problems</b></p>

<ul>

<li>Information from <tt>usbview</tt> or <tt>lsusb -v</tt> for the device</li>

<li>Relevant log extracts from kernel driver and pppd</li>

<li>The contents of your files <tt>/etc/ppp/peers/synce-device</tt> (created by
<tt>synce-serial-config</tt>) and <tt>/etc/ppp/options</tt>.</li>

<li>Kill <tt>dccm</tt>, and restart it like this: <tt>dccm -f -d 4</tt>. Now connect
your PDA, run <tt>synce-serial-start</tt>, and provide the output from
<tt>dccm</tt>.</li>

<li>Is your device password-protected or not?</li>

</ul></li>

<li class=SPACED><p><b>For MultiSync synchronization problems</b></p>

<ul> <li>MultiSync and Evolution versions</li>

<li>Messages in the terminal window when running MultiSync. Set
<tt>MULTISYNC_DEBUG=1</tt> (for example with <tt>export</tt> if you are using
the bash shell) in the environment before starting MultiSync.</li>

</ul></li>

<li class=SPACED><p><b>For SynCE-KDE (RAKI or RAPIP) problems</b></p>

<ul>
<li>SynCE-KDE version</li>
<li>KDE version</li>

</ul></li>
</ul>

<h2>Asking for help</h2>

<p>First of all, you must learn <a
href="http://www.catb.org/~esr/faqs/smart-questions.html#intro">how to ask
questions the smart way</a>!</p>

<p>When you know that you can use one of these ways to reach the SynCE
community:</p>

<ul>

<li class=SPACED>If you like IRC you can visit the IRC channel <tt>#synce</tt>
on <b>irc.freenode.net</b> and see if anyone there can help you.</li>

<li class=SPACED>The <a
href="http://sourceforge.net/forum/forum.php?forum_id=96106">SynCE Help
Forum</a>.</li>

<li class=SPACED>For end-user problems, you can use the SynCE-Users mailing list:
<tt>synce-users at lists.sourceforge.net</tt></li>

<li class=SPACED>For development problems, you can use the SynCE-Devel mailing list:
<tt>synce-users at lists.sourceforge.net</tt></li>

</ul>

<p>If you do not get a reply within a week or two, it is OK to reply to your
own message asking if it's really true that no one can help you!</p>

<p>Return to <a href="index.php">main page</a>.</p>

</div>
<?php include 'footer.php'; ?>
