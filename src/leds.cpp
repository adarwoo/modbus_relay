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

#include <ulog.h>
#include <asx/ioport.hpp>
#include <asx/timer.hpp>
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
      flashing,    // LED is flashing (50% duty cycle)
      pulsing,     // LED is pulsing (10% duty cycle)
      fast         // LED is fast flashing
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

   /// Indicate to show the locate mode
   auto show_locate_mode = false;

   /// Reactor to refresh the LED status
   auto react_on_flip_mode = asx::reactor::bind([]{
      show_locate_mode = not show_locate_mode;
      led::detail::react_on_refresh();
   });

   /// Timer to toggle the locate mode LED
   auto timer_toggle_locate_mode = asx::timer::null;

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
      ULOG_TRACE("Refreshing LED status");

      // Locate takes precedence on everything else
      if ( show_locate_mode ) {
         // Turn off the TCB1 timer to free the modbus LED
         TCB1.CTRLB &= ~TCB_CCMPEN_bm;

         // If in locate mode, set all LEDs to fast blink
         for ( auto& led_pair : leds ) {
            led_pair.first.clear();
            led_pair.second = LedState::fast;
         }

         return;
      }
      else {
         // Re-enable the TCB1 timer to resume modbus LED operation
         TCB1.CTRLB |= TCB_CCMPEN_bm;
      }

      // Recovery mode overwrites the modbus LEDs
      if ( state::is_in_recovery_mode() ) {
         // Fast flash EStop LED (So we can still set the modbus LED)
         leds[id::estop].second = LedState::fast;
      } else {
         // EStop led
         switch (estop::get_status()) {
         case estop::Status::estop:
            leds[id::estop].second = LedState::flashing;
            break;
         case estop::Status::terminated:
            leds[id::estop].second = LedState::on;
            break;
         default:
            leds[id::estop].second = LedState::off;
            break;
         }
      }

      // Infeed LED
      auto status = infeed::get_status();
      ULOG_INFO("Infeed status: {}", (uint8_t)status);

      switch (status) {
      case infeed::Status::voltage_present:
         leds[id::infeed].second = LedState::on;   // Voltage present
         break;
      case infeed::Status::faulty:
         leds[id::infeed].second = LedState::pulsing; // Faulty condition
         break;
      case infeed::Status::estop:
         leds[id::infeed].second = LedState::flashing; // EStop condition
         break;
      default:
         leds[id::infeed].second = LedState::off;  // Fallback to off
         break;
      }

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
            state = LedState::flashing;
            break;
         case relay::Status::disabled:
            state = LedState::pulsing;
            break;
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
         case LedState::flashing:
            if ( pulse == 0 or pulse == 5 ) {
               led.toggle();
            }
            break;
         case LedState::pulsing:
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

      // Bind the locate mode toggle handler


      // Arm a timer to transition after 2 seconds
      asx::reactor::bind([] {
         LED_MODBUS.clear();
         ALERT_OUTPUT_PIN.clear();

         // Set the reactor for refreshing (only now to maintain the 2s check)
         detail::react_on_refresh = asx::reactor::bind(on_refresh_leds_status);

         // Update right away any pending changes
         detail::react_on_refresh();

         // Turn on TCB1 for the Rx/Tx LED
         setup_modbus_led();

         // Start the blink tasklet called every 100ms
         asx::reactor::bind(blinker).repeat(100ms);
      }).delay(2s);
   }

   void toggle_locate_mode(bool locate_mode) {
      // Cancel the timer in any case
      timer_toggle_locate_mode.cancel();
      show_locate_mode = false;

      if ( locate_mode ) {
         // Arm the timer to toggle the locate mode every 2s
         timer_toggle_locate_mode = react_on_flip_mode.repeat(1ms, 2s);
      } else {
         // Restore normal operation
         detail::react_on_refresh();
      }
   }

} // namespace led
