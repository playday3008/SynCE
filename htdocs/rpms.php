<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="index.php">main page</a>.</p>

<h1>SynCE - installing RPM packages</h1>

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

<p><b>Note:</b> X.X-X is used as version number below. You will have to
substitute this for the version you are installing.</p>

<ol>

<li class=SPACED>Visit the <a href="http://sourceforge.net/project/showfiles.php?group_id=30550"
target=_blank>project filelist</a> (will be opened in a separate window) and download
the following RPM packages:<br/>

	<ul>
		<li>synce-librapi2-X.X-X.i386.rpm</li>
		<li>synce-synce-X.X-X.i386.rpm</li>
		<li>synce-dccm-X.X-X.i386.rpm</li>
		<li>synce-serial-X.X-X.noarch.rpm</li>
	</ul>
</li>

<li class=SPACED>Make sure you are running as the root user.</li>

<li class=SPACED>Install the packages:<br/>
<pre>rpm -Uvh synce*.rpm</pre>
</li>

</ol>

<p>Return to <a href="index.php">main page</a>.</p>

</div>
<?php include 'footer.php'; ?>
