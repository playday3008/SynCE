# Copyright 1999-2005 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: $

inherit python eutils subversion


DESCRIPTION="Common Library for Synce (connecting WinCE devices to Linux)"
HOMEPAGE="http://sourceforge.net/projects/synce/"
LICENSE="MIT"

SLOT="0"
KEYWORDS="~x86 ~amd64"

IUSE=""

DEPEND=">=dev-libs/check-0.8.2
	>=dev-libs/libwbxml-0.9.2_p48"

ESVN_REPO_URI="https://svn.sourceforge.net/svnroot/synce/trunk/oleavr-files/pywbxml"
ESVN_FETCH_CMD="svn checkout"
ESVN_UPDATE_CMD="svn up"

S="${WORKDIR}/trunk"
src_unpack() {
	subversion_src_unpack
	cd ${S}
}

src_compile() {
	export PYTHONPATH=$PYTHONPATH:${ROOT}usr/$(get_libdir)/python${PYVER}/site-packages
	./bootstrap
	econf || die "econf failed"
	emake || die
}

src_install() {
	make DESTDIR="${D%/}" install || die
}
