%define prefix   /usr
%define rel      1

Summary: SynCE: main package.
Name: synce
Version: 0.6
Release: %{rel}
License: MIT
Group: Development/Libraries
Source0: synce-libsynce-0.6.tar.gz
Source1: synce-librapi2-0.6.tar.gz
Source2: synce-dccm-0.6.tar.gz
Source3: synce-serial-0.6.tar.gz
URL: http://synce.sourceforge.net/
Distribution: SynCE RPM packages
Vendor: The SynCE Project
Packager: David Eriksson <twogood@users.sourceforge.net>
Buildroot: %{_tmppath}/%{name}-%{version}-%{release}-root
Requires: ppp

%description
The purpose of the SynCE project is to provide a means of communication with a
Windows CE or Pocket PC device from a computer running Linux, *BSD or other
unices.

See http://synce.sourceforge.net/ for more information.

%prep
for S in %{SOURCE0} %{SOURCE1} %{SOURCE2} %{SOURCE3}; do
	rm -rf `basename $S .tar.gz`
	tar zxf $S
done

%build
rm -rf $RPM_BUILD_ROOT
for S in %{SOURCE0} %{SOURCE1} %{SOURCE2} %{SOURCE3}; do
	cd `basename $S .tar.gz`
	%configure --with-libsynce=$RPM_BUILD_ROOT%{prefix} --with-librapi2=$RPM_BUILD_ROOT%{prefix}
	make
	cd ..
done

%install
for S in %{SOURCE0} %{SOURCE1} %{SOURCE2} %{SOURCE3}; do
	cd `basename $S .tar.gz`
	%makeinstall
	cd ..
done

%files
%defattr(-,root,root)
%{prefix}/bin
%{prefix}/include
%{prefix}/lib
%{prefix}/share/aclocal
%{prefix}/share/synce
%{_mandir}/man1
%{_mandir}/man8

