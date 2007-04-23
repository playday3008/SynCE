# Copyright 1999-2006 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: $

inherit eutils autotools subversion

DESCRIPTION="Synchronize Windows CE devices with computers running GNU/Linux, like MS ActiveSync."
HOMEPAGE="http://sourceforge.net/projects/synce/"

LICENSE="GPL-2"
SLOT="0"
KEYWORDS="~amd64 ~ppc ~x86"
IUSE=""

DEPEND=">=dev-libs/check-0.8.2
	>=dev-libs/libmimedir-0.4
	=app-pda/synce-libsynce-0.9.99
	=app-pda/synce-librapi2-0.9.99"


ESVN_REPO_URI="https://svn.sourceforge.net/svnroot/synce/trunk/rra"
ESVN_FETCH_CMD="svn checkout"
ESVN_UPDATE_CMD="svn up"

S="${WORKDIR}/trunk"
src_unpack() {
	subversion_src_unpack
	cd ${S}/lib
	#dirty workaround...
	ln -s . rra
}

src_compile() {
	cd ${S}
	./bootstrap
	econf \
		|| die "econf failed"
	CFLAGS="$CFLAGS -Ilib" emake || die
}

src_install() {
	make DESTDIR="${D}" install || die
}
