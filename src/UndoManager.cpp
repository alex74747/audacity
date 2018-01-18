/**********************************************************************

  Audacity: A Digital Audio Editor

  UndoManager.cpp

  Dominic Mazzoni

*******************************************************************//**

\class UndoManager
\brief Works with HistoryWindow to provide the Undo functionality.

*//****************************************************************//**

\class UndoStackElem
\brief Holds one item with description and time range for the
UndoManager

*//*******************************************************************/


#include "Audacity.h"
#include "UndoManager.h"

#include "BlockFile.h"
#include "Internat.h"
#include "Sequence.h"
#include "Track.h"
#include "WaveTrack.h"          // temp
#include "NoteTrack.h"  // for Sonify* function declarations

#include <algorithm>
#include <map>
#include <set>


UndoStackElem::~UndoStackElem()
{
   tracks->Clear(true);
   delete tracks;
}

UndoManager::UndoManager(TrackFactory *factory)
   : current(-1)
   , saved(-1)
   , consolidationCount(0)
   , mFactory(factory)
{
   ResetODChangesFlag();
}

UndoManager::~UndoManager()
{
   ClearStates();
   AbandonOldSavedStates();
}

// get the sum of the sizes of all blocks this track list
// references.  However, if a block is referred to multiple
// times it is only counted once.  Return value is in bytes.
wxLongLong UndoManager::CalculateSpaceUsage(int index)
{
   TrackListOfKindIterator iter(Track::Wave);
   WaveTrack *wt;
   WaveClipList::compatibility_iterator it;
   BlockArray *blocks;
   unsigned int i;

   // get a map of all blocks referenced in this TrackList
   std::map<BlockFile*, wxLongLong> cur;

   wt = (WaveTrack *) iter.First(stack[index]->tracks);
   while (wt) {
      for (it = wt->GetClipIterator(); it; it = it->GetNext()) {
         blocks = it->GetData()->GetSequenceBlockArray();
         for (i = 0; i < blocks->GetCount(); i++)
         {
            BlockFile* pBlockFile = blocks->Item(i)->f;
            if (pBlockFile->GetFileName().FileExists())
               cur[pBlockFile] = pBlockFile->GetSpaceUsage();
         }
      }
      wt = (WaveTrack *) iter.Next();
   }

   if (index > 0) {
      // get a set of all blocks referenced in all prev TrackList
      std::set<BlockFile*> prev;
      while (--index) {
         wt = (WaveTrack *) iter.First(stack[index]->tracks);
         while (wt) {
            for (it = wt->GetClipIterator(); it; it = it->GetNext()) {
               blocks = it->GetData()->GetSequenceBlockArray();
               for (i = 0; i < blocks->GetCount(); i++) {
                  prev.insert(blocks->Item(i)->f);
               }
            }
            wt = (WaveTrack *) iter.Next();
         }
      }

      // remove all blocks in prevBlockFiles from curBlockFiles
      std::set<BlockFile*>::const_iterator prevIter;
      for (prevIter = prev.begin(); prevIter != prev.end(); prevIter++) {
         cur.erase(*prevIter);
      }
   }

   // sum the sizes of the blocks remaining in curBlockFiles;
   wxLongLong bytes = 0;
   std::map<BlockFile*, wxLongLong>::const_iterator curIter;
   for (curIter = cur.begin(); curIter != cur.end(); curIter++) {
      bytes += curIter->second;
   }

   return bytes;
}

void UndoManager::GetLongDescription(unsigned int n, wxString *desc,
                                     wxString *size)
{
   n -= 1; // 1 based to zero based

   wxASSERT(n < stack.Count());

   *desc = stack[n]->description;

   *size = Internat::FormatSize(stack[n]->spaceUsage);
}

void UndoManager::GetShortDescription(unsigned int n, wxString *desc)
{
   n -= 1; // 1 based to zero based

   wxASSERT(n < stack.Count());

   *desc = stack[n]->shortDescription;
}

void UndoManager::SetLongDescription(unsigned int n, wxString desc)
{
   n -= 1;

   wxASSERT(n < stack.Count());

   stack[n]->description = desc;
}

void UndoManager::RemoveStateAt(int n)
{
   UndoStackElem *tmpStackElem = stack[n];

   if (tmpStackElem->saved == UndoStackElem::UNSAVED) {
      delete tmpStackElem;
   }
   else
      // Auto saved or fully saved, not yet permanently abandoned
      otherSaved.Add(tmpStackElem);

   stack.RemoveAt(n);
}


void UndoManager::RemoveStates(int num)
{
   for (int i = 0; i < num; i++) {
      RemoveStateAt(0);

      current = std::max(-1, current - 1);
      saved = std::max(-1, saved - 1);
   }
}

