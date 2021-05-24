/**********************************************************************

Audacity: A Digital Audio Editor

Registry.cpp

Paul Licameli split from Menus.cpp

**********************************************************************/

#include "Registry.h"

#include <unordered_set>

#include <wx/log.h>

#include "widgets/AudacityMessageBox.h"

namespace {

struct ItemOrdering;

using namespace Registry;
struct CollectedItems
{
   struct Item{
      // Predefined, or merged from registry already:
      BaseItem *visitNow;
      // Corresponding item from the registry, its sub-items to be merged:
      GroupItem *mergeLater;
      // Ordering hint for the merged item:
      OrderingHint hint;
   };
   std::vector< BaseItemSharedPtr > &computedItems;
   std::vector< Item > items;
   std::unordered_set<Identifier> mResolvedConflicts;

   // A linear search.  Smarter search may not be worth the effort.
   using Iterator = decltype( items )::iterator;
   auto Find( const Identifier &name ) -> Iterator
   {
      auto end = items.end();
      return name.empty()
         ? end
         : std::find_if( items.begin(), end,
            [&]( const Item& item ){
               return name == item.visitNow->name; } );
   }

   auto InsertNewItemUsingPreferences(
      ItemOrdering &itemOrdering, BaseItem *pItem ) -> bool;

   auto InsertNewItemUsingHint( ItemOrdering &itemOrdering,
      BaseItem *pItem, const OrderingHint &hint, size_t endItemsCount,
      bool force )
         -> bool;

   auto MergeLater( Item &found, const Identifier &name ) -> GroupItem *;

   auto SubordinateSingleItem( Item &found, BaseItem *pItem ) -> void;

   auto SubordinateMultipleItems( Item &found, GroupItem *pItems ) -> void;

   auto MergeWithExistingItem(
      Visitor &visitor, ItemOrdering &itemOrdering,
      BaseItem *pItem, OrderingHint::ConflictResolutionPolicy policy) -> bool;

   using NewItem = std::pair< BaseItem*, OrderingHint >;
   using NewItems = std::vector< NewItem >;

   auto InsertFirstNamedItem( ItemOrdering &itemOrdering,
      NewItem &item, size_t endItemsCount, bool force )
         -> bool;

   auto MergeLikeNamedItems(
      Visitor &visitor, ItemOrdering &itemOrdering,
      NewItems::const_iterator left, NewItems::const_iterator right,
      int iPass )
         -> void;

   auto MergeItemsAscendingNamesPass(
      Visitor &visitor, ItemOrdering &itemOrdering,
      NewItems &newItems, int iPass, size_t endItemsCount, bool force )
         -> void;

   auto MergeItemsDescendingNamesPass(
      Visitor &visitor, ItemOrdering &itemOrdering,
      NewItems &newItems, int iPass, size_t endItemsCount, bool force )
         -> void;

