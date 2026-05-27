#pragma once

#include <QObject>
#include <QString>
#include <QList>

namespace Msv::Core {

// ─────────────────────────────────────────────────────────────────────────────
/// Тип шага сценария.
// ─────────────────────────────────────────────────────────────────────────────
enum class StepType : int {
    Automatic,       ///< Выполняется автоматически (запрос сети, парсинг, …)
    OperatorAction,  ///< Требует физического действия оператора
    OperatorConfirm, ///< Оператор подтверждает выполненное действие
    Wait,
	Analysis,
	Capture,
};

// ─────────────────────────────────────────────────────────────────────────────
/// Результат выполнения шага.
// ─────────────────────────────────────────────────────────────────────────────
enum class StepResult : int {
    NotRun  = 0,
    Pass    = 1,
    Fail    = 2,
    Warning = 3,
    Skipped = 4,
};

// ─────────────────────────────────────────────────────────────────────────────
/// Описатель одного шага сценария (неизменяемый).
// ─────────────────────────────────────────────────────────────────────────────
struct StepDescriptor {
    int     id;           ///< Уникальный порядковый номер
    StepType type;
    QString title;        ///< Короткое название (для таблицы шагов в UI)
    QString description;  ///< Подробное описание/инструкция для оператора
    int     timeoutSec;   ///< Таймаут ожидания (0 = бесконечно)
};

// ─────────────────────────────────────────────────────────────────────────────
/// Текущее состояние диспетчера.
// ─────────────────────────────────────────────────────────────────────────────
enum class DispatcherState : int {
    Idle,             ///< Не запущен
    Running,          ///< Выполняется автоматический шаг
    WaitingOperator,  ///< Ожидает подтверждения оператора
    Paused,           ///< Приостановлен вручную
    Finished,         ///< Сценарий завершён (все шаги пройдены)
    Aborted,          ///< Прерван аварийно
    ReviewingResult,
};

// ─────────────────────────────────────────────────────────────────────────────
/// Абстрактный диспетчер сценариев — ядро системы.
///
/// Жизненный цикл:
///   Idle → (start) → Running ↔ WaitingOperator → ... → Finished
///                  ↘ (abort) → Aborted
///
/// UI-слой:
///   - Подписывается на сигналы (stepChanged, stateChanged, …)
///   - Вызывает только confirmOperatorAction() / abort() / start()
///   - Никогда не читает внутреннее состояние напрямую
// ─────────────────────────────────────────────────────────────────────────────
class IScenarioDispatcher : public QObject {
    Q_OBJECT
public:
    explicit IScenarioDispatcher(QObject* parent = nullptr) : QObject(parent) {}
    ~IScenarioDispatcher() override = default;

    // ── Управление ────────────────────────────────────────────────────────────
    virtual void start() = 0;           ///< Запустить сценарий с первого шага
    virtual void abort() = 0;           ///< Аварийное прерывание
    virtual void pause() = 0;           ///< Пауза перед текущим шагом
    virtual void resume() = 0;          ///< Возобновить из паузы
	virtual void continueToNextStep() = 0; ///< оператор нажал далее
	virtual void reset() = 0;           ///< Сброс для повтороного запуска

    /// Оператор нажал «Выполнено» — разрешить переход к следующему шагу.
    virtual void confirmOperatorAction() = 0;

    /// Оператор сообщил об ошибке на своём шаге.
    virtual void reportOperatorFailure(const QString& reason) = 0;

    // ── Опрос ─────────────────────────────────────────────────────────────────
    [[nodiscard]] virtual DispatcherState       state()       const = 0;
    [[nodiscard]] virtual int                   currentStepIndex() const = 0;
    [[nodiscard]] virtual QList<StepDescriptor> allSteps()    const = 0;
    [[nodiscard]] virtual QList<StepResult>     stepResults() const = 0;

signals:
    // ── Сигналы состояния ─────────────────────────────────────────────────────
    /// Диспетчер перешёл в новое состояние.
    void stateChanged(Msv::Core::DispatcherState newState);

    /// Начат новый шаг.
    void stepStarted(int stepIndex, const Msv::Core::StepDescriptor& descriptor);

    /// Шаг завершён с результатом.
    void stepFinished(int stepIndex, Msv::Core::StepResult result, const QString& details);

    /// Требуется физическое действие оператора (UI отображает инструкцию).
    void operatorActionRequired(int stepIndex, const Msv::Core::StepDescriptor& descriptor);

    /// Таймаут ожидания оператора истёк.
    void operatorActionTimeout(int stepIndex);

    /// Все шаги пройдены.
    void scenarioFinished(bool overallPass);

    /// Прогресс: [0..100]
    void progressChanged(int percent);
};

} // namespace Msv::Core
