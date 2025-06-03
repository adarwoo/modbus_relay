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

## Recovery mode

When a relay cannot be reached or is un-responsive, the following procedure can be used.<br/>

1. Push the relay push button for > 5s
   * The Alert LED Flashes fast
   * The communication values have been temporary reset to:

| Configuration     | value             | Explanation                                    |
|-------------------|-------------------|------------------------------------------------|
| **Slave ID**      | `44`              | The device address is 44 (decimal) by default  |
| **Baud rate**     | `9600`            | The device talks at 9600 by default            |
| **Serial setup**  | `8N1`                     | 8bits, no parity and 1 stop bit                |

2. Start the 'Relay Guarian' application, and select 'Recovery' from the menu.
3. Configure the device as required
4. Apply the new configuration and reset the device

**Note**: When activating the recovery mode, all relay operations are maintained.

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
**Note**: The relay may respond differently based on the relay configuration such as
polarity and deboucing.

## Input registers

The registers have been grouped so they can easily be accessed.

### Device Identification

| Zero-Based | Modbus Address | Access | Description | Value(s) |
|------------|---------------|--------|--------------|----------|
| 0          | 30001         | R      | Product ID   | 0x3701*  |
| 1          | 30002         | R      | HW version   | MSB=minor, LSB=major |
| 2          | 30003         | R      | SW version   | MSB=minor, LSB=major |
| 3–7        | 30004–30008   | —      | *Reserved*   |          |

The EStop relay is 0x3708. The simple modbus relay is 0x3701.

### Status & Monitoring

| Zero-Based | Modbus Address | Access | Description                 | Values |
|------------|---------------|--------|-----------------------------|--------|
| 8          | 30008         | R      | Current status              | 0=Device operational<br/>1=Device in EStop. Reset possible<br/>2=Device in terminal EStop |
| 9-10       | 30010–40011   | R      | Running minutes (32-bit)    | High word at 40010, Low word at 40011 |
| 11         | 30012         | R      | Current infeed voltage      | 1/10 volts |
| 12         | 30013         | R      | EStop root cause            | 0=Normal operation<br/>1=faulty relay<br/>2=modbus watchdog<br/>3=voltage monitor<br/>4=command |
| 13         | 30014         | R      | Given command diagnostic code | Diagnostic code given with EStop command |
| 14         | 30015         | R      | Infeed minimum voltage      | Infeed voltage in 1/10th of volts or 0 |
| 15         | 30016         | R      | Infeed maximum voltage      | Infeed voltage in 1/10th of volts or 0 |
| 16-23      | 30017–30042   | —      | *Reserved*                  |        |

### Relay Diagnostics & Stats

*Note:* The registers are organised by banks of upto 8 relays.
If variants of the board are created with more than 8 relays, simply add another
table of 24 registers.
This address scheme allow addressing up to 32 relays.

| Zero-Based | Modbus Address | Access | Description                | Values |
|------------|----------------|--------|----------------------------|--------|
| 24         | 30025          | R      | Relay 1 diagnostic         | 0=OK<br/>1=Faulty |
| 25         | 30026          | R      | Relay 2 diagnostic         | 0=OK<br/>1=Faulty |
| 26         | 30027          | R      | Relay 3 diagnostic         | 0=OK<br/>1=Faulty |
| 27         | 30028          | R      | Relay 4 diagnostic         | 0=OK<br/>1=Faulty |
| 28         | 30029          | R      | Relay 5 diagnostic         | 0=OK<br/>1=Faulty |
| 29         | 30030          | R      | Relay 6 diagnostic         | 0=OK<br/>1=Faulty |
| 30         | 30031          | R      | Relay 7 diagnostic         | 0=OK<br/>1=Faulty |
| 31         | 30032          | R      | Relay 8 diagnostic         | 0=OK<br/>1=Faulty |
| 32-33      | 30033–30034    | R      | Relay 1 number of cycles (32-bit) |      |
| 34-35      | 30035–30036    | R      | Relay 2 number of cycles (32-bit) |      |
| 36-37      | 30037–30038    | R      | Relay 3 number of cycles (32-bit) |      |
| 38-39      | 30039–30040    | R      | Relay 4 number of cycles (32-bit) |      |
| 40-41      | 30041–30042    | R      | Relay 5 number of cycles (32-bit) |      |
| 42-43      | 30043–30044    | R      | Relay 6 number of cycles (32-bit) |      |
| 44-45      | 30045–30046    | R      | Relay 7 number of cycles (32-bit) |      |
| 46-47      | 30047–30048    | R      | Relay 8 number of cycles (32-bit) |      |

## Holding registers

The holding registers can be read all together. Reserved value reads as 0.

### Communication Settings

