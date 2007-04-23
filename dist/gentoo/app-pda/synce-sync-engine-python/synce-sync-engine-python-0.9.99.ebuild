# Copyright 1999-2005 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: $

inherit eutils subversion


DESCRIPTION="Common Library for Synce (connecting WinCE devices to Linux)"
HOMEPAGE="http://sourceforge.net/projects/synce/"
LICENSE="MIT"

SLOT="0"
KEYWORDS="~x86 ~amd64"
#~x86
IUSE=""
#http://www.synce.org/index.php/Syncing_using_the_OpenSync_Plugin
#https://svn.sourceforge.net/svnroot/synce/trunk/pyrapi2
#https://svn.sourceforge.net/svnroot/synce/trunk/oleavr-files/patches
#bzcat ../patches/wbxml2-0.9.2-anonymous-support-and-misc-fixes.patch.bz2 | patch -p1
#https://svn.sourceforge.net/svnroot/synce/trunk/oleavr-files/pywbxml
DEPEND=">=dev-libs/check-0.8.2
	=app-pda/synce-librapi2-0.9.99
	dev-python/twisted-web2
	dev-python/pygobject
	dev-python/pyrex
	!app-pda/libopensync-plugin-synce
	!app-pda/synce-multisync_plugin
	=app-pda/synce-pyrapi2-0.9.99
	=app-pda/synce-pywbxml-0.9.99
	=app-pda/synce-wbxml2-0.9.99
	dev-python/dbus-python
	app-pda/libopensync"
#	>=app-pda/multisync-0.9*"

ESVN_REPO_URI="https://svn.sourceforge.net/svnroot/synce/trunk/oleavr-files/sync-engine-python"
ESVN_FETCH_CMD="svn checkout"
ESVN_UPDATE_CMD="svn up"

S="${WORKDIR}/trunk"
src_unpack() {
	subversion_src_unpack
	cd ${S}
}

src_compile() {
	touch COPYING
	touch INSTALL
	./autogen.sh
	econf \
		--with-multisync-include=/usr/include/multisync  \
		--with-rra-include=/usr/include \
		|| die "econf failed"
	emake || die
}

src_install() {
	make DESTDIR="${D%/}" install || die
}
