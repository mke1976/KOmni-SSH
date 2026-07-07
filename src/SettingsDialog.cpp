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
#include <QTabWidget>

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("Settings"));
    resize(550, 650);
    setupUi();
    loadSettings();
}

void SettingsDialog::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(10);

    // Title / Header
    QLabel* titleLabel = new QLabel(tr("Settings"), this);
    titleLabel->setStyleSheet("font-size: 20px; font-weight: bold; padding: 5px;");
    mainLayout->addWidget(titleLabel);

    // Create Tab Widget
    QTabWidget* tabWidget = new QTabWidget(this);

    // Tab 1: Connections
    QScrollArea* connectionsScroll = new QScrollArea(tabWidget);
    connectionsScroll->setWidgetResizable(true);
    connectionsScroll->setFrameShape(QFrame::NoFrame);

    QWidget* scrollWidget = new QWidget(connectionsScroll);
    QVBoxLayout* scrollLayout = new QVBoxLayout(scrollWidget);
    scrollLayout->setContentsMargins(0, 0, 0, 0);
    scrollLayout->setSpacing(15);

    scrollLayout->addWidget(createGlobalSection());
    scrollLayout->addWidget(createNgrokSection());
    scrollLayout->addWidget(createVpnSection());
    scrollLayout->addWidget(createDirectSection());

    scrollWidget->setLayout(scrollLayout);
    connectionsScroll->setWidget(scrollWidget);
    tabWidget->addTab(connectionsScroll, tr("Connections"));

    // Tab 2: Appearance (KDE Look and feel style)
    QWidget* appearanceWidget = createLookAndFeelSection();
    tabWidget->addTab(appearanceWidget, tr("Appearance"));

    // Tab 3: Backup
    QWidget* backupWidget = new QWidget(tabWidget);
    QVBoxLayout* backupLayout = new QVBoxLayout(backupWidget);
    backupLayout->setContentsMargins(10, 10, 10, 10);
    backupLayout->addWidget(createImportExportSection());
    backupLayout->addStretch();
    tabWidget->addTab(backupWidget, tr("Backup"));

    mainLayout->addWidget(tabWidget);

    // Bottom Buttons (Cancel, Save)
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    QPushButton* cancelBtn = new QPushButton(tr("Cancel"), this);
    cancelBtn->setStyleSheet("padding: 8px 16px; border-radius: 4px;");
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(cancelBtn);

    QPushButton* saveBtn = new QPushButton(tr("Save"), this);
    saveBtn->setStyleSheet("padding: 8px 16px; border-radius: 4px; background-color: palette(highlight); color: palette(highlighted-text); font-weight: bold;");
    connect(saveBtn, &QPushButton::clicked, this, &SettingsDialog::onSaveClicked);
    buttonLayout->addWidget(saveBtn);

    mainLayout->addLayout(buttonLayout);
    setLayout(mainLayout);
}

QWidget* SettingsDialog::createGlobalSection() {
    QGroupBox* box = new QGroupBox(tr("General Settings"));
    QFormLayout* form = new QFormLayout(box);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    m_csvUrlEdit = new QLineEdit(box);
    m_csvUrlEdit->setToolTip(tr("Google Sheet URL in CSV format"));
    form->addRow(tr("Google Sheet URL:"), m_csvUrlEdit);

    m_sshPassEdit = new QLineEdit(box);
    m_sshPassEdit->setEchoMode(QLineEdit::Password);
    m_sshPassEdit->setToolTip(tr("Global SSH password if sshpass is used"));
    form->addRow(tr("Global: Key Passphrase:"), m_sshPassEdit);

    return box;
}

