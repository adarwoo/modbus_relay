#include <avr/io.h>
#include <avr/interrupt.h>

#include <cmath>
#include <chrono>
#include <algorithm>

#include <asx/hw_timer.hpp>
#include <asx/ulog.hpp>

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

   ///< Muliplier to convert ADC samples to volts
   constexpr auto ADC_ONE_VOLT_IN_IS = 233.0 * std::sqrt(2.0); // At the ISO ampliers, input voltage yielding 1V out
   constexpr auto ADC_TO_VOLTS_DIVIDER_TENTH = (65536.0 / 2.5) / (ADC_ONE_VOLT_IN_IS * 10.0);
   constexpr auto ADC_DC_256_VOLTS_DIVIDER_TENTH = static_cast<uint16_t>(
      std::round(ADC_TO_VOLTS_DIVIDER_TENTH * ADC_SAMPLES_RATE)
   );
}

namespace infeed {

   struct RMS {
      static inline uint16_t result;  // RMS in 1/10 V
      static inline uint16_t sample_count;
      static inline uint32_t abs_sum;
      static inline int16_t last_sample = 0;
      static inline uint8_t zero_crossings = 0;

      static bool compute_next(int16_t sample) {
         abs_sum += std::abs(sample);
         ++sample_count;

         // Count zero crossings to determine if the signal is AC
         // We're sampling at 320Hz, so we expect at least 5 zero crossings for a 50Hz/60Hz signal
         if ((sample > 0 && last_sample < 0) || (sample < 0 && last_sample > 0)) {
            zero_crossings++;
         }

         if (sample_count >= ADC_SAMPLES_RATE) {
            if (zero_crossings < 5) {
               result = 0;  // Not enough zero crossings, assume no AC signal
            } else {
               // Approximate RMS 0.9 average absolute value
               uint32_t avgAbs = abs_sum / ADC_SAMPLES_RATE;
               result = avgAbs * 138 / 1000;  // Scaled to 1/10 V
            }

            abs_sum = 0;
            sample_count = 0;
            return true;
         }

         return false;
      }
   };

   /**
    * @brief Reactor handler to process the ADC sample
    * @param adc_sample The ADC sample to process
    * @note Th
    */
   void process_sample(int16_t adc_sample) {
      static int32_t dc_sum = 0;
      static uint16_t dc_count = 0;
      bool processing_required = false;
      bool above = false;
      bool below = false;
      auto status = Status{Status::none};

      ULOG_DEBUG2("ADC Sample: {}", adc_sample);

      // Compute the FFT for AC signals
      if ( RMS::compute_next(adc_sample) ) {
         processing_required = true;
         detail::last_ac_voltage = RMS::result;
         ULOG_DEBUG0("AC Voltage: {}", detail::last_ac_voltage);
      }

      // Compute an average of the last 256 samples for the DC signal
      dc_sum += adc_sample;

      if ( ++dc_count == ADC_SAMPLES_RATE ) {
         processing_required = true;
         detail::last_dc_voltage = static_cast<int16_t>(dc_sum / ADC_DC_256_VOLTS_DIVIDER_TENTH);
         dc_sum = 0;
         dc_count = 0;
         ULOG_DEBUG0("DC Voltage: {}", detail::last_dc_voltage);
      }

      uint16_t abs_dc_value = std::abs(detail::last_dc_voltage);

       // Sum up the AC and DC voltages for an overall detection
      if ( processing_required ) {
         bool detected = ( detail::last_ac_voltage + abs_dc_value > MIN_DETECTION );

         if ( config::get_config().infeed_type == infeed::CfgType::dc ) {
            // If the infeed type is DC, we only check the DC voltage
            above = ( abs_dc_value > config::get_config().infeed_max_volt_threshold );
            below = ( abs_dc_value < config::get_config().infeed_min_volt_threshold );
         } else { // ACs
            // If the infeed type is AC, we check the AC voltage
            above = ( abs_dc_value > config::get_config().infeed_max_volt_threshold );
            below = ( detail::last_ac_voltage < config::get_config().infeed_min_volt_threshold );
         }

         // Update the min and max voltage
         detail::max_voltage = std::max(detail::max_voltage, std::max(detail::last_ac_voltage, abs_dc_value));
         detail::min_voltage = std::min(detail::min_voltage, std::min(detail::last_ac_voltage, abs_dc_value));

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
    * @brief Initialize the infeed system
    * @param react_on_update The reactor handle to call with new results
    * @note This function initializes the ADC and the FFT instance.
    */
   void init() {
      ULOG_MILE("Initialising Infeed System");

      // Reset the min max
      reset_min_max();

      //
      // Initialise the ADC for the ingress system
      //

      // Enable (won't start yet - needs a command)
      ADC0.CTRLA = ADC_LOWLAT_bm | ADC_ENABLE_bm;

      // Select the ADC clock
      ADC0.CTRLB = ADC_PRESC_DIV2_gc;

      // Ref - Diff input is up to +-2.49V - so use the 2v5 reference
      ADC0.CTRLC = ADC_TIMEBASE << ADC_TIMEBASE_gp | ADC_REFSEL_2500MV_gc;

      // Sample duration
      ADC0.CTRLE = ADC_SAMPLE_DURATION;

      // Accumulate 512 samples to gain 4 bits of resolution
      ADC0.CTRLF = ADC_SAMPNUM_ACC512_gc;

      // No PGA
      ADC0.PGACTRL = 0;

      // Select the ADC channel
#     pragma GCC diagnostic push
#     pragma GCC diagnostic ignored "-Wdeprecated-enum-enum-conversion"
      ADC0.MUXPOS = ADC_VIA_ADC_gc | ADC_MUXPOS_AIN5_gc;
      ADC0.MUXNEG = ADC_VIA_ADC_gc | ADC_MUXNEG_AIN6_gc;
#     pragma GCC diagnostic pop

      // Ready the command using a event to start the ADC
      ADC0.COMMAND = ADC_DIFF_bm | ADC_MODE_BURST_SCALING_gc | ADC_START_EVENT_TRIGGER_gc;

      // Use timer B0 to trigger the ADC through the event system
      using timer = asx::hw_timer::TimerB<0>;
      timer::set_compare( SAMPLE_PERIOD );
      timer::TCB().EVCTRL = TCB_CAPT_bm | TCB_FILTER_bm; // Turn on event detection
      timer::TCB().CTRLA |= TCB_ENABLE_bm; // Turn on timer

      // Hook the event system to trigger the ADC
      EVSYS.CHANNEL4 = EVSYS_CHANNEL4_TCB0_CAPT_gc;
      EVSYS.USERADC0START = EVSYS_USER_CHANNEL4_gc;

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
