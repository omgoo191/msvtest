#include "UsbChecker.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QStorageInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <zlib.h>

namespace Msv::Usb {

Manifest UsbChecker::loadManifest(const QString& manifestPath)
{
    Manifest m;

    QFile f(manifestPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m.loadError = QStringLiteral("Не удалось открыть %1").arg(manifestPath);
        return m;
    }

    const QByteArray data = f.readAll();
    f.close();

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError) {
        m.loadError = QStringLiteral("Ошибка JSON: %1").arg(err.errorString());
        return m;
    }

    const QJsonObject root = doc.object();
    m.markerFile = root.value("markerFile").toString();

    const QJsonArray files = root.value("files").toArray();
    for (const auto& v : files) {
        const QJsonObject o = v.toObject();
        ManifestEntry entry;
        entry.relativePath = o.value("path").toString();
        entry.expectedCrc32 = static_cast<quint32>(
            o.value("crc32").toString().toULongLong(nullptr, 16));
        entry.expectedSize = o.contains("size")
            ? static_cast<qint64>(o.value("size").toDouble()) : -1;

        if (!entry.relativePath.isEmpty())
            m.files.append(entry);
    }

    m.valid = true;
    return m;
}

quint32 UsbChecker::computeCrc32(const QString& filePath)
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly))
        return 0;

    uLong crc = crc32(0L, Z_NULL, 0);

    QByteArray buffer;
    while (!f.atEnd()) {
        buffer = f.read(65536);
        crc = crc32(crc,
            reinterpret_cast<const Bytef*>(buffer.constData()),
            static_cast<uInt>(buffer.size()));
    }

    return static_cast<quint32>(crc);
}

UsbCheckResult UsbChecker::checkDrive(const QString& driveLetter,
                                       const Manifest& manifest)
{
    UsbCheckResult result;
    result.driveLetter = driveLetter;

    QString root = driveLetter;
    if (!root.endsWith('\\') && !root.endsWith('/'))
        root += "\\";

    QDir dir(root);
    if (!dir.exists()) {
        result.driveAccessible = false;
        result.error = QStringLiteral("Диск %1 недоступен").arg(driveLetter);
        return result;
    }

    result.driveAccessible = true;

    if (!manifest.valid) {
        result.error = manifest.loadError.isEmpty()
            ? "Манифест не загружен" : manifest.loadError;
        return result;
    }

    // Проверка каждого файла из манифеста
    for (const auto& entry : manifest.files) {
        FileCheckResult fr;
        fr.relativePath = entry.relativePath;

        const QString fullPath = root + entry.relativePath;
        QFileInfo fi(fullPath);

        fr.exists = fi.exists() && fi.isFile();
        if (!fr.exists) {
            result.fileResults.append(fr);
            continue;
        }

        fr.actualSize = fi.size();
        fr.sizeOk = (entry.expectedSize < 0) || (fr.actualSize == entry.expectedSize);

        fr.actualCrc32 = computeCrc32(fullPath);
        fr.crcOk = (fr.actualCrc32 == entry.expectedCrc32);

        result.fileResults.append(fr);
    }

    // Проверка маркер-файла (time_security.cfg) — только наличие
    if (!manifest.markerFile.isEmpty()) {
        result.markerFileName = manifest.markerFile;
        const QString markerPath = root + manifest.markerFile;
        result.markerFilePresent = QFileInfo::exists(markerPath);
    }

    return result;
}

QStringList UsbChecker::availableDrives()
{
    QStringList result;
    const auto volumes = QStorageInfo::mountedVolumes();
    for (const auto& v : volumes) {
        if (!v.isValid() || !v.isReady()) continue;
        const QString path = v.rootPath();
        // На Windows это вида "E:/"
        if (path.length() >= 2 && path[1] == ':')
            result.append(path.left(2));
    }
    return result;
}

} // namespace Msv::Usb
