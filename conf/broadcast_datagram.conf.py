#!/usr/bin/env python3
from modbus_rtu_rc import *  # Import everything from modbus_generator

Modbus({
    "slave": True,
    "namespace": "broadcast",
    "callbacks": {"on_reset": []},
    "device@0": [
        (WRITE_MULTIPLE_REGISTERS, u16(99), u16(2), u8(4), u32(0xDEAD5AFE), "on_reset")
    ]
})
