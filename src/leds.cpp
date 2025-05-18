/**
 * @file leds.cpp
 * @brief Instantiates the LED control in a single place to avoid copy issues
 * @author software@arreckx.com
 */
#include "leds.hpp"

namespace relay {
   namespace led {
      std::array<std::pair<Pin, LedState>, 4> leds = {{
         {LED_A,      LedState::off},
         {LED_B,      LedState::off},
         {LED_C,      LedState::off},
         {INFEED_LED, LedState::off}
      }};

      int8_t fault_index = -1;
   } // namespace led
} // namespace relay