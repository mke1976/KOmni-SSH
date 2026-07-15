Name:           komni-ssh
Version:        1.0.8
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
* Wed Jul 15 2026 mk <mk@address.com> - 1.0.8-1
- Refine status bar red-dot logic to prevent reappearing on subsequent sheet checks
- Fix KDE notifications by passing app-name and using system-installed icon
- Add Google Sheet check upon bringing program to foreground
- Implement configurable Google Sheet refresh period (1-90 minutes) with translations

* Thu Jul 10 2026 mk <mk@address.com> - 1.0.7-1
- Fix KDE notifications: switch from broken QSystemTrayIcon::showMessage() to
  native D-Bus notifications via notify-send for proper KDE integration
- Fix tray icon red dot logic: icon now correctly persists when any monitored
  PC is online and window is hidden; resets on window show
- Add QProcess include for notify-send invocation

* Thu Jul 09 2026 mk <mk@address.com> - 1.0.6-1
- Implement Enable Autostart setting under Behavior section of Settings menu
- Ensure Autostart conforms to Minimized startup status or last used window size
- Update German, French, and Chinese translations

* Wed Jul 08 2026 mk <mk@address.com> - 1.0.5-1
- Implement Start Minimized toggle setting on Appearance tab
- Implement window size tracking and restoration
- Update translations for German, French, and Chinese

* Fri Jun 26 2026 mk <mk@address.com> - 1.0.3-1
- Fix system language settings detection to support KDE Plasma locale and environment variables fallback
- Fix all compiler warnings (unused variables/parameters, unused slot function)
- Add proper comments and explanations in the code

* Fri Jun 19 2026 mk <mk@address.com> - 1.0.2-1
- Implement tabbed settings menu (Connections, Appearance, Backup)
- Implement dark mode with Light / Dark / System (follow KDE) selection
- Add i18n support: English, German, French, Chinese (follows system locale)

* Mon Jun 15 2026 mk <mk@address.com> - 1.0.1-1
- Remove CB Nickname field from settings
- Remove Settings Share button
- Use standard native KDE settings menu action and icon

* Tue Jun 09 2026 mk <mk@address.com> - 1.0.0-1
- Initial package release
