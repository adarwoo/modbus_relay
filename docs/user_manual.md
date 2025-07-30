# Modbus Relays Device with EStop <img src="https://github.com/user-attachments/assets/516eb8d2-8e22-4c80-9cc7-a677c1ba3664" height="30"><br/>**User manual**

## Introduction

This project features a MODBUS-RTU relays device with emergency stop, infeed measurements, monitoring and failsafe modes.
It is aimed at industrial systems such as a CNC or equivalent.

This document provides a description for the operations of the device.

### Device identification
The device identification is ARex_R3.
The hardware and the software are released together, the github TAG references this design uniquely.

### Features summary
This relay goes beyond a single Modbus relay and is packed with features:

1. Standard DIN Rail mountable PCB
2. 3 relays - up to 9.4A @ 250V per output
   * Individual filtering for ON and OFF cycles for each relay
3. Operational integrity minded
   * Uses safety relays with forcibly guided contacts, 10.10<sup>6</sup> operations, re-inforced isolation and position read back
   * Galvanically isolated infeed voltage measurement with acceptable range
   * Communication watchdog
4. Emergency Stop (EStop) management
   * Used to safe guard relay operations
      * Relay failure
      * Infeed voltage out-of-range
      * Communication loss
      * Application crash/corruption
   * Can be controlled by the Modbus server
     * Pulsed EStop
     * Resetable EStop with a push button
     * Terminal EStop for terminal cases
5. Operations statistics
   * Number of cycles
   * Running time
   * Fault codes
6. Diagnostics over Modbus
   * Infeed highs and lows
   * Infeed type and level
   * EStop causes and diagnostic

## Operations overview
Operations are performed over an RTU communication bus.<br/>
A push button is also featured with limited use:
* to reset EStops (when possible)
* to activate a recovery mode by placing the relay in a known state, allowing writting communication registers
<br/>
This section describes how to configure and operate the device, including its default settings, configuration mode, and reset process.

### System startup

The device starts with the EStop active (failsafe on power loss).
The device does a self-check, and starts measuring the infeed.
If all conditions are correct, the EStop is cleared. (the led turns off).
If a persistent fault is detect, the device goes in EStop.

### Relay polarity and failsafe position
The relays failsafe position is 'OFF' - that is - the contact is opened.
If the device loses power, the relays immediatly open the contacts - turning off the load.
A security prevents turning them back-on in case of crash.

The control of the relay always operates with a positive polarity, so writing '1' will close the relay - and energise the load.

### Basic operations
The relay works like any Modbus coil based devices, by writting the coil registers.
Each relay can be controlled individually, turned ON, OFF or inverted.
The relays can also be controlled in bulk.

