# Modbus Relay #

This application implements a MobBUS RTU Relay.

<img src="https://github.com/adarwoo/modbus_relay/blob/main/hw/back.png" alt="Alt Text" width="400">

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

KiCAD schematic and PCB are available.

Tested with QModMaster.

I plan to add address and serial configuration modbus command (and eeprom persistence).

## How to build ##
You will need a Linux shell, or WSL shell in Windows.
The tool 'gitman' is required as well as Docker. (docker-ce or else).

### Steps ###
1 - Clone this repo
2 - $ cd modbus_relay
3 - $ gitman update
4 - $ make NDEBUG=1

The binary modbus_relay.elf can be found in the directory
