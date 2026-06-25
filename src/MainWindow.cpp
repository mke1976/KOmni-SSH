#include "MainWindow.h"
#include "SettingsDialog.h"
#include "ConnectionManager.h"
#include "ConfigManager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QToolBar>
#include <QCloseEvent>
#include <QDateTime>
#include <QRegularExpression>
#include <QMenu>
#include <QMessageBox>
#include <QDebug>
#include <QMouseEvent>
#include <QStyle>
#include <QApplication>

// -----------------------------------------------------------------------------
// ComputerCardWidget Implementation
// -----------------------------------------------------------------------------
ComputerCardWidget::ComputerCardWidget(const QString& name, const QString& nickname, Type type, const QString& details, QWidget* parent)
    : QFrame(parent), m_name(name), m_nickname(nickname), m_type(type) {
    
    setObjectName("cardWidget");
    setFrameShape(QFrame::StyledPanel);
    setCursor(Qt::PointingHandCursor);

    // Dynamic Hover stylesheet using palette colors for native compatibility
    setStyleSheet(
        "QFrame#cardWidget {"
        "    background-color: palette(button);"
        "    border: 1px solid palette(mid);"
        "    border-radius: 8px;"
        "    padding: 10px;"
        "}"
        "QFrame#cardWidget:hover {"
        "    background-color: palette(light);"
        "    border: 1px solid palette(highlight);"
        "}"
    );

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 10, 12, 10);

    // Left Layout (Details)
    QVBoxLayout* detailsLayout = new QVBoxLayout();
    detailsLayout->setSpacing(4);

    m_nameLabel = new QLabel(m_name, this);
    m_nameLabel->setStyleSheet("font-size: 15px; font-weight: bold; color: palette(window-text);");
    detailsLayout->addWidget(m_nameLabel);

    QString typeStr;
    switch (m_type) {
        case Ngrok:  typeStr = tr("Ngrok Connection"); break;
        case Vpn:    typeStr = tr("Double VPN Connection"); break;
        case Direct: typeStr = tr("Direct Connection"); break;
    }

    m_typeLabel = new QLabel(QString("%1 | %2").arg(typeStr).arg(details), this);
    m_typeLabel->setStyleSheet("font-size: 11px; color: palette(placeholder-text);");
    detailsLayout->addWidget(m_typeLabel);

    m_statusTextLabel = new QLabel(tr("No status data available"), this);
    m_statusTextLabel->setStyleSheet("font-size: 11px; color: palette(placeholder-text);");
    detailsLayout->addWidget(m_statusTextLabel);

    layout->addLayout(detailsLayout);
    layout->addStretch();

    // Right Layout (Status button/pill)
    m_statusButton = new QPushButton(tr("UNKNOWN"), this);
    m_statusButton->setFixedWidth(90);
    setUnknownStatus();
    layout->addWidget(m_statusButton);

    setupMenu();
}

void ComputerCardWidget::setupMenu() {
    QMenu* menu = new QMenu(m_statusButton);
    
    QAction* termAct = menu->addAction(tr("Terminal (SSH)"));
    connect(termAct, &QAction::triggered, this, [this]() { emit connectRequested("Terminal"); });

    QAction* ffAct = menu->addAction(tr("Firefox (GUI)"));
    connect(ffAct, &QAction::triggered, this, [this]() { emit connectRequested("Firefox"); });

    QAction* dolAct = menu->addAction(tr("Dolphin (GUI)"));
    connect(dolAct, &QAction::triggered, this, [this]() { emit connectRequested("Dolphin"); });

    QAction* kwAct = menu->addAction(tr("KWrite (GUI)"));
    connect(kwAct, &QAction::triggered, this, [this]() { emit connectRequested("KWrite"); });

    m_statusButton->setMenu(menu);
    // Make it show the menu instantly on click
    connect(m_statusButton, &QPushButton::clicked, m_statusButton, &QPushButton::showMenu);
}

