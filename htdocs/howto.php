<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="index.php">main page</a>.</p>

<h1>SynCE - Linux USB HOWTO</h1>

<p><b>Please note that this HOWTO is not yet finished!</b></p>

<p>The purpose of this HOWTO is to connect a single Windows CE device with USB
support to a PC running Linux distribution with a 2.6.x kernel, as this is
assumed to be the most common setup for SynCE users.</p>

<p>You must also be running a Linux kernel build that supports loadable
modules. (If you don't know what this means, you probably don't have to worry
about this!)</p>

<h2>1. Installation of the SynCE software</h2>

<p><b>Please note that this HOWTO is not yet finished!</b></p>

<p>For the time being, this part is very brief!</p>

<p>If you are running an RPM-basen Linux distributions such as the ones in this
list, <a href="http://synce.sourceforge.net/synce/rpms.php">install the latest RPM version</a> of SynCE:</p>

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

<h2>2. Configuration of the kernel driver</h2>

<p><b>Please note that this HOWTO is not yet finished!</b></p>

<p>These actions described in this section have to be performed every time you
upgrade your Linux kernel!</p>

<p>SynCE uses a Linux kernel driver called <tt>ipaq</tt> in order to handle the
USB connection. You may need to patch this driver before you can make a
successful USB connection.</p>

<p>Everything in this section should be performed as the root user.</p>

<h3>Kernel version</h3>

<p>Run this command to find out your kernel version:</p>

<blockquote><tt>uname -r</tt></blockquote>

<p>Read more in the apropriate section below:</p>

<blockquote>

<p><b>2.4.x</b> Your kernel is not supported by this HOWTO, sorry.</p>

<p><b>2.5.x kernel</b> Your kernel is not supported at all by SynCE. We suggest
that you upgrade to a 2.6.x kernel!</p>

<p><b>2.6.x kernel</b> Keep going!</p>
</blockquote>

<a name="distro"></a>
<h3>Linux distribution</h3>

<p>If your Linux distribution is one of these you need to patch your Linux
kernel:</p>

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
&gt; D:  Ver= 1.00 Cls=ff(vend.) Sub=ff Prot=ff MxPS= 8 #Cfgs=  1
&gt; P:  Vendor=<span class=RED>049f</span> ProdID=<span class=RED>0003</span> Rev= 0.00
&gt; C:* #Ifs= 1 Cfg#= 1 Atr=c0 MxPwr=  2mA
&gt; I:  If#= 0 Alt= 0 #EPs= 2 Cls=ff(vend.) Sub=ff Prot=ff <span class=RED>Driver=ipaq</span>
<span class=RED>&gt; E:  Ad=01(O) Atr=02(Bulk) MxPS=  64 Ivl=0ms
&gt; E:  Ad=82(I) Atr=02(Bulk) MxPS=  16 Ivl=0ms</span>
&gt;</pre></blockquote>

<p>Important parts of the output have been marked with <span CLASS=RED>red
color</span>, and may be referenced in the instructions below.</p>

<h3>The Driver entry</h3>

<p>First you look at the <b>Driver</b> entry. Read more in the apropriate
section below.</p>

<blockquote>

<p><b>Driver=ipaq</b> Your kernel driver recognized your device, good!</p>

<p><b>Driver=(none)</b> Your kernel driver did not recognize your device. You
need to perform some special configuration:</p>

<blockquote>

<p>Use a text editor to open the file <tt>/etc/modprobe.conf</tt> if you have
it, otherwise open <tt>/etc/modules.conf</tt>. <p>Add a line like this, but
replace the red digits with the corresponding ones from the output from the
command you ran earlier:</p>

<blockquote><tt>options ipaq vendor=0x<span class=RED>049f</span> product=0x<span
class=RED>0003</span></tt></blockquote>

<p>If you have the file <tt>/etc/rc.local</tt>, open it with a text editor and
add this line in order to have things working directly next time you restart
your computer:</p>

<blockquote><tt>/sbin/modprobe ipaq</tt></blockquote>

<p>Now run these commands to reload the ipaq module:</p>

<blockquote><pre>rmmod ipaq
modprobe ipaq</pre></blockquote>

