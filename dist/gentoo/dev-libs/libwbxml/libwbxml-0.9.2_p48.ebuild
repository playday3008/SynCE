# Copyright 1999-2007 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: $

inherit eutils subversion

IUSE=""

MY_P="wbxml2-${PV}"

DESCRIPTION="Library and tools to parse, encode and handle WBXML documents."
HOMEPAGE="http://libwbxml.aymerick.com/"
ESVN_REPO_URI="http://libwbxml.aymerick.com:8080/repo/wbxml2/trunk"
ESVN_FETCH_CMD="svn checkout -r48"
ESVN_UPDATE_CMD="svn up"

LICENSE="GPL-2"
SLOT="0"
KEYWORDS="~amd64 ~ppc ~x86"

RDEPEND=">=dev-libs/expat-1.95.8"

DEPEND="${RDEPEND}
	>=sys-apps/sed-4.1.4"

S="${WORKDIR}/${MY_P}"

src_unpack()
{
	subversion_src_unpack

	# Get patches from SynCE SVN.  We need to temporarily change the
	# source directory so patches are downloaded to the correct place.
	OLD_S="${S}"
	S="${WORKDIR}/patches"
	ESVN_REPO_URI="https://synce.svn.sourceforge.net/svnroot/synce/trunk/oleavr-files/patches"
	ESVN_FETCH_CMD="svn checkout"
	subversion_src_unpack

	# Reset source directory
	S="${OLD_S}"
	cd ${S}

	# Apply the patches
	epatch ../patches/wbxml-svn-r48-anonymous.patch
	epatch ../patches/wbxml-svn-r48-build.patch
	epatch ../patches/wbxml-svn-r48-namespace.patch

	chmod 755 bootstrap
}

src_compile()
{
	./bootstrap

	econf || die "Configuration failed"
	emake || die "Compilation failed"
}

src_install()
{
	einstall || die "Installation failed"
	dodoc AUTHORS BUGS ChangeLog NEWS README References THANKS TODO
}
