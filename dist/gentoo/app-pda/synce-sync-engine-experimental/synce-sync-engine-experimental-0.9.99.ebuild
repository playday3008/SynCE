# Copyright 1999-2005 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: $

inherit eutils subversion


DESCRIPTION="Common Library for Synce (connecting WinCE devices to Linux)"
HOMEPAGE="http://sourceforge.net/projects/synce/"
LICENSE="MIT"

SLOT="0"
KEYWORDS="~x86 ~amd64"
IUSE=""

DEPEND=">=dev-libs/check-0.8.2
	dev-python/pygobject
	dev-python/dbus-python
	>=app-pda/libopensync-0.21
	>=app-pda/libopensync-plugin-python-0.21
	dev-libs/libxml2
	dev-libs/libxslt
	!app-pda/libopensync-plugin-synce
	!app-pda/synce-multisync_plugin
	!app-pda/synce-sync-engine
	>=app-pda/synce-libsynce-0.9.99
	>=app-pda/synce-rra-0.9.99
	>=app-pda/synce-librtfcomp-0.9.99
	>=app-pda/synce-pywbxml-0.9.99"

ESVN_REPO_URI="https://synce.svn.sourceforge.net/svnroot/synce/branches/sync-engine-experimental"
ESVN_FETCH_CMD="svn checkout"
ESVN_UPDATE_CMD="svn up"

S="${WORKDIR}/trunk"
src_unpack() {
	subversion_src_unpack
	cd ${S}
}

src_compile() {
	einfo "nothing to compile"
}

src_install() {
	mkdir -p ${D}/usr/shared/sync-engine || die
	cp -r ${S}/* ${D}/usr/shared/sync-engine || die
	mkdir -p ${D}/usr/lib/opensync/python-plugins
	cp ${D}/usr/shared/sync-engine/opensync-plugin.py ${D}/usr/lib/opensync/python-plugins/synce.py || die
}
