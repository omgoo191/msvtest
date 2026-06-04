#pragma once

#include "ScenarioDispatcher.h"
#include "IDeviceModel.h"
#include "network/WhoIAmScanner.h"
#include "network/WhoIAmTypes.h"
#include <QList>
#include "network/WebClient.h"
#include "network/SntpClient.h"
#include "serial/UartMonitor.h"
#include "report/ReportGenerator.h"
#include "core/LongRunMonitor.h"

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
	void startLongRun(int durationMinutes);
	void stopLongRun();
	void setLongRunActive(bool active) { m_longRunActive = active; }


public slots:
    /// Оператор выбрал устройство из списка.
    void selectDevice(const Network::WhoIAmResponse& device);

    /// Оператор ввёл IP вручную.
    void provideManualIp(const QHostAddress& ip);

	void selectPort(const QString& portName, int baudRate);

	void setOperatorName(const QString& name);
signals:
    /// Сканирование завершено — показать диалог выбора.
    /// Список может быть пустым (ни одного устройства не найдено).
    void deviceSelectionRequired(const Msv::Network::WhoIAmResponseList& found);
	void portSelectionRequired();
	void longRunProgressUpdate(int elapsedSec, int totalSec, const QString& stats);
	void longRunFinished(const Msv::Core::LongRunResult& result);
	void longRunPortSelectionRequired();
	void longRunDisplayUpdate(const QString& stats);

protected:
    void executeStep(int stepIndex) override;

private:
    void runWhoIAm();
	void runWebStatus();
	void runSntp();
	void runUartMonitor();
	void stopUartMonitoring();
	void reset() override;
	void runKzMonitoring();
	void runKzRecovery();
	void stopPollTimer();
	void runReport();
	void startBackgroundPolling();
	void stopBackgroundPolling();

    IDeviceModel*            m_deviceModel  {nullptr};
    Network::WhoIAmScanner*  m_whoIAmScanner{nullptr};
    Network::WhoIAmConfig    m_whoIAmConfig;
	Network::WebClient*	     m_webClient    {nullptr};
	Network::SntpClient*     m_sntpClient   {nullptr};
	Serial::UartMonitor*     m_uartMonitor  {nullptr};
	QTimer*                  m_uartTimer    {nullptr};
	QTimer*                  m_uartNoDataTimer    {nullptr};
	QTimer*					 m_pollTimer     {nullptr};
	int                      m_pollCount      {0};
	int 					 m_maxPolls       {0};
	DeviceSnapshot           m_snapshotBeforeKz;
	DeviceSnapshot           m_snapshotDuringKz;
	std::unique_ptr<Report::ReportGenerator>  m_reportGenerator {nullptr};
	QDateTime			     m_sessionStart;
	QString 				 m_operatorName;
	QTimer*				     m_backgroundPollTimer {nullptr};
	LongRunMonitor*			 m_longRunMonitor      {nullptr};
	bool 					 m_longRunActive	   {false};

    static constexpr const char* kSrc = "MsvDispatcher";

	struct UartStats {
		int total{0}, checksumOk{0}, rmcCount{0}, ggaCount{0}, monotonErrors{0};
		QDateTime lastUtc;
	};


};

} // namespace Msv::Core
