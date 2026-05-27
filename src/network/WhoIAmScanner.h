#pragma once

#include "WhoIAmTypes.h"
#include "core/ILogger.h"
#include <QObject>
#include <QList>
#include <memory>

QT_BEGIN_NAMESPACE
class QUdpSocket;
class QTimer;
QT_END_NAMESPACE

namespace Msv::Network {

// ─────────────────────────────────────────────────────────────────────────────
/// UDP-broadcast сканер.
///
/// Собирает ВСЕ ответы за время сканирования (все попытки), затем
/// эмитирует scanFinished(список). Не останавливается на первом ответе —
/// оператор сам выбирает нужное устройство из списка.
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
    /// Эмитируется по завершении сканирования. Список может быть пустым.
    void scanFinished(const QList<Msv::Network::WhoIAmResponse>& found);
    void errorOccurred(const QString& message);

protected:
    virtual bool parseResponse(const QByteArray&   data,
                               const QHostAddress& sender,
                               WhoIAmResponse&     out) const;

private slots:
    void onReadyRead();
    void onRetryTick();

private:
    void sendBroadcast();

    QUdpSocket*  m_socket     {nullptr};
    QTimer*      m_retryTimer {nullptr};

    WhoIAmConfig                m_config;
    bool                        m_scanning   {false};
    int                         m_retrysDone {0};
    QList<WhoIAmResponse>       m_found;      ///< Накопленные ответы

    std::shared_ptr<Core::ILogger> m_logger;
    static constexpr const char*   kSrc = "WhoIAmScanner";
};

} // namespace Msv::Network
