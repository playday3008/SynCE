# Copyright 1999-2006 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: $

inherit eutils linux-info linux-mod subversion

DESCRIPTION="rndis protocol driver"
HOMEPAGE="http://www.synce.org"
LICENSE="GPL"
KEYWORDS="~amd64 -x86"

IUSE=""

RDEPEND=""
DEPEND=">=virtual/linux-sources-2.6
	!sys-fs/usb-rndis
	!sys-fs/usb-rndis-lite
	${RDEPEND}"

ESVN_REPO_URI="https://svn.sourceforge.net/svnroot/synce/trunk/usb-rndis-ng"
ESVN_FETCH_CMD="svn checkout"
ESVN_UPDATE_CMD="svn up"

BUILD_TARGETS="default"

S=${WORKDIR}/${P}
MODULE_NAMES="rndis_host(usb/rndis:${S}) usbnet(usb/rndis:${S}) cdc_ether(usb/rndis:${S})"

pkg_setup() {
	if kernel_is 2 4; then
		die "${P} does not support building against kernel 2.4.x"
	else
		einfo "kernel version ok"
	fi

	linux_chkconfig_present USB_ETH || die "kernel CONFIG_USB_ETH is needed"
#	linux_chkconfig_present USB_NET_CDCETHER || die "kernel CONFIG_USB_NET_CDCETHER is needed"
	linux_chkconfig_present USB_ETH_RNDIS || die "kernel CONFIG_USB_ETH_RNDIS is needed"
	linux-mod_pkg_setup
}

src_unpack() {
	subversion_src_unpack
	cd ${S}

	if kernel_is gt 2 6 19; then
#		epatch ${FILESDIR}/kernel-2.6.20.patch
		patch -p0 < ${FILESDIR}/kernel-2.6.20.patch || die "patching failed"
	fi
}

src_compile() {
	./autogen.sh
	econf || die "configure failed"
#	linux-mod_src_compile
	emake || die "make failed"
}

src_install() {
	cd ${S}
#	./clean.sh
	./install-sh
#	linux-mod_src_install
}



