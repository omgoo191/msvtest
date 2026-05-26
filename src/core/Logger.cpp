#include "Logger.h"
#include <QMutexLocker>
#include <QTextStream>
#include <QDebug>
#include <QMetaObject>

namespace Msv::Core {

// ─── CompositeLogger ──────────────────────────────────────────────────────────

void CompositeLogger::addBackend(std::unique_ptr<ILogger> backend)
{
    QMutexLocker lock(&m_mutex);
    m_backends.push_back(std::move(backend));
}

void CompositeLogger::log(const LogEntry& entry)
{
    QMutexLocker lock(&m_mutex);
    for (auto& backend : m_backends)
        backend->log(entry);
}

// ─── ConsoleLogBackend ────────────────────────────────────────────────────────

static const char* levelTag(LogLevel lvl) {
    switch (lvl) {
        case LogLevel::Debug:   return "[DBG]";
        case LogLevel::Info:    return "[INF]";
        case LogLevel::Warning: return "[WRN]";
        case LogLevel::Error:   return "[ERR]";
        case LogLevel::Fatal:   return "[FTL]";
    }
    return "[???]";
}

void ConsoleLogBackend::log(const LogEntry& entry)
{
    const QString line = QStringLiteral("%1 %2 [%3] %4")
        .arg(entry.timestamp.toString(Qt::ISODateWithMs))
        .arg(QLatin1String(levelTag(entry.level)))
        .arg(entry.source)
        .arg(entry.message);

    switch (entry.level) {
        case LogLevel::Debug:
        case LogLevel::Info:
            qDebug().noquote()   << line; break;
        case LogLevel::Warning:
            qWarning().noquote() << line; break;
        case LogLevel::Error:
        case LogLevel::Fatal:
            qCritical().noquote()<< line; break;
    }
}

// ─── FileLogBackend ───────────────────────────────────────────────────────────

FileLogBackend::FileLogBackend(const QString& filePath)
    : m_file(filePath)
{
    m_file.open(QIODevice::Append | QIODevice::Text);
}

FileLogBackend::~FileLogBackend()
{
    if (m_file.isOpen())
        m_file.close();
}

void FileLogBackend::log(const LogEntry& entry)
{
    QMutexLocker lock(&m_mutex);
    if (!m_file.isOpen()) return;

    QTextStream out(&m_file);
    out << entry.timestamp.toString(Qt::ISODateWithMs)
        << '\t' << static_cast<int>(entry.level)
        << '\t' << entry.source
        << '\t' << entry.message
        << '\n';
    out.flush();
}

// ─── ModelLogBackend ─────────────────────────────────────────────────────────

ModelLogBackend::ModelLogBackend(QObject* parent)
    : QObject(parent)
{}

void ModelLogBackend::log(const LogEntry& entry)
{
    {
        QMutexLocker lock(&m_mutex);
        m_entries.append(entry);
    }
    // Эмит в GUI-поток
    QMetaObject::invokeMethod(this, [this, entry]() {
        emit entryAdded(entry);
    }, Qt::QueuedConnection);
}

QList<LogEntry> ModelLogBackend::entries() const
{
    QMutexLocker lock(&m_mutex);
    return m_entries;
}

void ModelLogBackend::clear()
{
    QMutexLocker lock(&m_mutex);
    m_entries.clear();
}

} // namespace Msv::Core
