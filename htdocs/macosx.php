<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="index.php">main page</a>.</p>

<h1>SynCE - hints for Mac OS X</h1>

<ul>

<li class=SPACED>To compile SynCE, you first need the <a
href="http://developer.apple.com/tools/macosxtools.html">Mac OS X Developer
Tools</a>.</li>

<li class=SPACED>You need <b>GNU libiconv</b> and <b>libpoll</b> from <a
href="http://fink.sf.net/">Fink</a>.</li>

<li class=SPACED>Add "-no-cpp-precomp" to your CPPFLAGS before running configure:<br>
<br>
<tt>export CPPFLAGS="$CPPFLAGS -no-cpp-precomp"</tt></li>

<li class=SPACED>If you get the "multiple definitions of symbol" error message
when compiling librapi2 you should <a
href="http://fink.sourceforge.net/doc/porting/libtool.php">patch the
"convenience library bug"</a> in libtool.</li>

<li class=SPACED>Use the <a
href="http://www.loolix.com/hacks/LoolixUSBPocketPC/">LoolixUSBPocketPC</a>
driver to connect your Pocket PC device over USB to your Mac.</p></li>

</ul>

<p>Return to <a href="index.php">main page</a>.</p>

</div>
<?php include 'footer.php'; ?>
