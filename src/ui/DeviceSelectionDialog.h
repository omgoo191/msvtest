#pragma once

#include "network/WhoIAmTypes.h"
#include <QDialog>
#include <QList>

QT_BEGIN_NAMESPACE
class QListWidget;
class QPushButton;
class QLabel;
QT_END_NAMESPACE

namespace Msv::Ui {

// ─────────────────────────────────────────────────────────────────────────────
/// Диалог выбора устройства после WhoIAm-сканирования.
///
/// Показывает список найденных устройств. Оператор выбирает одно
/// или вводит IP вручную. Всегда появляется — даже если найдено
/// только одно устройство или не найдено ни одного.
// ─────────────────────────────────────────────────────────────────────────────
class DeviceSelectionDialog : public QDialog {
    Q_OBJECT
public:
    explicit DeviceSelectionDialog(const QList<Network::WhoIAmResponse>& devices,
                                   QWidget* parent = nullptr);

    /// true  → оператор выбрал из списка, selectedDevice() валиден
    /// false → оператор ввёл IP вручную, manualIp() валиден
    [[nodiscard]] bool                      wasManualEntry()   const { return m_manualEntry; }
    [[nodiscard]] Network::WhoIAmResponse   selectedDevice()   const { return m_selected; }
    [[nodiscard]] QHostAddress              manualIp()         const { return m_manualIp; }

private slots:
    void onSelectClicked();
    void onManualClicked();
    void onListDoubleClicked();
    void onListSelectionChanged();

private:
    void buildUi(const QList<Network::WhoIAmResponse>& devices);
    void applyTheme();

    QListWidget* m_list       {nullptr};
    QPushButton* m_selectBtn  {nullptr};
    QPushButton* m_manualBtn  {nullptr};
    QLabel*      m_statusLabel{nullptr};

    QList<Network::WhoIAmResponse> m_devices;
    Network::WhoIAmResponse        m_selected;
    QHostAddress                   m_manualIp;
    bool                           m_manualEntry{false};
};

} // namespace Msv::Ui
