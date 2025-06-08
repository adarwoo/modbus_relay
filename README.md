# Modbus Relay with EStop <img src="https://github.com/user-attachments/assets/516eb8d2-8e22-4c80-9cc7-a677c1ba3664" height="30"> #

This project features is a MODBUS RTU relays controller, which includes failsafe mode and an EStop.
It is aimed for industrial systems such as a CNC or equivalent.

The project comes complete with documentation, schematic, PCB, 3D part, artwork and source code.

## Table of Contents
1. [Features Summary](#features-summary)
2. [Presentation of the Hardware](#presentation-of-the-hardware)
3. [Presentation of the Software](#presentation-of-the-software)
4. [How to Build](#how-to-build)
5. [Operations Overview](#operations-overview)
6. [Recovery Mode](#recovery-mode)
7. [Modbus Register Map](#modbus-register-map)
   - [Coil Registers](#coil-registers)
   - [Input Registers](#input-registers)
   - [Holding Registers](#holding-registers)
8. [Configuring the Device](#configuring-the-device)
9. [Using the Watchdog](#using-the-watchdog)
10. [Troubleshooting](#troubleshooting)
11. [FMEA – Modbus Relay Board for CNC Safety](#fmea--modbus-relay-board-for-cnc-safety)

---

## Features summary ##

1. Standard DIN Rail mountable PCB
2. 3 relays - 9.4A 250VAC per output
3. Operational integrity minded
   * Uses a safety relay with force conduits and read back
   * Infeed voltage measurement with acceptable range
   * Communication watchdog
4. EStop management
   * Used for failsafe of the relay operation
      * Relay failure
      * Infeed voltage out-of-range
      * Communication loss
   * Externally controlled
     * Pulsed EStop
     * Resetable
     * Terminal
6. Operational statistics
   * Number of cycles
   * Running time
   * Fault codes

## Presentation of the hardware ##

<div style="text-align: center; position: relative;">
  <img src="https://github.com/user-attachments/assets/c7a2c55f-4833-4e39-9875-c24443134138" height="500" style="display: inline-block;">
  <img src="https://github.com/user-attachments/assets/fd56e418-ea18-465f-a665-b4eb595fe600" height="500" style="display: inline-block;">
</div>

At the core of the relay is an AVR Tiny3227, an automotive grade MPU designed for harsh environment.
The MPU is clocked using the internal calibrated RC clock at 20MHz.

The relays are SISF brand used in safety critical applications.
They feature a forced conduit (so all contacts are driven by the same bar) and a dedicated read-back contact.
This allow validating the correct relay operations.
The relay are driven by small MOSFET. The read back features a contact scrubbing circuit as recommended by the manufacturer.

The modbus interface uses a RS485 driver LTC1785A (which features short protection) and transient suppressors.

The relay output features MOV suppressors for inductive loads.

The PCB can be placed in a DIN Rail PCB mount. 4 mounting screws can also be used.
A 3D cover is available to cover the whole PCBs. Together with the DIN rail mount assembly, the module should comply with IP2X.

### CAD files ###

The following files are available:
 * The [schematic](hw/modbus_relay.kicad.sch) edited with *KiCAD 9.1*
 * The [PCB](hw/modbus_relay.kicad.pcb) edited in *KiCAD 9.1*
 * The [BOM](hw/BOM.md) edited in *KiCAD 9.1*
 * The [Front plate artwork](hw/front_plate_graphics.svg) edited in LibreOffice Draw
 * [The front plate STL file](cad/Front%20Plate.3mf)
 * [LED holder STL file](cad/leds_spacer.STL)
 * [Spacer columns STL file](cad/3mm%2016mm.STL)
 * [Flex swtich membrane](cad/Flex%20switch%20membrane.STL)
 * [Flex retaining holder](cad/Flex%20Holder.STL)

#### Printing the files

 * The case was printed in mate green PLA with PLA support material for the AVR port.
 * The flexible switch was printed in TPU.
 * The holder in black PLA.

#### Printing the front plate

 * The front was printed on an adhesive vinyl sticker at max resolution then cut to size.

> [!Note]
> You may need to adjust the dimension to account for non-linearity of the printer.

## Presentation of the software ##
The software is build on top of a small framework revolving around a simple reactor pattern.
The reactor allow for an arbitrary function to be notified from any context, and will execute when the CPU become available.
A simple priority system allow for reactor functions to be called in priority (to process data in a register).

A dedicated reactor pin allows measuring the worse compute time for any reactor function, which in turn, determine the worse
case latency. This allow garanteeing a real time operation throughout.
As such, the system does handle all asynchronous events in realtime with no delay, including all Modbus transaction, even at 115200 Bauds.
The jitter on the RS485 is almost null, and all replies are instantanous (4ms delay to allow for proper end of frame detection).

A python scripts is used to generates the final source for the modbus functions.

> [!TIP]
> This project could be reused for other Modbus devices. A modbus interface generator is provided that makes adding modbus commands simple.

## Application software
The application software is written in C++23 as it uses meta programming, constant expressions and concepts for efficient code.<br/>
A <i>docker</i> file is provided to create a build environment.</br>
The code fits the 32Kb flash space with plenty spare.

## Hardware
The schematic and PCB have been edited in KiCad 9 and are supplied too.

## How to build ##
A docker container file is provided to recreated a full build environment.<br/>
The application can also be built in Microchip studio.<br/>
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

# Operations overview
The Modbus Relay Device is a configurable relay controller with Modbus RTU communication.<br/>
This section describes how to configure and operate the device, including its default settings, configuration mode, and reset process.

## Recovery mode

When a relay cannot be reached or is un-responsive, the following procedure can be used.<br/>

1. Push the relay push button for > 5s
   * The Alert LED Flashes fast
   * The communication values have been temporary reset to:

| Configuration     | value             | Explanation                                    |
|-------------------|-------------------|------------------------------------------------|
| **Slave ID**      | `44`              | The device address is 44 (decimal) by default  |
| **Baud rate**     | `9600`            | The device talks at 9600 by default            |
| **Serial setup**  | `8N1`             | 8bits, no parity and 1 stop bit                |

2. Start the 'Relay Guarian' application, and select 'Recovery' from the menu.
3. Configure the device as required
4. Apply the new configuration and reset the device

> [!NOTE]
> When activating the recovery mode, all relay operations are maintained.

# Operation

The relay works like any modbus coil by writting the coil registers.
Addtionally, this relay has operational controls such as checking the infeed voltage, the communication bus and the health of the relays.
It will open the EStop relay is any fail (depending on its configuration).
When the EStop is set, the relay does the following:
1. It opens the EStop relay
2. It lights the fault LED if the failure cannot be recovered
   * This is the case if a relay becomes faulty
3. It blinks the fault LED if the EStop can be reset
   * A push on the switch resets the EStop, unless the triggering condition has not be resolved
4. It records the fault
5. It flashes the corresponding LED to pin point the problem
  * If a relay is at fault, the corresponding relay flashes
  * If the infeed supply is at fault, it will flash. Note, the LED will flash (Long on short off) it a voltage is detected.
  * The modbus Rx LED will flash to indicate the EStop was triggered remotely

## Simple relay operation

In normal mode:
- The device responds only to frames addressed to its **Device ID**.
- The bus operates as configured
- Relays operate based on the configured default positions and command inversion settings.
- Regular coil commands do not respond in configuration mode to prevent mode mixing.

# Modbus Register Map

## Coil registers

The relay coils can be accessed from 0 (Relay 1) upto relay 31.
Coils can be written and read at will.

The device supports the following function code:
 * **01**: [Read coils](https://www.modbustools.com/modbus.html#function01)
<br/>Read from 1 to 32 contiguous status of coils.
<br/>Coils in the response message are packed as one per bit of a byte, 1=On and 0=Off.
<br/>If the requested quantity of coils are not a multiple of 8, zeros are padded in the final byte.
 * **05**: [Write single coil](https://www.modbustools.com/modbus.html#function05)
<br/>Write a single output to either On (1) or Off
 * **15**: [Write multiple coils](https://www.modbustools.com/modbus.html#function15)
<br/>Force each coil in a sequence of coils to either On or Off

> [!WARNING]
> When reading a relay with inverted command, the value is inverted too

> [!WARNING]
> The relays may respond differently based on the relay configuration such as polarity and deboucing.

## Input registers

The registers have been grouped so they can easily be accessed.
Reserved values reads as 0.

Only the function **04**: [Read input registers](https://www.modbustools.com/modbus.html#function04) is supported to read registers.

> [!WARNING]
> Make sure to issue function 04: Read input registers when reading input registers and not function ~~**03: Read Holding Regsiters**~~, as the memory of both types overlaps.

### Device Identification

This registers provides details about the device in use.

| Modbus Address | Hex value | Access | Description         | Value(s)                     |
|----------------|-----------|--------|:-------------------:|------------------------------|
| 30001          | 0x0000    | R      | Product ID          | MSB=Device Identification code <sup>1</sup><br/>LSB=Number of relays <sup>2</sup> |
| 30002          | 0x0001    | R      | HW version          | MSB=minor, LSB=major         |
| 30003          | 0x0002    | R      | SW version          | MSB=minor, LSB=major         |
| 30004          | 0x0003    | R      | Number of relays    | UINT16<br/>1-32              |
| 30005          | 0x0004    | R      | Number of relay banks | UINT16<br/>1-4             |
| 30006–30008    | 0x0005–0x0007 | R  | *Reserved*          |                              |

* <sup>1</sup> EStop relay code=0x37. Simple modbus code=0x36.
* <sup>2</sup> A 3 relay device would be 0x3703. A 32 relays device would be 0x3720.

### Status & Monitoring

| Modbus Address | Hex value | Access | Description                 | Values                        |
|----------------|-----------|--------|-----------------------------|-------------------------------|
| 30009          | 0x0008    | R      | Current status              | 0=Device operational<br/>1=Device in EStop. Reset possible<br/>2=Device in terminal EStop |
| 30010 (+1)     | 0x0009 (+1)| R     | Running minutes             | UINT32<br/>0-2<sup>32</sup>-1 |
| 30012          | 0x000B    | R      | Current infeed voltage AC   | 1/10 volts<br/>0-3000         |
| 30013          | 0x000C    | R      | Current infeed voltage DC   | 1/10 volts<br/>0-3000         |
| 30014          | 0x000D    | R      | EStop root cause            | 0=Normal operation<br/>1=faulty relay<br/>2=Modbus watchdog<br/>3=Voltage monitor<br/>4=Command |
| 30015          | 0x000E    | R      | Given command diagnostic code | Diagnostic code given with EStop command |
| 30016          | 0x000F    | R      | Infeed minimum voltage      | Infeed voltage in 1/10th of volts or 0 |
| 30017          | 0x0010    | R      | Infeed maximum voltage      | Infeed voltage in 1/10th of volts or 0 |
| 30018–30024    | 0x0011–0x0017 | —  | *Reserved*                  |                               |

### Relay Diagnostics & Stats

> [!NOTE]
> The registers are organised by banks of upto 8 relays.
> If variants of the board are created with more than 8 relays, simply add another table of 24 registers.
> This address scheme allow addressing up to 32 relays.

| Modbus Address | Hex value | Access | Description                | Values |
|----------------|-----------|--------|:--------------------------:|--------|
| 30025          | 0x0018    | R      | Relay 1 diagnostic         | 0=OK<br/>1=Faulty |
| 30026          | 0x0019    | R      | Relay 2 diagnostic         | 0=OK<br/>1=Faulty |
| 30027          | 0x001A    | R      | Relay 3 diagnostic         | 0=OK<br/>1=Faulty |
| 30028          | 0x001B    | R      | Relay 4 diagnostic         | 0=OK<br/>1=Faulty |
| 30029          | 0x001C    | R      | Relay 5 diagnostic         | 0=OK<br/>1=Faulty |
| 30030          | 0x001D    | R      | Relay 6 diagnostic         | 0=OK<br/>1=Faulty |
| 30031          | 0x001E    | R      | Relay 7 diagnostic         | 0=OK<br/>1=Faulty |
| 30032          | 0x001F    | R      | Relay 8 diagnostic         | 0=OK<br/>1=Faulty |
| 30033 (+1)     | 0x0020 (+1)| R     | Relay 1 number of cycles   | UINT32<br/>0-2<sup>32</sup>-1 |
| 30035 (+1)     | 0x0022 (+1)| R     | Relay 2 number of cycles   | UINT32<br/>0-2<sup>32</sup>-1 |
| 30037 (+1)     | 0x0024 (+1)| R     | Relay 3 number of cycles   | UINT32<br/>0-2<sup>32</sup>-1 |
| 30039 (+1)     | 0x0026 (+1)| R     | Relay 4 number of cycles   | UINT32<br/>0-2<sup>32</sup>-1 |
| 30041 (+1)     | 0x0028 (+1)| R     | Relay 5 number of cycles   | UINT32<br/>0-2<sup>32</sup>-1 |
| 30043 (+1)     | 0x002A (+1)| R     | Relay 6 number of cycles   | UINT32<br/>0-2<sup>32</sup>-1 |
| 30045 (+1)     | 0x002C (+1)| R     | Relay 7 number of cycles   | UINT32<br/>0-2<sup>32</sup>-1 |
| 30047 (+1)     | 0x002E (+1)| R     | Relay 8 number of cycles   | UINT32<br/>0-2<sup>32</sup>-1 |

> [!NOTE]
> Attempts to read a non-available relay will result in a data error.

> [!TIP]
> For generic software, the number of available relays and banks can be read in input registers 30004 and 30005.

## Holding registers

The holding registers can be read all together. Reserved value reads as 0.

> [!WARNING]
> Reserved values cannot be written and will generate an error.

### Supported function codes

The following function codes are supported:

* **03** - [Read Holding Registers](https://www.modbustools.com/modbus.html#function03)
<br/>Reads the values of one or more holding registers.
<br/>Commonly used to retrieve configuration or control values stored in the device.
* **06** - [Write Single Register](https://www.modbustools.com/modbus.html#function06)
<br/>Writes a single value to a specific holding register.
<br/>Used for updating configuration or control parameters.
* **16** - [Write Multiple Registers](https://www.modbustools.com/modbus.html#function16)
<br/>Writes a whole configuration group at once. See the sub-chapter for details

> [!WARNING]
> Do no issue the function 04: Read input registers when reading holding registers as the memory addresses of both types overlaps.

Other functions codes such as 23 ( Read/Write Multiple Registers) are not supported.

### Communication Settings

This group of registers allow configuring the communication settings of the relay.

| Modbus Address | Hex Value | Access | Description        | Factory<br/>Recovery | Values |
|----------------|-----------|--------|--------------------|---------------|--------|
| 40001          | 0x0000    | RW     | Device address      | 44            | [1-127] |
| 40002          | 0x0001    | RW     | Baud rate selection | 5             | 0=300<br/>1=600<br/>2=1200<br/>3=2400<br/>4=4800<br/>5=9600<br/>6=19200<br/>7=38400<br/>8=57600<br/>9=115200 |
| 40003          | 0x0002    | RW     | Parity              | 0             | 0=None<br/>1=Odd<br/>2=Even |
| 40004          | 0x0003    | RW     | Stopbits            | 1             | 1=1 Stop bit<br/>2=2 stop bits |
| 40005–40006    | 0x0004–0x0005 | R  | *Reserved*          |               |        |

> [!NOTE]
> Function ~~**06: Write single register**~~ is not availble for this group.
> You must use the command **16 - Write Multiple Registers**, writting registers 40001 to 40004 at once.

> [!WARNING]
> Once the command is acknowledged, the relay will immediatly start using the new settings.

### Power Infeed Configuration

| Modbus Address | Hex Value | Access | Description         | Factory Value | Values |
|----------------|-----------|--------|---------------------|---------------|--------|
| 40009          | 0x0008    | RW     | Ingress type        | 1             | 0=DC<br/>1=AC 50Hz<br/>2=AC 60Hz |
| 40010          | 0x0009    | RW     | Ingress Min voltage | 10            | 1/10 volts [100-3000] |
| 40011          | 0x000A    | RW     | Ingress Max voltage | 300           | 1/10 volts [100-3000] |
| 40012–40016    | 0x000B–0x000F | R  | *Reserved*          |               |        |

This group can be written with the command **16**, only by writting register 40009 to 40011 in 1 command.

Other combinations will return an error.

### Safety Logic Configuration

| Modbus Address | Hex Value | Access | Description                                | Factory Value | Values |
|----------------|-----------|--------|-------------------------------------------|---------------|--------|
| 40017          | 0x0010    | RW     | EStop on undervoltage                     | 1             | 0=no<br/>1=yes |
| 40018          | 0x0011    | RW     | EStop on overvoltage                      | 1             | 0=no<br/>1=yes |
| 40019          | 0x0012    | RW     | EStop on number of seconds without activity | 0             | 0=off<br/>[1-65535] Number of seconds |
| 40020–40024    | 0x0013–0x0017 | R  | *Reserved*                                |               |        |

This group can be written with the command **16**, only by writting register 40017 to 40019 in 1 command.

Other combinations will return an error.

### Relay Configuration

This group allow configuring the individual relays.
The value written is RCFG described below.

| Modbus Address | Hex Value | Access | Description         | Values |
|----------------|-----------|--------|---------------------|--------|
| 40025          | 0x0018    | RW     | Relay 1 config      | RCFG   |
| 40025 + n      | 0x0018 + n| RW     | Relay 1 + n config  | RCFG   |
| 40056          | 0x0038    | RW     | Relay 32 config     | RCFG   |

> [!WARNING]
> Read or writing a non-supported relay will generate an error.

This group can be written with the command **16**, only by writting register 40025 to the register of the last relay.
So, for a 3 relays devices, you must write registers 40025, 40026 and 40027 (3 registers) in the same transaction.

Any other combinations will return an error.

#### RCFG / Relay configuration values

The following configuration is available. By default, all features are off. These changes are all permanent.

| Bit position | Function | Explanation |
|--------------|----------|-------------|
| 0 (lsb)      | Disable  | A '1' disable the relay. It can no longer be used and will be in opened state irrespective of the default and invert settings |
| 1            | Default position | Sets default the coil value on power-up. Reading the coil value right after powerup will return this value. <sup>note</sup> |
| 2            | Inversion setting | If '1', invert the coils polarity such that witing a '1' will open the relay |
| 8-15 (MSB)   | Debounce time (s) | Number of debouce seconds in 1/10th second (0=no debounce, 1=0.1s debound, 255=25.5 seconds debounce. The relay can change state more often than <values> seconds. If a rapid succession of command are sent, the relay will remain in a given state for the given duration in 1/10th of secoonds, the apply the latest received setting. This prevents fast switch overs for inductive load, and could protect the circuit |

<sup>note</sup>: When both "default position" and "inversion" are set to 1, the relay is physically OFF at power-up. See the logic table in the following paragraph.

#### Combined settings table

The following table illustrates the effect of the invert and default settings:

| Inversion	|Default Position	| Physical State at Power-up|
|:---------:|:---------------:|---------------------------|
| 0         |0	               |  OFF = **Opened**   |
| 0         |**1**            |  ON  = **Closed**   |
| **1**     |0                |	OFF = **Closed**   |
| **1**     |**1**            |	ON  = **Opened**   |

---

> [!NOTE]
> 1. All relay diagnostics and counters are grouped in blocks of 8 for efficient Modbus access.
> 2. To add more relays, simply add another bank of 8 at the next available block.
> 3. Control registers are write-only and can be mapped to holding registers or coils as appropriate for your Modbus implementation.

### Device control register

This group of register allow controlling the EStop and the relay.

> [!NOTE]
> These registers are write only. The function ~~**03: Read multiple registers**~~ is not availble for this group.
> The register must be written individually. Function ~~**16 - Write Multiple Registers**~~ is not available.

| Modbus Address | Hex Value | Access | Description         | Values |
|----------------|-----------|--------|---------------------|--------|
| 40101          | 0x0064    | W      | Trigger the EStop   | ESTOP_CTRL<br/>See note <sup>1</sup>|
| 40102          | 0x0065    | W      | Reset to factory default and reboot  | 0xAA55 |
| 40103          | 0x0066    | W      | Reset the device    | 0xAA55  |

**Note <sup>1</sup>** : See the format below

#### ESTOP_CTRL : ESTop control ####

The following table documents the values to use to control the EStop.

| Byte | Function | Values |
|--------------|----------|-------------|
| MSB  | Type of EStop | 0x00 : Reset the EStop if possible. Returns an error if the ESTop could not be reset<br/>0x11 : Pulsed EStop. Create a 1 second EStop pulse<br/>0x22 : Resetable EStop mode. The EStop can be reset by pushing the EStop reset button<br/>0xFF : Terminal EStop. Only a device reset will clear the ESTop (Register 40103) |
| LSB          | Diagnostic code | A code can be applied to allow investigating the cause of EStop/Reset.<br/>Values: <0-255> |

## Configuring the device
The factory default communication settings for the relay are:
- Device address is 44
- Baud rate is **9600**
- **8 data bits**
- **No parity**
- **1 stop bit** (9600 8N1).

### Using the watchdog
The relay has a command watchdog which will release the relays following a period of inactivity.
Everytime this devices receives a valid modbus command (including holding register read etc.), the watchdog is reset.
This feature allow for the relays to be atomatically releases if the master was to fail.
The period is given in seconds.
A value of zero (default) turns off the feature.

---

## Troubleshooting
### Cannot Communicate with Device
- If the modbus LEDs are flickering, make sure the Relay Guarian and relay share the same settings.
- Active the recovery mode on the device and in Guardian Relay
- If no communication is established, check the serial adapter communication port
- Check the wriring. Modbus pins cannot be swapped.

### Lost Device ID
1. Send a valid broadcast frame to enter configuration mode.
2. Use the default Device ID (`0`) to reset the device via the reset register.

### Relay Does Not Respond
- Verify relay default positions and inversion settings in the holding registers.
- Check the watchdog timeout setting to ensure it isn’t triggering prematurely.

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

