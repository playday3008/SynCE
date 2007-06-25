<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="index.php">main page</a>.</p>

<h1>SynCE - Linux USB HOWTO</h1>

<p><b>Please note that this HOWTO is not yet finished!</b></p>

<p>The purpose of this HOWTO is to connect a <b>single Windows CE device</b>
with <b>USB support</b> to a <b>PC running a Linux distribution</b> with a
<b>2.6.x kernel</b>, as this is assumed to be the most common setup for SynCE
users.</p>

<p>If this does not apply to your system, go to the <a href="nohowto.php">No
HOWTO</a> page.</p>

<p>In case you are not on a 32-bit x86 system but for example using a 64-bit
system (such as AMD64) or a big-endian system (such as PowerPC), there might be
some additional quirks. You are very welcome to write to the <a
href="mailto:synce-users@lists.sourceforge.net">
synce-users@lists.sourceforge.net</a> list about your experiences with SynCE on
such systems!</p>

<p>You must also be running a Linux kernel build that supports loadable
modules. (If you don't know what this means, you probably don't have to worry
about this!)</p>

<h2>1. Special note for owners of Microsoft Smartphone devices</h2>

<p>These devices <b>are not guaranteed to work</b> with any released Linux
kernel! Read on for the solution!</p>

<p>This note is known or probable to apply to the following devices, but may
also apply to others:</p>

<ul>

<li>HTC Canary/Tanager (also known as <b>i-Mate Smartphone</b>, <b>Orange
SPV/SPV e100</b>, <b>Qtek 7070</b>)</li>

<li>HTC Voyager (also known as <b>i-Mate Smartphone 2</b>, <b>Orange SPV
e200</b>, <b>Qtek 8080</b>)</li>

<li>HTC Typhoon (also known as <b>Orange c500</b>, <b>Qtek 8010</b>) -- Dave
Jenkins reports that it works with an unpatched 2.6.11-1.1369_FC4 kernel.</li>

<li><b>Motorola MPx200</b></li>

<!-- <li><b>Neonode N1</b></li> -->

</ul>

<p>Without a fix for this problem, <tt>synce-serial-start</tt> just hangs,
because pppd will not be able to setup a PPP connection.</p>

<p>If you follow the <a href="usb_linux_debug.php">USB debug</a> instructions
for any of the device you will see something very similar to this in the debug
log, here with the most important line marked with red color:</p>

<blockquote><pre>pppd[1061]: pppd 2.4.2 started by root, uid 0
kernel: usbserial.c: serial_open
kernel: ipaq.c: ipaq_open - port 0
kernel: host/usb-uhci.c: interrupt, status 3, frame# 1541
kernel: ipaq.c: ipaq_read_bulk_callback - port 0
<span class=RED>kernel: ipaq.c: ipaq_read_bulk_callback - nonzero read bulk status received: -84</span>
kernel: usbserial.c: serial_ioctl - port 0, cmd 0x5416
kernel: usbserial.c: serial_ioctl - port 0, cmd 0x5401
kernel: usbserial.c: serial_ioctl - port 0, cmd 0x5401
kernel: usbserial.c: serial_ioctl - port 0, cmd 0x5404
kernel: usbserial.c: serial_chars_in_buffer = port 0
kernel: ipaq.c: ipaq_chars_in_buffer - queuelen 0
kernel: usbserial.c: serial_set_termios - port 0
kernel: usbserial.c: serial_ioctl - port 0, cmd 0x5401
kernel: usbserial.c: serial_open
kernel: usbserial.c: serial_close - port 0</pre></blockquote>

<p>If you have a device with the error above, download <a
href="http://synce.sourceforge.net/tmp/kernel-2.6-driver.tar.gz">kernel-2.6-driver.tar.gz</a>,
extract and follow the instructions in the README file in order for your device
to work with SynCE!</p>

<p><i>Note: The support for Microsoft Smartphone will be simplied in the
future!</i></p>

<h2>2. Installation of the SynCE software</h2>

<p><b>Please note that this HOWTO is not yet finished!</b></p>

<p>For the time being, this part is very brief!</p>

<p>If you are running an RPM-based Linux distributions such as the ones in this
list, <a href="http://synce.sourceforge.net/synce/rpms.php">install the latest
RPM version</a> of SynCE:</p>

<ul>

<li>Fedora</li>
<li>RedHat</li>
<li>Suse</li>
<li>Mandrake</li>

</ul>

<p>If you are running Debian Testing or Unstable, the latest version of SynCE
should be available in your repository.</p>

