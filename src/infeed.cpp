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
#include "state.hpp"

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

      // Use bit positions that are power-of-2 for efficient operations
      static uint8_t processing_flags = 0;
      static constexpr uint8_t RMS_READY = 1;      // Bit 0 (0x01)
      static constexpr uint8_t DC_READY = 2;       // Bit 1 (0x02)
      static constexpr uint8_t BOTH_READY = 3;     // RMS_READY | DC_READY (0x03)

      bool above = false;
      bool below = false;
      bool inverted = false;

      // Keep the old status for comparison
      auto old_status = detail::current_status;

      // Compute the pseudo RMS for AC signals
      if ( RMS::compute_next(adc_sample) ) {
         processing_flags |= RMS_READY;  // Set bit 0
         detail::last_ac_voltage = RMS::result;
      }

      // Compute DC average - optimize division
      {
         dc_sum += adc_sample;

         if ( ++dc_count == ADC_SAMPLES_RATE ) {  // 320 - not power of 2, but unavoidable
            processing_flags |= DC_READY;  // Set bit 1

            // Optimize: Use bit shift if possible, or keep division for accuracy
            detail::last_dc_voltage = static_cast<int16_t>(dc_sum / ADC_DC_256_VOLTS_DIVIDER_TENTH);

            dc_sum = 0;
            dc_count = 0;
         }
      }

      // Process only when both flags are set - optimized check
      if ( processing_flags >= BOTH_READY ) {  // Faster than masking for this case
         processing_flags = 0;  // Reset all flags

         ULOG_DEBUG0("DC Voltage: {}", detail::last_dc_voltage);
         ULOG_DEBUG0("AC Voltage: {}", detail::last_ac_voltage);

         // Optimize abs() - compiler should use efficient AVR abs instruction
         uint16_t abs_dc_value = (detail::last_dc_voltage < 0) ?
            -detail::last_dc_voltage : detail::last_dc_voltage;

         // Set input type to whichever (AC or DC) is highest and above MIN_DETECTION
         if (abs_dc_value > MIN_DETECTION && abs_dc_value >= detail::last_ac_voltage) {
            detail::current_input_type = InputType::dc;
            detail::current_status = Status::voltage_present;
         } else if (detail::last_ac_voltage > MIN_DETECTION) {
            detail::current_input_type = InputType::ac;
            detail::current_status = Status::voltage_present;
         } else {
            detail::current_input_type = InputType::none;
            detail::current_status = Status::none;
         }

         // Optimize inversion check - combine with threshold check
         inverted = (detail::last_dc_voltage < -static_cast<int16_t>(MIN_DETECTION));

         // Clear faults - use constexpr for compile-time optimization
         static constexpr auto INFEED_FAULTS = 0
            | state::fault::infeed_under_voltage
            | state::fault::infeed_over_voltage
            | state::fault::infeed_bad_type
            | state::fault::infeed_polarity_inverted;

         state::clear_faults(INFEED_FAULTS);

         // Cache config for fewer lookups
         const auto& cfg = config::get_config();
         const auto configured_type = cfg.infeed_type;

         // Store the min and max voltages in all cases
         if ( detail::current_input_type == InputType::dc ) {
            // DC input detected, update with DC voltage
            detail::min_voltage = std::min(detail::min_voltage, abs_dc_value);
            detail::max_voltage = std::max(detail::max_voltage, abs_dc_value);
         } else if ( detail::current_input_type == InputType::ac ) {
            // AC input detected, update with AC voltage
            detail::min_voltage = std::min(detail::min_voltage, detail::last_ac_voltage);
            detail::max_voltage = std::max(detail::max_voltage, detail::last_ac_voltage);
         }

         ULOG_DEBUG0("Min Voltage: {}", detail::min_voltage);
         ULOG_DEBUG0("Max Voltage: {}", detail::max_voltage);

         if ( configured_type == InputType::none ) {
            ULOG_WARN("Type none");
            // The system does not care about the input type - just report presence
            detail::current_status = (detail::current_input_type == InputType::none)
               ? Status::none
               : Status::voltage_present;
            ULOG_INFO("Updated current status to: {}", (uint8_t)detail::current_status);
         } else if (
            configured_type != detail::current_input_type
            and detail::current_input_type != InputType::none ) {
            ULOG_WARN("Type not match");
            // Type mismatch handling
            detail::current_status = Status::faulty;
            state::append_faults(state::fault::infeed_bad_type);

            if ( cfg.estop_on_bad_voltage_type ) {
               detail::current_status = Status::estop;
               estop::trigger(
                  estop::Cause::infeed_voltage_type,
                  0xFFFF,  // Constant for wrong type
                  estop::ExternalTriggerType::resetable
               );
            }
         } else {
            // Process voltage thresholds
            if ( detail::current_input_type == InputType::ac ) {
               above = (detail::last_ac_voltage > cfg.infeed_max_volt_threshold);
               below = (detail::last_ac_voltage < cfg.infeed_min_volt_threshold);
            } else { // DC processing
               if ( inverted ) {
                  state::append_faults(state::fault::infeed_polarity_inverted);
                  detail::current_status = Status::estop;  // Direct assignment since it's terminal

                  estop::trigger(
                     estop::Cause::infeed_polarity,
                     0xFFFE,  // Constant for polarity inversion
                     estop::ExternalTriggerType::resetable
                  );

                  return;  // Early exit to avoid further processing
               } else {
                  above = (abs_dc_value > cfg.infeed_max_volt_threshold);
                  below = (abs_dc_value < cfg.infeed_min_volt_threshold);
               }
            }

            // Handle voltage threshold faults - optimize with early exit pattern
            if ( above ) {
               detail::current_status = Status::faulty;

               state::append_faults(state::fault::infeed_over_voltage);

               if ( cfg.estop_on_overvolt ) {
                  detail::current_status = Status::estop;

                  estop::trigger(
                     estop::Cause::infeed_voltage_over,
                     detail::max_voltage,
                     estop::ExternalTriggerType::resetable
                  );
               }
            } else if ( below ) {
               detail::current_status = Status::faulty;

               if ( cfg.estop_on_undervolt ) {
                  state::append_faults(state::fault::infeed_under_voltage);

                  if ( cfg.estop_on_undervolt ) {
                     detail::current_status = Status::estop;

                     estop::trigger(
                        estop::Cause::infeed_voltage_under,
                        detail::min_voltage,
                        estop::ExternalTriggerType::resetable
                     );
                  }
               }
            }
         }

         if ( detail::current_status != old_status ) {
            led::refresh();  // Update LEDs on status change
         }
      } // Both measurements ready
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
