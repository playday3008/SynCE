# Copyright 1999-2006 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: $

DESCRIPTION="Synchronize Windows CE devices with Linux."
HOMEPAGE="http://sourceforge.net/projects/synce/"
SRC_URI=""

LICENSE="MIT"
SLOT="0"
KEYWORDS="~amd64 ~x86 ~ppc"
IUSE="gnome usb serial syncengine wininstallers"

#		!app-pda/synce

DEPEND="=app-pda/synce-libsynce-0.9.99
		>=app-pda/synce-librapi2-0.9.99
		>=app-pda/synce-odccm-0.9.99
		>=app-pda/synce-gnome-0.9.99
		usb? (sys-fs/usb-rndis-lite)
		syncengine?	(>=app-pda/synce-sync-engine-0.9.99)
		gnome? (>=app-pda/synce-gnomevfs-0.9.99
				>=app-pda/synce-software-manager-0.9.99
				>=app-pda/synce-trayicon-0.9.99
		)
		serial?(>=app-pda/synce-serial-0.9.1)
		wininstallers? (>=app-pda/dynamite-0.1
			>=app-pda/orange-0.3
			>=app-arch/unshield-0.5
		)"


src_compile() {
	return
}