<p>If you are running Gentoo, there is an ebuild for SynCE, but make sure it
uses the latest SynCE version!</p>

<p>If any of the above does not apply to you for some reason, you can <a
href="http://synce.sourceforge.net/synce/tarballs.php">compile SynCE
yourself</a>.</p>

<h2>3. Configuration of the kernel driver</h2>

<p><b>Please note that this HOWTO is not yet finished!</b></p>

<p>These actions described in this section have to be performed every time you
upgrade your Linux kernel!</p>

<p>SynCE uses a Linux kernel driver called <tt>ipaq</tt> in order to handle the
USB connection. You may need to patch this driver before you can make a
successful USB connection.</p>

<p>Everything in this section should be performed as the root user.</p>

<h3>If you compile the kernel yourself...</h3>

<p>Pre-compiled Linux kernels usually includes the <tt>ipaq</tt> and
<tt>ppp_async</tt> Linux kernel drivers that are required to use SynCE.</p>

<p>If you compile the Linux kernel yourself, make sure that these drivers are
included in your kernel configuration as loadable modules:</p>

<blockquote><tt>Device Drivers  ---&gt;<br>
USB Support ---&gt;<br>
USB Serial Converter support ---&gt;<br>
USB PocketPC PDA Driver</tt></blockquote>

<blockquote><tt>Device Drivers  ---&gt;<br>
Networking support  ---&gt;<br>
PPP (point-to-point protocol) support<br>
&nbsp;&nbsp;PPP support for async serial ports</tt></blockquote>

<h3>Kernel version</h3>

<p><i>Note: The recommended Linux kernel version for SynCE is 2.6.10 or later!</i></p>

<p>Run this command to find out your kernel version:</p>

<blockquote><tt>uname -r</tt></blockquote>

<p>Read more in the apropriate section below:</p>

<blockquote>

<p><b>2.4.x</b> Your kernel is not supported by this HOWTO, sorry. Use the <a
href="nohowto.php">No HOWTO</a> page.</p>

<p><b>2.5.x kernel</b> Your kernel is not supported at all by SynCE. We suggest
that you upgrade to a 2.6.x kernel!</p>

<p><b>2.6.x kernel</b> Keep going!</p>
</blockquote>

<a name="distro"></a>
<h3>Linux distribution</h3>

<p>If your Linux distribution is one of these and you have a kernel version
prior to 2.6.9 you need to patch your Linux kernel:</p>

<ul>
<li>Suse 9.1</li>
<li>Gentoo</li>
<li>Debian Unstable</li>
</ul>

<p>If this applies to you, download <a
href="http://synce.sourceforge.net/tmp/kernel-2.6-driver.tar.gz">kernel-2.6-driver.tar.gz</a>,
extract and follow the instructions in the README file.</p>

<a name="usbinfo"></a>
<h3>Find out USB information about your device</h3>

<p>In order to find out if your Linux kernel is ready for your Windows CE
device you shall connect the Windows CE device to the PC with the USB cable,
but first you should save a list of the current USB devices, to make it easy to
find the new device. Run this command:</p>

<blockquote><tt>cat /proc/bus/usb/devices &gt; /tmp/before</tt></blockquote>

<p>Now connect your Windows CE device and make sure it is turned on. Wait a few
seconds and run a similar command to get the list of USB devices including the
Windows CE device:</p>

<blockquote><tt>cat /proc/bus/usb/devices &gt; /tmp/after</tt></blockquote>

<p>After running the command above you should disconnect your Windows CE device.</p>

<p>Now you shall compare the two files to find out the USB information about
your device:</p>

<blockquote><tt>diff /tmp/before /tmp/after</tt></blockquote>

<p>The output from the above command will look something like this:</p>

<blockquote><pre>23a24,31
&gt; T:  Bus=02 Lev=01 Prnt=01 Port=00 Cnt=01 Dev#= 10 Spd=12  MxCh= 0
&gt; D:  Ver= 1.00 Cls=<span class=RED>ff(vend.)</span> Sub=<span class=RED>ff</span> Prot=<span class=RED>ff</span> MxPS= 8 <span class=RED>#Cfgs=  1</span>
&gt; P:  Vendor=<span class=RED>049f</span> ProdID=<span class=RED>0003</span> Rev= 0.00
&gt; C:* #Ifs= 1 Cfg#= 1 Atr=c0 MxPwr=  2mA
&gt; I:  If#= 0 Alt= 0 <span class=RED>#EPs= 2</span> Cls=ff(vend.) Sub=ff Prot=ff <span class=RED>Driver=ipaq</span>
<span class=RED>&gt; E:  Ad=01(O) Atr=02(Bulk) MxPS=  64 Ivl=0ms
&gt; E:  Ad=82(I) Atr=02(Bulk) MxPS=  16 Ivl=0ms</span>
&gt;</pre></blockquote>

