/**********************************************************************

  Audacity: A Digital Audio Editor

  Grid.cpp

  Leland Lucius

*******************************************************************//**

\class Grid
\brief Supplies an accessible grid based on wxGrid.

*//*******************************************************************/

#include "Audacity.h"
#include "Grid.h"

#include "MemoryX.h"

#include <wx/setup.h> // for wxUSE_* macros

#include <wx/defs.h>
#include <wx/choice.h>
#include <wx/dc.h>
#include <wx/grid.h>
#include <wx/intl.h>
#include <wx/settings.h>
#include <wx/toplevel.h>

#if wxUSE_ACCESSIBILITY
#include "WindowAccessible.h"

/**********************************************************************//**
\class GridAx
\brief wxAccessible object providing grid information for Grid.
**************************************************************************/
class GridAx final : public WindowAccessible
{

 public:

   GridAx(Grid *grid);

   void SetCurrentCell(int row, int col);
   void TableUpdated();
   bool GetRowCol(int childId, int & row, int & col);

   // Retrieves the address of an IDispatch interface for the specified child.
   // All objects must support this property.
   wxAccStatus GetChild(int childId, wxAccessible **child) override;

   // Gets the number of children.
   wxAccStatus GetChildCount(int *childCount) override;

   // Gets the default action for this object (0) or > 0 (the action for a child).
   // Return wxACC_OK even if there is no action. actionName is the action, or the empty
   // string if there is no action.
   // The retrieved string describes the action that is performed on an object,
   // not what the object does as a result. For example, a toolbar button that prints
   // a document has a default action of "Press" rather than "Prints the current document."
   wxAccStatus GetDefaultAction(int childId, wxString *actionName) override;

   // Returns the description for this object or a child.
   wxAccStatus GetDescription(int childId, wxString *description) override;

   // Gets the window with the keyboard focus.
   // If childId is 0 and child is NULL, no object in
   // this subhierarchy has the focus.
   // If this object has the focus, child should be 'this'.
   wxAccStatus GetFocus(int *childId, wxAccessible **child) override;

   // Returns help text for this object or a child, similar to tooltip text.
   wxAccStatus GetHelpText(int childId, wxString *helpText) override;

   // Returns the keyboard shortcut for this object or child.
   // Return e.g. ALT+K
   wxAccStatus GetKeyboardShortcut(int childId, wxString *shortcut) override;

   // Returns the rectangle for this object (id = 0) or a child element (id > 0).
   // rect is in screen coordinates.
   wxAccStatus GetLocation(wxRect & rect, int elementId) override;

   // Gets the name of the specified object.
   wxAccStatus GetName(int childId, wxString *name) override;

   // Gets the parent, or NULL.
   wxAccStatus GetParent(wxAccessible **parent) override;

   // Returns a role constant.
   wxAccStatus GetRole(int childId, wxAccRole *role) override;

   // Gets a variant representing the selected children
   // of this object.
   // Acceptable values:
   // - a null variant (IsNull() returns TRUE)
   // - a list variant (GetType() == wxT("list"))
   // - an integer representing the selected child element,
   //   or 0 if this object is selected (GetType() == wxT("long"))
   // - a "void*" pointer to a wxAccessible child object
   wxAccStatus GetSelections(wxVariant *selections) override;

   // Returns a state constant.
   wxAccStatus GetState(int childId, long* state) override;

   // Returns a localized string representing the value for the object
   // or child.
   wxAccStatus GetValue(int childId, wxString* strValue) override;

#if defined(__WXMAC__)
   // Selects the object or child.
   wxAccStatus Select(int childId, wxAccSelectionFlags selectFlags) override;
#endif

   Grid *mGrid;
   int mLastId;

};
#endif

///
///
///

BEGIN_EVENT_TABLE(Grid, wxGrid)
   EVT_SET_FOCUS(Grid::OnSetFocus)
   EVT_KEY_DOWN(Grid::OnKeyDown)
   EVT_GRID_SELECT_CELL(Grid::OnSelectCell)
