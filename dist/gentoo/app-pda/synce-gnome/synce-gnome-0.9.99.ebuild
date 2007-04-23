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
DEPEND=">=dev-libs/check-0.8.3.1
		dev-python/dbus-python"

# useflag release, if set it will create release-archives in /opt/yacy-svn/RELEASE

ESVN_REPO_URI="https://synce.svn.sourceforge.net/svnroot/synce/trunk/oleavr-files/synce-gnome"
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
	mkdir -p ${D}/usr/bin
	chmod 755 ${S}/src/test.py
	cp ${S}/src/test.py ${D}/usr/bin/synce-gnome-test.py
}
