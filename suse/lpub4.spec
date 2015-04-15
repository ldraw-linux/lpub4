# spec file for package lpub4
#
# Copyright (c) 2014 SUSE LINUX Products GmbH, Nuernberg, Germany.
#
# All modifications and additions to the file contributed by third parties
# remain the property of their copyright owners, unless otherwise agreed
# upon. The license for this file, and modifications and additions to the
# file, is the same license as for the pristine package itself (unless the
# license for the pristine package is not an Open Source License, in which
# case the license is the MIT License). An "Open Source License" is a
# license that conforms to the Open Source Definition (Version 1.9)
# published by the Open Source Initiative.

# Please submit bugfixes or comments via http://bugs.opensuse.org/
#

Name:           lpub4
Version:	__VERSION__
Release:	0
License:	GPL-2.0
Summary:	Creates building instructions for LEGO models using the LDraw file format
Url:		http://lpub4.sourceforge.net/
Group:		Productivity/Graphics/CAD
Source:		lpub4.tar.bz2
BuildRequires:	libqt4-devel libpng-devel gcc-c++
Requires:	ldraw-library>=2014.02.0 ldraw-renderer
BuildRoot:      %{_tmppath}/%{name}-%{version}-build
Provides:	lpub

%description
LPub is a program that allow the production of step by step building
instructions for LEGO models created using the LDraw file format. 

LDraw is an open standard for LEGO CAD programs that allow the user to create
virtual LEGO models and scenes.

%prep
%setup -q -n lpub4

%build
qmake
make

%install
install -d %{buildroot}/%{_bindir}
install -m 755 lpub  %{buildroot}/%{_bindir}/lpub.bin
ln -s %{_libexecdir}/ldraw-wrapper %{_bindir}/lpub

%files
%defattr(-,root,root)
%{_bindir}/lpub

%changelog
