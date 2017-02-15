/**********************************************************************

Audacity: A Digital Audio Editor

SpectrogramSettings.cpp

Paul Licameli

*******************************************************************//**

\class SpectrogramSettings
\brief Spectrogram settings, either for one track or as defaults.

*//*******************************************************************/

#include "../Audacity.h"
#include "SpectrogramSettings.h"

#include "../Experimental.h"

#include "../NumberScale.h"

#include <algorithm>

#include "../FFT.h"
#include "../Internat.h"
#include "../MemoryX.h"
#include "../Prefs.h"

#include <cmath>

#include "../widgets/AudacityMessageBox.h"

SpectrogramSettings::Globals::Globals()
{
   LoadPrefs();
}

void SpectrogramSettings::Globals::SavePrefs()
{
#ifdef SPECTRAL_SELECTION_GLOBAL_SWITCH
   gPrefs->Write(wxT("/Spectrum/EnableSpectralSelection"), spectralSelection);
#endif
}

void SpectrogramSettings::Globals::LoadPrefs()
{
#ifdef SPECTRAL_SELECTION_GLOBAL_SWITCH
   spectralSelection
      = (gPrefs->Read(wxT("/Spectrum/EnableSpectralSelection"), 0L) != 0);
#endif
}

SpectrogramSettings::Globals
&SpectrogramSettings::Globals::Get()
{
   static Globals instance;
   return instance;
}

SpectrogramSettings::SpectrogramSettings()
{
   LoadPrefs();
}

SpectrogramSettings::SpectrogramSettings(const SpectrogramSettings &other)
   : minFreq(other.minFreq)
   , maxFreq(other.maxFreq)
   , range(other.range)
   , gain(other.gain)
   , frequencyGain(other.frequencyGain)
   , windowType(other.windowType)
   , windowSize(other.windowSize)
#ifdef EXPERIMENTAL_ZERO_PADDED_SPECTROGRAMS
   , zeroPaddingFactor(other.zeroPaddingFactor)
#endif
   , isGrayscale(other.isGrayscale)
   , scaleType(other.scaleType)
#ifndef SPECTRAL_SELECTION_GLOBAL_SWITCH
   , spectralSelection(other.spectralSelection)
#endif
   , algorithm(other.algorithm)
#ifdef EXPERIMENTAL_FFT_Y_GRID
   , fftYGrid(other.fftYGrid)
#endif
#ifdef EXPERIMENTAL_FIND_NOTES
   , fftFindNotes(other.fftFindNotes)
   , findNotesMinA(other.findNotesMinA)
   , numberOfMaxima(other.numberOfMaxima)
   , findNotesQuantize(other.findNotesQuantize)
#endif

   // Do not copy these!
   , hFFT{}
   , window{}
   , tWindow{}
   , dWindow{}
   , kernels{}
{
}

SpectrogramSettings &SpectrogramSettings::operator= (const SpectrogramSettings &other)
{
   if (this != &other) {
      minFreq = other.minFreq;
      maxFreq = other.maxFreq;
      range = other.range;
      gain = other.gain;
      frequencyGain = other.frequencyGain;
      windowType = other.windowType;
      windowSize = other.windowSize;
#ifdef EXPERIMENTAL_ZERO_PADDED_SPECTROGRAMS
      zeroPaddingFactor = other.zeroPaddingFactor;
#endif
      isGrayscale = other.isGrayscale;
      scaleType = other.scaleType;
#ifndef SPECTRAL_SELECTION_GLOBAL_SWITCH
      spectralSelection = other.spectralSelection;
#endif
      algorithm = other.algorithm;
#ifdef EXPERIMENTAL_FFT_Y_GRID
      fftYGrid = other.fftYGrid;
#endif
#ifdef EXPERIMENTAL_FIND_NOTES
      fftFindNotes = other.fftFindNotes;
      findNotesMinA = other.findNotesMinA;
      numberOfMaxima = other.numberOfMaxima;
      findNotesQuantize = other.findNotesQuantize;
#endif

      // Invalidate the caches
      DestroyWindows();
   }
   return *this;
}

