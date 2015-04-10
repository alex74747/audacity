/**********************************************************************

  Audacity: A Digital Audio Editor

  ControlToolBar.cpp

  Dominic Mazzoni
  Shane T. Mueller
  James Crook
  Leland Lucius

*******************************************************************//**

\class ControlToolBar
\brief A ToolBar that has the main Transport buttons.

  In the GUI, this is referred to as "Transport Toolbar", as
  it corresponds to commands in the Transport menu.
  "Control Toolbar" is historic.
  This class, which is a child of Toolbar, creates the
  window containing the Transport (rewind/play/stop/record/ff)
  buttons. The window can be embedded within a
  normal project window, or within a ToolbarFrame that is
  managed by a global ToolBarStub called
  gControlToolBarStub.

  All of the controls in this window were custom-written for
  Audacity - they are not native controls on any platform -
  however, it is intended that the images could be easily
  replaced to allow "skinning" or just customization to
  match the look and feel of each platform.

*//*******************************************************************/

#include <algorithm>

#include "../Audacity.h"
#include "../Experimental.h"
#include "../FFT.h"

// For compilers that support precompilation, includes "wx/wx.h".
#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/app.h>
#include <wx/dc.h>
#include <wx/event.h>
#include <wx/image.h>
#include <wx/intl.h>
#include <wx/timer.h>
#endif
#include <wx/tooltip.h>

#include "ControlToolBar.h"
#include "SpectralSelectionBar.h"
#include "TranscriptionToolBar.h"
#include "MeterToolBar.h"

#include "../AColor.h"
#include "../AllThemeResources.h"
#include "../AudioIO.h"
#include "../ImageManipulation.h"
#include "../Prefs.h"
#include "../Project.h"
#include "../SpectrumTransformer.h"
#include "../Theme.h"
#include "../Track.h"
#include "../widgets/AButton.h"
#include "../widgets/Meter.h"

IMPLEMENT_CLASS(ControlToolBar, ToolBar);

////////////////////////////////////////////////////////////
/// Methods for ControlToolBar
////////////////////////////////////////////////////////////

BEGIN_EVENT_TABLE(ControlToolBar, ToolBar)
   EVT_CHAR(ControlToolBar::OnKeyEvent)
   EVT_BUTTON(ID_PLAY_BUTTON,   ControlToolBar::OnPlay)
   EVT_BUTTON(ID_STOP_BUTTON,   ControlToolBar::OnStop)
   EVT_BUTTON(ID_RECORD_BUTTON, ControlToolBar::OnRecord)
   EVT_BUTTON(ID_REW_BUTTON,    ControlToolBar::OnRewind)
   EVT_BUTTON(ID_FF_BUTTON,     ControlToolBar::OnFF)
   EVT_BUTTON(ID_PAUSE_BUTTON,  ControlToolBar::OnPause)
END_EVENT_TABLE()

//Standard constructor
// This was called "Control" toolbar in the GUI before - now it is "Transport".
// Note that we use the legacy "Control" string as the section because this
// gets written to prefs and cannot be changed in prefs to maintain backwards
// compatibility
ControlToolBar::ControlToolBar()
: ToolBar(TransportBarID, _("Transport"), wxT("Control"))
{
   mPaused = false;

   gPrefs->Read(wxT("/GUI/ErgonomicTransportButtons"), &mErgonomicTransportButtons, true);
   mStrLocale = gPrefs->Read(wxT("/Locale/Language"), wxT(""));

   mSizer = NULL;
   mTemporaryTracks = NULL;
}

ControlToolBar::~ControlToolBar()
{
   ClearTemporaryTracks();
}


void ControlToolBar::Create(wxWindow * parent)
{
   ToolBar::Create(parent);
}

// This is a convenience function that allows for button creation in
// MakeButtons() with fewer arguments
AButton *ControlToolBar::MakeButton(teBmps eEnabledUp, teBmps eEnabledDown, teBmps eDisabled,
                                    int id,
                                    bool processdownevents,
                                    const wxChar *label)
{
   AButton *r = ToolBar::MakeButton(
      bmpRecoloredUpLarge, bmpRecoloredDownLarge, bmpRecoloredHiliteLarge,
      eEnabledUp, eEnabledDown, eDisabled,
      wxWindowID(id),
      wxDefaultPosition, processdownevents,
      theTheme.ImageSize( bmpRecoloredUpLarge ));
   r->SetLabel(label);
   r->SetFocusRect( r->GetRect().Deflate( 12, 12 ) );

   return r;
}

