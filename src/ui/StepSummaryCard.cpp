#include "StepSummaryCard.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QWidget>

namespace Msv::Ui {

StepSummaryCard::StepSummaryCard(int index, const QString& title, QWidget* parent)
    : QFrame(parent)
    , m_index(index)
    , m_title(title)
{
    applyTheme();
    buildUi();
}

void StepSummaryCard::applyTheme()
{
    setStyleSheet(R"(
        StepSummaryCard {
            background-color: #0e0e0e;
            border: none;
            border-bottom: 1px solid #1a1a1a;
        }
        QLabel#iconLabel {
            font-size: 13pt;
        }
        QLabel#titleLabel {
            color: #c8c8c8;
            font-size: 10pt;
            font-weight: bold;
        }
        QLabel#timeLabel {
            color: #404040;
            font-size: 8pt;
            font-family: "JetBrains Mono", "Consolas", monospace;
        }
        QLabel#resultLabel {
            font-size: 9pt;
        }
        QLabel#detailsLabel {
            color: #707070;
            font-size: 9pt;
            padding-left: 4px;
        }
        QLabel#paramsLabel {
            color: #909090;
            font-size: 9pt;
            padding-left: 4px;
        }
        QLabel#liveLabel {
            color: #00e5cc;
            font-size: 9pt;
            font-family: "JetBrains Mono", "Consolas", monospace;
            padding: 6px;
            background-color: #0a1a1a;
            border-left: 2px solid #00e5cc;
        }
        QPushButton#toggleBtn {
            background: transparent;
            border: none;
            color: #404040;
            font-size: 10pt;
            padding: 0px;
        }
        QPushButton#toggleBtn:hover { color: #808080; }
        QPushButton#logBtn {
            background: transparent;
            border: 1px solid #252525;
            border-radius: 3px;
            color: #404040;
            font-size: 8pt;
            padding: 2px 8px;
        }
        QPushButton#logBtn:hover { color: #909090; border-color: #404040; }
    )");
}

void StepSummaryCard::buildUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // ── Заголовок карточки ────────────────────────────────────────────────────
    auto* header = new QWidget(this);
    header->setStyleSheet("background: transparent;");
    auto* hl = new QHBoxLayout(header);
    hl->setContentsMargins(16, 10, 12, 10);
    hl->setSpacing(10);

    m_iconLabel = new QLabel("●", header);
    m_iconLabel->setObjectName("iconLabel");
    m_iconLabel->setFixedWidth(20);
    m_iconLabel->setStyleSheet("color: #303030; font-size: 13pt;");

    auto* titleCol = new QVBoxLayout;
    titleCol->setSpacing(2);

    m_titleLabel = new QLabel(
        QStringLiteral("%1. %2").arg(m_index + 1).arg(m_title), header);
    m_titleLabel->setObjectName("titleLabel");

    m_timeLabel = new QLabel("—", header);
    m_timeLabel->setObjectName("timeLabel");

    titleCol->addWidget(m_titleLabel);
    titleCol->addWidget(m_timeLabel);

    m_resultLabel = new QLabel("", header);
    m_resultLabel->setObjectName("resultLabel");

    m_logBtn = new QPushButton("в журнале", header);
    m_logBtn->setObjectName("logBtn");
    m_logBtn->setFixedHeight(24);
    m_logBtn->hide();

    m_toggleBtn = new QPushButton("▼", header);
    m_toggleBtn->setObjectName("toggleBtn");
    m_toggleBtn->setFixedSize(24, 24);

    hl->addWidget(m_iconLabel);
    hl->addLayout(titleCol, 1);
    hl->addWidget(m_resultLabel);
    hl->addWidget(m_logBtn);
    hl->addWidget(m_toggleBtn);
    root->addWidget(header);

    // ── Тело карточки (сворачиваемое) ────────────────────────────────────────
    m_body = new QWidget(this);
    m_body->setStyleSheet("background: transparent;");
    auto* bl = new QVBoxLayout(m_body);
    bl->setContentsMargins(46, 0, 16, 12);
    bl->setSpacing(6);

    m_paramsLabel = new QLabel(m_body);
    m_paramsLabel->setObjectName("paramsLabel");
    m_paramsLabel->setWordWrap(true);
    m_paramsLabel->hide();

    m_detailsLabel = new QLabel(m_body);
    m_detailsLabel->setObjectName("detailsLabel");
    m_detailsLabel->setWordWrap(true);
    m_detailsLabel->hide();

    m_liveLabel = new QLabel(m_body);
    m_liveLabel->setObjectName("liveLabel");
    m_liveLabel->setWordWrap(true);
    m_liveLabel->hide();

    bl->addWidget(m_paramsLabel);
    bl->addWidget(m_detailsLabel);
    bl->addWidget(m_liveLabel);
    root->addWidget(m_body);

    connect(m_toggleBtn, &QPushButton::clicked,
            this, &StepSummaryCard::onToggleClicked);
	connect(m_logBtn, &QPushButton::clicked, this, [this]() {
		emit showInLogRequested(m_index);
	});
}

