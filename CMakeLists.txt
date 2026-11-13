cmake_minimum_required(VERSION 3.25)
project(CardFlowOthello VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(CARDFLOW_BUILD_TESTS "単体テスト（GoogleTest）をビルドする" ON)

# ─── 警告設定（自前ターゲットのみに適用。警告はエラー扱い）─────────────
add_library(cardflow_warnings INTERFACE)
if(MSVC)
    target_compile_options(cardflow_warnings INTERFACE /W4 /WX /permissive- /utf-8)
else()
    target_compile_options(cardflow_warnings INTERFACE -Wall -Wextra -Wpedantic -Werror)
endif()

# ─── ゲームロジック（UI 非依存）──────────────────────────────────────
add_library(cardflow_core STATIC
    src/core/board.cpp
    src/core/card.cpp
    src/core/player.cpp
    src/core/rules.cpp
)
target_include_directories(cardflow_core PUBLIC include)
target_link_libraries(cardflow_core PRIVATE cardflow_warnings)

# ─── コンソール描画 ──────────────────────────────────────────────────
add_library(cardflow_ui STATIC
    src/ui/board_view.cpp
)
target_link_libraries(cardflow_ui
    PUBLIC cardflow_core
    PRIVATE cardflow_warnings
)

# ─── 実行ファイル ────────────────────────────────────────────────────
add_executable(cardflow src/main.cpp)
target_link_libraries(cardflow PRIVATE cardflow_ui cardflow_warnings)
if(WIN32)
    target_compile_definitions(cardflow PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

# ─── テスト ──────────────────────────────────────────────────────────
if(CARDFLOW_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
