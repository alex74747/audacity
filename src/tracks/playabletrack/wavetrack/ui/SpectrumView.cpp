/**********************************************************************

Audacity: A Digital Audio Editor

SpectrumView.cpp

Paul Licameli split from WaveTrackView.cpp

**********************************************************************/

#include "../../../../Audacity.h"
#include "SpectrumView.h"

#include "../../../../Experimental.h"

#include "SpectrumVRulerControls.h"
#include "WaveTrackView.h"
#include "WaveTrackViewConstants.h"

#include "../../../../AColor.h"
#include "../../../../Prefs.h"
#include "../../../../NumberScale.h"
#include "../../../../TrackArtist.h"
#include "../../../../TrackPanelDrawingContext.h"
#include "../../../../ViewInfo.h"
#include "../../../../WaveClip.h"
#include "../../../../WaveTrack.h"
#include "../../../../prefs/SpectrogramSettings.h"

#include <wx/dcmemory.h>
#include <wx/graphics.h>

static WaveTrackSubView::Type sType{
   WaveTrackViewConstants::Spectrum,
   { wxT("Spectrogram"), XXO("&Spectrogram") }
};

static WaveTrackSubViewType::RegisteredType reg{ sType };

SpectrumView::~SpectrumView() = default;

bool SpectrumView::IsSpectral() const
{
   return true;
}

int SpectrumView::NumExtraPixelColumns(const AudacityProject &project)
{
   int extraColumns = 0;
   for (auto wt: TrackList::Get(project).Any<const WaveTrack>()) {
      auto &view = WaveTrackView::Get(*wt);
      if (view.GetMinimized())
         continue;
      const auto displays = view.GetDisplays();
         bool hasSpectral = (displays.end() != std::find(
            displays.begin(), displays.end(),
            WaveTrackSubView::Type{ WaveTrackViewConstants::Spectrum, {} }
         ) );
      if (hasSpectral) {
         const SpectrogramSettings &settings = wt->GetSpectrogramSettings();
         if (settings.style != SpectrogramSettings::styleFlat) {
            auto height = view.GetHeight() - (kTopMargin + kBottomMargin);
            const int extra = int(0.5 + (height - 1) / settings.GetSlope());
            extraColumns = std::max(extraColumns, extra);
         }
      }
   }
   return extraColumns;
}

std::vector<UIHandlePtr> SpectrumView::DetailedHitTest(
   const TrackPanelMouseState &state,
   const AudacityProject *pProject, int currentTool, bool bMultiTool )
{
   const auto wt = std::static_pointer_cast< WaveTrack >( FindTrack() );

   return WaveTrackSubView::DoDetailedHitTest(
      state, pProject, currentTool, bMultiTool, wt
   ).second;
}

void SpectrumView::DoSetMinimized( bool minimized )
{
   auto wt = static_cast<WaveTrack*>( FindTrack().get() );

#ifdef EXPERIMENTAL_HALF_WAVE
   bool bHalfWave;
   gPrefs->Read(wxT("/GUI/CollapseToHalfWave"), &bHalfWave, false);
   if( bHalfWave && minimized)
   {
      // It is all right to set the top of scale to a huge number,
      // not knowing the track rate here -- because when retrieving the
      // value, then we pass in a sample rate and clamp it above to the
      // Nyquist frequency.
      constexpr auto max = std::numeric_limits<float>::max();
      const bool spectrumLinear =
         (wt->GetSpectrogramSettings().scaleType ==
            SpectrogramSettings::stLinear);
      // Zoom out full
      wt->SetSpectrumBounds( spectrumLinear ? 0.0f : 1.0f, max );
   }
#endif

   TrackView::DoSetMinimized( minimized );
}

auto SpectrumView::SubViewType() const -> const Type &
{
   return sType;
}

std::shared_ptr<TrackVRulerControls> SpectrumView::DoGetVRulerControls()
{
   return std::make_shared<SpectrumVRulerControls>( shared_from_this() );
}

