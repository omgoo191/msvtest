#include <QApplication>
#include <QStyleFactory>
#include <QPalette>

#include "core/IScenarioDispatcher.h"
#include "core/MsvScenarioDispatcher.h"
#include "core/DeviceModel.h"
#include "core/Logger.h"
#include "ui/MainWindow.h"

static QList<Msv::Core::StepDescriptor> buildScenarioSteps()
{
    using namespace Msv::Core;
    return {
        {0, StepType::Automatic,      "Поиск МСВ (WhoIAm)",           "Автоматический широковещательный запрос WhoIAm.\nОжидается ответ от изделия в сети.", 30},
        {1, StepType::Automatic,      "Чтение Web-статуса",            "HTTP-запрос к Web-интерфейсу изделия.\nПроверяется: источник синхронизации, статус, Web-время.", 15},
        {2, StepType::Automatic,      "SNTP-запрос к изделию",         "UDP-запрос к SNTP-серверу изделия (порт 123).\nПроверяется: LI, Stratum, время ответа.", 15},
        {3, StepType::OperatorAction, "Подключить UART-дубликат",      "Подключите кабель UART-дубликата GNSS-выхода изделия к COM-порту ПК.\nПосле подключения нажмите «ВЫПОЛНЕНО».", 0},
        {4, StepType::Automatic,      "Мониторинг NMEA (UART)",        "Пассивное чтение NMEA-потока GNSS-приёмника изделия.\nПроверяется: checksum RMC/GGA, монотонность UTC.", 60},
        {5, StepType::OperatorAction, "Отключить штатную антенну",     "Отключите штатную GNSS-антенну от изделия.\nПодключите имитатор КЗ на антенный разъём.\nПосле замены нажмите «ВЫПОЛНЕНО».", 0},
        {6, StepType::Automatic,      "Мониторинг состояния при КЗ",   "Автоматическая фиксация состояния изделия (Web + SNTP + UART)\nпри закороченной антенне. Длительность: 60 с.", 90},
        {7, StepType::OperatorAction, "Восстановить штатную антенну",  "Отключите имитатор КЗ.\nПодключите штатную GNSS-антенну.\nПосле восстановления нажмите «ВЫПОЛНЕНО».", 0},
        {8, StepType::Automatic,      "Ожидание восстановления GNSS",  "Ожидание повторного захвата спутников и восстановления\nсинхронизации после замены антенны.", 300},
        {9, StepType::Automatic,      "Формирование протокола",        "Автоматическая генерация протокола проверки в файл.", 10},
    };
}

static void applyDarkPalette(QApplication& app)
{
    app.setStyle(QStyleFactory::create("Fusion"));

    QPalette p;
    p.setColor(QPalette::Window,         QColor(0x14, 0x14, 0x14));
    p.setColor(QPalette::WindowText,      QColor(0xd8, 0xd8, 0xd8));
    p.setColor(QPalette::Base,            QColor(0x0e, 0x0e, 0x0e));
    p.setColor(QPalette::AlternateBase,   QColor(0x1c, 0x1c, 0x1c));
    p.setColor(QPalette::Text,            QColor(0xd8, 0xd8, 0xd8));
    p.setColor(QPalette::BrightText,      Qt::white);
    p.setColor(QPalette::Button,          QColor(0x1e, 0x1e, 0x1e));
    p.setColor(QPalette::ButtonText,      QColor(0xd8, 0xd8, 0xd8));
    p.setColor(QPalette::Highlight,       QColor(0x00, 0x89, 0x7b));
    p.setColor(QPalette::HighlightedText, Qt::white);
    p.setColor(QPalette::ToolTipBase,     QColor(0x1c, 0x1c, 0x1c));
    p.setColor(QPalette::ToolTipText,     QColor(0xd8, 0xd8, 0xd8));
    p.setColor(QPalette::PlaceholderText, QColor(0x44, 0x44, 0x44));
    p.setColor(QPalette::Disabled, QPalette::WindowText, QColor(0x38, 0x38, 0x38));
    p.setColor(QPalette::Disabled, QPalette::Text,       QColor(0x38, 0x38, 0x38));
    p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(0x38, 0x38, 0x38));
    app.setPalette(p);
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("MsvTester");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("Lab");

    applyDarkPalette(app);

    // ── Model ─────────────────────────────────────────────────────────────────
    auto deviceModel = std::make_unique<Msv::Core::DeviceModel>();

    // ── Logger ────────────────────────────────────────────────────────────────
    auto compositeLogger = std::make_shared<Msv::Core::CompositeLogger>();
    compositeLogger->addBackend(std::make_unique<Msv::Core::ConsoleLogBackend>());
    compositeLogger->addBackend(std::make_unique<Msv::Core::FileLogBackend>("msv_session.log"));

    auto logModel = std::make_unique<Msv::Core::ModelLogBackend>();
    Msv::Core::ModelLogBackend* logModelPtr = logModel.get();
    compositeLogger->addBackend(std::move(logModel));

    // ── Controller ────────────────────────────────────────────────────────────
    Msv::Network::WhoIAmConfig whoIAmConfig;
    whoIAmConfig.port            = 54321;   // порт МСВ — уточнить по документации
    whoIAmConfig.retries         = 3;
    whoIAmConfig.retryIntervalMs = 2000;

    auto dispatcher = std::make_unique<Msv::Core::MsvScenarioDispatcher>(
        buildScenarioSteps(),
        compositeLogger,
        deviceModel.get(),
        whoIAmConfig
    );

    // ── View ──────────────────────────────────────────────────────────────────
    Msv::Ui::MainWindow window(dispatcher.get(), deviceModel.get(), logModelPtr);
    window.show();

    compositeLogger->info("main", "Приложение запущено");
    return app.exec();
}
