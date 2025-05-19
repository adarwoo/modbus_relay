# Modbus Relay #

This project is a fully working MODBUS RTU Relay, which also includes a control of the EStop.
The project comes with schematic and PCB, as well as the firmware of course!

Features:
1. Standard DIN Rail mountable PCB size
2. 3 relays (10A 250VAC)
3. Operational safety minded:
   . Safety relay with force conduits and read back
   . Infeed voltage measurement with acceptable range
   . Communication watchdog
4. EStop management (internal and external)
5. Operational statistics
   . Number of cycles
   . Running time
   . Fault codes
   
The core of the relay is an AVR Tiny3227, an automotive grade MPU designed for harsh environment.

This project can be reused for other Modbus devices. A modbus interface generator is provided that makes adding modbus commands simple.

![estop relay](https://github.com/user-attachments/assets/c7a2c55f-4833-4e39-9875-c24443134138)


<b>Solder side of the board</b>

Out of the box, the relay supports the following modbus commands:
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
# Optional - install gitman
$ pipx install gitman
$ gitman update
$ make NDEBUG=1 # Optional, add ARCH=attiny1624 to suit you device
```

The binary modbus_relay.elf can be found in the directory
# Modbus Relay Device User Manual

## Overview
The Modbus Relay Device is a configurable relay controller with Modbus RTU communication.
This document describes how to configure and operate the device, including its default settings, configuration mode, and reset process.

## Factory default settings
The relay comes pre-configured with the following value:

| Configuration     | Default value             | Explanation                                    |
|-------------------|---------------------------|------------------------------------------------|
| **Slave ID**      | `0x44`                    | The device address is 44 (decimal) by default  |
| **Baud rate**     | `9600`                    | The device talks at 9600 by default            |
| **Serial setup**  | `8N1`                     | 8bits, no parity and 1 stop bit                |
| **Watchdog**      | `0` (off)                 | The watchdog feature is off by default         |


## Resetting the relay to factory default setttings

If the relay is un-responsive, it may have been mis-configured.
The relay can be reset to it's factory default using the following procedure.

1. Power off the relay
2. Configure your Modbus master configuration tool (such as QModMaster) using a baud rate of **9600**, **8 data bits**, **No parity**, and **1 stop bit** (9600 8N1).
3. Ready the following message:

| Field             | Value                     | Explanation                                    |
|-------------------|---------------------------|------------------------------------------------|
| **Slave ID**      | `0x00`                    | Broadcast address.                             |
| **Function Code** | `0x10`                    | Write multiple holding registers.              |
| **Start Address** | `0x0063`                  | Address of the reset register (40100)          |
| **Quantity**      | `0x0002`                  | Writing 2 registers (32-bit number).           |
| **Byte Count**    | `0x04`                    | 4 bytes of data to follow.                     |
| **Data**          | `0xDEAD 0x5AFE`           | MSW = `0xDEAD`, LSW = `0x5AFE`.                |
| **CRC**           | `0xBAE7`                  | CRC                                            |

4. Power on the relay. All LEDs turn RED for 5 seconds.
5. During that time, send the 'Write multiple register' commmand
6. If the relay receives the command, the LEDs will blink fast for 5 seconds, then turn all on indicating the device was successfully reset
7. Now the device is factory reset and can be reached at its address `44` (0x2C).

## Simple relay operation

In normal mode:
- The device responds only to frames addressed to its **Device ID**.
- The bus operates as configured
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
  - Register `0x0000` (40001): Product ID (`0x3701`).
  - Register `0x0001` (40002): Device Hardware Version (`0xMMmm`, where MM = major, mm = minor).
  - Register `0x0002` (40003): Software Version (`0xMMmm`, where MM = major, mm = minor).
  - Register `0x0008` (40009): Device address
  - Register `0x0009` (40010): Baud rate selection (1/100th of the actual baud rate)
  - Register `0x000A` (40011): Parity. 0=None, 1=Odd, 2=Even
  - Register `0x000B` (40012): Number of stop bits. 1=1 stop bit, 2=2 stop bits
  - Register `0x000C` (40013): Watchdog window size in seconds
  - Register `0x0010` (40017 + 40019): Number of minutes in operation as a 32-bits
  - Register `0x0014` (40019 + 40020): Number of relay 0 movements as a 32-bits
  - Register `0x001A` (40021 + 40022): Number of relay 1 movements as a 32-bits
  - Register `0x001C` (40023 + 40024): Number of relay 2 movements as a 32-bits

---

# Modbus Register Map

## Device Identification

| Zero-Based | Modbus Address | Access | Description | Factory Value | Values |
|------------|----------------|--------|-------------|---------------|--------|
| 0 | 40001 | R | Product ID |  |  |
| 1 | 40002 | R | HW version |  |  |
| 2 | 40003 | R | SW version |  |  |

## Communication Settings

| Zero-Based | Modbus Address | Access | Description | Factory Value | Values |
|------------|----------------|--------|-------------|---------------|--------|
| 100 | 40101 | RW | Device address | 44 |  |
| 101 | 40102 | RW | Baud rate selection | 96 | 3=300 Baud<br/>6=600 Baud<br/>12=1200 Baud<br/>24=2400 Baud<br/>48=4800 Baud<br/>96=9600 Baud<br/>192=19200 Baud<br/>364=36400 Baud<br/>576=57600 Baud<br/>1152=115200 Baud |
| 102 | 40103 | RW | Parity | 0 | 0=None</br>1=Odd</br>2=Even</br> |
| 103 | 40104 | RW | Stopbits | 1 | 1=1 Stop bit</br>2=2 stop bits |

## Power Infeed Configuration

| Zero-Based | Modbus Address | Access | Description | Factory Value | Values |
|------------|----------------|--------|-------------|---------------|--------|
| 200 | 40201 | RW | Ingress type | 1 | 0=DC</br>1=AC 50Hz</br>2=AC 60Hz |
| 201 | 40202 | RW | Ingress Min voltage | 10 | 1/10 volts [100-3000] |
| 202 | 40203 | RW | Ingress Max voltage | 300 | 1/10 volts [100-3000] |

## Safety Logic Configuration

| Zero-Based | Modbus Address | Access | Description | Factory Value | Values |
|------------|----------------|--------|-------------|---------------|--------|
| 300 | 40301 | RW | EStop on undervoltage | 1 | 0=no</br>1=yes |
| 301 | 40302 | RW | EStop on overvoltage | 1 | 0=no</br>1=yes |
| 302 | 40303 | RW | EStop on number of seconds without valid modbus activity | 0 | 0=off</br>[1-65535] Number of seconds |

## Status & Monitoring

| Zero-Based | Modbus Address | Access | Description | Factory Value | Values |
|------------|----------------|--------|-------------|---------------|--------|
| 400 | 40401 | R | Current status |  | 0=OK, 1=ESTOP_RESETABLE, 2=ESTOP_TERMINAL |
| 401 | 40402–40403 | R | Running minutes |  | 32-bit unsigned int (High word at 40402, Low word at 40403) |
| 403 | 40404 | R | Infeed DC voltage |  | 1/10 volts |
| 404 | 40405 | R | Infeed AC voltage |  | 1/10 volts |
| 405 | 40406 | R | Fault type |  | 0=None</br>1=faulty relay</br>2=modbus watchdog</br>3=voltage monitor</br>4=external |
| 406 | 40407 | R | External diagnostic code |  | Diagnostic code given with EStop command or 0xffff |
| 407 | 40408 | R | Infeed out-of-range voltage |  | Invalid infeed voltage in 1/10th of volts or 0 |
| 408 | 40409 | R | Relay 1 diagnostic code |  | 0=Relay is OK<br/>1=Faulty relay |
| 409 | 40410 | R | Relay 2 diagnostic code |  | 0=Relay is OK<br/>1=Faulty relay |
| 410 | 40411 | R | Relay 3 diagnostic code |  | 0=Relay is OK<br/>1=Faulty relay |

## Relay Stats

| Zero-Based | Modbus Address | Access | Description | Factory Value | Values |
|------------|----------------|--------|-------------|---------------|--------|
| 500 | 40501 | R | Relay 1 number of cycles |  |  |
| 501 | 40502 | R | Relay 2 number of cycles |  |  |
| 502 | 40503 | R | Relay 3 number of cycles |  |  |

## Control

| Zero-Based | Modbus Address | Access | Description | Factory Value | Values |
|------------|----------------|--------|-------------|---------------|--------|
| 900 | 40901 | W | EStop |  | LSB contains a diagnostic code [1-127]<br/>MSB=1 : Pulse EStop for 1 second<br/>MSB=2 : Resettable EStop<br/>MSB=3 : Terminal EStop |
| 901 | 40902 | W | EStop reset |  | 0x0404 |

## Configuring the device
The factory default communication settings for the relay are:
- Device address is 44
- Baud rate is **9600**
- **8 data bits**
- **No parity**
- **1 stop bit** (9600 8N1).

### Configuration registers available
These can be accessed with a write single register command or the serial settings can be set with 1 write multiple command.

| Register Address | Function                          | Values/Description                    | Factory Default |
|------------------|-----------------------------------|---------------------------------------|-----------------|
| 40009 (0x0008)   | Device ID                         | 1 to 247                              | 44              |
| 40010 (0x0009)   | 1/100th of the baud Rate          | 12, 24, 48, 96, 192, 384, 576, 1152   | 96              |
| 40011 (0x0010)   | Parity                            | 0 = None, 1 = Odd, 2 = Even           | 0 (None)        |
| 40012 (0x0011)   | Stop Bits                         | 1 or 2                                | 1               |
| 40013 (0x0012)   | Watchdog Timeout (seconds)        | 0 = Disabled, 1->65535                | 0               |

**Note**:  When writting the configuration with a multiple write, all 5 settings must be written in the same transaction.

### Reset the device
The register at 0x63 (40100) can be written with 0xDEAD5AFE to reset the device remotely.
This is usefull to apply any changes made to the settings.
The command will reply, and the leds will flash fast for 2 seconds, then solid (the device is booting).

### Using the watchdog
The relay has a command watchdog which will release the relays following a period of inactivity.
Everytime a valid modbus command is received (including holding register read etc.), the watchdog is reset.
This feature allow for the relays to be atomatically releases if the master was to fail.
The period is given in seconds.
A value of zero (default) turns off the feature.

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

# FMEA – Modbus Relay Board for CNC Safety

This FMEA analyzes potential failure modes of a Modbus-controlled relay board designed for CNC safety, including load control and emergency stop (E-stop) functionality.
Any failure should result in the CNC stopping by letting go of the estop switch.
<br/>**Note**: The estop must be checked once at the start of operations. This step is done by the Masso CNC controller. This relay estop contact is Normally Opened. So when the CNC starts, the relay would be open.
If the relay was to close **after** the Masso had booted, this could be considered an estop test.
Therefore, the relay must be started well before the Masso. This is the case (100ms vs 10s).

The top events to consider are:
1. Lack of dust extraction clogs the work and leads to the cutter breaking.
The operation is done in normally closed and protected condition to safeguard the operator.
Mecanical damage only.
2. Lack of cooling of the spindle leads to spindle overheat.
NTP sensor on the spindle should detect the overheat if wired. Else, the spindle could be damaged.
A fire is unlikely. 

| Function             | Failure Mode                  | Effect of Failure                     | Cause(s)                               | Detection Method(s)         | S | O | D | RPN | Recommended Action(s)                                      |
|----------------------|-------------------------------|----------------------------------------|---------------------------------------|------------------------------|---|---|---|-----|-------------------------------------------------------------|
| Control Load Relays  | Relay not switching           | Load not powered leading to CNC or cutter damage | Relay failure, dry joint    | Feedback circuit             | 8 | 4 | 3 | 96  | Add redundant relay check, use industrial-grade relays     |
| Control Load Relays  | Relay stuck ON                | Unsafe load activation                | Relay contact welding                  | Feedback circuit             | 9 | 3 | 4 | 108 | Use a forced conduit relay and read back the status of the relay  |
| E-Stop Relay         | E-stop not triggered          | CNC continues in unsafe state         | Logic fault, relay failure             | Self-test, watchdog          | 10| 2 | 3 | 60  | Use safety-rated relay, periodic self-test                 |
| Power Monitoring     | Over/under voltage undetected | Damage to CNC or relay                | Sensor failure, ADC error              | Voltage threshold check      | 9 | 3 | 4 | 108 | Add voltage sensing, calibration check                    |
| Power Converter      | Converter fails               | Relay board unpowered, failsafe triggers | Component failure                   | Relay state monitoring       | 7 | 4 | 2 | 56  | Use robust converter, thermal protection. =<br/>Power the estop relay from the mains power |
| Modbus Communication | Loss of communication         | E-stop triggered, CNC halts           | Cable fault, EMI, software crash       | Timeout watchdog             | 6 | 5 | 2 | 60  | Loss of comms indicates the bus is damaged or the master has crashed. esop. |
| Test of the esop switch | estop relay opens and close cycle is considered a estop switch test  | False E-stop test                    | Non cold reboot of the firmware | Analysis of the reboot cause | 8 | 3 | 3 | 72  | Do not allow the modbus board to reboot |

## Legend
- **S (Severity)**: 1 (low) to 10 (catastrophic)
- **O (Occurrence)**: 1 (rare) to 10 (frequent)
- **D (Detection)**: 1 (certain detection) to 10 (undetectable)
- **RPN (Risk Priority Number)**: S × O × D

## Summary
Focus should be placed on:
- Improving detection of stuck or failed relays
- Enhancing voltage monitoring redundancy
- Ensuring robust communication and feedback diagnostics

