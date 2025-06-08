#pragma once
/**
 * @file leds.hpp
 * @brief Control of the relay module LEDs
 */
#include <cstdint>

namespace led {
   enum class LedState : uint8_t {
      off   = 0,
      on    = 1,
      blink = 2,
      pulse = 3,
      fast  = 4
   };

   namespace LedId {
      constexpr auto led_a  = 0;
      constexpr auto led_b  = 1;
      constexpr auto led_c  = 2;
      constexpr auto infeed = 3;
      constexpr auto estop  = 4;
   };

   /// @brief Initialise the LEDs
   void init();

   /// @brief Control a single LED
   void set(uint8_t index, LedState state);

} // namespace led