void UndoManager::ClearStates()
{
   RemoveStates(stack.Count());
}

void UndoManager::AbandonAutoSavedStates(UndoStack &theStack)
{
   for (int ii = 0, num = theStack.Count(); ii < num;) {
      UndoStackElem *state = theStack[ii];
      if (state->saved != UndoStackElem::SAVED) {
         delete state;
         theStack.RemoveAt(ii);
         --num;
      }
      else
         ++ii;
   }
}

void UndoManager::AbandonAutoSavedStates()
{
   AbandonAutoSavedStates(stack);
   AbandonAutoSavedStates(otherSaved);
}

void UndoManager::AbandonOldSavedStates()
{
   for (int ii = 0, num = otherSaved.Count(); ii < num; ++ii)
      delete otherSaved[ii];
   otherSaved.Clear();
}

unsigned int UndoManager::GetNumStates()
{
   return stack.Count();
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
   return (current < (int)stack.Count() - 1);
}

void UndoManager::ModifyState(TrackList * l,
                              const SelectedRegion &selectedRegion)
{
   if (current == wxNOT_FOUND) {
      return;
   }

   SonifyBeginModifyState();
   // Delete current
   stack[current]->tracks->Clear(true);
   delete stack[current]->tracks;

   // Duplicate
   TrackList *tracksCopy = new TrackList();
   TrackListIterator iter(l);
   Track *t = iter.First();
   while (t) {
      tracksCopy->Add(t->Duplicate());
      t = iter.Next();
   }

   // Replace
   stack[current]->tracks = tracksCopy;
   stack[current]->selectedRegion = selectedRegion;
   SonifyEndModifyState();
}

void UndoManager::PushState(TrackList * l,
                            const SelectedRegion &selectedRegion,
                            wxString longDescription,
                            wxString shortDescription,
                            int flags)
{
   unsigned int i;

   // If consolidate is set to true, group up to 3 identical operations.
   if (((flags&PUSH_CONSOLIDATE)!=0) && lastAction == longDescription &&
       consolidationCount < 2) {
      consolidationCount++;
      ModifyState(l, selectedRegion);
      // MB: If the "saved" state was modified by ModifyState, reset
      //  it so that UnsavedChanges returns true.
      if (current == saved) {
         saved = -1;
      }
      return;
   }

   consolidationCount = 0;

   // Destroy in-memory redo history
   i = current + 1;
   while (i < stack.Count()) {
      RemoveStateAt(i);
   }

   TrackList *tracksCopy = new TrackList();
   TrackListIterator iter(l);
   Track *t = iter.First();
   while (t) {
      tracksCopy->Add(t->Duplicate());
      t = iter.Next();
   }

   UndoStackElem *push = new UndoStackElem();
   push->tracks = tracksCopy;
   push->selectedRegion = selectedRegion;
   push->description = longDescription;
   push->shortDescription = shortDescription;
   // Calculate actual space usage after it's on the stack.

   stack.Add(push);
   current++;
   if( (flags&PUSH_CALC_SPACE)!=0)
      push->spaceUsage = this->CalculateSpaceUsage(current);

   if (saved >= current) {
      saved = -1;
   }

   lastAction = longDescription;
}

TrackList *UndoManager::SetStateTo(unsigned int n,
                                   SelectedRegion *selectedRegion)
{
   n -= 1;

   wxASSERT(n < stack.Count());

   current = n;

   if (current == int(stack.Count()-1)) {
      *selectedRegion = stack[current]->selectedRegion;
   }
   else {
      *selectedRegion = stack[current + 1]->selectedRegion;
   }

   lastAction = wxT("");
   consolidationCount = 0;

   return stack[current]->tracks;
}

TrackList *UndoManager::Undo(SelectedRegion *selectedRegion)
{
   wxASSERT(UndoAvailable());

   current--;

   *selectedRegion = stack[current]->selectedRegion;

   lastAction = wxT("");
   consolidationCount = 0;

   return stack[current]->tracks;
}

TrackList *UndoManager::Redo(SelectedRegion *selectedRegion)
{
   wxASSERT(RedoAvailable());

   current++;

   *selectedRegion = stack[current]->selectedRegion;

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

   lastAction = wxT("");
   consolidationCount = 0;

   return stack[current]->tracks;
}

bool UndoManager::UnsavedChanges()
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
//   for (unsigned int i = 0; i < stack.Count(); i++) {
//      TrackListIterator iter(stack[i]->tracks);
//      WaveTrack *t = (WaveTrack *) (iter.First());
//      wxPrintf(wxT("*%d* %s %f\n"), i, (i == (unsigned int)current) ? wxT("-->") : wxT("   "),
//             t ? t->GetEndTime()-t->GetStartTime() : 0);
//   }
//}

