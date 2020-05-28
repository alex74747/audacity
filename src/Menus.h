/**********************************************************************

  Audacity: A Digital Audio Editor

  Menus.h

  Dominic Mazzoni

**********************************************************************/
#ifndef __AUDACITY_MENUS__
#define __AUDACITY_MENUS__

#include "Identifier.h"

#include <wx/string.h> // member variable
#include "Prefs.h"
#include "ClientData.h"
#include "Registry.h"
#include "commands/CommandFlag.h"

class wxArrayString;
class wxCommandEvent;
class AudacityProject;
class CommandContext;
class CommandManager;
class PluginDescriptor;
class Track;
class TrackList;
class ViewInfo;

enum EffectType : int;

typedef wxString PluginID;
typedef wxString MacroID;
typedef wxArrayString PluginIDs;

// Base class for objects, to whose member functions, the CommandManager will
// dispatch.
//
// It, or a subclass of it, must be the first base class of the object, and the
// first base class of that base class, etc., for the same reason that
// wxEvtHandler must be first (that is, the downcast from a pointer to the base
// to a pointer to the object, must be a vacuous operation).
//
// In fact, then, we just make it an alias of wxEvtHandler, in case you really
// need to inherit from wxEvtHandler for other reasons, and otherwise you
// couldn't satisfy the requirement for both base classes at once.
using CommandHandlerObject = wxEvtHandler;

// First of two functions registered with each command: an extractor
// of the handler object from the AudacityProject
using CommandHandlerFinder =
   std::function< CommandHandlerObject&(AudacityProject &) >;

// Second of two function pointers registered with each command: a pointer
// to a member function of the handler object
using CommandFunctorPointer =
   void (CommandHandlerObject::*)(const CommandContext &context );

class AUDACITY_DLL_API MenuCreator
{
public:
   MenuCreator();
   ~MenuCreator();
   void CreateMenusAndCommands(AudacityProject &project);
   void RebuildMenuBar(AudacityProject &project);

   static void RebuildAllMenuBars();

public:
   CommandFlag mLastFlags;
   
   // Last effect applied to this project
   PluginID mLastGenerator{};
   PluginID mLastEffect{};
   PluginID mLastAnalyzer{};
   int mLastAnalyzerRegistration;
   int mLastAnalyzerRegisteredId;
   PluginID mLastTool{};
   int mLastToolRegistration;
   int mLastToolRegisteredId;
   enum {
      repeattypenone = 0,
      repeattypeplugin = 1,
      repeattypeunique = 2,
      repeattypeapplymacro = 3
   };
   unsigned mRepeatGeneratorFlags;
   unsigned mRepeatEffectFlags;
   unsigned mRepeatAnalyzerFlags;
   unsigned mRepeatToolFlags;
};

struct MenuVisitor : Registry::Visitor
{
   // final overrides
   void BeginGroup( Registry::GroupItem &item, const Path &path ) final;
   void EndGroup( Registry::GroupItem &item, const Path& ) final;
   void Visit( Registry::SingleItem &item, const Path &path ) final;

   // added virtuals
   virtual void DoBeginGroup( Registry::GroupItem &item, const Path &path );
   virtual void DoEndGroup( Registry::GroupItem &item, const Path &path );
   virtual void DoVisit( Registry::SingleItem &item, const Path &path );
   virtual void DoSeparator();

private:
   void MaybeDoSeparator();
   std::vector<bool> firstItem;
   std::vector<bool> needSeparator;
};

struct ToolbarMenuVisitor : MenuVisitor
{
   explicit ToolbarMenuVisitor( AudacityProject &p ) : project{ p } {}
   operator AudacityProject & () const { return project; }
   AudacityProject &project;
};

struct ToolbarMenuVisitor;

class AUDACITY_DLL_API MenuManager final
   : public MenuCreator
   , public ClientData::Base
   , private PrefsListener
{
public:

   static MenuManager &Get( AudacityProject &project );
   static const MenuManager &Get( const AudacityProject &project );

   explicit
   MenuManager( AudacityProject &project );
   MenuManager( const MenuManager & ) PROHIBITED;
   MenuManager &operator=( const MenuManager & ) PROHIBITED;
   ~MenuManager();

   static void Visit( ToolbarMenuVisitor &visitor );

   static void ModifyUndoMenuItems(AudacityProject &project);
   static void ModifyToolbarMenus(AudacityProject &project);
   // Calls ModifyToolbarMenus() on all projects
   static void ModifyAllProjectToolbarMenus();

   // checkActive is a temporary hack that should be removed as soon as we
   // get multiple effect preview working
   void UpdateMenus( bool checkActive = true );

   // If checkActive, do not do complete flags testing on an
   // inactive project as it is needlessly expensive.
   CommandFlag GetUpdateFlags( bool checkActive = false ) const;
   void UpdatePrefs() override;

   // Command Handling
   bool ReportIfActionNotAllowed(
      const TranslatableString & Name, CommandFlag & flags, CommandFlag flagsRqd );
   bool TryToMakeActionAllowed(
      CommandFlag & flags, CommandFlag flagsRqd );


private:
   void TellUserWhyDisallowed(const TranslatableString & Name, CommandFlag flagsGot,
      CommandFlag flagsRequired);

   void OnUndoRedo( wxCommandEvent &evt );

   AudacityProject &mProject;

public:
   // 0 is grey out, 1 is Autoselect, 2 is Give warnings.
   int  mWhatIfNoSelection;
   bool mStopIfWasPaused;
};

