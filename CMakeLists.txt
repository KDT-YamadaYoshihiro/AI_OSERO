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
    src/core/ai.cpp
    src/core/board.cpp
    src/core/card.cpp
    src/core/card_effects.cpp
    src/core/deck.cpp
    src/core/game.cpp
    src/core/player.cpp
    src/core/rules.cpp
)
target_include_directories(cardflow_core PUBLIC include)
target_link_libraries(cardflow_core PRIVATE cardflow_warnings)

# ─── GUI の状態と画面配置（描画ライブラリ非依存）────────────────────
add_library(cardflow_app STATIC
    src/app/gui_app.cpp
    src/app/layout.cpp
    src/app/text.cpp
)
target_link_libraries(cardflow_app
    PUBLIC cardflow_core
    PRIVATE cardflow_warnings
)

# ─── 実行ファイル（DxLib による GUI）────────────────────────────────
# DxLib は third_party/dxlib/ に「プロジェクトに追加すべきファイル_VC用」の中身を置く（git 管理外）。
# ライブラリは DxLib.h の #pragma comment(lib) で自動的にリンクされる。
set(CARDFLOW_DXLIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/dxlib" CACHE PATH "DxLib（VC 用）のフォルダ")
if(WIN32 AND MSVC AND EXISTS "${CARDFLOW_DXLIB_DIR}/DxLib.h")
    add_executable(cardflow WIN32
        src/gui/main.cpp
        src/gui/renderer.cpp
    )
    target_include_directories(cardflow SYSTEM PRIVATE "${CARDFLOW_DXLIB_DIR}")
    target_link_directories(cardflow PRIVATE "${CARDFLOW_DXLIB_DIR}")
    target_compile_definitions(cardflow PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_link_libraries(cardflow PRIVATE cardflow_app cardflow_warnings)
else()
    message(WARNING "DxLib が見つからないため GUI（cardflow）はビルドしません: ${CARDFLOW_DXLIB_DIR}")
endif()

# ─── テスト ──────────────────────────────────────────────────────────
if(CARDFLOW_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
