#include <avr/io.h>

#include "fft.hpp"
#include "ingress.hpp"

namespace ingress {
   static constexpr auto CENTER_FREQ = 50; // Center frequency for the FFT
   static constexpr auto FFT_SIZE = 64; // Size of the FFT
   static constexpr auto ADC_SAMPLES = 320; // Number of samples for the ADC
   static constexpr auto ADC_CHANNEL = 0; // ADC channel to read from
   static constexpr auto ADC_VREF = 3.3; // Reference voltage for the ADC
   static constexpr auto ADC_RESOLUTION = 12; // Resolution of the ADC in bits
   static constexpr auto ADC_MAX_VALUE = (1 << ADC_RESOLUTION) - 1; // Maximum value for the ADC
   static constexpr auto ADC_VOLTAGE = ADC_VREF / ADC_MAX_VALUE; // Voltage per ADC step
   static constexpr auto ADC_OFFSET = ADC_VREF / 2; // Offset for the ADC
   static constexpr auto ADC_GAIN = 1.0; // Gain for the ADC
   static constexpr auto ADC_SCALE = ADC_VOLTAGE * ADC_GAIN; // Scale for the ADC
   static constexpr auto ADC_BIAS = ADC_OFFSET * ADC_SCALE; // Bias for the ADC
   static constexpr auto ADC_MAX_VOLTAGE = ADC_VREF; // Maximum voltage for the ADC
   static constexpr auto ADC_MIN_VOLTAGE = 0; // Minimum voltage for the ADC
   static constexpr auto ADC_VOLTAGE_RANGE = ADC_MAX_VOLTAGE - ADC_MIN_VOLTAGE; // Voltage range for the ADC
   static constexpr auto ADC_VOLTAGE_STEP = ADC_VOLTAGE_RANGE / ADC_MAX_VALUE; // Voltage step for the ADC
   static constexpr auto ADC_VOLTAGE_OFFSET = ADC_VOLTAGE_STEP * (ADC_MAX_VALUE / 2); // Voltage offset for the ADC

   // Create the FFT instance
   using fft_t = asx::fft::FFT<FFT_SIZE, ADC_SAMPLES>;

   // Create an instance of the FFT class
   auto fft = fft_t {};

   void handle_fft_result(uint16_t result) {
      // Handle the FFT result
      // This function will be called when the FFT computation is complete
   }

   void init() {
      // Initialize the ingress system

      // Initialize the FFT instance for detecting 50Hz
      fft.init<CENTER_FREQ>(asx::reactor::bind(handle_fft_result));

      // Initialise the ADC for the ingress system

      // Select the ADC clock
      // We sample at 320Hz
      ADC0.CTRLB = ADC_PRESC_DIV2_gc;

      // Ref - Input is 1.0V for 230V RMS So MAX is 236V
      //ADC0.REFSEL = ADC_REFSEL_1024MV_gc;

      // Select the ADC channel
      ADC0.MUXPOS = ADC_VIA_PGA_gc | ADC_MUXPOS_2_bm;
      ADC0.MUXNEG = ADC_VIA_PGA_gc | ADC_MUXNEG_2_bm;
      //ADC0.PGACTRL = ADC_PGABIASSEL_;



   }
} // namespace ingress
