<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="index.php">main page</a>.</p>

<h1>SynCE - Tasks</h1>

<p>This page contains specific programming assignments that would benefit the
SynCE project.</p>

<h2>Contents</h2>

<ol>

<li><a href="#installshield">Porting i5comp and i6comp to
Linux/BSD</a></li>

<li><a href="#pwi">PocketWord Ink conversion</a></li>

</ol>


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

<p>This task is divided in two parts:</p>

<ol>

<li class=SPACED><p>Make a single tool without decompression. It
should:</p>

<ol>
<li>Be written in C or C++</li>
<li>Compile with gcc 3.2 on Linux/BSD using flags <tt>-Wall -ansi -pedantic</tt></li>
<li>Be licensed with a BSD or MIT license</li>
<li>Be able to perform these functions on InstallShield archive files:<ol>

<li>List files</li>
<li>Extract compressed version of a file</li>

</ol>
<li>Work with both single- and multi-volume archive files</li>
<li>Work with archives from InstallShield versions 5.x and 6.x</li>
</ol>


</li>

<li>Add file decompression to the tool mentioned above.</li>

</ol>

<li>Other features present in i5comp and i6comp could of course also be
added to the tool, but that is not part of the task.</li>


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

<a name="pwi"></a>
<h2>PocketWord Ink conversion</h2>

<h3>Background</h3>

<p>Pocket PC Notes and PocketWord documents are encoded in a binary format called
PocketWord Ink (PWI). This format is undocumented, but partly supported by
OpenOffice.</p>

<h2>Task</h2>

<p>This task is divided in several parts:</p>

<ol>

<li class=SPACED><p>Make a tool that should:</p>

<ol>
<li>Be written in C or C++</li>
<li>Compile with gcc 3.2 on Linux/BSD using flags <tt>-Wall -ansi -pedantic</tt></li>
<li>Be licensed with a BSD or MIT license</li>
<li>Be able to convert a PWI file to a plain text file</li>
</ol>

<li class=SPACED><p>Update the tool in (1) to support conversion from a PWI
file to a <a href="http://www.w3.org/TR/xhtml1/">XHTML 1.0</a> file,
supporting the following kinds of formatting:</p>

<ol>
<li>Font name</li>
<li>Font size</li>
<li>Color</li>
<li>Bold</li>
<li>Italic</li>
<li>Underline</li>
<li>Strikethrough</li>
<li>Highlight</li>
</ol>

</li>

<li class=SPACED>Update the tool in (2) to supprt conversion from a <a
href="http://www.w3.org/TR/xhtml1/">XHTML 1.0</a> file to a PWI file,
supporting the same kinds of formatting as listed above. (Should probably
use <a href="http://expat.sourceforge.net/">expat</a> or <a
href="http://xmlsoft.org/">libxml2</a> to comply with the
license requirement.)</li>

<li class=SPACED>Update the tool in (1) to be able to extract ink drawings
from a PWI file and store them as a PNG file. The image should look just
like the document looks on the PDA. (This is not documented and has not
been implemented before.)</li>

<li>Update the tool in (1) to be able to extract sound clips from a PWI
file and store each sound clip as a separate WAV file. (This is not
documented and has not been implemented before.)</li>

</ol>

<h3>Prerequisites</h3>

<p>Required: C or C++ programming knowledge on Linux/BSD.<br>
Recommended: A Pocket PC device.</p>

<h3>Links</h3>

<p><a
href="http://xml.openoffice.org/source/browse/xml/xmerge/java/org/openoffice/xmerge/converter/xml/sxw/pocketword/">OpenOffice
source code for PWI file conversion</a>.</p>




<p><br>Return to <a href="index.php">main page</a>.</p>

</div>
<?php include 'footer.php'; ?>
