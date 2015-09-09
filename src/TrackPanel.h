/**********************************************************************

  Audacity: A Digital Audio Editor

  TrackPanel.h

  Dominic Mazzoni

**********************************************************************/

#ifndef __AUDACITY_TRACK_PANEL__
#define __AUDACITY_TRACK_PANEL__

#include <memory>
#include <vector>

#include <wx/dcmemory.h>
#include <wx/panel.h>
#include <wx/timer.h>
#include <wx/window.h>

#include "Experimental.h"
#include "audacity/Types.h"
#include "UndoManager.h" //JKC: Included for PUSH_XXX definitions.
#include "widgets/NumericTextCtrl.h"

class wxMenu;
class wxRect;

class LabelTrack;
class SpectrumAnalyst;
class TrackPanel;
class TrackPanelCell;
class TrackPanelOverlay;
class TrackArtist;
class Ruler;
class SnapManager;
class AdornedRulerPanel;
class LWSlider;
class ControlToolBar; //Needed because state of controls can affect what gets drawn.
class ToolsToolBar; //Needed because state of controls can affect what gets drawn.
class AudacityProject;

class TrackPanelAx;

class ViewInfo;

class WaveTrack;
class WaveClip;
class Envelope;
class UIHandle;

// Declared elsewhere, to reduce compilation dependencies
class TrackPanelListener;

// JKC Nov 2011: Disabled warning C4251 which is to do with DLL linkage
// and only a worry when there are DLLs using the structures.
// Array classes are private in TrackInfo, so we will not
// access them directly from the DLL.
// TrackClipArray in TrackPanel needs to be handled with care in the derived
// class, but the C4251 warning is no worry in core Audacity.
// wxWidgets doesn't cater to the exact details we need in
// WX_DECLARE_EXPORTED_OBJARRAY to be able to use that for these two arrays.
#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable: 4251 )
#endif

DECLARE_EXPORTED_EVENT_TYPE(AUDACITY_DLL_API, EVT_TRACK_PANEL_TIMER, -1);

enum {
   kTimerInterval = 50, // milliseconds
   kOneSecondCountdown = 1000 / kTimerInterval,
};

class AUDACITY_DLL_API TrackInfo
{
public:
   TrackInfo(TrackPanel * pParentIn);
   ~TrackInfo();

public:
   int GetTrackInfoWidth() const;
   void SetTrackInfoFont(wxDC *dc) const;

   void DrawBackground(wxDC * dc, const wxRect & rect, bool bSelected, bool bHasMuteSolo, const int labelw, const int vrul) const;
   void DrawBordersWithin(wxDC * dc, const wxRect & rect, bool bHasMuteSolo ) const;
   void DrawCloseBox(wxDC * dc, const wxRect & rect, bool down) const;
   void DrawTitleBar(wxDC * dc, const wxRect & rect, Track * t, bool down) const;
   void DrawMuteSolo(wxDC * dc, const wxRect & rect, Track * t, bool down, bool solo, bool bHasSoloButton) const;
   void DrawVRuler(wxDC * dc, const wxRect & rect, Track * t) const;
#ifdef EXPERIMENTAL_MIDI_OUT
   void DrawVelocitySlider(wxDC * dc, NoteTrack *t, wxRect rect) const ;
#endif
   void DrawSliders(wxDC * dc, WaveTrack *t, wxRect rect) const;

   // Draw the minimize button *and* the sync-lock track icon, if necessary.
   void DrawMinimize(wxDC * dc, const wxRect & rect, Track * t, bool down) const;

   static void GetTrackControlsRect(const wxRect & rect, wxRect &dest);
   static void GetCloseBoxRect(const wxRect & rect, wxRect &dest);
   static void GetTitleBarRect(const wxRect & rect, wxRect &dest);
   static void GetMuteSoloRect(const wxRect & rect, wxRect &dest, bool solo, bool bHasSoloButton);
   static void GetGainRect(const wxRect & rect, wxRect &dest);
   static void GetPanRect(const wxRect & rect, wxRect &dest);
   static void GetMinimizeRect(const wxRect & rect, wxRect &dest);
   static void GetSyncLockIconRect(const wxRect & rect, wxRect &dest);

   // TrackSelFunc returns true if the click is not
   // set up to be handled, but click is on the sync-lock icon or the blank area to
   // the left of the minimize button, and we want to pass it forward to be a track select.
   static bool TrackSelFunc(Track * t, wxRect rect, int x, int y);

public:
   LWSlider * GainSlider(WaveTrack *t) const;
   LWSlider * PanSlider(WaveTrack *t) const;

private:
   TrackPanel * pParent;
   wxFont mFont;
   LWSlider *mGain;
   LWSlider *mPan;

   friend class TrackPanel;
};


const int DragThreshold = 3;// Anything over 3 pixels is a drag, else a click.


class AUDACITY_DLL_API TrackPanel:public wxPanel {
 public:

