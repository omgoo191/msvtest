#include "MainWindow.h"
#include "StepItemWidget.h"
#include "DeviceSelectionDialog.h"
#include "core/IScenarioDispatcher.h"
#include "core/MsvScenarioDispatcher.h"
#include "core/IDeviceModel.h"
#include "core/Logger.h"
#include "network/WhoIAmTypes.h"

#include <QInputDialog>
#include <QMessageBox>
#include <QHostAddress>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QTextEdit>
#include <QScrollArea>
#include <QSplitter>
#include <QFrame>
#include <QStatusBar>
#include <QFont>
#include <QFontDatabase>

namespace Msv::Ui {

// ─────────────────────────────────────────────────────────────────────────────

MainWindow::MainWindow(Core::IScenarioDispatcher* dispatcher,
                       Core::IDeviceModel*         deviceModel,
                       Core::ModelLogBackend*       logModel,
                       QWidget* parent)
    : QMainWindow(parent)
    , m_dispatcher (dispatcher)
    , m_deviceModel(deviceModel)
    , m_logModel   (logModel)
{
    setWindowTitle("МСВ ТЕСТЕР  ·  v0.1");
    setMinimumSize(1100, 720);
    resize(1280, 800);

    applyTheme();
    buildUi();
    connectSignals();
    updateButtons();

    // Заполнить шаги из диспетчера
    const auto steps = dispatcher->allSteps();
    for (int i = 0; i < steps.size(); ++i) {
        auto* item = new StepItemWidget(i, steps[i], m_stepsContainer);
        m_stepItems.append(item);
        m_stepsContainer->layout()->addWidget(item);
    }
    // Спейсер снизу
    qobject_cast<QVBoxLayout*>(m_stepsContainer->layout())->addStretch(1);
}

// ── Тема ─────────────────────────────────────────────────────────────────────

void MainWindow::applyTheme()
{
    setStyleSheet(R"(
        /* ── Базовые ── */
        QMainWindow, QWidget {
            background-color: #141414;
            color: #d8d8d8;
            font-family: "Segoe UI", "Helvetica Neue", Arial, sans-serif;
            font-size: 10pt;
        }

        /* ── Панели / карточки ── */
        QFrame#topBar {
            background-color: transparent;
            border-bottom: 1px solid #2a2a2a;
        }
        QFrame#stepsPanel {
            background-color: #111111;
            border-right: 1px solid #232323;
        }
        QFrame#card {
            background-color: #1c1c1c;
            border: 1px solid #282828;
            border-radius: 6px;
        }
        QFrame#logPanel {
            background-color: #0e0e0e;
            border-top: 1px solid #232323;
        }

        /* ── Лейблы секций ── */
        QLabel#sectionLabel {
            color: #00e5cc;
            font-size: 7.5pt;
            font-weight: bold;
            letter-spacing: 2px;
        }
        QLabel#stepIndex {
            color: #404040;
            font-size: 28pt;
            font-family: "JetBrains Mono", "Consolas", monospace;
            font-weight: bold;
        }
        QLabel#stepTitle {
            color: #e8e8e8;
            font-size: 16pt;
            font-weight: bold;
        }
        QLabel#stepType {
            color: #505050;
            font-size: 9pt;
        }
        QLabel#promptHeader {
            color: #00e5cc;
            font-size: 8pt;
            font-weight: bold;
            letter-spacing: 2px;
        }
        QLabel#promptBody {
            color: #c0c0c0;
            font-size: 11pt;
            line-height: 1.6;
        }

        /* ── Кнопки ── */
        QPushButton {
            border: none;
            border-radius: 4px;
            padding: 10px 24px;
            font-size: 10pt;
            font-weight: bold;
            letter-spacing: 0.5px;
        }
        QPushButton#startBtn {
            background-color: #00897b;
            color: #ffffff;
            min-width: 140px;
        }
        QPushButton#startBtn:hover   { background-color: #00a896; }
        QPushButton#startBtn:pressed { background-color: #00796b; }
        QPushButton#startBtn:disabled {
            background-color: #1a1a1a;
            color: #383838;
            border: 1px solid #282828;
        }

        QPushButton#confirmBtn {
            background-color: #1b5e1b;
            color: #4caf50;
            border: 1px solid #2e7d32;
            min-width: 140px;
        }
        QPushButton#confirmBtn:hover   { background-color: #1e6b1e; color: #66bb6a; }
        QPushButton#confirmBtn:pressed { background-color: #174d17; }
        QPushButton#confirmBtn:disabled {
            background-color: #1a1a1a;
            color: #303030;
            border: 1px solid #242424;
        }

        QPushButton#abortBtn {
            background-color: transparent;
            color: #555555;
            border: 1px solid #333333;
        }
        QPushButton#abortBtn:hover   { color: #f44336; border-color: #c62828; }
        QPushButton#abortBtn:pressed { background-color: #1a0000; }
        QPushButton#abortBtn:disabled { color: #2a2a2a; border-color: #222222; }

        /* ── Прогрессбар ── */
        QProgressBar {
            background-color: #1a1a1a;
            border: none;
            border-radius: 2px;
            max-height: 3px;
        }
        QProgressBar::chunk {
            background-color: #00e5cc;
            border-radius: 2px;
        }

        /* ── Лог ── */
        QTextEdit#logView {
            background-color: #0a0a0a;
            border: none;
            color: #606060;
            font-family: "JetBrains Mono", "Consolas", "Courier New", monospace;
            font-size: 8.5pt;
            selection-background-color: #1a3a3a;
        }

        /* ── Сплиттер ── */
        QSplitter::handle { background-color: #222222; }
        QSplitter::handle:horizontal { width: 1px;  }
        QSplitter::handle:vertical   { height: 1px; }

        /* ── Скроллбар ── */
        QScrollBar:vertical {
            background: transparent;
            width: 6px;
            margin: 0;
        }
        QScrollBar::handle:vertical {
            background: #2e2e2e;
            border-radius: 3px;
            min-height: 24px;
        }
        QScrollBar::handle:vertical:hover { background: #3e3e3e; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }

        QScrollBar:horizontal { height: 6px; }
        QScrollBar::handle:horizontal {
            background: #2e2e2e;
            border-radius: 3px;
        }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }

        /* ── Статусбар ── */
        QStatusBar {
            background-color: #0e0e0e;
            color: #404040;
            font-size: 8pt;
            border-top: 1px solid #1e1e1e;
        }
    )");
}

