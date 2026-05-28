#include "DeviceModel.h"
#include <QMutexLocker>
#include <QMetaObject>

namespace Msv::Core {

DeviceModel::DeviceModel(QObject* parent) : IDeviceModel(parent) {}

DeviceSnapshot DeviceModel::currentSnapshot() const
{
    QMutexLocker lock(&m_mutex);
    return m_snapshot;
}

bool DeviceModel::isDeviceFound() const
{
    QMutexLocker lock(&m_mutex);
    return m_deviceFound;
}

void DeviceModel::applyWhoIAmData(const QHostAddress& ip,
                                   const QString&      firmware,
                                   int                 deviceId,
                                   const QString&      deviceName)
{
    DeviceSnapshot snap;
    bool firstFound = false;
    {
        QMutexLocker lock(&m_mutex);
        m_snapshot.capturedAt      = QDateTime::currentDateTimeUtc();
        m_snapshot.ipAddress       = ip;
        m_snapshot.firmwareVersion = firmware;
        m_snapshot.deviceId        = deviceId;
        m_snapshot.deviceName      = deviceName;
        if (!m_deviceFound) { m_deviceFound = true; firstFound = true; }
        snap = m_snapshot;
    }
    QMetaObject::invokeMethod(this, [this, snap, firstFound]() {
        emit snapshotChanged(snap);
        if (firstFound) emit deviceFound(snap);
    }, Qt::QueuedConnection);
}

void DeviceModel::applySntpData(const QDateTime& time, int leapIndicator, int stratum)
{
    DeviceSnapshot snap;
    {
        QMutexLocker lock(&m_mutex);
        m_snapshot.capturedAt    = QDateTime::currentDateTimeUtc();
        m_snapshot.sntpTime      = time;
        m_snapshot.leapIndicator = leapIndicator;
        m_snapshot.stratumLevel  = stratum;
		m_snapshot.sntpCapturedAt = QDateTime::currentDateTimeUtc();
        snap = m_snapshot;
    }
    QMetaObject::invokeMethod(this, [this, snap]() {
        emit snapshotChanged(snap);
    }, Qt::QueuedConnection);
}


void DeviceModel::applyGnssTime(const QDateTime& utcTime)
{
    DeviceSnapshot snap;
    {
        QMutexLocker lock(&m_mutex);
        m_snapshot.capturedAt = QDateTime::currentDateTimeUtc();
        m_snapshot.gnssTime   = utcTime;
		m_snapshot.gnssCapturedAt = QDateTime::currentDateTimeUtc();
        snap = m_snapshot;
    }
    QMetaObject::invokeMethod(this, [this, snap]() {
        emit snapshotChanged(snap);
    }, Qt::QueuedConnection);
}

void DeviceModel::reset()
{
    {
        QMutexLocker lock(&m_mutex);
        m_snapshot    = DeviceSnapshot{};
        m_deviceFound = false;
    }
    QMetaObject::invokeMethod(this, [this]() {
        emit deviceLost();
    }, Qt::QueuedConnection);
}

void DeviceModel::applyWebStatus(const Core::WebStatusData& data)
{
	DeviceSnapshot snap;
	{
		QMutexLocker lock(&m_mutex);
		m_snapshot.capturedAt     = QDateTime::currentDateTimeUtc();
		m_snapshot.syncSource     = data.syncSource;
		m_snapshot.syncStatus     = data.syncStatus;
		m_snapshot.webTime        = data.webTime;
		m_snapshot.capturedAt     = QDateTime::currentDateTimeUtc();
		m_snapshot.macAddress     = data.macAddress;
		m_snapshot.gpsTime        = data.gpsTime;
		m_snapshot.gpsDate        = data.gpsDate;
		m_snapshot.gpsStatus      = data.gpsStatus;
		m_snapshot.gpsMode        = data.gpsMode;
		m_snapshot.gpsSatellites  = data.gpsSatellites;
		m_snapshot.gpsSignalLevel = data.gpsSignalLevel;
		m_snapshot.rtcTime        = data.rtcTime;
		m_snapshot.rtcDate        = data.rtcDate;
		m_snapshot.rtcValidity    = data.rtcValidity;
		m_snapshot.antennaStatus  = data.antennaStatus;
		m_snapshot.lastValidGps   = data.lastValidGps;
		m_snapshot.sntpSyncCount  = data.sntpSyncCount;
		m_snapshot.uptime         = data.uptime;
		m_snapshot.buildDate      = data.buildDate;
		m_snapshot.crc            = data.crc;
		snap = m_snapshot;
	}
	QMetaObject::invokeMethod(this, [this, snap]() {
		emit snapshotChanged(snap);
	}, Qt::QueuedConnection);
}

} // namespace Msv::Core
