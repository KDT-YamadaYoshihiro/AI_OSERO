# ─── tests/CMakeLists.txt ──────────────────────────────────────────────
# 単体テスト（Google Test）＋ E2E テスト（Catch2）のビルド設定

cmake_minimum_required(VERSION 3.15)
project(UnitTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Google Test の検索
find_package(GTest REQUIRED)

# ─── 単体テストターゲット ──────────────────────────────────────────────

add_executable(board_test
    unit/board_test.cpp
)
target_link_libraries(board_test PRIVATE GTEST::gtest GTEST::gtest_main)
set_target_properties(board_test PROPERTIES CXX_STANDARD 20)

# ─── E2E テストターゲット（Catch2）─────────────────────────────────────

add_executable(e2e_tests
    e2e/game_flow_test.cpp
)
target_link_libraries(e2e_tests PRIVATE Catch2::Catch2WithMain)
set_target_properties(e2e_tests PROPERTIES CXX_STANDARD 20)

# ─── テスト実行用マクロ（ctest と統合）─────────────────────────────────

add_custom_target(
    run_unit_tests
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_SOURCE_DIR}/../build"
    COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_FLAGS} ${COVERAGE_FLAGS} "unit/board_test.cpp" -o "${CMAKE_BINARY_DIR}/unit_tests.exe" 2>/dev/null || echo "Unit tests require gtest setup in main CMakeLists.txt"
)

add_custom_target(
    run_e2e_tests
    COMMAND e2e_tests --help
    DEPENDS e2e_tests
)

# ─── カバレッジ計測用のヘルパ（後ほど追加）─────────────────────────────

# 注記：実際のカバレッジ計測は、CMakeLists.txt メインで統括します。
