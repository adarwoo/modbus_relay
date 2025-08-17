#pragma once
/** @file relay.hpp
 *  @brief Control interface for relay outputs.
 *
 *  This module provides functions to control relay outputs, including
 *  setting their state and querying their status.
 */

#include <cstdint>


namespace relay {
   /** Specify the configuration of a single relay */
   struct Config {
      uint8_t on_filter;
      uint8_t off_filter;
   };

   /** Report the status of a relay - maps the modbus values */
   enum class Status {
      ok = 0,          ///< Relay is operational
      faulty = 1,     ///< Relay is faulty
      disabled = 2,   ///< Relay is disabled
   };

   /**
    * Initialize the relay control system.
    * This function should be called once at the start of the program.
    */
   void init();

   /**
    * Set the state of a relay.
    * The reaction time is set by the filters in place
    * @param index The relay index within the available range (0 to NUMBER_OF_RELAYS-1).
    * @param close True to close the relay, false to open it
    * @return false if the operation is not allowed (disabled or faulty)
    */
   bool set(uint8_t index, bool close=true);

   /**
    * Get the current and actual state of a relay (not the projected state)
    * A faulty or disabled relay will return false
    * @param index The relay index within the available range (0 to NUMBER_OF_RELAYS-1).
    * @return True if the relay is closed, false if it is open
    */
   bool get(uint8_t index);

   /**
    * Toggle a relay
    * A faulty or disabled relay will return false
    * @param index The relay index within the available range (0 to NUMBER_OF_RELAYS-1).
    * @return false if the operation is not allowed (disabled or faulty)
    */
   bool toggle(uint8_t index);

   /**
    * Apply the current configuration to the relay hardware.
    * To be called after setting the configuration.
    */
   void apply_config();

   /**
    * Apply the EStop conditions to the relay hardware.
    * To be called after setting the EStop conditions.
    */
   void apply_estop();

   /**
    * Check the status of a relay.
    * @param index The relay index within the available range (0 to NUMBER_OF_RELAYS-1).
    * @return True if the relay is operational, false if it is not
    */
   Status get_status(uint8_t index);
}
