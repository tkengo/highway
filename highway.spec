Summary: High performance source code search tool
Name: highway
Version: 1.0.0
Release: 1
Source: v%{version}.tar.gz
License: MIT License
BuildRoot: %{_tmppath}/%{name}-%{version}-buildroot
BuildPrereq: autoconf
BuildPrereq: automake
BuildPrereq: gperftools
Requires: autoconf
Requires: automake
Requires: gperftools-devel

%description
High performance source code search tool.

%prep
%setup -q -n php-%{version}

%build
# つくるくん。
./configure --with-apxs2=/usr/local/apache2/bin/apxs \
--enable-mbstring \
--enable-mbregex \
--with-gd \
--with-zlib \

make

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr/local/apache2/conf
mkdir $RPM_BUILD_ROOT/usr/local/apache2/bin
mkdir $RPM_BUILD_ROOT/usr/local/apache2/modules
cp /usr/local/apache2/conf/httpd.conf $RPM_BUILD_ROOT/usr/local/apache2/conf/httpd.conf

make install INSTALL_ROOT=$RPM_BUILD_ROOT 

rm -rf $RPM_BUILD_ROOT/usr/local/apache2/conf/httpd.conf
rm -rf $RPM_BUILD_ROOT/usr/local/apache2/conf/httpd.conf.bak

rm -rf $RPM_BUILD_ROOT/.channels
rm -rf $RPM_BUILD_ROOT/.registry
rm -rf $RPM_BUILD_ROOT/.depdb
rm -rf $RPM_BUILD_ROOT/.depdblock
rm -rf $RPM_BUILD_ROOT/.filemap
rm -rf $RPM_BUILD_ROOT/.lock

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
/usr/local/apache2/modules/libphp5.so
/usr/local/bin/pear
/usr/local/bin/peardev
/usr/local/bin/pecl
/usr/local/bin/php
/usr/local/bin/php-config
/usr/local/bin/phpize
%config /usr/local/etc/pear.conf
/usr/local/include/php/*
/usr/local/lib/php/*
/usr/local/lib/php/.channels/*
/usr/local/lib/php/.channels/.alias/*
/usr/local/lib/php/.registry/*
/usr/local/lib/php/.depdb
/usr/local/lib/php/.depdblock
/usr/local/lib/php/.filemap
/usr/local/lib/php/.lock
/usr/local/man/man1/php*

%changelog
* Thu Oct 28 2010 tesutosakusei tekitou@tekitou.tek
- tesutodesu
