# $Id$

PWD     ?= $(shell pwd)

TOPDIR   = $(PWD)/../..
TARBALLS = $(PWD)/tarballs
APTDIR   = $(PWD)/apt

RPMS     = $(APTDIR)/RPMS.synce
SRPMS    = $(APTDIR)/SRPMS.synce

LIBSYNCE_VERSION = 0.1
LIBRAPI2_VERSION = 0.1
DCCMD_VERSION    = 0.1

export PWD USER TOPDIR TARBALLS APTDIR RPMS SRPMS

.PHONY: tarballs rpms

all: tarballs rpms

tarballs:
	-mkdir $(TARBALLS)
	$(MAKE) -f Makefile.release NAME=libsynce VERSION=$(LIBSYNCE_VERSION) tarball
	$(MAKE) -f Makefile.release NAME=librapi2 VERSION=$(LIBRAPI2_VERSION) tarball
	$(MAKE) -f Makefile.release NAME=dccmd    VERSION=$(DCCMD_VERSION)    tarball

rpms:
	mkdir -p $(RPMS)
	mkdir -p $(SRPMS)
	$(MAKE) -f Makefile.release NAME=libsynce VERSION=$(LIBSYNCE_VERSION) rpm
	$(MAKE) -f Makefile.release NAME=librapi2 VERSION=$(LIBRAPI2_VERSION) rpm
	$(MAKE) -f Makefile.release NAME=dccmd    VERSION=$(DCCMD_VERSION)    rpm

apt:
	genbasedir --flat --bloat --bz2only $(APTDIR) synce
	cd $(APTDIR) && scp -r * shell.sourceforge.net:/home/users/t/tw/twogood/synce/htdocs/apt/redhat/en/i386/8.0/


