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
        "set_device_id"       : [(u8, "device_id")],
        "set_baud"            : [(u16, "baud")],
        "set_parity"          : [(u16, "parity")],
        "set_stopbits"        : [(u16, "stopbits")],
        "set_watchdog"        : [(u16, "watchdog")],
        "config_device"       : [(u8, "addr"), (u16, "baud"), (u8, "parity"), (u8, "stopbits"), (u16, "wd")],
        "on_read_reset"       : [],

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

        # 0 (40001): (R)  Product ID
        # 1 (40002): (R)  HW version
        # 2 (40003): (R)  SW version

        # 100 (40009): (RW) Device address
        # 101 (40010): (RW) Baud rate selection (1/10 of the baud rate)
        # 102 (40011): (RW) Parity
        # 103 (40012): (RW) Stopbits
        # 104 (40013): (RW) Watchdog for the bus communication : 0 = off, 1 = bus activity check, 2 = device activity
        # 105 (40013): (RW) Watchdog for the bus communication : number of seconds without activity
        # 106 (40014): (RW) Ingress monitor mode 0=Off, 1=DC, 2=AC_50Hz, 3=AC_60Hz
        # 107 (40015): (RW) Ingress Min DC voltage in 10th volts
        # 108 (40016): (RW) Ingress Max DC voltage in 10th volts
        # 109 (40017): (RW) Ingress Min AC voltage in 10th volts
        # 110 (40018): (RW) Ingress Max AC voltage in 10th volts
        # 105 (40014): (RW) EStop configuration 0 = no estop, 1 = comms only, 2 = relay, 4 = undervolt, 8 = overvolt, 255 = all

        # 100(40101): (R)  Running minutes
        # 101(40101): (R)  Relay 0 number of cycles
        # 102(40102): (R)  Relay 1 number of cycles
        # 103(40103): (R)  Relay 2 number of cycles

        # 200: (W) EStop Write 0x5104 to trigger an estop pulse of 1 second
        # 300: (W) Factory reset. Erase the memory back to factory settings. Write 0x5043

        (READ_HOLDING_REGISTERS, u16(0, 2), u16(1, 3), "on_read_info"),
        (READ_HOLDING_REGISTERS, u16(8, 12), u16(1, 5), "on_read_config"),
        (READ_HOLDING_REGISTERS, u16(16, 22), u16([2,4,6,8]), "on_read_stats"),
        (READ_HOLDING_REGISTERS, u16(99), u16(2), "on_read_reset"),

        (WRITE_SINGLE_REGISTER, u16(8), u16(1,255), "set_device_id"),
        (WRITE_SINGLE_REGISTER, u16(9), u16([12, 24, 48, 96, 192, 384, 576, 1152]), "set_baud"),
        (WRITE_SINGLE_REGISTER, u16(10), u16(0,2), "set_parity"),
        (WRITE_SINGLE_REGISTER, u16(11), u16([1, 2]), "set_stopbits"),
        (WRITE_SINGLE_REGISTER, u16(12), u16(), "set_watchdog"),

        (WRITE_MULTIPLE_REGISTERS, u16(99), u16(2), u8(4), u32(0xDEAD5AFE), "reset_device"),

    ],

})


