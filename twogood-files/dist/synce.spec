%define prefix   /usr
%define rel      1

Summary: SynCE: Core programs needed to use SynCE.
Name: synce
Version: 0.7
Release: %{rel}
License: MIT
Group: Applications/Communications
Source0: synce-libsynce-0.7.tar.gz
Source1: synce-librapi2-0.7.tar.gz
Source2: synce-dccm-0.7.tar.gz
Source3: synce-serial-0.7.tar.gz
Source4: synce-rra-0.7.tar.gz
URL: http://synce.sourceforge.net/
Distribution: SynCE RPM packages
Vendor: The SynCE Project
Buildroot: %{_tmppath}/%{name}-%{version}-%{release}-root
Requires: ppp

%description
Core programs needed to use SynCE.

The purpose of the SynCE project is to provide a means of communication with a
Windows CE or Pocket PC device from a computer running Linux, *BSD or other
unices.

See http://synce.sourceforge.net/ for more information.

%package devel
Summary:   SynCE: Files needed for developing with SynCE.
Group:     Development/Libraries
Requires:  %{name} = %{version}

%description devel
Files needed for developing with SynCE.

The purpose of the SynCE project is to provide a means of communication with a
Windows CE or Pocket PC device from a computer running Linux, *BSD or other
unices.

See http://synce.sourceforge.net/ for more information.


%prep
rm -rf $RPM_BUILD_ROOT
for S in %{SOURCE0} %{SOURCE1} %{SOURCE2} %{SOURCE3} %{SOURCE4}; do
	rm -rf `basename $S .tar.gz`
	tar zxf $S
done

%build
rm -rf $RPM_BUILD_ROOT
for S in %{SOURCE0} %{SOURCE1} %{SOURCE2} %{SOURCE3} %{SOURCE4}; do
	cd `basename $S .tar.gz`
	%configure --with-libsynce=$RPM_BUILD_ROOT%{prefix} --with-librapi2=$RPM_BUILD_ROOT%{prefix}
	%makeinstall
	cd ..
done

%install

%files
%defattr(-,root,root)
%{prefix}/bin
%{prefix}/lib/*.so.*
%{prefix}/share/synce
%{_mandir}/man1
%{_mandir}/man8

%files devel
%defattr(-,root,root)
%{prefix}/include
%{prefix}/lib/*.so
%{prefix}/lib/*.a
%{prefix}/lib/*.la
%{prefix}/share/aclocal

