%define prefix   /usr
%define name     synce-trayicon
%define ver      0.1
%define rel      1

Summary: SynCE: Tray icon for GNOME 2.
Name: %{name}
Version: %{ver}
Release: %{rel}
License: MIT LGPL
Group: Applications/Communications
Source: %{name}-%{version}.tar.gz
URL: http://synce.sourceforge.net/
Distribution: SynCE RPM packages
Vendor: The SynCE Project
Packager: David Eriksson <twogood@users.sourceforge.net>
#Buildroot: %{_tmppath}/%{name}-%{version}-%{release}-root
Buildroot: %{_tmppath}/synce-root
Requires: synce-librapi2 gtk2 atk libgnomeui

%description
synce-trayicon is part of the SynCE project:

  http://synce.sourceforge.net/

This application shows when a device is connected.

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
%{prefix}/bin/dccm
%{_mandir}/man1/dccm.*

