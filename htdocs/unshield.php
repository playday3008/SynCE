<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="index.php">main page</a>.</p>

<h1>SynCE - Unshield</h1>

<h2>About Unshield</h2>

<p>Extract from the <a
href="http://cvs.sourceforge.net/cgi-bin/viewcvs.cgi/synce/unshield/README?rev=HEAD&content-type=text/vnd.viewcvs-markup">README</a>:</p>

<p>To install a Pocket PC application remotely, an installable
Microsoft Cabinet File is copied to the /Windows/AppMgr/Install
directory on the PDA and then the wceload.exe is executed to
perform the actual install. That is a very simple procedure.</p>

<p>Unfortunately, many applications for Pocket PC are distributed as <a
href="http://installshield.com/">InstallShield</a> installers for Microsoft
Windows, and not as individual Microsoft Cabinet Files. That is very
impractical for users of other operating systems, such as Linux or FreeBSD.</p>

<p>An installer created by the InstallShield software stores the
files it will install inside of InstallShield Cabinet Files. It
would thus be desirable to be able to extract the Microsoft
Cabinet Files from the InstallShield Cabinet Files in order to be
able to install the applications without access to Microsoft
Windows.</p>

<p>The format of InstallShield Cabinet Files is not officially
documented but there are two tools available for Microsoft
Windows that extracts files from InstallShield installers, and
they are distributed with source code included. These tools are
named "i5comp" and "i6comp" and can be downloaded from the
Internet.</p>

<p>One major drawback with these tools are that for the actual decompression of
the files stored in the InstallShield Cabinet Files they require the use of
code written by InstallShield that is not available as source code. Luckily, by
examining this code with the 'strings' tool, I discovered that they were using
the open source zlib library (<a
href="http://www.gzip.org/zlib/">www.gzip.org/zlib</a>) for decompression.</p>

<p>I could have modified i5comp and i6comp to run on other operating
systems than Microsoft Windows, but I preferred to use them as a
reference for this implementation. The goals of this
implementation are:</p>

<ul> <li>Use a well known open source license (<a
href="http://opensource.org/licenses/mit-license.php">MIT</a>)</li>

<li>Work on both little-endian and big-endian systems</li>

<li>Separate the implementation in a tool and a library</li>

<li>Support InstallShield versions 5 and later</li>

<li>Be able to list contents of InstallShield Cabinet Files</li>

<li>Be able to extract files from InstallShield Cabinet Files</li>
</ul>

<h2>Download</h2>

<p>Unshield is still under development and is currently only available as the
<tt>unshield</tt> module in the SynCE <a
href="http://synce.sourceforge.net/synce/cvs.php">CVS</a>. It depends on the
<tt>libsynce</tt> module.</p>

<p>Return to <a href="index.php">main page</a>.</p>

</div>
<?php include 'footer.php'; ?>
