<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="index.php">main page</a>.</p>

<h1>How to capture packets for analyzing</h1>

<p>Note: I use Linux. That's why it says Linux in this text. You could probably do this on *BSD or other unices as well.</p>

<p>I use a simple tool called <a href="datapipe.c">datapipe</a> to forward the
ports used by Windows CE. On the Linux machine I use <a
href="http://www.ethereal.com/">Ethereal</a> to capture the packets. This is
a simple script to setup the forwarders:</p>

<pre>#!/bin/sh
# The IP of your Windows CE device
WINDOWSCE=192.168.131.201
# The IP of your Windows desktop machine
WINDOWS=192.168.0.148

RAPI_PORT=990
RRAC_PORT=5678
DCCM_PORT=5679
datapipe 0.0.0.0 $RAPI_PORT $WINDOWSCE $RAPI_PORT
datapipe 0.0.0.0 $RRAC_PORT $WINDOWS $RRAC_PORT
datapipe 0.0.0.0 $DCCM_PORT $WINDOWS $DCCM_PORT</pre>

<p>The script needs to be run as root because the RAPI port number is less
than 1024. You should not be running dccm as it binds to the same port as one
of the datapipes.</p>

<p>Ethereal is then set to listen on the interface that the Windows desktop
machine is connected to. Filter on the IP of the Windows machine (<i>host
192.168.0.148</i> for example).</p>

<p>Now you can use synce-serial-start on your Linux machine and begin capturing!</p>

<p>Note that in order to get ActiveSync happy, you should already have
estabilished a partnership directly with the machine that is running
Windows.</p>

<p>When you are finished, just use <i>killall datapipe</i> to shut down the
pipes.</p>

<p>To make it easier to see what happens over RAPI, I have created Wireshark
packet decoders for the protocols used by SynCE. Put the files in the top
Wireshark source directory and add their names to epan/Makefile.common. Then
can be found in the <a
href="http://svn.sourceforge.net/viewvc/synce/trunk/twogood-files/">twogood-files</a> directory.</p>

<ul>

<li>packet-dccm.c: DCCM packet decoder</a>.</li>

<li>packet-rapi.c: RAPI packet decoder</a>.</li>

<li>packet-rrac.c: RRAC packet decoder</a>.</li>

</ul>

<p>Return to <a href="index.php">main page</a>.</p>

</div>
<?php include 'footer.php'; ?>