   TrackPanel(wxWindow * parent,
              wxWindowID id,
              const wxPoint & pos,
              const wxSize & size,
              TrackList * tracks,
              ViewInfo * viewInfo,
              TrackPanelListener * listener,
              AdornedRulerPanel * ruler );

   virtual ~ TrackPanel();

   virtual void UpdatePrefs();

   virtual void OnSize(wxSizeEvent & event);
   virtual void OnPaint(wxPaintEvent & event);
   virtual void OnMouseEvent(wxMouseEvent & event);
   virtual void OnCaptureLost(wxMouseCaptureLostEvent & event);
   virtual void OnCaptureKey(wxCommandEvent & event);
   virtual void OnKeyDown(wxKeyEvent & event);
   virtual void OnChar(wxKeyEvent & event);
   virtual void OnKeyUp(wxKeyEvent & event);

   virtual void OnSetFocus(wxFocusEvent & event);
   virtual void OnKillFocus(wxFocusEvent & event);

   virtual void OnContextMenu(wxContextMenuEvent & event);

   virtual void OnTrackListResized(wxCommandEvent & event);
   virtual void OnTrackListUpdated(wxCommandEvent & event);
   virtual void UpdateViewIfNoTracks(); // Call this to update mViewInfo, etc, after track(s) removal, before Refresh().

   virtual double GetMostRecentXPos();

   virtual void OnTimer(wxTimerEvent& event);

   virtual int GetLeftOffset() const { return GetLabelWidth() + 1;}

   // Width and height, relative to upper left corner at (GetLeftOffset(), 0)
   // Either argument may be NULL
   virtual void GetTracksUsableArea(int *width, int *height) const;

   virtual void SelectNone();

   virtual void Refresh(bool eraseBackground = true,
                        const wxRect *rect = (const wxRect *) NULL);
   virtual void RefreshTrack(Track *trk, bool refreshbacking = true);

   virtual void DisplaySelection();

   // These two are neither used nor defined as of Nov-2011
   //virtual void SetSelectionFormat(int iformat)
   //virtual void SetSnapTo(int snapto)

   virtual void CancelDragging();
   virtual void HandleEscapeKey(bool down);
   virtual void HandleAltKey(bool down);
   virtual void HandleShiftKey(bool down);
   virtual void HandleControlKey(bool down);
   virtual void HandlePageUpKey();
   virtual void HandlePageDownKey();
   virtual AudacityProject * GetProject() const;

   virtual void OnPrevTrack(bool shift = false);
   virtual void OnNextTrack(bool shift = false);
   virtual void OnFirstTrack();
   virtual void OnLastTrack();
   virtual void OnToggle();

   virtual void ScrollIntoView(double pos);
   virtual void ScrollIntoView(int x);

   virtual void OnTrackMenu(Track *t = NULL);
   virtual Track * GetFirstSelectedTrack();
   virtual bool IsMouseCaptured();

   virtual void EnsureVisible(Track * t);

   virtual Track *GetFocusedTrack();
   virtual void SetFocusedTrack(Track *t);

   virtual void HandleCursorForLastMouseEvent();

   virtual void UpdateVRulers();
   virtual void UpdateVRuler(Track *t);
   virtual void UpdateTrackVRuler(Track *t);
   virtual void UpdateVRulerSize();

   // Returns the time corresponding to the pixel column one past the track area
   // (ignoring any fisheye)
   virtual double GetScreenEndTime() const;

 protected:
   virtual bool IsAudioActive();
   virtual void HandleTrackSpecificMouseEvent(wxMouseEvent & event);

public:
   virtual void UpdateAccessibility();

public:
   virtual void SelectTrackLength(Track *t);

protected:
   virtual void HandleCursor(wxMouseEvent & event);

   void FindCell(const wxMouseEvent &event, wxRect &inner, TrackPanelCell *& pCell, Track *& pTrack);

   // MM: Handle mouse wheel rotation
   virtual void HandleWheelRotation(wxMouseEvent & event);

public:
   virtual void MakeParentRedrawScrollbars();

protected:
   virtual void MakeParentModifyState(bool bWantsAutoSave);    // if true, writes auto-save file. Should set only if you really want the state change restored after
                                                               // a crash, as it can take many seconds for large (eg. 10 track-hours) projects
protected:
   // Find track info by coordinate
   virtual Track *FindTrack(int mouseX, int mouseY, bool label, bool link,
                     wxRect * trackRect = NULL);

   virtual wxRect FindTrackRect(Track * target, bool label);

   virtual int GetVRulerWidth() const;
   virtual int GetVRulerOffset() const { return mTrackInfo.GetTrackInfoWidth(); }

   virtual int GetLabelWidth() const { return mTrackInfo.GetTrackInfoWidth() + GetVRulerWidth(); }

// JKC Nov-2011: These four functions only used from within a dll such as mod-track-panel
// They work around some messy problems with constructors.
public:
   TrackList * GetTracks(){ return mTracks;}
   ViewInfo * GetViewInfo(){ return mViewInfo;}
   TrackPanelListener * GetListener(){ return mListener;}
   AdornedRulerPanel * GetRuler(){ return mRuler;}
// JKC and here is a factory function which just does 'new' in standard Audacity.
   static TrackPanel *(*FactoryFunction)(wxWindow * parent,
              wxWindowID id,
              const wxPoint & pos,
              const wxSize & size,
              TrackList * tracks,
              ViewInfo * viewInfo,
              TrackPanelListener * listener,
              AdornedRulerPanel * ruler);

protected:
   virtual void DrawTracks(wxDC * dc);

