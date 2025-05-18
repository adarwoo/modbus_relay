#include <avr/io.h>
#include <avr/interrupt.h>

#include <cmath>
#include <chrono>

#include <asx/hw_timer.hpp>

#include "stats.hpp"
#include "leds.hpp"
#include "config.hpp"
#include "fft.hpp"
#include "infeed.hpp"


namespace relay {
   namespace infeed {
      static constexpr auto CENTER_FREQ = 50; // Center frequency for the FFT
      static constexpr auto FFT_SIZE = 64; // Size of the FFT
      static constexpr auto ADC_SAMPLES_RATE = 320; // Sample rate in Hz
      static constexpr auto SAMPLE_PERIOD = asx::chrono::cpu_tick_t(
         static_cast<long long>(F_CPU / ADC_SAMPLES_RATE));

      static constexpr auto ADC_CLK_PRESCALE = 2;
      static constexpr auto ADC_CLK_PER = F_CPU / ADC_CLK_PRESCALE;
      static constexpr auto ADC_TIMEBASE = static_cast<uint8_t>(std::ceil(ADC_CLK_PER * 1e-6));

      static constexpr auto ADC_SAMPLE_DURATION = 2; // We have a low impedance input with input capacitange of 8pF, we can sample fast.

      static constexpr auto MIN_DETECTION = 25 * 100; // 25V

      static asx::reactor::Handle react_on_infeed_status = asx::reactor::null;

      // Create the FFT instance
      using fft_t = asx::fft::FFT<FFT_SIZE, ADC_SAMPLES_RATE>;


      /**
       * @brief Reactor handler to process the ADC sample
       * @param adc_sample The ADC sample to process
       * @note Th
       */
      void process_sample(int16_t adc_sample) {
         static uint16_t last_ac_voltage = 0;
         static uint16_t last_dc_voltage = 0;
         static int32_t dc_sum = 0;
         static int16_t dc_count = 0;
         bool processing_required = false;
         bool inrange = false;

         // Compute the FFT for AC signals
         if ( fft_t::next(adc_sample) ) {
            processing_required = true;
            last_ac_voltage = fft_t::get_result();

            if ( get_config().frequency != 0 ) {
               // If the frequency is not set, we assume it's a DC signal
               inrange = (
                  last_ac_voltage > get_config().infeed_ac_min &&
                  last_ac_voltage < get_config().infeed_ac_max
               );
            }
         }

         // Compute an average of the last 256 samples for the DC signal
         dc_sum += adc_sample;

         if ( ++dc_count >= 256 ) {
            processing_required = true;
            last_dc_voltage = static_cast<int16_t>(dc_sum / 256);
            dc_sum = dc_count = 0;

            if ( get_config().frequency == 0 ) {
               // If the frequency is not set, we assume it's a DC signal
               inrange = (
                  last_dc_voltage > get_config().infeed_dc_min &&
                  last_dc_voltage < get_config().infeed_dc_max
               );
            }
         }

         // Sum up the AC and DC voltages for an overall detection
         if ( processing_required ) {
            bool detected = ( last_ac_voltage + last_dc_voltage > MIN_DETECTION );

            // Report the voltage detection status (Ignore the estop case - it overrides)
            led::set( 
               led::LedId::infeed, 
               detected ? (
                  inrange ? led::LedState::on : led::LedState::blink 
               ) : led::LedState::off
            );
         }
      }

      /** Reactor handler to call from the ADC interrupt */
      auto react_on_adc_sample_ready = asx::reactor::bind(process_sample);

      /**
       * @brief Initialize the ingress system
       * @param react_on_update The reactor handle to call with new results
       * @note This function initializes the ADC and the FFT instance.
       */
      void init() {
         auto freq = get_config().frequency;

         // Initialize the FFT instance for detecting the configured frequency or 55Hz to detect the 50Hz/60Hz
         fft_t::init( freq==0 ? 55 : freq );

         //
         // Initialise the ADC for the ingress system
         //

         // Enable (won't start yet - needs a command)
         ADC0.CTRLA = ADC_LOWLAT_bm | ADC_ENABLE_bm;

         // Select the ADC clock
         ADC0.CTRLB = ADC_PRESC_DIV2_gc;

         // Ref - Input is 1.0V for 230V RMS So MAX is 236V
         ADC0.CTRLC = ADC_TIMEBASE << ADC_TIMEBASE_gp | ADC_REFSEL_1024MV_gc;

         // Sample duration
         ADC0.CTRLE = ADC_SAMPLE_DURATION;

         // Accumulate 512 samples to gain 4 bits of resolution
         ADC0.CTRLF = ADC_SAMPNUM_ACC512_gc;

         // No PGA
         ADC0.PGACTRL = 0;

         // Select the ADC channel
         ADC0.MUXPOS = ADC_MUXPOS_2_bm;
         ADC0.MUXNEG = ADC_MUXNEG_2_bm;

         // Ready the command using a event to start the ADC
         ADC0.COMMAND = ADC_DIFF_bm | ADC_MODE_BURST_SCALING_gc | ADC_START_EVENT_TRIGGER_gc;

         // Use timer B0 to trigger the ADC through the event system
         using timer = asx::hw_timer::TimerB<0>;
         timer::set_compare( SAMPLE_PERIOD );

         // Hook the event system to trigger the ADC
         EVSYS.CHANNEL0 = EVSYS_CHANNEL0_TCB0_CAPT_gc;    // Rx/Tx activity
         EVSYS.USERADC0START = EVSYS_USER_CHANNEL0_gc;

         // Enable the interrupt
         ADC0.INTCTRL |= ADC_RESRDY_bm;
      }

      /**
       * @brief ADC interrupt handler
       */
      ISR(ADC0_RESRDY_vect) {
         react_on_adc_sample_ready(static_cast<uint16_t>(ADC0.RESULT));
         ADC0.INTFLAGS |= ADC_RESRDY_bm;
      }
   } // namespace ingress
} // namespace relay