<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="tasks.php">SynCE Tasks</a> page.</p>

 
<h1>PocketWord Ink conversion</h1>

<h2>Background</h2>

<p>Pocket PC Notes and PocketWord documents are encoded in a binary format called
PocketWord Ink (PWI). This format is undocumented, but partly supported by
OpenOffice.</p>

<h2>Task</h2>

<p>This task is divided in several parts:</p>

<ol>

<li class=SPACED><p>Make a tool that should:</p>

<ol>
<li>Be written in C or C++</li>
<li>Compile with gcc 3.2 on Linux/BSD using flags <tt>-Wall -Werror -ansi -pedantic</tt></li>
<li>Be licensed with the <a href="http://opensource.org/licenses/mit-license.php">MIT license</a></li>
<li>Be able to convert a PWI file to a plain text file</li>
</ol>

<li class=SPACED><p>Update the tool in (1) to support conversion from a PWI
file to a <a href="http://www.w3.org/TR/xhtml1/">XHTML 1.0 Transitional</a> file,
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
href="http://www.w3.org/TR/xhtml1/">XHTML 1.0 Transitional</a> file to a PWI file,
supporting the same kinds of formatting as listed above. (Should probably
use <a href="http://expat.sourceforge.net/">expat</a> or <a
href="http://xmlsoft.org/">libxml2</a> to comply with the
license requirement.)</li>

<li class=SPACED>Update the tool in (1) to be able to extract ink drawings
from a PWI file and store them as a PNG file. The image should look just
like the document looks on the PDA. This is not documented and has not
been implemented in OpenOffice.</li>

<li>Update the tool in (1) to be able to extract sound clips from a PWI
file and store each sound clip as a separate WAV file. This is not
documented and has not been implemented in OpenOffice.</li>

</ol>

<h2>Prerequisites</h2>

<p>Required: C or C++ programming knowledge on Linux/BSD.<br>
Recommended: A Pocket PC device.</p>

<h2>Links</h2>

<p><a href="pwi/">Some info about the PWI file format</a>.</p>

<p><a
href="http://xml.openoffice.org/source/browse/xml/xmerge/java/org/openoffice/xmerge/converter/xml/sxw/pocketword/">OpenOffice
source code for PWI file conversion</a>.</p>

<p><br>Return to <a href="tasks.php">SynCE Tasks</a> page.</p>

</div>
<?php include 'footer.php'; ?>
