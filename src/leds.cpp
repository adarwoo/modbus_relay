/**
 * @file leds.cpp
 * @brief Manages the LED control
 * @details
 * This module provides functions to control the LEDs on the relay module,
 * including setting their states (on, off, blink, pulse) and handling
 * periodic updates to the LED states.
 * The module uses the information provided from the estop, relay, net and state
 * modules to determine the appropriate LED states based on the device's
 * operational status.
 * @author software@arreckx.com
 */
#include <avr/io.h>

#include <array>
#include <utility>
#include <chrono>

#include <asx/ulog.hpp>
#include <asx/ioport.hpp>
#include <asx/reactor.hpp>

#include "conf_board.h"
#include "leds.hpp"
#include "net.hpp"
#include "state.hpp"
#include "estop.hpp"

using namespace std::chrono;
using namespace asx::ioport;


namespace {
   // ----------------------------------------------------------------------------
   // Local types
   // ----------------------------------------------------------------------------
   enum class LedState : uint8_t {
      managed = 0, // Managed by the system
      off,         // LED is off
      on,          // LED is on
      blink,       // LED is blinking
      pulse,       // LED is pulsing
      fast         // LED is fast blinking
   };

   namespace id {
      constexpr auto estop  = 0;
      constexpr auto infeed = 1;
      constexpr auto modbus = 2;
      constexpr auto led_a  = 3;
      constexpr auto led_b  = 4;
      constexpr auto led_c  = 5;
   };

   // ----------------------------------------------------------------------------
   // Local variables
   // ----------------------------------------------------------------------------

   /// Store the normal state of an LED
   std::array<std::pair<Pin, LedState>, 6> leds = {{
      {ALERT_OUTPUT_PIN, LedState::off},
      {INFEED_LED,       LedState::off},
      {LED_MODBUS,       LedState::managed},
      {LED_A,            LedState::off},
      {LED_B,            LedState::off},
      {LED_C,            LedState::off},
   }};

   // ----------------------------------------------------------------------------
   // Local functions
   // ----------------------------------------------------------------------------

   /**
    * Combine the event system with the configurable custom logic to drive
    * the UART LEDs without software.
    * Upon detecting activity, a pulse is generated so the activity can be seen clearly.
    * TCB1 is used to create the Rx visible pulse.
    * The Tx LED is driven directly by the XDIR signal.
    * The clock input of the timers is wired from the PIT timers through an event channel.
    */
   void setup_modbus_led() {
      using namespace std::chrono;

      // Pulse duration

      // Define a custom duration type representing one tick of the PIT/64 clock
      using tick_duration = duration<int64_t, std::ratio<1, 32768 / 64>>;

      // Set the pulse duration
      constexpr auto pulse_duration = duration_cast<tick_duration>(4ms);

      // Event channels configuration
      EVSYS.CHANNEL0 = EVSYS_CHANNEL0_PORTA_PIN1_gc;    // Rx/Tx activity
      EVSYS.CHANNEL3 = EVSYS_CHANNEL3_RTC_PIT_DIV64_gc; // Output of the periodic timer

      EVSYS.USERTCB1CAPT  = EVSYS_USER_CHANNEL0_gc;     // TCB1 Capture = Ch0 [RxTx]
      EVSYS.USERTCB1COUNT = EVSYS_USER_CHANNEL3_gc;     // TCB1 count uses channel 3

      // TCB1 -> Drives the Tx pin directly
      TCB1.CCMP = pulse_duration.count();
      TCB1.CNT = pulse_duration.count();
      TCB1.EVCTRL = TCB_CAPTEI_bm | TCB_FILTER_bm; // Turn on event detection
      TCB1.CTRLB = TCB_ASYNC_bm | TCB_CCMPEN_bm | TCB_CNTMODE_SINGLE_gc; // Enable the output
      TCB1.CTRLA = TCB_CLKSEL_EVENT_gc | TCB_ENABLE_bm;  // Use the event channel as a clock source
   }


