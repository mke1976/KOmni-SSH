#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QProgressDialog>
#include <QProcess>
#include "ConfigManager.h"

class ConnectionManager : public QObject {
    Q_OBJECT
public:
    static ConnectionManager& instance();

    void connectNgrok(const NgrokConfig& pc, const QString& appChoice, QWidget* parent);
    void connectVpn(const VpnConfig& pc, const QString& appChoice, QWidget* parent);
    void connectDirect(const DirectConfig& pc, const QString& appChoice, QWidget* parent);

private:
    ConnectionManager();
    ~ConnectionManager() = default;

    QNetworkAccessManager m_networkManager;

    void launchSsh(const QString& target, const QStringList& sshOpts, const QString& appChoice, 
                   bool isVpn, const QString& bridgeConn = "", const QString& privConn = "");

    // VPN Sequence State
    struct VpnState {
        VpnConfig pc;
        QString appChoice;
        QProgressDialog* progress = nullptr;
        int step = 0;
        bool hasNativeIpv6 = false;
        int attempt = 1;
        QProcess* proc = nullptr;
    };

    void startVpnSequence(VpnState* state);
    void checkIpv6(VpnState* state);
    void connectBridge(VpnState* state);
    void connectPrivate(VpnState* state);
    void cleanupVpnState(VpnState* state, bool success);
};
