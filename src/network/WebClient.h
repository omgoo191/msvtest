#pragma once

#include "core/IDeviceModel.h"
#include "core/ILogger.h"
#include <QObject>
#include <QDateTime>
#include <QHostAddress>
#include <memory>

QT_BEGIN_NAMESPACE
class QNetworkAccessManager;
class QNetworkReply;
class QTimer;
QT_END_NAMESPACE

namespace Msv::Network {

// ─────────────────────────────────────────────────────────────────────────────
/// HTTP-клиент для чтения статуса МСВ.
///
/// Отправляет GET на http://<ip>/ и разбирает ответ.
/// По умолчанию читает заголовок Date (время) и тело (статус синхронизации).
/// Переопредели parseBody() под реальный API изделия.
// ─────────────────────────────────────────────────────────────────────────────
class WebClient : public QObject {
    Q_OBJECT
public:
    struct Result {
        Core::SyncSource syncSource {Core::SyncSource::Unknown};
        Core::SyncStatus syncStatus {Core::SyncStatus::Unknown};
        QDateTime        webTime;
        QString          rawBody;
		QString macAddress;
		QString gpsTime;
		QString gpsDate;
		QString gpsStatus;       // "st_gps"
		QString gpsMode;
		QString gpsSatellites;
		QString gpsSignalLevel;
		QString rtcTime;
		QString rtcDate;
		QString rtcValidity;
		QString antennaStatus;   // "as"
		QString lastValidGps;    // "lr"
		QString sntpSyncCount;   // "ssn"
		QString uptime;
		QString buildDate;
		QString crc;
    };

    explicit WebClient(std::shared_ptr<Core::ILogger> logger,
                       QObject* parent = nullptr);

    /// Запустить запрос к устройству. Результат придёт через сигналы.
    void request(const QHostAddress& ip,
                 const QString&      path      = "/",
                 int                 timeoutMs = 5000);

    void cancel();
	void setQuiet(bool quiet) { m_quiet = quiet; }

signals:
    void finished  (const Msv::Network::WebClient::Result& result);
    void failed    (const QString& reason);

protected:
    /// Разобрать тело ответа. Заполни out.syncSource/syncStatus.
    /// webTime уже заполнен из заголовка Date до вызова этого метода.
    /// Переопредели под реальный формат (JSON / HTML / текст).
    virtual void parseBody(const QByteArray& body, Result& out) const;

private slots:
    void onReplyFinished();
    void onTimeout();




private:
    QNetworkAccessManager*          m_nam     {nullptr};
    QNetworkReply*                  m_reply   {nullptr};
    QTimer*                         m_timer   {nullptr};
    std::shared_ptr<Core::ILogger>  m_logger;
	bool 							m_quiet {false};

    static constexpr const char* kSrc = "WebClient";
};

} // namespace Msv::Network

Q_DECLARE_METATYPE(Msv::Network::WebClient::Result)
