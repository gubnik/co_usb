# co_usb
> Version v2.0.0

`co_usb` is an asynchronous USB library using C++20 coroutines and [Capy](https://github.com/cppalliance/capy) to
create a high-level interface for [libusb](https://libusb.info/). It provides lightweight and feature-rich
adaptors around `libusb` API to enable easy integration into the coroutine ecosystem.

## Rationale

`libusb` asynchronous code is very efficient, but because it is a C library it is lacking in ergonomics when
put into modern C++20 project, especially one utilizing coroutines. This library serves as a bridge between
Capy based coroutine ecosystem and `libusb` asynchronous code.

### What co_usb does

- Asynchronous transfer submission for partial and complete I/O in a form of light adaptor types
- Hotplug API for dynamically connecting devices with an Asio style acceptor loop
- Wrappers around core `libusb` entities such as devices for smooth initialization of the library
- Event handler system for easy extension and safe shutdown

### What co_usb does not do

- Querying information for devices and endpoint
- Iteration over device list
- Generally, everything that does not relate to asynchronous operations

## Why not Asio?

Not all USB code needs a whole networking library. Using Capy allows the user to choose not to use
networking code and just keep the coroutines.

If you need to use networking I advise use `co_usb` with [Corosio](https://github.com/cppalliance/corosio),
since both operate on shared Capy concepts the code is easily made interoperable.

## Getting started

### vcpkg

To use it in your `vcpkg`-based projects, do the following:

1. Add [portfile](`./portfile.cmake`) and [vcpkg.json](./vcpkg.json) to your `ports` directory
2. Copy [Capy port](`./res/ports/boost-capy/`) to your `ports` directory and provide it as an overlay
3. Add `co_usb` as a dependency to your `vcpkg.json` as `cousb`

The library will be available in CMake as the following:
```cmake
find_package(co_usb CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE co_usb::co_usb)
```

### FetchContent

For non-`vcpkg` projects, you will have to use CMake's `FetchContent` module and provide
Capy and libusb on your own. This is method of consuming `co_usb` is *not endorsed* and may not work.
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
cmake --preset="$PRESET" 
cmake --build "build/$PRESET" --config "$BUILD_TYPE"
```

### Presets

Available presets are:
- `x64-linux`
- `x64-linux-static` (`udev` is disabled in port for static builds)
- `no-vcpkg` (used by docs CI)

Native Windows builds are coming soon.

### Build types

Available build types are: `Release`, `Debug`

## Documentation

See [docs](https://gubnik.github.io/co_usb) for generated docs

## Tests

For unit tests, see `./tests`. For QEMU device code, see `./qemu`.

I will gladly accept contributions to the testing suite.

## License

Source code of the library is distributed under the Boost Software License, Version 1.0.
(See accompanying file [LICENSE.md](LICENSE.md) or copy at
https://www.boost.org/LICENSE_1_0.txt)

QEMU devices code at `qemu/` is distributed under GNU General Public License v3.
(See accompanying file [qemu/LICENSE.md](qemu/LICENSE.md) or copy at
https://www.gnu.org/licenses/gpl-3.0.md)
