#pragma once

#include "IDeviceModel.h"
#include "ILogger.h"
#include "network/WebClient.h"
#include "network/SntpClient.h"
#include "serial/UartMonitor.h"
#include <QObject>
#include <QList>
#include <QDateTime>
#include <memory>

QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

namespace Msv::Core {

// ─────────────────────────────────────────────────────────────────────────────
/// Одна точка замера за период длительного теста.
// ─────────────────────────────────────────────────────────────────────────────
struct LongRunSample {
    QDateTime pcTime;            ///< Время ПК в момент замера

    // Web
    QDateTime    webTime;
    SyncSource   syncSource   {SyncSource::Unknown};
    SyncStatus   syncStatus   {SyncStatus::Unknown};
    QString      antennaStatus;
    QString      gpsStatus;
    int          stratum       {0};

    // SNTP
    QDateTime    sntpTime;
    int          leapIndicator {0};
    double       rttMs         {0.0};

    // GNSS UART (агрегат за интервал)
    QDateTime    gnssTime;
    bool         gnssValid     {false};
    int          nmeaTotal     {0};
    int          nmeaOk        {0};
    int          monotonErrors {0};
};


struct OffsetStats {
	qint64 minMs   {0};
	qint64 maxMs   {0};
	double meanMs  {0.0};
	double stdDevMs{0.0};
	int    count   {0};

	[[nodiscard]] qint64 rangeMs() const { return maxMs - minMs; }
};

// ─────────────────────────────────────────────────────────────────────────────
/// Итоговый результат длительного теста.
// ─────────────────────────────────────────────────────────────────────────────
struct LongRunResult {
    bool   completed        {false};  ///< false = прерван
    int    durationMinutes  {0};
    int    samplesCollected {0};

    // ── Монотонность ─────────────────────────────────────────────────────────
    int    webMonotonErrors  {0};
    int    sntpMonotonErrors {0};
    int    gnssMonotonErrors {0};

    // ── Расхождение с ПК (мс), только для валидных выборок ───────────────────
	OffsetStats webOffset;
	OffsetStats sntpOffset;
	OffsetStats gnssOffset;
	OffsetStats webVsGnss;

    // ── NMEA статистика ───────────────────────────────────────────────────────
    int    nmeaTotal         {0};
    int    nmeaOk            {0};
    int    nmeaMonotonErrors {0};

    // ── Расхождение Web vs GNSS (мс) ─────────────────────────────────────────


    // ── Изменения антенны / статуса ───────────────────────────────────────────
    int    antennaChanges    {0};
    int    syncStatusChanges {0};

    QList<LongRunSample> samples;
};

// ─────────────────────────────────────────────────────────────────────────────
/// Длительный мониторинг Web + SNTP + UART за заданное время.
///
/// Использует переданные клиенты. UART-порт должен быть уже открыт
/// (или не открыт — тогда GNSS-данные будут отсутствовать).
// ─────────────────────────────────────────────────────────────────────────────
class LongRunMonitor : public QObject {
    Q_OBJECT
public:
    explicit LongRunMonitor(IDeviceModel*                   deviceModel,
                            Network::WebClient*             webClient,
                            Network::SntpClient*            sntpClient,
                            Serial::UartMonitor*            uartMonitor,
                            std::shared_ptr<ILogger>        logger,
                            QObject*                        parent = nullptr);

    void start(int durationMinutes, const QHostAddress& deviceIp);
    void stop();

    [[nodiscard]] bool isRunning() const { return m_running; }

signals:
    /// Обновление прогресса каждые ~30 секунд.
    void progressUpdate(int elapsedSec, int totalSec, const QString& liveStats);

    /// Тест завершён (по времени или остановлен).
    void finished(const Msv::Core::LongRunResult& result);

	void displayUpdate(const QString& stats);

private slots:
    void onPollTick();
    void onFinished();
    void onNmeaReceived(const Msv::Serial::NmeaResult& result);

private:
    void collectSample();
    void analyzeSamples();
    QString buildLiveStats() const;

    static qint64 offsetMs(const QDateTime& src, const QDateTime& captured);

    IDeviceModel*            m_deviceModel {nullptr};
    Network::WebClient*      m_webClient   {nullptr};
    Network::SntpClient*     m_sntpClient  {nullptr};
    Serial::UartMonitor*     m_uartMonitor {nullptr};
    std::shared_ptr<ILogger> m_logger;

    QTimer*      m_pollTimer     {nullptr};
    QTimer*      m_finishTimer   {nullptr};
    QHostAddress m_deviceIp;
    bool         m_running       {false};

    // Текущий интервал UART
    struct UartInterval {
        int       total       {0};
        int       checksumOk  {0};
        int       monotonErrs {0};
        QDateTime lastUtc;
        QDateTime lastGnssTime;
        bool      lastValid   {false};
    } m_uartInterval;

    // Накопленные данные
    LongRunResult    m_result;
    LongRunSample    m_pendingSample;  ///< Собирается асинхронно
    bool             m_webPending  {false};
    bool             m_sntpPending {false};
    QDateTime        m_startTime;
    int              m_totalSec    {0};
	QDateTime m_prevWebTime;
	QDateTime m_prevSntpTime;
	QDateTime m_prevGnssTime;

    QString          m_lastAntennaStatus;
    SyncStatus       m_lastSyncStatus {SyncStatus::Unknown};

	QTimer*			 m_displayTimer {nullptr};

    static constexpr const char* kSrc = "LongRunMonitor";
};

} // namespace Msv::Core
