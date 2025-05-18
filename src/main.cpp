/*
 * Relay modbus device main entry point.
 * The relays are initialised by the static constructor
 */
#include <asx/reactor.hpp>

// Defines the modbus_slave
#include "stats.hpp"
#include "relay_ctrl.hpp"
#include "sw.hpp"
#include "leds.hpp"
#include "modbus.hpp"
#include "infeed.hpp"
#include "config.hpp"


using namespace std::chrono;

namespace relay {
   // Callback when a valid packet is recieved
   void on_ready_reply(std::string_view view) {
      // Reset the watchdog
      //watchdog_count = 0;
   }
}

int main()
{
   using namespace asx;

   // Initialise the LEDs
   relay::led::init();

   // Ready the stats
   relay::stat::init();

   // Ready the relay control
   relay::init();

   // Ready the ingress measurement system
   relay::infeed::init();

   // Ready the switch
   relay::sw::init(asx::reactor::null);

   // Initialise the modbus slave template API. Overrides the UART settings
   relay::modbus_slave::init();

   // Reset the LEDs to the actual state after 2 seconds
   reactor::bind([]() {
      relay::led::resume();
   }).delay(2s);

   // Run the reactor/scheduler
   reactor::run();
}
