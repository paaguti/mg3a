Name:		mg3a
Version:	250112
Release:	1%{?dist}
Summary:	Tiny Emacs-like editor

License:	Public Domain
URL:		https://github.com/paaguti/mg3a
Source0:	https://github.com/paaguti/%{name}/archive/refs/tags/%{version}.tar.gz

BuildArch: x86_64 

BuildRequires: make
BuildRequires: gcc
BuildRequires: ncurses-devel
BuildRequires: autoconf
BuildRequires: automake

%define _unpackaged_files_terminate_build 0

%description
mg is a tiny, mostly public-domain Emacs-like editor included in the base
OpenBSD system. It is compatible with Emacs because there shouldn't be any
reason to learn more editor types than Emacs or vi.

%prep
tar --strip-components=1 -xvf ../../SOURCES/mg3a-%{version}.tar.gz
./bootstrap.sh

%build
%configure --enable-mouse
%make_build

%install
%make_install

%files
%defattr(-,root,root,-)
%{_bindir}/mg
%license COPYING
%docdir /usr/share/doc/mg3a
/usr/share/doc/mg3a/READMES.zip
/usr/share/doc/mg3a/samples.zip
/usr/share/doc/mg3a/orig-doc.zip

%changelog
* Sun Jan 12 2025 Pedro A. Aranda - 250112
- Compress READMEs and examples
