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
	dev-python/pyrex
	dev-python/dbus-python
	app-pda/libopensync
	dev-libs/libxml2
	dev-lang/swig
	dev-libs/expat
	!app-pda/libopensync-plugin-synce
	!app-pda/synce-multisync_plugin
	>=app-pda/synce-librtfcomp-0.9.99
	=app-pda/synce-libsynce-0.9.99
	=app-pda/synce-rra-0.9.99
	>=app-pda/synce-pywbxml-0.9.99"

#  libxml2-dev
# libexpat1-dev
# python2.4-dev

ESVN_REPO_URI="https://synce.svn.sourceforge.net/svnroot/synce/trunk/sync-engine"
ESVN_FETCH_CMD="svn checkout"
ESVN_UPDATE_CMD="svn up"

S="${WORKDIR}/trunk"
src_unpack() {
	subversion_src_unpack
	cd ${S}
}

src_compile() {
	einfo "nothing to compile"
#	cd ${S}/doc
	#emake
}

src_install() {
	mkdir -p ${D}/usr/shared/sync-engine
	cp -r ${S}/* ${D}/usr/shared/sync-engine
	#cp -r ${S}/tools ${S}/engine ${D}/usr/shared/sync-engine
	mkdir -p ${D}/usr/lib/opensync/python-plugins
	cd ${D}/usr/lib/opensync/python-plugins
	ln -sf ../../../shared/sync-engine/opensync-plugin.py synce.py
}
