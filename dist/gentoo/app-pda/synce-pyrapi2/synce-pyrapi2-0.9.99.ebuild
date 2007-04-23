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
	dev-libs/expat
	dev-libs/libxml2
	dev-python/pyxml
	=app-pda/synce-librapi2-0.9.99"

#https://svn.sourceforge.net/svnroot/synce/branches/trayicon/TWOGOOD/trayicon

ESVN_REPO_URI="https://svn.sourceforge.net/svnroot/synce/trunk/pyrapi2"
ESVN_FETCH_CMD="svn checkout"
ESVN_UPDATE_CMD="svn up"

S="${WORKDIR}/trunk"
src_unpack() {
	subversion_src_unpack
	cd ${S}
}

src_compile() {
	#touch COPYING
	#touch INSTALL
	export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig/
	./bootstrap
	econf || die "econf failed"
	emake || die
}

src_install() {
	make DESTDIR="${D%/}" install || die
}
