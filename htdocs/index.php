<?php include 'header.php'; ?>

<div class="PAGE">

<A href="http://sourceforge.net"><IMG
src="http://sourceforge.net/sflogo.php?group_id=30550&amp;type=5" width="210"
height="62" border="0" alt="SourceForge Logo" align=right></A> 

<h1>The SynCE Project</h1>

<h2>Contents</h2>

<ul>
<li><a href="#introduction">Introduction</a></li>
<li><a href="#news">News</a></li>
<li><a href="#helpwanted">Help wanted!</a></li>
<li><a href="#overview">Overview</a></li>
<li><a href="architecture.php">Architecture</a></li>
<li><a href="using.php">Download and use SynCE</a></li>
<li><a href="#future">Future</a></li>
<li><a href="#developers">The SynCE developers</a></li>
<li><a href="capture.php">How to capture packets for analyzing</a></li>
<li><a href="http://sourceforge.net/mail/?group_id=30550">Mailing list information</a></li>
<li class=SPACED><a href="links.php">Links</a></li>
<li><a href="http://sourceforge.net/projects/synce/">SourceForge Project Page</a></li>
</ul>

<a name="introduction"></a>
<h2>Introduction</h2>

<p>The purpose of the SynCE project is to provide a means of communication
with a Windows CE or Pocket PC device from a computer running Linux,
*BSD or other unices.</p>

<a name="news"></a>
<h2>News</h2>

<p><b>November 28, 2002</b> Created mailing lists <a
href="http://lists.sourceforge.net/mailman/listinfo/synce-announce">synce-announce</a>
and <a
href="http://lists.sourceforge.net/mailman/listinfo/synce-users">synce-users</a>
in case <a
href="http://lists.sourceforge.net/mailman/listinfo/synce-devel">synce-devel</a>
gets to technical for someone.</p>

<p><b>November 24, 2002</b> Released SynCE version 2002-11-24 which includes
dccm 0.4, librapi2 0.4, libsynce 0.4, synce-serial 0.4 and trayicon 0.2.</p>

<p><b>November 21, 2002</b>. Added USB connection instructions to the <a
href="using.php">Download and use</a> page.</p>

<p><b>November 20, 2002</b>. SynCE-Serial 0.4 released. This should fix the
connection problems some people have experienced. <a
href="http://sourceforge.net/project/showfiles.php?group_id=30550&release_id=123266">Download</a>.</p>

<p><b>November 18, 2002</b>. SynCE 0.3.1 released, including the GNOME 2 tray
icon!</p>

<p><b>November 17, 2002</b>. New homepage released!</p>

<a name="helpwanted"></a>
<h2>Help wanted!</h2>

<p>We need some responsbile people who want to develop the following extensions
on top of the SynCE project:</p>

<ul>

<li class=SPACED>Debian packages: Of course we want to get into the Debian
distribution some day!</li>

<li class=SPACED>GNOME 2 integration: A <a
href="http://developer.gnome.org/doc/API/gnome-vfs/writing-modules.html">GNOME-VFS
module</a>.</li>

<li>KDE 3 integration: A KDE I/O slave for KDE 3. (There is already an <a
href="http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/synce/kio_synce/">I/O
slave for KDE 2</a>, you just need to make it work with KDE 3.)</li>

</ul>

<a name="overview"></a>
<h2>Overview</h2>

<p>SynCE is composed by the following parts:</p>

<table>

<tr>
<th>libsynce</th>
<td>Common functions used by other parts of SynCE.</td>
</tr>

<tr>
<th>librapi2</th>
<td>A library that implements RAPI, the Remote Access Programming
Interface, which allows remote control of a device connected through SynCE.
Also included are simple command line tools that uses RAPI.</td>
</tr>

<tr>
<th>dccm</th>
<td>An application that maintains the connection with a device
connected through SynCE.</td>
</tr>

<tr>
<th>serial</th>
<td>Tools for configuring, starting and aborting a serial connection for 
use with SynCE.</td>
</tr>

<tr>
<th>trayicon</th>
<td>A tray icon for GNOME 2 showing if a device is connected or not.</td>
</tr>

</table>

<a name="future"></a>
<h2>Plans for the future</h2>

<p>Future plans for SynCE include synchronization support.</p>

<p>Please read the <a href="http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/synce/twogood-files/roadmap.txt?rev=HEAD&amp;content-type=text/vnd.viewcvs-markup">roadmap</a> for more details.</p>

<a name="developers"></a><h2>The SynCE developers</h2>

<p><a href="mailto:llange@users.sourceforge.net">Ludovic Lange</a> started the
SynCE project and did the hard work in figuring out manu of the properitary
interfaces.</p>

<p><a href="mailto:twogood@users.sourceforge.net">David Eriksson</a> is the
current project manager and most active developer. Also the author of these
simple web pages.</p>

<p><a href="mailto:hippy@users.sourceforge.net">Richard Taylor</a> is working
on PyRAPI, Python wrappers for RAPI.</p>

<p><a href="mailto:vganesh@users.sourceforge.net">Ganesh Varadarajan</a> has
developed serial-over-USB driver for SynCE, both userspace and kernel
versions.</p>

</div>
<?php include 'footer.php'; ?>
