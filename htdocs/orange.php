<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="index.php">main page</a>.</p>

<h1>SynCE - Orange</h1>

<h2>About</h2>

<p>To install a Pocket PC application remotely, an installable Microsoft
Cabinet File is copied to the <tt>\Windows\AppMgr\Install</tt> directory on the
PDA and then the <tt>wceload.exe</tt> program is executed to perform the actual
install. That is a very simple procedure.</p>

<p>Unfortunately, many applications for Pocket PC are distributed as
self-extracting installers for Microsoft Windows, and not as individual
installablre Microsoft Cabinet Files. That is very impractical for users of
other operating systems, such as Linux or FreeBSD.</p>

<p><b>Orange</b> is a tool and library for squeezing out juicy installable
Microsoft Cabinet Files from self-extracting installers for Microsoft Windows.
It currently supports the following kinds of installers:</p>

<ul>

<li><a href="http://installshield.com">InstallShield</a> versions 5 and 6,
using <a href="unshield.php">Unshield</a></li>

<li><a href="http://www.indigorose.com">Setup
Factory</a> versions 5 and 6 using <a href="dynamite.php">Dynamite</a></li>

<li>Microsoft Cabinets (also self-extracting) when you have <a
href="www.kyz.uklinux.net/cabextract.php">Cabextract</a> installed</li>

<li>Zip files (also self-extracting) when you have <a
href="http://www.info-zip.org/UnZip.html">UnZip</a> installed</li>

</ul>

<h2>Download</h2>

<p>See the <b>Tools</b> package in the <a
href="http://sourceforge.net/project/showfiles.php?group_id=30550">SynCE File
List</a>.</p>

<br>

<p>Return to <a href="index.php">main page</a>.</p>

</div>
<?php include 'footer.php'; ?>
