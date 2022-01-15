/**********************************************************************

  Audacity: A Digital Audio Editor

  ShuttleGui.cpp

  James Crook

  Audacity is free software.
  This file is licensed under the wxWidgets license, see License.txt

**********************************************************************//**

\file ShuttleGui.cpp
\brief Implements ShuttleGui, ShuttleGuiBase and InvisiblePanel.

*//***************************************************************//**

\class ShuttleGui
\brief
  Derived from ShuttleGuiBase, an Audacity specific class for shuttling
  data to and from GUI.

  ShuttleGui extends the idea of the data Shuttle class to include creation
  of dialog controls.  As part of this it provides an interface to sizers
  that leads to shorter more readable code.

  It also allows the code that is used to create dialogs to be reused
  to shuttle information in and out.

  Most of the ShuttleGui functions are actually defined in
  ShuttleGuiBase.
     - wxWidgets widgets are dealt with by ShuttleGuiBase.
     - Audacity specific widgets are dealt with by ShuttleGui

  There is documentation on how to use this class in \ref ShuttleSystem

*//***************************************************************//**

\class ShuttleGuiBase
\brief Base class for shuttling data to and from a GUI.

see also ShuttleGui

Use the:
  - \p Start / \p End methods for containers, like two-column-layout.
  - \p Add methods if you are only interested in creating the controls.

The code in this file is fairly repetitive.  We are dealing with
  - Many different types of Widget.
  - Creation / Reading / Writing / Exporting / Importing
  - int, float, string variants (for example of TextCtrl contents).

You would rarely use ShuttleGuiBase directly, instead you'd use
ShuttleGui.

There is DOxygen documentation on how to use the ShuttleGui
class in \ref ShuttleSystem .

*//***************************************************************//**

\class InvisiblePanel
\brief An InvisiblePanel is a panel which does not repaint its
own background.

It is used (a) To group together widgets which need to be refreshed
together.  A single refresh of the panel causes all the subwindows to
refresh.  (b) as a base class for some flicker-free classes for which
the background is never repainted.

JKC: InvisiblePanel will probably be replaced in time by a mechanism
for registering for changes.

*//******************************************************************/



#include "ShuttleGui.h"



#include "MemoryX.h"
#include "Prefs.h"
#include "Theme.h"

#include <wx/setup.h> // for wxUSE_* macros
#include <wx/grid.h>
#include <wx/listctrl.h>
#include <wx/notebook.h>
#include <wx/simplebook.h>
#include <wx/treectrl.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>
#include <wx/bmpbuttn.h>
#include <wx/wrapsizer.h>

#include "ComponentInterface.h"
#include "widgets/ReadOnlyText.h"
#include "widgets/valnum.h"
#include "widgets/NumericTextCtrl.h"
#include "widgets/wxPanelWrapper.h"
#include "widgets/wxTextCtrlWrapper.h"
#include "AllThemeResources.h"

#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/combobox.h>
#include <wx/dcclient.h>
#include <wx/frame.h>
#include <wx/listbox.h>
#include <wx/menu.h>
#include <wx/radiobut.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/valtext.h>

#if wxUSE_ACCESSIBILITY
#include "widgets/WindowAccessible.h"
#endif

PreferenceVisitor::~PreferenceVisitor() = default;

