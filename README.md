# Modbus Relay with EStop <img src="https://github.com/user-attachments/assets/516eb8d2-8e22-4c80-9cc7-a677c1ba3664" height="30"> #

## Introduction
This project features is a MODBUS RTU relays controller, which includes failsafe mode and an EStop.
It is aimed for industrial systems such as a CNC or equivalent.

The project comes complete with documentation, schematic, PCB, 3D part, artwork and source code.

For operational documentation, please check the complete [User Manual](docs/user_manual.md)

---

## Features summary

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

## Presentation of the hardware

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

### CAD files

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

For printing instructions, refer to [Printing the Files](#printing-the-files).

#### Printing the files

 * The case was printed in mate green PLA with PLA support material for the AVR port.
 * The flexible switch was printed in TPU.
 * The holder in black PLA.

#### Printing the front plate

 * The front was printed on an adhesive vinyl sticker at max resolution then cut to size.

> [!Note]
> You may need to adjust the dimension to account for non-linearity of the printer.

## Presentation of the software
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

For build instructions, see [How to Build](#how-to-build).

## Hardware
The schematic and PCB have been edited in KiCad 9 and are supplied too.

## How to build
A docker container file is provided to recreated a full build environment.<br/>
The application can also be built in Microchip studio.<br/>
You will need a Linux shell, or WSL shell in Windows.
The tool 'gitman' is required as well as Docker. (docker-ce or else).

### Steps
```bash
# Clone this repo
$ git clone https://https://github.com/adarwoo/modbus_relay.git
$ cd modbus_relay
# Optional - install gitman
$ pipx install gitman
$ gitman update
$ make NDEBUG=1 # Optional, add ARCH=attiny1624 to suit you device
```
