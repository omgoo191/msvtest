#include "NmeaParser.h"
#include <QTimeZone>

namespace Msv::Serial {

bool NmeaParser::verifyChecksum(const QByteArray& sentence)
{
    const int dollar = sentence.indexOf('$');
    const int star   = sentence.lastIndexOf('*');
    if (dollar < 0 || star < 0 || star <= dollar) return false;
    if (sentence.size() < star + 3) return false;

    quint8 calc = 0;
    for (int i = dollar + 1; i < star; ++i)
        calc ^= static_cast<quint8>(sentence[i]);

    bool ok = false;
    const quint8 expected = sentence.mid(star + 1, 2).toUInt(&ok, 16);
    return ok && (calc == expected);
}

QString NmeaParser::sentenceType(const QByteArray& sentence)
{
    // "$GPRMC,..." → "RMC"
    // "$GNRMC,..." → "RMC"
    const int comma = sentence.indexOf(',');
    if (comma < 3) return {};
    // Пропускаем '$' и двухбуквенный talker ID (GP, GN, GL, GA, ...)
    return QString::fromLatin1(sentence.mid(3, comma - 3)).toUpper();
}

NmeaResult NmeaParser::parse(const QByteArray& sentence)
{
    NmeaResult result;
    result.rawSentence    = QString::fromLatin1(sentence).trimmed();
    result.checksumValid  = verifyChecksum(sentence);
    result.sentenceType   = sentenceType(sentence);

    if (!result.checksumValid) return result;

    // Убираем всё до '$' и после '*'
    const int dollar = sentence.indexOf('$');
    const int star   = sentence.lastIndexOf('*');
    const QByteArray body = sentence.mid(dollar + 1, star - dollar - 1);

    const QStringList fields = QString::fromLatin1(body).split(',');
    if (fields.isEmpty()) return result;

    if (result.sentenceType == "RMC")
        return parseRmc(fields, sentence);
    if (result.sentenceType == "GGA")
        return parseGga(fields, sentence);

    return result;
}

NmeaResult NmeaParser::parseRmc(const QStringList& fields, const QByteArray& raw)
{
    // $xxRMC,HHMMSS.SS,A,LLLL.LL,a,YYYYY.YY,a,x.x,x.x,DDMMYY,x.x,a*hh
    NmeaResult r;
    r.rawSentence   = QString::fromLatin1(raw).trimmed();
    r.checksumValid = true;
    r.sentenceType  = "RMC";

    if (fields.size() < 10) return r;

    // [1] время, [9] дата
    r.utcTime = parseUtcTime(fields[1], fields[9]);

    // [2] статус: A=valid, V=warning
    r.isValid = (fields[2].toUpper() == "A");

    // [3,4] широта, [5,6] долгота
    if (fields.size() > 6) {
        r.latitude  = parseCoord(fields[3], fields[4]);
        r.longitude = parseCoord(fields[5], fields[6]);
    }

    return r;
}

NmeaResult NmeaParser::parseGga(const QStringList& fields, const QByteArray& raw)
{
    // $xxGGA,HHMMSS.SS,LLLL.LL,a,YYYYY.YY,a,x,xx,x.x,x.x,M,x.x,M,,*hh
    NmeaResult r;
    r.rawSentence   = QString::fromLatin1(raw).trimmed();
    r.checksumValid = true;
    r.sentenceType  = "GGA";

    if (fields.size() < 10) return r;

    // [1] время
    r.utcTime = parseUtcTime(fields[1]);

    // [6] качество фикса
    r.fixQuality = fields[6].toInt();
    r.isValid    = (r.fixQuality > 0);

    // [7] спутники
    r.satellites = fields[7].toInt();

    // [3,4] широта, [5,6] долгота
    r.latitude  = parseCoord(fields[2], fields[3]);
    r.longitude = parseCoord(fields[4], fields[5]);

    // [9] высота
    if (fields.size() > 9)
        r.altitude = fields[9].toDouble();

    return r;
}

QDateTime NmeaParser::parseUtcTime(const QString& timeStr, const QString& dateStr)
{
    if (timeStr.length() < 6) return {};

    const int    h   = timeStr.mid(0, 2).toInt();
    const int    m   = timeStr.mid(2, 2).toInt();
    const double s   = timeStr.mid(4).toDouble();
    const int    sec = static_cast<int>(s);
    const int    ms  = static_cast<int>((s - sec) * 1000.0 + 0.5);

    const QTime time(h, m, sec, ms);
    if (!time.isValid()) return {};

    if (!dateStr.isEmpty() && dateStr.length() >= 6) {
        const int day  = dateStr.mid(0, 2).toInt();
        const int mon  = dateStr.mid(2, 2).toInt();
        int       year = dateStr.mid(4, 2).toInt();
        year += (year < 80) ? 2000 : 1900;

        const QDate date(year, mon, day);
        if (date.isValid())
            return QDateTime(date, time, QTimeZone::utc());
    }

    // Дата не известна — используем сегодняшнюю (только для сравнения времени)
    return QDateTime(QDate::currentDate(), time, QTimeZone::utc());
}

double NmeaParser::parseCoord(const QString& coord, const QString& hemisphere)
{
    if (coord.isEmpty()) return 0.0;

    // Формат: DDDMM.MMMM или DDMM.MMMM
    const int dotPos = coord.indexOf('.');
    if (dotPos < 3) return 0.0;

    const int    degDigits = dotPos - 2;
    const double degrees   = coord.left(degDigits).toDouble();
    const double minutes   = coord.mid(degDigits).toDouble();
    double result = degrees + minutes / 60.0;

    const QString h = hemisphere.toUpper();
    if (h == "S" || h == "W") result = -result;

    return result;
}

} // namespace Msv::Serial
