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

}

void MsvScenarioDispatcher::executeStep(int stepIndex)
{
    switch (stepIndex) {
        case 0: runWhoIAm(); break;
		case 1: runWebStatus(); break;
		case 2: runSntp();      break;
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

} // namespace Msv::Core
