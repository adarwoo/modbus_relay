#pragma once
/**
 * @file state.hpp
 * @brief Manages the device state including recovery mode, locate mode, and fault modes.
 *
 * This module provides functions to check and set the device's operational modes,
 * including recovery mode, locate mode, and fault modes.
 */
#include <avr/wdt.h>
#include <cstdbool>
#include <chrono>

#include <asx/reactor.hpp>

#include "leds.hpp"
#include "net.hpp"

namespace state {
   /**
    * Global fault indicator
    * Represents various fault conditions in the system.
    * Bit 0: One or more relays have faults. Check individual relay status.
    * Bit 2: Infeed polarity inverted
    * Bit 3: Incorrect infeed voltage type
    * Bit 4: Infeed voltage is under
    * Bit 5: Infeed voltage is over
    * Bit 8: Application crash detected
    * Bit 9: EEprom recovered
    * Bit 10: Supply voltage failure
   */
   namespace fault {
      constexpr uint16_t relay_fault             = 1 << 0;
      constexpr uint16_t infeed_polarity_inverted = 1 << 2;
      constexpr uint16_t infeed_bad_type         = 1 << 3;
      constexpr uint16_t infeed_under_voltage    = 1 << 4;
      constexpr uint16_t infeed_over_voltage     = 1 << 5;
      constexpr uint16_t external_estop          = 1 << 6;
      constexpr uint16_t app_crash               = 1 << 8;
      constexpr uint16_t eeprom_recovered        = 1 << 9;
      constexpr uint16_t supply_voltage_failure  = 1 << 10;
   }

   namespace detail {
      inline bool recovery_mode = false;
      inline bool locate_mode = false;

      inline auto on_reset_network = asx::reactor::bind(
         [] { net::init(); }
      );

      inline uint16_t current_faults{};
   }

   /** Get the current fault indicators */
   inline uint16_t get_faults() {
      return detail::current_faults;
   }

   /** Append a fault indicator */
   inline void append_faults(uint16_t faults) {
      detail::current_faults |= faults;
   }

   /** Clear all fault indicators */
   inline void clear_faults(uint16_t faults) {
      detail::current_faults &= ~faults;
   }

   /** Check if the device is in recovery mode */
   inline bool is_in_recovery_mode() {
      return detail::recovery_mode;
   }

   /** Set the value. Return true on a false->true transition */
   inline void set_recovery_mode(bool _recovery_mode) {
      // Implementation here
      if (detail::recovery_mode != _recovery_mode) {
         detail::recovery_mode = _recovery_mode;

         // Delay through the reactor so the UART can complete any on-going messages
         detail::on_reset_network.delay(std::chrono::milliseconds(100));
         led::refresh();
      }
   }

   /** Check if the device is in locate mode */
   inline bool is_in_locate_mode() {
      // Implementation here
      return detail::locate_mode;
   }

   /** Set the locate mode */
   inline void set_locate_mode(bool locate_mode) {
      detail::locate_mode = locate_mode;
      led::toggle_locate_mode( locate_mode );
   }

   /** Initialize the state manager */
   inline void init() {
      // Watchdog reset check
      if ( RSTCTRL.RSTFR & RSTCTRL_WDRF_bm ) {
         append_faults( fault::app_crash );
      }

      // Check the BOD reset flag
      if ( RSTCTRL.RSTFR & RSTCTRL_BORF_bm ) {
         append_faults( fault::supply_voltage_failure );
      }
   }
} // End of state namespace
