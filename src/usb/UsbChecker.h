#pragma once

#include <QString>
#include <QList>

namespace Msv::Usb {

// ─────────────────────────────────────────────────────────────────────────────
/// Одна запись из манифеста — эталонный файл с ожидаемым CRC32.
// ─────────────────────────────────────────────────────────────────────────────
struct ManifestEntry {
    QString relativePath;   ///< Путь относительно корня носителя, напр. "www/index.html"
    quint32 expectedCrc32 {0};
    qint64  expectedSize  {-1};  ///< -1 = размер не проверяется
};

// ─────────────────────────────────────────────────────────────────────────────
/// Манифест — список эталонных файлов + опциональный файл-маркер
/// (time_security.cfg), для которого проверяется только наличие.
// ─────────────────────────────────────────────────────────────────────────────
struct Manifest {
    QList<ManifestEntry> files;
    QString               markerFile;   ///< напр. "time_security.cfg"
    bool                  valid {false};
    QString               loadError;
};

// ─────────────────────────────────────────────────────────────────────────────
/// Результат проверки одного файла.
// ─────────────────────────────────────────────────────────────────────────────
struct FileCheckResult {
    QString relativePath;
    bool    exists       {false};
    bool    sizeOk        {true};
    bool    crcOk         {false};
    quint32 actualCrc32   {0};
    qint64  actualSize    {-1};
};

// ─────────────────────────────────────────────────────────────────────────────
/// Полный результат проверки USB-носителя.
// ─────────────────────────────────────────────────────────────────────────────
struct UsbCheckResult {
    bool    driveAccessible {false};
    QString driveLetter;
    QString error;

    QList<FileCheckResult> fileResults;
    bool    markerFilePresent {false};
    QString markerFileName;

    [[nodiscard]] bool allFilesOk() const
    {
        if (!driveAccessible) return false;
        for (const auto& f : fileResults)
            if (!f.exists || !f.sizeOk || !f.crcOk) return false;
        return true;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
/// Загрузка манифеста и проверка содержимого USB-носителя.
// ─────────────────────────────────────────────────────────────────────────────
class UsbChecker {
public:
    UsbChecker() = default;

    /// Загрузить манифест из JSON-файла (usb_manifest.json рядом с exe).
    static Manifest loadManifest(const QString& manifestPath);

    /// Проверить носитель по букве диска (напр. "E:").
    static UsbCheckResult checkDrive(const QString& driveLetter,
                                      const Manifest& manifest);

    /// Список доступных съёмных дисков в системе (буквы).
    static QStringList availableDrives();

private:
    static quint32 computeCrc32(const QString& filePath);
};

} // namespace Msv::Usb
