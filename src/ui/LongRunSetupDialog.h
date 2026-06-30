#pragma once

#include <QDialog>

QT_BEGIN_NAMESPACE
class QSpinBox;
class QPushButton;
class QLabel;
class QTextEdit;
class QProgressBar;
QT_END_NAMESPACE

namespace Msv::Core { struct LongRunResult; }

namespace Msv::Ui {

// ─────────────────────────────────────────────────────────────────────────────
/// Диалог длительного теста.
/// Фаза 1: настройка (длительность в минутах)
/// Фаза 2: живой прогресс во время теста
/// Фаза 3: итоговый результат
// ─────────────────────────────────────────────────────────────────────────────
class LongRunSetupDialog : public QDialog {
    Q_OBJECT
public:
    explicit LongRunSetupDialog(QWidget* parent = nullptr);

    [[nodiscard]] int durationMinutes() const;

    // Вызывается извне для обновления прогресса
    void updateProgress(int elapsedSec, int totalSec, const QString& stats);
    void showResult(const Core::LongRunResult& result);
	void updateStats(const QString& stats);
	void updateTimer(int elapsedSec, int totalSec);

signals:
    void startRequested(int durationMinutes);
    void stopRequested();

private slots:
    void onStartClicked();
    void onStopClicked();

private:
    void buildUi();
    void applyTheme();
    void switchToRunning();
    void switchToFinished();

    QSpinBox*     m_durationSpin   {nullptr};
    QPushButton*  m_startBtn       {nullptr};
    QPushButton*  m_stopBtn        {nullptr};
    QPushButton*  m_closeBtn       {nullptr};
    QProgressBar* m_progressBar    {nullptr};
    QLabel*       m_timeLabel      {nullptr};
    QTextEdit*    m_statsView      {nullptr};
    QLabel*       m_resultLabel    {nullptr};

    enum class Phase { Setup, Running, Finished };
    Phase m_phase {Phase::Setup};
};

} // namespace Msv::Ui
