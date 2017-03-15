MODBUS-TOOLS

Command-line tools that allows scripts and applications easily interact with MODBUS RTU devices via RS-485 adapter attached to PC or SoC (like Raspberry Pi). Also supports MODBUS TCP.
Built on top of libmodbus: http://libmodbus.org

Package consists of 4 tools:

* modbus-ping - for bus scanning and online monitoring purposes (0x08 function)
* modbus-id - verbose information about certain bus device (0x2b/0x0e MEI function)
* modbus-read - read inputs/registers and bits/coils from certain device (0x01, 0x02, 0x03, 0x04 functions)
* modbus-write - write registers and coils to certain device (0x05, 0x06 functions)

TODO:
* modbus-id functionality (now it's just a stub)
* modbus-read and modbus-write support for bits and coils
* configurable RS-485 (half-duplex) and RS-232 (full-duplex) modes
