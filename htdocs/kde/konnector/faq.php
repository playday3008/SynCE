<?php include 'header.php'; ?>

<a href="./index.php">back to main page</a>
<br>
<h3>Frequently Asked Questions</h3>
<ul>
    <li>As I always get answers when I ask myself there is not much here for now.</li>
    <li><a href="#general">1. General</a>
    <ul>
        <li><a href="#look">1.1 Why does this site look similar to synce.sourceforge.net?</a></li>
        <li><a href="#configure">1.2 Why is everything installed in <code>/usr/local/kde3/</code> and not in 
            <code>/opt/kde3/</code>?</a></li>
    </ul>
    <li><a href="#install">2. Installation</a>
    <ul>
        <li></li>
    </ul>
    </li>
    <li><a href="#syncing">3. Synchronization</a>
    <ul>
        <li></li>
    </ul>
    </li>
</ul>
<br>
<br>
<br>
<h4><a name="general" id="general">1. General</a></h4>
<h4><a name="look" id="look">1.1 Why does this site look similar to synce.sourceforge.net?</a></h4>
As this software should become part of the synCE-libraries and maybe this site will be transfered
to <a href="http://synce.sourceforge.net">synce.sourceforge.net</a> it should look the same.
<h4><a name="configure" id="configure">1.2 Why is everything installed in <code>/usr/local/kde3/</code> and not in 
            <code>/opt/kde3/</code>?</a></h4>
To have everything installed in the correct directory-tree do the following:
<br>
<div style="margin-left:1cm"><code>./configure --prefix=/opt/kde3</code></div>        
<br>
<h4><a name="install" id="install">2. Installation</a></h4>
<br>
<h4><a name="syncing" id="syncing">3. Synchronization</a></h4>

<?php include 'footer.php'; ?>
