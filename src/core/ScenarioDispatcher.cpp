#include "ScenarioDispatcher.h"
#include <QMetaEnum>

namespace Msv::Core {

ScenarioDispatcher::ScenarioDispatcher(QList<StepDescriptor> steps,
                                       std::shared_ptr<ILogger> logger,
                                       QObject* parent)
    : IScenarioDispatcher(parent)
    , m_steps(std::move(steps))
    , m_results(m_steps.size(), StepResult::NotRun)
    , m_logger(std::move(logger))
{
    m_timeoutTimer.setSingleShot(true);
    connect(&m_timeoutTimer, &QTimer::timeout,
            this,            &ScenarioDispatcher::onTimeoutTick);
}

// ── Управление ────────────────────────────────────────────────────────────────

void ScenarioDispatcher::start()
{
    if (m_state != DispatcherState::Idle) {
        m_logger->warning(kSource, "start() проигнорирован: диспетчер не в состоянии Idle");
        return;
    }
    m_results.fill(StepResult::NotRun);
    m_logger->info(kSource, QStringLiteral("Сценарий запущен (%1 шагов)").arg(m_steps.size()));
    advanceToStep(0);
}

void ScenarioDispatcher::abort()
{
    m_timeoutTimer.stop();
    m_logger->error(kSource, "Сценарий прерван оператором");
    setState(DispatcherState::Aborted);
    emit scenarioFinished(false);
}

void ScenarioDispatcher::pause()
{
    if (m_state == DispatcherState::Running ||
        m_state == DispatcherState::WaitingOperator)
    {
        m_timeoutTimer.stop();
        setState(DispatcherState::Paused);
        m_logger->info(kSource, "Сценарий приостановлен");
    }
}

void ScenarioDispatcher::resume()
{
    if (m_state != DispatcherState::Paused) return;

    const auto& step = m_steps[m_currentIdx];
    if (step.type == StepType::Automatic) {
        setState(DispatcherState::Running);
        executeStep(m_currentIdx);
    } else {
        setState(DispatcherState::WaitingOperator);
        if (step.timeoutSec > 0)
            m_timeoutTimer.start(step.timeoutSec * 1000);
    }
}

void ScenarioDispatcher::confirmOperatorAction()
{
    if (m_state != DispatcherState::WaitingOperator) {
        m_logger->warning(kSource, "confirmOperatorAction() вне ожидания оператора");
        return;
    }
    m_timeoutTimer.stop();
    m_logger->info(kSource,
        QStringLiteral("Шаг %1 подтверждён оператором").arg(m_currentIdx));
    finishCurrentStep(StepResult::Pass);
}

void ScenarioDispatcher::reportOperatorFailure(const QString& reason)
{
    if (m_state != DispatcherState::WaitingOperator) return;
    m_timeoutTimer.stop();
    m_logger->error(kSource,
        QStringLiteral("Шаг %1 — оператор сообщил об ошибке: %2")
            .arg(m_currentIdx).arg(reason));
    finishCurrentStep(StepResult::Fail, reason);
}

// ── Опрос ─────────────────────────────────────────────────────────────────────

DispatcherState ScenarioDispatcher::state() const            { return m_state; }
int             ScenarioDispatcher::currentStepIndex() const { return m_currentIdx; }
QList<StepDescriptor> ScenarioDispatcher::allSteps()   const { return m_steps; }
QList<StepResult>     ScenarioDispatcher::stepResults()const { return m_results; }

// ── Внутренняя логика ─────────────────────────────────────────────────────────

void ScenarioDispatcher::advanceToStep(int index)
{
    if (index >= m_steps.size()) {
        // Все шаги пройдены
        const bool overallPass = std::none_of(m_results.begin(), m_results.end(),
            [](StepResult r){ return r == StepResult::Fail; });

        m_logger->info(kSource, overallPass
            ? "Сценарий завершён: PASS"
            : "Сценарий завершён: FAIL");

        setState(DispatcherState::Finished);
        emit scenarioFinished(overallPass);
        return;
    }

    m_currentIdx = index;
	const auto& step = m_steps[index];

	emit currentStepChanged(index);
	m_logger->info(kSource, QStringLiteral("→ Шаг %1: %2").arg(index).arg(step.title));
    emit stepStarted(index, step);
    emitProgress();

    if (step.type == StepType::Automatic) {
        setState(DispatcherState::Running);
        executeStep(index);
    } else {
        // OperatorAction / OperatorConfirm
        setState(DispatcherState::WaitingOperator);
        emit operatorActionRequired(index, step);

        if (step.timeoutSec > 0)
            m_timeoutTimer.start(step.timeoutSec * 1000);
    }
}

void ScenarioDispatcher::executeStep(int stepIndex)
{
    // Заглушка Шага 1: все автоматические шаги проходят.
    // В Шагах 2–5 этот метод переопределяется.
    m_logger->debug(kSource,
        QStringLiteral("executeStep(%1) — stub PASS").arg(stepIndex));
    finishCurrentStep(StepResult::Pass, "stub");
}

void ScenarioDispatcher::finishCurrentStep(StepResult result, const QString& details)
{
    if (m_stepFinishing) {
        m_logger->warning(kSource, "finishCurrentStep: повторный вызов проигнорирован");
        return;
    }
    m_stepFinishing = true;
    m_results[m_currentIdx] = result;

    const QString tag = [&]{
        switch (result) {
            case StepResult::Pass:    return QStringLiteral("PASS");
            case StepResult::Fail:    return QStringLiteral("FAIL");
            case StepResult::Warning: return QStringLiteral("WARNING");
            case StepResult::Skipped: return QStringLiteral("SKIPPED");
            default:                  return QStringLiteral("?");
        }
    }();

    m_logger->info(kSource,
        QStringLiteral("Шаг %1 завершён: %2%3")
            .arg(m_currentIdx)
            .arg(tag)
            .arg(details.isEmpty() ? QString{} : QStringLiteral(" — ") + details));

    emit stepFinished(m_currentIdx, result, details);
	m_pendingNextStep = m_currentIdx + 1;
    m_stepFinishing = false;
	setState(DispatcherState::ReviewingResult);
}

void ScenarioDispatcher::onTimeoutTick()
{
    m_logger->warning(kSource,
        QStringLiteral("Таймаут ожидания оператора на шаге %1").arg(m_currentIdx));
    emit operatorActionTimeout(m_currentIdx);
    finishCurrentStep(StepResult::Fail, "таймаут оператора");
}

void ScenarioDispatcher::setState(DispatcherState newState)
{
    if (m_state == newState) return;
    m_state = newState;
    emit stateChanged(newState);
}

void ScenarioDispatcher::emitProgress()
{
    const int total = m_steps.size();
    const int pct   = total > 0 ? (m_currentIdx * 100 / total) : 0;
    emit progressChanged(pct);
}

void ScenarioDispatcher::continueToNextStep()
{
	if (m_state != DispatcherState::ReviewingResult) return;
	advanceToStep(m_pendingNextStep);
}

void ScenarioDispatcher::reset()
{
	m_timeoutTimer.stop();
	m_results.fill(StepResult::NotRun);
	m_currentIdx = -1;
	m_pendingNextStep = 0;
	m_stepFinishing = false;
	setState((DispatcherState::Idle));
	m_logger->info(kSource, "Сценарий сброшен");
}

void ScenarioDispatcher::restartFromStep(int index)
{
	if (index < 0 || index >= m_steps.size()) return;
	m_timeoutTimer.stop();

	// Очистить результаты начиная с index
	for (int i = index; i < m_results.size(); ++i)
		m_results[i] = StepResult::NotRun;

	m_stepFinishing = false;
	setState(DispatcherState::Running);
	advanceToStep(index);
}

} // namespace Msv::Core
