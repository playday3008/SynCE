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
		>=app-pda/synce-libsynce-0.9.99
		>=app-pda/synce-librapi2-0.9.99"

# useflag release, if set it will create release-archives in /opt/yacy-svn/RELEASE

ESVN_REPO_URI="https://svn.sourceforge.net/svnroot/synce/trunk/gnomevfs"
ESVN_FETCH_CMD="svn checkout"
ESVN_UPDATE_CMD="svn up"

#export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig/
#make LIBS=/usr/local/lib/libsynce.so

S="${WORKDIR}/trunk"
src_unpack() {
	subversion_src_unpack
	cd ${S}
}

src_compile() {
	export WANT_AUTOMAKE="1.9" # Force usage of automake 1.9 for safety
	./bootstrap
	econf
	#emake LIBS="/usr/lib/libsynce.so /usr/lib/librapi.so" || die "make failed"
	emake || die "make failed"
}

src_install() {
	make DESTDIR=${D} install || die
	dodoc README
}
