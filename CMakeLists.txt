cmake_minimum_required(VERSION 3.15)
project(GravityCascadeOthello VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# AGENT.md §4: 警告ゼロビルド
if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

option(BUILD_TESTS "Unit + e2e tests (dependency-free harness)." ON)
enable_testing()  # CMake 4.xではadd_testの前に明示的にCTest有効化が必要

# ─── DxLib (third_party/dxlib に解凍済み) ──────────────────────────────
set(DXLIB_ADD_DIR "${CMAKE_SOURCE_DIR}/third_party/dxlib/dxlib_vc")
if(NOT EXISTS "${DXLIB_ADD_DIR}/DxLib.h")
    message(FATAL_ERROR "DxLib not found: extract DxLib_VC3_25a.zip into third_party/dxlib/ (see quickstart.md)")
endif()

# ─── 規則引擎（UI/AI从这里だけ状態遷呼呼び出す）─────────────────────────
add_library(gco_engine
    src/core/board.cpp
    src/core/rules.cpp
    src/cards/effects.cpp
    src/ai/ai.cpp
)
target_include_directories(gco_engine PUBLIC ${CMAKE_SOURCE_DIR}/src)

# ─── GUI 主プログラム（DxLib）──────────────────────────────────────────
add_executable(gravitycascade src/main.cpp)
target_link_libraries(gravitycascade PRIVATE gco_engine "${DXLIB_ADD_DIR}/DxLib.lib")
target_include_directories(gravitycascade PRIVATE "${DXLIB_ADD_DIR}")
target_link_directories(gravitycascade PRIVATE "${DXLIB_ADD_DIR}")
# DxLib.h 自体がShift-JISコメントを含むため、UTF-8源码との混用時の既存警告は抑制
if(MSVC)
    target_compile_options(gravitycascade PRIVATE /wd4819 /wd4828 /wd4010 /wd4996)
endif()

# ─── 単体テスト（自前CHECK harness, T004）──────────────────────────────
if(BUILD_TESTS)
    set(GCO_TESTS
        tests/unit/board_test.cpp
        tests/unit/chain_test.cpp
        tests/unit/flip_test.cpp
        tests/unit/end_test.cpp
        tests/unit/cards_test.cpp
        tests/unit/ai_test.cpp
        tests/e2e/selfplay_test.cpp
    )
    foreach(test_src ${GCO_TESTS})
        get_filename_component(test_name ${test_src} NAME_WE)
        add_executable(${test_name} ${test_src})
        target_link_libraries(${test_name} PRIVATE gco_engine)
        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()
endif()
