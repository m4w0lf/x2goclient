Name:           x2goclient
Version:        4.1.2.3
Release:        0.0x2go1%{?dist}

# Default to Qt 4 for now.
%define         qt_version 4
# FC31+ or EPEL 8.
# Tumbleweed or *SuSE 15.3+.
%if ( "%{?_vendor}" == "redhat" && ( 0%{?fedora} > 31 || 0%{?el8} ) ) || ( "%{?_vendor}" == "suse" && ( 0%{?suse_version} > 1500 || 0%{?sle_version} >= 150200 ) )
%define         qt_version 5
%endif


Summary:        X2Go Client application (Qt%{qt_version})

%if 0%{?suse_version}
Group:          Production/Networking/Remote Desktop
License:        GPL-2.0+
%else
Group:          Applications/Communications
License:        GPLv2+
%endif

URL:            https://www.x2go.org
Source0:        https://code.x2go.org/releases/source/%{name}/%{name}-%{version}.tar.gz
Source1:        x2goclient-rpmlintrc

BuildRequires:  cups-devel
BuildRequires:  desktop-file-utils

%if 0%{?suse_version}
buildrequires:  openldap2-devel
%else
BuildRequires:  openldap-devel
%endif

%if %{qt_version} == 4
%if 0%{?suse_version}
BuildRequires:  libqt4-devel
%if 0%{?suse_version} >= 1310
BuildRequires:  libqt4-linguist
%endif
%else
%if 0%{?el5} || 0%{?el6}
BuildRequires:  qt4-devel
%else
BuildRequires:  qt-devel
%endif
%endif
%else
%if %{qt_version} == 5
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Gui)
BuildRequires:  pkgconfig(Qt5Svg)
BuildRequires:  pkgconfig(Qt5Widgets)
%if 0%{?suse_version}
BuildRequires:  libqt5-linguist
%else
BuildRequires:  qt5-linguist
%endif
%endif
%endif

%if "%{?_vendor}" == "redhat"
%if 0%{?fedora} || 0%{?el7}
BuildRequires:  man2html-core
%else
BuildRequires:  man
%endif
BuildRequires:  libssh-devel >= 0.5.5-2.1x2go1
BuildRequires:  libXpm-devel, libX11-devel
%endif

%if 0%{?el5} || 0%{?el6} || 0%{?el7}
# EPEL still calls the package pkconfig for some reason.
BuildRequires:  pkgconfig
%else
BuildRequires:  pkg-config
%endif

%if 0%{?fedora} || 0%{?rhel}
# For some reason qt(4)-dev doesn't depend upon redhat-rpm-config,
# but the GCC spec file is still used, which leads to gcc failing
# due to a missing annobin plugin during compilation.
# Let's build-depend upon redhat-rpm-config for now manually.
BuildRequires:  redhat-rpm-config
%endif

%if "%{?_vendor}" == "suse"
BuildRequires:  fdupes update-desktop-files
%if 0%{?suse_version} >= 1130
BuildRequires:  pkgconfig(libssh) >= 0.6.3
BuildRequires:  pkgconfig(x11) pkgconfig(xpm) pkgconfig(xproto)
%endif
%if 0%{?suse_version} && 0%{?suse_version} < 1130
BuildRequires:  libssh-devel >= 0.6.3
BuildRequires:  xorg-x11-libXpm-devel xorg-x11-proto-devel
BuildRequires:  xorg-x11-libX11-devel
%endif
%endif

Requires:       hicolor-icon-theme
Requires:       nxproxy
%if 0%{?suse_version}
Requires:       openssh
%else
Requires:       openssh-clients, openssh-server
%endif

%if "%{?_vendor}" == "suse"
Requires:       terminus-font
%endif
%if "%{?_vendor}" == "redhat"
Requires:       terminus-fonts
%endif
%if 0%{?suse_version} >= 1100
Suggests:       pinentry-x2go
%endif

%if 0%{?el5}
# For compatibility with EPEL5
BuildRoot:      %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)
%else
BuildRoot:      %{_tmppath}/%{name}-%{version}-build
%endif


%description
X2Go is a server-based computing environment with
    - session resuming
    - low bandwidth support
    - session brokerage support
    - client-side mass storage mounting support
    - client-side printing support
    - audio support
    - authentication by smartcard and USB stick

X2Go Client is a graphical client (Qt%{qt_version}) for the X2Go system.
You can use it to connect to running sessions and start new sessions.


%prep
%setup -q
# Fix up install issues
sed -i -e 's/-o root -g root//' Makefile
test -f ChangeLog && cp ChangeLog res/txt/changelog || test -f debian/changelog && cp debian/changelog res/txt/changelog || true
test -f ChangeLog.gitlog && cp ChangeLog.gitlog res/txt/git-info || true


%build
%define make_call make %{?_smp_mflags} CXXFLAGS='%{optflags}' QMAKE_OPTS='QMAKE_STRIP=:' QT_VERSION='%{qt_version}'
%if %{qt_version} == 4
%if 0%{?_qt4_bindir:1}
export PATH=%{_qt4_bindir}:$PATH
%endif
%if 0%{?el5}
%{make_call} QMAKE_BINARY='%{_libdir}/qt4/bin/qmake' LRELEASE_BINARY='%{_libdir}/qt4/bin/lrelease'
%else
%if 0%{?suse_version}
%{make_call} QMAKE_BINARY='%{_bindir}/qmake' LRELEASE_BINARY='%{_bindir}/lrelease'
%else
%{make_call}
%endif
%endif
%else
%if %{qt_version} == 5
%{make_call}
%endif
%endif


%install
make install DESTDIR=%{buildroot} PREFIX=%{_prefix}
desktop-file-validate %{buildroot}%{_datadir}/applications/%{name}.desktop

%if 0%{?suse_version}
%suse_update_desktop_file -r x2goclient Utility WebUtility
%fdupes %buildroot
%endif


%post
/bin/touch --no-create %{_datadir}/icons/hicolor &>/dev/null || :
/usr/bin/update-mime-database /usr/share/mime &>/dev/null || :

%postun
if [ $1 -eq 0 ] ; then
    /bin/touch --no-create %{_datadir}/icons/hicolor &>/dev/null
    /usr/bin/gtk-update-icon-cache %{_datadir}/icons/hicolor &>/dev/null || :
fi

%posttrans
/usr/bin/gtk-update-icon-cache %{_datadir}/icons/hicolor &>/dev/null || :


%files
%defattr(-,root,root)
%doc AUTHORS COPYING LICENSE
%{_bindir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/mime/packages/x-x2go.xml
%dir %{_datadir}/icons/hicolor
%dir %{_datadir}/icons/hicolor/128x128
%dir %{_datadir}/icons/hicolor/128x128/apps
%dir %{_datadir}/icons/hicolor/16x16
%dir %{_datadir}/icons/hicolor/16x16/apps
%dir %{_datadir}/icons/hicolor/32x32
%dir %{_datadir}/icons/hicolor/32x32/apps
%dir %{_datadir}/icons/hicolor/64x64
%dir %{_datadir}/icons/hicolor/64x64/apps
%{_datadir}/icons/hicolor/128x128/apps/%{name}.png
%{_datadir}/icons/hicolor/16x16/apps/%{name}.png
%{_datadir}/icons/hicolor/32x32/apps/%{name}.png
%{_datadir}/icons/hicolor/64x64/apps/%{name}.png
%{_datadir}/%{name}/
%{_mandir}/man1/%{name}.1.gz


%changelog
