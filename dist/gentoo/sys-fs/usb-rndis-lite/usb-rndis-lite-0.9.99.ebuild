# Copyright 1999-2006 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: $

inherit eutils linux-info linux-mod subversion

DESCRIPTION="rndis protocol driver"
HOMEPAGE="http://www.synce.org"
LICENSE="GPL"
KEYWORDS="~amd64 ~x86"

IUSE="debug vebose"

RDEPEND=""
DEPEND=">=virtual/linux-sources-2.6
	!sys-fs/usb-rndis
	${RDEPEND}"

ESVN_REPO_URI="https://svn.sourceforge.net/svnroot/synce/trunk/usb-rndis-lite"
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

#	check CONFIG_USB_ETH_RNDIS=y|m
#	check CONFIG_USB_NET_CDCETHER=y|m

	linux-mod_pkg_setup
}

src_unpack() {
	subversion_src_unpack
	cd ${S}
}

src_compile() {
#	if built_with_use debug; then
#		sed -i "s/\/\/ #define	DEBUG/#define DEBUG/g" *.c
#	fi
#	if built_with_use verbose; then
#		sed -i "s/\/\/ #define	VERBOSE/#define	VERBOSE/g" *.c
#	fi
	linux-mod_src_compile
}

src_install() {
	cd ${S}
#	./clean.sh
	linux-mod_src_install
}



