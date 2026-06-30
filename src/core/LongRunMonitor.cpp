#include "LongRunMonitor.h"
#include "serial/NmeaParser.h"
#include <QTimer>
#include <algorithm>
#include <cmath>

namespace Msv::Core {

LongRunMonitor::LongRunMonitor(IDeviceModel*            deviceModel,
                                Network::WebClient*      webClient,
                                Network::SntpClient*     sntpClient,
                                Serial::UartMonitor*     uartMonitor,
                                std::shared_ptr<ILogger> logger,
                                QObject*                 parent)
    : QObject(parent)
    , m_deviceModel(deviceModel)
    , m_webClient(webClient)
    , m_sntpClient(sntpClient)
    , m_uartMonitor(uartMonitor)
    , m_logger(std::move(logger))
{
    m_pollTimer   = new QTimer(this);
    m_finishTimer = new QTimer(this);
	m_tickTimer =  new QTimer(this);
	m_tickTimer->setInterval(1000);
    m_pollTimer->setSingleShot(false);
    m_finishTimer->setSingleShot(true);

    connect(m_pollTimer,   &QTimer::timeout, this, &LongRunMonitor::onPollTick);
    connect(m_finishTimer, &QTimer::timeout, this, &LongRunMonitor::onFinished);
	connect(m_tickTimer, &QTimer::timeout, this, [this]() {
		if(!m_running) return ;
		const int elapsed = static_cast<int>(
				m_startTime.secsTo(QDateTime::currentDateTimeUtc()));
		qDebug() << "TICK emit" << elapsed;
		emit tick(elapsed, m_totalSec);
	});

	m_displayTimer = new QTimer(this);
	m_displayTimer->setInterval(1000);
	connect(m_displayTimer, &QTimer::timeout, this, [this]() {
		if (!m_running) return;
		// Берём текущий снимок из модели напрямую
		const auto snap = m_deviceModel->currentSnapshot();
		const int elapsed = static_cast<int>(
				m_startTime.secsTo(QDateTime::currentDateTimeUtc()));

		const QString stats = QStringLiteral(
				"Замеров: %1  |  Прошло: %2 мин\n"
				"Антенна: %3\n"
				"Sync: %4\n"
				"Web время: %5\n"
				"SNTP время: %6   stratum=%7  LI=%8\n"
				"GNSS UTC: %9\n"
				"NMEA checksum: %10/%11\n"
				"Ошибок монотонности: web=%12 sntp=%13 gnss=%14")
				.arg(m_result.samplesCollected)
				.arg(elapsed / 60)
				.arg(snap.antennaStatus.isEmpty() ? "—" : snap.antennaStatus)
				.arg(snap.syncStatus == SyncStatus::Synchronized ? "Synchronized" :
					 snap.syncStatus == SyncStatus::Holdover     ? "Holdover"     :
					 snap.syncStatus == SyncStatus::Freerun      ? "Freerun"      : "—")
				.arg(snap.webTime.value_or(QDateTime{}).isValid()
					 ? snap.webTime.value_or(QDateTime{}).toString("HH:mm:ss") : "—")
				.arg(snap.sntpTime.value_or(QDateTime{}).isValid()
					 ? snap.sntpTime.value_or(QDateTime{}).toString("HH:mm:ss") : "—")
				.arg(snap.stratumLevel).arg(snap.leapIndicator)
				.arg(snap.gnssTime.value_or(QDateTime{}).isValid()
					 ? snap.gnssTime.value_or(QDateTime{}).toString("HH:mm:ss") : "—")
				.arg(m_result.nmeaOk).arg(m_result.nmeaTotal)
				.arg(m_result.webMonotonErrors)
				.arg(m_result.sntpMonotonErrors)
				.arg(m_result.gnssMonotonErrors);

		emit displayUpdate(stats);
	});
}

void LongRunMonitor::start(int durationMinutes, const QHostAddress& deviceIp)
{
    if (m_running) stop();
	m_displayTimer->start();
    m_deviceIp  = deviceIp;
    m_running   = true;
    m_startTime = QDateTime::currentDateTimeUtc();
    m_totalSec  = durationMinutes * 60;

    m_result = LongRunResult{};
    m_result.durationMinutes = durationMinutes;
    m_uartInterval = UartInterval{};

    m_lastAntennaStatus.clear();
    m_lastSyncStatus = SyncStatus::Unknown;

    // Подписка на UART
	if (m_uartMonitor) {
		m_uartMonitor->disconnect(this);
		if (m_uartMonitor->isOpen()) {
			connect(m_uartMonitor, &Serial::UartMonitor::sentenceReceived,
					this, &LongRunMonitor::onNmeaReceived);
			m_logger->info(kSrc, QStringLiteral("UART подключён: %1")
					.arg(m_uartMonitor->portName()));
		} else {
			m_logger->warning(kSrc, "UART порт не открыт — GNSS данные будут н/д");
		}
	}

    m_logger->info(kSrc, QStringLiteral("Длительный тест запущен: %1 мин")
        .arg(durationMinutes));

    m_pollTimer->start(30000);           // опрос каждые 30 секунд
    m_finishTimer->start(m_totalSec * 1000);
	m_tickTimer->start();
    // Первый опрос сразу
    onPollTick();
}

void LongRunMonitor::stop()
{
    if (!m_running) return;
	m_displayTimer->stop();
    m_running = false;
    m_pollTimer->stop();
    m_finishTimer->stop();
	m_tickTimer->stop();
    if (m_uartMonitor) m_uartMonitor->disconnect(this);
    m_webClient->disconnect(this);
    m_sntpClient->disconnect(this);
    m_logger->info(kSrc, "Длительный тест остановлен");
}

// ─────────────────────────────────────────────────────────────────────────────

void LongRunMonitor::onPollTick()
{
    if (!m_running) return;

    m_pendingSample = LongRunSample{};
    m_pendingSample.pcTime = QDateTime::currentDateTimeUtc();
    m_webPending  = true;
    m_sntpPending = true;

    // UART — берём накопленный интервал
    m_pendingSample.gnssTime      = m_uartInterval.lastGnssTime;
    m_pendingSample.gnssValid     = m_uartInterval.lastValid;
    m_pendingSample.nmeaTotal     = m_uartInterval.total;
    m_pendingSample.nmeaOk        = m_uartInterval.checksumOk;
    m_pendingSample.monotonErrors = m_uartInterval.monotonErrs;
    m_uartInterval = UartInterval{};  // сброс на следующий интервал

    // Web запрос
    m_webClient->disconnect(this);
    connect(m_webClient, &Network::WebClient::finished,
            this, [this](const Network::WebClient::Result& res)
    {
        m_webClient->disconnect(this);
        m_pendingSample.webTime      = res.webTime;
        m_pendingSample.syncSource   = res.syncSource;
        m_pendingSample.syncStatus   = res.syncStatus;
        m_pendingSample.antennaStatus= res.antennaStatus;
        m_pendingSample.gpsStatus    = res.gpsStatus;
        m_webPending = false;

        if (!m_sntpPending) collectSample();
    }, Qt::QueuedConnection);

    connect(m_webClient, &Network::WebClient::failed,
            this, [this](const QString&)
    {
        m_webClient->disconnect(this);
        m_webPending = false;
        if (!m_sntpPending) collectSample();
    }, Qt::QueuedConnection);

    m_webClient->request(m_deviceIp, "/tags.shtml");

    // SNTP запрос
    m_sntpClient->disconnect(this);
    connect(m_sntpClient, &Network::SntpClient::finished,
            this, [this](const Network::SntpClient::Result& res)
    {
        m_sntpClient->disconnect(this);
        m_pendingSample.sntpTime      = res.transmitTime;
        m_pendingSample.leapIndicator = res.leapIndicator;
        m_pendingSample.rttMs         = res.roundTripMs;
        m_pendingSample.stratum       = res.stratum;
		m_deviceModel->applySntpData(res.transmitTime,
									 res.leapIndicator,
									 res.stratum);
        m_sntpPending = false;

        if (!m_webPending) collectSample();
    }, Qt::QueuedConnection);

    connect(m_sntpClient, &Network::SntpClient::failed,
            this, [this](const QString&)
    {
        m_sntpClient->disconnect(this);
        m_sntpPending = false;
        if (!m_webPending) collectSample();
    }, Qt::QueuedConnection);

    m_sntpClient->request(m_deviceIp);
}

void LongRunMonitor::collectSample()
{
    m_result.samples.append(m_pendingSample);
	// Монотонность Web
	if (m_pendingSample.webTime.isValid()) {
		if (m_prevWebTime.isValid() && m_pendingSample.webTime <= m_prevWebTime)
			m_result.webMonotonErrors++;
		m_prevWebTime = m_pendingSample.webTime;
	}

// Монотонность SNTP
	if (m_pendingSample.sntpTime.isValid()) {
		if (m_prevSntpTime.isValid() && m_pendingSample.sntpTime <= m_prevSntpTime)
			m_result.sntpMonotonErrors++;
		m_prevSntpTime = m_pendingSample.sntpTime;
	}

// Монотонность GNSS
	if (m_pendingSample.gnssTime.isValid()) {
		if (m_prevGnssTime.isValid() && m_pendingSample.gnssTime <= m_prevGnssTime)
			m_result.gnssMonotonErrors++;
		m_prevGnssTime = m_pendingSample.gnssTime;
	}

    m_result.samplesCollected++;

    // Накапливаем NMEA статистику
    m_result.nmeaTotal        += m_pendingSample.nmeaTotal;
    m_result.nmeaOk           += m_pendingSample.nmeaOk;
    m_result.nmeaMonotonErrors+= m_pendingSample.monotonErrors;

    // Изменения антенны
    if (!m_pendingSample.antennaStatus.isEmpty() &&
        !m_lastAntennaStatus.isEmpty() &&
        m_pendingSample.antennaStatus != m_lastAntennaStatus)
    {
        m_result.antennaChanges++;
        m_logger->info(kSrc, QStringLiteral("Антенна изменилась: %1 → %2")
            .arg(m_lastAntennaStatus, m_pendingSample.antennaStatus));
    }
    m_lastAntennaStatus = m_pendingSample.antennaStatus;

    // Изменения статуса синхронизации
    if (m_pendingSample.syncStatus != SyncStatus::Unknown &&
        m_lastSyncStatus           != SyncStatus::Unknown &&
        m_pendingSample.syncStatus != m_lastSyncStatus)
    {
        m_result.syncStatusChanges++;
    }
    m_lastSyncStatus = m_pendingSample.syncStatus;

    // Эмит прогресса
    const int elapsed = static_cast<int>(
        m_startTime.secsTo(QDateTime::currentDateTimeUtc()));
    emit progressUpdate(elapsed, m_totalSec, buildLiveStats());
}

void LongRunMonitor::onNmeaReceived(const Serial::NmeaResult& result)
{
    m_uartInterval.total++;
    if (!result.checksumValid) return;
    m_uartInterval.checksumOk++;

    if (result.utcTime.isValid()) {
        if (m_uartInterval.lastUtc.isValid() &&
            result.utcTime <= m_uartInterval.lastUtc)
        {
            m_uartInterval.monotonErrs++;
        }
        m_uartInterval.lastUtc = result.utcTime;
    }

    if (result.isValid && result.utcTime.isValid()) {
        m_uartInterval.lastGnssTime = result.utcTime;
        m_uartInterval.lastValid    = true;
    }
}

void LongRunMonitor::onFinished()
{
    m_running = false;
    m_pollTimer->stop();
	m_tickTimer->stop();
	m_displayTimer->stop();
    if (m_uartMonitor) m_uartMonitor->disconnect(this);
    m_webClient->disconnect(this);
    m_sntpClient->disconnect(this);

    m_result.completed = true;
    analyzeSamples();

    m_logger->info(kSrc, QStringLiteral(
        "Длительный тест завершён: %1 замеров, "
        "NMEA ok=%2/%3, моно_ошибок: web=%4 sntp=%5 gnss=%6")
        .arg(m_result.samplesCollected)
        .arg(m_result.nmeaOk).arg(m_result.nmeaTotal)
        .arg(m_result.webMonotonErrors)
        .arg(m_result.sntpMonotonErrors)
        .arg(m_result.gnssMonotonErrors));

	emit progressUpdate(m_totalSec, m_totalSec, buildLiveStats());
    emit finished(m_result);
}

void LongRunMonitor::analyzeSamples()
{
	if (m_result.samples.isEmpty()) return;

	// Собираем выборки offset по каждому источнику
	QList<qint64> webOffs, sntpOffs, gnssOffs, wgDiffs;

	for (const auto& s : m_result.samples) {
		if (s.webTime.isValid())
			webOffs.append(s.pcTime.msecsTo(s.webTime));
		if (s.sntpTime.isValid())
			sntpOffs.append(s.pcTime.msecsTo(s.sntpTime));
		if (s.gnssTime.isValid())
			gnssOffs.append(s.pcTime.msecsTo(s.gnssTime));
		if (s.webTime.isValid() && s.gnssTime.isValid())
			wgDiffs.append(s.gnssTime.msecsTo(s.webTime));
	}

	auto computeStats = [](const QList<qint64>& vals) -> OffsetStats {
		OffsetStats st;
		st.count = vals.size();
		if (vals.isEmpty()) return st;

		st.minMs = *std::min_element(vals.begin(), vals.end());
		st.maxMs = *std::max_element(vals.begin(), vals.end());

		double sum = 0.0;
		for (qint64 v : vals) sum += v;
		st.meanMs = sum / vals.size();

		// СКО
		double sqSum = 0.0;
		for (qint64 v : vals) {
			const double d = v - st.meanMs;
			sqSum += d * d;
		}
		st.stdDevMs = vals.size() > 1
					  ? std::sqrt(sqSum / (vals.size() - 1))
					  : 0.0;

		return st;
	};

	m_result.webOffset  = computeStats(webOffs);
	m_result.sntpOffset = computeStats(sntpOffs);
	m_result.gnssOffset = computeStats(gnssOffs);
	m_result.webVsGnss  = computeStats(wgDiffs);
}

QString LongRunMonitor::buildLiveStats() const
{
    const auto& s = m_result.samples.isEmpty()
        ? LongRunSample{} : m_result.samples.last();

    const double checksumPct = m_result.nmeaTotal > 0
        ? 100.0 * m_result.nmeaOk / m_result.nmeaTotal : 0.0;

    return QStringLiteral(
        "Замеров: %1\n"
        "Антенна: %2\n"
        "Sync: %3\n"
        "Web время: %4\n"
        "SNTP время: %5   stratum=%6  LI=%7\n"
        "GNSS UTC: %8\n"
        "NMEA checksum: %9/%10 (%12%)\n"
        "Ошибок монотонности: web=%11 sntp=%12 gnss=%13")
        .arg(m_result.samplesCollected)
        .arg(s.antennaStatus.isEmpty() ? "—" : s.antennaStatus)
        .arg(s.syncStatus == SyncStatus::Synchronized ? "Synchronized" :
             s.syncStatus == SyncStatus::Holdover     ? "Holdover"     :
             s.syncStatus == SyncStatus::Freerun      ? "Freerun"      : "—")
        .arg(s.webTime.isValid()  ? s.webTime.toString("HH:mm:ss")  : "—")
        .arg(s.sntpTime.isValid() ? s.sntpTime.toString("HH:mm:ss") : "—")
        .arg(s.stratum).arg(s.leapIndicator)
        .arg(s.gnssTime.isValid() ? s.gnssTime.toString("HH:mm:ss") : "—")
        .arg(m_result.nmeaOk).arg(m_result.nmeaTotal)
        .arg(QString::number(checksumPct, 'f', 1))
        .arg(m_result.webMonotonErrors)
        .arg(m_result.sntpMonotonErrors)
        .arg(m_result.gnssMonotonErrors);
}

} // namespace Msv::Core