namespace MenuTable {
   using CommandParameter = CommandID;

   // type of a function that determines checkmark state
   using CheckFn = std::function< bool(AudacityProject&) >;

   struct Options
   {
      Options() {}
      // Allow implicit construction from an accelerator string, which is
      // a very common case
      Options( const wxChar *accel_ ) : accel{ accel_ } {}
      // A two-argument constructor for another common case
      Options(
         const wxChar *accel_,
         const TranslatableString &longName_ )
      : accel{ accel_ }, longName{ longName_ } {}

      Options &&Accel (const wxChar *value) &&
         { accel = value; return std::move(*this); }
      Options &&IsEffect (bool value = true) &&
         { bIsEffect = value; return std::move(*this); }
      Options &&Parameter (const CommandParameter &value) &&
         { parameter = value; return std::move(*this); }
      Options &&LongName (const TranslatableString &value ) &&
         { longName = value; return std::move(*this); }
      Options &&IsGlobal () &&
         { global = true; return std::move(*this); }
      Options &&UseStrictFlags () &&
         { useStrictFlags = true; return std::move(*this); }
      Options &&WantKeyUp () &&
         { wantKeyUp = true; return std::move(*this); }
      Options &&SkipKeyDown () &&
         { skipKeyDown = true; return std::move(*this); }

      // This option affects debugging only:
      Options &&AllowDup () &&
         { allowDup = true; return std::move(*this); }

      Options &&AllowInMacros ( int value = 1 ) &&
         { allowInMacros = value; return std::move(*this); }

      // CheckTest is overloaded
      // Take arbitrary predicate
      Options &&CheckTest (const CheckFn &fn) &&
         { checker = fn; return std::move(*this); }
      // Take a preference path
      Options &&CheckTest (const wxChar *key, bool defaultValue) && {
         checker = MakeCheckFn( key, defaultValue );
         return std::move(*this);
      }

      const wxChar *accel{ wxT("") };
      CheckFn checker; // default value means it's not a check item
      bool bIsEffect{ false };
      CommandParameter parameter{};
      TranslatableString longName{};
      bool global{ false };
      bool useStrictFlags{ false };
      bool wantKeyUp{ false };
      bool skipKeyDown{ false };
      bool allowDup{ false };
      int allowInMacros{ -1 }; // 0 = never, 1 = always, -1 = deduce from label

   private:
      static CheckFn
         MakeCheckFn( const wxString key, bool defaultValue );
      static CheckFn
         MakeCheckFn( const BoolSetting &setting );
   };

   // Define items that populate tables that specifically describe menu trees
   using namespace Registry;

   // These are found by dynamic_cast
   struct MenuSection {
      virtual ~MenuSection();
   };
   struct WholeMenu {
      WholeMenu( bool extend = false ) : extension{ extend }  {}
      virtual ~WholeMenu();
      bool extension;
   };

   // Describes a main menu in the toolbar, or a sub-menu
   struct MenuItem final : ConcreteGroupItem< false, ToolbarMenuVisitor >
      , WholeMenu {
      // Construction from an internal name and a previously built-up
      // vector of pointers
      MenuItem( const Identifier &internalName,
         const TranslatableString &title_, BaseItemPtrs &&items_ );
      // In-line, variadic constructor that doesn't require building a vector
      template< typename... Args >
         MenuItem( const Identifier &internalName,
            const TranslatableString &title_, Args&&... args )
            : ConcreteGroupItem< false, ToolbarMenuVisitor >{
               internalName, std::forward<Args>(args)... }
            , title{ title_ }
         {}
      ~MenuItem() override;

      TranslatableString title;
   };

   // Collects other items that are conditionally shown or hidden, but are
   // always available to macro programming
   struct ConditionalGroupItem final
      : ConcreteGroupItem< false, ToolbarMenuVisitor > {
      using Condition = std::function< bool() >;

      // Construction from an internal name and a previously built-up
      // vector of pointers
      ConditionalGroupItem( const Identifier &internalName,
         Condition condition_, BaseItemPtrs &&items_ );
      // In-line, variadic constructor that doesn't require building a vector
      template< typename... Args >
         ConditionalGroupItem( const Identifier &internalName,
            Condition condition_, Args&&... args )
            : ConcreteGroupItem< false, ToolbarMenuVisitor >{
               internalName, std::forward<Args>(args)... }
            , condition{ condition_ }
         {}
      ~ConditionalGroupItem() override;

      Condition condition;
   };

