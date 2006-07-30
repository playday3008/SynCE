<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="index.php">main page</a>.</p>

<h1>SynCE - Remote Replication Agent Connection</h1>

<p>As the contents of this document have been deduced by examining network
traffic and writing test programs, there are probably lots of errors.</p>

<p>Numeric values in the transferred data are stored in little-endian
format.</p>

<p>All values are given in hexadecimal form unless noted otherwise.</p>

<h1>Contents</h1>

<ul>

<li><a href="#connection">Connection</a></li>
<li><a href="#packetmap">Command packet map</a></li>
<li><a href="#commandheader">Command packet header</a></li>
<li><a href="#packet65">Command packet 65</a></li>
<li><a href="#packet66">Command packet 66</a></li>
<li><a href="#packet67">Command packet 67</a></li>
<li><a href="#packet69">Command packet 69</a></li>
<li><a href="#packet6c">Command packet 6c</a></li>
<li><a href="#packet6e">Command packet 6e</a></li>
<li><a href="#packet6f">Command packet 6f</a></li>
<li><a href="#packet70">Command packet 70</a></li>
<li><a href="#dataheader">Data header</a></li>
<li><a href="#datachunk">Data chunk</a></li>
<li><a href="#datastream">Data stream</a></li>

</ul>


<hr size=1 />
<a name="connection"></a>
<h2>Connection</h2>

<p>The desktop computer begin to listen on port 5678 for incoming connections.
It then makes the undocumented CeStartReplication RAPI function call to tell
the remote device to connect. The remote devices makes two connections to port
5678. The first connection is a command channel and the second connection is a
data channel.</p>

<p>The first command sent is usually <a href="#packet6f">command 6f,
subcommand c1</a> from desktop to handheld. This requests a list of object types
that can be synchronized.</p>

<p>The handheld device can at any time send a <a href="#packet69">command 69,
subcommand 0</a>, to inform that an object has been added, changed or deleted.</p>

<p>To add or modify objects on a handheld, the desktop simply writes the object
data to the data channel. The handheld responds with one <a
href="#packet65">command 65</a> for each object, specifying the id of the
object on the handheld. The desktop responds with a <a href="#packet65">command
65</a> to agree on the id.</p>


<hr size=1 />
<a name="dialogue"></a>
<h2>Synchronization dialogue</h2>

<h3>Initialization</h3>

<table cellspacing=8 width="100%">
<tr><th>From desktop</th><th>From handheld</th><th>Description</th></tr>

<tr><td>CeStartReplication()</td><td></td><td></td></tr>

<tr><td></td><td>TCP connection to port 5678</td><td>Command channel</td></tr>
<tr><td></td><td>TCP connection to port 5678</td><td>Data channel</td></tr>

<tr><td><a href="#packet6f">6f,&nbsp;subcommand&nbsp;c1</a></td><td></td>
<td>Request list of object types</td></tr>

<tr><td></td><td><a href="#packet6c">6c,&nbsp;reply&nbsp;to&nbsp;6f</a></td><td>Reply with list of object types</td></tr>

<tr><td><a href="#packet70">70,&nbsp;subcommand&nbsp;3</a></td><td></td>
<td>Specify what object types we don't want to synchronize</td></tr>

<tr><td></td><td><a href="#packet6c">6c,&nbsp;reply&nbsp;to&nbsp;70</a></td>
<td>Reply</td></tr>

<tr><td></td><td><a href="#packet69">69,&nbsp;subcommand&nbsp;2</a></td>
<td>Return partnership information</td></tr>

<tr><td><a href="#packet6f">6f,&nbsp;subcommand&nbsp;10</a></td><td></td>
<td>Unknown</td></tr>

<tr><td></td><td><a href="#packet69">69,&nbsp;subcommand&nbsp;4</a></td>
<td>One or more packets for for Appointment, Contact, Task and File.</td></tr>

<tr><td></td><td><a href="#packet69">69,&nbsp;subcommand&nbsp;6</a></td>
<td>One packet for each object type, in type ID order.</td></tr>

<tr><td><a href="#packet6f">6f,&nbsp;subcommand&nbsp;6</a></td><td></td>
<td>Unknown</td></tr>