namespace DialogDefinition {

auto ValidationState::ReserveSlot() -> Slot
{
   if ( mNext == nFlags - 1 ) {
      wxASSERT_MSG( false, "Too many controls in one dialog" );
      --mNext;
   }
   return mFlags[ mNext++ ];
}

ChoiceAdaptor::~ChoiceAdaptor() = default;

SingleChoiceAdaptor::~SingleChoiceAdaptor()  = default;

bool SingleChoiceAdaptor::Get( TargetType &target ) const
{
   return Get( target, mChoices.cache );
}

bool SingleChoiceAdaptor::Set( const TargetType &value )
{
   if ( value >= 0 && value < mChoices.cache.size() )
      return Set( value, mChoices.cache[ value ] );
   else
      return Set( value, {} );
}

IntChoiceAdaptor::~IntChoiceAdaptor() = default;

bool IntChoiceAdaptor::Get( int &index, const Identifiers & ) const
{
   index = mIndex;
   return true;
}

bool IntChoiceAdaptor::Set( int index, const Identifier & )
{
   mIndex = index;
   return true;
}

StringChoiceAdaptor::~StringChoiceAdaptor() = default;

bool StringChoiceAdaptor::Get( int &index, const Identifiers &strings ) const
{
   wxString value;
   if ( !(mpAdaptor && mpAdaptor->Get( value ) ) )
      return false;

   auto pStrings = &strings;
   if ( mInternals ) {
      // The stored string should be looked up in internals, not in the
      // user-visible strings that are passed in the argument
      pStrings = &mCachedInternals;

      // Be sure the cache is up-to-date
      if ( auto newStrings = mInternals() )
         mCachedInternals.swap( *newStrings );
   }
   
   const auto begin = pStrings->begin(), end = pStrings->end(),
      iter = std::find( begin, end, value );
   if ( iter == end )
      index = 0;
   else
      index = iter - begin;
   return true;
}

bool StringChoiceAdaptor::Set( int index, const Identifier &str )
{
   if ( mInternals ) {
      // Ignore the passed-in string, lookup index in internals
      Identifier value;
      if ( index >= 0 && index < mCachedInternals.size() )
         value = mCachedInternals[ index ];
      return ( mpAdaptor && mpAdaptor->Set( value.GET() ) );
   }
   else
      return ( mpAdaptor && mpAdaptor->Set( str.GET() ) );
}

MultipleChoiceAdaptor::~MultipleChoiceAdaptor()  = default;

NumberChoiceAdaptor::~NumberChoiceAdaptor() = default;

bool NumberChoiceAdaptor::Get( TargetType &target ) const
{
   if ( auto newValues = mFindValues() )
      mValues.swap( *newValues );

   int value;
   if ( !mpAdaptor->Get( value ) )
      return false;

   if ( mValues.empty() )
      target = value;
   else {
      const auto begin = mValues.begin(), end = mValues.end(),
         iter = std::find( begin, end, value );
      if ( iter != end )
         target = iter - begin;
      else
         // Last value is treated as the special default
         target = mValues.size() - 1;
   }
   return true;
}

bool NumberChoiceAdaptor::Set( const TargetType &value )
{
   if ( auto newValues = mFindValues() )
      mValues.swap( *newValues );

   // Interpret the given value as index into integer values
   if ( value >= 0 && value < mValues.size() )
      return mpAdaptor->Set( mValues[ value ] );
   else
      return mpAdaptor->Set( value );
}

ChoiceSettingAdaptor::~ChoiceSettingAdaptor() = default;

bool ChoiceSettingAdaptor::Get( TargetType &target ) const
{
   // to do: error handling
   target = mSetting.ReadIndex();
   return true;
}

bool ChoiceSettingAdaptor::Set( const TargetType &value )
{
   return mSetting.WriteIndex( value ) && gPrefs->Flush();
}

LabelSettingAdaptor::~LabelSettingAdaptor() = default;

bool LabelSettingAdaptor::Get( TargetType &target ) const
{
   // to do: error handling
   target = mSetting.ReadIndex();
   return true;
}

bool LabelSettingAdaptor::Set( const TargetType &value )
{
   return mSetting.WriteIndex( value ) && gPrefs->Flush();
}

BoolValidator::BoolValidator(
   const std::shared_ptr< ValidationState > &pValidationState,
   const std::shared_ptr< AdaptorType > &pAdaptor )
   : wxGenericValidator{ &mTemp }
   , AdaptingValidatorBase<bool>{ pValidationState, pAdaptor }
{}

BoolValidator::BoolValidator( const BoolValidator &other )
   // Make a "deep copy" so that the base class refers to its own temporary
   // in mTemp, not to other.mTemp
   : wxGenericValidator{ &mTemp }
   , AdaptingValidatorBase<bool>{ other }
{}

BoolValidator::~BoolValidator() = default;

wxObject *BoolValidator::Clone() const
{
   return safenew BoolValidator{ *this };
}

bool BoolValidator::Validate( wxWindow *pWindow )
{
   return mpAdaptor->Get( mTemp ) &&
      wxGenericValidator::Validate( pWindow );
}

bool BoolValidator::TransferFromWindow()
{
   return mSlot = wxGenericValidator::TransferFromWindow() &&
      mpAdaptor->Set( mTemp );
}

bool BoolValidator::TransferToWindow()
{
   mSlot = true;
   return mpAdaptor->Get( mTemp ) &&
      wxGenericValidator::TransferToWindow();
}
   
namespace {
void RepopulateChoices( wxItemContainer *pCtrl, ComputedChoices &adaptor )
{
   if ( adaptor.GetChoices ) {
      auto strings = adaptor.GetChoices();
      if ( strings ) {
         // Repopulation of the choices
         pCtrl->Clear();
         for ( const auto &ident : *strings )
            pCtrl->Append( ident.GET() );
         adaptor.cache.swap( *strings );
      }
   }
}

template< typename Adaptor >
void RepopulateChoices( wxItemContainer *pCtrl, Adaptor &adaptor )
{
   if ( auto pChoiceAdaptor = dynamic_cast<ChoiceAdaptor*>( &adaptor ) )
      RepopulateChoices( pCtrl, pChoiceAdaptor->mChoices );
}
}

IntValidator::IntValidator(
   const std::shared_ptr< ValidationState > &pValidationState,
   const std::shared_ptr< AdaptorType > &pAdaptor,
   NumValidatorStyle style, int min, int max)
   : AdaptingValidatorBase< int >{ pValidationState, pAdaptor }
   , mDelegate{ std::make_unique< IntegerValidator< int > >(
      &mTemp, style, min, max ) }
{}

IntValidator::IntValidator( const IntValidator &other )
   : AdaptingValidatorBase< int >{ other }
{
   // Make a "deep copy" so that the base class refers to its own temporary
   // in mTemp, not to other.mTemp
   auto otherDelegate =
      static_cast< IntegerValidator< int >* >( other.mDelegate.get() );
   mDelegate = std::make_unique< IntegerValidator< int > >(
      &mTemp, otherDelegate->GetStyle(),
      otherDelegate->GetMin(), otherDelegate->GetMax() );
}

IntValidator::~IntValidator() = default;

wxObject *IntValidator::Clone() const
{
   return safenew IntValidator{ *this };
}

bool IntValidator::TryBefore(wxEvent& event)
{
   return mDelegate->ProcessEventLocally( event );
}

bool IntValidator::Validate( wxWindow *pWindow )
{
   if ( dynamic_cast<wxTextCtrl*>( pWindow ) ||
        dynamic_cast<wxComboBox*>( pWindow ) ) {
      return mpAdaptor->Get( mTemp ) &&
         mDelegate->Validate( pWindow );
   }
   return true;
}

bool IntValidator::TransferFromWindow()
{
   auto pWindow = GetWindow();
   if ( dynamic_cast<wxTextCtrl*>( pWindow ) ||
        dynamic_cast<wxComboBox*>( pWindow ) )
      return mSlot = mDelegate->TransferFromWindow() &&
         mpAdaptor->Set( mTemp );
   else if ( auto pCtrl = dynamic_cast<wxItemContainer*>( pWindow ) )
      // This case covers wxListBox and wxChoice
      return mSlot = mpAdaptor->Set( pCtrl->GetSelection() );
   else if ( auto pCtrl = dynamic_cast<wxSlider*>( pWindow ) )
      return mSlot = mpAdaptor->Set( pCtrl->GetValue() );
   else if ( auto pCtrl = dynamic_cast<wxSpinCtrl*>( pWindow ) )
      return mSlot = mpAdaptor->Set( pCtrl->GetValue() );
   else if ( auto pCtrl = dynamic_cast<wxRadioButton*>( pWindow ) ) {
      if ( !pCtrl->GetValue() )
         // Do the real work only at the chosen button of the group
         return mSlot = true;
      else if ( mRadioButtons ) {
         auto begin = mRadioButtons->begin(), end = mRadioButtons->end(),
            iter = std::find( begin, end, pWindow );
         auto value = (iter == end) ? -1 : (iter - begin);
         return mSlot = mpAdaptor->Set( value );
      }
   }
   else if ( auto pCtrl = dynamic_cast<wxBookCtrlBase*>( pWindow ) )
      return mSlot = mpAdaptor->Set( pCtrl->GetSelection() );
   return mSlot = false;
}

bool IntValidator::TransferToWindow()
{
   mSlot = true;
   auto pWindow = GetWindow();
   if ( auto pCtrl = dynamic_cast<wxItemContainer*>( GetWindow() ) ) {
      // This case covers wxListBox, wxChoice, wxComboBox
      RepopulateChoices( pCtrl, *mpAdaptor );
      if ( !mpAdaptor->Get( mTemp ) )
         return false;
      if ( dynamic_cast<wxComboBox*>( pWindow ) )
         return mDelegate->TransferToWindow();
      else
         return ( pCtrl->SetSelection( mTemp ), true );
   }
   else if ( dynamic_cast<wxTextCtrl*>( pWindow ) ||
        dynamic_cast<wxComboBox*>( pWindow ) )
      return mpAdaptor->Get( mTemp ) &&
         mDelegate->TransferToWindow();
   else if ( auto pCtrl = dynamic_cast<wxSlider*>( GetWindow() ) )
      return mpAdaptor->Get( mTemp ) &&
         ( pCtrl->SetValue( mTemp ), true );
   else if ( auto pCtrl = dynamic_cast<wxSpinCtrl*>( GetWindow() ) )
      return mpAdaptor->Get( mTemp ) &&
         ( pCtrl->SetValue( mTemp ), true );
   else if ( auto pCtrl = dynamic_cast<wxRadioButton*>( pWindow ) ) {
      if ( !mpAdaptor->Get( mTemp ) || !mRadioButtons )
         return false;
      auto begin = mRadioButtons->begin(), end = mRadioButtons->end(),
         iter = std::find( begin, end, pWindow );
      bool value = (iter == end) ? false : ( mTemp == (iter - begin) );
      return mpAdaptor->Set( value );
   }
   else if ( auto pCtrl = dynamic_cast<wxBookCtrlBase*>( GetWindow() ) )
      return mpAdaptor->Get( mTemp ) &&
         ( pCtrl->SetSelection( mTemp ), true );
   return false;
}

IntVectorValidator::IntVectorValidator(
   const std::shared_ptr< ValidationState > &pValidationState,
   const std::shared_ptr< AdaptorType > &pAdaptor )
   : wxValidator{}
   , AdaptingValidatorBase< std::vector<int> >{ pValidationState, pAdaptor }
{}

IntVectorValidator::IntVectorValidator( const IntVectorValidator& ) = default;

IntVectorValidator::~IntVectorValidator() = default;

wxObject *IntVectorValidator::Clone() const
{
   return safenew IntVectorValidator{ *this };
}

bool IntVectorValidator::Validate( wxWindow *pWindow )
{
   return true;
}

bool IntVectorValidator::TransferFromWindow()
{
   if ( auto pCtrl = dynamic_cast<wxListBox*>( GetWindow() ) ) {
      wxArrayInt selections;
      pCtrl->GetSelections( selections );
      mTemp = { selections.begin(), selections.end() };
      return mSlot = mpAdaptor->Set( mTemp );
   }
   return mSlot = false;
}

bool IntVectorValidator::TransferToWindow()
{
   if ( auto pCtrl = dynamic_cast<wxListBox*>( GetWindow() ) ) {
      RepopulateChoices( pCtrl, *mpAdaptor );
      if ( !mpAdaptor->Get( mTemp ) )
         return false;
      pCtrl->SetSelection( -1 );
      for ( auto ii : mTemp )
         pCtrl->Select( ii );
      return true;
   }
   return false;
}

DoubleValidator::DoubleValidator(
   const std::shared_ptr< ValidationState > &pValidationState,
   const std::shared_ptr< AdaptorType > &pAdaptor,
   NumValidatorStyle style, int precision, double min, double max )
   : AdaptingValidatorBase<double>{ pValidationState, pAdaptor }
   , mDelegate{ std::make_unique< FloatingPointValidator< double > >(
      precision, &mTemp, style, min, max ) }
{}

DoubleValidator::DoubleValidator( const DoubleValidator &other )
   : AdaptingValidatorBase<double>{ other }
   , mExactValue{ other.mExactValue }
{
   // Make a "deep copy" so that the base class refers to its own temporary
   // in mTemp, not to other.mTemp
   auto otherDelegate =
      static_cast< FloatingPointValidator< double >* >( other.mDelegate.get() );
   mDelegate = std::make_unique< FloatingPointValidator< double > >(
      otherDelegate->GetPrecision(), &mTemp, otherDelegate->GetStyle(),
      otherDelegate->GetMin(), otherDelegate->GetMax() );
}

DoubleValidator::~DoubleValidator() = default;

wxObject *DoubleValidator::Clone() const
{
   return safenew DoubleValidator{ *this };
}

bool DoubleValidator::TryBefore(wxEvent& event)
{
   return mDelegate->ProcessEventLocally( event );
}

bool DoubleValidator::Validate( wxWindow *pWindow )
{
   if ( ( dynamic_cast<wxTextCtrl*>( pWindow ) ||
          dynamic_cast<wxComboBox*>( pWindow ) ) )
      return mpAdaptor->Get( mTemp ) &&
         mDelegate->Validate( pWindow );
   return true;
}

bool DoubleValidator::TransferFromWindow()
{
   auto pWindow = GetWindow();
   if ( ( dynamic_cast<wxTextCtrl*>( pWindow ) ||
          dynamic_cast<wxComboBox*>( pWindow ) ) ) {
      auto pDelegate =
         static_cast< FloatingPointValidator< double >* >( mDelegate.get() );
      if ( !pDelegate->TransferFromWindow() )
         return mSlot = false;
      if ( pDelegate->NormalizeValue( mExactValue ) ==
             pDelegate->GetTextEntry()->GetValue() )
         // Window hasn't changed since we transferred to it
         // Use the last stored value rather than suffer precision loss
         // converting back from text
         return mSlot = mpAdaptor->Set( mExactValue );
      else
         return mSlot = mpAdaptor->Set( mTemp );
   }
   else if ( auto pCtrl = dynamic_cast<wxSlider*>( GetWindow() ) )
      return mSlot = mpAdaptor->Set( pCtrl->GetValue() );
   else if ( auto pCtrl = dynamic_cast<NumericTextCtrl*>( GetWindow() ) )
      return mSlot = mpAdaptor->Set( pCtrl->GetValue() );
   return mSlot = false;
}

bool DoubleValidator::TransferToWindow()
{
   mSlot = true;
   auto pWindow = GetWindow();
   if ( ( dynamic_cast<wxTextCtrl*>( pWindow ) ||
          dynamic_cast<wxComboBox*>( pWindow ) ) ) {
      if ( !mpAdaptor->Get( mTemp ) )
         return false;
      mExactValue = mTemp;
      return mDelegate->TransferToWindow();
   }
   else if ( auto pCtrl = dynamic_cast<wxSlider*>( GetWindow() ) ) {
      if ( !mpAdaptor->Get( mTemp ) )
         return false;
      pCtrl->SetValue( mTemp );
      return true;
   }
   else if ( auto pCtrl = dynamic_cast<NumericTextCtrl*>( GetWindow() ) ) {
      if ( !mpAdaptor->Get( mTemp ) )
         return false;
      pCtrl->SetValue( mTemp );
      return true;
   }
   return false;
}

StringValidator::StringValidator(
   const std::shared_ptr< ValidationState > &pValidationState,
   const std::shared_ptr< AdaptorType > &pAdaptor,
   const Options &options )
   // Make a "deep copy" so that the base class refers to its own temporary
   // in mTemp, not to other.mTemp
   : wxTextValidator{ wxFILTER_NONE, &mTemp }
   , AdaptingValidatorBase<wxString>{ pValidationState, pAdaptor }
   , mOptions{ options }
{
   ApplyOptions();
}

StringValidator::StringValidator( const StringValidator &other )
   : wxTextValidator{ wxFILTER_NONE, &mTemp }
   , AdaptingValidatorBase<wxString>{ other }
   , mOptions{ other.mOptions }
{
   ApplyOptions();
}

StringValidator::~StringValidator() = default;

void StringValidator::ApplyOptions()
{
   if ( !mOptions.allowed.empty() ) {
      wxTextValidator::SetStyle( wxFILTER_INCLUDE_CHAR_LIST );
      wxArrayString strings;
      for ( wxChar character : mOptions.allowed )
         strings.push_back( wxString{ character } );
      wxTextValidator::SetIncludes( strings );
   }
   else if ( mOptions.numeric )
      wxTextValidator::SetStyle( wxFILTER_NUMERIC );
}

wxObject *StringValidator::Clone() const
{
   return safenew StringValidator{ *this };
}

bool StringValidator::Validate( wxWindow *pWindow )
{
   return mpAdaptor->Get( mTemp ) &&
      wxTextValidator::Validate( pWindow );
}

bool StringValidator::TransferFromWindow()
{
   // Not intended to inherit the behavior of wxGenericValidator for choice
   // controls!
   if ( dynamic_cast< wxChoice * >( GetWindow() ) )
      return mSlot = false;

   auto result = wxTextValidator::TransferFromWindow();
   return mSlot = mpAdaptor->Set( mTemp );
}

bool StringValidator::TransferToWindow()
{
   mSlot = true;

   // Not intended to inherit the behavior of wxGenericValidator for choice
   // controls!
   if ( dynamic_cast< wxChoice * >( GetWindow() ) )
      return false;

   return mpAdaptor->Get( mTemp ) &&
      wxTextValidator::TransferToWindow();
}

} // namespace DialogDefinition