///to mark as unsaved changes without changing the state/tracks.
void UndoManager::SetODChangesFlag()
{
   mODChangesMutex.Lock();
   mODChanges=true;
   mODChangesMutex.Unlock();
}

bool UndoManager::HasODChangesFlag()
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

void UndoManager::CloseLockState(UndoStackElem &state)
{
   if (state.saved != UndoStackElem::UNSAVED) {
      TrackListIterator iter(state.tracks);
      Track *t = iter.First();
      while (t) {
         if (t->GetKind() == Track::Wave)
            ((WaveTrack *)t)->CloseLock();
         t = iter.Next();
      }
   }
}

// Come here at file closing time.  Mark block files for non-removal
// for persistency of the last saved undo history.  There is no need
// to "unlock" again as the undo manager will soon be destroyed.
void UndoManager::CloseLockBlocks()
{
   for (int ii = 0, nn = stack.Count(); ii < nn; ++ii)
      CloseLockState(*stack[ii]);
   for (int ii = 0, nn = otherSaved.Count(); ii < nn; ++ii)
      CloseLockState(*otherSaved[ii]);
}

void UndoManager::WriteXML(XMLWriter &xmlFile, bool autoSaving)
{
   xmlFile.StartTag(wxT("undoHistory"));
   xmlFile.WriteAttr(wxT("current"), current);

   wxArrayString empty;
   for (int ii = 0, nn = stack.Count(); ii < nn; ++ii) {
      UndoStackElem &elem = *stack[ii];
      xmlFile.StartTag(wxT("undoRedoState"));

      xmlFile.WriteAttr(wxT("shortDescription"), elem.shortDescription);
      xmlFile.WriteAttr(wxT("description"), elem.description);
      xmlFile.WriteAttr(wxT("spaceUsage"), elem.spaceUsage.GetValue());
      elem.selectedRegion.WriteXMLAttributes(xmlFile);

      elem.tracks->WriteXML(xmlFile, false, empty);

      if (!autoSaving)
         elem.saved = UndoStackElem::SAVED;
      else if (elem.saved != UndoStackElem::SAVED)
         elem.saved = UndoStackElem::AUTOSAVED;

      xmlFile.EndTag(wxT("undoRedoState"));
   }

   if (autoSaving)
      // Undo history states purged with the history window
      // or removed by PushState, but previously saved to disk,
      // are not yet permanently abandoned.
      ;
   else
      // Latest .aup reflects the stack exactly.  All abandoned
      // block files may be cleaned up.
      AbandonOldSavedStates();

   xmlFile.EndTag(wxT("undoHistory"));
}

bool UndoManager::HandleXMLTag(const wxChar *tag, const wxChar **attrs)
{
   while (*attrs) {
      const wxChar *attr = *attrs++;
      const wxChar *value = *attrs++;

      if (!value)
         break;

      if (!wxStrcmp(attr, wxT("current"))) {
         long lCurrent = 0;
         wxString(value).ToLong(&lCurrent);
         current = lCurrent;
      }
      else {
         UndoStackElem &elem = *stack.Last();
        
         if (!wxStrcmp(attr, wxT("shortDescription"))) {
            elem.shortDescription = value;
         }
         else if (!wxStrcmp(attr, wxT("description"))) {
            elem.description = value;
         }
         else if (!wxStrcmp(attr, wxT("spaceUsage"))) {
            long lUsage = 0;
            wxString(value).ToLong(&lUsage);
            elem.spaceUsage = lUsage;
         }
         else
            elem.selectedRegion.HandleXMLAttribute(attr, value);
      }
   }

   return true;
}

void UndoManager::HandleXMLEndTag(const wxChar *tag)
{
   if (!wxStrcmp(tag, wxT("undoHistory"))) {
      // A little safety
      current = std::min(current, int(stack.Count()) - 1);
      saved = current;
   }
}

XMLTagHandler *UndoManager::HandleXMLChild(const wxChar *tag)
{
   if (!wxStrcmp(tag, wxT("undoHistory"))) {
      return this;
   }

   if (!wxStrcmp(tag, wxT("undoRedoState"))) {
      UndoStackElem *push = new UndoStackElem();
      push->tracks = new TrackList;
      push->spaceUsage = 0;
      push->saved = UndoStackElem::SAVED;
      stack.Add(push);
      return this;
   }

   {
      UndoStackElem &elem = *stack.Last();
      XMLTagHandler *handler =
         elem.tracks->HandleXMLChild(tag, mFactory);
      if (handler)
         return handler;
   }

   return NULL;
}
