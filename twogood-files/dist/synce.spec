%define prefix   /usr
%define rel      1

Summary: SynCE: Core programs needed to use SynCE.
Name: synce
Version: 0.8
Release: %{rel}
License: MIT
Group: Applications/Communications
Source0: synce-libsynce-0.8.tar.gz
Source1: synce-librapi2-0.8.tar.gz
Source2: synce-dccm-0.8.tar.gz
Source3: synce-serial-0.8.tar.gz
Source4: libmimedir-0.2.tar.gz
Source5: synce-rra-0.8.tar.gz
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
for S in %{SOURCE0} %{SOURCE1} %{SOURCE2} %{SOURCE3} %{SOURCE4} %{SOURCE5}; do
	rm -rf `basename $S .tar.gz`
	tar zxf $S
done

%build
rm -rf $RPM_BUILD_ROOT
for S in %{SOURCE0} %{SOURCE1} %{SOURCE2} %{SOURCE3} %{SOURCE4} %{SOURCE5}; do
	cd `basename $S .tar.gz`
	%configure --with-libsynce=$RPM_BUILD_ROOT%{prefix} --with-librapi2=$RPM_BUILD_ROOT%{prefix} --with-libmimedir=$RPM_BUILD_ROOT%{prefix}
	%makeinstall
	cd ..
done

%install
rm $RPM_BUILD_ROOT%{prefix}/include/libmimedir.h

#
# Try to prevent errors like this:
#
# libtool: link: warning: library `/usr/lib/librra.la' was moved.
# grep: /var/tmp/synce-0.8-1-root/usr/lib/librapi.la: No such file or directory
# /bin/sed: can't read /var/tmp/synce-0.8-1-root/usr/lib/librapi.la: No such file or directory
# libtool: link: `/var/tmp/synce-0.8-1-root/usr/lib/librapi.la' is not a valid libtool archive
#
PATTERN=`echo $RPM_BUILD_ROOT | sed 's/\//\\\\\//g'`
sed -i "s/$PATTERN//g" $RPM_BUILD_ROOT%{_libdir}/*.la

%files
%defattr(-,root,root)
%{_bindir}
%{_libdir}/*.so.*
%{_datadir}/synce
%{_mandir}/man1
%{_mandir}/man8

%files devel
%defattr(-,root,root)
%{_includedir}
%{_libdir}/*.so
%{_libdir}/*.a
%{_libdir}/*.la
%{_datadir}/aclocal

