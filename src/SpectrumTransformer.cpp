#include "Audacity.h"
#include "SpectrumTransformer.h"

#include <algorithm>
#include "FFT.h"
#include "WaveTrack.h"

SpectrumTransformer::SpectrumTransformer
(WindowFunctionChoice inWindowType,
 WindowFunctionChoice outWindowType,
 TrackFactory *factory,
 int windowSize, int stepsPerWindow, bool leadingPadding, bool trailingPadding)
: mLeadingPadding(leadingPadding)
, mTrailingPadding(trailingPadding)

, mFactory(factory)

, mWindowSize(windowSize)
, mSpectrumSize(1 + mWindowSize / 2)

, mStepsPerWindow(stepsPerWindow)
, mStepSize(mWindowSize / mStepsPerWindow)

, mQueue()

, hFFT(InitializeFFT(mWindowSize))
, mInSampleCount(0)
, mOutStepCount(0)
, mInWavePos(0)

, mFFTBuffer(mWindowSize)
, mInWaveBuffer(mWindowSize)
, mOutOverlapBuffer(mWindowSize)

, mInWindow()
, mOutWindow()

, mOutputTrack()
{
   // Check preconditions

   // Powers of 2 only!
   wxASSERT(mWindowSize > 0 &&
      0 == (mWindowSize & (mWindowSize - 1)));

   wxASSERT(mWindowSize % mStepsPerWindow == 0);

   wxASSERT(!(inWindowType == WFCRectangular && outWindowType == WFCRectangular));

   // To do:  check that inWindowType, outWindowType, and mStepsPerWindow
   // are compatible for correct overlap-add reconstruction.

   // Create windows as needed
   if (inWindowType != WFCRectangular) {
      mInWindow.resize(mWindowSize);
      std::fill(mInWindow.begin(), mInWindow.end(), 1.0f);
      NewWindowFunc(inWindowType, mWindowSize, false, &*mInWindow.begin());
   }
   if (outWindowType != WFCRectangular) {
      mOutWindow.resize(mWindowSize);
      std::fill(mOutWindow.begin(), mOutWindow.end(), 1.0f);
      NewWindowFunc(outWindowType, mWindowSize, false, &*mOutWindow.begin());
   }

   // Must scale one or the other window so overlap-add
   // comes out right
   double denom = 0;
   for (int ii = 0; ii < mWindowSize; ii += mStepSize) {
      denom +=
         (mInWindow.empty() ? 1.0 : mInWindow[ii])
         *
         (mOutWindow.empty() ? 1.0 : mOutWindow[ii]);
   }
   // It is ASSUMED that you have chosen window types and
   // steps per window, so that this sum denom would be the
   // same, starting the march anywhere from 0 to mStepSize - 1.
   // Else, your overlap-add won't be right, and the transformer
   // might not be an identity even when you do nothing to the
   // spectra.

   float *pWindow = 0;
   if (!mInWindow.empty())
      pWindow = &*mInWindow.begin();
   else if (!mOutWindow.empty())
      pWindow = &*mOutWindow.begin();
   else
      // Can only happen if both window types were rectangular
      wxASSERT(false);
   for (int ii = 0; ii < mWindowSize; ++ii)
      *pWindow++ /= denom;
}

SpectrumTransformer::~SpectrumTransformer()
{
   EndFFT(hFFT);
   for (int ii = 0, nn = mQueue.size(); ii < nn; ++ii)
      delete mQueue[ii];
}

