#pragma once

#include "core/IScenarioDispatcher.h"
#include <QFrame>
#include <QDateTime>

QT_BEGIN_NAMESPACE
class QLabel;
class QPushButton;
class QWidget;
QT_END_NAMESPACE

namespace Msv::Ui {

// ─────────────────────────────────────────────────────────────────────────────
/// Карточка одного шага в сводке.
/// Показывает: иконка + название + время + ключевые параметры.
/// Кнопка "В журнале" фильтрует лог по временному диапазону шага.
// ─────────────────────────────────────────────────────────────────────────────
class StepSummaryCard : public QFrame {
    Q_OBJECT
public:
    explicit StepSummaryCard(int index, const QString& title, QWidget* parent = nullptr);

    void setResult  (Core::StepResult result, const QString& details);
    void setRunning (const QString& liveDetails);
    void setStartTime(const QDateTime& startedAt);
    void setFinishTime(const QDateTime& finishedAt);

    /// Обновить ключевые параметры (человекочитаемо)
    void setKeyParams(const QString& params);

    [[nodiscard]] QDateTime startedAt()  const { return m_startedAt;  }
    [[nodiscard]] QDateTime finishedAt() const { return m_finishedAt; }

signals:
    /// Оператор хочет увидеть журнал этого шага
    void showInLogRequested(QDateTime from, QDateTime to);

private slots:
    void onToggleClicked();

private:
    void buildUi();
    void applyTheme();
    void updateHeader();
    void setExpanded(bool expanded);

    int              m_index;
    QString          m_title;
    Core::StepResult m_result   {Core::StepResult::NotRun};
    QDateTime        m_startedAt;
    QDateTime        m_finishedAt;
    bool             m_expanded {true};

    // UI
    QLabel*      m_iconLabel    {nullptr};
    QLabel*      m_titleLabel   {nullptr};
    QLabel*      m_timeLabel    {nullptr};
    QLabel*      m_resultLabel  {nullptr};
    QPushButton* m_toggleBtn    {nullptr};
    QPushButton* m_logBtn       {nullptr};
    QWidget*     m_body         {nullptr};
    QLabel*      m_detailsLabel {nullptr};
    QLabel*      m_paramsLabel  {nullptr};
    QLabel*      m_liveLabel    {nullptr};
};

} // namespace Msv::Ui