   // usage:
   //   auto scope = FinderScope( findCommandHandler );
   //   return Items( ... );
   //
   // or:
   //   return ( FinderScope( findCommandHandler ), Items( ... ) );
   //
   // where findCommandHandler names a function.
   // This is used before a sequence of many calls to Command() and
   // CommandGroup(), so that the finder argument need not be specified
   // in each call.
   class FinderScope : ValueRestorer< CommandHandlerFinder >
   {
      static CommandHandlerFinder sFinder;

   public:
      static CommandHandlerFinder DefaultFinder() { return sFinder; }

      explicit
      FinderScope( CommandHandlerFinder finder )
         : ValueRestorer( sFinder, finder )
      {}
   };

   // Describes one command in a menu
   struct CommandItem final : SingleItem {
      CommandItem(const CommandID &name_,
               const TranslatableString &label_in_,
               CommandFunctorPointer callback_,
               CommandFlag flags_,
               const Options &options_,
               CommandHandlerFinder finder_);

      // Takes a pointer to member function directly, and delegates to the
      // previous constructor; useful within the lifetime of a FinderScope
      template< typename Handler >
      CommandItem(const CommandID &name_,
               const TranslatableString &label_in_,
               void (Handler::*pmf)(const CommandContext&),
               CommandFlag flags_,
               const Options &options_,
               CommandHandlerFinder finder = FinderScope::DefaultFinder())
         : CommandItem(name_, label_in_,
            static_cast<CommandFunctorPointer>(pmf),
            flags_, options_, finder)
      {}

      ~CommandItem() override;

      const TranslatableString label_in;
      CommandHandlerFinder finder;
      CommandFunctorPointer callback;
      CommandFlag flags;
      Options options;
   };

   // Describes several successive commands in a menu that are closely related
   // and dispatch to one common callback, which will be passed a number
   // in the CommandContext identifying the command
   struct CommandGroupItem final : SingleItem {
      CommandGroupItem(const Identifier &name_,
               std::vector< ComponentInterfaceSymbol > items_,
               CommandFunctorPointer callback_,
               CommandFlag flags_,
               bool isEffect_,
               CommandHandlerFinder finder_);

      // Takes a pointer to member function directly, and delegates to the
      // previous constructor; useful within the lifetime of a FinderScope
      template< typename Handler >
      CommandGroupItem(const Identifier &name_,
               std::vector< ComponentInterfaceSymbol > items_,
               void (Handler::*pmf)(const CommandContext&),
               CommandFlag flags_,
               bool isEffect_,
               CommandHandlerFinder finder = FinderScope::DefaultFinder())
         : CommandGroupItem(name_, std::move(items_),
            static_cast<CommandFunctorPointer>(pmf),
            flags_, isEffect_, finder)
      {}

      ~CommandGroupItem() override;

      const std::vector<ComponentInterfaceSymbol> items;
      CommandHandlerFinder finder;
      CommandFunctorPointer callback;
      CommandFlag flags;
      bool isEffect;
   };

   // For manipulating the enclosing menu or sub-menu directly,
   // adding any number of items, not using the CommandManager
   struct SpecialItem final : SingleItem
   {
      using Appender = std::function< void( AudacityProject&, wxMenu& ) >;

      explicit SpecialItem( const Identifier &internalName, const Appender &fn_ )
      : SingleItem{ internalName }
      , fn{ fn_ }
      {}
      ~SpecialItem() override;

      Appender fn;
   };

   struct MenuPart : ConcreteGroupItem< false, ToolbarMenuVisitor >, MenuSection
   {
      template< typename... Args >
      explicit
      MenuPart( const Identifier &internalName, Args&&... args )
         : ConcreteGroupItem< false, ToolbarMenuVisitor >{
            internalName, std::forward< Args >( args )... }
      {}
   };
   using MenuItems = ConcreteGroupItem< true, ToolbarMenuVisitor >;

   // The following, and Shared(), are the functions to use directly
   // in writing table definitions.

   // Group items can be constructed two ways.
   // Pointers to subordinate items are moved into the result.
   // Null pointers are permitted, and ignored when building the menu.
   // Items are spliced into the enclosing menu.
   // The name is untranslated and may be empty, to make the group transparent
   // in identification of items by path.  Otherwise try to keep the name
   // stable across Audacity versions.
   template< typename... Args >
   inline std::unique_ptr< MenuItems > Items(
      const Identifier &internalName, Args&&... args )
         { return std::make_unique< MenuItems >(
            internalName, std::forward<Args>(args)... ); }

