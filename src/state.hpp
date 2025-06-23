#pragma once
/**
 * @file state.hpp
 * @brief Manages the device state including recovery mode, locate mode, and fault modes.
 *
 * This module provides functions to check and set the device's operational modes,
 * including recovery mode, locate mode, and fault modes.
 */

#include <cstdbool>
#include <chrono>
#include <asx/reactor.hpp>

#include "leds.hpp"
#include "net.hpp"

namespace state {
   namespace detail {
      inline bool recovery_mode = false;
      inline bool locate_mode = false;

      inline auto on_reset_network = asx::reactor::bind(
         [] { net::init(); }
      );
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
      led::refresh();
   }
} // End of state namespace
