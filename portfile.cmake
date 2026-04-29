vcpkg_from_github(OUT_SOURCE_PATH SOURCE_PATH REPO gubnik/co_usb REF dev)

vcpkg_configure_cmake(SOURCE_PATH ${SOURCE_PATH} PREFER_NINJA OPTIONS
                      -DCOUSB_BUILD_EXAMPLES=OFF)

vcpkg_install_cmake()

vcpkg_copy_pdbs()
