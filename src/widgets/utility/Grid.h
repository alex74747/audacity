/**********************************************************************

  Audacity: A Digital Audio Editor

  Grid.h

  Leland Lucius

**********************************************************************/

#ifndef __AUDACITY_WIDGETS_GRID__
#define __AUDACITY_WIDGETS_GRID__

#include <memory>
#include <vector>
#include <wx/setup.h> // for wxUSE_* macros
#include <wx/defs.h>
#include <wx/grid.h> // to inherit wxGridCellEditor

#if wxUSE_ACCESSIBILITY
class GridAx;
#endif

/**********************************************************************//**
\class Grid
\brief wxGrid with support for accessibility.
**************************************************************************/

class Grid final : public wxGrid
{

 public:

   Grid(wxWindow *parent,
        wxWindowID id,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        long style = wxWANTS_CHARS | wxBORDER,
        const wxString& name = wxPanelNameStr);

   ~Grid();

#if wxUSE_ACCESSIBILITY
   void ClearGrid();
   bool InsertRows(int pos = 0, int numRows = 1, bool updateLabels = true);
   bool AppendRows(int numRows = 1, bool updateLabels = true);
   bool DeleteRows(int pos = 0, int numRows = 1, bool updateLabels = true);
   bool InsertCols(int pos = 0, int numCols = 1, bool updateLabels = true);
   bool AppendCols(int numCols = 1, bool updateLabels = true);
   bool DeleteCols(int pos = 0, int numCols = 1, bool updateLabels = true);

   GridAx *GetNextAx(GridAx *parent, wxAccRole role, int row, int col);
#endif

 protected:

   void OnSetFocus(wxFocusEvent &event);
   void OnSelectCell(wxGridEvent &event);
   void OnKeyDown(wxKeyEvent &event);

 private:

#if wxUSE_ACCESSIBILITY
   GridAx *mAx;
   std::vector<std::unique_ptr<GridAx>> mChildren;
   int mObjNdx;
#endif

 public:

   DECLARE_EVENT_TABLE()
};

// If a cell editor derives from this class, then invoke the virtual function
// when composing the accessibility name.
class AccessibleGridCellEditor /* not final */
   : public wxGridCellEditor
{
public:
   virtual wxString ConvertValue( const wxString &value ) = 0;
};

#endif

