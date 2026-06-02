#include "ReportGenerator.h"

#include <QFile>
#include <QTextStream>
#include <QList>

namespace Msv::Report {

ReportGenerator::ReportGenerator(std::shared_ptr<Core::ILogger> logger)
    : m_logger(std::move(logger))
{}

bool ReportGenerator::generate(const SessionData&               session,
                                const Core::DeviceSnapshot&      snapshot,
                                const Core::IScenarioDispatcher* dispatcher,
                                const QString&                   filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_logger->error(kSrc, QStringLiteral("Не удалось открыть файл: %1").arg(filePath));
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);

    const QString line80(80, '=');
    const QString line80d(80, '-');

    out << line80 << "\n";
    out << "    ПРОТОКОЛ ПРОВЕРКИ МОДУЛЯ СИНХРОНИЗАЦИИ ВРЕМЕНИ (МСВ)\n";
    out << line80 << "\n\n";

    out << buildHeader(session, snapshot);
    out << "\n" << line80d << "\n\n";

    out << buildStepsTable(dispatcher);
    out << "\n" << line80d << "\n\n";

    out << buildTimeAnalysis(snapshot);
    out << "\n" << line80d << "\n\n";

    out << buildVerdict(dispatcher);
    out << "\n" << line80 << "\n";

