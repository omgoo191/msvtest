#pragma once

#include <QHostAddress>
#include <QString>
#include <QByteArray>
#include <QMap>

namespace Msv::Network {

struct WhoIAmConfig {
    quint16    port            {30000};
    int        retries         {3};
    int        retryIntervalMs {2000};
    QByteArray requestPayload  {"WHO IS ENCORE?"};
};

// ─────────────────────────────────────────────────────────────────────────────
/// Разобранный ответ от одного устройства.
// ─────────────────────────────────────────────────────────────────────────────
struct WhoIAmResponse {
    QHostAddress ipAddress;
    int          deviceId        {0};
    QString      firmwareVersion;
    QByteArray   rawData;

    /// Человекочитаемое имя по ID.
    [[nodiscard]] QString deviceName() const
    {
        static const QMap<int, QString> names {
            {20401, "Модуль синхронизации времени"},
            {20402, "Устройство 2"},
            {20403, "Устройство 3"},
        };
        return names.value(deviceId,
            QStringLiteral("Неизвестное устройство (ID %1)").arg(deviceId));
    }
};

} // namespace Msv::Network
