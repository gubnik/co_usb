find_package(Catch2 CONFIG REQUIRED COMPONENTS main)

include(CTest)
include(Catch)

file(GLOB_RECURSE TEST_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp)
add_executable(${PROJECT_NAME}_tests ${TEST_SRCS})
target_link_libraries(${PROJECT_NAME}_tests PRIVATE co_usb
                                                    Catch2::Catch2WithMain)

catch_discover_tests(
  ${PROJECT_NAME}_tests
  WORKING_DIRECTORY
  "${CMAKE_CURRENT_BINARY_DIR}"
  OUTPUT_DIR
  "${CMAKE_CURRENT_BINARY_DIR}/test_reports"
  ADD_TAGS_AS_LABELS
  SKIP_IS_FAILURE
  DISCOVERY_MODE
  POST_BUILD)
