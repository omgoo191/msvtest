#pragma once

#include "NmeaParser.h"
#include "core/ILogger.h"
#include <QObject>
#include <QByteArray>
#include <memory>

QT_BEGIN_NAMESPACE
class QSerialPort;
QT_END_NAMESPACE

namespace Msv::Serial {

// ─────────────────────────────────────────────────────────────────────────────
/// Пассивный монитор UART-дубликата GNSS.
///
/// Открывает порт только на чтение. Буферизует входящие данные,
/// разбивает на NMEA-предложения по '\n', парсит каждое через NmeaParser.
///
/// Ничего не пишет в порт — только читает.
// ─────────────────────────────────────────────────────────────────────────────
class UartMonitor : public QObject {
    Q_OBJECT
public:
    explicit UartMonitor(std::shared_ptr<Core::ILogger> logger,
                         QObject* parent = nullptr);
    ~UartMonitor() override;

    bool open (const QString& portName, int baudRate = 9600);
    void close();

    [[nodiscard]] bool     isOpen()    const;
    [[nodiscard]] QString  portName()  const;

signals:
    /// Каждое разобранное предложение (независимо от типа и валидности).
    void sentenceReceived(const Msv::Serial::NmeaResult& result);

    /// Сырые данные (для отладки).
    void rawLineReceived(const QByteArray& line);

    void errorOccurred(const QString& message);

private slots:
    void onReadyRead();
    void onErrorOccurred();

private:
    QSerialPort*                    m_port   {nullptr};
    QByteArray                      m_buffer;
    std::shared_ptr<Core::ILogger>  m_logger;

    static constexpr const char* kSrc = "UartMonitor";
};

} // namespace Msv::Serial