END_EVENT_TABLE()

Grid::Grid(wxWindow *parent,
           wxWindowID id,
           const wxPoint& pos,
           const wxSize& size,
           long style,
           const wxString& name)
: wxGrid(parent, id, pos, size, style | wxWANTS_CHARS, name)
{
#if wxUSE_ACCESSIBILITY
   GetGridWindow()->SetAccessible(mAx = safenew GridAx(this));
#endif

   // RegisterDataType takes ownership of renderer and editor
}

Grid::~Grid()
{
#if wxUSE_ACCESSIBILITY
   int cnt = mChildren.size();
   while (cnt--) {
      // PRL: I found this loop destroying right-to-left.
      // Is the sequence of destruction important?
      mChildren.pop_back();
   }
#endif
}

void Grid::OnSetFocus(wxFocusEvent &event)
{
   event.Skip();

#if wxUSE_ACCESSIBILITY
   mAx->SetCurrentCell(GetGridCursorRow(), GetGridCursorCol());
#endif
}

void Grid::OnSelectCell(wxGridEvent &event)
{
   event.Skip();

#if wxUSE_ACCESSIBILITY
   mAx->SetCurrentCell(event.GetRow(), event.GetCol());
#endif
}

void Grid::OnKeyDown(wxKeyEvent &event)
{
   switch (event.GetKeyCode())
   {
      case WXK_LEFT:
      case WXK_RIGHT:
      {
         int rows = GetNumberRows();
         int cols = GetNumberCols();
         int crow = GetGridCursorRow();
         int ccol = GetGridCursorCol();

         if (event.GetKeyCode() == WXK_LEFT) {
            if (crow == 0 && ccol == 0) {
               // do nothing
            }
            else if (ccol == 0) {
               SetGridCursor(crow - 1, cols - 1);
            }
            else {
               SetGridCursor(crow, ccol - 1);
            }
         }
         else {
            if (crow == rows - 1 && ccol == cols - 1) {
               // do nothing
            }
            else if (ccol == cols - 1) {
               SetGridCursor(crow + 1, 0);
            }
            else {
               SetGridCursor(crow, ccol + 1);
            }
         }

#if wxUSE_ACCESSIBILITY
         // Make sure the NEW cell is made available to the screen reader
         mAx->SetCurrentCell(GetGridCursorRow(), GetGridCursorCol());
#endif
      }
      break;

      case WXK_TAB:
      {
         int rows = GetNumberRows();
         int cols = GetNumberCols();
         int crow = GetGridCursorRow();
         int ccol = GetGridCursorCol();

         if (event.ControlDown()) {
            int flags = wxNavigationKeyEvent::FromTab |
                        ( event.ShiftDown() ?
                          wxNavigationKeyEvent::IsBackward :
                          wxNavigationKeyEvent::IsForward );
            Navigate(flags);
            return;
         }
         else if (event.ShiftDown()) {
            // Empty grid?
            if (crow == -1 && ccol == -1) {
               Navigate(wxNavigationKeyEvent::FromTab | wxNavigationKeyEvent::IsBackward);
               return;
            }

            if (crow == 0 && ccol == 0) {
               Navigate(wxNavigationKeyEvent::FromTab | wxNavigationKeyEvent::IsBackward);
               return;
            }
            else if (ccol == 0) {
               SetGridCursor(crow - 1, cols - 1);
            }
            else {
               SetGridCursor(crow, ccol - 1);
            }
         }
         else {
            // Empty grid?
            if (crow == -1 && ccol == -1) {
               Navigate(wxNavigationKeyEvent::FromTab | wxNavigationKeyEvent::IsForward);
               return;
            }

            if (crow == rows - 1 && ccol == cols - 1) {
               Navigate(wxNavigationKeyEvent::FromTab | wxNavigationKeyEvent::IsForward);
               return;
            }
            else if (ccol == cols - 1) {
               SetGridCursor(crow + 1, 0);
            }
            else {
               SetGridCursor(crow, ccol + 1);
            }
         }

         MakeCellVisible(GetGridCursorRow(), GetGridCursorCol());

#if wxUSE_ACCESSIBILITY
         // Make sure the NEW cell is made available to the screen reader
         mAx->SetCurrentCell(GetGridCursorRow(), GetGridCursorCol());
#endif
      }
      break;

      case WXK_RETURN:
      case WXK_NUMPAD_ENTER:
      {
         if (!IsCellEditControlShown()) {
            wxTopLevelWindow *tlw = wxDynamicCast(wxGetTopLevelParent(this), wxTopLevelWindow);
            wxWindow *def = tlw->GetDefaultItem();
            if (def && def->IsEnabled()) {
               wxCommandEvent cevent(wxEVT_COMMAND_BUTTON_CLICKED,
                                     def->GetId());
               GetParent()->GetEventHandler()->ProcessEvent(cevent);
            }
         }
         else {
            wxGrid::OnKeyDown(event);

            // This looks strange, but what it does is selects the cell when
            // enter is pressed after editing.  Without it, Jaws and Window-Eyes
            // do not speak the NEW cell contents (the one below the edited one).
            SetGridCursor(GetGridCursorRow(), GetGridCursorCol());
         }
         break;
      }

      default:
         wxGrid::OnKeyDown(event);
      break;
   }
}

