#include "Audacity.h"
#include "SpectrumTransformer.h"
#include <algorithm>
#include "FFT.h"
#include "WaveTrack.h"

enum { EXTRA_WINDOWS = 1 };

class SpectrumTransformer::InvFFTThread : public wxThread
{
public:
   explicit
      InvFFTThread(SpectrumTransformer *transformer, int queueLength)
      : wxThread(wxTHREAD_JOINABLE)
      , mTransformer(transformer)
      , mReadyForFFT(queueLength + EXTRA_WINDOWS)
      , mReadyForInvFFT()
   {}
   ~InvFFTThread() {}
   virtual ExitCode Entry();

   SpectrumTransformer *const mTransformer;
   wxSemaphore mReadyForFFT, mReadyForInvFFT;
};

wxThread::ExitCode SpectrumTransformer::InvFFTThread::Entry()
{
   while (true) {
      if (!mTransformer->PostProcess())
         // We got the NULL window pointer signifying the end
         break;
   }
   return 0;
}

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
, mQueueStart(-1)
, mQueueEnd(-1)

, hFFT(InitializeFFT(mWindowSize))
, mInSampleCount(0)
, mOutStepCount(0)
, mPartialBuffers(0)
, mInWavePos(0)

, mFFTBuffer(mWindowSize)
, mInvFFTBuffer(mWindowSize)
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
   mQueueStart = 0;
   mQueueEnd = mQueue.size() - 1;

   // Clean input and 7output buffers
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
      mPartialBuffers = mStepsPerWindow - 1;
      // This starts negative, to count up until the queue fills:
      mOutStepCount = -(queueLength - 1)
         // ... and then must pass over the padded windows,
         // before the first full window:
         - mPartialBuffers;
   }
   else
   {
      // We do not want leading zero padded windows
      mInWavePos = 0;
      mPartialBuffers = 0;
      mOutStepCount = -(queueLength - 1);
   }

   mInSampleCount = 0;

   mOutputTrack.reset(
      mFactory
      ? mFactory->NewWaveTrack(track->GetSampleFormat(), track->GetRate())
      : NULL);

   if (//false &&
       mOutputTrack.get()) {
      // wxThread::SetConcurrency(2);
      mInvFFTThread.reset(new InvFFTThread(this, queueLength));
      wxThreadError error = mInvFFTThread->Create();
      if (!error)
         error = mInvFFTThread->Run();
      if (error)
         return false;
   }

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
      bLoopSuccess =
         ProcessSamples(blockSize, &buffer[0]);
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
            bLoopSuccess =
               ProcessSamples(mStepSize, &empty[0]);
         }
      }
   }

   if (mInvFFTThread.get()) {
      // Guarantee at least one window for "input"
      if (!RotateWindows())
         return false;
      // Null that last input
      Window *&last = mQueue[(QueueSize() - 1 + mQueueStart) % mQueue.size()];
      delete last;
      last = NULL;
      // Send it to the worker thread as the signal to stop
      mInvFFTThread->mReadyForInvFFT.Post();
      mInvFFTThread->Wait();
      // Thank you kindly Mr. Thread.
      mInvFFTThread.reset();
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

bool SpectrumTransformer::TrackProgress()
{
   return true;
}

bool SpectrumTransformer::FinishTrack()
{
   return true;
}

int SpectrumTransformer::QueueSize() const
{
   int allocSize = mQueue.size() - EXTRA_WINDOWS;
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
   queueLength += EXTRA_WINDOWS;
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

   Window &window = Nth(0);

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

bool SpectrumTransformer::RotateWindows()
{
   /*
   Window *save = *mQueue.rbegin();
   mQueue.pop_back();
   mQueue.insert(mQueue.begin(), save);
   */
   if (mInvFFTThread.get()) {
      if (mInvFFTThread->mReadyForFFT.Wait())
         // semaphore error
         return false;
   }
   if (--mQueueStart < 0)
      mQueueStart = mQueue.size() - 1;
   return true;
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
         if (!RotateWindows())
            return false;
         FillFirstWindow();

         // invoke derived methods
         success = ProcessWindow()
            // && TrackProgress()
            ;

         if (!success) {
            // To do, tell other thread to quit
         }
         else if (mOutputTrack.get() && QueueIsFull()) {
            if (mInvFFTThread.get()) {
               if (mInvFFTThread->mReadyForInvFFT.Post())
                  // semaphore error
                  return false;
            }
            else
               PostProcess();
         }
         
         ++mOutStepCount;

         // Shift input.
         memmove(&mInWaveBuffer[0], &mInWaveBuffer[mStepSize],
            (mWindowSize - mStepSize) * sizeof(float));
         mInWavePos -= mStepSize;
      }
   }

   return success;
}

bool SpectrumTransformer::PostProcess()
{
   if (mInvFFTThread.get() &&
       mInvFFTThread->mReadyForInvFFT.Wait())
      // semaphore error
      return false;

   Window *pWindow = mQueue[mQueueEnd];
   if (!pWindow)
      // Thread is done!
      return false;

   // Use that window
   {
      Window &window = *pWindow;
      const float *pReal = &window.mRealFFTs[1];
      const float *pImag = &window.mImagFFTs[1];
      float *pBuffer = &mInvFFTBuffer[2];
      int nn = mSpectrumSize - 2;
      for (; nn--;) {
         *pBuffer++ = *pReal++;
         *pBuffer++ = *pImag++;
      }
      mInvFFTBuffer[0] = window.mRealFFTs[0];
      // The Fs/2 component is stored as the imaginary part of the DC component
      mInvFFTBuffer[1] = window.mImagFFTs[0];
   }

   // Done with the queue window, recycle it now!
   if (mInvFFTThread.get() &&
      mInvFFTThread->mReadyForFFT.Post())
      // semaphore error
      return false;

   if (--mQueueEnd < 0)
      mQueueEnd = mQueue.size() - 1;

   const int last = mSpectrumSize - 1;

   // Invert the FFT into the output buffer
   InverseRealFFTf(&mInvFFTBuffer[0], hFFT);

   // Overlap-add
   if (mOutWindow.size() > 0) {
      float *pOut = &mOutOverlapBuffer[0];
      float *pWindow = &mOutWindow[0];
      int *pBitReversed = &hFFT->BitReversed[0];
      for (int jj = 0; jj < last; ++jj) {
         int kk = *pBitReversed++;
         *pOut++ += mInvFFTBuffer[kk] * (*pWindow++);
         *pOut++ += mInvFFTBuffer[kk + 1] * (*pWindow++);
      }
   }
   else {
      float *pOut = &mOutOverlapBuffer[0];
      int *pBitReversed = &hFFT->BitReversed[0];
      for (int jj = 0; jj < last; ++jj) {
         int kk = *pBitReversed++;
         *pOut++ += mInvFFTBuffer[kk];
         *pOut++ += mInvFFTBuffer[kk + 1];
      }
   }

   float *outBuffer = &mOutOverlapBuffer[0];
   if (mPartialBuffers > 0)
      --mPartialBuffers;
   else {
      // Output the first portion of the overlap buffer, they're done
      mOutputTrack->Append((samplePtr)outBuffer, floatSample, mStepSize);
   }

   // Shift the remainder over.
   memmove(outBuffer, outBuffer + mStepSize, sizeof(float)*(mWindowSize - mStepSize));
   std::fill(outBuffer + mWindowSize - mStepSize, outBuffer + mWindowSize, 0.0f);

   return true;
}

SpectrumTransformer::Window::~Window()
{
}
