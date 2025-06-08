#pragma once

#include <stdint.h>

namespace relay {
   namespace estop {

      enum class Status {
         operational = 0,
         estop       = 1,
         terminated  = 2
      };

      enum class Cause {
         none,
         faulty_relay,
         modbus_watchdog,
         voltage_monitor,
         command
      };

      namespace {
         static inline auto status = Status{Status::operational};
         static inline auto cause  = Cause{Cause::none};
         static inline auto diagnostic_code = uint8_t{0};
      }

      inline Status get_status() {
         return status;
      }

      inline uint16_t get_status_value() {
         return static_cast<uint16_t>(status);
      }

      inline void set_status(const Status new_status) {
         status = new_status;
      }

      inline uint8_t get_diagnostic_code() {
         return diagnostic_code;
      }

      inline Cause get_cause() {
         return cause;
      }

      inline void set_cause(Cause new_cause) {
         cause = new_cause;
      }

      inline uint16_t get_cause_value() {
         return static_cast<uint16_t>(cause);
      }
   } // namespace estop
} // namespace relay
