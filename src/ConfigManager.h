#pragma once

#include <QString>
#include <QVector>
#include <QPalette>

struct NgrokConfig {
    bool enabled = false;
    QString name;
    QString nickname; // CB Nickname
    QString user = "mk";
    QString api;
    bool monitor = false;
};

struct VpnConfig {
    bool enabled = false;
    QString name;
    QString nickname; // CB Nickname
    QString user = "mk";
    QString ip;
    QString bridge = "WARP";
    QString priv = "fritz";
    bool monitor = false;
};

struct DirectConfig {
    bool enabled = false;
    QString name;
    QString nickname; // CB Nickname
    QString user = "mk";
    QString host;
    QString port = "22";
    bool monitor = false;
};

class ConfigManager {
public:
    static ConfigManager& instance();

    QString configFilePath() const;
    void load();
    void save();
    void importConfig(const QString& path);
    void exportConfig(const QString& path);

    // Configuration fields
    QString globalSshPass;
    QString sheetCsvUrl;
    QString theme;
    QPalette systemPalette;
    bool startMinimized = false;
    int windowWidth = 500;
    int windowHeight = 600;

    void applyTheme();

    QVector<NgrokConfig> ngrokPcs;
    QVector<VpnConfig> vpnPcs;
    QVector<DirectConfig> directPcs;

private:
    ConfigManager();
    void loadFromFile(const QString& path);
    void saveToFile(const QString& path);
    void initDefaults();
};
