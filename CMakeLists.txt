cmake_minimum_required(VERSION 3.16)
project(rvc-controller CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(GTest REQUIRED)

set(SRC_FILES
    src/CleaningController.cpp
    src/HighPowerTimer.cpp
    src/NavigationController.cpp
    src/RVCController.cpp
)

add_executable(rvc_tests
    ${SRC_FILES}
    tests/CleaningControllerTest.cpp
    tests/NavigationControllerTest.cpp
    tests/RVCControllerTest.cpp
    tests/HighPowerTimerTest.cpp
)

target_include_directories(rvc_tests PRIVATE src tests)
target_link_libraries(rvc_tests GTest::gtest GTest::gtest_main)

# ── 시각적 시뮬레이터 ────────────────────────────────────────────────────
add_executable(rvc_simulator
    ${SRC_FILES}
    simulator/main.cpp
)
target_include_directories(rvc_simulator PRIVATE src simulator)

# ── 인터랙티브 터미널 시뮬레이터 ──────────────────────────────────────────
add_executable(rvc_interactive
    ${SRC_FILES}
    simulator/interactive.cpp
)
target_include_directories(rvc_interactive PRIVATE src simulator)

# ── 시스템 테스트 (Google Test 없음) ─────────────────────────────────────
add_executable(rvc_system_tests
    ${SRC_FILES}
    system-tests/SystemTests.cpp
)
target_include_directories(rvc_system_tests PRIVATE src simulator system-tests)

# ── UP팀 시나리오 기반 시스템 테스트 v2 ──────────────────────────────────
add_executable(rvc_system_tests_v2
    ${SRC_FILES}
    system-tests/SystemTests_v2.cpp
)
target_include_directories(rvc_system_tests_v2 PRIVATE src simulator system-tests)

option(ENABLE_COVERAGE "Enable gcov/lcov coverage instrumentation" OFF)
if(ENABLE_COVERAGE)
    target_compile_options(rvc_tests        PRIVATE --coverage -O0 -g)
    target_link_options(rvc_tests           PRIVATE --coverage)
    target_compile_options(rvc_system_tests PRIVATE --coverage -O0 -g)
    target_link_options(rvc_system_tests    PRIVATE --coverage)
endif()