   auto MergeItems(
      Visitor &visitor, ItemOrdering &itemOrdering,
      const BaseItemPtrs &toMerge, const OrderingHint &hint ) -> void;
};

// When a computed or shared item, or nameless grouping, specifies a hint and
// the subordinate does not, propagate the hint.
OrderingHint ChooseHint(BaseItem *delegate, const OrderingHint &hint)
{
   auto result =
      !delegate || delegate->orderingHint.type == OrderingHint::Unspecified
      ? hint
      : delegate->orderingHint;
   auto policy =
      !delegate || delegate->orderingHint.policy == OrderingHint::Error
      ? hint.policy
      : delegate->orderingHint.policy;
   result.policy = policy;
   return result;
}

// "Collection" of items is the first pass of visitation, and resolves
// delegation and delayed computation and splices transparent group nodes.
// This first pass is done at each group, starting with a top-level group.
// This pass does not descend to the leaves.  Rather, the visitation passes
// alternate as the entire tree is recursively visited.

// forward declaration for mutually recursive functions
void CollectItem( Registry::Visitor &visitor,
   CollectedItems &collection, BaseItem *Item, const OrderingHint &hint );
void CollectItems( Registry::Visitor &visitor,
   CollectedItems &collection, const BaseItemPtrs &items,
   const OrderingHint &hint )
{
   for ( auto &item : items )
      CollectItem( visitor, collection, item.get(),
         ChooseHint( item.get(), hint ) );
}
void CollectItem( Registry::Visitor &visitor,
   CollectedItems &collection, BaseItem *pItem, const OrderingHint &hint )
{
   if (!pItem)
      return;

   using namespace Registry;
   if (const auto pShared =
       dynamic_cast<SharedItem*>( pItem )) {
      auto delegate = pShared->ptr.get();
      if ( delegate )
         // recursion
         CollectItem( visitor, collection, delegate,
            ChooseHint( delegate, hint ) );
   }
   else
   if (const auto pComputed =
       dynamic_cast<ComputedItem*>( pItem )) {
      auto result = pComputed->factory( visitor );
      if (result) {
         // Guarantee long enough lifetime of the result
         collection.computedItems.push_back( result );
         // recursion
         CollectItem( visitor, collection, result.get(),
            ChooseHint( result.get(), hint ) );
      }
   }
   else
   if (auto pGroup = dynamic_cast<GroupItem*>(pItem)) {
      if (pGroup->Transparent() && pItem->name.empty())
         // nameless grouping item is transparent to path calculations
         // collect group members now
         // recursion
         CollectItems(
            visitor, collection, pGroup->items, ChooseHint( pGroup, hint ) );
      else
         // all other group items
         // defer collection of members until collecting at next lower level
         collection.items.push_back( {pItem, nullptr, hint} );
   }
   else {
      wxASSERT( dynamic_cast<SingleItem*>(pItem) );
      // common to all single items
      collection.items.push_back( {pItem, nullptr, hint} );
   }
}

using Path = std::vector< Identifier >;

std::unordered_set< wxString > sBadPaths;
void BadPath(
  const TranslatableString &format, const wxString &key, const Identifier &name )
{
  // Warn, but not more than once in a session for each bad path
  auto badPath = key + '/' + name.GET();
  if ( sBadPaths.insert( badPath ).second ) {
     auto msg = TranslatableString{ format }.Format( badPath );
     // debug message
     wxLogDebug( msg.Debug() );
#ifdef IS_ALPHA
     // user-visible message
     AudacityMessageBox( msg );
#endif
  }
}

void ReportGroupGroupCollision( const wxString &key, const Identifier &name )
{
   BadPath(
XO("Plug-in group at %s was merged with a previously defined group"),
      key, name);
}

void ReportItemItemCollision( const wxString &key, const Identifier &name )
{
   BadPath(
XO("Plug-in item at %s conflicts with a previously defined item and was discarded"),
      key, name);
}

void ReportConflictingPlacements( const wxString &key, const Identifier &name )
{
   BadPath(
XO("Plug-in items at %s specify conflicting placements"),
      key, name);
}

struct ItemOrdering {
   wxString key;

   ItemOrdering( const Path &path )
   {
      // The set of path names determines only an unordered tree.
      // We want an ordering of the tree that is stable across runs.
      // The last used ordering for this node can be found in preferences at this
      // key:
      wxArrayString strings;
      for (const auto &id : path)
         strings.push_back( id.GET() );
      key = '/' + ::wxJoin( strings, '/', '\0' );
   }

   // Retrieve the old ordering on demand, if needed to merge something.
   bool gotOrdering = false;
   wxString strValue;
   wxArrayString ordering;

   auto Get() -> wxArrayString & {
      if ( !gotOrdering ) {
         gPrefs->Read(key, &strValue);
         ordering = ::wxSplit( strValue, ',' );
         gotOrdering = true;
      }
      return ordering;
   };

