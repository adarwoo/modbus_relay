#pragma once
/**
 * @file sw.hpp
 * @brief Manages the configuration sw button.
 * It used timer1 to measure the pulse width in conjuction with the PIT timer.
 */
#include <cstdint>
#include <chrono>

 #include <asx/reactor.hpp>
#include <asx/debouncer.hpp>
#include <asx/ioport.hpp>

#include "conf_board.h"


namespace relay {
   namespace sw {
      constexpr auto sampling_period = std::chrono::milliseconds(20);
      constexpr auto debounce_time = std::chrono::seconds(3);

      auto react_on_sw = asx::reactor::Handle{};

      // Sample the push button every 20ms and debounce it to 3s
      auto debouncer = asx::Debouncer<1, debounce_time / sampling_period>{};

      static inline void init(asx::reactor::Handle react_on_sw_pushed) {
         react_on_sw = react_on_sw_pushed;

         // Set the pin to input
         PUSH_BUTTON.init(
            asx::ioport::dir_t::in,
            asx::ioport::invert::inverted,
            asx::ioport::pullup::enabled
         );

         // Start the switch sampling
         asx::reactor::bind([]() {
            // Sample the switch
            debouncer.append(*PUSH_BUTTON);

            // If the switch is pressed, react
            if (debouncer.status().get()) {
               react_on_sw.notify();
            }
         }).repeat(sampling_period);
      }
   } // namespace sw
} // namespace relay