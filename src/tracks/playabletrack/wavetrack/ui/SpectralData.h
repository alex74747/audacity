#ifndef __AUDACITY_SPECTRAL_DATA__
#define __AUDACITY_SPECTRAL_DATA__

#include <map>
#include <set>
#include <vector>

using HopsAndBinsMap = std::map<long long, std::set<int>>;

class SpectralData{
private:
   double mSampleRate;
   int mWindowSize;
   int mHopSize;
   long long mStartSample;
   long long mEndSample;

public:
    SpectralData(double sr)
    :mSampleRate(sr)
    // Set start and end in reverse for comparison during data addition
    ,mStartSample(std::numeric_limits<long long>::max())
    ,mEndSample( 0 )
    ,mWindowSize( 2048 )
    ,mHopSize ( mWindowSize / 4 )
    {}
    SpectralData(const SpectralData& src) = delete;

    HopsAndBinsMap dataBuffer;
    std::vector<HopsAndBinsMap> dataHistory;
   // TODO: replace with two pairs to save space
   std::vector<std::pair<int, int>> coordHistory;

   // Abstracted the copy method for future extension
   void CopyFrom(const std::shared_ptr<SpectralData> &src){
      mStartSample = src->GetStartSample();
      mEndSample = src->GetEndSample();

      // std containers will perform deepcopy automatically
      dataHistory = src->dataHistory;
      dataBuffer = src->dataBuffer;
      coordHistory = src->coordHistory;
   }

   int GetHopSize() const {
      return mHopSize;
   }

   int GetWindowSize() const{
      return mWindowSize;
   };

   double GetSR() const{
      return mSampleRate;
   }

   long long GetStartSample() const{
      return mStartSample;
   }

   long long GetEndSample() const{
      return mEndSample;
   }

   // The double time points is quantized into long long
   void addHopBinData(int hopNum, int freqBin){
      // Update the start and end sampleCount of current selection
      if(hopNum * mHopSize > mEndSample)
         mEndSample = hopNum * mHopSize;
      if(hopNum * mHopSize < mStartSample)
         mStartSample = hopNum * mHopSize;

      dataBuffer[hopNum].insert(freqBin);
   }

   void removeHopBinData(int hopNum, int freqBin){
      // TODO: Recalculate the start and end in case hop falls in 0 || end
      for(auto &dataBuf: dataHistory){
         if(dataBuf.find(hopNum) != dataBuf.end()){
            dataBuf[hopNum].erase(freqBin);
         }
      }
   }

   void clearAllData(){
      // DataBuffer should be clear when the user release cursor
      dataHistory.clear();
      mStartSample = std::numeric_limits<long long>::max();
      mEndSample = 0;
   }

   void saveAndClearBuffer(){
      dataHistory.emplace_back(dataBuffer);
      dataBuffer.clear();
      coordHistory.clear();
   }
};

#endif

