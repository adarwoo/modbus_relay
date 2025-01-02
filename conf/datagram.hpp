/**
 * This file was generated to create a state machine for processing
 * uart data used for a modbus RTU. It should be included by
 * the modbus_rtu_slave.cpp file only which will create a full rtu slave device.
 */
#include <logger.h>
#include <stdint.h>
#include <asx/modbus_rtu.hpp>

namespace relay {
    // All callbacks registered
    void on_read_coils(uint8_t addr, uint8_t qty);
    void on_set_single(uint8_t addr, uint16_t operation);
    void on_set_multiple(uint8_t operation);
    void on_read_info(uint8_t addr, uint8_t qty);
    void on_read_running_time();
    void on_read_relay_stats(uint8_t addr, uint8_t qty);
    void reset_device();
    void set_baud(uint16_t baud);
    void set_parity(uint16_t parity);
    void set_stopbits(uint16_t stopbits);
    void set_watchdog(uint16_t watchdog);
    void set_config(uint16_t baud, uint16_t parity, uint16_t stopbits);
    void on_ready_reply(std::string_view);

    // All states to consider
    enum class state_t : uint8_t {
        IGNORE = 0,
        ERROR = 1,
        DEVICE_ADDRESS,
        DEVICE_44,
        DEVICE_44_READ_COILS,
        DEVICE_44_READ_COILS_addr,
        DEVICE_44_READ_COILS_addr__ON_READ_COILS__CRC,
        RDY_TO_CALL__ON_READ_COILS,
        DEVICE_44_WRITE_SINGLE_COIL,
        DEVICE_44_WRITE_SINGLE_COIL_addr,
        DEVICE_44_WRITE_SINGLE_COIL_addr__ON_SET_SINGLE__CRC,
        RDY_TO_CALL__ON_SET_SINGLE,
        DEVICE_44_WRITE_MULTIPLE_COILS,
        DEVICE_44_WRITE_MULTIPLE_COILS_from,
        DEVICE_44_WRITE_MULTIPLE_COILS_from_qty,
        DEVICE_44_WRITE_MULTIPLE_COILS_from_qty_count,
        DEVICE_44_WRITE_MULTIPLE_COILS_from_qty_count__ON_SET_MULTIPLE__CRC,
        RDY_TO_CALL__ON_SET_MULTIPLE,
        DEVICE_44_WRITE_SINGLE_REGISTER,
        DEVICE_44_WRITE_SINGLE_REGISTER_1,
        DEVICE_44_WRITE_SINGLE_REGISTER_1__SET_BAUD__CRC,
        RDY_TO_CALL__SET_BAUD,
        DEVICE_44_WRITE_SINGLE_REGISTER_2,
        DEVICE_44_WRITE_SINGLE_REGISTER_2__SET_PARITY__CRC,
        RDY_TO_CALL__SET_PARITY,
        DEVICE_44_WRITE_SINGLE_REGISTER_3,
        DEVICE_44_WRITE_SINGLE_REGISTER_3__SET_STOPBITS__CRC,
        RDY_TO_CALL__SET_STOPBITS,
        DEVICE_44_WRITE_SINGLE_REGISTER_4,
        DEVICE_44_WRITE_SINGLE_REGISTER_4__SET_WATCHDOG__CRC,
        RDY_TO_CALL__SET_WATCHDOG,
        DEVICE_44_WRITE_MULTIPLE_REGISTERS,
        DEVICE_44_WRITE_MULTIPLE_REGISTERS_1,
        DEVICE_44_WRITE_MULTIPLE_REGISTERS_1_1,
        DEVICE_44_WRITE_MULTIPLE_REGISTERS_1_1_1,
        DEVICE_44_WRITE_MULTIPLE_REGISTERS_1_1_1__RESET_DEVICE__CRC,
        RDY_TO_CALL__RESET_DEVICE,
        DEVICE_44_WRITE_MULTIPLE_REGISTERS_2,
        DEVICE_44_WRITE_MULTIPLE_REGISTERS_2_1,
        DEVICE_44_WRITE_MULTIPLE_REGISTERS_2_1_1,
        DEVICE_44_WRITE_MULTIPLE_REGISTERS_2_1_1_baud,
        DEVICE_44_WRITE_MULTIPLE_REGISTERS_2_1_1_baud_parity,
        DEVICE_44_WRITE_MULTIPLE_REGISTERS_2_1_1_baud_parity__SET_CONFIG__CRC,
        RDY_TO_CALL__SET_CONFIG,
        DEVICE_44_READ_HOLDING_REGISTERS,
        DEVICE_44_READ_HOLDING_REGISTERS_1,
        DEVICE_44_READ_HOLDING_REGISTERS_1__ON_READ_INFO__CRC,
        RDY_TO_CALL__ON_READ_INFO,
        DEVICE_44_READ_HOLDING_REGISTERS_2,
        DEVICE_44_READ_HOLDING_REGISTERS_2__ON_READ_RUNNING_TIME__CRC,
        RDY_TO_CALL__ON_READ_RUNNING_TIME,
        DEVICE_44_READ_HOLDING_REGISTERS_3,
        DEVICE_44_READ_HOLDING_REGISTERS_3__ON_READ_RELAY_STATS__CRC,
        RDY_TO_CALL__ON_READ_RELAY_STATS
    };

