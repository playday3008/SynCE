<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="index.php">main page</a>.</p>

<h1>SynCE - Questions and Answers for common issues</h1>

<p>For issues with synchronization (MultiSync or the rra module), please visit
the <a href="multisync.php">Using MultiSync</a> page.</p>

<p>For issues regarding Mac OS X, please visit the <a
href="macosx.php">Mac OS X hints</a> page.</p>

<h2>Unable to initialize RAPI</h2>

<p><b>Q:</b> I setup everything and connected my PDA but when I try the <a
href="tools.php">tools</a> I get a message similar to this in SynCE 0.8 or later:</p>

<pre>pstatus: Unable to initialize RAPI: An unspecified failure has occurred</pre>

<p>Or this message in earlier versions of SynCE:</p>

<pre>ReadConfigFile: stat: No such file or directory
pstatus: Unable to initialize RAPI: Failure</pre>


<p><b>A:</b> This means that the PDA has not connected to dccm or that you run
the tools and dccm as different users. Please make sure that:</p>

<ul>
<li>you supplied a password to dccm if your device is password-protected</li>
<li>dccm was started before you ran synce-serial-start</li>
<li>that the PPP connection is successful (look in your system logs)</li>
<li>no firewall configuration prevents the PDA from connecting to dccm</li>
<li>you run dccm and the tools as the same user</li>
</ul>

<p>See <a href="start.php">connect</a> for more information.</p>

<h2>PPP error: "Couldn't set tty to PPP discipline"</h2>

<p><b>Q:</b> I setup everything and connected my PDA but when I run
<tt>synce-serial-start</tt> I get messages similar to this in my system
log:</p>

<pre>kernel: PPP generic driver version 2.4.2
pppd[2156]: pppd 2.4.1 started by root, uid 0
pppd[2156]: Serial connection established.
modprobe: modprobe: Can't locate module tty-ldisc-3
pppd[2156]: Couldn't set tty to PPP discipline: Invalid argument
pppd[2156]: Exit.</pre>

<p><b>A:</b> The missing "tty-ldisc-3" module is the "ppp_async" kernel module.
Make sure that you have compiled that module. You may also have to add this
line to /etc/modules.conf:</p>

<pre>alias tty-ldisc-3 ppp_async</pre>


<h2>PPP error: "Connect script failed"</h2>

<p><b>Q:</b> I setup everything and connected my PDA but when I run
<tt>synce-serial-start</tt> I get messages similar to this in my system
log:</p>

<pre>pppd[6083]: pppd 2.4.1 started by root, uid 0
pppd[6083]: Terminating on signal 15.
pppd[6083]: Connect script failed
pppd[6083]: Exit.</pre>

<p><b>A:</b> Please use this checklist to find a solution to your problem:</p>

<ul>

<li class=SPACED>Make sure that the serial port is enabled in BIOS (serial cable only)</li>

<li class=SPACED>Make sure you are using the correct serial port (serial cable only)</li>

<li class=SPACED>Decrease the connection speed by changing 115200 to 19200
in /etc/ppp/peers/synce-device</li>

<li class=SPACED>Remove hotplug script for SynCE (USB cable only)</li>
  
<li class=SPACED>Make a soft reset of your PDA</li>
<li class=SPACED>Disconnect other USB devices (USB cable only)</li>
<li class=SPACED>Reboot your PC</li>
<li class=SPACED>Make sure your device works with Microsoft ActiveSync</li>

</ul>

<h3>If it still doesn't work...</h3>

<form action="https://www.paypal.com/cgi-bin/webscr" method="post">

<input type="hidden" name="cmd" value="_xclick">
<input type="hidden" name="business" value="twogood@users.sourceforge.net">
<input type="hidden" name="item_name" value="The SynCE Project">
<input type="hidden" name="no_shipping" value="1">
<input type="hidden" name="return" value="http://synce.sourceforge.net/synce/thanks.php">
<input type="hidden" name="cancel_return" value="http://synce.sourceforge.net/synce/">
<input type="hidden" name="cn" value="Comments">
<input type="hidden" name="currency_code" value="USD">
<input type="hidden" name="tax" value="0">

<table cellpadding=5 width="100%"><tr>

<td class=MIDDLE>If you have tried everything above and it still doesn't work,
maybe you are using a USB connection with a newer Pocket PC model such as the
iPAQ H5550, for which several persons have reported problems. Unfortunately no
SynCE developer has such a device, which makes it very hard for us to give
support. If we get enough donations to the project, we will have a much better
chance to help.</td>

<td class=MIDDLE><input type="image"
src="https://www.paypal.com/images/x-click-but21.gif" border="0" name="submit"
alt="Make a donation" title="Make a donation"></td>

</tr></table>
</form>



<h2>PPP error: "No response to 2 echo-requests"</h2>

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

<li>Try the <a href="usb_linux_userspace.php">user-space USB driver</a></li>

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
