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
      union {
         struct {
            union {
               uint8_t lsb; // Maps to debounce_time
               uint8_t debounce_time; // Alias for lsb
            };
            union {
               uint8_t msb; // Maps to bit flags
               uint8_t config;
               struct {
                  uint8_t disabled : 1;
                  uint8_t default_position : 1;
                  uint8_t invert : 1;
                  uint8_t fault_position : 1;
                  uint8_t reserved : 4; // Remaining bits in MSB
               };
            };
         };

         uint16_t value; // Full 16-bit value
      };
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
    * @param index The relay index within the available range (0 to NUMBER_OF_RELAYS-1).
    * @param close True to close the relay, false to open it
    */
   bool set(uint8_t index, bool close=true);

   /**
    * Get the state of a relay.
    * A faulty or disabled relay will return false
    * @param index The relay index within the available range (0 to NUMBER_OF_RELAYS-1).
    * @return True if the relay is closed, false if it is open
    */
   bool get(uint8_t index);

   /**
    * Apply the current configuration to the relay hardware.
    * To be called after setting the configuration.
    */
   void apply_config();

   /**
    * Check the status of a relay.
    * @param index The relay index within the available range (0 to NUMBER_OF_RELAYS-1).
    * @return True if the relay is operational, false if it is not
    */
   Status get_status(uint8_t index);
}
