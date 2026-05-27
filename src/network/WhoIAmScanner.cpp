#include "WhoIAmScanner.h"
#include <QUdpSocket>
#include <QTimer>

namespace Msv::Network {

WhoIAmScanner::WhoIAmScanner(std::shared_ptr<Core::ILogger> logger, QObject* parent)
    : QObject(parent)
    , m_logger(std::move(logger))
{
    m_socket     = new QUdpSocket(this);
    m_retryTimer = new QTimer(this);
    m_retryTimer->setSingleShot(true);

    connect(m_socket,     &QUdpSocket::readyRead, this, &WhoIAmScanner::onReadyRead);
    connect(m_retryTimer, &QTimer::timeout,        this, &WhoIAmScanner::onRetryTick);
}

WhoIAmScanner::~WhoIAmScanner() { stop(); }

void WhoIAmScanner::start(const WhoIAmConfig& config)
{
    if (m_scanning) stop();

    m_config     = config;
    m_scanning   = true;
    m_retrysDone = 0;
    m_found.clear();

    if (!m_socket->bind(QHostAddress::AnyIPv4, 0,
                        QAbstractSocket::ShareAddress |
                        QAbstractSocket::ReuseAddressHint))
    {
        const QString err = QStringLiteral("bind() failed: %1").arg(m_socket->errorString());
        m_logger->error(kSrc, err);
        emit errorOccurred(err);
        m_scanning = false;
        return;
    }

    m_logger->info(kSrc, QStringLiteral("Сканирование запущено (порт %1, попыток %2)")
        .arg(m_config.port).arg(m_config.retries));

    sendBroadcast();
}

void WhoIAmScanner::stop()
{
    if (!m_scanning) return;
    m_scanning = false;
    m_retryTimer->stop();
    m_socket->close();
}

void WhoIAmScanner::sendBroadcast()
{
    m_retrysDone++;
    m_logger->debug(kSrc, QStringLiteral("Broadcast → 255.255.255.255:%1  (попытка %2/%3)")
        .arg(m_config.port).arg(m_retrysDone).arg(m_config.retries));

    if (m_socket->writeDatagram(m_config.requestPayload,
                                 QHostAddress::Broadcast,
                                 m_config.port) < 0)
    {
        m_logger->warning(kSrc, QStringLiteral("writeDatagram: %1")
            .arg(m_socket->errorString()));
    }

    m_retryTimer->start(m_config.retryIntervalMs);
}

void WhoIAmScanner::onRetryTick()
{
    if (!m_scanning) return;

    if (m_retrysDone < m_config.retries) {
        sendBroadcast();
    } else {
        // Все попытки исчерпаны — отдаём всё что нашли (может быть пусто)
        m_logger->info(kSrc, QStringLiteral("Сканирование завершено, найдено устройств: %1")
            .arg(m_found.size()));
        stop();
        emit scanFinished(m_found);
    }
}

void WhoIAmScanner::onReadyRead()
{
    while (m_socket->hasPendingDatagrams()) {
        QByteArray   data(static_cast<int>(m_socket->pendingDatagramSize()), Qt::Uninitialized);
        QHostAddress sender;
        quint16      senderPort{0};

        m_socket->readDatagram(data.data(), data.size(), &sender, &senderPort);

        WhoIAmResponse resp;
        if (!parseResponse(data, sender, resp)) continue;

        // Дедупликация по IP
        const bool alreadyFound = std::any_of(m_found.begin(), m_found.end(),
            [&](const WhoIAmResponse& r){ return r.ipAddress == resp.ipAddress; });

        if (!alreadyFound) {
            m_logger->info(kSrc, QStringLiteral("Устройство: %1  IP=%2  FW=%3")
                .arg(resp.deviceName(), resp.ipAddress.toString(), resp.firmwareVersion));
            m_found.append(resp);
        }
    }
}

bool WhoIAmScanner::parseResponse(const QByteArray&   data,
                                   const QHostAddress& /*sender*/,
                                   WhoIAmResponse&     out) const
{
    // Протокол: "I AM ENCORE_10.22.82.155_20401_v.4.0.0"
    const QString str = QString::fromUtf8(data).trimmed();

    if (!str.startsWith(QLatin1String("I AM ENCORE"))) return false;

    const QStringList tokens = str.split('_');
    if (tokens.size() != 4) return false;

    QHostAddress ip(tokens[1].trimmed());
    if (ip.isNull()) return false;

    bool ok = false;
    const int id = tokens[2].trimmed().toInt(&ok);
    if (!ok) return false;

    const QString fw = tokens[3].trimmed();
    if (!fw.startsWith(QLatin1String("v."))) return false;

    out.ipAddress       = ip;
    out.deviceId        = id;
    out.firmwareVersion = fw;
    out.rawData         = data;
    return true;
}

} // namespace Msv::Network