// static
void ControlToolBar::MakeAlternateImages(AButton &button, int idx,
                                         teBmps eEnabledUp,
                                         teBmps eEnabledDown,
                                         teBmps eDisabled)
{
   ToolBar::MakeAlternateImages(button, idx,
      bmpRecoloredUpLarge, bmpRecoloredDownLarge, bmpRecoloredHiliteLarge,
      eEnabledUp, eEnabledDown, eDisabled,
      theTheme.ImageSize( bmpRecoloredUpLarge ));
}

void ControlToolBar::Populate()
{
   MakeButtonBackgroundsLarge();

   mPause = MakeButton(bmpPause, bmpPause, bmpPauseDisabled,
      ID_PAUSE_BUTTON,  true,  _("Pause"));

   mPlay = MakeButton( bmpPlay, bmpPlay, bmpPlayDisabled,
      ID_PLAY_BUTTON, true, _("Play"));
   MakeAlternateImages(*mPlay, 1, bmpLoop, bmpLoop, bmpLoopDisabled);
   MakeAlternateImages(*mPlay, 2,
      bmpCutPreview, bmpCutPreview, bmpCutPreviewDisabled);
   mPlay->FollowModifierKeys();

   mStop = MakeButton( bmpStop, bmpStop, bmpStopDisabled ,
      ID_STOP_BUTTON, false, _("Stop"));

   mRewind = MakeButton(bmpRewind, bmpRewind, bmpRewindDisabled,
      ID_REW_BUTTON, false, _("Skip to Start"));

   mFF = MakeButton(bmpFFwd, bmpFFwd, bmpFFwdDisabled,
      ID_FF_BUTTON, false, _("Skip to End"));

   mRecord = MakeButton(bmpRecord, bmpRecord, bmpRecordDisabled,
      ID_RECORD_BUTTON, true, _("Record"));
   MakeAlternateImages(*mRecord, 1, bmpAppendRecord, bmpAppendRecord,
      bmpAppendRecordDisabled);
   mRecord->FollowModifierKeys();

#if wxUSE_TOOLTIPS
   RegenerateToolsTooltips();
   wxToolTip::Enable(true);
   wxToolTip::SetDelay(1000);
#endif

   // Set default order and mode
   ArrangeButtons();
}

void ControlToolBar::RegenerateToolsTooltips()
{
#if wxUSE_TOOLTIPS
   for (long iWinID = ID_PLAY_BUTTON; iWinID < BUTTON_COUNT; iWinID++)
   {
      wxWindow* pCtrl = this->FindWindow(iWinID);
      wxString strToolTip = pCtrl->GetLabel();
      AudacityProject* pProj = GetActiveProject();
      CommandManager* pCmdMgr = (pProj) ? pProj->GetCommandManager() : NULL;
      if (pCmdMgr)
      {
         wxString strKey(wxT(" ("));
         switch (iWinID)
         {
            case ID_PLAY_BUTTON:
               strKey += pCmdMgr->GetKeyFromName(wxT("Play"));
               strKey += _(") / Loop Play (");
               strKey += pCmdMgr->GetKeyFromName(wxT("PlayLooped"));
               break;
            case ID_RECORD_BUTTON:
               strKey += pCmdMgr->GetKeyFromName(wxT("Record"));
               strKey += _(") / Append Record (");
               strKey += pCmdMgr->GetKeyFromName(wxT("RecordAppend"));
               break;
            case ID_PAUSE_BUTTON:
               strKey += pCmdMgr->GetKeyFromName(wxT("Pause"));
               break;
            case ID_STOP_BUTTON:
               strKey += pCmdMgr->GetKeyFromName(wxT("Stop"));
               break;
            case ID_FF_BUTTON:
               strKey += pCmdMgr->GetKeyFromName(wxT("SkipEnd"));
               break;
            case ID_REW_BUTTON:
               strKey += pCmdMgr->GetKeyFromName(wxT("SkipStart"));
               break;
         }
         strKey += wxT(")");
         strToolTip += strKey;
      }
      pCtrl->SetToolTip(strToolTip);
   }
#endif
}

void ControlToolBar::UpdatePrefs()
{
   bool updated = false;
   bool active;

   gPrefs->Read( wxT("/GUI/ErgonomicTransportButtons"), &active, true );
   if( mErgonomicTransportButtons != active )
   {
      mErgonomicTransportButtons = active;
      updated = true;
   }
   wxString strLocale = gPrefs->Read(wxT("/Locale/Language"), wxT(""));
   if (mStrLocale != strLocale)
   {
      mStrLocale = strLocale;
      updated = true;
   }

   if( updated )
   {
      ReCreateButtons(); // side effect: calls RegenerateToolsTooltips()
      Updated();
   }
   else
      // The other reason to regenerate tooltips is if keyboard shortcuts for
      // transport buttons changed, but that's too much work to check for, so just
      // always do it. (Much cheaper than calling ReCreateButtons() in all cases.
      RegenerateToolsTooltips();


   // Set label to pull in language change
   SetLabel(_("Transport"));

   // Give base class a chance
   ToolBar::UpdatePrefs();
}

