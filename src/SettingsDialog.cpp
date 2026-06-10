#include "SettingsDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QGroupBox>
#include <QLabel>
#include <QFormLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle("Settings");
    resize(550, 750);
    setupUi();
    loadSettings();
}

void SettingsDialog::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // Title / Header
    QLabel* titleLabel = new QLabel("Settings", this);
    titleLabel->setStyleSheet("font-size: 20px; font-weight: bold; padding: 5px;");
    mainLayout->addWidget(titleLabel);

    // Scroll Area
    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    QWidget* scrollWidget = new QWidget(scrollArea);
    QVBoxLayout* scrollLayout = new QVBoxLayout(scrollWidget);
    scrollLayout->setContentsMargins(0, 0, 0, 0);
    scrollLayout->setSpacing(15);

    // Add sections
    scrollLayout->addWidget(createGlobalSection());
    scrollLayout->addWidget(createNgrokSection());
    scrollLayout->addWidget(createVpnSection());
    scrollLayout->addWidget(createDirectSection());
    scrollLayout->addWidget(createImportExportSection());

    scrollWidget->setLayout(scrollLayout);
    scrollArea->setWidget(scrollWidget);
    mainLayout->addWidget(scrollArea);

    // Bottom Buttons (Cancel, Save)
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    QPushButton* cancelBtn = new QPushButton("Cancel", this);
    cancelBtn->setStyleSheet("padding: 8px 16px; border-radius: 4px;");
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(cancelBtn);

    QPushButton* saveBtn = new QPushButton("Save", this);
    saveBtn->setStyleSheet("padding: 8px 16px; border-radius: 4px; background-color: palette(highlight); color: palette(highlighted-text); font-weight: bold;");
    connect(saveBtn, &QPushButton::clicked, this, &SettingsDialog::onSaveClicked);
    buttonLayout->addWidget(saveBtn);

    mainLayout->addLayout(buttonLayout);
    setLayout(mainLayout);
}

QWidget* SettingsDialog::createGlobalSection() {
    QGroupBox* box = new QGroupBox("General Settings");
    QFormLayout* form = new QFormLayout(box);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    m_csvUrlEdit = new QLineEdit(box);
    m_csvUrlEdit->setToolTip("Google Sheet URL in CSV format");
    form->addRow("Google Sheet URL:", m_csvUrlEdit);

    m_sshPassEdit = new QLineEdit(box);
    m_sshPassEdit->setEchoMode(QLineEdit::Password);
    m_sshPassEdit->setToolTip("Global SSH password if sshpass is used");
    form->addRow("Global: Key Passphrase:", m_sshPassEdit);

    return box;
}

QWidget* SettingsDialog::createNgrokSection() {
    QGroupBox* mainBox = new QGroupBox("SSH Ngrok Settings");
    QVBoxLayout* layout = new QVBoxLayout(mainBox);
    layout->setSpacing(10);

    m_ngrokUi.resize(5);
    for (int i = 0; i < 5; ++i) {
        QGroupBox* pcBox = new QGroupBox(QString("Computer %1").arg(i + 1), mainBox);
        pcBox->setFlat(true);
        QVBoxLayout* pcLayout = new QVBoxLayout(pcBox);
        pcLayout->setContentsMargins(5, 5, 5, 5);

        // Header with Enable toggle
        QHBoxLayout* header = new QHBoxLayout();
        QCheckBox* enableCheck = new QCheckBox("Enable Computer", pcBox);
        enableCheck->setStyleSheet("font-weight: bold;");
        header->addWidget(enableCheck);
        header->addStretch();
        pcLayout->addLayout(header);

        // Content Widget (collapsible/de-activatable)
        QWidget* detailsWidget = new QWidget(pcBox);
        QFormLayout* form = new QFormLayout(detailsWidget);
        form->setContentsMargins(10, 5, 5, 5);

        QLineEdit* nameEdit = new QLineEdit(detailsWidget);
        form->addRow("Computer Name:", nameEdit);

        QLineEdit* nickEdit = new QLineEdit(detailsWidget);
        form->addRow("CB Nickname:", nickEdit);

        QLineEdit* userEdit = new QLineEdit(detailsWidget);
        form->addRow("User:", userEdit);

        QLineEdit* apiEdit = new QLineEdit(detailsWidget);
        form->addRow("Ngrok API Key:", apiEdit);

        QCheckBox* monCheck = new QCheckBox("Online monitor", detailsWidget);
        form->addRow("", monCheck);

        pcLayout->addWidget(detailsWidget);
        layout->addWidget(pcBox);

        // Connect enable checkbox to enable/disable details
        connect(enableCheck, &QCheckBox::toggled, detailsWidget, &QWidget::setEnabled);

        // Track UI elements
        m_ngrokUi[i].enabledCheck = enableCheck;
        m_ngrokUi[i].nameEdit = nameEdit;
        m_ngrokUi[i].nickEdit = nickEdit;
        m_ngrokUi[i].userEdit = userEdit;
        m_ngrokUi[i].apiEdit = apiEdit;
        m_ngrokUi[i].monCheck = monCheck;
        m_ngrokUi[i].container = detailsWidget;
    }

    return mainBox;
}

