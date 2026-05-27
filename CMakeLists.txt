cmake_minimum_required(VERSION 3.21)

project(MsvTester
        VERSION 0.1.0
        DESCRIPTION "Стендовое ПО для проверки МСВ"
        LANGUAGES CXX
        )

# ── Стандарт ──────────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── Qt автогенерация ───────────────────────────────────────────────────────────
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# ── Qt компоненты ──────────────────────────────────────────────────────────────
# Добавляем компоненты по мере реализации шагов:
#   Core, Gui, Widgets  — Шаг 1 (каркас)
#   Network             — Шаги 2–3 (WhoIAm, Web, SNTP)
#   SerialPort          — Шаг 4 (UART-GNSS)
find_package(Qt6 REQUIRED COMPONENTS
    Core Gui Widgets Network SerialPort
)

# ── Предупреждения компилятора ────────────────────────────────────────────────
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# ── Исходники (раскомментировать по мере реализации шагов) ────────────────────
set(SOURCES
        src/main.cpp

        # Шаг 1 — каркас
        src/core/Logger.cpp
        src/core/DeviceModel.cpp
        src/core/ScenarioDispatcher.cpp
        src/ui/MainWindow.cpp
        src/core/IDeviceModel.cpp
        src/core/IscenarioDispatcher.cpp
        src/ui/StepItemWidget.cpp

        # Шаг 2 — WhoIAm
        # src/network/WhoIAmScanner.cpp

        # Шаг 3 — Web + SNTP
        # src/network/WebClient.cpp
        # src/network/SntpClient.cpp

        # Шаг 4 — UART-GNSS
        # src/serial/UartMonitor.cpp
        # src/serial/NmeaParser.cpp

        # Шаг 5 — Анализатор
        # src/analysis/DataAnalyzer.cpp

        # Шаг 6 — Отчёт
        # src/report/ReportGenerator.cpp
        )

qt6_add_executable(MsvTester ${SOURCES})

target_include_directories(MsvTester PRIVATE src)

target_link_libraries(MsvTester PRIVATE
        Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network Qt6::SerialPort
        )

install(TARGETS MsvTester RUNTIME DESTINATION bin)