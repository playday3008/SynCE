<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="usb.php">USB page</a>.</p>

<h1>SynCE - USB connection - Linux</h1>

<p>Useful reading: How to <a href="usbids.php">find the USB Vendor and Product
IDs</a> for your PDA.</p>

<p>See <a href="#success">Successful USB connection</a> for a log message from
a successful USB connection.</p>

<p>Select your kernel version to see if your device works with your kernel:</p>

<ul>

<li><a href="#none">Kernel prior to 2.4.18</a></li>

<li><a href="#early">2.4.18 or 2.4.19 kernel</a></li>

<li><a href="#many">2.4.20 or newer 2.4.x kernel</a></li>

<li><a href="#notyet">2.5.x or 2.6.x kernel</a></li>

</ul>

<a name="none"></a>
<h2>Kernel prior to 2.4.18</h2>

<p>These kernels are not supported. Either upgrade or use the <a
href="usb_linux_userspace.php">user-space USB driver</a>.</p>

<a name="early"></a>
<h2>2.4.18 or 2.4.19 kernel</h2>

<p>If you going to use SynCE on an SMP system with this kernel you must <a
href="usbpatch.php">patch your kernel driver</a> or upgrade to <a
href="#many">2.4.20 or newer 2.4.x kernel</a>.</p>

<p>These kernel version only support these devices:</p>

<table cellpadding=3>
<tr><th>Vendor</th><th>Vendor ID</th><th>Product</th><th>Product ID</th></tr>
<tr><td>Casio</td><td>07cf</td><td>EM500 and probably others</td><td>2002</td></tr>
<tr><td>Compaq</td><td>049f</td><td>iPAQ (any model?)</td><td>0003</td></tr>
</table>

<p>If your device is in the list above, go ahead and <a
href="usb_linux_setup.php">configure SynCE</a>, otherwise you need to <a
href="usbpatch.php">patch your kernel driver</a> or upgrade to <a
href="#many">2.4.20 or newer 2.4.x kernel</a>.</p>

<a name="many"></a>
<h2>2.4.20 or later 2.4.x kernel</h2>

<p>This kernel supports many common and uncommon devices, but certain newer
devices are missing.</p>

<table cellpadding=3>
<tr><th>Vendor</th><th>Vendor ID</th><th>Product</th><th>Product ID</th></tr>

<tr><td>Askey</td><td>1690</td><td>Unknown</td><td>0601</td></tr>

<tr><td>Bcom</td><td>0960</td><td></td><td>0065<br>0066<br>0067</td></tr>

<tr><td>Casio</td><td>07cf</td><td>Unknown<br>EM500 etc</td><td>2001<br>2002</td></tr>

<tr><td>Compaq</td><td>049f</td><td>iPAQ<br>Unknown</td><td>0003<br>0032</td></tr>

<tr><td>Dell</td><td>413c</td><td>Axim</td><td>3001</td></tr>

<tr><td>FSC</td><td>0bf8</td><td>Loox</td><td>1001</td></tr>

<tr><td>HP</td><td>03f0</td><td>
Jornada 548<br>
Jornada 568<br>
Unknown
</td><td>
1016<br>
1116<br>
2016<br>
2116<br>
2216<br>
3016<br>
3116<br>
3216<br>
4016<br>
4116<br>
4216<br>
5016<br>
5116<br>
5216<br>
</td></tr>

<tr><td>Linkup</td><td>094b</td><td>Unknown</td><td>0001</td></tr>

<tr><td>Microsoft</td><td>045e</td><td>Unknown</td><td>ce00</td></tr>

<tr><td>Portatec</td><td>0961</td><td>Unknown</td><td>0010</td></tr>

<tr><td>Sagem</td><td>5e04</td><td>Unknown</td><td>ce00</td></tr>

<tr><td>Socket</td><td>0104</td><td>Unknown</td><td>00be</td></tr>

<tr><td>Toshiba</td><td>0930</td><td>
Unknown<br>
E740
</td><td>
0700<br>
0706
</td></tr>

<tr><td>HTC</td><td>0bb4</td><td>Unknown</td><td>00ce</td></tr>

<tr><td>NEC</td><td>0409</td><td>Unknown</td><td>00d5</td></tr>

<tr><td>Asus</td><td>0b05</td><td>A600</td><td>4201</td></tr>

</table>

<p>If your device is in the list above, go ahead and <a
href="usb_linux_setup.php">configure SynCE</a>, otherwise you need to <a
href="usbpatch.php">patch your kernel driver</a>.</p>

<p>If you can provide additional details above the devices listed above, please
<a href="help.php">contact the SynCE developers</a>.</p>

<a name="notyet"></a>
<h2>2.5.x or 2.6.x kernel</h2>

<p>The <tt>ipaq</tt> USB driver does not yet work on these kernels. We do not
know when this will be fixed. If you have kernel skills, please submit patches
to the driver.</p>

<a name="success"></a>
<h2>Successful USB connection</h2>

<p>When you connect your USB cable or insert your device into its cradle the
following message (or similar) should appear in your logs:</p>

<pre>kernel: hub.c: USB new device connect on bus1/1, assigned device number 2
kernel: usb.c: USB device 2 (vend/prod 0x49f/0x3) is not claimed by any active driver.
/etc/hotplug/usb.agent: Setup ipaq for USB product 49f/3/0
kernel: usb.c: registered new driver serial
kernel: usbserial.c: USB Serial support registered for Generic
kernel: usbserial.c: USB Serial Driver core v1.4
kernel: usbserial.c: USB Serial support registered for Compaq iPAQ
kernel: usbserial.c: Compaq iPAQ converter detected
kernel: usbserial.c: Compaq iPAQ converter now attached to ttyUSB0 (or usb/tts/0 for devfs)
kernel: ipaq.c: USB Compaq iPAQ, HP Jornada, Casio EM500 driver v0.2</pre>


</div>
<?php include 'footer.php'; ?>