#pragma once

#include "ILogger.h"
#include <QObject>
#include <vector>
#include <QMutex>
#include <QFile>
#include <memory>

namespace Msv::Core {

// ─────────────────────────────────────────────────────────────────────────────
/// Составной логгер — разгоняет запись во все зарегистрированные бэкенды.
///
/// Использование:
///   auto logger = std::make_shared<CompositeLogger>();
///   logger->addBackend(std::make_unique<ConsoleLogBackend>());
///   logger->addBackend(std::make_unique<FileLogBackend>("msv_session.log"));
///   logger->addBackend(std::make_unique<ModelLogBackend>(logModel));
// ─────────────────────────────────────────────────────────────────────────────
class CompositeLogger final : public ILogger {
public:
    void log(const LogEntry& entry) override;
    void addBackend(std::unique_ptr<ILogger> backend);

private:
    QMutex                              m_mutex;
    std::vector<std::unique_ptr<ILogger>>     m_backends;
};

// ─────────────────────────────────────────────────────────────────────────────
/// Бэкенд: вывод в qDebug/qWarning/qCritical.
// ─────────────────────────────────────────────────────────────────────────────
class ConsoleLogBackend final : public ILogger {
public:
    void log(const LogEntry& entry) override;
};

// ─────────────────────────────────────────────────────────────────────────────
/// Бэкенд: запись в текстовый файл (CSV-подобный формат для отчёта).
// ─────────────────────────────────────────────────────────────────────────────
class FileLogBackend final : public ILogger {
public:
    explicit FileLogBackend(const QString& filePath);
    ~FileLogBackend() override;

    void log(const LogEntry& entry) override;

private:
    QFile   m_file;
    QMutex  m_mutex;
};

// ─────────────────────────────────────────────────────────────────────────────
/// Бэкенд-модель: хранит записи в памяти, эмитирует QObject-сигнал.
/// UI подписывается на entryAdded() для отображения лога в реальном времени.
// ─────────────────────────────────────────────────────────────────────────────
class ModelLogBackend final : public QObject, public ILogger {
    Q_OBJECT
public:
    explicit ModelLogBackend(QObject* parent = nullptr);

    void log(const LogEntry& entry) override;

    [[nodiscard]] QList<LogEntry> entries() const;
    void clear();

signals:
    void entryAdded(const Msv::Core::LogEntry& entry);

private:
    mutable QMutex      m_mutex;
    QList<LogEntry>     m_entries;
};

} // namespace Msv::Core
