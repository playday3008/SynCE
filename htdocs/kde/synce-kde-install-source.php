<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<HTML>
<HEAD>
	<META HTTP-EQUIV="CONTENT-TYPE" CONTENT="text/html; charset=iso-8859-15">
	<TITLE>SynCE</TITLE>
	<META NAME="GENERATOR" CONTENT="OpenOffice.org 2.0  (Linux)">
	<META NAME="CREATED" CONTENT="20060218;11040700">
	<META NAME="CHANGED" CONTENT="20060303;22355900">
</HEAD>
<BODY LANG="en-GB" DIR="LTR">
<P>Return to <A HREF="synce-kde-intro.php">SynCE for KDE</A>.</P>
<P>This page was written by Marc (bonaventur) in March, 2006. Please sent corrections and suggestions to the <A HREF="http://sourceforge.net/mailarchive/forum.php?forum_id=15200">synce-users mailing list</A>.</P>
<H1>Step 4 &ndash; Installing SynCE for KDE from sources or CVS</H1>
<P>We describe here how to install SynCE for KDE from latest sources,
or even from CVS sources. Note that this may be an optional step, you
should check first whereas the packaged version is already sufficient
for your purpose (pass on directly to the
<A HREF="synce-kde-advanced-configuration.php">next step</A>).</P>
<P STYLE="margin-top: 0.42cm; page-break-after: avoid"><FONT FACE="Albany, sans-serif"><FONT SIZE=4>Requisites</FONT></FONT></P>
<P>We list here some requisites for compiling and installing SynCE
for KDE from source. However, it could be that the list is not exhaustive, in that case please keep me informed.</P>
<OL>
	<LI><P>Make sure to uninstall any previous version of SynCE. This is
	done either using your package manager, either (if you already
	installed SynCE from sources) executing command <FONT FACE="Courier 10 Pitch">make
	uninstall</FONT> for every component of SynCE.</P>
	<LI><P>Make sure all the compilation tools needed are installed,
	namely check whereas <FONT FACE="Courier 10 Pitch">gcc, pkgconfig,
	automake, autoconf</FONT> are
	present on your system.</P>
	<LI><P>Install the development headers
	<FONT FACE="Courier 10 Pitch">glib2-devel, xfree-devel,
	zlib-devel, qt3-devel, kdebase3-devel, kdepim3-sync</FONT>
	and <FONT FACE="Courier 10 Pitch">kdepim3-devel</FONT> (this list corresponds to a system
	running Suse 10.0)</P>
	<LI><P>Update to KDE 3.5 if you wish to use the synchronization with Kontact.</P>
</OL>
<P STYLE="margin-top: 0.42cm; page-break-after: avoid"><FONT FACE="Albany, sans-serif"><FONT SIZE=4>Downloading
the sources</FONT></FONT></P>
<P>You will find the latest source packages at the <A HREF="http://sourceforge.net/project/showfiles.php?group_id=30550">download
page</A>. Gather the following components:</P>
<P STYLE="margin-left: 2cm"><FONT FACE="Courier 10 Pitch">libmimedir<BR>synce-serial<BR>synce-libsynce,
synce-librapi2, synce-rra<BR>dynamite, unshield, orange<BR>synce-kde,
kcemirror, syncekonnector</FONT></P>
<P STYLE="margin-top: 0.42cm; page-break-after: avoid"><FONT FACE="Albany, sans-serif"><FONT SIZE=4>Downloading
the sources from CVS</FONT></FONT></P>
<P>In the case you prefer to use the very latest development
versions, you need to:</P>
<OL>
	<LI><P>Retrieve the <FONT FACE="Courier 10 Pitch">libmimedir</FONT>
	and the <FONT FACE="Courier 10 Pitch">synce-serial</FONT> tarballs
	from the project's <A HREF="http://sourceforge.net/project/showfiles.php?group_id=30550">download
	page</A>. 
	</P>
	<LI><P>In the directory where you wish to retrieve CVS sources,
	execute:</P>
</OL>
<P STYLE="margin-left: 2cm"><FONT FACE="Courier 10 Pitch">cvs -z3
-d:pserver:anonymous@cvs.sourceforge.net:/cvsroot/synce co -P
libsynce librapi2 dynamite unshield orange rra synce-kde
syncekonnector kcemirror</FONT></P>
<P>Note that CVS sources might lead to an unstable installation
of SynCE !</P>
<P STYLE="margin-top: 0.42cm; page-break-after: avoid"><FONT FACE="Albany, sans-serif"><FONT SIZE=4>Compilation</FONT></FONT></P>
<P>To compile and install SynCE for KDE from source, you just need to
proceed as follow: for every component in the list above, and
<B>according to that order</B>,
execute as super user either:</P>
<OL>
	<LI><P>For a source tarball:</P>
	<P STYLE="margin-left: 2cm"><FONT FACE="Courier 10 Pitch">tar
zxvf module.tar.gz<BR>cd
module/<BR>./configure<BR>make<BR>make install<BR>cd ..</FONT></P>
	<LI><P>For a CVS module</P>
	<P STYLE="margin-left: 2cm"><FONT FACE="Courier 10 Pitch">cd
module/<BR>./bootstrap<BR>./configure<BR>make<BR>make install<BR>cd
..</FONT></P>
</OL>
<P STYLE="margin-top: 0.42cm; page-break-after: avoid"><FONT FACE="Albany, sans-serif"><FONT SIZE=4>Extra for AvantGo Synchronization</FONT></FONT></P>
<P>If you want to include the AvantGo Synchronization plugin, there is an extra step. Download first the <A HREF="http://duskwood.lownewulf.com/agsync-0.2-pre.tgz">AvantGo plugin</A>. Compile it with <FONT FACE="Courier 10 Pitch">make</FONT>. Then, at the time of compiling <FONT FACE="Courier 10 Pitch">synce-kde</FONT>, make sure to configure it as
<FONT FACE="Courier 10 Pitch">./configure --with-agsync=/temp/agsync</FONT>, replacing <FONT FACE="Courier 10 Pitch">/temp/agsync</FONT> with the location of the compiled agsync plugin.
</P>

<P STYLE="margin-top: 0.42cm; page-break-after: avoid"><FONT FACE="Albany, sans-serif"><FONT SIZE=4>In
case of problems</FONT></FONT></P>
<P>Theoretically,
you should not observe any problem. However, I personally experienced
the following:</P>
<OL>
	<LI><P><FONT FACE="Courier 10 Pitch">synce-serial</FONT>
	had to be configured with the command <FONT FACE="Courier 10 Pitch">./configure --prefix=/usr</FONT>, else it was not possible to launch <FONT FACE="Courier 10 Pitch">synce-serial-start</FONT></P>
	<LI><P>Version
	0.9.1 of synce-rra issued an error during the compilation, to fix
	it, I had to remove the flag -Werror from the file lib/Makefile.</P>
</OL>
<P STYLE="margin-top: 0.42cm; page-break-after: avoid"><FONT FACE="Albany, sans-serif"><FONT SIZE=4>Uninstallation</FONT></FONT></P>
<P>To uninstall a given component of SynCE, you just need to issue
the command <FONT FACE="Courier 10 Pitch">make uninstall</FONT>
 in the corresponding directory. To
uninstall SynCE entirely, just do this for each component...</P>

<P>Once you've overcome the installation, and assuming that the 
<A HREF="synce-kde-basic-configuration.php">basic configuration</A> has been done, you can pass to the <A HREF="synce-kde-advanced-configuration.php">advanced configuration</A>.</P>
</BODY>
</HTML>