| Zero-Based | Modbus Address | Access | Description       | Factory Value | Values |
|------------|---------------|--------|--------------------|---------------|--------|
| 0         | 40011         | RW     | Device address      | 44            | [1-127] |
| 1         | 40012         | RW     | Baud rate selection | 96            | 3=300 Baud<br/>6=600 Baud<br/>12=1200 Baud<br/>24=2400 Baud<br/>48=4800 Baud<br/>96=9600 Baud<br/>192=19200 Baud<br/>364=36400 Baud<br/>576=57600 Baud<br/>1152=115200 Baud |
| 2         | 40013         | RW     | Parity              | 0             | 0=None<br/>1=Odd<br/>2=Even |
| 3         | 40014         | RW     | Stopbits            | 1             | 1=1 Stop bit<br/>2=2 stop bits |
| 4–7       | 40015–40020   | —      | *Reserved*          |               |        |

### Power Infeed Configuration

| Zero-Based | Modbus Address | Access | Description         | Factory Value | Values |
|------------|---------------|--------|---------------------|---------------|--------|
| 8          | 40021         | RW     | Ingress type        | 1             | 0=DC<br/>1=AC 50Hz<br/>2=AC 60Hz |
| 9          | 40022         | RW     | Ingress Min voltage | 10            | 1/10 volts [100-3000] |
| 10         | 40023         | RW     | Ingress Max voltage | 300           | 1/10 volts [100-3000] |
| 11-15      | 40024–40030   | —      | *Reserved*          |               |        |

### Safety Logic Configuration

| Zero-Based | Modbus Address | Access | Description                                 | Factory Value | Values |
|------------|---------------|--------|---------------------------------------------|---------------|--------|
| 16         | 40031         | RW     | EStop on undervoltage                       | 1             | 0=no<br/>1=yes |
| 17         | 40032         | RW     | EStop on overvoltage                        | 1             | 0=no<br/>1=yes |
| 18         | 40033         | RW     | EStop on number of seconds without activity | 0             | 0=off<br/>[1-65535] Number of seconds |
| 19-23      | 40034–40040   | —      | *Reserved*                                  |               |        |

---

### Relay Configuration (Bank 0: Relays 1–8)

| Zero-Based | Modbus Address | Access | Description         | Factory Value | Values |
|------------|---------------|--------|---------------------|---------------|--------|
| 24        | 40101         | RW     | Relay 1 config      | 0           |     |
| 25        | 40102         | RW     | Relay 2 config      | 0           | ...    |
| 26        | 40103         | RW     | Relay 3 config      | 0           | ...    |
| 27        | 40104         | RW     | Relay 4 config      | 0           | ...    |
| 28        | 40105         | RW     | Relay 5 config      | ...           | ...    |
| 29        | 40106         | RW     | Relay 6 config      | ...           | ...    |
| 30        | 40107         | RW     | Relay 7 config      | ...           | ...    |
| 31        | 40108         | RW     | Relay 8 config      | ...           | ...    |

**Next Bank (Relays 32–): 40121–40128**
(Repeat the above pattern for additional banks.)

#### Relay configuration values

The following configuration is available. By default, all features are off. These changes are all permanent.

| Bit position | Function | Explanation |
|--------------|----------|-------------|
| 0 (lsb)      | Disable  | A '1' disable the relay. It can no longer be used and will be in opened state irrespective of the default and invert settings |
| 1            | Default position | Sets default the coil value on power-up. Reading the coil value right after powerup will return this value. [^invnote] |
| 2            | Inversion setting | If '1', invert the coils polarity such that witing a '1' will open the relay |
| 8-15 (MSB)   | Debounce time (s) | Number of debouce seconds in 1/10th second (0=no debounce, 1=0.1s debound, 255=25.5 seconds debounce. The relay can change state more often than <values> seconds. If a rapid succession of command are sent, the relay will remain in a given state for the given duration in 1/10th of secoonds, the apply the latest received setting. This prevents fast switch overs for inductive load, and could protect the circuit |

[^invnote]: When both "default position" and "inversion" are set to 1, the relay is physically OFF at power-up. See the logic table in the following paragraph.

#### Combined settings table

The following table illustrates the effect of the invert and default settings:

|Default Position	| Inversion	| Physical State at Power-up|
|-----------------|-----------|---------------------------|
|0	               | 0	      |  Relay OFF                |
|1	               | 0	      |  Relay ON                 |
|0                | 1         |	Relay ON                 |
|1                | 1          |	Relay OFF                |

---

**Notes:**
- All relay diagnostics and counters are grouped in blocks of 8 for efficient Modbus access.
- To add more relays, simply add another bank of 8 at the next available block.
- Control registers are write-only and can be mapped to holding registers or coils as appropriate for your Modbus implementation.

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

