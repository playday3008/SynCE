<?php include 'header.php'; ?>

<p>Return to <a href="index.php">SynCE-KDE main page</a>.</p>
<h1>SynCE -
KDE Frequently Asked Questions</h1>
<h2>Contents</h2>
<ul id="mozToc">
<!--mozToc h2 1 h3 2 h4 3 h5 4 h6 5 h6 6--><li><a href="#mozTocId81386">Compilation
and
Installation</a>
    <ul>
      <li><a href="#mozTocId855728">Q11: I have
compiled SynCE-KDE myself. Even I have done a make
install I can't find the RAKI icon in my K-Menu hierarchy. I use
Linux distribution xy. </a></li>
    </ul>
  </li>
  <li><a href="#mozTocId363079">Startup</a>
    <ul>
      <li><a href="#mozTocId679954">Q21: I have RAKI up and running but
when I connect my PDA to the RS-232/USB/irDA port it didn't connect
automatically.</a></li>
      <li><a href="#mozTocId147393">Q22: I have RAKI up
and running but when I connect my PDA, RAKI didn't honor the connection
and the icon didn't turn colorful.</a></li>
    </ul>
  </li>
  <li><a href="#mozTocId293095">Handling</a>
    <ul>
      <li><a href="#mozTocId451238">Q31: When I initiate a connection
to my PDA RAKI brings up the initialization progress bar but blocks at
a progress of 33%. I have only the possibility to kill raki.</a></li>
    </ul>
  </li>
</ul>
<br>
<h2><a name="mozTocId81386" class="mozTocH2"></a>Compilation and
Installation</h2>
<h3><a name="mozTocId855728" class="mozTocH3"></a><b>Q11:</b> I have
compiled SynCE-KDE myself. Even I have done a <tt>make
install</tt> I can't find the RAKI icon in my K-Menu hierarchy. I use
Linux distribution xy. </h3>
<blockquote><b>A11:</b> You haven't configured your SynCE-KDE package
the right way. Every Linux distribution uses its own directory
structure and <tt>./configure </tt>can't find out the right one. So
you have to help a little.<br>
  <ol>
    <li>Find out the base directory of your KDE installation. An easy
way to find this out it to do a<br>
      <blockquote><tt># which konqueror<br>
$KDEDIR/bin/konqueror</tt><br>
      </blockquote>
The answer of this call is the full path to konqueror. Above you will
see which part of this path is the base KDE directory of your
distribution.<br>
    </li>
    <li>Fine out the directory where KDE expects the <tt>*.desktop</tt>
files. Every <tt>*.desktop</tt> file appears in the K-Menu as an menu
item. To find out this directory<br>
      <blockquote>search for the file <tt>konqbrowser.desktop</tt><br>
      </blockquote>
in your filesystem. Most likely it is in a subdirectory of <tt>$KDEDIR</tt>
namely <tt>$KDEDIR/share/applnk/Internet/konqbrowser.desktop</tt>.
The path to <tt>konqbrowser.desktop</tt> contains the relevant path
$kde_appsdir. It is the leading part of the path to <tt>konqbrowser.desktop</tt>
and follows the scheme<br>
      <blockquote><tt>$kde_appsdir/Internet/konqbrowser.desktop</tt></blockquote>
    </li>
    <li>Set the <tt>$KDE_DIR</tt> and <tt>kde_appsdir</tt>
environment variable and export them.<tt><br>
      </tt>
      <blockquote><tt># export KDEDIR=...<br>
# export kde_appsdir=...</tt><br>
      </blockquote>
    </li>
  </ol>
</blockquote>
<blockquote>Now you can configure your SynCE-KDE package by calling<br>
  <blockquote><tt># ./configure --prefix=$KDEDIR</tt><br>
  </blockquote>
After this do a <br>
  <blockquote><tt># make; make install </tt><br>
  </blockquote>
as usual. <br>
</blockquote>
<br>
<blockquote></blockquote>
<h2><a name="mozTocId363079" class="mozTocH2"></a>Startup</h2>
<h3><a name="mozTocId679954" class="mozTocH3"></a>Q21: I have RAKI up
and running but when I connect my PDA to the RS-232/USB/irDA port it
didn't connect automatically.</h3>
<blockquote><b>A21:</b> RAKI didn't initiate a connection to your PDA
by itself. You still have to run <tt>synce-serial-start</tt> as root
or you can use the <a
 href="http://synce.sourceforge.net/synce/hotplug.php">hotplug</a>
system if you connect via USB.<br>
</blockquote>
<h3><a name="mozTocId147393" class="mozTocH3"></a>Q22: I have RAKI up
and running but when I connect my PDA, RAKI didn't honor the connection
and the icon didn't turn colorful.</h3>
<blockquote><b>A22:</b> This problem could have a more than one reason:<br>
  <ol>
    <li>You have chosen a specific DCCM at the first start of RAKI but
you have started the other DCCM by hand before you start RAKI. <br>
      <b>Solution:</b> Kill the DCCM you have started by hand and let
RAKI start the right DCCM itself.</li>
    <li>You have chosen a specific DCCM at the first start of RAKI but
the connection management script <tt>~/.synce/scripts/dccm.sh</tt> is
the one designed for the other DCCM.<br>
Solution: Stop RAKI, open the file <tt>~/.kde/share/config/rakirc</tt>
with an
editor and remove the line <tt>VERSION=x.y.z</tt> in the block <tt>[RAKI]</tt>
and start
RAKI again. The splash screen appears again and you could again choose
a
DCCM. RAKI will copy the right <tt>dccm.sh</tt> script for you.</li>
  </ol>
</blockquote>
<br>
<h2><a name="mozTocId293095" class="mozTocH2"></a>Handling</h2>
<h3><a name="mozTocId451238" class="mozTocH3"></a>Q31: When I initiate
a connection to my PDA RAKI brings up the initialization progress bar
but blocks at a progress of 33%. I have only the possibility to kill
raki.</h3>
<blockquote><b>A31:</b> This is a bug in SynCE-KDE-0.6.1 and before. It
is fixed since SynCE-KDE-0.6.2.<br>
</blockquote>
<br>
<p>Return to <a href="index.php">SynCE-KDE main page</a>.</p>

<?php include 'footer.php'; ?>
