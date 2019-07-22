/**********************************************************************

Audacity: A Digital Audio Editor

ViewInfo.cpp

Paul Licameli

**********************************************************************/

#include "ViewInfo.h"



#include <algorithm>

#include "Prefs.h"
#include "Project.h"
#include "xml/XMLWriter.h"
#include "prefs/TracksBehaviorsPrefs.h"
#include "xml/XMLWriter.h"

wxDEFINE_EVENT( EVT_SELECTED_REGION_CHANGE, SelectedRegionEvent );

SelectedRegionEvent::SelectedRegionEvent(
   wxEventType commandType, NotifyingSelectedRegion *pReg )
: wxEvent{ 0, commandType }
, pRegion{ pReg }
{}

wxEvent *SelectedRegionEvent::Clone() const
{
   return safenew SelectedRegionEvent{ *this };
}

bool NotifyingSelectedRegion::HandleXMLAttribute
   (const wxChar *attr, const wxChar *value,
    const wxChar *legacyT0Name, const wxChar *legacyT1Name)
{
   auto result = mRegion.HandleXMLAttribute(
      attr, value, legacyT0Name, legacyT1Name );
   if ( result )
      Notify( true );
   return result;
}

NotifyingSelectedRegion& NotifyingSelectedRegion::operator =
( const SelectedRegion &other )
{
   if ( mRegion != other ) {
      mRegion = other;
      Notify();
   }
   return *this;
}

bool NotifyingSelectedRegion::setTimes(double t0, double t1)
{
   bool result = false;
   if ( mRegion.t0() != t0 || mRegion.t1() != t1 ) {
      result = mRegion.setTimes( t0, t1 );
      Notify();
   }
   return result;
}

bool NotifyingSelectedRegion::setT0(double t, bool maySwap)
{
   bool result = false;
   if ( mRegion.t0() != t ) {
      result = mRegion.setT0( t, maySwap );
      Notify();
   }
   return result;
}

bool NotifyingSelectedRegion::setT1(double t, bool maySwap)
{
   bool result = false;
   if ( mRegion.t1() != t ) {
      result = mRegion.setT1( t, maySwap );
      Notify();
   }
   return result;
}

void NotifyingSelectedRegion::collapseToT0()
{
   if ( mRegion.t0() !=  mRegion.t1() ) {
      mRegion.collapseToT0();
      Notify();
   }
}

void NotifyingSelectedRegion::collapseToT1()
{
   if ( mRegion.t0() !=  mRegion.t1() ) {
      mRegion.collapseToT1();
      Notify();
   }
}

void NotifyingSelectedRegion::move(double delta)
{
   if (delta != 0) {
      mRegion.move( delta );
      Notify();
   }
}

bool NotifyingSelectedRegion::setFrequencies(double f0, double f1)
{
   bool result = false;
   if ( mRegion.f0() != f0 || mRegion.f1() != f1 ) {
      result = mRegion.setFrequencies( f0, f1 );
      Notify();
   }
   return result;
}

bool NotifyingSelectedRegion::setF0(double f, bool maySwap)
{
   bool result = false;
   if ( mRegion.f0() != f ) {
      result = mRegion.setF0( f, maySwap );
      Notify();
   }
   return result;
}

bool NotifyingSelectedRegion::setF1(double f, bool maySwap)
{
   bool result = false;
   if ( mRegion.f1() != f ) {
      result = mRegion.setF1( f, maySwap );
      Notify();
   }
   return result;
}

void NotifyingSelectedRegion::Notify( bool delayed )
{
   SelectedRegionEvent evt{ EVT_SELECTED_REGION_CHANGE, this };
   if ( delayed )
      QueueEvent( evt.Clone() );
   else
      ProcessEvent( evt );
}

static const AudacityProject::AttachedObjects::RegisteredFactory key{
   []( AudacityProject &project ) {
      return std::make_unique<ViewInfo>(0.0, 1.0, ZoomInfo::GetDefaultZoom());
   }
};

ViewInfo &ViewInfo::Get( AudacityProject &project )
{
   return project.AttachedObjects::Get< ViewInfo >( key );
}

const ViewInfo &ViewInfo::Get( const AudacityProject &project )
{
   return Get( const_cast< AudacityProject & >( project ) );
}

