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
      constexpr auto tx     = 2;
      constexpr auto rx     = 3;
      constexpr auto led_a  = 4;
      constexpr auto led_b  = 5;
      constexpr auto led_c  = 6;
   };

   // ----------------------------------------------------------------------------
   // Local variables
   // ----------------------------------------------------------------------------

   /// Store the normal state of an LED
   std::array<std::pair<Pin, LedState>, 7> leds = {{
      {ALERT_OUTPUT_PIN, LedState::off},
      {INFEED_LED,       LedState::off},
      {LED_MODBUS_TX,    LedState::managed},
      {LED_MODBUS_RX,    LedState::managed},
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
   void setup_modbus_rx_led() {
      #pragma GCC diagnostic ignored "-Wdeprecated-enum-enum-conversion"
      using namespace std::chrono;

      // Pulse duration

      // Define a custom duration type representing one tick of the PIT/64 clock
      using tick_duration = duration<int64_t, std::ratio<1, 32768 / 64>>;

      // Set the pulse duration
      constexpr auto pulse_duration = duration_cast<tick_duration>(4ms);

      // Event channels configuration
      EVSYS.CHANNEL0 = EVSYS_CHANNEL0_PORTA_PIN1_gc;    // Rx/Tx activity
      EVSYS.CHANNEL1 = EVSYS_CHANNEL1_PORTA_PIN4_gc;    // XDIR direction selection
      EVSYS.CHANNEL2 = EVSYS_CHANNEL2_CCL_LUT0_gc;      // Output of the LUT0 (RTX & ~XDIR)
      EVSYS.CHANNEL3 = EVSYS_CHANNEL3_RTC_PIT_DIV64_gc; // Output of the periodic timer

      EVSYS.USERCCLLUT0A  = EVSYS_USER_CHANNEL0_gc;     // LUT0-EVENTA  = Ch0 [Rx/Tx activity]
      EVSYS.USERCCLLUT0B  = EVSYS_USER_CHANNEL1_gc;     // LUT0-EVENTB  = Ch1 [XDIR]

      EVSYS.USERCCLLUT1A  = EVSYS_USER_CHANNEL1_gc;     // LUT1-EVENTA  = Ch1 [XDIR]

      EVSYS.USERTCB1CAPT  = EVSYS_USER_CHANNEL2_gc;     // TCB1 Capture = Ch2 [LUT2-OUT=RxTx & ~XDIR]
      EVSYS.USERTCB1COUNT = EVSYS_USER_CHANNEL3_gc;     // TCB1 count uses channel 3

      // LUT0 configurations : IN0[A]=Ch0/RTX | IN1[B]=Ch1/XDIR | IN2[-] => Channel 2
      CCL.LUT0CTRLB = CCL_INSEL0_EVENTA_gc | CCL_INSEL1_EVENTB_gc;
      CCL.LUT0CTRLC = 0;
      CCL.TRUTH0    = 1; // LUT0_OUT = (~A & ~B) => CH0 & ~CH1 => ~RTX & ~DIR
      CCL.LUT0CTRLA = CCL_ENABLE_bm;

      // LUT1 configurations : IN0[A]=Ch2/LUT0-OUT | IN1[B]=Ch3/PIT | IN2[-] => Channel 2
      CCL.LUT1CTRLB = CCL_INSEL0_EVENTA_gc;
      CCL.LUT1CTRLC = 0;
      CCL.TRUTH1    = 0b10; // LUT1_OUT = XDIR
      CCL.LUT1CTRLA = CCL_ENABLE_bm | CCL_OUTEN_bm; // Enable the output

      // TCB1 -> Drives the Tx pin directly
      TCB1.CCMP = pulse_duration.count();
      TCB1.CNT = pulse_duration.count();
      TCB1.EVCTRL = TCB_CAPTEI_bm | TCB_FILTER_bm; // Turn on event detection
      TCB1.CTRLB = TCB_ASYNC_bm | TCB_CCMPEN_bm | TCB_CNTMODE_SINGLE_gc; // Enable the output
      TCB1.CTRLA = TCB_CLKSEL_EVENT_gc | TCB_ENABLE_bm;  // Use the event channel as a clock source

      // Activate the CCL for both
      CCL.CTRLA = CCL_ENABLE_bm;
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
         leds[id::tx].second = LedState::fast;
         leds[id::rx].second = LedState::fast;
      } else {
         // Go full automatic mode
         CCL.CTRLA = CCL_ENABLE_bm;
         TCB1.CTRLB |= TCB_CCMPEN_bm;

         leds[id::tx].second = LedState::managed;
         leds[id::rx].second = LedState::managed;
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

      ULOG_INFO("Initialising LEDs");

      // Set the reactor
      detail::react_on_refresh = asx::reactor::bind(on_refresh_leds_status);

      // Initialise the relay module LEDs
      LED_A.init(dir_t::out, value_t::high);
      LED_B.init(dir_t::out, value_t::high);
      LED_C.init(dir_t::out, value_t::high);

      // Set the infeed LED to high
      INFEED_LED.init(dir_t::out, value_t::high);

      // Force the Rx LED to high (driven by the event system)
      LED_MODBUS_RX.init(dir_t::out, value_t::high);
      // Force the XDIR pin to high to turn on the Tx LED
      LED_MODBUS_TX.init(dir_t::out, value_t::high);

      // Set the alert pin to high
      ALERT_OUTPUT_PIN.init(dir_t::out, value_t::high);

      // Arm a timer to transition after 2 seconds
      asx::reactor::bind([] {
         LED_MODBUS_RX.clear();
         LED_MODBUS_TX.clear();
         ALERT_OUTPUT_PIN.clear();

         // Turn on the CCL and TCB1 for the Rx/Tx LEDs
         setup_modbus_rx_led();

         // Start the blink tasklet called every 100ms
         asx::reactor::bind(blinker).repeat(100ms);
      }).delay(2s);
   }
} // namespace led
