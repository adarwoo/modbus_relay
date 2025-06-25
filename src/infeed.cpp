#include <avr/io.h>
#include <avr/interrupt.h>

#include <cmath>
#include <chrono>

#include <asx/hw_timer.hpp>
#include <asx/single_bin_fft.hpp>

#include "counters.hpp"
#include "leds.hpp"
#include "config.hpp"
#include "estop.hpp"
#include "infeed.hpp"

using namespace std::chrono;
using namespace infeed::literal;

using ticks_t = asx::chrono::cpu_tick_t;


// ----------------------------------------------------------------------------
// Constants and configuration
// ----------------------------------------------------------------------------
namespace {
   ///< Center frequency for the FFT
   constexpr auto CENTER_FREQ = 50;
   ///< Size of the FFT
   constexpr auto FFT_SIZE = 64;
   ///< Sample rate in Hz
   constexpr auto ADC_SAMPLES_RATE = 320;
   ///< Number of CPU ticks per sample
   constexpr auto SAMPLE_PERIOD = ticks_t(static_cast<long long>(F_CPU / ADC_SAMPLES_RATE));
   ///< ADC clock prescale factor
   constexpr auto ADC_CLK_PRESCALE = 2;
   ///< ADC clock period in prescaled ticks
   constexpr auto ADC_CLK_PER = F_CPU / ADC_CLK_PRESCALE;
   ///< ADC timebase in microseconds
   constexpr auto ADC_TIMEBASE = static_cast<uint8_t>(std::ceil(ADC_CLK_PER * 1e-6));
   ///< ADC sample duration in CPU ticks
   ///< We have a low impedance input with input capacitance of 8pF, we can sample fast.
   constexpr auto ADC_SAMPLE_DURATION = 2;
   ///< Minimum voltage to detect a signal
   constexpr auto MIN_DETECTION = 10_volts;
}

namespace infeed {

   // Create the FFT instance
   using fft_t = asx::fft::SingleBinFFT<FFT_SIZE, ADC_SAMPLES_RATE>;

   /**
    * @brief Reactor handler to process the ADC sample
    * @param adc_sample The ADC sample to process
    * @note Th
    */
   void process_sample(int16_t adc_sample) {
      static int32_t dc_sum = 0;
      static int16_t dc_count = 0;
      bool processing_required = false;
      bool above = false;
      bool below = false;
      auto status = Status{Status::none};

      // Compute the FFT for AC signals
      if ( fft_t::next(adc_sample) ) {
         processing_required = true;
         detail::last_ac_voltage = fft_t::get_result();
      }

      // Compute an average of the last 256 samples for the DC signal
      dc_sum += adc_sample;

      if ( ++dc_count >= 256 ) {
         processing_required = true;
         detail::last_dc_voltage = static_cast<int16_t>(dc_sum / 256);
         dc_sum = dc_count = 0;
      }

      // Sum up the AC and DC voltages for an overall detection
      if ( processing_required ) {
         bool detected = ( detail::last_ac_voltage + detail::last_dc_voltage > MIN_DETECTION );

         if ( config::get_config().infeed_type == infeed::CfgType::dc ) {
            // If the infeed type is DC, we only check the DC voltage
            above = ( detail::last_dc_voltage > config::get_config().infeed_max_volt_threshold );
            below = ( detail::last_dc_voltage < config::get_config().infeed_min_volt_threshold );
         } else { // ACs
            // If the infeed type is AC, we check the AC voltage
            above = ( detail::last_ac_voltage > config::get_config().infeed_max_volt_threshold );
            below = ( detail::last_ac_voltage < config::get_config().infeed_min_volt_threshold );
         }

         // Update the min and max voltage
         detail::max_voltage = std::max(detail::max_voltage, std::max(detail::last_ac_voltage, detail::last_dc_voltage));
         detail::min_voltage = std::min(detail::min_voltage, std::min(detail::last_ac_voltage, detail::last_dc_voltage));

         status = detected ? (
            above ? Status::above : below ? Status::below : Status::in_range
         ) : Status::none;

         if ( detail::current_status != status ) {
            detail::current_status = status;

            // Check applicability of the situation
            if ( status == Status::wrong_type && config::get_config().estop_on_bad_voltage_type ) {
               estop::trigger(
                  estop::Cause::infeed_voltage_type,
                  static_cast<uint16_t>(0xFFFF), // Diagnostic code for wrong type
                  estop::ExternalTriggerType::resetable
               );
            } else if ( status == Status::above && config::get_config().estop_on_overvolt ) {
               estop::trigger(
                  estop::Cause::infeed_voltage_over,
                  static_cast<uint16_t>(detail::max_voltage),
                  estop::ExternalTriggerType::resetable
               );
            } else if ( status == Status::below && config::get_config().estop_on_undervolt ) {
               estop::trigger(
                  estop::Cause::infeed_voltage_under,
                  static_cast<uint16_t>(detail::min_voltage),
                  estop::ExternalTriggerType::resetable
               );
            }
         }
      }
   }

   /**
    * @brief Initialize the ingress system
    * @param react_on_update The reactor handle to call with new results
    * @note This function initializes the ADC and the FFT instance.
    */
   void init() {
      uint8_t freq = 55; // Half way will detect either mains type
      auto infeed_type = config::get_config().infeed_type;

      if ( infeed_type == infeed::CfgType::ac_50hz ) {
         freq = 50;
      } else if ( infeed_type == infeed::CfgType::ac_60hz ) {
         freq = 60;
      }

      // Reset the min max
      reset_min_max();

      // Initialize the FFT instance for detecting the configured frequency or 55Hz to detect the 50Hz/60Hz
      fft_t::init( freq );

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

   /** Reactor handler to call from the ADC interrupt */
   auto react_on_adc_sample_ready = asx::reactor::bind(process_sample);

   /**
    * @brief ADC interrupt handler
    */
   ISR(ADC0_RESRDY_vect) {
      react_on_adc_sample_ready(static_cast<uint16_t>(ADC0.RESULT));
      ADC0.INTFLAGS |= ADC_RESRDY_bm;
   }
} // namespace ingress
