<?php include 'header.php'; ?>

<p>Return to <a href="../../index.php">SynCE-KDE main page</a>.</p>
<h1>SynCE - CE_DirView<br>
</h1>

<h2><a name="Contents" id="Contents"></a>Contents</h2>
<ul>
  <li><a href="#Introduction">Introduction</a></li>
  <li><a href="#Installation">Installation and Usage</a></li>
  <li><a href="#Screenshots">Screenshots</a></li>
  <li><a href="#Authors">Behind the sceenes</a></li>
</ul>

<h2><a name="Introduction" id="Introduction"></a>Introduction</h2>
Based on <b>PyRAPI</b> there is <b>CE_DirView.py</b> a Python-script to allow access
to a PocketPC with <a href="http://www.riverbankcomputing.co.uk/pyqt/download.php" target="_blank">PyQt</a> but not requiring KDE.<br>

The next step was to write a client-server solution to allow access
to a PocketPC via a remote computer.<br>

These add-ons are comming soon as part of the the <b>PyRAPI</b> package.<br>
You will find them in the <b>pyrapi/contrib</b> directory. <br>
<br>

<b>qt</b><br>

<blockquote>
Contains Python-scripts making usage of the <b>PyRAPI</b>-library.<br>
One of the scripts is <b>CE_DirView.py</b>.<br>
To allow client-access to this computer <b>CE_DirViewServer.py</b>
is the server-script.
</blockquote>
<br>

<b>qt_fake</b><br>

<blockquote>
You can use any other computer on your network to connect to your PocketPC.
This computer does not need SynCE installed. It requires
<a href="http://www.riverbankcomputing.co.uk/pyqt/download.php" target="_blank">PyQt</a>!
Once you have <a href="http://www.riverbankcomputing.co.uk/pyqt/download.php" target="_blank">PyQt</a>
you can use the same <b>CE_DirView.py</b>-script as one a computer with SynCE installed.<br>

The only difference is, that this package includes a <b>PyRAPI-fake</b>.
The <b>PyRAPI-fake</b> communicates with the <b>CE_DirViewServer.py</b>
(see above).<br>

So it's possible to use a <b>MacOS X</b>-Computer without SynCE or a <b>Sharp Zaurus</b> PDA
to view and manipulate files on a PocketPC.<br>
</blockquote>
<br>

<h2><a name="Installation" id="Installation"></a>Installation and Usage</h2>
<ol>
    <li>
       If you have SynCE and PyRAPI installed (direct connection):<br>
<br>
       $ CE_DirView.py<br>
    </li>
<br>
<br>
    <li>
       If you don't have SynCE installed, you must have "qt_fake" installed on the client.<br>
       You need a server with SynCE and PyRAPI (real-, not fake-version!) installed.<br>
    </li>
<br>
    <ol>
        <li>
          On the Server:<br>
<br>
          $ CE_DirViewServer.py -h host<br>
<br>
        </li>
         <li>
          On the client:<br>
<br>
        </li>
        <ol>
            <li>
             Define the server. This has to be done only once!<br>
<br>
             $ python pyrapi/pyrapi.py -h host [-p port]<br>
<br>
             Where host is the name or IP of the computer defined at "1.".<br>
<br>
            </li>
             <li>
             Start the client. E.g.:<br>
<br>
             $ CE_DirView.py<br>
            </li>
        </ol>
    </ol>
<br>
Use "pydoc CE_DirView" or "pydoc CE_DirViewServer.py" to get more info.<br>
</ol>
<br>

<h2><a name="Screenshots" id="Screenshots"></a>Screenshots</h2>
<table align="center" width="100%" border="0" cellspacing="2"
 cellpadding="2">
  <tbody>
    <tr><td>
        <h3>Linux</h3>
        </td></tr>
    <tr>
      <td align="center" valign="top"><img height="645" width="556" alt="Linux screen"
      title="Linux" src="images/linux_screen_00.jpg"><br>
      </td></tr>
    <tr>
      <td align="center" valign="top">On Linux you can <br>
drag & drop files<br>
to / from <b>konqueror</b>.<br>
      </td>
    </tr>

    <tr><td>
        <h3>MacOS X</h3>
        </td></tr>
    <tr>
      <td align="center" valign="top"><img height="455" width="570" alt="MacOS X screen"
      title="MacOS X" src="images/darwin_screen_00.jpg"><br>
      </td></tr>
    <tr>
      <td align="center" valign="top">On MacOS X you can<br>
drag & drop files<br>
to / from <b>Finder</b>.<br>
      </td>
    </tr>

    <tr><td>
        <h3>Sharp Zaurus</h3>
        </td></tr>
    <tr>
      <td align="center" valign="top"><img height="320" width="240" alt="Zaurus screen"
      title="Zaurus" src="images/zaurus_screen_01.jpg"><br>
      </td> </tr>
    <tr>
      <td align="center" valign="top">On Qtopia (e.g. Sharp Zaurus)<br>
use <b>tap and hold</b> to get<br>
the context-menu to move files.<br>
    </tr>
  </tbody>
</table>

<br>

<h2><a name="Authors" id="Authors"></a>Behind the sceenes</h2>
<a href="mailto:hippy@users.sourceforge.net">Richard Taylor</a> has written a Python
interface <b>PyRAPI</b> to the rapi-library opening SynCE to Python developers.<br>

<a href="mailto:mbier@users.sourceforge.net">Markus Biermaier</a> has written the Python- and PyQt-scripts.<br>

Many thanks to <a href="http://www.riverbankcomputing.co.uk/pyqt/download.php" target="_blank">Phil Tompson</a>
for writing the Python wrappers to the <a href="http://www.trolltech.com">Qt-Library</a>.<br>

Thanks also to <a href="mailto:twogood@users.sourceforge.net">David Eriksson</a>, the current project manager of the <b>SynCE</b>-project.

<p>Return to <a href="../../index.php">SynCE-KDE main page</a>.</p>

<?php include '../../footer.php'; ?>