    file.close();
    m_logger->info(kSrc, QStringLiteral("Отчёт сохранён: %1").arg(filePath));
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────

QString ReportGenerator::buildHeader(const SessionData&          session,
                                      const Core::DeviceSnapshot& snap) const
{
    QString s;
    QTextStream out(&s);

    out << "ОБЩИЕ СВЕДЕНИЯ\n\n";
    out << QStringLiteral("  Дата проверки    : %1\n")
        .arg(session.sessionStart.toString("dd.MM.yyyy"));
    out << QStringLiteral("  Начало           : %1\n")
        .arg(session.sessionStart.toString("HH:mm:ss UTC"));
    out << QStringLiteral("  Окончание        : %1\n")
        .arg(session.sessionEnd.toString("HH:mm:ss UTC"));
    out << QStringLiteral("  Оператор         : %1\n")
        .arg(session.operatorName.isEmpty() ? "—" : session.operatorName);

    out << "\nИДЕНТИФИКАЦИЯ ИЗДЕЛИЯ\n\n";
    out << QStringLiteral("  Наименование     : %1\n")
        .arg(snap.deviceName.isEmpty() ? "—" : snap.deviceName);
    out << QStringLiteral("  IP-адрес         : %1\n")
        .arg(snap.ipAddress.toString());
    out << QStringLiteral("  MAC-адрес        : %1\n")
        .arg(snap.macAddress.isEmpty() ? "—" : snap.macAddress);
    out << QStringLiteral("  Версия ПО        : %1\n")
        .arg(snap.firmwareVersion.isEmpty() ? "—" : snap.firmwareVersion);
    out << QStringLiteral("  ID устройства    : %1\n")
        .arg(snap.deviceId);
    out << QStringLiteral("  Дата сборки ПО   : %1\n")
        .arg(snap.buildDate.isEmpty() ? "—" : snap.buildDate);
    out << QStringLiteral("  CRC ПО           : %1\n")
        .arg(snap.crc.isEmpty() ? "—" : snap.crc);

    return s;
}

QString ReportGenerator::buildStepsTable(
    const Core::IScenarioDispatcher* dispatcher) const
{
    QString s;
    QTextStream out(&s);

    out << "РЕЗУЛЬТАТЫ ШАГОВ\n\n";

    const auto steps   = dispatcher->allSteps();
    const auto results = dispatcher->stepResults();

    static const QMap<Core::StepResult, QString> tags {
        {Core::StepResult::Pass,    "  PASS   "},
        {Core::StepResult::Fail,    "  FAIL   "},
        {Core::StepResult::Warning, " WARNING "},
        {Core::StepResult::Skipped, " SKIPPED "},
        {Core::StepResult::NotRun,  " NOT RUN "},
    };

    for (int i = 0; i < steps.size(); ++i) {
        const auto& step   = steps[i];
        const auto  result = (i < results.size())
            ? results[i] : Core::StepResult::NotRun;

        const QString tag = tags.value(result, "   ???   ");
        out << QStringLiteral("  [%1] %2. %3\n")
            .arg(tag)
            .arg(i + 1, 2)
            .arg(step.title);
    }

    // Подсчёт
    int pass = 0, fail = 0, warn = 0;
    for (const auto& r : results) {
        if (r == Core::StepResult::Pass)    pass++;
        if (r == Core::StepResult::Fail)    fail++;
        if (r == Core::StepResult::Warning) warn++;
    }
    out << QStringLiteral("\n  Итого: PASS=%1  FAIL=%2  WARNING=%3\n")
        .arg(pass).arg(fail).arg(warn);

    return s;
}

QString ReportGenerator::buildTimeAnalysis(
    const Core::DeviceSnapshot& snap) const
{
    QString s;
    QTextStream out(&s);

    out << "АНАЛИЗ СОГЛАСОВАННОСТИ ВРЕМЕНИ\n\n";

    // ── Таблица источников ────────────────────────────────────────────────────
    auto fmtTime = [](const QDateTime& dt) -> QString {
        return dt.isValid()
            ? dt.toString("dd.MM.yyyy HH:mm:ss.zzz UTC")
            : "нет данных";
    };

    auto fmtOffset = [](qint64 ms) -> QString {
        if (ms == LLONG_MIN) return "н/д";
        const QString sign = ms >= 0 ? "+" : "";
        if (qAbs(ms) < 1000)
            return QStringLiteral("%1%2 мс").arg(sign).arg(ms);
        return QStringLiteral("%1%2.%3 с")
            .arg(sign)
            .arg(ms / 1000)
            .arg(qAbs(ms % 1000), 3, 10, QChar('0'));
    };

    out << "  Источник         Время                          Отклонение от ПК\n";
    out << "  " << QString(76, '-') << "\n";

    // Системное время ПК (опорное)
	out << "  "
		<< QString("Система (ПК)").leftJustified(17)
		<< fmtTime(QDateTime::currentDateTimeUtc()).leftJustified(31)
		<< "(опорное)\n";

    // Web-время
    {
		const QDateTime wt = snap.webTime.value_or(QDateTime{});
        const qint64 off = offsetMs(wt, snap.webCapturedAt);
		out << "  "
			<< QString("Web (td)").leftJustified(17)
			<< fmtTime(wt).leftJustified(31)
			<< (off == LLONG_MIN ? "н/д" : fmtOffset(off)) << "\n";
    }

    // SNTP-время
    {
        const qint64 off = offsetMs(snap.sntpTime.value_or(QDateTime()),
                                    snap.sntpCapturedAt);
		out << "  "
			<< QString("SNTP").leftJustified(17)
			<< fmtTime(snap.sntpTime.value_or(QDateTime())).leftJustified(31)
			<< fmtOffset(off) << "\n";
    }

    // GNSS UART
    {
        const qint64 off = offsetMs(snap.gnssTime.value_or(QDateTime()),
                                    snap.gnssCapturedAt);
		out << "  "
			<< QString("GNSS (UART)").leftJustified(17)
			<< fmtTime(snap.gnssTime.value_or(QDateTime())).leftJustified(31)
			<< fmtOffset(off) << "\n";
    }

    // ── Расхождения между источниками ─────────────────────────────────────────
    out << "\n  РАСХОЖДЕНИЯ МЕЖДУ ИСТОЧНИКАМИ\n\n";

    auto diffMs = [&](const QDateTime& a, const QDateTime& captA,
                      const QDateTime& b, const QDateTime& captB) -> qint64
    {
        if (!a.isValid() || !b.isValid() ||
            !captA.isValid() || !captB.isValid())
            return LLONG_MIN;
        // Приводим к одному моменту через offset от ПК
        const qint64 offA = offsetMs(a, captA);
        const qint64 offB = offsetMs(b, captB);
        if (offA == LLONG_MIN || offB == LLONG_MIN) return LLONG_MIN;
        return offA - offB;
    };

	auto printDiff = [&](const QString& label, qint64 ms) {
		out << "  "
			<< label.leftJustified(32)
			<< (ms == LLONG_MIN ? "н/д" : fmtOffset(ms)) << "\n";
	};

    const auto sntpTime = snap.sntpTime.value_or(QDateTime());
    const auto gnssTime = snap.gnssTime.value_or(QDateTime());

	const QDateTime webTime = snap.webTime.value_or(QDateTime{});

    printDiff("Web vs SNTP:",
        diffMs(webTime, snap.webCapturedAt,
               sntpTime,     snap.sntpCapturedAt));

    printDiff("Web vs GNSS (UART):",
        diffMs(webTime, snap.webCapturedAt,
               gnssTime,     snap.gnssCapturedAt));

    printDiff("SNTP vs GNSS (UART):",
        diffMs(sntpTime,     snap.sntpCapturedAt,
               gnssTime,     snap.gnssCapturedAt));

    // ── Состояние антенны ─────────────────────────────────────────────────────
    out << "\n  СОСТОЯНИЕ АНТЕННЫ\n\n";
    out << QStringLiteral("  Статус           : %1\n")
        .arg(snap.antennaStatus.isEmpty() ? "—" : snap.antennaStatus);
    out << QStringLiteral("  Статус GPS       : %1\n")
        .arg(snap.gpsStatus.isEmpty() ? "—" : snap.gpsStatus);
    out << QStringLiteral("  Спутников        : %1\n")
        .arg(snap.gpsSatellites.isEmpty() ? "—" : snap.gpsSatellites);
    out << QStringLiteral("  RTC валидность   : %1\n")
        .arg(snap.rtcValidity.isEmpty() ? "—" : snap.rtcValidity);

    return s;
}

QString ReportGenerator::buildVerdict(
    const Core::IScenarioDispatcher* dispatcher) const
{
    QString s;
    QTextStream out(&s);

    const auto results = dispatcher->stepResults();

    const bool hasFail = std::any_of(results.begin(), results.end(),
        [](Core::StepResult r){ return r == Core::StepResult::Fail; });
    const bool hasWarn = std::any_of(results.begin(), results.end(),
        [](Core::StepResult r){ return r == Core::StepResult::Warning; });

    out << "ИТОГОВАЯ ОЦЕНКА\n\n";

    if (hasFail) {
        out << "  ╔══════════════════════════════╗\n";
        out << "  ║         НЕ ГОДЕН             ║\n";
        out << "  ╚══════════════════════════════╝\n";
        out << "\n  Один или несколько шагов завершились с результатом FAIL.\n";
    } else if (hasWarn) {
        out << "  ╔══════════════════════════════╗\n";
        out << "  ║    ГОДЕН С ЗАМЕЧАНИЯМИ       ║\n";
        out << "  ╚══════════════════════════════╝\n";
        out << "\n  Все шаги пройдены, имеются предупреждения (WARNING).\n";
    } else {
        out << "  ╔══════════════════════════════╗\n";
        out << "  ║            ГОДЕН             ║\n";
        out << "  ╚══════════════════════════════╝\n";
        out << "\n  Все шаги пройдены без замечаний.\n";
    }

    return s;
}

qint64 ReportGenerator::offsetMs(const QDateTime& sourceTime,
                                   const QDateTime& capturedAt)
{
    if (!sourceTime.isValid() || !capturedAt.isValid())
        return LLONG_MIN;
    return capturedAt.msecsTo(sourceTime);
}

} // namespace Msv::Report
