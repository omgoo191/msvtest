#pragma once

#include "IDeviceModel.h"
#include <QMutex>

namespace Msv::Core {

// ─────────────────────────────────────────────────────────────────────────────
/// Конкретная реализация IDeviceModel.
///
/// Thread-safety: методы apply*() могут вызываться из рабочих потоков
/// (сетевых, серийных), поэтому внутри используется QMutex.
/// Сигналы эмитируются всегда в GUI-потоке через Qt::QueuedConnection.
// ─────────────────────────────────────────────────────────────────────────────
class DeviceModel final : public IDeviceModel {
    Q_OBJECT
public:
    explicit DeviceModel(QObject* parent = nullptr);
    ~DeviceModel() override = default;

    // IDeviceModel interface
    [[nodiscard]] DeviceSnapshot currentSnapshot() const override;
    [[nodiscard]] bool           isDeviceFound()   const override;

    void applyWhoIAmData(const QHostAddress& ip,
                         const QString&      mac,
                         const QString&      firmware,
                         const QString&      serial) override;

    void applySntpData  (const QDateTime& time,
                         int leapIndicator,
                         int stratum) override;

    void applyWebStatus (SyncSource source,
                         SyncStatus status,
                         const QDateTime& webTime) override;

    void applyGnssTime  (const QDateTime& utcTime) override;

    void reset          () override;

private:
    mutable QMutex m_mutex;
    DeviceSnapshot m_snapshot;
    bool           m_deviceFound{false};
};

} // namespace Msv::Core