// ─────────────────────────────────────────────────────────────────────────────

void StepSummaryCard::setResult(Core::StepResult result, const QString& details)
{
    m_result    = result;
    m_liveLabel->hide();

    struct Style { QString icon; QString color; QString badge; };
    static const QMap<Core::StepResult, Style> styles {
        {Core::StepResult::Pass,    {"✔", "#4caf50", "PASS"}},
        {Core::StepResult::Fail,    {"✖", "#f44336", "FAIL"}},
        {Core::StepResult::Warning, {"⚠", "#ff9800", "WARN"}},
        {Core::StepResult::Skipped, {"–", "#606060", "SKIP"}},
    };

    const auto& s = styles.value(result, {"●", "#303030", ""});

    m_iconLabel->setText(s.icon);
    m_iconLabel->setStyleSheet(QStringLiteral(
        "color: %1; font-size: 13pt;").arg(s.color));

    if (!s.badge.isEmpty()) {
        m_resultLabel->setText(s.badge);
        m_resultLabel->setStyleSheet(QStringLiteral(
            "color: %1; font-size: 8pt; font-weight: bold;"
            " font-family: 'JetBrains Mono', 'Consolas', monospace;").arg(s.color));
    }

    if (!details.isEmpty()) {
        m_detailsLabel->setText(details);
        m_detailsLabel->show();
    }

    // Показать кнопку журнала только если есть временные метки
    if (m_startedAt.isValid())
        m_logBtn->show();
}

void StepSummaryCard::setRunning(const QString& liveDetails)
{
    m_iconLabel->setText("●");
    m_iconLabel->setStyleSheet("color: #00e5cc; font-size: 13pt;");
    m_resultLabel->setText("ВЫПОЛНЯЕТСЯ");
    m_resultLabel->setStyleSheet(
        "color: #00e5cc; font-size: 8pt; font-weight: bold; letter-spacing: 1px;");

    if (!liveDetails.isEmpty()) {
        m_liveLabel->setText(liveDetails);
        m_liveLabel->show();
    }
}

void StepSummaryCard::setStartTime(const QDateTime& startedAt)
{
    m_startedAt = startedAt;
    updateHeader();
}

void StepSummaryCard::setFinishTime(const QDateTime& finishedAt)
{
    m_finishedAt = finishedAt;
    updateHeader();
    if (m_startedAt.isValid())
        m_logBtn->show();
}

void StepSummaryCard::setKeyParams(const QString& params)
{
    if (params.isEmpty()) {
        m_paramsLabel->hide();
        return;
    }
    m_paramsLabel->setText(params);
    m_paramsLabel->show();
}

void StepSummaryCard::updateHeader()
{
    if (!m_startedAt.isValid()) return;

    QString timeText = m_startedAt.toLocalTime().toString("HH:mm:ss");
    if (m_finishedAt.isValid()) {
        const int secs = static_cast<int>(m_startedAt.secsTo(m_finishedAt));
        timeText += QStringLiteral("  (%1 с)").arg(secs);
    }
    m_timeLabel->setText(timeText);
}

void StepSummaryCard::onToggleClicked()
{
    m_expanded = !m_expanded;
    setExpanded(m_expanded);
}

void StepSummaryCard::setExpanded(bool expanded)
{
    m_expanded = expanded;
    m_body->setVisible(expanded);
    m_toggleBtn->setText(expanded ? "▼" : "▶");
}

} // namespace Msv::Ui
