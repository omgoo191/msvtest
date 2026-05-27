#pragma once

#include "ScenarioDispatcher.h"
#include "IDeviceModel.h"
#include "network/WhoIAmScanner.h"
#include "network/WhoIAmTypes.h"

namespace Msv::Core {

// ─────────────────────────────────────────────────────────────────────────────
/// Конкретный диспетчер МСВ.
///
/// Переопределяет executeStep() и подключает реальные подсистемы:
///   Шаг 0 — WhoIAmScanner
///   Шаги 1–9 — заглушки (будут заменены в Шагах 3–6)
///
/// Добавляет сигнал manualIpRequired() когда WhoIAm не нашёл устройство,
/// и слот provideManualIp() для получения ответа от UI.
// ─────────────────────────────────────────────────────────────────────────────
class MsvScenarioDispatcher : public ScenarioDispatcher {
    Q_OBJECT
public:
    explicit MsvScenarioDispatcher(QList<StepDescriptor>          steps,
                                   std::shared_ptr<ILogger>        logger,
                                   IDeviceModel*                   deviceModel,
                                   const Network::WhoIAmConfig&    whoIAmConfig = {},
                                   QObject*                        parent = nullptr);
    ~MsvScenarioDispatcher() override = default;

public slots:
    /// UI вызывает этот слот после того как оператор ввёл IP вручную.
    void provideManualIp(const QHostAddress& ip);

signals:
    /// Сканирование WhoIAm не нашло устройство — нужен ручной ввод IP.
    void manualIpRequired();

protected:
    void executeStep(int stepIndex) override;

private:
    // ── Исполнители шагов ─────────────────────────────────────────────────────
    void runWhoIAm();   ///< Шаг 0

    // ── Данные ───────────────────────────────────────────────────────────────
    IDeviceModel*               m_deviceModel  {nullptr};
    Network::WhoIAmScanner*     m_whoIAmScanner{nullptr};
    Network::WhoIAmConfig       m_whoIAmConfig;

    static constexpr const char* kSrc = "MsvDispatcher";
};

} // namespace Msv::Core