#if wxUSE_ACCESSIBILITY
void Grid::ClearGrid()
{
   wxGrid::ClearGrid();

   mAx->TableUpdated();

   return;
}

bool Grid::InsertRows(int pos, int numRows, bool updateLabels)
{
   bool res = wxGrid::InsertRows(pos, numRows, updateLabels);

   mAx->TableUpdated();

   return res;
}

bool Grid::AppendRows(int numRows, bool updateLabels)
{
   bool res = wxGrid::AppendRows(numRows, updateLabels);

   mAx->TableUpdated();

   return res;
}

bool Grid::DeleteRows(int pos, int numRows, bool updateLabels)
{
   bool res = wxGrid::DeleteRows(pos, numRows, updateLabels);

   mAx->TableUpdated();

   return res;
}

bool Grid::InsertCols(int pos, int numCols, bool updateLabels)
{
   bool res = wxGrid::InsertCols(pos, numCols, updateLabels);

   mAx->TableUpdated();

   return res;
}

bool Grid::AppendCols(int numCols, bool updateLabels)
{
   bool res = wxGrid::AppendCols(numCols, updateLabels);

   mAx->TableUpdated();

   return res;
}

bool Grid::DeleteCols(int pos, int numCols, bool updateLabels)
{
   bool res = wxGrid::DeleteCols(pos, numCols, updateLabels);

   mAx->TableUpdated();

   return res;
}

GridAx::GridAx(Grid *grid)
: WindowAccessible(grid->GetGridWindow())
{
   mGrid = grid;
   mLastId = -1;
}

void GridAx::TableUpdated()
{
   NotifyEvent(wxACC_EVENT_OBJECT_REORDER,
               mGrid->GetGridWindow(),
               wxOBJID_CLIENT,
               0);
}

void GridAx::SetCurrentCell(int row, int col)
{
   int id = (((row * mGrid->GetNumberCols()) + col) + 1);

   if (mLastId != -1) {
      NotifyEvent(wxACC_EVENT_OBJECT_SELECTIONREMOVE,
               mGrid->GetGridWindow(),
               wxOBJID_CLIENT,
               mLastId);
   }

   if (mGrid == wxWindow::FindFocus()) {
      NotifyEvent(wxACC_EVENT_OBJECT_FOCUS,
                  mGrid->GetGridWindow(),
                  wxOBJID_CLIENT,
                  id);
   }

   NotifyEvent(wxACC_EVENT_OBJECT_SELECTION,
               mGrid->GetGridWindow(),
               wxOBJID_CLIENT,
               id);

   mLastId = id;
}

