#pragma once

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QSystemTrayIcon>
#include <QMap>
#include <QVector>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

// Card widget representation for a computer
class ComputerCardWidget : public QFrame {
    Q_OBJECT
public:
    enum Type { Ngrok, Vpn, Direct };

    ComputerCardWidget(const QString& name, const QString& nickname, Type type, const QString& details, QWidget* parent = nullptr);
    ~ComputerCardWidget() = default;

    QString name() const { return m_name; }
    QString nickname() const { return m_nickname; }
    Type cardType() const { return m_type; }

    void updateStatus(bool online, qint64 timestamp);
    void setUnknownStatus();

signals:
    void connectRequested(const QString& appChoice);

private:
    QString m_name;
    QString m_nickname;
    Type m_type;
    
    QLabel* m_nameLabel;
    QLabel* m_typeLabel;
    QLabel* m_statusTextLabel;
    QPushButton* m_statusButton;

    void setupMenu();
};


class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() = default;

protected:
    void closeEvent(QCloseEvent* event) override;
    void changeEvent(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

private slots:
    void onRefreshTriggered();
    void onSettingsTriggered();
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void handleCsvReply();

private:
    void setupUi();
    void setupTrayIcon();
    void rebuildCards();
    void startPolling();
    void parseCsvData(const QByteArray& data);
    void checkStatusChanges();

    QScrollArea* m_scrollArea = nullptr;
    QWidget* m_scrollWidget = nullptr;
    QVBoxLayout* m_cardsLayout = nullptr;
    QLabel* m_lastRefreshedLabel = nullptr;

    QNetworkAccessManager m_networkManager;
    QTimer* m_refreshTimer = nullptr;
    QSystemTrayIcon* m_trayIcon = nullptr;

    // Status tracking
    QMap<QString, qint64> m_currentStatuses; // nickname -> timestamp
    QMap<QString, QString> m_prevStates;    // nickname -> "online"/"offline"
    bool m_firstRun = true;

    QVector<ComputerCardWidget*> m_cardWidgets;
};