<p>Important parts of the output have been marked with <span CLASS=RED>red
color</span>, and may be referenced in the instructions below.</p>

<h3>Class, subclass and protocol</h3>

<p>Does your USB device presents itself like this? Compare the parts
marked with red color.</p>

<pre>T:  Bus=02 Lev=01 Prnt=01 Port=00 Cnt=01 Dev#=  9 Spd=12  MxCh= 0
D:  Ver= 2.00 Cls=<span class=RED>ef(unk. )</span> Sub=<span class=RED>01</span> Prot=<span class=RED>01</span> MxPS=16 #Cfgs=  1
P:  Vendor=0bb4 ProdID=0b01 Rev= 0.00
S:  Manufacturer=MSFT
S:  Product=PocketPC USB Sync
C:* #Ifs= 2 Cfg#= 1 Atr=c0 MxPwr=500mA
I:  If#= 0 Alt= 0 #EPs= 1 Cls=ef(unk. ) Sub=01 Prot=01 Driver=(none)
E:  Ad=81(I) Atr=03(Int.) MxPS=   8 Ivl=1ms
I:  If#= 1 Alt= 0 #EPs= 2 Cls=0a(data ) Sub=00 Prot=00 Driver=(none)
E:  Ad=82(I) Atr=02(Bulk) MxPS=  64 Ivl=0ms
E:  Ad=03(O) Atr=02(Bulk) MxPS=  64 Ivl=0ms</pre>

<p>If your device matches the above it <b>does not work with SynCE or the
<tt>ipaq</tt> kernel driver</b> and will most likely crash your kernel when you
run ''synce-serial-start''. See bug report <a
href="https://sourceforge.net/tracker/index.php?func=detail&aid=1332550&group_id=30550&atid=399601">1332550</a>.
You are very welcome to amend your experiences to the bug report!</p>

<h3>The number of USB configurations</h3>

<p>First look at the value of <b>#Cfgs=</b>. If it's <b>1</b>, skip to <i>The
Driver entry</i> below! If it's <b>2</b> or more, continue this HOWTO but after
you have finished it please send a mail to <a
href="mailto:synce-devel@lists.sourceforge.net">
synce-devel@lists.sourceforge.net</a> and tell us:</p>

<ol>
<li>brand and model of your device</li>
<li>the output from <tt>diff /tmp/before /tmp/after</tt></li>
<li>if it worked properly with this HOWTO</li>
</ol>


<h3>The Driver entry</h3>

<p>Second you look at the <b>Driver</b> entry. Read more in the apropriate
section below.</p>

<blockquote>

<p><b>Driver=ipaq</b> or <b>Driver=usbserial</b> Your kernel driver recognized
your device, good!</p>

<p><b>Driver=(none)</b> Your kernel driver did not recognize your device. You
need to perform some special configuration:</p>

<ol>

<!--
<li><p>Only if your Linux kernel is 2.6.10 or later: please send a mail to <a
href="mailto:synce-devel@lists.sourceforge.net">
synce-devel@lists.sourceforge.net</a> and tell us...</p>

<ol>
<li>your kernel version</li>
<li>brand and model of your device</li>
<li>vendor/product USB IDs for your device (see Vendor= and ProdID=)</li>
</ol>
</li>
-->

<li><p>Follow the instructions in Appendix A to add options like this, but
replace the red digits with the corresponding ones from the output from the
command you ran earlier:</p>

<blockquote><tt>vendor=0x<span class=RED>049f</span> product=0x<span
class=RED>0003</span></tt></blockquote>

<li><p>If you have the file <tt>/etc/rc.local</tt>, open it with a text editor and
add this line in order to have things working directly next time you restart
your computer:</p>

<blockquote><tt>/sbin/modprobe ipaq</tt></blockquote></li>

<li><p>Now run these commands to reload the ipaq module:</p>

<blockquote><pre>rmmod ipaq
modprobe ipaq</pre></blockquote>

<p>If you get the message <i>ERROR: Module ipaq does not exist in
/proc/modules</i> when running the <tt>rmmod</tt> command, just ignore it.</p></li>

