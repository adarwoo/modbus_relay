#pragma once

#include <cstdint>
#include "stats.hpp"


namespace relay {

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
                  uint8_t disable : 1;
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

   /** When constructed, the LED is ON to test it */
   void init();
   void set(uint8_t index, bool close=true);
   bool get(uint8_t index);
   void apply_config();

   bool is_ok(uint8_t index);

   inline uint32_t get_cycles(uint8_t index) {
      return stat::get_op_count(index);
   }
}
