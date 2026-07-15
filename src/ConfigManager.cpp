#include "ConfigManager.h"
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QMap>
#include <QDebug>
#include <QApplication>

ConfigManager& ConfigManager::instance() {
    static ConfigManager inst;
    return inst;
}

ConfigManager::ConfigManager() {
    initDefaults();
}

QString ConfigManager::configFilePath() const {
    QString configDir = qgetenv("XDG_CONFIG_HOME");
    if (configDir.isEmpty()) {
        configDir = QDir::homePath() + "/.config";
    }
    return configDir + "/omni-ssh.conf";
}

void ConfigManager::load() {
    loadFromFile(configFilePath());
}

void ConfigManager::save() {
    QString path = configFilePath();
    // Ensure parent directory exists
    QFileInfo info(path);
    QDir().mkpath(info.absolutePath());
    saveToFile(path);

    // Update KDE autostart desktop entry
    QString autostartDir = QDir::homePath() + "/.config/autostart";
    QString desktopPath = autostartDir + "/komni-ssh.desktop";
    if (enableAutostart) {
        QDir().mkpath(autostartDir);
        QFile file(desktopPath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "[Desktop Entry]\n"
                << "Name=KOmni-SSH\n"
                << "Comment=SSH Connection and Remote Status Monitor\n"
                << "Exec=komni-ssh\n"
                << "Icon=komni-ssh\n"
                << "Terminal=false\n"
                << "Type=Application\n"
                << "Categories=Utility;Network;\n"
                << "StartupNotify=true\n";
            file.close();
        }
    } else {
        QFile::remove(desktopPath);
    }
}

void ConfigManager::importConfig(const QString& path) {
    loadFromFile(path);
    save(); // save to the active config path
}

void ConfigManager::exportConfig(const QString& path) {
    saveToFile(path);
}

void ConfigManager::initDefaults() {
    globalSshPass.clear();
    sheetCsvUrl = "https://docs.google.com/spreadsheets/d/1n3QWceMh5V1npxnWZv8Z25BHoqJCuv_lVu26dbqabDM/export?format=csv";
    theme = "system";
    startMinimized = false;
    enableAutostart = false;
    windowWidth = 500;
    windowHeight = 600;
    sheetRefreshInterval = 10;

    ngrokPcs.clear();
    ngrokPcs.resize(5);
    for (int i = 0; i < 5; ++i) {
        ngrokPcs[i] = NgrokConfig();
    }

    vpnPcs.clear();
    vpnPcs.resize(5);
    for (int i = 0; i < 5; ++i) {
        vpnPcs[i] = VpnConfig();
    }

    directPcs.clear();
    directPcs.resize(5);
    for (int i = 0; i < 5; ++i) {
        directPcs[i] = DirectConfig();
    }
}