   int Find( Identifier component ) {
      auto &components = Get();
      auto begin = components.begin();
      auto end = components.end();
      auto found = std::find( begin, end, component.GET() );
      if ( found == end )
         return -1;
      return found - begin;
   }
};

// For each group node, this is called only in the first pass of merging of
// items.  It might fail to place an item in the first visitation of a
// registry, but then succeed in later visitations in the same or later
// runs of the program, because of persistent side-effects on the
// preferences done at the very end of the visitation.
auto CollectedItems::InsertNewItemUsingPreferences(
   ItemOrdering &itemOrdering, BaseItem *pItem )
   -> bool
{
   // Note that if more than one plug-in registers items under the same
   // node, then it is not specified which plug-in is handled first,
   // the first time registration happens.  It might happen that you
   // add a plug-in, run the program, then add another, then run again;
   // registration order determined by those actions might not
   // correspond to the order of re-loading of modules in later
   // sessions.  But whatever ordering is chosen the first time some
   // plug-in is seen -- that ordering gets remembered in preferences.

   auto &name = pItem->name;
   if ( !name.empty() ) {
      // Check saved ordering first, and rebuild that as well as is possible
      auto &ordering = itemOrdering.Get();
      auto begin2 = ordering.begin(), end2 = ordering.end(),
         found2 = std::find( begin2, end2, name );
      if ( found2 != end2 ) {
         auto insertPoint = items.end();
         // Find the next name in the saved ordering that is known already
         // in the collection.
         while ( ++found2 != end2 ) {
            auto known = Find( *found2 );
            if ( known != insertPoint ) {
               insertPoint = known;
               break;
            }
         }
         items.insert( insertPoint, {pItem, nullptr,
            // Hints no longer matter:
            {}} );
         return true;
      }
   }

   return false;
}

// For each group node, this may be called in the second and later passes
// of merging of items
auto CollectedItems::InsertNewItemUsingHint( ItemOrdering &itemOrdering,
   BaseItem *pItem, const OrderingHint &hint, size_t endItemsCount,
   bool force ) -> bool
{
   auto begin = items.begin(), end = items.end(),
      insertPoint = end - endItemsCount;
   auto &ordering = itemOrdering.ordering;
   auto orderingInsertPoint = ordering.end() - endItemsCount;

   // pItem should have a name; if not, ignore the hint, and put it at the
   // default place, but only if in the final pass.
   if ( pItem->name.empty() ) {
      if ( !force )
         return false;
   }
   else {
      switch ( hint.type ) {
         case OrderingHint::Before:
         case OrderingHint::After: {
            // Default to the end if the name is not found.
            auto found = Find( hint.name );
            if ( found == end ) {
               if ( !force )
                  return false;
               else {
                  insertPoint = found;
                  orderingInsertPoint = ordering.end();
               }
            }
            else {
               insertPoint = found;
               orderingInsertPoint = std::find(
                  ordering.begin(), ordering.end(), hint.name );
               if ( hint.type == OrderingHint::After ) {
                  ++insertPoint;
                  if ( orderingInsertPoint != ordering.end() )
                     ++orderingInsertPoint;
               }
            }
            break;
         }
         case OrderingHint::Begin:
            insertPoint = begin;
            orderingInsertPoint = ordering.begin();
            break;
         case OrderingHint::End:
            orderingInsertPoint = ordering.end();
            insertPoint = end;
            break;
         case OrderingHint::Unspecified:
         default:
            if ( !force )
               return false;
            break;
      }
   }

   // Insert the item; the hint has been used and no longer matters
   items.insert( insertPoint, {pItem, nullptr,
      // Hints no longer matter:
      {}} );

   // update the ordering preference too, so as not to lose any information
   // in it, in case of named but not yet loaded items mentioned in the
   // preferences
   if (!itemOrdering.ordering.empty() && !pItem->name.empty())
      itemOrdering.ordering.insert( orderingInsertPoint, pItem->name.GET() );
   return true;
}

auto CollectedItems::MergeLater( Item &found, const Identifier &name )
   -> GroupItem *
{
   auto &subGroup = found.mergeLater;
   if ( !subGroup ) {
      auto newGroup = std::make_shared<TransparentGroupItem<>>( name );
      computedItems.push_back( newGroup );
      subGroup = newGroup.get();
   }
   return subGroup;
}

auto CollectedItems::SubordinateSingleItem( Item &found, BaseItem *pItem )
   -> void
{
   MergeLater( found, pItem->name )->items.push_back(
      std::make_unique<SharedItem>(
         // shared pointer with vacuous deleter
         std::shared_ptr<BaseItem>( pItem, [](void*){} ) ) );
}

auto CollectedItems::SubordinateMultipleItems( Item &found, GroupItem *pItems )
   -> void
{
   auto subGroup = MergeLater( found, pItems->name );
   for ( const auto &pItem : pItems->items )
      subGroup->items.push_back( std::make_unique<SharedItem>(
         // shared pointer with vacuous deleter
         std::shared_ptr<BaseItem>( pItem.get(), [](void*){} ) ) );
}

auto CollectedItems::MergeWithExistingItem(
   Visitor &visitor, ItemOrdering &itemOrdering,
   BaseItem *pItem, OrderingHint::ConflictResolutionPolicy policy) -> bool
{
   // Assume no null pointers remain after CollectItems:
   const auto &name = pItem->name;
   const auto found = Find( name );
   if (found != items.end()) {
      // Collision of names between collection and registry!
      // There are 2 * 2 = 4 cases, as each of the two are group items or
      // not.
      auto pCollectionGroup = dynamic_cast< GroupItem * >( found->visitNow );
      auto pRegistryGroup = dynamic_cast< GroupItem * >( pItem );
      if (pCollectionGroup) {
         if (pRegistryGroup) {
            // This is the expected case of collision.
            // Subordinate items from one of the groups will be merged in
            // another call to MergeItems at a lower level of path.
            // Note, however, that at most one of the two should be other
            // than a plain grouping item; if not, we must lose the extra
            // information carried by one of them.
            bool pCollectionGrouping = pCollectionGroup->Transparent();
            auto pRegistryGrouping = pRegistryGroup->Transparent();
            if ( !(pCollectionGrouping || pRegistryGrouping) )
               ReportGroupGroupCollision( itemOrdering.key, name );

            if ( pCollectionGrouping && !pRegistryGrouping ) {
               // Swap their roles
               found->visitNow = pRegistryGroup;
               SubordinateMultipleItems( *found, pCollectionGroup );
            }
            else
               SubordinateMultipleItems( *found, pRegistryGroup );
         }
         else {
            // Registered non-group item collides with a previously defined
            // group.
            // Resolve this by subordinating the non-group item below
            // that group.
            SubordinateSingleItem( *found, pItem );
         }
      }
      else {
         if (pRegistryGroup) {
            // Subordinate the previously merged single item below the
            // newly merged group.
            // In case the name occurred in two different static registries,
            // the final merge is the same, no matter which is treated first.
            auto demoted = found->visitNow;
            found->visitNow = pRegistryGroup;
            SubordinateSingleItem( *found, demoted );
         }
         else {
            // Collision of non-group items.
            // Try conflict resolution.
            switch( policy ) {
            case OrderingHint::Ignore:
               break;
            case OrderingHint::Replace:
               // At most one item with this policy may be substituted
               if (mResolvedConflicts.insert(name).second) {
                  found->visitNow = pItem;
                  break;
               }
               /* fallthrough */
            case OrderingHint::Error:
            default:
               // Unresolved collision of non-group items is the worst case!
               // The later-registered item is lost.
               // Which one you lose might be unpredictable when both originate
               // from static registries.
               ReportItemItemCollision( itemOrdering.key, name );
               break;
            }
         }
      }
      return true;
   }
   else
      // A name is registered that is not known in the collection.
      return false;
}

auto CollectedItems::InsertFirstNamedItem( ItemOrdering &itemOrdering,
   NewItem &item, size_t endItemsCount, bool force )
   -> bool
{
   // Try to place the first item of the range.
   // If such an item is a group, then we always retain the kind of
   // grouping that was registered.  (Which doesn't always happen when
   // there is name collision in MergeWithExistingItem.)

   // Later passes for choosing placements.
   // Maybe it fails in this pass, because a placement refers to some
   // other name that has not yet been placed.
   bool success = InsertNewItemUsingHint( itemOrdering,
      item.first, item.second, endItemsCount, force );
   wxASSERT( !force || success );

   return success;
}

auto CollectedItems::MergeLikeNamedItems(
   Visitor &visitor, ItemOrdering &itemOrdering,
   NewItems::const_iterator left, NewItems::const_iterator right,
   const int iPass )
   -> void
{
   // Resolve collisions among remaining like-named items.
   auto iter = left;
   auto &item = *iter;
   auto pItem = item.first;
   const auto &hint = item.second;
   ++iter;
   while ( iter != right ) {
      if ( iter->second.type != OrderingHint::Unspecified &&
          !( iter->second == hint ) ) {
         // A diagnostic message sometimes
         ReportConflictingPlacements( itemOrdering.key, pItem->name );
      }
      // Re-invoke MergeWithExistingItem for this item, which is known
      // to have a name collision, so ignore the return value.
      MergeWithExistingItem(
         visitor, itemOrdering, iter->first, iter->second.policy );
      ++iter;
   }
}

inline bool MajorComp(
   const CollectedItems::NewItem &a, const CollectedItems::NewItem &b) {
   // Descending sort!
   return a.first->name > b.first->name;
};
inline bool MinorComp(
   const CollectedItems::NewItem &a, const CollectedItems::NewItem &b){
   // Sort by hint type.
   // This sorts items with unspecified hints last.
   return a.second < b.second;
};
inline bool Comp(
   const CollectedItems::NewItem &a, const CollectedItems::NewItem &b){
   if ( MajorComp( a, b ) )
      return true;
   if ( MajorComp( b, a ) )
      return false;
   return MinorComp( a, b );
};

auto CollectedItems::MergeItemsAscendingNamesPass(
  Visitor &visitor, ItemOrdering &itemOrdering, NewItems &newItems,
  const int iPass, size_t endItemsCount, bool force ) -> void
{
   // Inner loop over ranges of like-named items.
   auto rright = newItems.rbegin();
   auto rend = newItems.rend();
   while ( rright != rend ) {
      // Find the range
      using namespace std::placeholders;
      auto rleft = std::find_if(
         rright + 1, rend, std::bind( MajorComp, _1, *rright ) );

      auto left = rleft.base(), right = rright.base();

      bool success = (left->second.type == iPass) &&
         InsertFirstNamedItem( itemOrdering,
            *left, endItemsCount, force );

      if (success)
         MergeLikeNamedItems(
            visitor, itemOrdering, left, right, iPass );

      if ( success ) {
         auto diff = rend - rleft;
         newItems.erase( left, right );
         rend = newItems.rend();
         rleft = rend - diff;
      }
      rright = rleft;
   }
}

auto CollectedItems::MergeItemsDescendingNamesPass(
  Visitor &visitor, ItemOrdering &itemOrdering, NewItems &newItems,
  const int iPass, size_t endItemsCount, bool force ) -> void
{
   // Inner loop over ranges of like-named items.
   auto left = newItems.begin();
   while ( left != newItems.end() ) {
      // Find the range
      using namespace std::placeholders;
      auto right = std::find_if(
         left + 1, newItems.end(), std::bind( MajorComp, *left, _1 ) );

      bool success = (left->second.type == iPass) &&
         InsertFirstNamedItem( itemOrdering,
            *left, endItemsCount, force );

      if (success)
         MergeLikeNamedItems(
            visitor, itemOrdering, left, right, iPass );

      if ( success )
         left = newItems.erase( left, right );
      else
         left = right;
   }
};

auto CollectedItems::MergeItems(
  Visitor &visitor, ItemOrdering &itemOrdering,
  const BaseItemPtrs &toMerge, const OrderingHint &hint ) -> void
{
   // First do expansion of nameless groupings, and caching of computed
   // items, just as for the previously collected items.
   CollectedItems newCollection{ computedItems };
   CollectItems( visitor, newCollection, toMerge, hint );

   // Try to merge each, resolving name collisions with items already in the
   // tree, and collecting those with names that don't collide.
   NewItems newItems;
   for ( const auto &item : newCollection.items )
      if ( !MergeWithExistingItem(
         visitor, itemOrdering, item.visitNow, item.hint.policy ) )
          newItems.push_back( { item.visitNow, item.hint } );

   // Choose placements for items with NEW names.
   auto begin = newItems.begin(), end = newItems.end();

   // Segregate the ones that are placed by preferences.
   const auto middle = std::partition( begin, end,
      [&](const NewItem &item){
         return -1 == itemOrdering.Find(item.first->name);
      } );

   // Sort those according to their (descending) place in the preferences.
   std::sort( middle, end,
      [&](const NewItem &a, const NewItem &b){
         return itemOrdering.Find(a.first->name) >
            itemOrdering.Find(b.first->name);
      } );

   // Process them
   for ( auto iter = middle; iter != end; ) {
      auto pItem = iter ->first;
      auto &name = pItem->name;
      bool success = InsertNewItemUsingPreferences( itemOrdering, pItem );
      wxASSERT( success );

      auto right = iter + 1;
      while ( right != end && right->first->name == name )
         ++right;
      MergeLikeNamedItems( visitor, itemOrdering, iter, right, -1 );
      iter = right;
   }

   newItems.erase( middle, end );
   end = newItems.end();

   // Sort others so that like named items are together, and for the same name,
   // items with more specific ordering hints come earlier.
   std::sort( begin, end, Comp );

   // Outer loop over trial passes.
   int iPass = 0;
   bool force = false;
   size_t oldSize = newItems.size();
   int endItemsCount = 0;
   auto prevSize = oldSize;
   while( !newItems.empty() )
   {
      // If several items have the same hint, we try to preserve the sort by
      // name (an internal identifier, not necessarily user visible), just to
      // have some determinacy.  That requires passing one or the other way
      // over newItems.
      bool descending =
         ( iPass == OrderingHint::After || iPass == OrderingHint::Begin );

      if ( descending )
         MergeItemsDescendingNamesPass(
            visitor, itemOrdering, newItems, iPass, endItemsCount, force );
      else
         MergeItemsAscendingNamesPass(
            visitor, itemOrdering, newItems, iPass, endItemsCount, force );

      auto newSize = newItems.size();

      if ( iPass == OrderingHint::End )
         // Remember how many were placed; so that default placement is
         // before all explicit End items, but after other items
         endItemsCount = prevSize - newSize;
      wxASSERT(endItemsCount >= 0);

      ++iPass;
      if ( iPass == OrderingHint::Unspecified ) {
         // Don't place the Unspecified until we have passed through the other
         // ordering hint types with no further progress in placement of
         // other items, and then once more, forcing placement with Before and
         // After hints that reference a nonexistent item.
         if ( !force ) {
            // Begin and End placements always succeed, so don't retry them.
            iPass = OrderingHint::Before;
            // Retry placement of Before and After items, in case they
            // depended on placement of other items that were not yet placed.
            force = (oldSize == newSize);
            oldSize = newSize;
         }
      }

      prevSize = newSize;
   }
}

// forward declaration for mutually recursive functions
void VisitItem(
   Registry::Visitor &visitor, CollectedItems &collection,
   Path &path, BaseItem *pItem,
   const GroupItem *pToMerge, const OrderingHint &hint,
   bool &doFlush );
void VisitItems(
   Registry::Visitor &visitor, CollectedItems &collection,
   Path &path, GroupItem *pGroup,
   const GroupItem *pToMerge, const OrderingHint &hint,
   bool &doFlush )
{
   // Make a NEW collection for this subtree, sharing the memo cache
   CollectedItems newCollection{ collection.computedItems };

   // Gather items at this level
   // (The ordering hint is irrelevant when not merging items in)
   CollectItems( visitor, newCollection, pGroup->items, {} );

   path.push_back( pGroup->name.GET() );

   // Merge with the registry
   if ( pToMerge )
   {
      ItemOrdering itemOrdering{ path };
      newCollection.MergeItems( visitor, itemOrdering, pToMerge->items, hint );

      // Remember the NEW ordering, if there was any need to use the old.
      // This makes a side effect in preferences.
      if ( itemOrdering.gotOrdering ) {
         wxString newValue;
         for ( const auto &name : itemOrdering.ordering ) {
            if ( !name.empty() )
               newValue += newValue.empty()
                  ? name
                  : ',' + name   ;
         }
         if (newValue != itemOrdering.strValue) {
            gPrefs->Write( itemOrdering.key, newValue );
            doFlush = true;
         }
      }
   }

   // Now visit them
   for ( const auto &item : newCollection.items )
      VisitItem( visitor, collection, path,
         item.visitNow, item.mergeLater, item.hint,
         doFlush );

   path.pop_back();
}
void VisitItem(
   Registry::Visitor &visitor, CollectedItems &collection,
   Path &path, BaseItem *pItem,
   const GroupItem *pToMerge, const OrderingHint &hint,
   bool &doFlush )
{
   if (!pItem)
      return;

   if (const auto pSingle =
       dynamic_cast<SingleItem*>( pItem )) {
      wxASSERT( !pToMerge );
      visitor.Visit( *pSingle, path );
   }
   else
   if (const auto pGroup =
       dynamic_cast<GroupItem*>( pItem )) {
      visitor.BeginGroup( *pGroup, path );
      // recursion
      VisitItems(
         visitor, collection, path, pGroup, pToMerge, hint, doFlush );
      visitor.EndGroup( *pGroup, path );
   }
   else
      wxASSERT( false );
}

}

