<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="../index.php">main page</a>.</p>

<h1>KDE-Integration of SynCE</h1>

<h2>Overview</h2>
<p>
One effort in integrating SynCE into KDE is the rapip/raki project.
The name rapip/raki will change in the next revision of the project.
<ul>
<li><b>Rapip</b> is the short form of &quot;<b>RAPI</b>-<b>P</b>rotocol&quot;. Rapip itself
is the io-slave used to browse through the PDA-Filesystem and copy files to and from the PDA.
</li>
<li><b>Raki</b> is a synonym for &quot;<b>R</b>emote-<b>A</b>ccess-<b>KI</b>cker-applet&quot;.
Like the name suggests it has first been an applet for kicker, the KDE panel but as time goes by 
the application-type changes. Today raki is an application docking into the KDE system-tray. 
It is responsible for all additional tasks which are desired for interacting 
with the PDA via the desktop.
</li>
</ul>
</p>

<h2>Features</h2>
<dl>
  <dt><b>Rapip (io-slave):</b></dt>
  <ul>
  <li>... copies files and directories via drag and drop to and from the PDA.</li>
  <li>... is capable to handle mime-types.</li>
  <li>... supports more than one connected device simultanously.</li>
  </ul><br>
  <dt><b>Raki:</b></dt>
  <ul>
  <li>... also supports more than one connected PDA.</li>
  <li>... installes cab-files on the device - you just have to drag them over the system-tray.</li>
  <li>... creates a NAT-Route for your PDA to the outer internet.</li>
  <li>... lets you manage the software installed on the PDA.</li>
  <li>... displays system information and power status about the PDA.</li>
  <li>... starts programs on the PDA.</li>
  <li>... answers password-requests of PDAs automatically.</li>
  <li>... synchronizes the contact-database of the PDA and the KDE-addressbook.</li>
  <li>... notifies about connections, password requests and disconnections of
  PDAs with sounds.</li>
  </ul>
</dl>

<h2>Requirements</h2>
<p>
Rapip/raki requires 
<ul><li>KDE-3.1</li>
<li>Qt &gt;= Qt-3.1</li>
<li>and the SynCE libraries version 0.7</li>
</ul>
</p>
<h2>Installation</h2>
<p>
First of all: Read the <tt>INSTALL</tt>-file. There you will find
the most actual installation instructions.<br>
The next source of information is the <tt>ChangeLog</tt>-file.<br><br>
Download rapip/raki (rapip-0.x.tar.gz) from the 
<a href="http://sourceforge.net/projects/synce/">SourceForge Project Page</a> or check out
the latest code from the <a href="http://sourceforge.net/cvs/?group_id=30550">CVS repository</a>.
<br><br>
Currently there are two development lines in the CVS repository:
<ol>
  <li>Main-trunk: Stable version but fewer features.</li>
  <li>MULTIPLE_DEVICES branch: Actual development branch. Many features.</li>
</ol>
After having downloaded a rapip-package execute commands in the following order:
<ul>
<li><tt>tar -xvzf rapip-0.x.tar.gz</tt></li>
<li><tt>cd rapip-0.x</tt></li>
<li><tt>make -f Makefile.cvs</tt></li>
<li><tt>./configure</tt></li>
<li><tt>make</tt></li>
<li><tt>make install</tt> (as root)</li>
<li><tt>cp raki/raki.sh ~/.synce/scripts/</tt> (for every rapip/raki user)</li>
<li><tt>chmod 755 ~/.synce/scripts/raki.sh</tt> (also for every user who wants to use rapip/raki)</li>
</ul>
</p>

<h2>Usage</h2>
<ul>
<li><b>Rapip:</b> Open konqueror and type <tt>rapip:/</tt> into the URL-Line. You should see the 
root-directory of your PDA</li>
<li><b>Raki:</b> Launch the application from the &quot;KMenu-&gt;Utilities&quot;-Menu. You will
 get a new icon located in the system-tray. </li>