namespace
{

inline
AColor::ColorGradientChoice ChangeColorSet
(AColor::ColorGradientChoice colorSet, bool timeOnly)
{
   switch (colorSet) {
   case AColor::ColorGradientUnselected:
   case AColor::ColorGradientTimeSelected:
      return colorSet;
   default:
      if (timeOnly)
         return AColor::ColorGradientTimeSelected;
      else
         return AColor::ColorGradientTimeAndFrequencySelected;
   }
}

static inline float findValue
(const float *spectrum, float bin0, float bin1, unsigned nBins,
 bool autocorrelation, int gain, int range)
{
   float value;


#if 0
   // Averaging method
   if ((int)(bin1) == (int)(bin0)) {
      value = spectrum[(int)(bin0)];
   } else {
      float binwidth= bin1 - bin0;
      value = spectrum[(int)(bin0)] * (1.f - bin0 + (int)bin0);

      bin0 = 1 + (int)(bin0);
      while (bin0 < (int)(bin1)) {
         value += spectrum[(int)(bin0)];
         bin0 += 1.0;
      }
      // Do not reference past end of freq array.
      if ((int)(bin1) >= (int)nBins) {
         bin1 -= 1.0;
      }

      value += spectrum[(int)(bin1)] * (bin1 - (int)(bin1));
      value /= binwidth;
   }
#else
   // Maximum method, and no apportionment of any single bins over multiple pixel rows
   // See Bug971
   int index, limitIndex;
   if (autocorrelation) {
      // bin = 2 * nBins / (nBins - 1 - array_index);
      // Solve for index
      index = std::max(0.0f, std::min(float(nBins - 1),
         (nBins - 1) - (2 * nBins) / (std::max(1.0f, bin0))
      ));
      limitIndex = std::max(0.0f, std::min(float(nBins - 1),
         (nBins - 1) - (2 * nBins) / (std::max(1.0f, bin1))
      ));
   }
   else {
      index = std::min<int>(nBins - 1, (int)(floor(0.5 + bin0)));
      limitIndex = std::min<int>(nBins, (int)(floor(0.5 + bin1)));
   }
   value = spectrum[index];
   while (++index < limitIndex)
      value = std::max(value, spectrum[index]);
#endif
   if (!autocorrelation) {
      // Last step converts dB to a 0.0-1.0 range
      value = (value + range + gain) / (double)range;
   }
   value = std::min(1.0f, std::max(0.0f, value));
   return value;
}

// dashCount counts both dashes and the spaces between them.
inline AColor::ColorGradientChoice
ChooseColorSet( float bin0, float bin1, float selBinLo,
   float selBinCenter, float selBinHi, int dashCount, bool isSpectral )
{
   if (!isSpectral)
      return  AColor::ColorGradientTimeSelected;
   if ((selBinCenter >= 0) && (bin0 <= selBinCenter) &&
       (selBinCenter < bin1))
      return AColor::ColorGradientEdge;
   if ((0 == dashCount % 2) &&
       (((selBinLo >= 0) && (bin0 <= selBinLo) && ( selBinLo < bin1))  ||
        ((selBinHi >= 0) && (bin0 <= selBinHi) && ( selBinHi < bin1))))
      return AColor::ColorGradientEdge;
   if ((selBinLo < 0 || selBinLo < bin1) && (selBinHi < 0 || selBinHi > bin0))
      return  AColor::ColorGradientTimeAndFrequencySelected;
   
   return  AColor::ColorGradientTimeSelected;
}

inline
void FadeRGB
(unsigned char &rv, unsigned char &bv, unsigned char &gv,
 unsigned char r0, unsigned char b0, unsigned char g0)
{
   rv = (char)(((int)rv + 3 * (int)r0) / 4);
   gv = (char)(((int)gv + 3 * (int)g0) / 4);
   bv = (char)(((int)bv + 3 * (int)b0) / 4);
}

// Find the highest grid frequency, in bins, that is at or below the given bin
inline
float GridLine(float bin, float binUnit, SpectrogramSettings::Grid gridType)
{
   static const float middleC = 261.6255653f;

   switch (gridType) {

   case SpectrogramSettings::gridNone:
   default:
      return -1.0f;

   case SpectrogramSettings::gridKHz:
   {
      const float kHz = bin * binUnit / 1000;
      return (1000 / binUnit) * floor(kHz);
   }

   case SpectrogramSettings::grid31Bands:
   {
      // Ten bands per decade from 20 to 20,000, as with the graphic equalizer
      const float bands = 10 * log10(bin * binUnit / 20);
      if (bands < 0)
         return -1;
      return (20 / binUnit) *
         pow(10, std::min<float>(31.0f, floor(bands)) / 10);
   }

   case SpectrogramSettings::gridDecades:
   {
      // 2, 20, 200, 2000, etc.
      const float decades = log10(bin * binUnit / 20);
      return (20 / binUnit) * pow(10, floor(decades));
   }

   case SpectrogramSettings::gridChromatic:
   {
      // A 440 tuning.
      const float semitones = 12 * log2(bin * binUnit / middleC);
      return (middleC / binUnit) * pow(2, floor(semitones) / 12);
   }

   case SpectrogramSettings::gridOctaves:
   {
      // At the C's, with A 440 tuning.
      const float octaves = log2(bin * binUnit / middleC);
      return (middleC / binUnit) * pow(2, floor(octaves));
   }

   }
}

void DrawClipSpectrum(TrackPanelDrawingContext &context,
                                   WaveTrackCache &waveTrackCache,
                                   const WaveClip *clip,
                                   const wxRect & rect)
{
   auto &dc = context.dc;
   const auto artist = TrackArtist::Get( context );
   const auto &selectedRegion = *artist->pSelectedRegion;
   const auto &zoomInfo = *artist->pZoomInfo;

#ifdef PROFILE_WAVEFORM
   Profiler profiler;
#endif

   const WaveTrack *const track = waveTrackCache.GetTrack().get();
   const SpectrogramSettings &settings = track->GetSpectrogramSettings();
   const bool autocorrelation = (settings.algorithm == SpectrogramSettings::algPitchEAC);

   enum { DASH_LENGTH = 10 /* pixels */ };

   const ClipParameters params{
      true, track, clip, rect, selectedRegion, zoomInfo };
   const wxRect &hiddenMid = params.hiddenMid;
   // The "hiddenMid" rect contains the part of the display actually
   // containing the waveform, as it appears without the fisheye.  If it's empty, we're done.
   if (hiddenMid.width <= 0) {
      return;
   }

   const double &t0 = params.t0;
   const double &tOffset = params.tOffset;
   const auto &ssel0 = params.ssel0;
   const auto &ssel1 = params.ssel1;
   const double &averagePixelsPerSample = params.averagePixelsPerSample;
   const double &rate = params.rate;
   const double &hiddenLeftOffset = params.hiddenLeftOffset;
   const double &leftOffset = params.leftOffset;
   const wxRect &mid = params.mid;

   double freqLo = SelectedRegion::UndefinedFrequency;
   double freqHi = SelectedRegion::UndefinedFrequency;
#ifdef EXPERIMENTAL_SPECTRAL_EDITING
   freqLo = selectedRegion.f0();
   freqHi = selectedRegion.f1();
#endif

   const bool &isGrayscale = settings.isGrayscale;
   const int &range = settings.range;
   const int &gain = settings.gain;

#ifdef EXPERIMENTAL_FIND_NOTES
   const bool &fftFindNotes = settings.fftFindNotes;
   const double &findNotesMinA = settings.findNotesMinA;
   const int &numberOfMaxima = settings.numberOfMaxima;
   const bool &findNotesQuantize = settings.findNotesQuantize;
#endif
#ifdef EXPERIMENTAL_FFT_Y_GRID
   const bool &fftYGrid = settings.fftYGrid;
#endif

   dc.SetPen(*wxTRANSPARENT_PEN);

   const SpectrogramSettings::Style style = settings.style;
   const bool waterfall =
#ifdef EXPERIMENTAL_WATERFALL_SPECTROGRAMS
      style != SpectrogramSettings::styleFlat;
#else
      false
#endif
      ;

   // We draw directly to a bit image in memory,
   // and then paint this directly to our offscreen
   // bitmap.  Note that this could be optimized even
   // more, but for now this is not bad.  -dmazzoni
   wxImage image((int)mid.width, (int)mid.height);
   if (!image.IsOk())
      return;
#ifdef EXPERIMENTAL_SPECTROGRAM_OVERLAY
   image.SetAlpha();
   unsigned char *alpha = image.GetAlpha();
#endif
   unsigned char *data = image.GetData();

   const auto half = settings.GetFFTLength() / 2;
   const double binUnit = rate / (2 * half);
   const float *freq = 0;
   const sampleCount *where = 0;
   bool updated;
   {
      const double pps = averagePixelsPerSample * rate;
      updated = clip->GetSpectrogram(waveTrackCache, freq, where,
                                     (size_t)hiddenMid.width,
         t0, pps);
   }
   auto nBins = settings.NBins();

   float minFreq, maxFreq;
   track->GetSpectrumBounds(&minFreq, &maxFreq);

   const SpectrogramSettings::ScaleType scaleType = settings.scaleType;

   // nearest frequency to each pixel row from number scale, for selecting
   // the desired fft bin(s) for display on that row
   float *bins = (float*)alloca(sizeof(*bins)*(hiddenMid.height + 1));
   {
      const NumberScale numberScale( settings.GetScale( minFreq, maxFreq ) );

      NumberScale::Iterator it = numberScale.begin(mid.height);
      float nextBin = std::max( 0.0f, std::min( float(nBins - 1),
         settings.findBin( *it, binUnit ) ) );

      int yy;
      for (yy = 0; yy < hiddenMid.height; ++yy) {
         bins[yy] = nextBin;
         nextBin = std::max( 0.0f, std::min( float(nBins - 1),
            settings.findBin( *++it, binUnit ) ) );
      }
      bins[yy] = nextBin;
   }

#ifdef EXPERIMENTAL_FFT_Y_GRID
   const float
      log2 = logf(2.0f),
      scale2 = (lmax - lmin) / log2,
      lmin2 = lmin / log2;

   ArrayOf<bool> yGrid{size_t(mid.height)};
   for (int yy = 0; yy < mid.height; ++yy) {
      float n = (float(yy) / mid.height*scale2 - lmin2) * 12;
      float n2 = (float(yy + 1) / mid.height*scale2 - lmin2) * 12;
      float f = float(minFreq) / (fftSkipPoints + 1)*powf(2.0f, n / 12.0f + lmin2);
      float f2 = float(minFreq) / (fftSkipPoints + 1)*powf(2.0f, n2 / 12.0f + lmin2);
      n = logf(f / 440) / log2 * 12;
      n2 = logf(f2 / 440) / log2 * 12;
      if (floor(n) < floor(n2))
         yGrid[yy] = true;
      else
         yGrid[yy] = false;
   }
#endif //EXPERIMENTAL_FFT_Y_GRID

   if (!updated && clip->mSpecPxCache->valid &&
      ((int)clip->mSpecPxCache->len == hiddenMid.height * hiddenMid.width)
      && scaleType == clip->mSpecPxCache->scaleType
      && gain == clip->mSpecPxCache->gain
      && range == clip->mSpecPxCache->range
      && minFreq == clip->mSpecPxCache->minFreq
      && maxFreq == clip->mSpecPxCache->maxFreq
#ifdef EXPERIMENTAL_FFT_Y_GRID
   && fftYGrid==fftYGridOld
#endif //EXPERIMENTAL_FFT_Y_GRID
#ifdef EXPERIMENTAL_FIND_NOTES
   && fftFindNotes == artist->fftFindNotesOld
   && findNotesMinA == artist->findNotesMinAOld
   && numberOfMaxima == artist->findNotesNOld
   && findNotesQuantize == artist->findNotesQuantizeOld
#endif
   ) {
      // Wave clip's spectrum cache is up to date,
      // and so is the spectrum pixel value cache
   }
   else {
      // Update the spectrum pixel cache
      clip->mSpecPxCache = std::make_unique<SpecPxCache>(hiddenMid.width * hiddenMid.height);
      clip->mSpecPxCache->valid = true;
      clip->mSpecPxCache->scaleType = scaleType;
      clip->mSpecPxCache->gain = gain;
      clip->mSpecPxCache->range = range;
      clip->mSpecPxCache->minFreq = minFreq;
      clip->mSpecPxCache->maxFreq = maxFreq;
#ifdef EXPERIMENTAL_FIND_NOTES
      artist->fftFindNotesOld = fftFindNotes;
      artist->findNotesMinAOld = findNotesMinA;
      artist->findNotesNOld = numberOfMaxima;
      artist->findNotesQuantizeOld = findNotesQuantize;
#endif

#ifdef EXPERIMENTAL_FIND_NOTES
      float log2 = logf( 2.0f ),
         lmin = logf( minFreq ), lmax = logf( maxFreq ), scale = lmax - lmin,
         lmins = lmin,
         lmaxs = lmax
         ;
#endif //EXPERIMENTAL_FIND_NOTES

#ifdef EXPERIMENTAL_FIND_NOTES
      int maxima[128];
      float maxima0[128], maxima1[128];
      const float
         f2bin = half / (rate / 2.0f),
         bin2f = 1.0f / f2bin,
         minDistance = powf(2.0f, 2.0f / 12.0f),
         i0 = expf(lmin) / binUnit,
         i1 = expf(scale + lmin) / binUnit,
         minColor = 0.0f;
      const size_t maxTableSize = 1024;
      ArrayOf<int> indexes{ maxTableSize };
#endif //EXPERIMENTAL_FIND_NOTES

#ifdef _OPENMP
#pragma omp parallel for
#endif
      for (int xx = 0; xx < hiddenMid.width; ++xx) {
#ifdef EXPERIMENTAL_FIND_NOTES
         int maximas = 0;
         const int x0 = nBins * xx;
         if (fftFindNotes) {
            for (int i = maxTableSize - 1; i >= 0; i--)
               indexes[i] = -1;

            // Build a table of (most) values, put the index in it.
            for (int i = (int)(i0); i < (int)(i1); i++) {
               float freqi = freq[x0 + (int)(i)];
               int value = (int)((freqi + gain + range) / range*(maxTableSize - 1));
               if (value < 0)
                  value = 0;
               if (value >= maxTableSize)
                  value = maxTableSize - 1;
               indexes[value] = i;
            }
            // Build from the indices an array of maxima.
            for (int i = maxTableSize - 1; i >= 0; i--) {
               int index = indexes[i];
               if (index >= 0) {
                  float freqi = freq[x0 + index];
                  if (freqi < findNotesMinA)
                     break;

                  bool ok = true;
                  for (int m = 0; m < maximas; m++) {
                     // Avoid to store very close maxima.
                     float maxm = maxima[m];
                     if (maxm / index < minDistance && index / maxm < minDistance) {
                        ok = false;
                        break;
                     }
                  }
                  if (ok) {
                     maxima[maximas++] = index;
                     if (maximas >= numberOfMaxima)
                        break;
                  }
               }
            }

// The f2pix helper macro converts a frequency into a pixel coordinate.
#define f2pix(f) (logf(f)-lmins)/(lmaxs-lmins)*hiddenMid.height

            // Possibly quantize the maxima frequencies and create the pixel block limits.
            for (int i = 0; i < maximas; i++) {
               int index = maxima[i];
               float f = float(index)*bin2f;
               if (findNotesQuantize)
               {
                  f = expf((int)(log(f / 440) / log2 * 12 - 0.5) / 12.0f*log2) * 440;
                  maxima[i] = f*f2bin;
               }
               float f0 = expf((log(f / 440) / log2 * 24 - 1) / 24.0f*log2) * 440;
               maxima0[i] = f2pix(f0);
               float f1 = expf((log(f / 440) / log2 * 24 + 1) / 24.0f*log2) * 440;
               maxima1[i] = f2pix(f1);
            }
         }

         int it = 0;
         bool inMaximum = false;
#endif //EXPERIMENTAL_FIND_NOTES

         for (int yy = 0; yy < hiddenMid.height; ++yy) {
            const float bin     = bins[yy];
            const float nextBin = bins[yy+1];

            if (settings.scaleType != SpectrogramSettings::stLogarithmic) {
               const float value = findValue
                  (freq + nBins * xx, bin, nextBin, nBins, autocorrelation, gain, range);
               clip->mSpecPxCache->values[xx * hiddenMid.height + yy] = value;
            }
            else {
               float value;

#ifdef EXPERIMENTAL_FIND_NOTES
               if (fftFindNotes) {
                  if (it < maximas) {
                     float i0 = maxima0[it];
                     if (yy >= i0)
                        inMaximum = true;

                     if (inMaximum) {
                        float i1 = maxima1[it];
                        if (yy + 1 <= i1) {
                           value = findValue(freq + x0, bin, nextBin, nBins, autocorrelation, gain, range);
                           if (value < findNotesMinA)
                              value = minColor;
                        }
                        else {
                           it++;
                           inMaximum = false;
                           value = minColor;
                        }
                     }
                     else {
                        value = minColor;
                     }
                  }
                  else
                     value = minColor;
               }
               else
#endif //EXPERIMENTAL_FIND_NOTES
               {
                  value = findValue
                     (freq + nBins * xx, bin, nextBin, nBins, autocorrelation, gain, range);
               }
               clip->mSpecPxCache->values[xx * hiddenMid.height + yy] = value;
            } // logF
         } // each yy
      } // each xx
   } // updating cache

   float selBinLo = settings.findBin( freqLo, binUnit);
   float selBinHi = settings.findBin( freqHi, binUnit);
   float selBinCenter = (freqLo < 0 || freqHi < 0)
      ? -1
      : settings.findBin( sqrt(freqLo * freqHi), binUnit );

   const bool isSpectral = settings.SpectralSelectionEnabled();
   const bool hidden = (ZoomInfo::HIDDEN == zoomInfo.GetFisheyeState());
   const int begin = hidden
      ? 0
      : std::max(0, (int)(zoomInfo.GetFisheyeLeftBoundary(-leftOffset)));
   const int end = hidden
      ? 0
      : std::min(mid.width, (int)(zoomInfo.GetFisheyeRightBoundary(-leftOffset)));
   const size_t numPixels = std::max(0, end - begin);

   SpecCache specCache;

   // need explicit resize since specCache.where[] accessed before Populate()
   specCache.Grow(numPixels, settings, -1, t0);

   if (numPixels > 0) {
      // Calculate pixel value for the varying-zoom fisheye area
      for (int ii = begin; ii < end; ++ii) {
         const double time = zoomInfo.PositionToTime(ii, -leftOffset) - tOffset;
         specCache.where[ii - begin] = sampleCount(0.5 + rate * time);
      }
      specCache.Populate
         (settings, waveTrackCache,
          0, 0, numPixels,
          clip->GetNumSamples(),
          tOffset, rate,
          0 // FIXME: PRL -- make reassignment work with fisheye
       );
   }

   // Now use the "pixel value cache," which contains floats in the
   // range 0 to 1, to color pixels.  This is not straightforward.

   // First, for x and y coordinates in the rectangle,
   // find corresponding column and row of the cache, which involves a
   // shearing of x with higher y in case of waterfalls.
   
   // Then determine the number of pixels to color at and above those
   // coordinates as a function of the value in the cache.
   // That function is just 1 always for ordinary view but proprtional to the
   // value for waterfall.

   // Then paint the column.  But work bottom up, and for waterfalls, do not
   // overpaint what was already done.  This achieves the "hidden line removal."

   const bool doSilhouettes = waterfall &&
#ifdef EXPERIMENTAL_WATERFALL_SPECTROGRAMS
      style != SpectrogramSettings::styleSolid;
#else
      false
#endif
      ;
   const double waterfallSlope = settings.GetSlope();
   const int waterfallHeight = settings.waterfallHeight;

   // build color gradient tables (not thread safe)
   if (!AColor::gradient_inited)
      AColor::PreComputeGradient();

   // left pixel column of the fisheye
   int fisheyeLeft = zoomInfo.GetFisheyeLeftBoundary(-leftOffset);

   // Bug 2389 - always draw at least one pixel of selection.
   int selectedX = zoomInfo.TimeToPosition(selectedRegion.t0(), -leftOffset);

   // Remember the pixel height of each frequency for the previous column.
   std::vector<int> prevColumn;
   if (waterfall) {
      prevColumn.resize(mid.height, -1);
   }

   // Record the pixel heights at which there are "silhouette" lines
   struct SilhouetteData {
      int yy;
      float value;
      AColor::ColorGradientChoice selected;
      bool hidden;

      SilhouetteData(int yy_, float value_, AColor::ColorGradientChoice selected_, bool hidden_)
         : yy(yy_), value(value_), selected(selected_)
         , hidden(hidden_)
      {}
   };
   std::vector<SilhouetteData> silhouetteData;
   // std::vector<SilhouetteData> prevSilhouetteData;
   if (waterfall) {
      silhouetteData.reserve(hiddenMid.height);
      // prevSilhouetteData.reserve(hiddenMid.height);
   }

   // Get values to average with for fading of hidden lines
   unsigned char r0, g0, b0;
   GetColorGradient(0, AColor::ColorGradientUnselected,
      isGrayscale, &r0, &g0, &b0);

#ifdef _OPENMP
#pragma omp parallel for
#endif
   for (int xx = 0; xx < mid.width; ++xx) {

      int correctedX = xx + leftOffset - hiddenLeftOffset;
      bool inFisheye = zoomInfo.InFisheye(xx, -leftOffset);
      int fisheyeColumn = 0;

      // in fisheye mode the time scale has changed, so the row values aren't cached
      // in the loop above, and must be fetched from fft cache
      float* uncached;
      if (!inFisheye) {
          uncached = 0;
      }
      else {
          fisheyeColumn = xx - fisheyeLeft;
          int specIndex = fisheyeColumn * nBins;
          wxASSERT(specIndex >= 0 && specIndex < (int)specCache.freq.size());
          uncached = &specCache.freq[specIndex];
      }

      // zoomInfo must be queried for each column since with fisheye enabled
      // time between columns is variable
      auto w0 = sampleCount(0.5 + rate *
                   (zoomInfo.PositionToTime(xx, -leftOffset) - tOffset));

      auto w1 = sampleCount(0.5 + rate *
                    (zoomInfo.PositionToTime(xx+1, -leftOffset) - tOffset));

      if (waterfall)
         // Must recompute
         w0 = sampleCount(0.5 + rate *
            (zoomInfo.PositionToTime(xx, -leftOffset) - tOffset));

      bool maybeSelected = ssel0 <= w0 && w1 < ssel1;
      maybeSelected = maybeSelected || (xx == selectedX);

      int maxY = -1;
      float prevValue = 0;
      int prevZ = -1;
      AColor::ColorGradientChoice prevSelected = AColor::ColorGradientUnselected;
      bool findPeak = true;

      for (int yy = 0; yy < hiddenMid.height; ++yy) {
         const float bin     = bins[yy];
         const float nextBin = bins[yy+1];

         int waterfallAdjustX = 0;
         if (waterfall) {
            waterfallAdjustX = int(0.5 + yy / waterfallSlope);
            if (waterfallAdjustX) {
               inFisheye = zoomInfo.InFisheye(xx - waterfallAdjustX, -leftOffset);
               if (inFisheye && waterfallAdjustX < fisheyeColumn)
                  uncached = &specCache.freq[(fisheyeColumn - 1 - waterfallAdjustX) * half];
               else
                  uncached = 0;
               w0 = sampleCount(0.5 + rate *
                  (zoomInfo.PositionToTime(xx - waterfallAdjustX, -leftOffset) - tOffset));
               w1 = sampleCount(0.5 + rate *
                  (zoomInfo.PositionToTime(xx - waterfallAdjustX + 1, -leftOffset) - tOffset));
            }
         }
         bool drawGridline =
#ifdef EXPERIMENTAL_WATERFALL_SPECTROGRAMS
            bin <= GridLine(nextBin, binUnit, settings.grid)
#else
            false
#endif
            ;

         float value = uncached
            ? findValue(uncached, bin, nextBin, half, autocorrelation, gain, range)
            : correctedX >= waterfallAdjustX
               ? clip->mSpecPxCache->values[(correctedX - waterfallAdjustX) * hiddenMid.height + yy]
               : 0;
         const int height =
            waterfall ? std::max(1, int(0.5 + value * waterfallHeight)) : 1;
         int zz = yy + height - 1;

         if (doSilhouettes) {
            if (findPeak && zz < prevZ) {
               // The previous row is now discovered to be a peak.
               const bool hidden = prevZ < maxY;
               if(!hidden || style == SpectrogramSettings::styleWireframe)
                  silhouetteData.push_back
                  (SilhouetteData(prevZ, prevValue, prevSelected, hidden));
               // Now we must find a trough before finding another peak.
               findPeak = false;
            }
            else if (!findPeak && zz > prevZ) {
               // Found a trough between peaks.
               // Now we can find a peak again.
               findPeak = true;
            }
         }

         prevZ = zz;
         prevValue = value;

         // This test does easy "HLR"
         if (waterfall && zz <= maxY) {
            if (!drawGridline || style != SpectrogramSettings::styleWireframe) {
               prevColumn[yy] = -1;
               continue;
            }
         }

         // For spectral selection, determine what colour
         // set to use.  We use a darker selection if
         // in both spectral range and time range.

         AColor::ColorGradientChoice selected = AColor::ColorGradientUnselected;

         // If we are in the time selected range, then we may use a different color set.
         if (maybeSelected)
            selected =
               ChooseColorSet(bin, nextBin, selBinLo, selBinCenter, selBinHi,
                  (xx - waterfallAdjustX + leftOffset - hiddenLeftOffset) / DASH_LENGTH, isSpectral);
         prevSelected = selected;

         const int initZ = zz;
         int bottomZ = zz;
         if (waterfall && (drawGridline || selected == AColor::ColorGradientEdge)) {
            // Draw a longer stroke down so that the curve appears unbroken
            const int prev = prevColumn[yy];
            if (prev >= 0) {
               bottomZ = std::min(initZ, prev);
               zz = std::max(initZ, prev);
            }
         }

         const int prevMaxY = maxY;
         maxY = std::max(maxY, zz);

         if (doSilhouettes)
            // Draw gray for now, except where there are
            // selection or grid lines.
            value = 0;

         // Draw top-down, maybe switching from curves to other colors
         for (; drawGridline || zz > prevMaxY; --zz) {
            if (zz < mid.height) {
               unsigned char rv, gv, bv;
               GetColorGradient(value * (zz - yy + 1) / height,
                  drawGridline ? AColor::ColorGradientEdge : selected,
                  isGrayscale, &rv, &gv, &bv);

#ifdef EXPERIMENTAL_FFT_Y_GRID
               if (fftYGrid && yGrid[yy]) {
                  rv /= 1.1f;
                  gv /= 1.1f;
                  bv /= 1.1f;
               }
#endif //EXPERIMENTAL_FFT_Y_GRID

               int px = ((mid.height - 1 - zz) * mid.width + xx);
#ifdef EXPERIMENTAL_SPECTROGRAM_OVERLAY
               // More transparent the closer to zero intensity.
               alpha[px]= wxMin( 200, (value+0.3) * 500) ;
#endif
               px *=3;
               if (zz <= prevMaxY)
                  // Fade out a hidden but not removed curve in wireframe
                  FadeRGB(rv, bv, gv, r0, b0, g0);
               data[px++] = rv;
               data[px++] = gv;
               data[px] = bv;
            }

            if (zz == bottomZ) {
               // Maybe change color set.
               drawGridline = false;
               selected = ChangeColorSet(selected, (bin <= selBinLo));
            }
         }

         if (waterfall)
            prevColumn[yy] = initZ;
      } // each yy

      if (doSilhouettes) {
         // Deferred drawing of the tops of crests, after having decided
         // where they are.

         // To do:  use prevSilhouetteData to stroke short vertical lines of more than one
         // pixel, in case the silhouette is steep and otherwise looks disconnected.

         const int nn = silhouetteData.size();
         for (int ii = nn; ii--;) {
            const SilhouetteData &sdata = silhouetteData[ii];

            if (sdata.yy >= mid.height)
               continue;

            unsigned char rv, gv, bv;
            GetColorGradient(sdata.value,
               sdata.selected,
               isGrayscale, &rv, &gv, &bv);

#ifdef EXPERIMENTAL_FFT_Y_GRID
            if (fftYGrid && yGrid[yy]) {
               rv /= 1.1f;
               gv /= 1.1f;
               bv /= 1.1f;
            }
#endif //EXPERIMENTAL_FFT_Y_GRID

            if (sdata.hidden)
               FadeRGB(rv, gv, bv, r0, g0, b0);

            int px = ((mid.height - 1 - sdata.yy) * mid.width + xx) * 3;
            data[px++] = rv;
            data[px++] = gv;
            data[px] = bv;
         }
      }

      // prevSilhouetteData.swap(silhouetteData);

      silhouetteData.clear();
   } // each xx

   wxBitmap converted = wxBitmap(image);

   wxMemoryDC memDC;

   memDC.SelectObject(converted);

   dc.Blit(mid.x, mid.y, mid.width, mid.height, &memDC, 0, 0, wxCOPY, FALSE);

   // Draw clip edges, as also in waveform view, which improves the appearance
   // of split views
   params.DrawClipEdges( dc, rect );
}

}

