#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QVector>
#include <QRadioButton>
#include "ConfigManager.h"

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = nullptr);
    ~SettingsDialog() = default;

private:
    void setupUi();
    void loadSettings();
    void saveSettings();

    // UI tracking structures
    struct NgrokUi {
        QCheckBox* enabledCheck = nullptr;
        QLineEdit* nameEdit = nullptr;
        QLineEdit* userEdit = nullptr;
        QLineEdit* apiEdit = nullptr;
        QCheckBox* monCheck = nullptr;
        QWidget* container = nullptr;
    };

    struct VpnUi {
        QCheckBox* enabledCheck = nullptr;
        QLineEdit* nameEdit = nullptr;
        QLineEdit* userEdit = nullptr;
        QLineEdit* ipEdit = nullptr;
        QLineEdit* bridgeEdit = nullptr;
        QLineEdit* privEdit = nullptr;
        QCheckBox* monCheck = nullptr;
        QWidget* container = nullptr;
    };

    struct DirectUi {
        QCheckBox* enabledCheck = nullptr;
        QLineEdit* nameEdit = nullptr;
        QLineEdit* userEdit = nullptr;
        QLineEdit* hostEdit = nullptr;
        QLineEdit* portEdit = nullptr;
        QCheckBox* monCheck = nullptr;
        QWidget* container = nullptr;
    };

    // Global edits
    QLineEdit* m_sshPassEdit = nullptr;
    QLineEdit* m_csvUrlEdit = nullptr;

    // Look & Feel edits
    QRadioButton* m_lightRadio = nullptr;
    QRadioButton* m_darkRadio = nullptr;
    QRadioButton* m_systemRadio = nullptr;
    QCheckBox* m_startMinimizedCheck = nullptr;

    QVector<NgrokUi> m_ngrokUi;
    QVector<VpnUi> m_vpnUi;
    QVector<DirectUi> m_directUi;

    // Helper builders
    QWidget* createGlobalSection();
    QWidget* createNgrokSection();
    QWidget* createVpnSection();
    QWidget* createDirectSection();
    QWidget* createLookAndFeelSection();
    QWidget* createImportExportSection();

private slots:
    void onSaveClicked();
    void onImportClicked();
    void onExportClicked();
};
