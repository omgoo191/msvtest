#pragma once

#include "core/IScenarioDispatcher.h"
#include <QFrame>

QT_BEGIN_NAMESPACE
class QLabel;
QT_END_NAMESPACE

namespace Msv::Ui {

// ─────────────────────────────────────────────────────────────────────────────
/// Одна строка в панели шагов слева.
/// Визуально: [●цифра] [Название / тип шага] [PASS/FAIL бейдж]
// ─────────────────────────────────────────────────────────────────────────────
class StepItemWidget : public QFrame {
    Q_OBJECT
public:
    explicit StepItemWidget(int index,
                            const Core::StepDescriptor& desc,
                            QWidget* parent = nullptr);

    void setActive(bool active);
    void setResult(Core::StepResult result);

protected:
	void mousePressEvent(QMouseEvent* event) override;
	void enterEvent(QEnterEvent* event) override;
	void leaveEvent(QEvent* event) override;

private:
    void refresh();

    QLabel* m_circle {nullptr};
    QLabel* m_title  {nullptr};
    QLabel* m_type   {nullptr};
    QLabel* m_badge  {nullptr};

    int              m_index;
    bool             m_active {false};
    Core::StepResult m_result {Core::StepResult::NotRun};

signals:
	void clicked(int index);
};

} // namespace Msv::Ui