bool GridAx::GetRowCol(int childId, int & row, int & col)
{
   if (childId == wxACC_SELF) {
      return false;
   }

   int cols = mGrid->GetNumberCols();
   int id = childId - 1;

   row = id / cols;
   col = id % cols;

   return true;
}

// Retrieves the address of an IDispatch interface for the specified child.
// All objects must support this property.
wxAccStatus GridAx::GetChild(int childId, wxAccessible** child)
{
   if (childId == wxACC_SELF) {
      *child = this;
   }
   else {
      *child = NULL;
   }

   return wxACC_OK;
}

// Gets the number of children.
wxAccStatus GridAx::GetChildCount(int *childCount)
{
   *childCount = mGrid->GetNumberRows() * mGrid->GetNumberCols();

   return wxACC_OK;
}

// Gets the default action for this object (0) or > 0 (the action for a child).
// Return wxACC_OK even if there is no action. actionName is the action, or the empty
// string if there is no action.
// The retrieved string describes the action that is performed on an object,
// not what the object does as a result. For example, a toolbar button that prints
// a document has a default action of "Press" rather than "Prints the current document."
wxAccStatus GridAx::GetDefaultAction(int WXUNUSED(childId), wxString *actionName)
{
   actionName->clear();

   return wxACC_OK;
}

// Returns the description for this object or a child.
wxAccStatus GridAx::GetDescription(int WXUNUSED(childId), wxString *description)
{
   description->clear();

   return wxACC_OK;
}

// Returns help text for this object or a child, similar to tooltip text.
wxAccStatus GridAx::GetHelpText(int WXUNUSED(childId), wxString *helpText)
{
   helpText->clear();

   return wxACC_OK;
}

// Returns the keyboard shortcut for this object or child.
// Return e.g. ALT+K
wxAccStatus GridAx::GetKeyboardShortcut(int WXUNUSED(childId), wxString *shortcut)
{
   shortcut->clear();

   return wxACC_OK;
}

// Returns the rectangle for this object (id = 0) or a child element (id > 0).
// rect is in screen coordinates.
wxAccStatus GridAx::GetLocation(wxRect & rect, int elementId)
{
   wxRect r;
   int row;
   int col;

   if (GetRowCol(elementId, row, col)) {
      rect = mGrid->CellToRect(row, col);
      rect.SetPosition(mGrid->GetGridWindow()->ClientToScreen(rect.GetPosition()));
   }
   else {
      rect = mGrid->GetRect();
      rect.SetPosition(mGrid->GetParent()->ClientToScreen(rect.GetPosition()));
   }

   return wxACC_OK;
}

// Gets the name of the specified object.
wxAccStatus GridAx::GetName(int childId, wxString *name)
{
   int row;
   int col;

   if (GetRowCol(childId, row, col)) {
      wxString n = mGrid->GetColLabelValue(col);
      wxString v = mGrid->GetCellValue(row, col);
      if (v.empty()) {
         v = _("Empty");
      }

      auto editor = mGrid->GetCellEditor(row, col);
      auto cleanup = finally([&]{
      if (editor)
         editor->DecRef();
      });
   
      if ( auto accessibleEditor =
          dynamic_cast<AccessibleGridCellEditor*>( editor ) )
         v = accessibleEditor->ConvertValue( v );

      *name = n + wxT(" ") + v;
   }

   return wxACC_OK;
}

wxAccStatus GridAx::GetParent(wxAccessible ** WXUNUSED(parent))
{
   return wxACC_NOT_IMPLEMENTED;
}