<tr><td></td><td><a href="#packet6c">6c,&nbsp;reply&nbsp;to&nbsp;6f</a></td>
<td>Reply</td></tr>

</table>

<h3>Deleting an item</h3>

<table cellspacing=8 width="100%">
<tr><th>From desktop</th><th>From handheld</th><th>Description</th></tr>

<tr><td><a href="#packet70">70,&nbsp;subcommand&nbsp;1</a></td><td></td><td>Unknown</td></tr>
<tr><td></td><td><a href="#packet6c">6c,&nbsp;reply&nbsp;to&nbsp;70</a></td><td>Reply</td></tr>
<tr><td><a href="#packet66">66</a></td><td></td><td>Delete object</td></tr>
<tr><td></td><td><a href="#packet65">65</a></td><td>Agree on object IDs</td></tr>
<tr><td><a href="#packet70">70,&nbsp;subcommand&nbsp;2</a></td><td></td><td>Unknown</td></tr>
<tr><td><a href="#packet6f">6f,&nbsp;subcommand&nbsp;6</a></td><td></td><td></td></tr>
<tr><td></td><td><a href="#packet6c">6c,&nbsp;reply&nbsp;to&nbsp;70</a></td><td>Reply</td></tr>
<tr><td></td><td><a href="#packet6c">6c,&nbsp;reply&nbsp;to&nbsp;6f</a></td><td>Reply</td></tr>

</table>

<h3>dummy</h3>

<table cellspacing=8 width="100%">
<tr><th>From desktop</th><th>From handheld</th><th>Description</th></tr>

<tr><td></td><td></td><td></td></tr>
<tr><td></td><td></td><td></td></tr>
<tr><td></td><td></td><td></td></tr>
<tr><td></td><td></td><td></td></tr>
<tr><td></td><td></td><td></td></tr>

</table>



<hr size=1 />
<a name="packetmap"></a>
<h2>Command packet map</h2>

<table cellspacing=5>
<tr><th>Packet type</th><th>Sent from desktop?</th><th>Sent from handheld?</th><th>In reply to</th><th>Replied with</th></tr>

<tr><td><a href="#packet65">65</a></td><td>Y</td><td>Y</td><td><a href="#packet65">65</a></td></tr>
<tr><td><a href="#packet66">66</a></td><td>Y</td><td>N</td><td></tr>
<tr><td><a href="#packet67">67</a></td><td>Y</td><td>Y</td><td></tr>
<tr><td><a href="#packet69">69</a></td><td>N</td><td>Y</td><td></tr>
<tr><td><a href="#packet6c">6c</a></td><td>N</td><td>Y</td><td><a href="#packet6f">6f</a><br><a href="#packet70">70</a></td></tr>
<tr><td><a href="#packet6e">6e</a></td><td>N</td><td>Y</td><td></tr>
<tr><td><a href="#packet6f">6f</a></td><td>Y</td><td>N</td><td>&nbsp;</td><td><a href="#packet6c">6c</a></td></tr>
<tr><td><a href="#packet70">70</a></td><td>Y</td><td>N</td><td>&nbsp;</td><td><a href="#packet6c">6c</a></td></tr>

</table>

<hr size=1 />
<a name="commandheader"></a>
<h2>Command packet header</h2>

<table cellspacing=5>
<tr><th>Offset</th><th>Size</th><th>Contents</th><th>Description</th></tr>
<tr><td>0000</td><td>2</td><td></td><td>Packet type</td></tr>
<tr><td>0002</td><td>2</td><td></td><td>Size of remaining packet</td></tr>
</table>


<hr size=1 />
<a name="packet65"></a>
<h2>Command packet 65</h2>

<p>Negotiate unique identifiers for objects.</p>

<table cellspacing=5>
<tr><th>Offset</th><th>Size</th><th>Contents</th><th>Description</th></tr>
<tr><td>0000</td><td>2</td><td>6c 00</td><td>Packet type</td></tr>
<tr><td>0002</td><td>2</td><td>10 00</td><td>Size of remaining packet</td></tr>
<tr><td>0004</td><td>4</td><td></td><td>Type of object</td></tr>
<tr><td>0008</td><td>4</td><td></td><td>Identifier on the other device</td></tr>
<tr><td>000c</td><td>4</td><td></td><td>New identifier suggested</td></tr>
<tr><td>000c</td><td>4</td><td></td><td>Flags</td></tr>
</table>

