<?php include 'header.php'; ?>

<a href="./index.php">back to main page</a>
<br>
<h3><a name="Contents" id="Contents"></a>Contents</h3>
<ul>
    <li><a href="#KonnectorUsage">Usage of the konnector within kitchensync</a></li>
    <li><a href="#Configuring">Configuring</a></li>
    <li><a href="#MetaData">Meta data</a></li>
    <li><a href="#Debugging">Debugging</a></li>
    <li><a href="#Synchronization">Synchronization</a></li>    
</ul>
<br>
<h4><a name="KonnectorUsage" id="KonnectorUsage">Usage of the konnector within kitchensync</a></h4>
Once you have installed everything and the communication with the device works you can start
to sync with kitchensync. As you may see <code>kitchensync</code> blocks while reading/writing from/to
the device. This is because this is a non-threaded konnector. This is done this way to allow
better testing.
<br>
<strong>Attention:</strong>
<br>
Please think about making a complete backup of your stored data as there may be bugs which 
could lead to complete data loss! See the <a href="./usage.php#Backup">Backup and Restore</a> instructions.
<br>
There is a backup and restore mechanism within kitchensync, but I for myself had trouble with it.
And it does not handle AddressBookData.
<br>
<h5><a name="Configuring" id="Configuring">Configuring</a></h5>
<p>
First you need a working konnector to your pda. 
<br>
To select one do a <code>Settings -> Configure Kitchensync</code>. There select <code>konnector</code> and
click <code>Add</code>. In the now shown list you should see a <code>PocketPC Konnector</code> entry. 
If you can not find it there is some installation problem. Please check if you have really installed everything
where it belongs to.
</p>
<br>
If there is the <code>PocketPC Konnector</code> please select and configure it. The only configuration entry
you can specify till now is the pda-name.
<br>
Additionally please add and configure a <code>LocalKonnector</code>. The synchronization works with the files
specified within this konnector.
<br>
<strong>Hint:</strong>
<br>
On my installation <code>kitchensync</code> needs a restart after changing the settings!
<h5><a name="MetaData" id="MetaData">Meta data</a></h5>
The konnector stores relevant information to enable synchronization and to avoid unnecessary communication
with the pda. The data is store in <code>~/.kitchensync/meta/pocketpc</code>. 
<h5><a name="Debugging" id="Debugging">Debugging</a></h5>
There is a <em>lot</em> of debugging output on the command line. If you think the information there is not enough
please use <code>kdebugdialog</code> to enable debug output of kitchensync.
<br>
You can configure <code>kitchensync</code> to provide something like a test shell for the konnector. 
This is called <code>Konnector Debugger</code> and can be added with <code>Settings -> Configure Profile</code>.
There you can test connecting and disconnecting of the device and reading and writing of the complete datasets.
<h5><a name="Synchronization" id="Synchronization">Synchronization</a></h5>
If everything works fine you can start your first synchronization. 
<br>
What happens when you do the first synchronization? 
<br>
The complete data is loaded from the device and merged appended to the local data. It is done this way to
avoid deletion or overwriting of local and remote data. So it is necessary to check the data manually after
the first synchronization and delete duplicate data. You can do this locally in your favorite application or
remote on the device. With the next synchronization the removed entries can be terminally deleted.
<br>
If you want to get a clean system as in before first synchronization just delete the meta data in
<code>~/.kitchensync/meta</code>. Do not forget to also delete the meta data of the <code>LocalKonnector</code>.

<?php include 'footer.php'; ?>