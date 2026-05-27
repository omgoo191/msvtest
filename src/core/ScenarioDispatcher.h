#pragma once

#include "IScenarioDispatcher.h"
#include "ILogger.h"
#include <QTimer>
#include <memory>

namespace Msv::Core {

// ─────────────────────────────────────────────────────────────────────────────
/// Конкретный диспетчер сценариев.
///
/// Шаги сценария задаются один раз через конструктор (или фабричный метод).
/// Каждый Automatic-шаг делегирует реальную работу через виртуальный
/// метод executeStep() — потомки переопределяют его под конкретные проверки.
///
/// Для Шага 1 все executeStep() возвращают Pass-заглушку.
/// Реальные проверки подключаются в Шагах 2–5.
// ─────────────────────────────────────────────────────────────────────────────
class ScenarioDispatcher : public IScenarioDispatcher {
    Q_OBJECT
public:
    explicit ScenarioDispatcher(QList<StepDescriptor> steps,
                                std::shared_ptr<ILogger> logger,
                                QObject* parent = nullptr);
    ~ScenarioDispatcher() override = default;

    // IScenarioDispatcher interface
    void start()                                       override;
    void abort()                                       override;
    void pause()                                       override;
    void resume()                                      override;
    void confirmOperatorAction()                       override;
    void reportOperatorFailure(const QString& reason)  override;

    [[nodiscard]] DispatcherState       state()            const override;
    [[nodiscard]] int                   currentStepIndex() const override;
    [[nodiscard]] QList<StepDescriptor> allSteps()         const override;
    [[nodiscard]] QList<StepResult>     stepResults()      const override;

protected:
    /// Переопределяется в подклассах для реальных проверок.
    /// Вызывается только для StepType::Automatic.
    /// Должен завершаться вызовом finishCurrentStep().
    virtual void executeStep(int stepIndex);

    /// Завершить текущий шаг с результатом.
    void finishCurrentStep(StepResult result, const QString& details = {});

private slots:
    void onTimeoutTick();

private:
    void advanceToStep(int index);
    void setState(DispatcherState newState);
    void emitProgress();

    QList<StepDescriptor>   m_steps;
    QList<StepResult>       m_results;
    DispatcherState         m_state      {DispatcherState::Idle};
    int                     m_currentIdx {-1};

    QTimer                  m_timeoutTimer;
    std::shared_ptr<ILogger> m_logger;
    bool                    m_stepFinishing {false};  ///< Защита от реентрантного вызова

    static constexpr const char* kSource = "ScenarioDispatcher";
};

} // namespace Msv::Core