QWidget* SettingsDialog::createVpnSection() {
    QGroupBox* mainBox = new QGroupBox("SSH IPv6+Wireguard (VPN) Settings");
    QVBoxLayout* layout = new QVBoxLayout(mainBox);
    layout->setSpacing(10);

    m_vpnUi.resize(5);
    for (int i = 0; i < 5; ++i) {
        QGroupBox* pcBox = new QGroupBox(QString("Computer %1").arg(i + 1), mainBox);
        pcBox->setFlat(true);
        QVBoxLayout* pcLayout = new QVBoxLayout(pcBox);
        pcLayout->setContentsMargins(5, 5, 5, 5);

        QHBoxLayout* header = new QHBoxLayout();
        QCheckBox* enableCheck = new QCheckBox("Enable Computer", pcBox);
        enableCheck->setStyleSheet("font-weight: bold;");
        header->addWidget(enableCheck);
        header->addStretch();
        pcLayout->addLayout(header);

        QWidget* detailsWidget = new QWidget(pcBox);
        QFormLayout* form = new QFormLayout(detailsWidget);
        form->setContentsMargins(10, 5, 5, 5);

        QLineEdit* nameEdit = new QLineEdit(detailsWidget);
        form->addRow("Computer Name:", nameEdit);

        QLineEdit* nickEdit = new QLineEdit(detailsWidget);
        form->addRow("CB Nickname:", nickEdit);

        QLineEdit* userEdit = new QLineEdit(detailsWidget);
        form->addRow("User:", userEdit);

        QLineEdit* ipEdit = new QLineEdit(detailsWidget);
        form->addRow("IP Address:", ipEdit);

        QLineEdit* bridgeEdit = new QLineEdit(detailsWidget);
        form->addRow("Bridge Connection (WARP):", bridgeEdit);

        QLineEdit* privEdit = new QLineEdit(detailsWidget);
        form->addRow("Private Connection (fritz):", privEdit);

        QCheckBox* monCheck = new QCheckBox("Online monitor", detailsWidget);
        form->addRow("", monCheck);

        pcLayout->addWidget(detailsWidget);
        layout->addWidget(pcBox);

        connect(enableCheck, &QCheckBox::toggled, detailsWidget, &QWidget::setEnabled);

        m_vpnUi[i].enabledCheck = enableCheck;
        m_vpnUi[i].nameEdit = nameEdit;
        m_vpnUi[i].nickEdit = nickEdit;
        m_vpnUi[i].userEdit = userEdit;
        m_vpnUi[i].ipEdit = ipEdit;
        m_vpnUi[i].bridgeEdit = bridgeEdit;
        m_vpnUi[i].privEdit = privEdit;
        m_vpnUi[i].monCheck = monCheck;
        m_vpnUi[i].container = detailsWidget;
    }

    return mainBox;
}

