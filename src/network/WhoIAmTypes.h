#pragma once

#include <QHostAddress>
#include <QString>
#include <QByteArray>

namespace Msv::Network {

// ─────────────────────────────────────────────────────────────────────────────
/// Конфигурация сканирования.
///
/// Протокол (по умолчанию):
///   Запрос:  "MSVWHOIAM?\r\n"
///   Ответ:   "MSVWHOIAM;MAC=AA:BB:..;FW=1.2.3;SN=12345678\r\n"
///
/// IP берётся из заголовка UDP-датаграммы (адрес отправителя).
/// Всё остальное — из тела ответа. Формат легко меняется переопределением
/// WhoIAmScanner::parseResponse().
// ─────────────────────────────────────────────────────────────────────────────
struct WhoIAmConfig {
    quint16    port            {54321};
    int        retries         {3};       ///< Сколько раз слать broadcast
    int        retryIntervalMs {2000};    ///< Интервал между попытками
    QByteArray requestPayload  {"MSVWHOIAM?\r\n"};
};

// ─────────────────────────────────────────────────────────────────────────────
/// Разобранный ответ от одного МСВ.
// ─────────────────────────────────────────────────────────────────────────────
struct WhoIAmResponse {
    QHostAddress ipAddress;        ///< IP из UDP-заголовка
    QString      macAddress;
    QString      firmwareVersion;
    QString      serialNumber;
    QByteArray   rawData;          ///< Сырой payload для отладки
};

} // namespace Msv::Network
