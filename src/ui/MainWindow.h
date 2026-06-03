#pragma once

#include <QMainWindow>
#include <QList>
#include <memory>
#include "network/WhoIAmTypes.h"
#include "ui/SerialPortSelectionDialog.h"

namespace Msv::Core {
    class IScenarioDispatcher;
    class IDeviceModel;
    class ModelLogBackend;
    class MsvScenarioDispatcher;   // для подключения manualIpRequired
    struct DeviceSnapshot;
    struct LogEntry;
    struct StepDescriptor;
    enum class DispatcherState : int;
    enum class StepResult : int;
}

QT_BEGIN_NAMESPACE
class QLabel;
class QProgressBar;
class QPushButton;
class QTextEdit;
class QScrollArea;
class QWidget;
QT_END_NAMESPACE

namespace Msv::Ui {

class StepItemWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(Core::IScenarioDispatcher* dispatcher,
                        Core::IDeviceModel*         deviceModel,
                        Core::ModelLogBackend*       logModel,
                        QWidget* parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void onStartClicked();
    void onConfirmClicked();
    void onAbortClicked();

    void onStateChanged     (Core::DispatcherState newState);
    void onStepStarted      (int idx, const Core::StepDescriptor& desc);
    void onStepFinished     (int idx, Core::StepResult result, const QString& details);
    void onOperatorActionRequired(int idx, const Core::StepDescriptor& desc);
    void onSnapshotChanged  (const Core::DeviceSnapshot& snap);
    void onLogEntryAdded    (const Core::LogEntry& entry);
    void onScenarioFinished (bool overallPass);
    void onManualIpRequired ();
    void onDeviceSelectionRequired(const Msv::Network::WhoIAmResponseList& found);   ///< WhoIAm не нашёл устройство — показать диалог
    void onContinueClicked();
	void onStepItemClicked(int index);
	void onPortSelectionRequired();

private:
    void buildUi();
    void applyTheme();
    void connectSignals();
    void updateButtons();
    void setStatusIndicator(const QString& text, const QString& color);
    void setPrompt(const QString& header, const QString& body, const QString& accentColor);

    // ── Top bar ───────────────────────────────────────────────────────────────
    QLabel*       m_deviceLabel   {nullptr};
    QLabel*       m_statusDot     {nullptr};
    QLabel*       m_statusText    {nullptr};
    QProgressBar* m_progressBar   {nullptr};

    // ── Steps panel (left) ────────────────────────────────────────────────────
    QWidget*      m_stepsContainer{nullptr};
    QList<StepItemWidget*> m_stepItems;

    // ── Main panel (center) ───────────────────────────────────────────────────
    QLabel*       m_stepIndexLabel{nullptr};
    QLabel*       m_stepTitleLabel{nullptr};
    QLabel*       m_stepTypeLabel {nullptr};
    QLabel*       m_promptHeader  {nullptr};
    QLabel*       m_promptBody    {nullptr};

    // ── Buttons ───────────────────────────────────────────────────────────────
    QPushButton*  m_startBtn      {nullptr};
    QPushButton*  m_confirmBtn    {nullptr};
    QPushButton*  m_abortBtn      {nullptr};

    // ── Log panel (bottom) ────────────────────────────────────────────────────
    QTextEdit*    m_logView       {nullptr};

    // ── Dependencies ─────────────────────────────────────────────────────────
    Core::IScenarioDispatcher* m_dispatcher  {nullptr};
    Core::IDeviceModel*        m_deviceModel {nullptr};
    Core::ModelLogBackend*     m_logModel    {nullptr};

	QPushButton* m_continueBtn {nullptr};

	QPushButton* m_restartFromBtn {nullptr};
	int          m_selectedStepIndex {-1};

	QPushButton* m_devModeBtn  {nullptr};
	bool         m_devMode     {false};

	struct StepDisplayRecord {
		QString promptHeader;
		QString promptBody;
		QString accentColor;
	};

	QLabel* m_antennaIndicator {nullptr};

	QList<StepDisplayRecord> m_stepRecords;
	int                      m_viewingStepIndex {-1};
};

} // namespace Msv::Ui
