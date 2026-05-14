## Release v1.0.0

- add transfer status and usb error mappings to std::error_code
- add RAII wrappers for device ref, device handle, interface, kernel driver guard and transfer
- add async transfer support
- - add compile-time endpoint direction semantics
- - add transfer awaitable
- - add transfer stream types
- add async hotplug support
- - add hotplug awaitable
- - add device acceptor
- add doxygen docs
- add examples
- add basic tests

