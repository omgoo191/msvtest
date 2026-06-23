#include "MainWindow.h"
#include "StepItemWidget.h"
#include "DeviceSelectionDialog.h"
#include "core/IScenarioDispatcher.h"
#include "core/MsvScenarioDispatcher.h"
#include "core/IDeviceModel.h"
#include "core/Logger.h"
#include "network/WhoIAmTypes.h"
#include "ui/SerialPortSelectionDialog.h"
#include "ui/LongRunSetupDialog.h"
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
#include <QScrollBar>
#include <QTimer>

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

	for (auto* item : m_stepItems)
		connect(item, &StepItemWidget::clicked,
				this, &MainWindow::onStepItemClicked);

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

		QPushButton#longRunBtn {
			background-color: transparent;
			color: #505050;
			border: 1px solid #2e2e2e;
			font-size: 14pt;
		}
		QPushButton#longRunBtn:hover { color: #00e5cc; border-color: #00e5cc; }

		QPushButton#restartFromBtn {
			background-color: transparent;
			color: #505050;
			border: 1px solid #2e2e2e;
		}
		QPushButton#restartFromBtn:hover { color: #ff9800; border-color: #ff9800; }
		QPushButton#restartFromBtn:pressed { background-color: #1a1000; }
		QPushButton#restartFromBtn:disabled {
			color: #2a2a2a; border-color: #222222;
		}

		QPushButton#devModeBtn {
			background-color: transparent;
			color: #303030;
			border: 1px solid #282828;
			font-size: 8pt;
			letter-spacing: 1px;
		}
		QPushButton#devModeBtn:hover   { color: #606060; border-color: #404040; }
		QPushButton#devModeBtn:checked {
			background-color: #1a0d00;
			color: #ff6600;
			border: 1px solid #ff6600;
		}

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

        QPushButton#continueBtn {
            background-color: #1a3a5c;
            color: #00e5cc;
            border: 1px solid #00e5cc;
			min-width: 130px;
        }
	    QPushButton#continueBtn:disabled {
	  		background-color: #1a1a1a;
            color: #2a2a2a;
            border: 1px solid #222222;
        }

		QPushButton#continueBtn:pressed {
			background-color: #0d2040;
			border-color: #00b8a0;
			color: #00b8a0;
		}
		QPushButton#continueBtn:hover {
			background-color: #1e4570;
			border-color: #00ffd8;
			color: #00ffd8;
		}

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
		m_rightTabs = new QTabWidget(hSplit);
		m_rightTabs->setObjectName("rightTabs");

		auto* mainWidget = new QWidget(m_rightTabs);
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

			m_stepTypeLabel = new QLabel("", stepHeader);
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

			auto* promptHeaderRow = new QHBoxLayout;
			promptHeaderRow->setContentsMargins(0, 0, 0, 0);

			m_promptHeader = new QLabel("ИНСТРУКЦИЯ", card);
			m_promptHeader->setObjectName("promptHeader");
			promptHeaderRow->addWidget(m_promptHeader, 1);

			m_antennaIndicator = new QLabel(card);
			m_antennaIndicator->setText("АНТЕННА: —");
			m_antennaIndicator->setStyleSheet(
					"color: #404040; font-size: 8pt; font-weight: bold; letter-spacing: 1px;");
			promptHeaderRow->addWidget(m_antennaIndicator);
			cl->addLayout(promptHeaderRow);

			auto* promptEdit = new QTextEdit(card);
			promptEdit->setObjectName("thePromptEdit");
			promptEdit->setReadOnly(true);
			promptEdit->setFrameShape(QFrame::NoFrame);
			promptEdit->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
			promptEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
			promptEdit->setStyleSheet(
					"QTextEdit#thePromptEdit {"
					"  background: transparent; color: #c0c0c0;"
					"  font-size: 11pt; border: none; padding: 0px;"
					"}"
			);
			promptEdit->setPlainText("Нажмите «ЗАПУСТИТЬ» для начала сценария проверки.");
			m_promptBody = nullptr;
			cl->addWidget(promptEdit, 1);
			ml->addWidget(card, 1);
		}

		// Кнопки
		{
			auto* btnRow = new QHBoxLayout;
			btnRow->setSpacing(10);

			m_startBtn       = new QPushButton("▶   ЗАПУСТИТЬ", mainWidget);
			m_confirmBtn     = new QPushButton("✔   ВЫПОЛНЕНО", mainWidget);
			m_abortBtn       = new QPushButton("ПРЕРВАТЬ",      mainWidget);
			m_continueBtn    = new QPushButton("ДАЛЕЕ",         mainWidget);
			m_restartFromBtn = new QPushButton("↺  ОТСЮДА",    mainWidget);
			m_devModeBtn     = new QPushButton("DEV",           mainWidget);

			m_startBtn      ->setObjectName("startBtn");
			m_confirmBtn    ->setObjectName("confirmBtn");
			m_abortBtn      ->setObjectName("abortBtn");
			m_continueBtn   ->setObjectName("continueBtn");
			m_restartFromBtn->setObjectName("restartFromBtn");
			m_devModeBtn    ->setObjectName("devModeBtn");

			for (auto* btn : {m_startBtn, m_confirmBtn, m_abortBtn,
							  m_continueBtn, m_restartFromBtn})
				btn->setFixedHeight(40);

			m_devModeBtn->setFixedSize(56, 40);
			m_devModeBtn->setCheckable(true);
			m_restartFromBtn->setEnabled(false);

			m_longRunBtn = new QPushButton("⏱", mainWidget);
			m_longRunBtn->setObjectName("longRunBtn");
			m_longRunBtn->setFixedSize(40, 40);
			m_longRunBtn->setToolTip("Длительный мониторинг");

			btnRow->addWidget(m_longRunBtn);
			btnRow->addWidget(m_restartFromBtn);
			btnRow->addWidget(m_continueBtn);
			btnRow->addWidget(m_startBtn);
			btnRow->addWidget(m_confirmBtn);
			btnRow->addStretch(1);
			btnRow->addWidget(m_abortBtn);
			btnRow->addWidget(m_devModeBtn);
			ml->addLayout(btnRow);
		}

		// Вкладка сводки
		auto* summaryWidget = new QWidget(m_rightTabs);
		auto* summaryLayout = new QVBoxLayout(summaryWidget);
		summaryLayout->setContentsMargins(0, 0, 0, 0);
		summaryLayout->setSpacing(0);

		auto* summaryScroll = new QScrollArea(summaryWidget);
		summaryScroll->setWidgetResizable(true);
		summaryScroll->setFrameShape(QFrame::NoFrame);
		summaryScroll->setStyleSheet("QScrollArea { background: #0e0e0e; border: none; }");

		m_summaryContainer = new QWidget(summaryScroll);
		m_summaryContainer->setStyleSheet("background: #0e0e0e;");
		auto* containerLayout = new QVBoxLayout(m_summaryContainer);
		containerLayout->setContentsMargins(0, 0, 0, 0);
		containerLayout->setSpacing(0);
		containerLayout->addStretch(1);

		summaryScroll->setWidget(m_summaryContainer);
		summaryLayout->addWidget(summaryScroll);

		m_rightTabs->addTab(mainWidget,    "  ИНСТРУКЦИЯ  ");
		m_rightTabs->addTab(summaryWidget, "  СВОДКА  ");

		hSplit->addWidget(m_rightTabs);
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
	connect(m_dispatcher, &Core::IScenarioDispatcher::stepProgressUpdate,
			this, &MainWindow::onStepProgressUpdate);
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

	connect(m_continueBtn, &QPushButton::clicked,
			this, &MainWindow::onContinueClicked);

	if(auto* msv = qobject_cast<Core::MsvScenarioDispatcher*>(m_dispatcher)) {
		connect(msv, &Core::MsvScenarioDispatcher::portSelectionRequired,
				this, &MainWindow::onPortSelectionRequired);
	}

    // MsvScenarioDispatcher-специфичный сигнал — подключаем через qobject_cast
    if (auto* msv = qobject_cast<Core::MsvScenarioDispatcher*>(m_dispatcher)) {
        connect(msv, &Core::MsvScenarioDispatcher::deviceSelectionRequired,
                this, &MainWindow::onDeviceSelectionRequired);

    }

	connect(m_restartFromBtn, &QPushButton::clicked, this, [this]() {
		if (m_selectedStepIndex < 0) return;
		for (int i = m_selectedStepIndex; i < m_stepItems.size(); ++i)
			m_stepItems[i]->setResult(Core::StepResult::NotRun);
		m_stepRecords = m_stepRecords.mid(0, m_selectedStepIndex);
		m_viewingStepIndex = -1;
		m_dispatcher->restartFromStep(m_selectedStepIndex);
		m_restartFromBtn->setEnabled(false);
		m_selectedStepIndex = -1;
	});

	connect(m_devModeBtn, &QPushButton::toggled, this, [this](bool checked) {
		m_devMode = checked;
		qDebug() << (checked ? "DEV MODE включён" : "DEV MODE выключен");
	});

	connect(m_longRunBtn, &QPushButton::clicked, this, &MainWindow::onLongRunClicked);

	if (auto* msv = qobject_cast<Core::MsvScenarioDispatcher*>(m_dispatcher)) {
		connect(msv, &Core::MsvScenarioDispatcher::longRunProgressUpdate,
				this, [this](int el, int tot, const QString& stats) {
					if (m_longRunDialog) m_longRunDialog->updateProgress(el, tot, stats);
				});
		connect(msv, &Core::MsvScenarioDispatcher::longRunFinished,
				this, [this](const Core::LongRunResult& result) {
					if (m_longRunDialog) m_longRunDialog->showResult(result);
				});
	}
}

