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
    Core
    Gui
    Widgets
    Network
    SerialPort
)

# ── Предупреждения компилятора ────────────────────────────────────────────────
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# ── Подпроекты ────────────────────────────────────────────────────────────────
add_subdirectory(src)

# ── Тесты (подключить позже) ──────────────────────────────────────────────────
# enable_testing()
# add_subdirectory(tests)