QWidget* SettingsDialog::createNgrokSection() {
    QGroupBox* mainBox = new QGroupBox(tr("SSH Ngrok Settings"));
    QVBoxLayout* layout = new QVBoxLayout(mainBox);
    layout->setSpacing(10);

    m_ngrokUi.resize(5);
    for (int i = 0; i < 5; ++i) {
        QGroupBox* pcBox = new QGroupBox(tr("Computer %1").arg(i + 1), mainBox);
        pcBox->setFlat(true);
        QVBoxLayout* pcLayout = new QVBoxLayout(pcBox);
        pcLayout->setContentsMargins(5, 5, 5, 5);

        // Header with Enable toggle
        QHBoxLayout* header = new QHBoxLayout();
        QCheckBox* enableCheck = new QCheckBox(tr("Enable Computer"), pcBox);
        enableCheck->setStyleSheet("font-weight: bold;");
        header->addWidget(enableCheck);
        header->addStretch();
        pcLayout->addLayout(header);

        // Content Widget (collapsible/de-activatable)
        QWidget* detailsWidget = new QWidget(pcBox);
        QFormLayout* form = new QFormLayout(detailsWidget);
        form->setContentsMargins(10, 5, 5, 5);

        QLineEdit* nameEdit = new QLineEdit(detailsWidget);
        form->addRow(tr("Computer Name:"), nameEdit);

        QLineEdit* userEdit = new QLineEdit(detailsWidget);
        form->addRow(tr("User:"), userEdit);

        QLineEdit* apiEdit = new QLineEdit(detailsWidget);
        form->addRow(tr("Ngrok API Key:"), apiEdit);

        QCheckBox* monCheck = new QCheckBox(tr("Online monitor"), detailsWidget);
        form->addRow("", monCheck);

        pcLayout->addWidget(detailsWidget);
        layout->addWidget(pcBox);

        // Connect enable checkbox to enable/disable details
        connect(enableCheck, &QCheckBox::toggled, detailsWidget, &QWidget::setEnabled);

        // Track UI elements
        m_ngrokUi[i].enabledCheck = enableCheck;
        m_ngrokUi[i].nameEdit = nameEdit;
        m_ngrokUi[i].userEdit = userEdit;
        m_ngrokUi[i].apiEdit = apiEdit;
        m_ngrokUi[i].monCheck = monCheck;
        m_ngrokUi[i].container = detailsWidget;
    }

    return mainBox;
}