namespace {

// A validator intended to have a no-fail side-effect on a control's
// appearance, only when transferring to window; it can add that to the
// behavior of some other validator.
class ValidatorDecorator : public wxValidator
{
public:
   using Updater = std::function< void( wxWindow* ) >;

   ValidatorDecorator( const Updater &updater,
      std::unique_ptr< wxValidator > pNext )
      : mUpdater{ updater }
      , mpNext{ std::move( pNext ) }
   {}
   ValidatorDecorator( const ValidatorDecorator & );
   ~ValidatorDecorator() ;

   wxObject *Clone() const override;
   bool Validate( wxWindow *pWindow ) override;
   bool TransferFromWindow() override;
   bool TransferToWindow() override;

private:
   Updater mUpdater;
   std::unique_ptr< wxValidator > mpNext;
};

ValidatorDecorator::ValidatorDecorator( const ValidatorDecorator &other )
   : mUpdater{ other.mUpdater }
{
   std::unique_ptr< wxObject > pNext{ other.mpNext->Clone() };
   // why does wxValidator::Clone() return wxObject* not wxValidator* ?
   // Let's be defensive.
   if ( dynamic_cast< wxValidator* >( pNext.get() ) )
      mpNext.reset( static_cast< wxValidator *>( pNext.release() ) );
   else if ( pNext )
      wxASSERT( false );
}

ValidatorDecorator::~ValidatorDecorator() = default;

wxObject *ValidatorDecorator::Clone() const
{
   return safenew ValidatorDecorator{ *this };
}

bool ValidatorDecorator::Validate( wxWindow *pWindow )
{
   return !mpNext || mpNext->Validate( pWindow );
}

bool ValidatorDecorator::TransferFromWindow()
{
   return !mpNext || mpNext->TransferFromWindow();
}

bool ValidatorDecorator::TransferToWindow()
{
   if ( mUpdater )
      mUpdater( GetWindow() );
   return !mpNext || mpNext->TransferToWindow();
}

}

ShuttleGuiState::ShuttleGuiState(
   wxWindow *pDlg, bool vertical, wxSize minSize,
   const std::shared_ptr< PreferenceVisitor > &pVisitor )
   : mpDlg{ pDlg }
   , mpVisitor{ pVisitor }
   , mpValidationState{ std::make_shared<DialogDefinition::ValidationState>() }
{
   wxASSERT( (pDlg != NULL ) );

   mpSizer = mpParent->GetSizer();

#if 0
   if( mpSizer == NULL )
   {
      wxWindow * pGrandParent = mpParent->GetParent();
      if( pGrandParent )
      {
         mpSizer = pGrandParent->GetSizer();
      }
   }
#endif

   if( !mpSizer )
   {
      mpParent->SetSizer(
         mpSizer = safenew wxBoxSizer(vertical ? wxVERTICAL : wxHORIZONTAL));
   }
   PushSizer();
   mpSizer->SetMinSize(minSize);
}

ShuttleGuiBase::ShuttleGuiBase(
   wxWindow * pParent, bool vertical, wxSize minSize,
   const std::shared_ptr< PreferenceVisitor > &pVisitor )
{
   mpState = std::make_shared< ShuttleGuiState >(
      pParent, vertical, minSize, pVisitor );
}

ShuttleGuiBase::ShuttleGuiBase(
   const std::shared_ptr< ShuttleGuiState > &pState )
      : mpState{ pState }
{
}

ShuttleGuiBase::ShuttleGuiBase( ShuttleGuiBase&& ) = default;

ShuttleGuiBase::~ShuttleGuiBase()
{
}

void ShuttleGuiBase::ResetId()
{
   miIdSetByUser = -1;
   miId = -1;
   mpState -> miIdNext = 3000;
}


int ShuttleGuiBase::GetBorder() const noexcept
{
   return mpState -> miBorder;
}

//---- Add Functions.

/// Right aligned text string.
void ShuttleGuiBase::AddPrompt(const TranslatableLabel &Prompt, int wrapWidth)
{
   //wxLogDebug( "Prompt: [%s] Id:%i (%i)", Prompt.c_str(), miId, miIdSetByUser ) ;
   if( Prompt.empty() )
      return;
   miProp=1;
   const auto translated = Prompt.Translation();
   auto text = safenew wxStaticText(GetParent(), -1, translated, wxDefaultPosition, wxDefaultSize,
      GetStyle( wxALIGN_RIGHT ));
   mpWind = text;
   if (wrapWidth > 0)
      text->Wrap(wrapWidth);
   mpWind->SetName(wxStripMenuCodes(translated)); // fix for bug 577 (NVDA/Narrator screen readers do not read static text in dialogs)
   UpdateSizersCore( false, wxALL | wxALIGN_CENTRE_VERTICAL, true );
}

/// Left aligned text string.
void ShuttleGuiBase::AddUnits(const TranslatableString &Units, int wrapWidth)
{
   if( Units.empty() )
      return;
   miProp = 1;
   const auto translated = Units.Translation();
   auto text = safenew wxStaticText(GetParent(), -1, translated, wxDefaultPosition, mItem.mWindowSize,
      GetStyle( wxALIGN_LEFT ));
   mpWind = text;
   if (wrapWidth > 0)
      text->Wrap(wrapWidth);
   mpWind->SetName(translated); // fix for bug 577 (NVDA/Narrator screen readers do not read static text in dialogs)
   UpdateSizersCore( false, wxALL | wxALIGN_CENTRE_VERTICAL );
}

/// Centred text string.
void ShuttleGuiBase::AddTitle(const TranslatableString &Title, int wrapWidth)
{
   if( Title.empty() )
      return;
   const auto translated = Title.Translation();
   auto text = safenew wxStaticText(GetParent(), -1, translated, wxDefaultPosition, mItem.mWindowSize,
      GetStyle( wxALIGN_CENTRE ));
   mpWind = text;
   if (wrapWidth > 0)
      text->Wrap(wrapWidth);
   mpWind->SetName(translated); // fix for bug 577 (NVDA/Narrator screen readers do not read static text in dialogs)
   UpdateSizers();
}

/// Very generic 'Add' function.  We can add anything we like.
/// Useful for unique controls
wxWindow* ShuttleGuiBase::AddWindow(wxWindow* pWindow, int PositionFlags)
{
   mpWind = pWindow;
   SetProportions( 0 );
   UpdateSizersCore(false, PositionFlags | wxALL);
   return pWindow;
}

void ShuttleGuiBase::AddNumericTextCtrl(NumericConverter::Type type,
      const NumericFormatSymbol &formatName,
      double value,
      double sampleRate,
      const NumericTextCtrl::Options &options,
      const wxPoint &pos,
      const wxSize &size )
{
   UseUpId();
   NumericTextCtrl * pCtrl = nullptr;
   mpWind = pCtrl = safenew NumericTextCtrl( GetParent(), miId,
      type, formatName, value, sampleRate, options, pos, size );
   CheckEventType( mItem, {wxEVT_TEXT} );
   UpdateSizers();
}

void ShuttleGuiBase::AddCheckBox( const TranslatableLabel &Prompt, bool Selected)
{
   auto realPrompt = Prompt.Translation();

   UseUpId();
   wxCheckBox * pCheckBox;
   miProp=0;
   mpWind = pCheckBox = safenew wxCheckBox(GetParent(), miId, realPrompt, wxDefaultPosition, mItem.mWindowSize,
      GetStyle( 0 ));
   CheckEventType( mItem, {wxEVT_CHECKBOX} );
   pCheckBox->SetValue(Selected);
   if (realPrompt.empty()) {
      // NVDA 2018.3 does not read controls which are buttons, check boxes or radio buttons which have
      // an accessibility name which is empty. Bug 1980.
#if wxUSE_ACCESSIBILITY
      // so that name can be set on a standard control
      pCheckBox->SetAccessible(safenew WindowAccessible(pCheckBox));
#endif
      pCheckBox->SetName(L"\a");      // non-empty string which screen readers do not read
   }
   UpdateSizers();
}

/// For a consistent two-column layout we want labels on the left and
/// controls on the right.  CheckBoxes break that rule, so we fake it by
/// placing a static text label and then a tick box with an empty label.
void ShuttleGuiBase::AddCheckBoxOnRight( const TranslatableLabel &Prompt, bool Selected)
{
   AddPrompt( Prompt );
   UseUpId();
   wxCheckBox * pCheckBox;
   miProp=0;
   mpWind = pCheckBox = safenew wxCheckBox(GetParent(), miId, L"", wxDefaultPosition, mItem.mWindowSize,
      GetStyle( 0 ));
   pCheckBox->SetValue(Selected);
   pCheckBox->SetName(Prompt.Stripped().Translation());
   UpdateSizers();
}

