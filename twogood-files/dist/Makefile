# $Id$

PWD     ?= $(shell pwd)

REDHAT_VERSION   = $(shell cat /etc/redhat-release | sed 's/.*\([0-9]\.[0-9]\).*/\1/')

TOPDIR   = $(PWD)/../..
TARBALLS = $(PWD)/tarballs
APTDIR   = $(PWD)/apt/$(REDHAT_VERSION)

RPMS     = $(APTDIR)/RPMS.synce
SRPMS    = $(APTDIR)/SRPMS.synce

export PWD USER TOPDIR TARBALLS APTDIR RPMS SRPMS

.PHONY: tarballs rpms apt

all: tarballs rpms

trayicon:
	$(MAKE) -f Makefile.release NAME=trayicon tarball
	$(MAKE) -f Makefile.release NAME=trayicon rpm

tag:
	$(MAKE) -f Makefile.release NAME=serial   tag
	$(MAKE) -f Makefile.release NAME=libsynce tag
	$(MAKE) -f Makefile.release NAME=librapi2 tag
	$(MAKE) -f Makefile.release NAME=dccm     tag
	$(MAKE) -f Makefile.release NAME=trayicon tag

tarballs:
	-mkdir $(TARBALLS)
	$(MAKE) -f Makefile.release NAME=serial   tarball
	$(MAKE) -f Makefile.release NAME=libsynce tarball
	$(MAKE) -f Makefile.release NAME=librapi2 tarball
	$(MAKE) -f Makefile.release NAME=dccm     tarball

rpms:
	-rm -rf /var/tmp/synce-root
	mkdir -p $(RPMS)
	-rm $(RPMS)/*
	mkdir -p $(SRPMS)
	-rm $(SRPMS)/*
	$(MAKE) -f Makefile.release NAME=serial   rpm
	$(MAKE) -f Makefile.release NAME=libsynce rpm
	$(MAKE) -f Makefile.release NAME=librapi2 rpm
	$(MAKE) -f Makefile.release NAME=dccm     rpm

apt:
	genbasedir --flat --bloat --bz2only $(APTDIR) synce
	cd $(APTDIR)/.. && scp -r $(REDHAT_VERSION) \
		shell.sourceforge.net:/home/users/t/tw/twogood/synce/htdocs/apt/redhat/en/i386/


