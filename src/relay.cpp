/**
 * @file relay.cpp
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
#include <chrono>
#include <array>

#include <asx/ulog.hpp>
#include <asx/reactor.hpp>
#include <asx/ioport.hpp>
#include <asx/bitstore.hpp>
#include <alert.h>

#include "relay.hpp"
#include "counters.hpp"
#include "estop.hpp"
#include "config.hpp"
#include "leds.hpp"

#include "conf_board.h"
#include "conf_version.hpp"


namespace relay {
   namespace {
      using namespace asx;
      using namespace asx::ioport;

      // -------------------------------------------------------------------------
      // Storage
      // -------------------------------------------------------------------------

      // The handle to the timer used for the background check
      auto on_check_health = reactor::Handle{};

      // Lookup for the duration
      using namespace std::chrono;

      // Prototype to bind
      class RelayControl;
      void on_cycle_end(uint8_t index);

      // Events
      struct RequestOn {};
      struct RequestOff {};
      struct TimerElapsed {};

      /**
       * Since relay class
       */
      class RelayControl {
         // Data
         uint8_t index;         // Index of this relay - required to query the config
         Pin relay_pin;         // Pin the relay is connected to
         Pin check_pin;         // Verification pin
         bool faulty;           // True if the relay is faulty
         uint8_t error_count;   // Number of errors counted so far
         timer::Instance timer; // Action to be delayed to to filtering
         reactor::Handle react_on_cycle_end; // Reactor for this relay
         bool projected_state;  // Where to go after the filter period

      public:
         /** Construct an instance */
         RelayControl(uint8_t _index, Pin relay, Pin check) :
            index(_index),
            relay_pin{relay},
            check_pin{check},
            faulty{false},
            error_count{0},
            timer{timer::null},
            projected_state{false}
         {
            relay_pin.set_dir(dir_t::out);

            // Initialise all read back pins
            check_pin.set_invert(invert::inverted);
            check_pin.set_dir(dir_t::in);

            react_on_cycle_end = reactor::bind(on_cycle_end);
         };

         /**
          * Set the relay to the requested state
          * This function will check if the relay is disabled or faulty.
          * Updates the counting statistics and the LED state (through the led API).
          *
          * @param index The relay index within the available range (0 to NUMBER_OF_RELAYS-1).
          * @param onoff Set the state (account for the polarity of the relay)
          * @return true if the relay was changed
          *         false if it was already in the requested state or if it is faulty
          */
         bool set(bool onoff) {
            auto filt_on  = config::get_config().relays_config[index].on_filter;

            // Check for no change
            if ( filt_on == 255 or faulty ) {
               return false;
            }

            // Do we have a filter running?
            if ( timer == timer::null ) {
               // Increment the counter on change
               if ( *relay_pin != onoff ) {
                  counter::increment(index);
               }

               // No - apply now
               relay_pin.set(onoff);

               // Reset the error counter as we are transitioning
               // We could in theory otherwise get a fault
               error_count = 0;

               if ( onoff == true ) {
                  if ( filt_on > 0 ) {
                     // Start a timer to hold this state
                     timer = react_on_cycle_end.delay(
                        std::chrono::milliseconds(filt_on * 100),
                        index
                     );
                  }
               } else {
                  auto filt_off = config::get_config().relays_config[index].off_filter;

                  if ( filt_off > 0 ) {
                     // Start a timer to hold this state
                     timer = react_on_cycle_end.delay(
                        std::chrono::milliseconds(filt_off * 100),
                        index
                     );
                  }
               }
            }

            // Store the projected state which will be applied at the end of the cycle
            projected_state = onoff;

            // Update the LED status
            led::refresh();

            return true;
         }

         void apply_projected_state() {
            timer = timer::null;

            if (*relay_pin != projected_state) { // only change if needed
               set(projected_state);             // may start ON or OFF filter
            }
         }

         void force_open() {
            timer = timer::null;
            relay_pin.set(false);
         }

         bool get() {
            return *relay_pin;
         }

         bool toggle() {
            return set(not projected_state);
         }

         Status get_status() {
            auto filt_on  = config::get_config().relays_config[index].on_filter;

            return (filt_on==255) ? Status::disabled :
               faulty ? Status::faulty :
                  Status::ok;
         }

         void check() {
            auto filt_on  = config::get_config().relays_config[index].on_filter;

            if ( (filt_on==255) or faulty ) {
               return;
            }

            // Check what it is supposed to be against what it is
            if ( *relay_pin == *check_pin ) {
               error_count = 0; // Reset the error counter
               return;
            }

            if ( ++error_count > 3 ) {
               // Store the fault to make it available in the modbus register
               faulty = true;

               // Notify the system about the fault
               estop::trigger(estop::Cause::faulty_relay, index);
            }
         }
      };

      /// All the relays!
      /// @brief Keep track of the number of errors for each relay
      std::array<RelayControl, NUMBER_OF_RELAYS> relays = {
         RelayControl{0, RELAY_A, CHECK_A},
         RelayControl{1, RELAY_B, CHECK_B},
         RelayControl{2, RELAY_C, CHECK_C}
      };

      void on_cycle_end(uint8_t index) {
         relays[index].apply_projected_state();
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
         for (auto &relay: relays) {
            relay.check();
         }
      }
   }

   // -------------------------------------------------------------------------
   // Implementation
   // -------------------------------------------------------------------------

   /**
    * Initialise all the port pins
    * Note: The configuration MUST have been set before calling this function.
    * Checks the configuration and apply default state and invertion as configured.
    * Start a periodic tasklet to check the health of the relays.
    * This function shall be called once at the start of the program, and also
    * whenever the configuration is changed.
    * Faulty relays remains faulty until the next reset.
    */
   void init() {
      using namespace std::chrono;

      ULOG_INFO("Initialising Relays");

      // Start the background check timer
      on_check_health = asx::reactor::bind(
         backgroud_check, asx::reactor::prio::low).repeat(100ms);
   }

   bool set(uint8_t index, bool onoff) {
      if ( index < NUMBER_OF_RELAYS ) {
         relays[index].set(onoff);
         return true;
      }

      return false;
   }

   /**
    * Get the status of the relay.
    * @param index The relay index within the available range (0 to NUMBER_OF_RELAYS-1).
    * @return True if the relay is on, false if it is off
    */
   bool get(uint8_t index) {
      return relays[index].get();
   }

   bool toggle(uint8_t index) {
      return relays[index].toggle();
   }

   /**
    * Return the status of a relay.
    * @param index The relay index within the available range (0 to NUMBER_OF_RELAYS-1).
    * @return The status of the relay
    * @note
    *   - If the relay is disabled and faulty, the status is `Status::disabled`.
    */
   Status get_status(uint8_t index) {
      return relays[index].get_status();
   }

   /**
    * Apply the EStop conditions to the relay hardware.
    * To be called after setting the EStop conditions.
    * React to watchdog condition -> projected state
    * React to infeed condition -> actual state
    */
   void apply_estop() {
      if ( estop::get_cause() == estop::Cause::infeed_polarity) {
         // All relay open right away!
         for (auto &relay: relays) {
            relay.force_open();
         }
      } else if (estop::get_cause() == estop::Cause::modbus_watchdog) {
         uint16_t mask = config::get_config().estop_commloss_mask;

         for (auto &relay: relays) {
            if ( mask & 1 ) {
               // Projected state only
               relay.set(false);
            }

            mask >>= 1;
         }
      } else if ( false
         or estop::get_cause() == estop::Cause::infeed_polarity
         or estop::get_cause() == estop::Cause::infeed_voltage_type
         or estop::get_cause() == estop::Cause::infeed_voltage_over
         or estop::get_cause() == estop::Cause::infeed_voltage_under ) {
         uint16_t mask = config::get_config().estop_infeed_mask;

         for (auto &relay: relays) {
            if ( mask & 1 ) {
               relay.force_open();
            }

            mask >>= 1;
         }
      }
   }
}