<p>To agree on an object identifier, this packet is sent with both object
identifiers equal to the identifier suggested by the other device in the
previous packet 65.</p>

<h3>Flags</h3>

<table cellspacing=5>
<tr><td>00000000</td><td>Sent from desktop when we have received an object and want to mark it as unchanged</td></tr>
<tr><td>00000002</td><td>Sent from handheld when we have sent an object</td></tr>
<tr><td>08000000</td><td>Sent from desktop when we have sent an object and want to mark it as unchanged</td></tr>
</table>

<hr size=1 />
<a name="packet66"></a>
<h2>Command packet 66</h2>

<p>Delete object on handheld.</p>

<table cellspacing=5>
<tr><th>Offset</th><th>Size</th><th>Contents</th><th>Description</th></tr>
<tr><td>0000</td><td>2</td><td>66 00</td><td>Packet type</td></tr>
<tr><td>0002</td><td>2</td><td>10 00</td><td>Size of remaining packet</td></tr>
<tr><td>0004</td><td>4</td><td>00 00 00 00</td><td>Unknown</td></tr>
<tr><td>0008</td><td>4</td><td></td><td>Type id</td></tr>
<tr><td>000c</td><td>4</td><td></td><td>Object id</td></tr>
<tr><td>0010</td><td>4</td><td>00 00 00 00</td><td>Unknown</td></tr>
</table>


<hr size=1 />
<a name="packet67"></a>
<h2>Command packet 67</h2>

<p>Request object data. The requested data will be sent on the data channel,
see <a href="#dataheader">Data header</a>.</p>

<table cellspacing=5>
<tr><th>Offset</th><th>Size</th><th>Contents</th><th>Description</th></tr>
<tr><td>0000</td><td>2</td><td>67 00</td><td>Packet type</td></tr>
<tr><td>0002</td><td>2</td><td></td><td>Size of remaining packet</td></tr>
<tr><td>0004</td><td>4</td><td>00 00 00 00</td><td>Unknown</td></tr>
<tr><td>0004</td><td>4</td><td></td><td>Object type</td></tr>
<tr><td>0004</td><td>4</td><td><i>count</i></td><td>Object id count</td></tr>
<tr><td>0004</td><td><i>count</i> * 4</td><td></td><td>Object id array</td></tr>
</table>

<h3>Example</h3>

<pre>0000  67 00 10 00 00 00 00 00 12 27 00 00 01 00 00 00   g........'......
0010  9d 0e 00 05                                       ....</pre>

<hr size=1 />
<a name="packet69"></a>
<h2>Command packet 69</h2>

<h3>Common packet format</h3>

<table cellspacing=5>
<tr><th>Offset</th><th>Size</th><th>Contents</th><th>Description</th></tr>
<tr><td>0000</td><td>2</td><td>69 00</td><td>Packet type</td></tr>
<tr><td>0002</td><td>2</td><td></td><td>Size of remaining packet</td></tr>
<tr><td>0004</td><td>4</td><td></td><td>Subcommand</td></tr>
</table>

<h3>Subcommand overview</h3>

<table>
<tr><td>00 00 00 00</td><td>List of object IDs</td></tr>
<tr><td>00 00 00 02</td><td>Unknown</td></tr>
<tr><td>00 00 00 04</td><td>List of object IDs</td></tr>
<tr><td>00 00 00 06</td><td>List of object IDs</td></tr>
</table>

<h3>Subcommand 2</h3>

<p>This package has always the same size.</p>

