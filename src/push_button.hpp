#pragma once
/**
 * @file sw.hpp
 * @brief Manages the configuration sw button.
 * It used timer1 to measure the pulse width in conjuction with the PIT timer.
 */
#include <cstdint>
#include <chrono>

#include <asx/reactor.hpp>
#include <asx/timer.hpp>
#include <asx/debouncer.hpp>
#include <asx/ioport.hpp>
#include <asx/ulog.hpp>

#include "net.hpp"
#include "state.hpp"
#include "estop.hpp"

#include "conf_board.h"


namespace sw {
   constexpr auto sampling_period = std::chrono::milliseconds(20);
   constexpr auto debounce_time = std::chrono::milliseconds(40);
   constexpr auto long_time = std::chrono::seconds(3);

   auto react_on_sw = asx::reactor::Handle{};

   // Sample the push button every 10ms and debounce it to 40ms
   auto debouncer = asx::Debouncer<1, debounce_time / sampling_period>{};

   static inline void init() {
      using namespace asx::ioport;

      ULOG_MILE("Initializing Push Button");

      // Set the pin to input
      PUSH_BUTTON.init( dir_t::in, invert::inverted, pullup::enabled );

      // Start the switch sampling
      asx::reactor::bind([]() {
         constexpr auto time_zero =
            asx::timer::steady_clock::time_point(asx::timer::steady_clock::duration::zero());
         static auto last_time = time_zero;
         static bool recovery_triggered = false;

         // Sample the switch
         debouncer.append(*PUSH_BUTTON);

         if (debouncer.status().get()) {
            if (last_time == time_zero) {
               last_time = asx::timer::steady_clock::now();
               recovery_triggered = false;
            } else if (!recovery_triggered) {
               auto now = asx::timer::steady_clock::now();
               auto duration = now - last_time;

               if (duration >= long_time) {
                  auto is_in_recovery = state::is_in_recovery_mode();
                  state::set_recovery_mode(!is_in_recovery);
                  recovery_triggered = true;

                  ULOG_INFO("Recovery mode long push detected. State is {}", !is_in_recovery);
               }
            }
         } else {
            if (last_time != time_zero) {
               if (!recovery_triggered) {
                  ULOG_INFO("Resetting the EStop");
                  // Short press: Reset any on-going EStop
                  estop::reset();
               }

               last_time = time_zero;
               recovery_triggered = false;
            }
         }
      }).repeat(sampling_period);
   }
} // namespace sw
