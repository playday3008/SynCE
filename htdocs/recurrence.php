<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="index.php">main page</a>.</p>

<h1>SynCE - Recurrence BLOB</h1>

<p>The BLOB has ID 4015.</p>

<p>See <a
href="http://msdn.microsoft.com/library/en-us/wcepoom/html/cerefIRecurrencePatternPropertyMethods.asp">IRecurrencePattern</a>
for the object representing recurrence.</p>

<p>Dates in the BLOB are expressed in minutes from January 1, 1601 00:00 GMT.</p>

<h2>MonthNth</h2>

<table cellspacing=5>
<tr><th>Offset</th><th>Size</th><th>Contents</th><th>Description</th></tr>

<tr><td>0000</td><td>4</td><td>04 30 04 30</td><td>The same for all types of recurrence?</td></tr>
<tr><td>0004</td><td>1</td><td>0c</td><td>Always this value for MonthNth?</td></tr>
<tr><td>0005</td><td>1</td><td>20</td><td>The same for all types of recurrence?</td></tr>
<tr><td>0006</td><td>4</td><td>3</td><td>olRecursMonthNth</td></tr>
<tr><td>000a</td><td>4</td><td>00 00 00 00</td><td>Unknown</td></tr>
<tr><td>000e</td><td>4</td><td></td><td>Interval</td></tr>
<tr><td>0012</td><td>4</td><td>00 00 00 00</td><td>Unknown</td></tr>
<tr><td>0016</td><td>4</td><td>00 00 00 00</td><td>DaysOfWeekMask</td></tr>
<tr><td>001a</td><td>4</td><td></td><td>Instance</td></tr>
<tr><td>001e</td><td>4</td><td></td><td>Flags:</td></tr>
<tr><td>0022</td><td>4</td><td></td><td>Occurrences</td></tr>
<tr><td>0026</td><td>4</td><td>00 00 00 00</td><td>Unknown</td></tr>
<tr><td>002a</td><td>4</td><td><i>total</i></td><td>Total number of exceptions (deleted and modified appointments within the recurrence)</td></tr>

<tr><td>002e</td><td>4*<i>total</i>=<i>X</i></td><td></td><td>The date for each exception</td></tr>

<tr><td>002e<i>+X</i></td><td>4</td><td><i>modified</i></td><td>Number of modified appointments</td></tr>

<tr><td>0032<i>+X</i></td><td>4*<i>total</i>=<i>Y</i></td><td></td><td>The date for each modified appointment</td></tr>


<tr><td>0032<i>+X+Y</i></td><td>4</td><td></td><td>Recurrence start date</td></tr>
<tr><td>0036<i>+X+Y</i></td><td>c</td><td>df 80 e9 5a<br>05 30 00 00 <br>05 30 00 00</td><td>Unknown</td></tr>

<tr><td>0042<i>+X+Y</i></td><td>4</td><td></td><td>Start minute</td></tr>
<tr><td>0046<i>+X+Y</i></td><td>4</td><td></td><td>End minute</td></tr>

<tr><td>004a<i>+X+Y</i></td><td>2</td><td><i>modified</i></td><td>Same value as above</td></tr>

</table>

<p>After this follows one of these for each modified appointment:</p>

<table cellspacing=5>
<tr><th>Offset</th><th>Size</th><th>Contents</th><th>Description</th></tr>
<tr><td>0000</td><td>4</td><td></td><td>New start date and time</td></tr>
<tr><td>0004</td><td>4</td><td></td><td>New end date and time</td></tr>
<tr><td>0008</td><td>4</td><td></td><td>Original start date and time</td></tr>
<tr><td>000c</td><td>2</td><td>00 00</td><td>Unknown</td></tr>
</table>

<p>The BLOB is padded with 00 to a size of at least 0x68 bytes</p>



<h3>Flags</h3>

<table cellspacing=5>
<tr><th>Value</th><th>Description</th></tr>

<tr><td>2021</td><td>Ends on date</td></tr>
<tr><td>2022</td><td>Ends after a number of occurences</td></tr>
<tr><td>2023</td><td>Does not end</td></tr>

</table>


<p><br>Return to <a href="index.php">main page</a>.</p>

</div>
<?php include 'footer.php'; ?>
