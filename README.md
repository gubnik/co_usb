> [!IMPORTANT]
> Experimental - may be unstable, expect all kind of changes

# co_usb

Modern C++ library using C++20 coroutines and [Boost.Capy](https://github.com/cppalliance/capy) to
create a lightweight interface for [libusb-1.0](https://libusb.info/). It provides minimal abstractions
over base libusb to enable efficient and clean concurrent I/O using Boost.Capy's common interfaces
so it can be used seamlessly with libraries that consume those interfaces, e.g. Boost.Http.

# Getting started

This project uses `vcpkg` as a package manager.
To use it in your `vcpkg`-based projects, do the following:
1. Add [portfile](`./portfile.cmake`) and [vcpkg.json](./vcpkg.json) to your ports directory
2. Copy `./res/ports/boost-capy/` and `./res/ports/libusb/` to your ports dir
3. Provide them as overlay to the `vcpkg`
4. Add the following to your `CMakeLists.txt`:
```cmake
find_package(co_usb CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE co_usb::co_usb)
```

For non-`vcpkg` projects, you will have to use CMake's `FetchContent` module and provide
Boost.Capy and libusb on your own. This is method of consuming `co_usb` is not endorsed and may not work.
```cmake
include(FetchContent)
FetchContent_Declare(co_usb
    GIT_REPOSITORY https://github.com/gubnik/co_usb.git
    GIT_TAG master
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(co_usb)

target_link_libraries(my_app co_usb::co_usb)
```

# Documentation

See [docs](https://gubnik.github.io/co_usb) for WIP generated docs