QWidget* SettingsDialog::createDirectSection() {
    QGroupBox* mainBox = new QGroupBox("SSH IPv4 (Direct) Settings");
    QVBoxLayout* layout = new QVBoxLayout(mainBox);
    layout->setSpacing(10);

    m_directUi.resize(5);
    for (int i = 0; i < 5; ++i) {
        QGroupBox* pcBox = new QGroupBox(QString("Computer %1").arg(i + 1), mainBox);
        pcBox->setFlat(true);
        QVBoxLayout* pcLayout = new QVBoxLayout(pcBox);
        pcLayout->setContentsMargins(5, 5, 5, 5);

        QHBoxLayout* header = new QHBoxLayout();
        QCheckBox* enableCheck = new QCheckBox("Enable Computer", pcBox);
        enableCheck->setStyleSheet("font-weight: bold;");
        header->addWidget(enableCheck);
        header->addStretch();
        pcLayout->addLayout(header);

        QWidget* detailsWidget = new QWidget(pcBox);
        QFormLayout* form = new QFormLayout(detailsWidget);
        form->setContentsMargins(10, 5, 5, 5);

        QLineEdit* nameEdit = new QLineEdit(detailsWidget);
        form->addRow("Computer Name:", nameEdit);

        QLineEdit* nickEdit = new QLineEdit(detailsWidget);
        form->addRow("CB Nickname:", nickEdit);

        QLineEdit* userEdit = new QLineEdit(detailsWidget);
        form->addRow("User:", userEdit);

        QLineEdit* hostEdit = new QLineEdit(detailsWidget);
        form->addRow("Host / Address:", hostEdit);

        QLineEdit* portEdit = new QLineEdit(detailsWidget);
        form->addRow("Port:", portEdit);

        QCheckBox* monCheck = new QCheckBox("Online monitor", detailsWidget);
        form->addRow("", monCheck);

        pcLayout->addWidget(detailsWidget);
        layout->addWidget(pcBox);

        connect(enableCheck, &QCheckBox::toggled, detailsWidget, &QWidget::setEnabled);

        m_directUi[i].enabledCheck = enableCheck;
        m_directUi[i].nameEdit = nameEdit;
        m_directUi[i].nickEdit = nickEdit;
        m_directUi[i].userEdit = userEdit;
        m_directUi[i].hostEdit = hostEdit;
        m_directUi[i].portEdit = portEdit;
        m_directUi[i].monCheck = monCheck;
        m_directUi[i].container = detailsWidget;
    }

    return mainBox;
}

QWidget* SettingsDialog::createImportExportSection() {
    QGroupBox* box = new QGroupBox("Import / Export");
    QHBoxLayout* layout = new QHBoxLayout(box);
    layout->setSpacing(10);

    QPushButton* exportBtn = new QPushButton("Export", box);
    connect(exportBtn, &QPushButton::clicked, this, &SettingsDialog::onExportClicked);
    layout->addWidget(exportBtn);

    QPushButton* importBtn = new QPushButton("Import", box);
    connect(importBtn, &QPushButton::clicked, this, &SettingsDialog::onImportClicked);
    layout->addWidget(importBtn);

    QPushButton* shareBtn = new QPushButton("Share", box);
    connect(shareBtn, &QPushButton::clicked, this, &SettingsDialog::onShareClicked);
    layout->addWidget(shareBtn);

    return box;
}

void SettingsDialog::loadSettings() {
    const auto& config = ConfigManager::instance();

    m_csvUrlEdit->setText(config.sheetCsvUrl);
    m_sshPassEdit->setText(config.globalSshPass);

    // Ngrok
    for (int i = 0; i < 5; ++i) {
        const auto& pc = config.ngrokPcs[i];
        m_ngrokUi[i].enabledCheck->setChecked(pc.enabled);
        m_ngrokUi[i].nameEdit->setText(pc.name);
        m_ngrokUi[i].nickEdit->setText(pc.nickname);
        m_ngrokUi[i].userEdit->setText(pc.user);
        m_ngrokUi[i].apiEdit->setText(pc.api);
        m_ngrokUi[i].monCheck->setChecked(pc.monitor);
        m_ngrokUi[i].container->setEnabled(pc.enabled);
    }

    // VPN
    for (int i = 0; i < 5; ++i) {
        const auto& pc = config.vpnPcs[i];
        m_vpnUi[i].enabledCheck->setChecked(pc.enabled);
        m_vpnUi[i].nameEdit->setText(pc.name);
        m_vpnUi[i].nickEdit->setText(pc.nickname);
        m_vpnUi[i].userEdit->setText(pc.user);
        m_vpnUi[i].ipEdit->setText(pc.ip);
        m_vpnUi[i].bridgeEdit->setText(pc.bridge);
        m_vpnUi[i].privEdit->setText(pc.priv);
        m_vpnUi[i].monCheck->setChecked(pc.monitor);
        m_vpnUi[i].container->setEnabled(pc.enabled);
    }

    // Direct
    for (int i = 0; i < 5; ++i) {
        const auto& pc = config.directPcs[i];
        m_directUi[i].enabledCheck->setChecked(pc.enabled);
        m_directUi[i].nameEdit->setText(pc.name);
        m_directUi[i].nickEdit->setText(pc.nickname);
        m_directUi[i].userEdit->setText(pc.user);
        m_directUi[i].hostEdit->setText(pc.host);
        m_directUi[i].portEdit->setText(pc.port);
        m_directUi[i].monCheck->setChecked(pc.monitor);
        m_directUi[i].container->setEnabled(pc.enabled);
    }
}

