<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="index.php">main page</a>.</p>

<h1>SynCE - Questions and Answers for common issues</h1>

<p>For issues with MultiSync or the rra module, please visit the <a
href="multisync.php">Using MultiSync</a> page.</p>

<p>For issues regarding Mac OS X, please visit the <a
href="macosx.php">Mac OS X hints</a> page.</p>

<h2>Connect script failed</h2>

<p><b>Q:</b> I setup everything and connected my PDA but when I run
<tt>synce-serial-start</tt> I get messages similar to this in my system
log:</p>

<pre>pppd[6083]: pppd 2.4.1 started by root, uid 0
pppd[6083]: Terminating on signal 15.
pppd[6083]: Connect script failed
pppd[6083]: Exit.</pre>

<p><b>A:</b> This is a strange problem that has been occuring lately.
Unfortunately it has not been resolved yet. Any information relating to this
problem that may be helpful would be most welcome!</p>

<p>You can try things like these and see if anything helps:</p>

<ul>

<li class=SPACED><p>Try this procedure:</p> 

  <ol><li><p>Replace synce-serial/src/synce-serial-chat.c with this file:</p>

  <p>http://synce.sourceforge.net/tmp/synce-serial-chat.c</p></li>

  <li>Compile and install synce-serial</li>

  <li>Try to connect</li>

  <li>Check /var/log/messages for messages from synce-serial-chat</li>
  
  </ol></li>

<li class=SPACED>Remove hotplug script for SynCE</li>
  
<li class=SPACED>Make a soft reset of your PDA</li>
<li class=SPACED>Disconnect other USB devices</li>
<li class=SPACED>Reboot your PC</li>
<li class=SPACED>Try your device with Microsoft ActiveSync</li>

</ul>


<h2>Unable to initialize RAPI</h2>

<p><b>Q:</b> I setup everything and connected my PDA but when I try the <a
href="tools.php">tools</a> I get a message similar to this:</p>

<pre>ReadConfigFile: stat: No such file or directory
pstatus: Unable to initialize RAPI: Failure</pre>

<p><b>A:</b> This means that the PDA has not connected to dccm or that you run
the tools or dccm as different users. Please make sure that:</p>

<ul>
<li>dccm is running</li>
<li>that the PPP connection is successful (look in your system logs)</li>
<li>no firewall configuration prevents the PDA from connecting to dccm</li>
<li>you run dccm and the tools as the same user</li>
</ul>

<h2>Serial cable connection dies when transferring large files</h2>

<p>(SynCE 0.7 or earlier)</p>

<p><b>Q:</b> I use a <a href="serial.php">serial cable</a> to connect my PDA to
my PC. My PPP connection dies when I transfer large files to my PDA. I see a
message similar to this in my logs:</p>

<pre>Apr 27 17:41:09 localhost pppd[10387]: No response to 2 echo-requests
Apr 27 17:41:09 localhost pppd[10387]: Serial link appears to be disconnected.
Apr 27 17:41:09 localhost pppd[10387]: Connection terminated.</pre>

<p><b>A:</b> This is a known problem with SynCE 0.7 and earlier. After running
<tt>synce-serial-config</tt>, please edit <tt>/etc/ppp/peers/synce-device</tt>. Remove these two lines:</p>

<pre>lcp-echo-failure 2
lcp-echo-interval 2</pre>

<p>Add this line:</p>

<pre>crtscts</pre>

<p>Thanks to Lamar Owen for the solution to this problem!</p>

<h2>Using a USB hub</h2>

<p><b>Q:</b> I can't use SynCE to connect to my PDA via a USB hub. What can I do
to make it work?</p>

<p><b>A:</b> You have a couple of options here, but no perfect solution:</p>

<ul>

<li>Fix the USB driver and share your patches with us</li>

<li>Don't use a USB hub</li>

<li>Try the user-space USB driver, which is the <tt>wince-usb</tt> module in <a
href="http://sourceforge.net/cvs/?group_id=30550">CVS</a></li>

</ul>

<h2>Accessing the Internet from the PDA via the PC</h2>

<p><b>Q:</b> I want to access the Internet from my PDA while it is connected
via SynCE. What shall I do?</p>

<p><b>A:</b> Easy. Just visit the <a href="ip_forward.php">IP forwarding</a>
page and follow the instructions.</p>

<p>Return to <a href="index.php">main page</a>.</p>

<h2>Auto-start of synce-serial-connect when I connect my USB cable</h2>

<p><b>Q:</b> I want synce-serial-connect to run automatically when I connect
the USB cable to my device.</p>

<p><b>A:</b> You need a <a href="hotplug.php">hotplug script</a>.</p>

</div>
<?php include 'footer.php'; ?>