// Returns a role constant.
wxAccStatus GridAx::GetRole(int childId, wxAccRole *role)
{
   if (childId == wxACC_SELF) {
#if defined(__WXMSW__)
      *role = wxROLE_SYSTEM_TABLE;
#endif

#if defined(__WXMAC__)
      *role = wxROLE_SYSTEM_GROUPING;
#endif
   }
   else {
      *role = wxROLE_SYSTEM_TEXT;
   }

   return wxACC_OK;
}

// Gets a variant representing the selected children
// of this object.
// Acceptable values:
// - a null variant (IsNull() returns TRUE)
// - a list variant (GetType() == wxT("list"))
// - an integer representing the selected child element,
//   or 0 if this object is selected (GetType() == wxT("long"))
// - a "void*" pointer to a wxAccessible child object
wxAccStatus GridAx::GetSelections(wxVariant * WXUNUSED(selections))
{
   return wxACC_NOT_IMPLEMENTED;
}

// Returns a state constant.
wxAccStatus GridAx::GetState(int childId, long *state)
{
   int flag = wxACC_STATE_SYSTEM_FOCUSABLE |
              wxACC_STATE_SYSTEM_SELECTABLE;
   int col;
   int row;

   if (!GetRowCol(childId, row, col)) {
      *state = 0;
      return wxACC_FAIL;
   }

#if defined(__WXMSW__)
   flag |= wxACC_STATE_SYSTEM_FOCUSED |
           wxACC_STATE_SYSTEM_SELECTED;

      if (mGrid->IsReadOnly(row, col)) {
         // It would be more logical to also include the state
         // wxACC_STATE_SYSTEM_FOCUSABLE, but this causes Window-Eyes to 
         // no longer read the cell as disabled
         flag = wxACC_STATE_SYSTEM_UNAVAILABLE | wxACC_STATE_SYSTEM_FOCUSED;
      }
#endif

#if defined(__WXMAC__)
   if (mGrid->IsInSelection(row, col)) {
      flag |= wxACC_STATE_SYSTEM_SELECTED;
   }

   if (mGrid->GetGridCursorRow() == row && mGrid->GetGridCursorCol() == col) {
       flag |= wxACC_STATE_SYSTEM_FOCUSED;
   }

   if (mGrid->IsReadOnly(row, col)) {
      flag |= wxACC_STATE_SYSTEM_UNAVAILABLE;
   }
#endif

   *state = flag;

   return wxACC_OK;
}

// Returns a localized string representing the value for the object
// or child.
#if defined(__WXMAC__)
wxAccStatus GridAx::GetValue(int childId, wxString *strValue)
#else
wxAccStatus GridAx::GetValue(int WXUNUSED(childId), wxString *strValue)
#endif
{
   strValue->clear();

#if defined(__WXMSW__)
   return wxACC_OK;
#endif

#if defined(__WXMAC__)
   return GetName(childId, strValue);
#endif
}

#if defined(__WXMAC__)
// Selects the object or child.
wxAccStatus GridAx::Select(int childId, wxAccSelectionFlags selectFlags)
{
   int row;
   int col;

   if (GetRowCol(childId, row, col)) {

      if (selectFlags & wxACC_SEL_TAKESELECTION) {
         mGrid->SetGridCursor(row, col);
      }

      mGrid->SelectBlock(row, col, row, col, selectFlags & wxACC_SEL_ADDSELECTION);
   }

   return wxACC_OK;
}
#endif

// Gets the window with the keyboard focus.
// If childId is 0 and child is NULL, no object in
// this subhierarchy has the focus.
// If this object has the focus, child should be 'this'.
wxAccStatus GridAx::GetFocus(int * childId, wxAccessible **child)
{
   if (mGrid == wxWindow::FindFocus()) {
      if (mGrid->GetNumberRows() * mGrid->GetNumberCols() == 0) {
         *child = this;
      }
      else {
         *childId = mGrid->GetGridCursorRow()*mGrid->GetNumberCols() +
            mGrid->GetGridCursorCol() + 1;
      }
   }

   return wxACC_OK;
}

#endif // wxUSE_ACCESSIBILITY
