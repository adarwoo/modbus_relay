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

auto react_after_5_seconds    = asx::reactor::bind(after_5_seconds);
auto react_flash_leds_20times = asx::reactor::bind(flash_leds);
auto react_every_second       = asx::reactor::bind(on_watchdog_tick);

// Watchdog counter
auto watchdog_count = uint16_t{0};

using namespace std::chrono;

/**
 * Combine the event system with the configurable custom logic to drive
 * the UART LEDs without software.
 * Upon detecting activity, a pulse is generated so the activity can be seen clearly.
 * Both Timers B are used. TCB0 is used for Rx and TCB1 for Tx
 * The clock input of the timers is wired from the PIT timers through an event channel.
 *
 * Rx Pin on PA5 is connected to TCB0 0.WO
 * Tx Pin on PA7 is connected to LUT1.OUT
 */
void setup_modbus_activity_leds() {
   #pragma GCC diagnostic ignored "-Wdeprecated-enum-enum-conversion"

   // Pulse duration

   // Define a custom duration type representing one tick of the PIT/64 clock
   using tick_duration = duration<int64_t, std::ratio<1, 32768 / 64>>;

   // Set the pulse duration
   constexpr auto pulse_duration = duration_cast<tick_duration>(4ms);

   // Event channels configuration
   EVSYS.CHANNEL0 = EVSYS_CHANNEL0_PORTA_PIN1_gc;    // Rx/Tx activity
   EVSYS.CHANNEL1 = EVSYS_CHANNEL1_PORTA_PIN4_gc;    // XDIR direction selection
   EVSYS.CHANNEL2 = EVSYS_CHANNEL2_CCL_LUT0_gc;      // Output of the LUT2 (RTX & ~XDIR)
   EVSYS.CHANNEL3 = EVSYS_CHANNEL3_RTC_PIT_DIV64_gc; // Output of the periodic timer

   EVSYS.USERCCLLUT0A  = EVSYS_USER_CHANNEL0_gc;     // LUT2-EVENTA  = Ch0 [Rx/Tx activity]
   EVSYS.USERCCLLUT0B  = EVSYS_USER_CHANNEL1_gc;     // LUT2-EVENTB  = Ch1 [XDIR]

   EVSYS.USERTCB0CAPT  = EVSYS_USER_CHANNEL2_gc;     // TCB0 Capture = Ch2 [LUT2-OUT=RxTx & ~XDIR]
   EVSYS.USERTCB0COUNT = EVSYS_USER_CHANNEL3_gc;     // TCB0 count uses channel 3
   EVSYS.USERTCB1CAPT  = EVSYS_USER_CHANNEL1_gc;     // TCB1 Capture = Ch1 [XDIR]
   EVSYS.USERTCB1COUNT = EVSYS_USER_CHANNEL3_gc;     // TCB1 count uses channel 3

   // LUT0 configurations : IN0[A]=Ch0/RTX | IN1[B]=Ch1/XDIR | IN2[-] => Channel 2
   CCL.LUT0CTRLB = CCL_INSEL0_EVENTA_gc | CCL_INSEL1_EVENTB_gc;
   CCL.LUT0CTRLC = 0;
   CCL.TRUTH0    = 1; // LUT2_OUT = (~A & ~B) => CH0 & ~CH1 => ~RTX & ~DIR
   CCL.LUT0CTRLA = CCL_ENABLE_bm;

   // TCB0 configuration : Drives the Rx pin directly
   TCB0.CCMP = pulse_duration.count();
   TCB0.CNT = pulse_duration.count();
   TCB0.EVCTRL = TCB_CAPTEI_bm | TCB_FILTER_bm; // Turn on event detection
   TCB0.CTRLB = TCB_ASYNC_bm | TCB_CCMPEN_bm | TCB_CNTMODE_SINGLE_gc; // Enable the output
   TCB0.CTRLA = TCB_CLKSEL_EVENT_gc | TCB_ENABLE_bm; // Use the event channel as a clock source

   // TCB1 -> Drives the Tx pin directly
   TCB1.CCMP = pulse_duration.count();
   TCB1.CNT = pulse_duration.count();
   TCB1.EVCTRL = TCB_CAPTEI_bm | TCB_FILTER_bm; // Turn on event detection
   TCB1.CTRLB = TCB_ASYNC_bm | TCB_CCMPEN_bm | TCB_CNTMODE_SINGLE_gc; // Enable the output
   TCB1.CTRLA = TCB_CLKSEL_EVENT_gc | TCB_ENABLE_bm;  // Use the event channel as a clock source

   // Activate the CCL for both
   CCL.CTRLA = CCL_ENABLE_bm;
}

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

void on_enter_config_mode() {
   // Cancel the initial 2 seconds
   react_after_5_seconds.clear();

   // Reset the config
   relay::reset_config();

   // Flash the leds fast for 2 seconds (then reset)
   react_flash_leds_20times.repeat(50ms);
#if 0
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
   #endif
}


int main()
{
   using namespace asx;

   // Ready the relay control
   relay::init();

   // Ready the stats
   relay::stat::init();

   // Reset the LEDs to the actual state after 2 seconds
   react_after_5_seconds.delay(2s);

   // Run the reactor/scheduler
   reactor::run();
}
