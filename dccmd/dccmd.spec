%define prefix   /usr
%define name     synce-dccmd
%define ver      0.3
%define rel      1

Summary: SynCE: Communication daemon.
Name: %{name}
Version: %{ver}
Release: %{rel}
License: MIT
Group: System Environment/Daemons
Source: %{name}-%{version}.tar.gz
URL: http://synce.sourceforge.net/
Distribution: SynCE RPM packages
Vendor: The SynCE Project
Packager: David Eriksson <twogood@users.sourceforge.net>
#Buildroot: %{_tmppath}/%{name}-%{version}-%{release}-root
Buildroot: %{_tmppath}/synce-root
Requires: synce-libsynce

%description
Dccmd is part of the SynCE project:

  http://synce.sourceforge.net/

This daemon is required to be able to communicate with a handheld device.

%prep
%setup

%build
%configure --with-libsynce=$RPM_BUILD_ROOT%{prefix}
#--with-libsynce=%{prefix}
make

%install
%makeinstall

%files
%doc README LICENSE
%{prefix}/bin/dccmd
%{_mandir}/man1/dccmd.*