// ── Слоты ─────────────────────────────────────────────────────────────────────

void MainWindow::onStartClicked()
{
	using S = Core::DispatcherState;
	const auto s = m_dispatcher->state();
	if (s == S::Finished || s == S::Aborted) {
		// Сброс UI
		m_stepRecords.clear();
		m_viewingStepIndex = -1;
		for (auto* item : m_stepItems)
			item->setResult(Core::StepResult::NotRun);
		m_dispatcher->reset();
	}
	m_dispatcher->start();
}
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
		{S::ReviewingResult, {"Результат",   "#00e5cc", "#00e5cc"}},
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
	{
		setPrompt("ВЫПОЛНЯЕТСЯ", desc.description, "#00e5cc");
	}
	StepDisplayRecord rec;
	rec.promptHeader = (desc.type == Core::StepType::Automatic)
					   ? "ВЫПОЛНЯЕТСЯ" : "ТРЕБУЕТСЯ ДЕЙСТВИЕ ОПЕРАТОРА";
	rec.promptBody   = desc.description;
	rec.accentColor  = (desc.type == Core::StepType::Automatic)
					   ? "#00e5cc" : "#ff9800";

	if (idx < m_stepRecords.size())
		m_stepRecords[idx] = rec;
	else
		m_stepRecords.append(rec);

	// Если смотрим live — обновить отображение
	if (m_viewingStepIndex == -1)
		setPrompt(rec.promptHeader, rec.promptBody, rec.accentColor);

	// Создать запись сводки для нового шага
	StepSummaryRecord sumRec;
	sumRec.index  = idx;
	sumRec.title  = desc.title;
	sumRec.result = Core::StepResult::NotRun;
	sumRec.startedAt = QDateTime::currentDateTimeUtc();
	if (idx < m_summaryRecords.size())
		m_summaryRecords[idx] = sumRec;
	else
		m_summaryRecords.append(sumRec);
	rebuildSummary();
}

