# Modbus Relay #
![modbus_rtu](https://github.com/user-attachments/assets/516eb8d2-8e22-4c80-9cc7-a677c1ba3664)

This project is a fully working MODBUS RTU triple Relay, aimed at a CNC or equivalent.
Beside the relay switching functions it also includes the control of the EStop on CNC.

The project comes complete with schematic, PCB, 3D part, artwork and source code.

**Features:**
1. Standard DIN Rail mountable PCB size
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

** Presentation of the hardware **
At the core of the relay is an AVR Tiny3227, an automotive grade MPU designed for harsh environment.
The relays are SISF brand used in safety critical applications.
They feature a forced conduit (so all contacts are driven by the same bar) and a dedicated read-back contact.
This allow to valdate the correct relay operation.
The modbus interface used a RS485 driver LTC1785 or equivalent.
The PCB can be placed in a DIN Rail PCB mount. 4 mounting screws can also be used.
A 3D cover is available to remove access to the contacts.

The schematic and PCB have been edited in KiCAD9.

** Presentation of the software **
The software is build on top of a small framework revolving around a simple reactor pattern.
The reactor allow for an arbitrary function to be notified from any context, and will execute when the CPU become available.
A simple priority system allow for reactor functions to be called in priority (to process data in a register).
The over computing time of any reactor function is measured, so the worse case latency is known.
The system does handle all asynchronous events in realtime with no delay.
The jitter on the RS485 is null, and all replies are instantanous (4ms delay to allow for proper end of frame detection).

The modbus registers are described in this document.
A python scripts generates the final source for the modbus funtions.
This project can be reused for other Modbus devices. A modbus interface generator is provided that makes adding modbus commands simple.

<img src="https://github.com/user-attachments/assets/c7a2c55f-4833-4e39-9875-c24443134138" width="500">
View of the PCB in the DIN Rail case

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

## Factory default settings
The relay comes pre-configured with the following value:

| Configuration     | Default value             | Explanation                                    |
|-------------------|---------------------------|------------------------------------------------|
| **Slave ID**      | `44`                      | The device address is 44 (decimal) by default  |
| **Baud rate**     | `9600`                    | The device talks at 9600 by default            |
| **Serial setup**  | `8N1`                     | 8bits, no parity and 1 stop bit                |

## Resetting the relay to factory default setttings

If the relay is un-responsive, it may have been mis-configured.
The relay can be reset to its factory default using the following procedure:<br/>
1. Push the switch for > 5s
   * All LEDs will light for 2s to indicate the relay has reset
   * The configuration values have been reset to the factory default
2. Connect QModbusMaster and try to connect to the device at 44 using serial port at 9600 8N1.
3. Configure the device as required
4. Apply the new configuration and reset the device

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

## Device Identification

| Zero-Based | Modbus Address | Access | Description |
|------------|----------------|--------|-------------|
| 0 | 40001 | R | Product ID |
| 1 | 40002 | R | HW version |
| 2 | 40003 | R | SW version |

## Communication Settings

| Zero-Based | Modbus Address | Access | Description | Factory Value | Values |
|------------|----------------|--------|-------------|---------------|--------|
| 100 | 40101 | RW | Device address | 44 | [1-127] |
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

| Zero-Based | Modbus Address | Access | Description | Values |
|------------|----------------|--------|-------------|--------|
| 400 | 40401 | R | Current status | 0=Device operational<br/>1=Device in EStop. Reset possible<br/>Device in terminal EStop |
| 401 | 40402–40403 | R | Running minutes | 32-bit unsigned int (High word at 40402, Low word at 40403) |
| 403 | 40404 | R | Infeed DC voltage | 1/10 volts |
| 404 | 40405 | R | Infeed AC voltage | 1/10 volts |
| 405 | 40406 | R | EStop root cause | 0=Normal operation</br>1=faulty relay</br>2=modbus watchdog</br>3=voltage monitor</br>4=external |
| 406 | 40407 | R | External diagnostic code | Diagnostic code given with EStop command or 0xffff |
| 407 | 40408 | R | Infeed out-of-range voltage | Invalid infeed voltage in 1/10th of volts or 0 |
| 408 | 40409 | R | Relay 1 diagnostic code | 0=Relay is OK<br/>1=Faulty relay |
| 409 | 40410 | R | Relay 2 diagnostic code | 0=Relay is OK<br/>1=Faulty relay |
| 410 | 40411 | R | Relay 3 diagnostic code | 0=Relay is OK<br/>1=Faulty relay |

## Relay Stats

| Zero-Based | Modbus Address | Access | Description |
|------------|----------------|--------|-------------|
| 500 | 40501-40502 | R | Relay 1 number of cycles |
| 502 | 40503-40504 | R | Relay 2 number of cycles |
| 504 | 40503-40504 | R | Relay 3 number of cycles |

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

### Using the watchdog
The relay has a command watchdog which will release the relays following a period of inactivity.
Everytime this devices receives a valid modbus command (including holding register read etc.), the watchdog is reset.
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

