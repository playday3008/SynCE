<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="index.php">main page</a>.</p>

<h1>SynCE - Architecture</h1>

<p>This information is probably most useful for contributors and developers.</p>

<h2>Contents</h2>

<ul>
<li><a href="#overview">Overview</a></li>
<li><a href="#dccm">The DCCM application</a></li>
<li><a href="#librapi2">The librapi2 library</a></li>
<li><a href="#libsynce">The libsynce library</a></li>
<li><a href="#rra">The rra module</a></li>
<li><a href="#synce-serial">The synce-serial scripts</a></li>
<li><a href="#ports">Registered IP ports used by SynCE</a></li>
<li><a href="#questions">Questions?</a></li>
</ul>

<a name="overview"></a>
<h2>Overview</h2>

<p>SynCE is composed by the following parts:</p>

<table>

<tr>
<th>libsynce</th>
<td>Common functions used by other parts of SynCE.</td>
</tr>

<tr>
<th>librapi2</th>
<td>A library that implements RAPI, the Remote Access Programming
Interface, which allows remote control of a device connected through SynCE.
Also included are simple command line tools that uses RAPI.</td>
</tr>

<tr>
<th>dccm</th>
<td>An application that maintains the connection with a device
connected through SynCE.</td>
</tr>

<tr>
<th>serial</th>
<td>Tools for configuring, starting and aborting a serial connection for 
use with SynCE.</td>
</tr>

<tr>
<th>rra</th>
<td>A library needed for synchronization functions.</td>
</tr>

<tr>
<th>multisync_plugin</th>
<td>A plugin for <a href="http://multisync.sourceforge.net">MultiSync</a> to
synchronize Contacts.</td> 
</tr>

<tr>
<th>trayicon</th>
<td>A tray icon for GNOME 2 showing if a device is connected or not.</td>
</tr>

</table>


<a name="dccm"></a>
<h2>The DCCM application</h2>

<p>DCCM is named after the registered port name it listens to. See <a
href="#ports">Registered IP ports used by SynCE</a> for details.</p>

<p>This application listens on port 5679 for incomning connections from a
Windows CE device. This connection remains as long as the connection icon is
shown on the Windows CE device.</p>

<p>When a device has connected and authenticated, the file
<i>~/synce/active_connection</i> is written with information about the connected
device. This file may look like this:</p>

<pre># Modifications to this file will be lost next time a client connects to dccm

[dccm]
pid=18432

[device]
name=nisse
class=Palm PC2
hardware=Compaq iPAQ H3600
ip=192.168.131.201
port=1234
password=1234
key=36
</pre>

<p>DCCM is capable of running programs or scripts on certain events. The
following events are currently used:</p>

<ul>
<li>start - DCCM has started</li>
<li>stop - DCCM will stop</li>
<li>connect - A device has connected and authenticated</li>
<li>disconnect - A device has disconnected</li>
</ul>

<p>In order to receive these events, put your program, script, or symbolic link
to one of those in the <i>~synce/scripts/</i> directory.</p>

<p>There is a special page documenting the <a href="dccm.php">DCCM protocol</a>.</p>

<a name="librapi2"></a>
<h2>The librapi2 library</h2>

<p>Librapi2 is a library that implements RAPI, the Remote Application
Programming Interface. The RAPI function calls are <a
href="http://msdn.microsoft.com/library/en-us/wcesdkr/html/_wcesdk_CeRapiInit.asp">documented
by Microsoft</a> but not the transport protocol. The '2' in "librapi2" is there
because this is the second implementation.</p>

<p>Librapi2 reads <i>~/synce/active_connection</i> file (see <a
href="#dccm">DCCM</a>) to get the device IP address and then connects to port
990 on the Windows CE device to make RAPI calls.  This port is not registered
by Microsoft. (Actually it is registered for Secure FTP!)</p>

<p>All strings in RAPI calls are "encoded" as 2-byte UNICODE chars. Helper
functions for converting to and from this string type is provided in <a
href="#libsynce">libsynce</a>. Librapi2 will handle conversions between big
endian and little endian internally whenever possible.</p>

<p>Librapi2 comes with a number of file management tools that provide useful
example code in how to use the RAPI calls.</p>

<p>There is a special page documenting the <a href="rapi.php">RAPI protocol</a>.</p>

<a name="libsynce"></a>
<h2>The libsynce library</h2>

<p>Libsynce contains common functions needed by DCCM, Librapi2 and that also
may be used by 3rd party applications:</p>

<ul>
<li>Logging functions (synce_log.h)</li>
<li>TCP socket functions (synce_socket.h)</li>
<li>Declaration of datatypes used on Microsoft Windows</li>
<li>Conversion between WCHAR* and char* strings</li>
<li>Conversion between FILETIME and time_t data types</li>
<li>Conversion between big endian and little endian integers</li>
</ul>

<a name="rra"></a>
<h2>The RRA module</h2>

<p>The RRA module is used for synchronization. It implements the <a
href="rrac.php">RRAC protocol</a>.</p>

<a name="synce-serial"></a>
<h2>The synce-serial scripts</h2>

<h3>synce-serial-config</h3>

<p>This script simply writes the file <i>/etc/ppp/peers/synce-device</i> with a
suitable configuration for use with SynCE.</p>

<h3>synce-serial-start</h3>

<p>This scripts checks that the <i>usepeerdns</i> option is not present in
<i>/etc/ppp/options</i>, and then runs <i>pppd call synce-device</i> to allow
the device to connect.</p>

<h3>synce-serial-abort</h3>

<p>This script brutally kills pppd if it is running.</p>

<a name="ports"></a>
<h2>Registered IP ports used by SynCE</h2>

<p>Below are the port numbers registrered by Microsoft. SynCE only use the TCP ports.</p>

<pre>rrac            5678/tcp   Remote Replication Agent Connection
rrac            5678/udp   Remote Replication Agent Connection
dccm            5679/tcp   Direct Cable Connect Manager
dccm            5679/udp   Direct Cable Connect Manager
#                          Mark Miller &lt;mmiller@MICROSOFT.com&gt;</pre>

<a name="questions"></a>
<h2>Questions?</h2>

<p>Send an e-mail to <a
href="mailto:synce-devel@lists.sourceforge.net">synce-devel@lists.sourceforge.net</a>!</p>

<p>Return to <a href="index.php">main page</a>.</p>

</div>
<?php include 'footer.php'; TC?>
