<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="index.php">main page</a>.</p>

<h1>SynCE - Unshield</h1>

<h2>About</h2>

<p>Extract from the README:</p>

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

<p><a href="http://packages.qa.debian.org/u/unshield.html">Debian
packages</a>.</p>

<p>See the <a
href="https://sourceforge.net/project/showfiles.php?group_id=30550&package_id=125523">Unshield
package</a> for the latest release, or the <tt>unshield</tt> module in
Subversion for the bleeding edge source code!</p>

<h2>InstallShield cabinet files vs. Microsoft cabinet files</h2>

<p>There are two types of .CAB files: InstallShield cabinet files and
Microsoft cabinet files. Unshield only supports the InstallShield cabinets,
  usually named <tt>data1.cab</tt>, <tt>data1.hdr</tt>, <tt>data2.cab</tt>,
  etc.</p>

<p>Microsoft cabinet files can be extracted with Stuart Caie's excellent <a
href="http://www.kyz.uklinux.net/cabextract.php">cabextract</a> tool.</p>

<br>

<p>Return to <a href="index.php">main page</a>.</p>

</div>
<?php include 'footer.php'; ?>
