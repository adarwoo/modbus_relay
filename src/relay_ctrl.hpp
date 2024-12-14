#pragma once

#include <cstdint>

namespace relay {
   /** When constructed, the LED is ON to test it */
   void init();
   void set(uint8_t index, bool close=true);
   void clean_leds();
   bool status(uint8_t index);
}
