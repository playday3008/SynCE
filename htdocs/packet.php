<?php include 'header.php'; ?>
<H1>RAPI Packets structure</H1>
<BR>

The RAPI protocol occurs on port 990. It is initiated by the client computer, in direction of the mobile device.

<H2>Protocol</H2>

The communication between device and host is the result of an (R)API call. There seems to be (and this is logical)
a 1-1 correspondance between a RAPI call and an information exchange (the host sends a request, and receives and answer).<BR>

<H2>Packets</H2>

<H3>Requests Packets</H3>

Requests packets (computer -> device) seems to always have the following format :<br><br>

<TABLE border="1" width="400" height="200">
<tr>
<TD>(long)</TD>
<TD>(long)</TD>
<TD>(long)</TD>
<TD>(long)</TD>
<TD>(long)</TD>
<TD>(long)</TD>
<TD>(long)</TD>
<TD>(long)</TD>
</tr>
<tr>
<TD>Buffer Length (bytes)</TD>
<TD>Command Number</TD>
<TD width="90%" colspan="6" align="center">&nbsp;Params&nbsp;</TD>
</tr>
<tr>
<TD colspan="8" align="center">&nbsp;Params&nbsp;</TD>
</tr>
<tr>
<TD colspan="8" align="center">&nbsp;Params&nbsp;</TD>
</tr>
<tr>
<TD colspan="8" align="center">&nbsp;Params&nbsp;</TD>
</tr>
<tr>
<TD colspan="8" align="center">&nbsp;Params&nbsp;</TD>
</tr>
</TABLE>
<BR>(The buffer is 4 + &lt;Buffer Length&gt; bytes long)

<BR><BR><BR><BR>

<H3>Response Packets</H3>

Response packets (device -> computer) seems to always have the following format :<br><br>

<TABLE border="1" width="400" height="200">
<tr>
<TD>(long)</TD>
<TD>(long)</TD>
<TD>(long)</TD>
<TD>(long)</TD>
<TD>(long)</TD>
<TD>(long)</TD>
<TD>(long)</TD>
<TD>(long)</TD>
</tr>
<tr>
<TD>Buffer Length (bytes)</TD>
<TD>Often, indicates succes or failure</TD>
<TD>Often, is LastError</TD>
<TD width="90%" colspan="5" align="center">&nbsp;Results&nbsp;</TD>
</tr>
<tr>
<TD colspan="8" align="center">&nbsp;Results&nbsp;</TD>
</tr>
<tr>
<TD colspan="8" align="center">&nbsp;Results&nbsp;</TD>
</tr>
<tr>
<TD colspan="8" align="center">&nbsp;Results&nbsp;</TD>
</tr>
<tr>
<TD colspan="8" align="center">&nbsp;Results&nbsp;</TD>
</tr>
</TABLE>
<BR>(The buffer is 4 + &lt;Buffer Length&gt; bytes long)

<H2></H2>
<BR><BR>

<H3>Parameters</H3>

Parameters are : Longs, Words and Strings. They are stacked one after the other in a defined order (depending of the function call),
 without indication of type (which you must guess).<BR>
String are in unicode (16 bits / 2 bytes per character), and are often preceded by their length ( a Long ). <BR>
They are often terminated by a NULL ( the Word 0x0000 ).

<BR><BR>
<H3>Example of a packet</H3>

Let's have a looke at the function <code>DWORD CeGetSpecialFolderPath( int nFolder, DWORD nBufferLength, LPWSTR lpBuffer )</code><BR>
Suppose we call it like this in a process : <code>CeGetSpecialFolderPath( CSIDL_DESKTOPDIRECTORY, sizeof(Buffer), &Buffer )</code> (Buffer is 0x104 bytes long)<br>
This would generate the following packet :<br><br>
<code>0  0c00 0000 4400 0000 0000 0000 0401 0000   ....D...........</code>
<ul>
<li>Size of the packet : 0x000000C0</li>
<li>Code of the function : 0x00000044 (0x44 is the code for CeGetSpecialFolderPath)</li>
<li>First parameter : nFolder : 0x00000000 (constant CSIDL_DESKTOPDIRECTORY)</li>
<li>Second parameter : size of the buffer : 0x00000104</li>
</ul>
<br><br>
The device would answer with the following packet : <br><br>
<pre>   0  2800 0000 0000 0000 0000 0000 0d00 0000   (...............
  10  5c00 4d00 7900 2000 4400 6f00 6300 7500   \.M.y. .D.o.c.u.
  20  6d00 6500 6e00 7400 7300 0000             m.e.n.t.s...</pre>
<ul>
<li>Size of the packet : 0x00000028</li>
<li>First result, unknown (result code ?) : 0x00000000</li>
<li>Second result, usually 'LastError' : 0x00000000</li>
<li>Third result : the length of the string : 0x0000000D</li>
<li>Fourth result : the string itself : '\My Documents' (followed by a terminal null)</li>
</ul>
<br><br>
<?php include 'footer.php'; ?>
