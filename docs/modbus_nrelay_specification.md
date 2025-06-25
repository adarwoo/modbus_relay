# ✅ Specification Requirements for Modbus N-Relay Device with EStop

## 1. Mechanical & Electrical Specifications

### 1.1 Relay and Voltage Ratings
- **SR-001**: The device shall support up to 3 relays, each rated for a maximum of 9.4A at 250V AC/DC.
- **SR-002**: The infeed voltage shall not exceed 240V (AC or DC).
- **SR-003**: The device power supply input shall operate between 15VDC and 30VDC.

## 2. Modbus RTU Interface

### 2.1 Communication
- **SR-004**: The device shall communicate over RS485 using Modbus RTU, supporting baud rates from 300 to 115200.
- **SR-005**: The communication parameters (Slave ID, baud rate, parity, stop bits) shall be configurable only in recovery mode via Modbus function 16 (Write Multiple Registers).
- **SR-006**: The device shall support Modbus function codes: 01 (Read Coils), 05 (Write Single Coil), 15 (Write Multiple Coils), 03 (Read Holding Registers), 04 (Read Input Registers), 06 (Write Single Register), 16 (Write Multiple Registers) only where specified.

### 2.2 Coil Operation
- **SR-007**: The relays shall be indexed from 0 (Relay 1) to 2 (Relay 3) in coil registers.
- **SR-008**: The device shall treat a coil written with 0xFF00 as ON (closed) and 0x0000 as OFF (open).
- **SR-009**: The failsafe state of each relay shall be OFF (open).
- **SR-010**: Writing to a disabled or faulty relay via function 05 or 15 shall return a slave device failure (code 4).

## 3. Safety & Monitoring

### 3.1 EStop Conditions
- **SR-011**: The device shall open all relays and block Modbus control when in any EStop condition.
- **SR-012**: The EStop condition shall be triggered under the following conditions:
  - Application crash
  - Power supply failure
  - Relay fault
  - Incorrect or out-of-range infeed voltage
  - Communication loss (based on watchdog timeout)
  - Modbus-issued EStop command
- **SR-013**: The device shall support 3 types of Modbus-triggered EStops:
  - Pulsed (resettable immediately after 4s)
  - Resettable (manual reset via push button)
  - Terminal (reset only via power cycle)

### 3.2 Infeed Voltage Monitoring
- **SR-014**: The device shall support configuration of acceptable voltage range between 10V to 300V (in 1/10V units).
- **SR-015**: The device shall support configuration of expected voltage type (AC 50Hz, AC 60Hz, or DC).
- **SR-016**: If voltage type or range is incorrect, the device shall enter an EStop state.
- **SR-017**: The infeed voltage type shall be reported as:
  - 0: No valid voltage (<10V)
  - 1: DC detected
  - 2: AC detected

### 3.3 Watchdog
- **SR-018**: If no Modbus command is received within the configured watchdog timeout (1–65535 seconds), the device shall trigger an EStop.
- **SR-019**: The watchdog timeout shall be configurable via holding register 40020.

## 4. Diagnostics & Statistics

- **SR-020**: The device shall store the cause and a diagnostic code for the most recent EStop event in registers 30016 and 30017, respectively.
- **SR-021**: Each relay shall maintain a counter of activation cycles (registers 30026, 30029, 30032).
- **SR-022**: The last, highest, and lowest measured infeed voltage shall be reported via input registers (30013–30015), with the ability to reset via register 40102 (write 0xAA55).

## 5. LED Behavior

- **SR-023**: On boot, all LEDs (except Modbus TX) shall turn ON for 2 seconds to test functionality.
- **SR-024**: EStop LED shall reflect fault state:
  - Off: Normal
  - On: Terminal EStop
  - 2Hz Flash: Resettable EStop
  - 2s Flash: Pulsed EStop
- **SR-025**: The Infeed LED shall:
  - Be OFF if voltage < 10V
  - Flash slowly if under-voltage
  - Flash with long ON, short OFF if over-voltage
  - Be ON if voltage is within range
- **SR-026**: The Modbus RX LED shall flash on activity or recovery mode, and TX LED shall flash only on outgoing traffic.
- **SR-027**: Each Relay LED shall:
  - Be ON if the relay is closed
  - Be OFF if open
  - Flash at 2Hz if the relay is faulty
  - Flash slowly if disabled

## 6. Relay Configuration

- **SR-028**: Each relay shall be individually configurable as:
  - Enabled with no filtering (`0`)
  - Enabled with a debounce filter between 100–60000 ms
  - Disabled (`0xFFFF`)
- **SR-029**: Configuration changes to relays shall require a device reset to take effect.

## 7. Recovery Mode

- **SR-030**: Recovery mode shall be activated by pressing the reset button for >3s.
- **SR-031**: In recovery mode:
  - Slave ID = 248
  - Baud rate = 9600
  - Serial = 8N1
- **SR-032**: Recovery mode shall maintain ongoing relay states and allow modification of communication settings.

## 8. Device Identification & Control

- **SR-033**: The device shall expose a unique Product ID (register 30001), HW version (30002), and SW version (30003).
- **SR-034**: The relay count (register 30004) shall match the actual number of relays supported (1–32).
- **SR-035**: The device shall support the following write-only control commands via register 40101–40106:
  - Set/Reset EStop
  - Zero measurements
  - Flash all LEDs
  - Factory reset
  - Exit recovery mode
  - Reset device

## 9. Fault Handling

- **SR-036**: If a relay position check fails, the EStop shall be triggered in terminal mode.
- **SR-037**: A faulty relay shall be identifiable via its LED flashing at 2Hz and input register status.

## 10. Default Settings

- **SR-038**: Default configuration on factory reset shall include:
  - Relay 1–3: Enabled, no filter
  - Watchdog timeout: 0 (disabled)
  - Infeed voltage type: AC 50Hz
  - Voltage thresholds: Lower = 100, Upper = 3000 (1/10V)

## 11. Open Requirements for Review

- **[OPEN-REQ-01]**: Behavior if voltage is exactly on threshold value (e.g., 100 or 3000). Should this be considered in-range or out-of-range?
- **[OPEN-REQ-02]**: If recovery mode times out or can persist indefinitely.
- **[OPEN-REQ-03]**: Definition of debounce filtering algorithm for relay inputs.