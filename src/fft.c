/**
 *@ingroup lib
 *@defgroup fft FFT API
 *@{
 *@file
 *****************************************************************************
 * Compute the FFT of a value progressively.
 * This code was written for a microcontroller with limited RAM space.
 * The FFT here is 'progressive', so each new sample participate in the
 *  computation of a new value from the previous samples. The computation
 *  has a predictable worse case duration, making this algorithm highly
 *  suitable for being called within a real time system.
 * The result will lag in time by 2x the FFT window size.
 * The twiddle factors are stored in flash memory to save RAM.
 * The incomming samples are stored in a separate buffer again to save RAM.
 *
 * @author software@arreckx.com
 *****************************************************************************
 -*/

#include <math.h>
#include <string.h>

#include "wgx.h"
#include "fft.h"

// Check all constants required. These should be defined in the config.h file
#ifndef FFT_M
   #error "FFT_M must be defined in config.h"
#endif

#ifndef ADC_SAMPLE_FREQUENCY
   #error "ADC_SAMPLE_FREQUENCY must be defined in config.h"
#endif


/** Size of the FFT window in points */
#define FFT_N (1 << FFT_M)

/** Half the size of the FFT window */
#define FFT_H (FFT_N/2)

/** Compute the bin to compute */
#define FFT_BIN_SIZE (ADC_SAMPLE_FREQUENCY / FFT_N)

/**
 * Define a fast index type, enough for the size of fft we
 *  are dealing with, but lean on memory
 */
#if (FFT_N > 256)
typedef uint16_t index_t;
#else
typedef uint8_t index_t;
#endif

/** Define the type of raw data comming in */
typedef int16_t sample_t;

/** Define the complex type */
typedef struct
{
   /** Real part of the number */
   float real;
   /** Imaginary part of the number */
   float imag;
} complex_t;

/** Holds data and statefull fft information */
typedef struct
{
   /**
    *  Store the incomming data and computed data in a separate
    *  buffer, since the raw data is 2 bytes long and requires N
    *  data, and the computed data is complex with 8 bytes and
    *  requires N/2, we save 8*N/2 - 2*N = 2N bytes. This is more
    *  efficient than declaring a N long complex buffer, but less
    *  efficient than overlapping both buffers somehow, which
    *  becomes very tricky to manage.
    *  A 64 point fft requires a 8*32 + 2*64 + 6 = 390 bytes buffer.
    */
   sample_t  s[FFT_N];

   /** Complex buffer for computed results */
   complex_t x[FFT_H];

   /**
    * Holds which butterfly computation to make for each pass
    * Each bit tells whether to compute a simple butterfly (0) or
    *  a 'twiddled' butterfly '1'.
    */
   index_t   bin;

   /** Overall index. Increments with each new sample */
   index_t   i;

   /** Holds the size of the group. Halves every pass */
   index_t   gSize;

   /** Holds the increment step to get the correct twiddle factor */
   index_t   wInc;

   /** Holds the current twiddle factor index */
   index_t   wIndex;

   /** Group count. Defines the space between samples */
   index_t   gCount;

   /**
    * Stores the final result of the fft for use by the application.
    * This result is normalised to the PSD of the measurement
    */
   float  result;
} fft_t;


// Include the twiddle coeffs locally.
// This will make the data local and the code much more compact
#include "twiddle.c"

/** The one static fft structure */
static fft_t fft;

/**
 * Forward declaration of internal methods
 */
static void fftNewCycle(void);

/**
 * Prepares the fft.
 * This method computes which butterflies to compute during each pass to ready
 *  the desired bin.
 */
void fftInit(uint16_t centerFrequency)
{
   uint8_t binNumber;
   index_t i;

   // Center frequency to measure. Given in Hz
   binNumber = centerFrequency;

   binNumber /= FFT_BIN_SIZE;

   // Compute path the bin
   fft.bin = 0;
   for (i=0; i<FFT_M; ++i)
   {
      fft.bin <<= 1;
      fft.bin |= (binNumber&1);
      binNumber >>= 1;
   }

   fftReset();
}