<p>If you get the message <i>ERROR: Module ipaq does not exist in
/proc/modules</i> when running the <tt>rmmod</tt> command, just ignore it.</p>

<p>If you got no output from the <tt>modprobe</tt> command (meaning it
succeeded), restart this HOWTO from the <a href="#usbinfo">Find out USB
information about your device</a> section.</p>

<p>If you get the message <i>FATAL: Module ipaq not found</i>, download <a
href="http://synce.sourceforge.net/tmp/kernel-2.6-driver.tar.gz">kernel-2.6-driver.tar.gz</a>,
extract and follow the instructions in the README file.</p>

<p>If you got another
error message, ask for <a href="help.php">help</a>!</p>

</blockquote>

<p><b>Another Driver entry</b> Ask for <a href="help.php">help</a>!</p>
</blockquote>

<h3>The number of USB endpoints</h3> 

<p>Next you count the number of lines beginning with <b>E:</b>, meaning the
number of USB endpoints:</p>

<blockquote>
<p><b>Two or three USB endpoints</b> Nothing to do here, good!</p>

<p><b>Four USB endpoints</b> You need some special action here:</p>

<blockquote>

<p>First, even if you already did this in the <a href="#distro">Linux
distribution</a> section, download <a
href="http://synce.sourceforge.net/tmp/kernel-2.6-driver.tar.gz">kernel-2.6-driver.tar.gz</a>,
extract, replace the file <tt>free_len_zero.patch</tt> with <a
href="patches/mitac_mio168.patch">mitac_mio168.patch</a>, and follow the
instructions in the README file.</p>

<p>Second, use a text editor to open the file <tt>/etc/modprobe.conf</tt> if
you have it, otherwise open <tt>/etc/modules.conf</tt>. Add a line like this:</p>

<blockquote><tt>options ipaq ttyUSB=1</tt></blockquote>

<p>Now unload the kernel module and load it again:</p>

<blockquote><pre>rmmod ipaq
modprobe ipaq</pre></blockquote>

<p>No output from the above commands means success.</p>

</blockquote>

<p><b>Another number of endpoints</b> Ask for <a href="help.php">help</a>!</p>
</blockquote>



<h2>3. Configuration of the serial connection</h2>

<p><b>Please note that this HOWTO is not yet finished!</b></p>

<p>Everything in this section should be performed as the root user.</p>

<p>For many systems, this works:</p>

<blockquote><tt>synce-serial-config ttyUSB0</tt></blockquote>

<p><b>Important!</b> If you had four USB endpoints in the USB information for
your device instead of the usual two, you should use ttyUSB1 instead of
ttyUSB0 in the command above!</p>

<p>If you get the error message <i>synce-serial-config was unable to find a
character device named "ttyUSB0"</i>, you may be using <b>devfs</b> and need to
do this a little differently:</p>

<blockquote>

<ol>

<li>Connect your Windows CE device</li>

<li><p>Run synce-serial-config like this:</p>

<blockquote><tt>synce-serial-config usb/tts/0</tt></blockquote>

<p>Use usb/tts/1 as parameter instead if you have four USB endpoints instead of
the usual two.</p>

</li>

</ol>

</blockquote>

<h2>4. Starting the serial connection</h2>

<p>For the time being, this part is very brief!</p>

<ol>

<li><p>As your own user (not root), start dccm:</p>
<blockquote><tt>dccm</tt></blockquote>
<p>This must be done after each time you have rebooted your computer.</p>
</li>

<li>Connect your Windows CE device</li>

<li><p>As root, run this command:</p>

<blockquote><tt>synce-serial-start</tt></blockquote>

</li>

</ol>

<h2>5. Testing the connection</h2>

<p>As your own user (not root), try this command:</p>

<blockquote><tt>pstatus</tt></blockquote>

<p>If you get the message below, the connection <b>failed</b>, and you should
make sure that you followed all the steps in this HOWTO properly.</p>

<blockquote><tt>pstatus: Unable to initialize RAPI: An unspecified failure has
occurred</tt></blockquote>


<h2>6. Installing and using a hotplug script</h2>

<p>TODO</p>

<p>Return to <a href="index.php">main page</a>.</p>

</div>
<?php include 'footer.php'; ?>
