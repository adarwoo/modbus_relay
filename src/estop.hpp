#pragma once

#include <stdint.h>

namespace estop {
   enum class Status {
      operational = 0,
      estop       = 1,
      terminated  = 2
   };

   enum class Cause {
      none = 0,
      faulty_relay = 1,
      modbus_watchdog = 2,
      voltage_monitor = 3,
      command = 4
   };

   enum class ExternalTriggerType {
      reset = 0,
      pulse = 0x11,
      resetable = 0x22,
      terminal = 0xff
   };

   void init();
   Status get_status();
   void set_status(const Status new_status);
   uint8_t get_diagnostic_code();
   Cause get_cause();
   void trigger(ExternalTriggerType trigger, uint8_t diagnostic);
   void trigger(Cause cause);
} // namespace estop
