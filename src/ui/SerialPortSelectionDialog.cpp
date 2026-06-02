#include "SerialPortSelectionDialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QSerialPortInfo>

namespace Msv::Ui {

SerialPortSelectionDialog::SerialPortSelectionDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Выбор COM-порта");
    setMinimumSize(460, 300);
    setModal(true);
    applyTheme();
    buildUi();
    populatePorts();
}

void SerialPortSelectionDialog::applyTheme()
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
        QLabel#statusLabel { color: #505050; font-size: 9pt; }
        QListWidget {
            background-color: #0e0e0e;
            border: 1px solid #2a2a2a;
            border-radius: 4px;
            outline: none;
        }
        QListWidget::item {
            padding: 8px 14px;
            border-bottom: 1px solid #1a1a1a;
            color: #c0c0c0;
            font-family: "JetBrains Mono", "Consolas", monospace;
            font-size: 9pt;
        }
        QListWidget::item:selected {
            background-color: #0d2137;
            border-left: 3px solid #00e5cc;
            color: #e8e8e8;
        }
        QComboBox {
            background-color: #1c1c1c;
            border: 1px solid #2e2e2e;
            border-radius: 4px;
            padding: 4px 10px;
            color: #c0c0c0;
            min-width: 100px;
        }
        QComboBox::drop-down { border: none; }
        QComboBox QAbstractItemView {
            background-color: #1c1c1c;
            border: 1px solid #2e2e2e;
            color: #c0c0c0;
            selection-background-color: #0d2137;
        }
        QPushButton {
            border: none; border-radius: 4px;
            padding: 9px 22px; font-size: 10pt; font-weight: bold;
        }
        QPushButton#selectBtn {
            background-color: #00897b; color: #ffffff; min-width: 120px;
        }
        QPushButton#selectBtn:hover { background-color: #00a896; }
        QPushButton#selectBtn:disabled {
            background-color: #1a1a1a; color: #333333;
            border: 1px solid #262626;
        }
        QPushButton#refreshBtn {
            background-color: transparent; color: #505050;
            border: 1px solid #2e2e2e;
        }
        QPushButton#refreshBtn:hover { color: #909090; border-color: #444444; }
        QScrollBar:vertical {
            background: transparent; width: 6px;
        }
        QScrollBar::handle:vertical {
            background: #2e2e2e; border-radius: 3px; min-height: 20px;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
    )");
}

void SerialPortSelectionDialog::buildUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(14);

    auto* title = new QLabel("ВЫБОР COM-ПОРТА (UART-ДУБЛИКАТ)", this);
    title->setObjectName("titleLabel");
    root->addWidget(title);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setObjectName("statusLabel");
    root->addWidget(m_statusLabel);

    m_portList = new QListWidget(this);
    m_portList->setSelectionMode(QAbstractItemView::SingleSelection);
    root->addWidget(m_portList, 1);

    // Скорость + кнопки
    auto* bottomRow = new QHBoxLayout;
    bottomRow->setSpacing(10);

    auto* baudLabel = new QLabel("Скорость:", this);
    baudLabel->setStyleSheet("color: #606060; font-size: 9pt;");

    m_baudCombo = new QComboBox(this);
    for (int baud : {4800, 9600, 19200, 38400, 57600, 115200})
        m_baudCombo->addItem(QString::number(baud), baud);
    m_baudCombo->setCurrentText("115200");

    m_selectBtn = new QPushButton("ВЫБРАТЬ", this);
    m_selectBtn->setObjectName("selectBtn");
    m_selectBtn->setFixedHeight(38);
    m_selectBtn->setEnabled(false);

    auto* refreshBtn = new QPushButton("↻  Обновить", this);
    refreshBtn->setObjectName("refreshBtn");
    refreshBtn->setFixedHeight(38);

    bottomRow->addWidget(baudLabel);
    bottomRow->addWidget(m_baudCombo);
    bottomRow->addStretch();
    bottomRow->addWidget(refreshBtn);
    bottomRow->addWidget(m_selectBtn);
    root->addLayout(bottomRow);

    connect(m_selectBtn, &QPushButton::clicked,
            this, &SerialPortSelectionDialog::onSelectClicked);
    connect(refreshBtn, &QPushButton::clicked,
            this, &SerialPortSelectionDialog::onRefreshClicked);
    connect(m_portList, &QListWidget::itemSelectionChanged,
            this, [this]() {
        m_selectBtn->setEnabled(m_portList->currentRow() >= 0);
    });
    connect(m_portList, &QListWidget::itemDoubleClicked,
            this, &SerialPortSelectionDialog::onSelectClicked);
}

void SerialPortSelectionDialog::populatePorts()
{
    m_portList->clear();
    const auto ports = QSerialPortInfo::availablePorts();

    m_statusLabel->setText(ports.isEmpty()
        ? "COM-порты не найдены"
        : QStringLiteral("Доступно портов: %1").arg(ports.size()));

    for (const auto& info : ports) {
        const QString text = QStringLiteral("%1  —  %2")
            .arg(info.portName())
            .arg(info.description().isEmpty() ? "нет описания" : info.description());
        m_portList->addItem(text);
    }

    m_selectBtn->setEnabled(false);
}

void SerialPortSelectionDialog::onSelectClicked()
{
    const int row = m_portList->currentRow();
    if (row < 0) return;

    const auto ports = QSerialPortInfo::availablePorts();
    if (row >= ports.size()) return;

    m_selectedPort = ports[row].portName();
    m_selectedBaud = m_baudCombo->currentData().toInt();
    accept();
}

void SerialPortSelectionDialog::onRefreshClicked()
{
    populatePorts();
}

} // namespace Msv::Ui