void ControlToolBar::ArrangeButtons()
{
   int flags = wxALIGN_CENTER | wxRIGHT;

   // (Re)allocate the button sizer
   if( mSizer )
   {
      Detach( mSizer );
      delete mSizer;
   }
   mSizer = new wxBoxSizer( wxHORIZONTAL );
   Add( mSizer, 1, wxEXPAND );

   // Start with a little extra space
   mSizer->Add( 5, 55 );

   // Add the buttons in order based on ergonomic setting
   if( mErgonomicTransportButtons )
   {
      mPause->MoveBeforeInTabOrder( mRecord );
      mPlay->MoveBeforeInTabOrder( mRecord );
      mStop->MoveBeforeInTabOrder( mRecord );
      mRewind->MoveBeforeInTabOrder( mRecord );
      mFF->MoveBeforeInTabOrder( mRecord );

      mSizer->Add( mPause,  0, flags, 2 );
      mSizer->Add( mPlay,   0, flags, 2 );
      mSizer->Add( mStop,   0, flags, 2 );
      mSizer->Add( mRewind, 0, flags, 2 );
      mSizer->Add( mFF,     0, flags, 10 );
      mSizer->Add( mRecord, 0, flags, 5 );
   }
   else
   {
      mRewind->MoveBeforeInTabOrder( mFF );
      mPlay->MoveBeforeInTabOrder( mFF );
      mRecord->MoveBeforeInTabOrder( mFF );
      mPause->MoveBeforeInTabOrder( mFF );
      mStop->MoveBeforeInTabOrder( mFF );

      mSizer->Add( mRewind, 0, flags, 2 );
      mSizer->Add( mPlay,   0, flags, 2 );
      mSizer->Add( mRecord, 0, flags, 2 );
      mSizer->Add( mPause,  0, flags, 2 );
      mSizer->Add( mStop,   0, flags, 2 );
      mSizer->Add( mFF,     0, flags, 5 );
   }

   // Layout the sizer
   mSizer->Layout();

   // Layout the toolbar
   Layout();

   // (Re)Establish the minimum size
   SetMinSize( GetSizer()->GetMinSize() );
}

void ControlToolBar::ReCreateButtons()
{
   bool playDown = false;
   bool playShift = false;
   bool pauseDown = false;
   bool recordDown = false;
   bool recordShift = false;

   // ToolBar::ReCreateButtons() will get rid of its sizer and
   // since we've attached our sizer to it, ours will get deleted too
   // so clean ours up first.
   if( mSizer )
   {
      playDown = mPlay->IsDown();
      playShift = mPlay->WasShiftDown();
      pauseDown = mPause->IsDown();
      recordDown = mRecord->IsDown();
      recordShift = mRecord->WasShiftDown();
      Detach( mSizer );

      delete mSizer;
      mSizer = NULL;
   }

   ToolBar::ReCreateButtons();

   if (playDown)
   {
      SetPlay(playDown, playShift, false);
   }

   if (pauseDown)
   {
      mPause->PushDown();
   }

   if (recordDown)
   {
      SetRecord(recordDown, recordShift);
   }

   EnableDisableButtons();

   RegenerateToolsTooltips();
}

void ControlToolBar::Repaint( wxDC *dc )
{
#ifndef USE_AQUA_THEME
   wxSize s = mSizer->GetSize();
   wxPoint p = mSizer->GetPosition();

   wxRect bevelRect( p.x, p.y, s.GetWidth() - 1, s.GetHeight() - 1 );
   AColor::Bevel( *dc, true, bevelRect );
#endif
}

void ControlToolBar::EnableDisableButtons()
{
   //TIDY-ME: Button logic could be neater.
   AudacityProject *const p = GetActiveProject();
   bool tracks = false;
   const bool playing = mPlay->IsDown();
   const bool recording = mRecord->IsDown();
   const bool busy = gAudioIO->IsBusy() || playing || recording;
   TranscriptionToolBar *const pttb = p ? p->GetTranscriptionToolBar() : 0;
   const bool playingAtSpeed = pttb && pttb->PlayIsDown();
   SpectralSelectionBar *const pssb = p ? p->GetSpectralSelectionBar() : 0;
   const bool playingFrequencies = pssb && pssb->PlayIsDown();

   // Only interested in audio type tracks
   if (p) {
      TrackListIterator iter( p->GetTracks() );
      for (Track *t = iter.First(); t; t = iter.Next()) {
         if (t->GetKind() == Track::Wave
#if defined(USE_MIDI)
         || t->GetKind() == Track::Note
#endif
         ) {
            tracks = true;
            break;
         }
      }
   }

   const bool enablePlay = (!recording) || (tracks && !busy);

   mPlay->SetEnabled(enablePlay);
   if (pttb)
      pttb->SetEnabled(enablePlay);
   if (pssb)
      pssb->SetEnabled(enablePlay);

   mRecord->SetEnabled(!busy && !(playingAtSpeed || playingFrequencies || playing));

   mStop->SetEnabled(busy);
   mRewind->SetEnabled(!busy);
   mFF->SetEnabled(tracks && !busy);
   mPause->SetEnabled(true);
}

