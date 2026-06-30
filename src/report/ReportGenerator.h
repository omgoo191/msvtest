#pragma once

#include "core/IDeviceModel.h"
#include "core/IScenarioDispatcher.h"
#include "core/LongRunMonitor.h"
#include "core/ILogger.h"
#include <QString>
#include <QDateTime>
#include <memory>

namespace Msv::Report {

struct SessionData {
    QString   operatorName;
    QDateTime sessionStart;
    QDateTime sessionEnd;
};

// ─────────────────────────────────────────────────────────────────────────────
/// Генератор итогового протокола проверки в PDF.
/// Верстает HTML, рендерит в PDF через QTextDocument + QPrinter.
// ─────────────────────────────────────────────────────────────────────────────
class ReportGenerator {
public:
    explicit ReportGenerator(std::shared_ptr<Core::ILogger> logger);

    /// Сгенерировать PDF-отчёт.
    /// longRun может быть nullptr если длительный тест не проводился.
    bool generatePdf(const SessionData&                session,
                     const Core::DeviceSnapshot&       snapshot,
                     const Core::IScenarioDispatcher*  dispatcher,
                     const Core::LongRunResult*        longRun,
                     const QString&                    filePath);

private:
    QString buildHtml(const SessionData&               session,
                      const Core::DeviceSnapshot&      snap,
                      const Core::IScenarioDispatcher* dispatcher,
                      const Core::LongRunResult*       longRun) const;

    QString htmlHeader      (const SessionData& s, const Core::DeviceSnapshot& snap) const;
    QString htmlStepsTable  (const Core::IScenarioDispatcher* d) const;
    QString htmlTimeAnalysis(const Core::DeviceSnapshot& snap) const;
    QString htmlLongRun     (const Core::LongRunResult& lr) const;
    QString htmlVerdict     (const Core::IScenarioDispatcher* d,
                             const Core::LongRunResult* lr) const;

    static qint64 offsetMs(const QDateTime& src, const QDateTime& captured);

    std::shared_ptr<Core::ILogger> m_logger;
    static constexpr const char*   kSrc = "ReportGenerator";
};

} // namespace Msv::Report
