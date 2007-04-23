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

DEPEND=">=dev-libs/check-0.8.2
	>=app-pda/synce-libsynce-0.9.99
	>=app-pda/synce-pyrapi2-0.9.99"

SRC_URI="http://prdownloads.sourceforge.net/wbxmllib/wbxml2-0.9.2.tar.gz"
#SVN_REPO_URI="http://synce.svn.sourceforge.net/viewvc/synce/trunk/oleavr-files/patches/"
ESVN_FETCH_CMD="svn checkout"
ESVN_UPDATE_CMD="svn up"

S="${WORKDIR}/trunk"
src_unpack() {
	unpack ${A}

	S="${WORKDIR}/patches"
	ESVN_REPO_URI="https://synce.svn.sourceforge.net/svnroot/synce/trunk/oleavr-files/patches"
	subversion_src_unpack

	S="${WORKDIR}/trunk"
	ESVN_REPO_URI="http://libwbxml.aymerick.com:8080/repo/wbxml2/trunk"
	subversion_src_unpack
	cd ${S}

#	bzcat ../patches/wbxml2-0.9.2-anonymous-support-and-misc-fixes.patch.bz2 | patch -p1 || die
#	bunzip2 -c ../patches/wbxml2-0.9.2-anonymous-support-and-misc-fixes.patch.bz2 > wbxml2-0.9.2-anonymous-support-and-misc-fixes.patch
#	epatch wbxml2-0.9.2-anonymous-support-and-misc-fixes.patch
	epatch ../patches/wbxml-svn-r48-anonymous.patch
	epatch ../patches/wbxml-svn-r48-build.patch
	epatch ../patches/wbxml-svn-r48-datetime.patch
	epatch ../patches/wbxml-svn-r48-namespace.patch

}

src_compile() {
	cd ${S}

	export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig/

	chmod +x bootstrap
	./bootstrap
	econf \
		|| die "econf failed"
	emake || die
}

src_install() {
	make DESTDIR="${D%/}" install || die
}
