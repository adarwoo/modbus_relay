#pragma once
/**
 * @file estop.hpp
 * @brief Emergency Stop (EStop) system interface.
 *
 * This module provides an interface for managing the emergency stop system,
 * including triggering, resetting, and querying the status and cause of the EStop.
 */

#include <stdint.h>

namespace estop {
   /** @brief The status of the EStop system */
   enum class Status {
      operational = 0,
      estop       = 1,
      terminated  = 2
   };

   enum class Cause {
      none = 0,
      faulty_relay = 1 << 0,
      modbus_watchdog = 1 << 1,
      infeed_polarity = 1 << 2,
      infeed_voltage_type = 1 << 3,
      infeed_voltage_under = 1 << 4,
      infeed_voltage_over = 1 << 5,
      command = 1 << 6,
      crash = 1 << 8,
      eeprom = 1 << 9,
      bod = 1 << 10,
   };

   enum class ExternalTriggerType : uint8_t {
      none = 0,
      pulse = 0x11,
      resetable = 0x22,
      terminal = 0xff
   };

   namespace detail {
      inline Status current_status = Status::operational;
      inline Cause current_cause = Cause::none;
      inline uint16_t diagnostic = 0;
   }

   /**
    * Initialize the EStop system.
    * This function should be called once at the start of the program.
    */
   void init();

   /**
    * Activate the EStop system with a specific cause.
    */
   void trigger(
      Cause cause,
      uint16_t diagnostic,
      ExternalTriggerType type = ExternalTriggerType::none
   );

   /**
    * Attempt to reset the EStop system.
    */
   void reset();

   /**
    * Get the current status of the EStop system.
    * @return The current status.
    */
   inline Status get_status() {
      return detail::current_status;
   }

   /**
    * Set the current status of the EStop system.
    * @param new_status The new status to set.
    */
   inline Cause get_cause() {
      return detail::current_cause;
   }

   /**
    * Get the diagnostic code associated with the EStop system.
    * @return The diagnostic code.
    */
   inline uint8_t get_diagnostic_code() {
      return detail::diagnostic;
   }
} // namespace estop