void SpectrumView::DoDraw( TrackPanelDrawingContext &context,
                                const WaveTrack *track,
                                const wxRect & rect )
{
   const auto artist = TrackArtist::Get( context );
   const auto &blankSelectedBrush = artist->blankSelectedBrush;
   const auto &blankBrush = artist->blankBrush;
   TrackArt::DrawBackgroundWithSelection(
      context, rect, track, blankSelectedBrush, blankBrush );

   WaveTrackCache cache(track->SharedPointer<const WaveTrack>());
   for (const auto &clip: track->GetClips())
      DrawClipSpectrum( context, cache, clip.get(), rect );

   DrawBoldBoundaries( context, track, rect );
}

void SpectrumView::Draw(
   TrackPanelDrawingContext &context, const wxRect &rect, unsigned iPass )
{
   if ( iPass == TrackArtist::PassTracks ) {
      auto &dc = context.dc;
      // Update cache for locations, e.g. cutlines and merge points
      // Bug2588: do this for both channels, even if one is not drawn, so that
      // cut-line editing (which depends on the locations cache) works properly.
      // If both channels are visible, we will duplicate this effort, but that
      // matters little.
      for( auto channel:
          TrackList::Channels(static_cast<WaveTrack*>(FindTrack().get())) )
         channel->UpdateLocationsCache();

      const auto wt = std::static_pointer_cast<const WaveTrack>(
         FindTrack()->SubstitutePendingChangedTrack());

      const auto artist = TrackArtist::Get( context );
      
#if defined(__WXMAC__)
      wxAntialiasMode aamode = dc.GetGraphicsContext()->GetAntialiasMode();
      dc.GetGraphicsContext()->SetAntialiasMode(wxANTIALIAS_NONE);
#endif
      
      DoDraw( context, wt.get(), rect );
      
#if defined(__WXMAC__)
      dc.GetGraphicsContext()->SetAntialiasMode(aamode);
#endif
   }
   CommonTrackView::Draw( context, rect, iPass );
}