SpectrogramSettings& SpectrogramSettings::defaults()
{
   static SpectrogramSettings instance;
   return instance;
}

//static
const EnumValueSymbols &SpectrogramSettings::GetScaleNames()
{
   static const EnumValueSymbols result{
      // Keep in correspondence with enum SpectrogramSettings::ScaleType:
      XO("Linear") ,
      XO("Logarithmic") ,
      /* i18n-hint: The name of a frequency scale in psychoacoustics */
      XO("Mel") ,
      /* i18n-hint: The name of a frequency scale in psychoacoustics, named for Heinrich Barkhausen */
      XO("Bark") ,
      /* i18n-hint: The name of a frequency scale in psychoacoustics, abbreviates Equivalent Rectangular Bandwidth */
      XO("ERB") ,
      /* i18n-hint: Time units, that is Period = 1 / Frequency */
      XO("Period") ,
   };
   return result;
}

//static
const TranslatableStrings &SpectrogramSettings::GetAlgorithmNames()
{
   static const TranslatableStrings results{
      // Keep in correspondence with enum SpectrogramSettings::Algorithm:
      XO("Frequencies") ,
      /* i18n-hint: the Reassignment algorithm for spectrograms */
      XO("Reassignment") ,
      /* i18n-hint: EAC abbreviates "Enhanced Autocorrelation" */
      XO("Pitch (EAC)") ,
      XO("Tones") ,
   };
   return results;
}

bool SpectrogramSettings::Validate(bool quiet)
{
   if (!quiet &&
      maxFreq < 100) {
      AudacityMessageBox( XO("Maximum frequency must be 100 Hz or above") );
      return false;
   }
   else
      maxFreq = std::max(100, maxFreq);

   if (!quiet &&
      minFreq < 0) {
      AudacityMessageBox( XO("Minimum frequency must be at least 0 Hz") );
      return false;
   }
   else
      minFreq = std::max(0, minFreq);

   if (!quiet &&
      maxFreq <= minFreq) {
      AudacityMessageBox( XO(
"Minimum frequency must be less than maximum frequency") );
      return false;
   }
   else
      maxFreq = std::max(1 + minFreq, maxFreq);

   if (!quiet &&
      range <= 0) {
      AudacityMessageBox( XO("The range must be at least 1 dB") );
      return false;
   }
   else
      range = std::max(1, range);

   if (!quiet &&
      frequencyGain < 0) {
      AudacityMessageBox( XO("The frequency gain cannot be negative") );
      return false;
   }
   else if (!quiet &&
      frequencyGain > 60) {
      AudacityMessageBox( XO(
"The frequency gain must be no more than 60 dB/dec") );
      return false;
   }
   else
      frequencyGain =
         std::max(0, std::min(60, frequencyGain));

   // The rest are controlled by drop-down menus so they can't go wrong
   // in the Preferences dialog, but we also come here after reading fom saved
   // preference files, which could be or from future versions.  Validate quietly.
   windowType =
      std::max(0, std::min(NumWindowFuncs() - 1, windowType));
   scaleType =
      ScaleType(std::max(0,
         std::min((int)(SpectrogramSettings::stNumScaleTypes) - 1,
            (int)(scaleType))));
   algorithm = Algorithm(
      std::max(0, std::min((int)(algNumAlgorithms) - 1, (int)(algorithm)))
   );
   ConvertToEnumeratedWindowSizes();
   ConvertToActualWindowSizes();

   return true;
}