// ── Построение UI ─────────────────────────────────────────────────────────────

void MainWindow::buildUi()
{
    auto* central = new QWidget(this);
    setCentralWidget(central);
    auto* rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

	// ════════════════════════════════════════════════════════════════════════
	// TOP BAR
	// ════════════════════════════════════════════════════════════════════════
	{
		auto* bar = new QFrame(central);
		bar->setObjectName("topBar");
		bar->setFixedHeight(56);
		auto* barLayout = new QHBoxLayout(bar);
		barLayout->setContentsMargins(20, 0, 20, 0);
		barLayout->setSpacing(0);

		// Логотип / название
		auto* appName = new QLabel("МСВ ТЕСТЕР", bar);
		appName->setStyleSheet(
				"color: #00e5cc;"
				"font-size: 12pt;"
				"font-weight: bold;"
				"letter-spacing: 3px;"
				"font-family: 'JetBrains Mono', 'Consolas', monospace;"
		);

		auto* version = new QLabel("  v0.1", bar);
		version->setStyleSheet("color: #303030; font-size: 9pt;");

		// Данные устройства
		m_deviceLabel = new QLabel("устройство не обнаружено", bar);
		m_deviceLabel->setStyleSheet(
				"color: #484848;"
				"font-size: 9pt;"
				"font-family: 'JetBrains Mono', 'Consolas', monospace;"
		);

		// Добавляем виджеты в layout с явным выравниванием по центру вертикали (Qt::AlignVCenter)
		barLayout->addWidget(appName, 0, Qt::AlignVCenter);
		barLayout->addWidget(version, 0, Qt::AlignVCenter);
		barLayout->addSpacing(24);
		barLayout->addWidget(m_deviceLabel, 1, Qt::AlignVCenter); // Растягивается (stretch: 1)

		// Статус-индикатор (справа)
		m_statusDot  = new QLabel(bar);
		m_statusDot->setFixedSize(8, 8);
		m_statusText = new QLabel("ГОТОВ", bar);

		// Стартовый цвет делаем через background-color
		m_statusDot->setStyleSheet("background-color: #303030; border-radius: 4px;");
		m_statusText->setStyleSheet(
				"color: #404040;"
				"font-size: 8pt;"
				"font-weight: bold;"
				"letter-spacing: 2px;"
		);

		// Кружок и текст статуса тоже выравниваем по центру вертикали
		barLayout->addWidget(m_statusDot, 0, Qt::AlignVCenter);
		barLayout->addSpacing(8); // Чуть увеличил отступ для красоты
		barLayout->addWidget(m_statusText, 0, Qt::AlignVCenter);

		rootLayout->addWidget(bar);

		// Прогрессбар — тонкая полоска под шапкой
		m_progressBar = new QProgressBar(central);
		m_progressBar->setRange(0, 100);
		m_progressBar->setValue(0);
		m_progressBar->setTextVisible(false);
		m_progressBar->setFixedHeight(3);
		rootLayout->addWidget(m_progressBar);


        // Прогрессбар — тонкая полоска под шапкой
        m_progressBar = new QProgressBar(central);
        m_progressBar->setRange(0, 100);
        m_progressBar->setValue(0);
        m_progressBar->setTextVisible(false);
        m_progressBar->setFixedHeight(3);
        rootLayout->addWidget(m_progressBar);
    }

    // ════════════════════════════════════════════════════════════════════════
    // MAIN SPLITTER  (верт: рабочая зона | лог)
    // ════════════════════════════════════════════════════════════════════════
    auto* vSplit = new QSplitter(Qt::Vertical, central);
    vSplit->setHandleWidth(1);
    rootLayout->addWidget(vSplit, 1);

    // ────────────────────────────────────────────────────────────────────────
    // Горизонтальный сплиттер: ШАГИ (слева) | ГЛАВНАЯ ПАНЕЛЬ (справа)
    // ────────────────────────────────────────────────────────────────────────
    auto* hSplit = new QSplitter(Qt::Horizontal, vSplit);
    hSplit->setHandleWidth(1);

    // ── ШАГИ (левая панель) ──────────────────────────────────────────────────
    {
        auto* stepsFrame = new QFrame(hSplit);
        stepsFrame->setObjectName("stepsPanel");
        stepsFrame->setMinimumWidth(240);
        stepsFrame->setMaximumWidth(340);
        auto* fl = new QVBoxLayout(stepsFrame);
        fl->setContentsMargins(0, 16, 0, 0);
        fl->setSpacing(0);

        auto* header = new QLabel("ПЛАН ПРОВЕРКИ", stepsFrame);
        header->setObjectName("sectionLabel");
        header->setContentsMargins(16, 0, 0, 12);
        fl->addWidget(header);

        auto* separator = new QFrame(stepsFrame);
        separator->setFrameShape(QFrame::HLine);
        separator->setStyleSheet("color: #1e1e1e;");
        fl->addWidget(separator);

        auto* scroll = new QScrollArea(stepsFrame);
        scroll->setWidgetResizable(true);
        scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setStyleSheet("QScrollArea { background: transparent; }");

        m_stepsContainer = new QWidget(scroll);
        m_stepsContainer->setStyleSheet("background: transparent;");
        auto* stepsLayout = new QVBoxLayout(m_stepsContainer);
        stepsLayout->setContentsMargins(0, 4, 0, 4);
        stepsLayout->setSpacing(0);
        scroll->setWidget(m_stepsContainer);
        fl->addWidget(scroll, 1);

        hSplit->addWidget(stepsFrame);
    }

    // ── ГЛАВНАЯ ПАНЕЛЬ ───────────────────────────────────────────────────────
    {
        auto* mainWidget = new QWidget(hSplit);
        auto* ml = new QVBoxLayout(mainWidget);
        ml->setContentsMargins(28, 24, 28, 20);
        ml->setSpacing(16);

        // Заголовок текущего шага
        {
            auto* stepHeader = new QWidget(mainWidget);
            auto* hl = new QHBoxLayout(stepHeader);
            hl->setContentsMargins(0, 0, 0, 0);
            hl->setSpacing(16);

            m_stepIndexLabel = new QLabel("—", stepHeader);
            m_stepIndexLabel->setObjectName("stepIndex");
            m_stepIndexLabel->setFixedWidth(52);

            auto* titleCol = new QVBoxLayout;
            titleCol->setSpacing(4);

            m_stepTitleLabel = new QLabel("Ожидание запуска", stepHeader);
            m_stepTitleLabel->setObjectName("stepTitle");

            m_stepTypeLabel  = new QLabel("", stepHeader);
            m_stepTypeLabel->setObjectName("stepType");

            titleCol->addWidget(m_stepTitleLabel);
            titleCol->addWidget(m_stepTypeLabel);

            hl->addWidget(m_stepIndexLabel);
            hl->addLayout(titleCol, 1);
            ml->addWidget(stepHeader);
        }

        // Карточка инструкции
        {
            auto* card = new QFrame(mainWidget);
            card->setObjectName("card");
            card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            auto* cl = new QVBoxLayout(card);
            cl->setContentsMargins(18, 14, 18, 14);
            cl->setSpacing(8);

            m_promptHeader = new QLabel("ИНСТРУКЦИЯ", card);
            m_promptHeader->setObjectName("promptHeader");

            // QTextEdit вместо QLabel — нет проблем с переносом и выходом текста за края
            auto* promptEdit = new QTextEdit(card);
            promptEdit->setObjectName("promptBody");
            promptEdit->setReadOnly(true);
            promptEdit->setFrameShape(QFrame::NoFrame);
            promptEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
            promptEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            promptEdit->setStyleSheet(
                "QTextEdit#promptBody {"
                "  background: transparent;"
                "  color: #c0c0c0;"
                "  font-size: 11pt;"
                "  font-family: 'Segoe UI', Arial, sans-serif;"
                "  border: none;"
                "  padding: 0px;"
                "}"
            );
            promptEdit->setPlainText("Нажмите «ЗАПУСТИТЬ» для начала сценария проверки.");
            // Сохраняем указатель через динамическое свойство для setPrompt()
            card->setProperty("promptEdit", QVariant::fromValue<QObject*>(promptEdit));
            m_promptBody = nullptr; // больше не используем m_promptBody напрямую

            cl->addWidget(m_promptHeader);
            cl->addWidget(promptEdit, 1);
            ml->addWidget(card, 1);

            // Сохраняем edit в поле для использования в setPrompt
            // Переопределяем через лямбду — храним указатель в m_promptBody как QLabel*
            // HACK: используем setObjectName чтобы найти виджет позже через findChild
            promptEdit->setObjectName("thePromptEdit");
        }

        // Кнопки
        {
            auto* btnRow = new QHBoxLayout;
            btnRow->setSpacing(10);

            m_startBtn   = new QPushButton("▶   ЗАПУСТИТЬ", mainWidget);
            m_confirmBtn = new QPushButton("✔   ВЫПОЛНЕНО", mainWidget);
            m_abortBtn   = new QPushButton("ПРЕРВАТЬ", mainWidget);

            m_startBtn  ->setObjectName("startBtn");
            m_confirmBtn->setObjectName("confirmBtn");
            m_abortBtn  ->setObjectName("abortBtn");

            m_startBtn  ->setFixedHeight(40);
            m_confirmBtn->setFixedHeight(40);
            m_abortBtn  ->setFixedHeight(40);

            btnRow->addWidget(m_startBtn);
            btnRow->addWidget(m_confirmBtn);
            btnRow->addStretch(1);
            btnRow->addWidget(m_abortBtn);
            ml->addLayout(btnRow);
        }

        hSplit->addWidget(mainWidget);
    }

    hSplit->setStretchFactor(0, 0);
    hSplit->setStretchFactor(1, 1);
    vSplit->addWidget(hSplit);

    // ── ЛОГ (нижняя панель) ──────────────────────────────────────────────────
    {
        auto* logFrame = new QFrame(vSplit);
        logFrame->setObjectName("logPanel");
        auto* ll = new QVBoxLayout(logFrame);
        ll->setContentsMargins(0, 0, 0, 0);
        ll->setSpacing(0);

        auto* logHeader = new QWidget(logFrame);
        logHeader->setFixedHeight(32);
        logHeader->setStyleSheet("background: #0e0e0e;");
        auto* lhl = new QHBoxLayout(logHeader);
        lhl->setContentsMargins(16, 0, 16, 0);

        auto* logLabel = new QLabel("ЖУРНАЛ СОБЫТИЙ", logHeader);
        logLabel->setObjectName("sectionLabel");
        lhl->addWidget(logLabel);
        lhl->addStretch();
        ll->addWidget(logHeader);

        m_logView = new QTextEdit(logFrame);
        m_logView->setObjectName("logView");
        m_logView->setReadOnly(true);
        m_logView->setMinimumHeight(120);
        ll->addWidget(m_logView, 1);

        vSplit->addWidget(logFrame);
    }

    vSplit->setStretchFactor(0, 3);
    vSplit->setStretchFactor(1, 1);

    statusBar()->setStyleSheet("QStatusBar { background:#0e0e0e; color:#303030; font-size:8pt; }");
    statusBar()->showMessage("  МСВ ТЕСТЕР  ·  готов к работе");
}

