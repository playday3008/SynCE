<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="tasks.php">SynCE Tasks</a> page.</p>

<h1>vEvent and vTodo support in RRA</h1>

<h2>Background</h2>

<p>The RRA module in SynCE is capable of converting between Contact entries
on Pocket PC and vCard files. However, there is no support for conversion
between Appointment/Task entries and iCalendar files.</p>

<h2>Task</h2>

<p>All source code should:</p>

<ol>
<li>Be written in C</li>
<li>Compile with gcc 3.2 on Linux/BSD using flags <tt>-Wall -Werror -ansi</tt></li>
<li>Be licensed with the <a href="http://opensource.org/licenses/mit-license.php">MIT license</a></li>
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


<h2>Notes</h2>

<p>See the <tt>rra_contact_to_vcard</tt> and
<tt>rra_contact_from_vcard</tt> functions in rra/lib/contact.c for
implementation ideas.</p>

<p>Maybe this code could use <a
href="http://softwarestudio.org/libical/">libical</a>, but  <a
href="http://sourceforge.net/projects/libmimedir/">Lev Walkin's
libmimedir</a> is better from a license point of view.</p>

<h2>Prerequisites</h2>

<p>Required: C programming knowledge on Linux/BSD.<br>
Recommended: A Pocket PC device.</p>

<h2>Links</h2>

<p><a href="http://www.imc.org/pdi/pdiproddev.html">vCalendar 1.0</a></p>

<p><a href="http://ftp.rfc-editor.org/in-notes/rfc2445.txt">RFC
2445 (iCalendar)</a></p>

<p><br>Return to <a href="tasks.php">SynCE Tasks</a> page.</p>

</div>
<?php include 'footer.php'; ?>