void ComputerCardWidget::updateStatus(bool online, qint64 timestamp) {
    QString timeStr = QDateTime::fromSecsSinceEpoch(timestamp).toString("yyyy-MM-dd hh:mm:ss");
    if (online) {
        m_statusTextLabel->setText(tr("Online | Updated: %1").arg(timeStr));
        m_statusTextLabel->setStyleSheet("font-size: 11px; color: #27ae60; font-weight: bold;");
        m_statusButton->setText(tr("ONLINE"));
        m_statusButton->setStyleSheet(
            "QPushButton {"
            "    background-color: #2ecc71;"
            "    color: white;"
            "    border-radius: 14px;"
            "    font-weight: bold;"
            "    border: none;"
            "    font-size: 11px;"
            "    padding: 6px;"
            "}"
            "QPushButton::menu-indicator { image: none; }"
        );
    } else {
        m_statusTextLabel->setText(tr("Offline | Last seen: %1").arg(timeStr));
        m_statusTextLabel->setStyleSheet("font-size: 11px; color: #c0392b;");
        m_statusButton->setText(tr("OFFLINE"));
        m_statusButton->setStyleSheet(
            "QPushButton {"
            "    background-color: #e74c3c;"
            "    color: white;"
            "    border-radius: 14px;"
            "    font-weight: bold;"
            "    border: none;"
            "    font-size: 11px;"
            "    padding: 6px;"
            "}"
            "QPushButton::menu-indicator { image: none; }"
        );
    }
}

void ComputerCardWidget::setUnknownStatus() {
    m_statusTextLabel->setText(tr("No status data available"));
    m_statusTextLabel->setStyleSheet("font-size: 11px; color: palette(placeholder-text);");
    m_statusButton->setText(tr("UNKNOWN"));
    m_statusButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #95a5a6;"
        "    color: white;"
        "    border-radius: 14px;"
        "    font-weight: bold;"
        "    border: none;"
        "    font-size: 11px;"
        "    padding: 6px;"
        "}"
        "QPushButton::menu-indicator { image: none; }"
    );
}

// -----------------------------------------------------------------------------
// MainWindow Implementation
// -----------------------------------------------------------------------------
MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(tr("KOmni-SSH status"));
    resize(500, 600);
    setWindowIcon(QIcon(":/icons/app_icon.png"));

    ConfigManager::instance().load();

    setupUi();
    setupTrayIcon();
    rebuildCards();
    startPolling();
}

void MainWindow::setupUi() {
    QMenuBar* m_menuBar = menuBar();
    QMenu* settingsMenu = m_menuBar->addMenu(tr("Settings"));
    QAction* configureAct = new QAction(QIcon::fromTheme("configure", style()->standardIcon(QStyle::SP_FileDialogListView)), tr("Configure KOmni-SSH..."), this);
    connect(configureAct, &QAction::triggered, this, &MainWindow::onSettingsTriggered);
    settingsMenu->addAction(configureAct);

    QWidget* central = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // Toolbar Header (Refresh, Settings, Exit)
    QToolBar* toolbar = new QToolBar(this);
    toolbar->setIconSize(QSize(20, 20));
    toolbar->setMovable(false);
    toolbar->setStyleSheet("QToolBar { background: transparent; border: none; }");
    
    QAction* refreshAct = new QAction(style()->standardIcon(QStyle::SP_BrowserReload), tr("Refresh"), this);
    connect(refreshAct, &QAction::triggered, this, &MainWindow::onRefreshTriggered);
    toolbar->addAction(refreshAct);

    QAction* settingsAct = new QAction(QIcon::fromTheme("configure", style()->standardIcon(QStyle::SP_FileDialogListView)), tr("Configure KOmni-SSH..."), this);
    connect(settingsAct, &QAction::triggered, this, &MainWindow::onSettingsTriggered);
    toolbar->addAction(settingsAct);

    toolbar->addSeparator();

    QAction* quitAct = new QAction(style()->standardIcon(QStyle::SP_DialogCloseButton), tr("Quit"), this);
    connect(quitAct, &QAction::triggered, qApp, &QApplication::quit);
    toolbar->addAction(quitAct);

    mainLayout->addWidget(toolbar);

    // Scroll Area for Status Cards
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFrameShape(QFrame::NoFrame);

    m_scrollWidget = new QWidget(m_scrollArea);
    m_cardsLayout = new QVBoxLayout(m_scrollWidget);
    m_cardsLayout->setContentsMargins(0, 0, 0, 0);
    m_cardsLayout->setSpacing(10);
    
    m_scrollWidget->setLayout(m_cardsLayout);
    m_scrollArea->setWidget(m_scrollWidget);
    mainLayout->addWidget(m_scrollArea);

    // Status footer (Last updated)
    m_lastRefreshedLabel = new QLabel(tr("Last Refresh: Never"), this);
    m_lastRefreshedLabel->setStyleSheet("font-size: 10px; color: palette(placeholder-text); padding: 2px;");
    mainLayout->addWidget(m_lastRefreshedLabel);

    setCentralWidget(central);
}

