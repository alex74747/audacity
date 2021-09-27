/**********************************************************************

  Audacity: A Digital Audio Editor

  SpectralDataManager.h

  Edward Hui

*******************************************************************//*!

\class SpectralDataManager
\brief Performs the calculation for spectral editing

*//*******************************************************************/

#include "./SpectrumTransformer.h"

class AudacityProject;
class SpectralData;

class SpectralDataManager{
public:
   SpectralDataManager();
   ~SpectralDataManager();
   static int ProcessTrack(AudacityProject &project,
      WaveTrack& track, SpectralData &data);
   static int FindFrequencySnappingBin(WaveTrack *wt,
                                  long long startSC,
                                  int hopSize,
                                  double threshold,
                                  int targetFreqBin);

   static std::vector<int> FindHighestFrequencyBins(WaveTrack *wt,
                                          long long int startSC,
                                          int hopSize,
                                          double threshold,
                                          int targetFreqBin);
private:
   class Worker;
   struct Setting;
};

class SpectralDataManager::Worker
      : public TrackSpectrumTransformer{
public:
   Worker(const Setting &setting);
   ~Worker();

   struct MyWindow : public Window
   {
      MyWindow(size_t windowSize)
            : Window{ windowSize }
            , mSpectrums(windowSize / 2 + 1)
            , mGains(windowSize / 2 + 1)
      {

      }
      ~MyWindow() override;

      FloatVector mSpectrums;
      FloatVector mGains;
   };

   bool Process(WaveTrack* wt, const SpectralData *sDataPtr);
   int ProcessSnapping(WaveTrack *wt, long long int startSC, int hopSize, size_t winSize,
                           double threshold, int targetFreqBin);
   std::vector<int> ProcessOvertones(WaveTrack *wt, long long int startSC, int hopSize, size_t winSize,
                       double threshold, int targetFreqBin);

protected:
   MyWindow &NthWindow(int nn) {
      return static_cast<MyWindow&>(Nth(nn));
   }
   std::unique_ptr<Window> NewWindow(size_t windowSize) override;
   bool DoStart() override;
   static bool Processor(SpectrumTransformer &transformer);
   static bool OvertonesProcessor(SpectrumTransformer &transformer);
   static bool SnappingProcessor(SpectrumTransformer &transformer);
   bool DoFinish() override;

private:
   bool ApplyEffectToSelection();
   const SpectralData *mpSpectralData;
   int mWindowCount { 0 };
   double mSnapSamplingRate;
   double mSnapThreshold;
   double mOvertonesThreshold;
   std::vector<int> mOvertonesTargetFreqBin;
   int mSnapTargetFreqBin;
   int mSnapReturnFreqBin { -1 };
   long long mStartHopNum { 0 };
};
