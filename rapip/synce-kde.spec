# This spec file was generated by Gideon
# Please report any problem to KDevelop Team <kdevelop-devel@kdevelop.org>
# Thanks to Matthias Saou for his explanations on http://freshrpms.net/docs/fight.html

%define prefix	/usr
%define name	synce-kde
%define rel	1

Name: %{name}
Version: 0.6
Release: %{rel}
Vendor: The SynCE Project
Copyright: MIT
Summary: KDE-Integration of SynCE. Kio-slave and Tray-Icon.
Group: Applications/Productivity
Packager: Volker Christian <voc@users.sourceforge.net>
BuildRoot: %{_tmppath}/%{name}-root
Source: %{name}-0.6.tar.gz

%description
SynCE-KDE is part of the SynCE project:

  http://synce.sourceforge.net/

This Package is a KDE-Integration of SynCE. It consists of a kio_slave
(RAPIP), a KDE System-Tray Application (RAKI) and an enhanced direct cable
connection manager (VDCCM).

RAPIP: Lets you transparently interact with your PockePC via konqueror.
RAKI:  Is a Linux-Incarnation of Activsync. It claims to be better than
       Activesync in future.

%prep
%setup
%build
export KDEDIR=/usr
export CFLAGS="-pipe -march=i386 -mcpu=i686"
export CXXFLAGS="-pipe -march=i386 -mcpu=i686"
%configure --with-agsync=/home/voc/usr/src/agsync-0.2-pre
%makeinstall
%clean
rm -rf %{buildroot}
%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig
%files
%defattr(-, root, root)
%doc AUTHORS COPYING ChangeLog NEWS README TODO
%{_bindir}/*
%{_libdir}/*
%{_datadir}/*
%{_includedir}/*