    class Datagram {
        using error_t = asx::modbus::error_t;

        ///< Adjusted buffer to only receive the largest amount of data possible
        inline static uint8_t buffer[32];
        ///< Number of characters in the buffer
        inline static uint8_t cnt;
        ///< Number of characters to send
        inline static uint8_t frame_size;
        ///< Error code
        inline static error_t error;
        ///< State
        inline static state_t state;
        ///< CRC for the datagram
        inline static asx::modbus::Crc crc{};

        static inline auto ntoh(const uint8_t offset) -> uint16_t {
            return (static_cast<uint16_t>(buffer[offset]) << 8) | static_cast<uint16_t>(buffer[offset + 1]);
        }

        static inline auto ntohl(const uint8_t offset) -> uint32_t {
            return
                (static_cast<uint32_t>(buffer[offset]) << 24) |
                (static_cast<uint32_t>(buffer[offset+1]) << 16) |
                (static_cast<uint32_t>(buffer[offset+2]) << 8) |
                static_cast<uint16_t>(buffer[offset+3]);
        }

    public:
        // Status of the datagram
        enum class status_t : uint8_t {
            GOOD_FRAME = 0,
            NOT_FOR_ME = 1,
            BAD_CRC = 2
        };

        static void reset() noexcept {
            cnt=0;
            crc.reset();
            error = error_t::ok;
            state = state_t::DEVICE_ADDRESS;
        }

        static status_t get_status() noexcept {
            if (state == state_t::IGNORE) {
                return status_t::NOT_FOR_ME;
            }

            return crc.check() ? status_t::GOOD_FRAME : status_t::BAD_CRC;
        }

