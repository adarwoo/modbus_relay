/*
 * Relay modbus device main entry point.
 * The relays are initialised by the static constructor
 */
#include <asx/reactor.hpp>
#include <asx/ulog.hpp>

#include "counters.hpp"
#include "relay.hpp"
#include "push_button.hpp"
#include "leds.hpp"
#include "infeed.hpp"
#include "net.hpp"
#include "estop.hpp"


int main()
{
   ULOG_MILE("Application main starting");
   
   // Initialise the LEDs
   led::init();

   // Ready the estop
   estop::init();

   // Ready the stats
   counter::init();

   // Ready the relay control
   relay::init();

   // Ready the ingress measurement system
   infeed::init();

   // Ready network handling, the modbus and the UART
   net::init();

   // Ready the push button
   sw::init();

   // Run the reactor/scheduler
   asx::reactor::run();
}