// ── Подключение сигналов ──────────────────────────────────────────────────────

void MainWindow::connectSignals()
{
    connect(m_startBtn,   &QPushButton::clicked, this, &MainWindow::onStartClicked);
    connect(m_confirmBtn, &QPushButton::clicked, this, &MainWindow::onConfirmClicked);
    connect(m_abortBtn,   &QPushButton::clicked, this, &MainWindow::onAbortClicked);

    connect(m_dispatcher, &Core::IScenarioDispatcher::stateChanged,
            this, &MainWindow::onStateChanged);
    connect(m_dispatcher, &Core::IScenarioDispatcher::stepStarted,
            this, &MainWindow::onStepStarted);
    connect(m_dispatcher, &Core::IScenarioDispatcher::stepFinished,
            this, &MainWindow::onStepFinished);
    connect(m_dispatcher, &Core::IScenarioDispatcher::operatorActionRequired,
            this, &MainWindow::onOperatorActionRequired);
    connect(m_dispatcher, &Core::IScenarioDispatcher::progressChanged,
            m_progressBar, &QProgressBar::setValue);
    connect(m_dispatcher, &Core::IScenarioDispatcher::scenarioFinished,
            this, &MainWindow::onScenarioFinished);

    connect(m_deviceModel, &Core::IDeviceModel::snapshotChanged,
            this, &MainWindow::onSnapshotChanged);

    connect(m_logModel, &Core::ModelLogBackend::entryAdded,
            this, &MainWindow::onLogEntryAdded);

    // MsvScenarioDispatcher-специфичный сигнал — подключаем через qobject_cast
    if (auto* msv = qobject_cast<Core::MsvScenarioDispatcher*>(m_dispatcher)) {
        connect(msv, &Core::MsvScenarioDispatcher::deviceSelectionRequired,
                this, &MainWindow::onDeviceSelectionRequired);
    }
}

