[]#!/usr/bin/env python3
from modbus_rtu_rc import *  # Import everything from modbus_generator

Modbus({
    "namespace": "net",
    "on_received": "on_payload_received",
    "slave": True,

    "callbacks": {
        # -----------------------------------------------------------------------------------------
        # Coils callbacks
        # -----------------------------------------------------------------------------------------
        "on_read_coils"             : [(u8, "addr"), (u8, "qty")],
        "on_set_single"             : [(u8, "addr"), (u16, "operation")],
        "on_set_multiple"           : [(u8, "operation")],

        # -----------------------------------------------------------------------------------------
        # Input registers callbacks
        # -----------------------------------------------------------------------------------------
        "on_read_inputs"            : [(u8, "addr"), (u8, "count")],

        # -----------------------------------------------------------------------------------------
        # Holding registers callbacks
        # -----------------------------------------------------------------------------------------
        "on_read_holdings"          : [(u8, "addr"), (u8, "count")],

        # Communication settings
        "on_write_comms_settings"   : [
            (u8, "addr"), (u8, "baud"), (u8, "parity"), (u8, "stopbits")
        ],

        # Power infeed configuration
        "on_write_estop_on_under"   : [(u8, "onoff")],
        "on_write_estop_on_over"    : [(u8, "onoff")],
        "on_write_estop_on_bad_voltage_type" : [(u8, "onoff")],
        "on_write_estop_on_timeout" : [(u16, "timeout")],

        # Single relay configuration
        "on_write_single_relay_cfg" : [(u8, "address"), (u8, "conf"), (u8, "filter")],

        # Device control
        "on_estop"                  : [(u8, "estop_type"), (u8, "diag")],
        "on_measurement_reset"      : [],
        "on_locate"                 : [(u8, "onoff")],
        "on_factory_reset"          : [],
        "on_reset"                  : []
    },

    "device": [
        # -----------------------------------------------------------------------------------------
        # Coils operations
        # -----------------------------------------------------------------------------------------
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

        # -----------------------------------------------------------------------------------------
        # Input registers
        # -----------------------------------------------------------------------------------------
        (READ_INPUT_REGISTERS,  u16(0, 0x2F), u16(1, 0x30),       "on_read_inputs"),

        # -----------------------------------------------------------------------------------------
        # Holding registers
        # -----------------------------------------------------------------------------------------

        # Generic read of holding registers
        (READ_HOLDING_REGISTERS, u16(0, 0x1A), u16(1,0x1B),       "on_read_holdings"),

        # Communication settings
        (WRITE_MULTIPLE_REGISTERS,
            u16(0), u16(4), u8(8),
                u16(1,127), # Device address
                u16(0,9),   # Baud rate
                u16(0,2),   # Parity
                u16(1,2),   # Stop bits
            "on_write_comms_settings"
        ),

        # Power infeed configuration
        (WRITE_SINGLE_REGISTER,  u16(0x10), u16(0,1),             "on_write_estop_on_under"),
        (WRITE_SINGLE_REGISTER,  u16(0x11), u16(0,1),             "on_write_estop_on_over"),
        (WRITE_SINGLE_REGISTER,  u16(0x12), u16(0,1),             "on_write_estop_on_bad_voltage_type"),
        (WRITE_SINGLE_REGISTER,  u16(0x13), u16(),                "on_write_estop_on_timeout"),

        # Relay configuration
        (WRITE_SINGLE_REGISTER,  u16(0x18,0x1A), u8(0,0xf), u8(), "on_write_single_relay_cfg"),

        # Device control
        (WRITE_SINGLE_REGISTER,  u16(0x64), u8([0,0x11,0x22,0xff]), u8(), "on_estop"),
        (WRITE_SINGLE_REGISTER,  u16(0x65), u16(0xAA55),          "on_measurement_reset"),
        (WRITE_SINGLE_REGISTER,  u16(0x66), u16(0,1),             "on_locate"),
        (WRITE_SINGLE_REGISTER,  u16(0x67), u16(0xAA55),          "on_factory_reset"),
        (WRITE_SINGLE_REGISTER,  u16(0x68), u16(0xAA55),          "on_reset"),
    ],
})
