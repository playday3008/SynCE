%define prefix   /usr
%define name     synce-librapi2
%define ver      0.4.1
%define rel      1

Summary: SynCE: Remote Application Programming Interface (RAPI) library.
Name: %{name}
Version: %{ver}
Release: %{rel}
License: MIT
Group: Development/Libraries
Source: %{name}-%{version}.tar.gz
URL: http://synce.sourceforge.net/
Distribution: SynCE RPM packages
Vendor: The SynCE Project
Packager: David Eriksson <twogood@users.sourceforge.net>
#Buildroot: %{_tmppath}/%{name}-%{version}-%{release}-root
Buildroot: %{_tmppath}/synce-root
Requires: synce-libsynce

%description
Librapi2 is part of the SynCE project:

  http://synce.sourceforge.net/

The RAPI library is an open source implementation that works like RAPI.DLL,
available on Microsoft operating systems. The library makes it possible to make
remote calls to a computer running Pocket PC. Documentation for the RAPI calls
is available at this address:

http://msdn.microsoft.com/library/default.asp?url=/library/en-us/wcesdkr/htm/_wcesdk_CeRapiInit.asp

%prep
%setup

%build
%configure --with-libsynce=$RPM_BUILD_ROOT%{prefix}
make

%install
%makeinstall

%files
%doc README LICENSE
%{prefix}/bin/pcp
%{prefix}/bin/pinfo
%{prefix}/bin/pls
%{prefix}/bin/pmkdir
%{prefix}/bin/pmv
%{prefix}/bin/prm
%{prefix}/bin/prmdir
%{prefix}/bin/prun
%{prefix}/bin/synce-install-cab
%{prefix}/include/rapi.h
%{prefix}/lib/librapi.*
%{_mandir}/man1/pcp.*
%{_mandir}/man1/pinfo.*
%{_mandir}/man1/pls.*
%{_mandir}/man1/pmkdir.*
%{_mandir}/man1/pmv.*
%{_mandir}/man1/prm.*
%{_mandir}/man1/prmdir.*
%{_mandir}/man1/prun.*
%{_mandir}/man1/synce-install-cab.*

