# ── Исходники по слоям ────────────────────────────────────────────────────────
# Каждый шаг добавляет свои файлы в соответствующую секцию.

set(CORE_SOURCES
    # Шаг 1 — каркас
    core/Logger.cpp
    core/DeviceModel.cpp
    core/ScenarioDispatcher.cpp

    # Шаг 2 — WhoIAm (добавить после реализации)
    # network/WhoIAmScanner.cpp

    # Шаг 3 — Web + SNTP (добавить после реализации)
    # network/WebClient.cpp
    # network/SntpClient.cpp

    # Шаг 4 — UART-GNSS (добавить после реализации)
    # serial/UartMonitor.cpp
    # serial/NmeaParser.cpp

    # Шаг 5 — Анализатор (добавить после реализации)
    # analysis/DataAnalyzer.cpp

    # Шаг 6 — Отчёт (добавить после реализации)
    # report/ReportGenerator.cpp
)

set(UI_SOURCES
    ui/MainWindow.cpp
    # ui/MainWindow.ui   ← добавить .ui файл когда будет готов
)

qt6_add_executable(MsvTester
    main.cpp
    ${CORE_SOURCES}
    ${UI_SOURCES}
)

target_include_directories(MsvTester PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}   # позволяет писать #include "core/Logger.h"
)

target_link_libraries(MsvTester PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::SerialPort
)

# Опциональная установка (десктоп)
install(TARGETS MsvTester
    RUNTIME DESTINATION bin
)
