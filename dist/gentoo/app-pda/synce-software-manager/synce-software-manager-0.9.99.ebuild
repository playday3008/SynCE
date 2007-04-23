# Copyright 1999-2006 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: $

inherit subversion
#gnome2 autotools

DESCRIPTION="Synchronize Windows CE devices with computers running GNU/Linux, like MS ActiveSync."
HOMEPAGE="http://sourceforge.net/projects/synce/"

LICENSE="GPL-2"
SLOT="0"
KEYWORDS="~amd64 ~x86 ~ppc"
IUSE=""

DEPEND="virtual/libc
	=app-pda/synce-librapi2-0.9.99
	>=x11-libs/gtk+-2.0
	>=gnome-base/libgtop-2
	>=gnome-base/libgnome-2
	>=gnome-base/libglade-2
	>=gnome-base/libgnomeui-2.0"

ESVN_REPO_URI="https://svn.sourceforge.net/svnroot/synce/trunk/software-manager"
#ESVN_REPO_URI="https://svn.sourceforge.net/svnroot/synce/branches/trayicon/TWOGOOD/trayicon"
ESVN_FETCH_CMD="svn checkout"
ESVN_UPDATE_CMD="svn up"

S="${WORKDIR}/trunk"
src_unpack() {
	subversion_src_unpack
	cd ${S}
}

src_compile() {
	touch COPYING
	touch INSTALL
	GTK=`pkg-config --libs-only-l libglade-2.0 libgnomeui-2.0 librapi2`

#	export WANT_AUTOMAKE="1.9" # Force usage of automake 1.9 for safety
	./autogen.sh
	econf || die "econf failed"
	sed -i s:@install_sh@:../install-sh: po/Makefile
	emake LIBS="$GTK" || die
}

src_install() {
	make DESTDIR=${D} install || die
}
