cmake_minimum_required(VERSION 3.31)

project(cousb VERSION 0.1.0 LANGUAGES CXX)

option(COUSB_ENABLE_EXAMPLES "co_usb examples" ON)

# libusb-1.0
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUSB REQUIRED IMPORTED_TARGET libusb-1.0)

# Boost::capy
find_package(boost_capy CONFIG REQUIRED)
find_package(Threads)

file(GLOB_RECURSE PROJECT_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/*cpp)
add_library(${PROJECT_NAME} ${PROJECT_SRCS})

target_link_libraries(${PROJECT_NAME} Boost::capy PkgConfig::LIBUSB)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(COUSB_ENABLE_EXAMPLES)
    file(GLOB_RECURSE EXAMPLES ${CMAKE_CURRENT_SOURCE_DIR}/examples/*.cpp)
    foreach(example_path IN LISTS EXAMPLES)
        get_filename_component(fname "${example_path}" NAME)
        get_filename_component(target_name "${fname}" NAME_WE)
        add_executable("${target_name}" "${example_path}")
        target_link_libraries(${target_name} PRIVATE ${PROJECT_NAME})
    
        set(BASE_OUT "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}/examples")
    
        foreach(cfg IN LISTS CMAKE_CONFIGURATION_TYPES)
            string(TOUPPER "${cfg}" cfg_upper)
            set_target_properties("${target_name}"
            PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY_${cfg_upper} "${BASE_OUT}"
                ARCHIVE_OUTPUT_DIRECTORY_${cfg_upper} "${BASE_OUT}"
                LIBRARY_OUTPUT_DIRECTORY_${cfg_upper} "${BASE_OUT}"
            )
        endforeach()
    
        set_target_properties("${target_name}"
        PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${BASE_OUT}"
            ARCHIVE_OUTPUT_DIRECTORY "${BASE_OUT}"
            LIBRARY_OUTPUT_DIRECTORY "${BASE_OUT}"
        )
    endforeach()
endif()
