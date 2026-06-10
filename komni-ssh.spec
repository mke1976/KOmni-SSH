Name:           komni-ssh
Version:        1.0.0
Release:        1%{?dist}
Summary:        KDE native SSH connection and remote status monitor

License:        GPLv3+
URL:            https://github.com/mk/komni-ssh
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  gcc-c++
BuildRequires:  cmake
BuildRequires:  qt6-qtbase-devel
BuildRequires:  desktop-file-utils

Requires:       qt6-qtbase
Requires:       waypipe
Requires:       sshpass
Requires:       openssh-clients
Requires:       konsole
Requires:       NetworkManager

%description
KOmni-SSH is a native KDE application written in C++ and Qt 6.
It displays the online/offline status of remote computers via a status sheet,
runs in the system tray in the background, sends notifications when monitored
computers come online, and launches applications (Terminal, Firefox, Dolphin,
KWrite) over SSH using double VPN sequences (via NetworkManager) or Ngrok tunnels.

%prep
%autosetup

%build
%cmake
%cmake_build

%install
%cmake_install

%check
desktop-file-validate %{buildroot}%{_datadir}/applications/%{name}.desktop

%files
%{_bindir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/pixmaps/%{name}.png

%changelog
* Tue Jun 09 2026 mk <mk@address.com> - 1.0.0-1
- Initial package release
