<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="index.php">main page</a>.</p>

<h1>Installation from .tar.gz files</h1>

<p><b>Note:</b> X.X is used as version number below. You will have to
substitute this for the version you are installing.</p>

<ol>

<li class=SPACED>Visit the <a href="http://sourceforge.net/project/showfiles.php?group_id=30550"
target=_blank>project filelist</a> (will be opened in a separate window) and download
the following .tar.gz files:<br/>

	<ul>
		<li>synce-librapi2-X.X.tar.gz</li>
		<li>synce-synce-X.X.tar.gz</li>
		<li>synce-dccm-X.X.tar.gz</li>
		<li>synce-serial-X.X.tar.gz</li>
		<li>synce-trayicon-X.X.tar.gz (if you want it :-)</li>
	</ul>
</li>


<li>Compile libsynce:<br/>
<pre>tar zxf synce-libsynce-X.X.tar.gz
cd synce-libsynce-X.X
./configure
make
make install
cd ..</pre>
</li>

<li>Compile librapi2:<br/>
<pre>tar zxf synce-librapi2-X.X.tar.gz
cd synce-librapi2-X.X
./configure
make
make install
cd ..</pre>
</li>

<li>Compile dccm:<br/>
<pre>tar zxf synce-dccm-X.X.tar.gz
cd synce-dccm-X.X
./configure
make
make install
cd ..</pre>
</li>

<li>Compile serial:<br/>
<pre>tar zxf synce-serial-X.X.tar.gz
cd synce-serial-X.X
./configure
make
make install
cd ..</pre>
</li>

<li>Compile trayicon:<br/>
<p>Note that requires a whole lot of GNOME 2 development packages.</p>
<pre>tar zxf synce-trayicon-X.X.tar.gz
cd synce-trayicon-X.X
./configure
make
make install
cd ..</pre>
</li>

</ol>

<p>Return to <a href="index.php">main page</a>.</p>

</div>
<?php include 'footer.php'; ?>