<table cellspacing=5>
<tr><th>Offset</th><th>Size</th><th>Contents</th><th>Description</th></tr>
<tr><td>0008</td><td>4</td><td>ff ff ff ff</td><td>Unknown</td></tr>
<tr><td>000c</td><td>4</td><td>03 00 00 00</td><td>Unknown</td></tr>
<tr><td>0010</td><td>4</td><td>0c 00 00 00</td><td>Size of remaining package?</td></tr>
<tr><td>0014</td><td>4</td><td>Current partner index</td><td>Value of HKLM\Software\Microsoft\<br>Windows CE Services\Partners\PCur</td></tr>
<tr><td>0018</td><td>4</td><td>Partner 1 ID</td><td>Value of HKLM\Software\Microsoft\<br>Windows CE Services\Partners\P1\PId</td></tr>
<tr><td>001c</td><td>4</td><td>Partner 2 ID</td><td>Value of HKLM\Software\Microsoft\<br>Windows CE Services\Partners\P2\PId</td></tr>
</table>

<h3>Subcommands 0, 4 and 6</h3>

<p>If the subcommand is 0, reply with a packet type 70, subcommand 2.</p>

<table cellspacing=5>
<tr><th>Offset</th><th>Size</th><th>Contents</th><th>Description</th></tr>
<tr><td>0008</td><td>4</td><td></td><td>Object type id</td></tr>

<tr><td>000c</td><td>4</td><td><i>x</i> &lt;= (<i>size</i>/4)</td><td>This is
probably the number of changed objects and the remaining objects are
deleted.</td></tr>

<tr><td>0010</td><td>4</td><td><i>size</i></td><td>Size of array</td></tr>

<tr><td>0014</td><td><i>size</i></td><td></td><td>Array with (<i>size</i>/4) object
identifiers</td></tr>


</table>


<hr size=1 />
<a name="packet6c"></a>
<h2>Command packet 6c</h2>

<p>Reply package.</p>

<h3>Common packet format</h3>

<table cellspacing=5>
<tr><th>Offset</th><th>Size</th><th>Contents</th><th>Description</th></tr>
<tr><td>0000</td><td>2</td><td>6c 00</td><td>Packet type</td></tr>
<tr><td>0002</td><td>2</td><td><i>size</i></td><td>Size of remaining packet</td></tr>
<tr><td>0004</td><td>4</td><td></td><td>Code for the command we are replying to</td></tr>

</table>

<h3>Reply to command 6f, subcommand 06</h3>
<p>To be documented.</p>

<pre>0000  6c 00 30 00 6f 00 00 00 00 00 00 00 20 00 00 00   l.0.o....... ...
0010  9e 0e 00 00 01 00 00 f0 01 00 00 00 02 00 00 00   ................
0020  02 00 00 00 00 00 00 00 04 00 00 00 00 60 f9 00   .............`..
0030  7c 54 f4 00                                       |T..</pre>

<pre>0000  6c 00 30 00 6f 00 00 00 00 00 00 00 20 00 00 00   l.0.o....... ...
0010  c9 00 00 00 01 00 00 f0 01 00 00 00 02 00 00 00   ................
0020  02 00 00 00 00 00 00 00 04 00 00 00 00 60 f9 00   .............`..
0030  c0 54 f4 00                                       .T..</pre>