   /**
    * @brief Update the status of the LEDs based on the current system state
    */
   void on_refresh_leds_status() {
      // Locate takes precedence on everything else
      if ( state::is_in_locate_mode() ) {
         // Unplug from the LUT and Timer
         CCL.CTRLA = 0;

         // If in locate mode, set all LEDs to fast blink
         for ( auto& led_pair : leds ) {
            led_pair.second = LedState::fast;
         }

         return;
      }

      // Recovery mode overwrites the modbus LEDs
      if ( state::is_in_recovery_mode() ) {
         // Unplug Rx and Tx from the LUT output and the timer
         CCL.CTRLA = 0;
         TCB1.CTRLB &= ~TCB_CCMPEN_bm; // Disable the timer output

         // Fast flash Rx and Tx LEDs
         leds[id::modbus].second = LedState::fast;
      } else {
         // Go full automatic mode
         CCL.CTRLA = CCL_ENABLE_bm;
         TCB1.CTRLB |= TCB_CCMPEN_bm;

         leds[id::modbus].second = LedState::managed;
      }

      // EStop led
      leds[id::estop].second =
         (estop::get_status() == estop::Status::estop) ? LedState::blink :
            (estop::get_status() == estop::Status::terminated) ? LedState::on : LedState::off;

      // Infeed LED
      bool is_infeed = false
         or (estop::get_cause() == estop::Cause::infeed_voltage_type)
         or (estop::get_cause() == estop::Cause::infeed_voltage_over)
         or (estop::get_cause() == estop::Cause::infeed_voltage_under);

      leds[id::infeed].second = is_infeed ? LedState::blink :
         (infeed::get_status() == infeed::Status::in_range) ? LedState::on :
         (infeed::get_status() == infeed::Status::above) ? LedState::fast :
         (infeed::get_status() == infeed::Status::below) ? LedState::pulse : LedState::off;

      // Relay LEDs
      for ( uint8_t i=id::led_a; i<=id::led_c; ++i ) {
         auto& state = leds[i].second;
         auto led_index = i - id::led_a;

         // Set the LED state based on the relay status
         switch ( relay::get_status(led_index) ) {
         case relay::Status::ok:
            state = relay::get(led_index) ? LedState::on : LedState::off;
            break;
         case relay::Status::faulty:
            state = LedState::pulse;
            break;
         case relay::Status::disabled:
         default:
            state = LedState::off;
            break;
         }
      }
   }

   /**
    * @brief Blinker tasklet. Drives the LEDs
    * @note This function is called every 100ms
    */
   void blinker() {
      static auto pulse = uint8_t{10};
      --pulse;

      for ( uint8_t i=0; i<leds.size(); ++i ) {
         auto& led_pair = leds[i];
         auto led = led_pair.first;
         auto state = led_pair.second;

         if ( state == LedState::managed ) {
            continue;
         }

         switch (state) {
         case LedState::fast:
            led.toggle();
            break;
         case LedState::blink:
            if ( pulse == 0 or pulse == 5 ) {
               led.toggle();
            }
            break;
         case LedState::pulse:
            led.set(pulse == 0);
            break;
         case LedState::off:
            led.clear();
            break;
         case LedState::on:
            led.set(value_t::high);
            break;
         default:
            break;
         }
      }

      if ( pulse == 0 ) {
         pulse = 10;
      }
   }
} // End of anonymous namespace


namespace led {
   /**
    * Initialise all the modules LEDs and turn them on for 2 seconds
    * Passed the 2 seconds, the LED resume their normal operations
    */
   void init() {
      using namespace asx::ioport;

      ULOG_MILE("Initialising LEDs");

      // Initialise the relay module LEDs
      LED_A.init(dir_t::out, value_t::high);
      LED_B.init(dir_t::out, value_t::high);
      LED_C.init(dir_t::out, value_t::high);

      // Set the infeed LED to high
      INFEED_LED.init(dir_t::out, value_t::high);

      // Force the Rx LED to high (driven by the event system)
      LED_MODBUS.init(dir_t::out, value_t::high);

      // Set the alert pin to high (already high given the external pull-up on the driver)
      ALERT_OUTPUT_PIN.init(dir_t::out, value_t::high);

      // Arm a timer to transition after 2 seconds
      asx::reactor::bind([] {
         LED_MODBUS.clear();
         ALERT_OUTPUT_PIN.clear();

         // Set the reactor for refreshing (only now to maintain the 2s check)
         detail::react_on_refresh = asx::reactor::bind(on_refresh_leds_status);

         // Update right away any pending changes
         detail::react_on_refresh();

         // Turn on the CCL and TCB1 for the Rx/Tx LEDs
         setup_modbus_led();

         // Start the blink tasklet called every 100ms
         asx::reactor::bind(blinker).repeat(100ms);
      }).delay(2s);
   }
} // namespace led