</ul>
<h2>Screenshots</h2>
These are from the "MULTIPLE_DEVICES" CVS Branch<br><br>
<table>
  <tbody>
    <tr>
      <td>
	  	<a href="images/menustructure-multiple-devices.png">
				<img src="images/menustructure-multiple-devices-300.png" width="300" border="0">
		</a>
	  </td>
      <td>
	  	<a href="images/main-configure-dialog.png">
				<img src="images/main-configure-dialog-300.png" width="300" border="0">
		</a>
	  </td>
    </tr>
    <tr>
      <td> <div align="center">More than one PDA is connected<br>vLoox is selected</div> </td>
      <td> <div align="center">Main Configuration Dialog</div> </td>
    </tr>
    <tr>
      <td> <p></p> </td>
      <td> <p></p> </td>
    </tr>
    <tr>
      <td> 
	  	<a href="images/pda-config.png">
				<img src="images/pda-config-300.png" width="300" border="0">
		</a>
	  </td>
      <td>
	  	<a href="images/konqueror.png">
				<img src="images/konqueror-300.png" width="300" border="0">
		</a>
	  </td>
    </tr>
    <tr>
      <td> <div align="center">Configuration of a specific PDA</div> </td>
      <td> <div align="center">Konqueror in action browsing vLoox</div>  </td>
    </tr>
    <tr>
      <td> <p></p> </td>
      <td> <p></p> </td>
    </tr>
    <tr>
      <td> 
	  	<a href="images/replication.png" rel="300">
				<img src="images/replication-300.png" width="300" border="0">
		</a>
	  </td>
      <td>
	  	<a href="images/execute.png" rel="300">
				<img src="images/execute-300.png" width="300" border="0">
		</a>
	  </td>
    </tr>
    <tr>
      <td> <div align="center">Replication of PDA-Databases<br>(Now only "Contacts"
	  is working) 
	  </div> </td>
      <td> <div align="center">Execution of a program on the PDA</div> </td>
    </tr>
    <tr>
      <td> <p></p> </td>
      <td> <p></p> </td>
    </tr>
	<tr>
      <td> 
	  	<a href="images/select-pda-for-install.png" rel="300">
				<img src="images/select-pda-for-install-300.png" width="300" border="0">
		</a>
	  </td>
      <td>
	  	<a href="images/install.png" rel="300">
				<img src="images/install-300.png" width="300" border="0">
		</a>
	  </td>
    </tr>
    <tr>
      <td> <div align="center">Selection of an installation destination</div> </td>
      <td> <div align="center">Installation process</div> </td>
    </tr>
    <tr>
      <td> <p></p> </td>
      <td> <p></p> </td>
    </tr>
    <tr>
      <td> 
	  	<a href="images/sysinfo.png" rel="300">
				<img src="images/sysinfo-300.png" width="300" border="0">
		</a>
	  </td>
      <td>
	  	<a href="images/powerstatus.png" rel="300">
				<img src="images/powerstatus-300.png" width="300" border="0">
		</a>
	  </td>
    </tr>
    <tr>
      <td> <div align="center">System information</div> </td>
      <td> <div align="center">Power status</div> </td>
    </tr>
    <tr>
      <td> <p></p> </td>
      <td> <p></p> </td>
    </tr>
    <tr>
      <td> 
	  	<a href="images/software-manager.png" rel="300">
				<img src="images/software-manager-300.png" width="300" border="0">
		</a>
	  </td>
	  <td>  
		<a href="images/password.png" rel="300">
				<img src="images/password-300.png" width="300" border="0">
		</a>
	  </td>
    </tr>
    <tr>
      <td> <div align="center">Software Manager</div> </td>
      <td> <div align="center">Password-request dialog</div> </td>
    </tr>
  </tbody>
</table>
<p>Return to <a href="../index.php">main page</a>.</p>

</div>
<?php include 'footer.php'; ?>