<pre>0000  6c 00 30 00 6f 00 00 00 00 00 00 00 20 00 00 00   l.0.o....... ...
0010  00 00 00 00 01 00 00 f0 01 00 00 00 02 00 00 00   ................
0020  02 00 00 00 00 00 00 00 04 00 00 00 00 60 f9 00   .............`..
0030  80 54 f4 00                                       .T..</pre>

<p>From synchronization with a HP 620 LX:</p>
<pre>
0030                           6c 00 94 00 6f 00 00 00            l...o...
0040  00 00 00 00 84 00 00 00  7b 00 30 00 01 00 00 f0   ........ {.0....ð
0050  01 00 00 00 02 00 00 00  02 00 00 00 05 00 00 00   ........ ........
0060  10 27 00 00 00 00 00 00  00 00 00 00 00 a8 62 f2   .'...... .....¨bò
0070  d0 4a c2 01 11 27 00 00  00 00 00 00 7c 01 00 00   ÐJÂ..'.. ....|...
0080  80 e3 3a 60 1e f8 bb 01  12 27 00 00 00 00 00 00   .ã:`.ø». .'......
0090  00 00 00 00 00 a4 67 ee  27 00 c3 01 13 27 00 00   .....¤gî '.Ã..'..
00a0  00 00 00 00 7c 01 00 00  80 e3 3a 60 1e f8 bb 01   ....|... .ã:`.ø».
00b0  14 27 00 00 02 00 00 00  14 03 00 00 80 ca 38 4e   .'...... .....Ê8N
00c0  72 2e c3 01 04 00 00 00  00 df 62 00 64 74 4c 00   r.Ã..... .ßb.dtL.</pre>


<h3>Reply to command 6f, subcommand 10</h3>
<p>To be documented.</p>

<h3>Reply to command 6f, subcommand c1</h3>

<table cellspacing=5>
<tr><th>Offset</th><th>Size</th><th>Contents</th><th>Description</th></tr>
<tr><td>0008</td><td>4</td><td>00 00 00 00</td><td>Unknown</td></tr>
<tr><td>000c</td><td>4</td><td><i>size</i> - 0x10</td><td></td></tr>

<tr><td>0010</td><td>10</td><td>00 00 00 00<br> 01 00 00 f0<br>01 00 00
00<br>01 00 00 00<br>01 00 00 00</td><td>Unknown</td></tr>

<tr><td>0020</td><td>4</td><td><i>n</i></td><td>Objec type record count</td></tr>
<tr><td>0024</td><td><i>n</i> * 0x180</td><td></td><td>Object type records</td></tr>
</table>

<p>Format of an object type record:</p>

<table cellspacing=5>
<tr><th>Offset</th><th>Size</th><th>Contents</th><th>Description</th></tr>
<tr><td>0000</td><td>4</td><td>00 00 00 00</td><td>Unknown</td></tr>
<tr><td>0004</td><td>c8</td><td></td><td>Object type name</td></tr>
<tr><td>00cc</td><td>50</td><td></td><td>Object type name again</td></tr>
<tr><td>011c</td><td>4</td><td></td><td>Object type id</td></tr>
<tr><td>0120</td><td>4</td><td></td><td>Number of items of this type</td></tr>
<tr><td>0124</td><td>4</td><td></td><td>Total size for items of this type</td></tr>
<tr><td>0128</td><td>8</td><td>FILETIME</td><td>Last modification time for this type</td></tr>
</table>


<h3>Reply to command 70</h3>

<table cellspacing=5>
<tr><th>Offset</th><th>Size</th><th>Contents</th><th>Description</th></tr>
<tr><td>0008</td><td>4</td><td>00 00 00 00</td><td>Unknown</td></tr>
<tr><td>000c</td><td>4</td><td>90 25 03 00 or c0 2f 04 00 (for example)</td><td>Unknown</td></tr>
<tr><td>0010</td><td>4</td><td>00 00 00 00 or 38 10 01 00 (for example)</td><td>Unknown</td></tr>

</table>


<hr size=1 />
<a name="packet6e"></a>
<h2>Command packet 6e</h2>

<p>Error package.</p>

<table cellspacing=5>
<tr><th>Offset</th><th>Size</th><th>Contents</th><th>Description</th></tr>
<tr><td>0000</td><td>2</td><td>6e 00</td><td>Packet type</td></tr>
<tr><td>0002</td><td>2</td><td>10 00</td><td>Size of remaining packet</td></tr>
<tr><td>0004</td><td>4</td><td></td><td>Type id</td></tr>
<tr><td>0008</td><td>4</td><td></td><td>Object id</td></tr>
<tr><td>000c</td><td>4</td><td></td><td>HRESULT</td></tr>
<tr><td>0010</td><td>4</td><td></td><td>Unknown</td></tr>
</table>

<hr size=1 />
<a name="packet6f"></a>
<h2>Command packet 6f</h2>

<h3>Packet format</h3>

<table cellspacing=5>
<tr><th>Offset</th><th>Size</th><th>Contents</th><th>Description</th></tr>

<tr><td>0000</td><td>2</td><td>6f 00</td><td>Packet type</td></tr>
<tr><td>0002</td><td>2</td><td>04 00</td><td>Size of remaining packet</td></tr>
<tr><td>0004</td><td>4</td><td></td><td>Subcommand</td></tr>
</table>

<h3>Known subcommands</h3>

<table>
<tr><td>06 00 00 00</td><td>Unknown</td></tr>
<tr><td>10 00 00 00</td><td>Unknown</td></tr>
<tr><td>c1 00 00 00</td><td>Request list of object types. This is the first request sent.</td></tr>
<tr><td>c1 01 00 00</td><td>Same as above but required by the SmartPhone 2002</td></tr>
<tr><td>c1 07 00 00</td><td>Same as above but used my ActiveSync 3.7.1</td></tr>
</table>

<hr size=1 />
<a name="packet70"></a>
<h2>Command packet 70</h2>

<p>More documentation to come.</p>

<h3>Common packet format</h3>

<table cellspacing=5>
<tr><th>Offset</th><th>Size</th><th>Contents</th><th>Description</th></tr>
<tr><td>0000</td><td>2</td><td>70 00</td><td>Packet type</td></tr>
<tr><td>0002</td><td>2</td><td></td><td>Size of remaining packet</td></tr>
<tr><td>0004</td><td>4</td><td></td><td>Size of remaining packet (yes, again!)</td></tr>
<tr><td>0008</td><td>4</td><td>01 00 00 f0</td><td>Unknown</td></tr>
<tr><td>000c</td><td>4</td><td></td><td>Subcommand</td></tr>
</table>

<h3>Known subcommands</h3>

<table>
<tr><td>02 00 00 00</td><td>Unknown</td></tr>
<tr><td>03 00 00 00</td><td>Specify list of object types we don't want</td></tr>
</table>

<h3>Subcommand 2</h3>

<h4>Reply to packet 69, subcommand 0</h4>

<pre>0000  <span class=RED>70 00 f4 00 f0 00 00 00 01 00 00 f0 02 00 00 00</span>   p...............
0010  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................
0020  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................
0030  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................
0040  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................
0050  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................
0060  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................
0070  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................
0080  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................
0090  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................
00a0  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................
00b0  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................
00c0  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................
00d0  00 00 00 00 00 00 00 00 <span class=RED>01 00 00 00 03 00 00 80</span>   ................
00e0  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................
00f0  00 00 00 00 00 00 00 00                           ........</pre>

<h4>Unknown</h4>

<pre>0000  <span class=RED>70 00 f4 00 f0 00 00 00 01 00 00 f0 02 00 00 00</span>   p...............
0010  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................
0020  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................
0030  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................
0040  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................
0050  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................
0060  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................
0070  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................
0080  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................
0090  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................
00a0  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................
00b0  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................
00c0  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................
00d0  00 00 00 00 00 00 00 00 <span class=RED>02 00 00 00 00 00 00 00</span>   ................
00e0  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................
00f0  00 00 00 00 00 00 00 00                           ........</pre>

<h3>Subcommand 3</h3>

<table cellspacing=5>
<tr><th>Offset</th><th>Size</th><th>Contents</th><th>Description</th></tr>
<tr><td>0010</td><td>10</td><td>
02 00 00 00<br>
00 00 00 00<br>
00 00 00 00<br>
00 00 00 00<br>
</td><td>Unknown</td></tr>
<tr><td>0020</td><td>4</td><td></td><td>Number of object types that we do <b>not</b> want to synchronize.</td></tr>
<tr><td>0024</td><td>4*n</td><td></td><td>List of object types that we do <b>not</b> want to synchronize.</td></tr>
</table>

<h4>Example</h4>
<pre>0000  70 00 40 00 3c 00 00 00 01 00 00 f0 03 00 00 00   p.@.&lt;...........
0010  02 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00   ................
0020  08 00 00 00 11 27 00 00 14 27 00 00 13 27 00 00   .....'...'...'..
0030  16 27 00 00 10 27 00 00 17 27 00 00 15 27 00 00   .'...'...'...'..
0040  18 27 00 00                                       .'..</pre>


<hr size=1 />
<a name="dataheader"></a>
<h2>Data header</h2>

<table cellspacing=5>
<tr><th>Offset</th><th>Size</th><th>Contents</th><th>Description</th></tr>
<tr><td>0000</td><td>4</td><td></td><td>Object id</td></tr>
<tr><td>0004</td><td>4</td><td></td><td>Type id</td></tr>
<tr><td>0008</td><td>4</td><td></td><td>Flags</td></tr>
</table>

<p>If the object id is 0xffffffff, this is the end of an object sequence.
Otherwise it is followed by a <a href="#datachunk">data chunk</a>.</p>

<h3>Flags</h3>

<table cellspacing=5>
<tr><td>00000000</td><td>Used with end of an object sequence</td></tr>
<tr><td>00000002</td><td>Used to create a new object.</td></tr>
<tr><td>00000040</td><td>Used to update an existing object</td></tr>
</table>


<hr size=1 />
<a name="datachunk"></a>
<h2>Data chunk</h2>

<table cellspacing=5>
<tr><th>Offset</th><th>Size</th><th>Contents</th><th>Description</th></tr>
<tr><td>0000</td><td>2</td><td><i>size</i></td><td>Chunk size</td></tr>
<tr><td>0002</td><td>2</td><td></td><td>Special value</td></tr>
<tr><td>0004</td><td><i>size</i></td><td></td><td>Chunk data</td></tr>
</table>

<p>If the special value is similar to 0xffa0, 0xffa4 or 0xffa8 this is the last
chunk. Otherwise the special value is the data offset of next chunk, which
follows after this chunk's data.</p>

<hr size=1 />
<a name="datastream"></a>
<h2>Data stream</h2>

<p>The <i>data stream</i> is all chunk data concatenated.</p>

<p>There is an example program called rra-decode that reads a data stream from
a file and decodes it as described here.</p>

<p>There are three known data formats:</p>

<ul>
<li>Directory entry</li>
<li>File entry</li>
<li>Database record</li>
</ul>

<h3>Directory entry</h3>

<table cellspacing=5>
<tr><th>Offset</th><th>Size</th><th>Contents</th><th>Description</th></tr>
<tr><td>0000</td><td>4</td><td>00000010</td><td>Data type for directory entry</td></tr>
<tr><td>0004</td><td>unknown</td><td>Directory name</td><td>2-byte UNICODE string with terminating NULL character.</td></tr>
</table>

<h3>File entry</h3>

<p>The total stream size is specified as <i>total_size</i> below.</p>

<table cellspacing=5>
<tr><th>Offset</th><th>Size</th><th>Contents</th><th>Description</th></tr>
<tr><td>0000</td><td>4</td><td>00000020</td><td>Data type for file entry</td></tr>
<tr><td>0004</td><td><i>name_size</i></td><td>File name</td><td>2-byte UNICODE string with terminating NULL character.</td></tr>
<tr><td>0004 + <i>name_size</i></td><td><i>total_size</i> - 4 - <i>name_size</i></td><td>File data</td><td></td></tr>
</table>


<h3>Database record</h3>

<table cellspacing=5>
<tr><th>Offset</th><th>Size</th><th>Contents</th><th>Description</th></tr>
<tr><td>0000</td><td>4</td><td><i>count</i></td><td>Number of property values</td></tr>
<tr><td>0004</td><td>4</td><td>0</td><td>Always zero</td></tr>
</table>

<p>The following is then repeated <i>count</i> times:</p>

<table cellspacing=5>
<tr><th>Offset</th><th>Size</th><th>Contents</th><th>Description</th></tr>
<tr><td>0000</td><td>4</td><td><i>propid</i></td><td><a href="http://msdn.microsoft.com/library/default.asp?url=/library/en-us/wcesdkr/htm/_wcesdk_cepropval.asp">CEPROPID</a> value.</td></tr>
<tr><td>0004</td><td>Depends on <i>propid</i></td><td><i>value</i></td><td>Data of the type specified with <i>propid</i></td></tr>
</table>

<p>If (<i>propid</i> &amp; 0x400) is true, then no data follows. This flag is
used to remove a property value.</p>

<p>Code to convert between a database record stream and an array of <a
href="http://msdn.microsoft.com/library/default.asp?url=/library/en-us/wcesdkr/htm/_wcesdk_cepropval.asp">CEPROVAL</a>
structures is available in the the RRA library (dbstream.h and dbstream.c to be exact).</p>

<p><br>Return to <a href="index.php">main page</a>.</p>

</div>
<?php include 'footer.php'; ?>
