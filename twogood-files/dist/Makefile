# $Id$

#
# Use of this Makefile:
#
# 1. Run `make synce`
#
# 2. Install the "synce" and "synce-devel" RPMs
#
# 3. Run `make extras`
#
# 4. Uninstall the "synce" and "synce-devel" RPMs
#

SYNCE_VERSION    = 0.8

LIBRAPI2_VERSION = 0.8
LIBSYNCE_VERSION = 0.8
DCCM_VERSION     = 0.8
SERIAL_VERSION   = 0.8
RRA_VERSION      = 0.8.1
TRAYICON_VERSION = 0.8
MULTISYNC_PLUGIN_VERSION = 0.8.1

GNOMEVFS_VERSION = 0.8

REDHAT_VERSION   = $(shell cat /etc/redhat-release | sed 's/.*\([0-9]\.[0-9]\).*/\1/')
RPMROOT=$(shell rpmbuild --showrc | grep ": _topdir	" | sed 's/^.*	//' | sed 's/%{_usrsrc}/\/usr\/src/')

TARBALL_DIR   = $(PWD)/V$(SYNCE_VERSION)/tarballs/

#APTDIR        = $(PWD)/apt/$(REDHAT_VERSION)
#RPM_DIR       = $(APTDIR)/RPMS.synce
#SRPM_DIR      = $(APTDIR)/SRPMS.synce
RPM_DIR        = $(PWD)/V$(SYNCE_VERSION)/rpms/
SRPM_DIR       = $(PWD)/V$(SYNCE_VERSION)/srpms/

export TARBALL_DIR RPM_DIR SRPM_DIR

.PHONY: tarballs rpms apt synce extras trayicon gnomevfs multisync_plugin

all:
	@echo "What?"

#trayicon:
#	$(MAKE) -f Makefile.release NAME=trayicon tarball
#	$(MAKE) -f Makefile.release NAME=trayicon rpm

#cvsclean:
#	$(MAKE) -f Makefile.module MODULE=libsynce  VERSION=$(LIBSYNCE_VERSION)  cvsclean
#	$(MAKE) -f Makefile.module MODULE=librapi2  VERSION=$(LIBRAPI2_VERSION)  cvsclean
#	$(MAKE) -f Makefile.module MODULE=dccm      VERSION=$(DCCM_VERSION)      cvsclean
#	$(MAKE) -f Makefile.module MODULE=serial    VERSION=$(SERIAL_VERSION)    cvsclean
#	$(MAKE) -f Makefile.module MODULE=rra       VERSION=$(RRA_VERSION)       cvsclean

#checkout:
#	$(MAKE) -f Makefile.module MODULE=libsynce  VERSION=$(LIBSYNCE_VERSION)  checkout
#	$(MAKE) -f Makefile.module MODULE=librapi2  VERSION=$(LIBRAPI2_VERSION)  checkout
#	$(MAKE) -f Makefile.module MODULE=dccm      VERSION=$(DCCM_VERSION)      checkout
#	$(MAKE) -f Makefile.module MODULE=serial    VERSION=$(SERIAL_VERSION)    checkout
#	$(MAKE) -f Makefile.module MODULE=rra       VERSION=$(RRA_VERSION)       checkout

LIBMIMEDIR_TGZ = $(PWD)/../../libmimedir-0.2.tar.gz

#libmimedir:
#	cd /var/tmp && \
#	tar zxf $(LIBMIMEDIR_TGZ) && \
#	cd `basename $(LIBMIMEDIR_TGZ) .tar.gz` && \
#	./configure --prefix=/var/tmp/synce && \
#	make all install

