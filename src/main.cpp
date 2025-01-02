/*
 * Relay modbus device main entry point.
 * The relays are initialised by the static constructor
 */
#include <asx/reactor.hpp>

// Defines the modbus_slave
#include "stats.hpp"
#include "relay_ctrl.hpp"
#include "modbus.hpp"
#include "broadcast_datagram.hpp"
#include "config.hpp"


using namespace std::chrono;

static void after_5_seconds();
static void flash_leds();
static void on_watchdog_tick();

auto react_after_5_seconds = asx::reactor::bind(after_5_seconds);
auto react_flash_leds_20times = asx::reactor::bind(flash_leds);
auto react_every_second = asx::reactor::bind(on_watchdog_tick);

// Watchdog counter
auto watchdog_count = uint16_t{0};


namespace relay {
   void reset_device() {
      react_flash_leds_20times.repeat(50ms);
      Datagram::set_size(6);
   }

   // Callback when a valid packet is recieved
   void on_ready_reply(std::string_view view) {
      // Reset the watchdog
      watchdog_count = 0;
   }
}

void on_watchdog_tick() {
   if ( ++watchdog_count > relay::get_config().watchdog ) {
      watchdog_count = 0;
      relay::set(0, false);
      relay::set(1, false);
      relay::set(2, false);
   }
}

void after_5_seconds() {
   relay::clean_leds();

   // Start the watchdog tick
   if ( relay::get_config().watchdog > 0 ) {
      react_every_second.repeat(1s);
   }

   // Initialise the modbus slave template API. Overrides the UART settings
   relay::modbus_slave::init();
}

void flash_leds() {
   static uint8_t count_down = 40;

   relay::flash_leds();

   if ( --count_down == 0 ) {
      // Reset the CPU!
      ccp_write_io((uint8_t *)&RSTCTRL.SWRR, RSTCTRL_SWRE_bm);
   }
}

namespace broadcast {
      // Broadcast request to reset to default settings
      void on_reset() {
         // Cancel the initial 2 seconds
         react_after_5_seconds.clear();

         // Reset the config
         relay::reset_config();

         // Flash the leds fast for 2 seconds (then reset)
         react_flash_leds_20times.repeat(50ms);
   }
}


int main()
{
   using namespace asx;

   // Ready the relay control
   relay::init();

   // Ready the stats
   relay::stat::init();

   // Create the broadcast modbus receiver
   modbus::Slave<
      broadcast::Datagram,
      uart::Uart<
         1,
         uart::CompileTimeConfig<
            9600, uart::width::_8, uart::parity::none, uart::stop::_1, uart::onewire | uart::rs485
         >
      >
   >::init();

   // Reset the LEDs to the actual state after 2seconds
   react_after_5_seconds.delay(5s);

   // Run the reactor/scheduler
   reactor::run();
}
