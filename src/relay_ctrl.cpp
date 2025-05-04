#include <asx/reactor.hpp>
#include <asx/ioport.hpp>

#include "relay_ctrl.hpp"
#include "stats.hpp"


namespace relay
{
   using namespace asx::ioport;

   auto constexpr LED_A = PinDef{B, 1};
   auto constexpr LED_B = PinDef{B, 0};
   auto constexpr LED_C = PinDef{A, 2};
   auto constexpr RELAY_A = PinDef{B, 3};
   auto constexpr RELAY_B = PinDef{A, 7};
   auto constexpr RELAY_C = PinDef{A, 6};

   /** When constructed, the LED is ON to test it */
   void init()
   {
      using namespace std::chrono;

      Pin(LED_A).init(value_t::high,  dir_t::out);
      Pin(LED_B).init(value_t::high,  dir_t::out);
      Pin(LED_C).init(value_t::high,  dir_t::out);
      Pin(RELAY_A).init(value_t::low, dir_t::out);
      Pin(RELAY_B).init(value_t::low, dir_t::out);
      Pin(RELAY_C).init(value_t::low, dir_t::out);
   }

   void set(uint8_t index, bool close)
   {
      // Read the status of the object to switch
      const PinDef *pin;
      const PinDef *led;

      switch (index) {
      case 0: pin = &RELAY_A; led = &LED_A; break;
      case 1: pin = &RELAY_B; led = &LED_B; break;
      case 2: pin = &RELAY_C; led = &LED_C; break;
      default: return;
      }

      bool change = *Pin(*pin);
      Pin(*pin).set(close);
      Pin(*led).set(close);
      change ^= *Pin(*pin);

      // Increment the switching count
      if ( change ) {
         stat::increment_op(index);
      }
   }

   void clean_leds()
   {
      Pin(LED_A).set(*Pin(RELAY_A));
      Pin(LED_B).set(*Pin(RELAY_B));
      Pin(LED_C).set(*Pin(RELAY_C));
   }

   void flash_leds()
   {
      Pin(LED_A).set(not *Pin(LED_A));
      Pin(LED_B).set(not *Pin(LED_B));
      Pin(LED_C).set(not *Pin(LED_C));
   }

   bool status(uint8_t index)
   {
      bool retval = false;

      switch (index)
      {
      case 0:
         retval = *Pin(RELAY_A);
         break;
      case 1:
         retval = *Pin(RELAY_B);
         break;
      case 2:
         retval = *Pin(RELAY_C);
         break;
      }

      return retval;
   }
}