static const WaveTrackSubViews::RegisteredFactory key{
   []( WaveTrackView &view ){
      return std::make_shared< SpectrumView >( view );
   }
};

// The following attaches the spectrogram settings item to the wave track popup
// menu.  It is appropriate only to spectrum view and so is kept in this
// source file with the rest of the spectrum view implementation.
#include "WaveTrackControls.h"
#include "../../../../AudioIOBase.h"
#include "../../../../Menus.h"
#include "../../../../ProjectHistory.h"
#include "../../../../RefreshCode.h"
#include "../../../../prefs/PrefsDialog.h"
#include "../../../../prefs/SpectrumPrefs.h"
#include "../../../../widgets/AudacityMessageBox.h"
#include "../../../../widgets/PopupMenuTable.h"

namespace {
struct SpectrogramSettingsHandler : PopupMenuHandler {

   PlayableTrackControls::InitMenuData *mpData{};
   static SpectrogramSettingsHandler &Instance()
   {
      static SpectrogramSettingsHandler instance;
      return instance;
   }

   void OnSpectrogramSettings(wxCommandEvent &);

   void InitUserData(void *pUserData) override
   {
      mpData = static_cast< PlayableTrackControls::InitMenuData* >(pUserData);
   }

   void DestroyMenu() override
   {
      mpData = nullptr;
   }
};

void SpectrogramSettingsHandler::OnSpectrogramSettings(wxCommandEvent &)
{
   class ViewSettingsDialog final : public PrefsDialog
   {
   public:
      ViewSettingsDialog(wxWindow *parent, AudacityProject &project,
         const TranslatableString &title, PrefsPanel::Factories &factories,
         int page)
         : PrefsDialog(parent, &project, title, factories)
         , mPage(page)
      {
      }

      long GetPreferredPage() override
      {
         return mPage;
      }

      void SavePreferredPage() override
      {
      }

   private:
      const int mPage;
   };

   auto gAudioIO = AudioIOBase::Get();
   if (gAudioIO->IsBusy()){
      AudacityMessageBox(
         XO(
"To change Spectrogram Settings, stop any\n playing or recording first."),
         XO("Stop the Audio First"),
         wxOK | wxICON_EXCLAMATION | wxCENTRE);
      return;
   }

   WaveTrack *const pTrack = static_cast<WaveTrack*>(mpData->pTrack);

   PrefsPanel::Factories factories;
   // factories.push_back(WaveformPrefsFactory( pTrack ));
   factories.push_back(SpectrumPrefsFactory( pTrack ));
   const int page =
      // (pTrack->GetDisplay() == WaveTrackViewConstants::Spectrum) ? 1 :
      0;

   auto title = XO("%s:").Format( pTrack->GetName() );
   ViewSettingsDialog dialog(
      mpData->pParent, mpData->project, title, factories, page);

   if (0 != dialog.ShowModal()) {
      // Redraw
      AudacityProject *const project = &mpData->project;
      ProjectHistory::Get( *project ).ModifyState(true);
      //Bug 1725 Toolbar was left greyed out.
      //This solution is overkill, but does fix the problem and is what the
      //prefs dialog normally does.
      MenuCreator::RebuildAllMenuBars();
      mpData->result = RefreshCode::RefreshAll;
   }
}

PopupMenuTable::AttachedItem sAttachment{
   GetWaveTrackMenuTable(),
   { "SubViews/Extra" },
   std::make_unique<PopupMenuSection>( "SpectrogramSettings",
      // Conditionally add menu item for settings, if showing spectrum
      PopupMenuTable::Computed< WaveTrackPopupMenuTable >(
         []( WaveTrackPopupMenuTable &table ) -> Registry::BaseItemPtr {
            using Entry = PopupMenuTable::Entry;
            static const int OnSpectrogramSettingsID =
            GetWaveTrackMenuTable().ReserveId();

            const auto pTrack = &table.FindWaveTrack();
            const auto &view = WaveTrackView::Get( *pTrack );
            const auto displays = view.GetDisplays();
            bool hasSpectrum = (displays.end() != std::find(
               displays.begin(), displays.end(),
               WaveTrackSubView::Type{ WaveTrackViewConstants::Spectrum, {} }
            ) );
            if( hasSpectrum )
               // In future, we might move this to the context menu of the
               // Spectrum vertical ruler.
               // (But the latter won't be satisfactory without a means to
               // open that other context menu with keystrokes only, and that
               // would require some notion of a focused sub-view.)
               return std::make_unique<Entry>( "SpectrogramSettings",
                  Entry::Item,
                  OnSpectrogramSettingsID,
                  XXO("S&pectrogram Settings..."),
                  (wxCommandEventFunction)
                     (&SpectrogramSettingsHandler::OnSpectrogramSettings),
                  SpectrogramSettingsHandler::Instance(),
                  []( PopupMenuHandler &handler, wxMenu &menu, int id ){
                     // Bug 1253.  Shouldn't open preferences if audio is busy.
                     // We can't change them on the fly yet anyway.
                     auto gAudioIO = AudioIOBase::Get();
                     menu.Enable(id, !gAudioIO->IsBusy());
                  } );
            else
               return nullptr;
         } ) )
};
}