void SpectrogramSettings::LoadPrefs()
{
   minFreq = gPrefs->Read(wxT("/Spectrum/MinFreq"), 0L);

   maxFreq = gPrefs->Read(wxT("/Spectrum/MaxFreq"), 8000L);

   range = gPrefs->Read(wxT("/Spectrum/Range"), 80L);
   gain = gPrefs->Read(wxT("/Spectrum/Gain"), 20L);
   frequencyGain = gPrefs->Read(wxT("/Spectrum/FrequencyGain"), 0L);

   windowSize = gPrefs->Read(wxT("/Spectrum/FFTSize"), 1024);

#ifdef EXPERIMENTAL_ZERO_PADDED_SPECTROGRAMS
   zeroPaddingFactor = gPrefs->Read(wxT("/Spectrum/ZeroPaddingFactor"), 1);
#endif

   gPrefs->Read(wxT("/Spectrum/WindowType"), &windowType, eWinFuncHanning);

   isGrayscale = (gPrefs->Read(wxT("/Spectrum/Grayscale"), 0L) != 0);

   scaleType = ScaleType(gPrefs->Read(wxT("/Spectrum/ScaleType"), 0L));

#ifndef SPECTRAL_SELECTION_GLOBAL_SWITCH
   spectralSelection = (gPrefs->Read(wxT("/Spectrum/EnableSpectralSelection"), 1L) != 0);
#endif

   algorithm = Algorithm(gPrefs->Read(wxT("/Spectrum/Algorithm"), 0L));

#ifdef EXPERIMENTAL_FFT_Y_GRID
   fftYGrid = (gPrefs->Read(wxT("/Spectrum/FFTYGrid"), 0L) != 0);
#endif //EXPERIMENTAL_FFT_Y_GRID

#ifdef EXPERIMENTAL_FIND_NOTES
   fftFindNotes = (gPrefs->Read(wxT("/Spectrum/FFTFindNotes"), 0L) != 0);
   findNotesMinA = gPrefs->Read(wxT("/Spectrum/FindNotesMinA"), -30.0);
   numberOfMaxima = gPrefs->Read(wxT("/Spectrum/FindNotesN"), 5L);
   findNotesQuantize = (gPrefs->Read(wxT("/Spectrum/FindNotesQuantize"), 0L) != 0);
#endif //EXPERIMENTAL_FIND_NOTES

   // Enforce legal values
   Validate(true);

   InvalidateCaches();
}

void SpectrogramSettings::SavePrefs()
{
   gPrefs->Write(wxT("/Spectrum/MinFreq"), minFreq);
   gPrefs->Write(wxT("/Spectrum/MaxFreq"), maxFreq);

   // Nothing wrote these.  They only varied from the linear scale bounds in-session. -- PRL
   // gPrefs->Write(wxT("/SpectrumLog/MaxFreq"), logMinFreq);
   // gPrefs->Write(wxT("/SpectrumLog/MinFreq"), logMaxFreq);

   gPrefs->Write(wxT("/Spectrum/Range"), range);
   gPrefs->Write(wxT("/Spectrum/Gain"), gain);
   gPrefs->Write(wxT("/Spectrum/FrequencyGain"), frequencyGain);

   gPrefs->Write(wxT("/Spectrum/FFTSize"), windowSize);

#ifdef EXPERIMENTAL_ZERO_PADDED_SPECTROGRAMS
   gPrefs->Write(wxT("/Spectrum/ZeroPaddingFactor"), zeroPaddingFactor);
#endif

   gPrefs->Write(wxT("/Spectrum/WindowType"), windowType);

   gPrefs->Write(wxT("/Spectrum/Grayscale"), isGrayscale);

   gPrefs->Write(wxT("/Spectrum/ScaleType"), (int) scaleType);

#ifndef SPECTRAL_SELECTION_GLOBAL_SWITCH
   gPrefs->Write(wxT("/Spectrum/EnableSpectralSelection"), spectralSelection);
#endif

   gPrefs->Write(wxT("/Spectrum/Algorithm"), (int) algorithm);

#ifdef EXPERIMENTAL_FFT_Y_GRID
   gPrefs->Write(wxT("/Spectrum/FFTYGrid"), fftYGrid);
#endif //EXPERIMENTAL_FFT_Y_GRID

#ifdef EXPERIMENTAL_FIND_NOTES
   gPrefs->Write(wxT("/Spectrum/FFTFindNotes"), fftFindNotes);
   gPrefs->Write(wxT("/Spectrum/FindNotesMinA"), findNotesMinA);
   gPrefs->Write(wxT("/Spectrum/FindNotesN"), numberOfMaxima);
   gPrefs->Write(wxT("/Spectrum/FindNotesQuantize"), findNotesQuantize);
#endif //EXPERIMENTAL_FIND_NOTES
}

