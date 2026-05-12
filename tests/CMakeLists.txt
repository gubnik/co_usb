cmake_minimum_required(VERSION 3.13...3.31)

project(
  co_usb_tests
  VERSION 0.1.0
  LANGUAGES CXX)

set(COUSB_TESTS tests)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Catch2 CONFIG REQUIRED COMPONENTS main)

include(CTest)
include(Catch)

file(GLOB_RECURSE TEST_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_executable(${PROJECT_NAME} ${TEST_SRCS})
target_link_libraries(${PROJECT_NAME} PRIVATE co_usb Catch2::Catch2WithMain)

catch_discover_tests(
  ${PROJECT_NAME}
  WORKING_DIRECTORY
  "${CMAKE_CURRENT_BINARY_DIR}"
  OUTPUT_DIR
  "${CMAKE_CURRENT_BINARY_DIR}/test_reports"
  ADD_TAGS_AS_LABELS
  SKIP_IS_FAILURE
  DISCOVERY_MODE
  POST_BUILD)
