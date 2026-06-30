#include "LongRunSetupDialog.h"
#include "core/LongRunMonitor.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QProgressBar>
#include <QGroupBox>
#include <QStyle>

namespace Msv::Ui {

LongRunSetupDialog::LongRunSetupDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Длительный мониторинг");
    setMinimumSize(540, 440);
    setModal(false);  // не блокирует основное окно
    applyTheme();
    buildUi();
}

int LongRunSetupDialog::durationMinutes() const
{
    return m_durationSpin->value();
}

void LongRunSetupDialog::applyTheme()
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
        QLabel#timeLabel {
            color: #e8e8e8;
            font-size: 14pt;
            font-family: "JetBrains Mono", "Consolas", monospace;
            font-weight: bold;
        }
        QLabel#resultPass  { color: #4caf50; font-size: 13pt; font-weight: bold; }
        QLabel#resultWarn  { color: #ff9800; font-size: 13pt; font-weight: bold; }
        QLabel#resultFail  { color: #f44336; font-size: 13pt; font-weight: bold; }
        QSpinBox {
            background-color: #1c1c1c;
            border: 1px solid #2e2e2e;
            border-radius: 4px;
            padding: 4px 10px;
            color: #c0c0c0;
            min-width: 80px;
        }
        QProgressBar {
            background-color: #1a1a1a;
            border: none;
            border-radius: 3px;
            max-height: 6px;
        }
        QProgressBar::chunk { background-color: #00e5cc; border-radius: 3px; }
        QTextEdit {
            background-color: #0a0a0a;
            border: 1px solid #232323;
            border-radius: 4px;
            color: #808080;
            font-family: "JetBrains Mono", "Consolas", monospace;
            font-size: 9pt;
            padding: 8px;
        }
        QPushButton {
            border: none; border-radius: 4px;
            padding: 9px 22px; font-size: 10pt; font-weight: bold;
        }
        QPushButton#startBtn {
            background-color: #00897b; color: #ffffff; min-width: 120px;
        }
        QPushButton#startBtn:hover { background-color: #00a896; }
        QPushButton#stopBtn {
            background-color: transparent; color: #f44336;
            border: 1px solid #c62828; min-width: 120px;
        }
        QPushButton#stopBtn:hover { background-color: #1a0000; }
        QPushButton#closeBtn {
            background-color: #1e1e1e; color: #909090;
            border: 1px solid #2e2e2e;
        }
        QPushButton#closeBtn:hover { color: #c0c0c0; }
    )");
}

