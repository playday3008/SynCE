<?php include 'header.php'; ?>

<p>Return to <a href="index.php">SynCE-KDE main page</a>.</p>
<h1>SynCE - KDE KCEMirror<br>
</h1>
<h2>Introduction</h2>
SynCE-KDE KCEMirror is a system to interact with a Windows CE device by
simply using your desktops display, mouse, and keyboard. This system is
made up of two dependent software components:<br>
<b><br>
screensnap</b><br>
<blockquote>is used on the Windows CE side to capture the screen of the
device continuously, to compress this captured image and to send it
over the network to the client software on the PC.<br>
It is also responsible for synthesing keyboard and mouse events on the
Windows CE device, received from the PC client software.<br>
</blockquote>
<b>kcemirror</b><br>
<blockquote>is the client software on the PC side. It receives the
compressed image of the Windows CE screen, decompresses it and displays
it on the PC's screen. Have a look at the screen shots below to get an
impression about the result on the desktop.<br>
  <br>
</blockquote>
<h2>Installation and Usage</h2>
Both components, screensnap and KCEMirror gets installed during the
usual installation process of SynCE-KDE. Nevertheless, scrennsnap gets
only installed into a specific directory on the desktop. On
start up KCEMirror checks the Windows CE device for an already
installed screensnap. If it doesn't find screensnap on the device
KCEMirror installs this component automatically on the device.<br>
Regardless KCEMirror has to install screensnap first or not, it will
start up
screensnap on the Windows CE device before initiating a
network connection to it. On success, KCEMirror presents a window on
the display of the PC showing the screen of the Windows CE device. <br>
As long as the KCEMirror window owns the input focus, all mouse and
keyboard events are propagated to the Windows CE device where they are
transformed into synthetic Windows CE mouse and key events by
screensnap.<br>
<br>
<h2>Technical Information</h2>
<h3>Compression/Decompression Algorithm</h3>
The compression of an image of the Windows CE device is a multi-stage
process.<br>
<ol>
  <li>The Windows CE screen is captured and is combined using an
exclusive or operation with the prior captured image. Because the
screen content would not change much most of the time, the resulting
image is largely black.</li>
  <li>A result of the XOR operation is encoded using a "run length
encoding" (RLE) algorithm. Because&nbsp; there are many solid black
areas in the resulting image the RLE algorithm is very efficient.
Furthermore, RLE is a very fast algorithm. The image is compressed by a
factor of 30 to 50 on average on normal use.<br>
  </li>
  <li>The RL encoded image is encoded using an Huffman encoder. This
gains again an factor of 3 in size on average.<br>
  </li>
</ol>
<br>
<h2>Screenshots</h2>
<table align="center" width="100%" border="0" cellspacing="2"
 cellpadding="2">
  <tbody>
    <tr>
      <td align="center" valign="top"><img height="403" width="247"
 alt="kcemirror-menu" title="Open Menu" src="images/pdamirror-menu.png"><br>
      </td>
      <td align="center" valign="top"><img height="403" width="247"
 alt="kcemirror-note" title="Written Note"
 src="images/pdamirror-note.png"><br>
      </td>
    </tr>
    <tr>
      <td align="center" valign="top">You can simply interact with your
      <br>
Windows CE device by using the<br>
mouse of your desktop PC<br>
      </td>
      <td align="center" valign="top">Interaction with your Windows CE<br>
device is also possible by using the<br>
keyboard of your desktop PC<br>
      </td>
    </tr>
  </tbody>
</table>
<p>Return to <a href="index.php">SynCE-KDE main page</a>.</p>

<?php include 'footer.php'; ?>
