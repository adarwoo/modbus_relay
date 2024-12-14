/*
 * Relay modbus device main entry point.
 * The relays are initialised by the static constructor
 */
#include <asx/reactor.hpp>

// Defines the modbus_slave
#include "relay_ctrl.hpp"
#include "modbus.hpp"


int main()
{
   using namespace asx;

   // Ready the relay control
   relay::init();

   // Initialise the modbus slave template API
   relay::modbus_slave::init();

   // Run the reactor/scheduler
   reactor::run();
}
