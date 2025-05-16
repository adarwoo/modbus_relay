#include <cmath>
#include <chrono>
#include <avr/io.h>

#include "fft.hpp"
#include "ingress.hpp"

namespace ingress {
   static constexpr auto CENTER_FREQ = 50; // Center frequency for the FFT
   static constexpr auto FFT_SIZE = 64; // Size of the FFT
   static constexpr auto ADC_SAMPLES_RATE = 320; // Sample rate in Hz
   static constexpr auto SAMPLE_PERIOD = std::chrono::duration<double>(1.0 / ADC_SAMPLES_RATE);

   static constexpr auto ADC_CLK_PRESCALE = 2;
   static constexpr auto ADC_CLK_PER = F_CPU / ADC_CLK_PRESCALE;
   static constexpr auto ADC_TIMEBASE = static_cast<uint8_t>(std::ceil(ADC_CLK_PER * 1e-6));

   static constexpr auto ADC_SAMPLE_DURATION = 2; // We have a low impedance input with input capacitange of 8pF, we can sample fast.

   // Create the FFT instance
   using fft_t = asx::fft::FFT<FFT_SIZE, ADC_SAMPLES>;

   // Create an instance of the FFT class
   auto fft = fft_t {};

   void handle_fft_result(uint16_t result) {
      // Handle the FFT result
      // This function will be called when the FFT computation is complete
      // Compare the threshold
      // TODO
   }

   void send_to_fft(int16_t adc_sample) {
      fft::compute(adc_sample);
   }

   auto react_on_adc_sample_ready = asx::reactor::bind(send_to_fft);

   void init() {
      // Initialize the ingress system

      // Initialize the FFT instance for detecting 50Hz
      fft.init<CENTER_FREQ>(asx::reactor::bind(handle_fft_result));

      // Initialise the ADC for the ingress system

      // Enable (won't start yet - needs a command)
      ADC0.CTRLA = ADC_LOWLAT_gc | ADC_ENABLE;

      // Select the ADC clock
      ADC0.CTRLB = ADC_PRESC_DIV2_gc;

      // Ref - Input is 1.0V for 230V RMS So MAX is 236V
      ADC0.CTRLC = ADC_TIMEBASE << ADC_TIMEBASE_gp | ADC_REFSEL_1024MV_gc;

      // Sample duration
      ADC0.CTRLE = ADC_SAMPLE_DURATION;

      // Accumulate 512 samples to gain 4 bits of resolution
      ADC0.CTRLF = ADC_SAMPNUM_ACC512;

      // No PGA
      ADC0.PGACTRL = ADC_PGA_DISABLE;

      // Select the ADC channel
      ADC0.MUXPOS = ADC_MUXPOS_2_bm;
      ADC0.MUXNEG = ADC_MUXNEG_2_bm;
      ADC0.PGACTRL = 0; // No PGA

      // Ready the command using a event to start the ADC
      ADC0.COMMAND = ADC_COMMAND_DIFF_bm | ADC_COMMAND_NODE_BURST_SCALING_gc | ADC_COMMAND_EVENT_TRIGGER_bm;

      // Use timer B0 to trigger the ADC through the event system
      using timer = asx::hw_timer::TimerB<0>;
      timer::set_compare( SAMPLE_PERIOD );

      // Hook the event system to trigger the ADC
      EVSYS.CHANNEL0 = EVSYS_CHANNEL0_TCB0_CAPT_gc;    // Rx/Tx activity
      EVSYS.USERADC0 = EVSYS_USER_CHANNEL0_gc;

      // Enable the interrupt
      ADC0.INTCTRL |= ADC_RESREADY;
   }

   ISR(ADC0_vect) {
      react_on_adc_sample_ready(ADC0.RESULT16);
   }
} // namespace ingress