void SpectrogramSettings::InvalidateCaches()
{
   DestroyWindows();
}

SpectrogramSettings::~SpectrogramSettings()
{
   DestroyWindows();
}

void SpectrogramSettings::DestroyWindows()
{
   hFFT.reset();
   window.reset();
   dWindow.reset();
   tWindow.reset();
   kernels.clear();
   cQBottom = 1;
}


namespace
{
   // Scale the window function to give 0dB spectrum for 0dB sine tone
   double computeScale( float *window, size_t length )
   {
      double scale = 0.0;
      for (size_t ii = 0; ii < length; ++ii)
         scale += window[ii];
      if (scale > 0)
         scale = 2.0 / scale;
      return scale;
   }

   enum { WINDOW, TWINDOW, DWINDOW };
   void RecreateWindow(
      Floats &window, int which, size_t fftLen,
      size_t padding, int windowType, size_t windowSize, double &scale)
   {
      // Create the requested window function
      window = Floats{ fftLen };
      size_t ii;

      const bool extra = padding > 0;
      wxASSERT(windowSize % 2 == 0);
      if (extra)
         // For windows that do not go to 0 at the edges, this improves symmetry
         ++windowSize;
      const size_t endOfWindow = padding + windowSize;
      // Left and right padding
      for (ii = 0; ii < padding; ++ii) {
         window[ii] = 0.0;
         window[fftLen - ii - 1] = 0.0;
      }
      // Default rectangular window in the middle
      for (; ii < endOfWindow; ++ii)
         window[ii] = 1.0;
      // Overwrite middle as needed
      switch (which) {
      case WINDOW:
         NewWindowFunc(windowType, windowSize, extra, window.get() + padding);
         break;
      case TWINDOW:
         NewWindowFunc(windowType, windowSize, extra, window.get() + padding);
         {
            for (int jj = padding, multiplier = -(int)windowSize / 2; jj < (int)endOfWindow; ++jj, ++multiplier)
               window[jj] *= multiplier;
         }
         break;
      case DWINDOW:
         DerivativeOfWindowFunc(windowType, windowSize, extra, window.get() + padding);
         break;
      default:
         wxASSERT(false);
      }

      if (which == WINDOW)
         scale = computeScale( window.get() + padding, windowSize );
      for (ii = padding; ii < endOfWindow; ++ii)
         window[ii] *= scale;
   }
}

SpectrogramSettings::ConstantQSettings::ConstantQSettings( double steps )
   : stepsPerOctave{ steps }
   , ratio{ pow( 2.0, 1.0 / stepsPerOctave ) }
   , sqrtRatio{ sqrt( ratio ) }
   , Q{ sqrtRatio / ( ratio - 1.0 ) }
{}

auto SpectrogramSettings::GetConstantQSettings() -> const ConstantQSettings &
{
   double N = 12; // resolve semitones
   // double N = 24; // resolve quarter-tones
   // double N = 3.0103; // ten per decade, as with equalization sliders
   static ConstantQSettings settings{ N };
   return settings;
}

