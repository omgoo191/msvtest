#include "StepItemWidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>

namespace Msv::Ui {

StepItemWidget::StepItemWidget(int index,
                               const Core::StepDescriptor& desc,
                               QWidget* parent)
    : QFrame(parent)
    , m_index(index)
{
	setMinimumHeight(54);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(12, 0, 12, 0);
    layout->setSpacing(12);

    // ── Кружок с номером ──────────────────────────────────────────────────────
    m_circle = new QLabel(QString::number(index + 1), this);
    m_circle->setFixedSize(28, 28);
    m_circle->setAlignment(Qt::AlignCenter);

    // ── Текст ─────────────────────────────────────────────────────────────────
    auto* textCol = new QVBoxLayout;
    textCol->setSpacing(2);

    m_title = new QLabel(desc.title, this);
    m_title->setObjectName("stepTitle");
	m_title->setWordWrap(true);

    const QString typeStr = (desc.type == Core::StepType::Automatic)
        ? "автоматически"
        : "действие оператора";
    m_type = new QLabel(typeStr, this);
    m_type->setObjectName("stepType");

    textCol->addWidget(m_title);
    textCol->addWidget(m_type);

    // ── Бейдж результата ──────────────────────────────────────────────────────
    m_badge = new QLabel(this);
    m_badge->setObjectName("stepBadge");
    m_badge->setFixedWidth(56);
    m_badge->setAlignment(Qt::AlignCenter);
    m_badge->hide();

    layout->addWidget(m_circle);
    layout->addLayout(textCol, 1);
    layout->addWidget(m_badge);

    refresh();
}

void StepItemWidget::setActive(bool active)
{
    m_active = active;
    refresh();
}

void StepItemWidget::setResult(Core::StepResult result)
{
    m_result = result;
    m_active = false;
    refresh();
}

void StepItemWidget::refresh()
{
    // ── Рамка строки ──────────────────────────────────────────────────────────
    if (m_active) {
        setStyleSheet(
            "StepItemWidget {"
            "  background-color: #0d2137;"
            "  border-left: 3px solid #00e5cc;"
            "  border-top: none; border-right: none; border-bottom: none;"
            "}"
        );
    } else {
        setStyleSheet(
            "StepItemWidget {"
            "  background-color: transparent;"
            "  border-left: 3px solid transparent;"
            "  border-top: none; border-right: none; border-bottom: none;"
            "}"
        );
    }

    // ── Кружок ────────────────────────────────────────────────────────────────
    struct CircleStyle { QString bg; QString fg; QString text; };
    CircleStyle cs;

    switch (m_result) {
        case Core::StepResult::Pass:
            cs = {"#1a472a", "#4caf50", "✓"}; break;
        case Core::StepResult::Fail:
            cs = {"#4a1010", "#f44336", "✗"}; break;
        case Core::StepResult::Warning:
            cs = {"#4a3000", "#ff9800", "!"}; break;
        case Core::StepResult::Skipped:
            cs = {"#2a2a2a", "#606060", "–"}; break;
        default:
            if (m_active)
                cs = {"#003d4f", "#00e5cc", QString::number(m_index + 1)};
            else
                cs = {"#1e1e1e", "#505050", QString::number(m_index + 1)};
    }

    m_circle->setText(cs.text);
	m_circle->setStyleSheet(QStringLiteral(" font-size: %1pt;").arg(m_index >= 9 ? 7 : 9));
    m_circle->setStyleSheet(QStringLiteral(
        "QLabel {"
        "  background-color: %1;"
        "  color: %2;"
        "  border-radius: 14px;"
        "  font-weight: bold;"
        "  font-family: 'JetBrains Mono', 'Consolas', monospace;"
        "}"
    ).arg(cs.bg, cs.fg));

    // ── Текст строки ──────────────────────────────────────────────────────────
    const QString titleColor = m_active ? "#e8e8e8"
        : (m_result == Core::StepResult::NotRun ? "#707070" : "#c8c8c8");
    m_title->setStyleSheet(QStringLiteral(
        "QLabel { color: %1; font-size: 9pt; font-weight: %2; }")
        .arg(titleColor, m_active ? "bold" : "normal"));
    m_type->setStyleSheet("QLabel { color: #404040; font-size: 8pt; }");

    // ── Бейдж ─────────────────────────────────────────────────────────────────
    struct BadgeStyle { QString bg; QString fg; QString text; };
    static const QMap<Core::StepResult, BadgeStyle> badges {
        {Core::StepResult::Pass,    {"#1a3a1a", "#4caf50", "PASS"}},
        {Core::StepResult::Fail,    {"#3a1010", "#f44336", "FAIL"}},
        {Core::StepResult::Warning, {"#3a2800", "#ff9800", "WARN"}},
        {Core::StepResult::Skipped, {"#202020", "#606060", "SKIP"}},
    };

    if (badges.contains(m_result)) {
        const auto& b = badges[m_result];
        m_badge->setText(b.text);
        m_badge->setStyleSheet(QStringLiteral(
            "QLabel {"
            "  background-color: %1;"
            "  color: %2;"
            "  border-radius: 3px;"
            "  font-size: 8pt;"
            "  font-weight: bold;"
            "  font-family: 'JetBrains Mono', 'Consolas', monospace;"
            "  padding: 2px 6px;"
            "}"
        ).arg(b.bg, b.fg));
        m_badge->show();
    } else {
        m_badge->hide();
    }
}

void StepItemWidget::mousePressEvent(QMouseEvent *event)
{
	QFrame::mousePressEvent(event);
	emit clicked(m_index);
}

} // namespace Msv::Ui
