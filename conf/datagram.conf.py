#!/usr/bin/env python3
from modbus_rtu_slave_rc import *  # Import everything from modbus_generator

Modbus({
    "buffer_size": 32,
    "namespace": "relay",

    "callbacks": {
        "on_read_coils":    [(u8, "addr"), (u8, "qty")],
        "on_set_single":    [(u8, "addr"), (u16, "operation")],
        "on_set_multiple":  [(u8, "operation")],
        "on_read_version":  [],
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

        (READ_HOLDING_REGISTERS, u16(1), "on_read_version"),
    ]
})
