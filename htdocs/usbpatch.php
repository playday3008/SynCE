<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to the <a href="usb.php">USB page</a>.</p>

<h1>SynCE - Patching the USB driver</h1>

<h2>Introduction</h2>

<p>Linux kernels 2.4.18 and later has kernel support for USB connection to
Windows CE and Pocket PC devices.</p>

<p>However, if your device don't work with the kernel driver ipaq.o out of the
box, you need to make some small modifications to this driver.</p>

<p>The kernel module currently only support the devices listed in this
table:</p>

<table cellpadding=3>
<tr><th>Vendor</th><th>Product</th><th>Vendor ID</th><th>Product IDs</th></tr>
<tr><td>Compaq</td><td>iPAQ (any model?)</td><td>049f</td><td>0003</td></tr>
<tr><td>HP</td><td>Jornada 548/568, iPAQ 1910, and probably others</td><td>03f0</td><td>1016<br>1116</td><td>Need to patch this on kernels before 2.4.20!</td></tr>
<tr><td>Casio</td><td>EM500 and probably others</td><td>07cf</td><td>2002</td></tr>
</table>

<p>The driver source code is located in these files in the kernel source tree:</p>
<p><code>drivers/usb/serial/ipaq.h</code><br>
<code>drivers/usb/serial/ipaq.c</code></p>

<p>There are three types of modifications that may need to be done, and then
you have to compile the kernel:</p>

<ol>
<li><a href="#hp">Correct the HP vendor ID</a></li>
<li><a href="#ids">Add vendor and product IDs</a></li>
<li><a href="#smp">Make the driver SMP safe</a></li>
<li><a href="#compile">Compile the kernel</a></li>
</ol>

<p>In case these instructions are not enough to get your driver working, we
would like you to first read <a
href="http://www.catb.org/~esr/faqs/smart-questions.html#intro">this
document</a> and search for the answer to your question in the <a
href="http://sourceforge.net/mail/?group_id=30550">mailing list archives</a>.
If your question remains unanswered, send mail to <a
href="mailto:synce-users@lists.sourceforge.net">synce-users@lists.sourceforge.net</a>.</p>

<a name="hp"></a>
<h2>Correct the HP vendor ID</h2>

<p>Some versions of the kernel module has the wrong vendor ID for HP devices.
This section describes how to correct this problem.</p>

<p><b>This is fixed in kernel version 2.4.20 and later!.</b></p>

<ol>

<li>Find this part of <code>ipaq.h</code>:
<pre>#define HP_VENDOR_ID		0x003f
#define HP_JORNADA_548_ID	0x1016
#define HP_JORNADA_568_ID	0x1116</pre>
</li>

<li>Modify the part marked with red like this:
<pre>#define HP_VENDOR_ID		<span class=RED>0x03f0</span>
#define HP_JORNADA_548_ID	0x1016
#define HP_JORNADA_568_ID	0x1116</pre>
</li>


</ol>

<a name="ids"></a>
<h2>Add vendor and product IDs</h2>

<p>To support other devices the module need to be modified as described below.</p>

<ol>

<li class=SPACED>Find the <a href="usbids.php">vendor and product IDs</a> for your device.</li>

<li>Find this part of <code>ipaq.h</code>:</li>

<pre>#define COMPAQ_VENDOR_ID	0x049f
#define COMPAQ_IPAQ_ID		0x0003</pre>

<li>Add the IDs for your device to  like in this example:

<pre><span class=RED>#define MY_VENDOR_ID		0x0bf8
#define MY_PRODUCT_ID		0x1001</span>

#define COMPAQ_VENDOR_ID	0x049f
#define COMPAQ_IPAQ_ID		0x0003</pre>
</li>

<li>Find this part of <code>ipaq.c</code>:

<pre>static __devinitdata struct usb_device_id ipaq_id_table [] = {
	{ USB_DEVICE(COMPAQ_VENDOR_ID, COMPAQ_IPAQ_ID) },
	{ USB_DEVICE(HP_VENDOR_ID, HP_JORNADA_548_ID) },
	{ USB_DEVICE(HP_VENDOR_ID, HP_JORNADA_568_ID) },
	{ USB_DEVICE(CASIO_VENDOR_ID, CASIO_EM500_ID) },
	{ }					/* Terminating entry */
};</pre>

</li>

<li>Add a line with information for your device similar to this:

<pre>static __devinitdata struct usb_device_id ipaq_id_table [] = {
	<span class=RED>{ USB_DEVICE(MY_VENDOR_ID, MY_PRODUCT_ID) },</span>
	{ USB_DEVICE(COMPAQ_VENDOR_ID, COMPAQ_IPAQ_ID) },
	{ USB_DEVICE(HP_VENDOR_ID, HP_JORNADA_548_ID) },
	{ USB_DEVICE(HP_VENDOR_ID, HP_JORNADA_568_ID) },
	{ USB_DEVICE(CASIO_VENDOR_ID, CASIO_EM500_ID) },
	{ }					/* Terminating entry */
};</pre>
</li>

</ol>

<a name="smp"></a>
<h2>Make the driver SMP safe</h2>

<p>If you are running on an SMP system, you need to modify <code>ipaq.c</code>
to be able to use the driver.</p>

<ol>

<li>Find this piece of code in ipaq.c:

<pre>static int __init ipaq_init(void)
{
	usb_serial_register(&amp;ipaq_device);
	info(DRIVER_DESC " " DRIVER_VERSION);

	return 0;
}</pre>

</li>

<li>Add the line marked with red below so that the code looks like this:

<pre>static int __init ipaq_init(void)
{
	<span class=RED>spin_lock_init(&amp;write_list_lock);</span>
	usb_serial_register(&amp;ipaq_device);
	info(DRIVER_DESC " " DRIVER_VERSION);

	return 0;
}</pre>
</li>
</ol>

<a name="compile"></a>
<h2>Compile the kernel</h2>

<li>Compile your kernel. If you haven't done this before, you can't wait
forever! :-) If you don't want to overwrite all of your existing kernel, you
can just replace the <code>ipaq.o</code> file, which is located in a directory
like this if your kernel version is 2.4.18-18.8.0:
<pre>/lib/modules/2.4.18-18.8.0/kernel/drivers/usb/serial/</pre> </li>


</ol>



<p>Return to the <a href="usb.php">USB page</a>.</p>

</div>
<?php include 'footer.php'; ?>
