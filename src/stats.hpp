#pragma once
#include <cstdint>

namespace relay {
   namespace stat {
      void increment_op(uint8_t index);
      uint32_t get_op_count(uint8_t index);
      uint32_t get_running_minutes();
      void init();
   }
}