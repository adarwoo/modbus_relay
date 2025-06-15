# Modbus NRelay Controller with EStop <img src="https://github.com/user-attachments/assets/516eb8d2-8e22-4c80-9cc7-a677c1ba3664" height="30"> #

This project features is a MODBUS RTU relays controller, which includes failsafe mode and an EStop.
It is aimed for industrial systems such as a CNC or equivalent.

---

This document provides a description of the operations of the device.

## Device ID

This document covers the series mbNR_37, which includes the mbNR_37/03

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

# Operations overview
The Modbus Relay Device is a configurable relay controller with Modbus RTU communication.<br/>
This section describes how to configure and operate the device, including its default settings, configuration mode, and reset process.

## Simple operation
The relay works like any modbus coil by writting the coil registers.
The coils can be written individually using command **05**: [Write single coil](https://www.modbustools.com/modbus.html#function05) or by bunch using command **15**: [Write multiple coils](https://www.modbustools.com/modbus.html#function01)

The status of the relay can be read back with command **01**: [Read coil](https://www.modbustools.com/modbus.html#function01)

The relay polarity can be inverted, and the default relay position can be set in the configuration.

## System Health Monitoring

This relay distinshes itself from simpler version by its built-in ability to monitor the health of the system it operates within, and to halt operations by opening an EStop relay.

The active elements being monitored are:
1. The modbus communication from the master
2. The infeed voltage (upsteam supply voltage)
3. The health of the relays
4. As instructed from the modbus master

### Modbus communication monitoring

The device can monitor for correct activity on the Modbus network, triggering a resetable EStop if the master stops issuing commands to the device.

> [!NOTE]
> The bus is considered active if commands addressed to the device are received at least once in the configured watchdog duration.
> For good operation, it is recommended to read the register 30009 (Status) periodically, like every second.

### Infeed monitoring

The module can sense the voltage of the relay infeed and act upon a failure.
The failure ranges from:
1. Overvoltage : The infeed voltage is higher than a configured threshold
2. Undervoltage : The infeed voltage is lower than a configured threshold

When an infeed defect is detected, the device will open the EStop relay. This should stop operations.
The relays can be individually configured to go Open or Close when a infeed fault is detected.

### Relay health monitoring

All the relay are equiped with a status read back, backed mecanically (using a forced conduit). If a relay is to fail, the fault is detected and the ESTop condition is trigggered in terminal mode - meaning, it cannot be reset, beside power cycling the device.
The faulty relay can be isolated with a disable command, but it should be replaced.

The relays can be individually configured to go Open or Close when a relay fault is detected.

### External EStop

The modbus master can issue an EStop command. This external EStop condition can be:
. Pulsed EStop. The system will halt, but can be resumed right after
. Resetable EStop. The EStop condition is reset by pushing the 'EStop reset' push button.
. Terminal. Only a power cycle can clear the condition.

## Recovery mode

When a relay cannot be reached or is un-responsive, the following procedure can be used.<br/>

1. Push the relay 'EStop reset' push button for > 3s
   * The Alert LED Flashes fast
   * The communication values have been temporary reset to:

| Configuration     | value             | Explanation                                    |
|-------------------|-------------------|------------------------------------------------|
| **Slave ID**      | `44`              | The device address is 44 (decimal) by default  |
| **Baud rate**     | `9600`            | The device talks at 9600 by default            |
| **Serial setup**  | `8N1`             | 8bits, no parity and 1 stop bit                |

2. Start the 'Relay Guarian' application, and select 'Recovery' from the menu.
3. Configure the device as required
4. Apply the new configuration. This ends the recovery mode, and the device is operational with the new settings.

> [!NOTE]
> The recovery mode maintains on-going relay operations.

> [!TIP]
> The recovery mode can also be ended by pressing the 'EStop reset' push button for another 3s.
> The device then applies the programmed settings.


## EStop mode

The EStop relay circuit is opened on a EStop condiiton.
When the relay controller is in EStop, the relay status is changed according to the configuration - that is, unchanged, or switched to the configured default state.
It is no longer possible to command the relays over the modbus network. The command will create an 'slave_device_failure' error.
When the EStop condition is reset (automatic, push on the reset button, or power cycle), operations resumes to normal.

In all cases, the EStop condition can be read over the modbus network.

## LEDs

The module features many LED to see the device operation and faults easilty.
> [!TIP]
> During boot, for the first 2seconds, all LEDs are lit. This allow checking for a faulty LED.

All LEDs serve multiple purspose with the exception of the modbus Tx LED which only indicates outgoing RS485 traffic. 
The expression 'All LEDs' de-fact excludes the modbus Tx LED.

To help find a relay controller, a locate command can be send. This will flash all LEDs at 10Hz for 1s, and the actual LED value for the next.

For detailed LED states, see:
- [EStop LED](#estop-led)
- [Infeed LED](#infeed-led)
- [Modbus LEDs](#modbus-leds)
- [Relay LEDs](#relay-leds)

---

### EStop LED

The fault LED state is as follow:

| State | Description |
|-------|-------------|
| Off   | Normal operations |
| On    | Device is terminated. A hard reboot is required. If the termination was caused by a failing relay, the correspond relay LED will flash at 2Hz |
| Flash at 2Hz | Fault detected. The fault can be cleared by pressing the ESTOP reset button.<br/>Another LED will synchronously blink to point to the fault.<br/><ul><li><b>INFEED LED</b>: Infeed fault, over or under</li><li><b>TX LED</b>: Modbus watchdog</li></ul>
| Flash once for 2s | A pulsed EStop condition was received from the modbus master |

When an EStop is is progress, a source LED is flashing fast to point to the source:
  * If a relay is at fault, the corresponding relay LED flashes
  * If the infeed supply is at fault, it will flash. Note, the LED will flash (Long on short off) it a voltage is detected.
  * The modbus Rx LED will flash to indicate the EStop was triggered remotely

### Infeed Led

The infeed LED provides visual information about the infeed voltage.

| State | Description |
|-------|-------------|
| Off   | Infeed voltage is < 10V (AC+DC) |
| On    | Infeed voltage detected, and within configured range |
| Flash . . | Voltage detecting, but below the configured threshold |
| Flash _ _ | Voltage detecting, but over the configured threshold |
| Flash at 2Hz | Along with the EStop. A infeed fault was detected. |

> [!CAUTION]
> The Infeed LED is for indication only. ***You must assume voltage is present at all times***.

### Modbus LEDs

The modbus LEDs show activity on the Modbus RS485 network.
* The Rx LED shows incomming traffic. It flashes fast when the device is in recovery or device locate mode.
* The Tx LED is lit during power-up boot, and show the outgoing traffic activity. It serves not other purposes.

> [!IMPORTANT]
> The Rx LED show traffic activity which may include packets not addressed to the controller

### Relay LEDS

Each relay have a dedicated LED.

The LED status is as follow:

| State | Description |
|-------|-------------|
| Off   | The relay is in the OFF state |
| On    | The relay is in the ON state |
| Flash 2Hz | A fault was detected |
| Flash . . | Relay is disabled |

> [!IMPORTANT]
> The ON state accounts for the configured polarity of the relay.

# Modbus Register Map

The Modbus registers are grouped into:
- [Coil Registers](#coil-registers)
- [Input Registers](#input-registers)
- [Holding Registers](#holding-registers)
- [Device Control Registers](#device-control-registers)

For communication settings, see [Communication Settings](#communication-settings).

---

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

### Detail of the function 05 : Write single coil

The following values can be used:

| Value | Meaning |
|-------|--------------------------------------------------------------------------------------------------------------|
| 0x0000| This specific 16-bit value is the Modbus standard representation for "OFF". |
| 0xFF00| This specific 16-bit value is the Modbus standard representation for "ON" when writing to a coil. |
| 0xAA00| This value will toggle the coil |

> [!IMPORTANT]
> The inversion setting is taken into account with writing coils.

## Input registers

Input registers are read-only registers used to report information about the controller.
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
| 30005–30008    | 0x0004–0x0007 | R  | *Reserved*          |                              |

* <sup>1</sup> EStop relay code=0x37. Simple modbus code=0x36.
* <sup>2</sup> A 3 relay device would be 0x3703. A 32 relays device would be 0x3720.

### Status & Monitoring

| Modbus Address | Hex value | Access | Description                 | Values                        |
|----------------|-----------|--------|-----------------------------|-------------------------------|
| 30009          | 0x0008    | R      | Current status              | 0=Device operational<br/>1=Device in EStop. Reset possible<br/>2=Device in terminal EStop |
| 30010 (+1)     | 0x0009 (+1)| R     | Running minutes             | UINT32<br/>0-2<sup>32</sup>-1 |
| 30012          | 0x000B    | R      | Current infeed voltage      | 1/10 volts<br/>0-3000         |
| 30013          | 0x000C    | R      | Actual infeed voltage type  | Reports the type of infeed voltage detected<br/>0=AC+DC is below 10V, 1=DC, 2=AC                    |
| 30014          | 0x000D    | R      | EStop root cause            | 0=normal. No ongoing estop<br/>1=relay. A relay fault was detected<br/>2=modbus. The communication watchdog reported a lack of communication<br/>3=voltage. An incorrect voltage or voltage type was detected<br/>4=external. A modbus command was issued.<br/><br/>Unless an EStop condition is still in progress, this register is cleared to 0 by reading the diagnostic code. |
| 30015          | 0x000E    | R      | Diagnostic code | Diagnostic code of the EStop condition<br>The content value depends on the EStop root cause:<ul><li><b>normal</b><br/>0</li><li><b>relay</b><br/>Holds the faulty relay number. The first relay number is 1.</li><li><b>modbus</b><br/>The timeout in seconds</li><li><b>voltage</b><br/>0xFFFF is the voltage type is incorrect, else the triggering voltage in 1/10V</li><li><b>external</b><br/>Contains the [EStop control](#estop-control) value</li></ul><br/><b>Note:</b> This register and the root cause are cleared when reading it unless an active EStop is in progress |
| 30016          | 0x000F    | R      | Infeed lowest voltage       | Infeed voltage in 1/10th of volts |
| 30017          | 0x0010    | R      | Infeed highest voltage      | Infeed voltage in 1/10th of volts |
| 30018–30024    | 0x0011–0x0017 | —  | *Reserved*                  |                               |

### Relay Diagnostics & Statistics

The status of each relays is accessible, and well as their indivual number of cycles.
A cycle is defined as a change of a relay state during operation (exclude powerloss transitions).
<br/>
The address space is structured to expand up to 32 relays for other variants of controllers.

| Modbus Address | Hex value    | Access | Description                | Values                           |
|----------------|--------------|--------|:--------------------------:|----------------------------------|
| 30025          | 0x0018       | R      | Relay 1 diagnostic         | 0=OK<br/>1=Faulty<br/>2=Disabled |
| 30026-30027    | 0x0019-0x001A| R      | Relay 1 number of cycles   | UINT32<br/>0-2<sup>32</sup>-1    |
| 30028          | 0x001B       | R      | Relay 2 diagnostic         | 0=OK<br/>1=Faulty<br/>2=Disabled |
| 30029-30030    | 0x001C-0x001D| R      | Relay 2 number of cycles   | UINT32<br/>0-2<sup>32</sup>-1    |
| 30031          | 0x001E       | R      | Relay 3 diagnostic         | 0=OK<br/>1=Faulty<br/>2=Disabled |
| 30032-30033    | 0x001F-0x0020| R      | Relay 3 number of cycles   | UINT32<br/>0-2<sup>32</sup>-1    |

> [!WARNING]
> Reading passed the last supported relay will generate a **illegal data address** error.

> [!TIP]
> For generic software, the number of available relays can be read in input registers 30004.

## Holding registers

Holding register contain values can be read or written. The hold configuration details.
They are grouped by functions.
It is possible to read all the holding registers with 1 command as the reserved values will read as one.
Writing these registers is restricted to prevent mis-behavious.

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
<br/>Writes a whole configuration group at once. This is only available for writting the communication settings.

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
| 40005–40008    | 0x0004–0x0007 | R  | *Reserved*          |               |        |

> [!NOTE]
> Function ~~**06: Write single register**~~ is not availble for this group.
> You must use the command **16 - Write Multiple Registers**, writting registers 40001 to 40004 at once.
> Once the command is acknowledged, the relay will immediatly start using the new settings.

### Power Infeed Configuration

| Modbus Address | Hex Value | Access | Description         | Factory Value | Values |
|----------------|-----------|--------|---------------------|---------------|--------|
| 40009          | 0x0008    | RW     | Ingress type        | 1             | 0=DC<br/>1=AC 50Hz<br/>2=AC 60Hz |
| 40010          | 0x0009    | RW     | Ingress minimum voltage threshold | 100      | 1/10 volts [100-3000] |
| 40011          | 0x000A    | RW     | Ingress maximum voltage threshold | 3000     | 1/10 volts [100-3000] |
| 40012–40016    | 0x000B–0x000F | R  | *Reserved*          |               |        |

> [!WARNING]
> The registers must be written individually. The command **16** is not supported.

### Safety Logic Configuration

| Modbus Address | Hex Value | Access | Description                               | Factory Value | Values |
|----------------|-----------|--------|-------------------------------------------|---------------|--------|
| 40017          | 0x0010    | RW     | Activate EStop on undervoltage            | 0             | 0=no<br/>1=yes |
| 40018          | 0x0011    | RW     | Activate EStop on overvoltage             | 0             | 0=no<br/>1=yes |
| 40019          | 0x0012    | RW     | Activate EStop on incorrect type of supply voltage<br>Example: AC detected with DC configured | 0 | 0=no<br/>1=yes |
| 40020          | 0x0013    | RW     | Activate EStop on number of seconds without modbus frame received | 0             | 0=off<br/>[1-65535] Number of seconds |
| 40021–40024    | 0x0014–0x0017 | R  | *Reserved*                                |               |        |

> [!WARNING]
> The registers must be written individually. The command **16** is not supported.

### Relay Configuration

This group allow configuring the individual relays.
The value written is RCFG described below.

| Modbus Address | Hex Value | Access | Description         | Factory default | Values |
|----------------|-----------|--------|---------------------|------------------|-------|
| 40025          | 0x0018    | RW     | Relay 1 config      | 0 (Enabled, Off by default, Non-inverted, Off on fault) | RCFG   |
| 40026          | 0x0019    | RW     | Relay 2 config      | 0 (Enabled, Off by default, Non-inverted, Off on fault) | RCFG   |
| 40027          | 0x001A    | RW     | Relay 3 config      | 0 (Enabled, Off by default, Non-inverted, Off on fault) | RCFG   |
| 40028-40056<sup>1</sup>    | 0x0018-0x38| RW     | Relay 4-32 config | 0 (Enabled, Off by default, Non-inverted, Off on fault) | RCFG   |

<sup>1</sup> Available on devices with more than 3 relays

> [!WARNING]
> The registers must be written individually. The command **16** is not supported.

> [!WARNING]
> Read or writing a non-supported relay will generate an error.

> [!IMPORTANT]
> A disabled relay is electrically opened for safety reasons

#### RCFG / Relay configuration values

The following configuration is available. By default, all features are off. These changes are all permanent.

| Bit position | Function | Explanation |
|--------------|----------|-------------|
| 0 (lsb)      | Disable  | A '1' disable the relay. It can no longer be used and will be in opened state irrespective of the default and invert settings |
| 1            | Default position | Sets default the coil value on power-up. Reading the coil value right after powerup will return this value. <sup>note</sup> |
| 2            | Inversion setting | If '1', invert the coils polarity such that witing a '1' will open the relay |
| 3            | Mode on fault | If '1', leave the relay as is. If '0', an attempt is made to position the relay to the configured default position |
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

## Device control registers

These register share the holding registers mapping address space but are write-only.
This group of register allows controlling the EStop and the controller device.

> [!NOTE]
> These registers are write only. The function ~~**03: Read multiple registers**~~ is not availble for this group.
> The register must be written individually. Function ~~**16 - Write Multiple Registers**~~ is not available.

| Modbus Address | Hex Value | Access | Description         | Values |
|----------------|-----------|--------|---------------------|--------|
| 40101          | 0x0064    | W      | Trigger the EStop   | ESTOP_CTRL<br/>See note <sup>1</sup>|
| 40102          | 0x0065    | W      | Zero measurements. Allow re-measuring min and max | 0xAA55  |
| 40103          | 0x0066    | W      | Locate device. Fast flash all the LEDs to locate the device | 0 = Turn off<br/>1 = Turn on |
| 40104          | 0x0067    | W      | Reset configuration to factory default and reboot  | 0xAA55 |
| 40105          | 0x0068    | W      | Reset the device    | 0xAA55  |

**Note <sup>1</sup>** : See the format below

### EStop control

The following table documents the type **ESTOP_CTRL** used to control the EStop.

| Byte | Function | Values |
|--------------|----------|-------------|
| MSB  | Type of EStop | 0x00 : Reset the EStop if possible. Returns an error if the EStop could not be reset<br/>0x11 : Pulsed EStop. Create a 4 second EStop pulse<br/>0x22 : Resetable EStop mode. The EStop can be reset by pushing the EStop reset button<br/>0xFF : Terminal EStop. Only a device reset will clear the ESTop (Register 40103) |
| LSB          | Diagnostic code | A code can be applied to allow investigating the cause of EStop/Reset.<br/>Values: <0-255> |

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
