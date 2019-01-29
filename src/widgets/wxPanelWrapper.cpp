//
//  wxPanelWrapper.cpp
//  Audacity
//
//  Created by Paul Licameli on 6/25/16.
//
//


#include "wxPanelWrapper.h"

#include <wx/filename.h>
#include <wx/grid.h>
#include "../Journal.h"

void wxPanelWrapper::SetLabel(const TranslatableString & label)
{
   wxPanel::SetLabel( label.Translation() );
}

void wxPanelWrapper::SetName(const TranslatableString & name)
{
   wxPanel::SetName( name.Translation() );
}

void wxPanelWrapper::SetToolTip(const TranslatableString &toolTip)
{
   wxPanel::SetToolTip( toolTip.Stripped().Translation() );
}

void wxPanelWrapper::SetName()
{
   wxPanel::SetName( GetLabel() );
}

void wxDialogWrapper::SetTitle(const TranslatableString & title)
{
   wxDialog::SetTitle( title.Translation() );
}

void wxDialogWrapper::SetLabel(const TranslatableString & label)
{
   wxDialog::SetLabel( label.Translation() );
}

void wxDialogWrapper::SetName(const TranslatableString & name)
{
   wxDialog::SetName( name.Translation() );
}

void wxDialogWrapper::SetName()
{
   wxDialog::SetName( wxDialog::GetTitle() );
}

namespace {

struct Entry{
   wxWeakRef< wxDialog > pDialog;

   operator wxDialog* () const { return pDialog; }
};
std::vector< Entry > sDialogStack;

bool IsOutermost( wxDialog &dialog )
{
   for ( const auto &entry : sDialogStack ) {
      if ( entry.pDialog == &dialog )
         return true;
      else if ( entry.pDialog && entry.pDialog->IsModal() )
         return false;
   }
   // Should have found dialog before reaching the end
   wxASSERT( false );
   return false;
}

}


namespace DialogHooks {

void wxTabTraversalWrapperCharHook(wxKeyEvent &event)
{
//#ifdef __WXMAC__
#if defined(__WXMAC__) || defined(__WXGTK__)
   // Compensate for the regressions in TAB key navigation
   // due to the switch to wxWidgets 3.0.2
   if (event.GetKeyCode() == WXK_TAB) {
      auto focus = wxWindow::FindFocus();
      if (dynamic_cast<wxGrid*>(focus)
         || (focus &&
             focus->GetParent() &&
             dynamic_cast<wxGrid*>(focus->GetParent()->GetParent()))) {
         // Let wxGrid do its own TAB key handling
         event.Skip();
         return;
      }
      // Apparently, on wxGTK, FindFocus can return NULL
      if (focus)
      {
         focus->Navigate(
            event.ShiftDown()
            ? wxNavigationKeyEvent::IsBackward
            :  wxNavigationKeyEvent::IsForward
         );
         return;
      }
   }
#endif

   event.Skip();
}

void BeginDialog( wxDialog &dialog )
{
   sDialogStack.push_back( { &dialog } );
}

void EndDialog( wxDialog &dialog )
{
   // Not always LIFO because some dialogs are modeless
   auto end = sDialogStack.end();
   sDialogStack.erase(
      std::remove( sDialogStack.begin(), end, &dialog ),
      end
   );
}

CallbacksBase::~CallbacksBase() {}
// Default no-op implementations
wxArrayString CallbacksBase::GetJournalData() const { return {}; }
void CallbacksBase::SetJournalData( const wxArrayString & ) {}

int ShowModal( wxDialog &dialog, CallbacksBase &callbacks )
{
   constexpr auto ModalDialogToken = wxT("ModalDialog");

   auto name = dialog.GetName();
   if ( Journal::IsReplaying() ) {
      Journal::Sync( { ModalDialogToken, name } );

      // Intercepted ShowModal call takes data from the journal, doesn't call
      // through to DoShowModal()
      auto data = Journal::GetTokens();
      if ( data.size() < 1 )
         throw Journal::SyncException{};

      auto strResult = data.back();
      long result;
      strResult.ToLong( &result );

      // callback may examine data and throw SyncException
      data.pop_back();
      callbacks.SetJournalData( data );

      if ( Journal::IsRecording() ) {
         data.push_back( strResult );
         Journal::Output( data );
      }

      return result;
   }
   else {
      bool record = Journal::IsRecording() && IsOutermost( dialog );
      if ( record )
         Journal::Output( { ModalDialogToken, name } );
      auto result = callbacks.DoShowModal( dialog );
      if ( record ) {
         auto data = callbacks.GetJournalData();
         data.push_back( wxString::Format( "%d", result ) );
         Journal::Output( data );
      }
      return result;
   }
}

}

namespace{
   wxString PathToJournal( const wxString &path )
   {
      auto home = wxFileName::GetHomeDir();
      wxFileName fname{ path };
      fname.MakeRelativeTo( home );
      return fname.GetFullPath();
   }

   wxString PathFromJournal( const wxString &path )
   {
      auto home = wxFileName::GetHomeDir();
      wxFileName fname{ path };
      fname.MakeAbsolute( home );
      return fname.GetFullPath();
   }
}

wxArrayString wxDirDialogWrapper::GetJournalData() const
{
   wxArrayString results;
   results.push_back( PathToJournal( GetPath() ) );
   return results;
}

void wxDirDialogWrapper::SetJournalData( const wxArrayString &data )
{
   if ( data.size() != 1 )
      throw Journal::SyncException{};
   SetPath( PathFromJournal( data[0] ) );
}

wxArrayString FileDialogWrapper::GetJournalData() const
{
   wxArrayString results;
#if 0
   for ( const auto &path : m_paths )
      results.push_back( PathToJournal( path ) );
#endif
   return results;
}

void FileDialogWrapper::SetJournalData( const wxArrayString &data )
{
#if 0
   m_paths.clear();
   for ( const auto &path : data )
      m_paths.push_back( PathFromJournal( path ) );
   SetPath( m_paths.empty() ? wxString{} : m_paths[0] );
#endif
}