void ShuttleGuiBase::AddButton(
   const TranslatableLabel &Label, int PositionFlags, bool setDefault)
{
   UseUpId();
   wxButton * pBtn;
   const auto translated = Label.Translation();
   mpWind = pBtn = safenew wxButton(GetParent(), miId,
      translated, wxDefaultPosition, mItem.mWindowSize,
      GetStyle( 0 ) );
   mpWind->SetName(wxStripMenuCodes(translated));
   CheckEventType( mItem, {wxEVT_BUTTON} );
   miProp=0;
   UpdateSizersCore(false, PositionFlags | wxALL);
   if (setDefault)
      pBtn->SetDefault();
}

void ShuttleGuiBase::AddBitmapButton(
   const wxBitmap &Bitmap, int PositionFlags, bool setDefault)
{
   UseUpId();
   wxBitmapButton * pBtn;
   mpWind = pBtn = safenew wxBitmapButton(GetParent(), miId, Bitmap,
      wxDefaultPosition, mItem.mWindowSize, GetStyle( wxBU_AUTODRAW ) );
   pBtn->SetBackgroundColour(
      wxColour( 246,246,243));
//      wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
   miProp=0;
   UpdateSizersCore(false, PositionFlags | wxALL);
   if (setDefault)
      pBtn->SetDefault();
}

void ShuttleGuiBase::AddChoice( const TranslatableLabel &Prompt,
   const TranslatableStrings &choices, int Selected )
{
   AddPrompt( Prompt );
   UseUpId();
   wxChoice * pChoice;
   miProp=0;

   mpWind = pChoice = safenew wxChoice(
      GetParent(),
      miId,
      wxDefaultPosition,
      mItem.mWindowSize,
      transform_container<wxArrayString>(
         choices, std::mem_fn( &TranslatableString::Translation ) ),
      GetStyle( 0 ) );
   CheckEventType( mItem, {wxEVT_CHOICE} );

   pChoice->SetMinSize( { 180, -1 } );// Use -1 for 'default size' - Platform specific.
#ifdef __WXMAC__
#if wxUSE_ACCESSIBILITY
   // so that name can be set on a standard control
   mpWind->SetAccessible(safenew WindowAccessible(mpWind));
#endif
#endif
   pChoice->SetName(Prompt.Stripped().Translation());
   if ( Selected >= 0 && Selected < (int)choices.size() )
      pChoice->SetSelection( Selected );

   UpdateSizers();
   SetMinSize(pChoice, choices);
}

void ShuttleGuiBase::AddChoice( const TranslatableLabel &Prompt,
   const TranslatableStrings &choices, const TranslatableString &Selected )
{
   AddChoice(
      Prompt, choices, make_iterator_range( choices ).index( Selected ) );
}

void ShuttleGuiBase::AddFixedText(
   const TranslatableString &Str, bool bCenter, int wrapWidth)
{
   const auto translated = Str.Translation();
   UseUpId();
   auto text = safenew wxStaticText(GetParent(),
      miId, translated, wxDefaultPosition, mItem.mWindowSize,
      GetStyle( wxALIGN_LEFT ));
   mpWind = text;
   if ( wrapWidth > 0 )
      text->Wrap( wrapWidth );
   mpWind->SetName(wxStripMenuCodes(translated)); // fix for bug 577 (NVDA/Narrator screen readers do not read static text in dialogs)
   if( bCenter )
   {
      miProp=1;
      UpdateSizersC();
   }
   else
      UpdateSizers();
}

void ShuttleGuiBase::AddVariableText(
   const TranslatableString &Str,
   bool bCenter, int PositionFlags, int wrapWidth )
{
   const auto translated = Str.Translation();
   UseUpId();

   wxStaticText *pStatic;
   auto text = pStatic = safenew wxStaticText(GetParent(), miId, translated,
      wxDefaultPosition, mItem.mWindowSize,
      GetStyle( wxALIGN_LEFT ));
   mpWind = text;
   if ( wrapWidth > 0 )
      text->Wrap( wrapWidth );
   mpWind->SetName(wxStripMenuCodes(translated)); // fix for bug 577 (NVDA/Narrator screen readers do not read static text in dialogs)
   if( bCenter )
   {
      miProp=1;
      if( PositionFlags )
         UpdateSizersCore( false, PositionFlags );
      else
         UpdateSizersC();
   }
   else
      if( PositionFlags )
         UpdateSizersCore( false, PositionFlags );
      else
         UpdateSizers();
}

void ShuttleGuiBase::AddReadOnlyText(
   const TranslatableLabel &Caption, const wxString &Value)
{
   const auto translated = Caption.Translation();
   auto style = GetStyle( wxBORDER_NONE );
   mItem.miStyle = wxALIGN_CENTER_VERTICAL;
   AddPrompt( Caption );
   UseUpId();
   ReadOnlyText * pReadOnlyText;
   miProp=0;

   mpWind = pReadOnlyText = safenew ReadOnlyText(GetParent(), miId, Value,
      wxDefaultPosition, wxDefaultSize, GetStyle( style ));
   mpWind->SetName(wxStripMenuCodes(translated));
   UpdateSizers();
}

void ShuttleGuiBase::AddCombo(
   const TranslatableLabel &Prompt,
   const wxString &Selected, const wxArrayStringEx & choices )
{
   const auto translated = Prompt.Translation();
   AddPrompt( Prompt );
   UseUpId();
   wxComboBox * pCombo;
   miProp=0;

   int n = choices.size();
   if( n>50 ) n=50;
   int i;
   wxString Choices[50];
   for(i=0;i<n;i++)
   {
      Choices[i] = choices[i];
   }

   mpWind = pCombo = safenew wxComboBox(GetParent(), miId, Selected, wxDefaultPosition, mItem.mWindowSize,
      n, Choices, GetStyle( 0 ));
   mpWind->SetName(wxStripMenuCodes(translated));

   UpdateSizers();
}


void ShuttleGuiBase::DoAddRadioButton(
   const TranslatableLabel &Prompt, int style)
{
   const auto translated = Prompt.Translation();
   UseUpId();
   wxRadioButton * pRad;
   mpWind = pRad = safenew wxRadioButton(GetParent(), miId, translated,
      wxDefaultPosition, mItem.mWindowSize, GetStyle( style ) );
   mpWind->SetName(wxStripMenuCodes(translated));
   if ( style )
      pRad->SetValue( true );
   UpdateSizers();
   pRad->SetValue( style != 0 );

   if ( auto pList = mpState -> mRadioButtons.get() )
      pList->push_back( pRad );
}

void ShuttleGuiBase::AddRadioButton(
   const TranslatableLabel &Prompt )
{
   wxASSERT( mRadioCount >= 0); // Did you remember to use StartRadioButtonGroup() ?
   DoAddRadioButton( Prompt, (mRadioCount++ == 0) ? wxRB_GROUP : 0 );
   mItem = mRadioItem;
}

#ifdef __WXMAC__
void wxSliderWrapper::SetFocus()
{
   // bypassing the override in wxCompositeWindow<wxSliderBase> which ends up
   // doing nothing
   return wxSliderBase::SetFocus();
}
#endif

void ShuttleGuiBase::AddSlider(
   const TranslatableLabel &Prompt, int pos, int Max, int Min,
   int lineSize, int pageSize )
{
   AddPrompt( Prompt );
   UseUpId();
   wxSlider * pSlider;
   mpWind = pSlider = safenew wxSliderWrapper(GetParent(), miId,
      pos, Min, Max,
      wxDefaultPosition,
      // Bug2289:  On Linux at least, sliders like to be constructed with the
      // proper size, not reassigned size later
      mItem.mWindowSize,
      GetStyle( wxSL_HORIZONTAL | wxSL_LABELS | wxSL_AUTOTICKS )
      );
   CheckEventType( mItem, {wxEVT_SLIDER} );
#if wxUSE_ACCESSIBILITY
   // so that name can be set on a standard control
   mpWind->SetAccessible(safenew WindowAccessible(mpWind));
#endif
   mpWind->SetName(wxStripMenuCodes(Prompt.Translation()));
   miProp=1;

   if ( lineSize > 0 )
      pSlider->SetLineSize( lineSize );
   if ( pageSize > 0 )
      pSlider->SetPageSize( pageSize );
   
   UpdateSizers();
}

void ShuttleGuiBase::AddSpinCtrl(
   const TranslatableLabel &Prompt, int Value, int Max, int Min)
{
   const auto translated = Prompt.Translation();
   AddPrompt( Prompt );
   UseUpId();
   wxSpinCtrl * pSpinCtrl;
   mpWind = pSpinCtrl = safenew wxSpinCtrl(GetParent(), miId,
      wxEmptyString,
      wxDefaultPosition, mItem.mWindowSize,
      GetStyle( wxSP_VERTICAL | wxSP_ARROW_KEYS ),
      Min, Max, Value
      );
   mpWind->SetName(wxStripMenuCodes(translated));
   miProp=1;
   UpdateSizers();
}

void ShuttleGuiBase::AddTextBox(
   const TranslatableLabel &Prompt, const wxString &Value, const int nChars)
{
   const auto translated = Prompt.Translation();
   AddPrompt( Prompt );
   UseUpId();
   wxTextCtrl * pTextCtrl;
   wxSize Size(mItem.mWindowSize);
   if( nChars > 0 && Size.GetX() == -1 )
   {
      int width;
      mpState -> mpDlg->GetTextExtent( L"9", &width, nullptr );
      Size.SetWidth( nChars * width );
   }
   miProp=0;

#ifdef EXPERIMENTAL_RIGHT_ALIGNED_TEXTBOXES
   long flags = wxTE_RIGHT;
#else
   long flags = wxTE_LEFT;
#endif

   mpWind = pTextCtrl = safenew wxTextCtrlWrapper(GetParent(), miId, Value,
      wxDefaultPosition, Size, GetStyle( flags ));
   CheckEventType( mItem, {wxEVT_TEXT} );
#if wxUSE_ACCESSIBILITY
   // so that name can be set on a standard control
   mpWind->SetAccessible(safenew WindowAccessible(mpWind));
#endif
   mpWind->SetName(wxStripMenuCodes(translated));
   UpdateSizers();
}

