# co_usb
> v2.0.0

Asynchronous USB library using C++20 coroutines and [Capy](https://github.com/cppalliance/capy) to
create a lightweight interface for [libusb-1.0](https://libusb.info/). It provides minimal necessary abstractions
over base libusb to enable efficient and clean concurrent I/O using Capy's common interfaces
for seamless interoperability with wider coroutine ecosystem.

## Rationale

`libusb` asynchronous code is very efficient, but because it is a C library it is lacking in ergonomics when
put into modern C++20 project, especially one utilizing coroutines. This library serves as a bridge between
Capy based coroutine ecosystem and `libusb` asynchronous code.

### What co_usb does

- Asynchronous transfer submission for partial and complete I/O in a form of light adaptor types
- Hotplug API for dynamically connecting devices with an Asio style acceptor loop
- Wrappers around core `libusb` entities such as devices for smooth initialization of the library
- Robust event handler system for easy extension and safe shutdown

### What co_usb does not do

- Querying information for devices and endpoint
- Iteration over device list
- Generally, everything that does not relate to asynchronous operations

## Why not Asio?

Not all USB code needs a whole networking library. Using Capy allows the user to choose not to use
networking code. If networking is needed the user is free to use `co_usb` with [Corosio](https://github.com/cppalliance/corosio),
since both operate on shared Capy concepts the code is easily made interoperable!

## Getting started

This project uses `vcpkg` as a package manager.
To use it in your `vcpkg`-based projects, do the following:
1. Add [portfile](`./portfile.cmake`) and [vcpkg.json](./vcpkg.json) to your `ports` directory
2. Copy `./res/ports/boost-capy/` and `./res/ports/libusb/` to your `ports` directory
3. Provide them as overlay to the `vcpkg`
4. Add the following to your `CMakeLists.txt`:
```cmake
find_package(co_usb CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE co_usb::co_usb)
```

For non-`vcpkg` projects, you will have to use CMake's `FetchContent` module and provide
Capy and libusb on your own. This is method of consuming `co_usb` is not endorsed and may not work.
```cmake
include(FetchContent)
FetchContent_Declare(co_usb
    GIT_REPOSITORY https://github.com/gubnik/co_usb.git
    GIT_TAG master
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(co_usb)

target_link_libraries(my_app co_usb::co_usb)
```

## Build from source

To build from source:
```
git clone https://github.com/gubnik/co_usb.git
cd co_usb
./setup.sh x64-static Debug # or x64-dyn instead of x64-static, or Release instead of Debug
```

To fully rebuild:
```
./setup.sh ${PRESET} ${BUILD_TYPE} reset
```

## Documentation

See [docs](https://gubnik.github.io/co_usb) for generated docs

## Tests

For unit tests, see `./tests`. For QEMU device code, see `./qemu`. I will gladly accept contributions to the testing suite.

## License

Source code of the library is distributed under the Boost Software License, Version 1.0.
(See accompanying file [LICENSE.md](LICENSE.md) or copy at
https://www.boost.org/LICENSE_1_0.txt)

QEMU devices code at `qemu/` is distributed under GNU General Public License v3.
(See accompanying file [qemu/LICENSE.md](qemu/LICENSE.md) or copy at
https://www.gnu.org/licenses/gpl-3.0.md)
