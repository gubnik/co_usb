cmake_minimum_required(VERSION 3.31)

project(
  co_usb_examples
  VERSION 0.1.0
  LANGUAGES CXX)

set(COUSB_EXAMPLES example)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE EXAMPLES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
foreach(example_path IN LISTS EXAMPLES)
  get_filename_component(fname "${example_path}" NAME)
  get_filename_component(target_name "${fname}" NAME_WE)
  set(EXAMPLE "${COUSB_EXAMPLES}_${target_name}")
  add_executable(${EXAMPLE} "${example_path}")
  target_link_libraries(${EXAMPLE} PRIVATE co_usb)
endforeach()
