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

SYNCE_VERSION    = 0.7

LIBRAPI2_VERSION = 0.7
LIBSYNCE_VERSION = 0.7
DCCM_VERSION     = 0.7
SERIAL_VERSION   = 0.7
RRA_VERSION      = 0.7
TRAYICON_VERSION = 0.4
GNOMEVFS_VERSION = 0.2
MULTISYNC_PLUGIN_VERSION = 0.7

REDHAT_VERSION   = $(shell cat /etc/redhat-release | sed 's/.*\([0-9]\.[0-9]\).*/\1/')

TARBALL_DIR   = $(PWD)/V$(SYNCE_VERSION)/tarballs/

#APTDIR        = $(PWD)/apt/$(REDHAT_VERSION)
#RPM_DIR       = $(APTDIR)/RPMS.synce
#SRPM_DIR      = $(APTDIR)/SRPMS.synce
RPM_DIR        = $(PWD)/V$(SYNCE_VERSION)/rpms/
SRPM_DIR       = $(PWD)/V$(SYNCE_VERSION)/srpms/

export TARBALL_DIR RPM_DIR SRPM_DIR

.PHONY: tarballs rpms apt synce extras trayicon gnomevfs multisync_plugin

all: synce

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

synce:
	mkdir -p $(RPM_DIR)
	-rm $(RPM_DIR)/*
	mkdir -p $(SRPM_DIR)
	-rm $(SRPM_DIR)/*
	-mkdir -p $(TARBALL_DIR)
	-rm $(TARBALL_DIR)/*
	$(MAKE) -f Makefile.module MODULE=libsynce  VERSION=$(LIBSYNCE_VERSION)  tarball
	$(MAKE) -f Makefile.module MODULE=librapi2  VERSION=$(LIBRAPI2_VERSION)  tarball
	$(MAKE) -f Makefile.module MODULE=dccm      VERSION=$(DCCM_VERSION)      tarball
	$(MAKE) -f Makefile.module MODULE=serial    VERSION=$(SERIAL_VERSION)    tarball
	$(MAKE) -f Makefile.module MODULE=rra       VERSION=$(RRA_VERSION)       tarball
	rpmbuild -ba synce.spec

extras: trayicon gnomevfs multisync_plugin

trayicon:
	$(MAKE) -f Makefile.module MODULE=trayicon  VERSION=$(TRAYICON_VERSION)  tarball
	$(MAKE) -f Makefile.module MODULE=trayicon  VERSION=$(TRAYICON_VERSION)  rpm

gnomevfs:
	$(MAKE) -f Makefile.module MODULE=gnomevfs  VERSION=$(GNOMEVFS_VERSION)  tarball
	$(MAKE) -f Makefile.module MODULE=gnomevfs  VERSION=$(GNOMEVFS_VERSION)  rpm

multisync_plugin:
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

