<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="index.php">main page</a>.</p>

<h1>SynCE - installing Debian packages</h1>

<p>SynCE is not yet available in the Debian distribution but we maintain an apt
repository on this web site. These packages have been provided by <a
href="mailto:tbutter@users.sourceforge.net">Thomas Butter</a>.</p>

<ol>

<li>Update your sources.list:

<ul>

<li><p><b>For Debian stable (woody)</b>

<p>Add this line to your /etc/apt/sources.list</p>

<pre>
deb     http://synce.sourceforge.net/debian/woody ./
</pre>
</li>

<li><p><b>For Debian testing (sarge) and unstable (sid)</b></p>

<p>Add this line to your /etc/apt/sources.list</p>

<pre>
deb     http://synce.sourceforge.net/debian/sid ./
</pre>
</li>
</ul>

<li><p>Update your package lists:</p>

<pre>apt-get update</pre>
</li>

<li><p>Install SynCE:</p>

<pre>apt-get install synce-dccm synce-serial librapi2-tools</pre>

<p>(This should implicitly install the <tt>librapi2</tt> and <tt>libsynce0</tt>
packages.)</p>
</li>

<li><p>Install development packages. This is only needed if you want to compile
some SynCE module from source, or if you want to develop your own tools for
SynCE.</p>

<pre>apt-get install libsynce0-dev librapi2-dev</pre>

<!-- <p>(This should implicitly install the <tt>libsynce0-dev</tt> package.)</p> -->

</li>

<li><p>Install the GNOME tray icon if you are running GNOME 2 on Debian testing
or unstable:</p>

<pre>apt-get install synce-trayicon</pre>

</li>

<li class=SPACED>If you want to use a module not available as a Debian package,
please <a href="tarballs.php">download and compile the source code</a>.</li>

</ol>

<p>Return to <a href="index.php">main page</a>.</p>

</div>
<?php include 'footer.php'; ?>
