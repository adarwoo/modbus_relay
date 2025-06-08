#pragma once

#include <cstdint>
#include "stats.hpp"

namespace relay {
   namespace relay {
      /** When constructed, the LED is ON to test it */
      void init();
      void set(uint8_t index, bool close=true);
      bool get(uint8_t index);

      bool is_ok(uint8_t index);

      inline uint32_t get_cycles(uint8_t index) {
         return stat::get_op_count(index);
      }
   }
}