void MainWindow::setupTrayIcon() {
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(QIcon(":/icons/app_icon.png"));
    m_trayIcon->setToolTip(tr("KOmni-SSH Status Monitor"));

    QMenu* trayMenu = new QMenu(this);
    QAction* showAct = trayMenu->addAction(tr("Show MainWindow"));
    connect(showAct, &QAction::triggered, this, &MainWindow::showNormal);

    QAction* refreshAct = trayMenu->addAction(tr("Refresh Status"));
    connect(refreshAct, &QAction::triggered, this, &MainWindow::onRefreshTriggered);

    QAction* settingsAct = new QAction(QIcon::fromTheme("configure", style()->standardIcon(QStyle::SP_FileDialogListView)), tr("Configure KOmni-SSH..."), this);
    connect(settingsAct, &QAction::triggered, this, &MainWindow::onSettingsTriggered);
    trayMenu->addAction(settingsAct);

    trayMenu->addSeparator();
    QAction* quitAct = trayMenu->addAction(tr("Quit"));
    connect(quitAct, &QAction::triggered, qApp, &QApplication::quit);

    m_trayIcon->setContextMenu(trayMenu);
    m_trayIcon->show();

    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayIconActivated);
}

void MainWindow::rebuildCards() {
    // Clean up old cards
    qDeleteAll(m_cardWidgets);
    m_cardWidgets.clear();

    const auto& config = ConfigManager::instance();

    // Ngrok PCs
    for (int i = 0; i < 5; ++i) {
        const auto& pc = config.ngrokPcs[i];
        if (pc.enabled) {
            ComputerCardWidget* card = new ComputerCardWidget(pc.name, pc.nickname, ComputerCardWidget::Ngrok, tr("Ngrok"), m_scrollWidget);
            m_cardsLayout->addWidget(card);
            m_cardWidgets.append(card);
            connect(card, &ComputerCardWidget::connectRequested, this, [this, card, pc](const QString& app) {
                ConnectionManager::instance().connectNgrok(pc, app, this);
            });
        }
    }

    // VPN PCs
    for (int i = 0; i < 5; ++i) {
        const auto& pc = config.vpnPcs[i];
        if (pc.enabled) {
            ComputerCardWidget* card = new ComputerCardWidget(pc.name, pc.nickname, ComputerCardWidget::Vpn, QString("%1 -> %2").arg(pc.bridge).arg(pc.priv), m_scrollWidget);
            m_cardsLayout->addWidget(card);
            m_cardWidgets.append(card);
            connect(card, &ComputerCardWidget::connectRequested, this, [this, card, pc](const QString& app) {
                ConnectionManager::instance().connectVpn(pc, app, this);
            });
        }
    }

    // Direct PCs
    for (int i = 0; i < 5; ++i) {
        const auto& pc = config.directPcs[i];
        if (pc.enabled) {
            ComputerCardWidget* card = new ComputerCardWidget(pc.name, pc.nickname, ComputerCardWidget::Direct, QString("%1:%2").arg(pc.host).arg(pc.port), m_scrollWidget);
            m_cardsLayout->addWidget(card);
            m_cardWidgets.append(card);
            connect(card, &ComputerCardWidget::connectRequested, this, [this, card, pc](const QString& app) {
                ConnectionManager::instance().connectDirect(pc, app, this);
            });
        }
    }

    // Empty state placeholder
    if (m_cardWidgets.isEmpty()) {
        QLabel* emptyLabel = new QLabel(tr("No computers enabled. Go to Settings to configure."), m_scrollWidget);
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setStyleSheet("color: palette(placeholder-text); padding: 50px; font-style: italic;");
        m_cardsLayout->addWidget(emptyLabel);
    } else {
        m_cardsLayout->addStretch();
    }

    // Trigger immediate refresh on change
    onRefreshTriggered();
}

void MainWindow::startPolling() {
    m_refreshTimer = new QTimer(this);
    // Refresh every 10 minutes (600,000 milliseconds)
    connect(m_refreshTimer, &QTimer::timeout, this, &MainWindow::onRefreshTriggered);
    m_refreshTimer->start(600000);
}

void MainWindow::onRefreshTriggered() {
    QString urlStr = ConfigManager::instance().sheetCsvUrl;
    if (urlStr.isEmpty()) {
        m_lastRefreshedLabel->setText(tr("Last Refresh: Fail (No Sheets URL set)"));
        return;
    }

    m_lastRefreshedLabel->setText(tr("Refreshing remote computer status..."));
    QUrl url(urlStr);
    QNetworkRequest request(url);
    QNetworkReply* reply = m_networkManager.get(request);
    connect(reply, &QNetworkReply::finished, this, &MainWindow::handleCsvReply);
}