   virtual void DrawEverythingElse(wxDC *dc, const wxRegion & region,
                           const wxRect & clip);
   virtual void DrawOutside(Track *t, wxDC *dc, const wxRect & rec,
                    const wxRect &trackRect);

   virtual void HighlightFocusedTrack (wxDC* dc, const wxRect &rect);
   virtual void DrawShadow            (Track *t, wxDC* dc, const wxRect & rect);
   virtual void DrawBordersAroundTrack(Track *t, wxDC* dc, const wxRect & rect, const int labelw, const int vrul);
   virtual void DrawOutsideOfTrack    (Track *t, wxDC* dc, const wxRect & rect);

public:
   // Register and unregister overlay objects.
   // The sequence in which they were registered is the sequence in
   // which they are painted.
   // TrackPanel is not responsible for their memory management.
   virtual void AddOverlay(TrackPanelOverlay *pOverlay);
   // Returns true if the overlay was found
   virtual bool RemoveOverlay(TrackPanelOverlay *pOverlay);
   virtual void ClearOverlays();

   // Set the object that performs catch-all event handling when the pointer
   // is not in any track or ruler or control panel.
   // TrackPanel does NOT assume memory management responsibility
   virtual void SetBackgroundCell(TrackPanelCell *pCell);

   // Erase and redraw things like the cursor, cheaply and directly to the
   // client area, without full refresh.
   virtual void DrawOverlays(bool repaint);

#ifdef EXPERIMENTAL_OUTPUT_DISPLAY
   void UpdateVirtualStereoOrder();
#endif
   // Accessors...
   virtual bool HasSoloButton(){  return mSoloPref!=wxT("None");}

   virtual wxString TrackSubText(Track *t);

   TrackInfo mTrackInfo;
 public:
    TrackInfo *GetTrackInfo() { return &mTrackInfo; }
    const TrackInfo *GetTrackInfo() const { return &mTrackInfo; }

protected:
   TrackPanelListener *mListener;

   TrackList *mTracks;
   ViewInfo *mViewInfo;

   AdornedRulerPanel *mRuler;

   TrackArtist *mTrackArtist;

   class AUDACITY_DLL_API AudacityTimer:public wxTimer {
   public:
     virtual void Notify() {
       // (From Debian)
       //
       // Don't call parent->OnTimer(..) directly here, but instead post
       // an event. This ensures that this is a pure wxWidgets event
       // (no GDK event behind it) and that it therefore isn't processed
       // within the YieldFor(..) of the clipboard operations (workaround
       // for Debian bug #765341).
       wxTimerEvent *event = new wxTimerEvent(*this);
       parent->GetEventHandler()->QueueEvent(event);
     }
     TrackPanel *parent;
   } mTimer;

   int mTimeCount;

   wxMemoryDC mBackingDC;
   wxBitmap *mBacking;
   bool mResizeBacking;
   bool mRefreshBacking;
   int mPrevWidth;
   int mPrevHeight;

#ifdef EXPERIMENTAL_SPECTRAL_EDITING

   // For toggling of spectral seletion
   double mLastF0;
   double mLastF1;

public:
   void ToggleSpectralSelection();
protected:

#endif

   bool mRedrawAfterStop;

   wxMouseEvent mLastMouseEvent;

   int mMouseMostRecentX;
   int mMouseMostRecentY;

public:
   // Old enumeration of click-and-drag states, which will shrink and disappear
   // as UIHandle subclasses take over the repsonsibilities.
   enum   MouseCaptureEnum
   {
      IsUncaptured=0,   // This is the normal state for the mouse
      IsClosing,
      IsMuting,
      IsSoloing,
      IsMinimizing,
      IsPopping,
   };

protected:
   bool mCircularTrackNavigation;

   friend class TrackPanelAx;

   TrackPanelAx *mAx;

public:
   TrackPanelAx &GetAx() { return *mAx; }

protected:

   wxString mSoloPref;

 protected:

   // The screenshot class needs to access internals
   friend class ScreenshotCommand;

 public:
   wxSize vrulerSize;

 protected:
   std::vector<TrackPanelOverlay*> *mOverlays;

   Track *mpClickedTrack;
   // TrackPanel is not responsible for memory management:
   UIHandle *mUIHandle;

   TrackPanelCell *mpBackground;

   DECLARE_EVENT_TABLE()
};

#ifdef _MSC_VER
#pragma warning( pop )
#endif


//This constant determines the size of the vertical region (in pixels) around
//the bottom of a track that can be used for vertical track resizing.
#define TRACK_RESIZE_REGION 5

#endif
