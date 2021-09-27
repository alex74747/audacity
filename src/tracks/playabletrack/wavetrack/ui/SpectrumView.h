/**********************************************************************

Audacity: A Digital Audio Editor

SpectrumView.h

Paul Licameli split from WaveTrackView.h

**********************************************************************/

#ifndef __AUDACITY_SPECTRUM_VIEW__
#define __AUDACITY_SPECTRUM_VIEW__

#include <functional>
#include "WaveTrackView.h" // to inherit


class WaveTrack;
class BrushHandle;
class SpectralData;

class SpectrumView final : public WaveTrackSubView
{
   SpectrumView &operator=( const SpectrumView& ) = delete;
public:
   SpectrumView(WaveTrackView &waveTrackView, const SpectrumView &src) = delete;
   explicit SpectrumView(WaveTrackView &waveTrackView);
   ~SpectrumView() override;

   const Type &SubViewType() const override;

   std::shared_ptr<TrackVRulerControls> DoGetVRulerControls() override;

   std::shared_ptr<SpectralData> GetSpectralData();

   bool IsSpectral() const override;

   static int mBrushRadius;

   void CopyToSubView( WaveTrackSubView *destSubView ) const override;

   class SpectralDataSaver;

private:
    std::weak_ptr<BrushHandle> mBrushHandle;
    std::shared_ptr<SpectralData> mpSpectralData, mpBackupSpectralData;
    bool mOnBrushTool;

   // TrackPanelDrawable implementation
   void Draw(
      TrackPanelDrawingContext &context,
      const wxRect &rect, unsigned iPass ) override;

   void DoDraw( TrackPanelDrawingContext &context,
      const WaveTrack *track,
      const WaveClip* selectedClip,
      const wxRect & rect );

   std::vector<UIHandlePtr> DetailedHitTest(
      const TrackPanelMouseState &state,
      const AudacityProject *pProject, int currentTool, bool bMultiTool )
      override;

   unsigned CaptureKey
   (wxKeyEvent& event, ViewInfo& viewInfo, wxWindow* pParent,
       AudacityProject* project) override;

   unsigned KeyDown(wxKeyEvent& event, ViewInfo& viewInfo, wxWindow* pParent,
       AudacityProject* project) override;

   unsigned Char
   (wxKeyEvent& event, ViewInfo& viewInfo, wxWindow* pParent,
       AudacityProject* project) override;

   static void ForAll( AudacityProject &project,
      std::function<void(SpectrumView &view)> fn );

   void DoSetMinimized( bool minimized ) override;
};

#endif
