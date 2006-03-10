<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<HTML>
<HEAD>
	<META HTTP-EQUIV="CONTENT-TYPE" CONTENT="text/html; charset=iso-8859-15">
	<TITLE>SynCE</TITLE>
	<META NAME="GENERATOR" CONTENT="OpenOffice.org 2.0  (Linux)">
	<META NAME="CREATED" CONTENT="20060218;11040700">
	<META NAME="CHANGED" CONTENT="20060309;17004200">
</HEAD>
<BODY LANG="en-GB" DIR="LTR">
<P>Return to <A HREF="synce-kde-intro.php">SynCE for KDE</A>.</P>
<P>This page was written by Marc (bonaventur) in March, 2006. Please sent corrections and suggestions to the <A HREF="http://sourceforge.net/mailarchive/forum.php?forum_id=15200">synce-users mailing list</A>.</P>
<H1>Step 5 &ndash; SynCE for KDE: Advanced configuration</H1>
<P>In this page we give some information about how to configure some
advanced features of SynCE for KDE. Note that for some of these features you
might need to update your installation, see <A HREF="synce-kde-install-source.php">step
4</A>. 
<P><FONT FACE="Albany, sans-serif"><FONT SIZE=4>AvantGo
Synchronization</FONT></FONT></P>
<P>To use AvantGo synchronization, there are two requisites. You must compile the agsync plug-in and notify it to synce-kde (cf. <A HREF="synce-kde-install-source.php">step
4</A>), and you also need to register at 
<A HREF="http://www.avantgo.com">www.avantgo.com</A> (make sure to backup all the files provided during the registration process). Then:</P>
<OL>
	<LI><P>Install the AvantGo client on the device. IMPORTANT: Current versions provided by AvantGo are not supported, therefore you have to use a former one. Go to the <A HREF="http://sourceforge.net/project/showfiles.php?group_id=30550">download area</A> to find it.</P>
	<LI><P>In Raki, click on "configure pocket_pc". Enable the AvantGo synchronization and configure it according to the file <FONT FACE="Courier 10 Pitch">autoconfig.mal</FONT> provided by AvantGo during the registration process. Make sure the box "Install AvantGo software on the device" is unchecked.</P>
	<LI><P>Launch immediately the AvantGo synchronization.</P>
	<LI><P>If it does not work, this might be because the AvantGo password was not transmitted correctly to the device (this information is kept by the PDA, not by Raki). In that case, you can configure by hand your AvantGo account on the PDA (Go to: Start menu, Configuration, Connections, AvantGo).</P>
</OL>
<P STYLE="margin-top: 0.42cm; page-break-after: avoid"><FONT FACE="Albany, sans-serif"><FONT SIZE=4>IP
Forwarding</FONT></FONT></P>
<P>To access the internet from the device, when connected to the desktop
PC, follow the steps below:</P>
<OL>
	<LI><P>Enable sudo for iptables, by adding the line</P>
<P STYLE="margin-left: 2cm"><FONT FACE="Courier 10 Pitch"><I>user
host</I>= NOPASSWD:/usr/sbin/iptables</FONT></P>
<P>to the file <FONT FACE="Courier 10 Pitch">/etc/sudoers</FONT>, replacing
<FONT FACE="Courier 10 Pitch"><I>user</I></FONT>
with your user name and <FONT FACE="Courier 10 Pitch"><I>host</I></FONT>
with the host name of your desktop PC. Make sure that, as normal user, the command
<FONT FACE="Courier 10 Pitch"> sudo /usr/bin/iptables </FONT>does not ask for any password.</SPAN></FONT></P>
	<LI><P>Enable IP Forwarding on your system. On mine (Suse 10.0), this is done with Yast: in the section "Network services", chose the module "Routing". Then, check the box "Activate IP transmission". For some other systems (including Debian) execute: <FONT FACE="Courier 10 Pitch">echo 1 &gt;
/proc/sys/net/ipv4/ip_forward</FONT></P>
	<LI><P>Configure synce-serial and indicate your DNS (which is given by <FONT FACE="Courier 10 Pitch">cat /etc/resolv.conf</FONT>)</P>
<P STYLE="margin-left: 2cm"><FONT FACE="Courier 10 Pitch">synce-serial-config
/dev/ttyUSB0 192.168.131.102:192.168.131.201 <I>nameserver</I></FONT></P>
	<P>Notice that you device must be connected to the USB cable.</P>
	<LI><P>In Raki, configure your device and check "Enable Masquerading".</P>
	<LI><P>On the PDA, open the configuration panel. Go to connections, tap on "Connections", then "Advanced", then "Select Networks" and select "Work network" as the default internet connection (these are literal translations from Spanish, original names are welcome).</P>
</OL>
<P>
Next time the PDA is connected, Raki should tell you (in the console where it was started) "Masquerade route created", and the PDA should have access to the internet. </P>
<P STYLE="margin-top: 0.42cm; page-break-after: avoid"><FONT FACE="Albany, sans-serif"><FONT SIZE=4>Synchronization
with Kontact</FONT></FONT></P>
<P>Almost no configuration is needed for using that feature - just make sure the <FONT FACE="Courier 10 Pitch">syncekonnector</FONT> plugin is installed. Currently, the synchronization has to be done from Raki itself
(even if <FONT FACE="Courier 10 Pitch">syncekonnector</FONT> is a
plugin for kitchensync).</P>

<P>Note that at the time being, the
synchronization IS NOT SAFE and that it is necessary to make backups of your data
before tempting any synchronization. I personnally use the following script to backup Kontact's data:
</P>
<P STYLE="margin-left: 2cm"><FONT FACE="Courier 10 Pitch">export
now=`date +%y%m%d_%Hh%M`<BR>cp ~/.kde/share/apps/korganizer/std.ics
std_$now.ics<BR>cp ~/.kde/share/apps/kabc/std.vcf std_$now.vcf</FONT></P>
<P>
<P>I also warmly recommend that you proceed to a complete backup of your PDA. It already happend that some missynchronised events resulted in the freezing of the PDA, leading to an hard reset...
</P>


</BODY>
</HTML>
