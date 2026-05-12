find_package(Doxygen REQUIRED)

if(NOT DOXYGEN_FOUND)
  message(FATAL_ERROR "Doxygen is required to build docs")
endif()

include(FetchContent)
FetchContent_Declare(
  doxygen-awesome-css
  URL https://github.com/jothepro/doxygen-awesome-css/archive/refs/heads/main.zip
)
FetchContent_MakeAvailable(doxygen-awesome-css)

FetchContent_GetProperties(doxygen-awesome-css SOURCE_DIR AWESOME_CSS_DIR)

set(DOXYFILE_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
set(DOXYFILE_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)

add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/docs/html
  COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYFILE_OUT}
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  COMMENT "Generating API documentation with Doxygen"
  VERBATIM)

add_custom_target(docs ALL DEPENDS ${CMAKE_BINARY_DIR}/docs/html)
