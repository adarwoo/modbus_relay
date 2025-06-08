#pragma once

#include <cstdint>

namespace infeed {
   /// @brief < Type of infeed to monitor
   enum class Type : uint8_t {
      dc = 0,
      ac_50hz = 1,
      ac_60hz = 2
   };

   namespace literal {
      static inline uint16_t operator"" _volts(unsigned long long int value) {
         return static_cast<uint16_t>(value * 10);
      }

      static inline uint16_t operator"" _volts(long double value) {
         return static_cast<uint16_t>(value * 10);
      }
   }

   uint16_t get_ac_voltage();
   uint16_t get_dc_voltage();
   uint16_t get_min_voltage();
   uint16_t get_max_voltage();

   void reset_min_max();

   void init();

} // namespace ingress
