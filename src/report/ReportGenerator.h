#pragma once

#include "core/IDeviceModel.h"
#include "core/IScenarioDispatcher.h"
#include "core/ILogger.h"
#include <QString>
#include <QDateTime>
#include <memory>

namespace Msv::Report {

// ─────────────────────────────────────────────────────────────────────────────
/// Данные сессии для отчёта.
/// Заполняется перед вызовом generate().
// ─────────────────────────────────────────────────────────────────────────────
struct SessionData {
    QString   operatorName;
    QDateTime sessionStart;
    QDateTime sessionEnd;
};

// ─────────────────────────────────────────────────────────────────────────────
/// Генератор итогового протокола проверки.
///
/// Формирует текстовый файл с разделами:
///   1. Заголовок (дата, оператор, изделие)
///   2. Результаты шагов (PASS/FAIL/WARNING)
///   3. Анализ согласованности времени (offsets)
///   4. Итоговая оценка
// ─────────────────────────────────────────────────────────────────────────────
class ReportGenerator {
public:
    explicit ReportGenerator(std::shared_ptr<Core::ILogger> logger);

    /// Сгенерировать отчёт и сохранить в filePath.
    /// Возвращает true при успехе.
    bool generate(const SessionData&                session,
                  const Core::DeviceSnapshot&       snapshot,
                  const Core::IScenarioDispatcher*  dispatcher,
                  const QString&                    filePath);

private:
    QString buildHeader    (const SessionData& session,
                            const Core::DeviceSnapshot& snap) const;

    QString buildStepsTable(const Core::IScenarioDispatcher* dispatcher) const;

    QString buildTimeAnalysis(const Core::DeviceSnapshot& snap) const;

    QString buildVerdict   (const Core::IScenarioDispatcher* dispatcher) const;

    /// Offset в мс: источник - ПК.
    static qint64 offsetMs(const QDateTime& sourceTime,
                            const QDateTime& capturedAt);

    std::shared_ptr<Core::ILogger> m_logger;
    static constexpr const char*   kSrc = "ReportGenerator";
};

} // namespace Msv::Report
