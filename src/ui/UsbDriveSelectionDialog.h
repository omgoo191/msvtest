#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QListWidget;
class QPushButton;
class QLabel;
QT_END_NAMESPACE

namespace Msv::Ui {

// ─────────────────────────────────────────────────────────────────────────────
/// Диалог выбора буквы диска USB-носителя.
// ─────────────────────────────────────────────────────────────────────────────
class UsbDriveSelectionDialog : public QDialog {
    Q_OBJECT
public:
    explicit UsbDriveSelectionDialog(QWidget* parent = nullptr);

    [[nodiscard]] QString selectedDrive() const { return m_selectedDrive; }

private slots:
    void onSelectClicked();
    void onRefreshClicked();

private:
    void buildUi();
    void applyTheme();
    void populateDrives();

    QListWidget* m_driveList   {nullptr};
    QPushButton* m_selectBtn   {nullptr};
    QLabel*      m_statusLabel {nullptr};

    QString m_selectedDrive;
};

} // namespace Msv::Ui
