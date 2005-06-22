<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="index.php">main page</a>.</p>

<h1>SynCE - The tools</h1>

<p>Try these tools that send commands to the remote device. They must be
executed by the user that is running dccm.</p>

<p><i>Note for Debian users:</i> Prepend <tt>synce-</tt> to the tool names!</p>

<table cellpadding=2>
<tr><th>pcp</th><td>copy files</td></tr>
<tr><th>pls</th><td>list directory contents</td></tr>
<tr><th>pmkdir</th><td>make directory</td></tr>
<tr><th>pmv</th><td>move (rename) files</td></tr>
<tr><th>prm</th><td>remove file</td></tr>
<tr><th>prmdir</th><td>remove an empty directory</td></tr>
<tr><th>prun</th><td>run a program</td></tr>
<tr><th>pstatus</th><td>display status about remote device</td></tr>
<tr><th>synce-install-cab</th><td>install a .cab file</td></tr>
<!-- <tr><th>synce-remove-program</th><td>remove a program</td></tr> -->
</table>

<h2>Contributed tools</h2>

<p>Download and <tt>chmod a+x</tt></p>

<p><b>2005-06-15</b> Michel Acuna has contributed the <a
href="contrib/pdabk">pdabk</a> bash script to backup files from a Windows CE
device. View the script source for documentation.</p>

<p><b>2004-09-02</b> Adrian Dimulescu has written a  <a
href="http://adrian.dimulescu.free.fr/article.php3?id_article=10">PocketWord
converter</a>. It can convert .pwi/.psw documents into OpenOffice sxw, HTML,
and plain text.</p>

<p><b>2004-05-04</b> Henrik Isacsson has contributed the <a
href="contrib/pcp-r">pcp-r</a> script to recursively copy a directory to or
from a Windows CE device. Run the script without parameters to get help.</p>

<p>Return to <a href="index.php">main page</a>.</p>

</div>
<?php include 'footer.php'; ?>