   // Like Items, but insert a menu separator between the menu section and
   // any other items or sections before or after it in the same (innermost,
   // enclosing) menu.
   // It's not necessary that the sisters of sections be other sections, but it
   // might clarify the logical groupings.
   template< typename... Args >
   inline std::unique_ptr< MenuPart > Section(
      const Identifier &internalName, Args&&... args )
         { return std::make_unique< MenuPart >(
            internalName, std::forward<Args>(args)... ); }
   
   // Menu items can be constructed two ways, as for group items
   // Items will appear in a main toolbar menu or in a sub-menu.
   // The name is untranslated.  Try to keep the name stable across Audacity
   // versions.
   // If the name of a menu is empty, then subordinate items cannot be located
   // by path.
   template< typename... Args >
   inline std::unique_ptr<MenuItem> Menu(
      const Identifier &internalName, const TranslatableString &title, Args&&... args )
         { return std::make_unique<MenuItem>(
            internalName, title, std::forward<Args>(args)... ); }
   inline std::unique_ptr<MenuItem> Menu(
      const Identifier &internalName, const TranslatableString &title, BaseItemPtrs &&items )
         { return std::make_unique<MenuItem>(
            internalName, title, std::move( items ) ); }

   // Conditional group items can be constructed two ways, as for group items
   // These items register in the CommandManager but are not shown in menus
   // if the condition evaluates false.
   // The name is untranslated.  Try to keep the name stable across Audacity
   // versions.
   // Name for conditional group must be non-empty.
   template< typename... Args >
   inline std::unique_ptr<ConditionalGroupItem> ConditionalItems(
      const Identifier &internalName,
      ConditionalGroupItem::Condition condition, Args&&... args )
         { return std::make_unique<ConditionalGroupItem>(
            internalName, condition, std::forward<Args>(args)... ); }
   inline std::unique_ptr<ConditionalGroupItem> ConditionalItems(
      const Identifier &internalName, ConditionalGroupItem::Condition condition,
      BaseItemPtrs &&items )
         { return std::make_unique<ConditionalGroupItem>(
            internalName, condition, std::move( items ) ); }

   // Make either a menu item or just a group, depending on the nonemptiness
   // of the title.
   // The name is untranslated and may be empty, to make the group transparent
   // in identification of items by path.  Otherwise try to keep the name
   // stable across Audacity versions.
   // If the name of a menu is empty, then subordinate items cannot be located
   // by path.
   template< typename... Args >
   inline BaseItemPtr MenuOrItems(
      const Identifier &internalName, const TranslatableString &title, Args&&... args )
         {  if ( title.empty() )
               return Items( internalName, std::forward<Args>(args)... );
            else
               return std::make_unique<MenuItem>(
                  internalName, title, std::forward<Args>(args)... ); }
   inline BaseItemPtr MenuOrItems(
      const Identifier &internalName,
      const TranslatableString &title, BaseItemPtrs &&items )
         {  if ( title.empty() )
               return Items( internalName, std::move( items ) );
            else
               return std::make_unique<MenuItem>(
                  internalName, title, std::move( items ) ); }

   template< typename Handler >
   inline std::unique_ptr<CommandItem> Command(
      const CommandID &name,
      const TranslatableString &label_in,
      void (Handler::*pmf)(const CommandContext&),
      CommandFlag flags, const Options &options = {},
      CommandHandlerFinder finder = FinderScope::DefaultFinder())
   {
      return std::make_unique<CommandItem>(
         name, label_in, pmf, flags, options, finder
      );
   }

   template< typename Handler >
   inline std::unique_ptr<CommandGroupItem> CommandGroup(
      const Identifier &name,
      std::vector< ComponentInterfaceSymbol > items,
      void (Handler::*pmf)(const CommandContext&),
      CommandFlag flags, bool isEffect = false,
      CommandHandlerFinder finder = FinderScope::DefaultFinder())
   {
      return std::make_unique<CommandGroupItem>(
         name, std::move(items), pmf, flags, isEffect, finder
      );
   }

   inline std::unique_ptr<SpecialItem> Special(
      const Identifier &name, const SpecialItem::Appender &fn )
         { return std::make_unique<SpecialItem>( name, fn ); }

   // Typically you make a static object of this type in the .cpp file that
   // also defines the added menu actions.
   // pItem can be specified by an expression using the inline functions above.
   struct AttachedItem final
   {
      AttachedItem( const Placement &placement, BaseItemPtr pItem );

      AttachedItem( const wxString &path, BaseItemPtr pItem )
         // Delegating constructor
         : AttachedItem( Placement{ path }, std::move( pItem ) )
      {}
   };

   void DestroyRegistry();

}

#endif
