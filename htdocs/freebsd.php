<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="index.php">main page</a>.</p>

<h1>SynCE - FreeBSD support</h1>

<h2>How to get</h2>

Most parts of SynCE are available ready to compile and install from
the <a href="http://www.freebsd.org/ports/">FreeBSD ports tree</a>.
Users wanting binary packages should install using <tt>pkg_add</tt>
with the remote fetch (<tt>-r</tt>) flag. Comments and
<i>FreeBSD-specific</i> support requests can be emailed to
<a href="mailto:boris@brooknet.com.au">Sam Lawrance</a>. Please do not
email this address with general support requests. For general support,
look <a href="help.php">here</a>.<p> 

<h2>How to connect</h2>

To connect with a serial cable, use <tt>palm/synce-serial</tt> and
follow the usual SynCE setup instructions.<br>
USB support is also available. To connect with USB, install
<tt>palm/uppc-kmod</tt> and run 'uppcsetup'. Further information and
tips for troubleshooting are available in the <tt>uppc(4)</tt>
manpage.

<p>Return to <a href="index.php">main page</a>.</p>

</div>
<?php include 'footer.php'; ?>