        static void process_char(const uint8_t c) noexcept {
            LOG_TRACE("DGRAM", "Char: 0x%.2x, index: %d, state: %d", c, cnt, (uint8_t)state);

            if (state == state_t::IGNORE) {
                return;
            }

            crc(c);

            if (state != state_t::ERROR) {
                // Store the frame
                buffer[cnt++] = c; // Store the data
            }

            switch(state) {
            case state_t::ERROR:
                break;
            case state_t::DEVICE_ADDRESS:
                if ( c == 44 ) {
                    state = state_t::DEVICE_44;
                } else {
                    error = error_t::ignore_frame;
                    state = state_t::IGNORE;
                }
                break;
            case state_t::DEVICE_44:
                if ( c == 1 ) {
                    state = state_t::DEVICE_44_READ_COILS;
                } else if ( c == 5 ) {
                    state = state_t::DEVICE_44_WRITE_SINGLE_COIL;
                } else if ( c == 15 ) {
                    state = state_t::DEVICE_44_WRITE_MULTIPLE_COILS;
                } else if ( c == 6 ) {
                    state = state_t::DEVICE_44_WRITE_SINGLE_REGISTER;
                } else if ( c == 16 ) {
                    state = state_t::DEVICE_44_WRITE_MULTIPLE_REGISTERS;
                } else if ( c == 3 ) {
                    state = state_t::DEVICE_44_READ_HOLDING_REGISTERS;
                } else {
                    error = error_t::illegal_function_code;
                    state = state_t::ERROR;
                }
                break;
            case state_t::DEVICE_44_READ_COILS:
                if ( cnt == 4 ) {
                    auto c = ntoh(cnt-2);

                    if ( c <= 2 ) {
                        state = state_t::DEVICE_44_READ_COILS_addr;
                    } else {
                        error = error_t::illegal_data_value;
                        state = state_t::ERROR;
                    };
                }
                break;
            case state_t::DEVICE_44_READ_COILS_addr:
                if ( cnt == 6 ) {
                    auto c = ntoh(cnt-2);

                    if ( c >= 1 and c <= 3 ) {
                        state = state_t::DEVICE_44_READ_COILS_addr__ON_READ_COILS__CRC;
                    } else {
                        error = error_t::illegal_data_value;
                        state = state_t::ERROR;
                    };
                }
                break;
            case state_t::DEVICE_44_READ_COILS_addr__ON_READ_COILS__CRC:
                if ( cnt == 8 ) {
                    state = state_t::RDY_TO_CALL__ON_READ_COILS;
                }
                break;
            case state_t::DEVICE_44_WRITE_SINGLE_COIL:
                if ( cnt == 4 ) {
                    auto c = ntoh(cnt-2);

                    if ( c <= 2 ) {
                        state = state_t::DEVICE_44_WRITE_SINGLE_COIL_addr;
                    } else {
                        error = error_t::illegal_data_value;
                        state = state_t::ERROR;
                    };
                }
                break;
            case state_t::DEVICE_44_WRITE_SINGLE_COIL_addr:
                if ( cnt == 6 ) {
                    auto c = ntoh(cnt-2);

                    if ( c == 0xff00 || c == 0x0 || c == 0x5500 ) {
                        state = state_t::DEVICE_44_WRITE_SINGLE_COIL_addr__ON_SET_SINGLE__CRC;
                    } else {
                        error = error_t::illegal_data_value;
                        state = state_t::ERROR;
                    };
                }
                break;
            case state_t::DEVICE_44_WRITE_SINGLE_COIL_addr__ON_SET_SINGLE__CRC:
                if ( cnt == 8 ) {
                    state = state_t::RDY_TO_CALL__ON_SET_SINGLE;
                }
                break;
            case state_t::DEVICE_44_WRITE_MULTIPLE_COILS:
                if ( cnt == 4 ) {
                    auto c = ntoh(cnt-2);

                    if ( c == 0 ) {
                        state = state_t::DEVICE_44_WRITE_MULTIPLE_COILS_from;
                    } else {
                        error = error_t::illegal_data_value;
                        state = state_t::ERROR;
                    };
                }
                break;
            case state_t::DEVICE_44_WRITE_MULTIPLE_COILS_from:
                if ( cnt == 6 ) {
                    auto c = ntoh(cnt-2);

                    if ( c == 3 ) {
                        state = state_t::DEVICE_44_WRITE_MULTIPLE_COILS_from_qty;
                    } else {
                        error = error_t::illegal_data_value;
                        state = state_t::ERROR;
                    };
                }
                break;
            case state_t::DEVICE_44_WRITE_MULTIPLE_COILS_from_qty:
                if ( c == 1 ) {
                    state = state_t::DEVICE_44_WRITE_MULTIPLE_COILS_from_qty_count;
                } else {
                    error = error_t::illegal_data_value;
                    state = state_t::ERROR;
                }
                break;
            case state_t::DEVICE_44_WRITE_MULTIPLE_COILS_from_qty_count:
                if ( c <= 7 ) {
                    state = state_t::DEVICE_44_WRITE_MULTIPLE_COILS_from_qty_count__ON_SET_MULTIPLE__CRC;
                } else {
                    error = error_t::illegal_data_value;
                    state = state_t::ERROR;
                }
                break;
            case state_t::DEVICE_44_WRITE_MULTIPLE_COILS_from_qty_count__ON_SET_MULTIPLE__CRC:
                if ( cnt == 10 ) {
                    state = state_t::RDY_TO_CALL__ON_SET_MULTIPLE;
                }
                break;
            case state_t::DEVICE_44_WRITE_SINGLE_REGISTER:
                if ( cnt == 4 ) {
                    auto c = ntoh(cnt-2);

                    if ( c == 4 ) {
                        state = state_t::DEVICE_44_WRITE_SINGLE_REGISTER_1;
                    } else if ( c == 5 ) {
                        state = state_t::DEVICE_44_WRITE_SINGLE_REGISTER_2;
                    } else if ( c == 6 ) {
                        state = state_t::DEVICE_44_WRITE_SINGLE_REGISTER_3;
                    } else if ( c == 7 ) {
                        state = state_t::DEVICE_44_WRITE_SINGLE_REGISTER_4;
                    } else {
                        error = error_t::illegal_data_value;
                        state = state_t::ERROR;
                    };
                }
                break;
            case state_t::DEVICE_44_WRITE_SINGLE_REGISTER_1:
                if ( cnt == 6 ) {
                    auto c = ntoh(cnt-2);

                    if ( c == 0x4b0 || c == 0x960 || c == 0x12c0 || c == 0x2580 || c == 0x4b00 || c == 0x9600 || c == 0xe100 || c == 0x1c200 ) {
                        state = state_t::DEVICE_44_WRITE_SINGLE_REGISTER_1__SET_BAUD__CRC;
                    } else {
                        error = error_t::illegal_data_value;
                        state = state_t::ERROR;
                    };
                }
                break;
            case state_t::DEVICE_44_WRITE_SINGLE_REGISTER_1__SET_BAUD__CRC:
                if ( cnt == 8 ) {
                    state = state_t::RDY_TO_CALL__SET_BAUD;
                }
                break;
            case state_t::DEVICE_44_WRITE_SINGLE_REGISTER_2:
                if ( cnt == 6 ) {
                    auto c = ntoh(cnt-2);

                    if ( c == 0x0 || c == 0x1 || c == 0x2 ) {
                        state = state_t::DEVICE_44_WRITE_SINGLE_REGISTER_2__SET_PARITY__CRC;
                    } else {
                        error = error_t::illegal_data_value;
                        state = state_t::ERROR;
                    };
                }
                break;
            case state_t::DEVICE_44_WRITE_SINGLE_REGISTER_2__SET_PARITY__CRC:
                if ( cnt == 8 ) {
                    state = state_t::RDY_TO_CALL__SET_PARITY;
                }
                break;
            case state_t::DEVICE_44_WRITE_SINGLE_REGISTER_3:
                if ( cnt == 6 ) {
                    auto c = ntoh(cnt-2);

                    if ( c == 0x0 || c == 0x1 ) {
                        state = state_t::DEVICE_44_WRITE_SINGLE_REGISTER_3__SET_STOPBITS__CRC;
                    } else {
                        error = error_t::illegal_data_value;
                        state = state_t::ERROR;
                    };
                }
                break;
            case state_t::DEVICE_44_WRITE_SINGLE_REGISTER_3__SET_STOPBITS__CRC:
                if ( cnt == 8 ) {
                    state = state_t::RDY_TO_CALL__SET_STOPBITS;
                }
                break;
            case state_t::DEVICE_44_WRITE_SINGLE_REGISTER_4:
                if ( cnt == 6 ) {
                    state = state_t::DEVICE_44_WRITE_SINGLE_REGISTER_4__SET_WATCHDOG__CRC;;
                }
                break;
            case state_t::DEVICE_44_WRITE_SINGLE_REGISTER_4__SET_WATCHDOG__CRC:
                if ( cnt == 8 ) {
                    state = state_t::RDY_TO_CALL__SET_WATCHDOG;
                }
                break;
            case state_t::DEVICE_44_WRITE_MULTIPLE_REGISTERS:
                if ( cnt == 4 ) {
                    auto c = ntoh(cnt-2);

                    if ( c == 0 ) {
                        state = state_t::DEVICE_44_WRITE_MULTIPLE_REGISTERS_1;
                    } else if ( c == 4 ) {
                        state = state_t::DEVICE_44_WRITE_MULTIPLE_REGISTERS_2;
                    } else {
                        error = error_t::illegal_data_value;
                        state = state_t::ERROR;
                    };
                }
                break;
            case state_t::DEVICE_44_WRITE_MULTIPLE_REGISTERS_1:
                if ( cnt == 6 ) {
                    auto c = ntoh(cnt-2);

                    if ( c == 2 ) {
                        state = state_t::DEVICE_44_WRITE_MULTIPLE_REGISTERS_1_1;
                    } else {
                        error = error_t::illegal_data_value;
                        state = state_t::ERROR;
                    };
                }
                break;
            case state_t::DEVICE_44_WRITE_MULTIPLE_REGISTERS_1_1:
                if ( c == 4 ) {
                    state = state_t::DEVICE_44_WRITE_MULTIPLE_REGISTERS_1_1_1;
                } else {
                    error = error_t::illegal_data_value;
                    state = state_t::ERROR;
                }
                break;
            case state_t::DEVICE_44_WRITE_MULTIPLE_REGISTERS_1_1_1:
                if ( cnt == 11 ) {
                    auto c = ntohl(cnt-4);

                    if ( c == 3735902974 ) {
                        state = state_t::DEVICE_44_WRITE_MULTIPLE_REGISTERS_1_1_1__RESET_DEVICE__CRC;
                    } else {
                        error = error_t::illegal_data_value;
                        state = state_t::ERROR;
                    };
                }
                break;
            case state_t::DEVICE_44_WRITE_MULTIPLE_REGISTERS_1_1_1__RESET_DEVICE__CRC:
                if ( cnt == 13 ) {
                    state = state_t::RDY_TO_CALL__RESET_DEVICE;
                }
                break;
            case state_t::DEVICE_44_WRITE_MULTIPLE_REGISTERS_2:
                if ( cnt == 6 ) {
                    auto c = ntoh(cnt-2);

                    if ( c == 3 ) {
                        state = state_t::DEVICE_44_WRITE_MULTIPLE_REGISTERS_2_1;
                    } else {
                        error = error_t::illegal_data_value;
                        state = state_t::ERROR;
                    };
                }
                break;
            case state_t::DEVICE_44_WRITE_MULTIPLE_REGISTERS_2_1:
                if ( c == 6 ) {
                    state = state_t::DEVICE_44_WRITE_MULTIPLE_REGISTERS_2_1_1;
                } else {
                    error = error_t::illegal_data_value;
                    state = state_t::ERROR;
                }
                break;
            case state_t::DEVICE_44_WRITE_MULTIPLE_REGISTERS_2_1_1:
                if ( cnt == 9 ) {
                    state = state_t::DEVICE_44_WRITE_MULTIPLE_REGISTERS_2_1_1_baud;;
                }
                break;
            case state_t::DEVICE_44_WRITE_MULTIPLE_REGISTERS_2_1_1_baud:
                if ( cnt == 11 ) {
                    state = state_t::DEVICE_44_WRITE_MULTIPLE_REGISTERS_2_1_1_baud_parity;;
                }
                break;
            case state_t::DEVICE_44_WRITE_MULTIPLE_REGISTERS_2_1_1_baud_parity:
                if ( cnt == 13 ) {
                    state = state_t::DEVICE_44_WRITE_MULTIPLE_REGISTERS_2_1_1_baud_parity__SET_CONFIG__CRC;;
                }
                break;
            case state_t::DEVICE_44_WRITE_MULTIPLE_REGISTERS_2_1_1_baud_parity__SET_CONFIG__CRC:
                if ( cnt == 15 ) {
                    state = state_t::RDY_TO_CALL__SET_CONFIG;
                }
                break;
            case state_t::DEVICE_44_READ_HOLDING_REGISTERS:
                if ( cnt == 4 ) {
                    auto c = ntoh(cnt-2);

                    if ( c <= 7 ) {
                        state = state_t::DEVICE_44_READ_HOLDING_REGISTERS_1;
                    } else if ( c == 100 ) {
                        state = state_t::DEVICE_44_READ_HOLDING_REGISTERS_2;
                    } else if ( c >= 200 and c <= 202 ) {
                        state = state_t::DEVICE_44_READ_HOLDING_REGISTERS_3;
                    } else {
                        error = error_t::illegal_data_value;
                        state = state_t::ERROR;
                    };
                }
                break;
            case state_t::DEVICE_44_READ_HOLDING_REGISTERS_1:
                if ( cnt == 6 ) {
                    auto c = ntoh(cnt-2);

                    if ( c >= 1 and c <= 8 ) {
                        state = state_t::DEVICE_44_READ_HOLDING_REGISTERS_1__ON_READ_INFO__CRC;
                    } else {
                        error = error_t::illegal_data_value;
                        state = state_t::ERROR;
                    };
                }
                break;
            case state_t::DEVICE_44_READ_HOLDING_REGISTERS_1__ON_READ_INFO__CRC:
                if ( cnt == 8 ) {
                    state = state_t::RDY_TO_CALL__ON_READ_INFO;
                }
                break;
            case state_t::DEVICE_44_READ_HOLDING_REGISTERS_2:
                if ( cnt == 6 ) {
                    auto c = ntoh(cnt-2);

                    if ( c == 2 ) {
                        state = state_t::DEVICE_44_READ_HOLDING_REGISTERS_2__ON_READ_RUNNING_TIME__CRC;
                    } else {
                        error = error_t::illegal_data_value;
                        state = state_t::ERROR;
                    };
                }
                break;
            case state_t::DEVICE_44_READ_HOLDING_REGISTERS_2__ON_READ_RUNNING_TIME__CRC:
                if ( cnt == 8 ) {
                    state = state_t::RDY_TO_CALL__ON_READ_RUNNING_TIME;
                }
                break;
            case state_t::DEVICE_44_READ_HOLDING_REGISTERS_3:
                if ( cnt == 6 ) {
                    auto c = ntoh(cnt-2);

                    if ( c >= 2 and c <= 6 ) {
                        state = state_t::DEVICE_44_READ_HOLDING_REGISTERS_3__ON_READ_RELAY_STATS__CRC;
                    } else {
                        error = error_t::illegal_data_value;
                        state = state_t::ERROR;
                    };
                }
                break;
            case state_t::DEVICE_44_READ_HOLDING_REGISTERS_3__ON_READ_RELAY_STATS__CRC:
                if ( cnt == 8 ) {
                    state = state_t::RDY_TO_CALL__ON_READ_RELAY_STATS;
                }
                break;
            case state_t::RDY_TO_CALL__ON_READ_COILS:
            case state_t::RDY_TO_CALL__ON_SET_SINGLE:
            case state_t::RDY_TO_CALL__ON_SET_MULTIPLE:
            case state_t::RDY_TO_CALL__SET_BAUD:
            case state_t::RDY_TO_CALL__SET_PARITY:
            case state_t::RDY_TO_CALL__SET_STOPBITS:
            case state_t::RDY_TO_CALL__SET_WATCHDOG:
            case state_t::RDY_TO_CALL__RESET_DEVICE:
            case state_t::RDY_TO_CALL__SET_CONFIG:
            case state_t::RDY_TO_CALL__ON_READ_INFO:
            case state_t::RDY_TO_CALL__ON_READ_RUNNING_TIME:
            case state_t::RDY_TO_CALL__ON_READ_RELAY_STATS:
            default:
                error = error_t::illegal_data_value;
                state = state_t::ERROR;
                break;
            }
        }

