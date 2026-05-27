#include "DeviceSelectionDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QInputDialog>
#include <QMessageBox>
#include <QHostAddress>
#include <QFont>

namespace Msv::Ui {

DeviceSelectionDialog::DeviceSelectionDialog(
    const QList<Network::WhoIAmResponse>& devices,
    QWidget* parent)
    : QDialog(parent)
    , m_devices(devices)
{
    setWindowTitle("Выбор устройства");
    setMinimumSize(520, 340);
    setModal(true);

    applyTheme();
    buildUi(devices);
}

void DeviceSelectionDialog::applyTheme()
{
    setStyleSheet(R"(
        QDialog {
            background-color: #141414;
            color: #d8d8d8;
            font-family: "Segoe UI", Arial, sans-serif;
        }
        QLabel#titleLabel {
            color: #00e5cc;
            font-size: 8pt;
            font-weight: bold;
            letter-spacing: 2px;
        }
        QLabel#statusLabel {
            color: #505050;
            font-size: 9pt;
        }
        QListWidget {
            background-color: #0e0e0e;
            border: 1px solid #2a2a2a;
            border-radius: 4px;
            outline: none;
        }
        QListWidget::item {
            padding: 10px 14px;
            border-bottom: 1px solid #1a1a1a;
            color: #c0c0c0;
        }
        QListWidget::item:selected {
            background-color: #0d2137;
            border-left: 3px solid #00e5cc;
            color: #e8e8e8;
        }
        QListWidget::item:hover:!selected {
            background-color: #1a1a1a;
        }
        QPushButton {
            border: none;
            border-radius: 4px;
            padding: 9px 22px;
            font-size: 10pt;
            font-weight: bold;
        }
        QPushButton#selectBtn {
            background-color: #00897b;
            color: #ffffff;
            min-width: 130px;
        }
        QPushButton#selectBtn:hover   { background-color: #00a896; }
        QPushButton#selectBtn:disabled {
            background-color: #1a1a1a;
            color: #333333;
            border: 1px solid #262626;
        }
        QPushButton#manualBtn {
            background-color: transparent;
            color: #505050;
            border: 1px solid #2e2e2e;
        }
        QPushButton#manualBtn:hover { color: #909090; border-color: #444444; }

        QScrollBar:vertical {
            background: transparent; width: 6px;
        }
        QScrollBar::handle:vertical {
            background: #2e2e2e; border-radius: 3px; min-height: 20px;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
    )");
}

void DeviceSelectionDialog::buildUi(const QList<Network::WhoIAmResponse>& devices)
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(14);

    // Заголовок
    auto* title = new QLabel("ОБНАРУЖЕННЫЕ УСТРОЙСТВА", this);
    title->setObjectName("titleLabel");
    root->addWidget(title);

    // Статус сканирования
    m_statusLabel = new QLabel(this);
    m_statusLabel->setObjectName("statusLabel");
    m_statusLabel->setText(devices.isEmpty()
        ? "Устройства не найдены в сети"
        : QStringLiteral("Найдено устройств: %1").arg(devices.size()));
    root->addWidget(m_statusLabel);

    // Список
    m_list = new QListWidget(this);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);

    for (const auto& dev : devices) {
        const QString text = QStringLiteral("%1\n"
            "IP: %2     FW: %3     ID: %4")
            .arg(dev.deviceName())
            .arg(dev.ipAddress.toString())
            .arg(dev.firmwareVersion)
            .arg(dev.deviceId);

        auto* item = new QListWidgetItem(text, m_list);
        item->setFont(QFont("JetBrains Mono, Consolas", 9));
        m_list->addItem(item);
    }

    // Если только одно устройство — выбрать его сразу
    if (devices.size() == 1)
        m_list->setCurrentRow(0);

    root->addWidget(m_list, 1);

    // Кнопки
    auto* btnRow = new QHBoxLayout;
    btnRow->setSpacing(10);

    m_selectBtn = new QPushButton("ВЫБРАТЬ", this);
    m_selectBtn->setObjectName("selectBtn");
    m_selectBtn->setFixedHeight(38);
    m_selectBtn->setEnabled(!devices.isEmpty());

    m_manualBtn = new QPushButton("Ввести IP вручную", this);
    m_manualBtn->setObjectName("manualBtn");
    m_manualBtn->setFixedHeight(38);

    btnRow->addWidget(m_selectBtn);
    btnRow->addStretch();
    btnRow->addWidget(m_manualBtn);
    root->addLayout(btnRow);

    connect(m_selectBtn, &QPushButton::clicked,
            this, &DeviceSelectionDialog::onSelectClicked);
    connect(m_manualBtn, &QPushButton::clicked,
            this, &DeviceSelectionDialog::onManualClicked);
    connect(m_list, &QListWidget::itemDoubleClicked,
            this, &DeviceSelectionDialog::onListDoubleClicked);
    connect(m_list, &QListWidget::itemSelectionChanged,
            this, &DeviceSelectionDialog::onListSelectionChanged);
}

void DeviceSelectionDialog::onSelectClicked()
{
    const int row = m_list->currentRow();
    if (row < 0 || row >= m_devices.size()) return;

    m_selected    = m_devices[row];
    m_manualEntry = false;
    accept();
}

void DeviceSelectionDialog::onListDoubleClicked()
{
    onSelectClicked();
}

void DeviceSelectionDialog::onListSelectionChanged()
{
    m_selectBtn->setEnabled(m_list->currentRow() >= 0);
}

void DeviceSelectionDialog::onManualClicked()
{
    bool ok = false;
    const QString ip = QInputDialog::getText(
        this,
        "Ручной ввод IP",
        "Введите IP-адрес устройства:",
        QLineEdit::Normal,
        "192.168.1.",
        &ok
    );

    if (!ok || ip.trimmed().isEmpty()) return;

    const QHostAddress addr(ip.trimmed());
    if (addr.isNull()) {
        QMessageBox::warning(this, "Ошибка",
            QStringLiteral("«%1» — неверный IP-адрес.").arg(ip));
        return;
    }

    m_manualIp    = addr;
    m_manualEntry = true;
    accept();
}

} // namespace Msv::Ui