bool SpectrumTransformer::ProcessTrack
(WaveTrack *track, int queueLength, sampleCount start, sampleCount len)
{
   if (track == NULL)
      return false;

   // Prepare clean queue
   ResizeQueue(queueLength);
   for (int ii = 0; ii < queueLength; ++ii)
      mQueue[ii]->Zero();

   // Clean input and output buffers
   {
      float *pFill;
      pFill = &mInWaveBuffer[0];
      std::fill(pFill, pFill + mWindowSize, 0.0f);
      pFill = &mOutOverlapBuffer[0];
      std::fill(pFill, pFill + mWindowSize, 0.0f);
   }

   if (mLeadingPadding)
   {
      // So that the queue gets primed with some windows,
      // zero-padded in front, the first having mStepSize
      // samples of wave data:
      mInWavePos = mWindowSize - mStepSize;
      // This starts negative, to count up until the queue fills:
      mOutStepCount = -(queueLength - 1)
         // ... and then must pass over the padded windows,
         // before the first full window:
         - (mStepsPerWindow - 1);
   }
   else
   {
      // We do not want leading zero padded windows
      mInWavePos = 0;
      mOutStepCount = -(queueLength - 1);
   }

   mInSampleCount = 0;

   mOutputTrack.reset(
      mFactory
      ? mFactory->NewWaveTrack(track->GetSampleFormat(), track->GetRate())
      : NULL);

   sampleCount bufferSize = track->GetMaxBlockSize();
   FloatVector buffer(bufferSize);

   bool bLoopSuccess = true;
   sampleCount blockSize;
   sampleCount samplePos = start;
   while (bLoopSuccess && samplePos < start + len) {
      //Get a blockSize of samples (smaller than the size of the buffer)
      blockSize = std::min(start + len - samplePos, track->GetBestBlockSize(samplePos));

      //Get the samples from the track and put them in the buffer
      track->Get((samplePtr)&buffer[0], floatSample, samplePos, blockSize);
      samplePos += blockSize;

      mInSampleCount += blockSize;
      bLoopSuccess = ProcessSamples(blockSize, &buffer[0]);
   }

   if (bLoopSuccess) {
      if (mTrailingPadding) {
         // Keep flushing empty input buffers through the history
         // windows until we've output exactly as many samples as
         // were input.
         // Well, not exactly, but not more than one step-size of extra samples
         // at the end.
         // We'll delete them below.

         FloatVector empty(mStepSize);
         while (bLoopSuccess && mOutStepCount * mStepSize < mInSampleCount) {
            bLoopSuccess = ProcessSamples(mStepSize, &empty[0]);
         }
      }
   }

   if (bLoopSuccess)
      // invoke derived method
      bLoopSuccess = FinishTrack();

   if (bLoopSuccess && mOutputTrack.get()) {
      // Flush the output WaveTrack (since it's buffered)
      mOutputTrack->Flush();

      // Take the output track and insert it in place of the original
      // sample data
      double t0 = mOutputTrack->LongSamplesToTime(start);
      double tLen = mOutputTrack->LongSamplesToTime(len);
      // Filtering effects always end up with more data than they started with.  Delete this 'tail'.
      mOutputTrack->HandleClear(tLen, mOutputTrack->GetEndTime(), false, false);
      bool bResult = track->ClearAndPaste(t0, t0 + tLen, &*mOutputTrack, true, false);
      wxASSERT(bResult); // TO DO: Actually handle this.
   }

   mOutputTrack.reset();
   return bLoopSuccess;
}

SpectrumTransformer::Window *SpectrumTransformer::NewWindow(int windowSize)
{
   return new Window(windowSize);
}

bool SpectrumTransformer::StartTrack()
{
   return true;
}

bool SpectrumTransformer::FinishTrack()
{
   return true;
}

int SpectrumTransformer::QueueSize() const
{
   int allocSize = mQueue.size();
   int size = mOutStepCount + allocSize - 1;
   if (mLeadingPadding)
      size += mStepsPerWindow - 1;

   return std::min(size, allocSize);
}

bool SpectrumTransformer::QueueIsFull() const
{
   if (mLeadingPadding)
      return (mOutStepCount >= -(mStepsPerWindow - 1));
   else
      return (mOutStepCount >= 0);
}

void SpectrumTransformer::ResizeQueue(int queueLength)
{
   int oldLen = mQueue.size();
   for (int ii = oldLen; ii-- > queueLength;)
      delete mQueue[ii];

   mQueue.resize(queueLength);
   for (int ii = oldLen; ii < queueLength; ++ii)
      // invoke derived method to get a queue element
      // with appropriate extra fields
      mQueue[ii] = NewWindow(mWindowSize);
}

