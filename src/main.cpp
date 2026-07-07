#include <QApplication>
#include <QTranslator>
#include <QLocale>
#include <QIcon>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QStringList>
#include <QDebug>
#include "MainWindow.h"
#include "ConfigManager.h"

// Helper function to check if a language string matches a target prefix.
// It checks if the string starts with the target prefix and is followed by
// a non-letter character or the end of the string (e.g. matches "de", "de-DE", "de_AT", "de-DE-1996").
bool isLanguageMatch(const QString &lang, const QString &target) {
    if (lang.startsWith(target, Qt::CaseInsensitive)) {
        if (lang.length() == target.length()) {
            return true;
        }
        QChar next = lang.at(target.length());
        if (!next.isLetter()) {
            return true;
        }
    }
    return false;
}

// Determines the system language preference based on KDE configuration
// and standard system environment variables.
// If the primary system language is not supported, it falls back to English.
QString determineLanguage() {
    QStringList candidateLists;

    // 1. Check standard environment variables first.
    // The LANGUAGE environment variable takes precedence for translations, followed by LC_ALL, LC_MESSAGES, and LANG.
    // This allows command-line overrides like `LANGUAGE=fr ./komni-ssh`.
    QString envLanguage = qEnvironmentVariable("LANGUAGE");
    if (!envLanguage.isEmpty()) {
        candidateLists << envLanguage;
    }
    QString envLcAll = qEnvironmentVariable("LC_ALL");
    if (!envLcAll.isEmpty()) {
        candidateLists << envLcAll;
    }
    QString envLcMessages = qEnvironmentVariable("LC_MESSAGES");
    if (!envLcMessages.isEmpty()) {
        candidateLists << envLcMessages;
    }
    QString envLang = qEnvironmentVariable("LANG");
    if (!envLang.isEmpty()) {
        candidateLists << envLang;
    }

    // 2. Check KDE Plasma localerc file which stores user's UI translations settings in the desktop session.
    // In KDE System Settings, adding multiple languages generates a colon-separated
    // list under [Translations]/LANGUAGE.
    QString localercPath = QDir::homePath() + "/.config/plasma-localerc";
    if (QFile::exists(localercPath)) {
        QSettings localerc(localercPath, QSettings::IniFormat);
        QString langSetting = localerc.value("Translations/LANGUAGE").toString();
        if (!langSetting.isEmpty()) {
            candidateLists << langSetting;
        }
    }

    // 3. Fallback to the Qt system locale name
    candidateLists << QLocale::system().name();

    // Flatten and parse the colon-separated lists into individual language items
    QStringList languagesToCheck;
    for (const QString &listStr : candidateLists) {
        QStringList parts = listStr.split(':');
        for (const QString &part : parts) {
            QString trimmed = part.trimmed();
            if (!trimmed.isEmpty() && !languagesToCheck.contains(trimmed)) {
                languagesToCheck << trimmed;
            }
        }
    }

    qDebug() << "KOmni-SSH Language priority list:" << languagesToCheck;

    // Check each language in priority order
    for (const QString &lang : languagesToCheck) {
        if (isLanguageMatch(lang, "de")) {
            return "de";
        } else if (isLanguageMatch(lang, "fr")) {
            return "fr";
        } else if (isLanguageMatch(lang, "zh")) {
            return "zh";
        } else if (isLanguageMatch(lang, "en")) {
            return "en";
        } else {
            // "If any not support language is set use English."
            // If the highest-priority language set on the system/KDE is unsupported
            // (e.g. Spanish, Italian), we default to English.
            return "en";
        }
    }

    return "en";
}

int main(int argc, char *argv[]) {
    // Enable system integration features
    QApplication::setApplicationName("komni-ssh");
    QApplication::setApplicationDisplayName("KOmni-SSH");
    QApplication::setOrganizationName("omni");
    QApplication::setDesktopSettingsAware(true);

    QApplication app(argc, argv);

    // Set style (on Plasma this automatically defaults to native Breeze)
    app.setWindowIcon(QIcon(":/icons/app_icon_transparent.png"));

    // Capture system palette BEFORE any theme is applied
    ConfigManager::instance().systemPalette = app.palette();

    // Determine target translation language and load it
    QTranslator translator;
    QString langCode = determineLanguage();
    if (langCode == "de" || langCode == "fr" || langCode == "zh") {
        // Load target translation from embedded resource path prefix.
        // QTranslator::load automatically handles appending the ".qm" extension.
        if (translator.load(":/translations/translations/komni-ssh_" + langCode)) {
            app.installTranslator(&translator);
            qDebug() << "Loaded translation for language:" << langCode;
        } else {
            qWarning() << "Failed to load translation for language:" << langCode;
        }
    } else {
        qDebug() << "Using default English UI (no translation file loaded)";
    }

    // Load configuration and apply saved theme
    ConfigManager::instance().load();
    ConfigManager::instance().applyTheme();

    // If running in test mode, exit immediately after setting up translations
    if (app.arguments().contains("--test-lang")) {
        return 0;
    }

    MainWindow w;
    if (!ConfigManager::instance().startMinimized) {
        w.show();
    }

    return app.exec();
}