void SpectrogramSettings::CacheWindows() const
{
   if (hFFT == NULL || window == NULL) {

      double scale;
      const auto fftLen = WindowSize() * ZeroPaddingFactor();
      const auto padding = (WindowSize() * (zeroPaddingFactor - 1)) / 2;

      hFFT = GetFFT(fftLen);
      RecreateWindow(window, WINDOW, fftLen, padding, windowType, windowSize, scale);
      if (algorithm == algReassignment) {
         RecreateWindow(tWindow, TWINDOW, fftLen, padding, windowType, windowSize, scale);
         RecreateWindow(dWindow, DWINDOW, fftLen, padding, windowType, windowSize, scale);
      }
      if (algorithm == algConstantQ) {
         /*
          Method desribed in:
          http://academics.wellesley.edu/Physics/brown/pubs/effalgV92P2698-P2701.pdf

          To compute one band of constant Q, we will convolve the sound
          with a windowed complex sinusoid (a "kernel" function).

          To compute these many convolutions efficiently, use Parseval's
          identity:
          sum [ s(t) k(t) ] = (1/N) sum [ S(f) K(f) ]
          where s is the sound, k is the kernel, S and K are their discrete
          Fourier transforms, and N is the FFT size.

          Thus, for each band, compute frequency-domain weights just
          once; then for each window of samples, take FFT once, and take
          a weighted sum of coefficients for each band; furthermore treat
          many coefficients as negligible to make this still run fast.

          The kernel k is conjugate-symmetric, therefore the coefficients
          of K are all real.

          The sound s is real, therefore the coefficients of S are conjugate-
          symmetric, and the RealFFTf function stores only the coefficients
          for the nonnegative frequencies.

          Thus the symmetric part of K times two, that is (K(f) + K(-f)), can
          weight just the stored real part of S, and the alternating part of K
          times two, which is (K(f) - K(-f)), the stored imaginary part of S.

          To compute the coefficients K, the trick is to take FFT of the
          real function Re(k) + Im(k).  This puts the symmetric part of K in the
          real places of the result, and the alternating in the imaginary.
          But then the weights are not used according the the rules of complex
          arithmetic, but rather as described above.
          */

         const auto &cqSettings = GetConstantQSettings();

         double linearBin = cqSettings.Q;
         cQBottom = linearBin / cqSettings.sqrtRatio;

         ArrayOf<float> scratch{ fftLen };
         auto half = fftLen / 2;
         for ( ; linearBin < half; linearBin *= cqSettings.ratio )
         {
            // Find the length of the window, Q cycles (with some roundoff)
            auto period = fftLen / linearBin;
            auto halfShortWindow = size_t(period * cqSettings.Q / 2);
            auto shortWindowSize = 2 * halfShortWindow;
            wxASSERT( shortWindowSize <= fftLen );

            // Compute the window function, centered in the larger window
            auto padding = ( fftLen - shortWindowSize ) / 2;
            auto size = padding * sizeof(float);
            memset(&scratch[0],                    0, size);
            memset(&scratch[0] + fftLen - padding, 0, size);
            auto shortWindow = &scratch[ padding ];
            std::fill( shortWindow, shortWindow + shortWindowSize, 1.0f );
            NewWindowFunc(windowType, shortWindowSize, true, shortWindow);

            // Normalize it
            auto scale =
               computeScale( shortWindow, shortWindowSize )
               * 2      // So that the K's weight both positive and negative
                        // frequencies of S
               / fftLen // The 1/N in Parseval's identity
            ;
            for (size_t ii = 0; ii < shortWindowSize; ++ii)
               shortWindow[ii] *= scale;

            // Multiply by sine plus cosine with zero phase at the center
            for( size_t ii = 1; ii < halfShortWindow; ++ii ) {
               double angle = 2 * M_PI * ii / period;
               double sine = sin(angle), cosine = cos(angle);
               scratch[ half + ii ] *= cosine + sine;
               scratch[ half - ii ] *= cosine - sine;
            }
            {
               // one more
               double angle = 2 * M_PI * halfShortWindow / period;
               double sine = sin(angle), cosine = cos(angle);
               scratch[ half - halfShortWindow ] *= cosine - sine;
            }

            RealFFTf( &scratch[0], hFFT.get() );

            // Coefficient value small enough to neglect
            // Some experiment hit on this:
            double threshold = 1.0 / (2.5 * half);
            threshold *= threshold;

            // Record the coefficients
            Kernel kernel;
            size_t firstBin = 1, lastBin = half - 1;

            float even, odd;
            for( ; firstBin < half; ++firstBin ) {
               auto index = hFFT->BitReversed[ firstBin ];
               auto even = scratch[ index ], odd = scratch[ index + 1 ];
               wxASSERT( !( std::isnan( even ) || std::isnan( odd ) ) );
               if ( even * even + odd * odd  > threshold )
                  break;
            }

            for( ; lastBin >= firstBin; --lastBin ) {
               auto index = hFFT->BitReversed[ lastBin ];
               auto even = scratch[ index ], odd = scratch[ index + 1 ];
               wxASSERT( !( std::isnan( even ) || std::isnan( odd ) ) );
               if ( even * even + odd * odd  > threshold )
                  break;
            }

            kernel.startBin = firstBin;

            for( auto bin = firstBin; bin <= lastBin; ++bin ) {
               auto index = hFFT->BitReversed[ bin ];
               auto even = scratch[ index ], odd = scratch[ index + 1 ];
               wxASSERT( !( std::isnan( even ) || std::isnan( odd ) ) );
               kernel.weights.push_back(even);
               kernel.weights.push_back(odd);
            }

            kernels.push_back( std::move( kernel ) );
         }
      }
   }
}