void LongRunSetupDialog::buildUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(16);

    // Заголовок
    auto* title = new QLabel("ДЛИТЕЛЬНЫЙ МОНИТОРИНГ", this);
    title->setObjectName("titleLabel");
    root->addWidget(title);

    // Настройки (фаза Setup)
    {
        auto* setupGroup = new QGroupBox("Параметры теста", this);
        setupGroup->setStyleSheet(
            "QGroupBox { color: #505050; font-size: 8pt; border: 1px solid #252525;"
            " border-radius: 4px; margin-top: 8px; padding-top: 8px; }"
            "QGroupBox::title { subcontrol-origin: margin; left: 8px; }");
        auto* fl = new QFormLayout(setupGroup);
        fl->setSpacing(10);

        m_durationSpin = new QSpinBox(this);
        m_durationSpin->setRange(1, 480);  // 1 мин — 8 часов
        m_durationSpin->setValue(60);
        m_durationSpin->setSuffix(" мин");

        fl->addRow("Длительность:", m_durationSpin);
        root->addWidget(setupGroup);
    }

    // Прогресс (скрыт на старте)
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(false);
    m_progressBar->hide();
    root->addWidget(m_progressBar);

    m_timeLabel = new QLabel("00:00 / 00:00", this);
    m_timeLabel->setObjectName("timeLabel");
    m_timeLabel->setAlignment(Qt::AlignCenter);
    m_timeLabel->hide();
    root->addWidget(m_timeLabel);

    // Живая статистика
    m_statsView = new QTextEdit(this);
    m_statsView->setReadOnly(true);
    m_statsView->setMinimumHeight(160);
    m_statsView->setPlainText("Нажмите «ЗАПУСТИТЬ» для начала теста.");
    root->addWidget(m_statsView, 1);

    // Результат
    m_resultLabel = new QLabel(this);
    m_resultLabel->setAlignment(Qt::AlignCenter);
    m_resultLabel->hide();
    root->addWidget(m_resultLabel);

    // Кнопки
    auto* btnRow = new QHBoxLayout;
    m_startBtn = new QPushButton("▶  ЗАПУСТИТЬ", this);
    m_startBtn->setObjectName("startBtn");
    m_startBtn->setFixedHeight(38);

    m_stopBtn = new QPushButton("■  ОСТАНОВИТЬ", this);
    m_stopBtn->setObjectName("stopBtn");
    m_stopBtn->setFixedHeight(38);
    m_stopBtn->hide();

    m_closeBtn = new QPushButton("Закрыть", this);
    m_closeBtn->setObjectName("closeBtn");
    m_closeBtn->setFixedHeight(38);

    btnRow->addWidget(m_startBtn);
    btnRow->addWidget(m_stopBtn);
    btnRow->addStretch();
    btnRow->addWidget(m_closeBtn);
    root->addLayout(btnRow);

    connect(m_startBtn,  &QPushButton::clicked, this, &LongRunSetupDialog::onStartClicked);
    connect(m_stopBtn,   &QPushButton::clicked, this, &LongRunSetupDialog::onStopClicked);
    connect(m_closeBtn,  &QPushButton::clicked, this, &QDialog::close);
}

void LongRunSetupDialog::onStartClicked()
{
    switchToRunning();
    emit startRequested(m_durationSpin->value());
}

void LongRunSetupDialog::onStopClicked()
{
    emit stopRequested();
    switchToFinished();
}

	void LongRunSetupDialog::switchToRunning()
	{
		m_phase = Phase::Running;
		m_progressBar->setValue(0);  // ← сброс прогресса
		m_progressBar->show();
		m_timeLabel->show();
		m_startBtn->hide();
		m_stopBtn->show();
		m_durationSpin->setEnabled(false);
		m_statsView->clear();
		m_resultLabel->hide();       // ← скрыть предыдущий результат
		m_resultLabel->setText("");  // ← очистить текст
	}

void LongRunSetupDialog::switchToFinished()
{
    m_phase = Phase::Finished;
    m_stopBtn->hide();
    m_startBtn->show();
    m_startBtn->setText("↺  ПОВТОРИТЬ");
    m_durationSpin->setEnabled(true);
}

void LongRunSetupDialog::updateProgress(int elapsedSec, int totalSec,
                                         const QString& stats)
{
	Q_UNUSED(elapsedSec)
	Q_UNUSED(totalSec)
    if (m_phase != Phase::Running) return;
    m_statsView->setPlainText(stats);
}

