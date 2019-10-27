Name:     twolame
Version:  0.4.0
Release:  1%{?dist}
Summary:  MPEG Audio Layer 2 (MP2) encoder
Group:    Applications/Multimedia
License:  LGPLv2
URL:      http://www.twolame.org/
Source:   http://www.twolame.org/download/%{name}-%{version}.tar.gz

BuildRequires:  libsndfile-devel
Requires: libsndfile
Requires: twolame-libs = %{version}-%{release}

%description
TwoLAME is an optimized MPEG Audio Layer 2 (MP2) encoder. It is based heavily on:
- tooLAME by Michael Cheng
- the ISO dist10 code
- improvement to algorithms as part of the LAME project

This package contains the command line frontend.


%package devel
Summary:  Libraries and header files for apps encoding MPEG Layer 2 audio
Group:    Development/Libraries
Requires: twolame-libs = %{version}-%{release}
Requires: pkgconfig

%description devel
Header files and a library for encoding MPEG Layer 2 audio, for developing apps
which will use the library.


%package libs
Summary:  Libraries for applications encoding MPEG Layer 2 audio
Group:    System Environment/Libraries

%description libs
Dynamic libraries for applications encoding MPEG Layer 2 audio.



%prep
%setup -q

%build
%configure
%{__make} %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
%{__make} DESTDIR=$RPM_BUILD_ROOT install

%clean
rm -rf $RPM_BUILD_ROOT

%post libs -p /sbin/ldconfig
%postun libs -p /sbin/ldconfig

%files
%defattr(-, root, root, 0755)
%attr(755,root,root) %{_bindir}/twolame
%{_mandir}/man1/*

%files libs
%doc COPYING
%{_libdir}/libtwolame.so.*

%files devel
%{_libdir}/pkgconfig/twolame.pc
%{_includedir}/twolame.h
%{_libdir}/*.so
%{_libdir}/libtwolame*.a
%{_libdir}/libtwolame*.la
%{_docdir}/*

%changelog