void ShuttleGuiBase::AddNumericTextBox(
   const TranslatableLabel &Prompt, const wxString &Value, const int nChars)
{
   const auto translated = Prompt.Translation();
   AddPrompt( Prompt );
   UseUpId();
   wxTextCtrl * pTextCtrl;
   wxSize Size(mItem.mWindowSize);
   if( nChars > 0 && Size.GetX() == -1 )
   {
      Size.SetWidth( nChars *5 );
   }
   miProp=0;

#ifdef EXPERIMENTAL_RIGHT_ALIGNED_TEXTBOXES
   long flags = wxTE_RIGHT;
#else
   long flags = wxTE_LEFT;
#endif

   wxTextValidator Validator(wxFILTER_NUMERIC);
   mpWind = pTextCtrl = safenew wxTextCtrl(GetParent(), miId, Value,
      wxDefaultPosition, Size, GetStyle( flags ),
      Validator // It's OK to pass this.  It will be cloned.
      );
#if wxUSE_ACCESSIBILITY
   // so that name can be set on a standard control
   mpWind->SetAccessible(safenew WindowAccessible(mpWind));
#endif
   mpWind->SetName(wxStripMenuCodes(translated));
   UpdateSizers();
}

/// Multiline text box that grows.
void ShuttleGuiBase::AddTextWindow(const wxString &Value)
{
   UseUpId();
   wxTextCtrl * pTextCtrl;
   SetProportions( 1 );
   mpWind = pTextCtrl = safenew wxTextCtrl(GetParent(), miId, Value,
      wxDefaultPosition, mItem.mWindowSize, GetStyle( wxTE_MULTILINE ));
#if wxUSE_ACCESSIBILITY
   // so that name can be set on a standard control
   mpWind->SetAccessible(safenew WindowAccessible(mpWind));
#endif
   UpdateSizers();
   // Start off at start of window...
   pTextCtrl->SetInsertionPoint( 0 );
   pTextCtrl->ShowPosition( 0 );
}

/// Single line text box of fixed size.
void ShuttleGuiBase::AddConstTextBox(
   const TranslatableString &Prompt, const TranslatableString &Value)
{
   TranslatableLabel label{ Prompt };
   AddPrompt( label );
   UseUpId();
   miProp=0;
   UpdateSizers();
   miProp=0;
   const auto translatedValue = Value.Translation();
   mpWind = safenew wxStaticText(GetParent(), miId,
      translatedValue, wxDefaultPosition, mItem.mWindowSize,
      GetStyle( 0 ));
   mpWind->SetName(translatedValue); // fix for bug 577 (NVDA/Narrator screen readers do not read static text in dialogs)
   UpdateSizers();
}

void ShuttleGuiBase::AddListBox(const wxArrayStringEx &choices)
{
   UseUpId();
   wxListBox * pListBox;
   SetProportions( 1 );
   mpWind = pListBox = safenew wxListBox(GetParent(), miId,
      wxDefaultPosition, mItem.mWindowSize, choices, GetStyle(0));
   pListBox->SetMinSize( wxSize( 120,150 ));
   CheckEventType( mItem, {wxEVT_LISTBOX, wxEVT_LISTBOX_DCLICK} );
   UpdateSizers();
}


void ShuttleGuiBase::AddGrid()
{
   UseUpId();
   wxGrid * pGrid;
   SetProportions( 1 );
   mpWind = pGrid = safenew wxGrid(GetParent(), miId, wxDefaultPosition,
      mItem.mWindowSize, GetStyle( wxWANTS_CHARS ));
   pGrid->SetMinSize( wxSize( 120, 150 ));
   UpdateSizers();
}

void ShuttleGuiBase::AddListControl(
   std::initializer_list<const ListControlColumn> columns,
   long listControlStyles )
{
   UseUpId();
   wxListCtrl * pListCtrl;
   SetProportions( 1 );
   mpWind = pListCtrl = safenew wxListCtrl(GetParent(), miId,
      wxDefaultPosition, mItem.mWindowSize, GetStyle( wxLC_ICON ));
   pListCtrl->SetMinSize( wxSize( 120,150 ));
   UpdateSizers();

   DoInsertListColumns( pListCtrl, listControlStyles, columns );
}

void ShuttleGuiBase::AddListControlReportMode(
   std::initializer_list<const ListControlColumn> columns,
   long listControlStyles )
{
   UseUpId();
   wxListCtrl * pListCtrl;
   SetProportions( 1 );
   auto Size = mItem.mWindowSize;
   if ( Size == wxDefaultSize )
      Size = wxSize(230,120);//mItem.mWindowSize;
   mpWind = pListCtrl = safenew wxListCtrl(GetParent(), miId,
      wxDefaultPosition,
      Size,
      GetStyle( wxLC_REPORT | wxLC_HRULES | wxLC_VRULES | wxSUNKEN_BORDER ));
//   pListCtrl->SetMinSize( wxSize( 120,150 ));
   UpdateSizers();

   DoInsertListColumns( pListCtrl, listControlStyles, columns );
}

void ShuttleGuiBase::DoInsertListColumns(
   wxListCtrl *pListCtrl,
   long listControlStyles,
   std::initializer_list<const ListControlColumn> columns )
{
   // Old comment from HistoryWindow.cpp follows
   // -- is it still correct for wxWidgets 3?

   // Do this BEFORE inserting the columns.  On the Mac at least, the
   // columns are deleted and later InsertItem()s will cause Audacity to crash.
   for ( auto style = 1l; style <= listControlStyles; style <<= 1 )
      if ( (style & listControlStyles) )
         pListCtrl->SetSingleStyle(style, true);

   long iCol = 0;
   bool dummyColumn =
      columns.size() > 0 && begin(columns)->format == wxLIST_FORMAT_RIGHT;

   //A dummy first column, which is then deleted, is a workaround -
   // under Windows the first column can't be right aligned.
   if (dummyColumn)
      pListCtrl->InsertColumn( iCol++, wxString{} );

   for (auto &column : columns)
      pListCtrl->InsertColumn(
         iCol++, column.heading.Translation(), column.format, column.width );

   if (dummyColumn)
      pListCtrl->DeleteColumn( 0 );
}

void ShuttleGuiBase::AddTree()
{
   UseUpId();
   wxTreeCtrl * pTreeCtrl;
   SetProportions( 1 );
   mpWind = pTreeCtrl = safenew wxTreeCtrl(GetParent(), miId, wxDefaultPosition, mItem.mWindowSize,
      GetStyle( wxTR_HAS_BUTTONS ));
   pTreeCtrl->SetMinSize( wxSize( 120,650 ));
   UpdateSizers();
}

void ShuttleGuiBase::AddIcon(wxBitmap *pBmp)
{
   UseUpId();
   wxBitmapButton * pBtn;
   mpWind = pBtn = safenew wxBitmapButton(GetParent(), miId, *pBmp,
      wxDefaultPosition, mItem.mWindowSize, GetStyle( wxBU_AUTODRAW ) );
   pBtn->SetWindowStyle( wxBORDER_NONE  );
   pBtn->SetCanFocus(false);
   UpdateSizersC();
}

ShuttleGuiBase & ShuttleGuiBase::Prop( int iProp )
{
   miPropSetByUser = iProp;
   return *this;
}

/// Starts a static box around a number of controls.
///  @param Str   The text of the title for the box.
///  @param iProp The resizing proportion value.
/// Use iProp == 0 for a minimum sized static box.
/// Use iProp == 1 for a box that grows if there is space to spare.
wxStaticBox * ShuttleGuiBase::StartStatic(
   const TranslatableString &Str, int iProp, int border)
{
   UseUpId();
   auto translated = Str.Translation();
   wxStaticBox * pBox = safenew wxStaticBoxWrapper(
      GetParent(), miId, translated );
   pBox->SetLabel( translated );
   if (Str.empty()) {
      // NVDA 2018.3 or later does not read the controls in a group box which has
      // an accessibility name which is empty. Bug 2169.
#if wxUSE_ACCESSIBILITY
      // so that name can be set on a standard control
      pBox->SetAccessible(safenew WindowAccessible(pBox));
#endif
      pBox->SetName(L"\a");      // non-empty string which screen readers do not read
   }
   else
      pBox->SetName( wxStripMenuCodes( translated ) );
   mpSubSizer = std::make_unique<wxStaticBoxSizer>(
      pBox,
      wxVERTICAL );
   miSizerProp = iProp;
   UpdateSizers();
   mpState -> mpParent = pBox;
   return pBox;
}

void ShuttleGuiBase::EndStatic()
{
   mpState -> PopSizer();
   mpState -> mpParent = mpState -> mpParent->GetParent();
}

/// This allows subsequent controls and static boxes to be in
/// a scrolled panel.  Very handy if you are running out of space
/// on a dialog.
///
/// The iStyle parameter is used in some very hacky code that
/// dynamically repopulates a dialog.  It also controls the
/// background colour.  Look at the code for details.
///  @param istyle deprecated parameter, but has been used for hacking.
wxScrolledWindow * ShuttleGuiBase::StartScroller(int iStyle, int border)
{
   UseUpId();

   wxScrolledWindow * pScroller;
   mpWind = pScroller = safenew wxScrolledWindow(GetParent(), miId, wxDefaultPosition, mItem.mWindowSize,
      GetStyle( wxSUNKEN_BORDER ) );
   pScroller->SetScrollRate( 20,20 );

   // This fools NVDA into not saying "Panel" when the dialog gets focus
   pScroller->SetName(L"\a");
   pScroller->SetLabel(L"\a");

   SetProportions( 1 );
   if( iStyle==2 )
   {
      UpdateSizersAtStart();
   }
   else
   {
     // mpWind->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_MENUBAR));
      UpdateSizers();  // adds window in to current sizer.
   }

   // create a sizer within the window...
   mpState -> mpParent = pScroller;
   pScroller->SetSizer(mpState -> mpSizer = safenew wxBoxSizer(wxVERTICAL));
   mpState -> PushSizer();
   return pScroller;
}

void ShuttleGuiBase::EndScroller()
{
   wxSize ScrollSize = mpState -> mpSizer->GetMinSize();
   int yMin = ScrollSize.y+4;
   int xMin = ScrollSize.x+4;
   if( yMin > 400)
   {
      yMin = 400;
      xMin+=50;// extra space for vertical scrollbar.
   }

   mpState -> mpParent->SetMinSize( wxSize(xMin, yMin) );

   mpState -> PopSizer();
   auto pScroller = static_cast<wxScrolledWindow*>(mpState -> mpParent);
   mpState -> mpParent = mpState -> mpParent->GetParent();
}

