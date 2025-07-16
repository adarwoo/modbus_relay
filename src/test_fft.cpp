#include <iostream>
#include <cmath>
#include <asx/single_bin_fft.hpp>


constexpr size_t FFT_N = 64; // FFT window size
constexpr size_t SAMPLE_FREQUENCY = 320; // Sampling frequency in Hz
constexpr size_t SINE_FREQUENCY = 50; // Sine wave frequency in Hz

constexpr int16_t sine_wave[320] = {
   15948, 14954, 321, -14050, -16398, -3871, 11388, 16505, 6907, -8474, -16431, -9920, 5651, 16328, 12880, -2652, -15795, -15235, -828, 13705, 16420, 4271, -10998, -16527, -7339, 8078, 16433, 10332, -5259, -16305, -13261, 2175, 15594, 15464, 1295, -13318, -16482, -4753, 10631, 16492, 7694, -7700, -16431, -10785, 4869, 16278, 13565, -1736, -15402, -15739, -1822, 12969, 16505, 5128, -10208, -16512, -8098, 7336, 16401, 11177, -4491, -16253, -13883, 1287};


int main() {
   using FFT = asx::fft::SingleBinFFT<FFT_N, SAMPLE_FREQUENCY>;

   std::cout << "Testing Single Bin FFT with " << FFT_N << " samples at " << SAMPLE_FREQUENCY << "Hz." << std::endl;

   // Initialize the FFT
   FFT::init(SINE_FREQUENCY);

   // Generate a 50Hz sine wave sampled at 320Hz
   constexpr size_t NUM_SAMPLES = 320; // Total number of samples
   int16_t sine_wave[NUM_SAMPLES];

   for (size_t i = 0; i < NUM_SAMPLES; ++i) {
      sine_wave[i] = 15000.0 * std::sin(2.0 * M_PI * SINE_FREQUENCY * i / SAMPLE_FREQUENCY);
      std::cout << "Sample " << i << ": " << sine_wave[i] << std::endl;
   }

   // Feed the sine wave into the FFT
   for (int j=0; j<2; ++j) {
      std::cout << "Cycle " << j+1 << std::endl;

      for (size_t i = 0; i < NUM_SAMPLES; ++i) {
         bool result_ready = FFT::next(static_cast<int16_t>(sine_wave[i])); // Scale to int16_t range

         if (result_ready) {
            std::cout << "FFT result: " << FFT::get_result() << std::endl;
         }
      }
   }

   return 0;
}