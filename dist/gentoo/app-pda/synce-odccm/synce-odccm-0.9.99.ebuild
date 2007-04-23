# Copyright 1999-2005 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: $

inherit eutils subversion


DESCRIPTION="Common Library for Synce (connecting WinCE devices to Linux)"
HOMEPAGE="http://sourceforge.net/projects/synce/"
LICENSE="MIT"

SLOT="0"
KEYWORDS="~x86 ~amd64"
IUSE="dbus"
DEPEND=">=dev-libs/check-0.8.3.1
		>=net-libs/gnet-2.0.0
		!app-pda/synce-dccm
		!app-pda/synce-vdccm
		app-pda/synce-libsynce"

# useflag release, if set it will create release-archives in /opt/yacy-svn/RELEASE

ESVN_REPO_URI="https://synce.svn.sourceforge.net/svnroot/synce/trunk/odccm"
ESVN_FETCH_CMD="svn checkout"
ESVN_UPDATE_CMD="svn up"

#export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig/
#make LIBS=/usr/local/lib/libsynce.so

S="${WORKDIR}/trunk"
src_unpack() {
	subversion_src_unpack
	cd ${S}
}

src_compile() {
	autoreconf -i
	econf
	emake || die "make failed"
}

src_install() {
	make DESTDIR=${D} install || die

	mkdir -p ${D}/etc/dbus-1/system.d/
	cp data/dbus/odccm.conf ${D}/etc/dbus-1/system.d/

	mkdir -p ${D}/lib/rcscripts/net
	cp ${FILESDIR}/triggerconnection.sh ${D}/lib/rcscripts/net/

	mkdir -p ${D}/etc/init.d
	cp ${FILESDIR}/init.synce ${D}/etc/init.d/synce

	ewarn " do:"
	ewarn ""
	ewarn "cd /etc/init.d"
	ewarn "ln -s net.lo net.eth1"
	ewarn ""
	ewarn " add in /etc/conf.d/net:"
	ewarn ""
	ewarn "modules=( \"dhclient\" )"
	ewarn "config_eth1=( \"dhcp\" \"triggerconnection\" )"
	ewarn "modules_eth1=( \"dhclient\")"
	ewarn "modules_eth1=( \"triggerconnection\" )"
	ewarn "dhclient_eth1=\" \""

	dodoc README NEWS
}