void SettingsDialog::saveSettings() {
    auto& config = ConfigManager::instance();

    config.sheetCsvUrl = m_csvUrlEdit->text().trimmed();
    config.globalSshPass = m_sshPassEdit->text();

    // Ngrok
    for (int i = 0; i < 5; ++i) {
        auto& pc = config.ngrokPcs[i];
        pc.enabled = m_ngrokUi[i].enabledCheck->isChecked();
        pc.name = m_ngrokUi[i].nameEdit->text().trimmed();
        pc.nickname = m_ngrokUi[i].nickEdit->text().trimmed();
        if (pc.nickname.isEmpty()) pc.nickname = pc.name;
        pc.user = m_ngrokUi[i].userEdit->text().trimmed();
        pc.api = m_ngrokUi[i].apiEdit->text().trimmed();
        pc.monitor = m_ngrokUi[i].monCheck->isChecked();
    }

    // VPN
    for (int i = 0; i < 5; ++i) {
        auto& pc = config.vpnPcs[i];
        pc.enabled = m_vpnUi[i].enabledCheck->isChecked();
        pc.name = m_vpnUi[i].nameEdit->text().trimmed();
        pc.nickname = m_vpnUi[i].nickEdit->text().trimmed();
        if (pc.nickname.isEmpty()) pc.nickname = pc.name;
        pc.user = m_vpnUi[i].userEdit->text().trimmed();
        pc.ip = m_vpnUi[i].ipEdit->text().trimmed();
        pc.bridge = m_vpnUi[i].bridgeEdit->text().trimmed();
        pc.priv = m_vpnUi[i].privEdit->text().trimmed();
        pc.monitor = m_vpnUi[i].monCheck->isChecked();
    }

    // Direct
    for (int i = 0; i < 5; ++i) {
        auto& pc = config.directPcs[i];
        pc.enabled = m_directUi[i].enabledCheck->isChecked();
        pc.name = m_directUi[i].nameEdit->text().trimmed();
        pc.nickname = m_directUi[i].nickEdit->text().trimmed();
        if (pc.nickname.isEmpty()) pc.nickname = pc.name;
        pc.user = m_directUi[i].userEdit->text().trimmed();
        pc.host = m_directUi[i].hostEdit->text().trimmed();
        pc.port = m_directUi[i].portEdit->text().trimmed();
        pc.monitor = m_directUi[i].monCheck->isChecked();
    }

    config.save();
}

void SettingsDialog::onSaveClicked() {
    saveSettings();
    accept();
}

void SettingsDialog::onImportClicked() {
    QString path = QFileDialog::getOpenFileName(this, "Import Configuration", "", "Configuration Files (*.conf);;All Files (*)");
    if (!path.isEmpty()) {
        try {
            ConfigManager::instance().importConfig(path);
            loadSettings(); // refresh UI
            QMessageBox::information(this, "Success", "Configuration imported successfully!");
        } catch (...) {
            QMessageBox::critical(this, "Error", "Failed to import configuration.");
        }
    }
}

void SettingsDialog::onExportClicked() {
    QString path = QFileDialog::getSaveFileName(this, "Export Configuration", "omni-ssh.conf", "Configuration Files (*.conf);;All Files (*)");
    if (!path.isEmpty()) {
        try {
            saveSettings(); // save current UI values to config manager
            ConfigManager::instance().exportConfig(path);
            QMessageBox::information(this, "Success", "Configuration exported successfully!");
        } catch (...) {
            QMessageBox::critical(this, "Error", "Failed to export configuration.");
        }
    }
}

void SettingsDialog::onShareClicked() {
    // Sharing details: copy configuration file path to clipboard
    QString path = ConfigManager::instance().configFilePath();
    QApplication::clipboard()->setText(path);
    QMessageBox::information(this, "Share Configuration", 
                             QString("Configuration path copied to clipboard:\n%1").arg(path));
}