See [Coil registers](#Coil_registers) chapter for details operations.

### Relay filtering
Filtering can be added to the relays to control the minimum ON and OFF cycles duration.
An ON filter of 2 sseconds means that the relay ON cycle will never be shorter that 2 seconds.
The same goes for the OFF filter, which guarantees a minimum duration of the OFF cycle.
Any command received during a filtered cycle will be accepted and will become the next relay state once the cycle is complete and unless overwritten by another command.

When toggling the relay, it is the projected state (the state at the end of the current cycle) that is toggled.

**Example**: a relay has an ON minimum cycle time of 5 seconds and no filter for the OFF.
* It receives a ON command. It turns ON. The cycle starts.
* Within 1 second, an OFF command is sent. This command become the next relay state, but the state of the relay remains unchanged.
* After 4 seconds, the relay turns off.
* A new ON is sent, the relay turn ON right away since the OFF cycle is not filtered.
* Within 1 second, an OFF is sent. The relay stays on.
* Another 1 second later, an ON is sent. The relay is already ON. The next state is the current state, so nothing happens.
* Another 4 seconds later, an OFF is sent. The relay turns OFF immediatly, because the ON cycle is now 6 seconds, less than the ON cycle filter duration.

> [!WARNING]
> When reading the status of a relay, the actual physical position is returned, not the projected position.

## System Health Monitoring

This device distinguishes itself from simpler models by its ability to monitor the health of the system, and to halt operations by opening an EStop relay.

The active elements being monitored and/or mesured are:
1. Device integrity
2. Modbus communications
3. Infeed voltage, that is the upsteam supply voltage
4. Proper operation of the relays

### Device integrity monitoring

The device monitors itself to guarantee proper execution.
All memories are verified:
* The flash memory integrity is verified at every boot
* The RAM is tested once on boot-up
* The EEprom storage contains a checksum and is reformatted if a corruption is detected
* The CPU power supply is monitoring by the brown-out-detector to safeguard the EEprom
* A failsafe exist which activates the EStop if the power supply was to fail
* The execution is monitored; an application crash will EStop the device

Any managed failure leads to the EStop being activated.

### Modbus communication monitoring

The device monitors the activity of the Modbus RS485 link, and through this, the health of the Modbus server.
This monitoring activity can be configured to trigger an resetable EStop.

> [!NOTE]
> The bus is considered active if commands addressed to the device are received at least once in the configured watchdog duration.

> [!IMPORTANT]
> For good operation, it is recommended to read the register 30009 (Status) periodically, like every second.
> This will keep the Modbus communication active, and all the Modbus master to detect an EStop by the relay device.

### Infeed monitoring

The module measures the infeed voltage of the relay and the type of infeed (AC vs DC).
The AC measurement is the approximate RMS value. The input signal is expected to be a 50Hz or 60Hz supply.

The last measured value, but also the lowest and highest measured values are accessible over the Modbus network.
Additional Modbus registers allow resetting those values during investigations.

The measurement can be used to trigger an EStop on:
1. Incorrect type (AC vs DC) of the infeed
2. Reverse polarity of the DC (The negative must be connected to 'N' and the positive to 'P').
3. Undervoltage : The infeed voltage is lower than a configured threshold
4. Overvoltage : The infeed voltage is higher than a configured threshold

When an infeed defect is detected, the device will open the EStop relay.<br/>
A reverse polarity is terminal.

### Relay health monitoring

All the relays are equiped with a position verification which is backed mecanically using a forced conduit.
If a relay fault is detected (open or close):
* the relay coil is de-energized, and the relay should become opened.
* the ESTop condition is trigggered in terminal mode where only a power cycle can clear the condition.

The faulty relay can be isolated by disabling it in the configuration, but it should be replaced.

### External EStop command

The modbus master can issue an EStop command. This external EStop condition can be:

* Pulsed EStop. The system will halt, but can be resumed right after
* Resetable EStop. The EStop condition is reset by pushing the 'EStop reset' push button.
* Terminal. Only a power cycle can clear the condition.

## Locate mode
The mode allows locating an relays device in setups with more than one.
When activated, the LEDs will alternate every seconds between:
 * all LEDs flash fast for 1 seconds
 * all display their normal state for the next 1 second
The locate mode is activated by a dedicated Modbus command.
It is ended by either a Modbus command, or by pressing the 'EStop reset' push button.

## Recovery mode
The recovery mode is used to recover a device with unknown setting, but also, to make changes to the communication paramters.
The recovery mode is entered by pressing the 'EStop reset' push button until the Modbus LEDs flash fast (around 3 seconds).<br/>
In these situations, the following procedure can be used:

1. Start the RelayGuardian application
2. Make sure the communication port is properly setup in the RelayGuardian
 * The connection icon should be OK
 * Issue a scan command in the RelayGuardian to make sure the RS485 adapter sends the data (Red LED)
 * If this is not the case, check the modbus is plugged correctly in the PC and the comm port is the correct one
3. If devices are identified during the scan, click the 'locate' icon one device at time to see if it is the right one
4. Else start the recovery
  1. Push the relay 'EStop reset' push button for > 3s
   * The Modbus LEDs are flashing
   * The communication values have been temporary reset to:
      | Configuration     | value             | Explanation                                    |
      |-------------------|-------------------|------------------------------------------------|
      | **Slave ID**      | `248`             | This value is reserved in the modbus standard, so no other devices should use it |
      | **Baud rate**     | `9600`            | The device talks at 9600 by default            |
      | **Serial setup**  | `8N1`             | 8bits, no parity and 1 stop bit                |

  2. In the 'Relay Guarian' application, click the 'Recovery' button
  3. Confirm proper connection
  4. In the 'RelayGuardian', click the 'Locate' button. All LEDs of relay module should be flashing fast!
   . The module is being reovered.
  5. Stop the device location mode
5. Go to the settings tabs, check the configuration, or adjust.
6. Hit the 'Synchronize' button in the relay guardian to match the relay configuration.
   . The guardian takes the relay out of recovery mode automatically.
   . You're all set!

> [!NOTE]
> The recovery mode maintains on-going relay operations.

> [!TIP]
> The recovery mode can also be ended by pressing the 'EStop reset' push button for another 3s.

## EStop mode

The device EStop relay opens the circuit when the device is in EStop condition.

The EStop condiiton is the result of:
* Detecting a internal failure (relay, crash, infeed, supply etc..)
* Through external command over Modbus

The EStop condition can be:
* Reset automatically (pulsed EStop)
* Manually reset
* Remotely reset

In all cases, the EStop condition can be read over the Modbus network.

> [!IMPORTANT]
> When the device is in both locate and EStop mode, the first push on the EStop reset clears the locate mode and the second clears the EStop.

### Relays in EStop

When the EStop is triggered by an internal condition, all the relays are **opened** (pending filtering) and no longer respond to Modbus commands (without errors).
A reset the EStop is only possible if the root cause has been cleared.
<br/>
When the EStop is triggered externally, the relays are not impacted, and it is still possible to command the relays over the modbus network.
<br/>
Example: As the infeed drops to 0 (below the threshold), all relays are opened and the EStop is activated. It is only possible to reset the EStop once the infeed voltage is back to normal.

## LEDs

The module features many LED to see the device operation and faults easilty.
All LEDs serve multiple pursposes.

### Boot

During boot, for the first 2 seconds, all LEDs are lit. This allow checking for a faulty LED.
The fault LED remains on if a persistent fault condition is detected during the auto-test.

### Locate function
To help locate a relays device, a Modbus locate command can be sent.<br/>
See [Locate mode](#Locate_mode)

The EStop reset push button can be pressed to end the locate cycle, or a new Modbus command can be sent.

For detailed LED states, see:
- [EStop LED](#estop-led)
- [Infeed LED](#infeed-led)
- [Modbus LEDs](#modbus-leds)
- [Relay LEDs](#relay-leds)

### Recovery mode
This only affects the Modbus Tx and Rx LEDs which flash at 2Hz in sync.

### EStop LED

The fault LED state is as follow:

| State | Description |
|-------|-------------|
| Off   | Normal operations |
| On    | The device is terminated. A hard reboot is required. If the termination was caused by a failing relay, the correspond relay LED will flash at 2Hz |
| Flash at 2Hz | Fault detected. The fault can be cleared by pressing the ESTOP reset button.<br/>Another LED will synchronously blink to point to the fault.<br/><ul><li><b>Infeed LED</b>: Infeed fault, under or over</li><li><b>TX LED</b>: Modbus watchdog</li></ul>
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

Both LEDs are overriden during boot, recovery and identification modes.

* The Rx LED shows received data on the RS485 bus. The data may or may not be addressed at the device
* The Tx LED shows data being sent from the device (as the device replies to the master)

### Relay LEDS

Each relay have a dedicated LED. The LED reports the status of the relay is corresponds to.

The LED status is as follow:

| State | Description                   |
|-------|-------------------------------|
| Off   | The relay is in the OFF state |
| On    | The relay is in the ON state  |
| Flash | The relay is faulty           |
| Blink | Relay is disabled             |

## Modbus communications and registers

The product uses [ModbusRTU over a half-duplex RS485](https://www.modbus.org/docs/Modbus_over_serial_line_V1.pdf).
It allow communication up-to 115200 baud to the relays device.

The Modbus registers are grouped into:
- [Coil Registers](#coil-registers)
- [Input Registers](#input-registers)
- [Holding Registers](#holding-registers)
- [Device Control Registers](#device-control-registers)

For communication settings, see [Communication Settings](#communication-settings).

> [!NOTE]
> By convention, all relays are indexed from 0.
> Therefore, the device relay label 'Relay 1' is indexed at 0. The relay '2' is indexed at 1 etc.

### Coil registers

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

> [!IMPORTANT]
> Relays operations are not impacted by the EStop
> Unless a relay is faulty or disabled, a command of that relay will always be executed, but could be delayed by one of the relay cycle filters

> [!WARNING]
> Attempts to write individually a disabled relay with the function <b>05</b> will generate a <i>slave device failure (4)</i> error.
> Attempts to turn ON a disabled or faulty relay as part of the function **15** will also generate a <i>slave device failure</i> error.
> You can still use function **15** providing that the faulty/disabled relay is commanded to OFF.

#### Detail of the function 05 : Write single coil

The following values can be used:

| Value | Meaning |
|-------|--------------------------------------------------------------------------------------------------------------|
| 0x0000| This specific 16-bit value is the Modbus standard representation for "OFF".                                  |
| 0xFF00| This specific 16-bit value is the Modbus standard representation for "ON" when writing to a coil.            |
| 0xAA00| This value toggles the coil at then end of the cycle                                                         |

### Input registers

Input registers are read-only registers used to report information about the device.
The registers have been grouped so they can easily be accessed.
Reserved values reads as 0, therefore, it is possible to read them all at once.

Only the function **04**: [Read input registers](https://www.modbustools.com/modbus.html#function04) is supported to read registers.

> [!WARNING]
> Make sure to issue function 04: Read input registers when reading input registers and not function ~~**03: Read Holding Regsiters**~~, as the memory of both types overlaps.

#### Device Identification

This registers provides details about the device in use.

| Modbus Address | Hex value | Access | Description         | Value(s)                     |
|----------------|-----------|--------|:-------------------:|------------------------------|
| 30001          | 0x0000    | R      | Product ID          | MSB=device Identification code <sup>1</sup><br/>LSB=Number of relays <sup>2</sup> |
| 30002          | 0x0001    | R      | HW version          | MSB=minor, LSB=major         |
| 30003          | 0x0002    | R      | SW version          | MSB=minor, LSB=major         |
| 30004          | 0x0003    | R      | Number of relays    | UINT16<br/>1-32              |
| 30005–30008    | 0x0004–0x0007 | R  | *Reserved*          |                              |

* <sup>1</sup> EStop relay code=0x37. Simple modbus code=0x36.
* <sup>2</sup> A 3 relays device would be 0x3703. A 32 relays device would be 0x3720.

#### Status & Monitoring

| Modbus Address | Hex value | Access | Description                 | Values                        |
|----------------|-----------|--------|-----------------------------|-------------------------------|
| 30009          | 0x0008    | R      | Current status              | <ul><li><b>0</b>: Device is operational</li><li><b>1</b>: Device is in EStop.<br/>A reset is possible</li><li><b>2</b>: Device in terminal EStop</li></ul> |
| 30010 (+1)     | 0x0009 (+1)| R     | Running minutes             | UINT32<br/>0-2<sup>32</sup>-1 |
| 30012          | 0x000B    | R      | Actual infeed voltage type  | Reports the type of infeed voltage detected<br/><ul><li><b>0</b>: Measured AC/DC voltages are below 10V</li><li><b>1</b>: DC</li><li><b>2</b>: AC</li></ul> |
| 30013          | 0x000C    | R      | Current infeed voltage      | 1/10 volts<br/>0-3000         |
| 30014          | 0x000D    | R      | Infeed highest voltage      | Returns the lowest measured infeed voltage until now in 1/10th of volts.<br/>This value can be reset with the command: - [40102 - Reset measurements](#device-control-registers) |
| 30015          | 0x000E    | R      | Infeed lowest voltage       | Same as 30014, but returns the lowest |
| 30016          | 0x000F    | R      | Last or on-going EStop root cause | <ul><li><b>0</b>: Normal<br>No EStop occured since power-up</li><li><b>1</b>: Relay<br/>A relay fault as detected<br/></li><li><b>2</b>: Modbus<br/>The communication watchdog reported a lack of communication</li><li><b>3</b>: Infeed voltage type<br/>An ncorrect voltage or voltage type was detected<br/></li><li><b>4</b>: Infeed voltage under<br/>The voltage has gone below the configured threshold</li><li><b>5</b>: Infeed voltage over<br/>The voltage has gone over the configured threshold</li><li><b>6</b>: Command<br/>A modbus command was issued to lace the device in EStop</li><li><b>7</b>: Application crash<br/>The device is recovering from an application crash</li></ul><br/>This register and the diagnostic code operate as a pair. They are not cleared when the EStop is reset, allowing for post crisis diagnostic, or checking the cause of pulsed resets. |
| 30017          | 0x0010    | R      | Diagnostic code | Diagnostic code of the EStop condition<br>The content value depends on the EStop root cause:<ul><li><b>normal</b><br/>0</li> <li><b>Relay</b><br/>Holds the faulty relay zero based index number.</li> <li><b>Modbus</b><br/>The timeout in seconds</li> <li><b>Infeed (all of them)</b><br/>0xFFFF if the voltage type is incorrect, else the triggering voltage in 1/10V</li> <li><b>Command</b><br/>Contains the EStop control value</li> <li><b>Application crash</b><br/>0xDEAD</li></ul>|
| 30018–30024    | 0x0011–0x0017 | —  | *Reserved*                  |                               |

#### Relay Diagnostics & Statistics

The status of each relays is accessible, and well as their indivual number of cycles.
A cycle is defined as a change of a relay state during operation (exclude powerloss transitions).
<br/>
The address space is structured to expand up to 32 relays for other variants of devices.

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

### Holding registers

Holding register contain values can be read or written. The hold configuration details.
They are grouped by functions.
It is possible to read all the holding registers with 1 command as the reserved values will read as one.
Writing these registers is restricted to prevent mis-behavious.

> [!WARNING]
> Reserved values cannot be written and will generate an error.

#### Supported function codes

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

#### Communication Settings

This group of registers allow configuring the communication settings of the relay.

> [!IMPORTANT]
> These registers are <ins>write protected</ins>.
> You must activate the 'recovery' mode to change them, by pressing the 'EStop Reset' button for more that 3 seconds.

> [!NOTE]
> Function ~~**06: Write single register**~~ is not availble for this group.
> You must use the command **16 - Write Multiple Registers**, writting registers 40001 to 40004 at once.
> Once a write is done, the device comes out of recovery mode and applies the changes right away.

| Modbus Address | Hex Value | Access              | Description         | Factory value | Values |
|----------------|-----------|---------------------|---------------------|---------------|--------|
| 40001          | 0x0000    | RW<sup>1</sup>      | device address      | 44            | [1-247] |
| 40002          | 0x0001    | RW<sup>1</sup>      | Baud rate selection | 5             | 0=300<br/>1=600<br/>2=1200<br/>3=2400<br/>4=4800<br/>5=9600<br/>6=19200<br/>7=38400<br/>8=57600<br/>9=115200 |
| 40003          | 0x0002    | RW<sup>1</sup>      | Parity              | 0             | 0=None<br/>1=Odd<br/>2=Even |
| 40004          | 0x0003    | RW<sup>1</sup>      | Stopbits            | 1             | 1=1 Stop bit<br/>2=2 stop bits |
| 40005–40008    | 0x0004–0x0007 | R  | *Reserved* |                     |               |

<sup>1</sup>Writable only in recovery mode

#### Power Infeed Configuration

| Modbus Address | Hex Value | Access | Description                    | Factory Value | Values |
|----------------|-----------|--------|--------------------------------|---------------|--------|
| 40009          | 0x0008    | RW     | Infeed voltage type            | 1             | 0=DC<br/>1=AC |
| 40010          | 0x0009    | RW     | Infeed lower voltage threshold | 100           | 1/10 volts [100-3000] |
| 40011          | 0x000A    | RW     | Infeed upper voltage threshold | 3000          | 1/10 volts [100-3000] |
| 40012–40016    | 0x000B–0x000F | R  | *Reserved*                     | 0             |        |

> [!WARNING]
> Registers 40009 to 40011 must be written together using command **16**.
> Function ~~**06: Write single register**~~ is not availble for this group.
> An illegal_data_value error (code 3) will be returned if (40011 - 40010) < 10

#### Safety Logic Configuration

Some of the monitored values can optionally control the EStop. It is also possible to set the behaviour of each relay on these optional EStop conditions.

| Modbus Address | Hex Value | Access | Description                               | Factory Value | Values |
|----------------|-----------|--------|-------------------------------------------|---------------|--------|
| 40017          | 0x0010    | RW     | Activate EStop on undervoltage            | 0             | 0=no<br/>1=yes |
| 40018          | 0x0011    | RW     | Activate EStop on overvoltage             | 0             | 0=no<br/>1=yes |
| 40019          | 0x0012    | RW     | Activate EStop on incorrect type of supply voltage<br>Example: AC detected with DC configured | 0 | 0=no<br/>1=yes |
| 40020          | 0x0013    | RW     | Activate EStop on number of seconds without modbus frame received | 0             | 0=off<br/>[1-65535] Number of seconds |
| 40021          | 0x0014    | RW     | Mask to open relays on electrical infeed fault.<br/>When a infeed fault is detected (undervoltage, overvoltage or incorrect voltage type), the projected relay state<sup>1</sup> is opened or left unchanged based on this value.<br/>A bit at 1 in the mask indicates the relay should open on a fault detection. The lsb (bit 0) masks the relay at index 0, the msb (bit 15) masks the relay at index 15 | 0 | 0-0xFFFF<br/><br/>Example:<br/>0: No relays are affected<br/>5 (binary 110): Relays at position 1 and 2 are set to open<br/>0xFFFF: All relays are set to open |
| 40022          | 0x0015    | RW     | Mask to open relays on modbus communication loss fault<br/>When a loss in communication is detected, the relay projected state<sup>1</sup> is opened is left unchanged with this value. | 0 | See 40021 |
| 40023–40024    | 0x0016–0x0017 | R  | *Reserved*                                |               |        |

<sup>1</sup>The projected state is the state of a relay once the on-going filter cycle is complete

> [!WARNING]
> The registers must be written individually. The command **16** is not supported.

#### Relays Configuration

This group allow configuring the individual relays.

| Modbus Address | Hex Value | Access | Description         | Factory default      | Values |
|----------------|-----------|--------|---------------------|----------------------|-------|
| 40025          | 0x0018    | RW     | Relay 1 config      | 0=Enabled, no filter | msb bits [15-8] are for the ON filter<br/>lsb bits [7-0] are the OFF filter<br/>The value 0xFFFF disables the relay.<br/><br/>The ON and OFF filters values are given in 1/10 of seconds from 0 to 254 (0 to 25.4s)<br/><u>Example</u>: 0x0132=The On state is guaranteed to last at least 100ms, whilst the OFF state is guaranteed to last at least 50 x 1/10s = 5s after a ON. See relay operations. |
| 40026          | 0x0019    | RW     | Relay 2 config      | 0=Enabled, no filter | Same as relay 1 config   |
| 40027          | 0x001A    | RW     | Relay 3 config      | 0=Enabled, no filter | Same as relay 1 config  |
| 40025+N<sup>1</sup>    | 0x0018+N| RW     | Relay N config | 1=Enabled, no filter | Same as relay 1 config |

<sup>1</sup> N is the relay index [0..(Number of relay-1)]

> [!WARNING]
> The registers must be written individually. The command **16** is not supported.

> [!WARNING]
> Read or writing a non-supported relay will generate an error.

### Device control registers

These register share the holding registers mapping address space but are write-only.
This group of register allows controlling the EStop and the device device.

> [!NOTE]
> These registers are write only.
> The function ~~**03: Read multiple registers**~~ is not availble for this group.
> Function ~~**16 - Write Multiple Registers**~~ is not available.
> The registers must be written individually.

| Modbus Address | Hex Value | Access | Description         | Values |
|----------------|-----------|--------|---------------------|--------|
| 40101          | 0x0064    | W      | Set/Reset the EStop   | ESTOP_CTRL<br/>See note <sup>1</sup>|
| 40102          | 0x0065    | W      | Zero measurements. Allow re-measuring min and max | 0xAA55  |
| 40103          | 0x0066    | W      | Identify the device. Fast flash all the LEDs to identify the device | 0 = Turn off<br/>1 = Turn on |
| 40104          | 0x0067    | W      | Reset configuration to factory default and reboot  | 0x178C |
| 40105          | 0x0068    | W      | Exit recovery mode and apply comms settings  | 0xAA55  |
| 40106          | 0x0069    | W      | Reset the device    | 0xAA55  |

**Note <sup>1</sup>** : See the format below

#### EStop control

The following table documents the type **ESTOP_CTRL** used to control the EStop.

| Byte | Function | Values |
|--------------|----------|-------------|
| MSB  | Type of EStop | 0x00 : Reset the EStop if possible. Returns an error if the EStop could not be reset<br/>0x11 : Pulsed EStop. Create a 4 second EStop pulse<br/>0x22 : Resetable EStop mode. The EStop can be reset by pushing the EStop reset button<br/>0xFF : Terminal EStop. Only a reset will clear the ESTop (Register 40103) |
| LSB          | Diagnostic code | A code can be applied to allow investigating the cause of EStop/Reset.<br/>Values: <0-255> |

## Electrical and mecanical characteristics

For relay data, refer to the datasheet of the [SiSF2 relay](https://www.ermec.com/catalogos/2021/ELESTA/sisf2_-_en.pdf) for detailed data.
Note that both contacts of the relay are paired as one.
The key information is summ

### Electrical characteristics

| Item | Min | Max |
|------|-----|-----|
| Device supply voltage | 18 VDC | 30 VDC |
| Infeed voltage | -      | 250V (AC/DC) |
| Relay contact current | - | 9.4A (AC/DC) |

### Contact life

| Item | Value |
|------|----------|
| 230VAC, 1A | 800k |
| 230VAC, 9.4A | 100k |
| 24VDC, 0.5A | 2M |
| 24VDC, 9.4A | 300k |

## Troubleshooting

### Cannot Communicate with device
- If the modbus LEDs are flickering, make sure the RelayGuarian and relay share the same settings.
- Active the recovery mode on the device and in the RelayGuardian
- If no communication is established, check the serial adapter communication port
- Check the wriring. Modbus pins cannot be swapped.

### Lost Device ID
1. Send a valid broadcast frame to enter configuration mode.
2. Use the default Device ID (`0`) to reset the device via the reset register.

### Relay Does Not Respond
- Check the watchdog timeout setting to ensure it isn’t triggering prematurely.

---

## FMEA – Modbus Relay Board for CNC Safety

This FMEA analyzes potential failure modes of a Modbus-controlled relay board designed for CNC safety, including load control and emergency stop (E-stop) functionality.
Any failure should result in the CNC stopping by letting go of the estop switch.
<br/>**Note**: The estop must be checked once at the start of operations. This step is done by the Masso CNC device. This relay estop contact is Normally Opened. So when the CNC starts, the relay would be open.
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
| Power Monitoring     | Under/over voltage undetected | Damage to CNC or relay                | Sensor failure, ADC error              | Voltage threshold check      | 9 | 3 | 4 | 108 | Add voltage sensing, calibration check                    |
| Power Converter      | Converter fails               | Relay board unpowered, failsafe triggers | Component failure                   | Relay state monitoring       | 7 | 4 | 2 | 56  | Use robust converter, thermal protection. =<br/>Power the estop relay from the mains power |
| Modbus Communication | Loss of communication         | E-stop triggered, CNC halts           | Cable fault, EMI, software crash       | Timeout watchdog             | 6 | 5 | 2 | 60  | Loss of comms indicates the bus is damaged or the master has crashed. esop. |
| Test of the esop switch | estop relay opens and close cycle is considered a estop switch test  | False E-stop test                    | Non cold reboot of the firmware | Analysis of the reboot cause | 8 | 3 | 3 | 72  | Do not allow the modbus board to reboot |

### Legend
- **S (Severity)**: 1 (low) to 10 (catastrophic)
- **O (Occurrence)**: 1 (rare) to 10 (frequent)
- **D (Detection)**: 1 (certain detection) to 10 (undetectable)
- **RPN (Risk Priority Number)**: S × O × D

### Summary
Focus should be placed on:
- Improving detection of stuck or failed relays
- Enhancing voltage monitoring redundancy
- Ensuring robust communication and feedback diagnostics
