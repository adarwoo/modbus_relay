#pragma once
/** @file counters.hpp
 *  @brief Counter management for relay operations
 *  @details This file provides an interface for managing operation counters
 *           for relays, including incrementing counters and retrieving their
 *           values.
 */
#include <cstdint>
#include <array>

#include <asx/ulog.hpp>
#include <asx/eeprom.hpp>

#include <avr/io.h>

namespace counter {
   namespace detail {
      using Counter = asx::eeprom::Counter;

      // Use the banks 0, 1 and 2 for the 3 relays
      auto inline relay_cycles_counters = std::array<Counter, 3>{0,1,2};

      // Use bank 3 for the operational count
      auto inline running_minutes = Counter(3);
   }

   /** Increment the operation counter for a specific relay index */
   inline void increment(uint8_t index) {
      detail::relay_cycles_counters[index].increment();
   }

   /** Get the operation count for a specific relay index */
   inline uint32_t get(uint8_t index) {
      return detail::relay_cycles_counters[index].get_count();
   }

   /** Get the total running minutes since the last reset */
   inline uint32_t get_running_minutes() {
      return detail::running_minutes.get_count();
   }

   /**
    * Initialize the counter API.
    * This function should be called once at the start of the program.
    * It sets up the RTC to generate an interrupt every minute, which will
    * be used to increment the running_minutes counter.
    * The RTC is configured to run in standby mode and uses a prescaler to
    * generate a 1/4 second tick. The period is set to 240 ticks, which results
    * in a 60 second period.
    * The RTC interrupt is enabled to handle the overflow event.
    */
   inline void init() {
      ULOG_MILE("Initialising Counters");

      // Initialise the PIT - turn the interrupt on
      RTC.CLKSEL = RTC_CLKSEL_INT32K_gc;
      RTC.CTRLA = RTC_RUNSTDBY_bm | RTC_PRESCALER_DIV8192_gc | RTC_RTCEN_bm; // 1/4 seconds
      RTC.PER = 240; // 32768 / 8192 / 4*60 = 60 seconds
      RTC.INTCTRL = RTC_OVF_bm;
   }
}
