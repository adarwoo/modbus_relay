[]#!/usr/bin/env python3
from modbus_rtu_rc import *  # Import everything from modbus_generator

Modbus({
    "buffer_size": 32,
    "namespace": "relay",
    "on_received": "on_ready_reply",
    "slave": True,

    "callbacks": {
        "on_read_coils"       : [(u8, "addr"), (u8, "qty")],
        "on_set_single"       : [(u8, "addr"), (u16, "operation")],
        "on_set_multiple"     : [(u8, "operation")],
        "on_read_info"        : [(u8, "addr"), (u8, "qty")],
        "on_read_config"      : [(u8, "addr"), (u8, "qty")],
        "on_read_stats"       : [(u8, "addr"), (u8, "qty")],
        "reset_device"        : [],
        "on_write_config"     : [(u8, "addr"), (u16, "value")],
    },

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

        #   0 (40001): (R)  Product ID
        #   1 (40002): (R)  HW version
        #   2 (40003): (R)  SW version

        # 100 (40101): (RW) Device address
        # 101 (40102): (RW) Baud rate selection (1/10 of the baud rate)
        # 102 (40103): (RW) Parity
        # 103 (40104): (RW) Stopbits
        # 104 (40105): (RW) Watchdog for the bus communication : number of seconds without activity
        # 105 (40106): (RW) Ingress monitor mode 0=DC, 2=AC_50Hz, 3=AC_60Hz
        # 106 (40107): (RW) Ingress Min DC voltage in volts
        # 107 (40108): (RW) Ingress Max DC voltage in volts
        # 108 (40109): (RW) Ingress Min AC voltage in volts
        # 109 (40110): (RW) Ingress Max AC voltage in volts
        # 110 (40111): (RW) EStop on communication watchdog
        # 111 (40112): (RW) EStop on relay
        # 112 (40113): (RW) EStop on undervoltage
        # 113 (40114): (RW) EStop on overvoltage

        # 200 (40101): (R)  Running minutes
        # 201 (40101): (R)  Relay 0 number of cycles
        # 202 (40102): (R)  Relay 1 number of cycles
        # 203 (40103): (R)  Relay 2 number of cycles

        # 300: (W) EStop Write 0x5104 to trigger an estop pulse of 1 second
        # 400: (W) Factory reset. Erase the memory back to factory settings. Write 0x5043

        (READ_HOLDING_REGISTERS,   u16(0,     2), u16(1, 3), "on_read_info"),
        (READ_HOLDING_REGISTERS,   u16(100, 110), u16(1, 5), "on_read_config"),
        (READ_HOLDING_REGISTERS,   u16(200, 203), u16([2,4,6,8]), "on_read_stats"),

        (WRITE_SINGLE_REGISTER,    u16(100, 111), u16(), "on_write_config"),

        #(WRITE_MULTIPLE_REGISTERS, u16(400), u16(2), u8(4), u32(0xDEAD5AFE), "reset_device"),

    ],

})