<li><p>If you got no output from the <tt>modprobe</tt> command (meaning it
succeeded), restart this HOWTO from the <a href="#usbinfo">Find out USB
information about your device</a> section.</p></li>

<li><p>If you get the message <i>FATAL: Module ipaq not found</i>, download <a
href="http://synce.sourceforge.net/tmp/kernel-2.6-driver.tar.gz">kernel-2.6-driver.tar.gz</a>,
extract and follow the instructions in the README file.</p></li>

<li><p>If you got another
error message, ask for <a href="help.php">help</a>!</p></li>
</ol>

<p><b>Another Driver entry</b> Ask for <a href="help.php">help</a>!</p>
</blockquote>

<h3>The number of USB endpoints</h3> 

<p>Next you look at the <b>#EPs=</b> entry or count the number of lines
beginning with <b>E:</b>, meaning the number of USB endpoints:</p>

<blockquote>
<p><b>Two or three USB endpoints</b> Nothing to do here, good!</p>

<p><b>Four USB endpoints</b> You need some special action here. Either follow <a
href="http://home.arcor.de/langeland/synce/">Stefan Langeland's
instructions</a> or these:</p>

<blockquote>

<p>First of all and unless you already did this previously, download <a
href="http://synce.sourceforge.net/tmp/kernel-2.6-driver.tar.gz">kernel-2.6-driver.tar.gz</a>,
extract and follow the instructions in the README file to upgrade your kernel
driver.</p>

<p>Second, follow the instructions in Appendix A to add the following option to
the kernel module:</p>

<blockquote><tt>ttyUSB=1</tt></blockquote>

<p>Now unload the kernel module and load it again:</p>

<blockquote><pre>rmmod ipaq
modprobe ipaq</pre></blockquote>

<p>No output from the above commands means success.</p>

<p><i>Note: The support for devices with four USB enpoints will be simplied in the
future!</i></p>

</blockquote>

<p><b>Another number of endpoints</b> Ask for <a href="help.php">help</a>!</p>
</blockquote>



<h2>4. Configuration of the connection</h2>

<p><b>Please note that this HOWTO is not yet finished!</b></p>

<p>Everything in this section should be performed as the root user.</p>

<ol>

<li>Connect your Windows CE device</li>

<li><p>Try the following synce-serial-config commands until one of them succeed:</p>

  <ul>
    <li><tt>synce-serial-config ttyUSB0</tt></li>
    <li><tt>synce-serial-config tts/USB0</tt></li>
    <li><tt>synce-serial-config usb/tts/0</tt></li>
    <li><tt>synce-serial-config usb/ttyUSB0</tt></li>
  </ul>

  </li>
  

<p><b>Important!</b> If you had four USB endpoints in the USB information for
your device instead of the usual two, you should use <b>1</b> instead of
<b>0</b> in the command above!</p>

<p>If you get the error message <i>synce-serial-config was unable to find a
character device named...</i>, ask for <a href="help.php">help</a>!</p>
</li>

</ol>

</blockquote>

<p>This does not have to be done again on your system unless your device
appears on a different tty for some reason.</p>


<h2>5. Configuring the firewall</h2>

<p><b>New section of the HOWTO!</b></p>

<p>SynCE requires the following TCP ports to be open in the firewall between your PC and the PDA:</p>

<blockquote>
<p><b>From PDA to PC</b></p>

<ul>
<li>5678</li>
<li>5679</li>
</ul>

<p><b>From PC to PDA</b></p>

<ul>
<li>990</li>
</ul>
</blockquote>

<p>Refer to the documenation of your Linux distribution for details on how to
allow traffic on these ports between your PC and PDA. Some details that might
be of interest when configuring the firewall:</p>

<p>When the synce-serial scripts are used, the PDA is connected via PPP. The
default local IP address of the PPP connection is  192.168.131.102 and the
default remote IP address is 192.168.131.201. These IP addresses can be changed
by passing different parameters to the <tt>synce-serial-config</tt> tool. Run
<tt>synce-serial-config</tt> without parameters or read its man page to know
more.</P>

