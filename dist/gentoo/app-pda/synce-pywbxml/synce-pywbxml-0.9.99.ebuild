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
	>=app-pda/synce-wbxml2-0.9.99"

ESVN_REPO_URI="https://svn.sourceforge.net/svnroot/synce/trunk/oleavr-files/pywbxml"
ESVN_FETCH_CMD="svn checkout"
ESVN_UPDATE_CMD="svn up"

S="${WORKDIR}/trunk"
src_unpack() {
	subversion_src_unpack
	cd ${S}
}

src_compile() {
	export PYTHONPATH=$PYTHONPATH:/usr/lib/python2.4/site-packages/
	export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig/
	./bootstrap
	econf || die "econf failed"
	emake || die
}

src_install() {
	make DESTDIR="${D%/}" install || die
}
