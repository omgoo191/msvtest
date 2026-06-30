#include "UsbDriveSelectionDialog.h"
#include "usb/UsbChecker.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>

namespace Msv::Ui {

UsbDriveSelectionDialog::UsbDriveSelectionDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Выбор USB-носителя");
    setMinimumSize(420, 320);
    setModal(true);
    applyTheme();
    buildUi();
    populateDrives();
}

void UsbDriveSelectionDialog::applyTheme()
{
    setStyleSheet(R"(
        QDialog { background-color: #141414; color: #d8d8d8;
                  font-family: "Segoe UI", Arial, sans-serif; }
        QLabel#titleLabel {
            color: #00e5cc; font-size: 8pt; font-weight: bold; letter-spacing: 2px;
        }
        QLabel#statusLabel { color: #505050; font-size: 9pt; }
        QListWidget {
            background-color: #0e0e0e; border: 1px solid #2a2a2a;
            border-radius: 4px; outline: none;
        }
        QListWidget::item {
            padding: 10px 14px; border-bottom: 1px solid #1a1a1a;
            color: #c0c0c0; font-family: "JetBrains Mono", "Consolas", monospace;
            font-size: 10pt;
        }
        QListWidget::item:selected {
            background-color: #0d2137; border-left: 3px solid #00e5cc; color: #e8e8e8;
        }
        QPushButton {
            border: none; border-radius: 4px; padding: 9px 22px;
            font-size: 10pt; font-weight: bold;
        }
        QPushButton#selectBtn { background-color: #00897b; color: #ffffff; min-width: 120px; }
        QPushButton#selectBtn:hover { background-color: #00a896; }
        QPushButton#selectBtn:disabled {
            background-color: #1a1a1a; color: #333333; border: 1px solid #262626;
        }
        QPushButton#refreshBtn {
            background-color: transparent; color: #505050; border: 1px solid #2e2e2e;
        }
        QPushButton#refreshBtn:hover { color: #909090; border-color: #444444; }
    )");
}

void UsbDriveSelectionDialog::buildUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(14);

    auto* title = new QLabel("ВЫБОР USB-НОСИТЕЛЯ", this);
    title->setObjectName("titleLabel");
    root->addWidget(title);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setObjectName("statusLabel");
    root->addWidget(m_statusLabel);

    m_driveList = new QListWidget(this);
    m_driveList->setSelectionMode(QAbstractItemView::SingleSelection);
    root->addWidget(m_driveList, 1);

    auto* bottomRow = new QHBoxLayout;
    bottomRow->setSpacing(10);

    auto* refreshBtn = new QPushButton("↻  Обновить", this);
    refreshBtn->setObjectName("refreshBtn");
    refreshBtn->setFixedHeight(38);

    m_selectBtn = new QPushButton("ВЫБРАТЬ", this);
    m_selectBtn->setObjectName("selectBtn");
    m_selectBtn->setFixedHeight(38);
    m_selectBtn->setEnabled(false);

    bottomRow->addWidget(refreshBtn);
    bottomRow->addStretch();
    bottomRow->addWidget(m_selectBtn);
    root->addLayout(bottomRow);

    connect(m_selectBtn, &QPushButton::clicked,
            this, &UsbDriveSelectionDialog::onSelectClicked);
    connect(refreshBtn, &QPushButton::clicked,
            this, &UsbDriveSelectionDialog::onRefreshClicked);
    connect(m_driveList, &QListWidget::itemSelectionChanged,
            this, [this]() { m_selectBtn->setEnabled(m_driveList->currentRow() >= 0); });
    connect(m_driveList, &QListWidget::itemDoubleClicked,
            this, &UsbDriveSelectionDialog::onSelectClicked);
}

void UsbDriveSelectionDialog::populateDrives()
{
    m_driveList->clear();
    const auto drives = Usb::UsbChecker::availableDrives();

    m_statusLabel->setText(drives.isEmpty()
        ? "Диски не найдены"
        : QStringLiteral("Доступно дисков: %1").arg(drives.size()));

    for (const auto& d : drives)
        m_driveList->addItem(QStringLiteral("%1\\").arg(d));

    m_selectBtn->setEnabled(false);
}

void UsbDriveSelectionDialog::onSelectClicked()
{
    const int row = m_driveList->currentRow();
    if (row < 0) return;

    const auto drives = Usb::UsbChecker::availableDrives();
    if (row >= drives.size()) return;

    m_selectedDrive = drives[row];
    accept();
}

void UsbDriveSelectionDialog::onRefreshClicked()
{
    populateDrives();
}

} // namespace Msv::Ui
