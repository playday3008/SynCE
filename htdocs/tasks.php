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


<li><a href="#vevent_vtodo">vEvent and vTodo support in RRA</a></li>

</ol>


<a name="installshield"></a><!-- {{{ -->
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
<li>Compile with gcc 3.2 on Linux/BSD using flags <tt>-Wall -Werror -ansi -pedantic</tt></li>
<li>Be licensed with an MIT license</li>
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

<p>Required: C or C++ programming knowledge on Linux/BSD.<br>
Useful: Ditto on Microsoft Windows.</p>

<h3>Links</h3>

<p><a href="http://www.google.com/search?q=i5comp+i6comp">Search for i5comp
and i6comp on Google.</a></p>
<!-- }}} -->
  
<a name="pwi"></a><!-- {{{ -->
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
<li>Compile with gcc 3.2 on Linux/BSD using flags <tt>-Wall -Werror -ansi -pedantic</tt></li>
<li>Be licensed with an MIT license</li>
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

<h3>Prerequisites</h3>

<p>Required: C or C++ programming knowledge on Linux/BSD.<br>
Recommended: A Pocket PC device.</p>

<h3>Links</h3>

<p><a
href="http://xml.openoffice.org/source/browse/xml/xmerge/java/org/openoffice/xmerge/converter/xml/sxw/pocketword/">OpenOffice
source code for PWI file conversion</a>.</p>
<!-- }}} -->

<a name="vevent_vtodo"></a><!-- {{{ -->
<h2>vEvent and vTodo support in RRA</h2>

<h3>Background</h3>

<p>The RRA module in SynCE is capable of converting between Contact entries
on Pocket PC and vCard files. However, there is no support for conversion
between Appointment/Task entries and iCalendar files.</p>

<h3>Task</h3>

<p>All source code should:</p>

<ol>
<li>Be written in C</li>
<li>Compile with gcc 3.2 on Linux/BSD using flags <tt>-Wall -Werror -ansi</tt></li>
<li>Be licensed with an MIT license</li>
<li>Support both <a href="http://www.imc.org/pdi/pdiproddev.html">vCalendar
1.0</a> and <a href="http://ftp.rfc-editor.org/in-notes/rfc2445.txt">RFC
2445 (iCalendar)</a>.</li>

</ol>

<p>This task is divided into several parts:</p>

<ol>

<li><p>Add the following functions to librra, without support for recurring
appointments:</p>

<pre>bool rra_appointment_to_vevent(
    uint32_t id,
    const uint8_t* data,
    size_t data_size,
    char** vevent,
    uint32_t flags);

bool rra_appointment_from_vevent(
    const char* vevent,
    uint32_t* id,
    uint8_t** data,
    size_t* data_size,
    uint32_t flags);
</pre>

<p>The functions should convert at least these vEvent attributes:</p>

<p><tt>CATEGORIES
CLASS 
DTSTAMP
DTSTART 
DTEND
LOCATION
SUMMARY
TRANSP
</tt></p>

<p>These functions should also convert at least the following vAlarm
attributes:</p>

<p><tt>
ACTION
DESCRIPTION
SUMMARY
TRIGGER
</tt></p>

<p>If more than one vAlarm section is present in the vEvent, the first one
should be used in the Appointment.</p> 

</li>

<li><p>Add the following functions to librra:</p>

<pre>bool rra_task_to_vtodo(
    uint32_t id,
    const uint8_t* data,
    size_t data_size,
    char** vtodo,
    uint32_t flags);

bool rra_task_from_vtodo(
    const char* vtodo,
    uint32_t* id,
    uint8_t** data,
    size_t* data_size,
    uint32_t flags);
</pre>

<p>The functions should convert at least these vTodo attributes:</p>

<p><tt>
CLASS
DESCRIPTION
DTSTAMP
DTSTART
DUE
PERCENT-COMPLETE
PRIORITY
SUMMARY
TRANSP
</tt></p>

</li>

<li>Add support for recurring appointments to the functions in (1). This
requires understanding of undocumented binary property values in the 
Appointment record.</li>

</ol>


<h3>Notes</h3>

<p>See the <tt>rra_contact_to_vcard</tt> and
<tt>rra_contact_from_vcard</tt> functions in rra/lib/contact.c for
implementation ideas.</p>

<p>Maybe this code could use <a
href="http://softwarestudio.org/libical/">libical</a>, but  <a
href="http://sourceforge.net/projects/libmimedir/">Lev Walkin's
libmimedir</a> is better from a license point of view.</p>

<h3>Prerequisites</h3>

<p>Required: C programming knowledge on Linux/BSD.<br>
Recommended: A Pocket PC device.</p>

<h3>Links</h3>

<p><a href="http://www.imc.org/pdi/pdiproddev.html">vCalendar 1.0</a></p>

<p><a href="http://ftp.rfc-editor.org/in-notes/rfc2445.txt">RFC
2445 (iCalendar)</a></p>

<!-- }}} -->


<p><br>Return to <a href="index.php">main page</a>.</p>

</div>
<?php include 'footer.php'; ?>