namespace Registry {

BaseItem::~BaseItem() {}

SharedItem::~SharedItem() {}

ComputedItem::~ComputedItem() {}

SingleItem::~SingleItem() {}

GroupItem::~GroupItem() {}

Visitor::~Visitor(){}
void Visitor::BeginGroup(GroupItem &, const Path &) {}
void Visitor::EndGroup(GroupItem &, const Path &) {}
void Visitor::Visit(SingleItem &, const Path &) {}

void Visit( Visitor &visitor, BaseItem *pTopItem, const GroupItem *pRegistry )
{
   visitor.computedItems.clear();
   bool doFlush = false;
   CollectedItems collection{ visitor.computedItems };
   Path emptyPath;
   VisitItem(
      visitor, collection, emptyPath, pTopItem,
      pRegistry, pRegistry->orderingHint, doFlush );
   // Flush any writes done by MergeItems()
   if (doFlush)
      gPrefs->Flush();
}

OrderingPreferenceInitializer::OrderingPreferenceInitializer(
   Literal root, Pairs pairs )
   : mPairs{ std::move( pairs ) }
   , mRoot{ root }
{
   (*this)();
}

void OrderingPreferenceInitializer::operator () ()
{
   bool doFlush = false;
   for (const auto &pair : mPairs) {
      const auto key = wxString{'/'} + mRoot + pair.first;
      if ( gPrefs->Read(key).empty() ) {
         gPrefs->Write( key, pair.second );
         doFlush = true;
      }
   }
   
   if (doFlush)
      gPrefs->Flush();
}

}