ViewInfo::ViewInfo(double start, double screenDuration, double pixelsPerSecond)
   : ZoomInfo(start, pixelsPerSecond)
   , selectedRegion()
   , total(screenDuration)
   , sbarH(0)
   , sbarScreen(1)
   , sbarTotal(1)
   , sbarScale(1.0)
   , scrollStep(16)
   , bUpdateTrackIndicator(true)
   , bScrollBeyondZero(false)
{
   UpdatePrefs();
}

void ViewInfo::UpdateSelectedPrefs( int id )
{
   if (id == UpdateScrollPrefsID())
      gPrefs->Read(wxT("/GUI/AutoScroll"), &bUpdateTrackIndicator,
                   true);
   ZoomInfo::UpdateSelectedPrefs( id );
}

void ViewInfo::UpdatePrefs()
{
   ZoomInfo::UpdatePrefs();
#ifdef EXPERIMENTAL_SCROLLING_LIMITS
   gPrefs->Read(TracksBehaviorsPrefs::ScrollingPreferenceKey(), &bScrollBeyondZero,
                TracksBehaviorsPrefs::ScrollingPreferenceDefault());
#endif
   gPrefs->Read(wxT("/GUI/AdjustSelectionEdges"), &bAdjustSelectionEdges,
      true);

   UpdateSelectedPrefs( UpdateScrollPrefsID() );
}

void ViewInfo::SetBeforeScreenWidth(wxInt64 beforeWidth, wxInt64 screenWidth, double lowerBoundTime)
{
   h =
      std::max(lowerBoundTime,
         std::min(total - screenWidth / zoom,
         beforeWidth / zoom));
}

void ViewInfo::WriteXMLAttributes(XMLWriter &xmlFile) const
// may throw
{
   selectedRegion.WriteXMLAttributes(xmlFile, wxT("sel0"), wxT("sel1"));
   xmlFile.WriteAttr(wxT("vpos"), vpos);
   xmlFile.WriteAttr(wxT("h"), h, 10);
   xmlFile.WriteAttr(wxT("zoom"), zoom, 10);
}

bool ViewInfo::ReadXMLAttribute(const wxChar *attr, const wxChar *value)
{
   if (selectedRegion.HandleXMLAttribute(attr, value, wxT("sel0"), wxT("sel1")))
      return true;

   if (!wxStrcmp(attr, wxT("vpos"))) {
      long longVpos;
      wxString(value).ToLong(&longVpos);
      vpos = (int)(longVpos);
      return true;
   }

   if (!wxStrcmp(attr, wxT("h"))) {
      Internat::CompatibleToDouble(value, &h);
      return true;
   }

   if (!wxStrcmp(attr, wxT("zoom"))) {
      Internat::CompatibleToDouble(value, &zoom);
      return true;
   }

   return false;
}

int ViewInfo::UpdateScrollPrefsID()
{
   static int value = wxNewId();
   return value;
}

/// Takes a value between min and max and returns a value between
/// height and 0
int GetWaveYPos(float value, float min, float max,
                int height, bool dB, bool outer,
                float dBr, bool clip)
{
   if (dB) {
      if (height == 0) {
         return 0;
      }

      float sign = (value >= 0 ? 1 : -1);

      if (value != 0.) {
         float db = LINEAR_TO_DB(fabs(value));
         value = (db + dBr) / dBr;
         if (!outer) {
            value -= 0.5;
         }
         if (value < 0.0) {
            value = 0.0;
         }
         value *= sign;
      }
   }
   else {
      if (!outer) {
         if (value >= 0.0) {
            value -= 0.5;
         }
         else {
            value += 0.5;
         }
      }
   }

   if (clip) {
      if (value < min) {
         value = min;
      }
      if (value > max) {
         value = max;
      }
   }

   value = (max - value) / (max - min);
   return (int) (value * (height - 1) + 0.5);
}

float FromDB(float value, double dBRange)
{
   if (value == 0)
      return 0;

   double sign = (value >= 0 ? 1 : -1);
   return DB_TO_LINEAR((fabs(value) * dBRange) - dBRange) * sign;
}

float ValueOfPixel(int yy, int height, bool offset,
   bool dB, double dBRange, float zoomMin, float zoomMax)
{
   wxASSERT(height > 0);
   // Map 0 to max and height - 1 (not height) to min
   float v =
      height == 1 ? (zoomMin + zoomMax) / 2 :
      zoomMax - (yy / (float)(height - 1)) * (zoomMax - zoomMin);
   if (offset) {
      if (v > 0.0)
         v += .5;
      else
         v -= .5;
   }

   if (dB)
      v = FromDB(v, dBRange);

   return v;
}
