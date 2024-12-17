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

---

## Default Configuration
Upon boot, the device initializes with the following default settings:
- **Baud Rate:** 115200
- **Data Bits:** 8
- **Parity:** Even
- **Stop Bits:** 1
- **Device ID:** 44
- **Watchdog Timeout:** Disabled
- **Relay Default Positions:** All open (0x0000)
- **Relay Command Inversion:** Disabled

---

## Configuration Mode
The device enters configuration mode for **2 seconds** after boot. During this time:
- The device listens for any valid Modbus broadcast frame (Address `0`) using a baud rate of **9600**, **8 data bits**, **No parity**, and **1 stop bit** (9600 8N1).
- If a valid frame is received, the boot process halts, and the three relay LEDs flash at **10 Hz**, indicating the device is in configuration mode.
- **Note**: The device does not reply to broadcast message. However, once the ID is programmed and known, the registers can be read at the device ID address.

### Configuration Registers
| Register Address | Function                          | Values/Description                     | Factory Default |
|------------------|-----------------------------------|----------------------------------------|-----------------|
| 40001 (0x0000)   | Device ID                         | 1 to 247                              | 44              |
| 40002 (0x0001)   | Baud Rate                         | 1200 to 115200 (e.g., 9600)           | 115200          |
| 40003 (0x0002)   | Parity                            | 0 = None, 1 = Odd, 2 = Even           | 2 (Even)        |
| 40004 (0x0003)   | Stop Bits                         | 1 or 2                                | 1               |
| 40005 (0x0004)   | Watchdog Timeout (seconds)        | 0 = Disabled                          | 0               |
| 40006 (0x0005)   | Relay 0 Default Position          | 0x0000 = Open, 0xFF00 = Closed        | 0x0000          |
| 40007 (0x0006)   | Relay 1 Default Position          | Same as above                         | 0x0000          |
| 40008 (0x0007)   | Relay 2 Default Position          | Same as above                         | 0x0000          |
| 40009 (0x0008)   | Relay Command Inversion (Relay 0) | 0x0000 = Normal, 0xFF00 = Inverted    | 0x0000          |
| 40010 (0x0009)   | Relay Command Inversion (Relay 1) | Same as above                         | 0x0000          |
| 40011 (0x000A)   | Relay Command Inversion (Relay 2) | Same as above                         | 0x0000          |
| 40012 (0x000B)   | Reset Command                     | Write `0x1234` to reset device        | -               |

### Steps to Configure the Device
1. **Enter Configuration Mode:**
   - Send any valid Modbus broadcast frame (Address `0`) during the first 2 seconds after boot.
   - LEDs will flash at **10 Hz**, indicating configuration mode is active.
2. **Set Parameters:**
   - Write to the configuration registers using Modbus Function Code `16` (Preset Multiple Registers).
3. **Confirm Settings (Optional):**
   - Set a new Device ID in Register `40001`.
   - Use the new ID to read back configuration values using Modbus Function Code `3` (Read Holding Registers).
4. **Exit Configuration Mode:**
   - Wait 2 seconds of inactivity, and the device will enter normal operation mode.

---

## Normal Operation
In normal mode:
- The device responds only to frames addressed to its **Device ID**.
- Relays operate based on the configured default positions and command inversion settings.
- Regular coil commands do not respond in configuration mode to prevent mode mixing.

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