// ── Слоты ─────────────────────────────────────────────────────────────────────

void MainWindow::onStartClicked()   { m_dispatcher->start(); }
void MainWindow::onConfirmClicked() { m_dispatcher->confirmOperatorAction(); }
void MainWindow::onAbortClicked()   { m_dispatcher->abort(); }

void MainWindow::onStateChanged(Core::DispatcherState newState)
{
    using S = Core::DispatcherState;
    struct StateVis { QString text; QString dotColor; QString textColor; };
    static const QMap<S, StateVis> vis {
        {S::Idle,            {"ГОТОВ",       "#303030", "#404040"}},
        {S::Running,         {"ВЫПОЛНЯЕТСЯ", "#00e5cc", "#00e5cc"}},
        {S::WaitingOperator, {"ЖДЁТ ОПЕРАТОРА","#ff9800","#ff9800"}},
        {S::Paused,          {"ПАУЗА",       "#505050", "#606060"}},
        {S::Finished,        {"ЗАВЕРШЁН",    "#4caf50", "#4caf50"}},
        {S::Aborted,         {"ПРЕРВАНО",    "#f44336", "#f44336"}},
    };
    const auto& v = vis.value(newState, {"?","#303030","#404040"});
    m_statusDot ->setStyleSheet(QStringLiteral("background-color: %1; border-radius: 4px;").arg(v.dotColor));
    m_statusText->setStyleSheet(QStringLiteral(
        "color: %1; font-size: 8pt; font-weight: bold; letter-spacing: 2px; background-color: transparent").arg(v.textColor));
    m_statusText->setText(v.text);

    updateButtons();
    statusBar()->showMessage("  " + v.text.toLower());
}

