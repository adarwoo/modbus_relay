# ✅ Test Plan for Modbus N-Relay Device with EStop

This document describes the test scenarios and test cases to verify compliance with the specification requirements.

---

## 🧪 Test Scenario 1: Relay Control via Modbus

### Test Case 1.1: Write Single Coil - ON
**Setup**: Relay 1 configured as enabled, no filtering  
**Operation**: Send Modbus function 05 with value `0xFF00` to coil 0  
**Expected Result**: Relay 1 closes (ON), relay LED turns ON

### Test Case 1.2: Write Single Coil - OFF
**Setup**: Relay 1 previously ON  
**Operation**: Send Modbus function 05 with value `0x0000` to coil 0  
**Expected Result**: Relay 1 opens (OFF), relay LED turns OFF

### Test Case 1.3: Write Single Coil - Toggle
**Setup**: Relay 1 initially ON  
**Operation**: Send Modbus function 05 with value `0xAA00` to coil 0  
**Expected Result**: Relay 1 toggles to OFF

### Test Case 1.4: Write to Disabled Relay
**Setup**: Relay 2 configured as disabled (0xFFFF)  
**Operation**: Send Modbus function 05 to coil 1  
**Expected Result**: Modbus returns slave device failure

---

## 🧪 Test Scenario 2: Infeed Voltage Monitoring

### Test Case 2.1: In-Range Voltage
**Setup**: Infeed set to 230V AC, configured range 200–240V  
**Operation**: Apply voltage  
**Expected Result**: No EStop triggered, infeed LED ON

### Test Case 2.2: Undervoltage Fault
**Setup**: Infeed set to 180V, configured lower threshold = 200V, undervoltage EStop enabled  
**Operation**: Apply voltage  
**Expected Result**: EStop triggered, EStop LED 2Hz blink, infeed LED slow blink

### Test Case 2.3: Overvoltage Fault
**Setup**: Infeed set to 250V, upper threshold = 240V, overvoltage EStop enabled  
**Operation**: Apply voltage  
**Expected Result**: EStop triggered, EStop LED 2Hz blink, infeed LED long ON short OFF

---

## 🧪 Test Scenario 3: Communication Watchdog

### Test Case 3.1: No Watchdog Trigger
**Setup**: Watchdog set to 5s, register 30009 read every second  
**Operation**: Let system run for 10s  
**Expected Result**: No EStop triggered

### Test Case 3.2: Watchdog Timeout
**Setup**: Watchdog set to 5s, stop communication  
**Operation**: Wait 6s  
**Expected Result**: EStop triggered, EStop cause = 2, diagnostic = 5

---

## 🧪 Test Scenario 4: EStop Functionality

### Test Case 4.1: Pulsed EStop
**Setup**: Normal state  
**Operation**: Write `0x1101` to register 40101  
**Expected Result**: EStop active for 4s, resets automatically

### Test Case 4.2: Resettable EStop
**Setup**: Normal state  
**Operation**: Write `0x2202` to register 40101  
**Expected Result**: EStop active until reset button pressed

### Test Case 4.3: Terminal EStop
**Setup**: Normal state  
**Operation**: Write `0xFF03` to register 40101  
**Expected Result**: EStop active, only reset via power cycle

---

## 🧪 Test Scenario 5: Recovery Mode

### Test Case 5.1: Enter Recovery
**Setup**: Device in normal mode  
**Operation**: Hold reset button > 3s  
**Expected Result**: LEDs flash, device uses slave ID 248, 9600 baud, 8N1

### Test Case 5.2: Modify Comm Settings
**Setup**: Device in recovery  
**Operation**: Write to registers 40001–40004 using function 16  
**Expected Result**: Settings updated, device exits recovery mode

---

## 🧪 Test Scenario 6: LED Indicators

### Test Case 6.1: Boot Test
**Setup**: Device powered OFF  
**Operation**: Power ON  
**Expected Result**: All LEDs ON for 2 seconds

### Test Case 6.2: Identify Command
**Setup**: Normal state  
**Operation**: Write `1` to register 40103  
**Expected Result**: All LEDs flash at 10Hz for 1s

---

## 🧪 Test Scenario 7: Relay Health & Diagnostics

### Test Case 7.1: Relay Cycle Counter
**Setup**: Relay 1 configured, OFF  
**Operation**: Toggle ON/OFF 5 times  
**Expected Result**: Register 30026–30027 increases by 5

### Test Case 7.2: Faulty Relay
**Setup**: Simulate stuck relay  
**Operation**: Toggle relay  
**Expected Result**: EStop triggered, LED blinks 2Hz, diagnostic code = relay index

---

## 🧪 Test Scenario 8: Factory Reset & Configuration

### Test Case 8.1: Factory Reset
**Setup**: Custom settings applied  
**Operation**: Write `0x178C` to register 40104  
**Expected Result**: Device resets, default settings restored

---

## 🧪 Test Scenario 9: Unsupported Conditions

### Test Case 9.1: Unsupported Relay Index
**Setup**: Device with 3 relays  
**Operation**: Access coil or register for relay 4  
**Expected Result**: Illegal data address error

### Test Case 9.2: Overlapping Read to Wrong Register Type
**Setup**: Normal state  
**Operation**: Read input register using function 03  
**Expected Result**: Unexpected data or error