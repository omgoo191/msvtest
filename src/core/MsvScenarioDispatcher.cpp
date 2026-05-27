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

void MsvScenarioDispatcher::executeStep(int stepIndex)
{
    switch (stepIndex) {
        case 0: runWhoIAm(); break;
        default: ScenarioDispatcher::executeStep(stepIndex); break;
    }
}

void MsvScenarioDispatcher::runWhoIAm()
{
    m_logger->info(kSrc, "Запуск WhoIAm-сканирования...");
    m_whoIAmScanner->disconnect(this);

    connect(m_whoIAmScanner, &Network::WhoIAmScanner::scanFinished,
            this, [this](const QList<Network::WhoIAmResponse>& found)
    {
        m_whoIAmScanner->disconnect(this);
        m_logger->info(kSrc, QStringLiteral("Сканирование завершено, найдено: %1").arg(found.size()));
        // Всегда показываем диалог — оператор должен подтвердить выбор
        emit deviceSelectionRequired(found);
        // Шаг НЕ завершаем — ждём selectDevice() или provideManualIp()
    }, Qt::QueuedConnection);

    connect(m_whoIAmScanner, &Network::WhoIAmScanner::errorOccurred,
            this, [this](const QString& msg)
    {
        m_whoIAmScanner->disconnect(this);
        m_logger->error(kSrc, QStringLiteral("WhoIAm socket error: %1").arg(msg));
        finishCurrentStep(StepResult::Fail, msg);
    }, Qt::QueuedConnection);

    m_whoIAmScanner->start(m_whoIAmConfig);
}

void MsvScenarioDispatcher::selectDevice(const Network::WhoIAmResponse& device)
{
    m_logger->info(kSrc, QStringLiteral("Выбрано устройство: %1  IP=%2  FW=%3")
        .arg(device.deviceName(), device.ipAddress.toString(), device.firmwareVersion));

    m_deviceModel->applyWhoIAmData(
        device.ipAddress,
        device.firmwareVersion,
        device.deviceId,
        device.deviceName()
    );

    finishCurrentStep(
        StepResult::Pass,
        QStringLiteral("IP=%1  FW=%2  ID=%3")
            .arg(device.ipAddress.toString(), device.firmwareVersion)
            .arg(device.deviceId)
    );
}

void MsvScenarioDispatcher::provideManualIp(const QHostAddress& ip)
{
    m_logger->info(kSrc, QStringLiteral("Ручной ввод IP: %1").arg(ip.toString()));

    m_deviceModel->applyWhoIAmData(ip, "—", 0, "Устройство (ручной ввод)");

    finishCurrentStep(
        StepResult::Warning,
        QStringLiteral("ручной IP: %1").arg(ip.toString())
    );
}

} // namespace Msv::Core
