<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="tasks.php">SynCE Tasks</a> page.</p>

 
<h1>Backup/restore support</h1>

<h2>Background</h2>

<p>Microsoft ActiveSync has a function to backup and restore the contents of a
PDA. This is not yet supported in SynCE.</p>

<h2>Task</h2>

<p>This task is divided in several parts:</p>

<ol>

<li><p>Document the secret CeRestoreDatabase and CeBackupDatabase RAPI functions
(RAPI calls 0x46 and 0x47).</p></li>

<li><p>Add the CeRestoreDatabase and CeBackupDatabase functions to
librapi2.</p></li>

<li class=spaced><p>Make a tool that should:</p>

<ol>
<li>Be written in C or C++</li>
<li>Compile with gcc 3.2 on Linux/BSD using flags <tt>-Wall -Werror -ansi -pedantic</tt></li>
<li>Be licensed with the <a href="http://opensource.org/licenses/mit-license.php">MIT license</a></li>
<li>Use CeBackupDatabase and CeRestoreDatabase to backup and restore databases.</li>
</ol>
</li>

<li><p>Document the backup procedure (RAPI calls) used by the backup/restore
function in Microsoft ActiveSync.</p></li>

<li><p>Implement missing RAPI calls needed for the backup procedure. One might guess that
one or more of RegCopyFile, RegRestoreFile and CeBackupFile may be needed, for example.</p></li>

<li><p>Document the file format used by the backup/restore function in
Microsoft ActiveSync.</p></li>

<li class=spaced><p>Make a tool that should:</p>

<ol>
<li>Be written in C or C++</li>
<li>Compile with gcc 3.2 on Linux/BSD using flags <tt>-Wall -Werror -ansi -pedantic</tt></li>
<li>Be licensed with the <a href="http://opensource.org/licenses/mit-license.php">MIT license</a></li>
<li>Be able to backup and restore everything on the PDA with the same file format as ActiveSync.</li>
</ol>
</li>


</ol>

<h2>Prerequisites</h2>

<p>Required: C or C++ programming knowledge on Linux/BSD.<br>
Recommended: A Pocket PC device.</p>

<p><br>Return to <a href="tasks.php">SynCE Tasks</a> page.</p>

</div>
<?php include 'footer.php'; ?>