void LongRunSetupDialog::showResult(const Core::LongRunResult& result)
{
	m_progressBar->setValue(100);  // ← добавить первой строкой
    switchToFinished();

    // Определяем итог
    const bool hasFail =
        result.webMonotonErrors  > 2 ||
        result.sntpMonotonErrors > 2 ||
        result.gnssMonotonErrors > 5 ||
        (result.nmeaTotal > 0 &&
         static_cast<double>(result.nmeaOk) / result.nmeaTotal < 0.5);

    const bool hasWarn =
        result.webMonotonErrors  > 0 ||
        result.sntpMonotonErrors > 0 ||
        result.gnssMonotonErrors > 0 ||
        result.antennaChanges    > 0 ||
        result.syncStatusChanges > 2;

    QString verdict, styleId;
    if (hasFail) {
        verdict = "✖  НЕ ГОДЕН";  styleId = "resultFail";
    } else if (hasWarn) {
        verdict = "⚠  С ЗАМЕЧАНИЯМИ"; styleId = "resultWarn";
    } else {
        verdict = "✔  ГОДЕН"; styleId = "resultPass";
    }

    m_resultLabel->setText(verdict);
    m_resultLabel->setObjectName(styleId);
    m_resultLabel->style()->unpolish(m_resultLabel);
    m_resultLabel->style()->polish(m_resultLabel);
    m_resultLabel->show();

    // Итоговая статистика
    const double checkPct = result.nmeaTotal > 0
        ? 100.0 * result.nmeaOk / result.nmeaTotal : 0.0;

    const auto fmtOff = [](qint64 minMs, qint64 maxMs) -> QString {
        if (minMs == 0 && maxMs == 0) return "н/д";
        return QStringLiteral("[%1 ... %2] мс").arg(minMs).arg(maxMs);
    };


	const auto fmtStats = [](const Core::OffsetStats& st) -> QString {
		if (st.count == 0) return "нет данных";
		return QStringLiteral(
				"мин %1  макс %2  среднее %3  СКО %4  размах %5 мс (n=%6)")
				.arg(st.minMs)
				.arg(st.maxMs)
				.arg(QString::number(st.meanMs,  'f', 1))
				.arg(QString::number(st.stdDevMs, 'f', 1))
				.arg(st.rangeMs())
				.arg(st.count);
	};

	m_statsView->setPlainText(QStringLiteral(
									  "ИТОГ ДЛИТЕЛЬНОГО ТЕСТА (%1 мин, %2 замеров)\n"
									  "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n"
									  "ОШИБКИ МОНОТОННОСТИ\n"
									  "  Web:  %3      SNTP: %4      GNSS: %5\n\n"
									  "РАСХОЖДЕНИЕ С ПК (мс)\n"
									  "  Web:  %6\n"
									  "  SNTP: %7\n"
									  "  GNSS: %8\n\n"
									  "РАСХОЖДЕНИЕ Web vs GNSS (мс)\n"
									  "  %9\n\n"
									  "NMEA\n"
									  "  Checksum: %10/%11 (%12%)\n"
									  "  Монотонность ошибок: %13\n\n"
									  "СОБЫТИЯ\n"
									  "  Смен антенны: %14\n"
									  "  Смен статуса sync: %15\n\n"
									  "ПОЯСНЕНИЕ:\n"
									  "  Среднее — систематическое смещение (вкл. задержку сети).\n"
									  "  СКО — стабильность (чем меньше, тем стабильнее).\n"
									  "  Размах — разброс между мин и макс.")
									  .arg(result.durationMinutes)
									  .arg(result.samplesCollected)
									  .arg(result.webMonotonErrors)
									  .arg(result.sntpMonotonErrors)
									  .arg(result.gnssMonotonErrors)
									  .arg(fmtStats(result.webOffset))
									  .arg(fmtStats(result.sntpOffset))
									  .arg(fmtStats(result.gnssOffset))
									  .arg(fmtStats(result.webVsGnss))
									  .arg(result.nmeaOk).arg(result.nmeaTotal)
									  .arg(QString::number(checkPct, 'f', 1))
									  .arg(result.nmeaMonotonErrors)
									  .arg(result.antennaChanges)
									  .arg(result.syncStatusChanges));
}

void LongRunSetupDialog::updateStats(const QString &stats)
{
	if(m_phase != Phase::Running) return;
	m_statsView->setPlainText(stats);
}

void LongRunSetupDialog::updateTimer(int elapsedSec, int totalSec)
{
	qDebug() << "UpdateTImer" << elapsedSec <<"phase" << (int)m_phase;
	if (m_phase != Phase::Running) return;
	const int pct = totalSec > 0 ? (elapsedSec * 100 / totalSec) : 0;
	m_progressBar->setValue(pct);

	const auto fmt = [](int sec)->QString{
		return QStringLiteral("%1:%2")
		.arg(sec /60, 2, 10, QChar('0'))
		.arg(sec %60, 2, 10, QChar('0'));
	};
	m_timeLabel->setText(QStringLiteral("%1 / %2")
		.arg(fmt(elapsedSec) , fmt(totalSec)));
}

} // namespace Msv::Ui
