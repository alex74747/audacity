/**********************************************************************

Audacity: A Digital Audio Editor

SpectrumTransformer.h

Paul Licameli

**********************************************************************/

#ifndef __AUDACITY_SPECTRUM_TRANSFORMER__
#define __AUDACITY_SPECTRUM_TRANSFORMER__

#include <vector>
#include "audacity/types.h"
#include "RealFFTf.h"

enum WindowFunctionChoice;
class TrackFactory;
class WaveTrack;

/*
 * A class that transforma a portion of a wave track (preserving duration)
 * by applying Fourier transform, then modifying coefficients, then inverse
 * Fourier transform and overlap-add to reconstruct.  Derived classes can
 * specify just the modification of coefficients, which can emply lookahead
 * and -behind to nearby windows.  May also be used just to gather information
 * without producing a transformed track.
 */
class SpectrumTransformer
{
public:
   // Public interface
   typedef std::vector<float> FloatVector;

   SpectrumTransformer
      (WindowFunctionChoice inWindowType, // Two window types are not both rectangular
       WindowFunctionChoice outWindowType,
       TrackFactory *factory, // If NULL, do not transform tracks
       int windowSize,     // must be a power of 2
       int stepsPerWindow, // determines the overlap; windowSize must be a multiple of this
       bool leadingPadding, // Whether to start the queue with windows that partially overlap
                            // the first window of input samples
       bool trailingPadding); // Whether to stop the procedure after the last complete window
                              // of input is added to the queue
   ~SpectrumTransformer();

   /*
    * This may be called more than one in the lifetime of the object:
    */
   bool ProcessTrack
      (WaveTrack *track, int queueLength, sampleCount start, sampleCount len);

protected:
   // Protected interface

   struct Window;
   // Allocates a window to place in the queue.  Only when initializing -- windows
   // are recycled thereafter.
   // You can derive from Window to add fields, and then override this factory function.
   // Executes in the main thread.
   virtual Window *NewWindow(int windowSize);

   // Derived class must supply this step that transforms coefficients.
   // May execute in a worker thread.
   // Called repeatedly, with the newest window in the queue taken from
   // input, and the last window of the queue about to be inverse-transformed
   // for output.
   // Return false to abort processing.
   virtual bool ProcessWindow() = 0; // return true for success

   // This may perform UI.  Do not do that in ProcessWindow().
   // Return false to abort.
   // Default implementation returns true.
   // Executes in the main thread.
   virtual bool TrackProgress();

   // Called after the last call to ProcessWindow().
   // If a factory was supplied to the contstructor, then the procedure is
   // about to paste into the original track.
   // Return false to abort processing.
   // Default implemantation just returns true.
   // Executes in the main thread.
   virtual bool FinishTrack();

   // You can use the following in overrides of the virtual functions:

   // How many windows in the queue have been filled?
   // (Not always the allocated size of the queue)
   int QueueSize() const;

   // Whether the last window in the queue overlapped the input
   // at least partially and its coefficients will affect output.
   bool QueueIsFull() const;

   // Access the queue, so you can inspect and modify any window in it
   // Newer windows are at earlier indices
   // You can't modify the length of it
   Window &Nth(int n) { return *mQueue[(n + mQueueStart) % mQueue.size()]; }

   Window &Newest() { return Nth(0); }
   Window &Latest() { return Nth(QueueSize() - 1); }

private:
   void ResizeQueue(int queueLength);
   void FillFirstWindow();
   bool RotateWindows();
   bool ProcessSamples(sampleCount len, const float *buffer);
   bool PostProcess();

protected:
   const bool mLeadingPadding;
   const bool mTrailingPadding;

   TrackFactory *const mFactory;

   const int mWindowSize;
   const int mSpectrumSize;

   const int mStepsPerWindow;
   const int mStepSize;

   // You can derive this class to add information to the queue.
   // See NewWindow()
   struct Window
   {
      Window(int windowSize)
         : mRealFFTs(windowSize / 2)
         , mImagFFTs(windowSize / 2)
      {
      }

      virtual ~Window();

      void Zero()
      {
         const int size = mRealFFTs.size();

         float *pFill = &mRealFFTs[0];
         std::fill(pFill, pFill + size, 0.0f);

         pFill = &mImagFFTs[0];
         std::fill(pFill, pFill + size, 0.0f);
      }

      // index zero holds the dc coefficient,
      // which has no imaginary part:
      FloatVector mRealFFTs;
      // index zero holds the nyquist frequency coefficient,
      // which is actually a real number:
      FloatVector mImagFFTs;
   };

private:
   std::vector<Window*> mQueue;
   int mQueueStart;
   int mQueueEnd;
   HFFT     hFFT;
   sampleCount mInSampleCount;
   int mOutStepCount;
   int mPartialBuffers;
   int mInWavePos;

   // These have size mWindowSize:
   FloatVector mFFTBuffer;
   FloatVector mInvFFTBuffer;
   FloatVector mInWaveBuffer;
   FloatVector mOutOverlapBuffer;
   // These have that size, or 0:
   FloatVector mInWindow;
   FloatVector mOutWindow;

   std::auto_ptr<WaveTrack> mOutputTrack;

   class InvFFTThread;
   friend class InvFFTThread;
   std::auto_ptr<InvFFTThread> mInvFFTThread;
};

#endif
