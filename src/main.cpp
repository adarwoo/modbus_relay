/*
 * Relay modbus device main entry point.
 * The relays are initialised by the static constructor
 */
#include <ulog.h>
#include <asx/reactor.hpp>

#include "counters.hpp"
#include "relay.hpp"
#include "push_button.hpp"
#include "leds.hpp"
#include "infeed.hpp"
#include "net.hpp"
#include "estop.hpp"
#include "state.hpp"

#include <chrono>


int main()
{
   ULOG_MILE("Modbus relay application starting!");

   /*
    * Initialize all modules
    * The order determines the reactor handler priorities
    */

   // Ready network handling, the modbus and the UART
   net::init();

   // Ready the relay control
   relay::init();

   // Ready the ingress measurement system
   infeed::init();

   // Ready the push button
   sw::init();

   // Initialize the state manager
   state::init();

   // Initialise the configuration system
   config::init();

   // Ready the estop
   estop::init();

   // Ready the stats
   counter::init();

   // Initialise the LEDs
   led::init();

   // Run the reactor/scheduler
   asx::reactor::run();
}
