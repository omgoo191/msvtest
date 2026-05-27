#pragma once

#include <QDialog>
#include <QList>
#include <QSerialPortInfo>

QT_BEGIN_NAMESPACE
class QListWidget;
class QPushButton;
class QComboBox;
class QLabel;
QT_END_NAMESPACE

namespace Msv::Ui {

// ─────────────────────────────────────────────────────────────────────────────
/// Диалог выбора COM-порта для UART-дубликата.
// ─────────────────────────────────────────────────────────────────────────────
class SerialPortSelectionDialog : public QDialog {
    Q_OBJECT
public:
    explicit SerialPortSelectionDialog(QWidget* parent = nullptr);

    [[nodiscard]] QString selectedPort() const { return m_selectedPort; }
    [[nodiscard]] int     selectedBaud() const { return m_selectedBaud; }

private slots:
    void onSelectClicked();
    void onRefreshClicked();

private:
    void buildUi();
    void applyTheme();
    void populatePorts();

    QListWidget* m_portList  {nullptr};
    QComboBox*   m_baudCombo {nullptr};
    QPushButton* m_selectBtn {nullptr};
    QLabel*      m_statusLabel{nullptr};

    QString m_selectedPort;
    int     m_selectedBaud {9600};
};

} // namespace Msv::Ui
