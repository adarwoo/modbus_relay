# Modbus Relay #

This project is a fully working MODBUS RTU Relay.
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

<img src="https://github.com/adarwoo/modbus_relay/blob/main/hw/back.png" width="400">
<b>Solder side of the board</b>

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
```bash
# Clone this repo
$ git clone https://https://github.com/adarwoo/modbus_relay.git
$ cd modbus_relay
$ gitman update
$ make NDEBUG=1 # Optional, add ARCH=attiny1624 to suit you device
```

The binary modbus_relay.elf can be found in the directory
# Modbus Relay Device User Manual

## Overview
The Modbus Relay Device is a configurable relay controller with Modbus RTU communication. This document describes how to configure and operate the device, including its default settings, configuration mode, and reset process.

## Normal Operation
In normal mode:
- The device responds only to frames addressed to its **Device ID**.
- The bus operated as configured
- Relays operate based on the configured default positions and command inversion settings.
- Regular coil commands do not respond in configuration mode to prevent mode mixing.

### Supported Modbus Functions
- **Read Coils (Function Code 1):**
  - Addresses `0x0000` to `0x0002` for Relay 0, Relay 1, and Relay 2.
- **Write Single Coil (Function Code 5):**
  - Write `0xFF00` to turn on, `0x0000` to turn off, and `0x5500` to toggle a relay.
- **Write Multiple Coils (Function Code 15):**
  - Write a bit array to set relay states (1 = closed, 0 = open).
- **Read Holding Registers (Function Code 3):**
  - Register `0x0000`: Product ID (`0x3701`).
  - Register `0x0001`: Device Hardware Version (`0xMMmm`, where MM = major, mm = minor).
  - Register `0x0002`: Software Version (`0xMMmm`, where MM = major, mm = minor).

### Note on Relay Command Inversion
If the relay command inversion configuration is active (`0xFF00` in relevant registers):
- A "close" command will be interpreted as "open" and vice versa.

---

## Resetting the relay to factory default setttings

The relay can be reset to it's factory default using the following procedure.

1- Power off the relay
1- Configure your Modbus master configuration tool (such as QModMaster) using a baud rate of **9600**, **8 data bits**, **No parity**, and **1 stop bit** (9600 8N1).
2- Ready the following message:

| Field            | Value                      | Explanation                                     |
|-------------------|----------------------------|------------------------------------------------|
| **Slave ID**      | `0x00`                    | Broadcast address.                             |
| **Function Code** | `0x10`                    | Write multiple holding registers.              |
| **Start Address** | `0x0000`                  | Address of the first register (reset).         |
| **Quantity**      | `0x0002`                  | Writing 2 registers (32-bit number).           |
| **Byte Count**    | `0x04`                    | 4 bytes of data to follow.                     |
| **Data**          | `0x5AFE 0xDEAD`           | MSW = `0x5AFE`, LSW = `0xDEAD`.                |
| **CRC**           | `0E9D4`                   | E4 D4                                          |

4- Power on the relay. All LEDs turn RED for 2 seconds.
5- During the first 2 seconds, send the 'Write multiple register' commmand
6- If the relay receives the command, the LEDs will blink fast (10Hz) for 5 seconds, then turn all on indicating the device is reset
7- Now the device is factory reset and can be reached at its address 44 (0x2C).

## Configuring the device
The factory default communication settings for the relay are:
- Device address is 44
- Baud rate is **9600**
- **8 data bits**
- **No parity**
- **1 stop bit** (9600 8N1).

### Configuration registers available
| Register Address | Function                          | Values/Description                    | Factory Default |
|------------------|-----------------------------------|---------------------------------------|-----------------|
| 40001 (0x0000)   | Device ID                         | 1 to 247                              | 44              |
| 40002 (0x0001)   | Baud Rate                         | 1200 to 115200 (e.g., 9600)           | 9600            |
| 40003 (0x0002)   | Parity                            | 0 = None, 1 = Odd, 2 = Even           | 0 (None)        |
| 40004 (0x0003)   | Stop Bits                         | 1 or 2                                | 1               |
| 40005 (0x0004)   | Watchdog Timeout (seconds)        | 0 = Disabled                          | 0               |
| 40006 (0x0005)   | Relay 0 Default Position          | 0x0000 = Open, 0xFF00 = Closed        | 0x0000          |
| 40007 (0x0006)   | Relay 1 Default Position          | Same as above                         | 0x0000          |
| 40008 (0x0007)   | Relay 2 Default Position          | Same as above                         | 0x0000          |
| 40009 (0x0008)   | Relay Command Inversion (Relay 0) | 0x0000 = Normal, 0xFF00 = Inverted    | 0x0000          |
| 40010 (0x0009)   | Relay Command Inversion (Relay 1) | Same as above                         | 0x0000          |
| 40011 (0x000A)   | Relay Command Inversion (Relay 2) | Same as above                         | 0x0000          |

### Using the watchdog

### Holding registers

---


## Resetting the Device
### Reset via Register
To reset the device to factory defaults:
1. Write `0x1234` to Register `40012` (Reset Command) using Modbus Function Code `16`.
2. The device will reboot and reinitialize with default settings.
3. Configuration mode will be available for 2 seconds after the reset.

### Reset Behavior
- All LEDs will blink in sync at **2 Hz** for 2 seconds to indicate a reset.
- The device will revert to the default configuration.

---

## Troubleshooting
### Cannot Communicate with Device
- Ensure the device is in **configuration mode** or responding to the correct **Device ID**.
- Check baud rate, parity, and stop bit settings.

### Lost Device ID
1. Send a valid broadcast frame to enter configuration mode.
2. Use the default Device ID (`0`) to reset the device via the reset register.

### Relay Does Not Respond
- Verify relay default positions and inversion settings in the holding registers.
- Check the watchdog timeout setting to ensure it isn’t triggering prematurely.

---

## Notes
- Configuration mode is only accessible during the 2-second boot window.
- For security and reliability, avoid using **Device ID 0** for normal operations.
- Always validate CRC and frame format for successful communication.

---
