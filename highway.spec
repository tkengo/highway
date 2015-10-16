Summary: High performance source code search tool
Name: highway
Version: 1.0.0
Release: 1
Source: https://github.com/tkengo/%{name}/archive/v%{version}.tar.gz
License: MIT License
BuildRoot: %{_tmppath}/%{name}-%{version}-buildroot
BuildRequires: autoconf, automake, gperftools-devel
Requires: autoconf, automake, gperftools-devel

%description
High performance source code search tool.

%prep
%setup -q -n highway-%{version}

%build
aclocal
autoconf
autoheader
automake --add-missing
./configure
make

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT

%clean
rm -rf $RPM_BUILD_ROOT

%files
/usr/local/bin/hw

%changelog
