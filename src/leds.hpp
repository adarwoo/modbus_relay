#pragma once
/**
 * @file leds.hpp
 * @brief Control of the relay module LEDs
 */
#include <cstdint>

#include <asx/reactor.hpp>

namespace led {
   namespace detail {
      ///< Reactor to refresh the LED status
      inline auto react_on_refresh = asx::reactor::null;
   } // namespace detail

   /// @brief Initialise the LEDs
   void init();

   /// @brief Force to re-evaluate the LED states
   inline void refresh() {
      detail::react_on_refresh();
   };

   /// @brief Toggle the locate mode LED
   /// @param locate_mode
   void toggle_locate_mode(bool locate_mode);
} // namespace led
