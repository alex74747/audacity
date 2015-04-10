/**********************************************************************

Audacity: A Digital Audio Editor

SpectralSelectionBar.h

Paul Licameli

**********************************************************************/

#ifndef __AUDACITY_SPECTRAL_SELECTION_BAR__
#define __AUDACITY_SPECTRAL_SELECTION_BAR__

#include <wx/defs.h>

#include "ToolBar.h"

class wxBitmap;
class wxCheckBox;
class wxChoice;
class wxComboBox;
class wxCommandEvent;
class wxDC;
class wxRadioButton;
class wxSizeEvent;

class AButton;
class SpectralSelectionBarListener;
class NumericTextCtrl;

class SpectralSelectionBar :public ToolBar {

public:

   SpectralSelectionBar();
   virtual ~SpectralSelectionBar();

   void Create(wxWindow *parent);

   virtual void Populate();
   virtual void Repaint(wxDC * WXUNUSED(dc)) {};
   virtual void EnableDisableButtons() {};
   virtual void UpdatePrefs();

   void SetFrequencies(double bottom, double top);
   void SetFrequencySelectionFormatName(const wxString & formatName);
   void SetLogFrequencySelectionFormatName(const wxString & formatName);
   void SetListener(SpectralSelectionBarListener *l);

   bool PlayIsDown() const;
   void SetEnabled(bool enabled);
   void SetPlaying(bool down, bool looped);
   void Play(bool looped);

private:

   void ValuesToControls();
   void OnUpdate(wxCommandEvent &evt);
   void OnPlay(wxCommandEvent & event);
   void OnCtrl(wxCommandEvent &evt);
   void OnChoice(wxCommandEvent &evt);

   void OnSize(wxSizeEvent &evt);

   void ModifySpectralSelection(bool done = false);

   SpectralSelectionBarListener * mListener;

   bool mbCenterAndWidth;

   double mCenter; // hertz
   double mWidth; // logarithm of ratio of hertz
   double mLow; // hertz
   double mHigh; // hertz

   AButton *mPlayButton;
   NumericTextCtrl *mCenterCtrl, *mWidthCtrl, *mLowCtrl, *mHighCtrl;
   wxChoice *mChoice;

   int mHeight;   // height of main sizer after creation - used by OnChoice()

public:

   DECLARE_CLASS(SpectralSelectionBar);
   DECLARE_EVENT_TABLE();
};

#endif

