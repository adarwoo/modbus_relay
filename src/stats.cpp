#include "stats.hpp"

#include <array>
#include <asx/reactor.hpp>
#include <asx/eeprom.hpp>

#include <avr/interrupt.h>

using namespace asx;

namespace {
   // Use the banks 0, 1 and 2 for the 3 relays
   auto op_counters = std::array<eeprom::Counter, 3>{0,1,2};

   // Use bank 3 for the operational count
   auto counter_minutes = eeprom::Counter(3);

   void on_minutes_elapsed() {
      counter_minutes.increment();
   }

   auto react_on_minute_elapsed = reactor::bind(on_minutes_elapsed);
}

namespace relay {
   namespace stat {

      void increment_op(uint8_t index) {
         op_counters[index].increment();
      }

      uint32_t get_op_count(uint8_t index) {
         return op_counters[index].get_count();
      }

      uint32_t get_running_minutes() {
         return counter_minutes.get_count();
      }

      void init() {
         // Initialise the PIT - turn the interrupt on
         RTC.CLKSEL = RTC_CLKSEL_INT32K_gc;
         RTC.CTRLA = RTC_RUNSTDBY_bm | RTC_PRESCALER_DIV8192_gc | RTC_RTCEN_bm; // 1/4 seconds
         RTC.PER = 240; // 32768 / 8192 / 4*60 = 60 seconds
         RTC.INTCTRL = RTC_OVF_bm;
      }

   }
}

// Called every minute
ISR(RTC_CNT_vect) {
    RTC.INTFLAGS = RTC_PI_bm;

    react_on_minute_elapsed();
}