namespace {

auto Finder( BaseItemPtrs **ppItems )
{
   // Since registration determines only an unordered tree of menu items,
   // we can sort children of each node lexicographically for our convenience.
   struct Comparator {
      bool operator()
         ( const Identifier &component, const BaseItemPtr& pItem ) const {
            return component < pItem->name; }
      bool operator()
         ( const BaseItemPtr& pItem, const Identifier &component ) const {
            return pItem->name < component; }
   };
   return [ppItems]( const Identifier &component ){
      return std::equal_range(
         ppItems[0]->begin(), ppItems[0]->end(), component, Comparator() ); };
};

/*!
 @post if `createGroups` then return value is not null
 */
BaseItemPtrs *LocateItem(
   GroupItem &registry, const Placement &placement, bool createGroups )
{
   BaseItemPtrs *pItems;
   const auto find = Finder(&pItems);

   auto pNode = &registry;
   pItems = &pNode->items;

   const auto pathComponents = ::wxSplit( placement.path, '/' );
   auto pComponent = pathComponents.begin(), end = pathComponents.end();

   // Descend the registry hierarchy, while groups matching the path components
   // can be found
   auto debugPath = wxString{'/'} + registry.name.GET();
   while ( pComponent != end ) {
      const auto &pathComponent = *pComponent;

      // Try to find an item already present that is a group item with the
      // same name; we don't care which if there is more than one.
      const auto range = find( pathComponent );
      const auto iter2 = std::find_if( range.first, range.second,
         [](const BaseItemPtr &pItem){
            return dynamic_cast< GroupItem* >( pItem.get() ); } );

      if ( iter2 != range.second ) {
         // A matching group in the registry, so descend
         pNode = static_cast< GroupItem* >( iter2->get() );
         pItems = &pNode->items;
         debugPath += '/' + pathComponent;
         ++pComponent;
      }
      else
         // Insert at this level;
         // If there are no more path components, and a name collision of
         // the added item with something already in the registry, don't resolve
         // it yet in this function, but see MergeItems().
         break;
   }

   if (!createGroups && pComponent != end)
      return nullptr;

   // Create path group items for remaining components
   while ( pComponent != end ) {
      auto newNode = std::make_unique<TransparentGroupItem<>>( *pComponent );
      pNode = newNode.get();
      pItems->insert( find( pNode->name ).second, std::move( newNode ) );
      pItems = &pNode->items;
      ++pComponent;
   }

   return pItems;
}

}

namespace Registry {

void RegisterItem( GroupItem &registry, const Placement &placement,
   BaseItemPtr pItem )
{
   auto pItems = LocateItem(registry, placement, true);
   wxASSERT( pItems );

   const auto find = Finder(&pItems);

   // Remember the hint, to be used later in merging.
   pItem->orderingHint = placement.hint;

   // Now insert the item.
   pItems->insert( find( pItem->name ).second, std::move( pItem ) );
}


bool UnregisterItem( GroupItem &registry, const Placement &placement,
   const BaseItem *pItem )
{
   auto pItems = LocateItem(registry, placement, false);
   if (!pItems)
      return false;

   const auto find = Finder(&pItems);
   const auto range = find( pItem->name );
   if (range.first == range.second)
      return false;

   // Assuming unregistration is complementary to the sequence of registrations,
   // this is the correct way to unregister in case of name collisions,
   // rather than erasing at range.first.
   pItems->erase( range.second - 1 );
   return true;
}

}
