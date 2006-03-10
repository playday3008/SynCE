<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<HTML>
<HEAD>
	<META HTTP-EQUIV="CONTENT-TYPE" CONTENT="text/html; charset=iso-8859-15">
	<TITLE>SynCE</TITLE>
	<META NAME="GENERATOR" CONTENT="OpenOffice.org 2.0  (Linux)">
	<META NAME="CREATED" CONTENT="20060218;11040700">
	<META NAME="CHANGED" CONTENT="20060302;16323300">
</HEAD>
<BODY LANG="en-GB" DIR="LTR">
<P>Return to <A HREF="http://synce.sourceforge.net/synce/">main page</A>.</P>
<P>This page was written by Marc (bonaventur) in March, 2006. Please sent corrections and suggestions to the <A HREF="http://sourceforge.net/mailarchive/forum.php?forum_id=15200">synce-users mailing list</A>.</P>
<H1>About SynCE for KDE</H1>
<P>If you own a Windows Mobile device and if you are a KDE user, this
project is probably what you were looking for. SynCE for KDE aims at
integrating your PDA into KDE, providing the expected communication
facilities.</P>
<P>In particular, the SynCE for KDE project permits you</P>
<UL>
	<LI><P>To browse the PDA file system by the mean of Konqueror, and
	to transfer files in both directions</P>
	<LI><P>To install CAB files directly to the PDA</P>
	<LI><P>To mirror the PDA screen on your PC desktop, being therefore
	able to use your main keyboard to control it, taking screenshots,
	etc.</P>
	<LI><P>To connect several PDAs at a time.</P>
</UL>
<P>It presents also advanced features like</P>
<UL>
	<LI><P>IP forwarding to access the internet from the PDA by the mean
	of the USB-cable connection</P>
</UL>
<UL>
	<LI><P>AvantGo synchronization</P>
</UL>
<UL>
	<LI><P>Synchronization between the PDA and Kontact organizer (still in active development at the moment).</P>
</UL>
<P>We suggest that you have a look at the screenshots section below to see
the main functionalities illustrated.</P>
<H1>How to install SynCE for KDE</H1>
<P>To install and use SynCE for KDE, we recommend that you follow the
steps below. Note that almost every information mentionned here has been tested, so we can raisonably expect this howto to be reliable. In case you had trouble with some point, check first if you followed this howto point by point, then contact the 
<A HREF="http://sourceforge.net/mailarchive/forum.php?forum_id=15200">synce-users</A> mailing list. </P>
<OL>
	<LI><P>Install SynCE for KDE <A HREF="synce-kde-install-package.php">from
	packages</A>. This is the recommended method if you are new to
	SynCE, and you should not use sources version or CVS versions until
	you are sure that the next step is completed.</P>
	<LI><P>Proceed with the basic configuration of SynCE. This is explained
	<A HREF="synce-kde-basic-configuration.php">here</A>.</P>
	<LI><P>Enjoy some functionalities of SynCE for KDE : get access to
	the device with Konqueror, install and uninstall cab files, etc.</P>
</OL>
<P>If you are already familiar with SynCE, then you can pass to the
following (advanced) steps:</P>
<OL START=4>
	<LI><P>(OPTIONAL) Compile and install SynCE for KDE from <A HREF="synce-kde-install-source.php">source
	or CVS</A>. Note that this step might be optional if your are using up-to-date packages: try first if you cannot get all features working with the precompiled version.</P>
	<LI><P>Proceed with the <A HREF="synce-kde-advanced-configuration.php">advanced
	configuration</A> of SynCE for KDE in order to use IP forwarding,
	AvantGo synchronization, use the synchronization framework for Kontact...</P>
</OL>
<H1>A few screenshots</H1>
<P>The following images are screenshots illustrating some
functionalities of SynCE for KDE. 
</P>
<P STYLE="margin-bottom: 0cm"><IMG SRC="screenshots/raki-taskbar.png" NAME="Image1" ALIGN=LEFT WIDTH=446 HEIGHT=325 BORDER=0><BR CLEAR=LEFT><FONT SIZE=2><I>Illustration
1: Integration of Raki in the KDE task bar</I></FONT></P>
<P STYLE="margin-bottom: 0cm"><IMG SRC="screenshots/rapip.png" NAME="Image2" ALIGN=LEFT WIDTH=488 HEIGHT=337 BORDER=0><BR CLEAR=LEFT><FONT SIZE=2><I>Illustration
2: Browsing the device using rapip:/</I></FONT></P>
<P><IMG SRC="screenshots/KCEMirror-SPV.png" NAME="Image5" ALIGN=LEFT WIDTH=184 HEIGHT=323 BORDER=0><BR CLEAR=LEFT><FONT SIZE=2><I>Illustration
3: Mirroring the device with KCEMirror</I></FONT></P>

<TABLE WIDTH=500 BORDER=0 CELLPADDING=0 CELLSPACING=0>
	<COL WIDTH=128*>
	<COL WIDTH=128*>
	<TR VALIGN=TOP>
		<TD WIDTH=280>
			<P><IMG SRC="screenshots/kcemirror-ag.png" NAME="Image3" ALIGN=LEFT WIDTH=248 HEIGHT=435 BORDER=0><BR CLEAR=LEFT></P>
		</TD>
		<TD WIDTH=280>
			<P><IMG SRC="screenshots/kcemirror-ip.png" NAME="Image4" ALIGN=LEFT WIDTH=248 HEIGHT=435 BORDER=0><BR CLEAR=LEFT></P>
		</TD>		
	</TR>
</TABLE>
<P><FONT SIZE=2><I>Illustration
			4:Usage of AvantGo and IP forwarding</I></FONT></P>
<P STYLE="margin-bottom: 0cm"><IMG SRC="screenshots/cab-context.png" NAME="Image6" ALIGN=LEFT WIDTH=566 HEIGHT=435 BORDER=0><BR CLEAR=LEFT><FONT SIZE=2><I>Illustration
5: Installing cab files</I></FONT></P>
<P STYLE="margin-bottom: 0cm"><IMG SRC="screenshots/raki-sync.png" NAME="Image7" ALIGN=LEFT WIDTH=476 HEIGHT=327 BORDER=0><BR CLEAR=LEFT><FONT SIZE=2><I>Illustration
6: Synchronization using Raki</I></FONT></P>
</BODY>
<H1>Further references</H1>
<P>In order to elaborate this howto, I had to compile information from several sources, the principal of them being the 
<A HREF="http://synce.sourceforge.net">synce web site</A> (pay attention, some information might be outdated) and the howto by Serhan D. Kiymaz at 
<A HREF="http://www.serotizm.com/howtos/syncekonnector-0.2.html">www.serotizm.com</A>, which I recommend if you need information specific to the Debian distribution.
</P>
<P>I also express my gratitude to Christian Volker, who helped me very patiently and efficiently for the elaboration of this howto.</P>
<P>The old and outdated SynCE-KDE webpages can be found <A HREF="old">here</A>.</P>
</HTML>