void ControlToolBar::SetPlay(bool down, bool looped, bool cutPreview)
{
   // Slightly hairy interaction with transcription and spectral
   // selection tool bars.  TIDY-ME
   // When down is true, push one and only one of the three play
   // buttons.  Else, pop all.

   AudacityProject *const p = GetActiveProject();
   TranscriptionToolBar *const pttb = p ? p->GetTranscriptionToolBar() : 0;
   const bool playingAtSpeed = pttb && pttb->PlayIsDown();
   SpectralSelectionBar *const pssb = p ? p->GetSpectralSelectionBar() : 0;
   const bool playingFrequencies = pssb && pssb->PlayIsDown();

   const bool thisUp = (!down || playingAtSpeed || playingFrequencies);
   if (thisUp) {
      mPlay->PopUp();
      mPlay->SetAlternateIdx(0);
   }
   else {
      mPlay->SetShift(looped);
      mPlay->SetControl(cutPreview);
      mPlay->SetAlternateIdx(cutPreview ? 2 : looped ? 1 : 0);
      mPlay->PushDown();
   }

   const bool transcriptionUp = !(down && playingAtSpeed);
   if (pttb)
      pttb->SetPlaying(!transcriptionUp, looped, cutPreview);

   const bool spectralUp = !(down && playingFrequencies);
   if (pssb)
      pssb->SetPlaying(!spectralUp, looped);

   EnableDisableButtons();
}

void ControlToolBar::SetStop(bool down)
{
   if (down)
      mStop->PushDown();
   else {
      if(FindFocus() == mStop)
         mPlay->SetFocus();
      mStop->PopUp();
   }
   EnableDisableButtons();
}

void ControlToolBar::SetRecord(bool down, bool append)
{
   if (down)
   {
      mRecord->SetAlternateIdx(append ? 1 : 0);
      mRecord->PushDown();
   }
   else
   {
      mRecord->SetAlternateIdx(0);
      mRecord->PopUp();
   }
   EnableDisableButtons();
}

bool ControlToolBar::IsRecordDown()
{
   return mRecord->IsDown();
}


namespace
{

class TrackMaker {
public:
   virtual WaveTrack *makeTrack(Track *orig) = 0;
   virtual void SetNTracks(int) {}
};

class CutPreviewTrackMaker : public TrackMaker {
public:
   CutPreviewTrackMaker(double cutStart, double cutEnd)
      : mCutStart(cutStart), mCutEnd(cutEnd)
   {}

   WaveTrack *makeTrack(Track *orig) {
      WaveTrack *track1 = static_cast<WaveTrack*>(orig->Duplicate());
      track1->Clear(mCutStart, mCutEnd);
      return track1;
   }

   double mCutStart, mCutEnd;
};

enum { WindowSize = 2048, StepsPerWindow = 4 };

class SpectralPlayTrackMaker : public TrackMaker {
public:
   SpectralPlayTrackMaker(TrackFactory *factory,
      double selStart, double selEnd,
      double f0, double f1)
      : mProgress(_("Play spectral selection"), _("Preparing Output"), pdlgHideStopButton)
      , mFactory(factory)
      , mSelStart(selStart)
      , mSelEnd(selEnd)
      , mF0(f0)
      , mF1(f1)
      , mCount(-1)
      , mNTracks(1)
      , mNWindows(0)
      , mLen(0)
   {}

   WaveTrack *makeTrack(Track *orig);
   void SetNTracks(int nTracks)
   {
      mNTracks = std::max(1, nTracks);
   }

   void NextTrack(sampleCount len)
   {
      ++mCount;
      mNWindows = 0;
      mLen = len;
   }

   bool TrackProgress()
   { 
      return eProgressCancelled != mProgress.Update(
         (mCount +
          (++mNWindows * (WindowSize / StepsPerWindow)) / double(mLen))
         / double(mNTracks)
      );
   }

