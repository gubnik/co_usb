## Release v2.0.0

### Transfers
- separate transfer resource from transfer operations
- add transfer sequences
- add streaming submission
- add complete I/O

### Hotplug
- fix acceptor to not segfault on cancellation
- change acceptor implementation to increase performance
- implement device detachment logic for acceptor
- add one-shot hotplug

### Wrappers
- add error protocol concept and its implementations
- change wrappers to be semantically more verbose

### Event handler
- add event handler functionality
- change context to be a non-template struct
- add event_handler_ref to combat premature shutdown


## Release v1.0.1

- add range clause to read/write_some of transfer types

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

