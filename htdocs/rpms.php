<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="index.php">main page</a>.</p>

<h1>SynCE - installing RPM packages</h1>

<!--

<h2>If you want to use APT to install packages</h2>

<p>More about using APT with RedHat Linux is available at <a href="http://freshrpms.net/apt/">freshrpms.net</a>.</p>

<ol>

<li>Make sure you are running as the root user.</li>

<li>Add these lines to your sources.list:<br>
<pre>rpm      http://synce.sourceforge.net/apt  redhat/en/i386/8.0 synce
rpm-src  http://synce.sourceforge.net/apt  redhat/en/i386/8.0 synce</pre></li>

<li class=SPACED>Run <code>apt-get update</code> to update your file lists.</li>

<li class=SPACED>Run <code>apt-get install 'synce-.*'</code> to install all SynCE packages.</li>

</ol>

<h2>If you want to download and install packages manually</h2>

-->

<p>The RPM packages are for normal users only. Developers should compile the
SynCE modules from source code.</p>

<p><b>Note:</b> X.X-X is used as version number below. You will have to
substitute this for the version you are installing.</p>

<ol>

<li class=SPACED>Visit the <a href="http://sourceforge.net/project/showfiles.php?group_id=30550"
target=_blank>project filelist</a> (will be opened in a separate window) and download
the following RPM packages:<br/><br/>

	<ul>
		<li><b>synce-X.X-X.i386.rpm</b>  (Note: this package includes the modules libsynce, librapi2, dccm, serial and rra.)</li>
<!--		<li><b>synce-devel-X.X-X.i386.rpm</b>  (for developers, or if you will compile other modules from source code)</li> -->
		<li><b>synce-multisync_plugin-X.X-X.i386.rpm</b> (for address book synchronization)</li>
		<li><b>synce-trayicon-X.X-X.i386.rpm</b> (suggested for GNOME 2.x users)</li>
		<li><b>synce-gnomevfs-X.X-X.i386.rpm</b> (suggested for GNOME 2.x users)</li>
	</ul>
</li>

<li class=SPACED>Make sure you are running as the root user.</li>

<li class=SPACED>Install the packages:<br/>
<pre>rpm -Uvh synce*.rpm</pre>
</li>

<li class=SPACED>If you want to use a module not available as RPM, please <a href="tarballs.php">download and compile the source code</a>.</li>

</ol>

<p>Return to <a href="index.php">main page</a>.</p>

</div>
<?php include 'footer.php'; ?>
