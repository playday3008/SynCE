<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<HTML>
<HEAD>
	<META HTTP-EQUIV="CONTENT-TYPE" CONTENT="text/html; charset=iso-8859-15">
	<TITLE>SynCE</TITLE>
	<META NAME="GENERATOR" CONTENT="OpenOffice.org 2.0  (Linux)">
	<META NAME="CREATED" CONTENT="20060218;11040700">
	<META NAME="CHANGED" CONTENT="20060302;17301000">
</HEAD>
<BODY LANG="en-GB" DIR="LTR">
<P>Return to <A HREF="synce-kde-intro.php">SynCE for KDE</A>.</P>
<P>This page was written by Marc (bonaventur) in March, 2006. Please sent corrections and suggestions to the <A HREF="http://sourceforge.net/mailarchive/forum.php?forum_id=15200">synce-users mailing list</A>.</P>
<H1>Step 2 &ndash; Basic configuration of SynCE for KDE</H1>
<P>We explain here how to establish the connection between your PDA
and the KDE desktop. We assume that you installed successfully
SynCE-KDE from packages, if it is not the case, go <A HREF="synce-kde-install-package.php">there</A>.</P>
<P STYLE="margin-top: 0.42cm; page-break-after: avoid"><FONT FACE="Albany, sans-serif"><FONT SIZE=4>Introduction</FONT></FONT></P>
<P>In order to configure the software, you need to have an idea about how it works. There
are several parts in the software</P>
<UL>
	<LI><P>The kernel module <FONT FACE="Courier 10 Pitch">ipaq</FONT>
	gives access to the device by the mean of the character device
	<FONT FACE="Courier 10 Pitch">/dev/ttyUSB0</FONT></P>
	<LI><P>The utilities <FONT FACE="Courier 10 Pitch">synce-serial-*</FONT>
	helps in establishing a ppp connection between the device and a
	daemon</P>
	<LI><P>The program <FONT FACE="Courier 10 Pitch">vdccm</FONT> is
	that daemon. It is the host for the ppp connection and it serves as
	an intermediary for all other requests from KDE</P>
	<LI><P>Then, all other utilities connect to <FONT FACE="Courier 10 Pitch">vdccm</FONT>
	in order to communicate with the PDA. We can mention the protocol
	rapip:/ used to browse the PDA in Konqueror, and Raki which will help
	you with all other tasks.</P>
