/**********************************************************************

  Audacity: A Digital Audio Editor

  UndoManager.cpp

  Dominic Mazzoni

*******************************************************************//**

\class UndoManager
\brief Works with HistoryDialog to provide the Undo functionality.

*//****************************************************************//**

\class UndoStackElem
\brief Holds one item with description and time range for the
UndoManager

*//*******************************************************************/


#include "Audacity.h"
#include "UndoManager.h"

#include <wx/hashset.h>

#include "Diags.h"
#include "Project.h"
//#include "NoteTrack.h"  // for Sonify* function declarations
#include "Diags.h"
#include "Tags.h"
#include "Track.h"


wxDEFINE_EVENT(EVT_UNDO_PUSHED, wxCommandEvent);
wxDEFINE_EVENT(EVT_UNDO_MODIFIED, wxCommandEvent);
wxDEFINE_EVENT(EVT_UNDO_OR_REDO, wxCommandEvent);
wxDEFINE_EVENT(EVT_UNDO_RESET, wxCommandEvent);

static const AudacityProject::AttachedObjects::RegisteredFactory key{
   [](AudacityProject &project)
      { return std::make_unique<UndoManager>( project ); }
};

UndoManager &UndoManager::Get( AudacityProject &project )
{
   return project.AttachedObjects::Get< UndoManager >( key );
}

const UndoManager &UndoManager::Get( const AudacityProject &project )
{
   return Get( const_cast< AudacityProject & >( project ) );
}

UndoManager::UndoManager( AudacityProject &project )
   : mProject{ project }
{
   current = -1;
   saved = -1;
   ResetODChangesFlag();
}

UndoManager::~UndoManager()
{
   ClearStates();
}

void UndoManager::GetShortDescription(unsigned int n, TranslatableString *desc)
{
   n -= 1; // 1 based to zero based

   wxASSERT(n < stack.size());

   *desc = stack[n]->shortDescription;
}

void UndoManager::SetLongDescription(
  unsigned int n, const TranslatableString &desc)
{
   n -= 1;

   wxASSERT(n < stack.size());

   stack[n]->description = desc;
}

void UndoManager::RemoveStateAt(int n)
{
   stack.erase(stack.begin() + n);
}


void UndoManager::RemoveStates(int num)
{
   for (int i = 0; i < num; i++) {
      RemoveStateAt(0);

      current -= 1;
      saved -= 1;
   }
}

void UndoManager::ClearStates()
{
   RemoveStates(stack.size());
   current = -1;
   saved = -1;
}

unsigned int UndoManager::GetNumStates()
{
   return stack.size();
}

unsigned int UndoManager::GetCurrentState()
{
   return current + 1;  // the array is 0 based, the abstraction is 1 based
}

bool UndoManager::UndoAvailable()
{
   return (current > 0);
}

bool UndoManager::RedoAvailable()
{
   return (current < (int)stack.size() - 1);
}

void UndoManager::ModifyState(const TrackList * l,
                              const SelectedRegion &selectedRegion,
                              const std::shared_ptr<Tags> &tags)
{
   if (current == wxNOT_FOUND) {
      return;
   }

//   SonifyBeginModifyState();
   // Delete current -- not necessary, but let's reclaim space early
   stack[current]->state.tracks.reset();

   // Duplicate
   auto tracksCopy = TrackList::Create( nullptr );
   for (auto t : *l) {
      if ( t->GetId() == TrackId{} )
         // Don't copy a pending added track
         continue;
      tracksCopy->Add(t->Duplicate());
   }

   // Replace
   stack[current]->state.tracks = std::move(tracksCopy);
   stack[current]->state.tags = tags;

   stack[current]->state.selectedRegion = selectedRegion;
//   SonifyEndModifyState();

   // wxWidgets will own the event object
   mProject.QueueEvent( safenew wxCommandEvent{ EVT_UNDO_MODIFIED } );
}

