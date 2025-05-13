#include <iostream>
#include <cmath>
#include "fft.hpp"

constexpr size_t FFT_N = 64; // FFT window size
constexpr size_t SAMPLE_FREQUENCY = 320; // Sampling frequency in Hz
constexpr size_t SINE_FREQUENCY = 50; // Sine wave frequency in Hz

int main() {
   using FFT = asx::fft::FFT<FFT_N, SAMPLE_FREQUENCY>;

   // Initialize the FFT
   FFT::init<SINE_FREQUENCY>();

   // Generate a 50Hz sine wave sampled at 320Hz
   constexpr size_t NUM_SAMPLES = 320; // Total number of samples
   float sine_wave[NUM_SAMPLES];
   for (size_t i = 0; i < NUM_SAMPLES; ++i) {
      sine_wave[i] = std::sin(2.0 * M_PI * SINE_FREQUENCY * i / SAMPLE_FREQUENCY);
   }

   // Feed the sine wave into the FFT
   for (size_t i = 0; i < NUM_SAMPLES; ++i) {
      bool result_ready = FFT::next(static_cast<int16_t>(sine_wave[i] * 15000)); // Scale to int16_t range
      if (result_ready) {
         std::cout << "FFT result: " << FFT::get_result() << std::endl;
      }
   }

   return 0;
}