        static void reply_error( error_t err ) noexcept {
            buffer[1] |= 0x80;
            buffer[2] = (uint8_t)err;
            cnt = 3;
        }

        template<typename T>
        static void pack(const T& value) noexcept {
            if constexpr ( sizeof(T) == 1 ) {
                buffer[cnt++] = value;
            } else if constexpr ( sizeof(T) == 2 ) {
                buffer[cnt++] = value >> 8;
                buffer[cnt++] = value & 0xff;
            } else if constexpr ( sizeof(T) == 4 ) {
                buffer[cnt++] = value >> 24;
                buffer[cnt++] = value >> 16 & 0xff;
                buffer[cnt++] = value >> 8 & 0xff;
                buffer[cnt++] = value & 0xff;
            }
        }

        static inline void set_size(uint8_t size) {
            cnt = size;
        }

        /** Called when a T3.5 has been detected, in a good sequence */
        static void ready_reply() noexcept {
            frame_size = cnt; // Store the frame size
            cnt = 2; // Points to the function code
            on_ready_reply(std::string_view{(char *)buffer, cnt});

            switch(state) {
            case state_t::IGNORE:
                break;
            case state_t::DEVICE_ADDRESS:
            case state_t::DEVICE_44:
            case state_t::DEVICE_44_READ_COILS:
            case state_t::DEVICE_44_READ_COILS_addr:
            case state_t::DEVICE_44_READ_COILS_addr__ON_READ_COILS__CRC:
            case state_t::DEVICE_44_WRITE_SINGLE_COIL:
            case state_t::DEVICE_44_WRITE_SINGLE_COIL_addr:
            case state_t::DEVICE_44_WRITE_SINGLE_COIL_addr__ON_SET_SINGLE__CRC:
            case state_t::DEVICE_44_WRITE_MULTIPLE_COILS:
            case state_t::DEVICE_44_WRITE_MULTIPLE_COILS_from:
            case state_t::DEVICE_44_WRITE_MULTIPLE_COILS_from_qty:
            case state_t::DEVICE_44_WRITE_MULTIPLE_COILS_from_qty_count:
            case state_t::DEVICE_44_WRITE_MULTIPLE_COILS_from_qty_count__ON_SET_MULTIPLE__CRC:
            case state_t::DEVICE_44_WRITE_SINGLE_REGISTER:
            case state_t::DEVICE_44_WRITE_SINGLE_REGISTER_1:
            case state_t::DEVICE_44_WRITE_SINGLE_REGISTER_1__SET_BAUD__CRC:
            case state_t::DEVICE_44_WRITE_SINGLE_REGISTER_2:
            case state_t::DEVICE_44_WRITE_SINGLE_REGISTER_2__SET_PARITY__CRC:
            case state_t::DEVICE_44_WRITE_SINGLE_REGISTER_3:
            case state_t::DEVICE_44_WRITE_SINGLE_REGISTER_3__SET_STOPBITS__CRC:
            case state_t::DEVICE_44_WRITE_SINGLE_REGISTER_4:
            case state_t::DEVICE_44_WRITE_SINGLE_REGISTER_4__SET_WATCHDOG__CRC:
            case state_t::DEVICE_44_WRITE_MULTIPLE_REGISTERS:
            case state_t::DEVICE_44_WRITE_MULTIPLE_REGISTERS_1:
            case state_t::DEVICE_44_WRITE_MULTIPLE_REGISTERS_1_1:
            case state_t::DEVICE_44_WRITE_MULTIPLE_REGISTERS_1_1_1:
            case state_t::DEVICE_44_WRITE_MULTIPLE_REGISTERS_1_1_1__RESET_DEVICE__CRC:
            case state_t::DEVICE_44_WRITE_MULTIPLE_REGISTERS_2:
            case state_t::DEVICE_44_WRITE_MULTIPLE_REGISTERS_2_1:
            case state_t::DEVICE_44_WRITE_MULTIPLE_REGISTERS_2_1_1:
            case state_t::DEVICE_44_WRITE_MULTIPLE_REGISTERS_2_1_1_baud:
            case state_t::DEVICE_44_WRITE_MULTIPLE_REGISTERS_2_1_1_baud_parity:
            case state_t::DEVICE_44_WRITE_MULTIPLE_REGISTERS_2_1_1_baud_parity__SET_CONFIG__CRC:
            case state_t::DEVICE_44_READ_HOLDING_REGISTERS:
            case state_t::DEVICE_44_READ_HOLDING_REGISTERS_1:
            case state_t::DEVICE_44_READ_HOLDING_REGISTERS_1__ON_READ_INFO__CRC:
            case state_t::DEVICE_44_READ_HOLDING_REGISTERS_2:
            case state_t::DEVICE_44_READ_HOLDING_REGISTERS_2__ON_READ_RUNNING_TIME__CRC:
            case state_t::DEVICE_44_READ_HOLDING_REGISTERS_3:
            case state_t::DEVICE_44_READ_HOLDING_REGISTERS_3__ON_READ_RELAY_STATS__CRC:
                error = error_t::illegal_data_value;
            case state_t::ERROR:
                buffer[1] |= 0x80; // Mark the error
                buffer[2] = (uint8_t)error; // Add the error code
                cnt = 3;
                break;
            case state_t::RDY_TO_CALL__ON_READ_COILS:
                on_read_coils(buffer[3], buffer[5]);
                break;
            case state_t::RDY_TO_CALL__ON_SET_SINGLE:
                on_set_single(buffer[3], ntoh(4));
                break;
            case state_t::RDY_TO_CALL__ON_SET_MULTIPLE:
                on_set_multiple(buffer[7]);
                break;
            case state_t::RDY_TO_CALL__SET_BAUD:
                set_baud(ntoh(4));
                break;
            case state_t::RDY_TO_CALL__SET_PARITY:
                set_parity(ntoh(4));
                break;
            case state_t::RDY_TO_CALL__SET_STOPBITS:
                set_stopbits(ntoh(4));
                break;
            case state_t::RDY_TO_CALL__SET_WATCHDOG:
                set_watchdog(ntoh(4));
                break;
            case state_t::RDY_TO_CALL__RESET_DEVICE:
                reset_device();
                break;
            case state_t::RDY_TO_CALL__SET_CONFIG:
                set_config(ntoh(7), ntoh(9), ntoh(11));
                break;
            case state_t::RDY_TO_CALL__ON_READ_INFO:
                on_read_info(buffer[3], buffer[5]);
                break;
            case state_t::RDY_TO_CALL__ON_READ_RUNNING_TIME:
                on_read_running_time();
                break;
            case state_t::RDY_TO_CALL__ON_READ_RELAY_STATS:
                on_read_relay_stats(buffer[3], buffer[5]);
                break;
            default:
                break;
            }

            // If the cnt is 2 - nothing was changed in the buffer - return it as is
            if ( cnt == 2 ) {
                // Framesize includes the previous CRC which still holds valid
                cnt = frame_size;
            } else {
                // Add the CRC
                crc.reset();
                auto _crc = crc.update(std::string_view{(char *)buffer, cnt});
                buffer[cnt++] = _crc & 0xff;
                buffer[cnt++] = _crc >> 8;
            }
        }

        static std::string_view get_buffer() noexcept {
            // Return the buffer ready to send
            return std::string_view{(char *)buffer, cnt};
        }
    }; // struct Processor
} // namespace modbus