/**
 * Restart the FFT measurement discaring any previous measurements.
 * Requires that fftInit has been called. Called by fftInit.
 */
void fftReset(void)
{
   // Set wInc to zero to indicate that no real samples are stored yet
   fft.wInc = 0;

   // Use the index to store a whole buffer full of actual values
   fft.i = 0;
}


/**
 * Starts a new FFT cycle.
 * Internal method which zeros all counters
 */
void fftNewCycle(void)
{
   fft.i      = 0;
   fft.wIndex = 0;
   fft.gCount = 0;
   fft.wInc   = 1;
   fft.gSize  = FFT_H;
}


/**
 * Return the last computed result
 *
 * @return The last computed result
 */
float fftGetResult(void)
{
   return fft.result;
}

/**
 * Pass in the next sample value to progress with computing the FFT
 *
 * @param sample A signed 16-bit sample value - already decimated and
 *                normalized.
 * @return true if the computation has produced a result, 1 if a result
 *         is ready to collect
 */
bool fftNext( int16_t sample )
{
   bool retval=false;

   if ( fft.wInc > 0 )
   {
      // Compute next value in-place
      if ( fft.i != (FFT_N - 1) )
      {
         // Compute next butterfly and storage location
         index_t ii = fft.i + fft.gSize;
         index_t jj = ii - FFT_H;


         if ( fft.bin & fft.gSize ) // Binary AND here
         {
            complex_t w;

            // Copy the correct twiddle factor in w
            memcpy_P( &w, &fftTwiddle[fft.wIndex], sizeof(complex_t) );

            if ( fft.i < FFT_H ) // Reading values from the real buffer
            {
               fft.x[jj].real =
                  ((float)fft.s[fft.i]) * w.real -
                  ((float)fft.s[ii]) * w.imag;

               fft.x[jj].imag = 0.0;
            }
            else  // Reading from the complex in place buffer
            {
               index_t k = fft.i-FFT_H;
               float temp=fft.x[jj].real;

               fft.x[jj].real =
                  w.real * ( fft.x[k].real - fft.x[jj].real ) -
                  w.imag * ( fft.x[k].imag - fft.x[jj].imag );

               fft.x[jj].imag =
                  w.real * ( fft.x[k].imag - fft.x[jj].imag ) +
                  w.imag * ( fft.x[k].real - temp );
            }
         }
         else // Simple butterfly
         {
            if ( fft.i < FFT_H ) // Reading values from the real buffer
            {
               // Since the sample are 12bits long, sum as int, then convert
               fft.x[jj].real = (float)( fft.s[fft.i] + fft.s[ii] );

               fft.x[jj].imag = 0.0;
            }
            else
            {
               index_t k = fft.i-FFT_H;
               fft.x[jj].real += fft.x[k].real;
               fft.x[jj].imag += fft.x[k].imag;
            }
         }

         // Store the new incomming value
         fft.s[fft.i] = sample;

         // Move on
         ++fft.i;
         ++fft.gCount;
         fft.wIndex += fft.wInc;

         // Are we still computing within the same group
         if ( fft.gCount == fft.gSize )
         {
            // Double-up the group size
            fft.gSize >>= 1;
            fft.gCount = 0;

            // Next pass
            fft.wIndex = 0;
            fft.wInc <<= 1;
         }
      }
      else // Last sample is different. No butterfly, but PSD calculation
      {
         // Store the new incomming value
         fft.s[fft.i] = sample;

         // Last pass simply needs to return the result
         // Compute the power
         fft.result = sqrt(
            square(fft.x[FFT_H - 1].real) +
            square(fft.x[FFT_H - 1].imag) );

         fft.result /= FFT_H;

         // Start again
         fftNewCycle();

         // Tell the caller the result is ready
         retval = true;
      }
   }
   else // wInc == 0
   {
      // When wInc is 0, simply store a first pass of values
      fft.s[fft.i] = sample;

      // Move on
      ++fft.i;

      // If all sample have been stored, turn on continuous mode
      if ( fft.i == FFT_N )
      {
         fftNewCycle();
      }
   }

   return retval;
}


/* ----------------------------  End of file  ---------------------------- */

