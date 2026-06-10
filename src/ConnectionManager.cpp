#include "ConnectionManager.h"
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QTimer>
#include <QDebug>
#include <QProcess>

ConnectionManager& ConnectionManager::instance() {
    static ConnectionManager inst;
    return inst;
}

ConnectionManager::ConnectionManager() : QObject(nullptr) {
}

void ConnectionManager::connectNgrok(const NgrokConfig& pc, const QString& appChoice, QWidget* parent) {
    if (pc.api.isEmpty()) {
        QMessageBox::warning(parent, "Ngrok Error", "Ngrok API Key is empty!");
        return;
    }

    QProgressDialog* progress = new QProgressDialog("Querying Ngrok API for tunnels...", "Cancel", 0, 0, parent);
    progress->setWindowTitle("Ngrok Resolution");
    progress->setWindowModality(Qt::WindowModal);
    progress->show();

    QUrl url("https://api.ngrok.com/tunnels");
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", QString("Bearer %1").arg(pc.api).toUtf8());
    request.setRawHeader("ngrok-version", "2");

    QNetworkReply* reply = m_networkManager.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, pc, appChoice, progress, parent]() {
        progress->close();
        progress->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            QMessageBox::critical(parent, "Ngrok Error", QString("Failed to query Ngrok API:\n%1").arg(reply->errorString()));
            reply->deleteLater();
            return;
        }

        QByteArray data = reply->readAll();
        reply->deleteLater();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isNull() || !doc.isObject()) {
            QMessageBox::critical(parent, "Ngrok Error", "Invalid JSON response from Ngrok API.");
            return;
        }

        QJsonObject obj = doc.object();
        QJsonArray tunnels = obj["tunnels"].toArray();
        QString publicUrl;
        for (const QJsonValue& val : tunnels) {
            QJsonObject tunnel = val.toObject();
            QString pubUrl = tunnel["public_url"].toString();
            if (pubUrl.startsWith("tcp://")) {
                publicUrl = pubUrl;
                break;
            }
        }

        if (publicUrl.isEmpty()) {
            QMessageBox::critical(parent, "Ngrok Error", "No active TCP tunnels found in your Ngrok account.");
            return;
        }

        // Parse tcp://host:port
        QString addr = publicUrl.mid(6); // remove tcp://
        int colonIdx = addr.lastIndexOf(':');
        if (colonIdx == -1) {
            QMessageBox::critical(parent, "Ngrok Error", QString("Invalid tunnel URL: %1").arg(publicUrl));
            return;
        }

        QString host = addr.left(colonIdx);
        QString port = addr.mid(colonIdx + 1);

        QStringList sshOpts;
        sshOpts << "-o" << "StrictHostKeyChecking=no"
                << "-o" << "UserKnownHostsFile=/dev/null"
                << "-p" << port;

        launchSsh(QString("%1@%2").arg(pc.user).arg(host), sshOpts, appChoice, false);
    });
}

void ConnectionManager::connectDirect(const DirectConfig& pc, const QString& appChoice, QWidget* parent) {
    if (pc.host.isEmpty()) {
        QMessageBox::warning(parent, "Direct SSH Error", "Host address is empty!");
        return;
    }

    QStringList sshOpts;
    sshOpts << "-p" << pc.port
            << "-o" << "StrictHostKeyChecking=no";

    launchSsh(QString("%1@%2").arg(pc.user).arg(pc.host), sshOpts, appChoice, false);
}

void ConnectionManager::connectVpn(const VpnConfig& pc, const QString& appChoice, QWidget* parent) {
    VpnState* state = new VpnState;
    state->pc = pc;
    state->appChoice = appChoice;
    state->progress = new QProgressDialog("Initializing Double VPN...", "Cancel", 0, 100, parent);
    state->progress->setWindowTitle("Double VPN Sequence");
    state->progress->setWindowModality(Qt::WindowModal);
    state->progress->setValue(10);
    state->progress->show();

    connect(state->progress, &QProgressDialog::canceled, this, [this, state]() {
        // Only run if not already cleaned up by the success path
        cleanupVpnState(state, false);
    });

    checkIpv6(state);
}