void ConfigManager::loadFromFile(const QString& path) {
    initDefaults();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QMap<QString, QString> keyValues;
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }

        // Parse key-value assignments separated by ';' on the same line
        QStringList parts;
        QString currentPart;
        bool inQuotes = false;
        for (int i = 0; i < line.length(); ++i) {
            QChar c = line[i];
            if (c == '"') {
                inQuotes = !inQuotes;
                currentPart.append(c);
            } else if (c == ';' && !inQuotes) {
                parts.append(currentPart.trimmed());
                currentPart.clear();
            } else {
                currentPart.append(c);
            }
        }
        if (!currentPart.trimmed().isEmpty()) {
            parts.append(currentPart.trimmed());
        }

        for (const QString& part : parts) {
            int eqIdx = part.indexOf('=');
            if (eqIdx == -1) continue;
            QString key = part.left(eqIdx).trimmed();
            QString val = part.mid(eqIdx + 1).trimmed();
            if (val.startsWith('"') && val.endsWith('"') && val.length() >= 2) {
                val = val.mid(1, val.length() - 2);
            }
            if (!key.isEmpty()) {
                keyValues[key] = val;
            }
        }
    }
    file.close();

    if (keyValues.contains("GLOBAL_SSH_PASS")) globalSshPass = keyValues["GLOBAL_SSH_PASS"];
    if (keyValues.contains("SHEET_CSV_URL")) sheetCsvUrl = keyValues["SHEET_CSV_URL"];
    if (keyValues.contains("THEME")) theme = keyValues["THEME"];
    if (keyValues.contains("START_MINIMIZED")) startMinimized = (keyValues["START_MINIMIZED"] == "ON");
    if (keyValues.contains("ENABLE_AUTOSTART")) enableAutostart = (keyValues["ENABLE_AUTOSTART"] == "ON");
    if (keyValues.contains("WINDOW_WIDTH")) {
        bool ok;
        int w = keyValues["WINDOW_WIDTH"].toInt(&ok);
        if (ok) windowWidth = w;
    }
    if (keyValues.contains("WINDOW_HEIGHT")) {
        bool ok;
        int h = keyValues["WINDOW_HEIGHT"].toInt(&ok);
        if (ok) windowHeight = h;
    }
    if (keyValues.contains("SHEET_REFRESH_INTERVAL")) {
        bool ok;
        int val = keyValues["SHEET_REFRESH_INTERVAL"].toInt(&ok);
        if (ok && val >= 1 && val <= 90) {
            sheetRefreshInterval = val;
        }
    }

    // Ngrok (N1 to N5)
    for (int i = 0; i < 5; ++i) {
        QString prefix = QString("N%1_").arg(i + 1);
        if (keyValues.contains(prefix + "EN")) ngrokPcs[i].enabled = (keyValues[prefix + "EN"] == "ON");
        if (keyValues.contains(prefix + "NAME")) ngrokPcs[i].name = keyValues[prefix + "NAME"];
        if (keyValues.contains(prefix + "NICK")) ngrokPcs[i].nickname = keyValues[prefix + "NICK"];
        if (ngrokPcs[i].nickname.isEmpty()) ngrokPcs[i].nickname = ngrokPcs[i].name;
        if (keyValues.contains(prefix + "USER")) ngrokPcs[i].user = keyValues[prefix + "USER"];
        if (keyValues.contains(prefix + "API")) ngrokPcs[i].api = keyValues[prefix + "API"];
        if (keyValues.contains(prefix + "MON")) ngrokPcs[i].monitor = (keyValues[prefix + "MON"] == "ON");
    }

    // VPN (V1 to V5)
    for (int i = 0; i < 5; ++i) {
        QString prefix = QString("V%1_").arg(i + 1);
        if (keyValues.contains(prefix + "EN")) vpnPcs[i].enabled = (keyValues[prefix + "EN"] == "ON");
        if (keyValues.contains(prefix + "NAME")) vpnPcs[i].name = keyValues[prefix + "NAME"];
        if (keyValues.contains(prefix + "NICK")) vpnPcs[i].nickname = keyValues[prefix + "NICK"];
        if (vpnPcs[i].nickname.isEmpty()) vpnPcs[i].nickname = vpnPcs[i].name;
        if (keyValues.contains(prefix + "USER")) vpnPcs[i].user = keyValues[prefix + "USER"];
        if (keyValues.contains(prefix + "IP")) vpnPcs[i].ip = keyValues[prefix + "IP"];
        if (keyValues.contains(prefix + "BRIDGE")) vpnPcs[i].bridge = keyValues[prefix + "BRIDGE"];
        if (keyValues.contains(prefix + "PRIV")) vpnPcs[i].priv = keyValues[prefix + "PRIV"];
        if (keyValues.contains(prefix + "MON")) vpnPcs[i].monitor = (keyValues[prefix + "MON"] == "ON");
    }

    // Direct (D1 to D5)
    for (int i = 0; i < 5; ++i) {
        QString prefix = QString("D%1_").arg(i + 1);
        if (keyValues.contains(prefix + "EN")) directPcs[i].enabled = (keyValues[prefix + "EN"] == "ON");
        if (keyValues.contains(prefix + "NAME")) directPcs[i].name = keyValues[prefix + "NAME"];
        if (keyValues.contains(prefix + "NICK")) directPcs[i].nickname = keyValues[prefix + "NICK"];
        if (directPcs[i].nickname.isEmpty()) directPcs[i].nickname = directPcs[i].name;
        if (keyValues.contains(prefix + "USER")) directPcs[i].user = keyValues[prefix + "USER"];
        if (keyValues.contains(prefix + "HOST")) directPcs[i].host = keyValues[prefix + "HOST"];
        if (keyValues.contains(prefix + "PORT")) directPcs[i].port = keyValues[prefix + "PORT"];
        if (keyValues.contains(prefix + "MON")) directPcs[i].monitor = (keyValues[prefix + "MON"] == "ON");
    }
}

