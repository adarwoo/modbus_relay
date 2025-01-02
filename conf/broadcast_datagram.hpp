/**
 * This file was generated to create a state machine for processing
 * uart data used for a modbus RTU. It should be included by
 * the modbus_rtu_slave.cpp file only which will create a full rtu slave device.
 */
#include <logger.h>
#include <stdint.h>
#include <asx/modbus_rtu.hpp>

namespace broadcast {
    // All callbacks registered
    void on_reset();

    // All states to consider
    enum class state_t : uint8_t {
        IGNORE = 0,
        ERROR = 1,
        DEVICE_ADDRESS,
        DEVICE_0,
        DEVICE_0_WRITE_MULTIPLE_REGISTERS,
        DEVICE_0_WRITE_MULTIPLE_REGISTERS_1,
        DEVICE_0_WRITE_MULTIPLE_REGISTERS_1_1,
        DEVICE_0_WRITE_MULTIPLE_REGISTERS_1_1_1,
        DEVICE_0_WRITE_MULTIPLE_REGISTERS_1_1_1__ON_RESET__CRC,
        RDY_TO_CALL__ON_RESET
    };

    class Datagram {
        using error_t = asx::modbus::error_t;

        ///< Adjusted buffer to only receive the largest amount of data possible
        inline static uint8_t buffer[14];
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
                if ( c == 0 ) {
                    state = state_t::DEVICE_0;
                } else {
                    error = error_t::ignore_frame;
                    state = state_t::IGNORE;
                }
                break;
            case state_t::DEVICE_0:
                if ( c == 16 ) {
                    state = state_t::DEVICE_0_WRITE_MULTIPLE_REGISTERS;
                } else {
                    error = error_t::illegal_function_code;
                    state = state_t::ERROR;
                }
                break;
            case state_t::DEVICE_0_WRITE_MULTIPLE_REGISTERS:
                if ( cnt == 4 ) {
                    auto c = ntoh(cnt-2);

                    if ( c == 99 ) {
                        state = state_t::DEVICE_0_WRITE_MULTIPLE_REGISTERS_1;
                    } else {
                        error = error_t::illegal_data_value;
                        state = state_t::ERROR;
                    };
                }
                break;
            case state_t::DEVICE_0_WRITE_MULTIPLE_REGISTERS_1:
                if ( cnt == 6 ) {
                    auto c = ntoh(cnt-2);

                    if ( c == 2 ) {
                        state = state_t::DEVICE_0_WRITE_MULTIPLE_REGISTERS_1_1;
                    } else {
                        error = error_t::illegal_data_value;
                        state = state_t::ERROR;
                    };
                }
                break;
            case state_t::DEVICE_0_WRITE_MULTIPLE_REGISTERS_1_1:
                if ( c == 4 ) {
                    state = state_t::DEVICE_0_WRITE_MULTIPLE_REGISTERS_1_1_1;
                } else {
                    error = error_t::illegal_data_value;
                    state = state_t::ERROR;
                }
                break;
            case state_t::DEVICE_0_WRITE_MULTIPLE_REGISTERS_1_1_1:
                if ( cnt == 11 ) {
                    auto c = ntohl(cnt-4);

                    if ( c == 3735902974 ) {
                        state = state_t::DEVICE_0_WRITE_MULTIPLE_REGISTERS_1_1_1__ON_RESET__CRC;
                    } else {
                        error = error_t::illegal_data_value;
                        state = state_t::ERROR;
                    };
                }
                break;
            case state_t::DEVICE_0_WRITE_MULTIPLE_REGISTERS_1_1_1__ON_RESET__CRC:
                if ( cnt == 13 ) {
                    state = state_t::RDY_TO_CALL__ON_RESET;
                }
                break;
            case state_t::RDY_TO_CALL__ON_RESET:
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
            

            switch(state) {
            case state_t::IGNORE:
                break;
            case state_t::DEVICE_ADDRESS:
            case state_t::DEVICE_0:
            case state_t::DEVICE_0_WRITE_MULTIPLE_REGISTERS:
            case state_t::DEVICE_0_WRITE_MULTIPLE_REGISTERS_1:
            case state_t::DEVICE_0_WRITE_MULTIPLE_REGISTERS_1_1:
            case state_t::DEVICE_0_WRITE_MULTIPLE_REGISTERS_1_1_1:
            case state_t::DEVICE_0_WRITE_MULTIPLE_REGISTERS_1_1_1__ON_RESET__CRC:
                error = error_t::illegal_data_value;
            case state_t::ERROR:
                buffer[1] |= 0x80; // Mark the error
                buffer[2] = (uint8_t)error; // Add the error code
                cnt = 3;
                break;
            case state_t::RDY_TO_CALL__ON_RESET:
                on_reset();
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