QWidget* SettingsDialog::createVpnSection() {
    QGroupBox* mainBox = new QGroupBox(tr("SSH IPv6+Wireguard (VPN) Settings"));
    QVBoxLayout* layout = new QVBoxLayout(mainBox);
    layout->setSpacing(10);

    m_vpnUi.resize(5);
    for (int i = 0; i < 5; ++i) {
        QGroupBox* pcBox = new QGroupBox(tr("Computer %1").arg(i + 1), mainBox);
        pcBox->setFlat(true);
        QVBoxLayout* pcLayout = new QVBoxLayout(pcBox);
        pcLayout->setContentsMargins(5, 5, 5, 5);

        QHBoxLayout* header = new QHBoxLayout();
        QCheckBox* enableCheck = new QCheckBox(tr("Enable Computer"), pcBox);
        enableCheck->setStyleSheet("font-weight: bold;");
        header->addWidget(enableCheck);
        header->addStretch();
        pcLayout->addLayout(header);

        QWidget* detailsWidget = new QWidget(pcBox);
        QFormLayout* form = new QFormLayout(detailsWidget);
        form->setContentsMargins(10, 5, 5, 5);

        QLineEdit* nameEdit = new QLineEdit(detailsWidget);
        form->addRow(tr("Computer Name:"), nameEdit);

        QLineEdit* userEdit = new QLineEdit(detailsWidget);
        form->addRow(tr("User:"), userEdit);

        QLineEdit* ipEdit = new QLineEdit(detailsWidget);
        form->addRow(tr("IP Address:"), ipEdit);

        QLineEdit* bridgeEdit = new QLineEdit(detailsWidget);
        form->addRow(tr("Bridge Connection (WARP):"), bridgeEdit);

        QLineEdit* privEdit = new QLineEdit(detailsWidget);
        form->addRow(tr("Private Connection (fritz):"), privEdit);

        QCheckBox* monCheck = new QCheckBox(tr("Online monitor"), detailsWidget);
        form->addRow("", monCheck);

        pcLayout->addWidget(detailsWidget);
        layout->addWidget(pcBox);

        connect(enableCheck, &QCheckBox::toggled, detailsWidget, &QWidget::setEnabled);

        m_vpnUi[i].enabledCheck = enableCheck;
        m_vpnUi[i].nameEdit = nameEdit;
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
    QGroupBox* mainBox = new QGroupBox(tr("SSH IPv4 (Direct) Settings"));
    QVBoxLayout* layout = new QVBoxLayout(mainBox);
    layout->setSpacing(10);

    m_directUi.resize(5);
    for (int i = 0; i < 5; ++i) {
        QGroupBox* pcBox = new QGroupBox(tr("Computer %1").arg(i + 1), mainBox);
        pcBox->setFlat(true);
        QVBoxLayout* pcLayout = new QVBoxLayout(pcBox);
        pcLayout->setContentsMargins(5, 5, 5, 5);

        QHBoxLayout* header = new QHBoxLayout();
        QCheckBox* enableCheck = new QCheckBox(tr("Enable Computer"), pcBox);
        enableCheck->setStyleSheet("font-weight: bold;");
        header->addWidget(enableCheck);
        header->addStretch();
        pcLayout->addLayout(header);

        QWidget* detailsWidget = new QWidget(pcBox);
        QFormLayout* form = new QFormLayout(detailsWidget);
        form->setContentsMargins(10, 5, 5, 5);

        QLineEdit* nameEdit = new QLineEdit(detailsWidget);
        form->addRow(tr("Computer Name:"), nameEdit);

        QLineEdit* userEdit = new QLineEdit(detailsWidget);
        form->addRow(tr("User:"), userEdit);

        QLineEdit* hostEdit = new QLineEdit(detailsWidget);
        form->addRow(tr("Host / Address:"), hostEdit);

        QLineEdit* portEdit = new QLineEdit(detailsWidget);
        form->addRow(tr("Port:"), portEdit);

        QCheckBox* monCheck = new QCheckBox(tr("Online monitor"), detailsWidget);
        form->addRow("", monCheck);

        pcLayout->addWidget(detailsWidget);
        layout->addWidget(pcBox);

        connect(enableCheck, &QCheckBox::toggled, detailsWidget, &QWidget::setEnabled);

        m_directUi[i].enabledCheck = enableCheck;
        m_directUi[i].nameEdit = nameEdit;
        m_directUi[i].userEdit = userEdit;
        m_directUi[i].hostEdit = hostEdit;
        m_directUi[i].portEdit = portEdit;
        m_directUi[i].monCheck = monCheck;
        m_directUi[i].container = detailsWidget;
    }

    return mainBox;
}

QWidget* SettingsDialog::createLookAndFeelSection() {
    QWidget* container = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(container);
    layout->setContentsMargins(15, 15, 15, 15);
    layout->setSpacing(15);

    QGroupBox* themeBox = new QGroupBox(tr("Color Scheme"), container);
    QVBoxLayout* themeLayout = new QVBoxLayout(themeBox);
    themeLayout->setSpacing(10);

    m_lightRadio = new QRadioButton(tr("Light"), themeBox);
    m_darkRadio = new QRadioButton(tr("Dark"), themeBox);
    m_systemRadio = new QRadioButton(tr("System (follow KDE setting)"), themeBox);

    themeLayout->addWidget(m_lightRadio);
    themeLayout->addWidget(m_darkRadio);
    themeLayout->addWidget(m_systemRadio);

    layout->addWidget(themeBox);

    QGroupBox* behaviorBox = new QGroupBox(tr("Behavior"), container);
    QVBoxLayout* behaviorLayout = new QVBoxLayout(behaviorBox);
    behaviorLayout->setSpacing(10);

    m_startMinimizedCheck = new QCheckBox(tr("Start minimized"), behaviorBox);
    behaviorLayout->addWidget(m_startMinimizedCheck);

    layout->addWidget(behaviorBox);
    layout->addStretch();

    return container;
}

QWidget* SettingsDialog::createImportExportSection() {
    QGroupBox* box = new QGroupBox(tr("Import / Export"));
    QVBoxLayout* vbox = new QVBoxLayout(box);
    vbox->setSpacing(10);

    QLabel* desc = new QLabel(tr("Backup or restore your connection settings."), box);
    desc->setStyleSheet("color: palette(placeholder-text); padding-bottom: 5px;");
    vbox->addWidget(desc);

    QHBoxLayout* layout = new QHBoxLayout();
    layout->setSpacing(10);

    QPushButton* exportBtn = new QPushButton(tr("Export"), box);
    connect(exportBtn, &QPushButton::clicked, this, &SettingsDialog::onExportClicked);
    layout->addWidget(exportBtn);

    QPushButton* importBtn = new QPushButton(tr("Import"), box);
    connect(importBtn, &QPushButton::clicked, this, &SettingsDialog::onImportClicked);
    layout->addWidget(importBtn);

    vbox->addLayout(layout);
    return box;
}

void SettingsDialog::loadSettings() {
    const auto& config = ConfigManager::instance();

    m_csvUrlEdit->setText(config.sheetCsvUrl);
    m_sshPassEdit->setText(config.globalSshPass);

    // Look and feel
    if (config.theme == "dark") {
        m_darkRadio->setChecked(true);
    } else if (config.theme == "light") {
        m_lightRadio->setChecked(true);
    } else {
        m_systemRadio->setChecked(true);
    }

    m_startMinimizedCheck->setChecked(config.startMinimized);

    // Ngrok
    for (int i = 0; i < 5; ++i) {
        const auto& pc = config.ngrokPcs[i];
        m_ngrokUi[i].enabledCheck->setChecked(pc.enabled);
        m_ngrokUi[i].nameEdit->setText(pc.name);
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

    // Look and feel
    if (m_darkRadio->isChecked()) {
        config.theme = "dark";
    } else if (m_lightRadio->isChecked()) {
        config.theme = "light";
    } else {
        config.theme = "system";
    }

    config.startMinimized = m_startMinimizedCheck->isChecked();

    config.applyTheme();

    // Ngrok
    for (int i = 0; i < 5; ++i) {
        auto& pc = config.ngrokPcs[i];
        pc.enabled = m_ngrokUi[i].enabledCheck->isChecked();
        pc.name = m_ngrokUi[i].nameEdit->text().trimmed();
        pc.nickname = pc.name;
        pc.user = m_ngrokUi[i].userEdit->text().trimmed();
        pc.api = m_ngrokUi[i].apiEdit->text().trimmed();
        pc.monitor = m_ngrokUi[i].monCheck->isChecked();
    }

    // VPN
    for (int i = 0; i < 5; ++i) {
        auto& pc = config.vpnPcs[i];
        pc.enabled = m_vpnUi[i].enabledCheck->isChecked();
        pc.name = m_vpnUi[i].nameEdit->text().trimmed();
        pc.nickname = pc.name;
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
        pc.nickname = pc.name;
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
    QString path = QFileDialog::getOpenFileName(this, tr("Import Configuration"), "", tr("Configuration Files (*.conf);;All Files (*)"));
    if (!path.isEmpty()) {
        try {
            ConfigManager::instance().importConfig(path);
            loadSettings(); // refresh UI
            QMessageBox::information(this, tr("Success"), tr("Configuration imported successfully!"));
        } catch (...) {
            QMessageBox::critical(this, tr("Error"), tr("Failed to import configuration."));
        }
    }
}

void SettingsDialog::onExportClicked() {
    QString path = QFileDialog::getSaveFileName(this, tr("Export Configuration"), tr("omni-ssh.conf"), tr("Configuration Files (*.conf);;All Files (*)"));
    if (!path.isEmpty()) {
        try {
            saveSettings(); // save current UI values to config manager
            ConfigManager::instance().exportConfig(path);
            QMessageBox::information(this, tr("Success"), tr("Configuration exported successfully!"));
        } catch (...) {
            QMessageBox::critical(this, tr("Error"), tr("Failed to export configuration."));
        }
    }
}