void ConnectionManager::checkIpv6(VpnState* state) {
    if (!state->progress || state->progress->wasCanceled()) return;

    state->progress->setLabelText("Checking IPv6 connectivity...");
    state->progress->setValue(15);

    state->proc = new QProcess(this);
    state->proc->start("ping6", QStringList() << "-c" << "1" << "-W" << "1" << "2001:4860:4860::8888");
    connect(state->proc, &QProcess::finished, this, [this, state](int exitCode) {
        state->proc->deleteLater();
        state->proc = nullptr;

        if (exitCode == 0) {
            state->hasNativeIpv6 = true;
            if (state->progress) {
                state->progress->setLabelText("Native IPv6 detected! Skipping Bridge...");
                state->progress->setValue(30);
            }
            state->attempt = 1;
            connectPrivate(state);
        } else {
            state->hasNativeIpv6 = false;
            state->attempt = 1;
            connectBridge(state);
        }
    });
}

void ConnectionManager::connectBridge(VpnState* state) {
    if (!state->progress || state->progress->wasCanceled()) return;

    if (state->attempt > 3) {
        QMessageBox::critical(state->progress->parentWidget(), "VPN Error", 
                              QString("Failed to connect Bridge: %1 after 3 attempts.").arg(state->pc.bridge));
        cleanupVpnState(state, false);
        return;
    }

    state->progress->setLabelText(QString("Connecting Bridge: %1 (Attempt %2/3)...").arg(state->pc.bridge).arg(state->attempt));
    state->progress->setValue(30 + (state->attempt - 1) * 5);

    state->proc = new QProcess(this);
    state->proc->start("nmcli", QStringList() << "connection" << "up" << state->pc.bridge);
    connect(state->proc, &QProcess::finished, this, [this, state](int exitCode) {
        state->proc->deleteLater();
        state->proc = nullptr;

        // Verify bridge connection
        QProcess* pingProc = new QProcess(this);
        pingProc->start("ping6", QStringList() << "-c" << "1" << "-W" << "1" << "2001:4860:4860::8888");
        connect(pingProc, &QProcess::finished, this, [this, state, pingProc](int pingExitCode) {
            pingProc->deleteLater();
            if (pingExitCode == 0) {
                if (state->progress) {
                    state->progress->setLabelText("Bridge Connected!");
                    state->progress->setValue(45);
                }
                state->attempt = 1;
                connectPrivate(state);
            } else {
                state->attempt++;
                connectBridge(state);
            }
        });
    });
}

void ConnectionManager::connectPrivate(VpnState* state) {
    if (!state->progress || state->progress->wasCanceled()) return;

    if (state->attempt > 3) {
        QMessageBox::critical(state->progress->parentWidget(), "VPN Error", 
                              QString("Failed to connect Private VPN: %1 after 3 attempts.").arg(state->pc.priv));
        cleanupVpnState(state, false);
        return;
    }

    state->progress->setLabelText(QString("Connecting Private: %1 (Attempt %2/3)...").arg(state->pc.priv).arg(state->attempt));
    state->progress->setValue(50 + (state->attempt - 1) * 15);

    state->proc = new QProcess(this);
    state->proc->start("nmcli", QStringList() << "connection" << "up" << state->pc.priv);
    connect(state->proc, &QProcess::finished, this, [this, state](int exitCode) {
        state->proc->deleteLater();
        state->proc = nullptr;

        // Verify connection by pinging FritzBox (192.168.2.1)
        QProcess* pingProc = new QProcess(this);
        pingProc->start("ping", QStringList() << "-c" << "1" << "-W" << "1" << "192.168.2.1");
        connect(pingProc, &QProcess::finished, this, [this, state, pingProc](int pingExitCode) {
            pingProc->deleteLater();
            if (pingExitCode == 0) {
                if (state->progress) {
                    state->progress->setLabelText("Private VPN Connected! Tunnel Active!");
                    state->progress->setValue(100);
                }
                
                QTimer::singleShot(500, this, [this, state]() {
                    if (state->progress) {
                        // Disconnect 'canceled' BEFORE close() to prevent double-cleanup.
                        // QProgressDialog::close() can emit canceled on some Qt builds,
                        // which would trigger cleanupVpnState a second time and corrupt the heap.
                        disconnect(state->progress, &QProgressDialog::canceled, nullptr, nullptr);
                        state->progress->close();
                    }

                    QStringList sshOpts;
                    sshOpts << "-o" << "KexAlgorithms=curve25519-sha256";

                    QString target = QString("%1@%2").arg(state->pc.user).arg(state->pc.ip);
                    launchSsh(target, sshOpts, state->appChoice, true, state->hasNativeIpv6 ? "" : state->pc.bridge, state->pc.priv);
                    cleanupVpnState(state, true);
                });
            } else {
                state->attempt++;
                connectPrivate(state);
            }
        });
    });
}

