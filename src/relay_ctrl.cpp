#include <asx/reactor.hpp>
#include <asx/ioport.hpp>

#include "relay_ctrl.hpp"


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

      // Reset the LEDs to the actual state after 2seconds
      asx::reactor::bind(clean_leds).delay(2s);
   }

   void set(uint8_t index, bool close)
   {
      switch (index)
      {
      case 0:
         Pin(LED_A).set(close);
         Pin(RELAY_A).set(close);
         break;
      case 1:
         Pin(LED_B).set(close);
         Pin(RELAY_B).set(close);
         break;
      case 2:
         Pin(LED_C).set(close);
         Pin(RELAY_C).set(close);
         break;
      default:
         Pin(LED_A).set(close);
         Pin(RELAY_A).set(close);
         Pin(LED_B).set(close);
         Pin(RELAY_B).set(close);
         Pin(LED_C).set(close);
         Pin(RELAY_C).set(close);
         break;
      }
   }

   void clean_leds()
   {
      Pin(LED_A).set(*Pin(RELAY_A));
      Pin(LED_B).set(*Pin(RELAY_B));
      Pin(LED_C).set(*Pin(RELAY_C));
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
