%define prefix   /usr
%define name     synce-serial
%define ver      0.2
%define rel      1

Summary: SynCE: Serial connection support.
Name: %{name}
Version: %{ver}
Release: %{rel}
License: MIT
Group: Applications/Communications
Source: %{name}-%{version}.tar.gz
URL: http://synce.sourceforge.net/
Distribution: SynCE RPM packages
Vendor: The SynCE Project
Packager: David Eriksson <twogood@users.sourceforge.net>
#Buildroot: %{_tmppath}/%{name}-%{version}-%{release}-root
Buildroot: %{_tmppath}/synce-root
Requires: ppp synce-dccmd

%description
Synce-serial is part of the SynCE project:

  http://synce.sourceforge.net/

This module contains helper scripts for setting up a serial connection for use
with SynCE.

%prep
%setup

%build
%configure
make

%install
%makeinstall

%files
%{prefix}/bin/synce-serial-*
%{prefix}/share/synce/synce-serial-*
%{_mandir}/man8/synce-serial-*

