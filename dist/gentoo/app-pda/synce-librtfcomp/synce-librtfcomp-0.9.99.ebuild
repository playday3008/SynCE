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

ESVN_REPO_URI="https://synce.svn.sourceforge.net/svnroot/synce/trunk/librtfcomp"
ESVN_FETCH_CMD="svn checkout"
ESVN_UPDATE_CMD="svn up"

S="${WORKDIR}/trunk"
src_unpack() {
	subversion_src_unpack
	cd ${S}
}

src_compile() {
	./bootstrap
	econf
	emake || die "make failed"
}

src_install() {
	make DESTDIR=${D} install || die
	mv ${D}/usr/bin/test  ${D}/usr/bin/testrtf
	dodoc README NEWS
}
