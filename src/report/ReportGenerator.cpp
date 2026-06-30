#include "ReportGenerator.h"

#include <QTextDocument>
#include <QPrinter>
#include <QPageLayout>
#include <QPageSize>
#include <QFileInfo>

namespace Msv::Report {

ReportGenerator::ReportGenerator(std::shared_ptr<Core::ILogger> logger)
    : m_logger(std::move(logger))
{}

bool ReportGenerator::generatePdf(const SessionData&               session,
                                   const Core::DeviceSnapshot&      snapshot,
                                   const Core::IScenarioDispatcher* dispatcher,
                                   const Core::LongRunResult*       longRun,
                                   const QString&                   filePath)
{
    const QString html = buildHtml(session, snapshot, dispatcher, longRun);

    QTextDocument doc;
	doc.setDefaultFont(QFont("Segoe UI", 11));
    doc.setHtml(html);

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(filePath);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageMargins(QMarginsF(15, 15, 15, 15), QPageLayout::Millimeter);

    doc.setPageSize(printer.pageRect(QPrinter::DevicePixel).size());
	doc.setTextWidth(printer.pageRect(QPrinter::DevicePixel).width());
    doc.print(&printer);

    QFileInfo fi(filePath);
    if (!fi.exists() || fi.size() == 0) {
        m_logger->error(kSrc, QStringLiteral("PDF не создан: %1").arg(filePath));
        return false;
    }

    m_logger->info(kSrc, QStringLiteral("PDF-отчёт сохранён: %1").arg(filePath));
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────

QString ReportGenerator::buildHtml(const SessionData&               session,
                                    const Core::DeviceSnapshot&      snap,
                                    const Core::IScenarioDispatcher* dispatcher,
                                    const Core::LongRunResult*       longRun) const
{
    QString body;
    body += htmlHeader(session, snap);
    body += htmlStepsTable(dispatcher);
    body += htmlTimeAnalysis(snap);
    if (longRun)
        body += htmlLongRun(*longRun);
    body += htmlVerdict(dispatcher, longRun);

    return QStringLiteral(R"(
<html>
<head><meta charset="utf-8"></head>
<body style="font-family: 'Segoe UI', Arial, sans-serif; color: #1a1a1a;">
%1
</body>
</html>
)").arg(body);
}

QString ReportGenerator::htmlHeader(const SessionData&          s,
                                     const Core::DeviceSnapshot& snap) const
{
    return QStringLiteral(R"(
<div style="border-bottom: 3px solid #00897b; padding-bottom: 8px; margin-bottom: 16px;">
  <h1 style="color: #00695c; font-size: 20pt; margin: 0;">
    ПРОТОКОЛ ПРОВЕРКИ МСВ
  </h1>
  <p style="color: #666; font-size: 9pt; margin: 4px 0 0 0;">
    Модуль синхронизации времени · автоматизированный стенд
  </p>
</div>

<table width="100%%" cellpadding="4" style="font-size: 10pt; margin-bottom: 16px;">
  <tr>
    <td width="50%%" valign="top">
      <b style="color:#00695c;">ОБЩИЕ СВЕДЕНИЯ</b><br>
      Дата: %1<br>
      Начало: %2<br>
      Окончание: %3<br>
      Оператор: %4
    </td>
    <td width="50%%" valign="top">
      <b style="color:#00695c;">ИЗДЕЛИЕ</b><br>
      %5<br>
      IP: %6 &nbsp; MAC: %7<br>
      Версия ПО: %8 &nbsp; ID: %9<br>
      Сборка: %10 &nbsp; CRC: %11
    </td>
  </tr>
</table>
)")
    .arg(s.sessionStart.toLocalTime().toString("dd.MM.yyyy"))
    .arg(s.sessionStart.toLocalTime().toString("HH:mm:ss"))
    .arg(s.sessionEnd.toLocalTime().toString("HH:mm:ss"))
    .arg(s.operatorName.isEmpty() ? "—" : s.operatorName.toHtmlEscaped())
    .arg(snap.deviceName.isEmpty() ? "—" : snap.deviceName.toHtmlEscaped())
    .arg(snap.ipAddress.toString())
    .arg(snap.macAddress.isEmpty() ? "—" : snap.macAddress)
    .arg(snap.firmwareVersion.isEmpty() ? "—" : snap.firmwareVersion)
    .arg(snap.deviceId)
    .arg(snap.buildDate.isEmpty() ? "—" : snap.buildDate.toHtmlEscaped())
    .arg(snap.crc.isEmpty() ? "—" : snap.crc);
}

QString ReportGenerator::htmlStepsTable(const Core::IScenarioDispatcher* d) const
{
    const auto steps   = d->allSteps();
    const auto results = d->stepResults();

    QString rows;
    int pass = 0, fail = 0, warn = 0;

    for (int i = 0; i < steps.size(); ++i) {
        const auto result = (i < results.size()) ? results[i] : Core::StepResult::NotRun;

        QString badge, color, bg;
        switch (result) {
            case Core::StepResult::Pass:
                badge = "PASS"; color = "#1b5e20"; bg = "#e8f5e9"; pass++; break;
            case Core::StepResult::Fail:
                badge = "FAIL"; color = "#b71c1c"; bg = "#ffebee"; fail++; break;
            case Core::StepResult::Warning:
                badge = "WARNING"; color = "#e65100"; bg = "#fff3e0"; warn++; break;
            case Core::StepResult::Skipped:
                badge = "SKIP"; color = "#555"; bg = "#f5f5f5"; break;
            default:
                badge = "—"; color = "#999"; bg = "#fafafa"; break;
        }

        rows += QStringLiteral(R"(
<tr style="background:%1;">
  <td style="padding:6px; border-bottom:1px solid #eee; text-align:center; color:#999;">%2</td>
  <td style="padding:6px; border-bottom:1px solid #eee;">%3</td>
  <td style="padding:6px; border-bottom:1px solid #eee; text-align:center;">
    <b style="color:%4;">%5</b>
  </td>
</tr>)")
            .arg(bg)
            .arg(i + 1)
            .arg(steps[i].title.toHtmlEscaped())
            .arg(color)
            .arg(badge);
    }

    return QStringLiteral(R"(
<h2 style="color:#00695c; font-size:13pt; border-bottom:1px solid #ccc; padding-bottom:4px;">
  Результаты шагов
</h2>
<table width="100%%" cellpadding="0" cellspacing="0" style="font-size:10pt; margin-bottom:8px;">
  <tr style="background:#00897b; color:white;">
    <td style="padding:6px; text-align:center; width:40px;">№</td>
    <td style="padding:6px;">Шаг</td>
    <td style="padding:6px; text-align:center; width:90px;">Результат</td>
  </tr>
  %1
</table>
<p style="font-size:10pt; margin-bottom:16px;">
  Итого: <b style="color:#1b5e20;">PASS %2</b> &nbsp;
  <b style="color:#e65100;">WARNING %3</b> &nbsp;
  <b style="color:#b71c1c;">FAIL %4</b>
</p>
)").arg(rows).arg(pass).arg(warn).arg(fail);
}

QString ReportGenerator::htmlTimeAnalysis(const Core::DeviceSnapshot& snap) const
{
    auto fmtTime = [](const QDateTime& dt) -> QString {
        return dt.isValid()
            ? dt.toLocalTime().toString("dd.MM.yyyy HH:mm:ss.zzz")
            : "<span style='color:#999;'>нет данных</span>";
    };

    auto fmtOff = [](qint64 ms) -> QString {
        if (ms == LLONG_MIN) return "<span style='color:#999;'>н/д</span>";
        const QString sign = ms >= 0 ? "+" : "";
        return QStringLiteral("%1%2 мс").arg(sign).arg(ms);
    };

    const QDateTime webT  = snap.webTime.value_or(QDateTime{});
    const QDateTime sntpT = snap.sntpTime.value_or(QDateTime{});
    const QDateTime gnssT = snap.gnssTime.value_or(QDateTime{});

    return QStringLiteral(R"(
<h2 style="color:#00695c; font-size:13pt; border-bottom:1px solid #ccc; padding-bottom:4px;">
  Анализ согласованности времени
</h2>
<table width="100%%" cellpadding="6" cellspacing="0" style="font-size:10pt; margin-bottom:16px;">
  <tr style="background:#f0f0f0;">
    <td><b>Источник</b></td><td><b>Время</b></td><td><b>Отклонение от ПК</b></td>
  </tr>
  <tr><td>Система (ПК)</td><td>%1</td><td style="color:#999;">опорное</td></tr>
  <tr><td>Web (td)</td><td>%2</td><td>%3</td></tr>
  <tr><td>SNTP</td><td>%4</td><td>%5</td></tr>
  <tr><td>GNSS (UART)</td><td>%6</td><td>%7</td></tr>
</table>
<table width="100%%" cellpadding="4" style="font-size:10pt; margin-bottom:16px;">
  <tr><td><b>Антенна:</b> %8</td><td><b>GPS:</b> %9</td></tr>
  <tr><td><b>Спутники:</b> %10</td><td><b>RTC:</b> %11</td></tr>
</table>
)")
    .arg(fmtTime(QDateTime::currentDateTimeUtc()))
    .arg(fmtTime(webT)).arg(fmtOff(offsetMs(webT, snap.webCapturedAt)))
    .arg(fmtTime(sntpT)).arg(fmtOff(offsetMs(sntpT, snap.sntpCapturedAt)))
    .arg(fmtTime(gnssT)).arg(fmtOff(offsetMs(gnssT, snap.gnssCapturedAt)))
    .arg(snap.antennaStatus.isEmpty() ? "—" : snap.antennaStatus.toHtmlEscaped())
    .arg(snap.gpsStatus.isEmpty() ? "—" : snap.gpsStatus.toHtmlEscaped())
    .arg(snap.gpsSatellites.isEmpty() ? "—" : snap.gpsSatellites.toHtmlEscaped())
    .arg(snap.rtcValidity.isEmpty() ? "—" : snap.rtcValidity.toHtmlEscaped());
}

QString ReportGenerator::htmlLongRun(const Core::LongRunResult& lr) const
{
    auto fmtStats = [](const Core::OffsetStats& st) -> QString {
        if (st.count == 0) return "<span style='color:#999;'>нет данных</span>";
        return QStringLiteral("мин %1 / макс %2 / среднее %3 / СКО %4 мс (n=%5)")
            .arg(st.minMs).arg(st.maxMs)
            .arg(QString::number(st.meanMs, 'f', 1))
            .arg(QString::number(st.stdDevMs, 'f', 1))
            .arg(st.count);
    };

    const double checkPct = lr.nmeaTotal > 0
        ? 100.0 * lr.nmeaOk / lr.nmeaTotal : 0.0;

    return QStringLiteral(R"(
<h2 style="color:#00695c; font-size:13pt; border-bottom:1px solid #ccc; padding-bottom:4px;">
  Длительный мониторинг (%1 мин, %2 замеров)
</h2>
<table width="100%%" cellpadding="5" cellspacing="0" style="font-size:10pt; margin-bottom:16px;">
  <tr style="background:#f0f0f0;"><td colspan="2"><b>Ошибки монотонности</b></td></tr>
  <tr><td>Web / SNTP / GNSS</td><td>%3 / %4 / %5</td></tr>
  <tr style="background:#f0f0f0;"><td colspan="2"><b>Расхождение с ПК</b></td></tr>
  <tr><td>Web</td><td>%6</td></tr>
  <tr><td>SNTP</td><td>%7</td></tr>
  <tr><td>GNSS</td><td>%8</td></tr>
  <tr><td>Web vs GNSS</td><td>%9</td></tr>
  <tr style="background:#f0f0f0;"><td colspan="2"><b>NMEA / События</b></td></tr>
  <tr><td>Checksum</td><td>%10 / %11 (%12%%)</td></tr>
  <tr><td>Смен антенны / sync</td><td>%13 / %14</td></tr>
</table>
)")
    .arg(lr.durationMinutes).arg(lr.samplesCollected)
    .arg(lr.webMonotonErrors).arg(lr.sntpMonotonErrors).arg(lr.gnssMonotonErrors)
    .arg(fmtStats(lr.webOffset))
    .arg(fmtStats(lr.sntpOffset))
    .arg(fmtStats(lr.gnssOffset))
    .arg(fmtStats(lr.webVsGnss))
    .arg(lr.nmeaOk).arg(lr.nmeaTotal)
    .arg(QString::number(checkPct, 'f', 1))
    .arg(lr.antennaChanges).arg(lr.syncStatusChanges);
}

QString ReportGenerator::htmlVerdict(const Core::IScenarioDispatcher* d,
                                      const Core::LongRunResult* lr) const
{
    const auto results = d->stepResults();

    bool hasFail = std::any_of(results.begin(), results.end(),
        [](Core::StepResult r){ return r == Core::StepResult::Fail; });
    bool hasWarn = std::any_of(results.begin(), results.end(),
        [](Core::StepResult r){ return r == Core::StepResult::Warning; });

    if (lr) {
        if (lr->webMonotonErrors > 2 || lr->sntpMonotonErrors > 2 ||
            lr->gnssMonotonErrors > 5) hasFail = true;
        if (lr->antennaChanges > 0 || lr->syncStatusChanges > 2) hasWarn = true;
    }

    QString verdict, color, bg;
    if (hasFail) {
        verdict = "НЕ ГОДЕН"; color = "#b71c1c"; bg = "#ffebee";
    } else if (hasWarn) {
        verdict = "ГОДЕН С ЗАМЕЧАНИЯМИ"; color = "#e65100"; bg = "#fff3e0";
    } else {
        verdict = "ГОДЕН"; color = "#1b5e20"; bg = "#e8f5e9";
    }

    return QStringLiteral(R"(
<div style="background:%1; border:2px solid %2; border-radius:6px;
            padding:14px; text-align:center; margin-top:16px;">
  <span style="color:%2; font-size:18pt; font-weight:bold;">%3</span>
</div>
)").arg(bg, color, verdict);
}

qint64 ReportGenerator::offsetMs(const QDateTime& src, const QDateTime& captured)
{
    if (!src.isValid() || !captured.isValid()) return LLONG_MIN;
    return captured.msecsTo(src);
}

} // namespace Msv::Report
