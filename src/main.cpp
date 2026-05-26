#include <QApplication>
#include <QStyleFactory>

#include "core/IScenarioDispatcher.h"
#include "core/ScenarioDispatcher.h"
#include "core/DeviceModel.h"
#include "core/Logger.h"
#include "ui/MainWindow.h"

// ─────────────────────────────────────────────────────────────────────────────
/// Фабрика шагов сценария — полный список проверок МСВ.
///
/// Шаг 1: заглушки (StepType::Automatic → executeStep stub).
/// Реальные реализации подключаются в Шагах 2–5.
// ─────────────────────────────────────────────────────────────────────────────
static QList<Msv::Core::StepDescriptor> buildScenarioSteps()
{
    using namespace Msv::Core;
    return {
        // ── Шаг 2: Обнаружение изделия ───────────────────────────────────────
        {
            0, StepType::Automatic,
            "Поиск МСВ (WhoIAm)",
            "Автоматический широковещательный запрос WhoIAm.\n"
            "Ожидается ответ от изделия в сети.",
            30
        },
        // ── Шаг 3a: Web-статус ───────────────────────────────────────────────
        {
            1, StepType::Automatic,
            "Чтение Web-статуса",
            "HTTP-запрос к Web-интерфейсу изделия.\n"
            "Проверяется: источник синхронизации, статус, Web-время.",
            15
        },
        // ── Шаг 3b: SNTP-проверка ────────────────────────────────────────────
        {
            2, StepType::Automatic,
            "SNTP-запрос к изделию",
            "UDP-запрос к SNTP-серверу изделия (порт 123).\n"
            "Проверяется: LI, Stratum, время ответа.",
            15
        },
        // ── Шаг 4: Подключение UART-дубликата ────────────────────────────────
        {
            3, StepType::OperatorAction,
            "Подключить UART-дубликат",
            "Подключите кабель UART-дубликата GNSS-выхода изделия к COM-порту ПК.\n"
            "После подключения нажмите «Выполнено».",
            0  // без таймаута
        },
        // ── Шаг 4b: Мониторинг NMEA ──────────────────────────────────────────
        {
            4, StepType::Automatic,
            "Мониторинг NMEA (UART)",
            "Пассивное чтение NMEA-потока GNSS-приёмника изделия.\n"
            "Проверяется: checksum RMC/GGA, монотонность UTC.",
            60
        },
        // ── Шаг 5a: Отключение штатной антенны ───────────────────────────────
        {
            5, StepType::OperatorAction,
            "Отключить штатную антенну",
            "Отключите штатную GNSS-антенну от изделия.\n"
            "Подключите имитатор КЗ на антенный разъём.\n"
            "После замены нажмите «Выполнено».",
            0
        },
        // ── Шаг 5b: Фиксация состояния КЗ ────────────────────────────────────
        {
            6, StepType::Automatic,
            "Мониторинг состояния при КЗ",
            "Автоматическая фиксация состояния изделия (Web + SNTP + UART)\n"
            "при закороченной антенне. Длительность: 60 с.",
            90
        },
        // ── Шаг 5c: Восстановление штатной антенны ───────────────────────────
        {
            7, StepType::OperatorAction,
            "Восстановить штатную антенну",
            "Отключите имитатор КЗ.\n"
            "Подключите штатную GNSS-антенну.\n"
            "После восстановления нажмите «Выполнено».",
            0
        },
        // ── Шаг 5d: Восстановление синхронизации ─────────────────────────────
        {
            8, StepType::Automatic,
            "Ожидание восстановления GNSS",
            "Ожидание повторного захвата спутников и восстановления\n"
            "синхронизации после замены антенны.",
            300
        },
        // ── Шаг 6: Формирование протокола ────────────────────────────────────
        {
            9, StepType::Automatic,
            "Формирование итогового протокола",
            "Автоматическая генерация протокола проверки в файл.",
            10
        },
    };
}

// ─────────────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("MsvTester");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("Lab");

    // ── Тёмная тема (опционально) ─────────────────────────────────────────────
    // app.setStyle(QStyleFactory::create("Fusion"));

    // ── Слой Model ────────────────────────────────────────────────────────────
    auto deviceModel = std::make_unique<Msv::Core::DeviceModel>();

    // ── Логгер: консоль + файл + модель для UI ────────────────────────────────
    auto compositeLogger = std::make_shared<Msv::Core::CompositeLogger>();
    compositeLogger->addBackend(std::make_unique<Msv::Core::ConsoleLogBackend>());
    compositeLogger->addBackend(std::make_unique<Msv::Core::FileLogBackend>("msv_session.log"));

    // ModelLogBackend живёт отдельно — его указатель нужен и диспетчеру, и MainWindow
    auto logModel = std::make_unique<Msv::Core::ModelLogBackend>();
    // Добавляем в составной логгер (CompositeLogger берёт владение копией через unique_ptr)
    // ВАЖНО: сохраняем raw-ptr до передачи владения
    Msv::Core::ModelLogBackend* logModelPtr = logModel.get();
    compositeLogger->addBackend(std::move(logModel));

    // ── Слой Controller ───────────────────────────────────────────────────────
    auto dispatcher = std::make_unique<Msv::Core::ScenarioDispatcher>(
        buildScenarioSteps(),
        compositeLogger
    );

    // ── Слой View ─────────────────────────────────────────────────────────────
    Msv::Ui::MainWindow window(
        dispatcher.get(),
        deviceModel.get(),
        logModelPtr
    );
    window.show();

    compositeLogger->info("main", "Приложение запущено");

    return app.exec();
}