wxPanel * ShuttleGuiBase::StartPanel(int iStyle, int border)
{
   UseUpId();
   wxPanel * pPanel;
   mpWind = pPanel = safenew wxPanelWrapper( GetParent(), miId, wxDefaultPosition, mItem.mWindowSize,
      GetStyle( wxNO_BORDER ));

   if( iStyle != 0 )
   {
      mpWind->SetBackgroundColour(
         iStyle==1
         ? wxColour( 190,200,230) :
         wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW)
         );
   }
   SetProportions(0);
   if (border >= 0)
      mpState -> miBorder = border;
   UpdateSizers();  // adds window in to current sizer.

   // create a sizer within the window...
   mpState -> mpParent = pPanel;
   pPanel->SetSizer(mpState -> mpSizer = safenew wxBoxSizer(wxVERTICAL));
   mpState -> PushSizer();
   return pPanel;
}

void ShuttleGuiBase::EndPanel()
{
   mpState -> PopSizer();
   auto pPanel = static_cast<wxPanel*>(mpState -> mpParent);
   mpState -> mpParent = mpState -> mpParent->GetParent();
}

wxNotebook * ShuttleGuiBase::StartNotebook()
{
   UseUpId();
   wxNotebook * pNotebook;
   mpWind = pNotebook = safenew wxNotebook(GetParent(),
      miId, wxDefaultPosition, mItem.mWindowSize, GetStyle( 0 ));
   SetProportions( 1 );
   UpdateSizers();
   mpState -> mpParent = pNotebook;
   return pNotebook;
}

void ShuttleGuiBase::EndNotebook()
{
   //PopSizer();
   mpState -> mpParent = mpState -> mpParent->GetParent();
}


wxSimplebook * ShuttleGuiBase::StartSimplebook()
{
   UseUpId();
   wxSimplebook * pNotebook;
   mpWind = pNotebook = safenew wxSimplebook(GetParent(),
      miId, wxDefaultPosition, mItem.mWindowSize, GetStyle( 0 ));
   SetProportions( 1 );
   UpdateSizers();
   mpState -> mpParent = pNotebook;
   return pNotebook;
}

void ShuttleGuiBase::EndSimplebook()
{
   //PopSizer();
   mpState -> mpParent = mpState -> mpParent->GetParent();
}


wxNotebookPage * ShuttleGuiBase::StartNotebookPage(
   const TranslatableString & Name, int border )
{
   auto pNotebook = static_cast< wxBookCtrlBase* >( mpState -> mpParent );
   wxNotebookPage * pPage = safenew wxPanelWrapper(GetParent());
   const auto translated = Name.Translation();
   pPage->SetName(translated);

   pNotebook->AddPage(
      pPage,
      translated);

   SetProportions( 1 );
   mpState -> mpParent = pPage;
   pPage->SetSizer(mpState -> mpSizer = safenew wxBoxSizer(wxVERTICAL));
   mpState -> PushSizer();
   //   UpdateSizers();
   return pPage;
}

void ShuttleGuiBase::EndNotebookPage()
{
   mpState -> PopSizer();
   mpState -> mpParent = mpState -> mpParent->GetParent();
}

// Doxygen description is at the start of the file
// this is a wxPanel with erase background disabled.
class InvisiblePanel final : public wxPanelWrapper
{
public:
   InvisiblePanel(
      wxWindow* parent,
      wxWindowID id = -1,
      const wxPoint& pos = wxDefaultPosition,
      const wxSize& size = wxDefaultSize,
      long style = wxTAB_TRAVERSAL ) :
      wxPanelWrapper( parent, id, pos, size, style )
   {
   };
   ~InvisiblePanel(){;};
   void OnPaint( wxPaintEvent &event );
   void OnErase(wxEraseEvent &/*evt*/){;};
   DECLARE_EVENT_TABLE()
};


BEGIN_EVENT_TABLE(InvisiblePanel, wxPanelWrapper)
//   EVT_PAINT(InvisiblePanel::OnPaint)
     EVT_ERASE_BACKGROUND( InvisiblePanel::OnErase)
END_EVENT_TABLE()

void InvisiblePanel::OnPaint( wxPaintEvent & WXUNUSED(event))
{
   // Don't repaint my background.
   wxPaintDC dc(this);
   // event.Skip(); // swallow the paint event.
}

wxPanel * ShuttleGuiBase::StartInvisiblePanel(int border)
{
   UseUpId();
   wxPanel * pPanel;
   mpWind = pPanel = safenew wxPanelWrapper(GetParent(), miId, wxDefaultPosition, mItem.mWindowSize,
      wxNO_BORDER);

   mpWind->SetBackgroundColour(
      wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE)
      );
   SetProportions( 1 );
   if (border >= 0)
      mpState -> miBorder = border;
   UpdateSizers();  // adds window in to current sizer.

   // create a sizer within the window...
   mpState -> mpParent = pPanel;
   pPanel->SetSizer(mpState -> mpSizer = safenew wxBoxSizer(wxVERTICAL));
   mpState -> PushSizer();
   return pPanel;
}

void ShuttleGuiBase::EndInvisiblePanel()
{
   EndPanel();
}


/// Starts a Horizontal Layout.
///  - Use wxEXPAND and 0 to expand horizontally but not vertically.
///  - Use wxEXPAND and 1 to expand horizontally and vertically.
///  - Use wxCENTRE and 1 for no expansion.
/// @param PositionFlag  Typically wxEXPAND or wxALIGN_CENTER.
/// @param iProp         Proportionality for resizing.
void ShuttleGuiBase::StartHorizontalLay(
   int PositionFlags, int iProp, int border )
{
   miSizerProp=iProp;
   mpSubSizer = std::make_unique<wxBoxSizer>( wxHORIZONTAL );
   // PRL:  wxALL has no effect because UpdateSizersCore ignores border
   UpdateSizersCore( false, PositionFlags | wxALL );
}

void ShuttleGuiBase::EndHorizontalLay()
{
   mpState -> PopSizer();
}

void ShuttleGuiBase::StartVerticalLay(int iProp, int border)
{
   miSizerProp=iProp;
   mpSubSizer = std::make_unique<wxBoxSizer>( wxVERTICAL );
   UpdateSizers();
}

void ShuttleGuiBase::StartVerticalLay2(int PositionFlags, int iProp, int border)
{
   miSizerProp=iProp;
   mpSubSizer = std::make_unique<wxBoxSizer>( wxVERTICAL );
   // PRL:  wxALL has no effect because UpdateSizersCore ignores border
   UpdateSizersCore( false, PositionFlags | wxALL );
}

void ShuttleGuiBase::EndVerticalLay()
{
   mpState -> PopSizer();
}

void ShuttleGuiBase::StartWrapLay(int PositionFlags, int iProp, int border)
{
   miSizerProp = iProp;
   mpSubSizer = std::make_unique<wxWrapSizer>(wxHORIZONTAL, 0);

   UpdateSizersCore(false, PositionFlags | wxALL);
}

void ShuttleGuiBase::EndWrapLay()
{
   mpState -> PopSizer();
}

void ShuttleGuiBase::StartMultiColumn( int nCols, const GroupOptions &options)
{
   mpSubSizer = std::make_unique<wxFlexGridSizer>( nCols );
   miSizerProp = options.proportion;
   // PRL:  wxALL has no effect because UpdateSizersCore ignores border
   UpdateSizersCore( false, options.positionFlags | wxALL );

   if ( wxFlexGridSizer *pSizer = wxDynamicCast(mpState -> mpSizer, wxFlexGridSizer) ) {
      for (unsigned ii : options.stretchyColumns)
         pSizer->AddGrowableCol( ii, 1 );
      for (unsigned ii : options.stretchyRows)
         pSizer->AddGrowableRow( ii, 1 );
   }
}

void ShuttleGuiBase::EndMultiColumn()
{
   mpState -> PopSizer();
}

//-----------------------------------------------------------------------//

/// This function must be within a StartRadioButtonGroup - EndRadioButtonGroup pair.
void ShuttleGuiBase::AddRadioButton()
{
   wxASSERT( mRadioCount >= 0); // Did you remember to use StartRadioButtonGroup() ?

   TranslatableLabel label;
   wxString Temp;
   if (mRadioCount >= 0 && mRadioCount < (int)mRadioLabels.size() )
      label = mRadioLabels[ mRadioCount ];

   // In what follows, WrappedRef is used in read only mode, but we
   // don't have a 'read-only' version, so we copy to deal with the constness.
   wxASSERT( !label.empty() ); // More buttons than values?

   mRadioCount++;

   DoAddRadioButton( label, (mRadioCount==1) ? wxRB_GROUP : 0 );
}

/// Call this before AddRadioButton calls.
void ShuttleGuiBase::StartRadioButtonGroup()
{
   mpState -> mRadioButtons =
      std::make_shared< ShuttleGuiState::RadioButtonList >();
   mRadioCount = 0;
   mRadioItem = mItem;
}

/// Call this before any AddRadioButton calls.
void ShuttleGuiBase::StartRadioButtonGroup( LabelSetting &Setting )
{
   mRadioLabels = Setting.GetLabels();
   mRadioValues = Setting.GetValues();

   // Now actually start the radio button group.
   mpRadioSetting = &Setting;
   mRadioCount = 0;

   mpState -> mRadioButtons =
      std::make_shared< DialogDefinition::IntValidator::RadioButtonList >();

   mRadioItem = mItem;
}

/// Call this after any AddRadioButton calls.
/// It's generic too.  We don't need type-specific ones.
void ShuttleGuiBase::EndRadioButtonGroup()
{
   // too few buttons?
   wxASSERT( !mpState -> mRadioButtons ||
      mRadioCount == mpState -> mRadioButtons->size() );

   mpRadioSetting = nullptr;
   mRadioCount = -1; // So we detect a problem.
   mRadioLabels.clear();
   mRadioValues.clear();
   mpState -> mRadioButtons.reset();
   mRadioItem = {};
}

//------------------------------------------------------------------//

// We're now into ShuttleGuiBase sizer and misc functions.