   ProgressDialog mProgress;
   TrackFactory *mFactory;
   double mSelStart, mSelEnd, mF0, mF1;
   int mCount, mNTracks, mNWindows, mLen;
};

class BandPasser : public SpectrumTransformer
{
public:
   BandPasser(SpectralPlayTrackMaker &maker,
              TrackFactory *factory, double rate, sampleCount len,
              double freqLo, double freqHi)
      : SpectrumTransformer(WFCHann, WFCHann, factory,
                            WindowSize, StepsPerWindow, true, true)
      , mMaker(maker)
   {
      double bin = rate / WindowSize;
      mBinLo = freqLo < 0 ? 0 : floor(freqLo / bin);
      mBinHi = freqHi < 0 ? (1 + WindowSize / 2) : ceil(freqHi / bin);
      mMaker.NextTrack(len);
   }

   virtual bool ProcessWindow()
   {
      int spectrumSize = (1 + WindowSize / 2);
      Window &window = Latest();
      if (0 < mBinLo)
         window.mRealFFTs[0] = 0;
      for (int ii = 1; ii < mBinLo; ++ii)
         window.mRealFFTs[ii] = window.mImagFFTs[ii] = 0;
      for (int ii = mBinHi; ii < spectrumSize - 1; ++ii)
         window.mRealFFTs[ii] = window.mImagFFTs[ii] = 0;
      if (mBinHi < spectrumSize)
         window.mImagFFTs[0] = 0;
      return true;
   }

   virtual bool TrackProgress()
   {
      return mMaker.TrackProgress();
   }

   SpectralPlayTrackMaker &mMaker;
   int mBinLo, mBinHi;
};

WaveTrack *SpectralPlayTrackMaker::makeTrack(Track *orig)
{
   std::auto_ptr<WaveTrack> track1(static_cast<WaveTrack*>(orig->Duplicate()));
   const sampleCount start = track1->TimeToLongSamples(mSelStart);
   const sampleCount end = track1->TimeToLongSamples(mSelEnd);
   BandPasser bp(*this, mFactory, track1->GetRate(), end - start, mF0, mF1);
   if (!bp.ProcessTrack(track1.get(), 1, start, end - start))
      track1.reset();
   return track1.release();
}

bool SetupTemporaryTracks(TrackList *trackList, TrackMaker &maker)
{
   int nTracks = 0;
   AudacityProject *p = GetActiveProject();
   if (p) {
      TrackListIterator it(p->GetTracks());
      for (Track *t = it.First(); t; t = it.Next())
      {
         if (t->GetKind() == Track::Wave)
            ++nTracks;
      }
   }
   maker.SetNTracks(nTracks);
   if (p) {
      TrackListIterator it(p->GetTracks());
      for (Track *t = it.First(); t; t = it.Next())
      {
         if (t->GetKind() == Track::Wave)
         {
            // Duplicate and change tracks
            WaveTrack *track1 = maker.makeTrack(t);
            if (track1) {
               track1->SetController(t);
               trackList->Add(track1);
            }
            else
               return false;
         }
      }
   }
   return true;
}

}

