<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="index.php">main page</a>.</p>

<h1>SynCE - Remote Replication Agent Connection</h1>

<p>Numeric values in the transferred data are stored in little-endian
format.</p>

<p>All values are given in hexadecimal form unless noted otherwise.</p>

<h2>Packet map</h2>

<table cellspacing=5>
<tr><th>Packet type</th><th>Sent from desktop?</th><th>Sent from handheld?</th><th>In reply to</th><th>Replied with</th></tr>

<tr><td><a href="#packet65">65</a></td><td>Y</td><td>Y</td><td><a href="#packet65">65</a><br><a href="#packet8x">83, 84, ...</a></td></tr>
<tr><td><a href="#packet66">66</a></td><td>Y</td><td>N</td><td></tr>
<tr><td><a href="#packet67">67</a></td><td>Y</td><td>Y</td><td></tr>
<tr><td><a href="#packet69">69</a></td><td>N</td><td>Y</td><td></tr>
<tr><td><a href="#packet6c">6c</a></td><td>N</td><td>Y</td><td><a href="#packet6f">6f</a><br><a href="#packet70">70</a></td></tr>
<tr><td><a href="#packet6f">6f</a></td><td>Y</td><td>N</td><td>&nbsp;</td><td><a href="#packet6c">6c</a></td></tr>
<tr><td><a href="#packet70">70</a></td><td>Y</td><td>N</td><td>&nbsp;</td><td><a href="#packet6c">6c</a></td></tr>
<tr><td><a href="#packet8x">83, 84, ...</a></td><td>Y</td><td>N</td><td>&nbsp;</td><td><a href="#packet65">65</a></td></tr>

</table>

<hr size=1 />
<a name="packet65"></a>
<h2>Packet type 65</h2>

<p>Negotiate unique identifiers for objects.</p>

<table cellspacing=5>
<tr><th>Offset</th><th>Size</th><th>Contents</th><th>Description</th></tr>
<tr><td>0000</td><td>2</td><td>6c 00</td><td>Packet type</td></tr>
<tr><td>0002</td><td>2</td><td>10 00</td><td>Size of remaining packet</td></tr>
<tr><td>0004</td><td>4</td><td></td><td>Type of object</td></tr>
<tr><td>0008</td><td>4</td><td></td><td>Identifier on the other device</td></tr>
<tr><td>000c</td><td>4</td><td></td><td>New identifier suggested</td></tr>
<tr><td>000c</td><td>4</td><td>00 00 00 00 or<br>02 00 00 00</td><td>Unknown</td></tr>
</table>

<hr size=1 />
<a name="packet67"></a>
<h2>Packet type 67</h2>

<table cellspacing=5>
<tr><th>Offset</th><th>Size</th><th>Contents</th><th>Description</th></tr>
<tr><td>0000</td><td>2</td><td>67 00</td><td>Packet type</td></tr>
<tr><td>0002</td><td>2</td><td>10 00</td><td>Size of remaining packet</td></tr>
<tr><td>0004</td><td>4</td><td>00 00 00 00</td><td>Unknown</td></tr>
<tr><td>0004</td><td>4</td><td></td><td>Object type</td></tr>
<tr><td>0004</td><td>4</td><td>01 00 00 00</td><td>Object id count?</td></tr>
<tr><td>0004</td><td>4</td><td></td><td>Object id</td></tr>
</table>

<pre>0000  67 00 10 00 00 00 00 00 12 27 00 00 01 00 00 00   g........'......
0010  9d 0e 00 05                                       ....</pre>

<hr size=1 />
<a name="packet69"></a>
<h2>Packet type 69</h2>

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
<h2>Packet type 6c</h2>

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

<p>Format of a object type record:</p>

<table cellspacing=5>
<tr><th>Offset</th><th>Size</th><th>Contents</th><th>Description</th></tr>
<tr><td>0000</td><td>4</td><td>00 00 00 00</td><td>Unknown</td></tr>
<tr><td>0004</td><td>c8</td><td></td><td>Object type name</td></tr>
<tr><td>00cc</td><td>50</td><td></td><td>Object type name again</td></tr>
<tr><td>016c</td><td>a0</td><td></td><td>Object type name again</td></tr>
</table>


<h3>Reply to command 70</h3>

<table cellspacing=5>
<tr><th>Offset</th><th>Size</th><th>Contents</th><th>Description</th></tr>
<tr><td>0008</td><td>4</td><td>00 00 00 00</td><td>Unknown</td></tr>
<tr><td>000c</td><td>4</td><td>90 25 03 00 or c0 2f 04 00 (for example)</td><td>Unknown</td></tr>
<tr><td>0010</td><td>4</td><td>00 00 00 00 or 38 10 01 00 (for example)</td><td>Unknown</td></tr>

</table>


<hr size=1 />
<a name="packet6f"></a>
<h2>Packet type 6f</h2>

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
</table>

<hr size=1 />
<a name="packet70"></a>
<h2>Packet type 70</h2>

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
<a name="packet8x"></a>
<h2>Packet type 83, 84, ...</h2>

<p>These are kind of special as their size is not known in advance.</p>

<table cellspacing=5>
<tr><th>Offset</th><th>Size</th><th>Contents</th><th>Description</th></tr>
<tr><td>0000</td><td>4</td><td>83 00<br>84 00<br>...</td><td>Packet type</td></tr>
<tr><td>0002</td><td>2</td><td>00 20</td><td>Unknown</td></tr>
<tr><td>0004</td><td>Unknown</td><td></td><td>Object data etc.</td></tr>
</table>

<p>More documentation to come.</p>


<p><br>Return to <a href="index.php">main page</a>.</p>

</div>
<?php include 'footer.php'; ?>
