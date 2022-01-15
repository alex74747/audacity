/**********************************************************************

  Audacity: A Digital Audio Editor

  FindClipping.cpp

  Leland Lucius

*******************************************************************//**

\class EffectFindClipping
\brief Locates clipping and inserts labels when found

*//****************************************************************//**

\class FindClippingDialog
\brief FindClippingDialog used with EffectFindClipping

*//*******************************************************************/



#include "FindClipping.h"
#include "LoadEffects.h"

#include <math.h>


#include "../ShuttleGui.h"
#include "../widgets/AudacityMessageBox.h"

#include "../LabelTrack.h"
#include "../WaveTrack.h"

// Define keys, defaults, minimums, and maximums for the effect parameters
//
//     Name    Type  Key                     Def   Min   Max      Scale
static auto Start = Parameter<int>(
                           L"Duty Cycle Start", 3,    1,    INT_MAX, 1   );
static auto Stop = Parameter<int>(
                           L"Duty Cycle End",   3,    1,    INT_MAX, 1   );

const ComponentInterfaceSymbol EffectFindClipping::Symbol
{ XO("Find Clipping") };

namespace{ BuiltinEffectsModule::Registration< EffectFindClipping > reg; }

EffectFindClipping::EffectFindClipping()
   : mParameters{
      mStart, Start,
      mStop, Stop
   }
{
   Parameters().Reset();
}

EffectFindClipping::~EffectFindClipping()
{
}

// ComponentInterface implementation

ComponentInterfaceSymbol EffectFindClipping::GetSymbol()
{
   return Symbol;
}

TranslatableString EffectFindClipping::GetDescription()
{
   return XO("Creates labels where clipping is detected");
}

ManualPageID EffectFindClipping::ManualPage()
{
   return L"Find_Clipping";
}

// EffectDefinitionInterface implementation

EffectType EffectFindClipping::GetType()
{
   return EffectTypeAnalyze;
}

// Effect implementation

bool EffectFindClipping::Process()
{
   std::shared_ptr<AddedAnalysisTrack> addedTrack;
   std::optional<ModifiedAnalysisTrack> modifiedTrack;
   const wxString name{ _("Clipping") };

   auto clt = *inputTracks()->Any< const LabelTrack >().find_if(
      [&]( const Track *track ){ return track->GetName() == name; } );

   LabelTrack *lt{};
   if (!clt)
      addedTrack = (AddAnalysisTrack(name)), lt = addedTrack->get();
   else
      modifiedTrack.emplace(ModifyAnalysisTrack(clt, name)),
      lt = modifiedTrack->get();

   int count = 0;

   // JC: Only process selected tracks.
   for (auto t : inputTracks()->Selected< const WaveTrack >()) {
      double trackStart = t->GetStartTime();
      double trackEnd = t->GetEndTime();
      double t0 = mT0 < trackStart ? trackStart : mT0;
      double t1 = mT1 > trackEnd ? trackEnd : mT1;

      if (t1 > t0) {
         auto start = t->TimeToLongSamples(t0);
         auto end = t->TimeToLongSamples(t1);
         auto len = end - start;

         if (!ProcessOne(lt, count, t, start, len)) {
            return false;
         }
      }

      count++;
   }

   // No cancellation, so commit the addition of the track.
   if (addedTrack)
      addedTrack->Commit();
   if (modifiedTrack)
      modifiedTrack->Commit();
   return true;
}

bool EffectFindClipping::ProcessOne(LabelTrack * lt,
                                    int count,
                                    const WaveTrack * wt,
                                    sampleCount start,
                                    sampleCount len)
{
   size_t blockSize = (mStart * 1000);

   if (len < mStart) {
      return true;
   }

   decltype(len) startrun = 0, stoprun = 0, samps = 0;
   double startTime = -1.0;

   bool bGoodResult;
   try {

   // mStart should be positive.
   // if we are throwing bad_alloc and mStart is negative, find out why.
   if (mStart < 0 || (int)blockSize < mStart)
      // overflow
      throw std::bad_alloc{};

   bGoodResult = ForEachBlock( { wt }, start, start + len, blockSize,
   [&]( sampleCount pos, size_t block, float *const *buffers, size_t ) {
      for (size_t ii = 0 ; ii < block; ++ii) {
         auto v = buffers[0][ii];
         if (v >= MAX_AUDIO) {
            if (startrun == 0) {
               startTime = wt->LongSamplesToTime(pos);
               samps = 0;
            }
            else {
               stoprun = 0;
            }
            startrun++;
            samps++;
         }
         else {
            if (startrun >= mStart) {
               stoprun++;
               samps++;

               if (stoprun >= mStop) {
                  lt->AddLabel(SelectedRegion(startTime,
                                             wt->LongSamplesToTime(pos - mStop)),
                              wxString::Format(L"%lld of %lld", startrun.as_long_long(), (samps - mStop).as_long_long()));
                  startrun = 0;
                  stoprun = 0;
                  samps = 0;
               }
            }
            else {
               startrun = 0;
            }
         }
      }
      return true;
   }, count );

   }
   catch( const std::bad_alloc & ) {
      Effect::MessageBox( XO("Requested value exceeds memory capacity.") );
      return false;
   }

   return bGoodResult;
}

void EffectFindClipping::PopulateOrExchange(ShuttleGui & S)
{
   S.StartMultiColumn(2, wxALIGN_CENTER);
   {
      S
         .Target( mStart, NumValidatorStyle::DEFAULT, Start.min )
         .AddTextBox(XXO("&Start threshold (samples):"), {}, 10);

      S
         .Target( mStop, NumValidatorStyle::DEFAULT, Stop.min )
         .AddTextBox(XXO("St&op threshold (samples):"), {}, 10);
   }
   S.EndMultiColumn();
}

