cmake_minimum_required(VERSION 3.15)
project(CardFlowOthello VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# コンパイラフラグ（C++20）
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -g)
endif()

# カバレッジ計測用フラグ
set(COVERAGE_FLAGS "-g -fprofile-arcs -ftest-coverage" CACHE STRING "Coverage flags")

# テストビルドのオン/オフ
option(BUILD_TESTS "Build Google Test and Catch2 based tests." ON)

# 依存ライブラリのフォークリポジトリ（C++20 サポート）
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTEST REQUIRED IMPORTED_TARGET gtest)
pkg_check_modules(CATCH2 REQUIRED IMPORTED_TARGET catch2)

# メインターゲット：ゲーム本体
add_executable(dxgame
    src/main.cpp
    src/board_impl.cpp
    src/stone_impl.cpp
    src/card_impl.cpp
    src/game_impl.cpp
)
target_include_directories(dxgame PRIVATE include)
set_target_properties(dxgame PROPERTIES CXX_STANDARD 20 CXX_EXTENSIONS OFF)

# ─── 単体テストターゲット（Google Test）──────────────────────────────────

if(BUILD_TESTS)
    # board_test.cpp
    add_executable(board_test
        tests/unit/board_test.cpp
    )
    target_link_libraries(board_test PRIVATE GTEST::gtest GTEST::gtest_main)
    set_target_properties(board_test PROPERTIES CXX_STANDARD 20 CXX_EXTENSIONS OFF)

    # card_test.cpp
    add_executable(card_test
        tests/unit/card_test.cpp
    )
    target_include_directories(card_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_link_libraries(card_test PRIVATE GTEST::gtest GTEST::gtest_main)
    set_target_properties(card_test PROPERTIES CXX_STANDARD 20 CXX_EXTENSIONS OFF)

    # player_test.cpp
    add_executable(player_test
        tests/unit/player_test.cpp
    )
    target_include_directories(player_test PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_link_libraries(player_test PRIVATE GTEST::gtest GTEST::gtest_main)
    set_target_properties(player_test PROPERTIES CXX_STANDARD 20 CXX_EXTENSIONS OFF)

endif()

# ─── E2E テストターゲット（Catch2）─────────────────────────────────────

if(BUILD_TESTS)
    add_executable(e2e_tests
        tests/e2e/game_flow_test.cpp
    )
    target_include_directories(e2e_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
    target_link_libraries(e2e_tests PRIVATE Catch2::Catch2WithMain)
    set_target_properties(e2e_tests PROPERTIES CXX_STANDARD 20 CXX_EXTENSIONS OFF)

endif()

# ─── カバレッジ情報用ファイル ────────────────────────────────────────────

configure_file(${CMAKE_SOURCE_DIR}/cmake/CoverageConfig.cmake.in
    ${CMAKE_BINARY_DIR}/CoverageConfig.cmake @ONLY)