/// The Ids increment as we add NEW controls.
/// However, the user can force the id manually, for example
/// if they need a specific Id for a button, and then let it
/// resume normal numbering later.
/// UseUpId() sets miId to the next Id, either using the
/// user specicfied one, or resuming the sequence.
void ShuttleGuiBase::UseUpId()
{
   if( miIdSetByUser > 0)
   {
      miId = miIdSetByUser;
      miIdSetByUser = -1;
      return;
   }
   miId = mpState -> miIdNext++;
}

void ShuttleGuiBase::SetProportions( int Default )
{
   if( miPropSetByUser >=0 )
   {
      miProp = miPropSetByUser;
      miPropSetByUser =-1;
      return;
   }
   miProp = Default;
}

namespace {
template< typename Target,
   typename Adaptor = DialogDefinition::SettingAdaptor< Target > >
inline bool VisitPref(
   PreferenceVisitor &visitor, wxWindow *pWind )
{
   const auto pValidator =
      dynamic_cast< DialogDefinition::AdaptingValidatorBase< Target > * >(
         pWind->GetValidator() );
   if ( pValidator ) {
      auto pAdaptor = dynamic_cast< Adaptor * >( &pValidator->GetAdaptor() );
      if ( pAdaptor ) {
         visitor.Visit( pWind->GetLabel(), pAdaptor->GetSetting() );
         return true;
      }
   }
   return false;
}
}

void ShuttleGuiBase::CheckEventType(
   const DialogDefinition::BaseItem &item,
   std::initializer_list<wxEventType> types )
{
   if ( item.mAction || item.mValidatorSetter ) {
      if ( item.mEventType ) {
         // Require the explicitly given event type to be one of the preferred
         // kinds
         auto begin = types.begin(), end = types.end(),
            iter = std::find( begin, end, item.mEventType );
         if ( iter == end ) {
            wxASSERT( false );
            item.mEventType = 0;
            //item.mAction = nullptr;
            //item.mpSink = nullptr;
         }
      }
      else if ( types.size() > 0 )
         // Supply the preferred event type
         item.mEventType = *types.begin();
      else
         wxASSERT( false );
   }
}

void ShuttleGuiBase::ApplyText( const DialogDefinition::ControlText &text,
   wxWindow *pWind )
{
   if ( !text.mToolTip.empty() )
      pWind->SetToolTip( text.mToolTip.Translation() );

   if ( !text.mName.empty() ) {
      // This affects the audible screen-reader name
      pWind->SetName( text.mName.Translation() );
#if 1//ndef __WXMAC__
      if (auto pButton = dynamic_cast< wxBitmapButton* >( pWind ))
         pButton->SetLabel(  text.mName.Translation() );
#endif
   }

   if ( !text.mLabel.empty() ) {
      // Takes precedence over any name specification,
      // for the (visible) label
      pWind->SetLabel( text.mLabel.Translation() );
   }

   if ( !text.mSuffix.empty() )
      pWind->SetName(
         pWind->GetName() + " " + text.mSuffix.Translation() );
}

void ShuttleGuiBase::ApplyItem( int step,
   const DialogDefinition::BaseItem &item,
   const std::shared_ptr< DialogDefinition::ValidationState > &pState,
   wxWindow *pWind, wxWindow *pDlg,
   PreferenceVisitor *pVisitor )
{
   if ( step == 0 ) {
      // Do these steps before adding the window to the sizer
      if( item.mUseBestSize )
         pWind->SetMinSize( pWind->GetBestSize() );
      else if( item.mHasMinSize )
         pWind->SetMinSize( item.mMinSize );

      if ( item.mWindowSize != wxDefaultSize )
         pWind->SetSize( item.mWindowSize );
   }
   else if ( step == 1 ) {
      // Apply certain other optional window attributes here

      if ( item.mAction || item.mValidatorSetter || item.mComputedText ) {
         if ( !item.mAction ) {
            auto action = item.mAction;
            pDlg->Bind(
               item.mEventType,
               [pWind, action, pDlg]( wxCommandEvent& ){
                  using namespace DialogDefinition;
                  auto pValidator = pWind->GetValidator();
                  if ( pValidator ) {
                     if ( !pValidator->TransferFromWindow() )
                        return;
                  }
                  if ( action )
                     action();
                  if ( pValidator || action ) {
                     // After action may have recalculated variables,
                     // update other controls
                     pDlg->TransferDataToWindow();
                  }
               },
               pWind->GetId() );
         }
         else
            wxASSERT( false );
      }

      if ( item.mValidatorSetter )
         item.mValidatorSetter( pState )( pWind );

         if ( pVisitor ) {
            // Detect set-up of shuttlings into preferences
            auto &visitor = *pVisitor;
            VisitPref< bool >( visitor, pWind ) ||
            VisitPref< int,
               DialogDefinition::ChoiceSettingAdaptor >( visitor, pWind ) ||
            VisitPref< int >( visitor, pWind ) ||
            VisitPref< double >( visitor, pWind ) ||
            VisitPref< wxString >( visitor, pWind );
         }

      if ( item.mComputedText ) {
         // Decorate the validator (if there is one) with a text updater
         // First copy any previous validator
         std::unique_ptr< wxObject > pObject{ pWind->GetValidator()->Clone() };

         // Defensive dynamic check that this wxObject really is a wxValidator
         if ( pObject && !dynamic_cast<wxValidator*>(pObject.get()) ) {
            wxASSERT( false );
            pObject.reset();
         }

         // Now install the decorator
         auto computedText = item.mComputedText;
         ValidatorDecorator newValidator{
            [computedText]( wxWindow *pWind ){
               ApplyText( computedText(), pWind );
            },
            std::unique_ptr< wxValidator >{
               static_cast< wxValidator* >( pObject.release() ) }
         };
         pWind->SetValidator( newValidator );
      }

      ApplyText( item.mText, pWind );

#if wxUSE_ACCESSIBILITY
         if ( item.mAccessibleFactory )
            pWind->SetAccessible( item.mAccessibleFactory( pWind ) );
#endif

      if (item.mFocused)
         pWind->SetFocus();

      if (item.mDefault) {
         if ( auto pButton = dynamic_cast< wxButton *>( pWind ) )
            pButton->SetDefault();
         else
            wxASSERT( false );
      }

      if (item.mDisabled)
         pWind->Enable( false );

      for (auto &pair : item.mRootConnections)
         pWind->Connect( pair.first, pair.second, nullptr, pDlg );

      if ( item.mEnableTest || item.mShowTest ) {
         auto enable = item.mEnableTest;
         auto show = item.mShowTest;
         pWind->Bind( wxEVT_UPDATE_UI, [enable, show]( wxUpdateUIEvent &evt ){
            auto enabled = !enable || enable();
            auto shown = !show || show();

            // Try not to trap focus in the control we are about to disable
            if ( !enabled || !shown )
               if ( auto pWindow = dynamic_cast<wxWindow*>( evt.GetEventObject() ) ) {
                  if ( pWindow == wxWindow::FindFocus() ) {
                     auto pOrigWindow = pWindow;
                     while( pWindow->Navigate() &&
                       pOrigWindow != ( pWindow = wxWindow::FindFocus() ) &&
                       pWindow &&
                       !( pWindow->IsEnabled() && pWindow->IsShown() ) )
                        ;
                  }
               }

            evt.Enable( enabled );
            evt.Show( shown );
         } );
      }
   }
}


void ShuttleGuiBase::UpdateSizersCore(bool bPrepend, int Flags, bool prompt)
{
   if( mpWind && mpState -> mpParent )
   {
      int useFlags = Flags;

      if ( !prompt && mItem.mWindowPositionFlags )
         // override the given Flags
         useFlags = mItem.mWindowPositionFlags;

      if (!prompt)
         ApplyItem( 0, mItem,
            mpState -> mpValidationState,
            mpWind, mpState -> mpDlg,
            mpState -> mpVisitor.get() );

      if( mpState -> mpSizer){
         if( bPrepend )
         {
            mpState -> mpSizer->Prepend(mpWind, miProp, useFlags, mpState -> miBorder);
         }
         else
         {
            mpState -> mpSizer->Add(mpWind, miProp, useFlags, mpState -> miBorder);
         }
      }

      if (!prompt) {
         ApplyItem( 1, mItem,
            mpState -> mpValidationState,
            mpWind, mpState -> mpDlg,
            mpState -> mpVisitor.get() );
         // Reset to defaults
         mItem = {};
      }
   }

   if( mpSubSizer && mpState -> mpSizer )
   {
      // When adding sizers into sizers, don't add a border.
      // unless it's a static box sizer.
      wxSizer *const pSubSizer = mpSubSizer.get();
      if (wxDynamicCast(pSubSizer, wxStaticBoxSizer))
      {
         mpState -> mpSizer->Add( mpSubSizer.release(), miSizerProp, Flags , mpState -> miBorder);
      }
      else
      {
         mpState -> mpSizer->Add( mpSubSizer.release(), miSizerProp, Flags ,0);//miBorder);
      }
      mpState -> mpSizer = pSubSizer;
      mpState -> PushSizer();
   }

   mpWind = NULL;
   miProp = 0;
   miSizerProp =0;
}

// Sizer is added into parent sizer, and will expand/shrink.
void ShuttleGuiBase::UpdateSizers()
{  UpdateSizersCore( false, wxEXPAND | wxALL );}

// Sizer is added into parent sizer, centred
void ShuttleGuiBase::UpdateSizersC()
{  UpdateSizersCore( false, wxALIGN_CENTRE | wxALL );}

// Sizer is added into parent sizer, and will expand/shrink.
// added to start of sizer list.
void ShuttleGuiBase::UpdateSizersAtStart()
{  UpdateSizersCore( true, wxEXPAND | wxALL );}

void ShuttleGuiState::PopSizer()
{
   mSizerDepth--;
   wxASSERT( mSizerDepth >=0 );
   mpSizer = pSizerStack[ mSizerDepth ];
}

void ShuttleGuiState::PushSizer()
{
   mSizerDepth++;
   wxASSERT( mSizerDepth < nMaxNestedSizers );
   pSizerStack[ mSizerDepth ] = mpSizer;
}

long ShuttleGuiBase::GetStyle( long style )
{
   if( mItem.miStyle )
      style = mItem.miStyle;
   mItem.miStyle = 0;
   return style;
}