void SpectrumTransformer::FillFirstWindow()
{
   // Transform samples to frequency domain, windowed as needed
   if (mInWindow.size() > 0)
      for (int ii = 0; ii < mWindowSize; ++ii)
         mFFTBuffer[ii] = mInWaveBuffer[ii] * mInWindow[ii];
   else
      memmove(&mFFTBuffer[0], &mInWaveBuffer[0], mWindowSize * sizeof(float));
   RealFFTf(&mFFTBuffer[0], hFFT);

   Window &window = *mQueue[0];

   // Store real and imaginary parts for later inverse FFT
   {
      float *pReal = &window.mRealFFTs[1];
      float *pImag = &window.mImagFFTs[1];
      int *pBitReversed = &hFFT->BitReversed[1];
      const int last = mSpectrumSize - 1;
      for (int ii = 1; ii < last; ++ii) {
         const int kk = *pBitReversed++;
         *pReal++ = mFFTBuffer[kk];
         *pImag++ = mFFTBuffer[kk + 1];
      }
      // DC and Fs/2 bins need to be handled specially
      const float dc = mFFTBuffer[0];
      window.mRealFFTs[0] = dc;

      const float nyquist = mFFTBuffer[1];
      window.mImagFFTs[0] = nyquist; // For Fs/2, not really imaginary
   }
}

void SpectrumTransformer::RotateWindows()
{
   Window *save = *mQueue.rbegin();
   mQueue.pop_back();
   mQueue.insert(mQueue.begin(), save);
}

bool SpectrumTransformer::ProcessSamples(sampleCount len, const float *buffer)
{
   bool success = true;
   while (success && len && mOutStepCount * mStepSize < mInSampleCount) {
      int avail = std::min(int(len), mWindowSize - mInWavePos);
      memmove(&mInWaveBuffer[mInWavePos], buffer, avail * sizeof(float));
      buffer += avail;
      len -= avail;
      mInWavePos += avail;

      if (mInWavePos == mWindowSize) {
         FillFirstWindow();

         // invoke derived method
         success = ProcessWindow();

         if (success && mOutputTrack.get()) {
            if (QueueIsFull()) {
               const int last = mSpectrumSize - 1;
               Window &window = **mQueue.rbegin();

               const float *pReal = &window.mRealFFTs[1];
               const float *pImag = &window.mImagFFTs[1];
               float *pBuffer = &mFFTBuffer[2];
               int nn = mSpectrumSize - 2;
               for (; nn--;) {
                  *pBuffer++ = *pReal++;
                  *pBuffer++ = *pImag++;
               }
               mFFTBuffer[0] = window.mRealFFTs[0];
               // The Fs/2 component is stored as the imaginary part of the DC component
               mFFTBuffer[1] = window.mImagFFTs[0];

               // Invert the FFT into the output buffer
               InverseRealFFTf(&mFFTBuffer[0], hFFT);

               // Overlap-add
               if (mOutWindow.size() > 0) {
                  float *pOut = &mOutOverlapBuffer[0];
                  float *pWindow = &mOutWindow[0];
                  int *pBitReversed = &hFFT->BitReversed[0];
                  for (int jj = 0; jj < last; ++jj) {
                     int kk = *pBitReversed++;
                     *pOut++ += mFFTBuffer[kk] * (*pWindow++);
                     *pOut++ += mFFTBuffer[kk + 1] * (*pWindow++);
                  }
               }
               else {
                  float *pOut = &mOutOverlapBuffer[0];
                  int *pBitReversed = &hFFT->BitReversed[0];
                  for (int jj = 0; jj < last; ++jj) {
                     int kk = *pBitReversed++;
                     *pOut++ += mFFTBuffer[kk];
                     *pOut++ += mFFTBuffer[kk + 1];
                  }
               }
            }
            float *outBuffer = &mOutOverlapBuffer[0];
            if (mOutStepCount >= 0) {
               // Output the first portion of the overlap buffer, they're done
               mOutputTrack->Append((samplePtr)outBuffer, floatSample, mStepSize);
            }
            if (QueueIsFull()) {
               // Shift the remainder over.
               memmove(outBuffer, outBuffer + mStepSize, sizeof(float)*(mWindowSize - mStepSize));
               std::fill(outBuffer + mWindowSize - mStepSize, outBuffer + mWindowSize, 0.0f);
            }
         }

         ++mOutStepCount;
         RotateWindows();

         // Shift input.
         memmove(&mInWaveBuffer[0], &mInWaveBuffer[mStepSize],
            (mWindowSize - mStepSize) * sizeof(float));
         mInWavePos -= mStepSize;
      }
   }

   return success;
}

SpectrumTransformer::Window::~Window()
{
}
