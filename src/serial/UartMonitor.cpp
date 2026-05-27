#include "UartMonitor.h"

#include <QSerialPort>
#include <QSerialPortInfo>

namespace Msv::Serial {

UartMonitor::UartMonitor(std::shared_ptr<Core::ILogger> logger, QObject* parent)
    : QObject(parent)
    , m_logger(std::move(logger))
{
    m_port = new QSerialPort(this);
    connect(m_port, &QSerialPort::readyRead,
            this,   &UartMonitor::onReadyRead);
    connect(m_port, &QSerialPort::errorOccurred,
            this,   &UartMonitor::onErrorOccurred);
}

UartMonitor::~UartMonitor()
{
    close();
}

bool UartMonitor::open(const QString& portName, int baudRate)
{
    close();

    m_port->setPortName(portName);
    m_port->setBaudRate(baudRate);
    m_port->setDataBits(QSerialPort::Data8);
    m_port->setParity(QSerialPort::NoParity);
    m_port->setStopBits(QSerialPort::OneStop);
    m_port->setFlowControl(QSerialPort::NoFlowControl);

    // Только чтение — не вмешиваемся в работу изделия
    if (!m_port->open(QIODevice::ReadOnly)) {
        const QString err = QStringLiteral("Не удалось открыть %1: %2")
            .arg(portName, m_port->errorString());
        m_logger->error(kSrc, err);
        emit errorOccurred(err);
        return false;
    }

    m_buffer.clear();
    m_logger->info(kSrc, QStringLiteral("Порт %1 открыт (%2 бод)")
        .arg(portName).arg(baudRate));
    return true;
}

void UartMonitor::close()
{
    if (m_port->isOpen()) {
        m_port->close();
        m_logger->info(kSrc, QStringLiteral("Порт %1 закрыт").arg(m_port->portName()));
    }
    m_buffer.clear();
}

bool UartMonitor::isOpen() const
{
    return m_port->isOpen();
}

QString UartMonitor::portName() const
{
    return m_port->portName();
}

void UartMonitor::onReadyRead()
{
    m_buffer.append(m_port->readAll());

    // Разбиваем на строки по '\n'
    int newline = -1;
    while ((newline = m_buffer.indexOf('\n')) != -1) {
        QByteArray line = m_buffer.left(newline + 1).trimmed();
        m_buffer.remove(0, newline + 1);

        if (line.isEmpty()) continue;
        if (!line.startsWith('$')) continue;   // не NMEA

        emit rawLineReceived(line);

        const NmeaResult result = NmeaParser::parse(line);
        emit sentenceReceived(result);
    }

    // Защита от переполнения буфера (> 4 КБ без '\n' — явный мусор)
    if (m_buffer.size() > 4096) {
        m_logger->warning(kSrc, "Буфер переполнен — сброс");
        m_buffer.clear();
    }
}

void UartMonitor::onErrorOccurred()
{
    if (m_port->error() == QSerialPort::NoError) return;
    const QString err = QStringLiteral("Ошибка порта %1: %2")
        .arg(m_port->portName(), m_port->errorString());
    m_logger->error(kSrc, err);
    emit errorOccurred(err);
}

} // namespace Msv::Serial
