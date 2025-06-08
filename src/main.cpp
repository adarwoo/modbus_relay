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
#include "infeed.hpp"
#include "config.hpp"
#include "datagram.hpp"
#include "net.hpp"
#include "estop.hpp"

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
   led::init();

   // Reset the estop
   estop::init();

   // Ready the stats
   stat::init();

   // Ready the relay control
   relay::init();

   // Ready the ingress measurement system
   infeed::init();

   // Ready the switch
   sw::init(asx::reactor::null);

   // Ready the modbus handlers and set the datagram ID
   net::init();

   // Run the reactor/scheduler
   reactor::run();
}