</UL>
<P>You need to be aware of this in order to understand correctly the
configuration process. However, once your desktop is correctly
configured, you can forget about <FONT FACE="Courier 10 Pitch">synce-serial</FONT>
and <FONT FACE="Courier 10 Pitch">vdccm</FONT>, and concentrate on
enjoying Raki and its features.</P>
<P>This tutorial has been successfully tested under the following
configuration: Desktop PC running Suse Linux 10.0 with default synce
packages (again, this was good for the test below, but you should use
the new rpm package since the default ones are buggy) connecting to an HP iPAQ
rz 1710.</P>
<P STYLE="margin-top: 0.42cm; page-break-after: avoid"><FONT FACE="Albany, sans-serif"><FONT SIZE=4>About
the kernel module</FONT></FONT></P>
<P>Connect your device to your desktop PC using the usb cable. Then,
open a Konsole window, log in as root and examine the output of
<FONT FACE="Courier 10 Pitch">dmesg</FONT>. If you get something
similar to the output below, this is fine and you should annotate the
character device (in red). Else, make sure the <FONT FACE="Courier 10 Pitch">ipaq</FONT>
module exists in your kernel, and load it manually with the command
<FONT FACE="Courier 10 Pitch">modprobe ipaq</FONT>.</P>
<P STYLE="margin-left: 2cm; margin-bottom: 0cm"><FONT FACE="Courier 10 Pitch">marc@kiwi:~&gt;
<FONT COLOR="#00ff00">su<BR></FONT>Password:<BR>kiwi:/home/marc #
<FONT COLOR="#00ff00">dmesg<BR></FONT>(...a few lines here...)<BR>usb
1-1: new full speed USB device using uhci_hcd and address 2<BR>usb
1-1: device descriptor read/64, error -110<BR>usbcore: registered new
driver usbserial<BR>drivers/usb/serial/usb-serial.c: USB Serial
support registered for Generic<BR>usbcore: registered new driver
usbserial_generic<BR>drivers/usb/serial/usb-serial.c: USB Serial
Driver core v2.0<BR>drivers/usb/serial/usb-serial.c: USB Serial
support registered for PocketPC PDA<BR>drivers/usb/serial/ipaq.c: USB
PocketPC PDA driver v0.5<BR>ipaq 1-1:1.0: PocketPC PDA converter
detected<BR>usb 1-1: PocketPC PDA converter now attached to <FONT COLOR="#ff0000">ttyUSB0</FONT></FONT></P>
<P STYLE="margin-bottom: 0cm">If you experience difficulties here,
have a look <A HREF="http://synce.sourceforge.net/synce/howto.php">here</A>
or explore the archives of the diffusion list
<A HREF="http://sourceforge.net/mailarchive/forum.php?forum_id=15200">synce-users</A>.
</P>
<P STYLE="margin-top: 0.42cm; page-break-after: avoid"><FONT FACE="Albany, sans-serif"><FONT SIZE=4>Configuration
of synce-serial</FONT></FONT></P>
<P>Do not remove your device and (still as super-user) run the command below. Make sure to replace
<FONT FACE="Courier 10 Pitch">ttyUSB0</FONT> with the appropriate
device !</B></P>
<P STYLE="margin-left: 2cm; margin-bottom: 0cm"><FONT COLOR="#000000"><FONT FACE="Courier 10 Pitch">kiwi:/home/marc
# <FONT COLOR="#00ff00">synce-serial-config /dev/ttyUSB0<BR></FONT>You
can now run synce-serial-start to start a serial connection.</FONT></FONT></P>
<P STYLE="margin-bottom: 0cm">And then try</P>
<P STYLE="margin-left: 2cm; margin-bottom: 0cm"><FONT COLOR="#000000"><FONT FACE="Courier 10 Pitch">kiwi:/home/marc
# <FONT COLOR="#00ff00">synce-serial-start<BR></FONT>Warning!<BR>synce-serial-start
cannot find the dccm process.<BR>Without dccm your PPP connection
will soon terminate!<BR><BR>Serial connection established.<BR>Using
interface ppp0<BR>Connect: ppp0 &lt;--&gt; /dev/ttyUSB0<BR>local IP
address 192.168.131.102<BR>remote IP address 192.168.131.201<BR>Script
/etc/ppp/ip-up finished (pid 7154), status = 0x0</FONT></FONT></P>
<P STYLE="margin-bottom: 0cm">If you get this, everything is fine.
Kill the synce-serial-start process with Ctrl+C. It says:</P>
<P STYLE="margin-left: 2cm; margin-bottom: 0cm"><FONT COLOR="#000000"><FONT FACE="Courier 10 Pitch">Terminating
on signal 2<BR>Connect time 2.9 minutes.<BR>Sent 0 bytes, received
1567 bytes.<BR>Connection terminated.<BR>Script /etc/ppp/ip-down
finished (pid 7327), status = 0x0<BR><BR>synce-serial-start was
unable to start the PPP daemon!</FONT></FONT></P>
<P STYLE="margin-top: 0.42cm; page-break-after: avoid"><FONT FACE="Albany, sans-serif"><FONT SIZE=4>Configuration
of the firewall</FONT></FONT></P>
<P>Since the connection between the PDA and the desktop PC is a modem
connection, we need to make sure that no firewall rule impedes the
connection. 
</P>
<P>Open a new tab in the same Konsole window (session-&gt;New shell).
It is important here NOT to connect as root user, since we are going
to launch <FONT FACE="Courier 10 Pitch">vdccm</FONT> which is the
intermediary between your KDE session and the PDA. Issue the
following command:</P>
<P STYLE="margin-left: 2cm; margin-bottom: 0cm"><FONT COLOR="#000000"><FONT FACE="Courier 10 Pitch">marc@kiwi:~&gt;
<FONT COLOR="#00ff00">pkill dccm ; pkill vdccm<BR></FONT>marc@kiwi:~&gt;
<FONT COLOR="#00ff00">vdccm -d 3 -f</FONT></FONT></FONT></P>
<P STYLE="margin-bottom: 0cm">Nothing more should happen. This is
FINE : <FONT FACE="Courier 10 Pitch">vdccm</FONT> is waiting for
<FONT FACE="Courier 10 Pitch">synce-serial-start</FONT> to connect.
So, let <FONT FACE="Courier 10 Pitch">vdccm</FONT> running and come
back to the first tab. Make sure that there is no error message on
the PDA, and launch again</P>
<P STYLE="margin-left: 2cm; margin-bottom: 0cm"><FONT COLOR="#000000"><FONT FACE="Courier 10 Pitch">kiwi:/home/marc
# <FONT COLOR="#00ff00">synce-serial-start<BR></FONT>Serial
connection established.<BR>Using interface ppp0<BR>Connect: ppp0 &lt;--&gt;
/dev/ttyUSB0<BR>local IP address 192.168.131.102<BR>remote IP address
192.168.131.201<BR>Script /etc/ppp/ip-up finished (pid 7751), status
= 0x0</FONT></FONT></P>
<P STYLE="margin-bottom: 0cm">If <FONT FACE="Courier 10 Pitch">vdccm</FONT>
does not output anything, then there is a problem with the firewall.
In the case of Suse 10.0, to allow the connection to use interface
ppp0 please proceed as follows: start YaST, go to the
Security Section. Click Firewall. Chose section Interfaces and select
&ldquo;Personalize&rdquo;. Then, add &ldquo;ppp0&rdquo; to the field
&ldquo;External Zone&rdquo;. Go to the section "Allowed services" and click on "advanced" to open ports 990, 5678 and 5679.
</P>
<P STYLE="margin-bottom: 0cm">Note: another possibility is to add the interface ppp0 to the internal zone... But I guess the first one is safer.
</P>
<P STYLE="margin-bottom: 0cm; font-weight: medium">Then, kill
<FONT FACE="Courier 10 Pitch">synce-serial-start</FONT> and start it
anew. If<FONT FACE="Courier 10 Pitch"><FONT COLOR="#000000"> vdccm</FONT></FONT>
does not answer, unplug the PDA, remove all error messages from
ActiveSync and start again until you obtain:</P>
<P STYLE="margin-left: 2cm; margin-bottom: 0cm"><FONT COLOR="#000000"><FONT FACE="Courier 10 Pitch">marc@kiwi:~&gt;
vdccm -d 3 -f<BR>info package (110 bytes):<BR>0000: 28 00 00 00 04 15
40 04 (.....@.<BR>0008: 11 0a 00 00 00 00 00 00 ........<BR>0010: e1
66 b4 3a 63 d2 a8 3e .f.:c..&gt;<BR>0018: 28 00 00 00 3c 00 00 00
(...&lt;...<BR>0020: 50 00 00 00 00 00 00 00 P.......<BR>0028: 50 00
6f 00 63 00 6b 00 P.o.c.k.<BR>0030: 65 00 74 00 5f 00 50 00
e.t._.P.<BR>0038: 43 00 00 00 50 00 6f 00 C...P.o.<BR>0040: 63 00 6b
00 65 00 74 00 c.k.e.t.<BR>0048: 50 00 43 00 00 00 00 00
P.C.....<BR>0050: 68 00 70 00 20 00 69 00 h.p...i.<BR>0058: 50 00 41
00 51 00 20 00 P.A.Q...<BR>0060: 72 00 7a 00 31 00 37 00
r.z.1.7.<BR>0068: 31 00 30 00 00 00 1.0...<BR>0070:</FONT></FONT></P>
<P STYLE="margin-top: 0.42cm; page-break-after: avoid"><FONT FACE="Albany, sans-serif"><FONT SIZE=4>Browsing
the device</FONT></FONT></P>
<P STYLE="margin-bottom: 0cm">Congratulations !! From now on, the
connection is established and you can already browse your device as
rapip:/ in Konqueror, just like in the screenshot below:</P>
<P STYLE="margin-bottom: 0cm"><IMG SRC="screenshots/rapip.png" NAME="Image1" ALIGN=LEFT WIDTH=488 HEIGHT=337 BORDER=0><BR CLEAR=LEFT><BR>
</P>
<P STYLE="margin-top: 0.42cm; page-break-after: avoid"><FONT FACE="Albany, sans-serif"><FONT SIZE=4>Starting
Raki</FONT></FONT></P>
<P STYLE="margin-bottom: 0cm">The SynCE for KDE project has
elaborated a plugin for KDE's taskbar which provides a very pleasant
integration of SynCE features in KDE. Unplug your device and make
sure you kill  <FONT FACE="Courier 10 Pitch"><FONT COLOR="#000000">vdccm</FONT></FONT>.
As a normal user, start  <FONT FACE="Courier 10 Pitch"><FONT COLOR="#000000">raki</FONT></FONT>.
Note that it is a good idea to start it from the Konsole if you want
to keep an eye on the debugging messages.</P>
<P STYLE="margin-bottom: 0cm; font-weight: medium">In the welcome
screen below, choose vdccm instead of dccm:</P>
<P STYLE="margin-bottom: 0cm"><IMG SRC="screenshots/raki-firsttime.png" NAME="Image2" ALIGN=LEFT WIDTH=482 HEIGHT=358 BORDER=0><BR CLEAR=LEFT><BR>
</P>
<P STYLE="margin-bottom: 0cm">After that, a grey icon appears in your
task bar. It remains grey until a connection is made... And for
that, you just need to execute, as root, the well-known
<FONT FACE="Courier 10 Pitch">synce-serial-start</FONT>.</P>
<P STYLE="margin-bottom: 0cm; font-weight: medium">It is now up to
you to explore Raki's features... Once the connection is made,
click-left on Raki's icon to get the following menu:</P>
<P STYLE="margin-bottom: 0cm"><IMG SRC="screenshots/raki-taskbar.png" NAME="Image3" ALIGN=LEFT WIDTH=446 HEIGHT=325 BORDER=0><BR CLEAR=LEFT><BR>
</P>
<P STYLE="margin-top: 0.42cm; page-break-after: avoid"><FONT FACE="Albany, sans-serif"><FONT SIZE=4>Miscellaneous
: automating the connection</FONT></FONT></P>
<P STYLE="margin-bottom: 0cm">
If you are tired to launch
<FONT FACE="Courier 10 Pitch">synce-serial-start</FONT> (as root)
each time you plug the device, take some more time to read the
following: it is possible to automate the connection using <FONT FACE="Courier 10 Pitch">udev</FONT>.
To do so, create as root the file <FONT FACE="Courier 10 Pitch">/etc/udev/rules.d/60-ipaq.rules</FONT>
and fill it with the following code 
</P>
<P STYLE="margin-left: 2cm; margin-bottom: 0cm"><FONT COLOR="#000000"><FONT FACE="Courier 10 Pitch">#
udev rules file for SynCE<BR>BUS!=&quot;usb&quot;, ACTION!=&quot;add&quot;,
KERNEL!=&quot;ttyUSB*&quot;, GOTO=&quot;synce_rules_end&quot;<BR>#
Establish the
connection<BR>RUN+=&quot;/usr/bin/synce-serial-start&quot;<BR>LABEL=&quot;synce_rules_end&quot;</FONT></FONT></P>
<P STYLE="margin-bottom: 0cm"><BR>Then, restart <FONT FACE="Courier 10 Pitch">udev</FONT>
with the command <FONT FACE="Courier 10 Pitch">udevstart</FONT> (as
root). 
</P>
<P STYLE="margin-bottom: 0cm"><B>Note</B>: You might need to precise
the rule in order to ensure that it does not apply to another device.
Note also that on my system, it happens that <FONT FACE="Courier 10 Pitch">synce-serial-start</FONT>
started by <FONT FACE="Courier 10 Pitch">udev</FONT> ends badly and
uses all the processor resources - in that case, kill it manually
(try <FONT FACE="Courier 10 Pitch">top</FONT> as root). Make sure also to wait the notification of disconnection in Raki before plugging again the PDA.
</P>
<P STYLE="margin-bottom: 0cm">At last, note that more elaborated solutions have been proposed and should be integrated into SynCE soon. Have a look for example at 
<A HREF="http://sourceforge.net/mailarchive/forum.php?forum_id=15200&max_rows=25&style=flat&viewmonth=200602&viewday=24">those messages</A>.
</BODY>
</HTML>
