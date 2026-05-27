#pragma once

#include "ScenarioDispatcher.h"
#include "IDeviceModel.h"
#include "network/WhoIAmScanner.h"
#include "network/WhoIAmTypes.h"
#include <QList>

namespace Msv::Core {

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
    /// Оператор выбрал устройство из списка.
    void selectDevice(const Network::WhoIAmResponse& device);

    /// Оператор ввёл IP вручную.
    void provideManualIp(const QHostAddress& ip);

signals:
    /// Сканирование завершено — показать диалог выбора.
    /// Список может быть пустым (ни одного устройства не найдено).
    void deviceSelectionRequired(const Msv::Network::WhoIAmResponseList& found);

protected:
    void executeStep(int stepIndex) override;

private:
    void runWhoIAm();

    IDeviceModel*            m_deviceModel  {nullptr};
    Network::WhoIAmScanner*  m_whoIAmScanner{nullptr};
    Network::WhoIAmConfig    m_whoIAmConfig;

    static constexpr const char* kSrc = "MsvDispatcher";
};

} // namespace Msv::Core
