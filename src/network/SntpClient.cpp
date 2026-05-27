#include "SntpClient.h"

#include <QUdpSocket>
#include <QTimer>
#include <QElapsedTimer>
#include <QtEndian>

namespace Msv::Network {

// ── NTP пакет (48 байт, RFC 4330) ─────────────────────────────────────────────
#pragma pack(push, 1)
struct NtpPacket {
    quint8  li_vn_mode;      // LI[7:6] | Version[5:3] | Mode[2:0]
    quint8  stratum;
    quint8  poll;
    quint8  precision;
    quint32 rootDelay;
    quint32 rootDispersion;
    quint32 referenceId;
    quint32 refTsHi;
    quint32 refTsLo;
    quint32 origTsHi;
    quint32 origTsLo;
    quint32 rxTsHi;
    quint32 rxTsLo;
    quint32 txTsHi;          // Transmit Timestamp — секунды с 1900-01-01
    quint32 txTsLo;          // Дробная часть (2^32 = 1 сек)
};
#pragma pack(pop)

static_assert(sizeof(NtpPacket) == 48, "NTP packet must be 48 bytes");

// ─────────────────────────────────────────────────────────────────────────────

SntpClient::SntpClient(std::shared_ptr<Core::ILogger> logger, QObject* parent)
    : QObject(parent)
    , m_logger(std::move(logger))
{
    m_socket = new QUdpSocket(this);
    m_timer  = new QTimer(this);
    m_timer->setSingleShot(true);

    connect(m_socket, &QUdpSocket::readyRead, this, &SntpClient::onReadyRead);
    connect(m_timer,  &QTimer::timeout,        this, &SntpClient::onTimeout);
}

void SntpClient::request(const QHostAddress& ip, quint16 port, int timeoutMs)
{
    cancel();
    m_serverIp = ip;

    if (!m_socket->bind(QHostAddress::AnyIPv4, 0)) {
        const QString err = QStringLiteral("bind failed: %1").arg(m_socket->errorString());
        m_logger->error(kSrc, err);
        emit failed(err);
        return;
    }

    // Формируем запрос: LI=0, Version=4, Mode=3 (client) → 0b00100011 = 0x23
    NtpPacket pkt{};
    pkt.li_vn_mode = 0x23;

    const qint64 sent = m_socket->writeDatagram(
        reinterpret_cast<const char*>(&pkt), sizeof(pkt), ip, port);

    if (sent < 0) {
        const QString err = QStringLiteral("send failed: %1").arg(m_socket->errorString());
        m_logger->error(kSrc, err);
        emit failed(err);
        return;
    }

    m_sentAtMs = QDateTime::currentMSecsSinceEpoch();
    m_logger->info(kSrc, QStringLiteral("SNTP запрос → %1:%2").arg(ip.toString()).arg(port));
    m_timer->start(timeoutMs);
}

void SntpClient::cancel()
{
    m_timer->stop();
    if (m_socket->state() != QAbstractSocket::UnconnectedState)
        m_socket->close();
}

void SntpClient::onReadyRead()
{
    while (m_socket->hasPendingDatagrams()) {
        QByteArray   buf(sizeof(NtpPacket), Qt::Uninitialized);
        QHostAddress sender;
        quint16      senderPort{0};

        const qint64 received = m_socket->readDatagram(
            buf.data(), buf.size(), &sender, &senderPort);

        if (received < static_cast<qint64>(sizeof(NtpPacket))) {
            m_logger->warning(kSrc, QStringLiteral("Пакет слишком короткий: %1 байт").arg(received));
            continue;
        }

        // Принимаем только от нашего сервера
        if (sender != m_serverIp) continue;

        m_timer->stop();
        cancel();

        const auto* p = reinterpret_cast<const NtpPacket*>(buf.constData());

        // ── Проверка: Mode должен быть 4 (server) ───────────────────────────
        const int mode = p->li_vn_mode & 0x07;
        if (mode != 4) {
            const QString err = QStringLiteral("Неверный Mode в ответе: %1").arg(mode);
            m_logger->warning(kSrc, err);
            emit failed(err);
            return;
        }

        // ── Извлекаем поля ───────────────────────────────────────────────────
        Result result;
        result.leapIndicator = (p->li_vn_mode >> 6) & 0x03;
        result.stratum       = p->stratum;
        result.roundTripMs   = static_cast<double>(
            QDateTime::currentMSecsSinceEpoch() - m_sentAtMs);

        // Transmit Timestamp → Unix → QDateTime
        const quint32 txSec  = qFromBigEndian(p->txTsHi);
        const quint32 txFrac = qFromBigEndian(p->txTsLo);
        const quint64 unixSec = txSec - kNtpToUnix;

        // Дробная часть → миллисекунды (txFrac / 2^32 * 1000)
        const int ms = static_cast<int>(
            (static_cast<quint64>(txFrac) * 1000ULL) >> 32);

        result.transmitTime = QDateTime::fromMSecsSinceEpoch(
            static_cast<qint64>(unixSec) * 1000 + ms, Qt::UTC);

        m_logger->info(kSrc, QStringLiteral(
            "SNTP ответ: time=%1  LI=%2  stratum=%3  RTT=%.1f ms")
            .arg(result.transmitTime.toString(Qt::ISODateWithMs))
            .arg(result.leapIndicator)
            .arg(result.stratum)
            .arg(result.roundTripMs));

        emit finished(result);
        return;
    }
}

void SntpClient::onTimeout()
{
    cancel();
    m_logger->warning(kSrc, "SNTP таймаут");
    emit failed("таймаут");
}

} // namespace Msv::Network
