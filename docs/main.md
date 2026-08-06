# co_usb

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

## License

Source code of the library is distributed under the Boost Software License, Version 1.0.
(See accompanying file [LICENSE.md](LICENSE.md) or copy at
https://www.boost.org/LICENSE_1_0.txt)

QEMU devices code at `qemu/` is distributed under GNU General Public License v3.
(See accompanying file [qemu/LICENSE.md](qemu/LICENSE.md) or copy at
https://www.gnu.org/licenses/gpl-3.0.md)

## Contributing

`co_usb` is open for contributions to:
- documentation
- test suite
- bug fixes

New features can be submitted are subject to thorough review process.
