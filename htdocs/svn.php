<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="index.php">main page</a>.</p>

<h1>SynCE - Getting SynCE from Subversion</h1>

<p>Read the <a href="http://sourceforge.net/svn/?group_id=30550"
target="_blank">SourceForge Subversion instructions</a> first. (Will open in a
separate window.) Then get these mandatory modules:</p>

<ul>

<li><b>libsynce</b></li>
<li><b>librapi2</b></li>
<li><b>dccm</b></li>
<li><b>serial</b></li>

</ul>

<p>And maybe these optional modules:</p>

<ul>

<li><b>rra</b> (for synchronization) - requires <a
href="http://sourceforge.net/project/showfiles.php?group_id=30550">libmimedir</a></li>

<li><b>multisync_plugin</b> (for synchronization with MultiSync)</li>
<li><b>trayicon</b> (to get a GNOME tray icon)</li>
<li><b>gnomevfs</b> (to get a GNOME virtual file system)</li>
<li><b>rapip</b> (to get support for KDE)</li>

</ul>

<p>Before continuing, make sure that you have correct versions of the
GNU "auto-tools". These versions are known to work:</p>

<ul>

<li>GNU autoconf 2.57</li>
<li>GNU automake 1.6.3</li>
<li>GNU libtool 1.4.3</li>

</ul>

<p>For each module, run the <tt>bootstrap</tt> script. The execution of this
command looks like this for the <b>libsynce</b> module:</p>

<pre><b>$ ./bootstrap</b>
Creating configure.ac...done.
Creating libsynce.spec...done.
+ aclocal -I m4
+ autoheader
+ libtoolize --copy --automake
+ automake --copy --foreign --add-missing
configure.ac: installing `./install-sh'
configure.ac: installing `./mkinstalldirs'
configure.ac: installing `./missing'
lib/Makefile.am: installing `./depcomp'
+ autoconf</pre>

<p>See the <a href="tarballs.php">Installation from .tar.gz files</a> page for
more details about compiling the modules.</p>

<p>Return to <a href="index.php">main page</a>.</p>

</div>
<?php include 'footer.php'; ?>
