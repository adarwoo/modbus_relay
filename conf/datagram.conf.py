[]#!/usr/bin/env python3
from modbus_rtu_slave_rc import *  # Import everything from modbus_generator

Modbus({
    "buffer_size": 32,
    "namespace": "relay",
    "on_received": "on_ready_reply",

    "callbacks": {
        "on_read_coils":    [(u8, "addr"), (u8, "qty")],
        "on_set_single":    [(u8, "addr"), (u16, "operation")],
        "on_set_multiple":  [(u8, "operation")],
        "on_read_info":     [(u8, "addr"), (u8, "qty")],
        "on_read_running_time": [],
        "on_read_relay_stats": [(u8, "addr"), (u8, "qty")],
        "reset_device"       : [],
        "set_baud"           : [(u16, "baud")],
        "set_parity"         : [(u16, "parity")],
        "set_stopbits"       : [(u16, "stopbits")],
        "set_watchdog"       : [(u16, "watchdog")],
        "set_config"         : [(u16, "baud"), (u16, "parity"), (u16, "stopbits")]
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

        (WRITE_SINGLE_REGISTER, u16(4), u16([1200,2400,4800,9600, 19200, 38400, 57600, 115200]), "set_baud"),
        (WRITE_SINGLE_REGISTER, u16(5), u16([0,1,2]), "set_parity"),
        (WRITE_SINGLE_REGISTER, u16(6), u16([0,1]), "set_stopbits"),
        (WRITE_SINGLE_REGISTER, u16(7), u16(), "set_watchdog"),

        (WRITE_MULTIPLE_REGISTERS, u16(0), u16(2), u8(4), u32(0xDEAD5AFE), "reset_device"),

        (WRITE_MULTIPLE_REGISTERS, u16(4), u16(3), u8(6), u16(alias="baud"), u16(alias="parity"), u16(alias="stopbits"), "set_config"),

        # Holding registers maps:
        # 0 - device into
        # 1000 - configuration
        # 2000 - relay stats
        # Holding regs:
        # 0 : device ID
        # 1 : HW rev
        # 2 : SW rev
        # 3 : reserved - reads as 0
        # 4 : baud - 1200, 2400, 4800, 9600, 19200, 38400, 56700, 115200
        # 5 : Parity - 0 (None), 1 (Odd), 2 (even)
        # 6 : Number of stop bits - 1 or 2
        # 100 : Reset the device

        (READ_HOLDING_REGISTERS, u16(0, 7), u16(1, 8), "on_read_info"),
        (READ_HOLDING_REGISTERS, u16(100), u16(2), "on_read_running_time"),
        (READ_HOLDING_REGISTERS, u16(200, 202), u16(2, 6), "on_read_relay_stats"),
    ],

})


