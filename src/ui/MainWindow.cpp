#include "MainWindow.h"

#include "core/IScenarioDispatcher.h"
#include "core/IDeviceModel.h"
#include "core/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QTableWidget>
#include <QTextEdit>
#include <QHeaderView>
#include <QGroupBox>
#include <QSplitter>
#include <QStatusBar>

namespace Msv::Ui {

MainWindow::MainWindow(Core::IScenarioDispatcher* dispatcher,
                       Core::IDeviceModel*         deviceModel,
                       Core::ModelLogBackend*       logModel,
                       QWidget* parent)
    : QMainWindow(parent)
    , m_dispatcher (dispatcher)
    , m_deviceModel(deviceModel)
    , m_logModel   (logModel)
{
    setWindowTitle(tr("МСВ — Стендовое ПО v0.1"));
    resize(1024, 720);

    buildUi();
    connectSignals();
    updateControlButtons();
}

// ── Построение интерфейса ─────────────────────────────────────────────────────

void MainWindow::buildUi()
{
    auto* central = new QWidget(this);
    setCentralWidget(central);

    auto* rootLayout = new QVBoxLayout(central);

    // — Шапка: статус + инфо об устройстве ────────────────────────────────────
    {
        auto* headerBox    = new QGroupBox(tr("Устройство"), central);
        auto* headerLayout = new QHBoxLayout(headerBox);

        m_deviceInfoLabel = new QLabel(tr("Устройство не обнаружено"), headerBox);
        m_statusLabel     = new QLabel(tr("Готов к запуску"), headerBox);
        m_statusLabel->setAlignment(Qt::AlignRight);

        headerLayout->addWidget(m_deviceInfoLabel, 1);
        headerLayout->addWidget(m_statusLabel);
        rootLayout->addWidget(headerBox);
    }

    // — Прогресс ──────────────────────────────────────────────────────────────
    {
        m_progressBar = new QProgressBar(central);
        m_progressBar->setRange(0, 100);
        m_progressBar->setValue(0);
        m_progressBar->setTextVisible(true);
        rootLayout->addWidget(m_progressBar);
    }

    // — Кнопки управления ─────────────────────────────────────────────────────
    {
        auto* btnLayout = new QHBoxLayout;

        m_startBtn   = new QPushButton(tr("▶  Запустить"), central);
        m_confirmBtn = new QPushButton(tr("✔  Выполнено"), central);
        m_abortBtn   = new QPushButton(tr("✖  Прервать"),  central);
        m_abortBtn->setObjectName("abortBtn"); // для стилизации красным

        btnLayout->addWidget(m_startBtn);
        btnLayout->addWidget(m_confirmBtn);
        btnLayout->addStretch();
        btnLayout->addWidget(m_abortBtn);
        rootLayout->addLayout(btnLayout);
    }

    // — Сплиттер: слева шаги, справа инструкция + лог ─────────────────────────
    auto* splitter = new QSplitter(Qt::Horizontal, central);

    // Таблица шагов
    {
        m_stepsTable = new QTableWidget(splitter);
        m_stepsTable->setColumnCount(3);
        m_stepsTable->setHorizontalHeaderLabels({tr("№"), tr("Шаг"), tr("Результат")});
        m_stepsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        m_stepsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
        m_stepsTable->setSelectionMode(QAbstractItemView::SingleSelection);
        splitter->addWidget(m_stepsTable);

        // Заполним таблицу шагами после запуска (в onStepStarted)
    }

    // Правая панель: инструкция + лог
    {
        auto* rightWidget = new QWidget(splitter);
        auto* rightLayout = new QVBoxLayout(rightWidget);

        auto* promptBox = new QGroupBox(tr("Инструкция оператору"), rightWidget);
        auto* promptLayout = new QVBoxLayout(promptBox);
        m_operatorPrompt = new QTextEdit(promptBox);
        m_operatorPrompt->setReadOnly(true);
        m_operatorPrompt->setMaximumHeight(160);
        promptLayout->addWidget(m_operatorPrompt);
        rightLayout->addWidget(promptBox);

        auto* logBox    = new QGroupBox(tr("Журнал событий"), rightWidget);
        auto* logLayout = new QVBoxLayout(logBox);
        m_logView = new QTextEdit(logBox);
        m_logView->setReadOnly(true);
        m_logView->setFont(QFont("Courier New", 9));
        logLayout->addWidget(m_logView);
        rightLayout->addWidget(logBox, 1);

        splitter->addWidget(rightWidget);
    }

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    rootLayout->addWidget(splitter, 1);

    statusBar()->showMessage(tr("Готов"));
}

// ── Подключение сигналов ──────────────────────────────────────────────────────

void MainWindow::connectSignals()
{
    // Кнопки → диспетчер
    connect(m_startBtn,   &QPushButton::clicked, this, &MainWindow::onStartClicked);
    connect(m_confirmBtn, &QPushButton::clicked, this, &MainWindow::onConfirmClicked);
    connect(m_abortBtn,   &QPushButton::clicked, this, &MainWindow::onAbortClicked);

    // Диспетчер → отображение
    connect(m_dispatcher, &Core::IScenarioDispatcher::stateChanged,
            this,          &MainWindow::onStateChanged);
    connect(m_dispatcher, &Core::IScenarioDispatcher::stepStarted,
            this,          &MainWindow::onStepStarted);
    connect(m_dispatcher, &Core::IScenarioDispatcher::stepFinished,
            this,          &MainWindow::onStepFinished);
    connect(m_dispatcher, &Core::IScenarioDispatcher::operatorActionRequired,
            this,          &MainWindow::onOperatorActionRequired);
    connect(m_dispatcher, &Core::IScenarioDispatcher::progressChanged,
            m_progressBar, &QProgressBar::setValue);
    connect(m_dispatcher, &Core::IScenarioDispatcher::scenarioFinished,
            this,          &MainWindow::onScenarioFinished);

    // Модель устройства → отображение
    connect(m_deviceModel, &Core::IDeviceModel::snapshotChanged,
            this,           &MainWindow::onSnapshotChanged);

    // Лог → текстовая область
    connect(m_logModel, &Core::ModelLogBackend::entryAdded,
            this,        &MainWindow::onLogEntryAdded);
}

// ── Слоты ─────────────────────────────────────────────────────────────────────

void MainWindow::onStartClicked()   { m_dispatcher->start(); }
void MainWindow::onConfirmClicked() { m_dispatcher->confirmOperatorAction(); }
void MainWindow::onAbortClicked()   { m_dispatcher->abort(); }

void MainWindow::onStateChanged(Core::DispatcherState newState)
{
    using S = Core::DispatcherState;
    const QMap<S, QString> labels{
        {S::Idle,            tr("Готов к запуску")},
        {S::Running,         tr("Выполняется автоматическая проверка…")},
        {S::WaitingOperator, tr("⚠  Требуется действие оператора")},
        {S::Paused,          tr("Пауза")},
        {S::Finished,        tr("Сценарий завершён")},
        {S::Aborted,         tr("Прервано")},
    };
    m_statusLabel->setText(labels.value(newState, tr("?")));
    statusBar()->showMessage(labels.value(newState));
    updateControlButtons();
}

void MainWindow::onStepStarted(int idx, const Core::StepDescriptor& desc)
{
    // Добавляем строку в таблицу, если нужно
    if (idx >= m_stepsTable->rowCount())
        m_stepsTable->setRowCount(idx + 1);

    m_stepsTable->setItem(idx, 0, new QTableWidgetItem(QString::number(idx + 1)));
    m_stepsTable->setItem(idx, 1, new QTableWidgetItem(desc.title));
    m_stepsTable->setItem(idx, 2, new QTableWidgetItem(tr("…")));
    m_stepsTable->scrollToItem(m_stepsTable->item(idx, 0));
}

void MainWindow::onStepFinished(int idx, Core::StepResult result, const QString& /*details*/)
{
    using R = Core::StepResult;
    const QMap<R, QPair<QString, QColor>> display{
        {R::Pass,    {tr("PASS"),    QColor("#2e7d32")}},
        {R::Fail,    {tr("FAIL"),    QColor("#c62828")}},
        {R::Warning, {tr("WARNING"), QColor("#f57f17")}},
        {R::Skipped, {tr("SKIP"),    QColor("#616161")}},
    };

    if (idx < m_stepsTable->rowCount()) {
        const auto [text, color] = display.value(result, {tr("?"), Qt::gray});
        auto* item = new QTableWidgetItem(text);
        item->setForeground(color);
        m_stepsTable->setItem(idx, 2, item);
    }
}

void MainWindow::onOperatorActionRequired(int /*idx*/, const Core::StepDescriptor& desc)
{
    m_operatorPrompt->setPlainText(desc.description);
}

void MainWindow::onSnapshotChanged(const Core::DeviceSnapshot& snap)
{
    if (!snap.isIdentified()) return;
    m_deviceInfoLabel->setText(
        QStringLiteral("IP: %1   MAC: %2   FW: %3   S/N: %4")
            .arg(snap.ipAddress.toString())
            .arg(snap.macAddress)
            .arg(snap.firmwareVersion)
            .arg(snap.serialNumber)
    );
}

void MainWindow::onLogEntryAdded(const Core::LogEntry& entry)
{
    static const QMap<Core::LogLevel, QString> colors{
        {Core::LogLevel::Debug,   "#888888"},
        {Core::LogLevel::Info,    "#000000"},
        {Core::LogLevel::Warning, "#e65100"},
        {Core::LogLevel::Error,   "#b71c1c"},
        {Core::LogLevel::Fatal,   "#b71c1c"},
    };
    const QString color = colors.value(entry.level, "#000000");
    const QString html  = QStringLiteral(
        "<span style='color:%1'>[%2] [%3] %4</span>")
        .arg(color)
        .arg(entry.timestamp.toString("hh:mm:ss.zzz"))
        .arg(entry.source.leftJustified(22))
        .arg(entry.message.toHtmlEscaped());

    m_logView->append(html);
}

void MainWindow::onScenarioFinished(bool overallPass)
{
    const QString msg = overallPass
        ? tr("✔ Сценарий завершён: ГОДЕН")
        : tr("✖ Сценарий завершён: НЕ ГОДЕН");
    m_operatorPrompt->setPlainText(msg);
    statusBar()->showMessage(msg);
}

void MainWindow::updateControlButtons()
{
    using S = Core::DispatcherState;
    const bool idle     = (m_dispatcher->state() == S::Idle);
    const bool waiting  = (m_dispatcher->state() == S::WaitingOperator);
    const bool running  = (m_dispatcher->state() == S::Running ||
                           m_dispatcher->state() == S::WaitingOperator);

    m_startBtn  ->setEnabled(idle);
    m_confirmBtn->setEnabled(waiting);
    m_abortBtn  ->setEnabled(running);
}

} // namespace Msv::Ui
