#include "WhoIAmScanner.h"

#include <QUdpSocket>
#include <QTimer>
#include <QNetworkInterface>

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

WhoIAmScanner::~WhoIAmScanner()
{
    stop();
}

// ─────────────────────────────────────────────────────────────────────────────

void WhoIAmScanner::start(const WhoIAmConfig& config)
{
    if (m_scanning) stop();

    m_config      = config;
    m_scanning    = true;
    m_foundAny    = false;
    m_retrysDone  = 0;

    // Привязываем сокет к любому порту (ОС выбирает сама).
    // Ответы МСВ придут на этот же порт, т.к. device видит наш src-port.
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
    m_logger->debug(kSrc, "Сканирование остановлено");
}

// ─────────────────────────────────────────────────────────────────────────────

void WhoIAmScanner::sendBroadcast()
{
    m_retrysDone++;
    m_logger->debug(kSrc, QStringLiteral("Broadcast → 255.255.255.255:%1  (попытка %2/%3)")
        .arg(m_config.port).arg(m_retrysDone).arg(m_config.retries));

    const qint64 sent = m_socket->writeDatagram(
        m_config.requestPayload,
        QHostAddress::Broadcast,
        m_config.port
    );

    if (sent < 0) {
        m_logger->warning(kSrc, QStringLiteral("writeDatagram error: %1")
            .arg(m_socket->errorString()));
    }

    // Запустить таймер следующей попытки / финального таймаута
    m_retryTimer->start(m_config.retryIntervalMs);
}

void WhoIAmScanner::onRetryTick()
{
    if (!m_scanning) return;

    if (m_foundAny) {
        finish(true);
        return;
    }

    if (m_retrysDone < m_config.retries) {
        sendBroadcast();
    } else {
        m_logger->warning(kSrc, "Устройства не найдены после всех попыток");
        finish(false);
    }
}

void WhoIAmScanner::onReadyRead()
{
    while (m_socket->hasPendingDatagrams()) {
        QByteArray   data(static_cast<int>(m_socket->pendingDatagramSize()), Qt::Uninitialized);
        QHostAddress sender;
        quint16      senderPort{0};

        m_socket->readDatagram(data.data(), data.size(), &sender, &senderPort);

        m_logger->debug(kSrc, QStringLiteral("Датаграмма от %1:%2  (%3 байт)")
            .arg(sender.toString()).arg(senderPort).arg(data.size()));

        WhoIAmResponse resp;
        if (parseResponse(data, sender, resp)) {
            m_foundAny = true;
            m_logger->info(kSrc, QStringLiteral("МСВ обнаружен: IP=%1 MAC=%2 FW=%3 SN=%4")
                .arg(resp.ipAddress.toString())
                .arg(resp.macAddress)
                .arg(resp.firmwareVersion)
                .arg(resp.serialNumber));

            emit deviceFound(resp);
            finish(true);  // останавливаем после первого найденного
            return;
        }
    }
}

void WhoIAmScanner::finish(bool foundAny)
{
    stop();
    emit scanFinished(foundAny);
}

// ─────────────────────────────────────────────────────────────────────────────

bool WhoIAmScanner::parseResponse(const QByteArray&   data,
                                   const QHostAddress& sender,
                                   WhoIAmResponse&     out) const
{
    // Ожидаемый формат ответа от МСВ:
    //   "MSVWHOIAM;MAC=AA:BB:CC:DD:EE:FF;FW=1.2.3;SN=12345678\r\n"
    //
    // IP — всегда из заголовка UDP (адрес отправителя).
    // Это виртуальный метод — переопредели в подклассе под реальный протокол.

    const QString str = QString::fromLatin1(data).trimmed();

    if (!str.startsWith(QLatin1String("MSVWHOIAM;"))) {
        m_logger->debug(kSrc, QStringLiteral("Не распознан: \"%1\"")
            .arg(str.left(32)));
        return false;
    }

    // Парсим key=value через ';'
    QMap<QString, QString> fields;
    const QStringList parts = str.split(';');
    for (const QString& part : parts) {
        const int eq = part.indexOf('=');
        if (eq > 0)
            fields[part.left(eq).trimmed()] = part.mid(eq + 1).trimmed();
    }

    out.ipAddress       = sender;
    out.macAddress      = fields.value("MAC");
    out.firmwareVersion = fields.value("FW");
    out.serialNumber    = fields.value("SN");
    out.rawData         = data;

    // Минимальная валидация
    if (out.macAddress.isEmpty() || out.firmwareVersion.isEmpty()) {
        m_logger->warning(kSrc, "Ответ MSVWHOIAM неполный (нет MAC или FW)");
        return false;
    }

    return true;
}

} // namespace Msv::Network