bool ControlToolBar::DoPlayPlayRegion(const SelectedRegion &region,
   bool looped,
   bool cutpreview,
   TimeTrack *timetrack,
   const double *pStartTime)
{
   ClearTemporaryTracks();

   // We may change these later
   double t0 = region.t0();
   double t1 = region.t1();

   if (gAudioIO->IsBusy())
      return false;

   if (cutpreview && t0==t1)
      return false; /* msmeyer: makes no sense */

   AudacityProject *p = GetActiveProject();
   if (!p)
      return false;  // Should never happen, but...

   TrackList *trackList = p->GetTracks();
   if (!trackList)
      return false;  // Should never happen, but...

   bool hasaudio = false;
   TrackListIterator iter(trackList);
   for (Track *trk = iter.First(); trk; trk = iter.Next()) {
      if (trk->GetKind() == Track::Wave
#ifdef EXPERIMENTAL_MIDI_OUT
         || trk->GetKind() == Track::Note
#endif
         ) {
         hasaudio = true;
         break;
      }
   }

   if (!hasaudio)
      return false;  // No need to continue without audio tracks

   double maxofmins,minofmaxs;
#if defined(EXPERIMENTAL_SEEK_BEHIND_CURSOR)
   double init_seek = 0.0;
#endif

   // JS: clarified how the final play region is computed;
   if (t1 == t0) {
      // msmeyer: When playing looped, we play the whole file, if
      // no range is selected. Otherwise, we play from t0 to end
      if (looped) {
         // msmeyer: always play from start
         t0 = trackList->GetStartTime();
      } else {
         // move t0 to valid range
         if (t0 < 0) {
            t0 = trackList->GetStartTime();
         }
         else if (t0 > trackList->GetEndTime()) {
            t0 = trackList->GetEndTime();
         }
#if defined(EXPERIMENTAL_SEEK_BEHIND_CURSOR)
         else {
            init_seek = t0;         //AC: init_seek is where playback will 'start'
            t0 = t->GetStartTime();
         }
#endif
      }

      // always play to end
      t1 = trackList->GetEndTime();
   }
   else {
      // always t0 < t1 right?

      // the set intersection between the play region and the
      // valid range maximum of lower bounds
      if (t0 < trackList->GetStartTime())
         maxofmins = trackList->GetStartTime();
      else
         maxofmins = t0;

      // minimum of upper bounds
      if (t1 > trackList->GetEndTime())
         minofmaxs = trackList->GetEndTime();
      else
         minofmaxs = t1;

      // we test if the intersection has no volume
      if (minofmaxs <= maxofmins)
         // no volume; play nothing
         return false;
      else {
         t0 = maxofmins;
         t1 = minofmaxs;
      }
   }

   // Can't play before 0...either shifted or latencey corrected tracks
   if (t0 < 0.0) {
      t0 = 0.0;
   }

   bool success = false;
   if (t1 > t0) {
      int token;
      if (cutpreview) {
         double beforeLen, afterLen;
         gPrefs->Read(wxT("/AudioIO/CutPreviewBeforeLen"), &beforeLen, 2.0);
         gPrefs->Read(wxT("/AudioIO/CutPreviewAfterLen"), &afterLen, 1.0);
         double tcp0 = t0-beforeLen;
         double tcp1 = (t1+afterLen) - (t1-t0);
         mTemporaryTracks = new TrackList();
         CutPreviewTrackMaker maker(t0, t1);
         if (mTemporaryTracks)
         {
            token = gAudioIO->StartStream(
               mTemporaryTracks->GetWaveTrackArray(false),
               WaveTrackArray(),
#ifdef EXPERIMENTAL_MIDI_OUT
               NoteTrackArray(),
#endif
               timetrack, p->GetRate(), tcp0, tcp1, p, false,
               t0, t1-t0,
               pStartTime);
         }
         else
            // Cannot create cut preview tracks, clean up and exit
            return false;
      }
      else {
         if (!timetrack)
            timetrack = trackList->GetTimeTrack();
         if (!(region.f0() < 0.0 && region.f1() < 0.0)) {
            // Prepare to play the spectral selection only
            mTemporaryTracks = new TrackList();
            SpectralPlayTrackMaker maker
               (p->GetTrackFactory(), t0, t1, region.f0(), region.f1());
            if (!SetupTemporaryTracks(mTemporaryTracks, maker)) {
               ClearTemporaryTracks();
               return false;
            }
            trackList = mTemporaryTracks;
         }
         token = gAudioIO->StartStream(trackList->GetWaveTrackArray(false),
                                       WaveTrackArray(),
#ifdef EXPERIMENTAL_MIDI_OUT
                                       t->GetNoteTrackArray(false),
#endif
                                       timetrack,
                                       p->GetRate(), t0, t1, p, looped,
                                       0, 0,
                                       pStartTime);
      }
      if (token != 0) {
         success = true;
         p->SetAudioIOToken(token);
#if defined(EXPERIMENTAL_SEEK_BEHIND_CURSOR)
         //AC: If init_seek was set, now's the time to make it happen.
         // PRL:  isn't SeekStream supposed to take a difference of times,
         // not an absolute time?
         gAudioIO->SeekStream(init_seek);
#endif
      }
      else {
         // msmeyer: Show error message if stream could not be opened
         wxMessageBox(
#if wxCHECK_VERSION(3,0,0)
            _("Error while opening sound device. "
            "Please check the playback device settings and the project sample rate."),
#else
            _("Error while opening sound device. "
            wxT("Please check the playback device settings and the project sample rate.")),
#endif
            _("Error"), wxOK | wxICON_EXCLAMATION, this);
      }
   }

   return success;
}

void ControlToolBar::PlayPlayRegion(const SelectedRegion &region,
   bool looped /* = false */,
   bool cutpreview /* = false */,
   TimeTrack *timetrack /* = NULL */,
   const double *pStartTime /* = NULL */)
{
   SetPlay(true, looped, cutpreview);
   if (!DoPlayPlayRegion(region, looped, cutpreview, timetrack, pStartTime)) {
      SetPlay(false);
      // Do we really need the following?
      SetStop(false);
      SetRecord(false);
   }
}

void ControlToolBar::PlayCurrentRegion(bool looped /* = false */,
                                       bool cutpreview /* = false */)
{
   AudacityProject *p = GetActiveProject();

   if (p)
   {
      if (looped)
         p->mLastPlayMode = loopedPlay;
      else
         p->mLastPlayMode = normalPlay;

      double playRegionStart, playRegionEnd;
      p->GetPlayRegion(&playRegionStart, &playRegionEnd);

      PlayPlayRegion(SelectedRegion(playRegionStart, playRegionEnd),
                     looped, cutpreview);
   }
}

