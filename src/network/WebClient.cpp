#include "WebClient.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QTimer>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimeZone>

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
        result.webTime = QDateTime::fromString(dateHeader, Qt::RFC2822Date).toUTC();
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
	// Парсим JSON от /tags.shtml
	const QJsonDocument doc = QJsonDocument::fromJson(body);
	if (doc.isNull() || !doc.isObject()) {
		m_logger->warning(kSrc, "parseBody: не JSON");
		return;
	}
	const QJsonObject obj = doc.object();

	// ── Прямое чтение всех полей ──────────────────────────────────────────────
	out.macAddress     = obj.value("mac").toString();
	out.gpsTime        = obj.value("t_gps").toString();
	out.gpsDate        = obj.value("d_gps").toString();
	out.gpsStatus      = obj.value("st_gps").toString();
	out.gpsMode        = obj.value("lm_gps").toString();
	out.gpsSatellites  = obj.value("vsn").toString();
	out.gpsSignalLevel = obj.value("sss").toString();
	out.rtcTime        = obj.value("t_rtc").toString();
	out.rtcDate        = obj.value("d_rtc").toString();
	out.rtcValidity    = obj.value("iv_rtc").toString();
	out.antennaStatus  = obj.value("as").toString();
	out.lastValidGps   = obj.value("lr").toString();
	out.sntpSyncCount  = obj.value("ssn").toString();
	out.uptime         = obj.value("wt").toString();
	out.buildDate      = obj.value("sbd").toString();
	out.crc            = obj.value("crc").toString();
	// ── MAC-адрес ─────────────────────────────────────────────────────────────
	out.macAddress    = obj.value("mac").toString();
	out.antennaStatus = obj.value("as").toString();
	out.gpsStatus     = obj.value("st_gps").toString();

	// ── Время устройства (td): "00:22:00.613 01 января 1970 год" ─────────────
	const QString td = obj.value("td").toString();
	const QStringList parts = td.split(' ', Qt::SkipEmptyParts);
	if (parts.size() >= 4) {
		static const QStringList months = {
				"января","февраля","марта","апреля","мая","июня",
				"июля","августа","сентября","октября","ноября","декабря"
		};
		const QTime  t   = QTime::fromString(parts[0], "HH:mm:ss.zzz");
		const int    day = parts[1].toInt();
		const int    mon = months.indexOf(parts[2].toLower()) + 1;
		const int    yr  = parts[3].toInt();
		if (t.isValid() && mon > 0 && yr >= 1970)
			out.webTime = QDateTime({yr, mon, day}, t, QTimeZone::utc());
	}

	// ── Источник синхронизации (src) ──────────────────────────────────────────
	const QString src = obj.value("src").toString().toUpper();
	if (src == "PPS")
		out.syncSource = Core::SyncSource::GNSS;
	else if (src == "NTP")
		out.syncSource = Core::SyncSource::NTP;
	else
		out.syncSource = Core::SyncSource::Internal;

	// ── Статус синхронизации ──────────────────────────────────────────────────
	// st_gps: "A - решение годно" = синхр., "V - решение не годно" = нет
	if (out.gpsStatus.startsWith("A"))
		out.syncStatus = Core::SyncStatus::Synchronized;
	else if (src == "PPS")
		out.syncStatus = Core::SyncStatus::Holdover; // PPS есть, GPS потерян
	else
		out.syncStatus = Core::SyncStatus::Freerun;
}

} // namespace Msv::Network
