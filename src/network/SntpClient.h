#pragma once

#include "core/ILogger.h"
#include <QObject>
#include <QDateTime>
#include <QHostAddress>
#include <memory>

QT_BEGIN_NAMESPACE
class QUdpSocket;
class QTimer;
QT_END_NAMESPACE

namespace Msv::Network {

// ─────────────────────────────────────────────────────────────────────────────
/// SNTP-клиент (RFC 4330).
///
/// Отправляет один UDP-запрос на порт 123 изделия и разбирает ответ.
/// Извлекает: время передачи, Leap Indicator, Stratum, RTT.
// ─────────────────────────────────────────────────────────────────────────────
class SntpClient : public QObject {
    Q_OBJECT
public:
    struct Result {
        QDateTime transmitTime;    ///< Время с сервера (txTimestamp, UTC)
        int       leapIndicator;   ///< 0=ok, 1=+1с, 2=-1с, 3=не синхр.
        int       stratum;         ///< 1=primary, 2..15=secondary, 0=неизв.
        double    roundTripMs;     ///< Круговая задержка в мс
    };

    explicit SntpClient(std::shared_ptr<Core::ILogger> logger,
                        QObject* parent = nullptr);

    void request(const QHostAddress& ip,
                 quint16             port      = 123,
                 int                 timeoutMs = 5000);

    void cancel();

signals:
    void finished  (const Msv::Network::SntpClient::Result& result);
    void failed    (const QString& reason);

private slots:
    void onReadyRead();
    void onTimeout();

private:
    QUdpSocket*                     m_socket  {nullptr};
    QTimer*                         m_timer   {nullptr};
    QHostAddress                    m_serverIp;
    qint64                          m_sentAtMs{0};
    std::shared_ptr<Core::ILogger>  m_logger;

    static constexpr const char* kSrc = "SntpClient";

    // NTP epoch → Unix epoch: 70 лет в секундах
    static constexpr quint64 kNtpToUnix = 2208988800ULL;
};

} // namespace Msv::Network

Q_DECLARE_METATYPE(Msv::Network::SntpClient::Result)