void ControlToolBar::OnKeyEvent(wxKeyEvent & event)
{
   if (event.ControlDown() || event.AltDown()) {
      event.Skip();
      return;
   }

   if (event.GetKeyCode() == WXK_SPACE) {
      if (gAudioIO->IsStreamActive(GetActiveProject()->GetAudioIOToken())) {
         SetPlay(false);
         SetStop(true);
         StopPlaying();
      }
      else if (!gAudioIO->IsBusy()) {
         //SetPlay(true);// Not needed as done in PlayPlayRegion
         SetStop(false);
         PlayCurrentRegion();
      }
      return;
   }
   event.Skip();
}

void ControlToolBar::OnPlay(wxCommandEvent & WXUNUSED(evt))
{
   StopPlaying();

   AudacityProject *p = GetActiveProject();
   if (p) p->TP_DisplaySelection();

   PlayDefault();
}

void ControlToolBar::OnStop(wxCommandEvent & WXUNUSED(evt))
{
   StopPlaying();
}

void ControlToolBar::PlayDefault()
{
   // Let control have precedence over shift
   const bool cutPreview = mPlay->WasControlDown();
   const bool looped = !cutPreview &&
      mPlay->WasShiftDown();
   PlayCurrentRegion(looped, cutPreview);
}

void ControlToolBar::StopPlaying(bool stopStream /* = true*/)
{
   mStop->PushDown();

   SetStop(false);
   if(stopStream)
      gAudioIO->StopStream();
   SetPlay(false);
   SetRecord(false);

   #ifdef AUTOMATED_INPUT_LEVEL_ADJUSTMENT
      gAudioIO->AILADisable();
   #endif

   mPause->PopUp();
   mPaused=false;
   //Make sure you tell gAudioIO to unpause
   gAudioIO->SetPaused(mPaused);

   ClearTemporaryTracks();

   // So that we continue monitoring after playing or recording.
   // also clean the MeterQueues
   AudacityProject *project = GetActiveProject();
   if( project ) {
      project->MayStartMonitoring();

      Meter *meter = project->GetPlaybackMeter();
      if( meter ) {
         meter->Clear();
      }
      
      meter = project->GetCaptureMeter();
      if( meter ) {
         meter->Clear();
      }
   }
}