void MainWindow::onStepStarted(int idx, const Core::StepDescriptor& desc)
{
    // Обновить левую панель
    if (idx < m_stepItems.size()) {
        if (idx > 0) m_stepItems[idx - 1]->setActive(false);
        m_stepItems[idx]->setActive(true);
    }

    // Обновить заголовок
    m_stepIndexLabel->setText(QStringLiteral("%1").arg(idx + 1, 2, 10, QChar('0')));
    m_stepTitleLabel->setText(desc.title);
    const QString typeStr = (desc.type == Core::StepType::Automatic)
        ? "автоматическая проверка"
        : "требуется действие оператора";
    m_stepTypeLabel->setText(typeStr);

    // Промпт для автошагов
    if (desc.type == Core::StepType::Automatic)
        setPrompt("ВЫПОЛНЯЕТСЯ", desc.description, "#00e5cc");
}

void MainWindow::onStepFinished(int idx, Core::StepResult result, const QString&)
{
    if (idx < m_stepItems.size())
        m_stepItems[idx]->setResult(result);
}

void MainWindow::onOperatorActionRequired(int, const Core::StepDescriptor& desc)
{
    setPrompt("ТРЕБУЕТСЯ ДЕЙСТВИЕ ОПЕРАТОРА", desc.description, "#ff9800");
}

