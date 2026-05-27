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
        snap = m_snapshot;
    }
    QMetaObject::invokeMethod(this, [this, snap]() {
        emit snapshotChanged(snap);
    }, Qt::QueuedConnection);
}

void DeviceModel::applyWebStatus(SyncSource source, SyncStatus status, const QDateTime& webTime)
{
    DeviceSnapshot snap;
    {
        QMutexLocker lock(&m_mutex);
        m_snapshot.capturedAt = QDateTime::currentDateTimeUtc();
        m_snapshot.syncSource = source;
        m_snapshot.syncStatus = status;
        m_snapshot.webTime    = webTime;
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

} // namespace Msv::Core