void MainWindow::onStepFinished(int idx, Core::StepResult result, const QString& details)
{
    if (idx < m_stepItems.size())
	{
		m_stepItems[idx]->setResult(result);
	}
	// Обновить сохранённую запись — добавить детали результата
	if (idx < m_stepRecords.size() && !details.isEmpty()) {
		m_stepRecords[idx].promptBody +=
				QStringLiteral("\n\n▸ %1").arg(details);
	}

	if (idx < m_summaryRecords.size()) {
		m_summaryRecords[idx].result  = result;
		m_summaryRecords[idx].details = details;
		m_summaryRecords[idx].finishedAt = QDateTime::currentDateTimeUtc();
		m_summaryRecords[idx].liveDetails.clear();
	}
	rebuildSummary();
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

	if (!snap.antennaStatus.isEmpty()) {
		struct AntennaStyle { QString color; QString prefix; };
		AntennaStyle style;

		const QString s = snap.antennaStatus.toLower();
		if (s.contains("короткое") || s.contains("замыкание")) {
			style = {"#f44336", "⚡ КЗ: "};
		} else if (s.contains("не подключена") || s.contains("отключена")) {
			style = {"#ff9800", "○ АНТЕННА: "};
		} else if (s.contains("подключена")) {
			style = {"#4caf50", "● АНТЕННА: "};
		} else {
			style = {"#606060", "● АНТЕННА: "};
		}

		m_antennaIndicator->setText(style.prefix + snap.antennaStatus);
		m_antennaIndicator->setStyleSheet(QStringLiteral(
												  "color: %1; font-size: 8pt; font-weight: bold; letter-spacing: 1px;")
												  .arg(style.color));
	}
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

	const bool idle      = (s == S::Idle);
	const bool reviewing = (s == S::ReviewingResult);
	const bool waiting   = (s == S::WaitingOperator);
	const bool active    = (s == S::Running || reviewing || waiting);
	const bool done      = (s == S::Finished || s == S::Aborted);

	m_startBtn  ->setText(done ? "↺  ПЕРЕЗАПУСТИТЬ" : "▶   ЗАПУСТИТЬ");
	m_startBtn  ->setEnabled(idle || done);
	m_continueBtn->setEnabled(reviewing);
	m_confirmBtn->setEnabled(waiting);
	m_abortBtn  ->setEnabled(active);
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

void MainWindow::onContinueClicked()
{
	m_viewingStepIndex = -1;  // вернуться к live-виду
	m_dispatcher->continueToNextStep();
}

void MainWindow::onStepItemClicked(int index)
{
	using S = Core::DispatcherState;
	m_selectedStepIndex = index;

	// В dev-режиме — сразу запускаем шаг
	if (m_devMode) {
		// Сбрасываем UI для этого и последующих шагов
		for (int i = index; i < m_stepItems.size(); ++i)
			m_stepItems[i]->setResult(Core::StepResult::NotRun);
		m_stepRecords = m_stepRecords.mid(0, index);
		m_viewingStepIndex = -1;
		m_selectedStepIndex = -1;
		m_restartFromBtn->setEnabled(false);

		// Если сценарий не запущен — запустить и сразу прыгнуть
		using S = Core::DispatcherState;
		const auto s = m_dispatcher->state();
		if (s == S::Idle) {
			m_dispatcher->start();
			// start() пойдёт с шага 0 — прыгаем на нужный
		}
		m_dispatcher->restartFromStep(index);
		return;
	}

	// Обычный режим — просмотр
	if (index < m_stepRecords.size()) {
		m_viewingStepIndex = index;
		const auto& rec = m_stepRecords[index];
		setPrompt(rec.promptHeader, rec.promptBody, rec.accentColor);
	}

	for (int i = 0; i < m_stepItems.size(); ++i)
		m_stepItems[i]->setActive(i == index &&
								  m_dispatcher->currentStepIndex() == index);

	const auto result = m_dispatcher->stepResults().value(
			index, Core::StepResult::NotRun);
	const bool canRestart =
			result != Core::StepResult::NotRun &&
			m_dispatcher->state() != S::Idle;
	m_restartFromBtn->setEnabled(canRestart);
}

void MainWindow::onPortSelectionRequired()
{
	setPrompt("ВЫБОР COM-ПОРТА",
			  "Выберите порт к которому подключён UART-дубликат GNSS.",
			  "#00e5cc");

	SerialPortSelectionDialog dlg(this);
	if (dlg.exec() != QDialog::Accepted) {
		m_dispatcher->abort();
		return;
	}

	if (auto* msv = qobject_cast<Core::MsvScenarioDispatcher*>(m_dispatcher))
		msv->selectPort(dlg.selectedPort(), dlg.selectedBaud());
}

// Оставляем как заглушку на случай если понадобится напрямую
void MainWindow::onManualIpRequired() {}

void MainWindow::onStepProgressUpdate(int stepIndex, const QString& details)
{
	if (stepIndex < m_summaryRecords.size())
		m_summaryRecords[stepIndex].liveDetails = details;
	else {
		StepSummaryRecord rec;
		rec.index       = stepIndex;
		rec.title       = m_dispatcher->allSteps().value(stepIndex).title;
		rec.liveDetails = details;
		m_summaryRecords.append(rec);
	}
	rebuildSummary();
	// Переключить на сводку во время выполнения
	if(m_viewingStepIndex == -1)
	{
		m_rightTabs->setCurrentIndex(1);
	}
}

void MainWindow::rebuildSummary()
{
	// Создаём карточки по мере появления записей
	while (m_summaryCards.size() < m_summaryRecords.size()) {
		const auto& rec = m_summaryRecords[m_summaryCards.size()];
		auto* card = new StepSummaryCard(rec.index, rec.title, m_summaryContainer);

		connect(card, &StepSummaryCard::showInLogRequested,
				this, [this](QDateTime from, QDateTime to) {
					// Фильтрация журнала — переключаем на нижнюю панель
					// и фильтруем по времени
					filterLogByTime(from, to);
				});

		// Вставить перед stretch
		auto* layout = qobject_cast<QVBoxLayout*>(m_summaryContainer->layout());
		layout->insertWidget(layout->count() - 1, card);
		m_summaryCards.append(card);
	}

	// Обновляем существующие карточки
	for (int i = 0; i < m_summaryRecords.size(); ++i) {
		const auto& rec  = m_summaryRecords[i];
		auto*       card = m_summaryCards[i];

		card->setStartTime(rec.startedAt);

		if (!rec.finishedAt.isNull())
			card->setFinishTime(rec.finishedAt);

		if (rec.result != Core::StepResult::NotRun)
			card->setResult(rec.result, rec.details);
		else if (!rec.liveDetails.isEmpty())
			card->setRunning(rec.liveDetails);
	}
}
void MainWindow::onLongRunClicked()
{
	if (!m_longRunDialog) {
		m_longRunDialog = new LongRunSetupDialog(this);

		auto* msv = qobject_cast<Core::MsvScenarioDispatcher*>(m_dispatcher);

		connect(m_longRunDialog, &LongRunSetupDialog::startRequested,
				this, [this, msv](int minutes) {
					SerialPortSelectionDialog portDlg(this);
					if (portDlg.exec() != QDialog::Accepted) return;

					msv->setLongRunActive(true);
					msv->selectPort(portDlg.selectedPort(), portDlg.selectedBaud());
					QTimer::singleShot(200, this, [msv, minutes]() {
						msv->startLongRun(minutes);
					});
				});

		connect(m_longRunDialog, &LongRunSetupDialog::stopRequested,
				this, [msv]() { msv->stopLongRun(); });

		if (msv) {
			connect(msv, &Core::MsvScenarioDispatcher::longRunProgressUpdate,
					this, [this](int el, int tot, const QString& stats) {
						if (m_longRunDialog) m_longRunDialog->updateProgress(el, tot, stats);
					});
			connect(msv, &Core::MsvScenarioDispatcher::longRunDisplayUpdate,
						   this, [this](const QString& stats) {
				if (m_longRunDialog) m_longRunDialog->updateStats(stats);
			});
			connect(msv, &Core::MsvScenarioDispatcher::longRunFinished,
					this, [this](const Core::LongRunResult& result) {
						if (m_longRunDialog) m_longRunDialog->showResult(result);
					});
		}
	}

	m_longRunDialog->show();
	m_longRunDialog->raise();
}

void MainWindow::filterLogByTime(const QDateTime& from, const QDateTime& to)
{
	Q_UNUSED(from)
	Q_UNUSED(to)
	// TODO:  фильтрация журнала по времени
}

} // namespace Msv::Ui