void ConfigManager::saveToFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }

    QTextStream out(&file);
    out << "GLOBAL_SSH_PASS=\"" << globalSshPass << "\"\n";
    out << "SHEET_CSV_URL=\"" << sheetCsvUrl << "\"\n";
    out << "THEME=\"" << theme << "\"\n";
    out << "START_MINIMIZED=\"" << (startMinimized ? "ON" : "OFF") << "\"\n";
    out << "ENABLE_AUTOSTART=\"" << (enableAutostart ? "ON" : "OFF") << "\"\n";
    out << "WINDOW_WIDTH=\"" << windowWidth << "\"\n";
    out << "WINDOW_HEIGHT=\"" << windowHeight << "\"\n";
    out << "SHEET_REFRESH_INTERVAL=\"" << sheetRefreshInterval << "\"\n";

    // Ngrok
    for (int i = 0; i < 5; ++i) {
        const auto& pc = ngrokPcs[i];
        out << QString("N%1_EN=\"%2\"; ").arg(i + 1).arg(pc.enabled ? "ON" : "OFF");
        out << QString("N%1_NAME=\"%2\"; ").arg(i + 1).arg(pc.name);
        out << QString("N%1_NICK=\"%2\"; ").arg(i + 1).arg(pc.nickname);
        out << QString("N%1_USER=\"%2\"; ").arg(i + 1).arg(pc.user);
        out << QString("N%1_API=\"%2\"; ").arg(i + 1).arg(pc.api);
        out << QString("N%1_MON=\"%2\"\n").arg(i + 1).arg(pc.monitor ? "ON" : "OFF");
    }

    // VPN
    for (int i = 0; i < 5; ++i) {
        const auto& pc = vpnPcs[i];
        out << QString("V%1_EN=\"%2\"; ").arg(i + 1).arg(pc.enabled ? "ON" : "OFF");
        out << QString("V%1_NAME=\"%2\"; ").arg(i + 1).arg(pc.name);
        out << QString("V%1_NICK=\"%2\"; ").arg(i + 1).arg(pc.nickname);
        out << QString("V%1_USER=\"%2\"; ").arg(i + 1).arg(pc.user);
        out << QString("V%1_IP=\"%2\"; ").arg(i + 1).arg(pc.ip);
        out << QString("V%1_BRIDGE=\"%2\"; ").arg(i + 1).arg(pc.bridge);
        out << QString("V%1_PRIV=\"%2\"; ").arg(i + 1).arg(pc.priv);
        out << QString("V%1_MON=\"%2\"\n").arg(i + 1).arg(pc.monitor ? "ON" : "OFF");
    }

    // Direct
    for (int i = 0; i < 5; ++i) {
        const auto& pc = directPcs[i];
        out << QString("D%1_EN=\"%2\"; ").arg(i + 1).arg(pc.enabled ? "ON" : "OFF");
        out << QString("D%1_NAME=\"%2\"; ").arg(i + 1).arg(pc.name);
        out << QString("D%1_NICK=\"%2\"; ").arg(i + 1).arg(pc.nickname);
        out << QString("D%1_USER=\"%2\"; ").arg(i + 1).arg(pc.user);
        out << QString("D%1_HOST=\"%2\"; ").arg(i + 1).arg(pc.host);
        out << QString("D%1_PORT=\"%2\"; ").arg(i + 1).arg(pc.port);
        out << QString("D%1_MON=\"%2\"\n").arg(i + 1).arg(pc.monitor ? "ON" : "OFF");
    }

    file.close();
}

void ConfigManager::applyTheme() {
    if (theme == "dark") {
        QPalette darkPalette;
        darkPalette.setColor(QPalette::Window, QColor(45, 45, 45));
        darkPalette.setColor(QPalette::WindowText, Qt::white);
        darkPalette.setColor(QPalette::Base, QColor(30, 30, 30));
        darkPalette.setColor(QPalette::AlternateBase, QColor(45, 45, 45));
        darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
        darkPalette.setColor(QPalette::ToolTipText, Qt::white);
        darkPalette.setColor(QPalette::Text, Qt::white);
        darkPalette.setColor(QPalette::Button, QColor(45, 45, 45));
        darkPalette.setColor(QPalette::ButtonText, Qt::white);
        darkPalette.setColor(QPalette::BrightText, Qt::red);
        darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
        darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
        darkPalette.setColor(QPalette::HighlightedText, Qt::white);
        darkPalette.setColor(QPalette::Light, QColor(60, 60, 60));
        darkPalette.setColor(QPalette::Mid, QColor(80, 80, 80));
        darkPalette.setColor(QPalette::Dark, QColor(20, 20, 20));
        darkPalette.setColor(QPalette::PlaceholderText, QColor(150, 150, 150));
        qApp->setPalette(darkPalette);
    } else if (theme == "light") {
        QPalette lightPalette;
        lightPalette.setColor(QPalette::Window, QColor(240, 240, 240));
        lightPalette.setColor(QPalette::WindowText, Qt::black);
        lightPalette.setColor(QPalette::Base, Qt::white);
        lightPalette.setColor(QPalette::AlternateBase, QColor(240, 240, 240));
        lightPalette.setColor(QPalette::ToolTipBase, Qt::black);
        lightPalette.setColor(QPalette::ToolTipText, Qt::black);
        lightPalette.setColor(QPalette::Text, Qt::black);
        lightPalette.setColor(QPalette::Button, QColor(240, 240, 240));
        lightPalette.setColor(QPalette::ButtonText, Qt::black);
        lightPalette.setColor(QPalette::BrightText, Qt::red);
        lightPalette.setColor(QPalette::Link, QColor(0, 120, 215));
        lightPalette.setColor(QPalette::Highlight, QColor(0, 120, 215));
        lightPalette.setColor(QPalette::HighlightedText, Qt::white);
        lightPalette.setColor(QPalette::Light, QColor(255, 255, 255));
        lightPalette.setColor(QPalette::Mid, QColor(200, 200, 200));
        lightPalette.setColor(QPalette::Dark, QColor(160, 160, 160));
        lightPalette.setColor(QPalette::PlaceholderText, QColor(100, 100, 100));
        qApp->setPalette(lightPalette);
    } else {
        qApp->setPalette(systemPalette);
    }
}
