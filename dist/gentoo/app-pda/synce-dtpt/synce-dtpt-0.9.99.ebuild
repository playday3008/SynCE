# Copyright 1999-2005 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: $

inherit eutils subversion


DESCRIPTION="SynCE DTPT daemon"
HOMEPAGE="http://sourceforge.net/projects/synce/"
LICENSE="MIT"

SLOT="0"
KEYWORDS="~x86 ~amd64"
IUSE=""

DEPEND="dev-lang/python"

ESVN_REPO_URI="https://synce.svn.sourceforge.net/svnroot/synce/trunk/device-manager/manager"
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
	mkdir -p ${D}/usr/share/dtpt || die
	cp -r ${S}/* ${D}/usr/share/dtpt || die
	chmod a+x ${D}/usr/share/dtpt/DTPT.py || die

	mkdir -p ${D}/etc/init.d || die
	cp ${FILESDIR}/init.dtpt ${D}/etc/init.d/dtpt || die
}

