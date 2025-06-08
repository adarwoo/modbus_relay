/**
 * @file leds.cpp
 * @brief Instantiates the LED control in a single place to avoid copy issues
 * @author software@arreckx.com
 */
#include "leds.hpp"

namespace led {
   std::array<std::pair<Pin, LedState>, 5> leds = {{
      {LED_A,      LedState::off},
      {LED_B,      LedState::off},
      {LED_C,      LedState::off},
      {INFEED_LED, LedState::off},
      {ALERT_OUTPUT_PIN, LedState::off}
   }};

   int8_t fault_index = -1;
} // namespace led
