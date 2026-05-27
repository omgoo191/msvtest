#pragma once

#include <QString>
#include <QDateTime>
#include <QByteArray>

namespace Msv::Serial {

// ─────────────────────────────────────────────────────────────────────────────
/// Результат разбора одного NMEA-предложения.
// ─────────────────────────────────────────────────────────────────────────────
struct NmeaResult {
    // — Общее ─────────────────────────────────────────────────────────────────
    QString   sentenceType;      ///< "RMC", "GGA", "GSA", ...
    bool      checksumValid{false};
    QString   rawSentence;

    // — Время ─────────────────────────────────────────────────────────────────
    QDateTime utcTime;           ///< UTC из поля предложения
    bool      isValid{false};    ///< A=valid, V=warning (для RMC)

    // — Позиция (GGA/RMC) ─────────────────────────────────────────────────────
    double    latitude  {0.0};
    double    longitude {0.0};
    double    altitude  {0.0};
    int       satellites{0};
    int       fixQuality{0};     ///< GGA: 0=no fix, 1=GPS, 2=DGPS
};

// ─────────────────────────────────────────────────────────────────────────────
/// Статический парсер NMEA-предложений.
// ─────────────────────────────────────────────────────────────────────────────
class NmeaParser {
public:
    NmeaParser() = delete;

    /// Проверить контрольную сумму предложения.
    static bool verifyChecksum(const QByteArray& sentence);

    /// Разобрать предложение. Checksum проверяется внутри.
    static NmeaResult parse(const QByteArray& sentence);

    /// Тип предложения без талкер-префикса: "$GPRMC" → "RMC".
    static QString sentenceType(const QByteArray& sentence);

private:
    static NmeaResult parseRmc(const QStringList& fields, const QByteArray& raw);
    static NmeaResult parseGga(const QStringList& fields, const QByteArray& raw);

    /// Разобрать время NMEA: "HHMMSS.SS" + опциональная дата "DDMMYY".
    static QDateTime parseUtcTime(const QString& timeStr,
                                  const QString& dateStr = {});

    /// NMEA координата "DDDMM.MMMM" → десятичные градусы.
    static double parseCoord(const QString& coord, const QString& hemisphere);
};

} // namespace Msv::Serial
