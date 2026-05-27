#pragma once

#include "IDeviceModel.h"
#include <QMutex>


namespace Msv::Core {

class DeviceModel final : public IDeviceModel {
    Q_OBJECT
public:
    explicit DeviceModel(QObject* parent = nullptr);
    ~DeviceModel() override = default;

    [[nodiscard]] DeviceSnapshot currentSnapshot() const override;
    [[nodiscard]] bool           isDeviceFound()   const override;

    void applyWhoIAmData(const QHostAddress& ip,
                         const QString&      firmware,
                         int                 deviceId,
                         const QString&      deviceName) override;

    void applySntpData  (const QDateTime& time, int leapIndicator, int stratum) override;
    void applyWebStatus (const WebStatusData& data) override;
    void applyGnssTime  (const QDateTime& utcTime) override;
    void reset          () override;

private:
    mutable QMutex m_mutex;
    DeviceSnapshot m_snapshot;
    bool           m_deviceFound{false};
};

} // namespace Msv::Core
