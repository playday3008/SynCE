<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="index.php">main page</a>.</p>

<h1>SynCE - TODO</h1>

<p>Things that need to be improved in SynCE versions after 0.9.0:</p>

<ul>

<li><p>Usability</p></li>

<li><p>Partnership management</p>
<ul>
<li>Make it possible to manage parterships in the SynCE plugin for MultiSync</li>
</ul>
</li>

<li><p>Timezone handling</p>
<ul>

<li>Evolution should show appointment times in the current timezone and not in
UTC</li>

</ul>
</li>

<li><p>64-bit support</p>
<ul>
<li>CEPROPVAL handling in librapi2</li>
</ul>
</li>

<li><p>Handling of dates before year 1970 and after year 2037</p>
<ul>
<li>Use something better than <tt>time_t</tt> and <tt>struct tm</tt></li>
</ul>
</li>

</ul>

<p>See also the <a href="tasks.php">tasks</a>.</p>

<p>Return to <a href="index.php">main page</a>.</p>

</div>
<?php include 'footer.php'; ?>
