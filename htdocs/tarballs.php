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

    <li class=SPACED><p><b>synce-X.X.tar.gz</b> for SynCE 0.8 or later includes
    the following .tar.gz. archives. They can also be downloaded
    separately.</p>
    
    <ul>
      <li><b>synce-librapi2-X.X.tar.gz</b></li>
      <li><b>synce-synce-X.X.tar.gz</b></li>
      <li><b>synce-dccm-X.X.tar.gz</b></li>
      <li><b>synce-serial-X.X.tar.gz</b></li>
      <li><b>synce-rra-X.X.tar.gz</b> (optional)</li>
      <li><b>libmimedir-X.X.tar.gz</b> (only in SynCE 0.8 or later)</li>
      </ul>
    </li>
		<li><b>synce-multisync_plugin-X.X.tar.g</b>z (for address book synchronization, requires rra)</li>
		<li><b>synce-trayicon-X.X.tar.gz</b> (suggested for GNOME 2.x users)</li>
		<li><b>synce-gnomevfs-X.X.tar.gz</b> (suggested for GNOME 2.x users)</li>
		<li><b>rapip-X.X.tar.gz</b> (suggested for KDE 3.x users)</li>
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

<li>Compile libmimedir:<br/>
<pre>tar zxf synce-libmimedir-X.X.tar.gz
cd libmimedir-X.X
./configure
make
make install
cd ..</pre>
</li>

<li>Compile rra:<br/>
<pre>tar zxf synce-rra-X.X.tar.gz
cd synce-rra-X.X
./configure
make
make install
cd ..</pre>
</li>

<li>Compile multisync_plugin:<br/>
<p>Note that this module requires the source code for Multisync.</p>
<pre>tar zxf synce-multisync_plugin-X.X.tar.gz
cd synce-multisync_plugin-X.X
./configure --with-multisync-source=<i>/path/to/multisync/source/code/</i>
make
make install
cd ..</pre>
</li>

<li>Compile trayicon:<br/>
<p>Note that this module requires a whole lot of GNOME 2 development packages.</p>
<pre>tar zxf synce-trayicon-X.X.tar.gz
cd synce-trayicon-X.X
./configure
make
make install
cd ..</pre>
</li>

<li>Compile gnomevfs:<br/>
<p>Note that this module requires a whole lot of GNOME 2 development packages.</p>
<pre>tar zxf synce-gnomevfs-X.X.tar.gz
cd synce-gnomevfs-X.X
./configure
make
make install
cd ..</pre>
</li>

<li>Compile rapip:<br/>
<p>Note that this module requires a whole lot of KDE 3.x development packages.</p>
<pre>tar zxf rapip-X.X.tar.gz
cd rapip-X.X
./configure
make
make install
cd ..</pre>
</li>

</ol>

<p>Return to <a href="index.php">main page</a>.</p>

</div>
<?php include 'footer.php'; ?>