void MainWindow::handleCsvReply() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    if (reply->error() != QNetworkReply::NoError) {
        m_lastRefreshedLabel->setText(tr("Last Refresh: Fail (%1)").arg(reply->errorString()));
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    parseCsvData(data);
    checkStatusChanges();

    QString timeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    m_lastRefreshedLabel->setText(tr("Last Refresh: %1 (Success)").arg(timeStr));
}

void MainWindow::parseCsvData(const QByteArray& data) {
    m_currentStatuses.clear();
    QString csvText = QString::fromUtf8(data);
    // Split lines robustly supporting Unix and Windows newlines
    QStringList lines = csvText.split(QRegularExpression("[\r\n]+"));
    for (const QString& line : lines) {
        if (line.trimmed().isEmpty()) continue;
        
        QStringList cols = line.split(',');
        if (cols.size() < 2) continue;

        QString name = cols.first().trimmed();
        // Strip outer quotes if any
        if (name.startsWith('"') && name.endsWith('"') && name.length() >= 2) {
            name = name.mid(1, name.length() - 2);
        }
        if (name == "PC Name") continue;

        QString tsStr = cols.last().trimmed();
        if (tsStr.startsWith('"') && tsStr.endsWith('"') && tsStr.length() >= 2) {
            tsStr = tsStr.mid(1, tsStr.length() - 2);
        }

        bool ok;
        qint64 ts = tsStr.toLongLong(&ok);
        if (ok) {
            m_currentStatuses[name] = ts;
        }
    }
}

void MainWindow::checkStatusChanges() {
    qint64 currentTs = QDateTime::currentSecsSinceEpoch();

    for (ComputerCardWidget* card : m_cardWidgets) {
        QString nick = card->nickname();
        if (m_currentStatuses.contains(nick)) {
            qint64 ts = m_currentStatuses[nick];
            // 12-minute threshold for online status
            bool isOnline = (currentTs - ts <= 720);
            card->updateStatus(isOnline, ts);

            QString currentStateStr = isOnline ? "online" : "offline";
            QString prevStateStr = m_prevStates.value(nick, "unknown");

            // Transitions to online trigger notification if enabled
            if (currentStateStr == "online" && prevStateStr != "online") {
                bool isMonitored = false;
                const auto& config = ConfigManager::instance();

                if (card->cardType() == ComputerCardWidget::Ngrok) {
                    for (const auto& pc : config.ngrokPcs) {
                        if (pc.nickname == nick && pc.enabled) isMonitored = pc.monitor;
                    }
                } else if (card->cardType() == ComputerCardWidget::Vpn) {
                    for (const auto& pc : config.vpnPcs) {
                        if (pc.nickname == nick && pc.enabled) isMonitored = pc.monitor;
                    }
                } else if (card->cardType() == ComputerCardWidget::Direct) {
                    for (const auto& pc : config.directPcs) {
                        if (pc.nickname == nick && pc.enabled) isMonitored = pc.monitor;
                    }
                }

                if (isMonitored && !m_firstRun) {
                    m_trayIcon->showMessage(
                        tr("Computer Online"),
                        tr("%1 is now ONLINE").arg(card->name()),
                        m_trayIcon->icon(), // use custom ic_launcher-playstore app icon
                        7000
                    );
                }
            }
            m_prevStates[nick] = currentStateStr;
        } else {
            card->setUnknownStatus();
            m_prevStates[nick] = "unknown";
        }
    }

    m_firstRun = false;
}

void MainWindow::onSettingsTriggered() {
    SettingsDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        rebuildCards();
    }
}

void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::DoubleClick || reason == QSystemTrayIcon::Trigger) {
        if (isVisible()) {
            hide();
        } else {
            showNormal();
            activateWindow();
        }
    }
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (m_trayIcon->isVisible()) {
        hide();
        event->ignore();
    } else {
        event->accept();
    }
}

void MainWindow::changeEvent(QEvent* event) {
    if (event->type() == QEvent::WindowStateChange) {
        if (isMinimized()) {
            // Give window a moment to minimize anim before hiding to tray
            QTimer::singleShot(250, this, &QWidget::hide);
        }
    }
    QMainWindow::changeEvent(event);
}