void UndoManager::PushState(const TrackList * l,
                            const SelectedRegion &selectedRegion,
                            const std::shared_ptr<Tags> &tags,
                            const TranslatableString &longDescription,
                            const TranslatableString &shortDescription,
                            UndoPush flags)
{
   unsigned int i;

   if ( ((flags & UndoPush::CONSOLIDATE) != UndoPush::MINIMAL) &&
       // compare full translations not msgids!
       lastAction.Translation() == longDescription.Translation() &&
       mayConsolidate ) {
      ModifyState(l, selectedRegion, tags);
      // MB: If the "saved" state was modified by ModifyState, reset
      //  it so that UnsavedChanges returns true.
      if (current == saved) {
         saved = -1;
      }
      return;
   }

   auto tracksCopy = TrackList::Create( nullptr );
   for (auto t : *l) {
      if ( t->GetId() == TrackId{} )
         // Don't copy a pending added track
         continue;
      tracksCopy->Add(t->Duplicate());
   }

   mayConsolidate = true;

   i = current + 1;
   while (i < stack.size()) {
      RemoveStateAt(i);
   }

   // Assume tags was duplicated before any changes.
   // Just save a NEW shared_ptr to it.
   stack.push_back(
      std::make_unique<UndoStackElem>
         (std::move(tracksCopy),
            longDescription, shortDescription, selectedRegion, tags)
   );

   current++;

   if (saved >= current) {
      saved = -1;
   }

   lastAction = longDescription;

   // wxWidgets will own the event object
   mProject.QueueEvent( safenew wxCommandEvent{ EVT_UNDO_PUSHED } );
}

void UndoManager::SetStateTo(unsigned int n, const Consumer &consumer)
{
   n -= 1;

   wxASSERT(n < stack.size());

   current = n;

   lastAction = {};
   mayConsolidate = false;

   consumer( *stack[current] );

   // wxWidgets will own the event object
   mProject.QueueEvent( safenew wxCommandEvent{ EVT_UNDO_RESET } );
}

void UndoManager::Undo(const Consumer &consumer)
{
   wxASSERT(UndoAvailable());

   current--;

   lastAction = {};
   mayConsolidate = false;

   consumer( *stack[current] );

   // wxWidgets will own the event object
   mProject.QueueEvent( safenew wxCommandEvent{ EVT_UNDO_OR_REDO } );
}

void UndoManager::Redo(const Consumer &consumer)
{
   wxASSERT(RedoAvailable());

   current++;

   /*
   if (!RedoAvailable()) {
      *sel0 = stack[current]->sel0;
      *sel1 = stack[current]->sel1;
   }
   else {
      current++;
      *sel0 = stack[current]->sel0;
      *sel1 = stack[current]->sel1;
      current--;
   }
   */

   lastAction = {};
   mayConsolidate = false;

   consumer( *stack[current] );

   // wxWidgets will own the event object
   mProject.QueueEvent( safenew wxCommandEvent{ EVT_UNDO_OR_REDO } );
}

void UndoManager::VisitStates( const Consumer &consumer, bool newestFirst )
{
   auto fn = [&]( decltype(stack[0]) &ptr ){ consumer( *ptr ); };
   if (newestFirst)
      std::for_each(stack.rbegin(), stack.rend(), fn);
   else
      std::for_each(stack.begin(), stack.end(), fn);
}

bool UndoManager::UnsavedChanges() const
{
   return (saved != current) || HasODChangesFlag();
}

void UndoManager::StateSaved()
{
   saved = current;
   ResetODChangesFlag();
}

// currently unused
//void UndoManager::Debug()
//{
//   for (unsigned int i = 0; i < stack.size(); i++) {
//      for (auto t : stack[i]->tracks->Any())
//         wxPrintf(wxT("*%d* %s %f\n"),
//                  i, (i == (unsigned int)current) ? wxT("-->") : wxT("   "),
//                t ? t->GetEndTime()-t->GetStartTime() : 0);
//   }
//}

///to mark as unsaved changes without changing the state/tracks.
void UndoManager::SetODChangesFlag()
{
   mODChangesMutex.Lock();
   mODChanges=true;
   mODChangesMutex.Unlock();
}

bool UndoManager::HasODChangesFlag() const
{
   bool ret;
   mODChangesMutex.Lock();
   ret=mODChanges;
   mODChangesMutex.Unlock();
   return ret;
}

void UndoManager::ResetODChangesFlag()
{
   mODChangesMutex.Lock();
   mODChanges=false;
   mODChangesMutex.Unlock();
}
