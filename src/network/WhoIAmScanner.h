#pragma once

#include "WhoIAmTypes.h"
#include "core/ILogger.h"
#include <QObject>
#include <memory>

QT_BEGIN_NAMESPACE
class QUdpSocket;
class QTimer;
QT_END_NAMESPACE

namespace Msv::Network {

// ─────────────────────────────────────────────────────────────────────────────
/// UDP-broadcast сканер для обнаружения МСВ в сети.
///
/// Жизненный цикл:
///   start() → [deviceFound × N] → scanFinished(foundAny)
///
/// Как только найдено первое устройство — сканирование останавливается
/// (в стенде ожидается ровно один МСВ). Если нужно несколько — убрать
/// вызов stop() в onReadyRead().
///
/// Протокол разбора вынесен в виртуальный parseResponse() — подкласс
/// может переопределить под реальный бинарный формат изделия.
// ─────────────────────────────────────────────────────────────────────────────
class WhoIAmScanner : public QObject {
    Q_OBJECT
public:
    explicit WhoIAmScanner(std::shared_ptr<Core::ILogger> logger,
                           QObject* parent = nullptr);
    ~WhoIAmScanner() override;

    void start(const WhoIAmConfig& config = {});
    void stop();

    [[nodiscard]] bool isScanning() const { return m_scanning; }

signals:
    void deviceFound  (const Msv::Network::WhoIAmResponse& response);
    void scanFinished (bool foundAny);
    void errorOccurred(const QString& message);

protected:
    /// Разбирает сырой UDP-payload. Возвращает true и заполняет out при успехе.
    virtual bool parseResponse(const QByteArray&    data,
                               const QHostAddress&  sender,
                               WhoIAmResponse&      out) const;

private slots:
    void onReadyRead();
    void onRetryTick();

private:
    void sendBroadcast();
    void finish(bool foundAny);

    QUdpSocket*  m_socket  {nullptr};
    QTimer*      m_retryTimer {nullptr};

    WhoIAmConfig m_config;
    bool         m_scanning  {false};
    bool         m_foundAny  {false};
    int          m_retrysDone{0};

    std::shared_ptr<Core::ILogger> m_logger;
    static constexpr const char* kSrc = "WhoIAmScanner";
};

} // namespace Msv::Network
