<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="tasks.php">SynCE Tasks</a> page.</p>

<h1>Porting i5comp and i6comp to Linux/BSD</h1>

<h2>Background</h2>

<p>Many applications for Pocket PC are distributed with the <a
href="http://www.installshield.com/">InstallShield</a> installer for
Microsoft Windows, which has its own archive format for the files it will
install. In order to extract files from this archive, the tools i5comp (for
version 5 of InstallShield) and i6comp (for version 6 of InstallShield) can
be used.</p>

<p>However, these tools are also written for Microsoft Windows and need <a
href="http://www.winehq.com">WINE</a> to work on Linux/BSD. The source code
to these tools is available with the exception of the file decompression
part which is only available as a DLL or LIB.</p>

<h2>Task</h2>

<p>This task is divided in two parts:</p>

<ol>

<li class=SPACED><p>Make a single tool without decompression. It
should:</p>

<ol>
<li>Be written in C or C++</li>
<li>Compile with gcc 3.2 on Linux/BSD using flags <tt>-Wall -Werror -ansi -pedantic</tt></li>
<li>Be licensed with the <a href="http://opensource.org/licenses/mit-license.php">MIT license</a></li>
<li>Be able to perform these functions on InstallShield archive files:<ol>

<li>List files</li>
<li>Extract compressed version of a file</li>

</ol>
<li>Work with both single- and multi-volume archive files</li>
<li>Work with archives from InstallShield versions 5.x and 6.x</li>
<li>Have a command line syntax similar to tar, unzip or unrar</li>
</ol>


</li>

<li>Add file decompression to the tool mentioned above.</li>

</ol>

<li>Other features present in i5comp and i6comp could of course also be
added to the tool, but that is not part of the task.</li>


<h2>Notes</h2>

<p>The file decompression can be achieved in two ways, where the first one
is the preferred solution:</p>

<ol>

<li class=SPACED>Decompress using <a
href="http://www.gzip.org/zlib/">zlib</a> or another implementation of the
deflate algorithm (<a
href="http://www.gzip.org/zlib/feldspar.html">description</a>, <a
href="http://www.gzip.org/zlib/rfc-deflate.html">specification</a>). This
should be feasible as the InstallShield decompression DLLs contain this
string: <tt>deflate Copyright 1995 Jean-loup Gailly</tt></li>

<li>Load and use the DLL file similar to the way that some movie players for
Linux/BSD does for video codecs.</li>

</ol>

<h2>Prerequisites</h2>

<p>Required: C or C++ programming knowledge on Linux/BSD.<br>
Useful: Ditto on Microsoft Windows.</p>

<h2>Links</h2>

<p><a href="http://www.google.com/search?q=i5comp+i6comp">Search for i5comp
and i6comp on Google.</a></p>
  
<p><br>Return to <a href="tasks.php">SynCE Tasks</a> page.</p>

</div>
<?php include 'footer.php'; ?>