<p>If the <tt>synce-serial-start</tt> script (see the next section, "Starting
the connection") detects any firewall rules whatsoever you will see this
message:</p>

<blockquote><pre>Warning!

You have firewall rules that may prevent SynCE from working properly!</pre></blockquote>

<p>This is just a warning and can be ignored if you know that your firewall
will not block the ports needed by SynCE.</p>


<h2>6. Starting the connection</h2>

<p>For the time being, this part is very brief!</p>

<ol>

<li><p>As your own user (not root), start dccm:</p>
<blockquote><tt>dccm</tt> (if your device is not password-protected)<br/>
<tt>dccm -p <i>password</i></tt> (if your device is password-protected)</blockquote>
<p>This must be done after each time you have rebooted your computer.</p>
</li>

<li>Connect your Windows CE device</li>

<li><p>As root, run this command:</p>

<blockquote><tt>synce-serial-start</tt></blockquote>

</li>

<li>If <tt>synce-serial-start</tt> just hangs with the message
"synce-serial-start is now waiting for your device to connect" and you have a
Smartphone, did you really follow the instructions in section 1 of this
HOWTO?</li>

</ol>

<h2>7. Testing the connection</h2>

<p>Applications and tools (except for synce-serial-*) that use SynCE must be
executed by the same user as is running dccm (not root).</p>

<p>As your own user (not root), try this command:</p>

<blockquote><tt>pstatus</tt> (not on Debian)<br>
<tt>synce-pstatus</tt> (on Debian, in the librapi2-tools package)</blockquote>

<p>If you successfully got information about your device, congratulations for
enduring all the quirks involved in setting up SynCE! :-)</p>

<p>Now is a good time to play with the other <a href="tools.php">command line
tools</a> included with SynCE. However, don't forget steps 7 and 8 below!</p>

<p>If you get the message below, the connection <b>failed</b>, and you should
make sure that you followed all the steps in this HOWTO properly.</p>

<blockquote><tt>pstatus: Unable to initialize RAPI: An unspecified failure has
occurred</tt></blockquote>

<p>If you truly did follow the HOWTO, you may want to <a href="help.php">get
help</a>.</p>

<h2>8. Disconnection</h2>

<p>In order to properly disconnect your device, you should first close the
network connection between Windows CE and SynCE. There are a couple of ways to
do this.</p>

<p>Just unplugging the USB cable is <b>not</b> proper disconnection when using
SynCE. (This will hopefully be fixed in the kernel driver some day.)</p>

<p>Before you unplug the cable you should do one of these actions, in order of
preference:</p>

<ol>

<li class=SPACED>Disconnect with the GNOME Tray Icon or with SynCE-KDE</li>

<li class=SPACED>Disconnect with the appropriate action on your PDA</li>

<li class=SPACED>Run <tt>killall -HUP dccm</tt> from the command line</li>

<li class=SPACED>Run <tt>synce-serial-abort</tt>. Please note that this command is only to
be used when everything else fails. It also seems like it only works for USB
connections while the USB cable is connected.</li>

</ol>

<h2>9. Donation</h2>

<p>To show how grateful you are for SynCE, you can <a
href="http://sourceforge.net/donate/index.php?group_id=30550">donate</a>!</p>

<!-- <h2>8. Installing and using a hotplug script</h2>

<p>TODO</p> -->

<h2>Appendix A. Adding kernel module options</h2>

<p>This is a special appendix because the actions here varies between distros.</p>

<p>First select the first alternative to exist on your Linux distribution:</p>

<ol>

<li>The directory <tt>/etc/modprobe.d</tt></li>
<li>The directory <tt>/etc/modutils</tt></li>

<li>The file <tt>/etc/modprobe.conf.local</tt></li>
<li>The file <tt>/etc/modprobe.conf</tt></li>
<li>The file <tt>/etc/modules.conf</tt></li>

</ol>

<p>If one of the directories exists on your Linux distribution, use a text
editor to create the file <tt>synce</tt> in that directory. If you didn't have
any of the directories, open the file you have with a text editor.</p>

<p>Add a line like this, but replace <tt><i>options</i></tt> with the actual
options you need to add to the kernel module:</p>

<blockquote><tt>options ipaq <i>options</i></tt></blockquote>

<p>If you created or edited the <tt>/etc/modutils/synce</tt> file, run the
<tt>update-modules</tt> command.</li>

<h3>Examples</h3>

<p>If you have a device with vendor ID 0x1114 and product id 0x0006 that is not
recognized by the kernel driver:</p>

<blockquote><tt>options ipaq vendor=0x1114 product=0x0006</tt></blockquote>

<p>If you have a device with four USB endpoints:</p>

<blockquote><tt>options ipaq ttyUSB=1</tt></blockquote>

<p><br/>Return to <a href="index.php">main page</a>.</p>

</div>
<?php include 'footer.php'; ?>
