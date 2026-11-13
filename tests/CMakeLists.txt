include(FetchContent)

# MSVC のランタイム設定（/MD）を本体と揃える
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)

FetchContent_Declare(googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SYSTEM
)
FetchContent_MakeAvailable(googletest)

add_executable(cardflow_tests
    ai_test.cpp
    board_test.cpp
    card_test.cpp
    deck_test.cpp
    game_test.cpp
    gui_app_test.cpp
    layout_test.cpp
    rules_test.cpp
    text_test.cpp
)
target_link_libraries(cardflow_tests PRIVATE
    cardflow_core
    cardflow_app
    cardflow_warnings
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(cardflow_tests)
