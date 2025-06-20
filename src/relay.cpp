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
#include <asx/reactor.hpp>
#include <asx/ioport.hpp>
#include <asx/bitstore.hpp>
#include <alert.h>

#include "relay.hpp"
#include "counters.hpp"
#include "estop.hpp"
#include "config.hpp"

#include "conf_board.h"
#include "conf_version.hpp"


namespace relay {
   namespace {
      using namespace asx::ioport;

      // -------------------------------------------------------------------------
      // Storage
      // -------------------------------------------------------------------------

      // The handle to the timer used for the background check
      auto timer = asx::reactor::Handle{};

      /// @brief < Keep track of faulty relays
      auto relays_fault = asx::BitStore<NUMBER_OF_RELAYS>{};

      /// @brief Keep track of disabled relay
      auto relays_disabled = asx::BitStore<NUMBER_OF_RELAYS>{};

      /// @brief Keep track of the number of errors for each relay
      std::array<uint8_t, NUMBER_OF_RELAYS> err_counts{};

      // Create const arrays for the relay and check pins
      auto relay_pins = std::array<Pin, NUMBER_OF_RELAYS>{
         RELAY_A, RELAY_B, RELAY_C
      };

      auto check_pins = std::array<Pin, NUMBER_OF_RELAYS>{
         CHECK_A, CHECK_B, CHECK_C
      };

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
         // Read the status of the object to switch
         for (uint8_t i = 0; i < NUMBER_OF_RELAYS; i++) {
            if ( relays_disabled.get(i) ) {
               continue; // Don't report disabled relay
            }

            if ( relays_fault.get(i) ) {
               continue; // Don't report the fault again - it is final
            }

            if ( get(i) == *check_pins[i] ) {
               err_counts[i] = 0; // Reset the error counter
               continue; // All good
            }

            if ( ++err_counts[i] > 3 ) {
               asm("break");
               // Store the fault to make it available in the modbus register
               relays_fault.set(i);

               // Notify the system about the fault
               estop::trigger(estop::Cause::faulty_relay, i);
            }
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
      for (uint8_t i = 0; i < NUMBER_OF_RELAYS; i++) {
         // Reset the error count for all relays
         err_counts[i] = 0;

         // Grab the configuration for the relay
         auto relay_config = config::get_config().relays_config[i];

         // Check if the relay is disabled - don't touch anything if disabled
         if ( relay_config.disabled ) {
            // Mark as disabled
            relays_disabled.set(i);

            // Leave the reset value (high impedance as is)

            continue; // Skip to the next relay
         }

         relay_pins[i].set_dir(dir_t::out);

         // Initialise all read back pins
         check_pins[i].set_invert(invert::inverted);
         check_pins[i].set_dir(dir_t::in);
      }

      using namespace std::chrono;

      // Start the background check timer
      timer = asx::reactor::bind(
         backgroud_check, asx::reactor::prio::low).repeat(100ms);
   }

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
   bool set(uint8_t index, bool onoff) {
      if (index >= NUMBER_OF_RELAYS) {
         alert_and_stop();
      }

      // Check if the relay is disabled or faulty or already in requested state
      if ( config::get_config().relays_config[index].disabled ) {
         return false;
      }

      // Check if the relay is faulty
      if ( relays_fault.get(index) ) {
         return false;
      }

      // Check if the relay is already in the requested state
      if ( *relay_pins[index] == onoff ) {
         return false; // No change needed
      }

      // Set the relay state
      relay_pins[index].set(onoff);

      // Increment the operation count
      counter::increment(index);

      return true; // State changed
   }

   /**
    * Get the status of the relay.
    * @param index The relay index within the available range (0 to NUMBER_OF_RELAYS-1).
    * @return True if the relay is on, false if it is off
    */
   bool get(uint8_t index)
   {
      alert_and_stop_if( index >= NUMBER_OF_RELAYS );

      return *relay_pins[index];
   }

   /**
    * Return the status of a relay.
    * @param index The relay index within the available range (0 to NUMBER_OF_RELAYS-1).
    * @return The status of the relay
    * @note
    *   - If the relay is disabled and faulty, the status is `Status::disabled`.
    */
   Status get_status(uint8_t index) {
      alert_and_stop_if( index >= NUMBER_OF_RELAYS );

      if ( relays_disabled.get(index) ) {
         return Status::disabled;
      }

      if ( relays_fault.get(index) ) {
         return Status::faulty;
      }

      return Status::ok;
   }
}
