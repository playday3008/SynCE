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
<li><a href="#overview">Overview</a></li>
<li><a href="architecture.php">Architecture</a></li>
<li><a href="using.php">Download and use SynCE</a></li>
<li><a href="#future">Future</a></li>
<li><a href="#developers">Developers</a></li>
<li><a href="http://sourceforge.net/projects/synce/">SourceForge Project Page</a></li>
<li><a href="links.php">Links</a></li>
</ul>

<a name="introduction"></a>
<h2>Introduction</h2>

<p>The purpose of the SynCE project is to provide a means of communication
with a Windows CE or Pocket PC device from a computer running Linux,
*BSD or other unices.</p>

<a name="news"></a>
<h2>News</h2>

<p><b>Today</b>. Working on a new homepage!</p>

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

</table>

<a name="future"></a>
<h2>Plans for the future</h2>

<p>Future plans for SynCE include synchronization support.</p>

<p>Please read the <a href="http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/synce/twogood-files/roadmap.txt?rev=HEAD&amp;content-type=text/vnd.viewcvs-markup">roadmap</a> for more details.</p>

<a name="developers"></a><h2>Developers</h2>

<p><a href="mailto:llange@users.sourceforge.net">Ludovic Lange</a> started the
SynCE project and did the hard work in figuring out the properitary
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
