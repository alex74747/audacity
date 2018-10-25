#include <wx/scrolbar.h>

// This wrapper prevents the scrollbars from retaining focus after being
// used.  Otherwise, the only way back to the track panel is to click it
// and that causes your original location to be lost.
class ScrollBar final : public wxScrollBar
{
public:
   ScrollBar(wxWindow* parent, wxWindowID id, long style)
   :   wxScrollBar(parent, id, wxDefaultPosition, wxDefaultSize, style)
   {
   }

   void OnSetFocus(wxFocusEvent & e)
   {
      wxWindow *w = e.GetWindow();
      if (w != NULL) {
         w->SetFocus();
      }
   }

   void SetScrollbar(int position, int thumbSize,
                     int range, int pageSize,
                     bool refresh = true) override;

private:
   DECLARE_EVENT_TABLE()
};
