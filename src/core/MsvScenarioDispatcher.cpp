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
	m_webClient = new Network::WebClient(logger, this);
	m_sntpClient = new Network::SntpClient(logger, this);
	m_uartMonitor = new Serial::UartMonitor(logger, this);

}

void MsvScenarioDispatcher::executeStep(int stepIndex)
{
	if(stepIndex !=4)
		stopUartMonitoring();

	stopPollTimer();

    switch (stepIndex) {
        case 0: runWhoIAm(); break;
		case 1: runWebStatus(); break;
		case 2: runSntp();      break;
		case 4: runUartMonitor(); break;
		case 6: runKzMonitoring(); break;
		case 8: runKzRecovery(); break;
		default: ScenarioDispatcher::executeStep(stepIndex); break;
    }
}

void MsvScenarioDispatcher::runWhoIAm()
{
    m_logger->info(kSrc, "Запуск WhoIAm-сканирования...");
    m_whoIAmScanner->disconnect(this);

    connect(m_whoIAmScanner, &Network::WhoIAmScanner::scanFinished,
            this, [this](const Network::WhoIAmResponseList& found)
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

void MsvScenarioDispatcher::runWebStatus()
{
	if (!m_deviceModel->isDeviceFound()) {
		m_logger->error(kSrc, "Web-статус: устройство не найдено в модели");
		finishCurrentStep(StepResult::Fail, "устройство не найдено");
		return;
	}

	const QHostAddress ip = m_deviceModel->currentSnapshot().ipAddress;
	m_logger->info(kSrc, QStringLiteral("Web-запрос → %1").arg(ip.toString()));

	m_webClient->disconnect(this);

	connect(m_webClient, &Network::WebClient::finished,
			this, [this](const Network::WebClient::Result& res)
			{
				m_webClient->disconnect(this);

				Core::WebStatusData data;
				data.syncSource    = res.syncSource;
				data.syncStatus    = res.syncStatus;
				data.webTime       = res.webTime;
				data.macAddress    = res.macAddress;
				data.gpsTime       = res.gpsTime;
				data.gpsDate       = res.gpsDate;
				data.gpsStatus     = res.gpsStatus;
				data.gpsMode       = res.gpsMode;
				data.gpsSatellites = res.gpsSatellites;
				data.gpsSignalLevel= res.gpsSignalLevel;
				data.rtcTime       = res.rtcTime;
				data.rtcDate       = res.rtcDate;
				data.rtcValidity   = res.rtcValidity;
				data.antennaStatus = res.antennaStatus;
				data.lastValidGps  = res.lastValidGps;
				data.sntpSyncCount = res.sntpSyncCount;
				data.uptime        = res.uptime;
				data.buildDate     = res.buildDate;
				data.crc           = res.crc;
				m_deviceModel->applyWebStatus(data);
				if(!res.macAddress.isEmpty())
				{
					const auto snap = m_deviceModel->currentSnapshot();
					m_deviceModel->applyWhoIAmData(
							snap.ipAddress,
							snap.firmwareVersion,
							snap.deviceId,
							snap.deviceName
							);
				}
				m_logger->info(kSrc, QStringLiteral("Антенна: %1 GPS: %2").arg(res.antennaStatus, res.gpsStatus));
				const bool hasTime   = res.webTime.isValid();
				const bool hasStatus = res.syncStatus != Core::SyncStatus::Unknown;

				StepResult result = StepResult::Pass;
				QString    detail;

				if (!hasTime && !hasStatus) {
					result = StepResult::Warning;
					detail = "ответ получен, данные не распознаны";
				} else if (res.syncStatus == Core::SyncStatus::Freerun) {
					result = StepResult::Warning;
					detail = "изделие не синхронизировано (Freerun)";
				} else {
					detail = QStringLiteral("time=%1  status=%2")
							.arg(res.webTime.toString("hh:mm:ss"))
							.arg(static_cast<int>(res.syncStatus));
				}

				finishCurrentStep(result, detail);
			}, Qt::QueuedConnection);

	connect(m_webClient, &Network::WebClient::failed,
			this, [this](const QString& reason)
			{
				m_webClient->disconnect(this);
				m_logger->error(kSrc, QStringLiteral("Web ошибка: %1").arg(reason));
				finishCurrentStep(StepResult::Fail, reason);
			}, Qt::QueuedConnection);

	m_webClient->request(ip, "/tags.shtml");
}

void MsvScenarioDispatcher::runSntp()
{
	if (!m_deviceModel->isDeviceFound()) {
		finishCurrentStep(StepResult::Fail, "устройство не найдено");
		return;
	}

	const QHostAddress ip = m_deviceModel->currentSnapshot().ipAddress;
	m_logger->info(kSrc, QStringLiteral("SNTP запрос → %1:123").arg(ip.toString()));

	m_sntpClient->disconnect(this);

	connect(m_sntpClient, &Network::SntpClient::finished,
			this, [this](const Network::SntpClient::Result& res)
			{
				m_sntpClient->disconnect(this);

				m_deviceModel->applySntpData(res.transmitTime, res.leapIndicator, res.stratum);

				StepResult result = StepResult::Pass;
				QString    detail = QStringLiteral("time=%1  LI=%2  stratum=%3  RTT=%.0f ms")
						.arg(res.transmitTime.toString("hh:mm:ss.zzz"))
						.arg(res.leapIndicator)
						.arg(res.stratum)
						.arg(res.roundTripMs);

				if (res.leapIndicator == 3) {
					result = StepResult::Warning;
					detail.prepend("LI=3 (не синхронизирован)  ");
				} else if (res.stratum == 0 || res.stratum > 15) {
					result = StepResult::Warning;
					detail.prepend("неверный stratum  ");
				}

				finishCurrentStep(result, detail);
			}, Qt::QueuedConnection);

	connect(m_sntpClient, &Network::SntpClient::failed,
			this, [this](const QString& reason)
			{
				m_sntpClient->disconnect(this);
				m_logger->error(kSrc, QStringLiteral("SNTP ошибка: %1").arg(reason));
				finishCurrentStep(StepResult::Fail, reason);
			}, Qt::QueuedConnection);

	m_sntpClient->request(ip);
}

void MsvScenarioDispatcher::runUartMonitor()
{
	m_logger->info(kSrc, "Запуск мониторинга UART-GNSS...");
	stopUartMonitoring();
	emit portSelectionRequired();  // всегда спрашиваем порт
}

void MsvScenarioDispatcher::selectPort(const QString& portName, int baudRate)
{
	m_logger->info(kSrc, QStringLiteral("Выбран порт: %1 @ %2")
			.arg(portName).arg(baudRate));

	if (!m_uartMonitor->open(portName, baudRate)) {
		finishCurrentStep(StepResult::Fail,
						  QStringLiteral("не удалось открыть %1").arg(portName));
		return;
	}

	auto stats = std::make_shared<UartStats>();

	connect(m_uartMonitor, &Serial::UartMonitor::sentenceReceived,
			this, [this, stats](const Serial::NmeaResult& r)
			{
				stats->total++;
				if (!r.checksumValid) return;
				stats->checksumOk++;

				if (r.sentenceType == "RMC") stats->rmcCount++;
				if (r.sentenceType == "GGA") stats->ggaCount++;

				if (r.utcTime.isValid() && stats->lastUtc.isValid()) {
					if (r.utcTime <= stats->lastUtc)
						stats->monotonErrors++;
				}
				if (r.utcTime.isValid())
					stats->lastUtc = r.utcTime;

				if (r.isValid && r.utcTime.isValid())
					m_deviceModel->applyGnssTime(r.utcTime);

				// Сбрасываем таймер "нет данных"
				if (m_uartNoDataTimer)
					m_uartNoDataTimer->start(5000);

			});

	// Таймер "нет данных 5 секунд"
	m_uartNoDataTimer = new QTimer(this);
	m_uartNoDataTimer->setSingleShot(true);
	connect(m_uartNoDataTimer, &QTimer::timeout,
			this, [this]()
			{
				if (m_uartNoDataTimer) {
					m_uartNoDataTimer->deleteLater();
					m_uartNoDataTimer = nullptr;
				}
				// Срабатываем только если основной таймер ещё жив
				if (!m_uartTimer) return;
				m_logger->warning(kSrc, "Нет данных 5 секунд — смена порта");
				stopUartMonitoring();
				emit portSelectionRequired();
			});
	m_uartNoDataTimer->start(5000);

	// Основной таймер 60 секунд
	m_uartTimer = new QTimer(this);
	m_uartTimer->setSingleShot(true);
	connect(m_uartTimer, &QTimer::timeout,
			this, [this, stats]()
			{
				if (m_uartNoDataTimer) {
					m_uartNoDataTimer->stop();
					m_uartNoDataTimer->deleteLater();
					m_uartNoDataTimer = nullptr;
				}

				m_uartTimer->deleteLater();
				m_uartTimer = nullptr;
				m_uartMonitor->disconnect(this);
				m_uartMonitor->close();

				m_logger->info(kSrc, QStringLiteral(
						"UART итог: всего=%1 checksum_ok=%2 RMC=%3 GGA=%4 монотон_ошибок=%5")
						.arg(stats->total).arg(stats->checksumOk)
						.arg(stats->rmcCount).arg(stats->ggaCount)
						.arg(stats->monotonErrors));

				StepResult result = StepResult::Pass;
				QString    detail;

				if (stats->total == 0) {
					result = StepResult::Fail;
					detail = "нет данных от порта";
				} else if (stats->checksumOk == 0) {
					result = StepResult::Fail;
					detail = QStringLiteral("все %1 предложений с ошибкой checksum")
							.arg(stats->total);
				} else if (stats->ggaCount == 0 && stats->rmcCount == 0) {
					result = StepResult::Fail;
					detail = "нет GGA/RMC предложений";
				} else if (stats->rmcCount == 0) {
					result = StepResult::Warning;
					detail = QStringLiteral("нет валидных RMC (возможно битые данные), GGA=%1")
							.arg(stats->ggaCount);
				} else if (stats->monotonErrors > 0) {
					result = StepResult::Warning;
					detail = QStringLiteral("нарушений монотонности UTC: %1")
							.arg(stats->monotonErrors);
				} else {
					detail = QStringLiteral("RMC=%1 GGA=%2 checksum_ok=%3/%4")
							.arg(stats->rmcCount).arg(stats->ggaCount)
							.arg(stats->checksumOk).arg(stats->total);
				}

				finishCurrentStep(result, detail);
			});
	m_uartTimer->start(60000);
}

void MsvScenarioDispatcher::reset()
{
	stopPollTimer();
	stopUartMonitoring();
	ScenarioDispatcher::reset();
}

void MsvScenarioDispatcher::stopUartMonitoring()
{
	if (m_uartTimer) {
		m_uartTimer->stop();
		m_uartTimer->deleteLater();
		m_uartTimer = nullptr;
	}
	if (m_uartNoDataTimer) {
		m_uartNoDataTimer->stop();
		m_uartNoDataTimer->deleteLater();
		m_uartNoDataTimer = nullptr;
	}

	m_uartMonitor->disconnect(this);
	if (m_uartMonitor->isOpen()) {
		m_uartMonitor->close();
		m_logger->info(kSrc, "UART порт закрыт");
	}

}
void MsvScenarioDispatcher::stopPollTimer()
{
	if (m_pollTimer) {
		m_pollTimer->stop();
		m_pollTimer->deleteLater();
		m_pollTimer = nullptr;
	}
	m_pollCount = 0;
}

void MsvScenarioDispatcher::runKzMonitoring()
{
	m_logger->info(kSrc, "Мониторинг состояния при КЗ...");

	if (!m_deviceModel->isDeviceFound()) {
		finishCurrentStep(StepResult::Fail, "устройство не найдено");
		return;
	}

	// Снимок "до КЗ" — уже в модели из предыдущих шагов
	m_snapshotBeforeKz = m_deviceModel->currentSnapshot();
	const QHostAddress ip = m_snapshotBeforeKz.ipAddress;

	struct KzStats {
		int     pollsDone        {0};
		int     syncDegradedCount{0};
		bool    antennaChanged   {false};
		QString initialAntenna;
		QString lastAntenna;
		QString lastGpsStatus;
	};
	auto stats = std::make_shared<KzStats>();
	stats->initialAntenna = m_snapshotBeforeKz.antennaStatus;
	stats->lastAntenna    = m_snapshotBeforeKz.antennaStatus;

	m_pollCount = 0;
	m_maxPolls  = 9;   // 9 × 10с = 90с

	m_pollTimer = new QTimer(this);
	m_pollTimer->setInterval(10000);

	connect(m_pollTimer, &QTimer::timeout, this, [this, ip, stats]()
	{
		m_pollCount++;
		stats->pollsDone = m_pollCount;

		m_webClient->disconnect(this);
		connect(m_webClient, &Network::WebClient::finished,
				this, [this, stats](const Network::WebClient::Result& res)
				{
					m_webClient->disconnect(this);

					// Обновляем модель
					Core::WebStatusData data;
					data.syncSource    = res.syncSource;
					data.syncStatus    = res.syncStatus;
					data.webTime       = res.webTime;
					data.macAddress    = res.macAddress;
					data.gpsTime       = res.gpsTime;
					data.gpsDate       = res.gpsDate;
					data.gpsStatus     = res.gpsStatus;
					data.gpsMode       = res.gpsMode;
					data.gpsSatellites = res.gpsSatellites;
					data.gpsSignalLevel= res.gpsSignalLevel;
					data.rtcTime       = res.rtcTime;
					data.rtcDate       = res.rtcDate;
					data.rtcValidity   = res.rtcValidity;
					data.antennaStatus = res.antennaStatus;
					data.lastValidGps  = res.lastValidGps;
					data.sntpSyncCount = res.sntpSyncCount;
					data.uptime        = res.uptime;
					data.buildDate     = res.buildDate;
					data.crc           = res.crc;
					m_deviceModel->applyWebStatus(data);

					// Отслеживаем изменения
					if (res.antennaStatus != stats->lastAntenna) {
						stats->antennaChanged = true;
						m_logger->info(kSrc, QStringLiteral("Антенна: '%1' → '%2'")
								.arg(stats->lastAntenna, res.antennaStatus));
						stats->lastAntenna = res.antennaStatus;
					}
					if (res.syncStatus == Core::SyncStatus::Freerun ||
						res.syncStatus == Core::SyncStatus::Holdover)
						stats->syncDegradedCount++;

					stats->lastGpsStatus = res.gpsStatus;

					m_logger->debug(kSrc, QStringLiteral(
							"КЗ опрос %1/%2: антенна='%3' sync=%4")
							.arg(stats->pollsDone).arg(m_maxPolls)
							.arg(res.antennaStatus)
							.arg(static_cast<int>(res.syncStatus)));

					// Конец мониторинга
					if (stats->pollsDone >= m_maxPolls) {
						stopPollTimer();
						m_snapshotDuringKz = m_deviceModel->currentSnapshot();

						StepResult result;
						QString    detail;

						if (stats->antennaChanged && stats->syncDegradedCount > 0) {
							result = StepResult::Pass;
							detail = QStringLiteral(
									"КЗ корректно отражено: антенна='%1', деградация sync в %2/%3 опросах")
									.arg(stats->lastAntenna)
									.arg(stats->syncDegradedCount)
									.arg(m_maxPolls);
						} else if(stats->antennaChanged || stats->syncDegradedCount > 0) {
							result = StepResult::Warning;
							detail = QStringLiteral(
									"частичное отражение КЗ: антенна=%1 sync_деградация=%2")
									.arg(stats->antennaChanged ? "изменилась" : "не изменилась")
									.arg(stats->syncDegradedCount > 0 ? "да" : "нет");
						} else {
							result = StepResult::Warning;
							detail = "изделие не отразило КЗ в web-статусе";
						}

						finishCurrentStep(result, detail);
					}
				}, Qt::QueuedConnection);

		connect(m_webClient, &Network::WebClient::failed,
				this, [this](const QString& reason) {
					m_webClient->disconnect(this);
					m_logger->warning(kSrc, QStringLiteral("КЗ опрос failed: %1").arg(reason));
				}, Qt::QueuedConnection);

		m_webClient->request(ip, "/tags.shtml");
	});

	m_pollTimer->start();
}

void MsvScenarioDispatcher::runKzRecovery()
{
	m_logger->info(kSrc, "Ожидание восстановления GNSS...");

	if (!m_deviceModel->isDeviceFound()) {
		finishCurrentStep(StepResult::Fail, "устройство не найдено");
		return;
	}

	const QHostAddress ip = m_deviceModel->currentSnapshot().ipAddress;

	m_pollCount = 0;
	m_maxPolls  = 20;  // 20 × 15с = 300с

	m_pollTimer = new QTimer(this);
	m_pollTimer->setInterval(15000);

	connect(m_pollTimer, &QTimer::timeout, this, [this, ip]()
	{
		m_pollCount++;

		m_webClient->disconnect(this);
		connect(m_webClient, &Network::WebClient::finished,
				this, [this](const Network::WebClient::Result& res)
				{
					m_webClient->disconnect(this);

					Core::WebStatusData data;
					data.syncSource    = res.syncSource;
					data.syncStatus    = res.syncStatus;
					data.webTime       = res.webTime;
					data.antennaStatus = res.antennaStatus;
					data.gpsStatus     = res.gpsStatus;
					data.rtcValidity   = res.rtcValidity;
					m_deviceModel->applyWebStatus(data);

					const bool recovered =
							res.syncStatus == Core::SyncStatus::Synchronized ||
							res.gpsStatus.startsWith("A");

					m_logger->info(kSrc, QStringLiteral(
							"Восстановление %1/%2: gps='%3' sync=%4")
							.arg(m_pollCount).arg(m_maxPolls)
							.arg(res.gpsStatus)
							.arg(static_cast<int>(res.syncStatus)));

					if (recovered) {
						stopPollTimer();
						finishCurrentStep(StepResult::Pass,
										  QStringLiteral("GNSS восстановлен через ~%1 с")
												  .arg(m_pollCount * 15));
						return;
					}

					if (m_pollCount >= m_maxPolls) {
						stopPollTimer();
						finishCurrentStep(StepResult::Warning,
										  "GNSS не восстановился за 300 с");
					}
				}, Qt::QueuedConnection);

		connect(m_webClient, &Network::WebClient::failed,
				this, [this](const QString& reason) {
					m_webClient->disconnect(this);
					m_logger->warning(kSrc, QStringLiteral("Опрос восстановления failed: %1")
							.arg(reason));
				}, Qt::QueuedConnection);

		m_webClient->request(ip, "/tags.shtml");
	});

	m_pollTimer->start();
}

} // namespace Msv::Core