void SpectrogramSettings::ConvertToEnumeratedWindowSizes()
{
   unsigned size;
   int logarithm;

   logarithm = -LogMinWindowSize;
   size = unsigned(windowSize);
   while (size > 1)
      size >>= 1, ++logarithm;
   windowSize = std::max(0, std::min(NumWindowSizes - 1, logarithm));

#ifdef EXPERIMENTAL_ZERO_PADDED_SPECTROGRAMS
   // Choices for zero padding begin at 1
   logarithm = 0;
   size = unsigned(zeroPaddingFactor);
   while (zeroPaddingFactor > 1)
      zeroPaddingFactor >>= 1, ++logarithm;
   zeroPaddingFactor = std::max(0,
      std::min(LogMaxWindowSize - (windowSize + LogMinWindowSize),
         logarithm
   ));
#endif
}

void SpectrogramSettings::ConvertToActualWindowSizes()
{
   windowSize = 1 << (windowSize + LogMinWindowSize);
#ifdef EXPERIMENTAL_ZERO_PADDED_SPECTROGRAMS
   zeroPaddingFactor = 1 << zeroPaddingFactor;
#endif
}

float SpectrogramSettings::findBin( float frequency, float binUnit ) const
{
   float linearBin = frequency / binUnit;
   if (linearBin < 0)
      return -1;
   else if (algorithm == algConstantQ)
      return log( linearBin / cQBottom ) / log( GetConstantQSettings().ratio );
   else
      return linearBin;
}

size_t SpectrogramSettings::GetFFTLength() const
{
#ifndef EXPERIMENTAL_ZERO_PADDED_SPECTROGRAMS
   return windowSize;
#else
   return windowSize * ((algorithm != algPitchEAC) ? zeroPaddingFactor : 1);
#endif
}

size_t SpectrogramSettings::NBins() const
{
   if ( algorithm == algConstantQ )
      return kernels.size();
   else
      // Omit the Nyquist frequency bin
      return GetFFTLength() / 2;
}

NumberScale SpectrogramSettings::GetScale( float minFreqIn, float maxFreqIn ) const
{
   NumberScaleType type = nstLinear;

   // Don't assume the correspondence of the enums will remain direct in the future.
   // Do this switch.
   switch (scaleType) {
   default:
      wxASSERT(false);
   case stLinear:
      type = nstLinear; break;
   case stLogarithmic:
      type = nstLogarithmic; break;
   case stMel:
      type = nstMel; break;
   case stBark:
      type = nstBark; break;
   case stErb:
      type = nstErb; break;
   case stPeriod:
      type = nstPeriod; break;
   }

   return NumberScale(type, minFreqIn, maxFreqIn);
}

bool SpectrogramSettings::SpectralSelectionEnabled() const
{
#ifdef SPECTRAL_SELECTION_GLOBAL_SWITCH
   return Globals::Get().spectralSelection;
#else
   return spectralSelection;
#endif
}
