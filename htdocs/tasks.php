<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="index.php">main page</a>.</p>

<h1>SynCE - Tasks</h1>

<p>This page contains specific programming assignments that would benefit the
SynCE project.</p>

<h2>Contents</h2>

<ul>
<li><a href="#installshield">Porting i5comp and i6comp to
Linux/BSD</a></li>
</ul>


<a name="installshield"></a>
<h2>Porting i5comp and i6comp to Linux/BSD</h2>

<h3>Background</h3>

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

<p>This task is divided in three parts:</p>

<ol>

<li class=SPACED>Make a single tool without decompression. It should:

<ol>
<li>Be written in C or C++</li>
<li>Compile with gcc 3.2 on Linux/BSD using flags <tt>-Wall -ansi -pedantic</tt></li>
<li>Be able to perform these functions on InstallShield archive files:<ol>

<li>List files</li>
<li>Extract compressed version of a file</li>

</ol>
<li>Work with both single- and multi-volume archive files</li>
</ol>


</li>
</ul>

<li class=SPACED>Add file decompression to the tool mentioned above.</li>

<li>Add all features present in i5comp and i6comp to the tool mentioned
above.</li>

</ol>

<h3>Notes</h3>

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

<h3>Prerequisites</h3>

<p>Required: C programming knowledge on Linux/BSD.<br>
Useful: Ditto on Microsoft Windows.</p>

<h3>Links</h3>

<p><a href="http://www.google.com/search?q=i5comp+i6comp">Search for i5comp
and i6comp on Google.</a></p>


<p>Return to <a href="index.php">main page</a>.</p>

</div>
<?php include 'footer.php'; ?>
