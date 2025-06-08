#pragma once

#include <asx/reactor.hpp>

namespace relay {
   namespace infeed {
      enum class Type : uint8_t {
         dc,
         ac_50hz,
         ac_60hz
      };

      Type get_type();
      uint8_t get_type_value();
      void set_type();

      uint16_t get_ac_voltage();
      uint16_t get_dc_voltage();
      uint16_t get_min_voltage();
      uint16_t get_max_voltage();
      void init();

   } // namespace ingress
}  // namespace relay
