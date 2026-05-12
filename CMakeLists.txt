cmake_minimum_required(VERSION 3.13...3.31)

project(
  co_usb
  VERSION 0.1.0
  LANGUAGES CXX)

option(COUSB_BUILD_TESTS "co_usb tests" ON)
option(COUSB_BUILD_EXAMPLES "co_usb examples" ON)
option(COUSB_BUILD_DOCS "co_usb docs" ON)
option(COUSB_ONLY_BUILD_DOCS "co_usb only docs" OFF)

if(COUSB_ONLY_BUILD_DOCS)
  include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Doxygen.cmake)
else()
  # libusb-1.0
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(LIBUSB REQUIRED IMPORTED_TARGET libusb-1.0)

  # Boost::capy
  find_package(boost_capy CONFIG REQUIRED)
  find_package(Threads)

  file(GLOB_RECURSE PROJECT_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/*cpp)
  add_library(${PROJECT_NAME} ${PROJECT_SRCS})

  target_link_libraries(${PROJECT_NAME} PUBLIC Boost::capy PkgConfig::LIBUSB)

  target_include_directories(
    ${PROJECT_NAME}
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

  target_compile_definitions(${PROJECT_NAME} INTERFACE)

  if(COUSB_BUILD_EXAMPLES)
    add_subdirectory(examples)
  endif()

  if(COUSB_BUILD_DOCS)
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Doxygen.cmake)
  endif()

  if(COUSB_BUILD_TESTS)
    add_subdirectory(tests)
  endif()

  # installation section

  include(GNUInstallDirs)

  install(
    TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

  install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "*.hh"
    PATTERN "*.inl")

  install(
    EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})

  include(CMakePackageConfigHelpers)

  set(CONFIG_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})
  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion)

  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake" @ONLY)

  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
                "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
          DESTINATION ${CONFIG_INSTALL_DIR})
endif()