void ConnectionManager::cleanupVpnState(VpnState* state, bool success) {
    // Guard: QProgressDialog::canceled can race with the success-path timer.
    // The 'cleaned' flag makes this function idempotent.
    if (state->cleaned) return;
    state->cleaned = true;

    if (state->proc) {
        state->proc->kill();
        state->proc->deleteLater();
        state->proc = nullptr;
    }
    if (!success) {
        // Tears down partially set up connections
        if (!state->hasNativeIpv6) {
            QProcess::execute("nmcli", QStringList() << "connection" << "down" << state->pc.bridge);
        }
        QProcess::execute("nmcli", QStringList() << "connection" << "down" << state->pc.priv);
    }
    // QPointer: safe even if dialog was already destroyed externally
    if (state->progress) {
        state->progress->deleteLater();
        state->progress = nullptr;
    }
    delete state;
}

void ConnectionManager::launchSsh(const QString& target, const QStringList& sshOpts, const QString& appChoice, 
                                  bool isVpn, const QString& bridgeConn, const QString& privConn) {
    QString globalPass = ConfigManager::instance().globalSshPass;
    bool useSshPass = !globalPass.isEmpty();

    QString program;
    QStringList args;

    if (appChoice == "Terminal") {
        program = "konsole";
        args << "--nofork" << "-e";
        if (useSshPass) {
            args << "bash" << "-c" << QString("sshpass -P pass -p '%1' ssh %2 %3").arg(globalPass).arg(sshOpts.join(" ")).arg(target);
        } else {
            args << "ssh";
            args.append(sshOpts);
            args << target;
        }
    } else {
        // GUI Apps: Firefox, Dolphin, KWrite
        // Format: [sshpass -P pass -p <pass>] waypipe -c lz4 --no-gpu ssh <opts> [port-forwarding-for-pulse-if-firefox] <target> <command>
        int cmdStart = 0;
        if (useSshPass) {
            program = "sshpass";
            args << "-P" << "pass" << "-p" << globalPass;
            args << "waypipe" << "-c" << "lz4" << "--no-gpu" << "ssh";
        } else {
            program = "waypipe";
            args << "-c" << "lz4" << "--no-gpu" << "ssh";
        }

        args.append(sshOpts);

        if (appChoice == "Firefox") {
            // Pulseaudio forward for Firefox
            args << "-R" << "/tmp/pulse.sock:/run/user/1000/pulse/native" << target
                 << "env" << "PULSE_SERVER=unix:/tmp/pulse.sock" << "firefox";
        } else if (appChoice == "Dolphin") {
            args << target << "dolphin";
        } else if (appChoice == "KWrite") {
            args << target << "kwrite";
        }
    }

    qDebug() << "Launching:" << program << args;

    QProcess* proc = new QProcess(this);
    proc->start(program, args);

    // If it's a VPN, we must disconnect when the application finishes.
    if (isVpn) {
        connect(proc, &QProcess::finished, this, [bridgeConn, privConn](int exitCode) {
            qDebug() << "Application process finished. Tearing down VPN tunnels...";
            QProcess::execute("nmcli", QStringList() << "connection" << "down" << privConn);
            if (!bridgeConn.isEmpty()) {
                QProcess::execute("nmcli", QStringList() << "connection" << "down" << bridgeConn);
            }
        });
    }

    // Always delete QProcess memory on finish
    connect(proc, &QProcess::finished, proc, &QObject::deleteLater);
}
