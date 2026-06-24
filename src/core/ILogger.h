#pragma once

#include <QObject>
#include <QString>
#include <QDateTime>

namespace Msv::Core {

// ─────────────────────────────────────────────────────────────────────────────
/// Уровень записи в журнал.
// ─────────────────────────────────────────────────────────────────────────────
enum class LogLevel : int {
    Debug   = 0,  ///< Отладочная информация (только в debug-сборке)
    Info    = 1,  ///< Штатные события
    Warning = 2,  ///< Предупреждение: отклонение, не блокирующее работу
    Error   = 3,  ///< Ошибка: шаг завершился неудачей
    Fatal   = 4,  ///< Критическая ошибка: работа невозможна
};

// ─────────────────────────────────────────────────────────────────────────────
/// Единичная запись журнала.
// ─────────────────────────────────────────────────────────────────────────────
struct LogEntry {
    QDateTime timestamp;   ///< Момент записи (UTC)
    LogLevel  level;
    QString   source;      ///< Имя подсистемы ("ScenarioDispatcher", "SntpClient", …)
    QString   message;
	int 	  stepIndex {-1};

	[[nodiscard]] QString levelStr() const
	{
		switch (level) {
			case LogLevel::Debug:   return "DBG";
			case LogLevel::Info:    return "INF";
			case LogLevel::Warning: return "WRN";
			case LogLevel::Error:   return "ERR";
			case LogLevel::Fatal:   return "FTL";
			default:                return "???";
		}
	}
};

// ─────────────────────────────────────────────────────────────────────────────
/// Абстрактный журнал. Реализации: ConsoleLogger, FileLogger, ModelLogger.
///
/// Намеренно НЕ наследуется от QObject — логгер может встраиваться
/// в любой слой без ограничений иерархии Qt.
// ─────────────────────────────────────────────────────────────────────────────
class ILogger {
public:
    virtual ~ILogger() = default;

    virtual void log(const LogEntry& entry) = 0;

    // ── Удобные хелперы ───────────────────────────────────────────────────────
    void debug  (const QString& src, const QString& msg) { log({QDateTime::currentDateTimeUtc(), LogLevel::Debug,   src, msg}); }
    void info   (const QString& src, const QString& msg) { log({QDateTime::currentDateTimeUtc(), LogLevel::Info,    src, msg}); }
    void warning(const QString& src, const QString& msg) { log({QDateTime::currentDateTimeUtc(), LogLevel::Warning, src, msg}); }
    void error  (const QString& src, const QString& msg) { log({QDateTime::currentDateTimeUtc(), LogLevel::Error,   src, msg}); }
    void fatal  (const QString& src, const QString& msg) { log({QDateTime::currentDateTimeUtc(), LogLevel::Fatal,   src, msg}); }
};

} // namespace Msv::Core