void MainWindow::onSnapshotChanged(const Core::DeviceSnapshot& snap)
{
    if (!snap.isIdentified()) return;
    m_deviceLabel->setText(QStringLiteral(
        "%1     IP %2     FW %3     ID %4")
        .arg(snap.deviceName.isEmpty() ? "—" : snap.deviceName)
        .arg(snap.ipAddress.toString())
        .arg(snap.firmwareVersion)
        .arg(snap.deviceId)
    );
    m_deviceLabel->setStyleSheet(
        "color: #707070;"
        "font-size: 9pt;"
        "font-family: 'JetBrains Mono', 'Consolas', monospace;"
		"background-color: transparent;"
    );
}

void MainWindow::onLogEntryAdded(const Core::LogEntry& entry)
{
    static const QMap<Core::LogLevel, QString> colors {
        {Core::LogLevel::Debug,   "#333333"},
        {Core::LogLevel::Info,    "#505050"},
        {Core::LogLevel::Warning, "#7a5000"},
        {Core::LogLevel::Error,   "#7a2020"},
        {Core::LogLevel::Fatal,   "#9a1010"},
    };
    static const QMap<Core::LogLevel, QString> tagColors {
        {Core::LogLevel::Debug,   "#404040"},
        {Core::LogLevel::Info,    "#006655"},
        {Core::LogLevel::Warning, "#cc7700"},
        {Core::LogLevel::Error,   "#cc3333"},
        {Core::LogLevel::Fatal,   "#ff2222"},
    };

    const QString bg  = colors   .value(entry.level, "#333333");
    const QString tag = tagColors.value(entry.level, "#555555");

    static const QMap<Core::LogLevel, QString> tags {
        {Core::LogLevel::Debug,   "DBG"},
        {Core::LogLevel::Info,    "INF"},
        {Core::LogLevel::Warning, "WRN"},
        {Core::LogLevel::Error,   "ERR"},
        {Core::LogLevel::Fatal,   "FTL"},
    };

    const QString html = QStringLiteral(
        "<span style='color:%1'>"
        "%2 "
        "<span style='color:%3'>[%4]</span> "
        "<span style='color:#2a4a4a'>%5</span> "
        "%6"
        "</span>"
    )
    .arg(bg)
    .arg(entry.timestamp.toString("hh:mm:ss.zzz"))
    .arg(tag, tags.value(entry.level, "???"))
    .arg(entry.source.leftJustified(22).toHtmlEscaped())
    .arg(entry.message.toHtmlEscaped());

    m_logView->append(html);
}

