#pragma once
/**
 * @file leds.hpp
 * @brief Control of the relay module LEDs
 */

#include <avr/io.h>

#include <array>
#include <utility>
#include <chrono>

#include <asx/ioport.hpp>
#include <asx/reactor.hpp>

#include "conf_board.h"

namespace relay {
   namespace led {
      using namespace asx::ioport;

      enum class LedState : uint8_t {
         off = 0,
         on  = 1,
         blink = 2,
         pulse = 3
      };

      namespace LedId {
         constexpr auto led_a = 0;
         constexpr auto led_b = 1;
         constexpr auto led_c = 2;
         constexpr auto infeed = 3;
      };

      extern std::array<std::pair<Pin, LedState>, 4> leds;
      extern int8_t fault_index;

      /**
       * Combine the event system with the configurable custom logic to drive
       * the UART LEDs without software.
       * Upon detecting activity, a pulse is generated so the activity can be seen clearly.
       * TCB1 is used to create the Rx visible pulse.
       * The Tx LED is driven directly by the XDIR signal.
       * The clock input of the timers is wired from the PIT timers through an event channel.
       */
      static inline void setup_modbus_rx_led() {
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

         EVSYS.USERTCB1CAPT  = EVSYS_USER_CHANNEL2_gc;     // TCB1 Capture = Ch2 [LUT2-OUT=RxTx & ~XDIR]
         EVSYS.USERTCB1COUNT = EVSYS_USER_CHANNEL3_gc;     // TCB1 count uses channel 3

         // LUT0 configurations : IN0[A]=Ch0/RTX | IN1[B]=Ch1/XDIR | IN2[-] => Channel 2
         CCL.LUT0CTRLB = CCL_INSEL0_EVENTA_gc | CCL_INSEL1_EVENTB_gc;
         CCL.LUT0CTRLC = 0;
         CCL.TRUTH0    = 1; // LUT0_OUT = (~A & ~B) => CH0 & ~CH1 => ~RTX & ~DIR
         CCL.LUT0CTRLA = CCL_ENABLE_bm;

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
       * Initialise all the modules LEDs and turn them on for 2 seconds
       * Passed the 2 seconds, the LED resume their normal operations
       */
      static inline void init() {
         using namespace asx::ioport;

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
      }


      /**
       * @brief Blinker tasklet. Drives the LEDs
       * @note This function is called every 100ms
       */
      static void blinker() {
         static auto pulse = uint8_t{10};

         --pulse;

         for ( uint8_t i=0; i<leds.size(); ++i ) {
            auto& led_pair = leds[i];
            auto& led = led_pair.first;
            auto& state = led_pair.second;

            // Override to pulse if responsible for the estop
            if ( i == fault_index ) {
               state = LedState::pulse;
            }

            switch (state) {
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

      /**
       * @brief Resume the LEDs after the 2 seconds check
       * @note This function is called after the 2 seconds check from main
       */
      static inline void resume() {
         using namespace std::chrono;

         LED_MODBUS_RX.clear();
         LED_MODBUS_TX.clear();
         setup_modbus_rx_led();
         ALERT_OUTPUT_PIN.clear();

         // Start the blink tasklet called every 100ms
         asx::reactor::bind(blinker).repeat(100ms);
      }

      /**
       * @brief Control a single LED
       * @param index The LED index
       * @param state The LED state
       */
      static inline void set(uint8_t index, LedState state) {
         if (index < leds.size()) {
            leds[index].second = state;
         }
      }

      /**
       * @brief Set the faulty LED
       * @param index The LED index
       */
      static inline void override(int8_t index) {
         fault_index = index;
      }
   } // namespace led
} // namespace relay