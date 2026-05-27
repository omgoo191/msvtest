#include "MsvScenarioDispatcher.h"

namespace Msv::Core {

MsvScenarioDispatcher::MsvScenarioDispatcher(
    QList<StepDescriptor>          steps,
    std::shared_ptr<ILogger>        logger,
    IDeviceModel*                   deviceModel,
    const Network::WhoIAmConfig&    whoIAmConfig,
    QObject*                        parent)
    : ScenarioDispatcher(std::move(steps), logger, parent)
    , m_deviceModel  (deviceModel)
    , m_whoIAmConfig (whoIAmConfig)
{
    m_whoIAmScanner = new Network::WhoIAmScanner(logger, this);
}

// ─────────────────────────────────────────────────────────────────────────────

void MsvScenarioDispatcher::executeStep(int stepIndex)
{
    switch (stepIndex) {
        case 0: runWhoIAm(); break;

        // Шаги 1–9: заглушки до реализации в Шагах 3–6
        default:
            ScenarioDispatcher::executeStep(stepIndex);
            break;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Шаг 0 — WhoIAm
// ─────────────────────────────────────────────────────────────────────────────

void MsvScenarioDispatcher::runWhoIAm()
{
    m_logger->info(kSrc, "Запуск WhoIAm-сканирования...");

    // Гарантируем чистые соединения при повторном запуске сценария
    m_whoIAmScanner->disconnect(this);

    // ── Устройство найдено ────────────────────────────────────────────────────
    connect(m_whoIAmScanner, &Network::WhoIAmScanner::deviceFound,
            this, [this](const Network::WhoIAmResponse& resp)
    {
        m_whoIAmScanner->disconnect(this);

        m_deviceModel->applyWhoIAmData(
            resp.ipAddress,
            resp.macAddress,
            resp.firmwareVersion,
            resp.serialNumber
        );

        finishCurrentStep(
            StepResult::Pass,
            QStringLiteral("IP=%1  FW=%2  SN=%3")
                .arg(resp.ipAddress.toString(), resp.firmwareVersion, resp.serialNumber)
        );
    }, Qt::QueuedConnection);

    // ── Сканирование завершено без результата ─────────────────────────────────
    connect(m_whoIAmScanner, &Network::WhoIAmScanner::scanFinished,
            this, [this](bool foundAny)
    {
        if (foundAny) return;   // deviceFound уже вызван — ничего не делаем

        m_whoIAmScanner->disconnect(this);
        m_logger->warning(kSrc, "WhoIAm: устройство не найдено, запрос ручного IP");
        emit manualIpRequired();
        // Шаг НЕ завершаем — ждём provideManualIp() или abort()
    }, Qt::QueuedConnection);

    // ── Ошибка сокета ─────────────────────────────────────────────────────────
    connect(m_whoIAmScanner, &Network::WhoIAmScanner::errorOccurred,
            this, [this](const QString& msg)
    {
        m_whoIAmScanner->disconnect(this);
        m_logger->error(kSrc, QStringLiteral("WhoIAm socket error: %1").arg(msg));
        finishCurrentStep(StepResult::Fail, msg);
    }, Qt::QueuedConnection);

    m_whoIAmScanner->start(m_whoIAmConfig);
}

// ─────────────────────────────────────────────────────────────────────────────

void MsvScenarioDispatcher::provideManualIp(const QHostAddress& ip)
{
    m_logger->info(kSrc, QStringLiteral("Ручной ввод IP: %1").arg(ip.toString()));

    // Записываем в модель с заглушками для неизвестных полей
    m_deviceModel->applyWhoIAmData(
        ip,
        QStringLiteral("—"),    // MAC неизвестен
        QStringLiteral("—"),    // FW неизвестна
        QStringLiteral("—")     // SN неизвестен
    );

    // Шаг завершён с предупреждением — устройство найдено вручную,
    // дальнейшие шаги попытаются получить реальные данные через Web/SNTP
    finishCurrentStep(
        StepResult::Warning,
        QStringLiteral("ручной IP: %1").arg(ip.toString())
    );
}

} // namespace Msv::Core
