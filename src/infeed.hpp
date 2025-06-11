#pragma once

#include <cstdint>

namespace infeed {
   /// @brief < Type of infeed to monitor
   enum class CfgType : uint8_t {
      dc = 0,
      ac_50hz = 1,
      ac_60hz = 2
   };

   /// @brief < Type of infeed to report
   enum class InputType : uint8_t {
      dc = 0,
      ac = 1
   };

   namespace literal {
      static inline uint16_t operator"" _volts(unsigned long long int value) {
         return static_cast<uint16_t>(value * 10);
      }

      static inline uint16_t operator"" _volts(long double value) {
         return static_cast<uint16_t>(value * 10);
      }
   }

   uint16_t get_lowest_voltage();
   uint16_t get_highest_voltage();
   uint16_t get_input_voltage();
   InputType get_input_voltage_type();

   void reset_min_max();

   void init();

} // namespace infeed
