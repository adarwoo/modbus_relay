This application implements a MobBUS RTU Relay.

The relay supports the following modbus commands:
READ_COILS 1-3
WRITE_SINGLE_COIL Support for 0x5500 to toggle
WRITE_MULTIPLE_COILS 1-3
READ_HOLDING_REGISTERS
  1 - Version
  2 - FW version
  3 - HW version

If compiling using the supplied Dockerfile, the elf is 7KB in size, so a ATTiny824 will work.

The device default address is 44 (Decimal).
By default, it runs at 115200 8E1.

Tested with QModMaster.

I plan to add address and serial configuration modbus command (and eeprom persistence).
