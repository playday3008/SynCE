<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="index.php">main page</a>.</p>

<h1>SynCE - Find the USB Vendor and Product IDs</h1>

<p>The USB Vendor and Product IDs can be found in different ways. You first
need to connect your PDA to your PC. Please note that this page is currently
quite Linux-specific.</p>

<h2>Read your logs</h2>

<p>When you connect your PDA you should get a message similar to this in your
logs:</p>

<pre>kernel: hub.c: USB new device connect on bus1/1, assigned device number 2
kernel: usb.c: USB device 2 (vend/prod 0x<span class=RED>49f</span>/0x<span class=RED>3</span>) is not claimed by any active driver.</pre>

<h2>Use the usbview tool</h2>

<p>The <tt>usbview</tt> tool should give output similar to this when your PDA
is connected. The IDs are marked with red color:</p>

<pre>serial
Speed: 12Mb/s (full)
USB Version:  1.01
Device Class: ff(vend.)
Device Subclass: ff
Device Protocol: ff
Maximum Default Endpoint Size: 16
Number of Configurations: 1
Vendor Id: <span class=RED>0bf8</span>
Product Id: <span class=RED>1001</span>
Revision Number:  0.00

Config Number: 1
        Number of Interfaces: 1
        Attributes: c0
        MaxPower Needed:   2mA

        Interface Number: 0
                Name: serial
                Alternate Number: 0
                Class: ff(vend.) 
                Sub Class: ff
                Protocol: ff
                Number of Endpoints: 2

                        Endpoint Address: 81
                        Direction: in
                        Attribute: 2
                        Type: Bulk
                        Max Packet Size: 64
                        Interval: 0ms

                        Endpoint Address: 02
                        Direction: out
                        Attribute: 2
                        Type: Bulk
                        Max Packet Size: 64
                        Interval: 0ms</pre>


<p>Return to <a href="index.php">main page</a>.</p>

</div>
<?php include 'footer.php'; ?>
