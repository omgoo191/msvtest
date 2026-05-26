#pragma once

#include <QMainWindow>
#include <memory>

// Forward-декларации: View не включает заголовки Core напрямую,
// только через указатели/ссылки на интерфейсы.
namespace Msv::Core {
    class IScenarioDispatcher;
    class IDeviceModel;
    class ModelLogBackend;
    struct DeviceSnapshot;
    struct LogEntry;
    enum class DispatcherState : int;
    enum class StepResult : int;
    struct StepDescriptor;
}

QT_BEGIN_NAMESPACE
class QLabel;
class QProgressBar;
class QPushButton;
class QTableWidget;
class QTextEdit;
class QStackedWidget;
QT_END_NAMESPACE

namespace Msv::Ui {

// ─────────────────────────────────────────────────────────────────────────────
/// Главное окно приложения.
///
/// View-слой: не содержит бизнес-логики. Только:
///   - отображение данных из модели,
///   - передача команд пользователя в диспетчер,
///   - подписка на сигналы Core-объектов.
// ─────────────────────────────────────────────────────────────────────────────
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(Core::IScenarioDispatcher* dispatcher,
                        Core::IDeviceModel*         deviceModel,
                        Core::ModelLogBackend*       logModel,
                        QWidget* parent = nullptr);
    ~MainWindow() override = default;

private slots:
    // — Команды оператора ─────────────────────────────────────────────────────
    void onStartClicked();
    void onConfirmClicked();
    void onAbortClicked();

    // — Обновление отображения ────────────────────────────────────────────────
    void onStateChanged(Core::DispatcherState newState);
    void onStepStarted(int idx, const Core::StepDescriptor& desc);
    void onStepFinished(int idx, Core::StepResult result, const QString& details);
    void onOperatorActionRequired(int idx, const Core::StepDescriptor& desc);
    void onSnapshotChanged(const Core::DeviceSnapshot& snap);
    void onLogEntryAdded(const Core::LogEntry& entry);
    void onScenarioFinished(bool overallPass);

private:
    void buildUi();
    void connectSignals();
    void updateControlButtons();

    // ── Виджеты ───────────────────────────────────────────────────────────────
    QLabel*        m_statusLabel       {nullptr};
    QLabel*        m_deviceInfoLabel   {nullptr};
    QProgressBar*  m_progressBar       {nullptr};

    QPushButton*   m_startBtn          {nullptr};
    QPushButton*   m_confirmBtn        {nullptr};
    QPushButton*   m_abortBtn          {nullptr};

    QTableWidget*  m_stepsTable        {nullptr};  ///< Таблица шагов с PASS/FAIL
    QTextEdit*     m_operatorPrompt    {nullptr};  ///< Инструкция для оператора
    QTextEdit*     m_logView           {nullptr};  ///< Живой лог событий

    // ── Зависимости (не владеет, только использует) ───────────────────────────
    Core::IScenarioDispatcher* m_dispatcher  {nullptr};
    Core::IDeviceModel*        m_deviceModel {nullptr};
    Core::ModelLogBackend*     m_logModel    {nullptr};
};

} // namespace Msv::Ui
