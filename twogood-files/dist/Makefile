# $Id$

LIBSYNCE_VERSION = 0.5
LIBRAPI2_VERSION = 0.5
DCCM_VERSION     = 0.5
SERIAL_VERSION   = 0.5
TRAYICON_VERSION = 0.3

REDHAT_VERSION   = $(shell cat /etc/redhat-release | sed 's/.*\([0-9]\.[0-9]\).*/\1/')

TARBALL_DIR   = $(PWD)/tarballs

APTDIR        = $(PWD)/apt/$(REDHAT_VERSION)
RPM_DIR       = $(APTDIR)/RPMS.synce
SRPM_DIR      = $(APTDIR)/SRPMS.synce

export TARBALL_DIR RPM_DIR SRPM_DIR

.PHONY: tarballs rpms apt

all: checkout rpms

#trayicon:
#	$(MAKE) -f Makefile.release NAME=trayicon tarball
#	$(MAKE) -f Makefile.release NAME=trayicon rpm

cvsclean:
	$(MAKE) -f Makefile.module MODULE=libsynce  VERSION=$(LIBSYNCE_VERSION)  cvsclean
	$(MAKE) -f Makefile.module MODULE=librapi2  VERSION=$(LIBRAPI2_VERSION)  cvsclean
	$(MAKE) -f Makefile.module MODULE=dccm      VERSION=$(DCCM_VERSION)      cvsclean
	$(MAKE) -f Makefile.module MODULE=serial    VERSION=$(SERIAL_VERSION)    cvsclean
	$(MAKE) -f Makefile.module MODULE=trayicon  VERSION=$(TRAYICON_VERSION)  cvsclean

checkout:
	$(MAKE) -f Makefile.module MODULE=libsynce  VERSION=$(LIBSYNCE_VERSION)  checkout
	$(MAKE) -f Makefile.module MODULE=librapi2  VERSION=$(LIBRAPI2_VERSION)  checkout
	$(MAKE) -f Makefile.module MODULE=dccm      VERSION=$(DCCM_VERSION)      checkout
	$(MAKE) -f Makefile.module MODULE=serial    VERSION=$(SERIAL_VERSION)    checkout
	$(MAKE) -f Makefile.module MODULE=trayicon  VERSION=$(TRAYICON_VERSION)  checkout

tarballs:
	-mkdir $(TARBALL_DIR)
	-rm $(TARBALL_DIR)/*
	$(MAKE) -f Makefile.module MODULE=libsynce  VERSION=$(LIBSYNCE_VERSION)  tarball
	$(MAKE) -f Makefile.module MODULE=librapi2  VERSION=$(LIBRAPI2_VERSION)  tarball
	$(MAKE) -f Makefile.module MODULE=dccm      VERSION=$(DCCM_VERSION)      tarball
	$(MAKE) -f Makefile.module MODULE=serial    VERSION=$(SERIAL_VERSION)    tarball
	$(MAKE) -f Makefile.module MODULE=trayicon  VERSION=$(TRAYICON_VERSION)  tarball

rpms:
	-rm -rf /var/tmp/synce-root
	mkdir -p $(RPM_DIR)
	-rm $(RPM_DIR)/*
	mkdir -p $(SRPM_DIR)
	-rm $(SRPM_DIR)/*
	$(MAKE) -f Makefile.module MODULE=libsynce  VERSION=$(LIBSYNCE_VERSION)  rpm
	$(MAKE) -f Makefile.module MODULE=librapi2  VERSION=$(LIBRAPI2_VERSION)  rpm
	$(MAKE) -f Makefile.module MODULE=dccm      VERSION=$(DCCM_VERSION)      rpm
	$(MAKE) -f Makefile.module MODULE=serial    VERSION=$(SERIAL_VERSION)    rpm
	$(MAKE) -f Makefile.module MODULE=trayicon  VERSION=$(TRAYICON_VERSION)  rpm

apt:
	genbasedir --flat --bloat --bz2only $(APTDIR) synce
	cd $(APTDIR)/.. && scp -r $(REDHAT_VERSION) \
		shell.sourceforge.net:/home/users/t/tw/twogood/synce/htdocs/apt/redhat/en/i386/

upload:
	ncftpput upload.sourceforge.net /incoming/ $(RPM_DIR)/* $(SRPM_DIR)/* $(TARBALL_DIR)/*

install:
	cd apt/$(REDHAT_VERSION)/RPMS.synce/ && rpm -Uvh synce-serial* synce-libsynce* synce-librapi2* synce-dccm*

uninstall:
	rpm -e synce-serial synce-libsynce synce-librapi2 synce-dccm synce-trayicon