// A rarely used helper function that sets a pointer
// ONLY if the value it is to be set to is non NULL.
void SetIfCreated( wxChoice * &Var, wxChoice * Val )
{
   if( Val != NULL )
      Var = Val;
};
void SetIfCreated( wxTextCtrl * &Var, wxTextCtrl * Val )
{
   if( Val != NULL )
      Var = Val;
};
void SetIfCreated( wxStaticText *&Var, wxStaticText * Val )
{
   if( Val != NULL )
      Var = Val;
};

#ifdef EXPERIMENTAL_TRACK_PANEL
// Additional includes down here, to make it easier to split this into
// two files at some later date.
#include "../extnpanel-src/GuiWaveTrack.h"
#endif

static
std::unique_ptr<wxSizer> CreateStdButtonSizer(
   wxWindow *pDlg, wxWindow *parent,
   const std::shared_ptr< DialogDefinition::ValidationState > &pValidationState,
   long buttons, DialogDefinition::Items items, wxWindow *extra,
   DialogDefinition::Item extraItem )
{
   wxASSERT(parent != NULL); // To justify safenew

   for ( const auto &item : items )
      buttons |= item.mStandardButton;

   bool givenDefault = std::any_of( items.begin(), items.end(),
      std::mem_fn( &DialogDefinition::Item::mDefault ) );

   int margin;
   {
#if defined(__WXMAC__)
      margin = 12;
#elif defined(__WXGTK20__)
      margin = 12;
#elif defined(__WXMSW__)
      wxButton b(parent, 0, wxEmptyString);
      margin = b.ConvertDialogToPixels(wxSize(2, 0)).x;
#else
      wxButton b(parent, 0, wxEmptyString);
      margin = b->ConvertDialogToPixels(wxSize(4, 0)).x;
#endif
   }

   wxButton *b = NULL;
   auto bs = std::make_unique<wxStdDialogButtonSizer>();
   
   const auto makeButton =
   [parent]( wxWindowID id, const wxString label = {} ) {
      auto result = safenew wxButton( parent, id, label );
      result->SetName( result->GetLabel() );
      return result;
   };

   auto addButton =
   [&]( StandardButtonID id, wxButton *pButton, int insertAt = 0 ) {
      // Find any matching item
      auto pred = [id]( decltype( *items.begin() ) &item ){
         return item.mStandardButton == id; };
      auto end = items.end(),
         iter = std::find_if( items.begin(), end, pred );
      // If found, it should be unique
      wxASSERT( iter == end ||
         end == std::find_if( iter + 1, end, pred ) );

      if (iter != end)
         ShuttleGuiBase::ApplyItem( 0, *iter,
            pValidationState,
            pButton, pDlg );
      if ( insertAt == 0 )
         bs->AddButton( pButton );
      else if ( insertAt == -1 )
         bs->Add( pButton, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT, margin );
      else
         bs->Insert( insertAt,
            pButton, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT, margin );
      if (iter != end) {
         ShuttleGuiBase::CheckEventType( *iter, { wxEVT_BUTTON } );
         ShuttleGuiBase::ApplyItem( 1, *iter,
            pValidationState,
            pButton, pDlg );
      }
   };

   if( buttons & eOkButton )
   {
      b = makeButton( wxID_OK );
      if( !givenDefault )
         b->SetDefault();
      addButton( eOkButton, b );
   }

   if( buttons & eCancelButton )
   {
      addButton( eCancelButton, makeButton( wxID_CANCEL ) );
   }

   if( buttons & eYesButton )
   {
      b = makeButton( wxID_YES );
      if( !givenDefault )
         b->SetDefault();
      addButton( eYesButton, b );
   }

   if( buttons & eNoButton )
   {
      addButton( eNoButton, makeButton( wxID_NO ) );
   }

   if( buttons & eApplyButton )
   {
      b = makeButton( wxID_APPLY );
      b->SetDefault();
      if( !givenDefault )
         b->SetDefault();
      addButton( eApplyButton, b );
   }

   if( buttons & eCloseButton )
   {
      addButton( eCloseButton,
         makeButton( wxID_CANCEL, XO("&Close").Translation() ) );
   }

#if defined(__WXMSW__)
   // See below for explanation
   if( buttons & eHelpButton )
   {
      // Replace standard Help button with smaller icon button.
      // bs->AddButton(safenew wxButton(parent, wxID_HELP));
      b = safenew wxBitmapButton(parent, wxID_HELP, theTheme.Bitmap( bmpHelpIcon ));
      b->SetToolTip( XO("Help").Translation() );
      b->SetLabel(XO("Help").Translation());       // for screen readers
      b->SetName( b->GetLabel() );
      addButton( eHelpButton, b );
   }
#endif

   if (buttons & ePreviewButton)
   {
      addButton( ePreviewButton,
         makeButton( ePreviewID, XO("&Preview").Translation() ),
         -1 );
   }
   if (buttons & ePreviewDryButton) {
      addButton( ePreviewDryButton,
         makeButton( ePreviewDryID, XO("Dry Previe&w").Translation() ),
         -1 );
      bs->Add( 20, 0 );
   }

   if( buttons & eSettingsButton )
   {
      addButton( eSettingsButton,
         makeButton( eSettingsID, XO("&Settings").Translation() ),
         -1 );
      bs->Add( 20, 0 );
   }

   if( extra )
   {
      ShuttleGuiBase::ApplyItem( 0, extraItem, pValidationState,
         extra, pDlg );
      bs->Add( extra, 0, wxALIGN_CENTER | wxLEFT | wxRIGHT, margin );
      bs->Add( 40, 0 );
      ShuttleGuiBase::ApplyItem( 1, extraItem, pValidationState,
         extra, pDlg );
   }

   bs->AddStretchSpacer();
   bs->Realize();

   size_t lastLastSpacer = 0;
   size_t lastSpacer = 0;
   wxSizerItemList & list = bs->GetChildren();
   for( size_t i = 0, cnt = list.size(); i < cnt; i++ )
   {
      if( list[i]->IsSpacer() )
      {
         lastSpacer = i;
      }
      else
      {
         lastLastSpacer = lastSpacer;
      }
   }

   // Add any buttons that need to cuddle up to the right hand cluster
   // Add any buttons that need to cuddle up to the right hand cluster
   if( buttons & eDebugButton )
   {
      addButton( eDebugButton,
         makeButton( eDebugID, XO("Debu&g").Translation() ),
         ++lastLastSpacer );
   }

#if !defined(__WXMSW__)
   // Bug #2432: Couldn't find GTK guidelines, but Mac HIGs state:
   //
   //    View style 	                                          Help button position
   //    Dialog with dismissal buttons (like OK and Cancel). 	Lower-left corner, vertically aligned with the dismissal buttons.
   //    Dialog without dismissal buttons. 	                  Lower-left or lower-right corner.
   //    Preference window or pane. 	                        Lower-left or lower-right corner.
   //
   // So, we're gonna cheat a little and use the lower-right corner.
   if( buttons & eHelpButton )
   {
      // Replace standard Help button with smaller icon button.
      // bs->AddButton(safenew wxButton(parent, wxID_HELP));
      b = safenew wxBitmapButton(parent, wxID_HELP, theTheme.Bitmap( bmpHelpIcon ));
      b->SetToolTip( XO("Help").Translation() );
      b->SetLabel(XO("Help").Translation());       // for screen readers
      b->SetName( b->GetLabel() );
      bs->Add( b, 0, wxALIGN_CENTER );
   }
#endif

   auto s = std::make_unique<wxBoxSizer>( wxVERTICAL );
   s->Add( bs.release(), 1, wxEXPAND | wxALL, 7 );
   s->Add( 0, 3 );   // a little extra space

   return std::unique_ptr<wxSizer>{ s.release() };
}

void ShuttleGuiBase::AddStandardButtons(
   long buttons, DialogDefinition::Items items, wxWindow *extra,
   DialogDefinition::Item extraItem,
   int border)
{
   StartVerticalLay( false, border );

   miSizerProp = false;
   mpSubSizer = CreateStdButtonSizer(
      mpState -> mpDlg, mpState -> mpParent,
      mpState -> mpValidationState,
      buttons, items, extra, extraItem );
   UpdateSizers();
   mpState -> PopSizer(); // to complement UpdateSizers

   EndVerticalLay();
}

wxSizerItem * ShuttleGuiBase::AddSpace( int width, int height, int prop )
{
//   SetProportions(0);
  // return mpSizer->Add( width, height, miProp);

   return mpState -> mpSizer->Add( width, height, prop );
}

void ShuttleGuiBase::SetMinSize( wxWindow *window, const TranslatableStrings & items )
{
   SetMinSize( window,
      transform_container<wxArrayStringEx>(
         items, std::mem_fn( &TranslatableString::Translation ) ) );
}

void ShuttleGuiBase::SetMinSize( wxWindow *window, const wxArrayStringEx & items )
{
   int maxw = 0;

   for( size_t i = 0; i < items.size(); i++ )
   {
      int x;
      int y;

      window->GetTextExtent(items[i], &x, &y );
      if( x > maxw )
      {
         maxw = x;
      }
   }

   // Would be nice to know the sizes of the button and borders, but this is
   // the best we can do for now.
#if defined(__WXMAC__)
   maxw += 50;
#elif defined(__WXMSW__)
   maxw += 50;
#elif defined(__WXGTK__)
   maxw += 50;
#else
   maxw += 50;
#endif

   window->SetMinSize( { maxw, -1 } );
}

/*
void ShuttleGuiBase::SetMinSize( wxWindow *window, const std::vector<int> & items )
{
   wxArrayStringEx strs;

   for( size_t i = 0; i < items.size(); i++ )
   {
      strs.Add( wxString::Format( L"%d", items[i] ) );
   }

   SetMinSize( window, strs );
}
*/

TranslatableStrings Msgids(
   const EnumValueSymbol strings[], size_t nStrings)
{
   return transform_range<TranslatableStrings>(
      strings, strings + nStrings,
      std::mem_fn( &EnumValueSymbol::Msgid )
   );
}

TranslatableStrings Msgids( const std::vector<EnumValueSymbol> &strings )
{
   return Msgids( strings.data(), strings.size() );
}