void MainWindow::onScenarioFinished(bool overallPass)
{
    if (overallPass) {
        setPrompt("РЕЗУЛЬТАТ", "Сценарий проверки успешно завершён.\nИзделие соответствует требованиям.", "#4caf50");
        setStatusIndicator("ГОДЕН", "#4caf50");
    } else {
        setPrompt("РЕЗУЛЬТАТ", "Сценарий завершён с ошибками.\nИзделие не прошло проверку.", "#f44336");
        setStatusIndicator("НЕ ГОДЕН", "#f44336");
    }
}

// ── Вспомогательные ───────────────────────────────────────────────────────────

void MainWindow::updateButtons()
{
    using S = Core::DispatcherState;
    const auto s = m_dispatcher->state();

    m_startBtn  ->setEnabled(s == S::Idle);
    m_confirmBtn->setEnabled(s == S::WaitingOperator);
    m_abortBtn  ->setEnabled(s == S::Running || s == S::WaitingOperator || s == S::Paused);
}

void MainWindow::setStatusIndicator(const QString& text, const QString& color)
{
    m_statusDot ->setStyleSheet(QStringLiteral("background-color: %1; border-radius: 4px;").arg(color));
    m_statusText->setStyleSheet(QStringLiteral(
        "color: %1; font-size: 8pt; font-weight: bold; letter-spacing: 2px;").arg(color));
    m_statusText->setText(text);
}

void MainWindow::setPrompt(const QString& header, const QString& body,
                            const QString& accentColor)
{
    m_promptHeader->setText(header);
    m_promptHeader->setStyleSheet(QStringLiteral(
        "color: %1; font-size: 8pt; font-weight: bold; letter-spacing: 2px;")
        .arg(accentColor));

    // Находим QTextEdit по objectName (установлен в buildUi)
    if (auto* edit = findChild<QTextEdit*>("thePromptEdit"))
        edit->setPlainText(body);
}

void MainWindow::onDeviceSelectionRequired(const Msv::Network::WhoIAmResponseList& found)
{
    setPrompt("ВЫБОР УСТРОЙСТВА",
              "Сканирование завершено. Выберите устройство из списка.",
              "#00e5cc");

    DeviceSelectionDialog dlg(found, this);
    if (dlg.exec() != QDialog::Accepted) {
        m_dispatcher->abort();
        return;
    }

    if (auto* msv = qobject_cast<Core::MsvScenarioDispatcher*>(m_dispatcher)) {
        if (dlg.wasManualEntry())
            msv->provideManualIp(dlg.manualIp());
        else
            msv->selectDevice(dlg.selectedDevice());
    }
}

// Оставляем как заглушку на случай если понадобится напрямую
void MainWindow::onManualIpRequired() {}


} // namespace Msv::Ui
