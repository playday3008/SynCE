<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="index.php">main page</a>.</p>

<h1>SynCE - installing Debian packages</h1>

<p>SynCE is not available in the Debian distribution and can therefore not yet
be installed with <tt>apt-get</tt>. These packages have been provided by <a
href="mailto:thomas@i.rule-the.net">Thomas Butter</a> and are made for Debian
<i>sid</i> ("unstable").</p>

<p><b>Note:</b> X.X-X is used as version number below. You will have to
substitute this for the version you are installing.</p>

<ol>

<li class=SPACED>Visit the <a href="http://sourceforge.net/project/showfiles.php?group_id=30550"
target=_blank>project filelist</a> (will be opened in a separate window) and download
the following Debian packages:<br/><br/>

<b>librapi2_X.X-X_i386.deb</b><br/>
<b>librapi2-dev_X.X-X_i386.deb</b> (Needed if you want to compile some other module from source code.)<br/>
<b>librapi2-tools_X.X-X_i386.deb</b><br/>
<b>libsynce0_X.X-X_i386.deb</b><br/>
<b>libsynce0-dev_X.X-X_i386.deb</b> (Needed if you want to compile some other module from source code.)<br/>
<b>synce-dccm_X.X-X_i386.deb</b><br/>
<b>synce-rra_X.X-X_i386.deb</b> (Only needed for the MultiSync plugin, which is not yet available as a Debian package.)<br/>
<b>synce-serial_X.X-X_i386.deb</b><br/>
<b>synce-trayicon_X.X-X_i386.deb</b> (Recommended for GNOME 2 users.)<br/>

</li>

<li class=SPACED>Make sure you are running as the root user.</li>

<li>Install the packages:<br/>
<pre>dpkg -i *.deb</pre></li>

<li class=SPACED>If you want to use a module not available as a Debian package,
please <a href="tarballs.php">download and compile the source code</a>.</li>

</ol>

<p>Return to <a href="index.php">main page</a>.</p>

</div>
<?php include 'footer.php'; ?>
