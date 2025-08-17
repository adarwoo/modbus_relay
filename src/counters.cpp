/** @file counters.cpp
 *  @brief Counter management for relay operations
 *  @details Provides the interrupt handler for the RTC to manage counters
 *           and the reactor to the interrupt.
 */
#include <avr/interrupt.h>

#include <asx/reactor.hpp>
#include <asx/ulog.hpp>

#include "counters.hpp"


namespace counter {
   namespace {
      /** React to minute elapsed events, signaled from the RTC interrupt */
      auto react_on_minute_elapsed = asx::reactor::bind(
         []() {
            ULOG_INFO("Minute elapsed");
            detail::running_minutes.increment();
         }
      );
   }
}

/** RTC Interrupt called every minute */
ISR(RTC_CNT_vect) {
   RTC.INTFLAGS = RTC_PI_bm;

   counter::react_on_minute_elapsed();
}