[]#!/usr/bin/env python3
from modbus_rtu_rc import *  # Import everything from modbus_generator

Modbus({
    "buffer_size": 127,
    "namespace": "relay",
    "on_received": "on_ready_reply",
    "slave": True,

    "callbacks": {
        "on_read_coils"             : [(u8, "addr"), (u8, "qty")],
        "on_set_single"             : [(u8, "addr"), (u16, "operation")],
        "on_set_multiple"           : [(u8, "operation")],

        "on_read_inputs"            : [(u8, "addr"), (u8, "count")],

        "on_read_holdings"          : [(u8, "addr"), (u8, "count")],

        "on_write_device_address"   : [(u8, "addr")],
        "on_write_baud_rate"        : [(u8, "baud")],
        "on_write_parity"           : [(u8, "parity")],
        "on_write_stopbits"         : [(u8, "stopbits")],
        "on_write_comms_settings"   : [
            (u8, "addr"), (u8, "baud"), (u8, "parity"), (u8, "stopbits")
        ],

        "on_write_estop_on_under"   : [(u8, "onoff")],
        "on_write_estop_on_over"    : [(u8, "onoff")],
        "on_write_estop_on_timeout" : [(u16, "timeout")],
        "on_write_estop_settings"   : [
            (u8, "over"), (u8, "under"), (u8, "timeout")
        ],

        "on_write_relay_cfgs"       : [
            (u8, "conf1"), (u8, "filter1"),
            (u8, "conf2"), (u8, "filter2"),
            (u8, "conf3"), (u8, "filter3")
        ],

        "on_write_single_relay_cfg" : [(u8, "address"), (u8, "conf"), (u8, "filter")]
    },

    "device@44": [
        # Coils operations
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

        # Input registers
        (READ_INPUT_REGISTERS,  u16(0, 0x2F), u16(1, 0x30), "on_read_inputs"),

        ## Holding registers
        (READ_HOLDING_REGISTERS, u16(0, 0x1A), u16(1,0x1B), "on_read_holdings"),

        # Communication settings
        (WRITE_SINGLE_REGISTER,  u16(0), u16(1,127), "on_write_device_address"),
        (WRITE_SINGLE_REGISTER,  u16(1), u16(0,9),   "on_write_baud_rate"),
        (WRITE_SINGLE_REGISTER,  u16(2), u16(0,2),   "on_write_parity"),
        (WRITE_SINGLE_REGISTER,  u16(3), u16(1,2),   "on_write_stopbits"),
        (WRITE_MULTIPLE_REGISTERS,
            u16(0), u16(4), u8(8),
                u16(1,127),
                u16(0,9),
                u16(0,2),
                u16(1,2),
            "on_write_comms_settings"
        ),

        # Power infeed configuration
        (WRITE_SINGLE_REGISTER,  u16(0x10), u16(0,1), "on_write_estop_on_under"),
        (WRITE_SINGLE_REGISTER,  u16(0x11), u16(0,1), "on_write_estop_on_over"),
        (WRITE_SINGLE_REGISTER,  u16(0x12), u16(),    "on_write_estop_on_timeout"),
        (WRITE_MULTIPLE_REGISTERS,
            u16(0x10), u16(3), u8(6),
                u16(0,1),
                u16(0,1),
                u16(),
            "on_write_estop_settings"
        ),

        # Relay configuration
        (WRITE_SINGLE_REGISTER,  u16(0x18,0x1A), u8(0,7), u8(), "on_write_single_relay_cfg"),
        (WRITE_MULTIPLE_REGISTERS,
            u16(0x18), u16(3), u8(6),
                u8(0,7), u8(),
                u8(0,7), u8(),
                u8(0,7), u8(),
            "on_write_relay_cfgs"
        ),
    ],
})


