#pragma once

#include <QObject>
#include <QString>
#include <QHostAddress>
#include <QDateTime>
#include <optional>

namespace Msv::Core {

// ─────────────────────────────────────────────────────────────────────────────
/// Источник синхронизации времени изделия.
// ─────────────────────────────────────────────────────────────────────────────
enum class SyncSource : int {
    Unknown  = 0,
    GNSS     = 1,  ///< Синхронизация от спутника (штатный режим)
    NTP      = 2,  ///< Внешний NTP-сервер
    Internal = 3,  ///< Внутренний генератор (нет внешней синхронизации)
};

// ─────────────────────────────────────────────────────────────────────────────
/// Статус синхронизации изделия.
// ─────────────────────────────────────────────────────────────────────────────
enum class SyncStatus : int {
    Unknown     = 0,
    Synchronized = 1,   ///< Изделие синхронизировано
    Holdover    = 2,    ///< Удержание (потерян источник, идёт счёт по ГГ)
    Freerun     = 3,    ///< Свободный ход, синхронизация отсутствует
};

// ─────────────────────────────────────────────────────────────────────────────
/// Снимок состояния изделия МСВ в конкретный момент времени.
/// Используется для фиксации «до/во время/после» воздействия.
// ─────────────────────────────────────────────────────────────────────────────
struct DeviceSnapshot {
    QDateTime   capturedAt;          ///< Момент снятия снимка (UTC ПК)

    // — Идентификация ─────────────────────────────────────────────────────────
    QHostAddress ipAddress;
    QString      macAddress;         ///< "AA:BB:CC:DD:EE:FF" (если доступен)
    int          deviceId       {0}; ///< ID из протокола WhoIAm (напр. 20401)
    QString      deviceName;         ///< Человекочитаемое имя по deviceId
    QString      firmwareVersion;    ///< Из ответа WhoIAm / Web

    // — Синхронизация ─────────────────────────────────────────────────────────
    SyncSource   syncSource  {SyncSource::Unknown};
    SyncStatus   syncStatus  {SyncStatus::Unknown};
    int          stratumLevel{0};    ///< NTP stratum (0 = неизвестно)
    int          leapIndicator{0};   ///< NTP LI (0=ok, 1=+1с, 2=-1с, 3=alarm)

    // — Времена ───────────────────────────────────────────────────────────────
    std::optional<QDateTime> webTime;   ///< Время из HTTP-ответа
    std::optional<QDateTime> sntpTime;  ///< Время из SNTP-ответа
    std::optional<QDateTime> gnssTime;  ///< Время из NMEA RMC/GGA

	// — GPS-данные ──────────────────────────────────────────────────────────────
	QString      gpsTime;             // "t_gps"
	QString      gpsDate;             // "d_gps"
	QString      gpsStatus;           // "st_gps": "A - решение годно" / "V - ..."
	QString      gpsMode;             // "lm_gps"
	QString      gpsSatellites;       // "vsn": "0/0"
	QString      gpsSignalLevel;      // "sss"

	// — RTC-данные ──────────────────────────────────────────────────────────────
	QString      rtcTime;             // "t_rtc"
	QString      rtcDate;             // "d_rtc"
	QString      rtcValidity;         // "iv_rtc": "Данные валидны" / "не валидны"

	// — Сервисные данные ────────────────────────────────────────────────────────
	QString      antennaStatus;       // "as": "Антенна подключена" / "не подключена"
	QString      lastValidGps;        // "lr": время последнего валидного GPS
	QString      sntpSyncCount;       // "ssn": число успешных SNTP-синхронизаций
	QString      uptime;              // "wt"

	// — Версия и сборка ─────────────────────────────────────────────────────────
	QString      buildDate;           // "sbd"
	QString      crc;                 // "crc"


    // — Флаг валидности ───────────────────────────────────────────────────────
    [[nodiscard]] bool isIdentified() const {
        return !ipAddress.isNull() && !firmwareVersion.isEmpty();
    }
};

struct WebStatusData {
	SyncSource syncSource {SyncSource::Unknown};
	SyncStatus syncStatus {SyncStatus::Unknown};
	QDateTime  webTime;
	QString    macAddress;
	QString    gpsTime;      QString gpsDate;    QString gpsStatus;
	QString    gpsMode;      QString gpsSatellites; QString gpsSignalLevel;
	QString    rtcTime;      QString rtcDate;    QString rtcValidity;
	QString    antennaStatus;
	QString    lastValidGps; QString sntpSyncCount; QString uptime;
	QString    buildDate;    QString crc;
};

// ─────────────────────────────────────────────────────────────────────────────
/// Абстрактная модель данных изделия.
///
/// Наследуется от QObject: UI-слой подписывается на сигналы изменений
/// через стандартную систему сигналов/слотов, не зная о конкретной реализации.
// ─────────────────────────────────────────────────────────────────────────────
class IDeviceModel : public QObject {
    Q_OBJECT
public:
    explicit IDeviceModel(QObject* parent = nullptr) : QObject(parent) {}
    ~IDeviceModel() override = default;

    // ── Чтение состояния ──────────────────────────────────────────────────────
    [[nodiscard]] virtual DeviceSnapshot currentSnapshot() const = 0;
    [[nodiscard]] virtual bool isDeviceFound() const = 0;

    // ── Запись состояния (вызывается подсистемами, не UI) ─────────────────────
    virtual void applyWhoIAmData  (const QHostAddress& ip,
                                   const QString&      firmware,
                                   int                 deviceId,
                                   const QString&      deviceName) = 0;

    virtual void applySntpData    (const QDateTime& time,
                                   int leapIndicator,
                                   int stratum) = 0;

    virtual void applyWebStatus   (const WebStatusData& data) = 0;

    virtual void applyGnssTime    (const QDateTime& utcTime) = 0;

    virtual void reset            () = 0;  ///< Сброс к начальному состоянию

signals:
    /// Эмитируется при любом изменении снимка (UI обновляет отображение).
    void snapshotChanged(const DeviceSnapshot& snapshot);

    /// Устройство обнаружено в сети первый раз (или после reset()).
    void deviceFound(const DeviceSnapshot& snapshot);

    /// Потеря связи с устройством (нет ответа дольше таймаута).
    void deviceLost();
};

} // namespace Msv::Core
