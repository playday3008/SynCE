<?php include 'header.php'; ?>

<div class="PAGE">

<p>Return to <a href="index.php">main page</a>.</p>

<h1>SynCE - IP forwarding</h1>

<p>IP forwarding allows you to use TCP/IP connections from your PDA.</p>

<p>Note: These instructions are for Linux, contributions for other operating systems
are welcome!</p>

<p>Hint: If you use the SynCE <a
href="http://synce.sourceforge.net/synce/kde/">KDE integration</a> you don't
have to do this manually!</p>

<ol>

<li><p>If you are not running a DNS server on your computer, set the DNS server
using one of the following methods.</p>

<ul>

<li>Run <tt>synce-serial-config</tt> with the DNS server parameter set.</li>

<li>Edit the <tt>ms-dns</tt> option in <tt>/etc/ppp/peers/synce-device</tt> to
set the correct DNS server.</li>

<li>Use <a href="http://www.thekelleys.org.uk/dnsmasq/doc.html">dnsmasq</a>.</li>

</ul>

<p>Hint: your current DNS server(s) can be found in the
<tt>/etc/resolv.conf</tt> file.</p>

</li>


<li>Enable IP forwarding in your Linux kernel:

<pre>echo 1 &gt; /proc/sys/net/ipv4/ip_forward</pre>
</li>

<li><p>Configure iptables:</p>

<pre>iptables -t nat -A POSTROUTING -s 192.168.131.201 -j MASQUERADE</pre>

<p>The IP address 192.168.131.201 is used as an example. The correct IP adress
for your device can be found in your <tt>~/.synce/active_connection</tt> file
when you have connected it.</p>

</li>

<li>If you want to browse the web with Pocket Internet Explorer, see <a
href="http://web.archive.org/web/20040219101402/http://www.tekguru.co.uk/EM500/usbtonet.htm">these
instructions</a>.</li>

</ol>

<p>Return to <a href="index.php">main page</a>.</p>

</div>
<?php include 'footer.php'; ?>
