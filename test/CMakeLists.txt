cmake_minimum_required(VERSION 3.16)

# Host-side unit tests for the pwm_controller firmware logic.
# Standalone project on purpose: the root CMakeLists locks in the ARM
# toolchain via pico_sdk_init(), so this must configure independently
# (docker chraac/pico-builder has native g++ alongside the cross toolchain).
#
# The lib sources compile against a fake pico-sdk (test/stubs) instead of the
# real SDK headers -- keep PWM_CONTROLLER_LIB_SRC in exec/CMakeLists.txt and
# the list below in sync.

project(pwm_controller_unit_tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
    URL_HASH SHA256=7b42b4d6ed48810c5362c265a17faebe90dc2373c885e5216439d37927f02926
)
# For fully-offline setups, point this at a local googletest checkout instead:
#   cmake -S test -B build-test -DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=/path/to/googletest
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set(EXEC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../exec)

# Fake pico-sdk: headers shadow the SDK paths, definitions record everything.
add_library(pico_stubs STATIC stubs/stubs.cc)
target_include_directories(pico_stubs PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs/include  # fake SDK headers
    ${CMAKE_CURRENT_SOURCE_DIR}/stubs           # stub_control.hh
)

# Mirror of PWM_CONTROLLER_LIB_SRC (exec/CMakeLists.txt), host-compiled.
# -fno-exceptions matches the firmware build (pico-sdk default): the .cc
# definitions omit the `noexcept` their headers declare, which GCC only
# accepts with exceptions disabled.
add_library(pwm_controller_hostlib STATIC
    ${EXEC_DIR}/frequency_counter.cc
    ${EXEC_DIR}/fan_speed_manager.cc
)
target_include_directories(pwm_controller_hostlib PUBLIC ${EXEC_DIR})
target_compile_options(pwm_controller_hostlib PRIVATE -fno-exceptions)
target_link_libraries(pwm_controller_hostlib PUBLIC pico_stubs)

enable_testing()

add_executable(unit_tests
    unit/pid_test.cc
    unit/ema_smoother_test.cc
    unit/temp_helper_test.cc
    unit/frequency_counter_test.cc
    unit/fan_speed_helper_test.cc
    unit/ina226_test.cc
    unit/fan_speed_manager_test.cc
)
target_link_libraries(unit_tests PRIVATE pwm_controller_hostlib GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(unit_tests)
