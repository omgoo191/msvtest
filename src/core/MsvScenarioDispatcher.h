#pragma once

#include "ScenarioDispatcher.h"
#include "IDeviceModel.h"
#include "network/WhoIAmScanner.h"
#include "network/WhoIAmTypes.h"
#include <QList>
#include "network/WebClient.h"
#include "network/SntpClient.h"
#include "serial/UartMonitor.h"
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

	void selectPort(const QString& portName, int baudRate);
signals:
    /// Сканирование завершено — показать диалог выбора.
    /// Список может быть пустым (ни одного устройства не найдено).
    void deviceSelectionRequired(const Msv::Network::WhoIAmResponseList& found);
	void portSelectionRequired();

protected:
    void executeStep(int stepIndex) override;

private:
    void runWhoIAm();
	void runWebStatus();
	void runSntp();
	void runUartMonitor();
	void stopUartMonitoring();
	void reset() override;

    IDeviceModel*            m_deviceModel  {nullptr};
    Network::WhoIAmScanner*  m_whoIAmScanner{nullptr};
    Network::WhoIAmConfig    m_whoIAmConfig;
	Network::WebClient*	     m_webClient    {nullptr};
	Network::SntpClient*     m_sntpClient   {nullptr};
	Serial::UartMonitor*     m_uartMonitor  {nullptr};
	QTimer*                  m_uartTimer    {nullptr};
	QTimer*                  m_uartNoDataTimer    {nullptr};

    static constexpr const char* kSrc = "MsvDispatcher";

	struct UartStats {
		int total{0}, checksumOk{0}, rmcCount{0}, ggaCount{0}, monotonErrors{0};
		QDateTime lastUtc;
	};
};

} // namespace Msv::Core
