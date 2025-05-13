/**
 * @file relay_ctrl.cpp
 * Relay control functions
 * @author software@arreckx.com
 *
 * @description
 *  This file contains the relay control functions. The relays are controlled
 *  by GPIO pins.
 * The relay used are safety relays with a normally closed contact. When switching
 *  a relay, the matching CHECK contact is read to check if the relay is closed.
 * If a discrepancy is detected, a notification is sent to the main application.
 */
#include <asx/reactor.hpp>
#include <asx/ioport.hpp>

#include "relay_ctrl.hpp"
#include "stats.hpp"

#include "conf_board.h"


namespace relay
{
   using namespace asx::ioport;

   // The handle to the timer used for the background check
   auto timer = asx::reactor::Handle{};

   // The handle to the error notification
   auto on_error = asx::reactor::Handle{};

   /** When constructed, the LED is ON to test it */
   void init( /*asx::reactor::Handle on_error_callback*/ )
   {
      // Register the error callback
      //on_error = on_error_callback;

      Pin(LED_A).init(value_t::high,  dir_t::out);
      Pin(LED_B).init(value_t::high,  dir_t::out);
      Pin(LED_C).init(value_t::high,  dir_t::out);

      Pin(RELAY_A).init(value_t::low, dir_t::out);
      Pin(RELAY_B).init(value_t::low, dir_t::out);
      Pin(RELAY_C).init(value_t::low, dir_t::out);

      // Check level will reflect the relay status
      Pin(CHECK_A).init(dir_t::in, invert::inverted);
      Pin(CHECK_B).init(dir_t::in, invert::inverted);
      Pin(CHECK_C).init(dir_t::in, invert::inverted);

      // Start the background check timer
      using namespace std::chrono;
      //timer = asx::reactor::bind(relay::backgroud_check, asx::reactor::prio::low).repeat(100ms);
   }

   void set(uint8_t index, bool close)
   {
      // Read the status of the object to switch
      port_pin_t pin;
      port_pin_t led;

      switch (index) {
      case 0: pin = RELAY_A.integral(); led = LED_A.integral(); break;
      case 1: pin = RELAY_B.integral(); led = LED_B.integral(); break;
      case 2: pin = RELAY_C.integral(); led = LED_C.integral(); break;
      default: return;
      }

      bool change = *Pin(pin);
      Pin(pin).set(close);
      Pin(led).set(close);
      change ^= *Pin(pin);

      // Increment the switching count
      if ( change ) {
         stat::increment_op(index);
      }
   }

   void clean_leds()
   {
      Pin(LED_A).set(*Pin(RELAY_A));
      Pin(LED_B).set(*Pin(RELAY_B));
      Pin(LED_C).set(*Pin(RELAY_C));
   }

   void flash_leds()
   {
      Pin(LED_A).set(not *Pin(LED_A));
      Pin(LED_B).set(not *Pin(LED_B));
      Pin(LED_C).set(not *Pin(LED_C));
   }

   bool status(uint8_t index)
   {
      bool retval = false;

      switch (index)
      {
      case 0:
         retval = *Pin(RELAY_A);
         break;
      case 1:
         retval = *Pin(RELAY_B);
         break;
      case 2:
         retval = *Pin(RELAY_C);
         break;
      }

      return retval;
   }

   bool check(uint8_t index)
   {
      bool retval = false;

      switch (index)
      {
      case 0:
         retval = *Pin(CHECK_A);
         break;
      case 1:
         retval = *Pin(CHECK_B);
         break;
      case 2:
         retval = *Pin(CHECK_C);
         break;
      }

      return retval;
   }

   /**
    * Check the status of all the relay. If a relay is closed, the check pin should be high.
    * The check pin status is delayed from the command:
    *    time for the mecanics to move
    *    RC filter at the check pin
    * Therefore, mismatches are filtered out.
    * This function is started from the init function and is called every 100ms.
    * A mistmatch must occur 3 times in a row to be considered a failure.
    * A failure is final and the function will cancel the repeating timer.
    */
   void backgroud_check() {
      static std::array<uint8_t, 3> err_counts = {0, 0, 0};

      // Read the status of the object to switch
      for (uint8_t i = 0; i < 3; i++) {
         if (status(i) != check(i)) {
            if ( ++err_counts[i] > 3 ) {
               // Notify the main application of a mismatch
               on_error.notify(i);

               // Cancel the timer
               asx::timer::cancel(timer);
            }
         } else {
            err_counts[i] = 0;
         }
      }

   }
}