synce:
	mkdir -p $(RPM_DIR)
	-rm $(RPM_DIR)/*
	mkdir -p $(SRPM_DIR)
	-rm $(SRPM_DIR)/*
	-mkdir -p $(TARBALL_DIR)
	-rm $(TARBALL_DIR)/*
	$(MAKE) -f Makefile.module MODULE=libsynce  VERSION=$(LIBSYNCE_VERSION)  checkout
	$(MAKE) -f Makefile.module MODULE=librapi2  VERSION=$(LIBRAPI2_VERSION)  checkout
	$(MAKE) -f Makefile.module MODULE=dccm      VERSION=$(DCCM_VERSION)      checkout
	$(MAKE) -f Makefile.module MODULE=serial    VERSION=$(SERIAL_VERSION)    checkout
	$(MAKE) -f Makefile.module MODULE=rra       VERSION=$(RRA_VERSION)       checkout
	$(MAKE) -f Makefile.module MODULE=libsynce  VERSION=$(LIBSYNCE_VERSION)  tarball
	$(MAKE) -f Makefile.module MODULE=librapi2  VERSION=$(LIBRAPI2_VERSION)  tarball
	$(MAKE) -f Makefile.module MODULE=dccm      VERSION=$(DCCM_VERSION)      tarball
	$(MAKE) -f Makefile.module MODULE=serial    VERSION=$(SERIAL_VERSION)    tarball
	$(MAKE) -f Makefile.module MODULE=rra       VERSION=$(RRA_VERSION)       tarball
	cp $(LIBMIMEDIR_TGZ) $(TARBALL_DIR)/
	cp $(LIBMIMEDIR_TGZ) $(RPMROOT)/SOURCES/
	rpmbuild -ba synce.spec
	cp $(RPMROOT)/SRPMS/synce-$(SYNCE_VERSION)-*.rpm       			$(SRPM_DIR)/
	cp $(RPMROOT)/RPMS/i386/synce-$(SYNCE_VERSION)-*.rpm        $(RPM_DIR)/
	cp $(RPMROOT)/RPMS/i386/synce-devel-$(SYNCE_VERSION)-*.rpm  $(RPM_DIR)/

extras: trayicon gnomevfs multisync_plugin

trayicon:
	$(MAKE) -f Makefile.module MODULE=trayicon  VERSION=$(TRAYICON_VERSION)  checkout
	$(MAKE) -f Makefile.module MODULE=trayicon  VERSION=$(TRAYICON_VERSION)  tarball
	$(MAKE) -f Makefile.module MODULE=trayicon  VERSION=$(TRAYICON_VERSION)  rpm

gnomevfs:
	$(MAKE) -f Makefile.module MODULE=gnomevfs  VERSION=$(GNOMEVFS_VERSION)  checkout
	$(MAKE) -f Makefile.module MODULE=gnomevfs  VERSION=$(GNOMEVFS_VERSION)  tarball
	$(MAKE) -f Makefile.module MODULE=gnomevfs  VERSION=$(GNOMEVFS_VERSION)  rpm

multisync_plugin:
	$(MAKE) -f Makefile.module MODULE=multisync_plugin VERSION=$(MULTISYNC_PLUGIN_VERSION)  checkout
	$(MAKE) -f Makefile.module MODULE=multisync_plugin VERSION=$(MULTISYNC_PLUGIN_VERSION)  tarball
	$(MAKE) -f Makefile.module MODULE=multisync_plugin VERSION=$(MULTISYNC_PLUGIN_VERSION)  rpm

#rpms:
#	-rm -rf /var/tmp/synce-root
#	$(MAKE) -f Makefile.module MODULE=libsynce  VERSION=$(LIBSYNCE_VERSION)  rpm
#	$(MAKE) -f Makefile.module MODULE=librapi2  VERSION=$(LIBRAPI2_VERSION)  rpm
#	$(MAKE) -f Makefile.module MODULE=dccm      VERSION=$(DCCM_VERSION)      rpm
#	$(MAKE) -f Makefile.module MODULE=serial    VERSION=$(SERIAL_VERSION)    rpm
#
#trayicon:
#	$(MAKE) -f Makefile.module MODULE=trayicon  VERSION=$(TRAYICON_VERSION)  checkout
#	$(MAKE) -f Makefile.module MODULE=trayicon  VERSION=$(TRAYICON_VERSION)  rpm

apt:
	genbasedir --flat --bloat --bz2only $(APTDIR) synce
	cd $(APTDIR)/.. && scp -r $(REDHAT_VERSION) \
		shell.sourceforge.net:/home/users/t/tw/twogood/synce/htdocs/apt/redhat/en/i386/

upload:
	ncftpput upload.sourceforge.net /incoming/ $(RPM_DIR)/* $(SRPM_DIR)/* $(TARBALL_DIR)/*

install:
	cd apt/$(REDHAT_VERSION)/RPMS.synce/ && rpm -Uvh synce-serial* synce-libsynce* synce-librapi2* synce-dccm*

trayicon_install:
	cd apt/$(REDHAT_VERSION)/RPMS.synce/ && rpm -Uvh synce-trayicon*

uninstall:
	-rpm -e synce-serial synce-libsynce synce-librapi2 synce-dccm synce-trayicon

