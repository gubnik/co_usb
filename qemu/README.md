# QEMU devices

> [!CAUTION]
> Do not treat these devices as full-on emulations of anything in any realistic scenario.
> They are all dumb on purpose and only test basic interactions.
> If you have encountered issues with the library in production usage please contact me via GitHub issues.

This subdirectory includes custom QEMU devices written by myself to test `co_usb` in practical ways.

## Building
All the devices are expected to work from the get-go if included in `hw/usb` directory of QEMU source tree, if this is not the
case for you, attempt using `--disable-werror` flag during QEMU configure step and double-check that you are using appropriate QEMU version.

