# Modbus Relay #

This project is a fully working MobBUS RTU Relay.
The project comes with schematic and PCB, as well as the firmware.

The core of the relay is an AVR Tiny824.
Note: The code optimized with -Os is <6KB. The -Og used typically for debug is 11600 (so you would need a ATTiny1624).
I always buy the largest device. Here the ATTiny3224.
Simply edit the makefile to specify your device.

This project can be reused for other Modbus devices.

The modbus slave code is created from an interface created in Python which generate the datagram decoder.

```python
"device@44": [
        (READ_COILS,            u16(0, 2, alias="addr"),
                                u16(1, 3, alias="qty"),
                                "on_read_coils"),

        (WRITE_SINGLE_COIL,     u16(0, 2, alias="addr"),
                                u16([0xFF00, 0, 0x5500], alias="op"),
                                "on_set_single"),

        (WRITE_MULTIPLE_COILS,  u16(0, alias="from"),
                                u16(3, alias="qty"),
                                u8(1, alias="count"),
                                u8(0, 7, alias="values"),
                                "on_set_multiple"),

        (READ_HOLDING_REGISTERS, u16(0, 2), u16(1, 3), "on_read_info"),
    ]
```
The handling code is just as simple:

```c++
   void on_read_info(uint8_t index, uint8_t qty) {
      Datagram::pack<uint8_t>(qty*2);

      while ( qty-- ) {
         switch(index++) {
         case 0: Datagram::pack( DEVICE_ID ); break;
         case 1: Datagram::pack( HW_VERSION ); break;
         case 2: Datagram::pack( FW_VERSION ); break;
         default:
            Datagram::reply_error(modbus::error_t::illegal_data_value);
         }
      }
   }
```

<img src="https://github.com/adarwoo/modbus_relay/blob/main/hw/back.png" alt="Alt Text" width="400">
*Solder side of the board* 

The relay supports the following modbus commands:
- READ_COILS 1-3
- WRITE_SINGLE_COIL Support for 0x5500 to toggle
- WRITE_MULTIPLE_COILS 1-3
- READ_HOLDING_REGISTERS
  1. Version
  2. FW version
  3. HW version

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

1. Clone this repo</li>
2. $ cd modbus_relay</li>
3. $ gitman update</li>
4. $ make NDEBUG=1</li>

The binary modbus_relay.elf can be found in the directory
