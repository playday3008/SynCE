<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="multisync_guide.php">MultiSync guide</a>.</p>

<h1>SynCE - MultiSync Questions and Answers</h1>

<p><b>Q:</b> What's the difference between <a
href="http://www.multisync.org/">MultiSync with 'C'</a> and <a
href="http://docs.kde.org/en/HEAD/kdepim/multisynk/">MultiSynk with 'K'</a>?</p>

<p><b>A:</b> A lot. This page is about MultiSync with 'C', a program often used
for synchronzing phones and handhelds with Novell Evolution. MultiSynk with 'K'
is a frontend for KDE's synchronization framework "kitchensync".</p>

<hr size=1 width="50%">

<p><b>Q:</b> Can I use SynCE with <a href="http://www.opensync.org/">OpenSync</a>?</p>

<p><b>A:</b> No. See <a href="opensync.php">SynCE and OpenSync</a>.</p>

<hr size=1 width="50%">

<p><b>Q:</b> Tasks that were completed on my PDA were copied to evolution as
uncompleted. Why?</p>

<p><b>A:</b> Is this still a problem in SynCE 0.9.0?</p>

<hr size=1 width="50%">

<p><b>Q:</b> When I synchronize to Evolution, the appointment times in my
calendar are displayed in GMT instead of my timezone.</p>

<p><b>A:</b> Please try the <tt>rra</tt> module from <a href="svn.php">Subversion</a>.</p>

<hr size=1 width="50%">

<p><b>Q:</b> How do I use the <tt>synce-matchmaker</tt> tool to create a
partnership between my PC and my device?</p>

<p><b>A:</b> To create a partnership it's simply this:</p>

<blockquote><tt>synce-matchmaker create</tt></blockquote>

<p>If you get the message "Partnership creation succeeded" everything is ok.</p>

<p>If you get the message "Partnership creation failed" you need to replace an
existing partnership on your device. To do this you first run
<tt>synce-matchmaker status</tt> to list the current partnerships on your
device. Decide which partnership (1 or 2) you want to replace. Second you run
<tt>synce-matchmaker replace X</tt>, where X is the partnership number.</p>

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

<p><br/>Return to <a href="multisync_guide.php">MultiSync guide</a>.</p>

</div>
<?php include 'footer.php'; ?>