void ControlToolBar::OnRecord(wxCommandEvent &evt)
{
   if (gAudioIO->IsBusy()) {
      mRecord->PopUp();
      return;
   }
   AudacityProject *p = GetActiveProject();

   if( evt.GetInt() == 1 ) // used when called by keyboard shortcut. Default (0) ignored.
      mRecord->SetShift(true);
   if( evt.GetInt() == 2 )
      mRecord->SetShift(false);

   SetRecord(true, mRecord->WasShiftDown());

   if (p) {
      TrackList *t = p->GetTracks();
      TrackListIterator it(t);
      if(it.First() == NULL)
         mRecord->SetShift(false);
      double t0 = p->GetSel0();
      double t1 = p->GetSel1();
      if (t1 == t0)
         t1 = 1000000000.0;     // record for a long, long time (tens of years)

      /* TODO: set up stereo tracks if that is how the user has set up
       * their preferences, and choose sample format based on prefs */
      WaveTrackArray newRecordingTracks, playbackTracks;
#ifdef EXPERIMENTAL_MIDI_OUT
      NoteTrackArray midiTracks;
#endif
      bool duplex;
      gPrefs->Read(wxT("/AudioIO/Duplex"), &duplex, true);

      if(duplex){
         playbackTracks = t->GetWaveTrackArray(false);
#ifdef EXPERIMENTAL_MIDI_OUT
         midiTracks = t->GetNoteTrackArray(false);
#endif
     }
      else {
         playbackTracks = WaveTrackArray();
#ifdef EXPERIMENTAL_MIDI_OUT
         midiTracks = NoteTrackArray();
#endif
     }

      // If SHIFT key was down, the user wants append to tracks
      int recordingChannels = 0;
      bool shifted = mRecord->WasShiftDown();
      if (shifted) {
         bool sel = false;
         double allt0 = t0;

         // Find the maximum end time of selected and all wave tracks
         // Find whether any tracks were selected.  (If any are selected,
         // record only into them; else if tracks exist, record into all.)
         for (Track *tt = it.First(); tt; tt = it.Next()) {
            if (tt->GetKind() == Track::Wave) {
               WaveTrack *wt = static_cast<WaveTrack *>(tt);
               if (wt->GetEndTime() > allt0) {
                  allt0 = wt->GetEndTime();
               }

               if (tt->GetSelected()) {
                  sel = true;
                  if (wt->GetEndTime() > t0) {
                     t0 = wt->GetEndTime();
                  }
               }
            }
         }

         // Use end time of all wave tracks if none selected
         if (!sel) {
            t0 = allt0;
         }

         // Pad selected/all wave tracks to make them all the same length
         // Remove recording tracks from the list of tracks for duplex ("overdub")
         // playback.
         for (Track *tt = it.First(); tt; tt = it.Next()) {
            if (tt->GetKind() == Track::Wave && (tt->GetSelected() || !sel)) {
               WaveTrack *wt = static_cast<WaveTrack *>(tt);
               if (duplex)
                  playbackTracks.Remove(wt);
               t1 = wt->GetEndTime();
               if (t1 < t0) {
                  WaveTrack *newTrack = p->GetTrackFactory()->NewWaveTrack();
                  newTrack->InsertSilence(0.0, t0 - t1);
                  newTrack->Flush();
                  wt->Clear(t1, t0);
                  bool bResult = wt->Paste(t1, newTrack);
                  wxASSERT(bResult); // TO DO: Actually handle this.
                  delete newTrack;
               }
               newRecordingTracks.Add(wt);
            }
         }

         t1 = 1000000000.0;     // record for a long, long time (tens of years)
      }
      else {
         recordingChannels = gPrefs->Read(wxT("/AudioIO/RecordChannels"), 2);
         for (int c = 0; c < recordingChannels; c++) {
            WaveTrack *newTrack = p->GetTrackFactory()->NewWaveTrack();

            newTrack->SetOffset(t0);

            if (recordingChannels > 2)
              newTrack->SetMinimized(true);

            if (recordingChannels == 2) {
               if (c == 0) {
                  newTrack->SetChannel(Track::LeftChannel);
                  newTrack->SetLinked(true);
               }
               else {
                  newTrack->SetChannel(Track::RightChannel);
               }
            }
            else {
               newTrack->SetChannel( Track::MonoChannel );
            }

            newRecordingTracks.Add(newTrack);
         }

         // msmeyer: StartStream calls a callback which triggers auto-save, so
         // we add the tracks where recording is done into now. We remove them
         // later if starting the stream fails
         for (unsigned int i = 0; i < newRecordingTracks.GetCount(); i++)
            t->Add(newRecordingTracks[i]);
      }

      //Automated Input Level Adjustment Initialization
      #ifdef AUTOMATED_INPUT_LEVEL_ADJUSTMENT
         gAudioIO->AILAInitialize();
      #endif

      int token = gAudioIO->StartStream(playbackTracks,
                                        newRecordingTracks,
#ifdef EXPERIMENTAL_MIDI_OUT
                                        midiTracks,
#endif
                                        t->GetTimeTrack(),
                                        p->GetRate(), t0, t1, p);

      bool success = (token != 0);

      if (success) {
         p->SetAudioIOToken(token);
      }
      else {
         // msmeyer: Delete recently added tracks if opening stream fails
         if (!shifted) {
            for (unsigned int i = 0; i < newRecordingTracks.GetCount(); i++) {
               t->Remove(newRecordingTracks[i]);
               delete newRecordingTracks[i];
            }
         }

         // msmeyer: Show error message if stream could not be opened
         wxMessageBox(_("Error while opening sound device. Please check the recording device settings and the project sample rate."),
                      _("Error"), wxOK | wxICON_EXCLAMATION, this);

         SetPlay(false);
         SetStop(false);
         SetRecord(false);
      }
   }
}


void ControlToolBar::OnPause(wxCommandEvent & WXUNUSED(evt))
{
   if(mPaused)
   {
      mPause->PopUp();
      mPaused=false;
   }
   else
   {
      mPause->PushDown();
      mPaused=true;
   }

   gAudioIO->SetPaused(mPaused);
}

void ControlToolBar::OnRewind(wxCommandEvent & WXUNUSED(evt))
{
   mRewind->PushDown();
   mRewind->PopUp();

   AudacityProject *p = GetActiveProject();
   if (p) {
      p->Rewind(mRewind->WasShiftDown());
   }
}

void ControlToolBar::OnFF(wxCommandEvent & WXUNUSED(evt))
{
   mFF->PushDown();
   mFF->PopUp();

   AudacityProject *p = GetActiveProject();

   if (p) {
      p->SkipEnd(mFF->WasShiftDown());
   }
}

void ControlToolBar::ClearTemporaryTracks()
{
   if (mTemporaryTracks)
   {
      mTemporaryTracks->Clear(true); /* delete track contents too */
      delete mTemporaryTracks;
      mTemporaryTracks = NULL;
   }
}

