#include "WebClient.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTimer>
#include <QUrl>

namespace Msv::Network {

WebClient::WebClient(std::shared_ptr<Core::ILogger> logger, QObject* parent)
    : QObject(parent)
    , m_logger(std::move(logger))
{
    m_nam   = new QNetworkAccessManager(this);
    m_timer = new QTimer(this);
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, &WebClient::onTimeout);
}

void WebClient::request(const QHostAddress& ip, const QString& path, int timeoutMs)
{
    cancel();

    const QUrl url(QStringLiteral("http://%1%2").arg(ip.toString(), path));
    m_logger->info(kSrc, QStringLiteral("GET %1").arg(url.toString()));

    QNetworkRequest req(url);
    req.setTransferTimeout(timeoutMs);

    m_reply = m_nam->get(req);
    connect(m_reply, &QNetworkReply::finished, this, &WebClient::onReplyFinished);

    m_timer->start(timeoutMs + 500);
}

void WebClient::cancel()
{
    m_timer->stop();
    if (m_reply) {
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }
}

void WebClient::onReplyFinished()
{
    m_timer->stop();
    if (!m_reply) return;

    auto* reply = m_reply;
    m_reply = nullptr;
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        const QString err = reply->errorString();
        m_logger->error(kSrc, QStringLiteral("HTTP ошибка: %1").arg(err));
        emit failed(err);
        return;
    }

    Result result;

    // ── Время из заголовка Date (всегда присутствует в HTTP) ──────────────────
    const QString dateHeader = reply->rawHeader("Date");
    if (!dateHeader.isEmpty()) {
        result.webTime = QDateTime::fromString(dateHeader, Qt::RFC2822Date);
        result.webTime.setTimeSpec(Qt::UTC);
        m_logger->debug(kSrc, QStringLiteral("Date header: %1").arg(dateHeader));
    }

    // ── Тело ответа → парсинг статуса ─────────────────────────────────────────
    const QByteArray body = reply->readAll();
    parseBody(body, result);

    m_logger->info(kSrc, QStringLiteral("Web-статус: source=%1 status=%2 time=%3")
        .arg(static_cast<int>(result.syncSource))
        .arg(static_cast<int>(result.syncStatus))
        .arg(result.webTime.toString(Qt::ISODate)));

    emit finished(result);
}

void WebClient::onTimeout()
{
    m_logger->warning(kSrc, "HTTP таймаут");
    cancel();
    emit failed("таймаут");
}

void WebClient::parseBody(const QByteArray& body, Result& out) const
{
    // ── Заглушка — переопредели под реальный API МСВ ──────────────────────────
    //
    // Пример если устройство возвращает JSON:
    //   {"syncSource":"GNSS","syncStatus":"Synchronized","time":"2024-01-01T12:00:00Z"}
    //
    // Пример если устройство возвращает текст:
    //   SYNC_SOURCE=GNSS\nSYNC_STATUS=OK\n
    //
    // Пока ищем ключевые слова в теле (регистронезависимо):

    out.rawBody = QString::fromUtf8(body);
    const QString bodyLower = out.rawBody.toLower();

    // Источник синхронизации
    if (bodyLower.contains("gnss") || bodyLower.contains("gps"))
        out.syncSource = Core::SyncSource::GNSS;
    else if (bodyLower.contains("ntp") || bodyLower.contains("external"))
        out.syncSource = Core::SyncSource::NTP;
    else if (bodyLower.contains("internal") || bodyLower.contains("free"))
        out.syncSource = Core::SyncSource::Internal;

    // Статус синхронизации
    if (bodyLower.contains("synchronized") || bodyLower.contains("sync_ok") || bodyLower.contains("locked"))
        out.syncStatus = Core::SyncStatus::Synchronized;
    else if (bodyLower.contains("holdover"))
        out.syncStatus = Core::SyncStatus::Holdover;
    else if (bodyLower.contains("freerun") || bodyLower.contains("free_run") || bodyLower.contains("unsync"))
        out.syncStatus = Core::SyncStatus::Freerun;
}

} // namespace Msv::Network
