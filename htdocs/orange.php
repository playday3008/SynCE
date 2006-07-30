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
installable Microsoft Cabinet Files. That is very impractical for users of
other operating systems, such as Linux or FreeBSD.</p>

<p><b>Orange</b> is a tool and library for squeezing out juicy installable
Microsoft Cabinet Files from self-extracting installers for Microsoft Windows
and some other installer file formats.  It currently supports the following
kinds of installers:</p>

<ul>

<li>Self-extracting installers (<b>EXE</b>):<ul>



<li>Early support for installers created by <a
href="http://www.mindvision.com">Installer VISE</a>.</li>

<li>Support for some installers created by <a href="http://innosetup.com">Inno
Setup</a>.</li>

<li><a href="http://www.indigorose.com">Setup Factory</a> versions 5 and 6
using <a href="dynamite.php">Dynamite</a> for data decompression.</li>

<li>Some other installer (using a DLL called inflate.dll). This is used by for
example <a
href="http://www.macromedia.com/software/flashplayer/pocketpc/2002.html">Macromedia
Flash Player 6 for Pocket PC 2002</a>.</li>

<li><a href="http://installshield.com">InstallShield</a> self-extracting
installers.</li>

</ul></li>

<li>Other formats:<ul>

<li><b>APK, ARH</b> Early support for the installer used by <a
href="http://www.tomtom.com">TomTom</a> products.</li>

<li><b>CAB</b> <a href="http://installshield.com">InstallShield</a> Cabinet files, using
<a href="unshield.php">Unshield</a>.</li>

<li><b>CAB</b> Microsoft Cabinet files (also self-extracting) when you have Stuart Caie's
excellent <a href="http://www.kyz.uklinux.net/cabextract.php">Cabextract</a>
tool installed.</li>

<li><b>MSI</b> Microsoft Installer files, if libole2 is available.</li>

<li><b>RAR</b> archives (also self-extracting) when you have Unrar installed.</li>

<li><b>ZIP</b> archives (also self-extracting) when you have <a
href="http://www.info-zip.org/UnZip.html">UnZip</a> installed.</li>

</ul></li>
</ul>

<p>When Orange has extracted an installable Microsoft Cabinet File, you can use
<tt>synce-install-cab</tt> to install the program on your PDA.</p>

<p>If you find an installer that Orange fails to handle and you have both
Cabextract and UnZip installed, write to <a
href="mailto:synce-devel@lists.sourceforge.net">synce-devel@lists.sourceforge.net</a>
and provide a link to where it can be downloaded.</p>

<h2>Download</h2>

<p>See the <a
href="https://sourceforge.net/project/showfiles.php?group_id=30550&package_id=157355">Orange
package</a> for the latest release, or the <tt>orange</tt> module in Subversion
for the bleeding edge source code!</p>

<br>

<p>Return to <a href="index.php">main page</a>.</p>

</div>
<?php include 'footer.php'; ?>
